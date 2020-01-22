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
#include <mutex>
#include <condition_variable>
#include <chrono>


int epollFd;
int servFd;

std::timed_mutex s30;
std::condition_variable condForTime;
std::mutex mutexForTime;
int forLocker = false;

std::unique_lock<std::mutex> locker;

struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client : public Handler {
    
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

std::unordered_set<Client*> clients;
std::chrono::time_point<std::chrono::steady_clock> start;
std::chrono::time_point<std::chrono::steady_clock> end;
std::fstream fileWithCodes;
bool afterStart = false;
bool endOfRound = true;
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

void clockRunStart();

void clockRunRegistration();

void clockRunGap();

void clockRunGame();

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