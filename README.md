# EDDY

[![Check](https://github.com/runekrauss/eddy/actions/workflows/check.yml/badge.svg)](https://github.com/runekrauss/eddy/actions/workflows/check.yml)
[![Build](https://github.com/runekrauss/eddy/actions/workflows/build.yml/badge.svg)](https://github.com/runekrauss/eddy/actions/workflows/build.yml)
[![Test](https://github.com/runekrauss/eddy/actions/workflows/test.yml/badge.svg)](https://github.com/runekrauss/eddy/actions/workflows/test.yml)

**EDDY** is a framework to **Engineer Decision Diagrams Yourself** for efficient solving of problems in VLSI CAD.

## :dart: Features

* Level-based unique tables for DD optimization
* Automatic memory management to delete dead DDs
* Dynamic cache to speed up DD operations
* Optimized operations for DD manipulation
* Manager concept so that instances can coexist
* Development of own DD types based on the manager
* Overloaded wrapping of DD handlers to improve usability
* Parameter configuration for specific problem solving
* Detailed output for effective debugging
* And much more

## :rocket: Getting Started

First, the steps to install EDDY are described. Second, it is explained how a DD type can be used to solve a problem.

### :wrench: Installation

EDDY is designed as a **header-only library** that can be integrated into external projects. To include EDDY and link it
against your [CMake](https://github.com/Kitware/CMake) project `<target>`, just clone it inside your project directory
and add the following lines of code to your *CMakeLists.txt*:

```cmake
add_subdirectory(eddy)
target_link_libraries(<target> eddy)
```

Depending on the location of EDDY, the path must be adjusted accordingly.

### :computer: Usage

In order to use a DD type for solving a problem such as
[equivalence checking of multipliers](https://dl.acm.org/doi/10.1145/370155.370315), the respective
[header](include/eddy/dd/bmd.hpp) of an appropriate type like a
[binary moment diagram](https://en.wikipedia.org/wiki/Binary_moment_diagram) must be **included** and the corresponding
manager `bmd_manager` must be **initialized** within your code file:

```cpp
#include <eddy/dd/bmd.hpp>

int main()
{
    eddy::dd::bmd_manager mgr;
}
```

In general, a correspondence is proved by interpreting binary signals $x_1,\ldots,x_n$ of a logical network $f$ using
an encoding function $e$ and comparing it with a word-level specification $g$:
$e(f(x_1,\ldots,x_n)) = g(e(x_1),\ldots,e(x_n))$.

In this example, a **bit-level implementation** $f$ for a 2-bit multiplier is developed via
[symbolic simulation](https://dl.acm.org/doi/abs/10.1145/123186.128296):

```cpp
/* includes */

int main()
{
    /* initialization */

    auto const a1 = mgr.var("a1");
    auto const b1 = mgr.var("b1");
    auto const a0 = mgr.var("a0");
    auto const b0 = mgr.var("b0");
    std::array<eddy::dd::bmd, 8> s;
    s[0] = a0 & b1;
    s[1] = a0 & b0;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[0] ^ s[2];
    s[5] = s[0] & s[2];
    s[6] = s[3] ^ s[5];
    s[7] = s[3] & s[5];
    std::vector<eddy::dd::bmd> f{s[1], s[4], s[6], s[7]};
}
```

While the manager handles BMD operations, the overloaded wrapper `bmd` allows direct access to a BMD. In this context,
the **word-level specification** $g$ can be realized by a sum of weighted bits $e$:

```cpp
/* includes */

int main()
{
    /* initialization */
    
    /* bit-level implementation */

    auto const g = mgr.weighted_sum({a0, a1}) * mgr.weighted_sum({b0, b1});
}
```

Now the multiplier can be **verified** by interpreting network outputs as a word and applying word interpretations to
network inputs:

```cpp
/* includes */

int main()
{
    /* initialization */
    
    /* bit-level implementation */
    
    /* word-level specification */

    return !(mgr.weighted_sum(f) == g);
}
```

In addition, depending on the problem, it is possible to adjust various **parameters** such as the initial size
`ct_size` of the operation cache:

```cpp
/* includes */

int main()
{
    eddy::config::ct_size = 127;

    /* initialization */
    
    /* bit-level implementation */
    
    /* word-level specification */

    /* verification */
}
```

Other parameters can be found in the [configuration](include/eddy/config.hpp).

## :white_check_mark: Tests

Tests can be built with CMake using the flag `-DEDDY_TEST=ON` on the command line and run by `ctest`:

```console
$ cmake -DCMAKE_BUILD_TYPE=Release -DEDDY_TEST=ON -B build/Release
$ cmake --build build/Release --config Release
$ cd build/Release
$ ctest
```

For debugging purposes, type `Debug` instead of `Release`. Note that this may have a negative effect on the performance
of EDDY. Additionally, you can add `-j k` to build on `k` cores, or `-v` to show in detail the commands used to build.

## :+1: Contribute

Do you want to contribute to EDDY? In particular, I am interested in the development of further **DD types**. Since the
[base manager](include/eddy/detail/manager.hpp) is **abstract**, the following pure virtual methods must be implemented
within the `eddy::dd` namespace in the [dd](include/eddy/dd) directory for basic operations to work:

| Method          | Description                       |
| --------------- | --------------------------------- |
| `add`           | Additive combination of DDs       |
| `apply`         | Adjusting a pair weight           |
| `complement`    | Computation of NOT                |
| `conj`          | Connecting two conjuncts (AND)    |
| `disj`          | Connecting two disjuncts (OR)     |
| `is_normalized` | Checking if a node is normalized  |
| `make_branch`   | Creation of an edge and a node    |
| `mul`           | Multiplicative combination of DDs |
| `neg`           | Negating a DD                     |
| `regw`          | Standard weight of an edge        |

The associated wrapper `<type>` for a DD handle is implemented by calling operations realized by the derived manager
`<type>_manager` as a pointer member. Examples can be found in [bdd.hpp](include/eddy/dd/bdd.hpp)
and [bmd.hpp](include/eddy/dd/bmd.hpp).

To check the functionality, I use [Catch2](https://github.com/catchorg/Catch2). Already existing tests are located in
the [test](test) directory and should be used as orientation for own DD types, where an executable is created
automatically from an existing test file when [building the tests](#white_check_mark-tests). To collect test coverage,
execute `ctest -T Test -T Coverage` in a terminal window.

While [leaks](https://unix.com/man-page/osx/1/leaks) or [Valgrind](https://github.com/tklengyel/valgrind) are
recommended for dynamic code analysis, please use [clang-tidy](https://clang.llvm.org/extra/clang-tidy) respecting my
[coding style](.clang-tidy) for static code analysis. By specifying the `-DCLANGTIDY=ON` option, such an analysis is
performed. To format the code according to my [style guide](.clang-format), use
[clang-format](https://clang.llvm.org/docs/ClangFormat.html) as follows:

```console
$ find . -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i
```

Following these standards, a [pull request](https://github.com/runekrauss/alf/pulls) with or without an **issue** can be
submitted according to the [Fork & Pull Request Workflow](https://gist.github.com/Chaser324/ce0505fbed06b947d962). If
you address an issue from the [tracking system](https://github.com/runekrauss/eddy/issues), it is helpful to also
specify the corresponding ID in the title. Compliance with the policies mentioned above is enforced through a
[CI/CD pipeline](https://github.com/runekrauss/eddy/actions) that has the following **workflows**:

| Workflow | Description                                 |
| -------- | ------------------------------------------- |
| Check    | Analyzing the codebase without any warnings |
| Build    | Building EDDY on Linux, macOS, and Windows  |
| Test     | Achieving a code coverage of at least 80%   |

Changes are only integrated into my main branch when each stage has been passed.

Thank you so much for your interest in growing the reach of EDDY! :blush:
