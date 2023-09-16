/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:57:56
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 11:48:14
 * @FilePath: /home/ServiceMigration/controllers/container_service_migration.cc
 * @Description: 
 */
#include "Logger.h"
#include "container_service_migration.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "migration_process.h"
#include "cpr/cpr.h"
#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include "migration_api.h"
#include "load_configuration.h"

using namespace container;



void service_migration::trigger(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) {

    std::cout << "trigger start" << std::endl;
    info("trigger 测试");
    using namespace BT;
    
    // 从外部XML文件中读取行为树定义
    std::string xml_migration = readXMLFromFile("migration_subtree.xml");
    if (xml_migration.empty()) {
        return; // 读取失败
    }
    
    using namespace DummyNodes;

    BehaviorTreeFactory factory;
    PolicyInterface policy;

    factory.registerSimpleAction("GetLocalResource", std::bind(&PolicyInterface::localResourceGet, &policy));
    factory.registerSimpleAction("IsCpuOver", std::bind(&PolicyInterface::isCpuOver, &policy));
    factory.registerSimpleAction("IsMemOver", std::bind(&PolicyInterface::isMemOver, &policy));
    factory.registerSimpleAction("MigrationNodesGet", std::bind(&PolicyInterface::migrationNodesGet, &policy));
    factory.registerSimpleAction("BestNodeGet", std::bind(&PolicyInterface::bestNodeGet, &policy));
    factory.registerSimpleAction("MigrationGetImbalance", std::bind(&PolicyInterface::overloadCountGet, &policy));
    factory.registerSimpleAction("IsOverloadFrequency", std::bind(&PolicyInterface::isOverload, &policy));
    factory.registerSimpleAction("runBestNode", std::bind(&PolicyInterface::bestNodeRun, &policy));
    factory.registerSimpleAction("IsPolicySuccess", std::bind(&PolicyInterface::isPolicySuccess, &policy));
    factory.registerSimpleAction("Update", std::bind(&PolicyInterface::update, &policy));

    auto tree = factory.createTreeFromText(xml_migration);

    std::cout << "trigger step1" << std::endl;
    //info("trigger step1");
    while (!policy._distribute) {

        tree.tickRoot();
        
    }

    // tree.tickRoot();

    Json::Value ret;
    ret["code"] = 200;
    std::string ip(policy._best->value.ip);
    ret["dstnode"] = ip;
    ret["msg"] = "success";

    auto resp = HttpResponse::newHttpJsonResponse(ret);
    callback(resp);

}

    /*
    * 函数名: distribution()
    * 函数功能：管控中心强制服务迁移接口
    * 变量含义:
    * bestNodeIp——最优迁移节点ip地址
    * nodeIp——本机ip地址
    * 
    */
void service_migration::cmd(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback) {
    std::cout << "service_migration cmd start" << std::endl;
    std::string dnsUrl;
    std::string nodeIp;
    //更新配置文件
    updateGlobalConfig();

    auto json = req->getJsonObject();

    if(json)
    {
        std::string container_name = (*json)["container_name"].asString();
        std::string bestNodeIp = (*json)["dstnode"].asString();

        // TODO: 服务迁移逻辑
        //容器开启。bestNodeIp 表示迁移目的节点，
        cpr::Response responseStart = startContainer(bestNodeIp);
        if (responseStart.status_code == 200) {
            std::cout << "Container started successfully." << std::endl;
            //info("Container started successfully.");
        } else {
            std::cerr << "Failed to start container." << std::endl;
        }

        migrateAllKeys(nodeIp, 6379, bestNodeIp, 6379, "miyaoredis");
        std::cout << "Data Migration！" << std::endl;
        std::cout << "policy distribute success" << std::endl;

        //关闭本地容器
        cpr::Response responseStop = stopContainer(nodeIp);
        if (responseStop.status_code == 200) {
            std::cout << "Container stopped successfully." << std::endl;
        } else {
            std::cerr << "Failed to stop container." << std::endl;
        }
        //DNS接口
        cpr::Response responseDns = modifyDNS(dnsUrl, bestNodeIp);
        std::cout << responseDns.text << std::endl;
        std::cout << "DNS修改成功" << std::endl;
        
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        // 使用Json::Value来构建JSON响应
        Json::Value res;
        res["code"] = 200;
        res["msg"] = "成功";

        std::string jsonStr = res.toStyledString();
        resp->setBody(jsonStr);

        callback(resp);
    }
    else
    {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);

        // 使用Json::Value来构建JSON响应
        Json::Value res;
        res["code"] = 400;
        res["msg"] = "Bad Request: JSON object not found or malformed";

        std::string jsonStr = res.toStyledString();
        resp->setBody(jsonStr);

        callback(resp);
    }

}
