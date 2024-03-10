package main

import (
	"bufio"
	"context"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"github.com/redis/go-redis/v9"
	"io"
	"math/rand"
	"net"
	"sync"
	"time"
)

var letters = []rune("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
var rdb *redis.Client
var ctx = context.Background()

func randSeq(n int) string {
	b := make([]rune, n)
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	return string(b)
}

type Channel struct {
	clients        map[*Client]int
	Name           string
	Table          string
	State          string
	clientsMux     sync.Mutex
	tableMux       sync.Mutex
	stateMux       sync.Mutex
	ClientPassword string
}

type Client struct {
	Conn    net.Conn
	IsJudge bool
	Cnl     *Channel
}

var channels = make(map[string]*Channel)
var mutex sync.Mutex
var clientMap = make(map[string]string)
var clientMapMutex sync.Mutex

func writeChannelDataToDB(ch *Channel) {
	if j, err := json.Marshal(ch); err == nil {
		s := string(j)
		rdb.Set(ctx, "bingosync:"+ch.Name, s, time.Hour*8)
	}
}

func loadAllChannelsFromDB() {
	keys, err := rdb.Keys(ctx, "bingosync:*").Result()
	if err != nil {
		return
	}
	for _, k := range keys {
		if v, err := rdb.Get(ctx, k).Result(); err == nil {
			c := &Channel{}
			if err := json.Unmarshal([]byte(v), c); err == nil {
				mutex.Lock()
				c.clients = make(map[*Client]int)
				channels[k[10:]] = c
				mutex.Unlock()
				clientMapMutex.Lock()
				clientMap[c.ClientPassword] = k[10:]
				clientMapMutex.Unlock()
			}
		}
	}
}

func getChannel(c string) *Channel {
	mutex.Lock()
	defer mutex.Unlock()
	if r, ok := channels[c]; ok {
		return r
	}
	cnl := &Channel{}
	cnl.clients = make(map[*Client]int)
	clientMapMutex.Lock()
	defer clientMapMutex.Unlock()
	for {
		rndPwd := randSeq(8)
		if _, ok := clientMap[rndPwd]; ok {
			continue
		}
		clientMap[rndPwd] = c
		cnl.ClientPassword = rndPwd
		break
	}
	cnl.Name = c
	channels[c] = cnl
	writeChannelDataToDB(cnl)
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
	c.clientsMux.Lock()
	defer c.clientsMux.Unlock()
	for k := range c.clients {
		if k.IsJudge {
			continue
		}
		_, _ = k.Conn.Write(s)
	}
}

func (c *Client) EnterChannel(isJudge bool, channel string) {
	randChannel := false
	if channel == "" {
		if isJudge {
			mutex.Lock()
			for {
				channel = randSeq(8)
				if _, ok := channels[channel]; ok {
					continue
				}
				break
			}
			mutex.Unlock()
			randChannel = true
		} else {
			_ = c.Conn.Close()
		}
	}
	c.IsJudge = isJudge
	c.Cnl = getChannel(channel)
	if c.Cnl == nil {
		return
	}
	c.Cnl.clientsMux.Lock()
	c.Cnl.clients[c] = 1
	c.Cnl.clientsMux.Unlock()
	if c.IsJudge {
		if randChannel {
			sendMsg(c.Conn, 'J', channel)
		}
		sendMsg(c.Conn, 'C', c.Cnl.ClientPassword)
	}
}

func (c *Client) LeaveChannel() {
	if c.Cnl == nil {
		return
	}
	c.Cnl.clientsMux.Lock()
	if _, ok := c.Cnl.clients[c]; ok {
		delete(c.Cnl.clients, c)
	}
	c.Cnl.clientsMux.Unlock()
	c.Cnl = nil
}

func (c *Client) FetchOrUpdateTable(table string) {
	if c.Cnl == nil {
		return
	}
	c.Cnl.tableMux.Lock()
	defer c.Cnl.tableMux.Unlock()
	if c.IsJudge {
		if c.Cnl.Table != table {
			c.Cnl.Table = table
			writeChannelDataToDB(c.Cnl)
			broadcastToPlayers(c.Cnl, 'T', table)
		}
	} else {
		if c.Cnl != nil {
			sendMsg(c.Conn, 'T', c.Cnl.Table)
		}
	}
}

func (c *Client) FetchOrUpdateState(state string) {
	if c.Cnl == nil {
		return
	}
	c.Cnl.stateMux.Lock()
	defer c.Cnl.stateMux.Unlock()
	if c.IsJudge {
		if c.Cnl.State != state {
			c.Cnl.State = state
			writeChannelDataToDB(c.Cnl)
			broadcastToPlayers(c.Cnl, 'S', state)
		}
	} else {
		if c.Cnl != nil {
			sendMsg(c.Conn, 'S', c.Cnl.State)
		}
	}
}

func handleMsg(c *Client, t byte, m string) {
	switch t {
	case 'C':
		clientMapMutex.Lock()
		v, ok := clientMap[m]
		clientMapMutex.Unlock()
		if ok {
			c.EnterChannel(false, v)
		} else {
			_ = c.Conn.Close()
		}
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
	rdb = redis.NewClient(&redis.Options{
		Addr:     "localhost:6379",
		Password: "",
		DB:       0,
	})
	loadAllChannelsFromDB()
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
