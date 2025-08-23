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
class has_const : public detail::operation  // constant search
{
  public:
    // for finding a cache result based on constant search input
    has_const(detail::edge_ptr<E, V> f, V c) :
            f{std::move(f)},
            c{std::move(c)}
    {
        assert(this->f);
    }

    auto result() noexcept -> bool&
    {
        return r;
    }

    [[nodiscard]] auto result() const noexcept -> bool const&
    {
        return r;
    }

  private:
    [[nodiscard]] auto hash() const noexcept -> std::size_t override
    {
        return std::hash<detail::edge_ptr<E, V>>()(f) * detail::P1 + std::hash<V>()(c) * detail::P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<has_const const&>(op);

        return f == other.f && c == other.c;
    }

    detail::edge_ptr<E, V> f;  // search operand

    V c;  // constant

    bool r{};  // constant search result
};

}  // namespace freddy::op
