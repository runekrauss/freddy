#include <catch2/catch_test_macros.hpp>

#include <freddy/config.hpp>
#include <freddy/dd/zdd.hpp>

#include <vector>

using namespace freddy;

namespace
{

auto run_cube_set_logic_minimization_example() -> void
{
    // Cube Set Representation:
    //  - cube = product of literals (e.g., xy, x!yz, xz)
    //  - SOP = disjunction (union) of cubes
    //
    // Subsumption / redundancy (teacher example):
    //  - cubes: xy, x!yz, xz
    //  - once xz is present, x!yz is redundant (xz subsumes x!yz)
    //
    // Note:
    //  - we encode negative literals explicitly as separate ZDD variables (~x, ~y, ~z)
    //    so that x!yz becomes {x, ~y, z} and is strictly longer than {x, z}.

    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 6}};

    auto const x  = mgr.var("x");
    auto const nx = mgr.var("~x");
    auto const y  = mgr.var("y");
    auto const ny = mgr.var("~y");
    auto const z  = mgr.var("z");
    auto const nz = mgr.var("~z");

    (void)nx;
    (void)nz;

    // Cubes
    auto const cube_xy   = x & y;        // xy
    auto const cube_xnyz = x & ny & z;   // x!yz
    auto const cube_xz   = x & z;        // xz

    // subsumption - adding xz makes x!yz redundant
    auto const before       = cube_xy | cube_xnyz;
    auto const after        = before | cube_xz;
    auto const expected_min = cube_xy | cube_xz;

    CHECK(after == expected_min);
    CHECK(after.size() <= before.size());

    // Don't care - xz does not depend on y-literals (y / ~y)
    {
        std::vector<bool> as(static_cast<std::size_t>(mgr.var_count()), false);

        for (bool xv : {false, true})
        {
            for (bool zv : {false, true})
            {
                as[0] = xv;  // x
                as[4] = zv;  // z

                as[2] = false; as[3] = false;
                auto const r00 = cube_xz.eval(as);

                as[2] = true;  as[3] = false;
                auto const r10 = cube_xz.eval(as);

                as[2] = false; as[3] = true;
                auto const r01 = cube_xz.eval(as);

                as[2] = true;  as[3] = true;
                auto const r11 = cube_xz.eval(as);

                CHECK(r00 == r10);
                CHECK(r00 == r01);
                CHECK(r00 == r11);
            }
        }
    }

    // Two-level SOP minimization (larger SOP)
    // F = xy + x!yz + xz + y  =>  G = xy + xz + y
    auto const F = cube_xy | cube_xnyz | cube_xz | y;
    auto const G = cube_xy | cube_xz | y;

    CHECK(F == G);
}

}

TEST_CASE("ZDD: two-level logic minimization via cube-set subsumption", "[example]")
{
    run_cube_set_logic_minimization_example();
}