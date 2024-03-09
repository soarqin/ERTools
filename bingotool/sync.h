#pragma once

#include <vector>
#include <string>

typedef void (*ConnectionCallback)();
typedef void (*ChannelCallback)(char type, const std::string &data);

void syncLoadConfig();
void syncProcess();
void syncSetMode(int mode);
int syncGetMode();
bool syncOpen(ConnectionCallback callback);
void syncClose();
bool syncSetChannel(ChannelCallback callback);
void syncSetChannelPassword(const std::string &password);
void syncSetChannelPasswordForC(const std::string &password);
const std::string &syncGetChannelPassword();
const std::string &syncGetChannelPasswordForC();
bool syncSendData(char type, const std::string &data);
bool syncSendData(char type, const std::vector<std::string> &data, char separator = '\n');
