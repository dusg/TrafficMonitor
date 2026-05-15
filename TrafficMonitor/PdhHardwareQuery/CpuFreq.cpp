#include "stdafx.h"
#include "CpuFreq.h"

CPdhCpuFreq::CPdhCpuFreq()
    : CPdhQuery(_T("\\Processor Information(_Total)\\Processor Frequency"))
{}

bool CPdhCpuFreq::GetCpuFreq(float& freq)
{
    double value{};
    if (QueryValue(value))
    {
        freq = static_cast<float>(value / 1000);
        return true;
    }
    return false;
}
