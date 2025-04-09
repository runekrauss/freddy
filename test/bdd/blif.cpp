//
// Created by marvi on 13.03.2025.
//
//
// Created by marvin on 09.10.24.
//
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>  // freddy::bdd::bdd_manager

#include <cassert>        // assert
#include <fstream>        // std::ifstream

#include "../test/util_blif.cpp"

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Types
// *********************************************************************************************************************

TEST_CASE("bdd blif c432 dtl sifting", "[blif]")
{
    test_blif("c432.blif", 10000);
}

TEST_CASE("bdd blif c880 dtl sifting", "[blif]")
{
    test_blif("c880.blif", 20000);
}

TEST_CASE("bdd blif c1355 dtl sifting", "[blif]")
{
    test_blif("c1355.blif", 2000);
}

TEST_CASE("bdd blif c1908 dtl sifting", "[blif]")
{
    test_blif("c1908.blif", 20000);
}

TEST_CASE("bdd blif c2670 dtl sifting", "[blif]")
{
    test_blif("c2670.blif", 25000);
}

TEST_CASE("bdd blif c3540 dtl sifting", "[blif]")
{
    test_blif("c3540.blif", 270000);
}

TEST_CASE("bdd blif c5315 dtl sifting", "[blif]")
{
    test_blif("c5315.blif", 10000);
}

}  // namespace