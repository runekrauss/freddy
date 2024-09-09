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
                   "4xx + 6xy + 8x + 2yy + 6y + 4\n"
                   "12x 6xy 8y 4"}, fs);
        fs.close();
        REQUIRE(true);
    }
}

TEST_CASE("sphdd_integer", "[phdd, int, addition]")
{
    dd::phdd_manager mgr;
    auto x0 = mgr.var("x0"); auto y0 = mgr.var("y0"); auto p0 = mgr.constant( 1);
    auto x1 = mgr.var("x1"); auto y1 = mgr.var("y1"); auto p1 = mgr.constant( 2);
    auto x2 = mgr.var("x2"); auto y2 = mgr.var("y2"); auto p2 = mgr.constant( 4);
    auto x3 = mgr.var("x3"); auto y3 = mgr.var("y3"); auto p3 = mgr.constant( 8);
    auto x4 = mgr.var("x4"); auto y4 = mgr.var("y4"); auto p4 = mgr.constant(16);

    auto x = p4*x4 + p3*x3 + p2*x2 + p1*x1 + p0*x0;
    auto y = p4*y4 + p3*y3 + p2*y2 + p1*y1 + p0*y0;

    auto sum = x+y;
    auto prod = x*y;

    auto int_to_bool_vec5 = [](int n) -> std::vector<bool>
    {
        std::vector<bool> result;
        for(int bit_mask = 1; bit_mask < 32; bit_mask <<= 1)
        {
            result.push_back((n & bit_mask) != 0);
        }
        return result;
    };

    for(int i = 0; i < 32; i++)
    {
        auto x_assignment= int_to_bool_vec5(i);
        for(int j = 0; j < 32; j++)
        {
            auto y_assignment = int_to_bool_vec5(j);
            std::vector<bool> assignment;
            for(unsigned int k = 0; k < x_assignment.size(); k++)
            {
                assignment.push_back(x_assignment[k]);
                assignment.push_back(y_assignment[k]);
            }
            REQUIRE(x.eval(assignment) == i);
            REQUIRE(y.eval(assignment) == j);
            REQUIRE(sum.eval(assignment) == i+j);
            REQUIRE(prod.eval(assignment) == i*j);
        }
    }
}


TEST_CASE("playground", "")
{

}