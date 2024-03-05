package main

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"sync"

	"github.com/redis/go-redis/v9"
)

type Channel struct {
	Clients map[*Client]int
	Table   string
	State   string
	mux     sync.Mutex
}

type Client struct {
	Conn    net.Conn
	IsJudge bool
	Cnl     *Channel
}

var channels = make(map[string]*Channel)
var mutex sync.Mutex

func getChannel(c string) *Channel {
	mutex.Lock()
	defer mutex.Unlock()
	if r, ok := channels[c]; ok {
		return r
	}
	cnl := &Channel{}
	cnl.Clients = make(map[*Client]int)
	channels[c] = cnl
	return cnl
}

func sendMsg(conn net.Conn, t byte, m string) {
	l := 1 + len(m)
	s := make([]byte, 4+l)
	binary.LittleEndian.PutUint32(s[0:4], uint32(l))
	s[4] = t
	copy(s[5:], m[:])
	_, _ = conn.Write(s)
}

func broadcastToPlayers(c *Channel, t byte, m string) {
	l := 1 + len(m)
	s := make([]byte, 4+l)
	binary.LittleEndian.PutUint32(s[0:4], uint32(l))
	s[4] = t
	copy(s[5:], m[:])
	c.mux.Lock()
	defer c.mux.Unlock()
	for k := range c.Clients {
		if k.IsJudge {
			continue
		}
		_, _ = k.Conn.Write(s)
	}
}

func (c *Client) EnterChannel(isJudge bool, channel string) {
	c.IsJudge = isJudge
	c.Cnl = getChannel(channel)
	c.Cnl.mux.Lock()
	c.Cnl.Clients[c] = 1
	c.Cnl.mux.Unlock()
}

func (c *Client) LeaveChannel() {
	if c.Cnl == nil {
		return
	}
	c.Cnl.mux.Lock()
	if _, ok := c.Cnl.Clients[c]; ok {
		delete(c.Cnl.Clients, c)
	}
	c.Cnl.mux.Unlock()
	c.Cnl = nil
}

func (c *Client) FetchOrUpdateTable(table string) {
	c.Cnl.mux.Lock()
	if c.IsJudge {
		if c.Cnl.Table != table {
			c.Cnl.Table = table
			broadcastToPlayers(c.Cnl, 'T', table)
		}
	} else {
		if c.Cnl != nil {
			sendMsg(c.Conn, 'T', c.Cnl.Table)
		}
	}
	c.Cnl.mux.Unlock()
}

func (c *Client) FetchOrUpdateState(state string) {
	c.Cnl.mux.Lock()
	if c.IsJudge {
		if c.Cnl.State != state {
			c.Cnl.State = state
			broadcastToPlayers(c.Cnl, 'S', state)
		}
	} else {
		if c.Cnl != nil {
			sendMsg(c.Conn, 'S', c.Cnl.State)
		}
	}
	c.Cnl.mux.Unlock()
}

func handleMsg(c *Client, t byte, m string) {
	switch t {
	case 'C':
		c.EnterChannel(false, m)
	case 'J':
		c.EnterChannel(true, m)
	case 'T':
		c.FetchOrUpdateTable(m)
	case 'S':
		c.FetchOrUpdateState(m)
	}
}

func processRead(c *Client, conn net.Conn) {
	defer func() {
		_ = conn.Close()
	}()
	c.Conn = conn
	reader := bufio.NewReader(conn)
	for {
		var l uint32
		err := binary.Read(reader, binary.LittleEndian, &l)
		if err != nil {
			if err != io.EOF {
				fmt.Println("read from client failed, err: ", err)
			}
			break
		}
		ilen := int(l)
		if ilen >= 0x40000 {
			fmt.Println("Suspecious packet got, drop and close connection!")
			break
		}
		msg := make([]byte, ilen)
		n, err := io.ReadFull(reader, msg)
		if err != nil {
			if err != io.EOF {
				fmt.Println("read from client failed, err: ", err)
			}
			break
		}
		if n < ilen {
			break
		}
		handleMsg(c, msg[0], string(msg[1:]))
	}
	c.LeaveChannel()
}

func main() {
	listen, err := net.Listen("tcp", "0.0.0.0:8307")
	if err != nil {
		fmt.Println("Listen() failed, err: ", err)
		return
	}
	rdb := redis.NewClient(&redis.Options{
		Addr:     "localhost:6379",
		Password: "",
		DB:       0,
	})
	defer func() {
		_ = rdb.Close()
	}()
	for {
		conn, err := listen.Accept()
		if err != nil {
			fmt.Println("Accept() failed, err: ", err)
			continue
		}
		client := &Client{}
		go processRead(client, conn)
	}
}
