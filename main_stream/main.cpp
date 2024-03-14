#include "../config/Config.hpp"
#include "../request/RequestParse.hpp"

int main(int argc, char **argv){
    if(argc != 2)
        return -1;
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
        struct timespec timeSpec;
        timeSpec.tv_nsec = 1000;
        for(socketSet::const_iterator it=tcpSocketSet.begin();it!=tcpSocketSet.end();it++){
            keventMap[*it] = new struct kevent;
            EV_SET(keventMap[*it], *it, EVFILT_READ, EV_ADD, 0, 0, NULL);
            if(kevent(kq, keventMap[*it], 1, NULL, 0, &timeSpec) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
                std::cout << strerror(errno) << std::endl;
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
                    std::vector<char> buf(MAX_BUF_LENGTH);
                    int bytesReceived = recv(sockfd, &buf[0], MAX_BUF_LENGTH, MSG_DONTWAIT);
                    if(bytesReceived > 0){
                        { //stringで受け取れれば足りるならこっち
                            std::string request(buf.begin(), buf.end());
                            RequestParse requestInfo(request);
                            if(requestInfo.getMethod() == "GET")
                                std::cout << "ここでGET METHODを実行する" << std::endl;
                            else if(requestInfo.getMethod() == "POST")
                                std::cout << "ここでPOST METHODを実行する" << std::endl;
                            else if(requestInfo.getMethod() == "DELETE")
                                std::cout << "ここでDELETE METHODを実行する" << std::endl;
                        }
                        // { //ファイルを生成してそこに入れたいならこっち
                        //     buf[bytesReceived] = '\0'; // 文字列の終端をセット
                        //     std::string request(buf.begin(), buf.end());
                        //     std::cout << request << std::endl;
                        //     int requestfd = open("../request/server_recv.txt", O_RDWR | O_CREAT, 0644);
                        //     if(requestfd == -1){
                        //         perror("open");
                        //         throw std::runtime_error("open failed");
                        //     }
                        //     write(requestfd, request.c_str(), request.size());
                        //     close(requestfd);
                        // }
                        
                    }   
                    close(sockfd);
                }
            }
        }
    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
    }

}