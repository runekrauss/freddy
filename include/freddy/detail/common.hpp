#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <boost/smart_ptr/intrusive_ptr.hpp>  // boost::intrusive_ptr

#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>  // getrlimit
#elifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // exclude unused APIs such as cryptography
#define NOMINMAX             // preventing conflicts with std::max
#include <windows.h>         // MEMORYSTATUSEX
#endif

#include <algorithm>    // std::max
#include <cassert>      // assert
#include <concepts>     // std::convertible_to
#include <cstddef>      // std::size_t
#include <memory>       // std::pointer_traits
#include <thread>       // std::thread
#include <type_traits>  // std::true_type
#include <utility>      // std::declval
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Concepts
// =====================================================================================================================

template <typename T>
concept hashable = requires(T const& key, T const& key2) {  // for unordered containers
    { std::hash<T>{}(key) } -> std::convertible_to<std::size_t>;
    { key == key2 } -> std::convertible_to<bool>;
};

// =====================================================================================================================
// Constants
// =====================================================================================================================

inline constexpr auto P1 = 12'583'037;  // prime

inline constexpr auto P2 = 4'256'383;

inline constexpr auto P3 = 741'563;

template <typename T1, typename T2>
inline constexpr auto is_nothrow_comparable = noexcept(std::declval<T1 const&>() == std::declval<T2 const&>());

// =====================================================================================================================
// Aliases
// =====================================================================================================================

template <typename Ptr>
using pointee = std::pointer_traits<Ptr>::element_type;  // data type pointed to

// =====================================================================================================================
// Types
// =====================================================================================================================

struct hash final
{
    using is_transparent = void;  // enable heterogeneous lookup

    using is_avalanching = std::true_type;  // do not use post-mixing

    template <class Ptr>
        requires std::is_same_v<Ptr, boost::intrusive_ptr<pointee<Ptr>>> ||
                 std::is_same_v<Ptr, std::unique_ptr<pointee<Ptr>>> || std::is_pointer_v<Ptr>
    auto operator()(Ptr const& ptr) const noexcept(std::is_nothrow_invocable_v<pointee<Ptr> const&>)
    {
        assert(ptr);

        return (*ptr)();  // using the key from ptr
    }
};

struct equal final
{
    using is_transparent = void;

    template <typename Ptr, typename StoredPtr>
        requires (std::is_same_v<Ptr, boost::intrusive_ptr<pointee<Ptr>>> ||
                  std::is_same_v<Ptr, std::unique_ptr<pointee<Ptr>>> || std::is_pointer_v<Ptr>) &&
                 (std::is_same_v<StoredPtr, boost::intrusive_ptr<pointee<StoredPtr>>> ||
                  std::is_same_v<StoredPtr, std::unique_ptr<pointee<StoredPtr>>> || std::is_pointer_v<StoredPtr>)
    auto operator()(Ptr const& needle, StoredPtr const& val) const
        noexcept(is_nothrow_comparable<pointee<Ptr>, pointee<StoredPtr>>)
    {
        assert(needle);
        assert(val);

        return *needle == *val;  // typeid(decltype(needle)).name()
    }
};

// =====================================================================================================================
// Functions
// =====================================================================================================================

inline auto heap_mem_limit() noexcept  // in bytes
{
#if defined(__APPLE__) || defined(__linux__)
    rlimit res{};  // resource
    if (getrlimit(RLIMIT_DATA, &res) == 0 && res.rlim_cur != RLIM_INFINITY)
    {
        return static_cast<std::size_t>(res.rlim_cur);  // soft data limit
    }
#elifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status))
    {
        return static_cast<std::size_t>(status.ullTotalPhys);  // since there is no equivalent to getrlimit
    }
#endif
    return 4uz << 30uz;  // unsupported platform or fallback
}

inline auto parallel_for(std::integral auto const a, std::integral auto const b, auto func)  // [a, b)
{
    assert(b > a);

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

}  // namespace freddy::detail
