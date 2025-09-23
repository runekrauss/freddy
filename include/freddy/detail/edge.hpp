#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"  // is_nothrow_comparable
#include "freddy/detail/node.hpp"    // ref_count

#include <boost/smart_ptr/intrusive_ptr.hpp>  // intrusive_ptr_release

#include <cassert>      // assert
#include <functional>   // std::hash
#include <stdexcept>    // std::overflow_error
#include <type_traits>  // std::is_nothrow_move_constructible_v
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

// =====================================================================================================================
// Aliases
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
using node_ptr = boost::intrusive_ptr<node<EWeight, NValue>>;  // for referencing nodes in a (shared) DD

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
class edge final
{
  public:
    using node_ptr = node_ptr<EWeight, NValue>;

    edge(EWeight&& w, node_ptr v) noexcept(std::is_nothrow_move_constructible_v<EWeight>) :  // v may already exist.
            v{std::move(v)},
            w{std::move(w)}
    {
        assert(this->v);
    }

    edge(edge const&) = delete;  // edge must be unique due to canonicity (normalization)

    edge(edge&&) noexcept(std::is_nothrow_move_constructible_v<EWeight>) = default;

    auto operator=(edge const&) = delete;

    auto operator=(edge&&) = delete;  // as an edge is intended for a UT

    auto operator()() const noexcept(std::is_nothrow_invocable_v<std::hash<EWeight> const&, EWeight const&>)
    {  // considering large primes typically results in few hash ranges with an accumulation of similarities
        return std::hash<EWeight>{}(w)*P1 + std::hash<node_ptr>{}(v)*P2;
    }

    friend auto operator==(edge const& lhs, edge const& rhs) noexcept(is_nothrow_comparable<EWeight, EWeight>)
    {
        return lhs.w == rhs.w && lhs.v == rhs.v;
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        return v->is_const();
    }

    [[nodiscard]] auto is_dead() const noexcept
    {
        return ref == 1;  // UT is the owner => edge is no longer directly reachable from any DD root
    }

    [[nodiscard]] auto weight() const noexcept -> EWeight const&
    {
        return w;
    }

    [[nodiscard]] auto ch() const noexcept -> node_ptr const&  // child
    {
        return v;
    }

  private:
    friend manager<EWeight, NValue>;

    ~edge() noexcept(std::is_nothrow_destructible_v<EWeight>) = default;  // for safety reasons

    friend auto intrusive_ptr_add_ref(edge* const e)
    {
        if (e->ref == std::numeric_limits<ref_count>::max())
        {  // although e seems to be very important
            throw std::overflow_error{"An edge has been maximally referenced. Change the variable order."};
        }

        ++e->ref;
    }

    // so that an edge can be freed, which is triggered by the UT
    friend auto intrusive_ptr_release(edge* const e) noexcept(std::is_nothrow_destructible_v<edge>)
    {
        assert(e->ref != 0);

        --e->ref;

        if (e->ref == 0)
        {
            delete e;  // NOLINT(cppcoreguidelines-owning-memory)
        }
    }

    node_ptr v;

    EWeight w;  // weight is placed here due to padding/alignment

    ref_count ref{};
};

static_assert(sizeof(edge<bool, bool>) <= 16, "edge size exceeds expected maximum");

}  // namespace freddy::detail
