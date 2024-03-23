#ifndef __HTTPSESSION_HPP_
# define __HTTPSESSION_HPP_

#include "../config/Config.hpp"
#include "HttpConnection.hpp"

typedef struct timespec timespec;
typedef std::map<int, struct kevent*> keventMap;

#define W 1
#define R 0
#define MAX_BUF_LENGTH 4096

class HttpSession{
    private:
        SOCKET socket;
        int kq;
        struct kevent event;
        struct kevent *eventlist;
    public:
        HttpSession(SOCKET socket);
        HttpSession(const HttpSession& other);
        HttpSession& operator=(const HttpSession& other);
        ~HttpSession();
        void startEventLoop(Config *conf);
        void createKqueue();
        void createNewEvent();
        void eventRegister();
    private:
        void requestHandler(Config *conf, SOCKET sockfd);
        void sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd);
        void executeCgi(Config *conf, RequestParse& requestInfo, int *pipe_c2p);
        void createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]);
    public:

};

#endif