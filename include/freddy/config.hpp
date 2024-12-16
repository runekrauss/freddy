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
// Factors
// ---------------------------------------------------------------------------------------------------------------------

auto inline dead_factor = 0.3f;  // percentage of how many nodes/edges must be deleted so that UTs are not resized

auto inline growth_factor = 1.2f;  // permitted growth of nodes during variable reordering

auto inline load_factor = 0.7f;  // percentage (UT occupancy) from which dead nodes/edges are deleted

// ---------------------------------------------------------------------------------------------------------------------
// Sizes
// ---------------------------------------------------------------------------------------------------------------------

auto inline ct_size = 262147;  // initial capacity of the computed table (operation cache)

auto inline ut_size = 257;  // initial capacity of a unique table (per DD level)

auto inline vl_size = 32;  // initial capacity of the variable list

}  // namespace freddy::config
