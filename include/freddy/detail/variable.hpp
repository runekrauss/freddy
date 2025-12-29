#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/common.hpp"  // hash
#include "freddy/detail/edge.hpp"    // edge
#include "freddy/detail/node.hpp"    // node
#include "freddy/expansion.hpp"      // expansion

#include <boost/smart_ptr/intrusive_ptr.hpp>       // boost::intrusive_ptr
#include <boost/unordered/unordered_flat_set.hpp>  // boost::unordered_flat_set

#include <cassert>      // assert
#include <cstddef>      // std::size_t
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <type_traits>  // std::is_fundamental_v

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

template <class T>
    requires (!std::is_fundamental_v<T>)
using unique_table = boost::unordered_flat_set<boost::intrusive_ptr<T>, hash, equal>;  // UT

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>
class variable final
{
  public:
    variable(expansion const t, std::string_view lbl, std::size_t const utable_size_hint) :
            t{t},
            lbl{lbl}
    {
        assert(!lbl.empty());  // for presentation reasons

        etable.reserve(utable_size_hint);  // rehash(ceil(utable_size_hint / 0.875))
        ntable.reserve(utable_size_hint);
    }

    variable(variable const&) = delete;

    variable(variable&&) = default;

    auto operator=(variable const&) = delete;

    auto operator=(variable&&) = delete;  // as a variable should uniquely belong to a list

    ~variable() = default;

    [[nodiscard]] auto decomposition() const noexcept
    {
        return t;
    }

    [[nodiscard]] auto label() const noexcept -> std::string_view
    {
        return lbl;
    }

    [[nodiscard]] auto edge_table() const noexcept -> unique_table<edge<EWeight, NValue>> const&
    {
        return etable;
    }

    [[nodiscard]] auto node_table() const noexcept -> unique_table<node<EWeight, NValue>> const&
    {
        return ntable;
    }

  private:
    friend manager<EWeight, NValue>;

    expansion t;  // decomposition type

    std::string lbl;  // (immutable) name

    unique_table<edge<EWeight, NValue>> etable;

    unique_table<node<EWeight, NValue>> ntable;
};

static_assert(std::is_nothrow_move_constructible_v<variable<bool, bool>>, "variable requires \"nothrow\" movement");

static_assert(std::is_nothrow_destructible_v<variable<bool, bool>>, "variable must be \"nothrow\" destructible");

}  // namespace freddy::detail
