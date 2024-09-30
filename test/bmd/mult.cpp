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

namespace
{

// =====================================================================================================================
// Functions
// =====================================================================================================================

auto bit_lvl_impl(dd::bmd_manager& mgr)
{
    auto const a1 = mgr.var("a1");
    auto const b1 = mgr.var("b1");
    auto const a0 = mgr.var("a0");
    auto const b0 = mgr.var("b0");

    // symbolic simulation
    std::array<dd::bmd, 8> s;
    s[0] = a0 & b1;
    s[1] = a0 & b0;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[0] ^ s[2];
    s[5] = s[0] & s[2];
    s[6] = s[3] ^ s[5];
    s[7] = s[3] & s[5];

    return std::vector<dd::bmd>{s[1], s[4], s[6], s[7]};
}

auto word_lvl_spec(dd::bmd_manager& mgr)
{
    std::vector<dd::bmd> a(2);
    std::vector<dd::bmd> b(2);

    a[1] = (mgr.var_count() > 0) ? mgr.var(0) : mgr.var("a1");
    b[1] = (mgr.var_count() > 1) ? mgr.var(1) : mgr.var("b1");
    a[0] = (mgr.var_count() > 2) ? mgr.var(2) : mgr.var("a0");
    b[0] = (mgr.var_count() > 3) ? mgr.var(3) : mgr.var("b0");

    return mgr.weighted_sum(a) * mgr.weighted_sum(b);
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("2-bit multiplier is verified", "[mult]")
{
    dd::bmd_manager mgr;
    auto const p = bit_lvl_impl(mgr);

    CHECK(mgr.weighted_sum(p) == word_lvl_spec(mgr));
}
