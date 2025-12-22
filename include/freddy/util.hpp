#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <cassert>   // assert
#include <cstdint>   // std::uint32_t, std::uint64_t
#include <vector>    // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Functions
// =====================================================================================================================

/// @brief Evaluates two DDs for equivalence by comparing all possible input combinations.
/// @tparam DD1 Type of the first decision diagram
/// @tparam DD2 Type of the second decision diagram
/// @param dd1 First decision diagram
/// @param dd2 Second decision diagram
/// @param skip Skip factor for input combinations (default 1 = check all)
/// @return true if both DDs produce the same output for all (checked) input combinations
template <typename DD1, typename DD2>
[[maybe_unused]] inline auto eval_dds(DD1 const& dd1, DD2 const& dd2, int skip = 1) -> bool
{
    assert(dd1.manager().var_count() == dd2.manager().var_count());

    auto const no_vars = static_cast<std::uint32_t>(dd1.manager().var_count());
    assert(no_vars <= 64);

    std::uint64_t const no_combs = 1ULL << no_vars;

    for (std::uint64_t i = 0; i < no_combs; i += skip)
    {
        std::vector<bool> input_vars;
        for (std::uint32_t j = 0; j < no_vars; j++)
        {
            input_vars.push_back((i & (1ULL << j)) != 0u);
        }
        if (dd1.eval(input_vars) != dd2.eval(input_vars))
        {
            return false;
        }
    }
    return true;
}

}  // namespace freddy
