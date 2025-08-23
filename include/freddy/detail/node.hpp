#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"  // EQ

#include <cassert>      // assert
#include <cstdint>      // std::uint_fast32_t
#include <functional>   // std::hash
#include <memory>       // std::shared_ptr
#include <tuple>        // std::tie
#include <type_traits>  // std::is_nothrow_destructible_v
#include <utility>      // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

using var_index = std::uint_fast32_t;  // variable index

using ref_count = std::uint16_t;  // to decide in each case whether a DD is "dead"

template <typename, typename>
class edge;

template <typename E, typename V>
using edge_ptr = std::shared_ptr<edge<E, V>>;  // for referencing edges in a (shared) DD

template <typename, typename>
class manager;

template <typename E, typename V>
class node final
{
  public:
    struct branch
    {
        var_index x;  // for decomposition during traversal

        edge_ptr<E, V> hi;  // high child

        edge_ptr<E, V> lo;  // low child
    };

    node(var_index const x, edge_ptr<E, V>&& hi, edge_ptr<E, V>&& lo) noexcept :
            tag{type::INODE},
            inner{x, std::move(hi), std::move(lo)}
    {
        assert(inner.hi);
        assert(inner.lo);
    }

    explicit node(V&& c) noexcept(std::is_nothrow_move_constructible_v<V>) :
            tag{type::LEAF},
            outer{std::move(c)}  // constant
    {}

    node(node const&) = delete;  // node must be unique due to canonicity

    node(node&& other) noexcept(std::is_nothrow_move_constructible_v<V>) :
            ref{other.ref},
            tag{other.tag}
    {
        if (is_const())
        {
            new (&outer) auto{std::move(other.outer)};
        }
        else
        {
            new (&inner) auto{std::move(other.inner)};
        }
    }

    auto operator=(node const&) = delete;

    auto operator=(node&&) = delete;  // as a node is intended directly for a UT

    // multiplicative hash whose primes have a LCM such that few collisions occur
    auto operator()() const noexcept(noexcept(std::hash<V>{}(outer)))
    {
        // x is not part of the key since several UTs are provided
        return is_const() ? std::hash<V>()(outer)  // hash can force an overflow to increase entropy
                          : std::hash<edge_ptr<E, V>>()(inner.hi) * P1 + std::hash<edge_ptr<E, V>>()(inner.lo) * P2;
    }

    friend auto operator==(node const& lhs, node const& rhs) noexcept(EQ<V>)
    {
        if (lhs.is_const() != rhs.is_const())
        {
            return false;
        }
        return lhs.is_const() ? lhs.outer == rhs.outer
                              : std::tie(lhs.inner.x, lhs.inner.hi, lhs.inner.lo) ==
                                    std::tie(rhs.inner.x, rhs.inner.hi, rhs.inner.lo);
    }

    ~node() noexcept(std::is_nothrow_destructible_v<V>)
    {
        if (is_const())
        {
            outer.~V();
        }
        else
        {
            inner.~branch();
        }
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        return tag == type::LEAF;
    }

    [[nodiscard]] auto is_dead() const noexcept
    {
        return ref == 0;
    }

    // node methods intended for DD types
    [[nodiscard]] auto br() const noexcept -> branch const&
    {
        assert(!is_const());

        return inner;
    }

    [[nodiscard]] auto c() const noexcept -> V const&
    {
        assert(is_const());

        return outer;
    }

  private:
    friend manager<E, V>;  // since it is designed as a monolith (e.g. due to reordering)

    ref_count ref{};

    enum struct type : std::uint8_t
    {
        INODE,  // inner node
        LEAF
    } tag;

    union
    {
        branch inner;  // inode

        V outer;  // leaf
    };
};

static_assert(sizeof(node<bool, bool>) <= 48, "node size exceeds expected maximum");

}  // namespace freddy::detail
