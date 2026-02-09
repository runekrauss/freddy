#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"     // P1, P2
#include "freddy/detail/edge.hpp"       // detail::edge
#include "freddy/detail/node.hpp"       // detail::edge_ptr
#include "freddy/detail/operation.hpp"  // operation

#include <cassert>     // assert
#include <functional>  // std::hash

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

/**
 * @brief Cache operation class for ZDD Set Union (P ∪ Q).
 *
 * This class is used to store and retrieve results of the ZDD union operation from the compute cache.
 * It uniquely identifies a union operation by its two operands.
 *
 * Key features:
 * - Commutativity: P ∪ Q == Q ∪ P. The operands are sorted in the constructor to ensure canonical
 *   representation in the cache (f < g). This doubles the cache hit rate.
 * - Hash collision avoidance: Adds +1 to the hash to distinguish it from other commutative operations
 *   like Intersection or Conjunction.
 */
template <hashable EWeight, hashable NValue>
class zdd_union final : public operation
{
  public:
    using edge = detail::edge<EWeight, NValue>;

    using edge_ptr = detail::edge_ptr<EWeight, NValue>;

    // for looking up a cached result using operands
    zdd_union(edge_ptr const& f, edge_ptr const& g) :
            f{f < g ? f.get() : g.get()},  // exploit commutativity: sort operands
            g{f < g ? g.get() : f.get()}
    {
        assert(this->f);
        assert(this->g);
    }

    [[nodiscard]] auto get_result() const noexcept -> edge_ptr
    {
        assert(result);

        return result;
    }

    auto set_result(edge_ptr const& res) noexcept
    {
        assert(res);
        assert(!result);

        result = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        // Unique hash for Union operation
        return std::hash<edge*>{}(f)*P1 + std::hash<edge*>{}(g)*P2 + 1;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<zdd_union const&>(op);

        return f == other.f && g == other.g;
    }

    edge* f;

    edge* g;

    edge* result{};
};

}  // namespace freddy::detail
