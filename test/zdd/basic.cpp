// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/bdd.hpp>  // bdd_manager
#include <freddy/dd/zdd.hpp>  // zdd_manager

#include <cstdint>  // std::uint64_t
#include <sstream>  // std::ostringstream
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Helpers
// *********************************************************************************************************************

namespace
{

auto all_assignments(std::size_t n) -> std::vector<std::vector<bool>>
{
    std::vector<std::vector<bool>> result;
    result.reserve(1uz << n);
    for (std::uint64_t mask = 0; mask < (1ull << n); ++mask)
    {
        std::vector<bool> as(n);
        for (std::size_t i = 0; i < n; ++i)
            as[i] = ((mask >> i) & 1u) != 0;
        result.push_back(std::move(as));
    }
    return result;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("ZDD is constructed", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const x0 = mgr.var(), x1 = mgr.var();

    SECTION("Variables are zero-suppressed")
    {
        CHECK_FALSE(x0.is_const());
        CHECK(x0.var() == 0);
        CHECK(x0.high().is_one());
        CHECK(x0.low().is_zero());
    }

    SECTION("Terminals are constants")
    {
        CHECK(mgr.zero().is_const());
        CHECK(mgr.zero().is_zero());
        CHECK(mgr.one().is_const());
        CHECK(mgr.one().is_one());
    }

    SECTION("Combination by conjunction")
    {
        auto const f = x0 & x1;

        CHECK_FALSE(f.eval({false, false}));
        CHECK_FALSE(f.eval({false, true}));
        CHECK_FALSE(f.eval({true, false}));
        CHECK(f.eval({true, true}));
    }

    SECTION("Combination by disjunction")
    {
        auto const f = x0 | x1;

        CHECK_FALSE(f.eval({false, false}));
        CHECK(f.eval({false, true}));
        CHECK(f.eval({true, false}));
        CHECK(f.eval({true, true}));
    }

    SECTION("Negation is applied")
    {
        auto const f = ~x0;

        CHECK(f.high().is_const());
        CHECK(f.low().is_const());
        CHECK(f.restr(0, true).is_zero());
        CHECK(f.restr(0, false).is_one());
    }
}

TEST_CASE("ZDD can be characterized", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = x0 | x1 | x2;

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constants are supported")
    {
        CHECK(mgr.const_count() == 2);
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() >= 3);
    }

    SECTION("Size is computed")
    {
        CHECK(f.size() == 5);
    }

    SECTION("Depth is computed")
    {
        CHECK(f.depth() == 3);
    }

    SECTION("Number of paths is computed")
    {
        CHECK(f.path_count() == 3);
    }

    SECTION("Essential variables are identifiable")
    {
        mgr.var();

        CHECK(f.is_essential(0));
        CHECK(f.is_essential(1));
        CHECK(f.is_essential(2));
        CHECK_FALSE(f.is_essential(3));
    }
}

TEST_CASE("ZDD is substituted", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    (void)x2;
    auto const f = x0 & x1;

    SECTION("Variable is replaced by function")
    {
        auto const x3 = mgr.var(), x4 = mgr.var();
        auto const g = f.compose(1, x3 & x4);

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.high().var() == 3);
        CHECK(g.high().high().var() == 4);
    }

    SECTION("Variable is restricted to constant")
    {
        CHECK(f.restr(0, false).is_zero());
        CHECK(f.restr(0, true).is_essential(1));
    }

    SECTION("Variable is eliminated by existential quantification")
    {
        auto const g = f.exist(0);

        CHECK_FALSE(g.is_essential(0));
        CHECK(g == mgr.var(1));
    }

    SECTION("Variable is eliminated by universal quantification")
    {
        CHECK(f.forall(0).is_zero());
    }
}

TEST_CASE("ZDD variable order is changeable", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 4}};
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x0 = mgr.var("x0"), x2 = mgr.var("x2");
    auto const f = (x0 & x1) | (x2 & x3);

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK_FALSE(f.is_zero());
        CHECK(f.size() >= 1);
        CHECK(f.eval({true, true, true, false}));
        CHECK(f.eval({false, true, false, true}));
        CHECK_FALSE(f.eval({true, false, false, true}));
    }
}

TEST_CASE("ZDD zero-suppression subsumes redundant cubes", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var(), y = mgr.var(), z = mgr.var();

    // x*y*z is subsumed by x*y: adding x*y*z to a cover that already contains x*y
    // leaves the ZDD structurally unchanged — a property unique to zero-suppression
    auto const f = x & y;
    auto const g = f | (x & y & z);

    CHECK(f == g);
    CHECK(g.size() == f.size());
}

TEST_CASE("ZDD eval matches BDD eval", "[basic]")
{
    SECTION("Shannon (x0&x1)|(x2&x3)")
    {
        bdd_manager bmgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 4}};
        auto const bx0 = bmgr.var(), bx1 = bmgr.var(), bx2 = bmgr.var(), bx3 = bmgr.var();
        auto const f_bdd = (bx0 & bx1) | (bx2 & bx3);

        zdd_manager zmgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 4}};
        auto const x0 = zmgr.var(), x1 = zmgr.var(), x2 = zmgr.var(), x3 = zmgr.var();
        auto const f_zdd = (x0 & x1) | (x2 & x3);

        for (auto const& as : all_assignments(4))
        {
            CHECK(f_zdd.eval(as) == f_bdd.eval(as));
        }
    }

    SECTION("Shannon (x0|x1)&(x0|x2)")
    {
        bdd_manager bmgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
        auto const bx0 = bmgr.var(), bx1 = bmgr.var(), bx2 = bmgr.var();
        auto const f_bdd = (bx0 | bx1) & (bx0 | bx2);

        zdd_manager zmgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
        auto const x0 = zmgr.var(), x1 = zmgr.var(), x2 = zmgr.var();
        auto const f_zdd = (x0 | x1) & (x0 | x2);

        for (auto const& as : all_assignments(3))
        {
            CHECK(f_zdd.eval(as) == f_bdd.eval(as));
        }
    }
}

TEST_CASE("ZDD eval satisfies positive and negative Davio expansion identities", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 4}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var(), x3 = mgr.var();
    auto const f = (x0 & x1) | (x2 & x3);

    auto const f_0 = f.restr(0, false);  // f|_{x0=0} = x2&x3
    auto const f_1 = f.restr(0, true);   // f|_{x0=1} = x1|(x2&x3)

    for (auto const& as : all_assignments(4))
    {
        bool const val   = f.eval(as);
        bool const val_0 = f_0.eval(as);
        bool const val_1 = f_1.eval(as);

        // positive Davio: f = f_0 XOR (x0 AND (f_0 XOR f_1))
        CHECK(val == (val_0 ^ (as[0] && (val_0 ^ val_1))));

        // negative Davio: f = f_1 XOR (~x0 AND (f_0 XOR f_1))
        CHECK(val == (val_1 ^ (!as[0] && (val_0 ^ val_1))));
    }
}

TEST_CASE("ZDD De Morgan identities do not hold structurally", "[basic]")
{
    SECTION("~(x0|x1) == (~x0)&(~x1)")
    {
        zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
        auto const x0 = mgr.var("x0"), x1 = mgr.var("x1");
        (void)mgr.var("x2");

        auto const lhs = ~(x0 | x1);
        auto const rhs = (~x0) & (~x1);

        for (auto const& as : all_assignments(3))
        {
            CHECK(lhs.eval(as) == rhs.eval(as));
        }
    }

    SECTION("~(f&g) == ~f|~g, f=x0&x1, g=x1&x2")
    {
        zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
        auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();

        auto const f = x0 & x1;
        auto const g = x1 & x2;

        auto const lhs = ~(f & g);
        auto const rhs = (~f) | (~g);

        for (auto const& as : all_assignments(3))
        {
            CHECK(lhs.eval(as) == rhs.eval(as));
        }
    }
}

TEST_CASE("ZDD can be visualized", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const x0 = mgr.var(), x1 = mgr.var();
    auto const f = x0 | x1;

    std::ostringstream oss;
    f.dump_dot(oss);

    CHECK_FALSE(oss.str().empty());
    CHECK(oss.str().find("digraph") != std::string::npos);
}