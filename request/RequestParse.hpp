#ifndef __REQUESTPARSE_H_
# define __REQUESTPARSE_H_

#include "../config/Config.hpp"

#define ALIVE true;
#define DEAD false;

class RequestParse{
    private:
        RequestParse();
    public:
        RequestParse(std::string requestMessage);
        ~RequestParse();
        RequestParse(const RequestParse& other);
        RequestParse& operator=(const RequestParse& other);
    public:
        std::string getMethod();
        std::string getPath();
        std::string getVersion();
        std::string getHeaders();
        bool getTcpKeepAlive();

    private: 
        std::string getFirstRow(std::string requestMessage);
        void setMethodPathVersion(std::string firstRow);
        std::string setHeaders(std::string requestMessage);
        bool setTcpKeepAlive();

    private:

    private:
        const std::string method;
        const std::string path;
        const std::string version;
        const std::string headers;
        const bool tcpKeepAlive;
};

#endif