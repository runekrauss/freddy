#include <freddy/dd/bhd.hpp>
#include "freddy/detail/manager.hpp"
#include <iostream>


void printBT();

int main() {
    freddy::dd::bhd_manager bhdM;

    freddy::dd::bhd varA = bhdM.var();
    freddy::dd::bhd varB = bhdM.var();

    //freddy::dd::bhd varE = bhdM.var();
    //freddy::dd::bhd varF = bhdM.var();
    auto exp = bhdM.getExp();


    freddy::dd::bhd varC = bhdM.var();
    freddy::dd::bhd varD = bhdM.var();

    auto pred = ~varA | exp;


    std::cout << "fertig" << std::endl;

    auto a = pred & varB;

    pred.print();


    //exp.print();

    return 0;
}