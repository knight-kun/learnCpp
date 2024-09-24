#include "time_meter.h"

long long TimeMeter::elapsed_ms(bool do_reset)
{
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
    if (do_reset)
    {
        reset();
    }
    return duration.count();
}

double TimeMeter::elapsed_s(bool do_reset)
{
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start_);
    if (do_reset)
    {
        reset();
    }
    return duration.count();
}