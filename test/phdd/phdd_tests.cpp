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

auto pow2(const int exp, dd::phdd_manager &mgr) -> dd::phdd
{
    auto r = mgr.one();
    auto t = mgr.two();
    auto h = mgr.constant(0.5);
    if(exp >= 0)
    {
        for(int i = 0; i < exp; i++) r *= t;
    }
    else
    {
        for(int i = 0; i > exp; i--) r *= h;
    }
    return r;
}

static auto ite(const dd::phdd &s, const dd::phdd &t, const dd::phdd &f) -> dd::phdd
{
    return (~s | t) & (s | f);
}

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("sphdd_integer", "[phdd, integer]")
{
    // representation of 5bit integer including addition, subtraction, multiplication

    dd::phdd_manager mgr;
    auto x0 = mgr.var(expansion::PD, "x0"); auto y0 = mgr.var(expansion::PD, "y0");
    auto x1 = mgr.var(expansion::PD, "x1"); auto y1 = mgr.var(expansion::PD, "y1");
    auto x2 = mgr.var(expansion::PD, "x2"); auto y2 = mgr.var(expansion::PD, "y2");
    auto x3 = mgr.var(expansion::PD, "x3"); auto y3 = mgr.var(expansion::PD, "y3");
    auto x4 = mgr.var(expansion::PD, "x4"); auto y4 = mgr.var(expansion::PD, "y4");

    auto x = pow2(4,mgr)*x4 + pow2(3,mgr)*x3 + pow2(2,mgr)*x2 + pow2(1,mgr)*x1 + pow2(0,mgr)*x0;
    auto y = pow2(4,mgr)*y4 + pow2(3,mgr)*y3 + pow2(2,mgr)*y2 + pow2(1,mgr)*y1 + pow2(0,mgr)*y0;

    auto sum = x+y;
    auto prod = x*y;
    auto diff = x-y;

    std::ofstream fs1("../phdd_integer.dot");
    mgr.print({x,y, sum, diff, prod}, {"x","y", "x+y","x-y", "x*y"}, fs1);
    fs1.close();

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
    // 1 sign bit, 4 exponent bits (bias = 7), 3 significant bits + hidden bit

    dd::phdd_manager mgr;
    auto x_sg = mgr.var(expansion::S, "x_sg");  auto y_sg = mgr.var(expansion::S, "y_sg");
    auto x_e0 = mgr.var(expansion::S, "x_e0");  auto y_e0 = mgr.var(expansion::S, "y_e0");
    auto x_e1 = mgr.var(expansion::S, "x_e1");  auto y_e1 = mgr.var(expansion::S, "y_e1");
    auto x_e2 = mgr.var(expansion::S, "x_e2");  auto y_e2 = mgr.var(expansion::S, "y_e2");
    auto x_e3 = mgr.var(expansion::S, "x_e3");  auto y_e3 = mgr.var(expansion::S, "y_e3");
    auto x_m0 = mgr.var(expansion::PD, "x_m0"); auto y_m0 = mgr.var(expansion::PD, "y_m0");
    auto x_m1 = mgr.var(expansion::PD, "x_m1"); auto y_m1 = mgr.var(expansion::PD, "y_m1");
    auto x_m2 = mgr.var(expansion::PD, "x_m2"); auto y_m2 = mgr.var(expansion::PD, "y_m2");

    auto x_is_sbn = (~x_e0 & ~x_e1 & ~x_e2 & ~x_e3);
    auto x_s = ite(x_sg, -mgr.one(), mgr.one());
    auto x_m = (pow2(2,mgr)*x_m2) +
               (pow2(1,mgr)*x_m1) +
               (pow2(0,mgr)*x_m0) +
               ite(x_is_sbn, mgr.zero(), pow2(3, mgr));
    auto x_e = (pow2(1<<3, mgr)*x_e3 | ~x_e3) *
               (pow2(1<<2, mgr)*x_e2 | ~x_e2) *
               (pow2(1<<1, mgr)*x_e1 | ~x_e1) *
               (pow2(1<<0, mgr)*x_e0 | ~x_e0) *
               (pow2(-7,mgr));
    auto x = x_s * x_m * x_e;

    auto y_is_sbn = (~y_e0 & ~y_e1 & ~y_e2 & ~y_e3);
    auto y_s = ite(y_sg, -mgr.one(), mgr.one());
    auto y_m = (pow2(2,mgr)*y_m2) +
               (pow2(1,mgr)*y_m1) +
               (pow2(0,mgr)*y_m0) +
               ite(y_is_sbn, mgr.zero(), pow2(3, mgr));
    auto y_e = (pow2(1<<3,mgr)*y_e3 | ~y_e3) *
               (pow2(1<<2,mgr)*y_e2 | ~y_e2) *
               (pow2(1<<1,mgr)*y_e1 | ~y_e1) *
               (pow2(1<<0,mgr)*y_e0 | ~y_e0) *
               (pow2(-7,mgr));
    auto y = y_s * y_m * y_e;

    auto sum = x+y;
    auto diff = x-y;
    auto prod = x*y;

    std::vector<std::pair<std::vector<bool>,float>> minifloat_table;
    for(int const s : {0,1})
    {
        for(int e = 0; e < 16; e++)
        {
            for(int m  = 0; m < 8; m++)
            {
                auto assignment = std::vector<bool>{static_cast<bool>(s)};
                auto e_assignment = int_to_bool_vec(e,4);
                auto m_assignment = int_to_bool_vec(m,3);
                assignment.insert(assignment.end(), e_assignment.begin(), e_assignment.end());
                assignment.insert(assignment.end(), m_assignment.begin(), m_assignment.end());
                auto spec = pow(-1,s) * (e==0 ? m : m+8) * pow(2, e-7);
                minifloat_table.emplace_back(assignment,spec);
            }
        }
    }

    for(const auto &x_entry : minifloat_table)
    {
        for(const auto &y_entry : minifloat_table)
        {
            std::vector<bool> assignment;
            for(unsigned int i=0; i < x_entry.first.size(); i++)
            {
                assignment.push_back(x_entry.first[i]);
                assignment.push_back(y_entry.first[i]);
            }
            REQUIRE(x.eval(assignment) == x_entry.second);
            REQUIRE(y.eval(assignment) == y_entry.second);
            REQUIRE(sum.eval(assignment)  == x_entry.second + y_entry.second);
            REQUIRE(diff.eval(assignment) == x_entry.second - y_entry.second);
            REQUIRE(prod.eval(assignment) == x_entry.second * y_entry.second);
        }
    }
}
