#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"  // P1

#include <cassert>     // assert
#include <functional>  // std::hash
#include <memory>      // std::shared_ptr
#include <utility>     // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename, typename>
class node;

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
    {  // considering large primes typically results in few hash ranges with an accumulation of similarities
        return std::hash<E>()(w) * P1 + std::hash<node_ptr>()(v) * P2;
    }

    auto friend operator==(edge const& lhs, edge const& rhs)
    {
        return lhs.w == rhs.w && lhs.v == rhs.v;
    }

    E w;  // weight

    node_ptr v;
};

}  // namespace freddy::detail
