// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/bmd.hpp>  // bmd_manager

#include <array>   // std::array
#include <vector>  // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("1-bit multiplier is verified", "[example]")
{
    bmd_manager mgr{config{.init_var_cap = 2}};
    auto const a = mgr.var("a");
    auto const b = mgr.var("b");

    auto const word_lvl_spec = mgr.unsigned_bin({a}) * mgr.unsigned_bin({b});

    std::vector const bit_lvl_impl{a & b, mgr.zero()};  // B^2n -> B^2n

    CHECK(mgr.unsigned_bin(bit_lvl_impl) == word_lvl_spec);
}

TEST_CASE("2-bit multiplier is verified", "[example]")
{
    bmd_manager mgr{{.init_var_cap = 4}};
    auto const a1 = mgr.var("a1");
    auto const a0 = mgr.var("a0");  // a1a0
    auto const b1 = mgr.var("b1");
    auto const b0 = mgr.var("b0");  // b1b0

    auto const word_lvl_spec = mgr.unsigned_bin({a0, a1}) * mgr.unsigned_bin({b0, b1});

    // symbolic simulation
    std::array<bmd, 8> s;
    s[0] = a0 & b0;  // p0
    s[1] = a0 & b1;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[1] ^ s[2];  // p1
    s[5] = s[1] & s[2];
    s[6] = s[3] ^ s[5];  // p2
    s[7] = s[3] & s[5];  // p3

    std::vector const bit_lvl_impl{s[0], s[4], s[6], s[7]};  // LSB...MSB

    CHECK(mgr.unsigned_bin(bit_lvl_impl) == word_lvl_spec);
}
