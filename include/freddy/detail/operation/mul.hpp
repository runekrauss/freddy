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
class mul final : public operation  // multiplication
{
  public:
    using edge = edge<EWeight, NValue>;

    using edge_ptr = edge_ptr<EWeight, NValue>;

    // for looking up a cached result using factors
    mul(edge_ptr const& f, edge_ptr const& g) :
            f{f < g ? f.get() : g.get()},  // exploit MUL's commutativity to improve cache efficiency
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
        assert(!this->res);  // ensure a valid product result is only set once

        this->res = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge*>{}(f)*P1 + std::hash<edge*>{}(g)*P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<mul const&>(op);

        return f == other.f && g == other.g;
    }

    edge* f;  // 1st factor

    edge* g;  // 2nd factor

    edge* res{};  // product result
};

}  // namespace freddy::detail
