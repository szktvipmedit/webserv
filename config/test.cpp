#include "Config.hpp"


#define CONFIG_PATH "../configures/webserv.conf"
int main(int argc, char **argv){
   if(argc == 2){
      Config *inst = Config::getInstance(argv[1]);
      (void)inst;
   }
}