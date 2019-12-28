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
#include "czat.h"

#define TIME_FOR_REGISTRATION 30
#define TIME_FOR_GAME 30

Client::Client(int fd) : _fd(fd) {
    epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);

    ::write(fd,"Welcome\n", sizeof("Welcome\n"));

    if(timeRun == false) { timeRun = true; start = std::chrono::steady_clock::now(); }

    if(registrationAvailable == true)
    {
        this->player = true;
        ::write(fd,"We'll start in a few seconds\n", sizeof("We'll start in a few seconds\n"));}
    else{
        this->player = false;
        ::write(fd,"Please wait for a next round\n", sizeof("Please wait for a next round\n"));}
}
Client::~Client(){
    epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
    shutdown(_fd, SHUT_RDWR);
    close(_fd);
}
Client::Client(){
    
}
void Client::handleEvent(uint32_t events){
    if(events & EPOLLIN) {
        char buffer[256];
        ssize_t count = read(_fd, buffer, 256);
        if(count > 0)
            sendToAllBut(_fd, buffer, count);
        else
            events |= EPOLLERR;
    }
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

    while(true){
        if(-1 == epoll_wait(epollFd, &ee, 5, -1)) {
            error(0,errno,"epoll_wait failed");
            ctrl_c(SIGINT);
        }
        ((Handler*)ee.data.ptr)->handleEvent(ee.events);
        end = std::chrono::steady_clock::now();
        if ((std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > TIME_FOR_REGISTRATION) & (registrationAvailable == true) ) 
        { registrationAvailable = false; sendToAllPly("start", 5);}

        if ((std::chrono::duration_cast<std::chrono::seconds>(end - start).count() > (TIME_FOR_REGISTRATION+TIME_FOR_GAME)) & (registrationAvailable == false) ) 
        { registrationAvailable = true; sendToAllPly("the end", 7);}

    }
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