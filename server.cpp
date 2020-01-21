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
#include "server.h"

#define TIME_FOR_REGISTRATION 60
#define TIME_GAP 10
#define TIME_FOR_GAME 10

Client::Client(int fd) : _fd(fd) {
    epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);

    ::write(fd,"welcome\n", std::strlen("welcome\n"));

    if(afterStart == false){
        clockRunStart();
    }

    timeRun = true;

    if(registrationAvailable == true)
    {
        this->player = true;
        Client::numberOfPlayers++;
        
        char duration[4];
        long int dur = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
        int length = sprintf(duration, "%ld\n", dur%10000);
        this->myWrite(duration, length);
    } else {
        this->player = false;
        ::write(fd,"wait\n", std::strlen("wait\n"));}
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
void Client::handleEvent(uint32_t events){
    if(gameRun == false)
        return;
    if(events & EPOLLIN) {
        char buffer[2];
        ssize_t count = read(_fd, buffer, 2);
        if(count > 0){
            std::string buff(buffer);
            std::string odp(buff.substr(0,1));
            std::cout<<odp;
            std::cout<<this->_fd;
            if(odp == "-1"){
                printf("Przegrana");
                this->remove();
                }
            else {
                printf("Znakow %s", myStringToChar(odp));
                }
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

    std::string line;
    fileWithCodes.open("hasla.txt");
    if(!fileWithCodes){
        error(1, errno, "Cannot open input file.\n");
    }
    while(getline(fileWithCodes, line))
        numberOfClues++;

    std::thread clockR(clockRunRegistration);
    // Pętla przyjmująca nowe połączenia oraz, w trakcie gry, zczytująca wyniki rundy
    while(true){
        if(-1 == epoll_wait(epollFd, &ee, 1, -1)) {
            error(0,errno,"epoll_wait failed");
            ctrl_c(SIGINT);
        }
        ((Handler*)ee.data.ptr)->handleEvent(ee.events);
        end = std::chrono::steady_clock::now();
        

    }
}

void clockRunStart(){
    
    start = std::chrono::steady_clock::now();
    afterStart = true;
}

void clockRunRegistration(){
    
    end = std::chrono::steady_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    printf("trwa rejestracja\n");
    //s30.try_lock_for(std::chrono::seconds(30));
    std::this_thread::sleep_for(std::chrono::seconds(TIME_FOR_REGISTRATION));
    printf("po rejestracji\n");
    clockRunGap();

    
}

void clockRunGap(){
    registrationAvailable = false; mySendInt(TIME_GAP);
    printf("synchronizacja\n");
    std::this_thread::sleep_for(std::chrono::seconds(TIME_GAP));
    clockRunGame();

}

void clockRunGame(){
    printf("START\n");
    gameRun = true;
    sendClueToPlayers();
    sendNumberOfPlayers();
    

    // nowa runda
/*
    if ((duration > (TIME_FOR_REGISTRATION + TIME_GAP + TIME_FOR_GAME)) & (registrationAvailable == false ) & (gameRun == true) ) { 
    registrationAvailable = true; gameRun = false; sendToAllPly(myStringToChar("end\n"), std::strlen("end\n"));
    start = std::chrono::steady_clock::now();
    if(Client::numberOfPlayers > 1)
    {(numberOfRound)++;}
    else
    {numberOfRound = 1;}
    }

    // nowa gra

    if ((duration < TIME_FOR_REGISTRATION) & (numberOfRound == 1)){
        addQueuersToGame();
    }
*/

}

void sendNumberOfPlayers(){
    mySendInt(Client::numberOfPlayers);
}

void sendClueToPlayers(){
    std::string actualCode;

    GotoLine(fileWithCodes, haslo(rng));
    fileWithCodes >> actualCode;
    sendToAllPly(myStringToChar(actualCode), actualCode.length());
    char endline = '\n';
    sendToAllPly(&endline, 1); 
}

char* myStringToChar(std::string str){
    return &str[0];
}

void mySendInt(int numb){
    char res[3];
    int length = sprintf(res, "%d\n", numb);
    sendToAllPly(res, length);
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

void addQueuersToGame(){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->player == false){
            client->player = true;
            Client::numberOfPlayers++;
            client->myWrite(myStringToChar("welcome\n"), 8);
            char duration[4];
            long int dur = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
            int length = sprintf(duration, "%ld\n", dur);
            client->myWrite(duration, length);
        }
    }
}

std::fstream& GotoLine(std::fstream& file, int num){
    file.seekg(std::ios::beg);
    for(int i=0; i < num; i++){
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
    return file;
}