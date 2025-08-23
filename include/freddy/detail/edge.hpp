#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"  // P1
#include "node.hpp"    // ref_count

#include <cassert>      // assert
#include <functional>   // std::hash
#include <memory>       // std::shared_ptr
#include <type_traits>  // std::is_nothrow_move_constructible_v
#include <utility>      // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
using node_ptr = std::shared_ptr<node<E, V>>;  // for referencing nodes in a (shared) DD

template <typename E, typename V>
class edge final
{
  public:
    edge(E&& w, node_ptr<E, V> v) noexcept(std::is_nothrow_move_constructible_v<E>) :  // v may already exist
            v{std::move(v)},
            w{std::move(w)}
    {
        assert(this->v);
    }

    edge(edge const&) = delete;

    edge(edge&&) noexcept(std::is_nothrow_move_constructible_v<E>) = default;

    auto operator=(edge const&) = delete;

    auto operator=(edge&&) = delete;  // as an edge is intended directly for a UT

    auto operator()() const noexcept(noexcept(std::hash<E>{}(w)))
    {  // considering large primes typically results in few hash ranges with an accumulation of similarities
        return std::hash<E>()(w) * P1 + std::hash<node_ptr<E, V>>()(v) * P2;
    }

    friend auto operator==(edge const& lhs, edge const& rhs) noexcept(EQ<E>)
    {
        return lhs.w == rhs.w && lhs.v == rhs.v;
    }

    ~edge() noexcept(std::is_nothrow_destructible_v<E>) = default;

    [[nodiscard]] auto is_dead() const noexcept
    {
        return ref == 0;
    }

    // edge methods intended for DD types
    [[nodiscard]] auto weight() const noexcept -> E const&
    {
        return w;
    }

    [[nodiscard]] auto ch() const noexcept -> node_ptr<E, V> const&  // child
    {
        return v;  // edge will still point to v in any case
    }

  private:
    friend manager<E, V>;

    node_ptr<E, V> v;

    E w;  // weight is positioned here due to padding/alignment

    ref_count ref{};
};

static_assert(sizeof(edge<bool, bool>) <= 24, "edge size exceeds expected maximum");

}  // namespace freddy::detail
