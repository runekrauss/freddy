#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <algorithm>    // std::min
#include <cassert>      // assert
#include <concepts>     // std::integral
#include <cstddef>      // std::size_t
#include <memory>       // std::shared_ptr
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <thread>       // std::thread
#include <type_traits>  // std::false_type
#include <utility>      // std::declval
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename>
struct shared_ptr : std::false_type
{};

template <typename T>
struct shared_ptr<std::shared_ptr<T>> : std::true_type
{};

template <typename T>
concept is_shared_ptr = shared_ptr<T>::value;

template <typename>
struct unique_ptr : std::false_type
{};

template <typename T>
struct unique_ptr<std::unique_ptr<T>> : std::true_type
{};

template <typename T>
concept is_unique_ptr = unique_ptr<T>::value;

struct hash
{
    using is_avalanching = std::true_type;  // do not use post-mixing

    using is_transparent = void;  // activate searches with a type other than the key

    template <typename T>
    requires is_shared_ptr<T> || is_unique_ptr<T> || std::is_pointer_v<T>
    auto operator()(T const& p) const noexcept(noexcept((*p)()))
    {
        assert(p);

        return p->operator()();
    }
};

struct comp
{
    using is_transparent = void;

    template <typename T1, typename T2>
    requires(is_shared_ptr<T1> || is_unique_ptr<T1> || std::is_pointer_v<T1>) &&
            (is_shared_ptr<T2> || is_unique_ptr<T2> || std::is_pointer_v<T2>)  // already stored value
    auto operator()(T1 const& lhs, T2 const& rhs) const noexcept(noexcept(*lhs == *rhs))
    {
        assert(lhs);
        assert(rhs);

        return *lhs == *rhs;
    }
};

// =====================================================================================================================
// Constants
// =====================================================================================================================

template <typename T>
inline auto constexpr EQ = noexcept(std::declval<T const&>() == std::declval<T const&>());

inline auto constexpr P1 = 12583037;  // prime

inline auto constexpr P2 = 4256383;

inline auto constexpr P3 = 741563;

// =====================================================================================================================
// Functions
// =====================================================================================================================

inline auto parallel_for(std::integral auto const a, std::integral auto const b, auto func)
{
    assert(b >= a);

    // determine workload
    static auto const n = std::max(1u, std::thread::hardware_concurrency());
    auto const total = b - a;  // number of iterations
    auto const workers = std::min<decltype(total)>(n, total);
    auto const slice = (total + workers - 1) / workers;  // How many iterations should each thread run?

    std::vector<std::thread> pool;
    pool.reserve(workers);

    for (std::remove_const_t<decltype(workers)> i = 0; i < workers; ++i)
    {  // run job
        auto const begin = a + i * slice;
        auto const end = std::min(begin + slice, b);
        pool.emplace_back([begin, end, &func]() {
            for (auto j = begin; j < end; ++j)
            {
                func(j);
            }
        });
    }

    for (auto& thread : pool)
    {
        thread.join();
    }
}

inline auto replace_all(std::string& str, std::string_view from, std::string_view to) -> std::string&
{
    assert(!from.empty());

    std::size_t pos{};
    while ((pos = str.find(from, pos)) != std::string::npos)
    {
        str.replace(pos, from.length(), to);

        pos += to.length();  // "from" can be a substring of "to"
    }

    return str;
}

}  // namespace freddy::detail
