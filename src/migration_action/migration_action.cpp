/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:38:28
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 10:14:04
 * @FilePath: /home/ServiceMigration/src/migration_action/migration_action.cpp
 * @Description: 
 */
#include "Logger.h"
#include <hiredis/hiredis.h>
#include "migration_process.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <etcd/Client.hpp>
#include <etcd/Response.hpp>
#include "load_configuration.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "migration_api.h"
#include "cpr/cpr.h"
#include "jsoncpp/json/json.h"
extern std::string etcdUrl;
extern std::string dnsUrl;
extern std::string nodeIp;
/**
 * @description:迁移单个数据 
 * @return {*}
 * sourceHost——本机ip地址
 * sourcePort——本机密钥redis数据库端口号
 * sourceHost——目的ip地址
 * sourcePort——目的密钥redis数据库端口号
 */
void migrateKey(const std::string& sourceHost, int sourcePort,
                const std::string& targetHost, int targetPort,
                const std::string& key, const std::string& password) {
    // 连接到源Redis服务器
    redisContext* sourceContext = redisConnect(sourceHost.c_str(), sourcePort);
    if (sourceContext == NULL || sourceContext->err) {
        // std::cout << "Error: cannot connect to the source Redis server." << std::endl;
        erro("Error: cannot connect to the source Redis server.");
        return;
    }

    // 在源服务器进行认证
    redisReply* reply = static_cast<redisReply*>(redisCommand(sourceContext, "AUTH %s", password.c_str()));
    if (reply->type == REDIS_REPLY_ERROR) {
        // std::cout << "Authentication failed on the source Redis server." << std::endl;
        info("Authentication failed on the source Redis server.")
        return;
    }

    // 在源服务器上DUMP key
    reply = static_cast<redisReply*>(redisCommand(sourceContext, "DUMP %s", key.c_str()));
    if (reply == NULL) {
        // std::cout << "Error: cannot dump the key on the source Redis server." << std::endl;
        erro("Error: cannot dump the key on the source Redis server.");
        return;
    }

    // 保存DUMP的结果
    std::string dumpedValue(reply->str, reply->len);

    // 连接到目标Redis服务器
    redisContext* targetContext = redisConnect(targetHost.c_str(), targetPort);
    if (targetContext == NULL || targetContext->err) {
        // std::cout << "Error: cannot connect to the target Redis server." << std::endl;
        erro("Error: cannot connect to the target Redis server.");
        return;
    }

    // 在目标服务器进行认证
    reply = static_cast<redisReply*>(redisCommand(targetContext, "AUTH %s", password.c_str()));
    if (reply->type == REDIS_REPLY_ERROR) {
        // std::cout << "Authentication failed on the target Redis server." << std::endl;
        info("Authentication failed on the target Redis server.");
        return;
    }

    // 在目标服务器上RESTORE key
    reply = static_cast<redisReply*>(redisCommand(targetContext, "RESTORE %s 0 %b", key.c_str(), dumpedValue.data(), dumpedValue.size()));
    if (reply == NULL) {
        // std::cout << "Error: cannot restore the key on the target Redis server." << std::endl;
        erro("Error: cannot restore the key on the target Redis server.");
        return;
    }

    // 关闭连接
    redisFree(sourceContext);
    redisFree(targetContext);
}

/**
 * @description: 迁移所有数据
 * @return {*}
 * sourceHost——本机ip地址
 * sourcePort——本机密钥redis数据库端口号
 * sourceHost——目的ip地址
 * sourcePort——目的密钥redis数据库端口号
 */
void migrateAllKeys(const std::string& sourceHost, int sourcePort,
                    const std::string& targetHost, int targetPort,
                    const std::string& password) {
    // 连接到源Redis服务器
    redisContext* sourceContext = redisConnect(sourceHost.c_str(), sourcePort);
    if (sourceContext == NULL || sourceContext->err) {
        // std::cout << "Error: cannot connect to the source Redis server." << std::endl;
        erro("Error: cannot connect to the source Redis server.");
        return;
    }

    // 在源服务器进行认证
    redisReply* reply = static_cast<redisReply*>(redisCommand(sourceContext, "AUTH %s", password.c_str()));
    if (reply->type == REDIS_REPLY_ERROR) {
        // std::cout << "Authentication failed on the source Redis server." << std::endl;
        info("Authentication failed on the source Redis server.");
        return;
    }

    // 获取所有的键
    reply = static_cast<redisReply*>(redisCommand(sourceContext, "KEYS *"));
    if (reply == NULL) {
        // std::cout << "Error: cannot get keys from the source Redis server." << std::endl;
        erro("rror: cannot get keys from the source Redis server.");
        return;
    }

    // 对每一个键，进行迁移操作
    for (size_t i = 0; i < reply->elements; i++) {
        migrateKey(sourceHost, sourcePort, targetHost, targetPort, reply->element[i]->str, password);
    }

    // 关闭连接
    redisFree(sourceContext);
}
