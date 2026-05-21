#include <freddy/config.hpp>
#include <freddy/dd/bdd.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace freddy;

namespace
{

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

auto main(int argc, char** argv) -> int
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::vector<std::string_view> const args(argv, argv + argc);

    if (args.size() < 2 || args.size() > 6)
    {
        std::cerr << "Usage: " << args.at(0)
                  << " <n> [<heap_mem_limit>] [<utable_size_hint>] [<cache_size_hint>] [<init_var_cap>]\n";
        return 1;
    }

    try
    {
        std::int32_t const n = std::stoi(std::string{args.at(1)});

        freddy::config cfg;
        if (args.size() >= 3)
        {
            cfg.heap_mem_limit = std::stoull(std::string{args.at(2)});
        }
        if (args.size() >= 4)
        {
            cfg.utable_size_hint = std::stoull(std::string{args.at(3)});
        }
        if (args.size() >= 5)
        {
            cfg.cache_size_hint = std::stoull(std::string{args.at(4)});
        }
        if (args.size() >= 6)
        {
            cfg.init_var_cap = static_cast<var_index>(std::stoull(std::string{args.at(5)}));
        }

        bdd_manager mgr{cfg};

        auto const start = std::chrono::high_resolution_clock::now();
        auto const res = queens(n, mgr);
        static_cast<void>(res.sharpsat());
        auto const end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> const elapsed = end - start;
        std::cout << elapsed.count() << "\n";
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
