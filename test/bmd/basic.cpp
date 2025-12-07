// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/bmd.hpp>  // bmd_manager

#include <limits>        // std::numeric_limits
#include <sstream>       // std::ostringstream
#include <system_error>  // std::system_error
#include <vector>        // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("BMD is constructed", "[basic]")
{
    bmd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const x0 = mgr.var(), x1 = mgr.var();

    SECTION("Negation is performed")
    {
        auto const f = -x0;

        CHECK(f.weight() == -1);
        CHECK(f.high().is_const());
        CHECK(f.low().is_const());
        CHECK_FALSE(f.fn(true).is_one());
        CHECK(f.fn(false).is_zero());
    }

    SECTION("Combination by multiplication")
    {
        auto const f = x0 * x1;

        CHECK(f.eval({false, false}) == 0);
        CHECK(f.eval({false, true}) == 0);
        CHECK(f.eval({true, false}) == 0);
        CHECK(f.eval({true, true}) == 1);
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

TEST_CASE("BMD can be characterized", "[basic]")
{
    bmd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = mgr.constant(8) - mgr.constant(20) * x2 + mgr.two() * x1 + mgr.constant(4) * x1 * x2 +
                   mgr.constant(12) * x0 + mgr.constant(24) * x0 * x2 + mgr.constant(15) * x0 * x1;

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("Constants are supported")
    {
        CHECK(mgr.const_count() == 1);
    }

    SECTION("#Edges is determined")
    {
        CHECK(mgr.edge_count() == 55);
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() == 15);
    }

    SECTION("Size is computed")
    {
        CHECK(f.size() == 6);
    }

    SECTION("Depth is computed")
    {
        CHECK(f.depth() == 3);
    }

    SECTION("Number of paths is computed")
    {
        CHECK(f.path_count() == 7);
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

TEST_CASE("BMD is substituted", "[basic]")
{
    bmd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const x0 = mgr.var(), x1 = mgr.var();
    auto const f = mgr.constant(8) - mgr.constant(20) * x1 + mgr.two() * x0 + mgr.constant(4) * x0 * x1;

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
        CHECK(g == mgr.constant(72) * mgr.var(1) - mgr.constant(6));
    }

    SECTION("Variable is eliminated by universal quantification")
    {
        CHECK(f.forall(0) == mgr.constant(16) - mgr.constant(88) * mgr.var(1));
    }
}

TEST_CASE("BMD variable order is changeable", "[basic]")
{
    bmd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 6}};
    auto const x1 = mgr.var("x1"), x3 = mgr.var("x3"), x5 = mgr.var("x5"), x0 = mgr.var("x0"), x2 = mgr.var("x2"),
               x4 = mgr.var("x4");
    auto const f = (x0 & x1) | (x2 & x3) | (x4 & x5);
    mgr.config().max_node_growth = 2.0f;

    SECTION("Levels can be swapped")
    {
        mgr.swap(1, 2);

        CHECK(f.eval({true, true, true, false, false, false}) == 0);
        CHECK(f.eval({true, false, false, true, false, false}) == 1);
        CHECK(f.eval({false, true, false, false, true, false}) == 1);
        CHECK(f.eval({false, false, true, false, false, true}) == 1);
    }

    SECTION("Reordering finds a minimum")
    {
        auto const prev_size = f.size();
        mgr.reorder();

        CHECK(prev_size > f.size());
        CHECK(f.eval({true, true, true, false, false, false}) == 0);
        CHECK(f.eval({true, false, false, true, false, false}) == 1);
        CHECK(f.eval({false, true, false, false, true, false}) == 1);
        CHECK(f.eval({false, false, true, false, false, true}) == 1);
    }
}

TEST_CASE("BMD can be cleaned up", "[basic]")
{
    bmd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = x0 + x1 + x2;
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

TEST_CASE("BMD detects misuse of word-level operations", "[basic]")
{
    bmd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 0}};
    auto const c = mgr.constant(std::numeric_limits<bmd_int>::max(), true);

    SECTION("Multiplication can overflow")
    {
        CHECK_THROWS_AS(c * mgr.two(), std::system_error);
    }

    SECTION("Addition can overflow")
    {
        CHECK_THROWS_AS(c + mgr.one(), std::system_error);
    }
}

TEST_CASE("BMD interprets bits numerically", "[basic]")
{
    bmd_manager mgr{{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 2}};
    auto const a = mgr.var("a"), b = mgr.var("b");
    std::vector const ha{a ^ b, a & b};

    SECTION("Unsigned number is encoded as weighted bit sum")
    {
        auto const f = mgr.unsigned_bin(ha);

        CHECK(f == a + b);
        CHECK(f.eval({false, false}) == 0);
        CHECK(f.eval({false, true}) == 1);
        CHECK(f.eval({true, false}) == 1);
        CHECK(f.eval({true, true}) == 2);
    }

    SECTION("Two's complement represents integers")
    {
        CHECK(mgr.twos_complement(ha) == a + b - mgr.constant(4) * a * b);
    }
}
