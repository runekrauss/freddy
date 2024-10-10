// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bhd.hpp>  // dd::bhd_heuristic::LVL

#include <algorithm>      // std::ranges::max_element
#include <cassert>        // assert
#include <cstddef>        // std::size_t
#include <cstdint>        // std::int32_t
#include <ios>            // std::ios_base::app
#include <istream>        // std::istream
#include <iterator>       // std::distance
#include <optional>       // std::optional
#include <set>            // std::set
#include <sstream>        // std::stringstream
#include <string>         // std::to_string
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::pair
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
                    assert(-lit <= static_cast<std::int32_t>(p.vars.size()));

                    auto const x = -lit - 1;
                    ++p.lits[x];
                    --p.pols[x];
                    clause.push_back(2 * x);
                }
                else if (lit > 0)  // positive polarity
                {
                    assert(lit <= static_cast<std::int32_t>(p.vars.size()));

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

            for (auto i = 0uz; i < p.vars.size(); ++i)
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

    auto static simplify(formula& q, std::int32_t const x)
    {
        assert(x < static_cast<std::int32_t>(q.lits.size()));

        q.lits[x] = 0;  // as all these literals are removed

        for (auto i = 0; i < static_cast<std::int32_t>(q.clauses.size()); ++i)
        {
            for (auto j = 0; j < static_cast<std::int32_t>(q.clauses[i].size()); ++j)
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

    auto static up(formula& q)
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
    dd::bhd_manager mgr{dd::bhd_heuristic::LVL, 2};
    mgr.var("s");
    mgr.var("a");
    mgr.var("b");
    return mgr;
}

auto mux_sat(std::vector<std::vector<std::pair<std::int32_t, bool>>> const& cnf,
             std::vector<std::vector<std::pair<std::int32_t, bool>>> const& uclauses)
{
    std::vector<std::vector<bool>> sols;

    for (auto const& exp : uclauses)
    {
        // prepare DIMACS CNF instance
        std::stringstream dimacs{"c MUX formula\np cnf 3 " + std::to_string(cnf.size() + exp.size()) + '\n',
                                 std::ios_base::app | std::ios_base::in | std::ios_base::out};  // header
        for (auto const& clause : cnf)
        {
            for (auto const& [x, a] : clause)  // base clause
            {
                dimacs << (a ? x + 1 : -x - 1) << ' ';
            }
            dimacs << 0 << '\n';
        }
        for (auto const& [x, a] : exp)  // unit clauses
        {
            dimacs << (a ? x + 1 : -x - 1) << ' ' << 0 << '\n';
        }

        // solve DIMACS CNF instance
        sat p{dimacs};
        auto sol = p.solve();
        if (!sol.empty())
        {
            sols.push_back(std::move(sol));
        }
    }

    return sols;
}

auto mux_sim(std::vector<std::vector<bool>> const& t)  // stuck-at fault simulation
{
    assert(!t.empty());

    // sab pattern -> detectable faults
    std::unordered_map<std::vector<bool>, std::set<std::string>> static const t2f{
        {{false, false, false}, {"b/1", "f/1"}},       {{false, false, true}, {"s/1", "b/0", "f/0"}},
        {{false, true, false}, {"s/1", "b/1", "f/1"}}, {{false, true, true}, {"b/0", "f/0"}},
        {{true, false, false}, {"a/1", "f/1"}},        {{true, false, true}, {"s/0", "a/1", "f/1"}},
        {{true, true, false}, {"s/0", "a/0", "f/0"}},  {{true, true, true}, {"a/0", "f/0"}}};

    // fault localization
    std::set<std::string> f{t2f.at(t[0])};
    for (auto i = 1uz; i < t.size(); ++i)
    {
        std::set<std::string> tmp;
        std::set_intersection(f.begin(), f.end(), t2f.at(t[i]).begin(), t2f.at(t[i]).end(),
                              std::inserter(tmp, tmp.begin()));
        f.swap(tmp);  // since intersection does not work in-place

        if (f.size() == 1)
        {
            break;
        }
    }
    return f;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("MUX f/0 is debugged", "[debug]")
{
    auto mgr = mux_mgr();
    auto const m = mgr.zero() ^ (mgr.var(0) & mgr.var(1) | ~mgr.var(0) & mgr.var(2));  // miter

    // test patterns
    auto t = m.sat();                                                                    // BHD
    t.append_range(mux_sat({{{0, false}, {1, true}}, {{0, true}, {2, true}}}, m.uc()));  // SAT solver

    auto const f = mux_sim(t);  // fault location

    CHECK(t.size() == 2);
    CHECK(f.size() == 1);
    CHECK(f.find("f/0") != f.end());
}

TEST_CASE("MUX f/1 is debugged", "[debug]")
{
    auto mgr = mux_mgr();
    auto const m = mgr.one() ^ (mgr.var(0) & mgr.var(1) | ~mgr.var(0) & mgr.var(2));

    auto t = m.sat();
    t.append_range(mux_sat({{{0, false}, {1, false}}, {{0, true}, {2, false}}}, m.uc()));

    auto const f = mux_sim(t);

    CHECK(t.size() == 2);
    CHECK(f.size() == 1);
    CHECK(f.find("f/1") != f.end());
}
