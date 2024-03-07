#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"       // comp
#include "eddy/config.hpp"  // config::ut_size

#include <cassert>        // assert
#include <cstdint>        // std::int32_t
#include <functional>     // std::hash
#include <memory>         // std::shared_ptr
#include <ostream>        // std::ostream
#include <string>         // std::string
#include <string_view>    // std::string_view
#include <unordered_set>  // std::unordered_set
#include <utility>        // std::move

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace eddy::detail
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

struct node;

// =====================================================================================================================
// Types
// =====================================================================================================================

enum class decomposition
{
    PD,  // positive Davio
    S    // Shannon
};

struct edge
{
    explicit edge(std::int32_t const w, std::shared_ptr<node> v = {}) :
            w{w},
            v{std::move(v)}
    {}

    auto operator()() const noexcept
    {
        return (std::hash<std::int32_t>()(w) ^ std::hash<std::shared_ptr<node>>()(v));
    }

    auto friend operator==(edge const& lhs, edge const& rhs) noexcept
    {
        return (lhs.w == rhs.w && lhs.v == rhs.v);
    }

    auto friend operator<<(std::ostream& s, edge const& e) -> std::ostream&
    {
        if (e.v)
        {
            s << '(' << e.w << ',' << e.v << ')';
        }
        else
        {  // constant
            s << e.w;
        }
        return s;
    }

    std::int32_t w;

    std::shared_ptr<node> v;
};

struct node
{
    node(std::int32_t const x, std::shared_ptr<edge> hi, std::shared_ptr<edge> lo) :
            x{x},
            hi{std::move(hi)},
            lo{std::move(lo)}
    {
        assert(x >= 0);
        assert(this->hi);
        assert(this->lo);
    }

    auto operator()() const noexcept
    {
        return (std::hash<std::int32_t>()(x) ^ std::hash<std::shared_ptr<edge>>()(hi) ^
                std::hash<std::shared_ptr<edge>>()(lo));
    }

    auto friend operator==(node const& lhs, node const& rhs) noexcept
    {
        return (lhs.x == rhs.x && lhs.hi == rhs.hi && lhs.lo == rhs.lo);
    }

    auto friend operator<<(std::ostream& s, node const& v) -> std::ostream&
    {
        s << '(' << v.x << ',' << v.hi << ',' << v.lo << ')';
        return s;
    }

    std::int32_t x;

    std::shared_ptr<edge> hi;

    std::shared_ptr<edge> lo;

    bool m{};  // marker indicating if the node was visited
};

// NOLINTBEGIN
struct variable
{
    variable(decomposition const t, std::string_view l) :
            t{t},
            l{l}
    {
        assert(!l.empty());

        nt.reserve(config::ut_size);
        et.reserve(config::ut_size);
    }

    auto friend operator<<(std::ostream& s, variable const& var) -> std::ostream&
    {
        s << var.l;

        s << "\nDT = ";
        switch (var.t)
        {
            case decomposition::PD: s << "pD"; break;
            case decomposition::S: s << 'S'; break;
            default: assert(false);
        }

        auto print = [&s](auto const& ut) {
            for (auto i = 0; i < static_cast<std::int32_t>(ut.bucket_count()); ++i)
            {
                if (ut.bucket_size(i) > 0)
                {
                    s << "| " << i << " | ";
                    for (auto it = ut.begin(i); it != ut.end(i); ++it)
                    {
                        s << *it << *(*it) << '[' << it->use_count() << "] ";
                    }
                    s << "|\n";
                }
            }
        };

        s << "\nNT =\n";
        print(var.nt);
        s << "#Nodes = " << var.nt.size();
        s << "\nOccupancy = " << var.nt.load_factor();

        s << "\nET =\n";
        print(var.et);
        s << "#Edges = " << var.et.size();
        s << "\nOccupancy = " << var.et.load_factor();

        return s;
    }

    decomposition t;

    std::string l;  // name

    std::unordered_set<std::shared_ptr<node>, hash, comp> nt;

    std::unordered_set<std::shared_ptr<edge>, hash, comp> et;
};
// NOLINTEND

}  // namespace eddy::detail
