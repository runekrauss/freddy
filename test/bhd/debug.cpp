// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bhd.hpp>  // dd::bhd_manager

#include <iostream>  // std::cout

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("BHD synthesis is performed", "[debug]")
{
    config::dead_factor = 0.75f;
    config::growth_factor = 1.25f;
    config::load_factor = 0.7f;
    config::ct_size = 11;
    config::ut_size = 7;
    config::vl_size = 2;
    dd::bhd_manager mgr;
    auto const x0 = mgr.var();
#ifndef NDEBUG
    std::cout << mgr << std::endl;
    std::cout << x0 << std::endl;
#endif
    SECTION("Complement bit is overwritten")
    {
        auto const f = x0 & mgr.exp();
#ifndef NDEBUG
        f.print();
#endif
        CHECK(f.high() == mgr.exp());
        CHECK(f.low().is_zero());
    }

    SECTION("EXP is complemented")
    {
        auto const f = ~x0 & mgr.exp();

        CHECK(f.high().is_zero());
        CHECK(f.low() == mgr.exp());
    }

    SECTION("EXP maintains its level")
    {
        auto const f = x0 & mgr.exp();
        auto const x1 = mgr.var();

        CHECK((f & x1) == f);
    }
}
