#pragma once

#include "networkmanager.h"

extern volatile int g_should_stop;

void handle_cmd(NetworkManager *network, char *line);
