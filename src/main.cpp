#include <QApplication>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <ctime>

using namespace std;
using namespace spdlog;
#include "mainwindow.h"

void setup_logger() {
    // 获取当前时间
    auto now = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(now);
    tm tm{};

    localtime_r(&tt, &tm);

    // 使用 format 构造文件名（仅用时分秒）
    string filename = format("log_{:02}_{:02}_{:02}.txt", tm.tm_hour,
                             tm.tm_min, tm.tm_sec);

    // 创建 log 目录
    filesystem::path log_dir =
        filesystem::path(CMAKE_CURRENT_DIR) / "log";
    filesystem::create_directories(log_dir);

    // 日志文件路径
    filesystem::path filepath = log_dir / filename;

    // 创建 sink
    auto console_sink = make_shared<sinks::stdout_color_sink_mt>();
    auto file_sink = make_shared<sinks::basic_file_sink_mt>(
        filepath.string(), true);

    // 设置统一格式（仅时间，无日期）
    const string pattern = "[%H:%M:%S.%e] [%^%l%$] %v";
    console_sink->set_pattern(pattern);
    file_sink->set_pattern(pattern);

    // 创建 logger（组合 sink），并设置为默认
    auto logger = make_shared<spdlog::logger>(
        "default", sinks_init_list{console_sink, file_sink});
    set_default_logger(logger);
    set_level(level::trace);
}

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
}
