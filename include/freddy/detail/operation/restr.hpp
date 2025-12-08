#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"            // var_index
#include "freddy/detail/common.hpp"     // P2
#include "freddy/detail/edge.hpp"       // detail::edge
#include "freddy/detail/node.hpp"       // detail::edge_ptr
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
class restr final : public operation  // variable substitution
{
  public:
    using edge = detail::edge<EWeight, NValue>;

    using edge_ptr = detail::edge_ptr<EWeight, NValue>;

    // for looking up a cached result using substitution input
    restr(edge_ptr const& f, var_index const x, bool const a) :
            f{f.get()},
            x{x},
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
        assert(!result);  // ensure a valid substitution result is only set once

        result = res.get();
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        // clang-format off
            return (std::hash<edge*>{}(f) + std::hash<var_index>{}(x)) * P1 + std::hash<bool>{}(a) * P2;
        // clang-format on
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<restr const&>(op);

        return std::tie(f, x, a) == std::tie(other.f, other.x, other.a);
    }

    edge* f;  // substitution operand

    var_index x;  // variable to assign

    bool a;  // truth value for the variable

    edge* result{};  // substitution result
};

}  // namespace freddy::detail
