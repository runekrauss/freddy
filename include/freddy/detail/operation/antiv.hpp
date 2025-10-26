#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"     // P1
#include "freddy/detail/edge.hpp"       // edge
#include "freddy/detail/node.hpp"       // intrusive_edge_ptr
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

template <hashable EWeight, hashable NValue>
class antiv final : public operation  // antivalence
{
  public:
    using edge = edge<EWeight, NValue>;

    using edge_ptr = intrusive_edge_ptr<EWeight, NValue>;

    // for looking up a cached result using XOR input
    antiv(edge_ptr const& f, edge_ptr const& g) :
            f{f < g ? f.get() : g.get()},  // exploit XOR's commutativity to improve cache efficiency
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
        assert(!result);  // ensure a valid XOR result is only set once

        result = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge*>{}(f)*P1 + std::hash<edge*>{}(g)*P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<antiv const&>(op);

        return f == other.f && g == other.g;
    }

    edge* f;  // 1st XOR operand

    edge* g;  // 2nd XOR operand

    edge* result{};  // XOR result
};

}  // namespace freddy::detail
