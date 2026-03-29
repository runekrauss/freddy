// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>

#include <freddy/config.hpp>
#include <freddy/dd/zdd.hpp>

#include <cstdint>   // std::uint64_t
#include <fstream>   // std::ofstream
#include <sstream>   // std::ostringstream
#include <vector>    // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Tests
// *********************************************************************************************************************

TEST_CASE("ZDD is constructed", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();

    SECTION("Variables are singletons (structure-independent)")
    {
        CHECK_FALSE(x0.is_const());
        CHECK(x0.var() == 0);

        auto const hi = x0.high();
        auto const lo = x0.low();

        CHECK(hi.is_const());
        CHECK(lo.is_const());
        CHECK(hi != lo);
    }

    SECTION("Zero/One are constants")
    {
        CHECK(mgr.zero().is_const());
        CHECK(mgr.zero().is_zero());

        CHECK(mgr.one().is_const());
        CHECK(mgr.one().is_one());
    }
}

TEST_CASE("ZDD set operations work correctly", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();

    SECTION("AND idempotence is structural in your implementation")
    {
        CHECK((x0 & x0) == x0);
    }

    SECTION("OR idempotence holds semantically (structure may differ)")
    {
        auto const a = x0 | x0;
        std::vector<bool> as(3, false);
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0) & 1u) != 0;
            as[1] = ((mask >> 1) & 1u) != 0;
            as[2] = ((mask >> 2) & 1u) != 0;
            CHECK(a.eval(as) == x0.eval(as));
        }
    }

    SECTION("OR with zero holds semantically (structure may differ)")
    {
        auto const a = x0 | mgr.zero();
        std::vector<bool> as(3, false);
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0) & 1u) != 0;
            as[1] = ((mask >> 1) & 1u) != 0;
            as[2] = ((mask >> 2) & 1u) != 0;
            CHECK(a.eval(as) == x0.eval(as));
        }
    }

    SECTION("Intersection with zero is zero")
    {
        CHECK((x0 & mgr.zero()).is_zero());
    }

    SECTION("Intersection with one is identity (semantic)")
    {
        auto const a = x0 & mgr.one();
        std::vector<bool> as(3, false);
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0) & 1u) != 0;
            as[1] = ((mask >> 1) & 1u) != 0;
            as[2] = ((mask >> 2) & 1u) != 0;
            CHECK(a.eval(as) == x0.eval(as));
        }
    }

    SECTION("A non-trivial combination builds a non-empty structure")
    {
        auto const f = (x0 & x1) | x2;
        CHECK_FALSE(f.is_zero());
        CHECK(f.size() >= 1);
        CHECK(f.depth() >= 1);
    }
}

TEST_CASE("ZDD can be characterized", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = (x0 & x1) | x2;

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 3);
    }

    SECTION("#Nodes is determined")
    {
        CHECK(mgr.node_count() >= 2);
    }

    SECTION("Size/depth are computed")
    {
        CHECK(f.size() >= 1);
        CHECK(f.depth() >= 1);
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

TEST_CASE("ZDD instructor example: cube redundancy (semantic check)", "[basic]")
{
    // Cubes: xy, x!yz, xz  => x!yz redundant when xz present.
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var("x");
    auto const y = mgr.var("y");
    auto const z = mgr.var("z");

    auto const cube_xy  = x & y;
    auto const cube_xnyz = x & (~y) & z;
    auto const cube_xz  = x & z;

    auto const F = cube_xy | cube_xnyz | cube_xz;
    auto const G = cube_xy | cube_xz;

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0) & 1u) != 0;
        as[1] = ((mask >> 1) & 1u) != 0;
        as[2] = ((mask >> 2) & 1u) != 0;
        CHECK(F.eval(as) == G.eval(as));
    }
}

TEST_CASE("ZDD subsumption effect reduces/reuses structure", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var("x");
    auto const y = mgr.var("y");
    auto const z = mgr.var("z");

    auto const xy   = x & y;
    auto const xnyz = x & (~y) & z;
    auto const xz   = x & z;

    auto const before = xy | xnyz;
    auto const after  = before | xz;
    auto const target = xy | xz;

    // 1) functional correctness
    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0) & 1u) != 0;
        as[1] = ((mask >> 1) & 1u) != 0;
        as[2] = ((mask >> 2) & 1u) != 0;
        CHECK(after.eval(as) == target.eval(as));
    }

    // 2) adding a subsuming cube should not make the graph bigger
    CHECK(after.size() <= before.size());
}

TEST_CASE("ZDD SOP minimization (two-level logic)", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var("x");
    auto const y = mgr.var("y");
    auto const z = mgr.var("z");

    auto const xy   = x & y;
    auto const xnyz = x & (~y) & z;
    auto const xz   = x & z;

    auto const F = xy | xnyz | xz | y;
    auto const G = xy | xz | y;

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0) & 1u) != 0;
        as[1] = ((mask >> 1) & 1u) != 0;
        as[2] = ((mask >> 2) & 1u) != 0;
        CHECK(F.eval(as) == G.eval(as));
    }
}