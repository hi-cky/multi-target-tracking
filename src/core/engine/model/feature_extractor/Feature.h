#pragma once

#include <cstddef>
#include <vector>

// 特征向量封装，提供归一化、相似度与基础算子
class Feature {
public:
    Feature();
    explicit Feature(std::vector<float> values);

    size_t size() const;
    float l2norm() const;
    Feature normalized() const;
    float dot(const Feature &other) const;
    float cosine_similarity(const Feature &other) const;

    // 逐元素相加
    Feature operator+(const Feature &other) const;
    // 特征与标量相乘
    Feature operator*(float scalar) const;
    // 允许标量在左侧
    friend Feature operator*(float scalar, const Feature &feat);

    const std::vector<float> &values() const;

private:
    void ensure_same_dim(const Feature &other) const;
    std::vector<float> data_;
};
