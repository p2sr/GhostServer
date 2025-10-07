#include "commands.h"
#include "networkmanager.h"
#include <iostream>
#include <string>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <algorithm>

#ifdef _WIN32
# include <Windows.h>
# define strdup _strdup
# define strcasecmp _stricmp
#else
# include <strings.h>
#endif

volatile int g_should_stop = 0;

static char *g_entered_pre = strdup("");
static char *g_entered_post = strdup("");

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


// TODO: save bans/whitelist to disk

void handle_cmd(NetworkManager *network, char *line) {
    std::string _line(line);
    std::vector<std::string> args;
    std::vector<std::string> argsL; // lower
    std::vector<std::string> argsR; // remaining
    for (size_t i = 0; i < _line.size(); ) {
        while (i < _line.size() && isspace(_line[i])) ++i;
        if (i >= _line.size()) break;

        size_t start = i;
        if (_line[i] == '"') {
            ++i;
            start = i;
            while (i < _line.size() && _line[i] != '"') ++i;
        } else {
            while (i < _line.size() && !isspace(_line[i])) ++i;
        }
        std::string arg = _line.substr(start, i - start);
        args.push_back(arg);
        for (auto &c : arg) c = tolower(c);
        argsL.push_back(arg);
        if (i < _line.size() && _line[i] == '"') ++i;
        while (i < _line.size() && isspace(_line[i])) ++i;
        argsR.push_back(_line.substr(i));
    }

#ifdef GHOST_GUI
    LINE("] %s", line);
#endif

    if (args.empty()) return;

    if (false) { // debug
        LINE("%d args:", (int)args.size());
        for (size_t i = 0; i < args.size(); ++i) {
            LINE("  %d: '%s' '%s' '%s'", (int)i, args[i].c_str(), argsL[i].c_str(), argsR[i].c_str());
        }
    }

    std::string cmd = argsL[0];
    if (cmd == "help") {
        LINE("Available commands:");
        LINE("  help                  show this list");
        LINE("  start                 start the server");
        LINE("  stop                  terminate the server");
        LINE("  list                  list all the currently connected clients");
        LINE("  countdown             start/modify a countdown");
        LINE("  kick                  kick a client");
        LINE("  ban                   ban a client");
        LINE("  unban                 unban a client");
        LINE("  accept                start accepting connections");
        LINE("  refuse                stop accepting connections");
        LINE("  say                   send a message to all clients");
        LINE("  whitelist             manage the whitelist");
        return;
    }

    if (cmd == "start") {
        if (network->isRunning) {
            LINE("Server is already running.");
            return;
        }
        int port = 53000;
        if (args.size() >= 2) {
            port = atoi(args[1].c_str());
            if (port < 1 || port > 65535) {
                LINE("Invalid port %d", port);
                return;
            }
        }
        if (!network->StartServer(port)) {
            LINE("Failed to start server on port %d", port);
            return;
        }
        LINE("Server started on port %d", port);
        return;
    }

    if (cmd == "stop") {
        if (!network->isRunning) {
            LINE("Server is not running.");
            return;
        }
        g_should_stop = 1;
        return;
    }

    if (!network->isRunning) {
        LINE("Server is not running. Use 'start' to start the server.");
        return;
    }

    if (cmd == "list") {
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

    if (cmd == "countdown") {
        if (args.size() < 2) {
            LINE("Usage: countdown <pre|post|start> [pre-cmds|post-cmds|duration]");
            return;
        }
        std::string subcmd = argsL[1];
        if (subcmd == "pre") {
            if (args.size() < 3) {
                LINE("Usage: countdown pre <pre-cmds>");
                return;
            }
            std::string pre_cmds = argsR[1];
            g_entered_pre = strdup(pre_cmds.c_str());
        } else if (subcmd == "post") {
            if (args.size() < 3) {
                LINE("Usage: countdown post <post-cmds>");
                return;
            }
            std::string post_cmds = argsR[1];
            g_entered_post = strdup(post_cmds.c_str());
        } else if (subcmd == "start") {
            if (args.size() != 3) {
                LINE("Usage: countdown start <duration>");
                return;
            }
            int duration = atoi(args[2].c_str());
            if (duration <= 0) {
                LINE("Duration must be positive");
                return;
            } else if (duration > 60) {
                LINE("Duration too long (max 60)");
                return;
            }
            network->ScheduleServerThread([=]() {
                network->StartCountdown(std::string(g_entered_pre), std::string(g_entered_post), duration);
            });
        } else {
            LINE("Usage: countdown <pre|post|start> [pre-cmds|post-cmds|duration]");
        }
        return;
    }

    if (cmd == "kick") {
        if (args.size() != 3) {
            LINE("Usage: kick <name|id|ip> <value>");
            return;
        }
        std::string subcmd = argsL[1];
        if (subcmd == "name") {
            std::string name = args[2];
            network->ScheduleServerThread([=]() {
                auto players = network->GetPlayerByName(name);
                if (players.empty()) {
                    LINE("No player named '%s'", name.c_str());
                } else {
                    for (auto &p : players) {
                        network->DisconnectPlayer(*p, "kicked");
                    }
                }
            });
        } else if (subcmd == "id") {
            int id = atoi(args[2].c_str());
            network->ScheduleServerThread([=]() {
                auto p = network->GetClientByID(id);
                if (!p) {
                    LINE("No player with ID %d", id);
                } else {
                    network->DisconnectPlayer(*p, "kicked");
                }
            });
        } else if (subcmd == "ip") {
            sf::IpAddress ip(args[2]);
            if (ip == sf::IpAddress::None) {
                LINE("Invalid IP address");
                return;
            }
            network->ScheduleServerThread([=]() {
                auto players = network->GetClientByIP(ip);
                if (players.empty()) {
                    LINE("No player with IP %s", ip.toString().c_str());
                } else {
                    for (auto &p : players) {
                        network->DisconnectPlayer(*p, "kicked");
                    }
                }
            });
        } else {
            LINE("Usage: kick <name|id|ip> <value>");
        }
        return;
    }

    if (cmd == "ban") {
        if (args.size() < 2 || args.size() > 3) {
            LINE("Usage: ban <name|id|ip|list> <value>");
            return;
        }
        std::string subcmd = argsL[1];
        if (subcmd == "name") {
            if (args.size() != 3) {
                LINE("Usage: ban name <value>");
                return;
            }
            std::string name = args[2];
            network->ScheduleServerThread([=]() {
                auto players = network->GetPlayerByName(name);
                if (players.empty()) {
                    LINE("No player named '%s'", name.c_str());
                } else {
                    for (auto p : players) {
                        network->BanClientIP(p->IP);
                    }
                }
            });
        } else if (subcmd == "id") {
            if (args.size() != 3) {
                LINE("Usage: ban id <value>");
                return;
            }
            int id = atoi(args[2].c_str());
            network->ScheduleServerThread([=]() {
                auto p = network->GetClientByID(id);
                if (!p) {
                    LINE("No player with ID %d", id);
                } else {
                    network->BanClientIP(p->IP);
                }
            });
        } else if (subcmd == "ip") {
            if (args.size() != 3) {
                LINE("Usage: ban ip <value>");
                return;
            }
            sf::IpAddress ip(args[2]);
            if (ip == sf::IpAddress::None) {
                LINE("Invalid IP address");
                return;
            }
            network->ScheduleServerThread([=]() {
                network->BanClientIP(ip);
            });
        } else if (subcmd == "list") {
            network->ScheduleServerThread([=]() {
                if (network->bannedIps.empty()) {
                    LINE("No banned IPs");
                } else {
                    LINE("Banned IPs:");
                    for (auto &ip : network->bannedIps) {
                        LINE("  %s", ip.toString().c_str());
                    }
                }
            });
        } else {
            LINE("Usage: ban <name|id|ip|list> <value>");
        }
        return;
    }

    if (cmd == "unban") {
        if (args.size() != 2) {
            LINE("Usage: unban <ip>");
            return;
        }
        sf::IpAddress ip(args[1]);
        if (ip == sf::IpAddress::None) {
            LINE("Invalid IP address");
            return;
        }
        network->ScheduleServerThread([=]() {
            auto it = network->bannedIps.find(ip);
            if (it == network->bannedIps.end()) {
                LINE("IP %s is not banned", ip.toString().c_str());
            } else {
                network->bannedIps.erase(it);
                LINE("Unbanned IP %s", ip.toString().c_str());
            }
        });
        return;
    }

    if (cmd == "accept") {
        if (args.size() != 2) {
            LINE("Usage: accept <players|spectators|all>");
            return;
        }
        std::string subcmd = argsL[1];
        if (subcmd == "players") {
            network->ScheduleServerThread([=]() {
                network->acceptingPlayers = true;
            });
            LINE("Now accepting connections from players");
        } else if (subcmd == "spectators") {
            network->ScheduleServerThread([=]() {
                network->acceptingSpectators = true;
            });
            LINE("Now accepting connections from spectators");
        } else if (subcmd == "all") {
            network->ScheduleServerThread([=]() {
                network->acceptingPlayers = true;
                network->acceptingSpectators = true;
            });
            LINE("Now accepting connections from players and spectators");
        } else {
            LINE("Usage: accept <players|spectators|all>");
        }
        return;
    }

    if (cmd == "refuse") {
        if (args.size() != 2) {
            LINE("Usage: refuse <players|spectators|all>");
            return;
        }
        std::string subcmd = argsL[1];
        if (subcmd == "players") {
            network->ScheduleServerThread([=]() {
                network->acceptingPlayers = false;
            });
            LINE("Now refusing connections from players");
        } else if (subcmd == "spectators") {
            network->ScheduleServerThread([=]() {
                network->acceptingSpectators = false;
            });
            LINE("Now refusing connections from spectators");
        } else if (subcmd == "all") {
            network->ScheduleServerThread([=]() {
                network->acceptingPlayers = false;
                network->acceptingSpectators = false;
            });
            LINE("Now refusing connections from players and spectators");
        } else {
            LINE("Usage: refuse <players|spectators|all>");
        }
        return;
    }

    if (cmd == "say") {
        if (args.size() < 2) {
            LINE("Usage: say <message>");
            return;
        }
        std::string message = argsR[0];
        network->ScheduleServerThread([=]() {
            network->ServerMessage(message.c_str());
        });
        return;
    }

    if (cmd == "whitelist") {
        if (args.size() < 2 || args.size() > 4) {
            LINE("Usage: whitelist <on|off|kick|add|remove|list> [name|ip] [value]");
            if (args.size() == 1) {
                LINE("Whitelist is currently %s", network->whitelistEnabled ? "on" : "off");
            }
            return;
        }
        std::string subcmd = argsL[1];
        if (subcmd == "on") {
            network->ScheduleServerThread([=]() {
                network->whitelistEnabled = true;
            });
            LINE("Whitelist now enabled");
        } else if (subcmd == "off") {
            network->ScheduleServerThread([=]() {
                network->whitelistEnabled = false;
            });
            LINE("Whitelist now disabled");
        } else if (subcmd == "kick") {
            for (auto &cl : network->clients) {
                if (!network->IsOnWhitelist(cl.name, cl.IP)) {
                    network->DisconnectPlayer(cl, "not on whitelist");
                }
            }
            LINE("Kicked all non-whitelisted clients");
        } else if (subcmd == "add") {
            if (args.size() != 4) {
                LINE("Usage: whitelist add <name|ip> <value>");
                return;
            }
            std::string type = argsL[2];
            std::string value = args[3];
            if (type == "name") {
                network->ScheduleServerThread([=]() {
                    network->whitelist.insert(WhitelistEntry{WhitelistEntryType::NAME, value});
                });
                LINE("Added name '%s' to whitelist", value.c_str());
            } else if (type == "ip") {
                sf::IpAddress ip(value);
                if (ip == sf::IpAddress::None) {
                    LINE("Invalid IP address");
                    return;
                }
                network->ScheduleServerThread([=]() {
                    network->whitelist.insert(WhitelistEntry{WhitelistEntryType::IP, ip.toString()});
                });
                LINE("Added IP '%s' to whitelist", ip.toString().c_str());
            } else {
                LINE("Usage: whitelist add <name|ip> <value>");
            }
        } else if (subcmd == "remove") {
            if (args.size() != 4) {
                LINE("Usage: whitelist remove <name|ip> <value>");
                return;
            }
            std::string type = argsL[2];
            std::string value = args[3];
            if (type == "name") {
                network->ScheduleServerThread([=]() {
                    network->whitelist.erase(WhitelistEntry{WhitelistEntryType::NAME, value});
                });
                LINE("Removed name '%s' from whitelist", value.c_str());
            } else if (type == "ip") {
                sf::IpAddress ip(value);
                if (ip == sf::IpAddress::None) {
                    LINE("Invalid IP address");
                    return;
                }
                network->ScheduleServerThread([=]() {
                    network->whitelist.erase(WhitelistEntry{WhitelistEntryType::IP, ip.toString()});
                });
                LINE("Removed IP '%s' from whitelist", ip.toString().c_str());
            } else {
                LINE("Usage: whitelist remove <name|ip> <value>");
            }
        } else if (subcmd == "list") {
            network->ScheduleServerThread([=]() {
                if (network->whitelist.empty()) {
                    LINE("Whitelist is empty");
                } else {
                    LINE("Whitelist entries:");
                    for (auto &entry : network->whitelist) {
                        if (entry.type == WhitelistEntryType::NAME) {
                            LINE("  Name: %s", entry.value.c_str());
                        } else if (entry.type == WhitelistEntryType::IP) {
                            LINE("  IP:   %s", entry.value.c_str());
                        }
                    }
                }
                LINE("Whitelist is currently %s", network->whitelistEnabled ? "on" : "off");
            });
        } else {
            LINE("Usage: whitelist <on|off|add|remove|list> [name|ip] [value]");
        }
        return;
    }

    LINE("Unknown command: '%s'", line);
    LINE("Enter 'help' for a list of commands.");
}
