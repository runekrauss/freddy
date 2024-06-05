#include <freddy/dd/bhd.hpp>
#include "freddy/detail/manager.hpp"
#include <iostream>


void printBT();

int main() {
    freddy::dd::bhd_manager bhdM = freddy::dd::bhd_manager(1);

    //freddy::dd::bhd varA = bhdM.var();
    //freddy::dd::bhd varB = bhdM.var();
    //freddy::dd::bhd varC = bhdM.var();
    //freddy::dd::bhd varD = bhdM.var();
    //freddy::dd::bhd varE = bhdM.var();
    //freddy::dd::bhd varF = bhdM.var();

    //auto pred = varA & varB;
    //pred.print();


    //pred = pred & varC;

    //std::cout << "fertig" << std::endl;

    //auto a = pred & varB;




    auto constexpr n = 5;  // number of queens

    std::array<std::array<freddy::dd::bhd, n>, n> x;
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = bhdM.var();
        }
    }

    auto pred = bhdM.one();
    for (auto i = 0; i < n; ++i)
    {
        auto tmp = bhdM.zero();
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

    pred.print();

    pred.createExpansionFiles();


    //exp.print();

    return 0;
}