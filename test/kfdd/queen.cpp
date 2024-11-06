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

auto enc_with_dtl(dd::kfdd_manager& mgr, std::vector<expansion>& dtl)
{
    config::dead_factor = 0.75f;
    config::growth_factor = 1.25f;
    config::load_factor = 0.7f;
    config::ct_size = 1013;
    config::ut_size = 101;
    config::vl_size = 16;

    auto constexpr n = 4;  // number of queens
    assert(dtl.size()>=(n^2));

    std::array<std::array<dd::kfdd, n>, n> x;
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = mgr.var(dtl[(i*n)+j]);
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

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd synthesis is performed", "[queen]")
{
    dd::kfdd_manager mgr;
    std::vector<expansion> dtl{};
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    dtl.push_back(expansion::S);
    auto const pred = enc_with_dtl(mgr, dtl);
    pred.print();
    //std::cout << "no. of solutions: " << pred.sharpsat() << "\n";
    pred.dtl_sift(true);
    //mgr.change_expansion_type(0, expansion::S);
    //pred.print();
    //mgr.change_expansion_type(0, expansion::PD);
    pred.print();
    //mgr.change_expansion_type(15,expansion::PD);
    //mgr.change_expansion_type(12, expansion::PD);
    //mgr.change_expansion_type(10, expansion::PD);
    //mgr.change_expansion_type(9, expansion::PD);
    //mgr.change_expansion_type(6, expansion::PD);
    //mgr.change_expansion_type(5, expansion::PD);
    //mgr.change_expansion_type(3, expansion::PD);
    //mgr.change_expansion_type(0, expansion::PD);
    //std::cout << "no. of solutions: " << pred.sharpsat() << "\n";
#ifndef NDEBUG
    //std::cout << mgr << '\n';
    //std::cout << pred << '\n';
    pred.print();
#endif
    const std::vector solution1 = {false,false,true,false,true,false,false,false,false,false,false,true,false,true,false,false};
    const std::vector solution2 = {false,true,false,false,false,false,false,true,true,false,false,false,false,false,true,false};
    CHECK(pred.eval(solution1)==true);
    CHECK(pred.eval(solution2)==true);
    for(int i = 0; i < 65536; i++)
    {
        auto var0 =  !!(i & 0b0000000000000001);
        auto var1 =  !!(i & 0b0000000000000010);
        auto var2 =  !!(i & 0b0000000000000100);
        auto var3 =  !!(i & 0b0000000000001000);
        auto var4 =  !!(i & 0b0000000000010000);
        auto var5 =  !!(i & 0b0000000000100000);
        auto var6 =  !!(i & 0b0000000001000000);
        auto var7 =  !!(i & 0b0000000010000000);
        auto var8 =  !!(i & 0b0000000100000000);
        auto var9 =  !!(i & 0b0000001000000000);
        auto var10 = !!(i & 0b0000010000000000);
        auto var11 = !!(i & 0b0000100000000000);
        auto var12 = !!(i & 0b0001000000000000);
        auto var13 = !!(i & 0b0010000000000000);
        auto var14 = !!(i & 0b0100000000000000);
        auto var15 = !!(i & 0b1000000000000000);
        const std::vector params = {var0,var1,var2,var3,var4,var5,var6,var7,var8,var9,var10,var11,var12,var13,var14,var15};
        if(params==solution1||params==solution2) {continue;}
        CHECK(pred.eval(params) == false);
    }
    //CHECK_FALSE(pred.high().is_complemented());
    //CHECK(pred.high().is_const());
    //CHECK(pred.high().is_zero());
    //CHECK((~pred.high()).is_one());
    //CHECK_FALSE(pred.low().is_const());
    //CHECK(pred.cof(false, true, false, false, false, false, false, true, true, false, false, false, false, false) ==
    //      (mgr.var(14) & ~mgr.var(15)));
    //CHECK(pred.cof(false, false, true, false, true, false, false, false, false, false, false, true, false, true) ==
    //      ~(mgr.var(14) | mgr.var(15)));
    //CHECK(pred.cof(false, true, false, false, false, false, false, true, true, false, false, false, false, false, true)
    //          .same_node(pred.cof(false, false, true, false, true, false, false, false, false, false, false, true,
    //                              false, true, false)));
    //CHECK((pred & mgr.var(1)) == pred.ite(mgr.var(1), mgr.zero()));
    //CHECK((pred | mgr.var(1)) == pred.ite(mgr.one(), mgr.var(1)));
    //CHECK((pred ^ mgr.var(1)) == pred.ite(~mgr.var(1), mgr.var(1)));
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

    CHECK(enc(mgr).sharpsat() == 2);
}
}  // namespace