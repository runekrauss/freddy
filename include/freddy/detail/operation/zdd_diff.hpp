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
 * @brief Cache operation class for ZDD Set Difference (P \ Q).
 *
 * This class is used to store and retrieve results of the ZDD difference operation from the compute cache.
 * It uniquely identifies a difference operation by its two operands.
 *
 * Key features:
 * - NO Commutativity: P \ Q != Q \ P. Unlike union and intersection, the order of operands matters.
 *   Therefore, the operands are NOT sorted in the constructor.
 * - Hash collision avoidance: Adds +3 to the hash to distinguish it from other operations.
 */
template <hashable EWeight, hashable NValue>
class zdd_diff final : public operation
{
  public:
    using edge = detail::edge<EWeight, NValue>;

    using edge_ptr = detail::edge_ptr<EWeight, NValue>;

    // for looking up a cached result using operands
    // Note: NOT commutative, so order matters
    zdd_diff(edge_ptr const& f, edge_ptr const& g) :
            f{f.get()},
            g{g.get()}
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
        // Unique hash for Difference operation
        return std::hash<edge*>{}(f)*P1 + std::hash<edge*>{}(g)*P2 + 3;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<zdd_diff const&>(op);

        return f == other.f && g == other.g;  // order matters!
    }

    edge* f;

    edge* g;

    edge* result{};
};

}  // namespace freddy::detail
