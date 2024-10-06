// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/kfdd.hpp>  // dd::kfdd_manager

#include <iostream>  // std::cout

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Macros
// *********************************************************************************************************************

TEST_CASE("kfdd normal test", "[normal]")
{

    dd::kfdd_manager mgr_normal1;
    auto var1 = mgr_normal1.var(expansion::PD);
    auto var2 = mgr_normal1.var(expansion::PD);
    auto negvar1 = ~var1;
    auto negvar2 = ~var2;
    auto mixed = negvar1 & negvar2;


    std::cout << "var1: " << "\n";
    var1.print();

    std::cout << "var2: " << "\n";
    var2.print();


    std::cout << "~var1: " << "\n";
    negvar1.print();

    std::cout << "~var2: " << "\n";
    negvar2.print();

    std::cout << "~var1 & ~var2: " << "\n";
    mixed.print();

    CHECK(mixed.eval({false,false}) == true);
    CHECK(mixed.eval({false,true}) == false);
    CHECK(mixed.eval({true,false}) == false);
    CHECK(mixed.eval({true,true}) == false);


}

}  //namespace