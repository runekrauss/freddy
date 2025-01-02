#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"  // considering primes typically results in few hash ranges with an accumulation of similarities
#include "edge.hpp"    // since DD operations are usually implemented using edges

#include <cstddef>   // std::size_t
#include <typeinfo>  // typeid

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

class operation  // for caching
{
  public:
    auto operator()() const
    {  // avoid an identical hash with the same input but different operation
        return typeid(*this).hash_code() ^ hash();
    }

    auto friend operator==(operation const& lhs, operation const& rhs)
    {
        return typeid(lhs) == typeid(rhs) && lhs.has_same_input(rhs);
    }

    virtual ~operation() noexcept = default;

  protected:
    operation() = default;

    operation(operation const&) = default;

    operation(operation&&) noexcept = default;

    auto operator=(operation const&) -> operation& = default;

    auto operator=(operation&&) noexcept -> operation& = default;

    [[nodiscard]] auto virtual hash() const -> std::size_t = 0;  // computes the hash code

    [[nodiscard]] auto virtual has_same_input(operation const&) const -> bool = 0;  // compares inputs
};

}  // namespace freddy::detail
