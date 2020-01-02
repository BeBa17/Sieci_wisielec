#include <cstdlib>
#include <cstdio>
#include <iostream>
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
#include <string>
#include <cstring>
#include <limits>
#include <random>
#include <fstream>
#include <thread>
#include "czat.h"

#define TIME_FOR_REGISTRATION 30
#define TIME_GAP 5
#define TIME_FOR_GAME 30

Client::Client(int fd) : _fd(fd) {
    epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);

    ::write(fd,"Welcome\n", std::strlen("Welcome\n"));

    if(timeRun == false) { timeRun = true; }

    if(registrationAvailable == true)
    {
        this->player = true;
        Client::numberOfPlayers++;
        
        char duration[10];
        sprintf(duration, "%ld", std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
        this->myWrite(duration, 2);}
    else{
        this->player = false;
        ::write(fd,"Please wait for a next round\n", std::strlen("Please wait for a next round\n"));}
}
Client::~Client(){
    if(this->player==true)
    {
        Client::numberOfPlayers--;
    }
    epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    shutdown(_fd, SHUT_RDWR);
    close(_fd);
}
Client::Client(){
    
}
void Client::handleEvent(uint32_t events){
    if(gameRun == false)
        return;
    if(events & EPOLLIN) {
        char buffer[256];
        ssize_t count = read(_fd, buffer, 256);
        if(count > 0){
            std::string buff(buffer);
            std::string odp(buff.substr(0,2));
            std::cout<<odp;
            std::cout<<this->_fd;
            if(odp == "00"){
                printf("Przegrana");}
                else
                {printf("Wygrana");}
            }
    }
    else
        events |= EPOLLERR;
    if(events & ~EPOLLIN){
        remove();
    }
}
void Client::myWrite(char * buffer, int count){
    if(count != ::write(_fd, buffer, count))
        remove();   
}
void Client::remove() {
    printf("removing %d\n", _fd);
    clients.erase(this);
    delete this;
}

class : Handler {
    public:
    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN){
            sockaddr_in clientAddr{};
            socklen_t clientAddrSize = sizeof(clientAddr);
            
            auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
            if(clientFd == -1) error(1, errno, "accept failed");
            
            printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
            
            clients.insert(new Client(clientFd));
        }
        if(events & ~EPOLLIN){
            error(0, errno, "Event %x on server socket", events);
            ctrl_c(SIGINT);
        }
    }
} servHandler;

int main(int argc, char ** argv){
    if(argc != 2) error(1, 0, "Need 1 arg (port)");
    auto port = readPort(argv[1]);
    
    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if(servFd == -1) error(1, errno, "socket failed");
    
    signal(SIGINT, ctrl_c);
    signal(SIGPIPE, SIG_IGN);
    
    setReuseAddr(servFd);
    
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
    int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res) error(1, errno, "bind failed");
    
    res = listen(servFd, 1);
    if(res) error(1, errno, "listen failed");

    epollFd = epoll_create1(0);
    
    epoll_event ee {EPOLLIN, {.ptr=&servHandler}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, servFd, &ee);

    fileWithCodes.open("hasla.txt");
    if(!fileWithCodes){
        error(1, errno, "Cannot open input file.\n");
    }

    std::thread clockR(clockRun, &start, &end, &registrationAvailable, &timeRun, &gameRun);
    while(true){
        if(-1 == epoll_wait(epollFd, &ee, 5, -1)) {
            error(0,errno,"epoll_wait failed");
            ctrl_c(SIGINT);
        }
        ((Handler*)ee.data.ptr)->handleEvent(ee.events);
        end = std::chrono::steady_clock::now();
        

    }
}

void clockRun(std::chrono::time_point<std::chrono::steady_clock> * start, std::chrono::time_point<std::chrono::steady_clock> * end, bool * registrationAvailable, bool * timeRun, bool * gameRun){
    
    bool afterStart = false;

    while(!afterStart){
        if(*timeRun == true)
        {
            *start = std::chrono::steady_clock::now();
            afterStart = true;
            break;
        }
        
    }

    while(true){

        *end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(*end - *start).count();
        
        std::string actualCode;

        if ((duration > TIME_FOR_REGISTRATION) & (*registrationAvailable == true) ) 
            { *registrationAvailable = false; mySendInt(TIME_GAP);}

        if ((duration > (TIME_FOR_REGISTRATION + TIME_GAP)) & (*gameRun == false) ) 
            { *gameRun = true; 
            GotoLine(fileWithCodes, haslo(rng));
            fileWithCodes >> actualCode;
            sendToAllPly(myStringToChar(actualCode), actualCode.length()); 
            mySendInt(Client::numberOfPlayers);}

        if ((duration > (TIME_FOR_REGISTRATION + TIME_GAP + TIME_FOR_GAME)) & (*registrationAvailable == false ) & (*gameRun == true) ) 
            { 
                *registrationAvailable = true; *gameRun = false; sendToAllPly(myStringToChar("the end"), std::strlen("the end"));
                *start = std::chrono::steady_clock::now();
            }
    }
}

char* myStringToChar(std::string str){
    return &str[0];
}

void mySendInt(int numb){
    char res[4];
    sprintf(res, "%d", numb);
    sendToAllPly(res, 4);
}

uint16_t readPort(char * txt){
    char * ptr;
    auto port = strtol(txt, &ptr, 10);
    if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
    return port;
}

void setReuseAddr(int sock){
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(res) error(1,errno, "setsockopt failed");
}

void ctrl_c(int){
    for(Client * client : clients)
        delete client;
    close(servFd);
    printf("Closing server\n");
    exit(0);
}

void sendToAllBut(int fd, char * buffer, int count){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->fd()!=fd)
            client->myWrite(buffer, count);
    }
}

void sendToAllCli(char * buffer, int count){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        client->myWrite(buffer, count);
    }
}

void sendToAllPly(char * buffer, int count){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->player == true)
            client->myWrite(buffer, count);
    }
}

void sendToAllQue(char * buffer, int count){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->player == false)
            client->myWrite(buffer, count);
    }
}

std::fstream& GotoLine(std::fstream& file, int num){
    file.seekg(std::ios::beg);
    for(int i=0; i < num; i++){
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    return file;
}
