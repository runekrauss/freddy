#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint32_t
#include <optional>     // std::nullopt
#include <type_traits>  // std::is_unsigned_v

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Aliases
// =====================================================================================================================

using var_index = std::uint32_t;  // variable index used to label internal nodes

static_assert(std::is_integral_v<var_index> && std::is_unsigned_v<var_index>, "var_index must be unsigned");

// =====================================================================================================================
// Types
// =====================================================================================================================

struct config final
{
    std::size_t utable_size_hint{1'679};  // minimum capacity of each UT per DD level

    std::size_t cache_size_hint{215'039};  // minimum capacity of the operation cache (a.k.a. CT)

    var_index init_var_cap{16};  // initial capacity for variables, which is subsequently doubled on demand

    float max_node_growth{1.2f};  // permitted node growth factor during variable reordering

    std::optional<std::size_t> heap_mem_limit{std::nullopt};  // heap usage in bytes before GC (auto-estimated if unset)
};

static_assert(std::is_trivially_copyable_v<config>, "config must be trivially copyable");

}  // namespace freddy
