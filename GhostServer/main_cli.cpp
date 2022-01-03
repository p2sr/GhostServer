#include "networkmanager.h"
#include <signal.h>
#include <unistd.h>

static volatile int g_should_stop = 0;

int main(int argc, char **argv) {
	NetworkManager nm{};

	signal(SIGINT, +[](int s) {
		g_should_stop = 1;
	});

	puts("Server starting up");
	nm.StartServer(53000);
	while (!g_should_stop) usleep(50000);
	puts("Server shutting down");
	nm.StopServer();

	return 0;
}
