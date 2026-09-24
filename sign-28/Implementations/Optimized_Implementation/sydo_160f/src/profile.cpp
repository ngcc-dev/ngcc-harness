#include "profile.hpp"

#if defined(SYDO_ENABLE_PROFILE)

#include <atomic>
#include <ctime>

namespace sydo::profile
{

namespace
{
std::atomic<bool> enabled{false};
std::array<std::atomic<bool>, STAGE_COUNT> stage_enabled{};
std::array<std::atomic<uint64_t>, STAGE_COUNT> stage_ns{};

struct init_stage_enabled
{
    init_stage_enabled()
    {
        for (auto& value : stage_enabled)
            value.store(true, std::memory_order_relaxed);
    }
} init_stage_enabled_instance;
}

void set_enabled(bool value)
{
    enabled.store(value, std::memory_order_relaxed);
}

void set_all_stages_enabled(bool value)
{
    for (auto& stage : stage_enabled)
        stage.store(value, std::memory_order_relaxed);
}

void set_stage_enabled(size_t stage, bool value)
{
    if (stage < STAGE_COUNT)
        stage_enabled[stage].store(value, std::memory_order_relaxed);
}

bool is_enabled()
{
    return enabled.load(std::memory_order_relaxed);
}

void reset()
{
    for (auto& value : stage_ns)
        value.store(0, std::memory_order_relaxed);
}

uint64_t now_ns()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

void add_time(size_t stage, uint64_t ns)
{
    if (stage < STAGE_COUNT)
        stage_ns[stage].fetch_add(ns, std::memory_order_relaxed);
}

std::array<uint64_t, STAGE_COUNT> snapshot()
{
    std::array<uint64_t, STAGE_COUNT> result{};
    for (size_t i = 0; i < STAGE_COUNT; ++i)
        result[i] = stage_ns[i].load(std::memory_order_relaxed);
    return result;
}

scope::scope(size_t stage)
    : stage_(stage), start_(0),
      active_(is_enabled() && stage < STAGE_COUNT &&
              stage_enabled[stage].load(std::memory_order_relaxed))
{
    if (active_)
        start_ = now_ns();
}

scope::~scope()
{
    if (active_)
        add_time(stage_, now_ns() - start_);
}

} // namespace sydo::profile

#endif
