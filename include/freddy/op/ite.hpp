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
class ite : public detail::operation  // if-then-else
{
  public:
    // for finding a cache result based on ITE input
    ite(detail::edge_ptr<E, V> f, detail::edge_ptr<E, V> g, detail::edge_ptr<E, V> h) :
            f{std::move(f)},
            g{std::move(g)},
            h{std::move(h)}
    {
        assert(this->f);
        assert(this->g);
        assert(this->h);
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
        return std::hash<detail::edge_ptr<E, V>>()(f) * detail::P1 +
               std::hash<detail::edge_ptr<E, V>>()(g) * detail::P2 +
               std::hash<detail::edge_ptr<E, V>>()(h) * detail::P3;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<ite const&>(op);

        return std::tie(f, g, h) == std::tie(other.f, other.g, other.h);
    }

    detail::edge_ptr<E, V> f;  // if

    detail::edge_ptr<E, V> g;  // then

    detail::edge_ptr<E, V> h;  // else

    detail::edge_ptr<E, V> r;  // conditional result
};

}  // namespace freddy::op
