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

int epollFd;
int servFd;

std::unordered_set<Client*> clients;
std::chrono::time_point<std::chrono::steady_clock> start;
std::chrono::time_point<std::chrono::steady_clock> end;
bool timeRun = false;
bool registrationAvailable = true;
bool gameRun = false;

void ctrl_c(int);

void clockRun(std::chrono::time_point<std::chrono::steady_clock> * start, std::chrono::time_point<std::chrono::steady_clock> * end, bool * registrationAvailable, bool * timeRun, bool * gameRun);

void sendToAllBut(int fd, char * buffer, int count);

void sendToAllPly(char * buffer, int count);

void sendToAllQue(char * buffer, int count);

void sendToAllCli(char * buffer, int count);

char* myStringToChar(std::string str);

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
    bool player;
    int fd() const {return _fd;}
    void handleEvent(uint32_t events);
    void myWrite(char * buffer, int count);
    void remove();
};