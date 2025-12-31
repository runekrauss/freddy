// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/phdd.hpp>    // phdd_manager
#include <freddy/expansion.hpp>  // expansion::pD

#include <cmath>    // std::pow
#include <utility>  // std::pair
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{

// =====================================================================================================================
// Functions
// =====================================================================================================================

auto int_to_bool_vec(int const n, int const size)
{
    std::vector<bool> res;
    for (auto bit_mask = 1u; bit_mask < (1u << static_cast<unsigned>(size)); bit_mask <<= 1u)
    {
        res.push_back((static_cast<unsigned>(n) & bit_mask) != 0);
    }
    return res;
}

auto pow2(int const exp, phdd_manager& mgr)
{
    auto r = mgr.one();
    auto const t = mgr.two();
    auto const h = mgr.constant(0.5);
    if (exp >= 0)
    {
        for (int i = 0; i < exp; i++)
        {
            r *= t;
        }
    }
    else
    {
        for (int i = 0; i > exp; i--)
        {
            r *= h;
        }
    }
    return r;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

// NOLINTBEGIN(readability-function-cognitive-complexity)
TEST_CASE("evaluate mini float (representation and operations + - *)", "[example]")
{
    // representing a 8bit mini float:
    // 1 sign bit, 4 exponent bits (bias = 7), 3 significant bits + hidden bit

    phdd_manager mgr;
    auto const x_sg = mgr.var(expansion::S, "x_sg");
    auto const y_sg = mgr.var(expansion::S, "y_sg");
    auto const x_e0 = mgr.var(expansion::S, "x_e0");
    auto const y_e0 = mgr.var(expansion::S, "y_e0");
    auto const x_e1 = mgr.var(expansion::S, "x_e1");
    auto const y_e1 = mgr.var(expansion::S, "y_e1");
    auto const x_e2 = mgr.var(expansion::S, "x_e2");
    auto const y_e2 = mgr.var(expansion::S, "y_e2");
    auto const x_e3 = mgr.var(expansion::S, "x_e3");
    auto const y_e3 = mgr.var(expansion::S, "y_e3");
    auto const x_m0 = mgr.var(expansion::pD, "x_m0");
    auto const y_m0 = mgr.var(expansion::pD, "y_m0");
    auto const x_m1 = mgr.var(expansion::pD, "x_m1");
    auto const y_m1 = mgr.var(expansion::pD, "y_m1");
    auto const x_m2 = mgr.var(expansion::pD, "x_m2");
    auto const y_m2 = mgr.var(expansion::pD, "y_m2");

    auto const x_is_sbn = (~x_e0 & ~x_e1 & ~x_e2 & ~x_e3);
    auto const x_s = x_sg.ite(-mgr.one(), mgr.one());
    auto const x_m =
        (pow2(2, mgr) * x_m2) + (pow2(1, mgr) * x_m1) + (pow2(0, mgr) * x_m0) + x_is_sbn.ite(mgr.zero(), pow2(3, mgr));
    auto const x_e = (pow2(1u << 3u, mgr) * x_e3 | ~x_e3) * (pow2(1u << 2u, mgr) * x_e2 | ~x_e2) *
                     (pow2(1u << 1u, mgr) * x_e1 | ~x_e1) * (pow2(1u << 0u, mgr) * x_e0 | ~x_e0) * (pow2(-7, mgr)) *
                     x_is_sbn.ite(mgr.two(), mgr.one());
    auto const x = x_s * x_m * x_e;

    auto const y_is_sbn = (~y_e0 & ~y_e1 & ~y_e2 & ~y_e3);
    auto const y_s = y_sg.ite(-mgr.one(), mgr.one());
    auto const y_m =
        (pow2(2, mgr) * y_m2) + (pow2(1, mgr) * y_m1) + (pow2(0, mgr) * y_m0) + y_is_sbn.ite(mgr.zero(), pow2(3, mgr));
    auto const y_e = (pow2(1u << 3u, mgr) * y_e3 | ~y_e3) * (pow2(1u << 2u, mgr) * y_e2 | ~y_e2) *
                     (pow2(1u << 1u, mgr) * y_e1 | ~y_e1) * (pow2(1u << 0u, mgr) * y_e0 | ~y_e0) * (pow2(-7, mgr)) *
                     y_is_sbn.ite(mgr.two(), mgr.one());
    auto const y = y_s * y_m * y_e;

    auto const sum = x + y;
    auto const diff = x - y;
    auto const prod = x * y;

    // make a table with all possible mini-floats (bit representation and float value)
    std::vector<std::pair<std::vector<bool>, float>> minifloat_table;
    for (int const s : {0, 1})
    {
        for (int e = 0; e < 16; e++)
        {
            for (int m = 0; m < 8; m++)
            {
                auto assignment = std::vector<bool>{static_cast<bool>(s)};
                auto e_assignment = int_to_bool_vec(e, 4);
                auto m_assignment = int_to_bool_vec(m, 3);
                assignment.insert(assignment.end(), e_assignment.begin(), e_assignment.end());
                assignment.insert(assignment.end(), m_assignment.begin(), m_assignment.end());
                auto spec = std::pow(-1, s) * (e == 0 ? static_cast<unsigned>(m) << 1u : static_cast<unsigned>(m) + 8) *
                            std::pow(2, e - 7);
                minifloat_table.emplace_back(assignment, spec);
            }
        }
    }

    for (auto const& x_entry : minifloat_table)
    {
        for (const auto& y_entry : minifloat_table)
        {
            std::vector<bool> assignment;
            for (unsigned int i = 0; i < x_entry.first.size(); i++)
            {  // interleaved var order
                assignment.push_back(x_entry.first[i]);
                assignment.push_back(y_entry.first[i]);
            }
            CHECK(x.eval(assignment) == x_entry.second);
            CHECK(y.eval(assignment) == y_entry.second);
            CHECK(sum.eval(assignment) == x_entry.second + y_entry.second);
            CHECK(diff.eval(assignment) == x_entry.second - y_entry.second);
            CHECK(prod.eval(assignment) == x_entry.second * y_entry.second);
        }
    }
}
// NOLINTEND(readability-function-cognitive-complexity)
