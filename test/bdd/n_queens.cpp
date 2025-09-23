// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>            // TEST_CASE
#include <catch2/generators/catch_generators.hpp>  // GENERATE

#include <freddy/config.hpp>  // var_index
#include <freddy/dd/bdd.hpp>  // bdd_manager

#include <algorithm>  // std::ranges::generate
#include <cassert>    // assert
#include <cstdint>    // std::int32_t
#include <utility>    // std::as_const
#include <vector>     // std::vector

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
    for (auto& row : x)
    {
        std::ranges::generate(row, [&mgr]() { return mgr.var(); });
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

    // Constraint (diagonal): Two queens must not be on the same diagonal (either up-right or down-right).
    auto diag_constr = [n, &mgr, &x = std::as_const(x)](std::int32_t const i, std::int32_t const j, bool const upward) {
        auto res = mgr.one();
        for (auto k = 0; k < n; ++k)
        {
            auto const col = upward ? j + k - i : j + i - k;
            if (col >= 0 && col < n && k != i)
            {
                res &= ~(x[i][j] & x[k][col]);
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
            pred &= diag_constr(i, j, true);
            pred &= diag_constr(i, j, false);

            row_existence |= x[i][j];  // There must be a queen in each row.
        }
        pred &= row_existence;
    }
    return pred;
}

}  // namespace

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("N-Queens solutions are counted", "[example]")
{
    auto const [n, expected] =
        GENERATE(std::pair{1, 1}, std::pair{2, 0}, std::pair{3, 0}, std::pair{4, 2}, std::pair{5, 10});

    bdd_manager mgr{{.init_var_cap = static_cast<var_index>(n * n)}};

    CHECK(queens(n, mgr).sharpsat() == expected);
}
