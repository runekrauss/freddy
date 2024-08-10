#pragma once

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
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
    XOR,      // antivalence
    MISC      // individual usage
};

}  // namespace freddy
