#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/operation.hpp"  // detail::operation

#include <cassert>     // assert
#include <functional>  // std::hash
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
    // for finding a cache result based on replacement input
    repl(detail::edge_ptr<E, V> f, bool const a) :
            f{std::move(f)},
            a{a}
    {
        assert(this->f);
    }

    auto result() noexcept -> detail::edge_ptr<E, V>&
    {
        return r;
    }

    [[nodiscard]] auto result() const noexcept -> detail::edge_ptr<E, V> const&
    {
        return r;
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<detail::edge_ptr<E, V>>()(f) * detail::P1 + std::hash<bool>()(a) * detail::P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<repl const&>(op);

        return f == other.f && a == other.a;
    }

    detail::edge_ptr<E, V> f;  // instance for the replacement

    bool a;  // current evaluation

    detail::edge_ptr<E, V> r;  // replacement result
};

}  // namespace freddy::op
