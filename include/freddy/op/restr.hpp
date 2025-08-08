#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <functional>  // std::hash
#include <tuple>       // std::tie
#include <utility>     // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::op
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
class restr : public detail::operation  // variable substitution
{
  public:
    // for finding a cache result based on substitution input
    restr(detail::edge_ptr<E, V> f, detail::var_index const x, bool const a) :
            f{std::move(f)},
            x{x},
            a{a}
    {
        assert(this->f);
    }

    auto result() noexcept -> detail::edge_ptr<E, V>&
    {
        return r;
    }

    [[nodiscard]] auto result() const noexcept -> detail::edge_ptr<E, V> const&
    {
        return r;
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return (std::hash<detail::edge_ptr<E, V>>()(f) + std::hash<detail::var_index>()(x)) * detail::P1 +
               std::hash<bool>()(a) * detail::P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<restr const&>(op);

        return std::tie(f, x, a) == std::tie(other.f, other.x, other.a);
    }

    detail::edge_ptr<E, V> f;  // substitution operand

    detail::var_index x;  // variable to assign

    bool a;  // truth value for the variable

    detail::edge_ptr<E, V> r;  // substitution result
};

}  // namespace freddy::op
