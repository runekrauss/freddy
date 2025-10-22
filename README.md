![FrEDDY](https://github.com/runekrauss/freddy/assets/5829946/560e4a18-3b76-4fba-875e-c29afc5ce6c4)

[![Build and Test](https://github.com/runekrauss/freddy/actions/workflows/build-test.yml/badge.svg)](https://github.com/runekrauss/freddy/actions/workflows/build-test.yml)
[![CodeQL](https://github.com/runekrauss/freddy/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/runekrauss/freddy/actions/workflows/github-code-scanning/codeql)
[![Dependabot Updates](https://github.com/runekrauss/freddy/actions/workflows/dependabot/dependabot-updates/badge.svg)](https://github.com/runekrauss/freddy/actions/workflows/dependabot/dependabot-updates)

**FrEDDY** is a **Framework** to **Engineer Decision Diagrams Yourself** for efficient solving of problems in VLSI CAD.

## :dart: Features

- Level-based unique tables for DD optimization
- Automatic memory management to delete dead DDs (delayed)
- Dynamic cache to speed up DD operations
- Optimized operations for DD manipulation
- Manager concept enabling instances to coexist
- Development of custom DD types based on the manager
- Registration of custom operations with the cache
- Overloaded wrapping of DD handlers to increase usability
- Parameter configuration for specific problem solving
- Detailed output for effective debugging
- And much more

## :rocket: Getting Started

The installation of FrEDDY is described first. Second, it is explained how a DD type can be used to solve a problem.

### :wrench: Installation

FrEDDY is designed as a **header-only library** that can be easily integrated into external projects. To integrate
FrEDDY into your [CMake](https://github.com/Kitware/CMake) project `<target>`, simply clone it inside your project
directory and add the following lines to its *CMakeLists.txt*:

```cmake
add_subdirectory(freddy)
target_link_libraries(<target> freddy)
```

> :information_source: Depending on where FrEDDY is located, you may need to adjust the path accordingly.

Note that this stage may take some time depending on your system configuration. For example, FrEDDY relies on certain
[Boost libraries](https://boost.org/libraries) like *Unordered*, which offers high-performance hash containers. If this
dependency cannot be found locally, it will be downloaded from an
[external repository](https://github.com/boostorg/boost/releases) at configure time, with the required Boost modules
automatically added to your build system.

### :computer: Usage

To solve problems such as [equivalence checking of multipliers](https://dl.acm.org/doi/10.1145/370155.370315),
**include** the [header](include/freddy/dd) of an appropriate reduced and ordered DD type, such as a multiplicative
[binary moment diagram](https://en.wikipedia.org/wiki/Binary_moment_diagram), and **initialize** the corresponding
manager in your code:

```cpp
#include <freddy/dd/bmd.hpp>

int main()
{
    freddy::bmd_manager mgr;
}
```

A correspondence is generally proven by interpreting the binary signals $x_1, \dots, x_n$ of a logical network $f$ using
an encoding function $e$ and comparing the result to a word-level specification $g$:
$e(f(x_1, \dots, x_n)) = g(e(x_1), \dots, e(x_n))$.

In this example, a **bit-level implementation** `f` of a 2-bit multiplier is generated using
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
    std::array<freddy::bmd, 8> s;
    s[0] = a0 & b0;
    s[1] = a0 & b1;
    s[2] = a1 & b0;
    s[3] = a1 & b1;
    s[4] = s[1] ^ s[2];
    s[5] = s[1] & s[2];
    s[6] = s[3] ^ s[5];
    s[7] = s[3] & s[5];
    std::vector<freddy::bmd> const f{s[0], s[4], s[6], s[7]};
}
```

While the `bmd_manager` handles BMD operations, the overloaded wrapper `bmd` allows direct access to a BMD. In this
context, the **word-level specification** `g` can be realized as a sum of weighted bits:

```cpp
/* includes */

int main()
{
    /* initialization */
    
    /* bit-level implementation */

    auto const g = mgr.unsigned_bin({a0, a1}) * mgr.unsigned_bin({b0, b1});
}
```

The multiplier can now be formally **verified** by interpreting the network outputs as a word and applying the
word-level encoding to the primary inputs:

```cpp
/* includes */

int main()
{
    /* initialization */
    
    /* bit-level implementation */
    
    /* word-level specification */

    return !(mgr.unsigned_bin(f) == g);
}
```

> :warning: Even though throwing exceptions – for example, when multiplying very large BMD constants – is unlikely, they
should still be handled using `catch` blocks.

Whereas encodings such as `unsigned_bin` (unsigned binary) are specific to BMDs, each DD type supports methods that are
common across all types. One such method called `dump_dot` involves drawing edges and nodes with
[DOT](https://en.wikipedia.org/wiki/DOT_(graph_description_language)). The resulting graph can be rendered using a
layout engine and looks like the following BMD:

![BMD](https://github.com/runekrauss/freddy/assets/5829946/4cf0d118-23f6-4157-999f-eb886d97197a)

Again, it's possible to adjust various [configuration parameters](include/freddy/config.hpp) depending on the problem.
For instance, the minimum capacity `cache_size_hint` of the cache can be passed to the manager, which is employed for
many other [operations](include/freddy/detail/operation) beyond multiplication. Here is an overview of the currently
configurable parameters:

| Parameter          | Default setting | Description                                                                 |
| ------------------ | --------------- | --------------------------------------------------------------------------- |
| `utable_size_hint` | 1,679           | Minimum capacity of a unique table                                          |
| `cache_size_hint`  | 215,039         | Minimum capacity of the operation cache                                     |
| `init_var_cap`     | 16              | Initial capacity of the variable list                                       |
| `max_node_growth`  | 1.2             | Permitted node growth factor during reordering                              |
| `heap_mem_limit`   | Unset           | Heap usage in bytes before garbage collection (auto-estimated if unset)     |

You can also choose how a variable should be decomposed, provided the DD type supports it. Although the
[positive Davio expansion](https://en.wikipedia.org/wiki/Reed–Muller_expansion) (*pD*) applies to BMDs, other expansion
types, such as the [Shannon expansion](https://en.wikipedia.org/wiki/Boole%27s_expansion_theorem), are available in the
[decomposition type list](include/freddy/expansion.hpp).

To simplify working with multiple DD types at once, it's highly recommended to include the provided
[umbrella header](include/freddy.hpp).

## :white_check_mark: Tests

[Tests](test) are split into **basic functionality checks** and (further) **example cases**. They can be built with
CMake by passing the flag `-DFREDDY_TEST=ON` on the command line and run using `ctest`:

```console
$ cmake -B build/Release -DCMAKE_BUILD_TYPE=Release -DFREDDY_TEST=ON
$ cmake --build build/Release --config Release
$ cd build/Release
$ ctest -C Release
```

For debugging purposes, type `Debug` instead of `Release`.

> :warning: Compiling in debug mode may negatively impact the performance of FrEDDY.

Additionally, you can add `-j k` to build on `k` cores or `-v` to show in detail the commands used to build.

## :handshake: Contributing

Do you want to contribute to FrEDDY? In particular, contributions towards the development of additional **DD types** are
welcome. Since the [base manager](include/freddy/detail/manager.hpp) is **abstract**, the following pure virtual methods
must be implemented within the `freddy` namespace in the [dd directory](include/freddy/dd) for basic operations to work:

| Method        | Description                                    |
| ------------- | ---------------------------------------------- |
| `agg`         | Aggregation of an edge weight and a node value |
| `branch`      | Creation or reuse of a node and incoming edge  |
| `comb`        | Adjustment of a weight pair                    |
| `complement`  | Computation of NOT                             |
| `conj`        | Connection of two conjuncts (AND)              |
| `disj`        | Connection of two disjuncts (OR)               |
| `merge`       | Evaluation of aggregates (subtrees)            |
| `mul`         | Multiplicative combination of DDs              |
| `plus`        | Additive combination of DDs                    |
| `regw`        | Regular weight of an edge                      |

While virtual methods such as `ite` (if-then-else) can be overridden if needed, both the **contradiction** and
**tautology** must be defined using a DD edge weight (`EWeight`) and node value (`NValue`), which are then passed to the
base constructor.

> :information_source: If `EWeight` or `NValue` aren't built-in types, the equality operator `==` must be overloaded for
hashing purposes. Of course, a custom specialization of [std::hash](https://en.cppreference.com/w/cpp/utility/hash)
must be provided.

Further DD operations can be implemented based on this foundation. A cache is available for so-called
**first-class operations** that are time-sensitive and frequently invoked. If operations to be cached are not yet
available in the [operation directory](include/freddy/detail/operation), they can be implemented similarly to the
manager concept. First-class operations are within the `freddy::detail` namespace and inherit from the **polymorphic**
class `operation` contained in [operation.hpp](include/freddy/detail/operation.hpp) by overriding the following methods:

| Method           | Description           |
| ---------------- | --------------------- |
| `equals`         | Comparing operands    |
| `hash`           | Computing hash code   |

> :information_source: The operation name is already hashed by default.

First-class operations are **automatically registered** with the cache, where results are stored using the manager
method `cache` and retrieved via `cached` if available. Corresponding code snippets can be found in the
[manager methods](include/freddy/detail/manager.hpp) sharing the **same names** as the first-class operations, such as
[compose](include/freddy/detail/operation/compose.hpp), [restr](include/freddy/detail/operation/restr.hpp), and others.

While operations are provided by the derived manager class `<type>_manager`, an associated **wrapper** `<type>`
ultimately enhances usability. It is implemented by simply forwarding a DD member, which corresponds to an
[intrusive pointer](https://boost.org/library/latest/smart_ptr), as a parameter to the operations being invoked.
Complete examples can be found in [bdd.hpp](include/freddy/dd/bdd.hpp), [bmd.hpp](include/freddy/dd/bmd.hpp),
[bhd.hpp](include/freddy/dd/bhd.hpp), and [add.hpp](include/freddy/dd/add.hpp).

To check functionality, I use [Catch2](https://github.com/catchorg/Catch2). Existing **tests** are located in the
[test directory](test) and serve as a reference for your own DD types. When
[building the tests](#white_check_mark-tests), an executable is automatically created from each test file. To collect
test coverage, execute `ctest -T Test -T Coverage` in a terminal.

A **statistics module** is available to help analyze the behavior of implemented DD types during performance debugging.
You can enable it with `-DFREDDY_STATS=ON` – for example, to measure the distribution of hash values in a specific
application. Use the manager's stream insertion operator `<<` to output experimental results. Below is an excerpt from
an **Edge Table** (`ET`) output produced by running the [usage example](#computer-usage).

```console
Variable "b0" [pD]
------------------------------------------------------------
ET #Elements                           |                   3
ET Max load                            |                1679
ET #Buckets                            |                1919
ET Insertion count                     |                   3
ET Avg insertion probe length          |                1.00
ET Avg unsuccessful lookup comparisons |                0.00
ET Miss rate                           |               23.08
ET Cleaned elements                    |                   0
ET Read/Write                          |                3.33
ET Successful lookup count             |                  10
ET Avg successful lookup probe length  |                1.00
ET Avg successful lookup comparisons   |                1.00
ET Hit rate                            |               76.92
```

While tools like [leaks](https://unix.com/man-page/osx/1/leaks) or [Valgrind](https://valgrind.org) are recommended for
**dynamic code analysis**, please use [Clang-Tidy](https://clang.llvm.org/extra/clang-tidy) in accordance with the
provided [coding style](.clang-tidy) for **static code analysis**. This analysis can be performed by specifying the
`-DCLANG_TIDY=ON` option. To **format** the code according to my [style guide](.clang-format), use
[ClangFormat](https://clang.llvm.org/docs/ClangFormat.html) as shown below.

```console
$ find . -iname '*.cpp' -o -iname '*.hpp' | xargs clang-format -i
```

Following these standards, you can submit a [pull request](https://github.com/runekrauss/freddy/pulls) – with or without
an **issue** from the [tracking system](https://github.com/runekrauss/freddy/issues) – according to the
[GitHub Standard Fork & Pull Request Workflow](https://gist.github.com/Chaser324/ce0505fbed06b947d962). The above
policies are enforced through a [CI pipeline](https://github.com/runekrauss/freddy/actions) that consists of the
following **workflows** and their respective requirements:

| Workflow           | Requirements                                                    |
| ------------------ | --------------------------------------------------------------- |
| Build and Test     | Builds on Linux, macOS, and Windows; at least 80% test coverage |
| CodeQL             | No vulnerabilities or errors in GitHub Actions                  |
| Dependabot Updates | Dependencies must be up to date                                 |

Changes are only merged into my main branch when all workflows have passed.

> :information_source: In order to accelerate development, the CI process leverages branch-based caching through
[Ccache](https://github.com/hendrikmuhs/ccache-action) and the [Cache action](https://github.com/actions/cache).

Thank you so much for helping grow FrEDDY! :blush::sparkles:
