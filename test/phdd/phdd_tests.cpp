// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include "freddy/dd/bmd.hpp"
#include "freddy/dd/phdd.hpp"  // *phdd_manager

#include <array>     // std::array
#include <fstream>   // std::ofstream
#include <iostream>  // std::cout

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Functions
// *********************************************************************************************************************

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("sphdd_constants", "[phdd, constants]")
{
    dd::phdd_manager mgr;

    auto x = mgr.var("x");
    auto y = mgr.var("y");

    SECTION("Constants with own member functions")
    {
        std::ofstream fs("../phdd_constants_1.dot");
        mgr.print({mgr.zero(), mgr.one(), mgr.two()}, {"zero()", "one()", "two()"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Constants over mgr.constant")
    {
        std::ofstream fs2("../phdd_constants_2.dot");
        mgr.print({mgr.constant(0), mgr.constant(1), mgr.constant(2), mgr.constant(3), mgr.constant(4), mgr.constant(5),
                   mgr.constant(6), mgr.constant(7), mgr.constant(8), mgr.constant(9), mgr.constant(10), mgr.constant(11)},
                  {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"}, fs2);
        fs2.close();
        REQUIRE(true);
    }
}

TEST_CASE("sphdd_addition", "[phdd, addition]")
{
    dd::phdd_manager mgr;
    auto x = mgr.var("x");
    auto y = mgr.var("y");

    SECTION("Add up constants")
    {
        std::ofstream fs("../phdd_add_const+consts.dot");
        mgr.print({mgr.constant(18), mgr.constant(14),
                   mgr.constant(18) + mgr.constant(14)},
                  {"18", "14", "18+14"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Add up var and constants")
    {
        std::ofstream fs("../phdd_add_var+consts.dot");
        mgr.print({mgr.zero() + x, mgr.one() + x, mgr.two() + x},
                  {"0+x", "1+x", "2+x"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Add up vars")
    {
        std::ofstream fs("../phdd_add_var+var.dot");
        mgr.print({x + x, y + x, y + y}, {"x+x", "y+x", "y+y"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Add linear function 3x+5y+5")
    {
        auto f = x + x + x + y + y + y + y + y + mgr.constant(5);

        std::ofstream fs("../phdd_add_lin.dot");
        mgr.print({f}, {"3x+5y+5"}, fs);
        fs.close();
        REQUIRE(true);
    }
}

TEST_CASE("sphdd_multiplication", "[phdd, multiplication]")
{
    dd::phdd_manager mgr;
    auto x = mgr.var("x");
    auto y = mgr.var("y");

    SECTION("Multiply constants")
    {
        std::ofstream fs("../phdd_mult_const*consts.dot");
        mgr.print({ mgr.constant(6) * mgr.constant(3),
                    mgr.constant(8) * mgr.constant(8),
                    mgr.constant(3) * mgr.constant(3)},
                  {"6*3", "8*8", "3*3"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Multiply constants with vars")
    {
        std::ofstream fs("../phdd_mult_const*vars.dot");
        mgr.print({mgr.constant(6) * x,
                   mgr.constant(16) * x,
                   mgr.constant(3) * x},
                  {"6x", "16x", "3x"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Multiply vars with vars")
    {
        std::ofstream fs("../phdd_mult_vars*vars.dot");
        mgr.print({x * x,
                   y * x,
                   y * y},
                  {"xx", "xy", "yy"}, fs);
        fs.close();
        REQUIRE(true);
    }

    SECTION("Multiply polynomials")
    {
        auto f1 = (mgr.two() * x) + (mgr.one() * y) + mgr.two();

        auto f2 = (mgr.two() * x) + (mgr.two() * y) + mgr.two();

        std::ofstream fs("../phdd_mult_poly.dot");
        mgr.print({f1,
                   f2,
                   f1 * f2},
                  {"2x+y+2", "2x+2y+2",
                   "4xx + 6xy + 8x + 2yy + 6y + 4\n 12x 6xy 8y 4"}, fs);
        fs.close();
        REQUIRE(true);
    }

}

TEST_CASE("playground", "")
{

}