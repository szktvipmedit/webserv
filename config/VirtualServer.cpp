#include "VirtualServer.hpp"


/*========================================
        orthodox canonical form
========================================*/
VirtualServer::VirtualServer(){}
VirtualServer::~VirtualServer(){}
VirtualServer::VirtualServer(const VirtualServer& other){
    if(this != &other)
        *this = other;
}

VirtualServer& VirtualServer::operator=(const VirtualServer& other){
    if(this == &other)
        return *this;
    serverSetting = other.serverSetting;
    locations = other.locations;
    isDefault = other.isDefault;
    return *this;
}


/*========================================
        public member functions
========================================*/
serverMap::iterator VirtualServer::searchSetting(std::string directiveName){
    return serverSetting.find(directiveName);
}

serverMap::iterator VirtualServer::getItEnd(){
    return serverSetting.end();
}

void VirtualServer::setSetting(std::string directiveName, std::string directiveContent){
    if(serverSetting.find(directiveName) != serverSetting.end())
        throw std::runtime_error(directiveName+" is duplicate");
    serverSetting[directiveName] = directiveContent;
}
std::string VirtualServer::getServerName(){
    return serverSetting["server_name"];
}
std::string VirtualServer::getListenPort(){
    return serverSetting["listen"];
}
void VirtualServer::setLocation(std::string locationPath, Location *location){
    if(locations.find(locationPath) != locations.end())
        throw std::runtime_error("server: "+locationPath+" is duplicate");
    locations[locationPath] = location;
}

void VirtualServer::confirmValues(){
    confirmServerName();
    confirmListenPort();
    confirmErrorPage();
    confirmCgi();
}
void VirtualServer::confirmServerName(){
    if(serverSetting.find("server_name") == serverSetting.end()){
        setSetting("server_name", "");
    }
}

void VirtualServer::confirmListenPort(){
    if(serverSetting.find("listen") == serverSetting.end()){
        setSetting("listen", "8080");
        return;
    }
    size_t len = serverSetting["listen"].size();
    for(size_t i=0;i<len;i++){
        if(serverSetting["listen"][i] == '\t')
               serverSetting["listen"][i] = ' ';
    }
    for(size_t i=0;i<len;i++){
        if(!std::isdigit(serverSetting["listen"][i])){
            std::vector<std::string> vec = split(serverSetting["listen"], ' ');
            if(vec.size() == 2 && vec[1] == "default_server"){
                isDefault = true;
                serverSetting["listen"] = vec[0];
                len = vec[0].size();
            }
            else
                throw std::runtime_error("Error: listen port is invalid");
        }
    }
    int port = stoi(serverSetting["listen"]);
    if(!(0 <= port && port <= 65535))
        throw std::runtime_error("Error: listen port is out of range"); 
}

void VirtualServer::confirmErrorPage(){
    if(serverSetting.find("error_page") == serverSetting.end()){
        setSetting("error_page", "404 document/404.html");
        return;
    }
    std::vector<std::string>status = split(serverSetting["error_page"], ' ');
    for(std::vector<std::string>::iterator it=status.begin(); it != status.end();it++){
        if(*it == "404")
            return;
    }
    serverSetting["error_page"] = "404 "+serverSetting["error_page"];
}


std::string VirtualServer::getCgiPath(){
    return serverSetting["cgi_path"];
}
void VirtualServer::confirmCgi(){
    if(serverSetting.find("cgi_path") == serverSetting.end()){
        setSetting("cgi_path", "../cgi/default.cgi");
        return ;
    }
}

locationsMap VirtualServer::getLocations(){
    return locations;
}