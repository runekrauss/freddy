// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bhd.hpp>  // dd::bhd_manager

#ifndef NDEBUG
#include <iostream>  // std::cout
#endif
#include <utility>  // std::pair
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("BHD is constructed", "[basic]")
{
    dd::bhd_manager mgr;
    auto const x0 = mgr.var(), x1 = mgr.var();

    SECTION("Negation uses complemented edges")
    {
        auto const f = ~x0;

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
        CHECK_FALSE(f.eval({false, false}).value());
        CHECK(f.eval({false, true}).value());
        CHECK(f.eval({true, false}).value());
        CHECK(f.eval({true, true}).value());
    }

    SECTION("Combination by conjunction")
    {
        auto const f = x0 & x1;

        CHECK(f == x0.ite(x1, mgr.zero()));
        CHECK_FALSE(f.eval({false, false}).value());
        CHECK_FALSE(f.eval({false, true}).value());
        CHECK_FALSE(f.eval({true, false}).value());
        CHECK(f.eval({true, true}).value());
    }

    SECTION("XOR is applied")
    {
        auto const f = x0 ^ x1;

        CHECK(f.high().same_node(f.low()));
        CHECK(f.high() != f.low());
        CHECK_FALSE(f.eval({false, false}).value());
        CHECK(f.eval({false, true}).value());
        CHECK(f.eval({true, false}).value());
        CHECK_FALSE(f.eval({true, true}).value());
    }

    SECTION("EXP maintains its level")
    {
        auto const f = x0 & mgr.exp();
        CHECK((f & x1) == f);

#ifndef NDEBUG
        std::cout << mgr << '\n';
        std::cout << f << '\n';
        f.print();
#endif
        CHECK(f.high().is_exp());
        CHECK(f.low().is_zero());
    }

    SECTION("Computing with EXP")
    {
        CHECK((mgr.exp() & ~mgr.exp()) == mgr.exp());
    }
}

TEST_CASE("BHD can be characterized", "[basic]")
{
    dd::bhd_manager mgr;
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = (x0 | x1) & (x2 | mgr.exp());

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constants are supported")
    {
        CHECK(mgr.const_count() == 2);
        CHECK(f.has_const(false));
        CHECK(f.has_const(true));
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() <= 11);
    }

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() <= 19);
    }

    SECTION("Nodes are counted")
    {
        CHECK(f.size() == 5);
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

TEST_CASE("BHD is substituted", "[basic]")
{
    dd::bhd_manager mgr;
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = (x0 & x1) | ~(x2 ^ mgr.exp());

    SECTION("Variable is replaced by function")
    {
        auto const g = f.compose(2, mgr.var() & mgr.var());

        CHECK_FALSE(g.is_essential(2));
        CHECK(g.high().low().same_node(g.low()));
        CHECK(g.is_essential(3));
        CHECK(g.is_essential(4));
        CHECK(g.low().var() == 3);
        CHECK(g.low().high().var() == 4);
    }

    SECTION("Variable is restricted to constant")
    {
        CHECK(f.restr(1, true).high().is_const());
        CHECK(f.restr(2, false).low().is_const());
    }

    SECTION("Variable is removed by existential quantification")
    {
        auto const g = f.exist(1);

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.high().is_one());
        CHECK(g.eval({true, true, true}).value());
    }

    SECTION("Variable is removed by universal quantification")
    {
        CHECK(f.forall(1) == ~(mgr.var(2) ^ mgr.exp()));
    }
}

TEST_CASE("BHD variable order is changeable", "[basic]")
{
    dd::bhd_manager mgr;
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x0 = mgr.var("x0"), x2 = mgr.var("x2");
    auto const f = (x0 & x1) | (x2 & x3) | mgr.exp();

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK(f.high().var() == 2);
        CHECK(f.high().low().var() == 1);
        CHECK(f.fn(true).fn(true).is_one());
        CHECK(f.eval({true, false, true, false}).value());
        CHECK(f.eval({false, true, false, true}).value());
        CHECK_FALSE(f.eval({true, false, false, true}).has_value());
        CHECK_FALSE(f.eval({false, true, true, false}).has_value());
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
        CHECK(f.eval({true, false, true, false}).value());
        CHECK(f.eval({false, true, false, true}).value());
        CHECK_FALSE(f.eval({true, false, false, true}).has_value());
        CHECK_FALSE(f.eval({false, true, true, false}).has_value());
    }
}

TEST_CASE("BHD SAT is analyzed", "[basic]")
{
    dd::bhd_manager mgr;
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = (x0 | x2) & (x1 | mgr.exp());

    SECTION("Solutions are generated symbolically")
    {
        auto const sols = f.sat();

        REQUIRE(sols.size() == 2);
        CHECK(sols[0] == std::vector{false, true, true});
        CHECK(sols[1] == std::vector{true, true, true});
    }

    SECTION("Unit clauses are generated")
    {
        auto const uclauses = f.uc();

        REQUIRE(uclauses.size() == 2);
        CHECK(uclauses[0] == std::vector{std::pair{0, false}, std::pair{1, false}});
        CHECK(uclauses[1] == std::vector{std::pair{0, true}, std::pair{1, false}});
    }
}

TEST_CASE("BHD heuristics restrict solution space", "[basic]")
{
    SECTION("Level heuristic introduces EXP")
    {
        dd::bhd_manager mgr{dd::bhd_heuristic::LVL, 2};
        auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var(), x3 = mgr.var();
        auto const f = x0 & x1 & x2 & x3;

        CHECK(f.depth() == 2);
        CHECK(f.has_const(true));
        CHECK_FALSE(f.eval({false, false, false, false}).value());
    }

    SECTION("Memory heuristic introduces EXP")
    {
        dd::bhd_manager mgr{dd::bhd_heuristic::MEM, 1};
        auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var(), x3 = mgr.var(), x4 = mgr.var(), x5 = mgr.var();
        auto const f = x0 & x1 & x2 & x3 & x4 & x5;

        CHECK(f.depth() == 4);
        CHECK(f.has_const(true));
        CHECK_FALSE(f.eval(std::vector(6, true)).has_value());
    }
}