//
// Created by marvin on 09.10.24.
//
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include "freddy/config.hpp"

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
    auto x10 = mgr.var(expansion::S);
    auto x11 = mgr.var(expansion::S);
    auto x12 = mgr.var(expansion::S);
    auto x13 = mgr.var(expansion::S);
    auto x14 = mgr.var(expansion::S);
    auto x15 = mgr.var(expansion::S);
    auto x16 = mgr.var(expansion::S);
    auto x17 = mgr.var(expansion::S);
    auto x18 = mgr.var(expansion::S);
    auto x19 = mgr.var(expansion::S);
    auto x20 = mgr.var(expansion::S);
    auto x21 = mgr.var(expansion::S);
    auto x22 = mgr.var(expansion::S);
    auto x23 = mgr.var(expansion::S);
    auto x24 = mgr.var(expansion::S);
    auto x25 = mgr.var(expansion::S);
    auto x26 = mgr.var(expansion::S);
    auto x27 = mgr.var(expansion::S);
    auto x28 = mgr.var(expansion::S);
    auto x29 = mgr.var(expansion::S);
    auto x30 = mgr.var(expansion::S);
    auto x31 = mgr.var(expansion::S);
    auto result = (x18 & x17 & x9 & x23 & x21 & x27 & x31 & x27 & x0 & x25) ^ (x31 & x17 & x15 & x14 & x28 & x21 & x8 & x3 & x12 & x7) ^ (x30 & x24 & x11 & x1 & x9 & x25 & x11 & x2 & x24 & x30) ^ (x31 & x6 & x8 & x11 & x31 & x24 & x3 & x12 & x2 & x19) ^ (x26 & x10 & x25 & x12 & x3 & x3 & x20 & x3 & x1 & x15) ^ (x20 & x10 & x2 & x24 & x26 & x11 & x20 & x16 & x24 & x5) ^ (x9 & x30 & x14 & x24 & x21 & x19 & x9 & x4 & x28 & x12) ^ (x25 & x16 & x1 & x18 & x22 & x23 & x11 & x26 & x12 & x29) ^ (x31 & x4 & x27 & x28 & x27 & x29 & x10 & x21 & x4 & x8) ^ (x7 & x23 & x14 & x26 & x29 & x6 & x10 & x16 & x15 & x6) ^ (x10 & x2 & x25 & x18 & x2 & x7 & x1 & x6 & x20 & x25) ^ (x22 & x23 & x22 & x22 & x3 & x2 & x28 & x24 & x29 & x25) ^ (x1 & x27 & x28 & x6 & x0 & x27 & x19 & x26 & x23 & x2) ^ (x0 & x25 & x23 & x21 & x14 & x17 & x15 & x7 & x4 & x28) ^ (x2 & x7 & x29 & x14 & x18 & x11 & x16 & x12 & x31 & x4) ^ (x0 & x10 & x2 & x10 & x27 & x16 & x8 & x31 & x25 & x11) ^ (x24 & x20 & x2 & x31 & x8 & x24 & x21 & x24 & x25 & x2) ^ (x6 & x25 & x28 & x12 & x13 & x21 & x30 & x18 & x0 & x21) ^ (x15 & x18 & x9 & x13 & x17 & x8 & x0 & x5 & x1 & x3) ^ (x12 & x11 & x22 & x10 & x8 & x22 & x26 & x28 & x24 & x30);
    //auto result = (x3 & x5 & x4 & x2 & x7 & x2 & x5 & x7 & x6 & x1) ^ (x4 & x9 & x1 & x1 & x1 & x0 & x6 & x4 & x0 & x3) ^ (x7 & x2 & x7 & x5 & x1 & x7 & x1 & x4 & x3 & x0) ^ (x7 & x8 & x0 & x7 & x7 & x3 & x6 & x9 & x1 & x5) ^ (x6 & x4 & x6 & x4 & x8 & x4 & x6 & x1 & x2 & x8) ^ (x6 & x3 & x5 & x9 & x8 & x1 & x6 & x5 & x4 & x1) ^ (x7 & x8 & x4 & x6 & x4 & x5 & x9 & x4 & x3 & x3) ^ (x4 & x3 & x5 & x7 & x6 & x2 & x5 & x3 & x3 & x8) ^ (x3 & x6 & x9 & x4 & x0 & x9 & x7 & x9 & x5 & x4) ^ (x8 & x3 & x9 & x0 & x9 & x8 & x1 & x0 & x8 & x3);
    std::cout << result.size() << '\n';
    result.print();
    result.dtl_sift();
    std::cout << result.size() << '\n';
    result.print();


}
}