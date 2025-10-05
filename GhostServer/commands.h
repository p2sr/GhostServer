#pragma once

#include "networkmanager.h"

enum CommandType {
    CMD_NONE,
    CMD_COUNTDOWN_SET,
    CMD_DISCONNECT,
    CMD_DISCONNECT_ID,
    CMD_BAN,
    CMD_BAN_ID,
    CMD_SERVER_MSG,
    CMD_WHITELIST_ENABLE,
    CMD_WHITELIST_ADD_NAME,
    CMD_WHITELIST_ADD_IP,
    CMD_WHITELIST_REMOVE_NAME,
    CMD_WHITELIST_REMOVE_IP,
};

extern volatile int g_should_stop;
extern enum CommandType g_current_cmd;

void handle_cmd(NetworkManager *network, char *line);
