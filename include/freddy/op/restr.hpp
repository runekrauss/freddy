#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <cstdint>     // std::int32_t
#include <functional>  // std::hash
#include <memory>      // std::shared_ptr
#include <utility>     // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::op
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
class restr : public detail::operation  // variable substitution
{
  public:
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    restr(edge_ptr f, std::int32_t const x, bool const a) :  // for finding a cache result based on substitution input
            f{std::move(f)},
            x{x},
            a{a}
    {
        assert(this->f);
        assert(x >= 0);
    }

    edge_ptr r;  // substitution result
  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f) ^ std::hash<std::int32_t>()(x) ^ std::hash<bool>()(a);
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<restr const&>(op);

        return f == other.f && x == other.x && a == other.a;
    }

    auto print(std::ostream& s) const -> void override
    {
        s << '(' << f << ',' << x << ',' << a << ")->" << r;
    }

    edge_ptr f;  // substitution operand

    std::int32_t x;  // variable to assign

    bool a;  // truth value for the variable
};

}  // namespace freddy::op
