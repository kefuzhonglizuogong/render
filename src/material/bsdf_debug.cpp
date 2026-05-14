#include "material/bsdf_debug.h"

#include <iostream>
#include <algorithm>

void debugBSDFSampling(const Material& material,const Vec3& normal,const Vec3& wo,int sampleCount) {
    int validCount = 0; // 有效采样次数
    int invalidCount = 0;// 无效采样（出错的）
    int zeroPdfCount = 0;// PDF=0 的采样（会炸）

    double minPdf = 1e30;// PDF 最大最小值
    double maxPdf = 0.0;

    double minWeight = 1e30;// 路径追踪权重
    double maxWeight = 0.0;
    double avgWeight = 0.0;

    Vec3 n = normal.normalized();
    Vec3 out = wo.normalized();

    //对材质采样 N 次，得到反射方向 wi、PDF、颜色 f
    for (int i = 0; i < sampleCount; ++i) {
        BSDFSample sample = material.sample(out, n);

        if (!sample.valid) {
            ++invalidCount;
            continue;
        }

        ++validCount;

        Vec3 wi = sample.wi.normalized();

        double cosTheta = std::max(0.0, dot(n, wi));

        if (sample.pdf <= 1e-12) {
            ++zeroPdfCount;
            continue;
        }

        Color f = material.eval(out, n, wi);
        double pdf = material.pdfValue(out, n, wi);

        double weight =maxComponent(f) * cosTheta / sample.pdf;

        minPdf = std::min(minPdf, pdf);
        maxPdf = std::max(maxPdf, pdf);

        minWeight = std::min(minWeight, weight);
        maxWeight = std::max(maxWeight, weight);
        avgWeight += weight;
    }

    if (validCount > 0) {
        avgWeight /= static_cast<double>(validCount);
    }

    std::cout << "\n=== BSDF Sampling Debug ===\n";
    std::cout << "Sample count:       " << sampleCount << "\n";
    std::cout << "Valid samples:      " << validCount << "\n";
    std::cout << "Invalid samples:    " << invalidCount << "\n";
    std::cout << "Zero pdf samples:   " << zeroPdfCount << "\n";
    std::cout << "Min pdf:            " << minPdf << "\n";
    std::cout << "Max pdf:            " << maxPdf << "\n";
    std::cout << "Min weight:         " << minWeight << "\n";
    std::cout << "Max weight:         " << maxWeight << "\n";
    std::cout << "Avg weight:         " << avgWeight << "\n";
    std::cout << "===========================\n";
}