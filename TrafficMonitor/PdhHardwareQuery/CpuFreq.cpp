#include "stdafx.h"
#include "CpuFreq.h"
#include <PowrProf.h>

namespace
{
    // 某些 Windows SDK / 工具链组合下无法解析 PROCESSOR_POWER_INFORMATION，
    // 这里保留与 ProcessorInformation 输出缓冲区一致的结构布局作为回退路径使用。
    struct ProcessorPowerInformationRecord
    {
        ULONG Number{};
        ULONG MaxMhz{};
        ULONG CurrentMhz{};
        ULONG MhzLimit{};
        ULONG MaxIdleState{};
        ULONG CurrentIdleState{};
    };

    bool GetCpuFreqByPowerInformation(float& freq)
    {
        SYSTEM_INFO system_info{};
        GetSystemInfo(&system_info);
        if (system_info.dwNumberOfProcessors == 0)
            return false;

        std::vector<ProcessorPowerInformationRecord> power_info(system_info.dwNumberOfProcessors);
        if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, power_info.data(),
            static_cast<ULONG>(power_info.size() * sizeof(ProcessorPowerInformationRecord))) != 0)
        {
            return false;
        }

        std::vector<double> freq_values_mhz;
        freq_values_mhz.reserve(power_info.size());
        for (const auto& processor_info : power_info)
        {
            if (processor_info.CurrentMhz == 0)
                continue;
            freq_values_mhz.push_back(static_cast<double>(processor_info.CurrentMhz));
        }
        return CPdhCpuFreq::CalculateCpuFreq(freq_values_mhz, freq);
    }
}

CPdhCpuFreq::CPdhCpuFreq()
    : CPdhQuery(_T("\\Processor Information(*)\\Processor Frequency"))
    , m_processor_performance_query(_T("\\Processor Information(_Total)\\% Processor Performance"))
    , m_processor_base_freq_query(_T("\\Processor Information(_Total)\\Processor Performance Base Frequency"))
{}

bool CPdhCpuFreq::GetCpuFreq(float& freq)
{
    // 先使用更实时的性能级别计数器，再回退到旧的频率来源。
    double processor_performance{};
    double base_freq_mhz{};
    if (m_processor_performance_query.QueryValue(processor_performance)
        && m_processor_base_freq_query.QueryValue(base_freq_mhz)
        && CalculateCpuFreq(processor_performance, base_freq_mhz, freq))
        return true;

    std::vector<CounterValueItem> values;
    if (QueryValues(values))
    {
        std::vector<double> freq_values_mhz;
        freq_values_mhz.reserve(values.size());
        for (const auto& value : values)
        {
            if (value.name == L"_Total")
                continue;
            freq_values_mhz.push_back(value.value);
        }
        if (CalculateCpuFreq(freq_values_mhz, freq))
            return true;
    }
    return GetCpuFreqByPowerInformation(freq);
}

bool CPdhCpuFreq::CalculateCpuFreq(double processor_performance, double base_freq_mhz, float& freq)
{
    if (processor_performance <= 0 || base_freq_mhz <= 0)
        return false;

    // processor_performance 是相对于基础频率的百分比，先除以100得到倍率，
    // 再将 MHz 转为 GHz。
    freq = static_cast<float>(processor_performance * base_freq_mhz / 100000.0);
    return true;
}

bool CPdhCpuFreq::CalculateCpuFreq(const std::vector<double>& freq_values_mhz, float& freq)
{
    double total_freq{};
    size_t valid_freq_count{};
    for (double freq_value_mhz : freq_values_mhz)
    {
        if (freq_value_mhz <= 0)
            continue;

        total_freq += freq_value_mhz;
        ++valid_freq_count;
    }
    if (valid_freq_count == 0)
        return false;

    // 使用活跃逻辑核心的平均频率，避免单个核心长期保持高频时界面看起来固定不变。
    freq = static_cast<float>(total_freq / valid_freq_count / 1000.0);
    return true;
}
