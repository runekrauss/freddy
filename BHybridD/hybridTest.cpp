#include <freddy/dd/bhd.hpp>
#include "freddy/detail/manager.hpp"
#include <iostream>


void printBT();

int main() {
    freddy::dd::bhd_manager bhdM = freddy::dd::bhd_manager(3, 11);

    //funktionieren nicht: 5,11


    /*
    freddy::dd::bhd varA = bhdM.var();
    freddy::dd::bhd varB = bhdM.var();
    freddy::dd::bhd varC = bhdM.var();
    freddy::dd::bhd varD = bhdM.var();
    freddy::dd::bhd varE = bhdM.var();
    freddy::dd::bhd varF = bhdM.var();

    auto pred = varE & varF;
    pred = pred & varB;
    pred = pred & varD;
    pred = pred & varA;
    */


    //pred = pred & varC;

    //std::cout << "fertig" << std::endl;

    //auto a = pred & varB;

    /*

    auto constexpr n = 4;  // number of queens

    std::array<std::array<freddy::dd::bhd, n>, n> x;
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = bhdM.var();
        }
    }

    auto pred = bhdM.one();
    std::cout << "pred = 1" << std::endl;
    for (auto i = 0; i < n; ++i)
    {
        std::cout << "tmp = 0" << std::endl;
        auto tmp = bhdM.zero();
        for (auto j = 0; j < n; ++j)
        {
            // two queens must not be in the same row
            for (auto k = 0; k < n; ++k)
            {
                if (k != j)
                {
                    std::cout << "pred &= ~(x" << i << "-" << j << " & x" << i << "-"<< k << ")" << std::endl;
                    pred &= ~(x[i][j] & x[i][k]);
                }
            }

            // two queens must not be in the same column
            for (auto k = 0; k < n; ++k)
            {
                if (k != i)
                {
                    std::cout << "pred &= ~(x" << i << "-" << j << " & x" << k << "-"<< j << ")" << std::endl;
                    pred &= ~(x[i][j] & x[k][j]);
                }
            }

            // two queens must not be along an up right diagonal
            for (auto k = 0; k < n; ++k)
            {
                auto const l = j + k - i;
                if (l >= 0 && l < n && k != i)
                {
                    std::cout << "pred &= ~(x" << i << "-" << j << " & x" << k << "-"<< l << ")" << std::endl;
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // two queens must not be along a down right diagonal
            for (auto k = 0; k < n; ++k)
            {
                auto const l = j + i - k;
                if (l >= 0 && l < n && k != i)
                {
                    std::cout << "pred &= ~(x" << i << "-" << j << "& x" << k << "-"<< l << ")" << std::endl;
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // there must be a queen in each row globally
            std::cout << "tmp |= " << "x" << i << "-"<< j << std::endl;
            tmp |= x[i][j];
        }
        std::cout << "pred &= tmp" << std::endl;
        pred &= tmp;
    }
    */



    freddy::dd::bhd var1 = bhdM.var();
    freddy::dd::bhd var2 = bhdM.var();
    freddy::dd::bhd var3 = bhdM.var();
    freddy::dd::bhd var4 = bhdM.var();
    freddy::dd::bhd var5 = bhdM.var();
    freddy::dd::bhd var6 = bhdM.var();
    freddy::dd::bhd var7 = bhdM.var();
    freddy::dd::bhd var8 = bhdM.var();
    freddy::dd::bhd var9 = bhdM.var();
    freddy::dd::bhd var10 = bhdM.var();
    freddy::dd::bhd var11 = bhdM.var();
    freddy::dd::bhd var12 = bhdM.var();
    freddy::dd::bhd var13 = bhdM.var();
    freddy::dd::bhd var14 = bhdM.var();
    freddy::dd::bhd var15 = bhdM.var();
    freddy::dd::bhd var16 = bhdM.var();

    auto pred = bhdM.one();
    auto tmp = bhdM.zero();

    pred &= ~(var1 & var2);
    pred &= ~(var1 & var3);
    pred &= ~(var1 & var4);
    pred &= ~(var1 & var5);
    pred &= ~(var1 & var9);
    pred &= ~(var1 & var13);
    pred &= ~(var1 & var6);
    pred &= ~(var1 & var11);
    pred &= ~(var1 & var16);
    tmp |= var1;
    pred &= ~(var2 & var1);
    pred &= ~(var2 & var3);
    pred &= ~(var2 & var4);
    pred &= ~(var2 & var6);
    pred &= ~(var2 & var10);
    pred &= ~(var2 & var14);
    pred &= ~(var2 & var7);
    pred &= ~(var2 & var12);
    pred &= ~(var2 & var5);
    tmp |= var2;

    pred &= ~(var3 & var1);
    pred &= ~(var3 & var2);
    pred &= ~(var3 & var4);
    pred &= ~(var3 & var7);
    pred &= ~(var3 & var11);
    pred &= ~(var3 & var15);
    pred &= ~(var3 & var8);
    pred &= ~(var3 & var6);
    pred &= ~(var3 & var9);
    tmp |= var3;
    pred &= ~(var4 & var1);
    pred &= ~(var4 & var2);
    pred &= ~(var4 & var3);
    pred &= ~(var4 & var8);
    pred &= ~(var4 & var12);
    pred &= ~(var4 & var16);
    pred &= ~(var4 & var7);
    pred &= ~(var4 & var10);
    pred &= ~(var4 & var13);
    tmp |= var4;
    pred &= tmp;
    tmp = bhdM.zero();
    pred &= ~(var5 & var6);
    pred &= ~(var5 & var7);
    pred &= ~(var5 & var8);
    pred &= ~(var5 & var1);
    pred &= ~(var5 & var9);
    pred &= ~(var5 & var13);
    pred &= ~(var5 & var10);
    pred &= ~(var5 & var15);
    pred &= ~(var5 & var2);

    tmp |= var5;
    pred &= ~(var6 & var5);
    pred &= ~(var6 & var7);
    pred &= ~(var6 & var8);
    pred &= ~(var6 & var2);
    pred &= ~(var6 & var10);
    pred &= ~(var6 & var14);
    pred &= ~(var6 & var1);
    pred &= ~(var6 & var11);
    pred &= ~(var6 & var16);
    pred &= ~(var6 & var3);
    pred &= ~(var6 & var9);
    tmp |= var6;
    pred &= ~(var7 & var5);
    pred &= ~(var7 & var6);
    pred &= ~(var7 & var8);
    pred &= ~(var7 & var3);
    pred &= ~(var7 & var11);
    pred &= ~(var7 & var15);
    pred &= ~(var7 & var2);
    pred &= ~(var7 & var12);
    pred &= ~(var7 & var4);
    pred &= ~(var7 & var10);
    pred &= ~(var7 & var13);
    tmp |= var7;
    pred &= ~(var8 & var5);
    pred &= ~(var8 & var6);
    pred &= ~(var8 & var7);
    pred &= ~(var8 & var4);
    pred &= ~(var8 & var12);
    pred &= ~(var8 & var16);
    pred &= ~(var8 & var3);
    pred &= ~(var8 & var11);
    pred &= ~(var8 & var14);
    tmp |= var8;
    pred &= tmp;
    tmp = bhdM.zero();
    pred &= ~(var9 & var10);
    pred &= ~(var9 & var11);
    pred &= ~(var9 & var12);
    pred &= ~(var9 & var1);
    pred &= ~(var9 & var5);
    pred &= ~(var9 & var13);
    pred &= ~(var9 & var14);
    pred &= ~(var9 & var3);
    pred &= ~(var9 & var6);
    tmp |= var9;
    pred &= ~(var10 & var9);
    pred &= ~(var10 & var11);
    pred &= ~(var10 & var12);
    pred &= ~(var10 & var2);
    pred &= ~(var10 & var6);
    pred &= ~(var10 & var14);
    pred &= ~(var10 & var5);
    pred &= ~(var10 & var15);
    pred &= ~(var10 & var4);
    pred &= ~(var10 & var7);
    pred &= ~(var10 & var13);
    tmp |= var10;
    pred &= ~(var11 & var9);
    pred &= ~(var11 & var10);
    pred &= ~(var11 & var12);
    pred &= ~(var11 & var3);
    pred &= ~(var11 & var7);
    pred &= ~(var11 & var15);
    pred &= ~(var11 & var1);
    pred &= ~(var11 & var6);
    pred &= ~(var11 & var16);
    pred &= ~(var11 & var8);
    pred &= ~(var11 & var14);
    tmp |= var11;
    pred &= ~(var12 & var9);
    pred &= ~(var12 & var10);
    pred &= ~(var12 & var11);
    pred &= ~(var12 & var4);
    pred &= ~(var12 & var8);
    pred &= ~(var12 & var16);
    pred &= ~(var12 & var2);
    pred &= ~(var12 & var7);
    pred &= ~(var12 & var15);
    tmp |= var12;
    pred &= tmp;
    tmp = bhdM.zero();
    pred &= ~(var13 & var14);
    pred &= ~(var13 & var15);
    pred &= ~(var13 & var16);
    pred &= ~(var13 & var1);
    pred &= ~(var13 & var5);
    pred &= ~(var13 & var9);
    pred &= ~(var13 & var4);
    pred &= ~(var13 & var7);
    pred &= ~(var13 & var10);
    tmp |= var13;
    pred &= ~(var14 & var13);
    pred &= ~(var14 & var15);
    pred &= ~(var14 & var16);
    pred &= ~(var14 & var2);
    pred &= ~(var14 & var6);
    pred &= ~(var14 & var10);
    //funktioniert
    auto srs = ~(var14 & var9);
    pred.print();
    srs.print();
    pred &= srs;
    //pred.print();
    //falsch
    pred &= ~(var14 & var8);
    pred &= ~(var14 & var11);
    tmp |= var14;
    pred &= ~(var15 & var13);
    pred &= ~(var15 & var14);
    pred &= ~(var15 & var16);
    pred &= ~(var15 & var3);
    pred &= ~(var15 & var7);
    pred &= ~(var15 & var11);
    pred &= ~(var15 & var5);
    pred &= ~(var15 & var10);
    pred &= ~(var15 & var12);
    tmp |= var15;
    pred &= ~(var16 & var13);
    pred &= ~(var16 & var14);
    pred &= ~(var16 & var15);
    pred &= ~(var16 & var4);
    pred &= ~(var16 & var8);
    pred &= ~(var16 & var12);
    pred &= ~(var16 & var1);
    pred &= ~(var16 & var6);
    pred &= ~(var16 & var11);
    tmp |= var16;
    pred &= tmp;




    //auto xlxl = ~(var8 & var12);
    //xlxl.print();

    //pred.print();

    //pred.createExpansionFiles();

    return 0;
}