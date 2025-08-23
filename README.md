![FrEDDY](https://github.com/runekrauss/freddy/assets/5829946/560e4a18-3b76-4fba-875e-c29afc5ce6c4)

[![Check](https://github.com/runekrauss/freddy/actions/workflows/check.yml/badge.svg)](https://github.com/runekrauss/freddy/actions/workflows/check.yml)
[![Build](https://github.com/runekrauss/freddy/actions/workflows/build.yml/badge.svg)](https://github.com/runekrauss/freddy/actions/workflows/build.yml)
[![Test](https://github.com/runekrauss/freddy/actions/workflows/test.yml/badge.svg)](https://github.com/runekrauss/freddy/actions/workflows/test.yml)

**FrEDDY** is a **Framework** to **Engineer Decision Diagrams Yourself** for efficient solving of problems in VLSI CAD.

## :dart: Features

- Level-based unique tables for DD optimization
- Automatic memory management to delete dead DDs
- Dynamic cache to speed up DD operations
- Optimized operations for DD manipulation
- Manager concept so that instances can coexist
- Development of custom DD types based on the manager
- Registration of custom operations with the cache
- Overloaded wrapping of DD handlers to increase usability
- Parameter configuration for specific problem solving
- Detailed output for effective debugging
- And much more

## :rocket: Getting Started

The installation of FrEDDY is described first. Second, it is explained how a DD type can be used to solve a problem.

### :wrench: Installation

FrEDDY is designed as a **header-only library** that can be integrated into external projects. To include FrEDDY and
link it against your [CMake](https://github.com/Kitware/CMake) project `<target>`, just clone it inside your project
directory and add the following lines of code to your *CMakeLists.txt*:

```cmake
add_subdirectory(freddy)
target_link_libraries(<target> freddy)
```

> :information_source: Depending on the location of FrEDDY, the path must be adjusted accordingly.

Note that this stage may take some time depending on your system configuration. For example, FrEDDY relies on the
[boost::unordered](https://boost.org/doc/libs) library that offers a catalog of hash containers for top performance. If
this dependency cannot be found, it is downloaded from an
[external repository](https://github.com/MikePopoloski/boost_unordered) at configure time and identified targets will be
added to the build system.

### :computer: Usage

In order to use a DD type for solving a problem such as
[equivalence checking of multipliers](https://dl.acm.org/doi/10.1145/370155.370315), the respective
[header](include/freddy/dd) of an appropriate reduced and ordered type like a
[binary moment diagram](https://en.wikipedia.org/wiki/Binary_moment_diagram) must be **included**, and the corresponding
manager `bmd_manager` must be **initialized** within your code file:

```cpp
#include <freddy/dd/bmd.hpp>

int main()
{
    freddy::dd::bmd_manager mgr;
}
```

A correspondence is generally proven by interpreting binary signals $x_1, \dots, x_n$ of a logical network $f$ using an
encoding function $e$ and comparing it with a word-level specification $g$:
$e(f(x_1, \dots, x_n)) = g(e(x_1), \dots, e(x_n))$.

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
    std::array<freddy::dd::bmd, 8> s;
    s[0] = a0 & b1;
    s[1] = a0 & b0;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[0] ^ s[2];
    s[5] = s[0] & s[2];
    s[6] = s[3] ^ s[5];
    s[7] = s[3] & s[5];
    std::vector<freddy::dd::bmd> const f{s[1], s[4], s[6], s[7]};
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

The multiplier can now be **verified** by interpreting network outputs as a word and applying word interpretations to
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

While there are individual encodings such as `weighted_sum`, numerous methods exist for each DD type. One of these
involves drawing nodes and edges with [DOT](https://en.wikipedia.org/wiki/DOT_(graph_description_language)). The graph
can be rendered with the corresponding layout engine and looks similar to the following BMD in this example:

![BMD](https://github.com/runekrauss/freddy/assets/5829946/4cf0d118-23f6-4157-999f-eb886d97197a)

It is still possible to adjust various [configuration](include/freddy/config.hpp) **parameters** depending on the
problem. For example, the initial capacity `ct_size` of the cache used for many other [operations](include/freddy/op) in
addition to multiplication can be set as follows:

```cpp
/* includes */

int main()
{
    freddy::config::ct_size = 25;

    /* initialization */
    
    /* bit-level implementation */
    
    /* word-level specification */

    /* verification */
}
```

You can incidentally also choose how a variable should be decomposed if the DD type supports it. Although the
[positive Davio expansion](https://en.wikipedia.org/wiki/Reed–Muller_expansion) applies to BMDs, other expansion types
can be found in the [decomposition type list](include/freddy/expansion.hpp).

## :white_check_mark: Tests

[Tests](test) can be built with CMake using the flag `-DFREDDY_TEST=ON` on the command line and run by `ctest`:

```console
$ cmake -DCMAKE_BUILD_TYPE=Release -DFREDDY_TEST=ON -B build/Release
$ cmake --build build/Release --config Release
$ cd build/Release
$ ctest -C Release
```

For debugging purposes, type `Debug` instead of `Release`.

> :warning: Compiling using debug mode may have a negative effect on the performance of FrEDDY.

Additionally, you can add `-j k` to build on `k` cores or `-v` to show in detail the commands used to build.

## :+1: Contribute

Do you want to contribute to FrEDDY? In particular, I am interested in the development of further **DD types**. Since
the [base manager](include/freddy/detail/manager.hpp) is **abstract**, the following pure virtual methods must be
implemented within the `freddy::dd` namespace in the [dd](include/freddy/dd) directory for basic operations to work:

| Method        | Description                         |
| ------------- | ----------------------------------- |
| `add`         | Additive combination of DDs         |
| `agg`         | Aggregating a weight and node value |
| `comb`        | Adjusting a pair weight             |
| `complement`  | Computation of NOT                  |
| `conj`        | Connecting two conjuncts (AND)      |
| `disj`        | Connecting two disjuncts (OR)       |
| `make_branch` | Creation of a node and an edge      |
| `merge`       | Evaluation of aggregates (subtrees) |
| `mul`         | Multiplicative combination of DDs   |
| `regw`        | Regular weight of an edge           |

While virtual methods such as `ite` (if-then-else) can be overridden if needed, both a **contradiction** and
**tautology** must be defined using a DD edge weight `E` and node value `V`, and passed to the base constructor.

> :information_source: If `E` or `V` does not correspond to a built-in type, the operator `==` must be overloaded for
hashing purposes. A specialization for [std::hash](https://en.cppreference.com/w/cpp/utility/hash) must also be added.

Further operations can be implemented on this basis. A cache is available for so-called **first-class operations** that
are time-sensitive and called frequently. If operations to be cached are not yet available in the
[op](include/freddy/op) directory, they can be implemented in a similar way to the manager concept. First-class
operations are within the `freddy::op` namespace and inherit from the **polymorphic** class `operation` contained in
[operation.hpp](include/freddy/detail/operation.hpp) by overriding the following methods:

| Method           | Description           |
| ---------------- | --------------------- |
| `hash`           | Hash code computation |
| `has_same_input` | Comparing operands    |

> :information_source: The operation name is already hashed by default.

First-class operations are **automatically registered** with the cache, where results are written using the manager
method `cache` and read by `cached` if they exist. Corresponding code snippets can be found in
[manager methods](include/freddy/detail/manager.hpp) of the **same name** as first-class operations:
[compose](include/freddy/op/compose.hpp), [restr](include/freddy/op/restr.hpp), etc.

While operations are provided by the derived manager class `<type>_manager`, an associated **wrapper** `<type>` finally
increases usability. It is implemented by simply calling operations through a DD pointer member. Complete examples can
be found in [bdd.hpp](include/freddy/dd/bdd.hpp), [bmd.hpp](include/freddy/dd/bmd.hpp),
[bhd.hpp](include/freddy/dd/bhd.hpp), and [mtbdd.hpp](include/freddy/dd/mtbdd.hpp).

To check the functionality, I use [Catch2](https://github.com/catchorg/Catch2). Already existing **tests** are located
in the [test](test) directory and should be used as orientation for your own DD types, where an executable is created
automatically from an existing test file when [building the tests](#white_check_mark-tests). To collect test coverage,
execute `ctest -T Test -T Coverage` in a terminal window.

A **statistics module** is available to analyze the behavior of implemented DD types during performance debugging. You
can enable it via `-DFREDDY_STATS=ON`, for example, to measure the distribution of hash values for a specific
application. To output experimental results, use the manager's stream insertion operator `<<`. Below is an excerpt of
**Edge Table** (ET) output, resulting from running the [usage example](#computer-usage).

```console
Variable "b0" [pD]
-------------------------------------------------------------
ET #Elements                            |                   3
ET Max. load                            |                 209
ET #Buckets                             |                 239
ET Insertion count                      |                   3
ET Insertion probe length               |                1.00
ET Unsuccessful lookup comparison count |                0.00
ET Miss rate                            |               23.08
ET Number of cleaned elements           |                   0
ET Read/Write                           |                3.33
ET Successful lookup count              |                  10
ET Successful lookup probe length       |                1.00
ET Successful lookup comparison count   |                1.00
ET Hit rate                             |               76.92
```

While [leaks](https://unix.com/man-page/osx/1/leaks) or [Valgrind](https://github.com/tklengyel/valgrind) are
recommended for **dynamic code analysis**, please use [clang-tidy](https://clang.llvm.org/extra/clang-tidy) respecting
my [coding style](.clang-tidy) for **static code analysis**. By specifying the `-DCLANGTIDY=ON` option, such an analysis
is performed. To **format** the code according to my [style guide](.clang-format), use
[clang-format](https://clang.llvm.org/docs/ClangFormat.html) as follows:

```console
$ find . -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i
```

Following these standards, a [pull request](https://github.com/runekrauss/freddy/pulls) with or without an **issue** can
be submitted according to the [Fork & Pull Request Workflow](https://gist.github.com/Chaser324/ce0505fbed06b947d962). If
you address an issue from the [tracking system](https://github.com/runekrauss/freddy/issues), it is helpful to also
specify the corresponding ID in the title. Compliance with the policies mentioned above is enforced through a
[CI/CD pipeline](https://github.com/runekrauss/freddy/actions) that has the following **workflows**:

| Workflow | Description                                  |
| -------- | -------------------------------------------- |
| Check    | Analyzing the codebase without any warnings  |
| Build    | Building FrEDDY on Linux, macOS, and Windows |
| Test     | Achieving a code coverage of at least 80%    |

Changes are only integrated into my main branch when each stage has been passed.

Thank you so much for your interest in growing the reach of FrEDDY! :blush:
