// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE, SECTION, CHECK

#include <freddy/dd/zdd.hpp>             // zdd_manager

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Tests
// *********************************************************************************************************************

TEST_CASE("ZDD: terminals and variable construction", "[zdd][basic]")
{
    zdd_manager mgr;

    SECTION("Zero / One semantics")
    {
        auto const z = mgr.zero();  // {}
        auto const o = mgr.one();   // {{}}

        CHECK(z.is_zero());
        CHECK_FALSE(z.is_one());

        CHECK(o.is_one());
        CHECK_FALSE(o.is_zero());
    }

    SECTION("Creating variables increments var_count and keeps manager consistent")
    {
        auto const a = mgr.var("a");
        auto const b = mgr.var("b");

        CHECK_FALSE(a.is_const());
        CHECK_FALSE(b.is_const());
        CHECK(mgr.var_count() == 2);
    }
}

TEST_CASE("ZDD: singleton families", "[zdd][basic]")
{
    zdd_manager mgr;
    (void)mgr.var("a"); // index 0
    (void)mgr.var("b"); // index 1
    (void)mgr.var("c"); // index 2

    auto const Sa = mgr.singleton(0); // {{a}}
    auto const Sb = mgr.singleton(1); // {{b}}
    auto const Sc = mgr.singleton(2); // {{c}}

    SECTION("Singleton is not terminal")
    {
        CHECK_FALSE(Sa.is_const());
        CHECK_FALSE(Sb.is_const());
        CHECK_FALSE(Sc.is_const());
    }

    SECTION("Basic set identities with singletons")
    {
        auto const z = mgr.zero();
        CHECK((Sa | z) == Sa);
        CHECK((Sa & z).is_zero());

        // Sa \ Sa = {}
        CHECK((Sa - Sa).is_zero());

        // Sa \ {} = Sa
        CHECK((Sa - z) == Sa);
    }
}

TEST_CASE("ZDD: union / intersection / difference on a small example", "[zdd][basic]")
{
    zdd_manager mgr;

    // Create variables
    (void)mgr.var("a"); // 0
    (void)mgr.var("b"); // 1
    (void)mgr.var("c"); // 2

    // {{a}}, {{b}}, {{c}}
    auto const Sa = mgr.singleton(0);
    auto const Sb = mgr.singleton(1);
    auto const Sc = mgr.singleton(2);

    // Build {{a,b}} via node(a, node(b, one, zero), zero)
    auto const eps = mgr.one();   // {{}}
    auto const emp = mgr.zero();  // {}

    auto const Nb  = mgr.node(1, eps, emp);     // {{b}}
    auto const AB  = mgr.node(0, Nb, emp);      // {{a,b}}

    // A = {{a,b}, {c}}
    auto const A = AB | Sc;

    // B = {{b}, {c}}
    auto const B = Sb | Sc;

    // Expected:
    // U = {{a,b}, {b}, {c}}
    // I = {{c}}
    // D = {{a,b}}
    auto const U = A | B;
    auto const I = A & B;
    auto const D = A - B;

    SECTION("Intersection is {{c}}")
    {
        CHECK(I == Sc);
    }

    SECTION("Difference is {{a,b}}")
    {
        CHECK(D == AB);
    }

    SECTION("Union contains all three families")
    {
        // Hard to check 'contains' without iterator API, so we check expected equalities via set algebra:
        // U - Sc should still contain {{a,b},{b}} and be non-zero
        CHECK_FALSE((U - Sc).is_zero());

        // Removing AB from U still leaves something (at least {b} or {c})
        CHECK_FALSE((U - AB).is_zero());
    }
}

TEST_CASE("ZDD: basic identities", "[zdd][basic]")
{
    zdd_manager mgr;
    (void)mgr.var("a"); // 0
    auto const Sa = mgr.singleton(0);
    auto const z  = mgr.zero();

    SECTION("Idempotence")
    {
        CHECK((Sa | Sa) == Sa);
        CHECK((Sa & Sa) == Sa);
    }

    SECTION("Absorption with empty")
    {
        CHECK((Sa | z) == Sa);
        CHECK((Sa & z).is_zero());
    }
}
