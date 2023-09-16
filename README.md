<!--
 * @Author: Tao
 * @Date: 2023-09-16 10:29:40
 * @LastEditors: Tao
 * @LastEditTime: 2023-09-16 15:19:04
 * @Description: 
 * @FilePath: /home/ServiceMigration/README.md
-->
# 使用方法
该部分包含大量依赖，最佳使用方式为从docker hub中拉取：
docker pull ttaaoo/service_migration:9.18  （可以根据最新的版本拉取）
运行方法：
## step1：
docker run --network=host --name ServiceMigration -dit ttaaoo/service_migration:9.18
## step2：
进入容器：
docker exec -it 容器名 /bin/bash
开启远程：
/etc/init.d/ssh start
使用ifconfig查看ip后可以使用vscode打开编辑代码一般来说是127开头的ip

cd /home/ServiceMigration 该文件为代码所在部分
build 中使用make可以重新编译，可执行文件也存在与该文件夹下mig_server


# 各个文件含义
文件结构包含bulid、include、lib、models、src等
主要迁移代码位于main.cc controllers/ src/migration-process/ 这些文件下
其中主函数为main.cc
Drogon监听的端口定义在controllers文件夹下的migration-register.cpp以及migration-register.h中，注册了两个post请求接口，分别为trriger以及cmd
src/migration-process/ 文件下的migration-process.cpp以及migration-process.h中定义以及实现行为树绑定的函数




# 容器远程操作的使用方法
有两种远程操作容器的方法，建议使用第二种方法
## 一、
主机docker 接口修改步骤：
### 1.修改
Sudo vim /lib/systemd/system/docker.service
注释下面的并添加新的
#ExecStart=/usr/bin/dockerd -H fd://
ExecStart=/usr/bin/dockerd -H tcp://0.0.0.0:2375 -H unix:///var/run/docker.sock
### 2.重启服务
$ sudo systemctl daemon-reload
$ sudo systemctl restart docker.service
## 二、
或者本机创建python文件
```python
from flask import Flask, jsonify
import docker

app = Flask(__name__)
client = docker.DockerClient(base_url='unix://var/run/docker.sock')

@app.route("/containers/<container_name>/stop", methods=['POST'])
def stop_container(container_name):
    try:
        container = client.containers.get(container_name)
        container.stop()
        return jsonify({'message': f'Container {container_name} stopped successfully!'}), 204
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route("/containers/<container_name>/start", methods=['POST'])
def start_container(container_name):
    try:
        container = client.containers.get(container_name)
        container.start()
        return jsonify({'message': f'Container {container_name} started successfully!'}), 204
    except Exception as e:
        return jsonify({'error': str(e)}), 500
if __name__ == "__main__":
    app.run(host='0.0.0.0', port=2375)  # Listening on all interfaces on port 5000utocmd FileType python set expandtab
