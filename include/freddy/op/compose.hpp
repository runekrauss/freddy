#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <cstdint>     // std::int32_t
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
class compose : public detail::operation  // function substitution
{
  public:
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    compose(edge_ptr f, std::int32_t const x, edge_ptr g) :
            // for finding a cache result based on composition input
            f{std::move(f)},
            x{x},
            g{std::move(g)}
    {
        assert(this->f);
        assert(x >= 0);
        assert(this->g);
    }

    edge_ptr r;  // composition result
  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f) ^ std::hash<std::int32_t>()(x) ^ std::hash<edge_ptr>()(g);
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<compose const&>(op);

        return f == other.f && x == other.x && g == other.g;
    }

    auto print(std::ostream& s) const -> void override
    {
        s << '(' << f << ',' << x << ',' << g << ")->" << r;
    }

    edge_ptr f;  // composition operand

    std::int32_t x;  // variable to be substituted

    edge_ptr g;  // function that substitutes
};

}  // namespace freddy::op
