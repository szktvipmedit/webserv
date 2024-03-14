#include "RequestParse.hpp"

int main(){
    const unsigned int MAX_BUF_LENGTH = 4096;
    std::vector<char> buf(MAX_BUF_LENGTH);

    int fd = open("server_recv.txt", O_RDONLY);
    if (fd == -1) {
        std::cerr << "error: open failed" << std::endl;
        return 1;
    }

    read(fd, &buf[0], 4095);
    close(fd);

    std::string requestMessage(buf.begin(), buf.end());
    try{
        RequestParse inst(requestMessage);

        std::cout << inst.getMethod() << std::endl;
        std::cout << inst.getPath() << std::endl;
        std::cout << inst.getVersion() << std::endl;
        std::cout << inst.getTcpKeepAlive() << std::endl;
    }catch(std::runtime_error& e){
        std::cerr << e.what() << std::endl;
    }
    
}