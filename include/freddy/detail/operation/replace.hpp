#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"     // P2
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
class replace final : public operation  // 1-path replacement
{
  public:
    using edge = edge<EWeight, NValue>;

    using edge_ptr = intrusive_edge_ptr<EWeight, NValue>;

    // for looking up a cached result using replacement input
    replace(edge_ptr const& f, bool const a) :
            f{f.get()},
            a{a}
    {
        assert(this->f);
    }

    [[nodiscard]] auto get_result() const noexcept -> edge_ptr
    {
        assert(result);

        return result;
    }

    auto set_result(edge_ptr const& res) noexcept
    {
        assert(res);
        assert(!result);  // ensure a valid replacement result is only set once

        result = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge*>{}(f)*P1 + std::hash<bool>{}(a)*P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<replace const&>(op);

        return f == other.f && a == other.a;
    }

    edge* f;  // instance for the replacement

    bool a;  // current evaluation

    edge* result{};  // replacement result
};

}  // namespace freddy::detail
