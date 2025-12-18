#include "YoloDetector.h"
#include "IDetector.h"
#include "../OrtEnvSingleton.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <numeric>
#include <stdexcept>

// --------------------------
//       构造函数
// --------------------------
YoloDetector::YoloDetector(const DetectorConfig &config)
    : config_(config),
      focus_class_id_set_(),
      session_(nullptr) {

    // 1. 检查模型路径是否合法
    std::string model_path = config_.ort_env_config.model_path;
    if (model_path.empty()) {
        throw std::invalid_argument("YoloDetector: 模型路径为空");
    }

    // 预处理关注类别列表（空列表表示不过滤）
    // 这样在推理循环里就能用 O(1) 的 unordered_set 查找，避免每次遍历 vector。
    if (!config_.focus_class_ids.empty()) {
        focus_class_id_set_.reserve(config_.focus_class_ids.size());
        for (int id : config_.focus_class_ids) {
            focus_class_id_set_.insert(id);
        }
    }

    if (!std::filesystem::exists(model_path)) {
        throw std::runtime_error("YoloDetector: 模型文件不存在 -> " + model_path);
    }

    // 2. 创建 ONNX 运行时环境
    session_ = CreateSession(config.ort_env_config);

    // 3. 获取输入形状、输入名、输出名
    Ort::AllocatorWithDefaultOptions allocator;

    // 获取输入 shape（模型要求的输入维度）
    input_shape_ = session_->GetInputTypeInfo(0)
                       .GetTensorTypeAndShapeInfo()
                       .GetShape();

    // 输入名
    auto input_name_alloc = session_->GetInputNameAllocated(0, allocator);
    input_name_ = input_name_alloc.get();

    // 输出名（可能有多个输出）
    const size_t output_count = session_->GetOutputCount();
    output_names_.reserve(output_count);
    for (size_t i = 0; i < output_count; ++i) {
        auto name_alloc = session_->GetOutputNameAllocated(i, allocator);
        output_names_.emplace_back(name_alloc.get());
    }
}

// --------------------------
//        预处理图像
// --------------------------
YoloDetector::PreprocessResult YoloDetector::preprocess(const cv::Mat &frame) const {
    if (frame.empty()) {
        throw std::invalid_argument("YoloDetector: 输入图像为空");
    }

    // 原图尺寸
    const int src_w = frame.cols;
    const int src_h = frame.rows;

    // YOLO 输入通常需要 Letterbox：等比例缩放 + 填充灰色
    const float scale = std::min(
        static_cast<float>(config_.input_width) / static_cast<float>(src_w),
        static_cast<float>(config_.input_height) / static_cast<float>(src_h)
    );

    const int resize_w = static_cast<int>(std::round(src_w * scale));
    const int resize_h = static_cast<int>(std::round(src_h * scale));

    // 剩余部分需要 padding（左右和上下）
    const int pad_x = (config_.input_width - resize_w) / 2;
    const int pad_y = (config_.input_height - resize_h) / 2;

    // 调整大小
    cv::Mat resized;
    cv::resize(frame, resized, cv::Size(resize_w, resize_h));

    // Letterbox：创建 114 灰色背景
    cv::Mat canvas(config_.input_height, config_.input_width, CV_8UC3, cv::Scalar(114, 114, 114));

    // 把 resize 后的图写入正确的位置
    resized.copyTo(canvas(cv::Rect(pad_x, pad_y, resize_w, resize_h)));

    // OpenCV 是 BGR，而 ONNX 里的模型通常是 RGB
    cv::cvtColor(canvas, canvas, cv::COLOR_BGR2RGB);

    // 归一化：uint8 → float32 (0~1)
    canvas.convertTo(canvas, CV_32F, 1.0 / 255.0);

    // 将 HWC → CHW
    std::vector<cv::Mat> chw(3);
    cv::split(canvas, chw);

    // 塞进连续内存 (float 数组)
    PreprocessResult result;
    result.tensor.resize(static_cast<size_t>(3 * config_.input_width * config_.input_height));
    result.scale = scale;
    result.pad_x = static_cast<float>(pad_x);
    result.pad_y = static_cast<float>(pad_y);

    const size_t channel_size = static_cast<size_t>(config_.input_width * config_.input_height);
    for (int c = 0; c < 3; ++c) {
        std::memcpy(
            result.tensor.data() + c * channel_size,
            chw[c].ptr<float>(),
            channel_size * sizeof(float)
        );
    }

    return result;
}

// --------------------------
//         推理（前向）
// --------------------------
std::vector<BBox>
YoloDetector::runInference(const PreprocessResult &prep,
                           const cv::Size &original_size) const {
    if (!session_) {
        throw std::runtime_error("YoloDetector: 推理会话尚未初始化");
    }

    // ONNX 输入 shape：一般是 [1,3,H,W]
    std::vector<int64_t> actual_shape = input_shape_;
    if (actual_shape.empty()) {
        actual_shape = {1, 3, config_.input_height, config_.input_width};
    }
    if (actual_shape.size() == 4) {
        actual_shape[0] = 1;
        actual_shape[1] = 3;
        actual_shape[2] = config_.input_height;
        actual_shape[3] = config_.input_width;
    }

    // 创建 ONNX Runtime 输入张量
    Ort::MemoryInfo memory_info =
        Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    auto input_tensor = Ort::Value::CreateTensor<float>(
        memory_info,
        const_cast<float *>(prep.tensor.data()),
        prep.tensor.size(),
        actual_shape.data(),
        actual_shape.size()
    );

    // 准备输入/输出名字
    const char *input_names[] = {input_name_.c_str()};
    std::vector<const char *> output_name_ptrs;
    output_name_ptrs.reserve(output_names_.size());
    for (const auto &name : output_names_) {
        output_name_ptrs.push_back(name.c_str());
    }

    // 执行前向推理
    auto output_tensors = session_->Run(
        Ort::RunOptions{nullptr},
        input_names, &input_tensor, 1,
        output_name_ptrs.data(), output_name_ptrs.size()
    );

    if (output_tensors.empty()) {
        throw std::runtime_error("YoloDetector: 推理输出为空");
    }

    // YOLO 常用输出形状：
    // [1, N, 85] 或 [1, 85, N] 或直接 [N,85]
    const auto &output_tensor = output_tensors.front();
    const auto type_info = output_tensor.GetTensorTypeAndShapeInfo();
    std::vector<int64_t> shape = type_info.GetShape();
    const float *data = output_tensor.GetTensorData<float>();

    if (shape.size() < 2) {
        throw std::runtime_error("YoloDetector: 不支持的输出维度");
    }

    size_t num_boxes = 0;
    size_t attr_count = 0;
    bool channels_first = false;

    // YOLO 输出有可能是 [1, 85, 25200]（channels first）
    // 或 [1, 25200, 85]（channels last）
    if (shape.size() == 3) {
        const int64_t dim1 = shape[1];
        const int64_t dim2 = shape[2];

        if (dim1 <= dim2) {
            channels_first = true;
            attr_count = static_cast<size_t>(dim1);
            num_boxes = static_cast<size_t>(dim2);
        } else {
            channels_first = false;
            attr_count = static_cast<size_t>(dim2);
            num_boxes = static_cast<size_t>(dim1);
        }
    } else {
        // 若是 [N,85]
        channels_first = false;
        num_boxes = static_cast<size_t>(shape[0]);
        attr_count = static_cast<size_t>(shape[1]);
    }

    if (attr_count < 6) {
        throw std::runtime_error("YoloDetector: 输出维度不足以解析检测框");
    }

    // 根据布局读取数据
    auto value_at = [&](size_t box_idx, size_t attr_idx) -> float {
        if (channels_first) {
            return data[attr_idx * num_boxes + box_idx];
        }
        return data[box_idx * attr_count + attr_idx];
    };

    // ------------------------
    //   解码 YOLO 输出
    // ------------------------
    std::vector<BBox> candidates;
    candidates.reserve(num_boxes);

    for (size_t i = 0; i < num_boxes; ++i) {
        // 中文注释：部分导出的 YOLO（如 yolo11/12）输出形状为 [batch, 84, 8400]，仅包含 4+80 项（无 obj）。
        // 如果 attr_count==4+num_classes，则认为没有 objectness，直接将其视为 1；否则按常规位置读取。
        const bool no_objectness = (attr_count == 84);  // 典型 coco 80 类：4+80
        const size_t class_start = no_objectness ? 4 : 5;
        const float objectness = no_objectness ? 1.0F : value_at(i, 4);

        // 找最高类别分数
        float best_class_score = 0.0F;
        int best_class = -1;
        for (size_t c = class_start; c < attr_count; ++c) {
            const float cls_score = value_at(i, c);
            if (cls_score > best_class_score) {
                best_class_score = cls_score;
                best_class = static_cast<int>(c - class_start);
            }
        }

        // 最终置信度 = objectness * class_score
        const float final_score = objectness * best_class_score;

        if (final_score < config_.score_threshold) {
            continue;  // 太小的框不要
        }

        // 中文注释：按关注类别过滤（若 focus_class_id_set_ 为空，则表示不过滤）
        if (!focus_class_id_set_.empty() &&
            focus_class_id_set_.find(best_class) == focus_class_id_set_.end()) {
            continue;
        }

        // cx, cy, w, h 是 YOLO 格式
        const float cx = value_at(i, 0);
        const float cy = value_at(i, 1);
        const float w = value_at(i, 2);
        const float h = value_at(i, 3);

        // 反 letterbox 映射到原图
        float x0 = (cx - w / 2.0F - prep.pad_x) / prep.scale;
        float y0 = (cy - h / 2.0F - prep.pad_y) / prep.scale;
        float x1 = (cx + w / 2.0F - prep.pad_x) / prep.scale;
        float y1 = (cy + h / 2.0F - prep.pad_y) / prep.scale;

        // 超出边界需要裁剪
        x0 = std::clamp(x0, 0.0F, static_cast<float>(original_size.width));
        y0 = std::clamp(y0, 0.0F, static_cast<float>(original_size.height));
        x1 = std::clamp(x1, 0.0F, static_cast<float>(original_size.width));
        y1 = std::clamp(y1, 0.0F, static_cast<float>(original_size.height));

        if (x1 <= x0 || y1 <= y0) {
            continue;  // 非法框
        }
        // emplace_back: 将新元素直接构造到容器的末尾，避免拷贝或移动操作
        // 这里是将一个 BBox 对象直接构造到 candidates 容器末尾
        candidates.emplace_back(
            cv::Rect2f(cv::Point2f(x0, y0), cv::Point2f(x1, y1)),
            best_class,
            final_score
        );
    }

    // NMS 非极大值抑制
    return applyNms(candidates, config_.nms_threshold);
}

// --------------------------
//           NMS
// --------------------------
std::vector<BBox>
YoloDetector::applyNms(const std::vector<BBox> &boxes,
                       float iou_threshold) {

    // 排序：按 score 从大到小
    std::vector<int> indices(boxes.size());
    // 初始化索引数组为 [0, 1, 2, ..., n-1]
    // 用于后续根据置信度对检测框进行排序时，保持原始索引关系
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&](int lhs, int rhs) {
        return boxes[lhs].score > boxes[rhs].score;
    });

    std::vector<BBox> picked;
    std::vector<bool> suppressed(boxes.size(), false);

    for (size_t i = 0; i < indices.size(); ++i) {
        const int idx = indices[i];
        if (suppressed[idx]) continue; // 已被抑制则跳过

        picked.emplace_back(boxes[idx]);

        // 对剩余框进行 IoU 判断
        for (size_t j = i + 1; j < indices.size(); ++j) {
            const int next_idx = indices[j];
            if (suppressed[next_idx]) continue;

            // NMS 只抑制同类别框
            if (boxes[idx].class_id != boxes[next_idx].class_id) continue;

            if ((boxes[idx] & boxes[next_idx]) > iou_threshold) {
                suppressed[next_idx] = true;
            }
        }
    }

    return picked;
}

// --------------------------
//      对外 detect 接口
// --------------------------
std::vector<BBox> YoloDetector::detect(const cv::Mat &frame, int frame_index) {
    auto prep = preprocess(frame);
    return runInference(prep, frame.size());
}
