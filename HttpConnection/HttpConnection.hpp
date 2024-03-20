#ifndef __HTTPCONNECTION_HPP_
# define __HTTPCONNECTION_HPP_

#include "../config/Config.hpp"
#include "../request/RequestParse.hpp"

typedef struct timespec timespec;
typedef std::map<int, struct kevent*> keventMap;

#define W 1
#define R 0
#define MAX_BUF_LENGTH 4096

class HttpConnection{
    private:
        static int kq;
        static keventMap events;
        static struct kevent *eventlist;
        static timespec timeSpec;
    private:
        HttpConnection();
        HttpConnection(socketSet tcpSockets);
        HttpConnection(const HttpConnection& other);
        HttpConnection& operator=(const HttpConnection& other);
        ~HttpConnection();
    public:
        static HttpConnection* getInstance(socketSet tpcSockets);
        static void destroy(HttpConnection *inst);
        void startEventLoop(Config *conf, socketSet tcpSockets);
    private:
        static void connectionPrepare(socketSet tcpSockets);
        static void createKqueue();
        static void createTcpConnectionEvents(socketSet tcpSockets);
        static void createNewEvent(SOCKET targetSocket);
        static void eventRegister(SOCKET fd);
        void eventExecute(Config *conf, SOCKET sockefd, socketSet tcpSockets);
        void establishTcpConnection(SOCKET sockfd);
        void requestHandler(Config *conf, SOCKET sockfd);
        void sendResponse(Config *conf, RequestParse& requestInfo, SOCKET sockfd);
        void executeCgi(Config *conf, RequestParse& requestInfo, int *pipe_c2p);
        void createResponseFromCgiOutput(pid_t pid, SOCKET sockfd, int pipe_c2p[2]);
};

#endif