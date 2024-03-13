#include "../config/Config.hpp"

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
        std::vector<struct kevent> kevents;
        struct timespec timeSpec;
        timeSpec.tv_nsec = 1000;
        for(socketSet::const_iterator it=tcpSocketSet.begin();it==tcpSocketSet.end();it++){
            size_t i=0;
            EV_SET(&kevents[i], *it, EVFILT_READ, EV_ADD, 0, 0, NULL);
            if(kevent(kq, &kevents[i], 1, NULL, 0, &timeSpec) == -1){ // ポーリングするためにはtimeout引数を非NULLのtimespec構造体ポインタを指す必要がある
                for(socketSet::const_iterator it=tcpSocketSet.begin();it==tcpSocketSet.end();it++)
                    close(*it);
                throw std::runtime_error("Error: kevent() failed()");
            }
            i++;
        }
        struct kevent *eventlist = new struct kevent[tcpSocketSet.size()];
        while(1){
            size_t nevent = kevent(kq, NULL, 0, eventlist, sizeof(*eventlist), NULL);
            for(size_t i = 0; i<nevent;i++){
                int sockfd = eventlist[i].ident;
                struct sockaddr_storage client_sa;  // sockaddr_in 型ではない。 
                socklen_t len = sizeof(client_sa);   
                std::set<int>::iterator it = tcpSocketSet.find(sockfd);
                if (it != tcpSocketSet.end()) {
                    int new_sock = accept(sockfd, (struct sockaddr*) &client_sa, &len);
                    struct kevent *newKevent = new struct kevent;
                    kevents.push_back(*newKevent);
                    EV_SET(&kevents[kevents.size()-1], new_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
                }
                else
                {
                    if (fork() == 0) { 
                        // 子プロセス 
                        std::string buf;
                        int len = read(sockfd, &buf, SIZE_MAX);
                        std::cout << buf << std::endl;
                        (void)len;
                        exit(0); 
                    } 
                }
            }
        }
    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
    }

}