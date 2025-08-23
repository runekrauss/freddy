#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"            // comp
#include "edge.hpp"              // edge
#include "freddy/expansion.hpp"  // expansion
#include "node.hpp"              // node

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/unordered/unordered_flat_set.hpp>  // boost::unordered_flat_set
#include <boost/variant2/variant.hpp>

#include <cassert>      // assert
#include <cstddef>      // std::size_t
#include <memory>       // std::shared_ptr
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <type_traits>  // std::is_nothrow_move_constructible_v

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename T>
using utable = boost::unordered_flat_set<std::shared_ptr<T>, hash, comp>;  // unique table

template <typename E, typename V>
auto operator<<(std::ostream&, manager<E, V> const&) -> std::ostream&;

template <typename E, typename V>
class variable final
{
  public:
    variable(expansion const t, std::string_view l, std::size_t const ut_size = {}) :
            t{t},
            l{l}
    {
        assert(!l.empty());  // for presentation reasons

        et.reserve(ut_size);  // rehash(ceil(ut_size / 0.875))
        nt.reserve(ut_size);
    }

    variable(variable const&) = delete;

    variable(variable&&) noexcept(std::is_nothrow_move_constructible_v<utable<edge<E, V>>> &&
                                  std::is_nothrow_move_constructible_v<utable<node<E, V>>>) = default;

    auto operator=(variable const&) = delete;

    auto operator=(variable&&) = delete;  // as a variable should uniquely belong to a list

    ~variable() noexcept(std::is_nothrow_destructible_v<utable<edge<E, V>>> &&
                         std::is_nothrow_destructible_v<utable<node<E, V>>>) = default;

  private:
    friend manager<E, V>;

    friend auto operator<< <>(std::ostream&, manager<E, V> const&) -> std::ostream&;

    expansion t;  // decomposition type

    std::string l;  // (immutable) name

    utable<edge<E, V>> et;

    utable<node<E, V>> nt;
};

}  // namespace freddy::detail
