#include <iostream>
#include <intrin.h>

int main() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    int nIds = cpuInfo[0];
    
    bool sse42 = false, avx = false, avx2 = false;
    bool avx512f = false, avx512bw = false, avx512vl = false, avx512vbmi = false;
    
    if (nIds >= 1) {
        __cpuid(cpuInfo, 1);
        sse42 = (cpuInfo[2] & (1 << 20)) != 0;
        avx   = (cpuInfo[2] & (1 << 28)) != 0;
    }
    if (nIds >= 7) {
        __cpuidex(cpuInfo, 7, 0);
        avx2       = (cpuInfo[1] & (1 << 5)) != 0;
        avx512f    = (cpuInfo[1] & (1 << 16)) != 0;
        avx512bw   = (cpuInfo[1] & (1 << 30)) != 0;
        avx512vl   = (cpuInfo[1] & (1 << 31)) != 0;
        avx512vbmi = (cpuInfo[2] & (1 << 1)) != 0;
    }
    
    std::cout << "SSE4.2:      " << (sse42 ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    std::cout << "AVX:         " << (avx ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    std::cout << "AVX2:        " << (avx2 ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    std::cout << "AVX-512F:    " << (avx512f ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    std::cout << "AVX-512BW:   " << (avx512bw ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    std::cout << "AVX-512VL:   " << (avx512vl ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    std::cout << "AVX-512VBMI: " << (avx512vbmi ? "SUPPORTED" : "UNAVAILABLE") << "\n";
    return 0;
}
