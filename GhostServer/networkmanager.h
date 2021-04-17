#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <SFML/Network.hpp>

#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>

#include <QObject>

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
    MODEL_CHANGE
};

struct DataGhost {
    Vector position;
    Vector view_angle;
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
    uint32_t heartbeatToken;
    bool returnedHeartbeat;
    bool missedLastHeartbeat;
};

class NetworkManager : public QObject
{
    Q_OBJECT

private:

    bool isRunning;

    unsigned short int serverPort;
    sf::IpAddress serverIP;
    sf::TcpListener listener;
    sf::SocketSelector selector;
	int port = 0;

    sf::Uint32 lastID;

    int updateRate;

	std::thread serverThread;

	sf::Clock clock;

    void DoHeartbeats();

public:
	sf::UdpSocket udpSocket;
    NetworkManager();
	std::vector<Client> clients;

    Client* GetClientByID(sf::Uint32 ID);

    bool StartServer(const int port);
	void StopServer();
	void RunServer();

    bool IsAlreadyConnected(const sf::IpAddress &ip);
    void DisconnectPlayer(Client &client);
    void DisconnectPlayer(sf::Uint32 ID);
    std::vector<sf::Uint32> GetPlayerIDByName(std::string name);
    void StartCountdown(const std::string preCommands, const std::string postCommands, const int duration);

	void CheckConnection();
	void ReceiveUDPUpdates(std::vector<std::pair<unsigned short, sf::Packet>>& buffer);
	void TreatUDP(std::vector<std::pair<unsigned short, sf::Packet>>& buffer);
	void TreatTCP(sf::Packet& packet);

signals:
    void OnNewEvent(QString event);


};

#endif // NETWORKMANAGER_H
