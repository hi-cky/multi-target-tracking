#pragma once

#include "../model/detector/BBox.h"
#include "../model/feature_extractor/Feature.h"

struct TrackerInner {
    BBox box;
    Feature feature;
};
