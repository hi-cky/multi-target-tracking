#include "config/ConfigManager.h"

#include <array>
#include <filesystem>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>

#include <opencv2/core.hpp>

#include "config/AppConfig.h"

// 轻量级“反射”与读写工具
namespace {

// ---------- 基础读写 ----------
inline void writeScalar(cv::FileStorage &fs, const char *key, bool v) { fs << key << static_cast<int>(v ? 1 : 0); }
inline void writeScalar(cv::FileStorage &fs, const char *key, int v) { fs << key << v; }
inline void writeScalar(cv::FileStorage &fs, const char *key, float v) { fs << key << v; }
inline void writeScalar(cv::FileStorage &fs, const char *key, double v) { fs << key << v; }
inline void writeScalar(cv::FileStorage &fs, const char *key, const std::string &v) { fs << key << v; }
inline void writeScalar(cv::FileStorage &fs, const char *key, size_t v) { fs << key << static_cast<double>(v); }

template <typename T>
void writeValue(cv::FileStorage &fs, const char *key, const T &v);

template <>
void writeValue<std::vector<int>>(cv::FileStorage &fs, const char *key, const std::vector<int> &vec) {
    fs << key << "[";
    for (int x : vec) fs << x;
    fs << "]";
}

template <>
void writeValue<cv::Scalar>(cv::FileStorage &fs, const char *key, const cv::Scalar &value) {
    fs << key << "[";
    fs << value[0] << value[1] << value[2];
    fs << "]";
}

template <typename T>
inline void writeValue(cv::FileStorage &fs, const char *key, const T &v) {
    writeScalar(fs, key, v);
}

// ---------- 读取 ----------
template <typename T>
bool readValue(const cv::FileNode &node, const char *key, T &out) {
    const cv::FileNode v = node[key];
    if (v.empty()) return false;
    v >> out;
    return true;
}

template <>
bool readValue<bool>(const cv::FileNode &node, const char *key, bool &out) {
    const cv::FileNode v = node[key];
    if (v.empty()) return false;
    int tmp = out ? 1 : 0;
    v >> tmp;
    out = (tmp != 0);
    return true;
}

template <>
bool readValue<size_t>(const cv::FileNode &node, const char *key, size_t &out) {
    const cv::FileNode v = node[key];
    if (v.empty()) return false;
    double tmp = static_cast<double>(out);
    v >> tmp;
    if (tmp > 0) out = static_cast<size_t>(tmp);
    return true;
}

template <>
bool readValue<std::vector<int>>(const cv::FileNode &node, const char *key, std::vector<int> &out) {
    const cv::FileNode v = node[key];
    if (v.empty() || !v.isSeq()) return false;
    out.clear();
    for (auto it = v.begin(); it != v.end(); ++it) {
        int val = 0;
        *it >> val;
        out.push_back(val);
    }
    return true;
}

template <>
bool readValue<cv::Scalar>(const cv::FileNode &node, const char *key, cv::Scalar &out) {
    const cv::FileNode v = node[key];
    if (v.empty() || !v.isSeq()) return false;
    std::vector<double> vals;
    for (auto it = v.begin(); it != v.end(); ++it) {
        double val = 0.0;
        *it >> val;
        vals.push_back(val);
    }
    if (vals.size() >= 3) {
        out = cv::Scalar(vals[0], vals[1], vals[2]);
        return true;
    }
    return false;
}

// ---------- 反射描述 ----------
template <typename T, typename MemberT>
struct Field {
    const char *name;
    MemberT T::*ptr;
};

// 预声明 reflect 元函数
template <typename T>
struct Reflect;

// ---------- 递归序列化/反序列化 ----------
template <typename T>
std::enable_if_t<!std::is_class_v<T> || std::is_same_v<T, std::string>, void>
serialize(cv::FileStorage &fs, const char *key, const T &value) {
    writeValue(fs, key, value);
}

template <typename T>
std::enable_if_t<!std::is_class_v<T> || std::is_same_v<T, std::string>, void>
deserialize(const cv::FileNode &node, const char *key, T &value) {
    readValue(node, key, value);
}

// 对向量<int> 的偏特化在 read/writeValue 中已实现

template <typename T>
std::enable_if_t<std::is_class_v<T> && !std::is_same_v<T, std::string>, void>
serialize(cv::FileStorage &fs, const char *key, const T &obj);

template <typename T>
std::enable_if_t<std::is_class_v<T> && !std::is_same_v<T, std::string>, void>
deserialize(const cv::FileNode &node, const char *key, T &obj);

// 针对 std::vector<int> 的显式序列化/反序列化，避免走类反射路径
template <>
inline void serialize<std::vector<int>>(cv::FileStorage &fs, const char *key, const std::vector<int> &v) {
    writeValue(fs, key, v);
}

template <>
inline void deserialize<std::vector<int>>(const cv::FileNode &node, const char *key, std::vector<int> &v) {
    readValue(node, key, v);
}

// 针对 cv::Scalar 的显式序列化/反序列化，避免走类反射路径
template <>
inline void serialize<cv::Scalar>(cv::FileStorage &fs, const char *key, const cv::Scalar &v) {
    writeValue(fs, key, v);
}

template <>
inline void deserialize<cv::Scalar>(const cv::FileNode &node, const char *key, cv::Scalar &v) {
    readValue(node, key, v);
}

// 序列化类：写入一个 map 节点
template <typename Obj, typename Tuple, typename Fn>
void for_each_field(Obj &obj, Tuple &&t, Fn &&fn) {
    std::apply([&](auto &&...field) {
        (fn(field.name, obj.*(field.ptr)), ...);
    }, std::forward<Tuple>(t));
}

template <typename T>
std::enable_if_t<std::is_class_v<T> && !std::is_same_v<T, std::string>, void>
serialize(cv::FileStorage &fs, const char *key, const T &obj) {
    fs << key << "{";
    for_each_field(const_cast<T &>(obj), Reflect<T>::fields(), [&](const char *name, const auto &value) {
        serialize(fs, name, value);
    });
    fs << "}";
}

// 反序列化类：按字段名读取（缺失则保持默认值）
template <typename T>
std::enable_if_t<std::is_class_v<T> && !std::is_same_v<T, std::string>, void>
deserialize(const cv::FileNode &node, const char *key, T &obj) {
    const cv::FileNode child = node[key];
    if (child.empty()) return;
    for_each_field(obj, Reflect<T>::fields(), [&](const char *name, auto &value) {
        deserialize(child, name, value);
    });
}

// ------------- 各配置结构的反射表 -------------

template <>
struct Reflect<OrtEnvConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<OrtEnvConfig, std::string>{"model_path", &OrtEnvConfig::model_path},
            Field<OrtEnvConfig, bool>{"using_gpu", &OrtEnvConfig::using_gpu},
            Field<OrtEnvConfig, int>{"device_id", &OrtEnvConfig::device_id},
            Field<OrtEnvConfig, size_t>{"gpu_mem_limit", &OrtEnvConfig::gpu_mem_limit}
        );
    }
};

template <>
struct Reflect<DetectorConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<DetectorConfig, int>{"input_width", &DetectorConfig::input_width},
            Field<DetectorConfig, int>{"input_height", &DetectorConfig::input_height},
            Field<DetectorConfig, float>{"score_threshold", &DetectorConfig::score_threshold},
            Field<DetectorConfig, float>{"nms_threshold", &DetectorConfig::nms_threshold},
            Field<DetectorConfig, bool>{"filter_edge_boxes", &DetectorConfig::filter_edge_boxes},
            Field<DetectorConfig, std::vector<int>>{"focus_class_ids", &DetectorConfig::focus_class_ids},
            Field<DetectorConfig, OrtEnvConfig>{"ort_env", &DetectorConfig::ort_env_config}
        );
    }
};

template <>
struct Reflect<FeatureExtractorConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<FeatureExtractorConfig, int>{"input_height", &FeatureExtractorConfig::input_height},
            Field<FeatureExtractorConfig, int>{"input_width", &FeatureExtractorConfig::input_width},
            Field<FeatureExtractorConfig, OrtEnvConfig>{"ort_env", &FeatureExtractorConfig::ort_env_config}
        );
    }
};

template <>
struct Reflect<MatcherConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<MatcherConfig, float>{"iou_weight", &MatcherConfig::iou_weight},
            Field<MatcherConfig, float>{"feature_weight", &MatcherConfig::feature_weight},
            Field<MatcherConfig, float>{"threshold", &MatcherConfig::threshold}
        );
    }
};

template <>
struct Reflect<TrackerConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<TrackerConfig, int>{"max_life", &TrackerConfig::max_life},
            Field<TrackerConfig, float>{"feature_momentum", &TrackerConfig::feature_momentum},
            Field<TrackerConfig, float>{"healthy_percent", &TrackerConfig::healthy_percent},
            Field<TrackerConfig, float>{"kf_pos_noise", &TrackerConfig::kf_pos_noise},
            Field<TrackerConfig, float>{"kf_size_noise", &TrackerConfig::kf_size_noise}
        );
    }
};

template <>
struct Reflect<TrackerManagerConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<TrackerManagerConfig, MatcherConfig>{"matcher", &TrackerManagerConfig::matcher_cfg},
            Field<TrackerManagerConfig, TrackerConfig>{"tracker", &TrackerManagerConfig::tracker_cfg}
        );
    }
};

template <>
struct Reflect<RoiConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<RoiConfig, bool>{"enabled", &RoiConfig::enabled},
            Field<RoiConfig, float>{"x", &RoiConfig::x},
            Field<RoiConfig, float>{"y", &RoiConfig::y},
            Field<RoiConfig, float>{"w", &RoiConfig::w},
            Field<RoiConfig, float>{"h", &RoiConfig::h}
        );
    }
};

template <>
struct Reflect<TrackingEngineConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<TrackingEngineConfig, DetectorConfig>{"detector", &TrackingEngineConfig::detector},
            Field<TrackingEngineConfig, FeatureExtractorConfig>{"extractor", &TrackingEngineConfig::extractor},
            Field<TrackingEngineConfig, TrackerManagerConfig>{"tracker_mgr", &TrackingEngineConfig::tracker_mgr},
            Field<TrackingEngineConfig, RoiConfig>{"roi", &TrackingEngineConfig::roi}
        );
    }
};

template <>
struct Reflect<RecorderConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<RecorderConfig, std::string>{"stats_csv_path", &RecorderConfig::stats_csv_path},
            Field<RecorderConfig, bool>{"enable_extra_statistics", &RecorderConfig::enable_extra_statistics}
        );
    }
};

template <>
struct Reflect<VisualizerConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<VisualizerConfig, int>{"box_thickness", &VisualizerConfig::box_thickness},
            Field<VisualizerConfig, int>{"text_thickness", &VisualizerConfig::text_thickness},
            Field<VisualizerConfig, double>{"font_scale", &VisualizerConfig::font_scale},
            Field<VisualizerConfig, int>{"font_face", &VisualizerConfig::font_face},
            Field<VisualizerConfig, int>{"text_padding", &VisualizerConfig::text_padding},
            Field<VisualizerConfig, bool>{"show_score", &VisualizerConfig::show_score},
            Field<VisualizerConfig, bool>{"show_class_id", &VisualizerConfig::show_class_id},
            Field<VisualizerConfig, RoiConfig>{"roi", &VisualizerConfig::roi},
            Field<VisualizerConfig, int>{"roi_thickness", &VisualizerConfig::roi_thickness},
            Field<VisualizerConfig, cv::Scalar>{"roi_color", &VisualizerConfig::roi_color}
        );
    }
};

template <>
struct Reflect<AppConfig> {
    static constexpr auto fields() {
        return std::make_tuple(
            Field<AppConfig, TrackingEngineConfig>{"engine", &AppConfig::engine},
            Field<AppConfig, RecorderConfig>{"recorder", &AppConfig::recorder},
            Field<AppConfig, VisualizerConfig>{"visualizer", &AppConfig::visualizer}
        );
    }
};

}  // namespace

// ---------------- ConfigManager 实现 ----------------

ConfigManager::ConfigManager(std::string path)
    : path_(std::move(path)) {}

AppConfig ConfigManager::load() const {
    AppConfig cfg;

    if (!std::filesystem::exists(path_)) {
        // 不存在返回默认配置
        return cfg;
    }

    cv::FileStorage fs(path_, cv::FileStorage::READ);
    if (!fs.isOpened()) {
        throw std::runtime_error("ConfigManager: 无法打开配置文件 -> " + path_);
    }

    const cv::FileNode root = fs.root();
    if (!root.empty()) {
        // 必须存在 engine 节点，否则视为无效配置，返回默认值
        const cv::FileNode eng = root["engine"];
        if (eng.empty()) return cfg;

        deserialize(root, "engine", cfg.engine);
        deserialize(root, "recorder", cfg.recorder);
        deserialize(root, "visualizer", cfg.visualizer);
    }
    return cfg;
}

void ConfigManager::save(const AppConfig &cfg) const {
    const std::filesystem::path p(path_);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    cv::FileStorage fs(path_, cv::FileStorage::WRITE);
    if (!fs.isOpened()) {
        throw std::runtime_error("ConfigManager: 无法写入配置文件 -> " + path_);
    }

    serialize(fs, "engine", cfg.engine);
    serialize(fs, "recorder", cfg.recorder);
    serialize(fs, "visualizer", cfg.visualizer);
}
