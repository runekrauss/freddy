// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bmd.hpp>  // dd::bmd

#include <array>   // std::array
#include <vector>  // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("1-bit multiplier is verified", "[mult]")
{
    dd::bmd_manager mgr;
    auto const a = mgr.var("a");
    auto const b = mgr.var("b");

    auto const word_lvl_spec = mgr.weighted_sum({a}) * mgr.weighted_sum({b});

    std::vector const bit_lvl_impl{a & b};

    CHECK(mgr.weighted_sum(bit_lvl_impl) == word_lvl_spec);
}

TEST_CASE("2-bit multiplier is verified", "[mult]")
{
    dd::bmd_manager mgr;
    auto const a1 = mgr.var("a1");
    auto const a0 = mgr.var("a0");  // a1a0
    auto const b1 = mgr.var("b1");
    auto const b0 = mgr.var("b0");  // b1b0

    auto const word_lvl_spec = mgr.weighted_sum({a0, a1}) * mgr.weighted_sum({b0, b1});

    // symbolic simulation
    std::array<dd::bmd, 8> s;
    s[0] = a0 & b0;  // p0
    s[1] = a0 & b1;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[1] ^ s[2];  // p1
    s[5] = s[1] & s[2];
    s[6] = s[3] ^ s[5];  // p2
    s[7] = s[3] & s[5];  // p3
    std::vector const bit_lvl_impl{s[0], s[4], s[6], s[7]};  // LSB...MSB

    CHECK(mgr.weighted_sum(bit_lvl_impl) == word_lvl_spec);
}
