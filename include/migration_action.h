/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:42:23
 * @LastEditors: Huang Tao taoh0790@gmail.com
 * @LastEditTime: 2023-09-04 21:17:35
 * @FilePath: /home/ServiceMigration/include/migration_action.h
 * @Description: 
 */

//迁移单个数据
void migrateKey(const std::string& sourceHost, int sourcePort,
                const std::string& targetHost, int targetPort,
                const std::string& key, const std::string& password);
//迁移所有数据
void migrateAllKeys(const std::string& sourceHost, int sourcePort,
                    const std::string& targetHost, int targetPort,
                    const std::string& password);