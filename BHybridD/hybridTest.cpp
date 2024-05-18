#include <freddy/dd/bhd.hpp>
#include "freddy/detail/manager.hpp"
#include <iostream>


void printBT();

int main() {
    freddy::dd::bhd_manager bhdM;

    freddy::dd::bhd varA = bhdM.var();
    //freddy::dd::bhd varB = bhdM.var();
    //freddy::dd::bhd varC = bhdM.var();
    //freddy::dd::bhd varD = bhdM.var();
    //freddy::dd::bhd varE = bhdM.var();
    //freddy::dd::bhd varF = bhdM.var();
    auto exp = bhdM.getExp();

    auto pred = varA & exp;

    //auto e = pred & exp;

    pred.print();


    //exp.print();

    return 0;
}