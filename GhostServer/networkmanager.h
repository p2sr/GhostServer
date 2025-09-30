#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <SFML/Network.hpp>

#include <vector>
#include <set>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>
#include <functional>

#ifdef GHOST_GUI
#include <QObject>
#endif

struct Vector {
    float x;
    float y;
    float z;
};

enum class HEADER {
    NONE,
    PING,
    CONNECT,
    DISCONNECT,
    STOP_SERVER,
    MAP_CHANGE,
    HEART_BEAT,
    MESSAGE,
    COUNTDOWN,
    UPDATE,
    SPEEDRUN_FINISH,
    MODEL_CHANGE,
    COLOR_CHANGE,
};

struct DataGhost {
    Vector position;
    Vector view_angle;
    float view_offset;
    bool grounded;
};

struct Color {
    uint8_t r, g, b;
};

struct Client {
    sf::Uint32 ID;
    sf::IpAddress IP;
    unsigned short int port;
    std::string name;
    DataGhost data;
    std::string modelName;
    std::string currentMap;
    std::unique_ptr<sf::TcpSocket> tcpSocket;
    bool TCP_only;
    Color color;
    uint32_t heartbeatToken;
    bool returnedHeartbeat;
    bool missedLastHeartbeat;
    bool spectator;
};

enum class WhitelistEntryType {
    NAME,
    IP,
};

struct WhitelistEntry {
    WhitelistEntryType type;
    std::string value;

    bool operator<(const WhitelistEntry& rhs) const {
        return value < rhs.value;
    }
};

#ifdef GHOST_GUI
class NetworkManager : public QObject
{
    Q_OBJECT
#else
class NetworkManager
{
#endif

private:
    bool isRunning;

    unsigned short int serverPort;
    sf::IpAddress serverIP;
    sf::TcpListener listener;
    sf::SocketSelector selector;
    int port = 0;

    sf::Uint32 lastID;

    std::thread serverThread;

    sf::Clock clock;

    void DoHeartbeats();

public:
    NetworkManager(const char *logfile = "ghost_log");
    ~NetworkManager();

    sf::UdpSocket udpSocket;
    std::vector<Client> clients;
    std::vector<sf::IpAddress> bannedIps;
    bool acceptingPlayers = true;
    bool acceptingSpectators = true;

    bool whitelistEnabled = false;
    std::set<WhitelistEntry> whitelist;

    void ScheduleServerThread(std::function<void()> func);

    std::vector<Client *> GetPlayerByName(std::string name);
    Client* GetClientByID(sf::Uint32 ID);
    std::vector<Client *> GetClientByIP(sf::IpAddress ip);

    bool StartServer(const int port);
    void StopServer();
    void RunServer();

    bool ShouldBlockConnection(const sf::IpAddress &ip);
    void DisconnectPlayer(Client &client, const char *reason);
    void StartCountdown(const std::string preCommands, const std::string postCommands, const int duration);

    void CheckConnection();
    void ReceiveUDPUpdates(std::vector<std::tuple<sf::Packet, sf::IpAddress, unsigned short>>& buffer);
    void Treat(sf::Packet& packet, sf::IpAddress ip, unsigned short udp_port);

    void BanClientIP(Client &cl);
    void ServerMessage(const char *msg);

    bool IsOnWhitelist(std::string name, sf::IpAddress IP);

#ifdef GHOST_GUI
signals:
    void OnNewEvent(QString event);
#endif


};

#endif // NETWORKMANAGER_H
