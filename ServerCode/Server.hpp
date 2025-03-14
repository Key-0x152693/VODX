#include "Data.hpp"
#include "httplib.h"

namespace vod
{
    // 定义静态资源的根目录，服务器将从该目录下查找和提供静态文件
    #define WWWROOT "./www"
    // 定义视频文件存储的相对路径
    #define VIDEO_ROOT "/video/"
    // 定义图片文件存储的相对路径
    #define IMAGE_ROOT "/image/"

    // 声明一个指向 TableVideo 类的指针，用于管理视频表的数据库操作
    TableVideo *tb_video = NULL;

    // 定义 Server 类，用于搭建和运行 HTTP 服务器，处理视频相关的请求
    class Server
    {
    private:
        // 服务器监听的端口号
        int _port;
        // httplib 库的服务器实例，用于处理 HTTP 请求
        httplib::Server _srv;

    private:
        // 处理 POST 请求，用于插入新的视频信息
        static void Insert(const httplib::Request &req, httplib::Response &rsp)
        {
            // 检查请求中是否包含必要的文件信息（视频名称、简介、视频文件、图片文件）
            if (req.has_file("name") == false ||
                req.has_file("info") == false ||
                req.has_file("video") == false ||
                req.has_file("image") == false)
            {
                // 如果缺少必要信息，返回 400 错误响应
                rsp.status = 400;
                rsp.body = R"({"result":false, "reason":"上传的数据信息错误"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 从请求中获取视频名称的文件信息
            httplib::MultipartFormData name = req.get_file_value("name");
            // 从请求中获取视频简介的文件信息
            httplib::MultipartFormData info = req.get_file_value("info");
            // 从请求中获取视频文件的信息
            httplib::MultipartFormData video = req.get_file_value("video");
            // 从请求中获取图片文件的信息
            httplib::MultipartFormData image = req.get_file_value("image");
            // 提取视频名称
            std::string video_name = name.content;
            // 提取视频简介
            std::string video_info = info.content;
            // 构建静态资源根目录
            std::string root = WWWROOT;
            // 构建视频文件的存储路径
            std::string video_path = root + VIDEO_ROOT + video_name + video.filename;
            // 构建图片文件的存储路径
            std::string image_path = root + IMAGE_ROOT + video_name + image.filename;

            // 将视频文件内容写入指定路径，如果写入失败
            if (FileUtil(video_path).SetContent(video.content) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"视频文件存储失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 将图片文件内容写入指定路径，如果写入失败
            if (FileUtil(image_path).SetContent(image.content) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"图片文件存储失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 创建一个 Json::Value 对象，用于存储视频信息
            Json::Value video_json;
            // 设置视频名称
            video_json["name"] = video_name;
            // 设置视频简介
            video_json["info"] = video_info;
            // 设置视频文件的相对路径
            video_json["video"] = VIDEO_ROOT + video_name + video.filename;
            // 设置图片文件的相对路径
            video_json["image"] = IMAGE_ROOT + video_name + image.filename;
            // 将视频信息插入数据库，如果插入失败
            if (tb_video->Insert(video_json) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"数据库新增数据失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 插入成功后，重定向到首页
            rsp.set_redirect("/index.html", 303);
            return;
        }

        // 处理 DELETE 请求，用于删除指定 ID 的视频信息
        static void Delete(const httplib::Request &req, httplib::Response &rsp)
        {
            // 从请求中提取要删除的视频 ID
            int video_id = std::stoi(req.matches[1]);
            // 创建一个 Json::Value 对象，用于存储查询到的视频信息
            Json::Value video;
            // 根据视频 ID 查询视频信息，如果查询失败
            if (tb_video->SelectOne(video_id, &video) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"不存在视频信息"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 构建静态资源根目录
            std::string root = WWWROOT;
            // 构建要删除的视频文件的路径
            std::string video_path = root + video["video"].asString();
            // 构建要删除的图片文件的路径
            std::string image_path = root + video["image"].asString();
            // 删除视频文件
            remove(video_path.c_str());
            // 删除图片文件
            remove(image_path.c_str());
            // 从数据库中删除该视频信息，如果删除失败
            if (tb_video->Delete(video_id) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"删除数据库信息失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            return;
        }

        // 处理 PUT 请求，用于更新指定 ID 的视频信息
        static void Update(const httplib::Request &req, httplib::Response &rsp)
        {
            // 从请求中提取要更新的视频 ID
            int video_id = std::stoi(req.matches[1]);
            // 创建一个 Json::Value 对象，用于存储更新后的视频信息
            Json::Value video;
            // 将请求体中的 JSON 数据解析到 video 对象中，如果解析失败
            if (JsonUtil::UnSerialize(req.body, &video) == false)
            {
                // 返回 400 错误响应
                rsp.status = 400;
                rsp.body = R"({"result":false, "reason":"新的视频信息格式解析失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 更新数据库中该视频的信息，如果更新失败
            if (tb_video->Update(video_id, video) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"修改数据库信息失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            return;
        }

        // 处理 GET 请求，用于查询指定 ID 的视频信息
        static void SelectOne(const httplib::Request &req, httplib::Response &rsp)
        {
            // 从请求中提取要查询的视频 ID
            int video_id = std::stoi(req.matches[1]);
            // 创建一个 Json::Value 对象，用于存储查询到的视频信息
            Json::Value video;
            // 根据视频 ID 查询视频信息，如果查询失败
            if (tb_video->SelectOne(video_id, &video) == false)
            {
                // 返回 500 错误响应
                rsp.status = 500;
                rsp.body = R"({"result":false, "reason":"查询数据库指定视频信息失败"})";
                rsp.set_header("Content-Type", "application/json");
                return;
            }
            // 将查询到的视频信息序列化为 JSON 字符串
            JsonUtil::Serialize(video, &rsp.body);
            // 设置响应的内容类型为 JSON
            rsp.set_header("Content-Type", "application/json");
            return;
        }

        // 处理 GET 请求，用于查询所有视频信息或根据关键字模糊查询视频信息
        static void SelectAll(const httplib::Request &req, httplib::Response &rsp)
        {
            // 默认进行全量查询
            bool select_flag = true;
            // 存储查询关键字
            std::string search_key;
            // 如果请求中包含 search 参数
            if (req.has_param("search") == true)
            {
                // 切换为模糊查询
                select_flag = false;
                // 获取查询关键字
                search_key = req.get_param_value("search");
            }
            // 创建一个 Json::Value 对象，用于存储查询到的视频列表
            Json::Value videos;
            // 如果是全量查询
            if (select_flag == true)
            {
                // 查询所有视频信息，如果查询失败
                if (tb_video->SelectAll(&videos) == false)
                {
                    // 返回 500 错误响应
                    rsp.status = 500;
                    rsp.body = R"({"result":false, "reason":"查询数据库所有视频信息失败"})";
                    rsp.set_header("Content-Type", "application/json");
                    return;
                }
            }
            else
            {
                // 根据关键字进行模糊查询，如果查询失败
                if (tb_video->SelectLike(search_key, &videos) == false)
                {
                    // 返回 500 错误响应
                    rsp.status = 500;
                    rsp.body = R"({"result":false, "reason":"查询数据库匹配视频信息失败"})";
                    rsp.set_header("Content-Type", "application/json");
                    return;
                }
            }
            // 将查询到的视频列表序列化为 JSON 字符串
            JsonUtil::Serialize(videos, &rsp.body);
            // 设置响应的内容类型为 JSON
            rsp.set_header("Content-Type", "application/json");
            return;
        }

    public:
        // 构造函数，初始化服务器监听的端口号
        Server(int port) : _port(port) {}

        // 启动服务器的主要方法
        bool RunModule()
        {
            // 创建 TableVideo 类的实例，用于管理视频表的数据库操作
            tb_video = new TableVideo();
            // 创建静态资源根目录
            FileUtil(WWWROOT).CreateDirectory();
            // 构建视频文件存储的实际路径
            std::string root = WWWROOT;
            std::string video_real_path = root + VIDEO_ROOT;
            // 创建视频文件存储目录
            FileUtil(video_real_path).CreateDirectory();
            // 构建图片文件存储的实际路径
            std::string image_real_path = root + IMAGE_ROOT;
            // 创建图片文件存储目录
            FileUtil(image_real_path).CreateDirectory();
            // 设置静态资源的根目录
            _srv.set_mount_point("/", WWWROOT);
            // 注册 POST 请求处理函数，用于插入新的视频信息
            _srv.Post("/video", Insert);
            // 注册 DELETE 请求处理函数，用于删除指定 ID 的视频信息
            _srv.Delete("/video/(\\d+)", Delete);
            // 注册 PUT 请求处理函数，用于更新指定 ID 的视频信息
            _srv.Put("/video/(\\d+)", Update);
            // 注册 GET 请求处理函数，用于查询指定 ID 的视频信息
            _srv.Get("/video/(\\d+)", SelectOne);
            // 注册 GET 请求处理函数，用于查询所有视频信息或根据关键字模糊查询视频信息
            _srv.Get("/video", SelectAll);
            // 启动服务器，监听指定端口
            _srv.listen("0.0.0.0", _port);
            return true;
        }
    };
}