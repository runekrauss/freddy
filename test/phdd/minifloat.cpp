// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE
#include "freddy/dd/phdd.hpp"  // *phdd_manager
#include <array>     // std::array

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

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("evaluate mini float (representation and operations + - *)", "[phdd, mini-float]")
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
    auto x_s = x_sg.ite(-mgr.one(), mgr.one());
    auto x_m = (pow2(2,mgr)*x_m2) +
               (pow2(1,mgr)*x_m1) +
               (pow2(0,mgr)*x_m0) +
                x_is_sbn.ite(mgr.zero(), pow2(3, mgr));
    auto x_e = (pow2(1<<3, mgr)*x_e3 | ~x_e3) *
               (pow2(1<<2, mgr)*x_e2 | ~x_e2) *
               (pow2(1<<1, mgr)*x_e1 | ~x_e1) *
               (pow2(1<<0, mgr)*x_e0 | ~x_e0) *
               (pow2(-7,mgr)) *
                x_is_sbn.ite(mgr.two(), mgr.one());
    auto x = x_s * x_m * x_e;

    auto y_is_sbn = (~y_e0 & ~y_e1 & ~y_e2 & ~y_e3);
    auto y_s = y_sg.ite(-mgr.one(), mgr.one());
    auto y_m = (pow2(2,mgr)*y_m2) +
               (pow2(1,mgr)*y_m1) +
               (pow2(0,mgr)*y_m0) +
                y_is_sbn.ite(mgr.zero(), pow2(3, mgr));
    auto y_e = (pow2(1<<3,mgr)*y_e3 | ~y_e3) *
               (pow2(1<<2,mgr)*y_e2 | ~y_e2) *
               (pow2(1<<1,mgr)*y_e1 | ~y_e1) *
               (pow2(1<<0,mgr)*y_e0 | ~y_e0) *
               (pow2(-7,mgr)) *
                y_is_sbn.ite(mgr.two(), mgr.one());
    auto y = y_s * y_m * y_e;

    auto sum = x+y;
    auto diff = x-y;
    auto prod = x*y;

    // make a table with all possible mini-floats (bit representation and float value)
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
                auto spec = pow(-1,s) * (e == 0 ? m << 1 : m + 8) * pow(2, e-7);
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
            {   // interleaved var order
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
