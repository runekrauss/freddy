#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"  // P1
#include "edge.hpp"    // edge

#include <cassert>     // assert
#include <cstdint>     // std::int32_t
#include <functional>  // std::hash
#include <memory>      // std::shared_ptr
#include <utility>     // std::move
#include <variant>     // std::variant

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
class node
{
  public:
    using edge_ptr = std::shared_ptr<edge<E, V>>;

    struct branch
    {
        std::int32_t x;  // variable index for decomposition during traversing

        edge_ptr hi;

        edge_ptr lo;
    };

    node(std::int32_t const x, edge_ptr hi, edge_ptr lo) :
            val{branch{x, std::move(hi), std::move(lo)}}
    {
        assert(br().x >= 0);
        assert(br().hi);
        assert(br().lo);
    }

    explicit node(V c) :
            // constant
            val{std::move(c)}
    {}

    auto operator()() const  // multiplicative hash whose primes have a LCM such that few collisions occur
    {
        // x is not part of the key since several UTs are provided
        return is_const() ? std::hash<V>()(c())  // hash can force an overflow to increase entropy
                          : std::hash<edge_ptr>()(br().hi) * P1 + std::hash<edge_ptr>()(br().lo) * P2;
    }

    auto friend operator==(node const& lhs, node const& rhs)
    {
        if (lhs.is_const() != rhs.is_const())
        {
            return false;
        }
        if (lhs.is_const())
        {
            return lhs.c() == rhs.c();
        }
        return lhs.br().x == rhs.br().x && lhs.br().hi == rhs.br().hi && lhs.br().lo == rhs.br().lo;
    }

    [[nodiscard]] auto is_const() const -> bool
    {
        return std::get_if<V>(&val);
    }

    auto br() -> branch&
    {
        assert(!is_const());

        return std::get<branch>(val);
    }

    [[nodiscard]] auto br() const -> branch const&
    {
        assert(!is_const());

        return std::get<branch>(val);
    }

    auto c() -> V&
    {
        assert(is_const());

        return std::get<V>(val);
    }

    [[nodiscard]] auto c() const -> V const&
    {
        assert(is_const());

        return std::get<V>(val);
    }

  private:
    std::variant<branch, V> val;  // inode or leaf
};

}  // namespace freddy::detail
