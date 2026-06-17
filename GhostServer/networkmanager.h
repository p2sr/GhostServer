#pragma once

#include <SFML/Network.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

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
    TAUNT,
    LOCATOR,
    VOICE,
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
    std::chrono::time_point<std::chrono::steady_clock> lastLocator;
    bool ready = false;
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
    unsigned short int serverPort;
    sf::IpAddress serverIP;
    sf::TcpListener listener;
    sf::SocketSelector selector;
    int port = 0;

    sf::Uint32 lastID;

    std::thread serverThread;

    sf::Clock clock;

    std::map<int, std::tuple<std::string, std::function<void()>>> uiEventCallbacks;
    int uiEventCallbackId = 1;

    void DoHeartbeats();

    std::string adminUsername;
    std::string adminPassword;

public:
    NetworkManager(const char *logfile = "ghost_log.log");
    ~NetworkManager();

    sf::UdpSocket udpSocket;
    std::vector<Client> clients;
    bool isRunning = false;
    bool acceptingPlayers = true;
    bool acceptingSpectators = true;
    bool alwaysListClients = false;
    
    bool whitelistEnabled = false;
    std::set<WhitelistEntry> whitelist;
    std::set<sf::IpAddress> bannedIps;
    
    std::vector<std::string> tickerStrings;
    size_t tickerIndex = 0;
    size_t tickerIntervalMs = 300000; // 5 minutes
    std::string countdownPreCommands = "";
    std::string countdownPostCommands = "";
    int countdownDuration = 3;

    void ScheduleServerThread(std::function<void()> func);

    const std::vector<Client *> GetClients();
    std::vector<Client *> GetPlayerByName(const std::string name);
    Client* GetClientByID(const sf::Uint32 ID);
    std::vector<Client *> GetClientByIP(const sf::IpAddress ip);

    bool StartServer(const int port);
    void StopServer();
    void RunServer();

    bool ShouldBlockConnection(const sf::IpAddress &ip);
    void DisconnectPlayer(Client &client, const std::string reason);
    void StartCountdown(const std::string preCommands, const std::string postCommands, const int duration);
    void StartCountdown() { StartCountdown(this->countdownPreCommands, this->countdownPostCommands, this->countdownDuration); }
    void SetAccept(bool players, bool accept);
    void SetAlwaysListClients(bool alwaysList);

    void CheckConnection();
    void ReceiveUDPUpdates(std::vector<std::tuple<sf::Packet, sf::IpAddress, unsigned short>>& buffer);
    void Treat(sf::Packet& packet, sf::IpAddress ip, unsigned short udp_port);

    void BanClientIP(sf::IpAddress ip);

    void ServerMessage(std::string msg);
    void ServerMessage(sf::Uint32 playerID, std::string msg);
    void SendPacket(sf::Packet& packet);
    void SendPacket(sf::Uint32 playerID, sf::Packet& packet);
    void SendPacketExclude(sf::Uint32 playerID, sf::Packet& packet);
    void SendPacketMap(std::string mapName, sf::Packet& packet);
    void SendPacketMapExclude(std::string mapName, sf::Uint32 playerID, sf::Packet& packet);

    void ListClients();
    bool IsOnWhitelist(std::string name, sf::IpAddress IP);
    void SetAdminUsername(std::string username);
    void SetAdminPassword(std::string password);
    bool IsAdmin(sf::Uint32 playerID, std::string password);
    void SetReady(sf::Uint32 playerID, bool ready);

    void Log(const std::string& message);
    void UI_EVENT(std::string event);
    int RegisterEventCallback(std::string type, std::function<void()> callback);
    void UnregisterEventCallback(int id) { uiEventCallbacks.erase(id); }

#ifdef GHOST_GUI
signals:
    void OnNewEvent(QString log);
    void UIEvent(std::string event);
#endif
};
