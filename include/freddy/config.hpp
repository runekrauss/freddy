#pragma once

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Types
// =====================================================================================================================

struct config
{
    float dead_factor{0.75};
};

// percentage of how many nodes/edges must be deleted so that UTs are halved instead of doubled
//inline auto dead_factor = 0.75f;

//inline auto growth_factor = 1.2f;  // permitted growth of nodes during variable reordering

}  // namespace freddy
