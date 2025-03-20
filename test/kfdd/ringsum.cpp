//
// Created by marvin on 09.10.24.
//
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include "freddy/config.hpp"
#include "freddy/expansion.hpp"

#include <freddy/dd/kfdd.hpp>  // dd::kfdd_manager

#include <array>     // std::array
// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd ringsum sift test", "[ringsum]")
{
    dd::kfdd_manager mgr_ctrl{};
    auto x0_c = mgr_ctrl.var(expansion::S);
    auto x1_c = mgr_ctrl.var(expansion::S);
    auto x2_c = mgr_ctrl.var(expansion::S);
    auto x3_c = mgr_ctrl.var(expansion::S);
    auto x4_c = mgr_ctrl.var(expansion::S);
    auto x5_c = mgr_ctrl.var(expansion::S);
    auto x6_c = mgr_ctrl.var(expansion::S);
    auto x7_c = mgr_ctrl.var(expansion::S);
    auto x8_c = mgr_ctrl.var(expansion::S);
    auto x9_c = mgr_ctrl.var(expansion::S);

    dd::kfdd_manager mgr{};
    auto x0 = mgr.var(expansion::S);
    auto x1 = mgr.var(expansion::S);
    auto x2 = mgr.var(expansion::S);
    auto x3 = mgr.var(expansion::S);
    auto x4 = mgr.var(expansion::S);
    auto x5 = mgr.var(expansion::S);
    auto x6 = mgr.var(expansion::S);
    auto x7 = mgr.var(expansion::S);
    auto x8 = mgr.var(expansion::S);
    auto x9 = mgr.var(expansion::S);
    //auto x10 = mgr.var(expansion::S);
    //auto x11 = mgr.var(expansion::S);
    //auto x12 = mgr.var(expansion::S);
    //auto x13 = mgr.var(expansion::S);
    //auto x14 = mgr.var(expansion::S);
    //auto x15 = mgr.var(expansion::S);
    //auto x16 = mgr.var(expansion::S);
    //auto x17 = mgr.var(expansion::S);
    //auto x18 = mgr.var(expansion::S);
    //auto x19 = mgr.var(expansion::S);
    //auto x20 = mgr.var(expansion::S);
    //auto x21 = mgr.var(expansion::S);
    //auto x22 = mgr.var(expansion::S);
    //auto x23 = mgr.var(expansion::S);
    //auto x24 = mgr.var(expansion::S);
    //auto x25 = mgr.var(expansion::S);
    //auto x26 = mgr.var(expansion::S);
    //auto x27 = mgr.var(expansion::S);
    //auto x28 = mgr.var(expansion::S);
    //auto x29 = mgr.var(expansion::S);
    //auto x30 = mgr.var(expansion::S);
    //auto x31 = mgr.var(expansion::S);
    //auto result = (x18 & x17 & x9 & x23 & x21 & x27 & x31 & x27 & x0 & x25) ^ (x31 & x17 & x15 & x14 & x28 & x21 & x8 & x3 & x12 & x7) ^ (x30 & x24 & x11 & x1 & x9 & x25 & x11 & x2 & x24 & x30) ^ (x31 & x6 & x8 & x11 & x31 & x24 & x3 & x12 & x2 & x19) ^ (x26 & x10 & x25 & x12 & x3 & x3 & x20 & x3 & x1 & x15) ^ (x20 & x10 & x2 & x24 & x26 & x11 & x20 & x16 & x24 & x5) ^ (x9 & x30 & x14 & x24 & x21 & x19 & x9 & x4 & x28 & x12) ^ (x25 & x16 & x1 & x18 & x22 & x23 & x11 & x26 & x12 & x29) ^ (x31 & x4 & x27 & x28 & x27 & x29 & x10 & x21 & x4 & x8) ^ (x7 & x23 & x14 & x26 & x29 & x6 & x10 & x16 & x15 & x6) ^ (x10 & x2 & x25 & x18 & x2 & x7 & x1 & x6 & x20 & x25) ^ (x22 & x23 & x22 & x22 & x3 & x2 & x28 & x24 & x29 & x25) ^ (x1 & x27 & x28 & x6 & x0 & x27 & x19 & x26 & x23 & x2) ^ (x0 & x25 & x23 & x21 & x14 & x17 & x15 & x7 & x4 & x28) ^ (x2 & x7 & x29 & x14 & x18 & x11 & x16 & x12 & x31 & x4) ^ (x0 & x10 & x2 & x10 & x27 & x16 & x8 & x31 & x25 & x11) ^ (x24 & x20 & x2 & x31 & x8 & x24 & x21 & x24 & x25 & x2) ^ (x6 & x25 & x28 & x12 & x13 & x21 & x30 & x18 & x0 & x21) ^ (x15 & x18 & x9 & x13 & x17 & x8 & x0 & x5 & x1 & x3) ^ (x12 & x11 & x22 & x10 & x8 & x22 & x26 & x28 & x24 & x30);
    auto result_c = (x3_c & x5_c & x4_c & x2_c & x7_c & x2_c & x5_c & x7_c & x6_c & x1_c) ^ (x4_c & x9_c & x1_c & x1_c & x1_c & x0_c & x6_c & x4_c & x0_c & x3_c) ^ (x7_c & x2_c & x7_c & x5_c & x1_c & x7_c & x1_c & x4_c & x3_c & x0_c) ^ (x7_c & x8_c & x0_c & x7_c & x7_c & x3_c & x6_c & x9_c & x1_c & x5_c) ^ (x6_c & x4_c & x6_c & x4_c & x8_c & x4_c & x6_c & x1_c & x2_c & x8_c) ^ (x6_c & x3_c & x5_c & x9_c & x8_c & x1_c & x6_c & x5_c & x4_c & x1_c) ^ (x7_c & x8_c & x4_c & x6_c & x4_c & x5_c & x9_c & x4_c & x3_c & x3_c) ^ (x4_c & x3_c & x5_c & x7_c & x6_c & x2_c & x5_c & x3_c & x3_c & x8_c) ^ (x3_c & x6_c & x9_c & x4_c & x0_c & x9_c & x7_c & x9_c & x5_c & x4_c) ^ (x8_c & x3_c & x9_c & x0_c & x9_c & x8_c & x1_c & x0_c & x8_c & x3_c);
    auto result = (x3 & x5 & x4 & x2 & x7 & x2 & x5 & x7 & x6 & x1) ^ (x4 & x9 & x1 & x1 & x1 & x0 & x6 & x4 & x0 & x3) ^ (x7 & x2 & x7 & x5 & x1 & x7 & x1 & x4 & x3 & x0) ^ (x7 & x8 & x0 & x7 & x7 & x3 & x6 & x9 & x1 & x5) ^ (x6 & x4 & x6 & x4 & x8 & x4 & x6 & x1 & x2 & x8) ^ (x6 & x3 & x5 & x9 & x8 & x1 & x6 & x5 & x4 & x1) ^ (x7 & x8 & x4 & x6 & x4 & x5 & x9 & x4 & x3 & x3) ^ (x4 & x3 & x5 & x7 & x6 & x2 & x5 & x3 & x3 & x8) ^ (x3 & x6 & x9 & x4 & x0 & x9 & x7 & x9 & x5 & x4) ^ (x8 & x3 & x9 & x0 & x9 & x8 & x1 & x0 & x8 & x3);
    for(int i = 0; i < 1024; i++)
    {
        auto var0 =  !!(i & 0b0000000001);
        auto var1 =  !!(i & 0b0000000010);
        auto var2 =  !!(i & 0b0000000100);
        auto var3 =  !!(i & 0b0000001000);
        auto var4 =  !!(i & 0b0000010000);
        auto var5 =  !!(i & 0b0000100000);
        auto var6 =  !!(i & 0b0001000000);
        auto var7 =  !!(i & 0b0010000000);
        auto var8 =  !!(i & 0b0100000000);
        auto var9 =  !!(i & 0b1000000000);
        const std::vector params = {var0,var1,var2,var3,var4,var5,var6,var7,var8,var9};
        CHECK(result.eval(params) == result_c.eval(params));
    }
    std::cout << result.size() << '\n';
    result.print();
    mgr.dtl_sift();
    std::cout << result.size() << '\n';
    result.print();


}

TEST_CASE("simple ringsum", "[ringsum]")
{
    auto const noVars = 5;
    auto const noCombs = 2 << noVars;
    dd::kfdd_manager mgr{};
    auto x0 = mgr.var(expansion::S);
    auto x1 = mgr.var(expansion::S);
    auto x2 = mgr.var(expansion::S);
    auto x3 = mgr.var(expansion::S);
    auto x4 = mgr.var(expansion::S);
    auto ringsum = mgr.one() ^ (x3 & x0) ^ (x4 & x0) ^ (x0 & x1);

    std::cout << "ringsum 1:" << '\n';
    ringsum.print();

    dd::kfdd_manager mgr2{};
    auto y0 = mgr2.var(expansion::S);
    auto y1 = mgr2.var(expansion::S);
    auto y2 = mgr2.var(expansion::S);
    auto y3 = mgr2.var(expansion::S);
    auto y4 = mgr2.var(expansion::S);
    auto ringsum2 = mgr2.one() ^ (y3 & y0) ^ (y4 & y0) ^ (y0 & y1);

    mgr2.dtl_sift();

    std::cout << "ringsum 2:" << '\n';
    ringsum2.print();

    for(int i = 0; i < noCombs; i++)
    {
        std::vector<bool> input_vars;
        for(int j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (2 << j)));
        }
        CHECK(ringsum.eval(input_vars) == ringsum2.eval(input_vars));
    }

}

TEST_CASE("wiki example ringsum", "[ringsum]")
{
    auto const noVars = 5;
    auto const noCombs = 2 << noVars;
    dd::kfdd_manager mgr{};
    auto A = mgr.var(expansion::S);
    auto B = mgr.var(expansion::S);
    auto C = mgr.var(expansion::S);
    auto ringsum = mgr.one() ^ B ^ (A & B) ^ (A & C) ^ (A & B & C);

    std::cout << "ringsum 1:" << '\n';
    ringsum.print();

    dd::kfdd_manager mgr2{};
    auto A2 = mgr2.var(expansion::S);
    auto B2 = mgr2.var(expansion::S);
    auto C2 = mgr2.var(expansion::S);
    auto ringsum2 = mgr2.one() ^ B2 ^ (A2 & B2) ^ (A2 & C2) ^ (A2 & B2 & C2);

    CHECK(ringsum2.eval({false,false,false})==true);
    CHECK(ringsum2.eval({false,false,true})==true);
    CHECK(ringsum2.eval({false,true,false})==false);
    CHECK(ringsum2.eval({false,true,true})==false);
    CHECK(ringsum2.eval({true,false,false})==true);
    CHECK(ringsum2.eval({true,false,true})==false);
    CHECK(ringsum2.eval({true,true,false})==true);
    CHECK(ringsum2.eval({true,true,true})==true);

    std::cout << "ringsum 2 before sifting:" << '\n';
    ringsum2.print();

    mgr2.dtl_sift();

    std::cout << "ringsum 2 after sifting:" << '\n';
    ringsum2.print();

    CHECK(ringsum.eval({false,false,false})==true);
    CHECK(ringsum.eval({false,false,true})==true);
    CHECK(ringsum.eval({false,true,false})==false);
    CHECK(ringsum.eval({false,true,true})==false);
    CHECK(ringsum.eval({true,false,false})==true);
    CHECK(ringsum.eval({true,false,true})==false);
    CHECK(ringsum.eval({true,true,false})==true);
    CHECK(ringsum.eval({true,true,true})==true);


    CHECK(ringsum2.eval({false,false,false})==true);
    CHECK(ringsum2.eval({false,false,true})==true);
    CHECK(ringsum2.eval({false,true,false})==false);
    CHECK(ringsum2.eval({false,true,true})==false);
    CHECK(ringsum2.eval({true,false,false})==true);
    CHECK(ringsum2.eval({true,false,true})==false);
    CHECK(ringsum2.eval({true,true,false})==true);
    CHECK(ringsum2.eval({true,true,true})==true);


}

}