#include "commands.h"
#include "networkmanager.h"
#include <iostream>
#include <string>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <algorithm>

volatile int g_should_stop = 0;
CommandType g_current_cmd = CMD_NONE;

static char *g_entered_pre;
static char *g_entered_post;
static int g_countdown_duration = -1;

std::string ssprintf(const char *fmt, ...) {
	va_list ap1, ap2;
	va_start(ap1, fmt);
	va_copy(ap2, ap1);
	size_t sz = vsnprintf(NULL, 0, fmt, ap1) + 1;
	va_end(ap1);
	char *buf = (char *)malloc(sz);
	vsnprintf(buf, sz, fmt, ap2);
	va_end(ap2);
	std::string str(buf);
	free(buf);
	return str;
}

#ifdef GHOST_GUI
# define LINE(x, ...) emit network->OnNewEvent(QString::fromStdString(ssprintf(x, ##__VA_ARGS__)))
# define LINE_NONL(x) emit network->OnNewEvent(QString::fromStdString(ssprintf(x)))
#else
# define LINE(x, ...) printf(x "\n", ##__VA_ARGS__)
# define LINE_NONL(x) (fputs(x, stdout), fflush(stdout))
#endif

#ifdef _WIN32
# define strcasecmp _stricmp
#else
# include <strings.h>
#endif

void handle_cmd(NetworkManager *network, char *line) {
    while (isspace(*line)) ++line;

    size_t len = strlen(line);

    while (len > 0 && isspace(line[len-1])) {
        line[len-1] = 0;
        len -= 1;
    }

#ifdef GHOST_GUI
    LINE("] %s", line);
#endif

    if (!network->isRunning) {
        LINE("Start the server first.");
        g_current_cmd = CMD_NONE;
        return;
    }

    std::string _line(line);

    switch (g_current_cmd) {
    case CMD_NONE:
        if (len == 0) return;

        if (!strcmp(line, "quit")) {
            g_should_stop = 1;
            return;
        }

        if (!strcmp(line, "help")) {
            LINE("Available commands:");
            LINE("  help                  show this list");
            LINE("  quit                  terminate the server");
            LINE("  list                  list all the currently connected clients");
            LINE("  countdown_set         set the pre/post cmds and countdown duration");
            LINE("  countdown             start a countdown");
            LINE("  disconnect            disconnect a client by name");
            LINE("  disconnect_id         disconnect a client by ID");
            LINE("  ban                   ban connections from a certain IP by ghost name");
            LINE("  ban_id                ban connections from a certain IP by ghost ID");
            LINE("  accept_players        start accepting connections from players");
            LINE("  refuse_players        stop accepting connections from players");
            LINE("  accept_spectators     start accepting connections from spectators");
            LINE("  refuse_spectators     stop accepting connections from spectators");
            LINE("  server_msg            send all clients a message from the server");
            LINE("  whitelist_enable      enable whitelist");
            LINE("  whitelist_disable     disable whitelist");
            LINE("  whitelist_add_name    add player to whitelist");
            LINE("  whitelist_add_ip      add player to whitelist");
            LINE("  whitelist_remove_name remove player from whitelist");
            LINE("  whitelist_remove_ip   remove player from whitelist");
            LINE("  whitelist             print out all entries on whitelist");
            return;
        }

        if (!strcmp(line, "list")) {
            network->ScheduleServerThread([=]() {
                if (network->clients.empty()) {
                    LINE("No clients");
                } else {
                    LINE("Clients:");
                    for (auto &cl : network->clients) {
                        LINE("  %-3d %s @ %s:%d", cl.ID, cl.name.c_str(), cl.IP.toString().c_str(), (int)cl.port);
                    }
                }
            });
            return;
        }

        if (!strcmp(line, "countdown")) {
            if (g_countdown_duration == -1) {
                LINE("Set a countdown first using countdown_set.");
                return;
            }
            network->ScheduleServerThread([=]() {
                network->StartCountdown(std::string(g_entered_pre), std::string(g_entered_post), g_countdown_duration);
            });
            LINE("Started countdown with:");
            LINE("  pre-cmd '%s'", g_entered_pre);
            LINE("  post-cmd '%s'", g_entered_post);
            LINE("  duration %d", g_countdown_duration);
            return;
        }

        if (!strcmp(line, "countdown_set")) {
            g_current_cmd = CMD_COUNTDOWN_SET;
            if (g_entered_pre) free(g_entered_pre);
            if (g_entered_post) free(g_entered_post);
            g_entered_pre = g_entered_post = NULL;
            g_countdown_duration = -1;
            LINE_NONL("Pre-countdown command: ");
            return;
        }

        if (!strcmp(line, "disconnect")) {
            g_current_cmd = CMD_DISCONNECT;
            LINE_NONL("Name of player to disconnect: ");
            return;
        }

        if (!strcmp(line, "disconnect_id")) {
            g_current_cmd = CMD_DISCONNECT_ID;
            LINE_NONL("ID of player to disconnect: ");
            return;
        }

        if (!strcmp(line, "ban")) {
            g_current_cmd = CMD_BAN;
            LINE_NONL("Name of player to ban: ");
            return;
        }

        if (!strcmp(line, "ban_id")) {
            g_current_cmd = CMD_BAN_ID;
            LINE_NONL("ID of player to ban: ");
            return;
        }

        if (!strcmp(line, "accept_players")) {
            network->ScheduleServerThread([=]() {
                network->acceptingPlayers = true;
            });
            LINE("Now accepting connections from players");
            return;
        }

        if (!strcmp(line, "refuse_players")) {
            network->ScheduleServerThread([=]() {
                network->acceptingPlayers = false;
            });
            LINE("Now refusing connections from players");
            return;
        }

        if (!strcmp(line, "accept_spectators")) {
            network->ScheduleServerThread([=]() {
                network->acceptingSpectators = true;
            });
            LINE("Now accepting connections from spectators");
            return;
        }

        if (!strcmp(line, "refuse_spectators")) {
            network->ScheduleServerThread([=]() {
                network->acceptingSpectators = false;
            });
            LINE("Now refusing connections from spectators");
            return;
        }

        if (!strcmp(line, "server_msg")) {
            g_current_cmd = CMD_SERVER_MSG;
            LINE_NONL("Message to send: ");
            return;
        }

        if (!strcmp(line, "whitelist_enable")) {
            g_current_cmd = CMD_WHITELIST_ENABLE;
            LINE_NONL("Disconnect players not on whitelist? (y/N) ");
            return;
        }

        if (!strcmp(line, "whitelist_disable")) {
            network->ScheduleServerThread([=]() {
                network->whitelistEnabled = false;
            });
            LINE("Whitelist now disabled");
            return;
        }

        if (!strcmp(line, "whitelist_add_name")) {
            g_current_cmd = CMD_WHITELIST_ADD_NAME;
            LINE_NONL("Player to add: ");
            return;
        }

        if (!strcmp(line, "whitelist_add_ip")) {
            g_current_cmd = CMD_WHITELIST_ADD_IP;
            LINE_NONL("Player IP to add: ");
            return;
        }

        if (!strcmp(line, "whitelist_remove_name")) {
            g_current_cmd = CMD_WHITELIST_REMOVE_NAME;
            LINE_NONL("Player to remove: ");
            return;
        }

        if (!strcmp(line, "whitelist_remove_ip")) {
            g_current_cmd = CMD_WHITELIST_REMOVE_IP;
            LINE_NONL("Player IP to remove: ");
            return;
        }

        if (!strcmp(line, "whitelist")) {
            if (network->whitelist.size() == 0) {
                LINE("No players on whitelist");
            } else {
                LINE("Players on whitelist:");
                for (auto& entry : network->whitelist) {
                    LINE("  %s", entry.value.c_str());
                }
            }

            if (network->whitelistEnabled)
                LINE("(Whitelist enabled)");
            else
                LINE("(Whitelist disabled)");
            return;
        }
        
        LINE("Unknown command: '%s'", line);
        LINE("Enter 'help' for a list of commands.");
        return;

    case CMD_COUNTDOWN_SET:
        if (!g_entered_pre) {
            g_entered_pre = strdup(line);
            LINE_NONL("Post-countdown command: ");
            return;
        }

        if (!g_entered_post) {
            g_entered_post = strdup(line);
            LINE_NONL("Countdown duration: ");
            return;
        }

        g_countdown_duration = atoi(line);
        if (g_countdown_duration < 0 || g_countdown_duration > 60) {
            LINE("Bad countdown duration; not setting countdown.");
            if (g_entered_pre) free(g_entered_pre);
            if (g_entered_post) free(g_entered_post);
            g_entered_pre = g_entered_post = NULL;
            g_countdown_duration = -1;
            g_current_cmd = CMD_NONE;
            return;
        }

        LINE("Initialized countdown");
        g_current_cmd = CMD_NONE;

        return;

    case CMD_DISCONNECT:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            network->ScheduleServerThread([=]() {
                auto players = network->GetPlayerByName(_line);
                for (auto cl : players) network->DisconnectPlayer(*cl, "kicked");
            });
            LINE("Disconnected player '%s'", line);
        } else {
            LINE("No player name given; not disconnecting anyone.");
        }
        return;

    case CMD_DISCONNECT_ID:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            int id = atoi(line);
            network->ScheduleServerThread([=]() {
                auto cl = network->GetClientByID(id);
                if (cl) network->DisconnectPlayer(*cl, "kicked");
            });
            LINE("Disconnected player ID %d", id);
        } else {
            LINE("No player ID given; not disconnecting anyone.");
        }
        return;

    case CMD_BAN:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            network->ScheduleServerThread([=]() {
                auto players = network->GetPlayerByName(_line);
                for (auto cl : players) network->BanClientIP(*cl);
            });
            LINE("Banned player '%s'", line);
        } else {
            LINE("No player name given; not banning anyone.");
        }
        return;

    case CMD_BAN_ID:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            int id = atoi(line);
            network->ScheduleServerThread([=]() {
                auto cl = network->GetClientByID(id);
                if (cl) network->BanClientIP(*cl);
            });
            LINE("Banned player ID %d", id);
        } else {
            LINE("No player ID given; not banning anyone.");
        }
        return;

    case CMD_SERVER_MSG:
        g_current_cmd = CMD_NONE;
        network->ServerMessage(line);
        return;

    case CMD_WHITELIST_ENABLE:
        g_current_cmd = CMD_NONE;
        network->ScheduleServerThread([=] {
            network->whitelistEnabled = true; 
            if (!strcmp(_line.c_str(), "y")) {
                for (auto& client : network->clients) {
                    if (!network->IsOnWhitelist(client.name, client.IP)) {
                        network->DisconnectPlayer(client, "not on whitelist");
                    }
                }
            }
        });

        LINE("Whitelist now enabled!");
        return;

    case CMD_WHITELIST_ADD_NAME:
        g_current_cmd = CMD_NONE;
        network->ScheduleServerThread([=] {
            network->whitelist.insert({ WhitelistEntryType::NAME, _line });
        });
        LINE("Added player %s to whitelist", line);
        return;

    case CMD_WHITELIST_ADD_IP:
        g_current_cmd = CMD_NONE;
        network->ScheduleServerThread([=] {
            network->whitelist.insert({ WhitelistEntryType::IP, _line });
        });
        LINE("Added player IP %s to whitelist", line);
        return;

    case CMD_WHITELIST_REMOVE_NAME: {
        g_current_cmd = CMD_NONE;

        auto index = std::find_if(network->whitelist.begin(), network->whitelist.end(), [&](const WhitelistEntry& entry) {
            return entry.type == WhitelistEntryType::NAME && strcasecmp(entry.value.c_str(), _line.c_str()) == 0;
        });

        if (index != network->whitelist.end()) {
            auto clients = network->GetPlayerByName(_line);

            network->ScheduleServerThread([=]() {
                network->whitelist.erase(index);

                for (auto client : clients) {
                    network->DisconnectPlayer(*client, "not on whitelist");
                }
            });
        }

        LINE("Removed player %s from whitelist", line);
        return;
    }

    case CMD_WHITELIST_REMOVE_IP: {
        g_current_cmd = CMD_NONE;

        sf::IpAddress ip(_line);
        if (ip == sf::IpAddress::None) {
            LINE("Invalid IP address: %s", line);
            return;
        }

        auto index = std::find_if(network->whitelist.begin(), network->whitelist.end(), [&](const WhitelistEntry& entry) {
            return entry.type == WhitelistEntryType::IP && entry.value == ip.toString();
        });

        if (index != network->whitelist.end()) {
            network->ScheduleServerThread([=]() {
                network->whitelist.erase(index);

                auto clients = network->GetClientByIP(ip);
                for (auto client : clients) {
                    network->DisconnectPlayer(*client, "not on whitelist");
                }
            });
        }

        LINE("Removed player IP %s from whitelist", line);
        return;
    }
    }
}
