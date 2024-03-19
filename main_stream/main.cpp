#include "../config/Config.hpp"
#include "../request/RequestParse.hpp"
#include <sys/wait.h>

#define W 1
#define R 0
int main(int argc, char **argv){
    if(argc != 2){
        std::cout << "Error: invalid execute" << std::endl;
        return 1;
    }
    try{
        Config *conf = Config::getInstance(argv[1]);
        std::set<int> tcpSocketSet = conf->getTcpSockets();
        int kq = kqueue();
        if(kq == -1){
            for(socketSet::const_iterator it=tcpSocketSet.begin();it==tcpSocketSet.end();it++)
                close(*it);
            throw std::runtime_error("Error: kqueue() failed()"); 
        }
        std::map<int, struct kevent*>keventMap;
        struct timespec timeSpec = {0,10000000};
        for(socketSet::const_iterator it=tcpSocketSet.begin();it!=tcpSocketSet.end();it++){
            keventMap[*it] = new struct kevent;
            EV_SET(keventMap[*it], *it, EVFILT_READ, EV_ADD, 0, 0, NULL);
            if(kevent(kq, keventMap[*it], 1, NULL, 0, &timeSpec) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
                perror("kevent"); // kevent: エラーメッセージ
                printf("errno = %d (%s)\n", errno, strerror(errno)); 
                for(socketSet::const_iterator it=tcpSocketSet.begin();it!=tcpSocketSet.end();it++)
                    close(*it);
                throw std::runtime_error("Error: kevent() failed()");
            }
        }
        struct kevent eventlist[tcpSocketSet.size()+(tcpSocketSet.size()/2)];
        std::set<int>acceptedSockets;
        while(1){
            std::cout << "======== EVENT MONITORING =========" << std::endl;
            size_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), NULL);
            std::cout << "nevent: " << nevent << std::endl;
            for(size_t i = 0; i<nevent;i++){
                int sockfd = eventlist[i].ident;
                // std::cout << "event generate socket num = " << sockfd << std::endl;
                struct sockaddr_storage client_sa;  // sockaddr_in 型ではない。 
                socklen_t len = sizeof(client_sa);   
                std::set<int>::iterator it = tcpSocketSet.find(sockfd);
                if (it != tcpSocketSet.end() && acceptedSockets.find(sockfd) == acceptedSockets.end()) {
                    int new_sock = accept(sockfd, (struct sockaddr*) &client_sa, &len);
                    keventMap[new_sock] = new struct kevent;
                    EV_SET(keventMap[new_sock], new_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    if(kevent(kq, keventMap[new_sock], 1, NULL, 0, &timeSpec) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
                        for(socketSet::const_iterator it=tcpSocketSet.begin();it==tcpSocketSet.end();it++){
                            free(keventMap[*it]);
                            close(*it);
                        }
                        throw std::runtime_error("Error: kevent2() failed()");
                    }
                    acceptedSockets.insert(sockfd);
                    std::cout << "TCP CONNECTION CREATE: new socket = " << new_sock << std::endl;
                }
                else
                {
                    const unsigned int MAX_BUF_LENGTH = 4096;
                    char buf[MAX_BUF_LENGTH];
                    char res_buf[MAX_BUF_LENGTH];
                    int bytesReceived = recv(sockfd, &buf, MAX_BUF_LENGTH, MSG_DONTWAIT);
                    if(bytesReceived > 0){
                        { //stringで受け取れれば足りるならこっち
                            std::string request = std::string(buf, buf+bytesReceived);
                            RequestParse requestInfo(request);
                            int pipe_c2p[2];
                            if(pipe(pipe_c2p) < 0){
                                std::cerr << "Error: pipe() failed" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            std::cout << "create pipe" << std::endl;
                            pid_t pid = fork();
                            if(pid == 0){
                                //child prosess
                                close(pipe_c2p[R]);
                                dup2(pipe_c2p[W],1);
                                close(pipe_c2p[W]);
                                extern char** environ;
                                VirtualServer server = conf->getServer(requestInfo.getHostName());
                                std::string cgiPath = server.getCgiPath();
                                char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), NULL };
                                if(execve("../cgi/test.cgi", cgi_argv, environ) < 0)
                                    std::cout << "Error: execve() failed" << std::endl;
                            }else{
                                waitpid(pid, NULL, 0);
                                std::cout << "finish waitpid" << std::endl;
                                close(pipe_c2p[W]);
                                ssize_t byte = read(pipe_c2p[R], &res_buf, MAX_BUF_LENGTH);
                                if(byte > 0){
                                    res_buf[byte] = '\0';
                                    if(send(sockfd, &res_buf, byte, 0) < 0){
                                        std::cerr << "Error: send() failed" << std::endl;
                                    }else{
                                        std::cout << "send!!!!!!" << std::endl;
                                    }
                                }
                            }
                        }
                    }   
                }
            }
        }
    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
    }

}