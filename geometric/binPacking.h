#ifndef BIN_PACKING_H
#define BIN_PACKING_H
#include <vector>
#include <memory>
#include "geometric/aabbox.h"

namespace BinPacking
{
using AABBoxPtr = std::shared_ptr<SAABBox2D>;

std::vector<AABBoxPtr> PackFCNR(std::vector<AABBoxPtr>& bboxes, float binWidth, float binHeight);
}

#endif
