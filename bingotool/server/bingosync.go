package main

import (
	"bufio"
	"context"
	"encoding/binary"
	"encoding/json"
	"fmt"
	"io"
	"math/rand"
	"net"
	"sync"
	"time"

	"github.com/redis/go-redis/v9"
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
	clients        map[*Client]int `json:"-"`
	Name           string          `json:"name"`
	Table          string          `json:"table"`
	State          string          `json:"state"`
	Config         string          `json:"config"`
	clientsMux     sync.Mutex      `json:"-"`
	tableMux       sync.Mutex      `json:"-"`
	stateMux       sync.Mutex      `json:"-"`
	configMux      sync.Mutex      `json:"-"`
	ClientPassword string          `json:"client_password"`
}

type Client struct {
	Conn    net.Conn
	IsJudge bool
	Chan    *Channel
	Pending chan []byte
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

func simpleCrc(data []byte) byte {
	crc := byte(0xFF)
	for _, b := range data {
		crc ^= b - 0x25
	}
	return crc
}

func sendMsg(c *Client, t byte, m string) {
	l := 1 + len(m)
	if l > 0xFFFFFF {
		return
	}
	s := make([]byte, 4+l)
	binary.LittleEndian.PutUint32(s[0:4], uint32(l))
	s[4] = t
	copy(s[5:], m[:])
	s[3] = simpleCrc(s[4:])
	c.Pending <- s
}

func broadcastToPlayers(c *Channel, t byte, m string) {
	l := 1 + len(m)
	s := make([]byte, 4+l)
	if l > 0xFFFFFF {
		return
	}
	binary.LittleEndian.PutUint32(s[0:4], uint32(l))
	s[4] = t
	copy(s[5:], m[:])
	s[3] = simpleCrc(s[4:])
	c.clientsMux.Lock()
	defer c.clientsMux.Unlock()
	for k := range c.clients {
		if k.IsJudge {
			continue
		}
		k.Pending <- s
	}
}

func (c *Client) EnterChannel(isJudge bool, channel string) {
	randChannel := false
	if channel == "" {
		if isJudge {
			mutex.Lock()
			channel = randSeq(8)
			for _, ok := channels[channel]; ok; channel = randSeq(8) {
			}
			mutex.Unlock()
			randChannel = true
		} else {
			_ = c.Conn.Close()
			return
		}
	}
	c.IsJudge = isJudge
	c.Chan = getChannel(channel)
	if c.Chan == nil {
		return
	}
	c.Chan.clientsMux.Lock()
	c.Chan.clients[c] = 1
	c.Chan.clientsMux.Unlock()
	if c.IsJudge {
		if randChannel {
			sendMsg(c, 'J', channel)
		}
		sendMsg(c, 'C', c.Chan.ClientPassword)
	}
}

func (c *Client) LeaveChannel() {
	close(c.Pending)
	ch := c.Chan
	if ch == nil {
		return
	}
	c.Chan = nil
	ch.clientsMux.Lock()
	delete(ch.clients, c)
	ch.clientsMux.Unlock()
}

func (c *Client) FetchOrUpdateConfig(config string) {
	if c.Chan == nil {
		return
	}
	c.Chan.configMux.Lock()
	defer c.Chan.configMux.Unlock()
	if c.IsJudge {
		if c.Chan.Config != config {
			c.Chan.Config = config
			writeChannelDataToDB(c.Chan)
			broadcastToPlayers(c.Chan, 'G', config)
		}
	} else {
		if c.Chan != nil {
			sendMsg(c, 'G', c.Chan.Config)
		}
	}
}

func (c *Client) FetchOrUpdateTable(table string) {
	if c.Chan == nil {
		return
	}
	c.Chan.tableMux.Lock()
	defer c.Chan.tableMux.Unlock()
	if c.IsJudge {
		if c.Chan.Table != table {
			c.Chan.Table = table
			writeChannelDataToDB(c.Chan)
			broadcastToPlayers(c.Chan, 'T', table)
		}
	} else {
		if c.Chan != nil {
			sendMsg(c, 'T', c.Chan.Table)
		}
	}
}

func (c *Client) FetchOrUpdateState(state string) {
	if c.Chan == nil {
		return
	}
	c.Chan.stateMux.Lock()
	defer c.Chan.stateMux.Unlock()
	if c.IsJudge {
		if c.Chan.State != state {
			c.Chan.State = state
			writeChannelDataToDB(c.Chan)
			broadcastToPlayers(c.Chan, 'S', state)
		}
	} else {
		if c.Chan != nil {
			sendMsg(c, 'S', c.Chan.State)
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
	case 'G':
		c.FetchOrUpdateConfig(m)
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
		ilen := int(l) & 0xFFFFFF
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
		if simpleCrc(msg) != byte(l>>24) {
			fmt.Println("Checksum mismatch, drop and close connection!")
			break
		}
		handleMsg(c, msg[0], string(msg[1:]))
	}
	c.LeaveChannel()
}

func processWrite(c *Client) {
	for data := range c.Pending {
		_, _ = c.Conn.Write(data)
	}
}

func main() {
	listen, err := net.Listen("tcp", "0.0.0.0:8309")
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
		client := &Client{
			Pending: make(chan []byte, 4),
		}
		go processRead(client, conn)
		go processWrite(client)
	}
}
