/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:39:14
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 10:13:28
 * @FilePath: /home/ServiceMigration/src/utils/load_configuration.cpp
 * @Description: 
 */
#include "Logger.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "jsoncpp/json/json.h"
#include "load_configuration.h"
extern std::string etcdUrl;
extern std::string dnsUrl;
extern std::string nodeIp;
// 
/**
 * @description: 函数用于更新配置变量
 * @return {*}
 */
void updateGlobalConfig() {
    std::ifstream file("configuration.json", std::ifstream::binary);
    if (!file.is_open()) {
        std::cout << "Error opening configuration file." << std::endl;
        return;
    }

    // 创建 Json::Reader 和 Json::Value 对象
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string errs;

    // 从文件解析 JSON 数据
    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        std::cout << "Error parsing JSON: " << errs << std::endl;
        return;
    }

    // 更新全局变量
    if (root.isMember("ETCD_URL") && root["ETCD_URL"].isString()) {
        etcdUrl = root["ETCD_URL"].asString();
    }

    if (root.isMember("DNS_URL") && root["DNS_URL"].isString()) {
        dnsUrl = root["DNS_URL"].asString();
    }
    if (root.isMember("NODE_IP") && root["NODE_IP"].isString()) {
        nodeIp = root["NODE_IP"].asString();
    }
}
/**
 * @description: 读取行为树xml模板
 * @param {string&} filename
 * @return {*}
 */
std::string readXMLFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open the file: " << filename << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
