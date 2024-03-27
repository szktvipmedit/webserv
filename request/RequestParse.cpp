#include "RequestParse.hpp"

RequestParse::RequestParse(std::string requestMessage){
    setMethodPathVersion(requestMessage);
    setHeadersAndBody(requestMessage);
}

RequestParse::~RequestParse(){

}

RequestParse::RequestParse(const RequestParse& other){
    if(this != &other)
        *this = other;
}

RequestParse& RequestParse::operator=(const RequestParse& other){
    if(this != &other){
        const_cast<std::string&>(method) = other.method;
        const_cast<std::string&>(path) = other.path;
        const_cast<std::string&>(version) = other.version;

        return *this;
    }
    return *this;
}
std::string RequestParse::getRequestLine(std::string& requestMessage){
    std::string::size_type idx = requestMessage.find("\n", 0);
    if(idx == std::string::npos)
        throw std::runtime_error("Error: invalid request: non new line");
    std::string firstRow = requestMessage.substr(0, idx);
    firstRow.pop_back();
    requestMessage = requestMessage.substr(idx+1);
    return firstRow;
}

void RequestParse::setMethodPathVersion(std::string& requestMessage){
    std::string firstRow = getRequestLine(requestMessage);
    strVec splitFirstRow = split(firstRow, ' ');
    if(splitFirstRow.size() != 3)
        throw std::runtime_error("Error: invalid request: non space in first line"); 
    method = splitFirstRow[0];
    path = splitFirstRow[1];
    version = splitFirstRow[2];

}

void RequestParse::setHeadersAndBody(std::string& requestMessage){
    strVec linesVec = splitLines(requestMessage, '\n');
    strVec::iterator it;
    setHeaders(linesVec, it);
    setBody(linesVec, it);
}

strVec createHeaderAndDirective(strVec::iterator it);

void RequestParse::setHeaders(strVec linesVec, strVec::iterator& it){
    for(it=linesVec.begin();it != linesVec.end();it++){
        // std::cout << *it << std::endl;
        if(*it == "" || *it == "\r" || (*it).size() == 0)
            break;
        strVec line = createHeaderAndDirective(it);
        cutPrefixSpace(line[1]);
        headers[line[0]] = line[1];
    }
    it++;
}

void RequestParse::setBody(strVec  linesVec, strVec::iterator itFromBody) {
    // std::cout << (*itFromBody).size() << std::endl;
    if((*itFromBody).size() == 0 || itFromBody == linesVec.end()){
        body = "";
        return;
    }
    if(headers["Transfer-Encoding"] == "chunked")
        return bodyUnChunk(linesVec, itFromBody);
    body = createBodyStringFromLinesVector(linesVec, itFromBody);
}
void RequestParse::bodyUnChunk(strVec linesVec, strVec::iterator itFromBody){
    /*
        文字数が書かれた行を無視して連結するだけの処理
    */
   std::string bodyString;
   size_t mod = 0;
    for(strVec::iterator it=itFromBody;it!=linesVec.end();it++){
        if(mod % 2 == 1){
            *it = *it+'\n';
            bodyString += *it;
        }
        mod++;
    }
    body = bodyString; 
}
std::string RequestParse::createBodyStringFromLinesVector(strVec linesVec, strVec::iterator itFromBody){
    std::string bodyString;
    for(strVec ::iterator it=itFromBody;it!=linesVec.end();it++){
        *it = *it+'\n';
        bodyString += *it;
    }
    bodyString.pop_back();
    return bodyString;
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
std::string RequestParse::getHeader(std::string header){
    return headers[header];
}
std::string RequestParse::getBody(){
    return body;
}
void RequestParse::test__headers(){
    for(headersMap::iterator it=headers.begin();it!=headers.end();it++){
        std::cout << "header: " << it->first << std::endl;
        std::cout << "      directive: " << it->second << std::endl;
    }
}
strVec RequestParse::splitLines(std::string str, char sep) {
   size_t first = 0;
   std::string::size_type last = str.find_first_of(sep);
   strVec  result;
    if(last == std::string::npos){
        result.push_back(str);
        return result;
    }
   while (first < str.size()) {
     result.push_back(str.substr(first, last - first));
     first = last + 1;
     last = str.find_first_of(sep, first);
     if (last == std::string::npos) last = str.size();
   }
   return result;
 }

std::string createDirectiveFromSplitVec(strVec line){
    size_t idx = 0;
    std::string directive;
    for(strVec::iterator it=line.begin();it!=line.end();it++){
        if(idx > 0){
            *it+=':';
            directive += *it;
        }
        idx++;
    }
    directive.pop_back();
    return directive;
}

strVec createHeaderAndDirective(strVec::iterator it){
    strVec line = split(*it, ':');
    if(line.size() < 2){
        throw std::runtime_error("Unexpected Error: Encountered header not delimited by ':'");//Notice: このケースがあるならParseの方法を変更しなければならない
    }
    if(line.size() > 2)
        line[1] = createDirectiveFromSplitVec(line);
    return line;
}

std::string RequestParse::getHostName(){
    std::string directive = getHeader("Host");
    if(directive != ""){
        strVec spDirective = split(directive, ':');
        return spDirective[0];
    }
    return "";
}


/*
    cgi
*/

char*const* RequestParse::createCgiArgs(VirtualServer server){
    // int argc = countArgs();
    std::string cgiPath = server.getCgiPath();
    std::string path = getRootDir(server)+getPath();
    std::cerr << "root+path: " << path << std::endl;
    // char*const* cgiArgs[argc];
    char* const cgi_argv[] = { const_cast<char*>(cgiPath.c_str()), const_cast<char*>(path.c_str()), NULL };

}

std::string RequestParse::getRootDir(VirtualServer& server){
    Location location = getNearLocation(server.getLocations());
    return location.getSetting("root");
}

size_t getMatchNumOfHierarchy(strVec &splitRequestPath, strVec &splitPath){
    size_t match = 0;
    size_t min = std::min(splitRequestPath.size(), splitPath.size());
    for(size_t i=0;i<min;i++){
        if(splitRequestPath[i] != splitPath[i])
            return match;
        match++;
    }
    return match;
}
Location RequestParse::getNearLocation(locationsMap locations){
    strVec splitRequestPath = split(getPath(), '/');
    Location nearLocation;
    size_t match = 0;

    for(locationsMap::iterator it=locations.begin();it!=locations.end();it++){
        strVec splitPath = split(it->first, '/');//location / {のときの動作が多分うまくいかない
        size_t m = getMatchNumOfHierarchy(splitRequestPath, splitPath);
        if(m > match){
            match = m;
            nearLocation = *(it->second);
        }
    }
    return nearLocation;
}