#include "RequestParse.hpp"

RequestParse::RequestParse(std::string requestMessage): tcpKeepAlive(true){
    setMethodPathVersion(requestMessage);
    const_cast<std::string&>(headers) = setHeaders(requestMessage); 
    const_cast<bool&>(tcpKeepAlive) = setTcpKeepAlive(); 

}

RequestParse::~RequestParse(){

}

RequestParse::RequestParse(const RequestParse& other):tcpKeepAlive(other.tcpKeepAlive){
    if(this != &other)
        *this = other;
}

RequestParse& RequestParse::operator=(const RequestParse& other){
    if(this != &other){
        const_cast<std::string&>(method) = other.method;
        const_cast<std::string&>(path) = other.path;
        const_cast<std::string&>(version) = other.version;
        const_cast<std::string&>(headers) = other.headers;
        const_cast<bool&>(tcpKeepAlive) = other.tcpKeepAlive;

        return *this;
    }
    return *this;
}
std::string RequestParse::getFirstRow(std::string requestMessage){
    std::string::size_type idx = requestMessage.find("\n", 0);
    if(idx == std::string::npos)
        throw std::runtime_error("Error: invalid request: non new line");
    std::string firstRow = requestMessage.substr(0, idx);
    firstRow.pop_back();
    return firstRow;
}


void RequestParse::setMethodPathVersion(std::string requestMessage){
    std::string firstRow = getFirstRow(requestMessage);
    std::vector<std::string>splitFirstRow = split(firstRow, ' ');
    if(splitFirstRow.size() != 3)
        throw std::runtime_error("Error: invalid request: non space in first line"); 
    const_cast<std::string&>(method) = splitFirstRow[0];
    const_cast<std::string&>(path) = splitFirstRow[1];
    const_cast<std::string&>(version) = splitFirstRow[2];
}
std::string RequestParse::setHeaders(std::string requestMessage){
    std::string::size_type idx = requestMessage.find("\n", 0);
    return requestMessage.substr(idx+1);
}
bool RequestParse::setTcpKeepAlive(){
    std::string::size_type firstIdx = 0;
    std::string::size_type secondIdx = headers.find("\n", 0);
    while(secondIdx != std::string::npos){
        std::string row(headers.begin()+firstIdx, headers.begin()+secondIdx);
        row.pop_back();
        std::vector<std::string>item = split(row, ' ');
        if(item.size() == 2 && item[0] == "Connection:" && item[1] == "keep-alive")
            return ALIVE;
        firstIdx = secondIdx+1;
        secondIdx = headers.find("\n", secondIdx+1);
    }
    return DEAD;
}

std::string RequestParse::getMethod(){
    return method;
}
std::string RequestParse::getPath(){
    return path;
}
std::string RequestParse::getVersion(){
    return version;
}
std::string RequestParse::getHeaders(){
    return headers;
}
bool RequestParse::getTcpKeepAlive(){
    return tcpKeepAlive;
}