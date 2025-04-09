#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>
#include <freddy/dd/kfdd.hpp>  // dd::kfdd

#include <cassert>  // assert
#include <cstdint>  // std::uint8_t
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

using namespace freddy;

namespace
{

[[maybe_unused]] auto eval_dds(dd::kfdd const& dd1, dd::kfdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    int32_t noVars = dd1.manager().var_count();
    assert(noVars <= 64);

    uint64_t noCombs = 1 << noVars;

    for(uint64_t i = 0; i < noCombs; i += skip)
    {
        std::vector<bool> input_vars;
        for(auto j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (1 << j)));
        }
        bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

[[maybe_unused]] auto eval_dds(dd::bdd const& dd1, dd::kfdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    int32_t noVars = dd1.manager().var_count();
    assert(noVars <= 64);

    uint64_t noCombs = 1 << noVars;

    for(uint64_t i = 0; i < noCombs; i += skip)
    {
        std::vector<bool> input_vars;
        for(auto j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (1 << j)));
        }
        bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

[[maybe_unused]] auto eval_dds(dd::kfdd const& dd1, dd::bdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    int32_t noVars = dd1.manager().var_count();
    assert(noVars <= 64);

    uint64_t noCombs = 1 << noVars;

    for(uint64_t i = 0; i < noCombs; i += skip)
    {
        std::vector<bool> input_vars;
        for(auto j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (1 << j)));
        }
        bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

[[maybe_unused]] auto eval_dds(dd::bdd const& dd1, dd::bdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    int32_t noVars = dd1.manager().var_count();
    assert(noVars <= 64);

    uint64_t noCombs = 1 << noVars;

    for(uint64_t i = 0; i < noCombs; i += skip)
    {
        std::vector<bool> input_vars;
        for(auto j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (1 << j)));
        }
        bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

}