#pragma once
#include <Pdh.h>
#include <PdhMsg.h>
#include <vector>
#include "PdhQuery.h"

class CPdhCpuFreq : public CPdhQuery
{
public:
    class CCounterQuery : public CPdhQuery
    {
    public:
        explicit CCounterQuery(LPCTSTR _fullCounterPath)
            : CPdhQuery(_fullCounterPath)
        {}

        using CPdhQuery::QueryValue;
        using CPdhQuery::QueryValues;
    };

    CPdhCpuFreq();

    // 调用此函数获取CPU频率。
    bool GetCpuFreq(float& freq);
    static bool CalculateCpuFreq(double processor_performance, double base_freq_mhz, float& freq);
    static bool CalculateCpuFreq(const std::vector<double>& freq_values_mhz, float& freq);

private:
    CCounterQuery m_processor_performance_query;
    CCounterQuery m_processor_base_freq_query;
};
