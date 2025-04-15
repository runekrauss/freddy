// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>   // freddy::bdd::bdd_manager
#include <freddy/dd/kfdd.hpp>  // freddy::kfdd::kfdd_manager

#include <cassert>        // assert
#include <cstdint>        // std::int32_t
#include <fstream>        // std::ifstream
#include <iostream>       // std::cout
#include <vector>

#include "../test/util_blif.cpp"

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Types
// *********************************************************************************************************************



TEST_CASE("kfdd blif c432 synthesis with dtl sifting", "[blif_kfdd]")
{
    test_blif("c432.blif", 10000);
}

TEST_CASE("kfdd blif c432 dtl sifting + correctness", "[blif_kfdd]")
{
    auto blif_name = "c432.blif";
    auto kfdd_blif = load_blif_kfdd(blif_name);
    auto kfdd_blif2 = load_blif_kfdd(blif_name);
    auto bdd_blif = load_blif_bdd(blif_name);
    std::cout << "size pre-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";
    kfdd_blif.mgr.dtl_sift();
    std::cout << "size post-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";

    CHECK(blif_eq(kfdd_blif, kfdd_blif2,65537));
    CHECK(blif_eq(kfdd_blif, bdd_blif,65537));
}

TEST_CASE("kfdd dtl sifting", "[blif_kfdd]")
{
    auto blif_name = "c432.blif";
    auto kfdd_blif = load_blif_kfdd(blif_name);
    kfdd_blif.mgr.dtl_sift();
}

TEST_CASE("kfdd blif c880 synthesis with dtl sifting", "[blif_kfdd]")
{
    test_blif("c880.blif", 20000);
}

TEST_CASE("kfdd blif c880 dtl sifting + correctness", "[blif_kfdd]")
{
    auto blif_name = "c880.blif";
    auto kfdd_blif = load_blif_kfdd(blif_name);
    auto kfdd_blif2 = load_blif_kfdd(blif_name);
    auto bdd_blif = load_blif_bdd(blif_name);
    std::cout << "size pre-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";
    kfdd_blif.mgr.dtl_sift();
    std::cout << "size post-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";

    CHECK(blif_eq(kfdd_blif, kfdd_blif2,8796093022091LL));
    CHECK(blif_eq(kfdd_blif, bdd_blif,8796093022091LL));
}

TEST_CASE("kfdd blif c1355 synthesis with dtl sifting", "[blif_kfdd]")
{
    test_blif("c1355.blif", 110000);
}

TEST_CASE("kfdd blif c1355 dtl sifting + correctness", "[blif_kfdd]")
{
    auto blif_name = "c1355.blif";
    auto kfdd_blif = load_blif_kfdd(blif_name);
    auto kfdd_blif2 = load_blif_kfdd(blif_name);
    auto bdd_blif = load_blif_bdd(blif_name);
    std::cout << "size pre-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";
    kfdd_blif.mgr.dtl_sift();
    std::cout << "size post-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";

    CHECK(blif_eq(kfdd_blif, kfdd_blif2,2097131));
    CHECK(blif_eq(kfdd_blif, bdd_blif,2097131));
}

TEST_CASE("kfdd blif c1908 synthesis with dtl sifting", "[blif_kfdd]")
{
    test_blif("c1908.blif", 20000);
}

TEST_CASE("kfdd blif c1908 dtl sifting + correctness", "[blif_kfdd]")
{
    auto blif_name = "c1908.blif";
    auto kfdd_blif = load_blif_kfdd(blif_name);
    auto kfdd_blif2 = load_blif_kfdd(blif_name);
    auto bdd_blif = load_blif_bdd(blif_name);
    std::cout << "size pre-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";
    kfdd_blif.mgr.dtl_sift();
    std::cout << "size post-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";

    CHECK(blif_eq(kfdd_blif, kfdd_blif2,8171));
    CHECK(blif_eq(kfdd_blif, bdd_blif,8171));
}

TEST_CASE("kfdd blif c2670 synthesis with dtl sifting", "[blif_kfdd]")
{
    test_blif("c2670.blif", 25000);
}

TEST_CASE("kfdd blif c3540 synthesis with dtl sifting", "[blif_kfdd]")
{
    test_blif("c3540.blif", 270000);
}

TEST_CASE("kfdd blif c3540 dtl sifting + correctness", "[blif_kfdd]")
{
    auto blif_name = "c3540.blif";
    auto kfdd_blif = load_blif_kfdd(blif_name);
    auto kfdd_blif2 = load_blif_kfdd(blif_name);
    auto bdd_blif = load_blif_bdd(blif_name);
    std::cout << "size pre-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";
    kfdd_blif.mgr.dtl_sift();
    std::cout << "size post-sifting: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";

    CHECK(blif_eq(kfdd_blif, kfdd_blif2,536870869));
    CHECK(blif_eq(kfdd_blif, bdd_blif,536870869));
}

// TEST_CASE("kfdd blif c5315 synthesis with dtl sifting", "[blif_kfdd]")
// {
//     test_blif("c5315.blif", 10000);
// }

TEST_CASE("kfdd/bdd correctness c432", "[blif]")
{
    auto kfdd_blif = load_blif_kfdd("c432.blif");
    kfdd_blif.mgr.dtl_sift();
    std::cout << "kfdd size after dtl sift: " << kfdd_blif.mgr.size(kfdd_blif.f) << "\n";

    auto bdd_blif = load_blif_bdd("c432.blif");
    std::cout << "bdd size: " << bdd_blif.mgr.size(bdd_blif.f) << "\n";

    blif_eq(kfdd_blif, bdd_blif, 65537);
}

}  // namespace