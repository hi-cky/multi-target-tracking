#include "Feature.h"

#include <cmath>
#include <numeric>
#include <stdexcept>

Feature::Feature() = default;

Feature::Feature(std::vector<float> values) : data_(std::move(values)) {}

size_t Feature::size() const { return data_.size(); }

float Feature::l2norm() const {
    float sum = std::inner_product(data_.begin(), data_.end(), data_.begin(), 0.0f);
    return std::sqrt(sum);
}

Feature Feature::normalized() const {
    float n = l2norm();
    if (n < 1e-12f) {
        throw std::runtime_error("零向量无法归一化");
    }
    std::vector<float> out(data_);
    for (auto &v : out) v /= n;
    return Feature(std::move(out));
}

float Feature::dot(const Feature &other) const {
    ensure_same_dim(other);
    float s = 0.0f;
    for (size_t i = 0; i < data_.size(); ++i) s += data_[i] * other.data_[i];
    return s;
}

float Feature::cosine_similarity(const Feature &other) const {
    float denom = l2norm() * other.l2norm();
    if (denom < 1e-12f) {
        throw std::runtime_error("余弦相似度计算时范数为零");
    }
    return dot(other) / denom;
}

Feature Feature::operator+(const Feature &other) const {
    ensure_same_dim(other);
    std::vector<float> out(data_.size());
    for (size_t i = 0; i < data_.size(); ++i) out[i] = data_[i] + other.data_[i];
    return Feature(std::move(out));
}

Feature Feature::operator*(float scalar) const {
    std::vector<float> out(data_.size());
    for (size_t i = 0; i < data_.size(); ++i) out[i] = data_[i] * scalar;
    return Feature(std::move(out));
}

Feature operator*(float scalar, const Feature &feat) { return feat * scalar; }

const std::vector<float> &Feature::values() const { return data_; }

void Feature::ensure_same_dim(const Feature &other) const {
    if (data_.size() != other.data_.size()) {
        throw std::runtime_error("特征维度不一致");
    }
}
