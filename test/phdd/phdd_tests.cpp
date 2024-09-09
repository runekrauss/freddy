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

static auto int_to_bool_vec(int n, int size) -> std::vector<bool>
{
    std::vector<bool> result;
    for(int bit_mask = 1; bit_mask < (1 << size); bit_mask <<= 1)
    {
        result.push_back((n & bit_mask) != 0);
    }
    return result;
}

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("sphdd_integer", "[phdd, integer]")
{
    // representation of 5bit integer including addition, subtraction, multiplication

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
    auto diff = x-y;

    for(int i = 0; i < 32; i++)
    {
        auto x_assignment= int_to_bool_vec(i,5);
        for(int j = 0; j < 32; j++)
        {
            auto y_assignment = int_to_bool_vec(j,5);
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
            REQUIRE(diff.eval(assignment) == i-j);
        }
    }
}


TEST_CASE("sphdd_minifloat", "[phdd, mini-float]")
{
    // representing a 8bit mini float:
    // 1 sign bit, 4 exponent bits, 3 significant bits
    // no hidden bit, bias = 7

    dd::phdd_manager mgr;
    auto x_sg = mgr.var("x_sg"); //auto y_sg = mgr.var("y_sg");
    auto x_e0 = mgr.var("x_e0"); //auto y_e0 = mgr.var("y_e0");
    auto x_e1 = mgr.var("x_e1"); //auto y_e1 = mgr.var("y_e1");
    auto x_e2 = mgr.var("x_e2"); //auto y_e2 = mgr.var("y_e2");
    auto x_e3 = mgr.var("x_e3"); //auto y_e3 = mgr.var("y_e3");
    auto x_m0 = mgr.var("x_m0"); //auto y_m0 = mgr.var("y_m0");
    auto x_m1 = mgr.var("x_m1"); //auto y_m1 = mgr.var("y_m1");
    auto x_m2 = mgr.var("x_m2"); //auto y_m2 = mgr.var("y_m2");

    auto x_s = (-mgr.two() * x_sg) + mgr.one();
    auto x_m = (mgr.constant(4) * x_m2) +
               (mgr.constant(2) * x_m1) +
               (mgr.constant(1) * x_m0);
    auto x_e = (mgr.constant(255) * x_e3 + mgr.one()) *
               (mgr.constant( 15) * x_e2 + mgr.one()) *
               (mgr.constant(  3) * x_e1 + mgr.one()) *
               (mgr.constant(  1) * x_e0 + mgr.one()) *
               (mgr.constant(0.0078125)); // 2^(-7)
    auto x = x_s * x_m * x_e;

        std::ofstream fs1("../phdd_minifloat.dot");
        mgr.print({x,x_s,x_e,x_m}, {"x","s","e","m"}, fs1);
        fs1.close();


    for(int const s : {0,1})
    {
        for(int e = 0; e < 16; e++)
        {
            for(int m  = 0; m < 8; m++)
            {
                auto assignment = std::vector<bool>{(bool)s};
                auto e_assignment = int_to_bool_vec(e,4);
                auto m_assignment = int_to_bool_vec(m,3);
                for(auto b : e_assignment) assignment.push_back(b);
                for(auto b : m_assignment) assignment.push_back(b);

                auto val = x.eval(assignment);
                auto spec = pow(-1,s) * m * pow(2, e-7);
                assert(val == spec);
            }
        }
    }
}


TEST_CASE("playground", "")
{

}