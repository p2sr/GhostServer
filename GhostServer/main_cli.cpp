#include "commands.h"
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

static NetworkManager *g_network;

int main(int argc, char **argv) {
    signal(SIGINT, +[](int s) {
        g_should_stop = 1;
    });

    if (argc > 3 || (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {
        printf("Usage: %s [port] [logfile]\n", argv[0]);
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
        printf("Failed to start server on port %d\n", port);
        return 1;
    }
    while (!g_should_stop) {
#ifdef _WIN32
        if (_kbhit()) {
            std::string input;
            std::getline(std::cin, input);
            if (!input.empty()) handle_cmd(g_network, input.data());
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
            if (getline(&line, &len, stdin) != -1) handle_cmd(g_network, line);
            if (line) free(line);
        }
#endif
    }
    puts("Server shutting down");
    network.StopServer();

    return 0;
}
