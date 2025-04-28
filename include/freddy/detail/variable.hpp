#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"            // comp
#include "edge.hpp"              // edge
#include "freddy/config.hpp"     // config::ut_size
#include "freddy/expansion.hpp"  // expansion
#include "node.hpp"              // node

#include <boost/unordered/unordered_flat_set.hpp>  // BOOST_UNORDERED_ENABLE_STATS

#include <cassert>      // assert
#include <format>       // std::format
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

        et.reserve(config::ut_size);  // rehash(ceil(config::ut_size / 0.875))
        nt.reserve(config::ut_size);
    }

    auto friend operator<<(std::ostream& s, variable const& var) -> std::ostream&
    {
        // table head
        s << "Variable '" << var.l << "' [" << to_string(var.t) << "]\n";
        s << std::format("{:-<61}\n", '-');

        // body content
        auto print = [&s](auto const& ut, auto prefix) {
            s << std::format("{:2} {:36} | {:19}\n", prefix, "#Buckets", ut.bucket_count());
            s << std::format("{:2} {:36} | {:19}\n", prefix, "#Elements", ut.size());
            s << std::format("{:2} {:36} | {:19}", prefix, "Max. load", ut.max_load());
#ifdef BOOST_UNORDERED_ENABLE_STATS  // calculate statistics to determine the quality of hash functions
            auto const& stats = ut.get_stats();
            if (stats.insertion.count != 0)
            {  // operation was performed at least once
                // average number of bucket groups accessed during insertion
                s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Insertion probe length",
                                 stats.insertion.probe_length.average);  // should be close to 1.0
            }
            if (stats.successful_lookup.count != 0)
            {
                s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Successful lookup probe length",
                                 stats.successful_lookup.probe_length.average);
                // average number of nodes/edges compared during lookup
                s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Successful lookup comparison count",
                                 stats.successful_lookup.num_comparisons.average);
            }
            if (stats.unsuccessful_lookup.count != 0)
            {
                s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Unsuccessful lookup probe length",
                                 stats.unsuccessful_lookup.probe_length.average);
                s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Unsuccessful lookup comparison count",
                                 stats.unsuccessful_lookup.num_comparisons.average);  // should be close to 0.0
            }
#endif
        };
        print(var.et, "ET");
        s << '\n';
        print(var.nt, "NT");

        return s;
    }

    expansion t;

    std::string l;  // name

    boost::unordered_flat_set<std::shared_ptr<edge<E, V>>, hash, comp> et;

    boost::unordered_flat_set<std::shared_ptr<node<E, V>>, hash, comp> nt;
};

}  // namespace freddy::detail
