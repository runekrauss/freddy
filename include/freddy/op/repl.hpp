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
class repl : public detail::operation  // 1-path replacement
{
  public:
    using edge_ptr = std::shared_ptr<detail::edge<E, V>>;

    repl(edge_ptr f, bool const a) :
            // for finding a cache result based on replacement input
            f{std::move(f)},
            a{a}
    {
        assert(this->f);
    }

    edge_ptr r;  // replacement result
  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<edge_ptr>()(f) ^ std::hash<bool>()(a);
    }

    [[nodiscard]] auto has_same_input(operation const& op) const noexcept -> bool override
    {
        auto other = static_cast<repl const&>(op);

        return f == other.f && a == other.a;
    }

    auto print(std::ostream& s) const -> void override
    {
        s << '(' << f << ',' << a << ")->" << r;
    }

    edge_ptr f;  // instance for the replacement

    bool a;  // current evaluation
};

}  // namespace freddy::op
