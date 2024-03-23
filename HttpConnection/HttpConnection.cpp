#include "HttpConnection.hpp"

int HttpConnection::kq;
keventMap HttpConnection::tcpEvents;
struct kevent *HttpConnection::eventlist;
timespec HttpConnection::timeSpec = {0,1000000000};

HttpConnection::HttpConnection(){}
HttpConnection::~HttpConnection(){}
HttpConnection::HttpConnection(socketSet tcpSockets){
    connectionPrepare(tcpSockets);
}
/*========================================
        public member functions
========================================*/
HttpConnection* HttpConnection::getInstance(socketSet tcpSockets){
    HttpConnection *inst;
    inst = new HttpConnection(tcpSockets);
    return inst;
}

void HttpConnection::destroy(HttpConnection* inst){
    delete inst;
}

void HttpConnection::connectionPrepare(socketSet tcpSockets){
    createKqueue();
    createTcpConnectionEvents(tcpSockets);
    eventlist = new struct kevent[tcpSockets.size()*6]; //最近のブラウザは最大6つのtcpコネクションを確立するらしいのでtcpSockets*6倍のイベント容量を用意する
}

void HttpConnection::createKqueue(){
    kq = kqueue();
    if(kq == -1){
        // for(socketSet::const_iterator it=tcpSockets.begin();it==tcpSockets.end();it++)
        //     close(*it);
        throw std::runtime_error("Error: kqueue() failed()"); 
    }
}
void HttpConnection::createTcpConnectionEvents(socketSet tcpSockets){
    for(socketSet::const_iterator it=tcpSockets.begin();it!=tcpSockets.end();it++){
        createNewEvent(*it);
        eventRegister(*it);
    }
}

void HttpConnection::createNewEvent(SOCKET targetSocket){
    tcpEvents[targetSocket] = new struct kevent;
    EV_SET(tcpEvents[targetSocket], targetSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
}

void HttpConnection::eventRegister(SOCKET fd){
    if(kevent(kq, tcpEvents[fd], 1, NULL, 0, NULL) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
        perror("kevent"); // kevent: エラーメッセージ
        printf("errno = %d (%s)\n", errno, strerror(errno)); 
        // for(socketSet::const_iterator it=tcpSockets.begin();it!=tcpSockets.end();it++)
        //     close(*it);
        // for(socketSet::const_iterator it=tcpSockets.begin();*it!=fd;it++)
        //     delete tcpEvents[*it];
        // delete tcpEvents[fd];//現在のループで作った分
        throw std::runtime_error("Error: kevent() failed()");
    }
}

void HttpConnection::deleteZombie(){
    pid_t pid;
    int status;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        std::cout << "delete zombie status: "<< &status << std::endl;
    }
}

void HttpConnection::startEventLoop(Config *conf, socketSet tcpSockets){
    while(1){
        deleteZombie();
        size_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), NULL);
        for(size_t i = 0; i<nevent;i++){
            eventConnect(conf, eventlist[i].ident, tcpSockets);
        }
    }
}

void HttpConnection::eventConnect(Config *conf, SOCKET sockfd, socketSet tcpSockets){
    if (tcpSockets.find(sockfd) != tcpSockets.end()) 
        establishTcpConnection(conf, sockfd);
    else
        std::cerr << "bug??? unexpect socket event" << std::endl;
}


void HttpConnection::establishTcpConnection(Config *conf, SOCKET sockfd){
    std::cout << "TCP CONNECTION ESTABLISHED"<< std::endl;

    struct sockaddr_storage client_sa;  // sockaddr_in 型ではない。 
    socklen_t len = sizeof(client_sa);   
    int newSocket = accept(sockfd, (struct sockaddr*) &client_sa, &len);
    pid_t pid = fork();
    if(pid == 0){
        startCommunicateWithClient(conf, newSocket);
    }
    close(newSocket);
    std::cout << "event socket = " << sockfd << std::endl;
    std::cout << "new socket = " << newSocket << std::endl;
}

void HttpConnection::closeParentSockets(){
    for(keventMap::iterator it=tcpEvents.begin(); it != tcpEvents.end();it++){
        close(it->first);
    }
}

void HttpConnection::startCommunicateWithClient(Config *conf, SOCKET newSocket){
    closeParentSockets();
    // tcpEvents[newSocket] = new struct kevent;
    HttpSession session = HttpSession(newSocket);
    session.startEventLoop(conf);
}
