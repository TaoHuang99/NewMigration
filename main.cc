/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:29:33
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 10:12:25
 * @FilePath: /home/ServiceMigration/main.cc
 * @Description: 
 */
#include "Logger.h"
#include <drogon/drogon.h>
#include <iostream>
#include "Logger.h"
std::string etcdUrl = "";
std::string dnsUrl = "";
std::string nodeIp = "";

//#include"src/Logger.cpp"
using namespace Sakura::Logger;
int main() {
    //logger文件
    
    Logger::getInstance()->open("./test.log");
    Logger::getInstance()->setMax(1024);
    info("Logger 主函数测试");
    //Set HTTP listener address and port
    drogon::app().addListener("0.0.0.0",8021);
    //Load config file
    //drogon::app().loadConfigFile("../config.json");
    //Run HTTP framework,the method will block in the internal event loop
    drogon::app().run();
    return 0;
}
