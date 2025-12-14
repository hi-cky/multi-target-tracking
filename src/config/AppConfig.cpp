#include "config/AppConfig.h"

#include <filesystem>
#include <stdexcept>

namespace {

// 中文注释：安全读取工具：如果 key 不存在则保持默认值
template <typename T>
void readIfExists(const cv::FileNode &node, const char *key, T &out) {
    const cv::FileNode v = node[key];
    if (v.empty()) return;
    v >> out;
}

// 中文注释：读取 bool（OpenCV 的 YAML 可能用 0/1 或 true/false）
void readBoolIfExists(const cv::FileNode &node, const char *key, bool &out) {
    const cv::FileNode v = node[key];
    if (v.empty()) return;
    int tmp = out ? 1 : 0;
    v >> tmp;
    out = (tmp != 0);
}

// 中文注释：读取 int 列表（YAML 序列）
void readIntVectorIfExists(const cv::FileNode &node, const char *key, std::vector<int> &out) {
    const cv::FileNode v = node[key];
    if (v.empty()) return;
    if (!v.isSeq()) return;
    out.clear();
    for (auto it = v.begin(); it != v.end(); ++it) {
        int id = 0;
        *it >> id;
        out.push_back(id);
    }
}

void loadOrtEnvConfig(const cv::FileNode &node, OrtEnvConfig &out) {
    readIfExists(node, "model_path", out.model_path);
    readBoolIfExists(node, "using_gpu", out.using_gpu);
    readIfExists(node, "device_id", out.device_id);

    // 中文注释：gpu_mem_limit 用 size_t 存储，YAML 里建议写整数（单位字节）
    // OpenCV FileStorage 对 int/float/double 支持更好，这里用 double 读写，避免 long long 二义性
    double mem_limit = static_cast<double>(out.gpu_mem_limit);
    readIfExists(node, "gpu_mem_limit", mem_limit);
    if (mem_limit > 0) out.gpu_mem_limit = static_cast<size_t>(mem_limit);
}

void saveOrtEnvConfig(cv::FileStorage &fs, const OrtEnvConfig &cfg) {
    fs << "model_path" << cfg.model_path;
    fs << "using_gpu" << static_cast<int>(cfg.using_gpu ? 1 : 0);
    fs << "device_id" << cfg.device_id;
    fs << "gpu_mem_limit" << static_cast<double>(cfg.gpu_mem_limit);
}

void loadDetectorConfig(const cv::FileNode &node, DetectorConfig &out) {
    readIfExists(node, "input_width", out.input_width);
    readIfExists(node, "input_height", out.input_height);
    readIfExists(node, "score_threshold", out.score_threshold);
    readIfExists(node, "nms_threshold", out.nms_threshold);
    readIntVectorIfExists(node, "focus_class_ids", out.focus_class_ids);

    const cv::FileNode ort_node = node["ort_env"];
    if (!ort_node.empty()) loadOrtEnvConfig(ort_node, out.ort_env_config);
}

void saveDetectorConfig(cv::FileStorage &fs, const DetectorConfig &cfg) {
    fs << "input_width" << cfg.input_width;
    fs << "input_height" << cfg.input_height;
    fs << "score_threshold" << cfg.score_threshold;
    fs << "nms_threshold" << cfg.nms_threshold;

    fs << "focus_class_ids" << "[";
    for (int id : cfg.focus_class_ids) fs << id;
    fs << "]";

    fs << "ort_env" << "{";
    saveOrtEnvConfig(fs, cfg.ort_env_config);
    fs << "}";
}

void loadFeatureConfig(const cv::FileNode &node, FeatureExtractorConfig &out) {
    readIfExists(node, "input_width", out.input_width);
    readIfExists(node, "input_height", out.input_height);

    const cv::FileNode ort_node = node["ort_env"];
    if (!ort_node.empty()) loadOrtEnvConfig(ort_node, out.ort_env_config);
}

void saveFeatureConfig(cv::FileStorage &fs, const FeatureExtractorConfig &cfg) {
    fs << "input_width" << cfg.input_width;
    fs << "input_height" << cfg.input_height;

    fs << "ort_env" << "{";
    saveOrtEnvConfig(fs, cfg.ort_env_config);
    fs << "}";
}

void loadTrackerMgrConfig(const cv::FileNode &node, TrackerManagerConfig &out) {
    readIfExists(node, "iou_weight", out.iou_weight);
    readIfExists(node, "feature_weight", out.feature_weight);
    readIfExists(node, "match_threshold", out.match_threshold);
    readIfExists(node, "max_life", out.max_life);
    readIfExists(node, "feature_momentum", out.feature_momentum);
}

void saveTrackerMgrConfig(cv::FileStorage &fs, const TrackerManagerConfig &cfg) {
    fs << "iou_weight" << cfg.iou_weight;
    fs << "feature_weight" << cfg.feature_weight;
    fs << "match_threshold" << cfg.match_threshold;
    fs << "max_life" << cfg.max_life;
    fs << "feature_momentum" << cfg.feature_momentum;
}

}  // namespace

AppConfig AppConfig::loadFromFile(const std::string &path) {
    AppConfig cfg;

    if (!std::filesystem::exists(path)) {
        // 中文注释：不存在则返回默认配置（由各 struct 默认值决定）
        return cfg;
    }

    cv::FileStorage fs(path, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        throw std::runtime_error("AppConfig: 无法打开配置文件 -> " + path);
    }

    const cv::FileNode root = fs.root();
    if (root.empty()) return cfg;

    const cv::FileNode det_node = root["detector"];
    if (!det_node.empty()) loadDetectorConfig(det_node, cfg.detector);

    const cv::FileNode feat_node = root["feature"];
    if (!feat_node.empty()) loadFeatureConfig(feat_node, cfg.feature);

    const cv::FileNode tm_node = root["tracker_mgr"];
    if (!tm_node.empty()) loadTrackerMgrConfig(tm_node, cfg.tracker_mgr);

    // 中文注释：额外配置（可选）
    readIfExists(root, "stats_csv_path", cfg.stats_csv_path);

    return cfg;
}

void AppConfig::saveToFile(const std::string &path) const {
    // 中文注释：确保父目录存在
    const std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    cv::FileStorage fs(path, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        throw std::runtime_error("AppConfig: 无法写入配置文件 -> " + path);
    }

    fs << "detector" << "{";
    saveDetectorConfig(fs, detector);
    fs << "}";

    fs << "feature" << "{";
    saveFeatureConfig(fs, feature);
    fs << "}";

    fs << "tracker_mgr" << "{";
    saveTrackerMgrConfig(fs, tracker_mgr);
    fs << "}";

    fs << "stats_csv_path" << stats_csv_path;
}
