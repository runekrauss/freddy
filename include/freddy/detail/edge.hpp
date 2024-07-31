#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <cassert>     // assert
#include <functional>  // std::hash
#include <memory>      // std::shared_ptr
#include <ostream>     // std::ostream
#include <utility>     // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

template <typename, typename>
class node;

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
struct edge
{
    using node_ptr = std::shared_ptr<node<E, V>>;

    edge(E w, node_ptr v) :
            w{std::move(w)},
            v{std::move(v)}
    {
        assert(this->v);
    }

    auto operator()() const
    {
        return (std::hash<E>()(w) ^ std::hash<node_ptr>()(v));
    }

    auto friend operator==(edge const& lhs, edge const& rhs)
    {
        return (lhs.w == rhs.w && lhs.v == rhs.v);
    }

    auto friend operator<<(std::ostream& s, edge const& e) -> std::ostream&
    {
        s << '(' << e.w << ',' << e.v << ')';
        return s;
    }

    E w;  // weight

    node_ptr v;
};

}  // namespace freddy::detail
