/* C++ */
#include <iostream>
#include <string>
#include <unistd.h>
#include <fcntl.h>
using namespace std;
#define BUFFER_SIZE 4048
int main(int argc, char** argv)
{
  if(argc < 2){
    std::cout <<"error" << std::endl;
  }
  #define CGI_PATH argv[0]
  #define REQUEST_PATH argv[1]
  /* 出力データを文字列としてためておくための変数 */
  string strOutData;
  std::string content;
  int fd = open(REQUEST_PATH, O_RDWR);
  char buf[BUFFER_SIZE];
  int byte = read(fd, &buf, BUFFER_SIZE);
  if(byte > 0){
    buf[byte] = '\0';
    content = std::string(buf, buf+byte);
  }
  std::string contentLen = "Content-Length: " + std::to_string(content.size()) + "\n"; 
  /* httpヘッダ */
  /* HTML出力であることを出力 */
  strOutData = "HTTP/1.1 200 OK\n";
  strOutData += "Date: Wed, 28 Oct 2020 07:57:45 GMT\n";
  strOutData += "Server: Apache/2.4.41 (Unix)\n";
  strOutData += "Content-Location: index.html.en\n";
  strOutData += "Vary: negotiate\n";
  strOutData += "TCN: choice\n";
  strOutData += "Last-Modified: Thu, 29 Aug 2019 05:05:59 GMT\n";
  strOutData += "ETag: \"";
  strOutData += "2d-5913a76187bc0\"";
  strOutData += "\n";
  strOutData += "Accept-Ranges: bytes\n";
  strOutData += contentLen;
  strOutData += "Keep-Alive: timeout=5, max=100\n";
  strOutData += "Connection: Keep-Alive\n";
  strOutData += "Content-Type: text/html\n";
  strOutData += "\n";
  /* 標準出力に設定内容を出力 */
  strOutData += content;
  std::cout << strOutData.c_str();
}