#pragma once

#include "core/vec3.h"


/*
wi       ：采样得到的下一跳方向
f        ：BRDF 值
pdf      ：采样这个方向的概率密度
type     ：采样事件类型，Diffuse / Glossy / Delta...
isDelta  ：是否是 delta 事件，例如理想镜面
valid    ：采样是否有效
*/

enum class BSDFSampleType {
    Diffuse,
    Glossy,
    DeltaReflection,
    DeltaTransmission,
    None
};

struct BSDFSample {
    Vec3 wi;
    Color f;
    double pdf = 0.0;

    BSDFSampleType type = BSDFSampleType::None;
    bool isDelta = false;
    bool valid = false;
};