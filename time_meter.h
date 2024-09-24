/*****************************************************************
 * \file   time_meter.h
 * \brief  计时器类
 *
 * \author yangsk
 * \date   July 2024
 *********************************************************************/
#ifndef TIME_METER_H_
#define TIME_METER_H_
#include <chrono>

/**
 * @brief 计时器，用于统计程序运行时间，
 */
class TimeMeter
{

public:
    TimeMeter() : start_(std::chrono::high_resolution_clock::now()) {}

    inline void reset() { start_ = std::chrono::high_resolution_clock::now(); }

    /**
     * @brief 获取当前计时器的持续时间
     * @tparam T 返回的时间类型
     * @param reset 是否更新计时器
     */
    template <typename T>
    auto elapsed(bool do_reset)
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<T>(end - start_);
        do_reset ? reset() : void();
        return duration.count();
    }

    /**
     * @brief 获取当前计时器的持续时间，单位ms
     * @param do_reset 是否重置计时器，默认重置
     */
    long long elapsed_ms(bool do_reset = true);

    /**
     * @brief 获取当前计时器的持续时间，单位s
     * @param do_reset 是否重置计时器，默认重置
     */
    double elapsed_s(bool do_reset = true);

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

#endif