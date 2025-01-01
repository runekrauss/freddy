#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"            // comp
#include "edge.hpp"              // edge
#include "freddy/config.hpp"     // config::ut_size
#include "freddy/expansion.hpp"  // expansion
#include "node.hpp"              // node

#include <boost/unordered/unordered_flat_set.hpp>  // boost::unordered_flat_set

#include <cassert>      // assert
#include <memory>       // std::shared_ptr
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view

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

        s << "\nET:";
        s << "\n#Buckets = " << var.et.bucket_count();
        s << "\n#Edges = " << var.et.size();
        s << "\nMax. load = " << var.et.max_load();

        s << "\nNT:";
        s << "\n#Buckets = " << var.nt.bucket_count();
        s << "\n#Nodes = " << var.nt.size();
        s << "\nMax. load = " << var.nt.max_load();

        return s;
    }

    expansion t;

    std::string l;  // name

    boost::unordered_flat_set<std::shared_ptr<edge<E, V>>, hash, comp> et;

    boost::unordered_flat_set<std::shared_ptr<node<E, V>>, hash, comp> nt;
};

}  // namespace freddy::detail
