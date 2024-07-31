// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bmd.hpp>  // dd::bmd_manager

#include <array>     // std::array
#include <iostream>  // std::cout
#include <vector>    // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Functions
// *********************************************************************************************************************

auto static bit_lvl_impl(dd::bmd_manager& mgr)
{
    config::dead_factor = 0.75f;
    config::growth_factor = 1.25f;
    config::load_factor = 0.7f;
    config::ct_size = 23;
    config::ut_size = 11;
    config::vl_size = 4;

    auto const a1 = mgr.var("a1");
    auto const b1 = mgr.var("b1");
    auto const a0 = mgr.var("a0");
    auto const b0 = mgr.var("b0");

    // symbolic simulation
    std::array<dd::bmd, 8> s;
    s[0] = a0 & b1;
    s[1] = a0 & b0;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[0] ^ s[2];
    s[5] = s[0] & s[2];
    s[6] = s[3] ^ s[5];
    s[7] = s[3] & s[5];

    return std::vector<dd::bmd>{s[1], s[4], s[6], s[7]};
}

auto static word_lvl_spec(dd::bmd_manager& mgr)
{
    config::dead_factor = 0.75f;
    config::growth_factor = 1.25f;
    config::load_factor = 0.7f;
    config::ct_size = 23;
    config::ut_size = 11;
    config::vl_size = 4;

    std::vector<dd::bmd> a(2);
    std::vector<dd::bmd> b(2);

    a[1] = (mgr.var_count() > 0) ? mgr.var(0) : mgr.var("a1");
    b[1] = (mgr.var_count() > 1) ? mgr.var(1) : mgr.var("b1");
    a[0] = (mgr.var_count() > 2) ? mgr.var(2) : mgr.var("a0");
    b[0] = (mgr.var_count() > 3) ? mgr.var(3) : mgr.var("b0");

    return (mgr.weighted_sum(a) * mgr.weighted_sum(b));
}

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("BMD synthesis is performed", "[mult]")
{
    dd::bmd_manager mgr;
    auto const f = word_lvl_spec(mgr);

    CHECK(f.weight() == 1);
    CHECK_FALSE(f.high().is_const());
    CHECK(f.low().low().low().is_zero());
    CHECK(f.high().low().high().is_one());
    CHECK(f.high().high().is_two());
    CHECK(f == ((mgr.two() * mgr.var(0) * (mgr.two() * mgr.var(1) + mgr.var(3))) +
                (mgr.var(2) * (mgr.two() * mgr.var(1) + mgr.var(3)))));
    CHECK(f.high() == mgr.two() * (mgr.two() * mgr.var(1) + mgr.var(3)));
    CHECK(f.low() == mgr.var(2) * (mgr.two() * mgr.var(1) + mgr.var(3)));
    CHECK_FALSE(f.cof(true, false) == f.cof(false, false, true));
    CHECK(f.cof(true, false).equals(f.cof(false, false, true)));
}

TEST_CASE("BMD can represent Boolean functions", "[mult]")
{
    dd::bmd_manager mgr;
    auto const p = bit_lvl_impl(mgr);
#ifndef NDEBUG
    std::cout << mgr << std::endl;
    for (auto const& g : p)
    {
        std::cout << g << std::endl;
    }
    mgr.print(p, {"p0", "p1", "p2", "p3"});
#endif
    CHECK(p[0].high() == mgr.var(3));
    CHECK(p[0].low().is_zero());
    CHECK(p[1].cof(true, true) == mgr.constant(-2) * (mgr.var(2) & mgr.var(3)));
    CHECK(p[1].cof(true, false) == mgr.var(3));
    CHECK(p[1].cof(false, true) == mgr.var(2));
    CHECK(p[2].cof(true, true) == -(mgr.var(2) * mgr.var(3)) + mgr.one());
    CHECK(p[3].high() == mgr.var(2) * mgr.var(1) * mgr.var(3));
    CHECK(p[3].low().is_zero());
    CHECK(p[3].ite(mgr.zero(), mgr.one()) == ~p[3]);
}

TEST_CASE("BMD character can be investigated", "[mult]")
{
    dd::bmd_manager mgr;
    auto const p = bit_lvl_impl(mgr);

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 4);
    }

    SECTION("Number of constants is determined")
    {
        CHECK(mgr.const_count() == 1);
    }

    SECTION("Number of nodes is determined")
    {
        CHECK(mgr.node_count() <= 19);
    }

    SECTION("Number of edges is determined")
    {
        CHECK(mgr.edge_count() <= 36);
    }

    SECTION("Nodes are counted")
    {
        CHECK(p[0].size() == 3);
        CHECK(p[1].size() == 7);
        CHECK(p[2].size() == 5);
        CHECK(p[3].size() == 5);
        CHECK(mgr.size(p) == 12);
    }

    SECTION("Longest path is computed")
    {
        CHECK(p[0].depth() == 2);
        CHECK(mgr.depth(p) == 4);
    }

    SECTION("Number of paths is computed")
    {
        CHECK(p[1].path_count() == 8);
    }

    SECTION("Assignments are evaluated")
    {
        CHECK(p[0].eval({true, false, true, true}) == 1);
        CHECK(p[0].eval({false, true, true, false}) == 0);
        CHECK(p[2].eval({true, true, true, true}) == 0);
        CHECK(p[2].eval({true, true, false, true}) == 1);
    }

    SECTION("Constant is found")
    {
        CHECK(p[0].has_const(1));
        CHECK_FALSE(p[0].has_const(2));
    }

    SECTION("Essential variables are identifiable")
    {
        CHECK_FALSE(p[0].is_essential(0));
        CHECK_FALSE(p[0].is_essential(1));
        CHECK(p[0].is_essential(2));
        CHECK(p[0].is_essential(3));
    }
}

TEST_CASE("BMD can interpret bits numerically", "[mult]")
{
    dd::bmd_manager mgr;
    auto const p = bit_lvl_impl(mgr);

    SECTION("Bits can be weighted")
    {
        CHECK(mgr.weighted_sum(p) == ((mgr.two() * mgr.var(0) * (mgr.two() * mgr.var(1) + mgr.var(3))) +
                                      (mgr.var(2) * (mgr.two() * mgr.var(1) + mgr.var(3)))));
    }

    SECTION("Two's complement is a sum of weighted bits")
    {
        CHECK(mgr.twos_complement(p).cof(true, true).var() == 2);
        CHECK(mgr.twos_complement(p).high().high().high().weight() == -4);
    }
}

TEST_CASE("BMD substitution can be made", "[mult]")
{
    dd::bmd_manager mgr;
    auto const f = word_lvl_spec(mgr);

    SECTION("Variable is replaced by function")
    {
        auto const x4 = mgr.var();
        auto const x5 = mgr.var();
        auto const g = f.compose(3, x4 * x5);

        CHECK_FALSE(g.is_essential(3));
        CHECK(g.cof(false, false, true) == x4 * x5);
        CHECK(g.eval({true, false, false, true, true, true}) == 2);
    }

    SECTION("Variable is restricted to constant value")
    {
        CHECK(f.restr(1, true).low().var() == 2);
        CHECK(f.restr(1, false).high().var() == 3);
    }

    SECTION("Variable is removed by existential quantification")
    {
        auto const g = f.exist(1);

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.high().var() == 2);
        CHECK(g.low().var() == 2);
    }

    SECTION("Variable is removed by universal quantification")
    {
        auto const g = f.forall(1);

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.high().var() == 2);
        CHECK(g.low().var() == 2);
    }
}

TEST_CASE("BMD variable order is changeable", "[mult]")
{
    dd::bmd_manager mgr;
    auto const f = word_lvl_spec(mgr);

    SECTION("Levels can be swapped")
    {
        mgr.swap(0, 3);
        mgr.swap(1, 2);

        CHECK(f.var() == 3);
        CHECK(f.high() == mgr.two() * mgr.var(0) + mgr.var(2));
        CHECK(f.low() == (mgr.two() * mgr.var(0) + mgr.var(2)) * (mgr.two() * mgr.var(1)));
        CHECK(f.eval({true, false, true, true}) == 3);
        CHECK(f.eval({false, true, true, false}) == 2);
        CHECK_FALSE(f.cof(true, false) == f.cof(false, false, true));
        CHECK(f.cof(true, false).equals(f.cof(false, false, true)));
    }

    SECTION("Variable reordering finds a minimum")
    {
        auto const ncnt_old = f.size();
        mgr.reorder();

        CHECK(ncnt_old > f.size());
        CHECK(f.var() == 0);
        CHECK(f.low().var() == 2);
        CHECK(f.cof(false, true).var() == 1);
        CHECK(f.cof(false, true, false).var() == 3);
    }
}

TEST_CASE("BMD equivalence is checked", "[mult]")
{
    dd::bmd_manager mgr;

    CHECK(mgr.weighted_sum(bit_lvl_impl(mgr)) == word_lvl_spec(mgr));
}
