// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/add.hpp>  // add_manager

#include <cmath>         // std::nextafter
#include <cstdint>       // std::int32_t
#include <limits>        // std::numeric_limits
#include <sstream>       // std::ostringstream
#include <stdexcept>     // std::overflow_error
#include <system_error>  // std::system_error
#include <vector>        // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("ADD is constructed", "[basic]")
{
    add_manager<std::int32_t> mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const x0 = mgr.var(), x1 = mgr.var();

    SECTION("Negation is performed")
    {
        auto const f = -x0;

        CHECK(f.high().is_const());
        CHECK(f.low().is_const());
        CHECK_FALSE(f.fn(true).is_one());
        CHECK(f.fn(false).is_zero());
    }

    SECTION("Combination by multiplication")
    {
        auto const f = x0 * x1;

        CHECK(f.eval(std::vector{false, false}) == 0);
        CHECK(f.eval(std::vector{false, true}) == 0);
        CHECK(f.eval(std::vector{true, false}) == 0);
        CHECK(f.eval(std::vector{true, true}) == 1);
    }

    SECTION("Combination by addition")
    {
        auto const f = x0 + x1;

        CHECK(f.eval({false, false}) == 0);
        CHECK(f.eval({false, true}) == 1);
        CHECK(f.eval({true, false}) == 1);
        CHECK(f.eval({true, true}) == 2);
    }

    SECTION("Logical complement is represented")
    {
        CHECK(~x0 == mgr.one() - x0);
    }

    SECTION("Conjunction is represented")
    {
        auto const f = x0 & x1;

        CHECK(f == x0.ite(x1, mgr.zero()));
        CHECK_FALSE(f.eval({false, false}));
        CHECK_FALSE(f.eval({false, true}));
        CHECK_FALSE(f.eval({true, false}));
        CHECK(f.eval({true, true}) == 1);
    }

    SECTION("Disjunction is represented")
    {
        auto const f = x0 | x1;

        CHECK(f == x0.ite(mgr.one(), x1));
        CHECK_FALSE(f.eval({false, false}));
        CHECK(f.eval({false, true}) == 1);
        CHECK(f.eval({true, false}) == 1);
        CHECK(f.eval({true, true}) == 1);
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

TEST_CASE("ADD can be characterized", "[basic]")
{
    add_manager<std::int32_t> mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = mgr.constant(4) * x2 + mgr.two() * x1 + x0;

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constants are supported")
    {
        CHECK(mgr.const_count() == 9);
        CHECK(f.has_const(0));
        CHECK(f.has_const(7));
    }

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() == 20);
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() == 20);
    }

    SECTION("Size is computed")
    {
        CHECK(f.size() == 15);
    }

    SECTION("Depth is computed")
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

TEST_CASE("ADD is substituted", "[basic]")
{
    add_manager<float> mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const x0 = mgr.var(), x1 = mgr.var();
    auto const f = mgr.constant(8.5f) - mgr.constant(20.0f) * x1 + mgr.two() * x0 + mgr.constant(4.0f) * x0 * x1;

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

    SECTION("Variable is eliminated by existential quantification")
    {
        auto const g = f.exist(0);

        CHECK_FALSE(g.is_essential(0));
        CHECK(g == -mgr.constant(70.25f) - mgr.constant(10.0f) * x1);
    }

    SECTION("Variable is eliminated by universal quantification")
    {
        CHECK(f.forall(1) == mgr.constant(-97.75f) + mgr.constant(40.0f) * x0);
    }
}

TEST_CASE("ADD variable order is changeable", "[basic]")
{
    add_manager<std::int32_t> mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 6}};
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x5 = mgr.var("x5"), x0 = mgr.var("x0"), x2 = mgr.var("x2"),
               x4 = mgr.var("x4");
    auto const f = (x0 & x1) | (x2 & x3) | (x4 & x5);
    mgr.config().max_node_growth = 2.0f;

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK(f.eval({true, false, false, true, false, false}) == 1);
        CHECK(f.eval({false, true, false, false, true, false}) == 1);
        CHECK(f.eval({false, false, true, false, false, true}) == 1);
    }

    SECTION("Reordering finds a minimum")
    {
        auto const prev_size = f.size();
        mgr.reorder();

        CHECK(prev_size > f.size());
        CHECK(f.eval({true, false, false, true, false, false}) == 1);
        CHECK(f.eval({false, true, false, false, true, false}) == 1);
        CHECK(f.eval({false, false, true, false, false, true}) == 1);
    }
}

TEST_CASE("ADD can be cleaned up", "[basic]")
{
    add_manager<float> mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const f = mgr.var() - mgr.var() - mgr.var();
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

TEST_CASE("ADD detects misuse of word-level operations", "[basic]")
{
    add_manager<std::int32_t> imgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 0}};
    auto const ic = imgr.constant(std::numeric_limits<std::int32_t>::max(), true);
    add_manager<float> fmgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 0}};
    auto const fc = fmgr.constant(std::numeric_limits<float>::max(), true);

    SECTION("Multiplication can overflow")
    {
        CHECK_THROWS_AS(ic * imgr.two(), std::system_error);
        CHECK_THROWS_AS(fc * fmgr.constant(std::nextafter(1.0f, 2.0f)), std::overflow_error);
    }

    SECTION("Addition can overflow")
    {
        CHECK_THROWS_AS(ic + imgr.one(), std::system_error);
        CHECK_THROWS_AS(fc + fmgr.constant(std::ldexp(std::numeric_limits<float>::max(), -1)), std::overflow_error);
    }
}
