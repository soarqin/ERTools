#include "sync.h"

#include <uv.h>
#include <fstream>
#include <list>

static int gMode = 0;
static std::string gServer = "bingosync.soar.im";
static std::string gChannel;
static std::string gPassword;

static uv_loop_t *loop = nullptr;
static uv_tcp_t *clientCtx = nullptr;
static int state = 0;
static size_t headerRead = 0;
static char header[4];
static size_t msgPos = 0;
static std::string msg;
static ChannelCallback syncCallback = nullptr;
static ConnectionCallback syncOpenCallback = nullptr;
static uv_timer_t reconnectTimer;

void read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

void doReconnect() {
    uv_timer_init(loop, &reconnectTimer);
    uv_timer_start(&reconnectTimer, [](uv_timer_t* handle) {
        uv_timer_stop(handle);
        syncOpen(syncOpenCallback);
    }, 10000, 0);
}

void on_connect(uv_connect_t *req, int status) {
    delete req;
    if (status < 0) {
        doReconnect();
        return;
    }
    uv_tcp_keepalive(clientCtx, 1, 60);
    state = 0;
    uv_read_start((uv_stream_t*)clientCtx, [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        auto *mem = new char[suggested_size];
        *buf = uv_buf_init(mem, suggested_size);
    }, read_cb);
    if (syncOpenCallback) syncOpenCallback();
}

void read_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread == 0) return;
    if (nread < 0) {
        syncClose();
        doReconnect();
        return;
    }
    auto *data = buf->base;
    while (nread > 0) {
        switch (state) {
            case 0: {
                if (headerRead + nread < 4) {
                    memcpy(header + headerRead, data, nread);
                    headerRead += nread;
                    break;
                } else {
                    auto toRead = (ssize_t)(4 - headerRead);
                    memcpy(header + headerRead, data, toRead);
                    msgPos = 0;
                    msg.resize(*(int *)header);
                    headerRead = 0;
                    state = 1;
                    data += toRead;
                    nread -= toRead;
                    if (nread == 0) break;
                }
                // fallthrough
            }
            case 1: {
                if (nread + msgPos >= msg.size()) {
                    auto toRead = (ssize_t)(msg.size() - msgPos);
                    memcpy(&msg[msgPos], data, toRead);
                    data += toRead;
                    nread -= toRead;
                    state = 0;
                    msgPos = 0;
                    switch (msg[0]) {
                        case 'C':
                            syncSetChannelPasswordForC(msg.substr(1));
                            break;
                        case 'J':
                            syncSetChannelPassword(msg.substr(1));
                            break;
                        default:
                            syncCallback(msg[0], msg.substr(1));
                            break;
                    }
                    msg.clear();
                    break;
                }
                memcpy(&msg[msgPos], data, nread);
                msgPos += nread;
                data += nread;
                nread = 0;
                break;
            }
        }
    }
    delete[] buf->base;
}

void saveState() {
    std::ofstream file("sync_state.txt");
    if (!file.is_open()) return;
    file << gMode << std::endl;
    file << gChannel << std::endl;
    file << gPassword;
    file.close();
}

void loadState() {
    std::ifstream file("sync_state.txt");
    if (!file.is_open()) return;
    std::string line;
    std::getline(file, line);
    gMode = std::stoi(line);
    std::getline(file, gChannel);
    std::getline(file, gPassword);
    file.close();
}

void syncLoadConfig() {
    std::ifstream file("data/sync.txt");
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        if (line[0] == ';') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);
        if (key == "Mode") {
            gMode = std::stoi(value);
        } else if (key == "Server") {
            gServer = value;
        } else if (key == "Channel") {
            gChannel = value;
        }
    }
    file.close();
    loadState();
}

void syncProcess() {
    if (loop != nullptr) {
        uv_run(loop, UV_RUN_NOWAIT);
    }
}

void syncSetMode(int mode) {
    gMode = mode;
    saveState();
}

int syncGetMode() {
    return gMode;
}

bool syncOpen(ConnectionCallback callback) {
    if (gMode == 0 && gPassword.empty()) return true;
    if (loop == nullptr) {
        loop = uv_default_loop();
    }
    if (clientCtx != nullptr) {
        syncClose();
    }
    clientCtx = new uv_tcp_t;
    if (uv_tcp_init(loop, clientCtx) < 0)
        return false;
    int port = 8307;
    auto h = gServer;
    auto pos = h.find(':');
    if (pos != std::string::npos) {
        port = std::stoi(h.substr(pos + 1));
        h = h.substr(0, pos);
    }
    struct addrinfo *result = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(h.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        return false;
    }
    auto *connectCtx = new uv_connect_t;
    memset(connectCtx, 0, sizeof(*connectCtx));
    syncOpenCallback = callback;
    return uv_tcp_connect(connectCtx, clientCtx, result->ai_addr, on_connect) == 0;
}

void syncClose() {
    if (clientCtx == nullptr || uv_is_closing((uv_handle_t*)clientCtx)) return;
    uv_close((uv_handle_t*)clientCtx, [](uv_handle_t* handle) {
        delete (uv_tcp_t*)handle;
        syncCallback = nullptr;
        state = 0;
        headerRead = 0;
        msgPos = 0;
        msg.clear();
    });
    clientCtx = nullptr;
}

bool syncSetChannel(ChannelCallback callback) {
    if (gMode == 0 && gPassword.empty()) return false;
    syncCallback = callback;
    return syncSendData(gMode == 0 ? 'C' : 'J', gMode == 0 ? gPassword : gChannel);
}

void syncSetChannelPassword(const std::string &password) {
    if ((gMode == 0 ? gPassword : gChannel) == password) return;
    (gMode == 0 ? gPassword : gChannel) = password;
    saveState();
}

void syncSetChannelPasswordForC(const std::string &password) {
    if (gPassword == password) return;
    gPassword = password;
    saveState();
}

const std::string &syncGetChannelPassword() {
    return gMode == 0 ? gPassword : gChannel;
}

const std::string &syncGetChannelPasswordForC() {
    return gPassword;
}

void write_cb(uv_write_t* req, int status);

bool syncSendData(char type, const std::string &data) {
    if (gMode == 0 && gPassword.empty()) return false;
    auto len = (int)data.size();
    uv_buf_t buf;
    buf.len = (unsigned int)(4 + 1 + len);
    buf.base = new char[4 + 1 + len];
    *(int *)buf.base = 1 + len;
    buf.base[4] = type;
    if (len > 0)
        memcpy(buf.base + 5, data.c_str(), len);
    auto *req = new uv_write_t;
    memset(req, 0, sizeof(uv_write_t));
    req->data = buf.base;
    return uv_write(req, (uv_stream_t*)clientCtx, &buf, 1, write_cb) == 0;
}

void write_cb(uv_write_t *req, int status) {
    delete[] (char *)req->data;
    delete req;
}

bool syncSendData(char type, const std::vector<std::string> &data, char separator) {
    if (gMode == 0 && gPassword.empty()) return false;
    std::string n;
    for (auto &s : data) {
        n += s;
        n += separator;
    }
    if (!n.empty())
        n.resize(n.size() - 1);
    return syncSendData(type, n);
}
