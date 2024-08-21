// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include "freddy/config.hpp"

#include <freddy/dd/kfdd.hpp>  // dd::kfdd_manager

#include <array>     // std::array

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Functions
// *********************************************************************************************************************
namespace {
auto enc(dd::kfdd_manager& mgr)
{
    config::dead_factor = 0.75f;
    config::growth_factor = 1.25f;
    config::load_factor = 0.7f;
    config::ct_size = 1013;
    config::ut_size = 101;
    config::vl_size = 16;

    auto constexpr n = 4;  // number of queens

    std::array<std::array<dd::kfdd, n>, n> x;
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = mgr.var(expansion::PD);
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

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd synthesis is performed", "[queen]")
{
    dd::kfdd_manager mgr;
    auto const pred = enc(mgr);
#ifndef NDEBUG
    //std::cout << mgr << '\n';
    //std::cout << pred << '\n';
    //pred.print();
#endif
    CHECK_FALSE(pred.high().is_complemented());
    CHECK(pred.high().is_const());
    CHECK(pred.high().is_zero());
    CHECK((~pred.high()).is_one());
    CHECK_FALSE(pred.low().is_const());
    CHECK(pred.cof(false, true, false, false, false, false, false, true, true, false, false, false, false, false) ==
          (mgr.var(14) & ~mgr.var(15)));
    CHECK(pred.cof(false, false, true, false, true, false, false, false, false, false, false, true, false, true) ==
          ~(mgr.var(14) | mgr.var(15)));
    CHECK(pred.cof(false, true, false, false, false, false, false, true, true, false, false, false, false, false, true)
              .same_node(pred.cof(false, false, true, false, true, false, false, false, false, false, false, true,
                                  false, true, false)));
    CHECK((pred & mgr.var(1)) == pred.ite(mgr.var(1), mgr.zero()));
    CHECK((pred | mgr.var(1)) == pred.ite(mgr.one(), mgr.var(1)));
    CHECK((pred ^ mgr.var(1)) == pred.ite(~mgr.var(1), mgr.var(1)));
}

TEST_CASE("kfdd character can be investigated", "[queen]")
{
    dd::kfdd_manager mgr;
    auto const pred = enc(mgr);

    SECTION("Variables are supported")
    {
        CHECK(mgr.var_count() == 16);
    }

    SECTION("Number of constants is determined")
    {
        CHECK(mgr.const_count() == 1);
    }

    SECTION("Number of nodes is determined")
    {
        CHECK(mgr.node_count() <= 440);
    }

    SECTION("Number of edges is determined")
    {
        CHECK(mgr.edge_count() <= 688);
    }

    SECTION("Nodes are counted")
    {
        CHECK(pred.size() == 30);
    }

    SECTION("Longest path is computed")
    {
        CHECK(pred.depth() == 16);
    }

    SECTION("Number of paths is computed")
    {
        CHECK(pred.path_count() == 31);
    }

    SECTION("Assignments are evaluated")
    {
        CHECK(pred.eval({false, true, false, false, false, false, false, true, true, false, false, false, false, false,
                         true, false}));
        CHECK_FALSE(pred.eval({false, true, false, false, false, false, false, true, true, false, false, false, false,
                               false, true, true}));
    }

    SECTION("Constant is found")
    {
        CHECK(pred.has_const(false));
        CHECK_FALSE(pred.has_const(true));
    }

    SECTION("Essential variables are identifiable")
    {
        CHECK(pred.is_essential(0));
        CHECK(pred.is_essential(1));
        CHECK(pred.is_essential(15));
    }
}

TEST_CASE("kfdd substitution can be made", "[queen]")
{
    dd::kfdd_manager mgr;
    auto const pred = enc(mgr);

    SECTION("Variable is replaced by function")
    {
        auto const x16 = mgr.var(expansion::PD);
        auto const x17 = mgr.var(expansion::PD);
        auto const f = pred.compose(15, x16 & x17);

        CHECK_FALSE(f.is_essential(15));
        CHECK(f.cof(false, false, true, false, true, false, false, false, false, false, false, true, false, true,
                    false) == ~(x16 & x17));
    }

    SECTION("Variable is restricted to constant value")
    {
        CHECK(pred.restr(10, true).is_zero());
        CHECK(pred.restr(10, false).cof(false, false, true, false, true, false, false, false, false, false).var() ==
              11);
    }

    SECTION("Variable is removed by existential quantification")
    {
        auto const f = pred.exist(2);

        CHECK_FALSE(f.is_essential(2));
        CHECK(f.cof(false, false).var() == 3);
        CHECK(f.cof(false, true).var() == 3);
    }

    SECTION("Variable is removed by universal quantification")
    {
        CHECK(pred.forall(2).is_zero());
    }
}

TEST_CASE("kfdd variable order is changeable", "[queen]")
{
    dd::kfdd_manager mgr;
    auto const pred = enc(mgr);

    SECTION("Levels can be swapped")
    {
        mgr.swap(15, 0);

        CHECK(pred.var() == 15);
        CHECK(pred.high().is_zero());
        CHECK(pred.cof(false, true, false, false, false, false, false, true, true, false, false, false, false, false) ==
              (mgr.var(14) & ~mgr.var(0)));
        CHECK(pred.cof(false, false, true, false, true, false, false, false, false, false, false, true, false, true) ==
              ~(mgr.var(14) | mgr.var(0)));
        CHECK(pred.eval({false, true, false, false, false, false, false, true, true, false, false, false, false, false,
                         true, false}));
    }

    SECTION("Variable reordering finds a minimum")
    {
        auto const ncnt_old = pred.size();
        mgr.reorder();

        CHECK(ncnt_old > pred.size());
        CHECK(pred.var() == 1);
        CHECK(pred.low().var() == 2);
        CHECK(pred.cof(false, true).var() == 4);
        CHECK(pred.cof(false, true, true).var() == 7);
        CHECK(pred.cof(false, true, true, false).var() == 8);
        CHECK(pred.cof(false, true, true, false, false).var() == 11);
        CHECK(pred.cof(false, true, true, false, false, true).var() == 13);
        CHECK(pred.cof(false, true, true, false, false, true, true).var() == 14);
        CHECK(pred.cof(false, true, true, false, false, true, true, false).var() == 12);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false).var() == 10);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false, false).var() == 9);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false, false, false).var() == 6);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false, false, false, false).var() == 5);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false, false, false, false, false).var() ==
              3);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false, false, false, false, false, false)
                  .var() == 0);
        CHECK(pred.cof(false, true, true, false, false, true, true, false, false, false, false, false, false, false,
                       false)
                  .var() == 15);
        CHECK(pred.cof(false, true, true, false, false, true, true, false)
                  .same_node(pred.cof(true, false, false, true, true, false, false, true)));
        CHECK(pred.eval({false, true, false, false, false, false, false, true, true, false, false, false, false, false,
                         true, false}));
        CHECK_FALSE(pred.eval({false, true, false, false, false, false, false, true, true, false, false, false, false,
                               false, true, true}));
    }
}

TEST_CASE("kfdd SAT analysis is performed", "[queen]")
{
    dd::kfdd_manager mgr;

    CHECK(enc(mgr).satcount() == 2);
}
}  // namespace