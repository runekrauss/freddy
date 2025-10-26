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
#include <optional>    // std::optional

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
class has_const final : public operation  // constant search
{
  public:
    using edge = edge<EWeight, NValue>;

    // for looking up a cached result using constant search input
    has_const(intrusive_edge_ptr<EWeight, NValue> const& f, NValue const& c) :
            f{f.get()},
            c{c}
    {
        assert(this->f);
    }

    [[nodiscard]] auto get_result() const noexcept
    {
        assert(result);

        return *result;
    }

    auto set_result(bool const res) noexcept
    {
        assert(!result);  // ensure a valid constant search result is only set once

        result = res;
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge*>{}(f)*P1 + std::hash<NValue>{}(c)*P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<has_const const&>(op);

        return f == other.f && c == other.c;
    }

    edge* f;  // search operand

    NValue c;  // constant

    std::optional<bool> result;  // constant search result designed this way for safety reasons
};

}  // namespace freddy::detail
