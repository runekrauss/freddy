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
class compose : public detail::operation  // function substitution
{
  public:
    // for finding a cache result based on composition input
    compose(detail::edge_ptr<E, V> f, detail::var_index const x, detail::edge_ptr<E, V> g) :
            f{std::move(f)},
            x{x},
            g{std::move(g)}
    {
        assert(this->f);
        assert(this->g);
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
               std::hash<detail::edge_ptr<E, V>>()(g) * detail::P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<compose const&>(op);

        return std::tie(f, x, g) == std::tie(other.f, other.x, other.g);
    }

    detail::edge_ptr<E, V> f;  // composition operand

    detail::var_index x;  // variable to be substituted

    detail::edge_ptr<E, V> g;  // function that substitutes

    detail::edge_ptr<E, V> r;  // composition result
};

}  // namespace freddy::op
