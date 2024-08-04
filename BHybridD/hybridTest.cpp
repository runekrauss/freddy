#include <freddy/dd/bhd.hpp>
#include "freddy/detail/manager.hpp"
#include <iostream>


using namespace freddy;


int main() {
    dd::bhd_manager bhd = dd::bhd_manager(3, 12);;

    auto constexpr n = 4;  // number of queens

    std::array<std::array<dd::bhd, n>, n> x;
    for (auto i = 0; i < n; ++i)
    {
        for (auto j = 0; j < n; ++j)
        {
            x[i][j] = bhd.var();
        }
    }

    auto pred = bhd.one();
    //std::cout << "pred = 1" << std::endl;
    for (auto i = 0; i < n; ++i)
    {
        //std::cout << "tmp = 0" << std::endl;
        auto tmp = bhd.zero();
        for (auto j = 0; j < n; ++j)
        {
            // two queens must not be in the same row
            for (auto k = 0; k < n; ++k)
            {
                if (k != j)
                {
                    //std::cout << "pred &= ~(x" << i << "-" << j << " & x" << i << "-"<< k << ")" << std::endl;
                    pred &= ~(x[i][j] & x[i][k]);
                }
            }

            // two queens must not be in the same column
            for (auto k = 0; k < n; ++k)
            {
                if (k != i)
                {
                    //std::cout << "pred &= ~(x" << i << "-" << j << " & x" << k << "-"<< j << ")" << std::endl;
                    pred &= ~(x[i][j] & x[k][j]);
                }
            }

            // two queens must not be along an up right diagonal
            for (auto k = 0; k < n; ++k)
            {
                auto const l = j + k - i;
                if (l >= 0 && l < n && k != i)
                {
                    //std::cout << "pred &= ~(x" << i << "-" << j << " & x" << k << "-"<< l << ")" << std::endl;
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // two queens must not be along a down right diagonal
            for (auto k = 0; k < n; ++k)
            {
                auto const l = j + i - k;
                if (l >= 0 && l < n && k != i)
                {
                    //std::cout << "pred &= ~(x" << i << "-" << j << "& x" << k << "-"<< l << ")" << std::endl;
                    pred &= ~(x[i][j] & x[k][l]);
                }
            }

            // there must be a queen in each row globally
            //std::cout << "tmp |= " << "x" << i << "-"<< j << std::endl;
            tmp |= x[i][j];
        }
        //std::cout << "pred &= tmp" << std::endl;
        pred &= tmp;
    }

    pred.print();
    pred.createExpansionFiles();
    return 0;
}