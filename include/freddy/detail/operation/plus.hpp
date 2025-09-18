#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"     // P2
#include "freddy/detail/edge.hpp"       // edge
#include "freddy/detail/node.hpp"       // edge_ptr
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
class plus final : public operation  // addition
{
  public:
    using edge = edge<EWeight, NValue>;

    using edge_ptr = edge_ptr<EWeight, NValue>;

    // for looking up a cached result using summands
    plus(edge_ptr const& f, edge_ptr const& g) :
            f{f < g ? f.get() : g.get()},  // exploit ADD's commutativity to improve cache efficiency
            g{f < g ? g.get() : f.get()}
    {
        assert(this->f);
        assert(this->g);
    }

    [[nodiscard]] auto get_result() const noexcept -> edge_ptr
    {
        assert(res);

        return res;
    }

    auto set_result(edge_ptr const& res) noexcept
    {
        assert(res);
        assert(!this->res);  // ensure a valid sum result is only set once

        this->res = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge*>{}(f)*P1 + std::hash<edge*>{}(g)*P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<plus const&>(op);

        return f == other.f && g == other.g;
    }

    edge* f;  // 1st summand

    edge* g;  // 2nd summand

    edge* res{};  // sum result
};

}  // namespace freddy::detail
