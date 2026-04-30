#include <catch2/catch_test_macros.hpp>

#include <freddy/config.hpp>
#include <freddy/dd/zdd.hpp>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

using namespace freddy;

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

TEST_CASE("ZDD AND is idempotent", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    CHECK((x0 & x0) == x0);
}

TEST_CASE("ZDD OR is idempotent (semantic)", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const a = x0 | x0;
    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(a.eval(as) == x0.eval(as));
    }
}

TEST_CASE("ZDD OR with zero holds semantically", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const a = x0 | mgr.zero();
    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(a.eval(as) == x0.eval(as));
    }
}

TEST_CASE("ZDD AND with zero is zero", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    CHECK((x0 & mgr.zero()).is_zero());
}

TEST_CASE("ZDD AND with one is identity (semantic)", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const a = x0 & mgr.one();
    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(a.eval(as) == x0.eval(as));
    }
}

TEST_CASE("ZDD non-trivial combination builds non-empty structure", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = (x0 & x1) | x2;
    CHECK_FALSE(f.is_zero());
    CHECK(f.size() >= 1);
    CHECK(f.depth() >= 1);
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

TEST_CASE("ZDD De Morgan identity holds", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var("x0");
    auto const x1 = mgr.var("x1");
    auto const x2 = mgr.var("x2");

    auto const lhs = ~(x0 | x1);
    auto const rhs = (~x0) & (~x1);

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(lhs.eval(as) == rhs.eval(as));
    }
}

TEST_CASE("ZDD can be characterized (extended)", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    auto const f = x0 | x1 | x2;

    SECTION("Number of paths is computed")
    {
        CHECK(f.path_count() >= 1);
    }

    SECTION("Essential variables are identifiable")
    {
        CHECK(x0.is_essential(0));
        CHECK_FALSE(x0.is_essential(1));
        CHECK(x1.is_essential(1));
        CHECK(x2.is_essential(2));
    }

    SECTION("same_node detects structural identity")
    {
        auto const& g = x0;
        CHECK(x0.same_node(g));
        CHECK_FALSE(x0.same_node(x1));
    }
}

TEST_CASE("ZDD variable is restricted to constant", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const g = x0.restr(0, false);
    CHECK(g.is_zero());
    auto const h = x0.restr(0, true);
    CHECK(h.is_one());
}

TEST_CASE("ZDD variable is replaced by function (compose)", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const g = x0.compose(0, x0);
    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(g.eval(as) == x0.eval(as));
    }
}

TEST_CASE("ZDD variable is eliminated by existential quantification", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const g = x0.exist(0);
    CHECK_FALSE(g.is_zero());
}

TEST_CASE("ZDD variable is eliminated by universal quantification", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var();
    (void)mgr.var();
    (void)mgr.var();
    auto const g = x0.forall(0);
    CHECK(g.is_zero());
}

TEST_CASE("ZDD instructor example: cube redundancy (semantic check)", "[basic]")
{
    // Cubes: xy, x!yz, xz  => x!yz redundant when xz present.
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var("x");
    auto const y = mgr.var("y");
    auto const z = mgr.var("z");

    auto const cube_xy = x & y;
    auto const cube_xnyz = x & (~y) & z;
    auto const cube_xz = x & z;

    auto const f = cube_xy | cube_xnyz | cube_xz;
    auto const g = cube_xy | cube_xz;

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(f.eval(as) == g.eval(as));
    }
}

TEST_CASE("ZDD subsumption effect reduces/reuses structure", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var("x");
    auto const y = mgr.var("y");
    auto const z = mgr.var("z");

    auto const xy = x & y;
    auto const xnyz = x & (~y) & z;
    auto const xz = x & z;

    auto const before = xy | xnyz;
    auto const after = before | xz;
    auto const target = xy | xz;

    // functional correctness
    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(after.eval(as) == target.eval(as));
    }

    // adding a subsuming cube should not increase graph size
    CHECK(after.size() <= before.size());
}

TEST_CASE("ZDD SOP minimization (two-level logic)", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x = mgr.var("x");
    auto const y = mgr.var("y");
    auto const z = mgr.var("z");

    auto const xy = x & y;
    auto const xnyz = x & (~y) & z;
    auto const xz = x & z;

    // f = xy + x!yz + xz + y  =>  g = xy + xz + y  (x!yz redundant)
    auto const f = xy | xnyz | xz | y;
    auto const g = xy | xz | y;

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(f.eval(as) == g.eval(as));
    }
}

// =====================================================================================================================
// Complement wrapper tests
// =====================================================================================================================

TEST_CASE("ZDD complement wrapper: terminal behaviour", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};

    SECTION("Complement of zero is one")
    {
        CHECK((~mgr.zero()).is_one());
    }

    SECTION("Complement of one is zero")
    {
        CHECK((~mgr.one()).is_zero());
    }

    SECTION("Double negation of zero returns zero")
    {
        CHECK(~~mgr.zero() == mgr.zero());
    }

    SECTION("Double negation of one returns one")
    {
        CHECK(~~mgr.one() == mgr.one());
    }

    SECTION("Complement is involutory on constants: ~zero != ~one")
    {
        CHECK(~mgr.zero() != ~mgr.one());
    }
}

TEST_CASE("ZDD complement wrapper: pointwise correctness on constants", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    (void)x0;
    (void)x1;
    (void)x2;

    std::vector<bool> as(3, false);

    SECTION("~zero evaluates to true everywhere")
    {
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0u) & 1u) != 0;
            as[1] = ((mask >> 1u) & 1u) != 0;
            as[2] = ((mask >> 2u) & 1u) != 0;
            CHECK((~mgr.zero()).eval(as));
        }
    }

    SECTION("~one evaluates to false everywhere")
    {
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0u) & 1u) != 0;
            as[1] = ((mask >> 1u) & 1u) != 0;
            as[2] = ((mask >> 2u) & 1u) != 0;
            CHECK_FALSE((~mgr.one()).eval(as));
        }
    }
}

// =====================================================================================================================
// Disjunction wrapper tests
// =====================================================================================================================

TEST_CASE("ZDD disjunction wrapper: commutativity", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();

    auto const f = x0 & x1;
    auto const g = x1 & x2;

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK((f | g).eval(as) == (g | f).eval(as));
    }
}

TEST_CASE("ZDD disjunction wrapper: associativity", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(((x0 | x1) | x2).eval(as) == (x0 | (x1 | x2)).eval(as));
    }
}

TEST_CASE("ZDD disjunction wrapper: identity and annihilation", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    (void)x2;
    auto const f = x0 & x1;

    std::vector<bool> as(3, false);

    SECTION("f | zero == f")
    {
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0u) & 1u) != 0;
            as[1] = ((mask >> 1u) & 1u) != 0;
            as[2] = ((mask >> 2u) & 1u) != 0;
            CHECK((f | mgr.zero()).eval(as) == f.eval(as));
        }
    }

    SECTION("zero | f == f")
    {
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0u) & 1u) != 0;
            as[1] = ((mask >> 1u) & 1u) != 0;
            as[2] = ((mask >> 2u) & 1u) != 0;
            CHECK((mgr.zero() | f).eval(as) == f.eval(as));
        }
    }
}

// =====================================================================================================================
// Boolean algebra laws (BDD-aligned)
// =====================================================================================================================

TEST_CASE("ZDD boolean algebra: distributivity of & over |", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();

    // f & (g | h) == (f & g) | (f & h)
    auto const lhs = x0 & (x1 | x2);
    auto const rhs = (x0 & x1) | (x0 & x2);

    std::vector<bool> as(3, false);
    for (std::uint64_t mask = 0; mask < 8; ++mask)
    {
        as[0] = ((mask >> 0u) & 1u) != 0;
        as[1] = ((mask >> 1u) & 1u) != 0;
        as[2] = ((mask >> 2u) & 1u) != 0;
        CHECK(lhs.eval(as) == rhs.eval(as));
    }
}

TEST_CASE("ZDD boolean algebra: absorption laws", "[basic]")
{
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 3}};
    auto const x0 = mgr.var(), x1 = mgr.var(), x2 = mgr.var();
    (void)x2;

    std::vector<bool> as(3, false);

    SECTION("f & (f | g) == f")
    {
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0u) & 1u) != 0;
            as[1] = ((mask >> 1u) & 1u) != 0;
            as[2] = ((mask >> 2u) & 1u) != 0;
            CHECK((x0 & (x0 | x1)).eval(as) == x0.eval(as));
        }
    }

    SECTION("f | (f & g) == f")
    {
        for (std::uint64_t mask = 0; mask < 8; ++mask)
        {
            as[0] = ((mask >> 0u) & 1u) != 0;
            as[1] = ((mask >> 1u) & 1u) != 0;
            as[2] = ((mask >> 2u) & 1u) != 0;
            CHECK((x0 | (x0 & x1)).eval(as) == x0.eval(as));
        }
    }
}
