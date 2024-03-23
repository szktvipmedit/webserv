#include "HttpSession.hpp"

HttpSession::HttpSession(SOCKET newSocket){
    socket = newSocket;
    eventlist = new struct kevent[6];
    createKqueue();
    createNewEvent();
    eventRegister(); 
}

HttpSession::~HttpSession(){
    delete [] eventlist;
}

void HttpSession::startEventLoop(Config *conf){
      while(1){
        size_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), NULL);
        for(size_t i = 0; i<nevent;i++){
            requestHandler(conf, socket);
        }
    }
}

void HttpSession::requestHandler(Config *conf, SOCKET sockfd){
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
        //if(tcpコネクション終了) exit(EXIT_SUCCESS);
    }   
}

void HttpSession::sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd){
    int pipe_c2p[2];
    if(pipe(pipe_c2p) < 0)
        throw std::runtime_error("Error: pipe() failed");
    pid_t pid = fork();
    if(pid == 0)
        executeCgi(conf, requestInfo, pipe_c2p);
    createResponseFromCgiOutput(pid, sockfd, pipe_c2p);
}

void HttpSession::executeCgi(Config *conf, RequestParse& requestInfo, int pipe_c2p[2]){
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

void HttpSession::createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]){
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

void HttpSession::createKqueue(){
    kq = kqueue();
    if(kq == -1){
        // for(socketSet::const_iterator it=tcpSockets.begin();it==tcpSockets.end();it++)
        //     close(*it);
        throw std::runtime_error("Error: kqueue() failed()"); 
    } 
}
void HttpSession::createNewEvent(){
    EV_SET(&event, socket, EVFILT_READ, EV_ADD, 0, 0, NULL);
}
void HttpSession::eventRegister(){
    if(kevent(kq, &event, 1, NULL, 0, NULL) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
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