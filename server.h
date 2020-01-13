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
#include <fstream>
#include <chrono>


class Client;

int epollFd;
int servFd;

std::unordered_set<Client*> clients;
std::chrono::time_point<std::chrono::steady_clock> start;
std::chrono::time_point<std::chrono::steady_clock> end;
std::fstream fileWithCodes;
bool timeRun = false;
bool registrationAvailable = true;
bool gameRun = false;
int numberOfRound = 1;
int numberOfPlayers = 0;
int numberOfClues = 0;

std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> haslo(0,numberOfClues);

void ctrl_c(int);

void clockRun(std::chrono::time_point<std::chrono::steady_clock> * start, std::chrono::time_point<std::chrono::steady_clock> * end, bool * registrationAvailable, bool * timeRun, bool * gameRun, int * numberOfRound);

void sendToAllBut(int fd, char * buffer, int count);

void sendToAllPly(char * buffer, int count);

void sendToAllQue(char * buffer, int count);

void sendToAllCli(char * buffer, int count);

void sendNumberOfPlayers();

void sendClueToPlayers();

char* myStringToChar(std::string str);

void mySendInt(int numb);

void addQueuersToGame();

uint16_t readPort(char * txt);

void setReuseAddr(int sock);

std::fstream& GotoLine(std::fstream& file, int num);

struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client {
    
public:
    static int numberOfPlayers;
    int _fd;
    Client(int fd);
    Client();
    virtual ~Client();
    bool player;
    int fd() const {return _fd;}
    virtual void handleEvent(uint32_t events);
    void myWrite(char * buffer, int count);
    void remove();
};

int Client::numberOfPlayers = 0;