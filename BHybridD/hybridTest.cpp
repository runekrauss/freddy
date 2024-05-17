#include <freddy/dd/bhd.hpp>
#include "freddy/detail/manager.hpp"
#include <iostream>


void printBT();

int main() {
    freddy::dd::bhd_manager mgr;



    freddy::dd::bhd varA = mgr.var();
    freddy::dd::bhd varB = mgr.var();
    freddy::dd::bhd varC = mgr.var();
    freddy::dd::bhd varD = mgr.var();
    freddy::dd::bhd varE = mgr.var();
    freddy::dd::bhd varF = mgr.var();


    //auto pred = varA | varB | varC | varD;
    //auto pred = varA & varB | varC & varD | varE & varF;

    auto pred = varA & varB;

    pred.print();

    return 0;
}