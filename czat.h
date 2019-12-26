#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <unordered_set>
#include <signal.h>
#include <chrono>

class Client;
class Player;
class Queuer;

int epollFd;
int servFd;

std::unordered_set<Client*> clients;
std::unordered_set<Player*> players;
std::unordered_set<Queuer*> queuers;
std::chrono::time_point<std::chrono::steady_clock> start;
std::chrono::time_point<std::chrono::steady_clock> end;
bool timeRun = false;
bool registrationAvailable = true;

void ctrl_c(int);

void sendToAllPly(char * buffer);

void sendToAllQue(char * buffer);

void sendToAllCli(char * buffer);

uint16_t readPort(char * txt);

void setReuseAddr(int sock);

struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client {
    
public:
    int _fd;
    Client(int fd);
    Client();
    ~Client();
    int fd() const {return _fd;}
    virtual void handleEvent(uint32_t events);
    void myWrite(char * buffer, int count);
    void remove();
};

class Player : public Client {
    public:
    int _fd;
    Player(int fd);
    int fd() const {return _fd;}
    void myWrite(char * buffer, int count);
    void remove();

};

class Queuer : public Client {
    public:
    int _fd;
    Queuer(int fd);
    int fd() const {return _fd;}
    void myWrite(char * buffer, int count);
    void remove();

};