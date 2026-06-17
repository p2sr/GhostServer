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

    network->UI_EVENT("get_countdown_info");
    std::string cmd = argsL[0];
    if (cmd == "help") {
        MSG("Available chat commands:");
        MSG("  !help \t show this list");
        MSG("  !ping \t Pong!");
        if (network->countdownPostCommands != "") {
            MSG("  !ready \t toggle ready status (!r)");
        }
        MSG("  !roll \t pick a random number");
        if (network->tickerStrings.size() > 0) {
            MSG("  !ticker \t toggle ticker messages");
        }
        // MSG("  !admin \t server management command");
        return true;
    }

    if (cmd == "ping") {
        MSG("Pong! Use 'ghost_ping' to measure latency.");
        return true;
    }

    if (cmd == "ready" || cmd == "r") {
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
        size_t min = 1;
        size_t max = 100;
        if (args.size() > 3) {
            MSG("Usage: !roll [max] or !roll <min> <max>");
            return true;
        }
        if (args.size() == 2 || args.size() == 3) {
            if (args.size() == 3) {
                min = atol(args[1].c_str());
                if (min <= 0) {
                    MSG("Usage: !roll [max] or !roll <min> <max>");
                    return true;
                }
            }
            max = atol(args[args.size() - 1].c_str());
            if (max <= 1) {
                MSG("Usage: !roll [max]");
                return true;
            }
            if (max <= min) {
                MSG("Max must be greater than min.");
                return true;
            }
            if (max > RAND_MAX || max > 1000000) {
                MSG("That's just too big. Maximum is 1,000,000.");
                return true;
            }
        }
        int roll = rand() % (max - min + 1) + min;
        if (min == 1 && max == 100) {
            MSG_ALL("%s rolled a %d.", client->name.c_str(), roll);
        } else {
            MSG_ALL("%s rolled a %d from %lu-%lu.", client->name.c_str(), roll, min, max);
        }
        return false;
    }

    if (cmd == "ticker") {
        if (args.size() == 1) {
            client->tickerSub = !client->tickerSub;
        } else if (args.size() == 2) {
            std::string subcmd = argsL[1];
            if (subcmd == "on") {
                client->tickerSub = true;
            } else if (subcmd == "off") {
                client->tickerSub = false;
            } else {
                MSG("Usage: !ticker [on|off]");
                return true;
            }
        } else {
            MSG("Usage: !ticker [on|off]");
            return true;
        }
        MSG("Ticker messages %s.", client->tickerSub ? "enabled" : "disabled");
        return true;
    }

    if (cmd == "admin") {
        if (args.size() < 2) {
            MSG("Usage: !admin <login|logout|cmd>");
            return true;
        }

        std::string subcmd = argsL[1];
        if (subcmd == "login") {
            if (client->admin) {
                MSG("You are already logged in.");
                return true;
            }
            if (args.size() != 4) {
                MSG("Usage: !admin login <username> <password>");
                return true;
            }
            std::string username = args[2];
            std::string password = args[3];
            if (network->IsAdminCredential(username, password)) {
                network->Log("[admin] " + username + " logged in");
                MSG("Admin login successful.");
                client->admin = true;
            } else {
                network->Log("[admin] failed login attempt for username: " + username);
                MSG("Admin login failed. It's case sensitive.");
            }
            return true;
        } else if (subcmd == "logout") {
            if (!client->admin) {
                MSG("You are not logged in.");
                return true;
            }
            client->admin = false;
            network->Log("[admin] " + client->name + " logged out");
            MSG("Admin logout successful.");
            return true;
        } else if (subcmd == "cmd") {
            if (args.size() < 3) {
                MSG("Usage: !admin cmd <command>");
                return true;
            }
            if (!client->admin) {
                MSG("You are not logged in.");
                return true;
            }
            std::string command = argsR[1];
            network->Log("[admin] cmd: " + command);
            handle_cmd(network, const_cast<char*>(command.c_str()));
            MSG("Executed command: '%s'", command.c_str());
            return true;
        }
        MSG("Usage: !admin <login|logout|cmd>");
        return true;
    }

    MSG("Unknown command: !%s. Type !help for a list of commands.", cmd.c_str());
    return false;
}
