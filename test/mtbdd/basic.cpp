// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/mtbdd.hpp>  // dd::mtbdd_manager

#include <cstdint>  // std::int32_t
#ifndef NDEBUG
#include <iostream>  // std::cout
#endif

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("MTBDD is constructed", "[basic]")
{
    dd::mtbdd_manager<std::int32_t> mgr;
    auto const x0 = mgr.var(), x1 = mgr.var();

    SECTION("Negation is performed")
    {
        auto const f = -x0;

        CHECK(f.high().is_const());
        CHECK(f.low().is_const());
        CHECK_FALSE(f.fn(true).is_one());
        CHECK(f.fn(false).is_zero());
    }

    SECTION("Combination by addition")
    {
        auto const f = x0 + x1;
#ifndef NDEBUG
        std::cout << mgr << '\n';
        std::cout << f << '\n';
        f.print();
#endif
        CHECK(f.eval({false, false}) == 0);
        CHECK(f.eval({false, true}) == 1);
        CHECK(f.eval({true, false}) == 1);
        CHECK(f.eval({true, true}) == 2);
    }

    SECTION("Combination by multiplication")
    {
        auto const f = x0 * x1;

        CHECK(f.eval({false, false}) == 0);
        CHECK(f.eval({false, true}) == 0);
        CHECK(f.eval({true, false}) == 0);
        CHECK(f.eval({true, true}) == 1);
    }

    SECTION("Logical negation is represented")
    {
        auto const f = ~x0;

        CHECK(f == mgr.one() - x0);
    }

    SECTION("Disjunction is represented")
    {
        auto const f = x0 | x1;

        CHECK(f == x0.ite(mgr.one(), x1));
        CHECK_FALSE(f.eval({false, false}));
        CHECK(f.eval({false, true}));
        CHECK(f.eval({true, false}));
        CHECK(f.eval({true, true}));
    }

    SECTION("Conjunction is represented")
    {
        auto const f = x0 & x1;

        CHECK(f == x0.ite(x1, mgr.zero()));
        CHECK_FALSE(f.eval({false, false}));
        CHECK_FALSE(f.eval({false, true}));
        CHECK_FALSE(f.eval({true, false}));
        CHECK(f.eval({true, true}));
    }

    SECTION("XOR is applied")
    {
        auto const f = x0 ^ x1;

        CHECK(f == x0 + x1 - mgr.two() * x0 * x1);
        CHECK_FALSE(f.high().same_node(f.low()));
        CHECK(f.high() != f.low());
        CHECK_FALSE(f.fn(true, true).is_two());
    }
}

TEST_CASE("MTBDD can be characterized", "[basic]")
{
    dd::mtbdd_manager<std::int32_t> mgr;
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = x0 + mgr.two() * x1 + mgr.constant(4) * x2;

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constants is supported")
    {
        CHECK(mgr.const_count() == 9);
        CHECK(f.has_const(0));
        CHECK(f.has_const(7));
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() == 22);
    }

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() == 22);
    }

    SECTION("Nodes are counted")
    {
        CHECK(f.size() == 15);
    }

    SECTION("Longest path is computed")
    {
        CHECK(f.depth() == 3);
    }

    SECTION("Number of paths is computed")
    {
        CHECK(f.path_count() == 8);
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

TEST_CASE("MTBDD is substituted", "[basic]")
{
    dd::mtbdd_manager<float> mgr;
    auto const x0 = mgr.var(), x1 = mgr.var();
    auto const f = mgr.constant(8.5f) - mgr.constant(20.0f) * x0 + mgr.two() * x1 + mgr.constant(4.0f) * x0 * x1;

    SECTION("Variable is replaced by function")
    {
        auto const g = f.compose(1, mgr.var() * mgr.var());

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.is_essential(2));
        CHECK(g.is_essential(3));
        CHECK(g.high().var() == 2);
        CHECK(g.high().high().var() == 3);
    }

    SECTION("Variable is restricted to constant")
    {
        CHECK(f.restr(1, true).high().is_const());
        CHECK(f.restr(1, false).high().is_const());
    }

    SECTION("Variable is removed by existential quantification")
    {
        auto const g = f.exist(0);

        CHECK_FALSE(g.is_essential(0));
        CHECK(g == mgr.constant(94.75f) - mgr.constant(20.0f) * x1 - mgr.constant(12.0f) * x1 * x1);
    }

    SECTION("Variable is removed by universal quantification")
    {
        CHECK(f.forall(1) == mgr.constant(89.25f) - mgr.constant(346.0f) * x0 + mgr.constant(320.0f) * x0 * x0);
    }
}

TEST_CASE("MTBDD variable order is changeable", "[basic]")
{
    dd::mtbdd_manager<std::int32_t> mgr;
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x5 = mgr.var("x5"), x0 = mgr.var("x0"), x2 = mgr.var("x2"),
               x4 = mgr.var("x4");
    auto const f = (x0 & x1) | (x2 & x3) | (x4 & x5);

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK(f.eval({true, false, false, true, false, false}));
        CHECK(f.eval({false, true, false, false, true, false}));
        CHECK(f.eval({false, false, true, false, false, true}));
    }

    SECTION("Variable reordering finds a minimum")
    {
        auto const ncnt_old = mgr.node_count();
        auto const ecnt_old = mgr.edge_count();
        auto const size_old = f.size();
        mgr.reorder();

        CHECK(ncnt_old > mgr.node_count());
        CHECK(ecnt_old > mgr.edge_count());
        CHECK(size_old > f.size());
        CHECK(f.eval({true, false, false, true, false, false}));
        CHECK(f.eval({false, true, false, false, true, false}));
        CHECK(f.eval({false, false, true, false, false, true}));
    }
}
