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

#include "../test/util.cpp"
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
    std::vector<dd::kfdd> c;
    for (auto i = 0; i < 32; ++i)
    {
        c.push_back(mgr_ctrl.var(expansion::S));
    }

    dd::kfdd_manager mgr{};
    std::vector<dd::kfdd> v;
    for (auto i = 0; i < 32; ++i)
    {
        v.push_back(mgr_ctrl.var(expansion::S));
    }

    auto result_c = (c[18] & c[17] & c[9] & c[23] & c[21] & c[27] & c[31] & c[27] & c[0] & c[25]) ^ (c[31] & c[17] & c[15] & c[14] & c[28] & c[21] & c[8] & c[3] & c[12] & c[7]) ^ (c[30] & c[24] & c[11] & c[1] & c[9] & c[25] & c[11] & c[2] & c[24] & c[30]) ^ (c[31] & c[6] & c[8] & c[11] & c[31] & c[24] & c[3] & c[12] & c[2] & c[19]) ^ (c[26] & c[10] & c[25] & c[12] & c[3] & c[3] & c[20] & c[3] & c[1] & c[15]) ^ (c[20] & c[10] & c[2] & c[24] & c[26] & c[11] & c[20] & c[16] & c[24] & c[5]) ^ (c[9] & c[30] & c[14] & c[24] & c[21] & c[19] & c[9] & c[4] & c[28] & c[12]) ^ (c[25] & c[16] & c[1] & c[18] & c[22] & c[23] & c[11] & c[26] & c[12] & c[29]) ^ (c[31] & c[4] & c[27] & c[28] & c[27] & c[29] & c[10] & c[21] & c[4] & c[8]) ^ (c[7] & c[23] & c[14] & c[26] & c[29] & c[6] & c[10] & c[16] & c[15] & c[6]) ^ (c[10] & c[2] & c[25] & c[18] & c[2] & c[7] & c[1] & c[6] & c[20] & c[25]) ^ (c[22] & c[23] & c[22] & c[22] & c[3] & c[2] & c[28] & c[24] & c[29] & c[25]) ^ (c[1] & c[27] & c[28] & c[6] & c[0] & c[27] & c[19] & c[26] & c[23] & c[2]) ^ (c[0] & c[25] & c[23] & c[21] & c[14] & c[17] & c[15] & c[7] & c[4] & c[28]) ^ (c[2] & c[7] & c[29] & c[14] & c[18] & c[11] & c[16] & c[12] & c[31] & c[4]) ^ (c[0] & c[10] & c[2] & c[10] & c[27] & c[16] & c[8] & c[31] & c[25] & c[11]) ^ (c[24] & c[20] & c[2] & c[31] & c[8] & c[24] & c[21] & c[24] & c[25] & c[2]) ^ (c[6] & c[25] & c[28] & c[12] & c[13] & c[21] & c[30] & c[18] & c[0] & c[21]) ^ (c[15] & c[18] & c[9] & c[13] & c[17] & c[8] & c[0] & c[5] & c[1] & c[3]) ^ (c[12] & c[11] & c[22] & c[10] & c[8] & c[22] & c[26] & c[28] & c[24] & c[30]);
    auto result_v = (v[18] & v[17] & v[9] & v[23] & v[21] & v[27] & v[31] & v[27] & v[0] & v[25]) ^ (v[31] & v[17] & v[15] & v[14] & v[28] & v[21] & v[8] & v[3] & v[12] & v[7]) ^ (v[30] & v[24] & v[11] & v[1] & v[9] & v[25] & v[11] & v[2] & v[24] & v[30]) ^ (v[31] & v[6] & v[8] & v[11] & v[31] & v[24] & v[3] & v[12] & v[2] & v[19]) ^ (v[26] & v[10] & v[25] & v[12] & v[3] & v[3] & v[20] & v[3] & v[1] & v[15]) ^ (v[20] & v[10] & v[2] & v[24] & v[26] & v[11] & v[20] & v[16] & v[24] & v[5]) ^ (v[9] & v[30] & v[14] & v[24] & v[21] & v[19] & v[9] & v[4] & v[28] & v[12]) ^ (v[25] & v[16] & v[1] & v[18] & v[22] & v[23] & v[11] & v[26] & v[12] & v[29]) ^ (v[31] & v[4] & v[27] & v[28] & v[27] & v[29] & v[10] & v[21] & v[4] & v[8]) ^ (v[7] & v[23] & v[14] & v[26] & v[29] & v[6] & v[10] & v[16] & v[15] & v[6]) ^ (v[10] & v[2] & v[25] & v[18] & v[2] & v[7] & v[1] & v[6] & v[20] & v[25]) ^ (v[22] & v[23] & v[22] & v[22] & v[3] & v[2] & v[28] & v[24] & v[29] & v[25]) ^ (v[1] & v[27] & v[28] & v[6] & v[0] & v[27] & v[19] & v[26] & v[23] & v[2]) ^ (v[0] & v[25] & v[23] & v[21] & v[14] & v[17] & v[15] & v[7] & v[4] & v[28]) ^ (v[2] & v[7] & v[29] & v[14] & v[18] & v[11] & v[16] & v[12] & v[31] & v[4]) ^ (v[0] & v[10] & v[2] & v[10] & v[27] & v[16] & v[8] & v[31] & v[25] & v[11]) ^ (v[24] & v[20] & v[2] & v[31] & v[8] & v[24] & v[21] & v[24] & v[25] & v[2]) ^ (v[6] & v[25] & v[28] & v[12] & v[13] & v[21] & v[30] & v[18] & v[0] & v[21]) ^ (v[15] & v[18] & v[9] & v[13] & v[17] & v[8] & v[0] & v[5] & v[1] & v[3]) ^ (v[12] & v[11] & v[22] & v[10] & v[8] & v[22] & v[26] & v[28] & v[24] & v[30]);

    mgr.dtl_sift();

    eval_dds(result_v, result_c);
}

TEST_CASE("simple ringsum", "[ringsum]")
{
    dd::kfdd_manager mgr{};
    auto x0 = mgr.var(expansion::S);
    auto x1 = mgr.var(expansion::S);
    auto x2 = mgr.var(expansion::S);
    auto x3 = mgr.var(expansion::S);
    auto x4 = mgr.var(expansion::S);
    auto ringsum = mgr.one() ^ (x3 & x0) ^ (x4 & x0) ^ (x0 & x1);

    dd::kfdd_manager mgr2{};
    auto y0 = mgr2.var(expansion::S);
    auto y1 = mgr2.var(expansion::S);
    auto y2 = mgr2.var(expansion::S);
    auto y3 = mgr2.var(expansion::S);
    auto y4 = mgr2.var(expansion::S);
    auto ringsum2 = mgr2.one() ^ (y3 & y0) ^ (y4 & y0) ^ (y0 & y1);

    mgr2.dtl_sift();

    eval_dds(ringsum, ringsum2);
}

TEST_CASE("wiki example ringsum", "[ringsum]")
{
    dd::kfdd_manager mgr{};
    auto A = mgr.var(expansion::S);
    auto B = mgr.var(expansion::S);
    auto C = mgr.var(expansion::S);
    auto ringsum = mgr.one() ^ B ^ (A & B) ^ (A & C) ^ (A & B & C);

    dd::kfdd_manager mgr2{};
    auto A2 = mgr2.var(expansion::S);
    auto B2 = mgr2.var(expansion::S);
    auto C2 = mgr2.var(expansion::S);
    auto ringsum2 = mgr2.one() ^ B2 ^ (A2 & B2) ^ (A2 & C2) ^ (A2 & B2 & C2);

    mgr2.dtl_sift();

    eval_dds(ringsum, ringsum2);
}

}