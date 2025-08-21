//
// Created by marvin on 09.10.24.
//
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include "freddy/expansion.hpp"

#include <freddy/dd/kfdd.hpp>  // dd::kfdd_manager

#include <freddy/util.hpp>
#include <vector>
// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

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

    auto const result_c = (c[18] & c[17] & c[9] & c[23] & c[21] & c[27] & c[31] & c[27] & c[0] & c[25]) ^ (c[31] & c[17] & c[15] & c[14] & c[28] & c[21] & c[8] & c[3] & c[12] & c[7]) ^ (c[30] & c[24] & c[11] & c[1] & c[9] & c[25] & c[11] & c[2] & c[24] & c[30]) ^ (c[31] & c[6] & c[8] & c[11] & c[31] & c[24] & c[3] & c[12] & c[2] & c[19]) ^ (c[26] & c[10] & c[25] & c[12] & c[3] & c[3] & c[20] & c[3] & c[1] & c[15]) ^ (c[20] & c[10] & c[2] & c[24] & c[26] & c[11] & c[20] & c[16] & c[24] & c[5]) ^ (c[9] & c[30] & c[14] & c[24] & c[21] & c[19] & c[9] & c[4] & c[28] & c[12]) ^ (c[25] & c[16] & c[1] & c[18] & c[22] & c[23] & c[11] & c[26] & c[12] & c[29]) ^ (c[31] & c[4] & c[27] & c[28] & c[27] & c[29] & c[10] & c[21] & c[4] & c[8]) ^ (c[7] & c[23] & c[14] & c[26] & c[29] & c[6] & c[10] & c[16] & c[15] & c[6]) ^ (c[10] & c[2] & c[25] & c[18] & c[2] & c[7] & c[1] & c[6] & c[20] & c[25]) ^ (c[22] & c[23] & c[22] & c[22] & c[3] & c[2] & c[28] & c[24] & c[29] & c[25]) ^ (c[1] & c[27] & c[28] & c[6] & c[0] & c[27] & c[19] & c[26] & c[23] & c[2]) ^ (c[0] & c[25] & c[23] & c[21] & c[14] & c[17] & c[15] & c[7] & c[4] & c[28]) ^ (c[2] & c[7] & c[29] & c[14] & c[18] & c[11] & c[16] & c[12] & c[31] & c[4]) ^ (c[0] & c[10] & c[2] & c[10] & c[27] & c[16] & c[8] & c[31] & c[25] & c[11]) ^ (c[24] & c[20] & c[2] & c[31] & c[8] & c[24] & c[21] & c[24] & c[25] & c[2]) ^ (c[6] & c[25] & c[28] & c[12] & c[13] & c[21] & c[30] & c[18] & c[0] & c[21]) ^ (c[15] & c[18] & c[9] & c[13] & c[17] & c[8] & c[0] & c[5] & c[1] & c[3]) ^ (c[12] & c[11] & c[22] & c[10] & c[8] & c[22] & c[26] & c[28] & c[24] & c[30]);
    auto const result_v = (v[18] & v[17] & v[9] & v[23] & v[21] & v[27] & v[31] & v[27] & v[0] & v[25]) ^ (v[31] & v[17] & v[15] & v[14] & v[28] & v[21] & v[8] & v[3] & v[12] & v[7]) ^ (v[30] & v[24] & v[11] & v[1] & v[9] & v[25] & v[11] & v[2] & v[24] & v[30]) ^ (v[31] & v[6] & v[8] & v[11] & v[31] & v[24] & v[3] & v[12] & v[2] & v[19]) ^ (v[26] & v[10] & v[25] & v[12] & v[3] & v[3] & v[20] & v[3] & v[1] & v[15]) ^ (v[20] & v[10] & v[2] & v[24] & v[26] & v[11] & v[20] & v[16] & v[24] & v[5]) ^ (v[9] & v[30] & v[14] & v[24] & v[21] & v[19] & v[9] & v[4] & v[28] & v[12]) ^ (v[25] & v[16] & v[1] & v[18] & v[22] & v[23] & v[11] & v[26] & v[12] & v[29]) ^ (v[31] & v[4] & v[27] & v[28] & v[27] & v[29] & v[10] & v[21] & v[4] & v[8]) ^ (v[7] & v[23] & v[14] & v[26] & v[29] & v[6] & v[10] & v[16] & v[15] & v[6]) ^ (v[10] & v[2] & v[25] & v[18] & v[2] & v[7] & v[1] & v[6] & v[20] & v[25]) ^ (v[22] & v[23] & v[22] & v[22] & v[3] & v[2] & v[28] & v[24] & v[29] & v[25]) ^ (v[1] & v[27] & v[28] & v[6] & v[0] & v[27] & v[19] & v[26] & v[23] & v[2]) ^ (v[0] & v[25] & v[23] & v[21] & v[14] & v[17] & v[15] & v[7] & v[4] & v[28]) ^ (v[2] & v[7] & v[29] & v[14] & v[18] & v[11] & v[16] & v[12] & v[31] & v[4]) ^ (v[0] & v[10] & v[2] & v[10] & v[27] & v[16] & v[8] & v[31] & v[25] & v[11]) ^ (v[24] & v[20] & v[2] & v[31] & v[8] & v[24] & v[21] & v[24] & v[25] & v[2]) ^ (v[6] & v[25] & v[28] & v[12] & v[13] & v[21] & v[30] & v[18] & v[0] & v[21]) ^ (v[15] & v[18] & v[9] & v[13] & v[17] & v[8] & v[0] & v[5] & v[1] & v[3]) ^ (v[12] & v[11] & v[22] & v[10] & v[8] & v[22] & v[26] & v[28] & v[24] & v[30]);

    mgr.dtl_sift();

    eval_dds(result_v, result_c);
}

TEST_CASE("simple ringsum", "[ringsum]")
{
    dd::kfdd_manager mgr1{};
    auto const x0 = mgr1.var(expansion::S);
    auto const x1 = mgr1.var(expansion::S);
    auto const x2 = mgr1.var(expansion::S);
    auto const x3 = mgr1.var(expansion::S);
    auto const x4 = mgr1.var(expansion::S);
    auto const ringsum = mgr1.one() ^ (x3 & x0) ^ (x4 & x0) ^ (x0 & x1);

    dd::kfdd_manager mgr2{};
    auto const y0 = mgr2.var(expansion::S);
    auto const y1 = mgr2.var(expansion::S);
    auto const y2 = mgr2.var(expansion::S);
    auto const y3 = mgr2.var(expansion::S);
    auto const y4 = mgr2.var(expansion::S);
    auto const ringsum2 = mgr2.one() ^ (y3 & y0) ^ (y4 & y0) ^ (y0 & y1);

    mgr2.dtl_sift();

    eval_dds(ringsum, ringsum2);
}

TEST_CASE("wiki example ringsum", "[ringsum]")
{
    dd::kfdd_manager mgr{};
    const auto a1 = mgr.var(expansion::S);
    const auto b1 = mgr.var(expansion::S);
    const auto c1 = mgr.var(expansion::S);
    const auto ringsum1 = mgr.one() ^ b1 ^ (a1 & b1) ^ (a1 & c1) ^ (a1 & b1 & c1);

    dd::kfdd_manager mgr2{};
    auto const a2 = mgr2.var(expansion::S);
    auto const b2 = mgr2.var(expansion::S);
    auto const c2 = mgr2.var(expansion::S);
    auto const ringsum2 = mgr2.one() ^ b2 ^ (a2 & b2) ^ (a2 & c2) ^ (a2 & b2 & c2);

    mgr2.dtl_sift();

    eval_dds(ringsum1, ringsum2);
}
