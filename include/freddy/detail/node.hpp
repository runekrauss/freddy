#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"         // var_index
#include "freddy/detail/common.hpp"  // hashable

#include <boost/smart_ptr/intrusive_ptr.hpp>  // intrusive_ptr_add_ref

#include <cassert>      // assert
#include <cstdint>      // std::uint16_t
#include <functional>   // std::hash
#include <limits>       // std::numeric_limits
#include <stdexcept>    // std::overflow_error
#include <tuple>        // std::tie
#include <type_traits>  // std::is_nothrow_destructible_v
#include <utility>      // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

template <hashable, hashable>
class manager;

template <hashable, hashable>
class edge;

// =====================================================================================================================
// Aliases
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
using edge_ptr = boost::intrusive_ptr<edge<EWeight, NValue>>;  // for referencing an edge in a (shared) DD

using ref_count = std::uint16_t;  // to decide in each case whether a DD is "dead"

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
class node final
{
  public:
    using edge_ptr = edge_ptr<EWeight, NValue>;

    struct branch
    {
        var_index x;  // for decomposition during traversal

        edge_ptr hi;  // high child

        edge_ptr lo;  // low child
    };

    node(var_index const x, edge_ptr&& hi, edge_ptr&& lo) noexcept :
            tag{type::INODE},
            inner{x, std::move(hi), std::move(lo)}
    {
        assert(inner.hi);
        assert(inner.lo);
    }

    explicit node(NValue&& c) noexcept(std::is_nothrow_move_constructible_v<NValue>) :
            tag{type::LEAF},
            outer{std::move(c)}  // constant
    {}

    node(node const&) = delete;  // node must be unique due to canonicity

    node(node&& other) noexcept(std::is_nothrow_move_constructible_v<NValue>) :
            tag{other.tag},
            ref{other.ref}
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

    auto operator=(node&&) = delete;  // as a node is intended for a UT

    // multiplicative hash whose primes have a LCM such that few collisions occur
    auto operator()() const noexcept(std::is_nothrow_invocable_v<std::hash<NValue> const&, NValue const&>)
    {
        // x is not part of this function since several UTs are provided.
        return is_const() ? std::hash<NValue>{}(outer)  // hash can force an overflow to increase entropy
                          : std::hash<edge_ptr>{}(inner.hi) * P1 + std::hash<edge_ptr>{}(inner.lo) * P2;
    }

    friend auto operator==(node const& lhs, node const& rhs) noexcept(is_nothrow_comparable<NValue, NValue>)
    {
        if (lhs.is_const() != rhs.is_const())
        {
            return false;
        }
        return lhs.is_const() ? lhs.outer == rhs.outer
                              : std::tie(lhs.inner.x, lhs.inner.hi, lhs.inner.lo) ==
                                    std::tie(rhs.inner.x, rhs.inner.hi, rhs.inner.lo);  // x for completeness
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        return tag == type::LEAF;
    }

    [[nodiscard]] auto is_dead() const noexcept
    {
        return ref == 1;  // UT is the owner => node is no longer directly reachable from any DD root
    }

    [[nodiscard]] auto br() const noexcept -> branch const&
    {
        assert(!is_const());

        return inner;
    }

    [[nodiscard]] auto value() const noexcept -> NValue const&
    {
        assert(is_const());

        return outer;
    }

  private:
    enum struct type : std::uint8_t
    {
        INODE,  // inner node
        LEAF
    } tag;

    friend manager<EWeight, NValue>;  // since it is designed as a monolith (e.g. due to reordering)

    ~node() noexcept(std::is_nothrow_destructible_v<NValue>)
    {
        if (is_const())
        {
            outer.~NValue();
        }
        else
        {
            inner.~branch();
        }
    }

    friend auto intrusive_ptr_add_ref(node* const v)
    {
        if (v->ref == std::numeric_limits<ref_count>::max())
        {  // although v seems to be very important
            throw std::overflow_error{"A node has been maximally referenced. Change the variable order."};
        }

        ++v->ref;
    }

    // so that a node can be freed, which is triggered by the UT
    friend auto intrusive_ptr_release(node* const v) noexcept(std::is_nothrow_destructible_v<node>)
    {
        assert(v->ref != 0);

        --v->ref;

        if (v->ref == 0)
        {
            delete v;
        }
    }

    ref_count ref{};  // reference counter that is important for a potential GC

    union
    {
        branch inner;  // inode

        NValue outer;  // leaf
    };
};

static_assert(sizeof(node<bool, bool>) <= 32, "node size exceeds expected maximum");

}  // namespace freddy::detail
