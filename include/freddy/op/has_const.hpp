#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
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
class has_const : public detail::operation  // constant search
{
  public:
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    has_const(edge_ptr f, V c) :
            // for finding a cache result based on constant search input
            f{std::move(f)},
            c{std::move(c)}
    {
        assert(this->f);
    }

    bool r{};  // constant search result

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f) + std::hash<V>()(c);
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<has_const const&>(op);

        return f == other.f && c == other.c;
    }

    auto print(std::ostream& s) const -> void override
    {
        s << '(' << f << ',' << c << ")->" << r;
    }

    edge_ptr f;  // search operand

    V c;  // constant
};

}  // namespace freddy::op
