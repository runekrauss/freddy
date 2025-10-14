// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // var_index
#include <freddy/dd/bhd.hpp>  // bhd_heuristic::LEVEL

#include <boost/unordered/unordered_flat_map.hpp>  // boost::unordered_flat_map

#include <algorithm>  // std::ranges::max_element
#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <cstdint>    // std::int32_t
#include <istream>    // std::istream
#include <iterator>   // std::back_inserter
#include <limits>     // std::numeric_limits
#include <optional>   // std::optional
#include <ranges>     // std::ranges::set_intersection
#include <set>        // std::set
#include <sstream>    // std::stringstream
#include <string>     // std::string
#include <utility>    // std::pair
#include <vector>     // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{

// =====================================================================================================================
// Types
// =====================================================================================================================

class sat
{
  public:
    explicit sat(std::istream& dimacs)
    {
        while (true)
        {  // process comments and DIMACS CNF header
            std::string str;
            char c{};
            dimacs >> c;
            if (c == 'c')  // comment
            {
                std::getline(dimacs, str);  // skip
            }
            else  // p
            {
                dimacs >> str;  // cnf
                break;
            }
        }

        // variables
        std::size_t var_count{};
        dimacs >> var_count;
        p.vars.resize(var_count);
        p.lits.resize(var_count, 0);
        p.pols.resize(var_count, 0);

        // clauses
        std::size_t clause_count{};
        dimacs >> clause_count;
        p.clauses.resize(clause_count);
        for (auto& clause : p.clauses)
        {
            while (true)
            {
                std::int32_t lit{};
                dimacs >> lit;
                if (lit < 0)  // negative polarity
                {
                    assert(std::cmp_less_equal(-lit, p.vars.size()));

                    auto const x = -lit - 1;
                    ++p.lits[x];
                    --p.pols[x];
                    clause.push_back(2 * x);
                }
                else if (lit > 0)  // positive polarity
                {
                    assert(std::cmp_less_equal(lit, p.vars.size()));

                    auto const x = lit - 1;
                    ++p.lits[x];
                    ++p.pols[x];
                    clause.push_back(2 * x + 1);
                }
                else  // EOL
                {
                    break;  // next clause
                }
            }
        }
    }

    auto solve()
    {
        std::vector<bool> sol;

        if (dpll(p) == stat::KN)
        {
            assert(p.clauses.empty());

            sol.resize(p.vars.size());

            for (decltype(p.vars.size()) i = 0; i < p.vars.size(); ++i)
            {
                if (p.vars[i].has_value())  // variables that are not set can take any truth value
                {
                    sol[i] = *p.vars[i];
                }
            }
        }

        return sol;  // empty if p is UNSAT
    }

  private:
    enum struct stat : std::uint8_t
    {
        SAT,  // satisfiable
        UNSAT,
        KN,
        UNKN  // unknown
    };

    struct formula
    {
        std::vector<std::optional<bool>> vars;  // variables are assumed to be zero-indexed

        std::vector<std::int32_t> lits;  // #literals: -1 and 1 refer to 0, -2 and 2 mean 1, etc.

        std::vector<std::int32_t> pols;  // difference in the number of occurrences w.r.t. polarity

        std::vector<std::vector<std::int32_t>> clauses;  // 2x is stored if x is negative, otherwise 2x+1
    };

    static auto simplify(formula& q, std::int32_t const x)
    {
        assert(std::cmp_less(x, q.lits.size()));

        q.lits[x] = 0;  // as all these literals are removed

        for (auto i = 0; std::cmp_less(i, q.clauses.size()); ++i)
        {
            for (auto j = 0; std::cmp_less(j, q.clauses[i].size()); ++j)
            {
                if (q.clauses[i][j] == 2 * x + static_cast<std::int32_t>(*q.vars[x]))
                {  // same polarity => remove clause
                    q.clauses.erase(q.clauses.begin() + i--);

                    if (q.clauses.empty())
                    {
                        return stat::SAT;
                    }

                    break;
                }
                if (q.clauses[i][j] / 2 == x)
                {  // opposite polarity => remove literal
                    q.clauses[i].erase(q.clauses[i].begin() + j--);

                    if (q.clauses[i].empty())
                    {
                        return stat::UNSAT;  // formula is currently unsatisfiable
                    }

                    break;
                }
            }
        }

        return stat::UNKN;
    }

    static auto up(formula& q)
    {
        if (q.clauses.empty())
        {
            return stat::SAT;
        }

        auto uclause_exists = false;
        do
        {  // apply unit resolution successively
            uclause_exists = false;
            for (auto const& clause : q.clauses)
            {
                if (clause.size() == 1)
                {
                    uclause_exists = true;

                    auto const x = clause[0] / 2;
                    q.vars[x] = clause[0] % 2;  // true if positive, false otherwise

                    auto const r = simplify(q, x);
                    if (r != stat::UNKN)
                    {
                        return r;  // SAT or UNSAT
                    }

                    break;  // since formula simplification may affect previous clauses
                }
            }
        } while (uclause_exists);

        return stat::UNKN;
    }

    auto dpll(formula q) -> stat
    {
        // unit propagation
        auto r = up(q);
        if (r == stat::SAT)
        {
            p = q;
            return stat::KN;
        }
        if (r == stat::UNSAT)
        {
            return r;
        }

        auto const x = std::distance(q.lits.begin(), std::ranges::max_element(q.lits));

        assert(q.lits[x] > 0);

        // conditioning
        for (auto a = 0; a < 2; ++a)
        {
            auto u = q;
            u.vars[x] = u.pols[x] < 0 ? a : (a + 1) % 2;

            r = simplify(u, static_cast<std::int32_t>(x));  // by implication
            if (r == stat::SAT)
            {
                p = u;
                return stat::KN;
            }
            if (r == stat::UNSAT)
            {
                continue;  // backtracking
            }

            if (dpll(u) == stat::KN)  // branching
            {
                return stat::KN;
            }
        }

        return stat::UNSAT;
    }

    formula p;  // CNF instance
};

// =====================================================================================================================
// Functions
// =====================================================================================================================

auto mux_mgr()
{
    bhd_manager mgr{bhd_heuristic::LEVEL, 2, {.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    mgr.var("s");
    mgr.var("a");
    mgr.var("b");
    return mgr;
}

auto mux_sat(std::vector<std::vector<std::pair<std::int32_t, bool>>> const& cnf,
             std::vector<std::vector<std::pair<var_index, bool>>> const& ucs)
{
    std::vector<std::vector<bool>> sols;
    sols.reserve(ucs.size());

    for (auto const& exp_path : ucs)
    {
        // prepare DIMACS CNF
        std::stringstream dimacs;
        dimacs << "c MUX formula\n";
        dimacs << "p cnf 3 " << cnf.size() + exp_path.size() << '\n';  // header
        for (auto const& clause : cnf)
        {
            for (auto const& [x, pol] : clause)
            {
                dimacs << (pol ? x + 1 : -x - 1) << ' ';
            }
            dimacs << 0 << '\n';
        }
        for (auto const& [x, pol] : exp_path)  // unit clauses
        {
            assert(x < static_cast<var_index>(std::numeric_limits<std::int32_t>::max()));

            dimacs << (pol ? x + 1 : -x - 1) << ' ' << 0 << '\n';
        }

        // solve DIMACS CNF
        sat p{dimacs};
        auto sol = p.solve();
        if (!sol.empty())
        {
            sols.push_back(std::move(sol));
        }
    }

    return sols;
}

auto mux_sim(std::vector<std::vector<bool>> const& patterns)  // stuck-at fault simulation
{
    assert(!patterns.empty());

    // sab pattern -> detectable faults
    static boost::unordered_flat_map<std::vector<bool>, std::set<std::string>> const det_faults{
        {{false, false, false}, {"b/1", "f/1"}},       {{false, false, true}, {"s/1", "b/0", "f/0"}},
        {{false, true, false}, {"s/1", "b/1", "f/1"}}, {{false, true, true}, {"b/0", "f/0"}},
        {{true, false, false}, {"a/1", "f/1"}},        {{true, false, true}, {"s/0", "a/1", "f/1"}},
        {{true, true, false}, {"s/0", "a/0", "f/0"}},  {{true, true, true}, {"a/0", "f/0"}}};

    // fault localization
    std::set<std::string> faults{det_faults.at(patterns[0])};
    for (auto i = 1uz; i < patterns.size(); ++i)
    {
        std::set<std::string> tmp;
        std::ranges::set_intersection(faults, det_faults.at(patterns[i]), std::inserter(tmp, tmp.begin()));
        faults.swap(tmp);  // as intersection does not work in-place

        if (faults.size() == 1)
        {
            break;
        }
    }
    return faults;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("MUX f/0 is debugged", "[example]")
{
    auto mgr = mux_mgr();
    auto const diff = mgr.zero() ^ (mgr.var(0) & mgr.var(1) | ~mgr.var(0) & mgr.var(2));  // miter

    // test patterns
    auto patterns = diff.sat_solutions();                                                                  // BHD
    auto more_patterns = mux_sat({{{0, false}, {1, true}}, {{0, true}, {2, true}}}, diff.unit_clauses());  // SAT solver
    std::ranges::move(more_patterns, std::back_inserter(patterns));

    auto const faults = mux_sim(patterns);  // fault location

    CHECK(patterns.size() == 2);
    CHECK(faults.size() == 1);
    CHECK(faults.contains("f/0"));
}

TEST_CASE("MUX f/1 is debugged", "[example]")
{
    auto mgr = mux_mgr();
    auto const diff = mgr.one() ^ (mgr.var(0) & mgr.var(1) | ~mgr.var(0) & mgr.var(2));

    auto patterns = diff.sat_solutions();
    auto more_patterns = mux_sat({{{0, false}, {1, false}}, {{0, true}, {2, false}}}, diff.unit_clauses());
    std::ranges::move(more_patterns, std::back_inserter(patterns));

    auto const faults = mux_sim(patterns);

    CHECK(patterns.size() == 2);
    CHECK(faults.size() == 1);
    CHECK(faults.contains("f/1"));
}
