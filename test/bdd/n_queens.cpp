// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>            // TEST_CASE
#include <catch2/generators/catch_generators.hpp>  // GENERATE

#include <freddy/config.hpp>  // var_index
#include <freddy/dd/bdd.hpp>  // bdd_manager

#include <cassert>  // assert
#include <cstdint>  // std::int32_t
#include <utility>  // std::as_const
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

    // initialize nxn chessboard of BDD variables
    std::vector<std::vector<bdd>> x(n, std::vector<bdd>(n));
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = mgr.var();
        }
    }

    // Constraint (horizontal): Two queens must not be in the same row.
    auto horiz_constr = [n, &mgr, &x = std::as_const(x)](std::int32_t const i, std::int32_t const j) {
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

    // Constraint (vertical): Two queens must not be in the same column.
    auto vert_constr = [n, &mgr, &x = std::as_const(x)](std::int32_t const i, std::int32_t const j) {
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

    // Constraint (up-right diagonal): Two queens must not be on the same up-right diagonal.
    auto up_diag_constr = [n, &mgr, &x = std::as_const(x)](std::int32_t const i, std::int32_t const j) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            auto const d = j + k - i;
            if (d >= 0 && d < n && k != i)
            {
                res &= ~(x[i][j] & x[k][d]);
            }
        }
        return res;
    };

    // Constraint (down-right diagonal): Two queens must not be on the same down-right diagonal.
    auto down_diag_constr = [n, &mgr, &x = std::as_const(x)](std::int32_t const i, std::int32_t const j) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            auto const d = j + i - k;
            if (d >= 0 && d < n && k != i)
            {
                res &= ~(x[i][j] & x[k][d]);
            }
        }
        return res;
    };

    // build predicate encoding the n-queens requirements
    auto pred = mgr.one();
    for (auto i = 0; i < n; ++i)
    {
        auto row_existence = mgr.zero();
        for (auto j = 0; j < n; ++j)
        {
            pred &= horiz_constr(i, j);
            pred &= vert_constr(i, j);
            pred &= up_diag_constr(i, j);
            pred &= down_diag_constr(i, j);

            row_existence |= x[i][j];  // there must be a queen in each row
        }
        pred &= row_existence;
    }
    return pred;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("n-Queens solutions are counted", "[queen]")
{
    auto const [n, expected] =
        GENERATE(std::pair{1, 1}, std::pair{2, 0}, std::pair{3, 0}, std::pair{4, 2}, std::pair{5, 10});

    bdd_manager mgr{{.init_var_cap = static_cast<var_index>(n * n)}};

    CHECK(queens(n, mgr).sharpsat() == expected);
}
