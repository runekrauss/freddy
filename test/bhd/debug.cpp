// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // var_index
#include <freddy/dd/bhd.hpp>  // bhd_heuristic::LEVEL

#include <algorithm>  // std::ranges::set_intersection
#include <cassert>    // assert
#include <cstddef>    // std::size_t
#include <cstdint>    // std::int32_t
#include <istream>    // std::istream
#include <iterator>   // std::back_inserter
#ifndef NDEBUG
#include <limits>  // std::numeric_limits
#endif
#include <optional>       // std::optional
#include <set>            // std::set
#include <sstream>        // std::stringstream
#include <string>         // std::getline
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::cmp_less_equal
#include <vector>         // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{

// =====================================================================================================================
// Types
// =====================================================================================================================

class sat final
{
  public:
    explicit sat(std::istream& dimacs)
    {
        char c{};
        while (dimacs >> c)
        {  // read DIMACS comment(s) and header
            std::string str;

            if (c == 'c')  // comment
            {
                std::getline(dimacs, str);  // skip
            }
            else
            {
                assert(c == 'p');

                dimacs >> str;

                assert(str == "cnf");

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
            std::int32_t lit{};
            dimacs >> lit;
            while (lit != 0)
            {
                if (lit < 0)
                {
                    assert(std::cmp_less_equal(-lit, p.vars.size()));

                    auto const x = -lit - 1;
                    ++p.lits[x];
                    --p.pols[x];
                    clause.push_back(2 * x);
                }
                else  // positive polarity
                {
                    assert(std::cmp_less_equal(lit, p.vars.size()));

                    auto const x = lit - 1;
                    ++p.lits[x];
                    ++p.pols[x];
                    clause.push_back(2 * x + 1);
                }

                dimacs >> lit;  // next literal
            }
        }
    }

    auto solve()
    {
        std::vector<bool> sol;

        if (dpll(p) == status::SAT)
        {
            assert(p.clauses.empty());

            sol.resize(p.vars.size());

            for (auto i = 0uz; i < p.vars.size(); ++i)
            {
                if (p.vars[i])  // variables that are not set can take any truth value
                {
                    sol[i] = *p.vars[i];
                }
            }
        }

        return sol;
    }

  private:
    enum struct status : std::uint8_t
    {
        SAT,  // satisfiable
        UNSAT,
        UNKNOWN
    };

    struct cnf
    {
        std::vector<std::optional<bool>> vars;  // variables are assumed to be zero-indexed

        std::vector<std::uint32_t> lits;  // #Literals: -1 and 1 refer to 0, -2 and 2 mean 1, etc.

        std::vector<std::int32_t> pols;  // difference in the number of occurrences w.r.t. polarity

        std::vector<std::vector<std::uint32_t>> clauses;  // 2x is stored if x is negative, otherwise 2x+1
    };

    static auto simplify(cnf& q, std::uint32_t const x) noexcept
    {
        q.lits[x] = 0;  // as all these literals are removed

        for (auto i = 0; std::cmp_less(i, q.clauses.size()); ++i)
        {
            for (auto j = 0; std::cmp_less(j, q.clauses[i].size()); ++j)
            {
                if (q.clauses[i][j] == 2 * x + static_cast<std::uint32_t>(*q.vars[x]))
                {  // same polarity => remove clause
                    q.clauses.erase(q.clauses.begin() + i--);

                    if (q.clauses.empty())
                    {
                        return status::SAT;
                    }

                    break;
                }
                if (q.clauses[i][j] / 2 == x)
                {  // opposite polarity => remove literal
                    q.clauses[i].erase(q.clauses[i].begin() + j--);

                    if (q.clauses[i].empty())
                    {
                        return status::UNSAT;
                    }

                    break;
                }
            }
        }

        return status::UNKNOWN;
    }

    static auto up(cnf& q) noexcept
    {
        if (q.clauses.empty())
        {
            return status::SAT;
        }

        auto uc_exists = false;  // unit clause
        do
        {  // apply unit resolution successively
            uc_exists = false;
            for (auto const& clause : q.clauses)
            {
                if (clause.size() == 1)
                {
                    uc_exists = true;

                    auto const x = clause[0] / 2;
                    q.vars[x] = clause[0] % 2;  // true if positive, false otherwise

                    auto const res = simplify(q, x);
                    if (res != status::UNKNOWN)
                    {
                        return res;  // SAT or UNSAT
                    }

                    break;  // as formula simplification may affect previous clauses
                }
            }
        } while (uc_exists);

        return status::UNKNOWN;
    }

    auto dpll(cnf q) -> status
    {
        // unit propagation
        auto res = up(q);
        if (res == status::SAT)
        {
            p = q;
            return res;
        }
        if (res == status::UNSAT)
        {
            return res;
        }

        // conditioning
        auto const x = static_cast<std::uint32_t>(std::distance(q.lits.begin(), std::ranges::max_element(q.lits)));
        for (auto const a : {false, true})
        {
            auto r = q;
            r.vars[x] = r.pols[x] < 0 ? a : !a;

            res = simplify(r, x);  // by implication
            if (res == status::SAT)
            {
                p = r;
                return res;
            }
            if (res == status::UNSAT)
            {
                continue;  // backtracking
            }

            if (dpll(r) == status::SAT)  // branching
            {
                return status::SAT;
            }
        }

        return status::UNSAT;
    }

    cnf p;  // problem instance
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

auto mux_sat(std::vector<std::vector<std::int32_t>> const& clauses,  // base
             std::vector<std::vector<std::pair<var_index, bool>>> const& ucs)
{
    std::vector<std::vector<bool>> sols;
    sols.reserve(ucs.size());

    for (auto const& exp_path : ucs)
    {
        // create DIMACS CNF
        std::stringstream dimacs;
        dimacs << "c MUX formula\n";
        dimacs << "p cnf 3 " << clauses.size() + exp_path.size() << '\n';  // header
        for (auto const& clause : clauses)
        {
            for (auto const lit : clause)
            {
                dimacs << lit << ' ';
            }
            dimacs << 0 << '\n';
        }
        for (auto const& [x, pol] : exp_path)  // unit clauses
        {
            assert(x < static_cast<var_index>(std::numeric_limits<std::int32_t>::max()));

            auto const var = static_cast<std::int32_t>(x);
            dimacs << (pol ? var + 1 : -var - 1) << ' ' << 0 << '\n';
        }

        // solve DIMACS CNF
        sat cnf{dimacs};
        auto sol = cnf.solve();
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
    static std::unordered_map<std::vector<bool>, std::set<std::string>> const det_faults{
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
    auto const diff = mgr.zero() ^ ((mgr.var(0) & mgr.var(1)) | (~mgr.var(0) & mgr.var(2)));  // miter

    // test patterns
    auto patterns = diff.sat_solutions();                                  // BHD
    auto more_patterns = mux_sat({{-1, 2}, {1, 3}}, diff.unit_clauses());  // SAT solver
    std::ranges::move(more_patterns, std::back_inserter(patterns));

    auto const faults = mux_sim(patterns);  // fault location

    CHECK(patterns.size() == 2);
    CHECK(faults.size() == 1);
    CHECK(faults.contains("f/0"));
}

TEST_CASE("MUX f/1 is debugged", "[example]")
{
    auto mgr = mux_mgr();
    auto const diff = mgr.one() ^ ((mgr.var(0) & mgr.var(1)) | (~mgr.var(0) & mgr.var(2)));

    auto patterns = diff.sat_solutions();
    auto more_patterns = mux_sat({{-1, -2}, {1, -3}}, diff.unit_clauses());
    std::ranges::move(more_patterns, std::back_inserter(patterns));

    auto const faults = mux_sim(patterns);

    CHECK(patterns.size() == 2);
    CHECK(faults.size() == 1);
    CHECK(faults.contains("f/1"));
}
