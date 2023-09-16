/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:39:30
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 15:08:27
 * @FilePath: /home/ServiceMigration/src/migration_process/migration_process.cpp
 * @Description: 
 */
#include "Logger.h"
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
#include <drogon/HttpController.h>
extern std::string etcdUrl;
extern std::string dnsUrl;
extern std::string nodeIp;
float imbalanceThresholdComputing = 0.7; 
float imbalanceThresholdStore = 0.7;     
int overloadFrequencyThreshold = 20;     




namespace DummyNodes
{
    //网络拓扑的相关函数操作，包括初始化、添加、删除等操作
    void globalInit(struct Global *g)
    {
        memset(g, 0, sizeof(struct Global));
    }

    void nodeInit(struct Node *n)
    {
        memset(n, 0, sizeof(struct Node));
    }

    void globalPush(struct Global *g, struct Node *n)
    {
        if (g->tail)
        {
            g->tail->next = n;
            n->prior = g->tail;
        }
        else
        {
            g->head = n;
            g->head->prior = NULL;
        }

        g->tail = n;
        g->tail->next = NULL;
        g->count++;
    }

    void globalDelete(struct Global *g, struct Node *n)
    {
        if (n == g->head)
        {
            g->head = n->next;
            g->head->prior = NULL;
        }
        if (n == g->tail)
        {
            g->tail = n->prior;
            g->tail->next = NULL;
        }
        else
        {
            n->next->prior = n->prior;
            n->prior->next = n->next;
        }
        free(n);
    }


    
    /*
    * 函数名: getGraph()
    * 函数功能：获取网络拓扑信息
    * 变量含义:

    */
    int get_graph(struct Graph *g)
    {
        char buff[1024];
        //获取图，从文件打开，后续应该是从接口中获取
        FILE *fp = fopen("graph.txt", "r");

        if (NULL == fp)
        {
            // printf("open error\n");
            erro("open error");
            return 0;
        }
        fgets(buff, sizeof(buff) - 1, fp);

        sscanf(buff, "vertexnum,edgenum:%d %d\n", &(g->vexnum), &(g->edgenum));

        for (int i = 0; i < g->vexnum; i++)
        {
            g->vertex[i] = i;
        }

        for (int i = 0; i < g->vexnum; i++)
        {
            fgets(buff, sizeof(buff) - 1, fp);
            //printf("strlen(buff) = %d, V%d ip = %s", strlen(buff), i, buff);
            strncpy(*(g->ip + i), buff, strlen(buff) - 1);
            //strncpy((char *)ip+20*i, buff, strlen(buff)-1);
            // printf("%s", (char *)ip+20*i);
            // printf("%s", (char *)ip+20*i)
            *(*(g->ip + i) + strlen(buff) - 1) = '\0';
            //*((char *)ip + i*20 + (strlen(buff) - 1)) = '\0';
            //printf("%s\n", *(ip+i));
        }

        for (int i = 0; i < g->vexnum; i++)
        {
            for (int j = 0; j < g->vexnum; j++)
            {
                g->edge[i][j] = INFINITY;
            }
        }

        int v1, v2, w;

        while (1)
        {

            fgets(buff, sizeof(buff) - 1, fp);
            //printf("%s\n", buff);

            sscanf(buff, "%d %d %d\n", &v1, &v2, &w);
            //printf("%d %d %d\n", v1, v2, w);
            g->edge[v1][v2] = w;
            g->edge[v2][v1] = w;

            if (feof(fp))
            {
                break;
            }
        }

        fclose(fp);
        return 1;
    }

        /**
     * @description: 获取本地资源信息
     * @return {LocalHost}
     */    
    DummyNodes::LocalHost *getLocalhost()
    {
        //info("logger 测试");
        char buff[1024];
        struct LocalHost *newlocalhost;
        newlocalhost = (struct LocalHost *)malloc(sizeof(struct LocalHost));
        /*
        get etcdUrl and dnsUrl
        */
        std::ifstream file("configuration.json", std::ifstream::binary);
        if (!file.is_open()) {
            // std::cout << "Error opening configuration file." << std::endl;
            erro("Error opening configuration file.");
           
        }

        // 创建 Json::Reader 和 Json::Value 对象
        Json::CharReaderBuilder builder;
        Json::Value root;
        std::string errs;

        // 从文件解析 JSON 数据
        if (!Json::parseFromStream(builder, file, &root, &errs)) {
            // std::cout << "Error parsing JSON: " << errs << std::endl;
            erro("Error parsing JSON");
            
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
        strcpy(newlocalhost->ip, nodeIp.c_str());

        return newlocalhost;
    }

    /*
    * 函数名: shortestPathDijkstra()
    * 函数功能：最短路径算法获取本节点到其他节点的最短跳数
    * 变量含义:
    * Graph——网络节点拓扑图
    * pathMatrix——路径矩阵
    * shortPathTable——最短跳数表
    */
    void shortestPathDijkstra(struct Graph *G, int v0, pathMatrix *p, shortPathTable *D)
    {
        int final[MAX_VERTEX_NUM]; //用于存储各顶点是否已经确定最短路径的数组
        //对各数组进行初始化
        for (int v = 0; v < G->vexnum; v++)
        {
            final[v] = 0;
            (*D)[v] = G->edge[v0][v];
            (*p)[v] = 0;
            std::cout<<G->edge[v0][v]<<std::endl;
        }
        //由于以v0位下标的顶点为起始点，所以不用再判断
        (*D)[v0] = 0;
        final[v0] = 1;
        int k = 0;
        for (int i = 0; i < G->vexnum; i++)
        {
            int min = INFINITY;
            //选择到各顶点权值最小的顶点，即为本次能确定最短路径的顶点
            for (int w = 0; w < G->vexnum; w++)
            {
                if (!final[w])
                {
                    if ((*D)[w] < min)
                    {
                        k = w;
                        min = (*D)[w];
                    }
                }
            }
            //设置该顶点的标志位为1，避免下次重复判断
            final[k] = 1;
            //对v0到各顶点的权值进行更新
            for (int w = 0; w < G->vexnum; w++)
            {
                if (!final[w] && (min + G->edge[k][w] < (*D)[w]))
                {
                    (*D)[w] = min + G->edge[k][w];
                    (*p)[w] = k; //记录各个最短路径上存在的顶点
                }
            }
        }
        std::cout<<  "测试dijstra" <<(*D)[0]<<(*D)[1]<<(*D)[2]<<std::endl;
    }
    /*
    * 函数名: getGain()
    * 函数功能：计算迁移收益
    * 变量含义:

    */
    void getGain(struct Global *g, char (*ip)[20], shortPathTable *w, int num)
    {
        struct Node *n = g->head;
        // char *value_ip;
        for (int i = 0; i < g->count; i++)
        {
            // value_ip = n->value.ip;
            // value_ip = &n->value.ip;

            for (int j = 0; j < num; j++)
            {
                if (strcmp(n->value.ip, *(ip + j)))
                {
                    continue;
                }
                
                //printf("V%d cost: %d\n", j, (*w)[j]);
                n->cost = (*w)[j];
                //printf("------%s getGain: %d\n", n->value.ip, n->cost);
                //std::cout << "测试get_gain" << std::endl;
                n->gain = (n->benefit + n->cost) / 2;
                std::cout << "============================================" << std::endl;
                
                    
                    std::cout << n->value.ip << "  cost: " << n->cost << "  benifit: " << n->benefit << "  gain: " << n->gain << std::endl;
                    
                
                std::cout << "============================================" << std::endl;
            }

            n = n->next;
        }
    }
    /*
    * 函数名: getBestNode()
    * 函数功能：计算最优迁移节点
    * 变量含义:

    */
    struct Node *getBestNode(struct Global *g)
    {
        struct Node *n = g->head;
        struct Node *best = g->head;

        for (int i = 0; i < g->count; i++)
        {
            if (best->gain <= n->gain)
            {
                n = n->next;
                continue;
            }
            best = n;
            n = n->next;
        }

        return best;
    }
    /*
    * 函数名: printCost()
    * 函数功能：打印最短跳数 目前并未使用
    * 变量含义:
    * shortPathTable
    */
    void printCost(shortPathTable *t, int num)
    {
        for (int i = 0; i < num; i++)
        {
            printf("V%d cost: %d\n", i, (*t)[i]);
        }
    }




 
    /*
    * 函数名: printGlobal()
    * 函数功能：打印全网所有节点的迁移收益
    * 变量含义:

    */
    void printGlobal(struct Global *g)
    {
        struct Node *n = g->head;
        // printf("--------\n");
        std::cout << "============================================" << std::endl;
        for (int i = 0; i < g->count; i++)
        {
            // printf("%s cost: %d, benifit: %f, gain: %f\n", n->value.ip, n->cost, n->benefit, n->gain);
            std::cout << n->value.ip << "  cost: " << n->cost << "  benifit: " << n->benefit << "  gain: " << n->gain << std::endl;
            n = n->next;
        }
        std::cout << "============================================" << std::endl;
        // printf("-------\n");
    }

    /*
    * 函数名: standardCalculate()
    * 函数功能：计算迁移花费
    * 变量含义:

    */
    void standardCalculate(struct Node *n)
    {
        float ave1, ave2, stand1, stand2;

        ave1 = (n->value.computing + n->value.store ) / 2.0;
        ave2 = (n->value.computing + n->value.store + 0.5) / 2.0;
        stand1 = ((n->value.computing  - ave1) * (n->value.computing  - ave1) + (n->value.store  - ave1) * (n->value.store  - ave1)) / 2.0;
        stand2 = ((n->value.computing + 0.2 - ave1) * (n->value.computing + 0.2 - ave1) + (n->value.store + 0.3 - ave1) * (n->value.store + 0.3 - ave1)) / 2.0;

        n->benefit = (stand2 - stand1) / ( stand1 + 0.01);
    }

    /*
    * 函数名: getBenefit()
    * 函数功能：
    * 变量含义:

    */
    void getBenefit(struct Global *g)
    {
        struct Node *n;
        n = g->head;

        do
        {
            if (NULL == n)
            {
                break;
            }
            standardCalculate(n);
            if (n->benefit <= 0)
            {
                globalDelete(g, n);
            }
            n = n->next;
        } while (NULL != n);
    }


    
    /**
     * @description: 创建拓扑图
     * @return {*}
     */    
    void PolicyInterface::getGraph()
    {
        struct Graph *G;
        G = (struct Graph *)malloc(sizeof(struct Graph));
        _graph = G;
        get_graph(_graph);
    }

    //添加迁移判决内部函数
    /**
     * @description: 添加迁移判决内部函数，判断超过阈值的频繁程度
     * @return {*}
     */    
    int PolicyInterface::getImbalance()
    {
        //应该需要重新获取本地节点资源信息，只是使用对象的数值应该是不会变的
        float call;
        float stoo;

        std::string str_url(etcdUrl);
        etcd::Client etcd(str_url);
        std::string ip(_localhost->ip);
        std::string index1 = "/" + ip + "/cal/cpuUsage/";
        std::string index2 = "/" + ip + "/sto/memUsage/";
        etcd::Response response1 = etcd.get(index1).get();
        etcd::Response response2 = etcd.get(index2).get();
        call = stof(response1.value().as_string());
        stoo = stof(response2.value().as_string());
        if (call > imbalanceThresholdComputing || stoo > imbalanceThresholdStore)
        {
            std::cout << "get_imbalance测试" << std::endl;
            return 0;
        }
        else
        {

            std::cout << "get_imbalance测试2" << std::endl;
            return 1;
        }
    };
    /**
     * @description: 获取20个时间点内超过阈值的次数
     * @return {*}cunt次数
     */    
    int PolicyInterface::getOverloadFrequency()
    {
        int cunt = 0;
        int i;
        if (!getImbalance())
        {
            cunt++;
            for (i = 0; i < 20; i++)
            {
                if (!getImbalance())
                {
                    cunt++;
                }
                sleep(0.2);
            }
        }
        std::cout << "get_overload_frequency测试" << std::endl;
        return cunt;
    };
    /**
     * @description: 获取本节点的资源信息
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::localResourceGet()
    {
        std::cout << "trigger getLocalResource" << std::endl;

        // struct LocalHost *LocalHost;
        _localhost = getLocalhost();
        // _localhost = LocalHost;
        
        std::string str_url(etcdUrl);
        etcd::Client etcd(str_url);

        std::string ip(_localhost->ip);

        //格式：/IPaddress/type/res/param/,type是指四类资源："forward", "storage", "calculate", "safe";
        //res就是资源的名称； param就是“ctn”,"image", "disk"
        std::string index1 = "/" + ip + "/cal/cpuUsage/";
        std::string index2 = "/" + ip + "/sto/memUsage/";
        // std::string index1 = "/" + ip + "/cal/memUsage/";
        // std::string index2 = "/" + ip + "/storage/diskUsage/";

        std::cout << "trigger getLocalResource -----1" << std::endl;

        etcd::Response response1 = etcd.get(index1).get();
        etcd::Response response2 = etcd.get(index2).get();
        if(response1.error_code() )
        {   
            std::cout<<response1.error_code()<<std::endl;
            return BT::NodeStatus::FAILURE;
        }
        std::cout << "trigger getLocalResource -----2" << std::endl;

        std::cout << "local_cal:" <<response1.value().as_string() << std::endl;
        std::cout << "local_store:" <<response2.value().as_string() << std::endl;

        _localhost->computing = stof(response1.value().as_string());
        _localhost->store = stof(response2.value().as_string());

        std::cout << "trigger getLocalResource -----3" << std::endl;

        std::cout << "Get LocalResource Success!!" << std::endl;
        return BT::NodeStatus::SUCCESS;
    };
    /**
     * @description: 判断cpu资源是否超过阈值
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::isCpuOver()
    {
        std::cout << "CPU资源检查" << std::endl;

        if (_localhost->computing > imbalanceThresholdComputing)
        {
            std::cout << "CPU Resource exceeds threshold" << std::endl;
            return BT::NodeStatus::FAILURE;
        }

        std::cout << "CPU Resource did not exceed threshold" << std::endl;
        return BT::NodeStatus::SUCCESS;
    };
    /**
     * @description: 判断内存是否超过阈值
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::isMemOver()
    {
        std::cout << "内存资源检查" << std::endl;

        if (_localhost->store > imbalanceThresholdStore)
        {
            std::cout << "MEM Resource exceeds threshold" << std::endl;
            return BT::NodeStatus::FAILURE;
        }

        std::cout << "MEM Resource did not exceed threshold" << std::endl;
        return BT::NodeStatus::SUCCESS;
    };
    /**
     * @description: 获取所有可迁移节点
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::migrationNodesGet()
    {
        if (_resource == true)
            return BT::NodeStatus::FAILURE;

        struct Global *newglobal;
        struct Node *newnode;
        newglobal = (struct Global *)malloc(sizeof(struct Global));
            if (newglobal == NULL) {
                // handle error
                std::cout<<"newglobal == NULL"<<std::endl;
            }

        newnode = (struct Node *)malloc(sizeof(struct Node));
            if (newnode == NULL) {
                // handle error
                std::cout<<"newglobal == NULL"<<std::endl;
            }

        // struct LocalHost *LocalHost;

        //std::string str_url = "http://192.168.0.155:2379";
        std::string str_url(etcdUrl);
        etcd::Client etcd(str_url);

        // LocalHost = getLocalhost();
        newglobal = (struct Global *)malloc(sizeof(struct Global));
        globalInit(newglobal);

        std::ifstream file("graph.txt");
        if (!file.is_open()) {
            std::cerr << "Could not open file\n";
            
        }

        std::vector<std::string> nodes;
        std::string line;

        // 首先读取第一行并解析出节点数量
        std::getline(file, line);
        std::stringstream ss(line);
        std::string token;
        std::getline(ss, token, ':'); // 读取":"前的字符串，也就是"vertexnum,edgenum"
        int vertexnum;
        ss >> vertexnum; // 读取":"后的第一个数字，也就是节点数量

        // 读取相应数量的IP地址
        for (int i = 0; i < vertexnum; ++i) {
            if (std::getline(file, line)) {
                nodes.push_back(line);
            }
        }

        file.close();

        for (int i = 0; i < nodes.size(); i++)
        {
            newnode = (struct Node *)malloc(sizeof(struct Node));
            nodeInit(newnode);
            std::string ip = nodes[i];
            strcpy(newnode->value.ip, ip.c_str());

            if (!strcmp(_localhost->ip, newnode->value.ip))
            {
                free(newnode);
                newnode = NULL;
                continue;
            }

            // std::string index1 = "/" + ip + "/cal";
            // std::string index2 = "/" + ip + "/store";

            //格式：/IPaddress/type/res/param/,type是指四类资源："forward", "storage", "calculate", "safe";
            //res就是资源的名称； param就是“ctn”,"image", "disk"
            std::string index1 = "/" + ip + "/cal/cpuUsage/";
            std::string index2 = "/" + ip + "/sto/memUsage/";
            // std::string index1 = "/" + ip + "/cal/memUsage/";
            // std::string index2 = "/" + ip + "/storage/diskUsage/";

            etcd::Response response1 = etcd.get(index1).get();
            etcd::Response response2 = etcd.get(index2).get();

            newnode->value.computing = stof(response1.value().as_string());
            newnode->value.store = stof(response2.value().as_string());
            //dubug
            std::cout<<newnode->value.ip<<":"<<newnode->value.computing<<";"<<newnode->value.store<<std::endl;

            if (newnode->value.computing > 0.6 || newnode->value.store > 0.5)
            {
                free(newnode);
                newnode = NULL;
                continue;
            }

            globalPush(newglobal, newnode);
            newnode = NULL;
        }
        
        _global = newglobal;
        // _localhost = LocalHost;
        getBenefit(_global);
        
        //printGlobal(newglobal);

        // getBenefit(_global);
        getGraph();
        
        std::cout << "get nodes success" << std::endl;
        return BT::NodeStatus::SUCCESS;
    };


    /**
     * @description: 计算得到最优迁移节点
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::bestNodeGet()
    {
        pathMatrix *new_P;
        new_P = (pathMatrix *)malloc(sizeof(pathMatrix));
        _P = new_P;
        shortPathTable *new_D;
        new_D = (shortPathTable *)malloc(sizeof(shortPathTable));
        _D = new_D;

        std::cout << "11111111" << std::endl;

        shortestPathDijkstra(_graph, 0, _P, _D);
        std::cout << "22222222" << std::endl;

        getGain(_global, _graph->ip, _D, _graph->vexnum);
        
        std::cout << "33333333" << std::endl;
        _best = getBestNode(_global);
        std::cout << "44444444" << std::endl;
        std::cout << "============================================" << std::endl;
        std::cout << "the best Node:" << _best->value.ip << std::endl;
        std::cout << "============================================" << std::endl;

        _policy = true;

        // std::cout << "Migration policy specified successfully!!" << std::endl;
        info("Migration policy specified successfully!!")
       
        return BT::NodeStatus::SUCCESS;
    };
    /**
     * @description: 获取阈值超出后的次数
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::overloadCountGet()
    {
        _overloadCount = getOverloadFrequency();
        return BT::NodeStatus::SUCCESS;


    };
    /**
     * @description: 判断是否过载
     * @return {*}
     */
    BT::NodeStatus PolicyInterface::isOverload()
    {
        
        if (_overloadCount > overloadFrequencyThreshold)
        {
            if (!getImbalance())
            {
                return BT::NodeStatus::FAILURE; // 如果资源利用严重不均衡，并且频繁超载，那么我们返回失败，表示需要进行迁移
            }
            else
            {
                return BT::NodeStatus::SUCCESS; // 否则，我们返回成功，表示不需要进行迁移
            }
        }
        else
        {
            return BT::NodeStatus::SUCCESS;
        }
    };
    /**
     * @description: 运行目的节点容器
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::bestNodeRun()
    {
        std::string bestNodeIp(_best->value.ip);
        //开启容器接口函数调用
        cpr::Response response = startContainer(bestNodeIp);
        if (response.status_code == 204) {
            // std::cout << "Container started successfully." << std::endl;
            info("Container started successfully.");
        } else if(response.status_code == 500) {
            // std::cerr << "Failed to start container." << std::endl;
            erro("Failed to start container.");
            return BT::NodeStatus::FAILURE;
        }else if(response.status_code == 304){
            info("container already run");
        }else{
            return BT::NodeStatus::FAILURE;
        }

        _distribute = true;
        
        //数据迁移接口函数
        std::string nodeIp(_localhost->ip); // 将字符数组转换为 std::string
        migrateAllKeys(nodeIp, 6379, bestNodeIp, 6379, "miyaoredis");
        std::cout << "DATA MIGRATION" << std::endl;
        std::cout << "policy distribute success" << std::endl;
        return BT::NodeStatus::SUCCESS;
    };
    /**
     * @description: 判断目的节点容器是否开启
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::isPolicySuccess()
    {
        if(_distribute){
            return BT::NodeStatus::FAILURE;
        }
        return BT::NodeStatus::SUCCESS;
    };
    /**
     * @description: 关闭本节点容器，修改域名地址
     * @return {*}
     */    
    BT::NodeStatus PolicyInterface::update()
    {
        std::string bestNodeIp(_best->value.ip);
        cpr::Response response11 = stopContainer(nodeIp);
        if (response11.status_code == 204) {
            // std::cout << "Container stopped successfully." << std::endl;
            info("Container stopped successfully.");
        } else if (response11.status_code == 304) {
            // std::cerr << "Failed to stop container." << std::endl;
            info("Container already stopped.");
            
        }else{
            erro("Failed to stop container.");
            return BT::NodeStatus::FAILURE;
        }
        //调用修改DNS接口函数
        cpr::Response responseDns = modifyDNS(dnsUrl, bestNodeIp);
        std::cout << responseDns.text << std::endl;
        std::cout << "DNS修改成功" << std::endl;
                
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k200OK);
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        

        return BT::NodeStatus::SUCCESS;
    };
    
}
