// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include "freddy/dd/bmd.hpp"
#include "freddy/dd/phdd.hpp"  // *phdd_manager

#include <array>     // std::array
#include <fstream>   // std::ofstream
#include <iostream>  // std::cout

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

// *********************************************************************************************************************
// Functions
// *********************************************************************************************************************

// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("sphdd_constants", "")
{
    dd::phdd_manager mgr;

    auto x = mgr.var("x");
    auto y = mgr.var("y");

    std::ofstream fs("../phdd_constants_1.dot");
    mgr.print({mgr.zero(),mgr.one(), mgr.two()},
              {"zero()", "one()", "two()"},
              fs);
    fs.close();

    std::ofstream fs2("../phdd_constants_2.dot");
    mgr.print({mgr.constant(0),mgr.constant(1),mgr.constant(2),mgr.constant(3),
               mgr.constant(4),mgr.constant(5),mgr.constant(6),mgr.constant(7),
               mgr.constant(8),mgr.constant(9),mgr.constant(10),mgr.constant(11)},
              {"0", "1", "2","3", "4", "5","6", "7", "8","9", "10", "11"},
              fs2);
    fs2.close();
}

TEST_CASE("sphdd_addition", "[xx]")
{
    dd::phdd_manager mgr;

    auto x = mgr.var("x");
    auto y = mgr.var("y");

    std::ofstream fs("../phdd_constants_1.dot");
    mgr.print({mgr.one(),mgr.one(), mgr.two()},
              {"zero()", "one()", "two()"},
              fs);
    fs.close();

    std::ofstream fs2("../phdd_constants_2.dot");
    mgr.print({mgr.constant(0),mgr.constant(1),mgr.constant(2),mgr.constant(3),
               mgr.constant(4),mgr.constant(5),mgr.constant(6),mgr.constant(7),
               mgr.constant(8),mgr.constant(9),mgr.constant(10),mgr.constant(11)},
              {"0", "1", "2","3", "4", "5","6", "7", "8","9", "10", "11"},
              fs2);
    fs2.close();
}