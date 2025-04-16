#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>
#include <freddy/dd/kfdd.hpp>  // dd::kfdd

#include <cassert>  // assert
#include <cstdint>  // std::uint8_t
#include <vector>   // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

[[maybe_unused]] inline auto eval_dds(dd::kfdd const& dd1, dd::kfdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    const auto no_vars = static_cast<uint32_t>(dd1.manager().var_count());
    assert(no_vars <= 64);

    const uint64_t no_combs = 1ULL << no_vars;

    for(uint64_t i = 0; i < no_combs; i += skip)
    {
        std::vector<bool> input_vars;
        for(uint32_t j = 0; j < no_vars; j++)
        {
            input_vars.push_back(0 != (i & (1ULL << j)));
        }
        const bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

[[maybe_unused]] inline auto eval_dds(dd::bdd const& dd1, dd::kfdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    const auto no_vars = static_cast<uint32_t>(dd1.manager().var_count());
    assert(no_vars <= 64);

    const uint64_t no_combs = 1ULL << no_vars;

    for(uint64_t i = 0; i < no_combs; i += skip)
    {
        std::vector<bool> input_vars;
        for(uint32_t j = 0; j < no_vars; j++)
        {
            input_vars.push_back((i & (1ULL << j)) != 0u);
        }
        const bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

[[maybe_unused]] inline auto eval_dds(dd::kfdd const& dd1, dd::bdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    const auto no_vars = static_cast<uint32_t>(dd1.manager().var_count());
    assert(no_vars <= 64);

    const uint64_t no_combs = 1ULL << no_vars;

    for(uint64_t i = 0; i < no_combs; i += skip)
    {
        std::vector<bool> input_vars;
        for(uint32_t j = 0; j < no_vars; j++)
        {
            input_vars.push_back(0u != (i & (1ULL << j)));
        }
        const bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

[[maybe_unused]] inline auto eval_dds(dd::bdd const& dd1, dd::bdd const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    const auto no_vars = static_cast<uint32_t>(dd1.manager().var_count());
    assert(no_vars <= 64);

    const uint64_t no_combs = 1ULL << no_vars;

    for(uint64_t i = 0; i < no_combs; i += skip)
    {
        std::vector<bool> input_vars;
        for(uint32_t j = 0; j < no_vars; j++)
        {
            input_vars.push_back(0u != (i & (1ULL << j)));
        }
        const bool result = dd1.eval(input_vars) == dd2.eval(input_vars);
        CHECK(result);
        if (!result)
        {
            return false;
        }
    }
    return true;
}

} //namespace freddy