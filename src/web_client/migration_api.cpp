/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:39:48
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 10:13:18
 * @FilePath: /home/ServiceMigration/src/web_client/migration_api.cpp
 * @Description: 
 */
#include "Logger.h"
#include <iostream>
#include <string>
#include <sstream>
#include "cpr/cpr.h"
#include "migration_api.h"
extern std::string etcdUrl;
extern std::string dnsUrl;
extern std::string nodeIp;

/**
 * @description: 封装 DNS 修改的函数
 * @param {string&} dnsUrl 
 * @param {string&} ip 修改后的ip地址
 * @return {Response}
 */
cpr::Response modifyDNS(const std::string& dnsUrl, const std::string& ip) {
    std::ostringstream jsonStream;
    jsonStream << R"({"ip":")" << ip << R"("})";
    std::string jsonBody = jsonStream.str();  // 使用原始字符串字面量来创建JSON字符串

    cpr::Response response = cpr::Post(
        cpr::Url{dnsUrl},
        cpr::Body{jsonBody},
        cpr::Header{{"Content-Type", "application/json"}}  // 设置Content-Type为application/json
    );

    return response;
}


/**
 * @description: 封装停止容器的函数
 * @param {string&} ip_str 本地ip地址
 * @return {Response}
 */
cpr::Response stopContainer(const std::string& ip_str) {
    // 组合 URL
    std::string url_stop = "http://" + ip_str + ":2375/containers/PISKES_Server/stop";

    // 执行 POST 请求
    cpr::Response responseContainerStop = cpr::Post(cpr::Url{url_stop});

    return responseContainerStop;
}

/**
 * @description: 封装启动容器的函数
 * @param {string&} dst_ip 迁移目的节点ip地址
 * @return {Response}
 */
cpr::Response startContainer(const std::string& dst_ip) {
    // 组合 URL
    std::string url = "http://" + dst_ip + ":2375/containers/PISKES_Server/start";

    // 执行 POST 请求
    cpr::Response response = cpr::Post(cpr::Url{url});

    return response;
}