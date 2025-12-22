// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/kfdd.hpp>  // dd::kfdd
#include <freddy/expansion.hpp>
#include <freddy/util.hpp>

#include <cassert>   // assert
#include <cstdint>   // std::uint8_t
#include <vector>    // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

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
                if (auto const l = j + k - i; l >= 0 && l < n && k != i)
                {
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // two queens must not be along a down right diagonal
            for (auto k = 0; k < n; ++k)
            {
                if (auto const l = j + i - k; l >= 0 && l < n && k != i)
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

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd 1-Queens is solvable", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(1, mgr).sharpsat() == 1);
}

TEST_CASE("kfdd 2-Queens is unsolvable", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(2, mgr).sharpsat() == 0);
}

TEST_CASE("kfdd 3-Queens is unsolvable", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(3, mgr).sharpsat() == 0);
}

TEST_CASE("kfdd 4-Queens is solvable", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(4, mgr).sharpsat() == 2);
}

TEST_CASE("kfdd 5-Queens is solvable", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(5, mgr).sharpsat() == 10);
}

TEST_CASE("kfdd 4-Queens dtl sifts correctly", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;
    auto const queens1 = enc(4, mgr);

    dd::kfdd_manager mgr2;
    auto const queens2 = enc(4, mgr2);

    mgr2.dtl_sift();

    CHECK(eval_dds(queens1, queens2));
}

TEST_CASE("kfdd 5-Queens dtl sifts correctly", "[kfdd][queen]")
{
    dd::kfdd_manager mgr;
    auto const queens1 = enc(5, mgr);

    dd::kfdd_manager mgr2;
    auto const queens2 = enc(5, mgr2);

    mgr2.dtl_sift();

    CHECK(eval_dds(queens1, queens2));
}
