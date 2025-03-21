// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/kfdd.hpp>  // dd::kfdd

#include <cassert>  // assert
#include <cstdint>  // std::uint8_t
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

auto enc(std::uint8_t const n, dd::kfdd_manager& mgr)
{
    assert(n > 0);

    std::vector<std::vector<dd::kfdd>> x(n, std::vector<dd::kfdd>(n));
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = mgr.var(expansion::S);
        }
    }

    auto pred = mgr.one();
    for (auto i = 0; i < n; ++i)
    {
        auto tmp = mgr.zero();
        for (auto j = 0; j < n; ++j)
        {
            // two queens must not be in the same row
            for (auto k = 0; k < n; ++k)
            {
                if (k != j)
                {
                    pred &= ~(x[i][j] & x[i][k]);
                }
            }

            // two queens must not be in the same column
            for (auto k = 0; k < n; ++k)
            {
                if (k != i)
                {
                    pred &= ~(x[i][j] & x[k][j]);
                }
            }

            // two queens must not be along an up right diagonal
            for (auto k = 0; k < n; ++k)
            {
                auto const l = j + k - i;
                if (l >= 0 && l < n && k != i)
                {
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // two queens must not be along a down right diagonal
            for (auto k = 0; k < n; ++k)
            {
                auto const l = j + i - k;
                if (l >= 0 && l < n && k != i)
                {
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // there must be a queen in each row globally
            tmp |= x[i][j];
        }
        pred &= tmp;
    }
    return pred;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("1-Queens is solvable", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(1, mgr).sharpsat() == 1);
}

TEST_CASE("2-Queens is unsolvable", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(2, mgr).sharpsat() == 0);
}

TEST_CASE("3-Queens is unsolvable", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(3, mgr).sharpsat() == 0);
}

TEST_CASE("4-Queens is solvable", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(4, mgr).sharpsat() == 2);
}

TEST_CASE("5-Queens is solvable", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(5, mgr).sharpsat() == 10);
}

TEST_CASE("4-Queens dtl sifts correctly", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;
    auto queens1 = enc(4, mgr);

    dd::kfdd_manager mgr2;
    auto queens2 = enc(4, mgr2);

    mgr2.dtl_sift();


    auto const noVars = 16;
    auto const noCombs = 2 << noVars;

    for(int i = 0; i < noCombs; i++)
    {
        std::vector<bool> input_vars;
        for(int j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (2 << j)));
        }
        CHECK(queens1.eval(input_vars) == queens2.eval(input_vars));
    }

    std::cout << "queens1:" << "\n";
    queens1.print();
    std::cout << "queens2:" << "\n";
    queens2.print();

}

TEST_CASE("5-Queens dtl sifts correctly", "[kfdd_queen]")
{
    dd::kfdd_manager mgr;
    auto queens1 = enc(5, mgr);

    dd::kfdd_manager mgr2;
    auto queens2 = enc(5, mgr2);

    mgr2.dtl_sift();


    auto const noVars = 25;
    auto const noCombs = 2 << noVars;

    for(int i = 0; i < noCombs; i++)
    {
        std::vector<bool> input_vars;
        for(int j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (2 << j)));
        }
        CHECK(queens1.eval(input_vars) == queens2.eval(input_vars));
    }

}
