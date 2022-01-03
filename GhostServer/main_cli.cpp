#include "networkmanager.h"
#include <signal.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <string>

static volatile int g_should_stop = 0;

static enum {
	CMD_NONE,
	CMD_COUNTDOWN_SET,
	CMD_DISCONNECT,
	CMD_DISCONNECT_ID,
} g_current_cmd = CMD_NONE;

static char *g_entered_pre;
static char *g_entered_post;
static int g_countdown_duration = -1;

static NetworkManager g_network;

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
			puts("  help             show this list");
			puts("  quit             terminate the server");
			puts("  list             list all the currently connected clients");
			puts("  countdown_set    set the pre/post cmds and countdown duration");
			puts("  countdown        start a countdown");
			puts("  disconnect       disconnect a client by name");
			puts("  disconnect_id    disconnect a client by ID");
			return;
		}

		if (!strcmp(line, "list")) {
			if (g_network.clients.empty()) {
				puts("No clients");
			} else {
				puts("Clients:");
				for (auto &cl : g_network.clients) {
					printf("  %-3d %s @ %s:%d\n", cl.ID, cl.name.c_str(), cl.IP.toString().c_str(), (int)cl.port);
				}
			}
			return;
		}

		if (!strcmp(line, "countdown")) {
			if (g_countdown_duration == -1) {
				puts("Set a countdown first using countdown_set.");
				return;
			}
			g_network.StartCountdown(std::string(g_entered_pre), std::string(g_entered_post), g_countdown_duration);
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
		if (len == 0) return;
		{
			auto players = g_network.GetPlayerIDByName(std::string(line));
			for (auto id : players) g_network.DisconnectPlayer(id);
		}
		printf("Disconnected player '%s'\n", line);
		return;

	case CMD_DISCONNECT_ID:
		g_current_cmd = CMD_NONE;
		if (len == 0) return;
		{
			int id = atoi(line);
			g_network.DisconnectPlayer(id);
			printf("Disconnected player ID %d\n", id);
		}
		return;
	}
}

int main(int argc, char **argv) {
	signal(SIGINT, +[](int s) {
		g_should_stop = 1;
	});

	puts("Server starting up");
	g_network.StartServer(53000);
	while (!g_should_stop) {
		struct pollfd fds[] = {
			(struct pollfd){
				.fd = STDIN_FILENO,
				.events = POLLIN,
				.revents = 0,
			},
		};

		if (poll(fds, sizeof fds / sizeof fds[0], 50) == 1) {
			if (fds[0].revents & POLLIN) {
				char *line = NULL;
				size_t len = 0;
				if (getline(&line, &len, stdin) != -1) handle_cmd(line);
				if (line) free(line);
			}
		}
	}
	puts("Server shutting down");
	g_network.StopServer();

	return 0;
}
