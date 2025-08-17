#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "edge.hpp"  // as DD operations are normally implemented using edges/nodes

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
    auto operator()() const noexcept(noexcept(hash()))
    {  // avoid an identical hash with the same input but different operation
        return typeid(*this).hash_code() ^ hash();
    }

    friend auto operator==(operation const& lhs, operation const& rhs) noexcept
    {
        return typeid(lhs) == typeid(rhs) && lhs.equals(rhs);
    }

    operation(operation const&) = delete;

    auto operator=(operation const&) = delete;

    auto operator=(operation&&) = delete;  // since an operation is to be written directly to the CT

    virtual ~operation() noexcept = default;

  protected:
    operation() noexcept = default;

    operation(operation&&) noexcept = default;

    [[nodiscard]] virtual auto hash() const -> std::size_t = 0;  // computes the hash code

    [[nodiscard]] virtual auto equals(operation const&) const noexcept -> bool = 0;  // compares inputs
};

}  // namespace freddy::detail
