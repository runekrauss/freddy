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
        assert(op == operation::SAT);
        assert(this->f);
    }

    entry(operation const op, edge_ptr f, bool const a) :
            op{op},
            f{std::move(f)},
            a{a}
    {
        assert(op == operation::MISC);
        assert(this->f);
    }

    entry(operation const op, edge_ptr f, edge_ptr g) :
            op{op},
            f{std::move(f)},
            g{std::move(g)}
    {
        assert(op == operation::ADD || op == operation::AND || op == operation::MUL || op == operation::XOR);
        assert(this->f);
        assert(this->g);
    }

    entry(operation const op, edge_ptr f, edge_ptr g, edge_ptr h) :
            op{op},
            f{std::move(f)},
            g{std::move(g)},
            h{std::move(h)}
    {
        assert(op == operation::ITE);
        assert(this->f);
        assert(this->g);
        assert(this->h);
    }

    entry(operation const op, edge_ptr f, std::int32_t const x, edge_ptr g) :
            op{op},
            f{std::move(f)},
            g{std::move(g)},
            x{x}
    {
        assert(op == operation::COMPOSE);
        assert(this->f);
        assert(x >= 0);
        assert(this->g);
    }

    entry(operation const op, edge_ptr f, std::int32_t const x, bool const a) :
            op{op},
            f{std::move(f)},
            x{x},
            a{a}
    {
        assert(op == operation::RESTR);
        assert(this->f);
        assert(x >= 0);
    }

    auto friend operator==(entry const& lhs, entry const& rhs)
    {
        if (lhs.op != rhs.op)
        {
            return false;
        }

        bool r{};
        switch (lhs.op)
        {
            case operation::ADD:  // commutativity check
            case operation::AND:
            case operation::MUL:
            case operation::XOR: r = (lhs.f == rhs.f && lhs.g == rhs.g) || (lhs.f == rhs.g && lhs.g == rhs.f); break;
            default:
                r = lhs.op == rhs.op && lhs.f == rhs.f && lhs.g == rhs.g && lhs.h == rhs.h && lhs.x == rhs.x &&
                    lhs.a == rhs.a;
        }
        return r;
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
