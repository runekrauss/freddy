#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"  // mix_hash

#include <cstddef>  // std::size_t
#include <cstdint>  // std::uint8_t
#include <typeinfo>

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

enum class op_kind : std::uint8_t
{
    ANTIV,
    COMPOSE,
    CONJ,
    HAS_CONST,
    ITE,
    MUL,
    PLUS,
    REPL,
    RESTR,
    SHARPSAT
};

class operation  // for caching
{
  public:
    [[nodiscard]] virtual auto kind() const noexcept -> op_kind = 0;

    auto operator()() const noexcept
    {
        return mix_hash(static_cast<std::size_t>(kind()) * 2654435761uz + hash());
    }

    friend auto operator==(operation const& lhs, operation const& rhs) noexcept
    {
        return lhs.kind() == rhs.kind() && lhs.equals(rhs);
    }

    operation(operation const&) = delete;

    auto operator=(operation const&) = delete;

    auto operator=(operation&&) = delete;  // since an operation is to be written directly to the CT

    virtual ~operation() noexcept = default;

  protected:
    operation() noexcept = default;

    operation(operation&&) noexcept = default;

    // ---- Abstract Methods for DD Operations -------------------------------------------------------------------------

    [[nodiscard]] virtual auto hash() const noexcept -> std::size_t = 0;  // computes a hash code

    [[nodiscard]] virtual auto equals(operation const&) const noexcept -> bool = 0;  // compares the operands
};

}  // namespace freddy::detail
