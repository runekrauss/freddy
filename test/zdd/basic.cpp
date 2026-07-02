// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/zdd.hpp>  // zdd_manager

#include <sstream>  // std::ostringstream
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

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