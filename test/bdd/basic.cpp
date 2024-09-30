// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>  // dd::bdd_manager

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

TEST_CASE("BDD is constructed", "[basic]")
{
    dd::bdd_manager mgr;
    auto const x0 = mgr.var();
    auto const x1 = mgr.var();

    SECTION("Negation uses complemented edges")
    {
        auto const f = ~x0;
#ifndef NDEBUG
        std::cout << mgr << '\n';
        std::cout << f << '\n';
        f.print();
#endif
        CHECK(f.is_complemented());
        CHECK(f.high().is_const());
        CHECK(f.low().is_const());
        CHECK(f.fn(true).is_zero());
        CHECK(f.fn(false).is_one());
    }

    SECTION("Combination by disjunction")
    {
        auto const f = x0 | x1;

        CHECK(f == x0.ite(mgr.one(), x1));
        CHECK_FALSE(f.eval({false, false}));
        CHECK(f.eval({false, true}));
        CHECK(f.eval({true, false}));
        CHECK(f.eval({true, true}));
    }

    SECTION("Combination by conjunction")
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

        CHECK(f.high().same_node(f.low()));
        CHECK(f.high() != f.low());
        CHECK_FALSE(f.eval({false, false}));
        CHECK(f.eval({false, true}));
        CHECK(f.eval({true, false}));
        CHECK_FALSE(f.eval({true, true}));
    }
}

TEST_CASE("BDD can be characterized", "[basic]")
{
    dd::bdd_manager mgr;
    auto const f = mgr.var() & mgr.var() | ~mgr.var();

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constant is supported")
    {
        CHECK(mgr.const_count() == 1);
        CHECK(f.has_const(false));
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() <= 7);
    }

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() <= 12);
    }

    SECTION("Nodes are counted")
    {
        CHECK(f.size() == 4);
    }

    SECTION("Longest path is computed")
    {
        CHECK(f.depth() == 3);
    }

    SECTION("Number of paths is computed")
    {
        CHECK(f.path_count() == 5);
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

TEST_CASE("BDD is substituted", "[basic]")
{
    dd::bdd_manager mgr;
    auto const f = ~(mgr.var() | mgr.var()) & mgr.var();

    SECTION("Variable is replaced by function")
    {
        auto const g = f.compose(1, mgr.var() & mgr.var());

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.high().same_node(g.low().low()));
        CHECK(g.is_essential(3));
        CHECK(g.is_essential(4));
        CHECK(g.low().high().var() == 3);
        CHECK(g.low().high().high().var() == 4);
    }

    SECTION("Variable is restricted to constant")
    {
        CHECK(f.restr(1, true).is_zero());
        CHECK(f.restr(2, false).is_zero());
    }

    SECTION("Variable is removed by existential quantification")
    {
        auto const g = f.exist(2);

        CHECK_FALSE(g.is_essential(2));
        CHECK(g == ~(mgr.var(0) | mgr.var(1)));
    }

    SECTION("Variable is removed by universal quantification")
    {
        CHECK(f.forall(0).is_zero());
    }
}

TEST_CASE("BDD variable order is changeable", "[basic]")
{
    dd::bdd_manager mgr;
    auto const x1 = mgr.var("x1");
    auto const x3 = mgr.var("x3");
    auto const x0 = mgr.var("x0");
    auto const x2 = mgr.var("x2");
    auto const f = x0 & x1 | x2 & x3;

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK(f.high().var() == 2);
        CHECK(f.high().low().var() == 1);
        CHECK(f.fn(true).fn(true).is_one());
        CHECK(f.eval({true, false, true, false}));
        CHECK(f.eval({false, true, false, true}));
        CHECK_FALSE(f.eval({true, false, false, true}));
        CHECK_FALSE(f.eval({false, true, true, false}));
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
        CHECK(f.eval({true, false, true, false}));
        CHECK(f.eval({false, true, false, true}));
        CHECK_FALSE(f.eval({true, false, false, true}));
        CHECK_FALSE(f.eval({false, true, true, false}));
    }
}

TEST_CASE("BDD solves #SAT", "[basic]")
{
    dd::bdd_manager mgr;

    CHECK((mgr.var() & mgr.var() | ~mgr.var()).sharpsat() == 5);
}
