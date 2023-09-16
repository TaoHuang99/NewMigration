/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:42:44
 * @LastEditors: Huang Tao taoh0790@gmail.com
 * @LastEditTime: 2023-09-05 09:07:31
 * @FilePath: /home/ServiceMigration/include/migration_api.h
 * @Description: 
 */
#include <iostream>
#include <string>
#include "cpr/cpr.h"
cpr::Response modifyDNS(const std::string& dnsUrl, const std::string& ip);
cpr::Response stopContainer(const std::string& ip_str);
cpr::Response startContainer(const std::string& dst_ip);