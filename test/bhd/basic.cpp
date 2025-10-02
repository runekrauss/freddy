// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // var_index
#include <freddy/dd/bhd.hpp>  // bhd_manager

#include <sstream>  // std::ostringstream
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
    bhd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
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
        CHECK_FALSE(f.eval({false, false}).value());
        CHECK_FALSE(f.eval({false, true}).value());
        CHECK_FALSE(f.eval({true, false}).value());
        CHECK(f.eval({true, true}).value());
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

    SECTION("Computing with the expansion node")
    {
        CHECK((mgr.exp() & ~mgr.exp()) == mgr.exp());
    }

    SECTION("Expansion node maintains its level")
    {
        auto const f = x0 & mgr.exp();

        CHECK(f.high().is_exp());
        CHECK(f.low().is_zero());
        CHECK((f & x1) == f);
    }
}

TEST_CASE("BHD can be characterized", "[basic]")
{
    bhd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
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

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() == 16);
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() == 9);
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
    bhd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = x0 & x1 | ~(x2 ^ mgr.exp());

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

    SECTION("Variable is eliminated by existential quantification")
    {
        auto const g = f.exist(1);

        CHECK_FALSE(g.is_essential(1));
        CHECK(g.high().is_one());
        CHECK(g.eval({true, true, true}).value());
    }

    SECTION("Variable is eliminated by universal quantification")
    {
        CHECK(f.forall(1) == ~(mgr.var(2) ^ mgr.exp()));
    }
}

TEST_CASE("BHD variable order is changeable", "[basic]")
{
    bhd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 4}};
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x0 = mgr.var("x0"), x2 = mgr.var("x2");
    auto const f = x0 & x1 | x2 & x3 | mgr.exp();
    mgr.config().max_node_growth = 2.0f;

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

    SECTION("Reordering finds a minimum")
    {
        auto const prev_size = f.size();
        mgr.reorder();

        CHECK(prev_size > f.size());
        CHECK(f.eval({true, false, true, false}).value());
        CHECK(f.eval({false, true, false, true}).value());
        CHECK_FALSE(f.eval({true, false, false, true}).has_value());
        CHECK_FALSE(f.eval({false, true, true, false}).has_value());
    }
}

TEST_CASE("BHD can be cleaned up", "[basic]")
{
    bhd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const f = mgr.var() & mgr.var() & mgr.var() & mgr.exp();
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

TEST_CASE("BHD SAT can be utilized", "[basic]")
{
    bhd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = (x0 | x2) & (x1 | mgr.exp());

    SECTION("Solutions are represented simultaneously")
    {
        auto const sols = f.sat_solutions();

        REQUIRE(sols.size() == 2);
        CHECK(sols[0] == std::vector{false, true, true});
        CHECK(sols[1] == std::vector{true, true, true});
    }

    SECTION("Unit clauses are generated")
    {
        auto const ucs = f.unit_clauses();

        REQUIRE(ucs.size() == 2);
        CHECK(ucs[0] == std::vector{std::pair<var_index, bool>{0, false}, std::pair<var_index, bool>{1, false}});
        CHECK(ucs[1] == std::vector{std::pair<var_index, bool>{0, true}, std::pair<var_index, bool>{1, false}});
    }
}

TEST_CASE("BHD heuristics restrict solution spaces", "[basic]")
{
    SECTION("Level heuristic can create expansion paths")
    {
        bhd_manager mgr{bhd_heuristic::LEVEL, 2, {.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 4}};
        auto const f = mgr.var() & mgr.var() & mgr.var() & mgr.var();

        CHECK(f.depth() == 2);
        CHECK(f.has_exp());
        CHECK_FALSE(f.eval({false, false, false, false}).value());
    }

    SECTION("Memory heuristic can create expansion paths")
    {
        bhd_manager mgr{bhd_heuristic::MEMORY,
                        1'024,
                        {.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 8}};
        auto const f = mgr.var() & mgr.var() & mgr.var() & mgr.var() & mgr.var() & mgr.var() & mgr.var() & mgr.var();

        CHECK(f.depth() == 6);
        CHECK(f.has_exp());
        CHECK_FALSE(f.eval(std::vector(mgr.var_count(), true)).has_value());
    }
}
