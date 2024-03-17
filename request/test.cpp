#include "RequestParse.hpp"

int main(){
    const unsigned int MAX_BUF_LENGTH = 4096;
    char *buf[MAX_BUF_LENGTH];

        // int fd = open("request/server_recv.txt", O_RDONLY);
        int fd = open("server_recv.txt", O_RDONLY);
    if (fd == -1) {
        std::cerr << "error: open failed" << std::endl;
        return 1;
    }

    ssize_t byte = read(fd, &buf, MAX_BUF_LENGTH);
    close(fd);

    std::string requestMessage(buf, buf+byte);
    try{
        RequestParse inst(requestMessage);

        std::cout << inst.getMethod() << std::endl;
        std::cout << inst.getPath() << std::endl;
        std::cout << inst.getVersion() << std::endl;
        // inst.test__headers();
        // std::cout << inst.getBody() << std::endl;

    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
    }
    
}