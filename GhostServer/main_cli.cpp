#include "networkmanager.h"

#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <string>
#ifdef _WIN32
# include <conio.h> // for _kbhit(), _getch()
# include <windows.h>
#else
# include <poll.h>
# include <unistd.h>
#endif

static volatile int g_should_stop = 0;

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
static CommandType g_current_cmd = CMD_NONE;

static char *g_entered_pre;
static char *g_entered_post;
static int g_countdown_duration = -1;

static NetworkManager *g_network;

static void handle_cmd(char *line) {
    while (isspace(*line)) ++line;

    size_t len = strlen(line);

    while (len > 0 && isspace(line[len-1])) {
        line[len-1] = 0;
        len -= 1;
    }

    switch (g_current_cmd) {
    case CMD_NONE:
        if (len == 0) return;

        if (!strcmp(line, "quit")) {
            g_should_stop = 1;
            return;
        }

        if (!strcmp(line, "help")) {
            puts("Available commands:");
            puts("  help                  show this list");
            puts("  quit                  terminate the server");
            puts("  list                  list all the currently connected clients");
            puts("  countdown_set         set the pre/post cmds and countdown duration");
            puts("  countdown             start a countdown");
            puts("  disconnect            disconnect a client by name");
            puts("  disconnect_id         disconnect a client by ID");
            puts("  ban                   ban connections from a certain IP by ghost name");
            puts("  ban_id                ban connections from a certain IP by ghost ID");
            puts("  accept_players        start accepting connections from players");
            puts("  refuse_players        stop accepting connections from players");
            puts("  accept_spectators     start accepting connections from spectators");
            puts("  refuse_spectators     stop accepting connections from spectators");
            puts("  server_msg            send all clients a message from the server");
            puts("  whitelist_enable      enable whitelist");
            puts("  whitelist_disable     disable whitelist");
            puts("  whitelist_add_name    add player to whitelist");
            puts("  whitelist_add_ip      add player to whitelist");
            puts("  whitelist_remove_name remove player from whitelist");
            puts("  whitelist_remove_ip   remove player from whitelist");
            puts("  whitelist             print out all entries on whitelist");
            return;
        }

        if (!strcmp(line, "list")) {
            g_network->ScheduleServerThread([]() {
                if (g_network->clients.empty()) {
                    puts("No clients");
                } else {
                    puts("Clients:");
                    for (auto &cl : g_network->clients) {
                        printf("  %-3d %s @ %s:%d\n", cl.ID, cl.name.c_str(), cl.IP.toString().c_str(), (int)cl.port);
                    }
                }
            });
            return;
        }

        if (!strcmp(line, "countdown")) {
            if (g_countdown_duration == -1) {
                puts("Set a countdown first using countdown_set.");
                return;
            }
            g_network->ScheduleServerThread([]() {
                g_network->StartCountdown(std::string(g_entered_pre), std::string(g_entered_post), g_countdown_duration);
            });
            puts("Started countdown with:");
            printf("  pre-cmd '%s'\n", g_entered_pre);
            printf("  post-cmd '%s'\n", g_entered_post);
            printf("  duration %d\n", g_countdown_duration);
            return;
        }

        if (!strcmp(line, "countdown_set")) {
            g_current_cmd = CMD_COUNTDOWN_SET;
            if (g_entered_pre) free(g_entered_pre);
            if (g_entered_post) free(g_entered_post);
            g_entered_pre = g_entered_post = NULL;
            g_countdown_duration = -1;
            fputs("Pre-countdown command: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "disconnect")) {
            g_current_cmd = CMD_DISCONNECT;
            fputs("Name of player to disconnect: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "disconnect_id")) {
            g_current_cmd = CMD_DISCONNECT_ID;
            fputs("ID of player to disconnect: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "ban")) {
            g_current_cmd = CMD_BAN;
            fputs("Name of player to ban: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "ban_id")) {
            g_current_cmd = CMD_BAN_ID;
            fputs("ID of player to ban: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "accept_players")) {
            g_network->ScheduleServerThread([]() {
                g_network->acceptingPlayers = true;
            });
            puts("Now accepting connections from players");
            return;
        }

        if (!strcmp(line, "refuse_players")) {
            g_network->ScheduleServerThread([]() {
                g_network->acceptingPlayers = false;
            });
            puts("Now refusing connections from players");
            return;
        }

        if (!strcmp(line, "accept_spectators")) {
            g_network->ScheduleServerThread([]() {
                g_network->acceptingSpectators = true;
            });
            puts("Now accepting connections from spectators");
            return;
        }

        if (!strcmp(line, "refuse_spectators")) {
            g_network->ScheduleServerThread([]() {
                g_network->acceptingSpectators = false;
            });
            puts("Now refusing connections from spectators");
            return;
        }

        if (!strcmp(line, "server_msg")) {
            g_current_cmd = CMD_SERVER_MSG;
            fputs("Message to send: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "whitelist_enable")) {
            g_current_cmd = CMD_WHITELIST_ENABLE;
            fputs("Disconnect players not on whitelist? (y/N) ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "whitelist_disable")) {
            g_network->ScheduleServerThread([] {
                g_network->whitelistEnabled = false;
            });
            puts("Whitelist now disabled");
            return;
        }

        if (!strcmp(line, "whitelist_add_name")) {
            g_current_cmd = CMD_WHITELIST_ADD_NAME;
            fputs("Player to add: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "whitelist_add_ip")) {
            g_current_cmd = CMD_WHITELIST_ADD_IP;
            fputs("Player IP to add: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "whitelist_remove_name")) {
            g_current_cmd = CMD_WHITELIST_REMOVE_NAME;
            fputs("Player to remove: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "whitelist_remove_ip")) {
            g_current_cmd = CMD_WHITELIST_REMOVE_IP;
            fputs("Player IP to remove: ", stdout);
            fflush(stdout);
            return;
        }

        if (!strcmp(line, "whitelist")) {
            if (g_network->whitelist.size() == 0) {
                puts("No players on whitelist");
            } else {
                puts("Players on whitelist:");
                for (auto& entry : g_network->whitelist) {
                    printf("  %s\n", entry.value.c_str());
                }
            }

            if (g_network->whitelistEnabled)
                puts("(Whitelist enabled)");
            else
                puts("(Whitelist disabled)");
            return;
        }
        
        printf("Unknown command: '%s'\n", line);
        return;

    case CMD_COUNTDOWN_SET:
        if (!g_entered_pre) {
            g_entered_pre = strdup(line);
            fputs("Post-countdown command: ", stdout);
            fflush(stdout);
            return;
        }

        if (!g_entered_post) {
            g_entered_post = strdup(line);
            fputs("Countdown duration: ", stdout);
            fflush(stdout);
            return;
        }

        g_countdown_duration = atoi(line);
        if (g_countdown_duration < 0 || g_countdown_duration > 60) {
            puts("Bad countdown duration; not setting countdown.");
            if (g_entered_pre) free(g_entered_pre);
            if (g_entered_post) free(g_entered_post);
            g_entered_pre = g_entered_post = NULL;
            g_countdown_duration = -1;
            g_current_cmd = CMD_NONE;
            return;
        }

        puts("Initialized countdown");
        g_current_cmd = CMD_NONE;

        return;

    case CMD_DISCONNECT:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            std::string l1(line);
            g_network->ScheduleServerThread([=]() {
                auto players = g_network->GetPlayerByName(l1);
                for (auto cl : players) g_network->DisconnectPlayer(*cl, "kicked");
            });
            printf("Disconnected player '%s'\n", line);
        }
        return;

    case CMD_DISCONNECT_ID:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            int id = atoi(line);
            g_network->ScheduleServerThread([=]() {
                auto cl = g_network->GetClientByID(id);
                if (cl) g_network->DisconnectPlayer(*cl, "kicked");
            });
            printf("Disconnected player ID %d\n", id);
        }
        return;

    case CMD_BAN:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            std::string l1(line);
            g_network->ScheduleServerThread([=]() {
                auto players = g_network->GetPlayerByName(l1);
                for (auto cl : players) g_network->BanClientIP(*cl);
            });
            printf("Banned player '%s'\n", line);
        }
        return;

    case CMD_BAN_ID:
        g_current_cmd = CMD_NONE;
        if (len != 0) {
            int id = atoi(line);
            g_network->ScheduleServerThread([=]() {
                auto cl = g_network->GetClientByID(id);
                if (cl) g_network->BanClientIP(*cl);
            });
            printf("Banned player ID %d\n", id);
        }
        return;

    case CMD_SERVER_MSG:
        g_current_cmd = CMD_NONE;
        g_network->ServerMessage(line);
        return;

    case CMD_WHITELIST_ENABLE:
        g_current_cmd = CMD_NONE;
        g_network->ScheduleServerThread([=] {
            g_network->whitelistEnabled = true; 

            if (!strcmp(line, "y")) {
                for (auto& client : g_network->clients) {
                    if (!g_network->IsOnWhitelist(client.name, client.IP)) {
                        g_network->DisconnectPlayer(client, "Not on whitelist");
                    }
                }
            }
        });

        puts("Whitelist now enabled!");
        return;

    case CMD_WHITELIST_ADD_NAME:
        g_current_cmd = CMD_NONE;
        g_network->ScheduleServerThread([=] {
            g_network->whitelist.insert({ WhitelistEntryType::NAME, line });
        });
        printf("Added player %s to whitelist\n", line);
        return;

    case CMD_WHITELIST_ADD_IP:
        g_current_cmd = CMD_NONE;
        g_network->ScheduleServerThread([=] {
            g_network->whitelist.insert({ WhitelistEntryType::IP, line });
        });
        printf("Added player IP %s to whitelist\n", line);
        return;

    case CMD_WHITELIST_REMOVE_NAME: {
        g_current_cmd = CMD_NONE;

        std::string _line(line);

        auto index = std::find_if(g_network->whitelist.begin(), g_network->whitelist.end(), [&](const WhitelistEntry& entry) {
            return entry.type == WhitelistEntryType::NAME && entry.value == _line;
        });

        if (index != g_network->whitelist.end()) {
            auto clients = g_network->GetPlayerByName(_line);

            g_network->ScheduleServerThread([=]() {
                g_network->whitelist.erase(index);

                for (auto client : clients) {
                    g_network->DisconnectPlayer(*client, "Not on whitelist!");
                }
            });
        }

        printf("Removed player %s from whitelist\n", line);
        return;
    }

    case CMD_WHITELIST_REMOVE_IP: {
        g_current_cmd = CMD_NONE;

        std::string _line(line);

        auto index = std::find_if(g_network->whitelist.begin(), g_network->whitelist.end(), [&](const WhitelistEntry& entry) {
            return entry.type == WhitelistEntryType::IP && entry.value == _line;
        });

        if (index != g_network->whitelist.end()) {
            g_network->ScheduleServerThread([=]() {
                g_network->whitelist.erase(index);

                auto clients = g_network->GetClientByIP(_line);
                for (auto client : clients) {
                    g_network->DisconnectPlayer(*client, "Not on whitelist!");
                }
            });
        }

        printf("Removed player IP %s from whitelist\n", line);
        return;
    }
    }
}

int main(int argc, char **argv) {
    signal(SIGINT, +[](int s) {
        g_should_stop = 1;
    });

    if (argc > 3) {
        printf("Usage: %s [port]\n", argv[0]);
        return 1;
    }

    int port = 53000;
    if (argc >= 2) {
        port = atoi(argv[1]);
        if (port < 1 || port > 65535) {
            printf("Invalid port %d\n", port);
            return 1;
        }
    }

    const char *logfile = "ghost_log";
    if (argc >= 3) {
        logfile = argv[2];
    }

    NetworkManager network(logfile);
    g_network = &network;

    puts("Server starting up");
    if (!network.StartServer(port)) {
        printf("Failed to start server on port %d", port);
        return 1;
    }
    while (!g_should_stop) {
#ifdef _WIN32
        if (_kbhit()) {
            std::string input;
            std::getline(std::cin, input);
            if (!input.empty()) handle_cmd(input.data());
        }
        Sleep(50); // ms
#else
        struct pollfd fds[] = {
            (struct pollfd){
                .fd = STDIN_FILENO,
                .events = POLLIN,
                .revents = 0,
            },
        };

        if (poll(fds, sizeof fds / sizeof fds[0], 50) == 1 && (fds[0].revents & POLLIN)) {
            char *line = NULL;
            size_t len = 0;
            if (getline(&line, &len, stdin) != -1) handle_cmd(line);
            if (line) free(line);
        }
#endif
    }
    puts("Server shutting down");
    network.StopServer();

    return 0;
}
