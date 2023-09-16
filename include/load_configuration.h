/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:42:05
 * @LastEditors: Huang Tao taoh0790@gmail.com
 * @LastEditTime: 2023-09-04 21:26:32
 * @FilePath: /home/ServiceMigration/include/load_configuration.h
 * @Description: 
 */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H
void updateGlobalConfig();
std::string readXMLFromFile(const std::string& filename);
#endif