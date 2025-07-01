#include <httplib.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include "FileSync.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

static std::string srcDir = "/";
static std::string dstDir = "/";
static std::mutex sync_mutex;
static std::string sync_status = "idle";
static std::vector<json> conflict_list;

void run_sync() {
    std::lock_guard<std::mutex> lock(sync_mutex);
    sync_status = "running";
    try {
        FileSync sync(srcDir, dstDir);
        sync.setConflictCallback([](const std::string& path){
            conflict_list.push_back({{"path", path}});
        });
        sync.syncBidirectional();
        sync_status = "done";
    } catch (const std::exception& e) {
        sync_status = std::string("error: ") + e.what();
    }
}

int main() {
    httplib::Server svr;
    // 目录浏览
    svr.Get("/list_dir", [](const httplib::Request& req, httplib::Response& res) {
        std::string path = req.get_param_value("path");
        json j = json::array();
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                j.push_back({
                    {"name", entry.path().filename().string()},
                    {"is_dir", entry.is_directory()}
                });
            }
        } catch (...) {}
        res.set_content(j.dump(), "application/json");
    });
    // 设置源目录
    svr.Post("/set_src_dir", [](const httplib::Request& req, httplib::Response& res) {
        auto j = json::parse(req.body);
        srcDir = j["path"].get<std::string>();
        res.set_content("{}", "application/json");
    });
    // 设置目标目录
    svr.Post("/set_dst_dir", [](const httplib::Request& req, httplib::Response& res) {
        auto j = json::parse(req.body);
        dstDir = j["path"].get<std::string>();
        res.set_content("{}", "application/json");
    });
    // 启动同步
    svr.Post("/sync", [](const httplib::Request&, httplib::Response& res) {
        if (sync_status == "running") {
            res.set_content("{\"status\":\"already running\"}", "application/json");
            return;
        }
        conflict_list.clear();
        std::thread(run_sync).detach();
        res.set_content("{\"status\":\"started\"}", "application/json");
    });
    // 日志
    svr.Get("/logs", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream fin("sync.log");
        std::string log((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
        res.set_content(log, "text/plain");
    });
    // 冲突
    svr.Get("/conflicts", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(json(conflict_list).dump(), "application/json");
    });
    // 状态
    svr.Get("/status", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(json({{"status", sync_status}}).dump(), "application/json");
    });
    svr.set_mount_point("/", "../web"); // 静态文件服务
    svr.listen("0.0.0.0", 8080);
    return 0;
} 