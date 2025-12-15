#include "config/AppConfig.h"

#include "config/ConfigManager.h"
AppConfig AppConfig::loadFromFile(const std::string &path) {
    // 中文注释：委托 ConfigManager，保证递归加载逻辑集中维护
    ConfigManager mgr(path);
    return mgr.load();
}

void AppConfig::saveToFile(const std::string &path) const {
    ConfigManager mgr(path);
    mgr.save(*this);
}
