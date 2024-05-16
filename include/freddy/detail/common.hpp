#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <algorithm>  // std::min
#include <cassert>    // assert
#include <cmath>      // std::ceil
#include <concepts>   // std::integral
#include <cstdint>    // std::int32_t
#include <memory>     // std::shared_ptr
#include <thread>     // std::thread
#include <vector>     // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

struct comp
{
    template <typename T>
    auto operator()(std::shared_ptr<T> const& a, std::shared_ptr<T> const& b) const noexcept
    {
        assert(a);
        assert(b);

        return (*a == *b);
    }
};

struct hash
{
    template <typename T>
    auto operator()(std::shared_ptr<T> const& p) const noexcept
    {
        assert(p);

        return p->operator()();
    }
};

// =====================================================================================================================
// Functions
// =====================================================================================================================

template <typename T, typename Callable>
requires std::integral<T>
auto inline parallel_for(T const a, T const b, Callable func)
{
    assert(b >= a);

    auto static const n = std::thread::hardware_concurrency();

    assert(n > 0);

    auto const slice = static_cast<std::int32_t>(std::ceil((b - a) / static_cast<float>(n)));

    auto const run = [&func](auto const a2, auto const b2) {
        for (auto i = a2; i < b2; ++i)
        {
            func(i);
        }
    };

    // create pool and run jobs
    std::vector<std::thread> pool;
    pool.reserve(n);
    auto a2 = a;
    auto b2 = std::min(a + slice, b);
    for (auto i = 0; i + 1 < static_cast<std::int32_t>(n) && a2 < b; ++i)
    {
        pool.emplace_back(run, a2, b2);

        a2 = b2;
        b2 = std::min(a2 + slice, b);
    }
    if (a2 < b)
    {
        pool.emplace_back(run, a2, b);
    }

    for (auto& thread : pool)
    {
        thread.join();
    }
}

}  // namespace freddy::detail
