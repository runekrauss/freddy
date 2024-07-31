#pragma once

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

enum class operation
{
    ADD,      // addition
    AND,      // conjunction
    COMPOSE,  // function substitution
    ITE,      // if-then-else
    MUL,      // multiplication
    RESTR,    // variable substitution
    SAT,      // satisfiability
    XOR       // antivalence
};

}  // namespace freddy::detail
