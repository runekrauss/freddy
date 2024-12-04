//
// Created by marvin on 09.10.24.
//
// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include <lorina/blif.hpp>  // lorina::read_blif

#include <freddy/dd/bdd.hpp>  // freddy::bdd::bdd_manager
#include <catch2/catch_test_macros.hpp>  // TEST_CASE
#include <algorithm>      // std::find
#include <cassert>        // assert
#include <cstdint>        // std::int32_t
#include <cstdlib>        // EXIT_SUCCESS
#include <fstream>        // std::ifstream
#include <iostream>       // std::cout
#include <unordered_map>  // std::unordered_map
#include <utility>        // std::pair

using namespace freddy;

namespace
{
// *********************************************************************************************************************
// Types
// *********************************************************************************************************************

struct dd
{
    bool counting_is_done{false};  // Is the analysis phase over?

    freddy::dd::bdd_manager mgr;  // for handling DD-related operations

    std::vector<freddy::dd::bdd> f;

    // name -> (use count, node/edge)
    std::unordered_map<std::string, std::pair<std::int32_t, freddy::dd::bdd>> gates;

    std::string model;

    std::vector<std::string> inputs;

    std::vector<std::string> outputs;
};

class dd_reader : public lorina::blif_reader
{
public:
    explicit dd_reader(dd& g) :
            g{g}
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
    dd& g;
};

// *********************************************************************************************************************
// Functions
// *********************************************************************************************************************

// =====================================================================================================================
// Parsing
// =====================================================================================================================

auto static read_blif(std::ifstream& file, dd_reader const& reader)
{
    auto code = lorina::read_blif(file, reader);  // analysis

    if (code == lorina::return_code::success)
    {
        file.seekg(0, std::ios::beg);
        code = lorina::read_blif(file, reader);  // symbolic simulation
    }

    return code;
}

// =====================================================================================================================
// Startup
// =====================================================================================================================

// int main(int, char* argv[])
// {
//     std::ifstream file{std::string{"blif/"} + argv[1]};
//     if (!file.is_open())
//     {
//         std::cout << "Failed to open " << argv[1] << std::endl;
//         return EXIT_FAILURE;
//     }
//
//     dd g;
//     dd_reader reader{g};
//
//     if (read_blif(file, reader) != lorina::return_code::success)  // parse BLIF in topological order
//     {
//         file.close();
//         std::cout << "Failed to read " << argv[1] << std::endl;
//         return EXIT_FAILURE;
//     }
//     file.close();
//
//     std::cout << "Size: " << g.mgr.size(g.f) << std::endl;
//
//     return EXIT_SUCCESS;
// }

TEST_CASE("bdd blif c432 parsing", "[blif]")
{
    auto filename = std::string{"c432.blif"};
    std::ifstream file{filename};
    if (!file.is_open())
    {
        std::cout << "Failed to open " << filename << '\n';
        assert(false);
    }

    dd g;
    dd_reader reader{g};

    if (read_blif(file, reader) != lorina::return_code::success)  // parse BLIF in topological order
    {
        file.close();
        std::cout << "Failed to read " << filename << '\n';
        assert(false);
    }
    file.close();

    //std::cout << g.mgr << '\n';
    std::cout << "Size: " << g.mgr.size(g.f) << '\n';
    g.mgr.sift(15,35);
    //std::cout << g.mgr << '\n';
    std::cout << "Size: " << g.mgr.size(g.f) << '\n';
}
}  // namespace