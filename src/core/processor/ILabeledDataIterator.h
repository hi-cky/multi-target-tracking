#pragma once

#include "structure/LabeledData.h"

class ILabeledDataIterator {
public:
    virtual ~ILabeledDataIterator() = default;
    virtual bool hasNext() const = 0;
    virtual bool next(LabeledFrame &outFrame) = 0;
    virtual const cv::Mat &lastFrame() const = 0;
};
