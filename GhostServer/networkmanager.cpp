#include "networkmanager.h"

#include <memory>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <queue>

#include <SFML/Network.hpp>

static FILE *g_logFile;

static void file_log(std::string str) {
    if (g_logFile) {
        time_t now = time(NULL);
        char buf[sizeof "2000-01-01T00:00:00Z"];
        strftime(buf, sizeof buf, "%FT%TZ", gmtime(&now));
        fprintf(g_logFile, "[%s] %s\n", buf, str.c_str());
        fflush(g_logFile);
    }
}

#ifdef GHOST_GUI
# include <QVector>
# define GHOST_LOG(x) (file_log(x), emit this->OnNewEvent(QString::fromStdString(x)))
#else
# include <stdio.h>
# define GHOST_LOG(x) (file_log(x), printf("[LOG] %s\n", std::string(x).c_str()))
#endif

#define HEARTBEAT_RATE 5000
#define HEARTBEAT_RATE_UDP 1000 // We don't actually respond to these, they're just to keep the connection alive
#define UPDATE_RATE 50
#define CONNECT_TIMEOUT 1500

#ifdef _WIN32
# define strcasecmp _stricmp
#else
#	include <strings.h>
#endif

static std::chrono::time_point<std::chrono::steady_clock> lastHeartbeat;
static std::chrono::time_point<std::chrono::steady_clock> lastHeartbeatUdp;
static std::chrono::time_point<std::chrono::steady_clock> lastUpdate;

// DataGhost

sf::Packet& operator>>(sf::Packet& packet, Vector& vec)
{
    return packet >> vec.x >> vec.y >> vec.z;
}

sf::Packet& operator<<(sf::Packet& packet, const Vector& vec)
{
    return packet << vec.x << vec.y << vec.z;
}

sf::Packet& operator>>(sf::Packet& packet, DataGhost& dataGhost)
{
    uint8_t data;
    auto &ret = packet >> dataGhost.position >> dataGhost.view_angle >> data;
    dataGhost.view_offset = (float)(data & 0x7F);
    dataGhost.grounded = (data & 0x80) != 0;
    return ret;
}
sf::Packet& operator<<(sf::Packet& packet, const DataGhost& dataGhost)
{
    uint8_t data = ((int)dataGhost.view_offset & 0x7F) | (dataGhost.grounded ? 0x80 : 0x00);
    return packet << dataGhost.position << dataGhost.view_angle << data;
}

// HEADER

sf::Packet& operator>>(sf::Packet& packet, HEADER& header)
{
    sf::Uint8 tmp;
    packet >> tmp;
    header = static_cast<HEADER>(tmp);
    return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const HEADER& header)
{
    return packet << static_cast<sf::Uint8>(header);
}

// Color

sf::Packet& operator>>(sf::Packet& packet, Color &col)
{
    return packet >> col.r >> col.g >> col.b;
}

sf::Packet& operator<<(sf::Packet& packet, const Color &col)
{
    return packet << col.r << col.g << col.b;
}

NetworkManager::NetworkManager(const char *logfile)
    : isRunning(false)
    , serverPort(53000)
    , serverIP("localhost")
    , lastID(1) // 0 == server
{
    g_logFile = logfile ? fopen(logfile, "w") : NULL;
}

NetworkManager::~NetworkManager() {
    if (g_logFile) fclose(g_logFile);
}

static std::mutex g_server_queue_mutex;
static std::vector<std::function<void()>> g_server_queue;

void NetworkManager::ScheduleServerThread(std::function<void()> func) {
    g_server_queue_mutex.lock();
    g_server_queue.push_back(func);
    g_server_queue_mutex.unlock();
}

std::vector<Client *> NetworkManager::GetPlayerByName(std::string name)
{
    std::vector<Client *> matches;
    for (auto &client : this->clients) {
        if (client.name == name) {
            matches.push_back(&client);
        }
    }

    return matches;
}

Client* NetworkManager::GetClientByID(sf::Uint32 ID)
{
    for (auto& client : this->clients) {
        if (client.ID == ID) {
            return &client;
        }
    }

    return nullptr;
}

std::vector<Client *> NetworkManager::GetClientByIP(sf::IpAddress ip) {
    std::vector<Client *> clients;
    for (auto& client : this->clients) {
        if (client.IP == ip) {
            clients.push_back(&client);
        }
    }
    return clients;
}

bool NetworkManager::StartServer(const int port)
{
    if (this->isRunning) {
        GHOST_LOG("Server is already running!");
        return false;
    }

    if (this->udpSocket.bind(port) != sf::Socket::Done) {
        GHOST_LOG("Failed to bind UDP socket to port " + std::to_string(port) + ".");
        this->udpSocket.unbind();
        this->listener.close();
        return false;
    }

    if (this->listener.listen(port) != sf::Socket::Done) {
        GHOST_LOG("Failed to bind listener to port " + std::to_string(port) + ".");
        this->udpSocket.unbind();
        this->listener.close();
        return false;
    }

    this->serverPort = port;
    this->udpSocket.setBlocking(false);

    this->selector.add(this->listener);

    this->serverThread = std::thread(&NetworkManager::RunServer, this);
    this->serverThread.detach();

    GHOST_LOG("Server started on " + this->serverIP.toString() + " (public IP: " + sf::IpAddress::getPublicAddress().toString() + ") on port " + std::to_string(this->serverPort));
    GHOST_LOG("Enter 'help' for a list of commands.");

    return true;
}

void NetworkManager::StopServer()
{
    if (this->isRunning) {
        this->isRunning = false;
        return;
    }

    while (this->clients.size() > 0) {
        this->DisconnectPlayer(this->clients.back(), "server stopped");
    }

    this->isRunning = false;
    this->clients.clear();
    this->udpSocket.unbind();
    this->listener.close();

    GHOST_LOG("Server stopped!");
}

void NetworkManager::DisconnectPlayer(Client& c, const char *reason)
{
    sf::Packet packet;
    packet << HEADER::DISCONNECT << c.ID;
    int id = 0;
    int toErase = -1;
    for (; id < this->clients.size(); ++id) {
        auto& client = this->clients[id];
        if (client.IP != c.IP) {
            client.tcpSocket->send(packet);
        } else {
            GHOST_LOG("Disconnect: " + client.name + " (" + (client.spectator ? "spectator" : "player") + ") @ " + client.IP.toString() + ":" + std::to_string(client.port) + " Reason: " + reason);
            this->selector.remove(*client.tcpSocket);
            client.tcpSocket->disconnect();
            toErase = id;
        }
    }

    if (toErase != -1) {
        this->clients.erase(this->clients.begin() + toErase);
    }
}

void NetworkManager::StartCountdown(const std::string preCommands, const std::string postCommands, const int duration)
{
    if (!this->isRunning) {
        GHOST_LOG("Server is not running!");
        return;
    }
    GHOST_LOG("Countdown starting: " + std::to_string(duration) + " seconds");
    GHOST_LOG("Pre-command: " + preCommands);
    GHOST_LOG("Post-command: " + postCommands);
    sf::Packet packet;
    packet << HEADER::COUNTDOWN << sf::Uint32(0) << sf::Uint8(0) << sf::Uint32(duration) << preCommands << postCommands;
    for (auto& client : this->clients) {
        client.tcpSocket->send(packet);
    }
    if (true) { // TODO: Make this a setting?
        this->acceptingPlayers = false;
        GHOST_LOG("Now refusing connections from players");
    }
}

bool NetworkManager::ShouldBlockConnection(const sf::IpAddress& ip)
{
    if (std::find_if(this->clients.begin(), this->clients.end(), [&ip](const Client& c) { return ip == c.IP; }) != this->clients.end()) {
        return true;
    }

    if (this->bannedIps.find(ip) != this->bannedIps.end()) {
        return true;
    }

    return false;
}

void NetworkManager::CheckConnection()
{
    Client client;
    client.tcpSocket = std::make_unique<sf::TcpSocket>();

    if (this->listener.accept(*client.tcpSocket) != sf::Socket::Done) {
        GHOST_LOG("Failed to accept connection");
        return;
    }
    client.IP = client.tcpSocket->getRemoteAddress();
    
    if (this->ShouldBlockConnection(client.IP)) {
        GHOST_LOG("Refused connection from " + client.IP.toString() + " - banned or IP already connected");
        return;
    }

    sf::Packet connection_packet;
    sf::SocketSelector conn_selector;
    conn_selector.add(*client.tcpSocket);
    if (!conn_selector.wait(sf::milliseconds(CONNECT_TIMEOUT))) {
        GHOST_LOG("Connection timeout from " + client.IP.toString());
        return;
    }

    client.tcpSocket->receive(connection_packet);

    HEADER header;
    unsigned short int port;
    std::string name;
    DataGhost data;
    std::string model_name;
    std::string level_name;
    bool TCP_only;
    Color col;
    bool spectator;

    connection_packet >> header >> port >> name >> data >> model_name >> level_name >> TCP_only >> col >> spectator;

    if (!(spectator ? this->acceptingSpectators : this->acceptingPlayers)) {
        GHOST_LOG("Refused connection from " + name + " (" + (spectator ? "spectator" : "player") + ") @ " + client.IP.toString() + ":" + std::to_string(port) + " - not accepting this type");
        return;
    }

    if (whitelistEnabled) {
        if (!IsOnWhitelist(name, client.IP)) {
            GHOST_LOG("Refused connection from " + name + " (" + (spectator ? "spectator" : "player") + ") @ " + client.IP.toString() + ":" + std::to_string(port) + " - not on whitelist");
            return;
        }
    }

    client.ID = this->lastID++;
    client.IP = client.tcpSocket->getRemoteAddress();
    client.port = port;
    client.name = name;
    client.data = data;
    client.modelName = model_name;
    client.currentMap = level_name;
    client.TCP_only = TCP_only;
    client.color = col;
    client.returnedHeartbeat = true; // Make sure they don't get immediately disconnected; their heartbeat starts on next beat
    client.missedLastHeartbeat = false;
    client.spectator = spectator; // People can break the run when joining in the middle of a run

    this->selector.add(*client.tcpSocket);

    sf::Packet packet_new_client;

    packet_new_client << client.ID; // Send Client's ID
    packet_new_client << sf::Uint32(this->clients.size()); // Send every players informations
    for (auto& c : this->clients) {
        packet_new_client << c.ID << c.name.c_str() << c.data << c.modelName.c_str() << c.currentMap.c_str() << c.color << c.spectator;
    }

    client.tcpSocket->send(packet_new_client);

    sf::Packet packet_notify_all; // Notify every players of a new connection
    packet_notify_all << HEADER::CONNECT << client.ID << client.name.c_str() << client.data << client.modelName.c_str() << client.currentMap.c_str() << client.color << client.spectator;

    for (auto& c : this->clients) {
        c.tcpSocket->send(packet_notify_all);
    }

    GHOST_LOG("Connection: " + client.name + " (" + (client.spectator ? "spectator" : "player") + ") @ " + client.IP.toString() + ":" + std::to_string(client.port));

    this->clients.push_back(std::move(client));
}

void NetworkManager::ReceiveUDPUpdates(std::vector<std::tuple<sf::Packet, sf::IpAddress, unsigned short>>& buffer)
{
    sf::Socket::Status status;
    do {
        sf::Packet packet;
        sf::IpAddress ip;
        unsigned short int port;
        status = this->udpSocket.receive(packet, ip, port);
        if (status == sf::Socket::Done) {
            buffer.push_back({ packet, ip, port });
        }
    } while (status == sf::Socket::Done);
}

#define SEND_TO_OTHERS(packet) \
    for (auto& other : this->clients) { \
        if (other.ID != ID) { \
            other.tcpSocket->send(packet); \
        } \
    }

#define BROADCAST(packet) \
    for (auto& other : this->clients) { \
        other.tcpSocket->send(packet); \
    }

void NetworkManager::Treat(sf::Packet& packet, sf::IpAddress ip, unsigned short udp_port)
{
    HEADER header;
    sf::Uint32 ID;
    packet >> header >> ID;

    // Prevent impersonation
    auto client = this->GetClientByID(ID);
    if (!client || client->IP != ip) {
        return;
    }

    if (udp_port != 0) {
        client->port = udp_port;
    }

    switch (header) {
    case HEADER::NONE:
        break;
    case HEADER::PING: {
        sf::Packet ping_packet;
        ping_packet << HEADER::PING;
        client->tcpSocket->send(ping_packet);
        break;
    }
    case HEADER::DISCONNECT: {
        this->DisconnectPlayer(*client, "requested");
        break;
    }
    case HEADER::STOP_SERVER:
        // this->StopServer();
        break;
    case HEADER::MAP_CHANGE: {
        std::string map;
        packet >> map;
        client->currentMap = map;
        GHOST_LOG(client->name + " is now on " + map);
        SEND_TO_OTHERS(packet);
        break;
    }
    case HEADER::HEART_BEAT: {
        uint32_t token;
        packet >> token;
        if (token == client->heartbeatToken) {
            // Good heartbeat!
            client->returnedHeartbeat = true;
        }
        break;
    }
    case HEADER::MESSAGE: {
        std::string message;
        packet >> message;
        GHOST_LOG("[message] " + client->name + ": " + message);
        SEND_TO_OTHERS(packet);
        break;
    }
    case HEADER::COUNTDOWN: {
        sf::Packet packet_confirm;
        packet_confirm << HEADER::COUNTDOWN << sf::Uint32(0) << sf::Uint8(1);
        client->tcpSocket->send(packet_confirm);
        break;
    }
    case HEADER::SPEEDRUN_FINISH: {
        SEND_TO_OTHERS(packet);
        break;
    }
    case HEADER::MODEL_CHANGE: {
        std::string modelName;
        packet >> modelName;
        client->modelName = modelName;
        SEND_TO_OTHERS(packet);
        break;
    }
    case HEADER::COLOR_CHANGE: {
        Color col;
        packet >> col;
        client->color = col;
        SEND_TO_OTHERS(packet);
        break;
    }
    case HEADER::UPDATE: {
        DataGhost data;
        packet >> data;
        client->data = data;
        break;
    }
    default:
        break;
    }
}

void NetworkManager::BanClientIP(sf::IpAddress ip) {
    if (ip == sf::IpAddress::None) return;
    for (auto cl : this->GetClientByIP(ip)) {
        this->DisconnectPlayer(*cl, "banned");
    }
    if (this->bannedIps.find(ip) != this->bannedIps.end()) return;
    this->bannedIps.insert(ip);
    GHOST_LOG("Banned IP " + ip.toString());
}

void NetworkManager::ServerMessage(const char *msg) {
    GHOST_LOG(std::string("[server message] ") + msg);
    sf::Packet packet;
    packet << HEADER::MESSAGE << sf::Uint32(0) << msg;
    for (auto &client : this->clients) {
        client.tcpSocket->send(packet);
    }
}

// Threaded function
void NetworkManager::RunServer()
{
    this->isRunning = true;
    this->clock.restart();

    while (this->isRunning) {
        auto now = std::chrono::steady_clock::now();
        if (now > lastHeartbeat + std::chrono::milliseconds(HEARTBEAT_RATE)) {
            this->DoHeartbeats();
            lastHeartbeat = now;
        }

        if (now > lastHeartbeatUdp + std::chrono::milliseconds(HEARTBEAT_RATE_UDP)) {
            for (auto &client : this->clients) {
                if (!client.TCP_only) {
                    sf::Packet packet;
                    packet << HEADER::HEART_BEAT << sf::Uint32(client.ID) << sf::Uint32(0);
                    this->udpSocket.send(packet, client.IP, client.port);
                }
            }
            lastHeartbeatUdp = now;
        }

        if (now > lastUpdate + std::chrono::milliseconds(UPDATE_RATE)) {
            // Send bulk update packet
            sf::Packet packet;
            packet << HEADER::UPDATE << sf::Uint32(0) << sf::Uint32(this->clients.size());
            for (auto &client : this->clients) {
                packet << sf::Uint32(client.ID) << client.data;
            }
            for (auto &client : this->clients) {
                if (client.TCP_only) {
                    client.tcpSocket->send(packet);
                } else {
                    this->udpSocket.send(packet, client.IP, client.port);
                }
            }
            lastUpdate = now;
        }

        // UDP
        std::vector<std::tuple<sf::Packet, sf::IpAddress, unsigned short>> buffer;
        this->ReceiveUDPUpdates(buffer);
        for (auto [packet, ip, port] : buffer) {
            this->Treat(packet, ip, port);
        }

        if (this->selector.wait(sf::milliseconds(UPDATE_RATE))) { // If a packet is received
            if (this->selector.isReady(this->listener)) {
                this->CheckConnection(); // A player wants to connect
            } else {
                for (int i = 0; i < this->clients.size(); ++i) {
                    if (this->selector.isReady(*this->clients[i].tcpSocket)) {
                        sf::Packet packet;
                        sf::Socket::Status status = this->clients[i].tcpSocket->receive(packet);
                        if (status == sf::Socket::Disconnected) {
                            this->DisconnectPlayer(this->clients[i], "socket died");
                            --i;
                            continue;
                        }
                        this->Treat(packet, this->clients[i].IP, 0);
                    }
                }
            }
        }

        g_server_queue_mutex.lock();
        for (auto &f : g_server_queue) {
            f();
        }
        g_server_queue.clear();
        g_server_queue_mutex.unlock();
    }

    this->StopServer();
}

void NetworkManager::DoHeartbeats()
{
    // We don't disconnect clients in the loop; else, the loop will have
    // UB
    for (size_t i = 0; i < this->clients.size(); ++i) {
        auto &client = this->clients[i];
        if (!client.returnedHeartbeat && client.missedLastHeartbeat) {
            // Client didn't return heartbeat in time; sever connection
            this->DisconnectPlayer(client, "missed two heartbeats");
            --i;
        } else {
            // Send a heartbeat
            client.heartbeatToken = rand();
            client.missedLastHeartbeat = !client.returnedHeartbeat;
            client.returnedHeartbeat = false;
            sf::Packet packet;
            packet << HEADER::HEART_BEAT << sf::Uint32(client.ID) << sf::Uint32(client.heartbeatToken);
            if (client.tcpSocket->send(packet) == sf::Socket::Disconnected) {
                this->DisconnectPlayer(client, "socket died");
                --i;
            }
        }
    }
}

bool NetworkManager::IsOnWhitelist(std::string name, sf::IpAddress IP) {
    if (whitelist.empty()) return false;

    auto index = std::find_if(whitelist.begin(), whitelist.end(), [&name, &IP](const WhitelistEntry& entry) {
        switch (entry.type) {
        case WhitelistEntryType::NAME:
            return strcasecmp(entry.value.c_str(), name.c_str()) == 0;
        case WhitelistEntryType::IP:
            return entry.value == IP.toString();
        default:
            return false;
        }
    });

    return index != whitelist.end();
}
