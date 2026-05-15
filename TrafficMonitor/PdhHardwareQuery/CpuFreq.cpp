#include "stdafx.h"
#include "CpuFreq.h"
#include <PowrProf.h>

namespace
{
    bool GetCpuFreqByPowerInformation(float& freq)
    {
        SYSTEM_INFO system_info{};
        GetSystemInfo(&system_info);
        if (system_info.dwNumberOfProcessors == 0)
            return false;

        std::vector<PROCESSOR_POWER_INFORMATION> power_info(system_info.dwNumberOfProcessors);
        if (CallNtPowerInformation(ProcessorInformation, nullptr, 0, power_info.data(),
            static_cast<ULONG>(power_info.size() * sizeof(PROCESSOR_POWER_INFORMATION))) != 0)
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
{}

bool CPdhCpuFreq::GetCpuFreq(float& freq)
{
    if (GetCpuFreqByPowerInformation(freq))
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
    return false;
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
