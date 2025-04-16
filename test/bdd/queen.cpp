// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>  // dd::bdd

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

auto enc(std::uint8_t const n, dd::bdd_manager& mgr)
{
    assert(n > 0);

    std::vector<std::vector<dd::bdd>> x(n, std::vector<dd::bdd>(n));
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = mgr.var();
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

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("1-Queens is solvable", "[queen]")
{
    dd::bdd_manager mgr;

    CHECK(enc(1, mgr).sharpsat() == 1);
}

TEST_CASE("2-Queens is unsolvable", "[queen]")
{
    dd::bdd_manager mgr;

    CHECK(enc(2, mgr).sharpsat() == 0);
}

TEST_CASE("3-Queens is unsolvable", "[queen]")
{
    dd::bdd_manager mgr;

    CHECK(enc(3, mgr).sharpsat() == 0);
}

TEST_CASE("4-Queens is solvable", "[queen]")
{
    dd::bdd_manager mgr;

    CHECK(enc(4, mgr).sharpsat() == 2);
}

TEST_CASE("5-Queens is solvable", "[queen]")
{
    dd::bdd_manager mgr;

    CHECK(enc(5, mgr).sharpsat() == 10);
}
