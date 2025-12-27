// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/kfdd.hpp>  // dd::kfdd_manager
#include <freddy/expansion.hpp>

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// restr Tests - Shannon Decomposition
// *********************************************************************************************************************

TEST_CASE("kfdd restr Shannon basic", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);

    // Shannon: f|_{x=0} = f⁰, f|_{x=1} = f¹
    CHECK(x0.restr(0, false) == mgr.zero());
    CHECK(x0.restr(0, true) == mgr.one());
}

TEST_CASE("kfdd restr Shannon complex", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);
    auto const x2 = mgr.var(expansion::S);

    // f = x0 & x1 | x2
    auto const f = (x0 & x1) | x2;

    // Restrict x0 = 0: f becomes x2
    auto const f_x0_0 = f.restr(0, false);
    CHECK(f_x0_0.eval({false, false, false}) == false);
    CHECK(f_x0_0.eval({false, false, true}) == true);
    CHECK(f_x0_0.eval({false, true, false}) == false);
    CHECK(f_x0_0.eval({false, true, true}) == true);

    // Restrict x0 = 1: f becomes x1 | x2
    auto const f_x0_1 = f.restr(0, true);
    CHECK(f_x0_1.eval({true, false, false}) == false);
    CHECK(f_x0_1.eval({true, false, true}) == true);
    CHECK(f_x0_1.eval({true, true, false}) == true);
    CHECK(f_x0_1.eval({true, true, true}) == true);

    // Restrict x1 = 0: f becomes x2
    auto const f_x1_0 = f.restr(1, false);
    CHECK(f_x1_0.eval({false, false, false}) == false);
    CHECK(f_x1_0.eval({false, false, true}) == true);
    CHECK(f_x1_0.eval({true, false, false}) == false);
    CHECK(f_x1_0.eval({true, false, true}) == true);

    // Restrict x2 = 1: f becomes 1
    auto const f_x2_1 = f.restr(2, true);
    CHECK(f_x2_1 == mgr.one());
}

// *********************************************************************************************************************
// restr Tests - Positive Davio Decomposition
// *********************************************************************************************************************

TEST_CASE("kfdd restr pD basic", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);

    // pD: f = f⁰ ⊕ x·f²
    // f|_{x=0} = f⁰ = lo
    // f|_{x=1} = f⁰ ⊕ f² = lo ⊕ hi
    CHECK(x0.restr(0, false) == mgr.zero());
    CHECK(x0.restr(0, true) == mgr.one());
}

TEST_CASE("kfdd restr pD complex", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::pD);
    auto const x2 = mgr.var(expansion::pD);

    // f = x0 ^ x1 ^ x2 (XOR chain - natural for Davio)
    auto const f = x0 ^ x1 ^ x2;

    // Verify original function
    CHECK(f.eval({false, false, false}) == false);
    CHECK(f.eval({false, false, true}) == true);
    CHECK(f.eval({false, true, false}) == true);
    CHECK(f.eval({false, true, true}) == false);
    CHECK(f.eval({true, false, false}) == true);
    CHECK(f.eval({true, false, true}) == false);
    CHECK(f.eval({true, true, false}) == false);
    CHECK(f.eval({true, true, true}) == true);

    // Restrict x0 = 0: f becomes x1 ^ x2
    auto const f_x0_0 = f.restr(0, false);
    CHECK(f_x0_0.eval({false, false, false}) == false);
    CHECK(f_x0_0.eval({false, false, true}) == true);
    CHECK(f_x0_0.eval({false, true, false}) == true);
    CHECK(f_x0_0.eval({false, true, true}) == false);

    // Restrict x0 = 1: f becomes ~(x1 ^ x2) = x1 XNOR x2
    auto const f_x0_1 = f.restr(0, true);
    CHECK(f_x0_1.eval({true, false, false}) == true);
    CHECK(f_x0_1.eval({true, false, true}) == false);
    CHECK(f_x0_1.eval({true, true, false}) == false);
    CHECK(f_x0_1.eval({true, true, true}) == true);
}

// *********************************************************************************************************************
// restr Tests - Negative Davio Decomposition
// *********************************************************************************************************************

TEST_CASE("kfdd restr nD basic", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);

    // nD: f = f¹ ⊕ x̄·f²
    // f|_{x=0} = f¹ ⊕ f² = lo ⊕ hi
    // f|_{x=1} = f¹ = lo
    CHECK(x0.restr(0, false) == mgr.zero());
    CHECK(x0.restr(0, true) == mgr.one());
}

TEST_CASE("kfdd restr nD complex", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);
    auto const x1 = mgr.var(expansion::nD);
    auto const x2 = mgr.var(expansion::nD);

    // f = x0 ^ x1 ^ x2
    auto const f = x0 ^ x1 ^ x2;

    // Verify original function
    CHECK(f.eval({false, false, false}) == false);
    CHECK(f.eval({false, false, true}) == true);
    CHECK(f.eval({false, true, false}) == true);
    CHECK(f.eval({false, true, true}) == false);
    CHECK(f.eval({true, false, false}) == true);
    CHECK(f.eval({true, false, true}) == false);
    CHECK(f.eval({true, true, false}) == false);
    CHECK(f.eval({true, true, true}) == true);

    // Restrict x1 = 0: f becomes x0 ^ x2
    auto const f_x1_0 = f.restr(1, false);
    CHECK(f_x1_0.eval({false, false, false}) == false);
    CHECK(f_x1_0.eval({false, false, true}) == true);
    CHECK(f_x1_0.eval({true, false, false}) == true);
    CHECK(f_x1_0.eval({true, false, true}) == false);

    // Restrict x1 = 1: f becomes ~(x0 ^ x2)
    auto const f_x1_1 = f.restr(1, true);
    CHECK(f_x1_1.eval({false, true, false}) == true);
    CHECK(f_x1_1.eval({false, true, true}) == false);
    CHECK(f_x1_1.eval({true, true, false}) == false);
    CHECK(f_x1_1.eval({true, true, true}) == true);
}

// *********************************************************************************************************************
// restr Tests - Mixed Decomposition Types
// *********************************************************************************************************************

TEST_CASE("kfdd restr mixed decomposition", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);   // Shannon
    auto const x1 = mgr.var(expansion::pD);  // Positive Davio
    auto const x2 = mgr.var(expansion::nD);  // Negative Davio

    // f = x0 & x1 & x2
    auto const f = x0 & x1 & x2;

    // Verify original function (AND of all three)
    CHECK(f.eval({false, false, false}) == false);
    CHECK(f.eval({false, false, true}) == false);
    CHECK(f.eval({false, true, false}) == false);
    CHECK(f.eval({false, true, true}) == false);
    CHECK(f.eval({true, false, false}) == false);
    CHECK(f.eval({true, false, true}) == false);
    CHECK(f.eval({true, true, false}) == false);
    CHECK(f.eval({true, true, true}) == true);

    // Restrict Shannon variable x0 = 0
    auto const f_x0_0 = f.restr(0, false);
    CHECK(f_x0_0 == mgr.zero());

    // Restrict Shannon variable x0 = 1
    auto const f_x0_1 = f.restr(0, true);
    CHECK(f_x0_1.eval({true, true, true}) == true);
    CHECK(f_x0_1.eval({true, false, true}) == false);

    // Restrict pD variable x1 = 1
    auto const f_x1_1 = f.restr(1, true);
    CHECK(f_x1_1.eval({true, true, true}) == true);
    CHECK(f_x1_1.eval({false, true, true}) == false);

    // Restrict nD variable x2 = 1
    auto const f_x2_1 = f.restr(2, true);
    CHECK(f_x2_1.eval({true, true, true}) == true);
    CHECK(f_x2_1.eval({true, false, true}) == false);
}

// *********************************************************************************************************************
// restr Tests - Non-essential Variables
// *********************************************************************************************************************

TEST_CASE("kfdd restr non-essential variable", "[kfdd][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);
    auto const x2 = mgr.var(expansion::S);

    // f = x0 & x1 (does not depend on x2)
    auto const f = x0 & x1;

    // Restricting non-essential variable should return the same function
    CHECK(f.restr(2, false) == f);
    CHECK(f.restr(2, true) == f);
}

// *********************************************************************************************************************
// compose Tests - Shannon Decomposition
// *********************************************************************************************************************

TEST_CASE("kfdd compose Shannon basic", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);
    auto const x2 = mgr.var(expansion::S);

    // compose(x0, 0, x1 & x2) replaces x0 with (x1 & x2)
    auto const g = x1 & x2;
    auto const composed = x0.compose(0, g);

    // Result should be equivalent to x1 & x2
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == false);
    CHECK(composed.eval({false, true, false}) == false);
    CHECK(composed.eval({false, true, true}) == true);
    CHECK(composed.eval({true, false, false}) == false);
    CHECK(composed.eval({true, false, true}) == false);
    CHECK(composed.eval({true, true, false}) == false);
    CHECK(composed.eval({true, true, true}) == true);
}

TEST_CASE("kfdd compose Shannon complex", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);
    auto const x2 = mgr.var(expansion::S);
    auto const x3 = mgr.var(expansion::S);

    // f = x0 & x1
    auto const f = x0 & x1;
    // g = x2 | x3
    auto const g = x2 | x3;
    // compose(f, 1, g) replaces x1 with (x2 | x3)
    auto const composed = f.compose(1, g);

    // Result: x0 & (x2 | x3)
    CHECK(composed.eval({false, false, false, false}) == false);
    CHECK(composed.eval({true, false, false, false}) == false);
    CHECK(composed.eval({true, false, true, false}) == true);
    CHECK(composed.eval({true, false, false, true}) == true);
    CHECK(composed.eval({true, false, true, true}) == true);
}

// *********************************************************************************************************************
// compose Tests - Positive Davio Decomposition
// *********************************************************************************************************************

TEST_CASE("kfdd compose pD basic", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::pD);
    auto const x2 = mgr.var(expansion::pD);

    // compose(x0, 0, x1 ^ x2) replaces x0 with (x1 ^ x2)
    auto const g = x1 ^ x2;
    auto const composed = x0.compose(0, g);

    // Result should be equivalent to x1 ^ x2
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == true);
    CHECK(composed.eval({false, true, false}) == true);
    CHECK(composed.eval({false, true, true}) == false);
}

TEST_CASE("kfdd compose pD complex", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::pD);
    auto const x2 = mgr.var(expansion::pD);
    auto const x3 = mgr.var(expansion::pD);

    // f = x0 ^ x1
    auto const f = x0 ^ x1;
    // g = x2 & x3
    auto const g = x2 & x3;
    // compose(f, 0, g) replaces x0 with (x2 & x3)
    auto const composed = f.compose(0, g);

    // Result: (x2 & x3) ^ x1
    CHECK(composed.eval({false, false, false, false}) == false);
    CHECK(composed.eval({false, true, false, false}) == true);
    CHECK(composed.eval({false, false, true, true}) == true);
    CHECK(composed.eval({false, true, true, true}) == false);
}

// *********************************************************************************************************************
// compose Tests - Negative Davio Decomposition
// *********************************************************************************************************************

TEST_CASE("kfdd compose nD basic", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);
    auto const x1 = mgr.var(expansion::nD);
    auto const x2 = mgr.var(expansion::nD);

    // compose(x0, 0, x1 | x2) replaces x0 with (x1 | x2)
    auto const g = x1 | x2;
    auto const composed = x0.compose(0, g);

    // Result should be equivalent to x1 | x2
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == true);
    CHECK(composed.eval({false, true, false}) == true);
    CHECK(composed.eval({false, true, true}) == true);
}

// *********************************************************************************************************************
// compose Tests - Mixed Decomposition Types
// *********************************************************************************************************************

TEST_CASE("kfdd compose mixed decomposition", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);   // Shannon
    auto const x1 = mgr.var(expansion::pD);  // Positive Davio
    auto const x2 = mgr.var(expansion::nD);  // Negative Davio
    auto const x3 = mgr.var(expansion::S);   // Shannon

    // f = x0 ^ x1
    auto const f = x0 ^ x1;
    // g = x2 & x3
    auto const g = x2 & x3;
    // compose(f, 0, g) replaces x0 with (x2 & x3)
    auto const composed = f.compose(0, g);

    // Result: (x2 & x3) ^ x1
    CHECK(composed.eval({false, false, false, false}) == false);
    CHECK(composed.eval({false, true, false, false}) == true);
    CHECK(composed.eval({false, false, true, true}) == true);
    CHECK(composed.eval({false, true, true, true}) == false);
}

// *********************************************************************************************************************
// compose Tests - With Constants
// *********************************************************************************************************************

TEST_CASE("kfdd compose with constants", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);

    auto const f = x0 & x1;

    // compose(f, 0, zero) should equal restr(f, 0, false)
    CHECK(f.compose(0, mgr.zero()) == f.restr(0, false));

    // compose(f, 0, one) should equal restr(f, 0, true)
    CHECK(f.compose(0, mgr.one()) == f.restr(0, true));

    // compose(f, 1, zero) should equal restr(f, 1, false)
    CHECK(f.compose(1, mgr.zero()) == f.restr(1, false));

    // compose(f, 1, one) should equal restr(f, 1, true)
    CHECK(f.compose(1, mgr.one()) == f.restr(1, true));
}

TEST_CASE("kfdd compose with constants pD", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::pD);

    auto const f = x0 ^ x1;

    // compose(f, 0, zero) should equal restr(f, 0, false)
    CHECK(f.compose(0, mgr.zero()) == f.restr(0, false));

    // compose(f, 0, one) should equal restr(f, 0, true)
    CHECK(f.compose(0, mgr.one()) == f.restr(0, true));
}

TEST_CASE("kfdd compose with constants nD", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);
    auto const x1 = mgr.var(expansion::nD);

    auto const f = x0 ^ x1;

    // compose(f, 0, zero) should equal restr(f, 0, false)
    CHECK(f.compose(0, mgr.zero()) == f.restr(0, false));

    // compose(f, 0, one) should equal restr(f, 0, true)
    CHECK(f.compose(0, mgr.one()) == f.restr(0, true));
}

// *********************************************************************************************************************
// Mathematical Properties
// *********************************************************************************************************************

TEST_CASE("kfdd compose identity", "[kfdd][compose][restr]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);
    auto const x2 = mgr.var(expansion::S);

    auto const f = (x0 & x1) | x2;

    // compose(f, x, x) == f (identity)
    CHECK(f.compose(0, x0) == f);
    CHECK(f.compose(1, x1) == f);
    CHECK(f.compose(2, x2) == f);
}

TEST_CASE("kfdd compose identity pD", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::pD);

    auto const f = x0 ^ x1;

    // compose(f, x, x) == f (identity)
    CHECK(f.compose(0, x0) == f);
    CHECK(f.compose(1, x1) == f);
}

TEST_CASE("kfdd compose identity nD", "[kfdd][compose]")
{
    dd::kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);
    auto const x1 = mgr.var(expansion::nD);

    auto const f = x0 ^ x1;

    // compose(f, x, x) == f (identity)
    CHECK(f.compose(0, x0) == f);
    CHECK(f.compose(1, x1) == f);
}

TEST_CASE("kfdd restr compose equivalence all types", "[kfdd][compose][restr]")
{
    dd::kfdd_manager mgr_s;
    auto const s0 = mgr_s.var(expansion::S);
    auto const s1 = mgr_s.var(expansion::S);
    auto const f_s = s0 & s1;

    dd::kfdd_manager mgr_pd;
    auto const pd0 = mgr_pd.var(expansion::pD);
    auto const pd1 = mgr_pd.var(expansion::pD);
    auto const f_pd = pd0 ^ pd1;

    dd::kfdd_manager mgr_nd;
    auto const nd0 = mgr_nd.var(expansion::nD);
    auto const nd1 = mgr_nd.var(expansion::nD);
    auto const f_nd = nd0 ^ nd1;

    // Shannon: compose with constants equals restr
    CHECK(f_s.compose(0, mgr_s.zero()) == f_s.restr(0, false));
    CHECK(f_s.compose(0, mgr_s.one()) == f_s.restr(0, true));

    // pD: compose with constants equals restr
    CHECK(f_pd.compose(0, mgr_pd.zero()) == f_pd.restr(0, false));
    CHECK(f_pd.compose(0, mgr_pd.one()) == f_pd.restr(0, true));

    // nD: compose with constants equals restr
    CHECK(f_nd.compose(0, mgr_nd.zero()) == f_nd.restr(0, false));
    CHECK(f_nd.compose(0, mgr_nd.one()) == f_nd.restr(0, true));
}
