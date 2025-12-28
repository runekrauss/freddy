// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/kfdd.hpp>  // kfdd_manager
#include <freddy/expansion.hpp>

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd basic consts tests", "[kfdd][basic]")
{
    kfdd_manager mgr_basics;
    auto const test_one = mgr_basics.one();

    CHECK(test_one.eval({}) == 1);

    auto const test_zero = mgr_basics.zero();

    CHECK(test_zero.eval({}) == 0);
}

TEST_CASE("kfdd basic XOR tests [S]", "[kfdd][basic]")
{
    kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::S);
    auto xor_v2 = mgr_xor.var(expansion::S);

    auto xor_v1_v2 = xor_v1 ^ xor_v2;

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);

    auto xor_v3 = mgr_xor.var(expansion::S);

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic XOR tests [nD]", "[kfdd][basic]")
{
    kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::nD);
    auto xor_v2 = mgr_xor.var(expansion::nD);

    auto xor_v1_v2 = xor_v1 ^ xor_v2;

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);

    auto xor_v3 = mgr_xor.var(expansion::nD);

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic XOR tests [pD]", "[kfdd][basic]")
{
    kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::pD);
    auto xor_v2 = mgr_xor.var(expansion::pD);

    auto xor_v1_v2 = xor_v1 ^ xor_v2;

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);

    auto xor_v3 = mgr_xor.var(expansion::pD);

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic XOR tests [mixed]", "[kfdd][basic]")
{
    kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::S);
    auto xor_v2 = mgr_xor.var(expansion::pD);

    auto xor_v1_v2 = xor_v1 ^ xor_v2;

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);

    auto xor_v3 = mgr_xor.var(expansion::nD);

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [nD]", "[kfdd][basic]")
{
    kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::nD);
    auto or_v2 = mgr_or.var(expansion::nD);

    auto or_v1_v2 = or_v1 | or_v2;

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true}) == 1);
    CHECK(or_v1_v2.eval({true, false}) == 1);
    CHECK(or_v1_v2.eval({true, true}) == 1);

    auto or_v3 = mgr_or.var(expansion::nD);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [mixed]", "[kfdd][basic]")
{
    kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::pD);
    auto or_v2 = mgr_or.var(expansion::nD);

    auto or_v1_v2 = or_v1 | or_v2;

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true}) == 1);
    CHECK(or_v1_v2.eval({true, false}) == 1);
    CHECK(or_v1_v2.eval({true, true}) == 1);

    auto or_v3 = mgr_or.var(expansion::S);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [pD]", "[kfdd][basic]")
{
    kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::pD);
    auto or_v2 = mgr_or.var(expansion::pD);

    auto or_v1_v2 = or_v1 | or_v2;

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true}) == 1);
    CHECK(or_v1_v2.eval({true, false}) == 1);
    CHECK(or_v1_v2.eval({true, true}) == 1);

    auto or_v3 = mgr_or.var(expansion::pD);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [S]", "[kfdd][basic]")
{
    kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::S);
    auto or_v2 = mgr_or.var(expansion::S);

    auto or_v1_v2 = or_v1 | or_v2;

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true}) == 1);
    CHECK(or_v1_v2.eval({true, false}) == 1);
    CHECK(or_v1_v2.eval({true, true}) == 1);

    auto or_v3 = mgr_or.var(expansion::S);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [pD]", "[kfdd][basic]")
{
    kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::pD);
    auto and_v2 = mgr_and.var(expansion::pD);

    auto and_v1_v2 = and_v1 & and_v2;

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::pD);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [mixed]", "[kfdd][basic]")
{
    kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::S);
    auto and_v2 = mgr_and.var(expansion::nD);

    auto and_v1_v2 = and_v1 & and_v2;

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::pD);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [nD]", "[kfdd][basic]")
{
    kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::nD);
    auto and_v2 = mgr_and.var(expansion::nD);

    CHECK(and_v1.eval({false, false}) == false);
    CHECK(and_v1.eval({true, false}) == true);
    CHECK(and_v1.eval({false, true}) == false);
    CHECK(and_v1.eval({true, true}) == true);

    CHECK(and_v2.eval({false, false}) == false);
    CHECK(and_v2.eval({false, true}) == true);
    CHECK(and_v2.eval({true, false}) == false);
    CHECK(and_v2.eval({true, true}) == true);

    mgr_and.dtl_sift();

    CHECK(and_v1.eval({false, false}) == false);
    CHECK(and_v1.eval({true, false}) == true);
    CHECK(and_v1.eval({false, true}) == false);
    CHECK(and_v1.eval({true, true}) == true);

    CHECK(and_v2.eval({false, false}) == false);
    CHECK(and_v2.eval({false, true}) == true);
    CHECK(and_v2.eval({true, false}) == false);
    CHECK(and_v2.eval({true, true}) == true);

    auto and_v1_v2 = and_v1 & and_v2;

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    mgr_and.dtl_sift();

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::nD);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);

    mgr_and.dtl_sift();

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);

    kfdd_manager mgr_and2;
    auto and2_x2 = mgr_and2.var(expansion::pD, "x2");
    auto and2_x1 = mgr_and2.var(expansion::S, "x1");
    auto and2_x0 = mgr_and2.var(expansion::nD, "x0");
    auto pred = and2_x0 & and2_x1 & and2_x2;
    CHECK(pred.eval({false, false, false}) == 0);
    CHECK(pred.eval({true, false, false}) == 0);
    CHECK(pred.eval({false, true, false}) == 0);
    CHECK(pred.eval({true, true, false}) == 0);
    CHECK(pred.eval({false, false, true}) == 0);
    CHECK(pred.eval({true, false, true}) == 0);
    CHECK(pred.eval({false, true, true}) == 0);
    CHECK(pred.eval({true, true, true}) == 1);

    kfdd_manager mgr_and3;
    auto and3_x0 = mgr_and3.var(expansion::S);
    auto and3_x1 = mgr_and3.var(expansion::S);
    auto and3_x2 = mgr_and3.var(expansion::S);
    auto pred3 = and3_x0 & and3_x1 & and3_x2;
    mgr_and3.change_expansion_type(2, expansion::pD);  // var 2 at level 2 (bottom) - OK
    mgr_and3.swap(0, 2);                               // Swap levels 0 and 2
    mgr_and3.change_expansion_type(0, expansion::nD);  // Now var 0 is at bottom - OK
    mgr_and3.swap(0, 1);                               // More swapping to test reordering
    mgr_and3.swap(0, 2);                               // Continue swapping

    CHECK(pred3.eval({false, false, false}) == 0);
    CHECK(pred3.eval({false, false, true}) == 0);
    CHECK(pred3.eval({false, true, false}) == 0);
    CHECK(pred3.eval({false, true, true}) == 0);
    CHECK(pred3.eval({true, false, false}) == 0);
    CHECK(pred3.eval({true, false, true}) == 0);
    CHECK(pred3.eval({true, true, false}) == 0);
    CHECK(pred3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [S]", "[kfdd][basic]")
{
    kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::S);
    auto and_v2 = mgr_and.var(expansion::S);

    auto and_v1_v2 = and_v1 & and_v2;

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::S);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic NEG tests [pD]", "[kfdd][basic]")
{
    kfdd_manager mgr_neg;
    auto const v1 = mgr_neg.var(expansion::pD);

    CHECK(v1.eval({false}) == 0);
    CHECK(v1.eval({true}) == 1);

    auto const negated_v1 = ~v1;

    CHECK(negated_v1.eval({false}) == 1);
    CHECK(negated_v1.eval({true}) == 0);
}

TEST_CASE("kfdd basic NEG tests [nD]", "[kfdd][basic]")
{
    kfdd_manager mgr_neg;
    auto const v1 = mgr_neg.var(expansion::nD);

    CHECK(v1.eval({false}) == 0);
    CHECK(v1.eval({true}) == 1);

    auto const negated_v1 = ~v1;

    CHECK(negated_v1.eval({false}) == 1);
    CHECK(negated_v1.eval({true}) == 0);
}

TEST_CASE("kfdd basic NEG tests [S]", "[kfdd][basic]")
{
    kfdd_manager mgr_neg;
    auto const v1 = mgr_neg.var(expansion::S);

    CHECK(v1.eval({false}) == 0);
    CHECK(v1.eval({true}) == 1);

    auto const negated_v1 = ~v1;

    CHECK(negated_v1.eval({false}) == 1);
    CHECK(negated_v1.eval({true}) == 0);
}

TEST_CASE("kfdd basic restr test S", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const res0 = x0.restr(0, false);
    CHECK(res0 == mgr.zero());
    auto const res1 = x0.restr(0, true);
    CHECK(res1 == mgr.one());
}

TEST_CASE("kfdd basic restr test pD", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const res0 = x0.restr(0, false);
    CHECK(res0 == mgr.zero());
    auto const res1 = x0.restr(0, true);
    CHECK(res1 == mgr.one());
}

TEST_CASE("kfdd basic restr test nD", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);
    auto const res0 = x0.restr(0, false);
    CHECK(res0 == mgr.zero());
    auto const res1 = x0.restr(0, true);
    CHECK(res1 == mgr.one());
}

TEST_CASE("kfdd basic compose test S", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::S);
    auto const x1 = mgr.var(expansion::S);
    auto const x2 = mgr.var(expansion::S);
    auto const pred = x1 & x2;
    auto const composed = x0.compose(0, pred);
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == false);
    CHECK(composed.eval({false, true, false}) == false);
    CHECK(composed.eval({false, true, true}) == true);
    CHECK(composed.eval({true, false, false}) == false);
    CHECK(composed.eval({true, false, true}) == false);
    CHECK(composed.eval({true, true, false}) == false);
    CHECK(composed.eval({true, true, true}) == true);
}

TEST_CASE("kfdd basic compose test pD", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::pD);
    auto const x2 = mgr.var(expansion::pD);
    auto const pred = x1 & x2;
    auto const composed = x0.compose(0, pred);
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == false);
    CHECK(composed.eval({false, true, false}) == false);
    CHECK(composed.eval({false, true, true}) == true);
    CHECK(composed.eval({true, false, false}) == false);
    CHECK(composed.eval({true, false, true}) == false);
    CHECK(composed.eval({true, true, false}) == false);
    CHECK(composed.eval({true, true, true}) == true);
}

TEST_CASE("kfdd basic compose test nD", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::nD);
    auto const x1 = mgr.var(expansion::nD);
    auto const x2 = mgr.var(expansion::nD);
    auto const pred = x1 & x2;
    auto const composed = x0.compose(0, pred);
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == false);
    CHECK(composed.eval({false, true, false}) == false);
    CHECK(composed.eval({false, true, true}) == true);
    CHECK(composed.eval({true, false, false}) == false);
    CHECK(composed.eval({true, false, true}) == false);
    CHECK(composed.eval({true, true, false}) == false);
    CHECK(composed.eval({true, true, true}) == true);
}

TEST_CASE("kfdd basic compose test mixed", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto const x0 = mgr.var(expansion::pD);
    auto const x1 = mgr.var(expansion::nD);
    auto const x2 = mgr.var(expansion::S);
    auto const pred = x1 & x2;
    auto const composed = x0.compose(0, pred);
    CHECK(composed.eval({false, false, false}) == false);
    CHECK(composed.eval({false, false, true}) == false);
    CHECK(composed.eval({false, true, false}) == false);
    CHECK(composed.eval({false, true, true}) == true);
    CHECK(composed.eval({true, false, false}) == false);
    CHECK(composed.eval({true, false, true}) == false);
    CHECK(composed.eval({true, true, false}) == false);
    CHECK(composed.eval({true, true, true}) == true);
}

TEST_CASE("restr prints", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto x0 = mgr.var(expansion::S);
    auto x1 = mgr.var(expansion::S);
    auto x2 = mgr.var(expansion::S);

    auto pred1 = x0 & x1 & x2;
    auto pred2 = pred1.restr(1, true);
    auto pred3 = pred1.restr(1, false);

    CHECK(pred2.eval({true, false, true}));
    CHECK(pred2.eval({true, true, true}));
    CHECK(pred3.is_zero());
}

TEST_CASE("compose prints", "[kfdd][basic]")
{
    kfdd_manager mgr;
    auto x0 = mgr.var(expansion::S);
    auto x1 = mgr.var(expansion::S);
    auto x2 = mgr.var(expansion::S);
    auto x3 = mgr.var(expansion::S);
    auto x4 = mgr.var(expansion::S);

    auto pred1 = x0 & x1 & x4;
    auto comp = x2 & x3;
    auto pred2 = pred1.compose(1, comp);

    CHECK(pred2.eval({true, false, true, true, true}) == true);
    CHECK(pred2.eval({true, true, true, true, true}) == true);
    CHECK(pred2.eval({true, false, false, true, true}) == false);
}
