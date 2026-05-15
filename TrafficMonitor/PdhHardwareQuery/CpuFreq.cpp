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

        ULONG max_freq{};
        bool freq_acquired = false;
        for (const auto& processor_info : power_info)
        {
            if (processor_info.CurrentMhz == 0)
                continue;

            if (!freq_acquired || processor_info.CurrentMhz > max_freq)
            {
                max_freq = processor_info.CurrentMhz;
                freq_acquired = true;
            }
        }
        if (!freq_acquired)
            return false;

        freq = static_cast<float>(max_freq) / 1000.0f;
        return true;
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
        double max_freq{};
        bool freq_acquired = false;
        for (const auto& value : values)
        {
            if (value.name == L"_Total")
                continue;

            if (!freq_acquired || value.value > max_freq)
            {
                max_freq = value.value;
                freq_acquired = true;
            }
        }
        if (freq_acquired)
        {
            freq = static_cast<float>(max_freq / 1000);
            return true;
        }
    }
    return false;
}
