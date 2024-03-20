#include "HttpConnection.hpp"

int HttpConnection::kq;
keventMap HttpConnection::events;
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
    eventlist = new struct kevent[tcpSockets.size()+(tcpSockets.size()/2)]; //ここの容量は適当に決めました。
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
    events[targetSocket] = new struct kevent;
    EV_SET(events[targetSocket], targetSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);
}

void HttpConnection::eventRegister(SOCKET fd){
    if(kevent(kq, events[fd], 1, NULL, 0, NULL) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
        perror("kevent"); // kevent: エラーメッセージ
        printf("errno = %d (%s)\n", errno, strerror(errno)); 
        // for(socketSet::const_iterator it=tcpSockets.begin();it!=tcpSockets.end();it++)
        //     close(*it);
        // for(socketSet::const_iterator it=tcpSockets.begin();*it!=fd;it++)
        //     delete events[*it];
        // delete events[fd];//現在のループで作った分
        throw std::runtime_error("Error: kevent() failed()");
    }
}

void HttpConnection::startEventLoop(Config *conf, socketSet tcpSockets){
    while(1){
        size_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), NULL);
        for(size_t i = 0; i<nevent;i++){
            eventExecute(conf, eventlist[i].ident, tcpSockets);
        }
    }
}

void HttpConnection::eventExecute(Config *conf, SOCKET sockfd, socketSet tcpSockets){
    if (tcpSockets.find(sockfd) != tcpSockets.end()) 
        establishTcpConnection(sockfd);
    else
        requestHandler(conf, sockfd);
}

void HttpConnection::establishTcpConnection(SOCKET sockfd){
    std::cout << "TCP CONNECTION ESTABLISHED"<< std::endl;

    struct sockaddr_storage client_sa;  // sockaddr_in 型ではない。 
    socklen_t len = sizeof(client_sa);   
    int newSocket = accept(sockfd, (struct sockaddr*) &client_sa, &len);
    createNewEvent(newSocket);
    eventRegister(newSocket);
    std::cout << "event socket = " << sockfd << std::endl;
    std::cout << "new socket = " << newSocket << std::endl;
}

void HttpConnection::requestHandler(Config *conf, SOCKET sockfd){
    char buf[MAX_BUF_LENGTH];
    int bytesReceived = recv(sockfd, &buf, MAX_BUF_LENGTH, MSG_DONTWAIT);
    // std::cout << "bytesRecieved: " << bytesReceived << std::endl;
    if(bytesReceived > 0){
        // std::cout << "==========EVENT==========" << std::endl;
        // std::cout << "event socket = " << sockfd << std::endl;
        // std::cout << "HTTP REQUEST"<< std::endl;
        std::string request = std::string(buf, buf+bytesReceived);
        // std::cout << buf << std::endl;
        RequestParse requestInfo(request);
        sendResponse(conf, requestInfo, sockfd);
    }   
}

void HttpConnection::sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd){
    int pipe_c2p[2];
    if(pipe(pipe_c2p) < 0)
        throw std::runtime_error("Error: pipe() failed");
    pid_t pid = fork();
    if(pid == 0)
        executeCgi(conf, requestInfo, pipe_c2p);
    createResponseFromCgiOutput(pid, sockfd, pipe_c2p);
}

void HttpConnection::executeCgi(Config *conf, RequestParse& requestInfo, int pipe_c2p[2]){
    close(pipe_c2p[R]);
    dup2(pipe_c2p[W],1);
    close(pipe_c2p[W]);
    extern char** environ;
    VirtualServer server = conf->getServer(requestInfo.getHostName());
    std::string cgiPath = server.getCgiPath();
    char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), NULL };
    if(execve("../cgi/test.cgi", cgi_argv, environ) < 0)
        std::cout << "Error: execve() failed" << std::endl;
}

void HttpConnection::createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]){
    char res_buf[MAX_BUF_LENGTH];
    waitpid(pid, NULL, 0);
    close(pipe_c2p[W]);
    ssize_t byte = read(pipe_c2p[R], &res_buf, MAX_BUF_LENGTH);
    if(byte > 0){
        res_buf[byte] = '\0';
        if(send(sockfd, &res_buf, byte, 0) < 0)
            std::cerr << "Error: send() failed" << std::endl;
        else
            std::cout << "send!!!!!!" << std::endl;
    }
}