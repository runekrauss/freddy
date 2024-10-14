// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/kfdd.hpp>  // dd::kfdd_manager

#include <iostream>  // std::cout

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd basic consts tests", "[basic]")
{

    dd::kfdd_manager mgr_basics;
    auto test_one = mgr_basics.one();
    //test_one.print();

    CHECK(test_one.eval({}) == 1);

    auto test_zero = mgr_basics.zero();
    //test_zero.print();

    CHECK(test_zero.eval({}) == 0);
}

TEST_CASE("kfdd basic XOR tests [S]", "[basic]")
{
    dd::kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::S);
    std::cout << "xor_v1:" << '\n';
    xor_v1.print();

    auto xor_v2 = mgr_xor.var(expansion::S);
    std::cout << "xor_v2:" << '\n';
    xor_v2.print();

    auto xor_v1_v2 = xor_v1 ^ xor_v2;
    std::cout << "xor_v1_v2:" << '\n';
    xor_v1_v2.print();

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);


    auto xor_v3 = mgr_xor.var(expansion::S);
    std::cout << "xor_v3:" << '\n';
    xor_v3.print();

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;
    std::cout << "xor_v1_v2_v3:" << '\n';
    xor_v1_v2_v3.print();

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}
TEST_CASE("kfdd basic XOR tests [ND]", "[basic]")
{
    dd::kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::ND);
    std::cout << "xor_v1:" << '\n';
    xor_v1.print();

    auto xor_v2 = mgr_xor.var(expansion::ND);
    std::cout << "xor_v2:" << '\n';
    xor_v2.print();

    auto xor_v1_v2 = xor_v1 ^ xor_v2;
    std::cout << "xor_v1_v2:" << '\n';
    xor_v1_v2.print();

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);


    auto xor_v3 = mgr_xor.var(expansion::ND);
    std::cout << "xor_v3:" << '\n';
    xor_v3.print();

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;
    std::cout << "xor_v1_v2_v3:" << '\n';
    xor_v1_v2_v3.print();

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}


TEST_CASE("kfdd basic XOR tests [PD]", "[basic]")
{
    dd::kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::PD);
    std::cout << "xor_v1:" << '\n';
    xor_v1.print();

    auto xor_v2 = mgr_xor.var(expansion::PD);
    std::cout << "xor_v2:" << '\n';
    xor_v2.print();

    auto xor_v1_v2 = xor_v1 ^ xor_v2;
    std::cout << "xor_v1_v2:" << '\n';
    xor_v1_v2.print();

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);


    auto xor_v3 = mgr_xor.var(expansion::PD);
    std::cout << "xor_v3:" << '\n';
    xor_v3.print();

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;
    std::cout << "xor_v1_v2_v3:" << '\n';
    xor_v1_v2_v3.print();

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}
TEST_CASE("kfdd basic XOR tests [mixed]", "[basic]")
{
    dd::kfdd_manager mgr_xor;

    auto xor_v1 = mgr_xor.var(expansion::S);
    std::cout << "xor_v1:" << '\n';
    xor_v1.print();

    auto xor_v2 = mgr_xor.var(expansion::PD);
    std::cout << "xor_v2:" << '\n';
    xor_v2.print();

    auto xor_v1_v2 = xor_v1 ^ xor_v2;
    std::cout << "xor_v1_v2:" << '\n';
    xor_v1_v2.print();

    CHECK(xor_v1_v2.eval({true, true}) == 0);
    CHECK(xor_v1_v2.eval({true, false}) == 1);
    CHECK(xor_v1_v2.eval({false, true}) == 1);
    CHECK(xor_v1_v2.eval({false, false}) == 0);


    auto xor_v3 = mgr_xor.var(expansion::ND);
    std::cout << "xor_v3:" << '\n';
    xor_v3.print();

    auto xor_v1_v2_v3 = xor_v1_v2 ^ xor_v3;
    std::cout << "xor_v1_v2_v3:" << '\n';
    xor_v1_v2_v3.print();

    CHECK(xor_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(xor_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(xor_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [ND]", "[basic]")
{
    dd::kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::ND);
    auto or_v2 = mgr_or.var(expansion::ND);

    auto or_v1_v2 = or_v1 | or_v2;
    std::cout << "or_v1_v2:" << '\n';
    or_v1_v2.print();

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true }) == 1);
    CHECK(or_v1_v2.eval({true,  false}) == 1);
    CHECK(or_v1_v2.eval({true,  true }) == 1);

    auto or_v3 = mgr_or.var(expansion::ND);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;
    std::cout << "or_v1_v2_v3:" << '\n';
    or_v1_v2_v3.print();

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [mixed]", "[basic]")
{
    dd::kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::PD);
    auto or_v2 = mgr_or.var(expansion::ND);

    auto or_v1_v2 = or_v1 | or_v2;
    std::cout << "or_v1_v2:" << '\n';
    or_v1_v2.print();

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true }) == 1);
    CHECK(or_v1_v2.eval({true,  false}) == 1);
    CHECK(or_v1_v2.eval({true,  true }) == 1);

    auto or_v3 = mgr_or.var(expansion::S);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;
    std::cout << "or_v1_v2_v3:" << '\n';
    or_v1_v2_v3.print();

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [PD]", "[basic]")
{
    dd::kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::PD);
    auto or_v2 = mgr_or.var(expansion::PD);

    auto or_v1_v2 = or_v1 | or_v2;
    std::cout << "or_v1_v2:" << '\n';
    or_v1_v2.print();

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true }) == 1);
    CHECK(or_v1_v2.eval({true,  false}) == 1);
    CHECK(or_v1_v2.eval({true,  true }) == 1);

    auto or_v3 = mgr_or.var(expansion::PD);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;
    std::cout << "or_v1_v2_v3:" << '\n';
    or_v1_v2_v3.print();

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic OR tests [S]", "[basic]")
{
    dd::kfdd_manager mgr_or;
    auto or_v1 = mgr_or.var(expansion::S);
    auto or_v2 = mgr_or.var(expansion::S);

    auto or_v1_v2 = or_v1 | or_v2;
    std::cout << "or_v1_v2:" << '\n';
    or_v1_v2.print();

    CHECK(or_v1_v2.eval({false, false}) == 0);
    CHECK(or_v1_v2.eval({false, true }) == 1);
    CHECK(or_v1_v2.eval({true,  false}) == 1);
    CHECK(or_v1_v2.eval({true,  true }) == 1);

    auto or_v3 = mgr_or.var(expansion::S);
    auto or_v1_v2_v3 = or_v1_v2 | or_v3;
    std::cout << "or_v1_v2_v3:" << '\n';
    or_v1_v2_v3.print();

    CHECK(or_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(or_v1_v2_v3.eval({false, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({false, true, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, false, true}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, false}) == 1);
    CHECK(or_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [PD]", "[basic]")
{
    dd::kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::PD);
    auto and_v2 = mgr_and.var(expansion::PD);

    auto and_v1_v2 = and_v1 & and_v2;
    std::cout << "and_v1_v2:" << '\n';
    and_v1_v2.print();

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::PD);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;
    std::cout << "and_v1_v2_v3:" << '\n';
    and_v1_v2_v3.print();

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [mixed]", "[basic]")
{
    dd::kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::S);
    auto and_v2 = mgr_and.var(expansion::ND);

    auto and_v1_v2 = and_v1 & and_v2;
    std::cout << "and_v1_v2:" << '\n';
    and_v1_v2.print();

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::PD);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;
    std::cout << "and_v1_v2_v3:" << '\n';
    and_v1_v2_v3.print();

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [ND]", "[basic]")
{
    dd::kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::ND);
    auto and_v2 = mgr_and.var(expansion::ND);

    auto and_v1_v2 = and_v1 & and_v2;
    std::cout << "and_v1_v2:" << '\n';
    and_v1_v2.print();

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::ND);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;
    std::cout << "and_v1_v2_v3:" << '\n';
    and_v1_v2_v3.print();

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic AND tests [S]", "[basic]")
{
    dd::kfdd_manager mgr_and;
    auto and_v1 = mgr_and.var(expansion::S);
    auto and_v2 = mgr_and.var(expansion::S);

    auto and_v1_v2 = and_v1 & and_v2;
    std::cout << "and_v1_v2:" << '\n';
    and_v1_v2.print();

    CHECK(and_v1_v2.eval({false, false}) == false);
    CHECK(and_v1_v2.eval({false, true}) == false);
    CHECK(and_v1_v2.eval({true, false}) == false);
    CHECK(and_v1_v2.eval({true, true}) == true);

    auto and_v3 = mgr_and.var(expansion::S);
    auto and_v1_v2_v3 = and_v1_v2 & and_v3;
    std::cout << "and_v1_v2_v3:" << '\n';
    and_v1_v2_v3.print();

    CHECK(and_v1_v2_v3.eval({false, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({false, true, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, false, true}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, false}) == 0);
    CHECK(and_v1_v2_v3.eval({true, true, true}) == 1);
}

TEST_CASE("kfdd basic NEG tests [PD]", "[basic]")
{
    dd::kfdd_manager mgr_neg;
    auto v1 = mgr_neg.var(expansion::PD);
    std::cout << "v1:" << '\n';
    v1.print();

    CHECK(v1.eval({false}) == 0);
    CHECK(v1.eval({true}) == 1);

    auto negated_v1 = ~v1;
    std::cout << "~v1:" << '\n';
    negated_v1.print();

    CHECK(negated_v1.eval({false}) == 1);
    CHECK(negated_v1.eval({true}) == 0);
}

TEST_CASE("kfdd basic NEG tests [ND]", "[basic]")
{
    dd::kfdd_manager mgr_neg;
    auto v1 = mgr_neg.var(expansion::ND);
    std::cout << "v1:" << '\n';
    v1.print();

    CHECK(v1.eval({false}) == 0);
    CHECK(v1.eval({true}) == 1);

    auto negated_v1 = ~v1;
    std::cout << "~v1:" << '\n';
    negated_v1.print();

    CHECK(negated_v1.eval({false}) == 1);
    CHECK(negated_v1.eval({true}) == 0);
}

TEST_CASE("kfdd basic NEG tests [S]", "[basic]")
{
    dd::kfdd_manager mgr_neg;
    auto v1 = mgr_neg.var(expansion::S);
    std::cout << "v1:" << '\n';
    v1.print();

    CHECK(v1.eval({false}) == 0);
    CHECK(v1.eval({true}) == 1);

    auto negated_v1 = ~v1;
    std::cout << "~v1:" << '\n';
    negated_v1.print();

    CHECK(negated_v1.eval({false}) == 1);
    CHECK(negated_v1.eval({true}) == 0);
}

}  //namespace