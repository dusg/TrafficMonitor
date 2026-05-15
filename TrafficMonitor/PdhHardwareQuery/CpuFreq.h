#pragma once
#include <Pdh.h>
#include <PdhMsg.h>
#include "PdhQuery.h"

class CPdhCpuFreq : public CPdhQuery
{
public:
    CPdhCpuFreq();

    // 调用此函数获取CPU频率。
    bool GetCpuFreq(float& freq);
};
