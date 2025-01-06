#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <functional>  // std::hash
#include <memory>      // std::shared_ptr
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
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    ite(edge_ptr f, edge_ptr g, edge_ptr h) :
            // for finding a cache result based on ITE input
            f{std::move(f)},
            g{std::move(g)},
            h{std::move(h)}
    {
        assert(this->f);
        assert(this->g);
        assert(this->h);
    }

    edge_ptr r;  // conditional result

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f) * detail::P1 + std::hash<edge_ptr>()(g) * detail::P2 +
               std::hash<edge_ptr>()(h) * detail::P3;
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<ite const&>(op);

        return f == other.f && g == other.g && h == other.h;
    }

    edge_ptr f;  // if

    edge_ptr g;  // then

    edge_ptr h;  // else
};

}  // namespace freddy::op
