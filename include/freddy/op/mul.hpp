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
class mul : public detail::operation  // multiplication
{
  public:
    // for finding a cache result based on factors
    mul(detail::edge_ptr<E, V> f, detail::edge_ptr<E, V> g) :
            f{std::move(f)},
            g{std::move(g)}
    {
        assert(this->f);
        assert(this->g);
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
        return std::hash<detail::edge_ptr<E, V>>()(f) * detail::P1 +
               std::hash<detail::edge_ptr<E, V>>()(g) * detail::P2;
    }

    [[nodiscard]] auto equals(operation const& op) const noexcept -> bool override
    {
        auto& other = static_cast<mul const&>(op);

        return f == other.f && g == other.g;
    }

    detail::edge_ptr<E, V> f;  // 1st factor

    detail::edge_ptr<E, V> g;  // 2nd factor

    detail::edge_ptr<E, V> r;  // product result
};

}  // namespace freddy::op
