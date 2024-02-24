#pragma once

#include <vector>
#include <string>

typedef void (*ConnectionCallback)();
typedef void (*ChannelCallback)(char type, const std::string &data);

void syncInit();
void syncProcess();
int syncGetMode();
bool syncOpen(ConnectionCallback callback);
void syncClose();
bool syncSetChannel(ChannelCallback callback);
bool syncSendData(char type, const std::string &data);
bool syncSendData(char type, const std::vector<std::string> &data, char separator = '\n');
