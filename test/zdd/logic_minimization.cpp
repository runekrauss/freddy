#include <catch2/catch_test_macros.hpp>

#include <freddy/config.hpp>
#include <freddy/dd/zdd.hpp>

#include <cstddef>
#include <vector>

using namespace freddy;

namespace
{

auto check_dont_care_invariant(zdd const& cube_xz, bool const xv, bool const zv, std::size_t const num_vars) -> void
{
    std::vector<bool> as(num_vars, false);
    as[0] = xv;  // x
    as[4] = zv;  // z

    as[2] = false;
    as[3] = false;
    auto const r00 = cube_xz.eval(as);

    as[2] = true;
    as[3] = false;
    auto const r10 = cube_xz.eval(as);

    as[2] = false;
    as[3] = true;
    auto const r01 = cube_xz.eval(as);

    as[2] = true;
    as[3] = true;
    auto const r11 = cube_xz.eval(as);

    CHECK(r00 == r10);
    CHECK(r00 == r01);
    CHECK(r00 == r11);
}

auto check_dont_care(zdd const& cube_xz, std::size_t const num_vars) -> void
{
    for (bool const xv : {false, true})
    {
        for (bool const zv : {false, true})
        {
            check_dont_care_invariant(cube_xz, xv, zv, num_vars);
        }
    }
}

auto run_subsumption_check() -> void
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

    auto const x = mgr.var("x");
    (void)mgr.var("~x");
    auto const y = mgr.var("y");
    auto const ny = mgr.var("~y");
    auto const z = mgr.var("z");
    (void)mgr.var("~z");

    auto const cube_xy = x & y;         // xy
    auto const cube_xnyz = x & ny & z;  // x!yz
    auto const cube_xz = x & z;         // xz

    auto const before = cube_xy | cube_xnyz;
    auto const after = before | cube_xz;
    auto const expected_min = cube_xy | cube_xz;

    CHECK(after == expected_min);
    CHECK(after.size() <= before.size());

    // Don't care - xz does not depend on y-literals (y / ~y)
    auto const num_vars = static_cast<std::size_t>(mgr.var_count());
    check_dont_care(cube_xz, num_vars);
}

auto run_sop_minimization() -> void
{
    // Two-level SOP minimization (larger SOP)
    // f = xy + x!yz + xz + y  =>  g = xy + xz + y
    zdd_manager mgr{config{.utable_size_hint = 25, .cache_size_hint = 3'359, .init_var_cap = 6}};

    auto const x = mgr.var("x");
    (void)mgr.var("~x");
    auto const y = mgr.var("y");
    auto const ny = mgr.var("~y");
    auto const z = mgr.var("z");
    (void)mgr.var("~z");

    auto const cube_xy = x & y;
    auto const cube_xnyz = x & ny & z;
    auto const cube_xz = x & z;

    auto const f = cube_xy | cube_xnyz | cube_xz | y;
    auto const g = cube_xy | cube_xz | y;

    CHECK(f == g);
}

}  // namespace

TEST_CASE("ZDD: cube-set subsumption and don't-care check", "[example]")
{
    run_subsumption_check();
}

TEST_CASE("ZDD: two-level SOP minimization", "[example]")
{
    run_sop_minimization();
}
