#include "chatcommands.h"

#include "commands.h"
#include "networkmanager.h"

#include <string>

#include <cstdlib>

#ifdef _WIN32
# include <Windows.h>
# define strdup _strdup
# define strcasecmp _stricmp
#else
# include <strings.h>
#endif

#define MSG(x, ...) network->ServerMessage(playerID, ssprintf(x, ##__VA_ARGS__))
#define MSG_ALL(x, ...) network->ServerMessage(ssprintf(x, ##__VA_ARGS__))

bool handle_chat_cmd(NetworkManager *network, sf::Uint32 playerID, const std::string message) {
    if (message.empty() || message[0] != '!') return false;
    Client *client = network->GetClientByID(playerID);
    if (!client) return false;
    std::string _message(message);
    _message = _message.substr(1);
    std::vector<std::string> args;
    std::vector<std::string> argsL; // lower
    std::vector<std::string> argsR; // remaining
    for (size_t i = 0; i < _message.size(); ) {
        while (i < _message.size() && isspace(_message[i])) ++i;
        if (i >= _message.size()) break;

        size_t start = i;
        if (_message[i] == '"') {
            ++i;
            start = i;
            while (i < _message.size() && _message[i] != '"') ++i;
        } else {
            while (i < _message.size() && !isspace(_message[i])) ++i;
        }
        std::string arg = _message.substr(start, i - start);
        args.push_back(arg);
        for (auto &c : arg) c = tolower(c);
        argsL.push_back(arg);
        if (i < _message.size() && _message[i] == '"') ++i;
        while (i < _message.size() && isspace(_message[i])) ++i;
        argsR.push_back(_message.substr(i));
    }
    if (args.empty()) return false;

    std::string cmd = argsL[0];
    if (cmd == "help") {
        MSG("Available chat commands:");
        MSG("  !help  \t show this list");
        MSG("  !ping  \t Pong!");
        MSG("  !ready \t toggle ready status (!r)");
        MSG("  !roll  \t roll a die (1-100 or !roll <max>)");
        MSG("  !admin \t server management command");
        return true;
    }

    if (cmd == "ping") {
        MSG("Pong! Use 'ghost_ping' to measure latency.");
        return true;
    }

    if (cmd == "ready" || cmd == "r") {
        network->UI_EVENT("get_countdown_info");
        if (network->countdownPostCommands == "") {
            MSG("Countdown is not enabled.");
            return true;
        }
        bool ready = !client->ready;
        MSG_ALL("%s is %sready!", client->name.c_str(), ready ? "" : "not ");
        network->SetReady(playerID, ready);
        return true;
    }

    if (cmd == "roll") {
        size_t max = 100;
        if (args.size() == 2) {
            max = atol(args[1].c_str());
            if (max <= 1) {
                MSG("Usage: !roll [max]");
                return true;
            }
            if (max > RAND_MAX || max > 1000000) {
                MSG("That's just too big. Maximum is 1,000,000.");
                return true;
            }
        }
        int roll = rand() % max + 1;
        if (max == 100) {
            MSG_ALL("%s rolled a %d.", client->name.c_str(), roll);
        } else {
            MSG_ALL("%s rolled a %d out of %lu.", client->name.c_str(), roll, max);
        }
        return false;
    }

    if (cmd == "admin") {
        if (args.size() < 3) {
            MSG("Usage: !admin <password> <command>");
            return true;
        }
        std::string password = args[1];
        std::string command = argsR[1];
        if (network->IsAdmin(playerID, password)) {
            network->Log("[admin] cmd: " + command);
            handle_cmd(network, const_cast<char*>(command.c_str()));
            MSG("Executed admin command: '%s'", command.c_str());
        } else {
            network->Log("[admin] reject: incorrect credentials");
            MSG("Admin command rejected.");
        }
        return true;
    }

    MSG("Unknown command: !%s. Type !help for a list of commands.", cmd.c_str());
    return false;
}
