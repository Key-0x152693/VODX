#ifndef __MY_UTIL__
#define __MY_UTIL__

#include "Log.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <jsoncpp/json/json.h>

using namespace log_es;

namespace vod
{
    // 定义一个文件操作工具类 FileUtil
    class FileUtil
    {
    private:
        // 私有成员变量，存储文件的路径名称
        std::string _name;

    public:
        // 构造函数，接收一个字符串参数 name，用于初始化文件路径名称
        FileUtil(const std::string name) : _name(name) {}

        // 判断文件是否存在的函数
        bool Exists()
        {
            // 使用 access 函数的 F_OK 模式检测文件是否存在，存在则返回 0
            int ret = access(_name.c_str(), F_OK);
            // 如果 ret 不等于 0，说明文件不存在
            if (ret != 0)
            {
                // 记录一条信息级别的日志，表示文件不存在
                LOG(INFO, "FILE IS NOT EXISTS!\n");
                return false;
            }
            return true;
        } // 判断文件是否存在

        // 获取文件大小的函数
        size_t Size()
        {
            // 先调用 Exists 函数判断文件是否存在，如果不存在则返回 0
            if (this->Exists() == false)
            {
                return 0;
            }
            // 定义一个 stat 结构体变量，用于存储文件的状态信息
            struct stat st;
            // 使用 stat 函数获取文件的状态信息，存储到 st 结构体中
            int ret = stat(_name.c_str(), &st);
            // 如果 ret 不等于 0，说明获取文件状态信息失败
            if (ret != 0)
            {
                // 记录一条错误级别的日志，表示获取文件状态信息失败
                LOG(ERROR, "GET FILE STAT FAILED!\n");
                return 0;
            }
            // 返回文件的大小
            return st.st_size;
        } // 获取文件大小

        // 读取文件数据到 body 字符串中的函数
        bool GetContent(std::string *body)
        {
            // 定义一个文件输入流对象
            std::ifstream ifs;
            // 以二进制模式打开文件
            ifs.open(_name, std::ios::binary);
            // 如果文件打开失败
            if (ifs.is_open() == false)
            {
                // 记录一条错误级别的日志，表示打开文件失败
                LOG(ERROR, "OPEN FILE FAILED!\n");
                return false;
            }
            // 获取文件的大小
            size_t flen = this->Size();
            // 调整 body 字符串的大小为文件的大小
            body->resize(flen);
            // 从文件中读取数据到 body 字符串中
            ifs.read(&(*body)[0], flen);
            // 如果读取操作失败
            if (ifs.good() == false)
            {
                // 记录一条错误级别的日志，表示读取文件内容失败
                LOG(ERROR, "READ FILE CONTENT FAILED!\n");
                // 关闭文件
                ifs.close();
                return false;
            }
            // 关闭文件
            ifs.close();
            return true;
        } // 读取文件数据到 body 中

        // 向文件写入数据的函数
        bool SetContent(const std::string &body)
        {
            // 定义一个文件输出流对象
            std::ofstream ofs;
            // 以二进制模式打开文件
            ofs.open(_name, std::ios::binary);
            // 如果文件打开失败
            if (ofs.is_open() == false)
            {
                // 记录一条错误级别的日志，表示打开文件失败
                LOG(ERROR, "OPEN FILE FAILED!\n");
                return false;
            }
            // 将 body 字符串的内容写入文件
            ofs.write(body.c_str(), body.size());
            // 如果写入操作失败
            if (ofs.good() == false)
            {
                // 记录一条错误级别的日志，表示写入文件内容失败
                LOG(ERROR, "WRITE FILE  CONTENT FAILED!\n");
                // 关闭文件
                ofs.close();
                return false;
            }
            // 关闭文件
            ofs.close();
            return true;
        } // 向文件写入数据

        // 针对目录时创建目录的函数
        bool CreateDirectory()
        {
            // 先判断目录是否已经存在，如果存在则直接返回 true
            if (this->Exists())
            {
                return true;
            }
            // 使用 mkdir 函数创建目录，权限设置为 0777
            mkdir(_name.c_str(), 0777);
            return true;
        } // 针对目录时创建目录
    };

    // 定义一个 JSON 数据处理工具类 JsonUtil
    class JsonUtil
    {
    public:
        // 将 Json::Value 对象序列化为字符串的静态函数
        static bool Serialize(const Json::Value &value, std::string *body)
        {
            // 创建一个 Json::StreamWriterBuilder 对象
            Json::StreamWriterBuilder swb;
            // 使用 swb 创建一个 Json::StreamWriter 智能指针
            std::unique_ptr<Json::StreamWriter> sw(swb.newStreamWriter());

            // 定义一个字符串流对象
            std::stringstream ss;
            // 将 Json::Value 对象写入字符串流中
            int ret = sw->write(value, &ss);
            // 如果写入操作失败
            if (ret != 0)
            {
                // 记录一条错误级别的日志，表示序列化失败
                LOG(ERROR, "SERIALIZE FAILED!\n");
                return false;
            }
            // 将字符串流中的内容赋值给 body 字符串
            *body = ss.str();
            return true;
        }

        // 将字符串反序列化为 Json::Value 对象的静态函数
        static bool UnSerialize(const std::string &body, Json::Value *value)
        {
            // 创建一个 Json::CharReaderBuilder 对象
            Json::CharReaderBuilder crb;
            // 使用 crb 创建一个 Json::CharReader 智能指针
            std::unique_ptr<Json::CharReader> cr(crb.newCharReader());

            // 定义一个字符串用于存储错误信息
            std::string err;
            // 将字符串解析为 Json::Value 对象
            bool ret = cr->parse(body.c_str(), body.c_str() + body.size(), value, &err);
            // 如果解析操作失败
            if (ret == false)
            {
                // 记录一条错误级别的日志，表示反序列化失败
                LOG(ERROR, "UNSERIALIZE FAILED!\n");
                return false;
            }
            return true;
        }
    };
}

// 结束防止头文件重复包含的预处理指令
#endif