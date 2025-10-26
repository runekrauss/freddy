#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"     // P3
#include "freddy/detail/edge.hpp"       // edge
#include "freddy/detail/node.hpp"       // edge_ptr
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
class ite final : public operation  // if-then-else
{
  public:
    using edge = edge<EWeight, NValue>;

    using edge_ptr = edge_ptr<EWeight, NValue>;

    // for looking up a cached result using ITE input
    ite(edge_ptr const& f, edge_ptr const& g, edge_ptr const& h) :
            f{f.get()},
            g{g.get()},
            h{h.get()}
    {
        assert(this->f);
        assert(this->g);
        assert(this->h);
    }

    [[nodiscard]] auto get_result() const noexcept -> edge_ptr
    {
        assert(result);

        return result;
    }

    auto set_result(edge_ptr const& res) noexcept
    {
        assert(res);
        assert(!result);  // ensure a valid ITE result is only set once

        result = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge*>{}(f)*P1 + std::hash<edge*>{}(g)*P2 + std::hash<edge*>{}(h)*P3;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<ite const&>(op);

        return std::tie(f, g, h) == std::tie(other.f, other.g, other.h);
    }

    edge* f;  // if

    edge* g;  // then

    edge* h;  // else

    edge* result{};  // conditional result
};

}  // namespace freddy::detail
