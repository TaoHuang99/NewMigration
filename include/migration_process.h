/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:42:58
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 10:10:13
 * @FilePath: /home/ServiceMigration/include/migration_process.h
 * @Description: 
 */

#ifndef SIMPLE_BT_NODES_H
#define SIMPLE_BT_NODES_H

#include <string>
#include <hiredis/hiredis.h>
#include <iostream>
#include "behaviortree_cpp_v3/behavior_tree.h"
#include "behaviortree_cpp_v3/bt_factory.h"
#include "migration_action.h"
#define MAX_VERTEX_NUM 30
#define INFINITY 65535

/*
*DummyNodes命名空间表示将整个网络拓扑中的节点抽象出来，并使用结构体保存各个节点的信息，包括ip、资源利用率等等

*/
namespace DummyNodes
{

  struct Graph {
      int vertex[MAX_VERTEX_NUM];
      int edge[MAX_VERTEX_NUM][MAX_VERTEX_NUM];
      int vexnum, edgenum;
      char ip[MAX_VERTEX_NUM][20];
  };

  typedef int pathMatrix[MAX_VERTEX_NUM];     //The subscript used to store the vertices passed in the shortest path
  typedef int shortPathTable[MAX_VERTEX_NUM]; //Used to store the weight sum of each shortest path

  // typedef char ip[MAX_VERTEX_NUM][20];   //用于存储各个节点的ip

  struct LocalHost {
    char ip[20];
    float computing;
    float store;
  };

  struct Node {
      struct Node *next;
      struct Node *prior;
      struct LocalHost value;
      float benefit;
      int cost;
      float gain;
  };

  struct Global {
      struct Node *head;
      struct Node *tail;
      int count;
  };


  using BT::NodeStatus;

  

  class PolicyInterface
  {
    // friend class PolicyInterface;

    public:
      //Add Migration Judgment Class Members

      //
      PolicyInterface() : _policy(false) {}
      //迁移节点预选
      
      NodeStatus getSource();

      void getGraph();
      int getImbalance();
      int getOverloadFrequency();


      NodeStatus localResourceGet();

      NodeStatus migrationNodesGet();

      NodeStatus isCpuOver();

      NodeStatus isMemOver();

      NodeStatus bestNodeGet();

      NodeStatus overloadCountGet();

      NodeStatus isOverload();

      NodeStatus bestNodeRun();

      NodeStatus isPolicySuccess();

      NodeStatus update();
      

      int _sockfd;

      int _overloadCount = 0;

      struct Node *_best;

      bool _policy = false;

      bool _distribute = false;

      bool _resource = false;

    private:
      // bool _source;

      struct LocalHost *_localhost;

      struct Global *_global;

      struct Graph *_graph;

      // bool _policy;

      pathMatrix *_P;

      shortPathTable *_D;

      // struct Node *_best;
      
      // int _sockfd;

  };

  /*not use*/
  class GripperInterface
  {
    public:
      GripperInterface() : _opened(true)
      {
      }

      NodeStatus open();

      NodeStatus close();

    private:
      bool _opened;
  };



} // end namespace

#endif   // SIMPLE_BT_NODES_H