#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"            // comp
#include "edge.hpp"              // edge
#include "freddy/config.hpp"     // config::ut_size
#include "freddy/expansion.hpp"  // expansion
#include "node.hpp"              // node

#include <cassert>        // assert
#include <cstdint>        // std::int32_t
#include <memory>         // std::shared_ptr
#include <ostream>        // std::ostream
#include <string>         // std::string
#include <string_view>    // std::string_view
#include <unordered_set>  // std::unordered_set

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>
struct variable
{
    variable(expansion const t, std::string_view l) :
            t{t},
            l{l}
    {
        assert(!l.empty());  // for presentation reasons

        et.reserve(config::ut_size);
        nt.reserve(config::ut_size);
    }

    auto friend operator<<(std::ostream& s, variable const& var) -> std::ostream&
    {
        s << var.l;

        s << "\nDT = ";
        switch (var.t)
        {
            case expansion::PD: s << "pD"; break;
            case expansion::S: s << 'S'; break;
            default: assert(false);
        }

        auto print = [&s](auto const& ut) {
            for (auto i = 0uz; i < ut.bucket_count(); ++i)
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

        s << "\nET:\n";
        print(var.et);
        s << "#Edges = " << var.et.size();
        s << "\nOccupancy = " << var.et.load_factor();

        s << "\nNT:\n";
        print(var.nt);
        s << "#Nodes = " << var.nt.size();
        s << "\nOccupancy = " << var.nt.load_factor();

        return s;
    }

    expansion t;

    std::string l;  // name

    std::unordered_set<std::shared_ptr<edge<E, V>>, hash, comp> et;

    std::unordered_set<std::shared_ptr<node<E, V>>, hash, comp> nt;
};

}  // namespace freddy::detail
