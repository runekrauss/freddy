#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "edge.hpp"              // edge
#include "freddy/operation.hpp"  // operation

#include <cassert>      // assert
#include <cstddef>      // std::size_t
#include <cstdint>      // std::int32_t
#include <functional>   // std::hash
#include <optional>     // std::optional
#include <ostream>      // std::ostream
#include <type_traits>  // std::underlying_type
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
struct entry  // for caching
{
    using edge_ptr = std::shared_ptr<edge<E, V>>;

    struct hash
    {
        auto operator()(entry const& o) const -> std::size_t
        {
            return (std::hash<std::int32_t>()(static_cast<std::underlying_type<operation>::type>(o.op)) ^
                    std::hash<edge_ptr>()(o.f) ^ std::hash<edge_ptr>()(o.g) ^ std::hash<edge_ptr>()(o.h) ^
                    std::hash<std::int32_t>()(o.x) ^ std::hash<std::optional<bool>>()(o.a));
        }
    };

    entry(operation const op, edge_ptr f) :
            op{op},
            f{std::move(f)}
    {
        assert(this->f);
    }

    entry(operation const op, edge_ptr f, edge_ptr g) :
            entry(op, std::move(f))
    {
        assert(g);

        this->g = std::move(g);
    }

    entry(operation const op, edge_ptr f, edge_ptr g, edge_ptr h) :
            entry(op, std::move(f), std::move(g))
    {
        assert(h);

        this->h = std::move(h);
    }

    entry(operation const op, edge_ptr f, std::int32_t const x, edge_ptr g) :
            entry(op, std::move(f), std::move(g))
    {
        assert(x >= 0);

        this->x = x;
    }

    entry(operation const op, edge_ptr f, std::int32_t const x, bool const a) :
            entry(op, std::move(f))
    {
        assert(x >= 0);

        this->x = x;
        this->a = a;
    }

    auto friend operator==(entry const& lhs, entry const& rhs)
    {
        return (lhs.op == rhs.op && lhs.f == rhs.f && lhs.g == rhs.g && lhs.h == rhs.h && lhs.x == rhs.x &&
                lhs.a == rhs.a);
    }

    auto friend operator<<(std::ostream& s, entry const& o) -> std::ostream&
    {
        s << '{' << static_cast<std::int32_t>(o.op) << ',' << o.f;

        if (o.x != -1)
        {
            s << ',' << o.x;
        }
        if (o.g)
        {
            s << ',' << o.g;
        }
        if (o.h)
        {
            s << ',' << o.h;
        }
        if (o.a.has_value())
        {
            s << ',' << *o.a;
        }

        s << '}';

        return s;
    }

    operation op{};

    edge_ptr f;

    edge_ptr g;

    edge_ptr h;  // for operations such as ITE

    std::int32_t x{-1};  // for substitution

    std::optional<bool> a;  // for restriction
};

}  // namespace freddy::detail
