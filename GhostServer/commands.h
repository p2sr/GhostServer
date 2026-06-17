#pragma once

#include "networkmanager.h"

extern volatile int g_should_stop;

std::string ssprintf(const char *fmt, ...);
void handle_cmd(NetworkManager *network, char *line);
