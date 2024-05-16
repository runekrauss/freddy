#include <eddy/dd/bhd.hpp>
#include "eddy/detail/manager.hpp"
#include <iostream>


void printBT();

int main() {
    eddy::dd::bhd_manager mgr;



    eddy::dd::bhd varA = mgr.var();
    eddy::dd::bhd varB = mgr.var();
    eddy::dd::bhd varC = mgr.var();
    eddy::dd::bhd varD = mgr.var();
    eddy::dd::bhd varE = mgr.var();
    eddy::dd::bhd varF = mgr.var();


    //auto pred = varA | varB | varC | varD;
    //auto pred = varA & varB | varC & varD | varE & varF;

    pred.print();

    return 0;
}