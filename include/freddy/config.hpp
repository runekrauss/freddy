#pragma once

namespace freddy::config
{

auto inline ct_size = 262147;  // initial size of the computed table (operation cache)

auto inline ut_size = 257;  // initial size of a unique table (per level)

auto inline vl_size = 32;  // initial capacity of the variable list

auto inline dead_factor = 0.3f;  // percentage of how many nodes/edges must be deleted so that no resizing takes place

auto inline growth_factor = 1.2f;  // permitted growth of nodes during variable reordering

auto inline load_factor = 0.7f;  // percentage (hash table occupancy) from which dead nodes/edges are deleted

}  // namespace freddy::config
