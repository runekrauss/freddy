#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint_fast32_t
#include <optional>     // std::optional
#include <type_traits>  // std::is_unsigned_v

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Aliases
// =====================================================================================================================

using var_index = std::uint_fast32_t;  // variable index used to label internal nodes

static_assert(std::is_integral_v<var_index> && std::is_unsigned_v<var_index>, "var_index must be unsigned");

// =====================================================================================================================
// Types
// =====================================================================================================================

struct config
{
    std::size_t utable_size_hint{1'679};  // minimum capacity of each UT per DD level

    std::size_t cache_size_hint{215'039};  // minimum capacity of the operation cache (a.k.a. CT)

    var_index init_var_cap{16};  // initial capacity for variables, which is subsequently doubled on demand

    float max_node_growth{1.2f};  // permitted node growth factor during variable reordering

    std::optional<std::size_t> heap_mem_limit;  // target heap usage in bytes before GC (auto-estimated if unset)
};

}  // namespace freddy
