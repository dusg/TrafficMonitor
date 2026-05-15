#include "stdafx.h"
#include "CpuFreq.h"

CPdhCpuFreq::CPdhCpuFreq()
    : CPdhQuery(_T("\\Processor Information(*)\\Processor Frequency"))
{}

bool CPdhCpuFreq::GetCpuFreq(float& freq)
{
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
