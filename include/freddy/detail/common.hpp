#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <algorithm>    // std::min
#include <cassert>      // assert
#include <cmath>        // std::ceil
#include <concepts>     // std::integral
#include <cstddef>      // std::size_t
#include <cstdint>      // std::int32_t
#include <memory>       // std::shared_ptr
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <thread>       // std::thread
#include <type_traits>  // std::false_type
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename T>
struct shared_ptr : std::false_type
{};

template <typename T>
struct shared_ptr<std::shared_ptr<T>> : std::true_type
{};

template <typename T>
concept is_shared_ptr = shared_ptr<T>::value;

template <typename T>
struct unique_ptr : std::false_type
{};

template <typename T>
struct unique_ptr<std::unique_ptr<T>> : std::true_type
{};

template <typename T>
concept is_unique_ptr = unique_ptr<T>::value;

struct comp
{
    using is_transparent = void;  // activate searches with a type other than the key

    template <typename T1, typename T2>
        requires(is_shared_ptr<T1> || is_unique_ptr<T1> || std::is_pointer_v<T1>) &&
                (is_shared_ptr<T2> || is_unique_ptr<T2> || std::is_pointer_v<T2>)  // already stored value
    auto operator()(T1 const& lhs, T2 const& rhs) const
    {
        assert(lhs);
        assert(rhs);

        return *lhs == *rhs;
    }
};

struct hash
{
    using is_transparent = void;

    template <typename T>
        requires is_shared_ptr<T> || is_unique_ptr<T> || std::is_pointer_v<T>
    auto operator()(T const& p) const
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

auto inline replace_all(std::string& str, std::string_view from, std::string_view to)
{
    assert(!from.empty());

    std::size_t pos{0};
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);

        pos += to.length();  // "from" can be a substring of "to"
    }

    return str;
}

}  // namespace freddy::detail
