/*
 * @Author: Huang Tao taoh0790@gmail.com
 * @Date: 2023-09-04 18:57:56
 * @LastEditors: Huang Tao taoh0790@gmail.com
 * @LastEditTime: 2023-09-04 19:05:01
 * @FilePath: /home/ServiceMigration/include/container_service_migration.h
 * @Description: 
 */
#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace container
{
class service_migration : public drogon::HttpController<service_migration>
{
  public:
    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    // METHOD_ADD(service_migration::get, "/{2}/{1}", Get); // path is /container/service_migration/{arg2}/{arg1}
    // METHOD_ADD(service_migration::your_method_name, "/{1}/{2}/list", Get); // path is /container/service_migration/{arg1}/{arg2}/list
    // ADD_METHOD_TO(service_migration::your_method_name, "/absolute/path/{1}/{2}/list", Get); // path is /absolute/path/{arg1}/{arg2}/list

    METHOD_ADD(service_migration::trigger, "/trigger", Post);
    METHOD_ADD(service_migration::cmd, "/cmd", Post);
    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    // void your_method_name(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, double p1, int p2) const;
    void trigger(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);
    void cmd(const HttpRequestPtr &req, std::function<void (const HttpResponsePtr &)> &&callback);

};
}
