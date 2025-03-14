#ifndef __MY_DATA__
#define __MY_DATA__
#include "Util.hpp"
#include <cstdlib>
#include <mutex>
#include <mariadb/mysql.h>

using namespace log_es;

namespace vod
{
    // 定义数据库连接所需的主机地址
    #define HOST "127.0.0.1"
    // 定义数据库连接所需的用户名
    #define USER "xxx_name"
    // 定义数据库连接所需的密码
    #define PASS "xxx_pass"
    // 定义要连接的数据库名称
    #define NAME "vod_system"

    // 初始化 MySQL 连接
    static MYSQL *MysqlInit()
    {
        // 初始化一个 MySQL 实例
        MYSQL *mysql = mysql_init(NULL);
        // 检查 MySQL 实例是否初始化成功
        if (mysql == NULL)
        {
            // 记录错误日志
            LOG(ERROR,"INIT MYSQL INSTANCE FAILED!\n");
            return NULL;
        }
        // 尝试连接到 MySQL 服务器
        if (mysql_real_connect(mysql, HOST, USER, PASS, NAME, 0, NULL, 0) == NULL)
        {
            // 记录错误日志
            LOG(ERROR,"CONNECT MYSQL SERVER FAILED!\n");
            // 关闭 MySQL 连接
            mysql_close(mysql);
            return NULL;
        }
        // 设置字符集为 utf8
        mysql_set_character_set(mysql, "utf8");
        return mysql;
    }

    // 销毁 MySQL 连接
    static void MysqlDestroy(MYSQL *mysql)
    {
        // 检查 MySQL 连接指针是否为空
        if (mysql != NULL)
        {
            // 关闭 MySQL 连接
            mysql_close(mysql);
        }
        return;
    }

    // 执行 MySQL 查询
    static bool MysqlQuery(MYSQL *mysql, const std::string &sql)
    {
        // 执行 SQL 查询
        int ret = mysql_query(mysql, sql.c_str());
        // 检查查询是否执行成功
        if (ret != 0)
        {
            // 输出执行的 SQL 语句
            std::cout << sql << std::endl;
            // 输出 MySQL 错误信息
            std::cout << mysql_error(mysql) << std::endl;
            return false;
        }
        return true;
    }

    // 视频表操作类
    class TableVideo
    {
    private:
        // MySQL 连接指针
        MYSQL *_mysql;
        // 互斥锁，用于保护多线程访问时的数据安全
        std::mutex _mutex;

    public:
        // 构造函数，初始化 MySQL 连接
        TableVideo()
        {
            // 调用 MysqlInit 函数初始化 MySQL 连接
            _mysql = MysqlInit();
            // 检查 MySQL 连接是否成功
            if (_mysql == NULL)
            {
                // 若连接失败，退出程序
                exit(-1);
            }
        }

        // 析构函数，销毁 MySQL 连接
        ~TableVideo()
        {
            // 调用 MysqlDestroy 函数关闭 MySQL 连接
            MysqlDestroy(_mysql);
        }

        // 向视频表中插入一条记录
        bool Insert(const Json::Value &video)
        {
            // 视频表的字段：id, name, info, video, image
            std::string sql;
            // 调整字符串大小，防止简介过长
            sql.resize(4096 + video["info"].asString().size());
            // 定义插入语句的格式化字符串
            #define INSERT_VIDEO "insert tb_video values(null, '%s', '%s', '%s', '%s');"
            // 检查视频名称是否为空
            if (video["name"].asString().size() == 0)
            {
                return false;
            }
            // 使用 sprintf 函数生成插入语句
            sprintf(&sql[0], INSERT_VIDEO, video["name"].asCString(),
                    video["info"].asCString(),
                    video["video"].asCString(),
                    video["image"].asCString());
            // 调用 MysqlQuery 函数执行插入语句
            return MysqlQuery(_mysql, sql);
        }

        // 更新视频表中的一条记录
        bool Update(int video_id, const Json::Value &video)
        {
            std::string sql;
            // 调整字符串大小，防止简介过长
            sql.resize(4096 + video["info"].asString().size());
            // 定义更新语句的格式化字符串
            #define UPDATE_VIDEO "update tb_video set name='%s', info='%s' where id=%d;"
            // 使用 sprintf 函数生成更新语句
            sprintf(&sql[0], UPDATE_VIDEO, video["name"].asCString(),
                    video["info"].asCString(), video_id);
            // 调用 MysqlQuery 函数执行更新语句
            return MysqlQuery(_mysql, sql);
        }

        // 删除视频表中的一条记录
        bool Delete(int video_id)
        {
            // 定义删除语句的格式化字符串
            #define DELETE_VIDEO "delete from tb_video where id=%d;"
            char sql[1024] = {0};
            // 使用 sprintf 函数生成删除语句
            sprintf(sql, DELETE_VIDEO, video_id);
            // 调用 MysqlQuery 函数执行删除语句
            return MysqlQuery(_mysql, sql);
        }

        // 查询视频表中的所有记录
        bool SelectAll(Json::Value *videos)
        {
            // 定义查询所有记录的 SQL 语句
            #define SELECTALL_VIDEO "select * from tb_video;"
            // 加锁，保护查询与保存结果到本地的过程
            _mutex.lock(); 
            // 调用 MysqlQuery 函数执行查询语句
            bool ret = MysqlQuery(_mysql, SELECTALL_VIDEO);
            if (ret == false)
            {
                // 解锁
                _mutex.unlock();
                return false;
            }
            // 存储查询结果
            MYSQL_RES *res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                std::cout << "mysql store result failed!\n";
                // 解锁
                _mutex.unlock();
                return false;
            }
            // 解锁
            _mutex.unlock(); 
            // 获取查询结果的行数
            int num_rows = mysql_num_rows(res);
            for (int i = 0; i < num_rows; i++)
            {
                // 获取一行记录
                MYSQL_ROW row = mysql_fetch_row(res);
                Json::Value video;
                video["id"] = atoi(row[0]);
                video["name"] = row[1];
                video["info"] = row[2];
                video["video"] = row[3];
                video["image"] = row[4];
                // 将记录添加到 videos 中
                videos->append(video);
            }
            // 释放查询结果
            mysql_free_result(res);
            return true;
        }

        // 查询视频表中的一条记录
        bool SelectOne(int video_id, Json::Value *video)
        {
            // 定义查询一条记录的 SQL 语句
            #define SELECTONE_VIDEO "select * from tb_video where id=%d;"
            char sql[1024] = {0};
            // 使用 sprintf 函数生成查询语句
            sprintf(sql, SELECTONE_VIDEO, video_id);
            // 加锁，保护查询与保存结果到本地的过程
            _mutex.lock(); 
            // 调用 MysqlQuery 函数执行查询语句
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                // 解锁
                _mutex.unlock();
                return false;
            }
            // 存储查询结果
            MYSQL_RES *res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                std::cout << "mysql store result failed!\n";
                // 解锁
                _mutex.unlock();
                return false;
            }
            // 解锁
            _mutex.unlock(); 
            // 获取查询结果的行数
            int num_rows = mysql_num_rows(res);
            if (num_rows != 1)
            {
                std::cout << "have no data!\n";
                // 释放查询结果
                mysql_free_result(res);
                return false;
            }
            // 获取一行记录
            MYSQL_ROW row = mysql_fetch_row(res);
            (*video)["id"] = video_id;
            (*video)["name"] = row[1];
            (*video)["info"] = row[2];
            (*video)["video"] = row[3];
            (*video)["image"] = row[4];
            // 释放查询结果
            mysql_free_result(res);
            return true;
        }

        // 模糊查询视频表中的记录
        bool SelectLike(const std::string &key, Json::Value *videos)
        {
            // 定义模糊查询的 SQL 语句
            #define SELECTLIKE_VIDEO "select * from tb_video where name like '%%%s%%';"
            char sql[1024] = {0};
            // 使用 sprintf 函数生成查询语句
            sprintf(sql, SELECTLIKE_VIDEO, key.c_str());
            // 加锁，保护查询与保存结果到本地的过程
            _mutex.lock(); 
            // 调用 MysqlQuery 函数执行查询语句
            bool ret = MysqlQuery(_mysql, sql);
            if (ret == false)
            {
                // 解锁
                _mutex.unlock();
                return false;
            }
            // 存储查询结果
            MYSQL_RES *res = mysql_store_result(_mysql);
            if (res == NULL)
            {
                std::cout << "mysql store result failed!\n";
                // 解锁
                _mutex.unlock();
                return false;
            }
            // 解锁
            _mutex.unlock(); 
            // 获取查询结果的行数
            int num_rows = mysql_num_rows(res);
            for (int i = 0; i < num_rows; i++)
            {
                // 获取一行记录
                MYSQL_ROW row = mysql_fetch_row(res);
                Json::Value video;
                video["id"] = atoi(row[0]);
                video["name"] = row[1];
                video["info"] = row[2];
                video["video"] = row[3];
                video["image"] = row[4];
                // 将记录添加到 videos 中
                videos->append(video);
            }
            // 释放查询结果
            mysql_free_result(res);
            return true;
        }
    };
}

#endif