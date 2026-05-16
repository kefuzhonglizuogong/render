#pragma once

#include "core/vec3.h"
#include "material/bsdf_sample.h"


/*这个结构表示路径上的一个顶点
position         当前交点位置
normal           法线
wo               出射方向，也就是看向上一跳
wi               下一跳采样方向
throughput       当前路径权重 beta
bsdfValue        当前 BRDF / BSDF 值
bsdfPdf          BSDF 采样 pdf
lightPdf         同方向 light pdf
cosTheta         cos 项
depth            当前 bounce
eventType        Diffuse / Glossy / Delta...
isDelta          是否 delta 事件
*/
struct PathVertex {
    Point3 position;

    Vec3 geometricNormal;
    Vec3 shadingNormal;

    // 从当前点指向上一跳 / 相机方向
    Vec3 wo;

    // 当前点采样得到的下一跳方向
    Vec3 wi;

    Color throughput;

    Color bsdfValue;

    double bsdfPdf = 0.0;
    double lightPdf = 0.0;

    double cosTheta = 0.0;

    int depth = 0;

    BSDFSampleType eventType = BSDFSampleType::None;
    bool isDelta = false;
    bool valid = false;
};