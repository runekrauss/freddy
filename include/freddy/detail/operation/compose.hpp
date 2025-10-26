#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"            // var_index
#include "freddy/detail/common.hpp"     // P2
#include "freddy/detail/edge.hpp"       // edge
#include "freddy/detail/operation.hpp"  // operation

#include <cassert>     // assert
#include <functional>  // std::hash
#include <tuple>       // std::tie

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
class compose final : public operation  // function substitution
{
  public:
    using edge = edge<EWeight, NValue>;

    using edge_ptr = edge_ptr<EWeight, NValue>;

    // for looking up a cached result using composition input
    compose(edge_ptr const& f, var_index const x, edge_ptr const& g) :
            f{f.get()},
            x{x},
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
        assert(!result);  // ensure a valid composition result is only set once

        result = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        // clang-format off
            return (std::hash<edge*>{}(f) + std::hash<var_index>{}(x)) * P1 + std::hash<edge*>{}(g) * P2;
        // clang-format on
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<compose const&>(op);

        return std::tie(f, x, g) == std::tie(other.f, other.x, other.g);
    }

    edge* f;  // composition operand

    var_index x;  // variable to be substituted

    edge* g;  // function that substitutes

    edge* result{};  // composition result
};

}  // namespace freddy::detail
