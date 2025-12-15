#pragma once

#include <string>

class AppConfig;

// 中文注释：配置管理器，集中处理 YAML 的读写与目录创建。
// 设计：根据 AppConfig 的嵌套结构递归读写，避免在业务层手写键名。
class ConfigManager {
public:
    explicit ConfigManager(std::string path);

    AppConfig load() const;
    void save(const AppConfig &cfg) const;

    const std::string &path() const { return path_; }

private:
    std::string path_;
};
