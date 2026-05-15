#include "stdafx.h"
#include "CpuFreq.h"
#include <PowrProf.h>
#include <unordered_map>

namespace
{
    // 某些 Windows SDK / Visual Studio 工具链组合下，PROCESSOR_POWER_INFORMATION
    // 会出现无法解析或定义不可见的编译问题，因此这里保留与
    // ProcessorInformation 输出缓冲区一致的结构布局作为回退路径使用。
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
    , m_processor_performance_query(_T("\\Processor Information(*)\\% Processor Performance"))
    , m_processor_base_freq_query(_T("\\Processor Information(*)\\Processor Performance Base Frequency"))
{}

bool CPdhCpuFreq::GetCpuFreq(float& freq)
{
    // 先使用更实时的性能级别计数器，再回退到旧的频率来源。
    std::vector<CounterValueItem> processor_performance_values;
    std::vector<CounterValueItem> base_freq_values;
    if (m_processor_performance_query.QueryValues(processor_performance_values)
        && m_processor_base_freq_query.QueryValues(base_freq_values))
    {
        std::unordered_map<std::wstring, double> base_freq_map;
        for (const auto& value : base_freq_values)
        {
            if (value.name == L"_Total" || value.value <= 0)
                continue;
            base_freq_map[value.name] = value.value;
        }

        std::vector<double> realtime_freq_values_mhz;
        realtime_freq_values_mhz.reserve(processor_performance_values.size());
        for (const auto& value : processor_performance_values)
        {
            if (value.name == L"_Total" || value.value <= 0)
                continue;
            auto iter = base_freq_map.find(value.name);
            if (iter == base_freq_map.end())
                continue;
            realtime_freq_values_mhz.push_back(value.value * iter->second / 100.0);
        }
        if (CalculateCpuFreq(realtime_freq_values_mhz, freq))
            return true;
    }

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

    // processor_performance 是相对于基础频率的百分比，这里统一除以100000：
    // 其中 ÷100 用于把百分比转成倍率，÷1000 用于把 MHz 转成 GHz。
    freq = static_cast<float>(processor_performance * base_freq_mhz / 100000.0);
    return true;
}

bool CPdhCpuFreq::CalculateCpuFreq(const std::vector<double>& freq_values_mhz, float& freq)
{
    double max_freq{};
    for (double freq_value_mhz : freq_values_mhz)
    {
        if (freq_value_mhz <= 0)
            continue;

        if (freq_value_mhz > max_freq)
            max_freq = freq_value_mhz;
    }
    if (max_freq <= 0)
        return false;

    // 显示当前最高频的活跃逻辑核心，避免平均值掩盖瞬时升频。
    freq = static_cast<float>(max_freq / 1000.0);
    return true;
}
