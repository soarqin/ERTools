package main

import (
	"bufio"
	"encoding/binary"
	"fmt"
	"io"
	"net"
	"sync"
)

type Channel struct {
	Clients map[*Client]int
	Table string
	State string
	mux sync.Mutex
}

type Client struct {
	Conn net.Conn
	IsJudge bool
	Cnl *Channel
}

var channels = make(map[string]*Channel)
var mutex sync.Mutex

func getChannel(c string)*Channel {
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
	s := make([]byte, 4 + l)
	binary.LittleEndian.PutUint32(s[0:4], uint32(l))
	s[4] = t
	copy(s[5:], m[:])
	conn.Write(s)
}

func broadcastToPlayers(c *Channel, t byte, m string) {
	l := 1 + len(m)
	s := make([]byte, 4 + l)
	binary.LittleEndian.PutUint32(s[0:4], uint32(l))
	s[4] = t
	copy(s[5:], m[:])
	c.mux.Lock()
	defer c.mux.Unlock()
	for k, _ := range(c.Clients) {
		if k.IsJudge {
			continue
		}
		k.Conn.Write(s)
	}
}

func handleMsg(c *Client, t byte, m string) {
	switch t {
	case 'C':
		c.IsJudge = false
		c.Cnl = getChannel(m)
		c.Cnl.mux.Lock()
		c.Cnl.Clients[c] = 1
		c.Cnl.mux.Unlock()
	case 'J':
		c.IsJudge = true
		c.Cnl = getChannel(m)
		c.Cnl.mux.Lock()
		c.Cnl.Clients[c] = 1
		c.Cnl.mux.Unlock()
	case 'T':
		if c.IsJudge {
			if c.Cnl.Table != m {
				c.Cnl.Table = m
				broadcastToPlayers(c.Cnl, 'T', m)
			}
		} else {
			if c.Cnl != nil {
				sendMsg(c.Conn, 'T', c.Cnl.Table)
			}
		}
	case 'S':
		if c.IsJudge {
			if c.Cnl.State != m {
				c.Cnl.State = m
				broadcastToPlayers(c.Cnl, 'S', m)
			}
		} else {
			if c.Cnl != nil {
				sendMsg(c.Conn, 'S', c.Cnl.State)
			}
		}
	}
}

func processRead(c *Client, conn net.Conn) {
	defer conn.Close()
	c.Conn = conn
	reader := bufio.NewReader(conn)
	for {
		var l uint32
		err := binary.Read(reader, binary.LittleEndian, &l)
		if err != nil {
			fmt.Println("read from client failed, err: ", err)
			break
		}
		len := int(l)
		msg := make([]byte, len)
		n, err := io.ReadFull(reader, msg)
		if err != nil {
			fmt.Println("read from client failed, err: ", err)
			break
		}
		if n < len {
			break
		}
		handleMsg(c, byte(msg[0]), string(msg[1:]))
	}
	if c.Cnl == nil {
		return
	}
	if _, ok := c.Cnl.Clients[c]; ok {
		delete(c.Cnl.Clients, c)
	}
}

func main() {
	listen, err := net.Listen("tcp", "0.0.0.0:8307")
	if err != nil {
		fmt.Println("Listen() failed, err: ", err)
		return
	}
	for {
		conn, err := listen.Accept()
		if err != nil {
			fmt.Println("Accept() failed, err: ", err)
			continue
		}
		client := &Client {}
		go processRead(client, conn)
	}
}
