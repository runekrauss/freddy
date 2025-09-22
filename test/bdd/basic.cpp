// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/bdd.hpp>  // bdd_manager

#include <sstream>  // std::ostringstream
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("BDD is constructed", "[basic]")
{
    bdd_manager mgr{config{25, 3'359, 2}};
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

    SECTION("Combination by conjunction")
    {
        auto const f = x0 & x1;

        CHECK(f == x0.ite(x1, mgr.zero()));
        CHECK_FALSE(f.eval(std::vector{false, false}));
        CHECK_FALSE(f.eval(std::vector{false, true}));
        CHECK_FALSE(f.eval(std::vector{true, false}));
        CHECK(f.eval(std::vector{true, true}));
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
    bdd_manager mgr{{25, 3'359, 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = x0 & x1 | ~x2;

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constants are supported")
    {
        CHECK(mgr.const_count() == 1);
        CHECK(f.has_const(false));
    }

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() == 12);
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() == 7);
    }

    SECTION("Size is computed")
    {
        CHECK(f.size() == 4);
    }

    SECTION("Depth is computed")
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
    bdd_manager mgr{{25, 3'359, 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = ~(x0 | x1) & x2;

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

    SECTION("Variable is eliminated by existential quantification")
    {
        auto const g = f.exist(2);

        CHECK_FALSE(g.is_essential(2));
        CHECK(g == ~(mgr.var(0) | mgr.var(1)));
    }

    SECTION("Variable is eliminated by universal quantification")
    {
        CHECK(f.forall(0).is_zero());
    }
}

TEST_CASE("BDD variable order is changeable", "[basic]")
{
    bdd_manager mgr{{25, 3'359, 4}};
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x0 = mgr.var("x0"), x2 = mgr.var("x2");
    auto const f = x0 & x1 | x2 & x3;
    mgr.config().max_node_growth = 2.0f;

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK(f.high().var() == 2);
        CHECK(f.high().low().var() == 1);
        CHECK(f.fn(true, true).is_one());
        CHECK(f.eval({true, false, true, false}));
        CHECK(f.eval({false, true, false, true}));
        CHECK_FALSE(f.eval({true, false, false, true}));
        CHECK_FALSE(f.eval({false, true, true, false}));
    }

    SECTION("Reordering finds a minimum")
    {
        auto const prev_size = f.size();
        mgr.reorder();

        CHECK(prev_size > f.size());
        CHECK(f.eval({true, false, true, false}));
        CHECK(f.eval({false, true, false, true}));
        CHECK_FALSE(f.eval({true, false, false, true}));
        CHECK_FALSE(f.eval({false, true, true, false}));
    }
}

TEST_CASE("BDD can be cleaned up", "[basic]")
{
    bdd_manager mgr{{25, 3'359, 3}};
    auto const f = mgr.var() | mgr.var() | mgr.var();
    auto const prev_ecount = mgr.edge_count();
    auto const prev_ncount = mgr.node_count();
    mgr.gc();
    std::ostringstream oss;
    oss << mgr << "\n\n";
    oss << f << "\n\n";
    f.dump_dot(oss);
    // std::cout << oss.str();

    CHECK(prev_ecount > mgr.edge_count());
    CHECK(prev_ncount > mgr.node_count());
}

TEST_CASE("BDD solves #SAT", "[basic]")
{
    bdd_manager mgr{{25, 3'359, 3}};

    CHECK((mgr.var() & mgr.var() | ~mgr.var()).sharpsat() == 5);
}
