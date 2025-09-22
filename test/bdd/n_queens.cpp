// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/config.hpp>  // config
#include <freddy/dd/bdd.hpp>  // bdd_manager

#include <cassert>  // assert
#include <cstdint>  // std::int32_t
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

auto queens(std::int32_t const n, bdd_manager& mgr)
{
    assert(n > 0);

    // initialization
    std::vector<std::vector<bdd>> x(n, std::vector<bdd>(n));
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = mgr.var();
        }
    }

    // two queens must not be in the same row
    auto horiz_constr = [n, &mgr, &x](std::int32_t const i, std::int32_t const j) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            if (k != j)
            {
                res &= ~(x[i][j] & x[i][k]);
            }
        }
        return res;
    };

    // two queens must not be in the same column
    auto vert_constr = [n, &mgr, &x](std::int32_t const i, std::int32_t const j) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            if (k != i)
            {
                res &= ~(x[i][j] & x[k][j]);
            }
        }
        return res;
    };

    // two queens must not be on the same up-right diagonal
    auto up_diag_constr = [n, &mgr, &x](std::int32_t const i, std::int32_t const j) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            auto const l = j + k - i;
            if (l >= 0 && l < n && k != i)
            {
                res &= ~(x[i][j] & x[k][l]);
            }
        }
        return res;
    };

    // two queens must not be on the same down-right diagonal
    auto down_diag_constr = [n, &mgr, &x](std::int32_t const i, std::int32_t const j) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            auto const l = j + i - k;
            if (l >= 0 && l < n && k != i)
            {
                res &= ~(x[i][j] & x[k][l]);
            }
        }
        return res;
    };

    auto pred = mgr.one();  // predicate
    for (auto i = 0; i < n; ++i)
    {
        auto row_existence = mgr.zero();
        for (auto j = 0; j < n; ++j)
        {
            pred &= horiz_constr(i, j);
            pred &= vert_constr(i, j);
            pred &= up_diag_constr(i, j);
            pred &= down_diag_constr(i, j);

            // there must be a queen in each row
            row_existence |= x[i][j];
        }
        pred &= row_existence;
    }
    return pred;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("1-Queens is solvable", "[queen]")
{
    bdd_manager mgr{config{.init_var_cap = 1}};

    CHECK(queens(1, mgr).sharpsat() == 1);
}

TEST_CASE("2-Queens is unsolvable", "[queen]")
{
    bdd_manager mgr{{.init_var_cap = 4}};

    CHECK(queens(2, mgr).sharpsat() == 0);
}

TEST_CASE("3-Queens is unsolvable", "[queen]")
{
    bdd_manager mgr{{.init_var_cap = 9}};

    CHECK(queens(3, mgr).sharpsat() == 0);
}

TEST_CASE("4-Queens is solvable", "[queen]")
{
    bdd_manager mgr{{.init_var_cap = 16}};

    CHECK(queens(4, mgr).sharpsat() == 2);
}

TEST_CASE("5-Queens is solvable", "[queen]")
{
    bdd_manager mgr{{.init_var_cap = 25}};

    CHECK(queens(5, mgr).sharpsat() == 10);
}
