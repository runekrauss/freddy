#pragma once

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::config
{

// =====================================================================================================================
// Variables
// =====================================================================================================================

// ---------------------------------------------------------------------------------------------------------------------
// Sizes
// ---------------------------------------------------------------------------------------------------------------------

auto inline ct_size = 262147;  // minimum capacity of the computed table (operation cache)

auto inline ut_size = 257;  // minimum capacity of a unique table (per DD level)

auto inline vl_size = 32;  // minimum capacity of the variable list

// ---------------------------------------------------------------------------------------------------------------------
// Factors
// ---------------------------------------------------------------------------------------------------------------------

// percentage of how many nodes/edges must be deleted so that UTs are halved instead of doubled
auto inline dead_factor = 0.75f;

auto inline growth_factor = 1.2f;  // permitted growth of nodes during variable reordering

}  // namespace freddy::config
