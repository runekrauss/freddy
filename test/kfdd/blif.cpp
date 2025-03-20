// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <catch2/catch_test_macros.hpp>  // TEST_CASE

#include <freddy/dd/bdd.hpp>   // freddy::bdd::bdd_manager
#include <freddy/dd/kfdd.hpp>  // freddy::kfdd::kfdd_manager

#include <algorithm>      // std::find
#include <cassert>        // assert
#include <cstdint>        // std::int32_t
#include <cstdlib>        // EXIT_SUCCESS
#include <fstream>        // std::ifstream
#include <iostream>       // std::cout
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::pair
#include <vector>

#include <lorina/blif.hpp>  // lorina::read_blif

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Types
// *********************************************************************************************************************

struct blif_kfdd
{
    bool counting_is_done{false};  // Is the analysis phase over?

    dd::kfdd_manager mgr;  // for handling DD-related operations

    std::vector<dd::kfdd> f;

    // name -> (use count, node/edge)
    std::unordered_map<std::string, std::pair<std::int32_t, dd::kfdd>> gates;

    std::string model;

    std::vector<std::string> inputs;

    std::vector<std::string> outputs;
};

class kfdd_reader : public lorina::blif_reader
{
  public:
    explicit kfdd_reader(blif_kfdd& g, int size_first_sifting) :
            g{g},
            next_sifting{size_first_sifting}
    {}

    auto on_model(std::string const& name) const -> void override
    {
        assert(!name.empty());

        g.model = name;
    }

    auto on_input(std::string const& name) const -> void override
    {
        assert(!name.empty());

        if (!g.counting_is_done)
        {  // analysis phase
            g.gates[name] = std::make_pair(0, g.mgr.var(expansion::S, name));

            g.inputs.push_back(name);
        }
    }

    auto on_output(std::string const& name) const -> void override
    {
        assert(!name.empty());

        if (!g.counting_is_done)
        {
            g.outputs.push_back(name);
        }
    }

    auto on_gate(std::vector<std::string> const& inputs, std::string const& output, output_cover_t const& cover) const
        -> void override
    {
        assert(!output.empty());
        assert(cover.size() == 1 && cover[0].second.length() == 1);  // output (controlling value)

        if (inputs.empty())  // B_0
        {
            assert(cover[0].first.empty());  // input

            g.gates[output] = std::make_pair(0, (cover[0].second == "0") ? g.mgr.zero() : g.mgr.one());
        }
        else if (inputs.size() == 1)  // B_1
        {
            assert(g.gates.contains(inputs[0]));
            assert(cover[0].first.length() == 1);

            if (g.counting_is_done)
            {
                if (cover[0].first == "0")
                {
                    g.gates[output].second =
                        (cover[0].second == "0") ? g.gates[inputs[0]].second : ~g.gates[inputs[0]].second;
                }
                else
                {
                    assert(cover[0].first == "1");

                    g.gates[output].second =
                        (cover[0].second == "0") ? ~g.gates[inputs[0]].second : g.gates[inputs[0]].second;
                }

                if (--g.gates[inputs[0]].first == 0 &&
                    std::find(g.outputs.begin(), g.outputs.end(), inputs[0]) == g.outputs.end())
                {
                    g.gates.erase(inputs[0]);
                }
            }
            else
            {
                ++g.gates[inputs[0]].first;

                g.gates[output] = std::make_pair(0, freddy::dd::kfdd{});
            }
        }
        else
        {  // B_2
            assert(inputs.size() == 2);
            assert(g.gates.contains(inputs[0]) && g.gates.contains(inputs[1]));
            assert(cover[0].first.length() == 2);

            if (g.counting_is_done)
            {
                switch (std::stoi(cover[0].first))
                {
                    case 00:  // or, nor
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? g.gates[inputs[0]].second | g.gates[inputs[1]].second
                                                     : ~(g.gates[inputs[0]].second | g.gates[inputs[1]].second);
                        break;
                    case 01:  // >=, <
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? g.gates[inputs[0]].second | ~g.gates[inputs[1]].second
                                                     : ~g.gates[inputs[0]].second & g.gates[inputs[1]].second;
                        break;
                    case 10:  // <=, >
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? ~g.gates[inputs[0]].second | g.gates[inputs[1]].second
                                                     : g.gates[inputs[0]].second & ~g.gates[inputs[1]].second;
                        break;
                    case 11:  // nand, and
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? ~(g.gates[inputs[0]].second & g.gates[inputs[1]].second)
                                                     : g.gates[inputs[0]].second & g.gates[inputs[1]].second;
                        break;
                    default: assert("Gate is unknown");
                }

                if (--g.gates[inputs[0]].first == 0 &&
                    std::find(g.outputs.begin(), g.outputs.end(), inputs[0]) == g.outputs.end())
                {
                    g.gates.erase(inputs[0]);
                }
                if (--g.gates[inputs[1]].first == 0 &&
                    std::find(g.outputs.begin(), g.outputs.end(), inputs[1]) == g.outputs.end())
                {
                    g.gates.erase(inputs[1]);
                }
            }
            else
            {
                ++g.gates[inputs[0]].first;
                ++g.gates[inputs[1]].first;

                g.gates[output] = std::make_pair(0, freddy::dd::kfdd{});
            }
        }
        if (g.mgr.node_count() > next_sifting)
        {
            // g.mgr.gc();
            // if (g.mgr.node_count() > next_sifting)
            // {
            std::cout << g.mgr.node_count() << " exceeds " << next_sifting << '\n';
            std::cout << "before sifting:" << '\n';
            std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
            std::cout << "Size: " << g.mgr.size(g.f) << '\n';
            g.mgr.dtl_sift();
            next_sifting = next_sifting * 2;

            std::cout << "after sifting:" << '\n';
            std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
            std::cout << "Size: " << g.mgr.size(g.f) << '\n';
            // }
        }
    }

    auto on_end() const -> void override
    {
        if (g.counting_is_done)
        {
            assert(g.gates.size() == g.outputs.size());  // outputs can be redundant

            g.f.reserve(g.gates.size());
            for (auto const& po : g.gates)
            {
                g.f.push_back(po.second.second);
            }
        }
        g.counting_is_done = true;
    }

  private:
    blif_kfdd& g;
    mutable int next_sifting;
};

struct blif_bdd
{
    bool counting_is_done{false};  // Is the analysis phase over?

    dd::bdd_manager mgr;  // for handling DD-related operations

    std::vector<dd::bdd> f;

    // name -> (use count, node/edge)
    std::unordered_map<std::string, std::pair<std::int32_t, dd::bdd>> gates;

    std::string model;

    std::vector<std::string> inputs;

    std::vector<std::string> outputs;
};

class bdd_reader : public lorina::blif_reader
{
  public:
    explicit bdd_reader(blif_bdd& g, int size_first_sifting) :
            g{g},
            next_sifting{size_first_sifting}
    {}

    auto on_model(std::string const& name) const -> void override
    {
        assert(!name.empty());

        g.model = name;
    }

    auto on_input(std::string const& name) const -> void override
    {
        assert(!name.empty());

        if (!g.counting_is_done)
        {  // analysis phase
            g.gates[name] = std::make_pair(0, g.mgr.var(name));

            g.inputs.push_back(name);
        }
    }

    auto on_output(std::string const& name) const -> void override
    {
        assert(!name.empty());

        if (!g.counting_is_done)
        {
            g.outputs.push_back(name);
        }
    }

    auto on_gate(std::vector<std::string> const& inputs, std::string const& output, output_cover_t const& cover) const
        -> void override
    {
        assert(!output.empty());
        assert(cover.size() == 1 && cover[0].second.length() == 1);  // output (controlling value)

        if (inputs.empty())  // B_0
        {
            assert(cover[0].first.empty());  // input

            g.gates[output] = std::make_pair(0, (cover[0].second == "0") ? g.mgr.zero() : g.mgr.one());
        }
        else if (inputs.size() == 1)  // B_1
        {
            assert(g.gates.contains(inputs[0]));
            assert(cover[0].first.length() == 1);

            if (g.counting_is_done)
            {
                if (cover[0].first == "0")
                {
                    g.gates[output].second =
                        (cover[0].second == "0") ? g.gates[inputs[0]].second : ~g.gates[inputs[0]].second;
                }
                else
                {
                    assert(cover[0].first == "1");

                    g.gates[output].second =
                        (cover[0].second == "0") ? ~g.gates[inputs[0]].second : g.gates[inputs[0]].second;
                }

                if (--g.gates[inputs[0]].first == 0 &&
                    std::find(g.outputs.begin(), g.outputs.end(), inputs[0]) == g.outputs.end())
                {
                    g.gates.erase(inputs[0]);
                }
            }
            else
            {
                ++g.gates[inputs[0]].first;

                g.gates[output] = std::make_pair(0, freddy::dd::bdd{});
            }
        }
        else
        {  // B_2
            assert(inputs.size() == 2);
            assert(g.gates.contains(inputs[0]) && g.gates.contains(inputs[1]));
            assert(cover[0].first.length() == 2);

            if (g.counting_is_done)
            {
                switch (std::stoi(cover[0].first))
                {
                    case 00:  // or, nor
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? g.gates[inputs[0]].second | g.gates[inputs[1]].second
                                                     : ~(g.gates[inputs[0]].second | g.gates[inputs[1]].second);
                        break;
                    case 01:  // >=, <
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? g.gates[inputs[0]].second | ~g.gates[inputs[1]].second
                                                     : ~g.gates[inputs[0]].second & g.gates[inputs[1]].second;
                        break;
                    case 10:  // <=, >
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? ~g.gates[inputs[0]].second | g.gates[inputs[1]].second
                                                     : g.gates[inputs[0]].second & ~g.gates[inputs[1]].second;
                        break;
                    case 11:  // nand, and
                        g.gates[output].second = (cover[0].second == "0")
                                                     ? ~(g.gates[inputs[0]].second & g.gates[inputs[1]].second)
                                                     : g.gates[inputs[0]].second & g.gates[inputs[1]].second;
                        break;
                    default: assert("Gate is unknown");
                }

                if (--g.gates[inputs[0]].first == 0 &&
                    std::find(g.outputs.begin(), g.outputs.end(), inputs[0]) == g.outputs.end())
                {
                    g.gates.erase(inputs[0]);
                }
                if (--g.gates[inputs[1]].first == 0 &&
                    std::find(g.outputs.begin(), g.outputs.end(), inputs[1]) == g.outputs.end())
                {
                    g.gates.erase(inputs[1]);
                }
            }
            else
            {
                ++g.gates[inputs[0]].first;
                ++g.gates[inputs[1]].first;

                g.gates[output] = std::make_pair(0, freddy::dd::bdd{});
            }
        }
        if (g.mgr.node_count() > next_sifting)
        {
            // g.mgr.gc();
            // if (g.mgr.node_count() > next_sifting)
            // {
            std::cout << g.mgr.node_count() << " exceeds " << next_sifting << '\n';
            std::cout << "before sifting:" << '\n';
            std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
            std::cout << "Size: " << g.mgr.size(g.f) << '\n';
            g.mgr.reorder();
            next_sifting = next_sifting * 2;

            std::cout << "after sifting:" << '\n';
            std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
            std::cout << "Size: " << g.mgr.size(g.f) << '\n';
            // }
        }
    }

    auto on_end() const -> void override
    {
        if (g.counting_is_done)
        {
            assert(g.gates.size() == g.outputs.size());  // outputs can be redundant

            g.f.reserve(g.gates.size());
            for (auto const& po : g.gates)
            {
                g.f.push_back(po.second.second);
            }
        }
        g.counting_is_done = true;
    }

  private:
    blif_bdd& g;
    mutable int next_sifting;
};

// *********************************************************************************************************************
// Functions
// *********************************************************************************************************************

// =====================================================================================================================
// Parsing
// =====================================================================================================================

auto static read_blif(std::ifstream& file, bdd_reader const& reader)
{
    auto code = lorina::read_blif(file, reader);  // analysis

    if (code == lorina::return_code::success)
    {
        file.clear();
        file.seekg(0, std::ios::beg);
        code = lorina::read_blif(file, reader);  // symbolic simulation
    }

    return code;
}

auto static read_blif(std::ifstream& file, kfdd_reader const& reader)
{
    auto code = lorina::read_blif(file, reader);  // analysis

    if (code == lorina::return_code::success)
    {
        file.clear();
        file.seekg(0, std::ios::beg);
        code = lorina::read_blif(file, reader);  // symbolic simulation
    }

    return code;
}


auto static test_blif(std::string blif_name, int size_first_sifting)
{
    std::ifstream file{blif_name};
    if (!file.is_open())
    {
        std::cout << "Failed to open " << blif_name << '\n';
        assert(false);
    }

    blif_kfdd g;
    kfdd_reader reader{g, size_first_sifting};

    if (read_blif(file, reader) != lorina::return_code::success)  // parse BLIF in topological order
    {
        file.close();
        std::cout << "Failed to read " << blif_name << '\n';
        assert(false);
    }
    file.close();
    auto noFs = g.f.size();
    //g.gates.clear();
    //g.mgr.gc();
    //std::cout << g.mgr << '\n';

    std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
    std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    //g.mgr.reorder();
    g.mgr.dtl_sift();
    std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
    std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    return g;
}

auto static load_blif_bdd(std::string blif_name, int size_first_sifting)
{
    std::ifstream file{blif_name};
    if (!file.is_open())
    {
        std::cout << "Failed to open " << blif_name << '\n';
        assert(false);
    }

    blif_bdd g;
    bdd_reader reader{g, size_first_sifting};

    if (read_blif(file, reader) != lorina::return_code::success)  // parse BLIF in topological order
    {
        file.close();
        std::cout << "Failed to read " << blif_name << '\n';
        assert(false);
    }
    file.close();
    auto noFs = g.f.size();
    //g.gates.clear();
    //g.mgr.gc();
    //std::cout << g.mgr << '\n';

    //std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
    std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    //g.mgr.reorder();
    //g.mgr.dtl_sift();
    //std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
    //std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    return g;
}

auto static load_blif_kfdd(std::string blif_name, int size_first_sifting)
{
    std::ifstream file{blif_name};
    if (!file.is_open())
    {
        std::cout << "Failed to open " << blif_name << '\n';
        assert(false);
    }

    blif_kfdd g;
    kfdd_reader reader{g, size_first_sifting};

    if (read_blif(file, reader) != lorina::return_code::success)  // parse BLIF in topological order
    {
        file.close();
        std::cout << "Failed to read " << blif_name << '\n';
        assert(false);
    }
    file.close();
    auto noFs = g.f.size();
    //g.gates.clear();
    //g.mgr.gc();
    //std::cout << g.mgr << '\n';

    //std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
    std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    //g.mgr.reorder();
    //g.mgr.dtl_sift();
    //std::cout << "Node_Count: " << g.mgr.node_count() << '\n';
    //std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    return g;
}

TEST_CASE("kfdd blif c880 dtl sifting", "[blif_kfdd]")
{
    test_blif("c880.blif", 20000);
}

TEST_CASE("kfdd blif c432 dtl sifting", "[blif_kfdd]")
{
    test_blif("c432.blif", 10000);
}

TEST_CASE("kfdd blif c1355 dtl sifting", "[blif_kfdd]")
{
    test_blif("c1355.blif", 2000000);
}

TEST_CASE("kfdd blif c1908 dtl sifting", "[blif_kfdd]")
{
    test_blif("c1908.blif", 20000000);
}

TEST_CASE("kfdd blif c2670 dtl sifting", "[blif_kfdd]")
{
    test_blif("c2670.blif", 25000000);
}

TEST_CASE("kfdd blif c3540 dtl sifting", "[blif_kfdd]")
{
    test_blif("c3540.blif", 270000000);
}

TEST_CASE("kfdd blif c5315 dtl sifting", "[blif_kfdd]")
{
    test_blif("c5315.blif", 10000000);
}

TEST_CASE("kfdd/bdd correctness", "[blif]")
{

    auto const noVars = 36;
    long long int noCombs = 2;
    for (auto i = 0; i < noVars-1; i++)
    {
        noCombs *= 2;
    }

    auto kfdd_blif = load_blif_kfdd("c432.blif", 10000000);
    auto const selected_var = 5;
    auto initial_level = kfdd_blif.mgr.var2lvl[selected_var];
    kfdd_blif.mgr.sift(initial_level, kfdd_blif.mgr.var_count() -1);
    kfdd_blif.mgr.change_expansion_type(selected_var, expansion::ND);
    kfdd_blif.mgr.sift(kfdd_blif.mgr.var_count()-1, initial_level);
    auto kfdd = kfdd_blif.f.at(0);

    auto bdd_blif = load_blif_bdd("c432.blif", 10000000);
    auto bdd = bdd_blif.f.at(0);

    for (long long int i = 0; i < noCombs; i += 133337)
    {
        std::vector<bool> input_vars;
        for (int j = 0; j < noVars; j++)
        {
            input_vars.push_back(!!(i & (2 << j)));
        }
        CHECK(bdd.eval(input_vars) == kfdd.eval(input_vars));
    }
}

}  // namespace