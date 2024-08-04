#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/detail/manager.hpp"  // detail::manager

#include <algorithm>    // std::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::pow
#include <cstdint>      // std::int32_t
#include <iostream>     // std::cout
#include <iterator>     // std::back_inserter
#include <memory>       // std::shared_ptr
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::make_pair
#include <vector>       // std::vector

#include <fstream>
#include <random>
#include <filesystem>

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::dd
{

// =====================================================================================================================
// Declarations
// =====================================================================================================================

class bhd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class bhd
{
  public:
    bhd() = default;  // so that BHDs initially work with standard containers

    auto operator~() const;

    auto operator&=(bhd const&) -> bhd&;

    auto operator|=(bhd const&) -> bhd&;

    auto operator^=(bhd const&) -> bhd&;

    auto friend operator&(bhd lhs, bhd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    auto friend operator|(bhd lhs, bhd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    auto friend operator^(bhd lhs, bhd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    auto friend operator==(bhd const& lhs, bhd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BHD manager

        return (lhs.f == rhs.f);
    }

    auto friend operator!=(bhd const& lhs, bhd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    auto friend operator<<(std::ostream& s, bhd const& g) -> std::ostream&
    {
        s << "Wrapper = " << g.f;
        s << "\nBHD manager = " << g.mgr;
        return s;
    }

    [[nodiscard]] auto same_node(bhd const& g) const noexcept
    {
        assert(f);

        return (f->v == g.f->v);
    }

    [[nodiscard]] auto is_complemented() const noexcept
    {
        assert(f);

        return f->w;
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        assert(f);

        return f->v->is_const();
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto var() const
    {
        assert(!is_const());

        return f->v->br().x;
    }

    [[nodiscard]] auto high(bool = false) const;

    [[nodiscard]] auto low(bool = false) const;

    template <typename T, typename... Ts>
    auto cof(T, Ts...) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto has_const(bool) const;

    [[nodiscard]] auto is_essential(std::int32_t) const;

    [[nodiscard]] auto ite(bhd const&, bhd const&) const;

    [[nodiscard]] auto compose(std::int32_t, bhd const&) const;

    [[nodiscard]] auto restr(std::int32_t, bool) const;

    [[nodiscard]] auto exist(std::int32_t) const;

    [[nodiscard]] auto forall(std::int32_t) const;

    [[nodiscard]] auto satcount() const;

    auto print() const;

    auto createExpansionFiles() const;

  private:
    friend bhd_manager;

    // wrapper is controlled by its BHD manager
    bhd(std::shared_ptr<detail::edge<bool, bool>> f, bhd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(mgr);
    }

    std::shared_ptr<detail::edge<bool, bool>> f;  // DD handle

    bhd_manager* mgr{};  // must be destroyed after this wrapper
};

class bhd_manager : public detail::manager<bool, bool>
{
  public:
    friend bhd;

    bhd_manager(int h1, int h2) :
            manager(tmls())
    {
        consts.push_back(make_const(false, true)); // tmls[2] == Exp weight 0
        consts.push_back(make_const(true, true));  // tmls[3] == Exp weight 1

        heuristic = h1;
        heuristicAtt = h2;
    }

    auto var(std::string_view l = {})
    {
        return bhd{make_var(expansion::S, l), this};
    }

    auto var(std::int32_t const i) noexcept
    {
        assert(i >= 0);
        assert(i < var_count());

        return bhd{vars[i], this};
    }

    auto zero() noexcept
    {
        return bhd{consts[0], this};
    }

    auto one() noexcept
    {
        return bhd{consts[1], this};
    }

    auto exp(){
        return bhd{consts[2], this};
    }

    [[nodiscard]] auto size(std::vector<bhd> const& fs) const
    {
        return node_count(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bhd> const& fs) const
    {
        assert(!fs.empty());

        return longest_path(transform(fs));
    }

    auto print(std::vector<bhd> const& fs, std::vector<std::string> const& outputs = {},
               std::ostream& s = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        to_dot(transform(fs), outputs, s);
    }

    void createExpansionFiles(edge_ptr const& f){

        for (const auto& entry : std::filesystem::directory_iterator("ExpansionNodes")) {
            if (std::filesystem::is_regular_file(entry)) {
                std::filesystem::remove(entry);
            }
        }

        std::vector<std::pair<std::int32_t, bool>> v;
        searchExpansionNodes(f, v, false);
    }

  private:

    int heuristic;
    int heuristicAtt;

    int expCount = 0;

    void searchExpansionNodes(edge_ptr const& f, std::vector<std::pair<std::int32_t, bool>> path, bool goingTrue){

        if (f == consts[1]){
            if (!goingTrue){
                newTruePath(path);
            }
            return;
        }

        if (f == consts[0]){
            if (goingTrue){
                newTruePath(path);
            }
            return;
        }

        if (f == consts[2] || f == consts[3]){
            newExpansionFile(path);
        } else{

            if (f->w == 1){
                goingTrue = !goingTrue;
            }

            path.push_back(std::pair<std::int32_t,bool> (f->v->br().x, true));
            searchExpansionNodes(f->v->br().hi, path, goingTrue);

            path.pop_back();
            path.push_back(std::pair<std::int32_t,bool> (f->v->br().x, false));
            searchExpansionNodes(f->v->br().lo, path, goingTrue);
        }
    }

    void newExpansionFile(std::vector<std::pair<std::int32_t, bool>> path){
        std::string s = "ExpansionNodes/e" + std::to_string(expCount);
        s += ".txt";

        std::ofstream outFile(s);
        expCount++;

        for (auto e : path){
            if (e.second == 0){
                outFile << "-";
            }
            outFile << e.first + 1 << " 0\n";
        }
    }

    void newTruePath(std::vector<std::pair<std::int32_t, bool>> path){
        for (auto e : path){
            std::cout << e.first << ":" << e.second << "--";
        }
        std::cout << "\n";
    }


    using bool_edge = detail::edge<bool, bool>;

    using bool_node = detail::node<bool, bool>;

    auto satcount(edge_ptr const& f) -> double
    {
        std::cout << "satcount WIRD BENUTZT\n";

        assert(f);

        if (f->v->is_const())
        {
            return (f == consts[0]) ? 0 : std::pow(2, var_count());
        }

        auto const cr = ct.find({operation::SAT, f});
        if (cr != ct.end())
        {
            return cr->second.second;
        }

        auto count = (satcount(f->v->br().hi) + satcount(f->v->br().lo)) / 2;
        if (f->w)
        {  // complemented edge
            count = std::pow(2, var_count()) - count;
        }

        ct.insert_or_assign({operation::SAT, f}, std::make_pair(edge_ptr{}, count));

        return count;
    }

    auto simplify(edge_ptr const& f, edge_ptr& g, edge_ptr& h) const noexcept
    {
        std::cout << "simplify WIRD BENUTZT\n";

        assert(f);
        assert(g);
        assert(h);

        if (f == g)
        {  // ite(f, f, h) => ite(f, 1, h)
            g = consts[1];
            return 1;
        }
        if (f == h)
        {  // ite(f, g, f) => ite(f, g, 0)
            h = consts[0];
            return 2;
        }
        if (f->v == h->v && !(f->w == h->w))
        {  // ite(f, g, ~f) => ite(f, g, 1)
            h = consts[1];
            return 3;
        }
        if (f->v == g->v && !(f->w == g->w))
        {  // ite(f, ~f, h) => ite(f, 0, h)
            g = consts[0];
            return 4;
        }
        return 0;
    }

    auto std_triple(std::int32_t const simpl, edge_ptr& f, edge_ptr& g, edge_ptr& h)
    {
        std::cout << "std_triple WIRD BENUTZT\n";

        assert(f);
        assert(!f->v->is_const());
        assert(g);
        assert(h);

        switch (simpl)
        {
            case 1:
                assert(!h->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[h->v->br().x])
                {  // ite(f, 1, h) == ite(h, 1, f)
                    std::swap(f, h);
                }
                break;
            case 2:
                assert(!g->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[g->v->br().x])
                {  // ite(f, g, 0) == ite(g, f, 0)
                    std::swap(f, g);
                }
                break;
            case 3:
                assert(!g->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[g->v->br().x])
                {  // ite(f, g, 1) == ite(~g, ~f, 1)
                    f = complement(g);
                    g = complement(f);
                }
                break;
            case 4:
                assert(!h->v->is_const());

                if (var2lvl[f->v->br().x] >= var2lvl[h->v->br().x])
                {  // ite(f, 0, h) == ite(~h, 0, ~f)
                    f = complement(h);
                    h = complement(f);
                }
                break;
            default: assert(false);
        }
    }

    auto ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr override
    {
        std::cout << "ite WIRD BENUTZT\n";

        assert(f);
        assert(g);
        assert(h);

        auto const ret = simplify(f, g, h);

        // terminal cases
        if (f == consts[0])
        {
            return h;
        }
        if (f == consts[1])
        {
            return g;
        }
        if (g == h)
        {
            return g;
        }
        if (h == consts[0] && g == consts[1])
        {
            return f;
        }
        if (g == consts[0] && h == consts[1])
        {
            return complement(f);
        }

        if (ret != 0)
        {
            std_triple(ret, f, g, h);
        }

        auto const cr = ct.find({operation::ITE, f, g, h});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto const x = (f->v->br().x == top_var(f, g)) ? top_var(f, h) : top_var(g, h);
        auto r = make_branch(x, ite(cof(f, x, true), cof(g, x, true), cof(h, x, true)),
                             ite(cof(f, x, false), cof(g, x, false), cof(h, x, false)));

        ct.insert_or_assign({operation::ITE, f, g, h}, std::make_pair(r, 0.0));

        return r;
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        std::cout << "antiv WIRD BENUTZT\n";


        assert(f);
        assert(g);

        if (f == consts[0])
        {
            return g;
        }
        if (g == consts[0])
        {
            return f;
        }
        if (f == consts[1])
        {
            return complement(g);
        }
        if (g == consts[1])
        {
            return complement(f);
        }
        if (f == g)
        {
            return consts[0];
        }
        if (f == complement(g))
        {
            return consts[1];
        }

        auto const cr = ct.find({operation::XOR, f, g});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto const x = top_var(f, g);
        auto r = make_branch(x, antiv(cof(f, x, true), cof(g, x, true)), antiv(cof(f, x, false), cof(g, x, false)));

        ct.insert_or_assign({operation::XOR, f, g}, std::make_pair(r, 0.0));

        return r;
    }

    auto add(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        std::cout << "add WIRD BENUTZT\n";

        assert(f);
        assert(g);

        return antiv(f, g);
    }

    [[nodiscard]] auto agg(bool const& w, bool const& val) const noexcept -> bool override
    {
        std::cout << "agg WIRD BENUTZT\n";

        return !(w == val);  // XOR
    }

    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const noexcept -> bool override
    {
        return !(w1 == w2);
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        return (!f->w ? foa(std::make_shared<bool_edge>(true, f->v)) : foa(std::make_shared<bool_edge>(false, f->v)));
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if ((f == consts[2] || f == consts[3]) && (g == consts[2] || g == consts[3])){
            return consts[2];
        }

        if (f == consts[2] || f == consts[3]){

            return replaceOnesWithExp(g, false, f);
        }
        if (g == consts[2] || g == consts[3]){
            return replaceOnesWithExp(f, false, g);
        }

        if (f == consts[1])
        {  // 1g == g
            return g;
        }
        if (g == consts[1])
        {  // f1 == f
            return f;
        }
        if (f->v == g->v)
        {  // check for complement
            if (f->w == g->w){
                return f;
            }
            if (has_const(f, true)){
                return consts[2];
            }
            return consts[0];

            //return ((f->w == g->w) ? f : consts[0]);
        }

        auto const cr = ct.find({operation::AND, f, g});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto const x = top_var(f, g);


        auto r = doHeuristic(f, g, x);



        ct.insert_or_assign({operation::AND, f, g}, std::make_pair(r, 0.0));

        return r;
    }

    edge_ptr doHeuristic(edge_ptr f, edge_ptr g, std::int32_t x)
    {
        switch(heuristic)
        {
            case 0: return heuristicNoExp(f, g, x);
            case 1: return heuristicMemory(f, g, x);
            case 2: return heuristicNodePathCount(f, g, x);
            case 3: return heuristicLayer(f, g, x);
            case 4: return heuristicRandom(f, g, x);
        }

        return f;
    }

    edge_ptr heuristicNoExp(edge_ptr f, edge_ptr g, std::int32_t x)
    {
        return make_branch(x, conj(cof(f, x, true), cof(g, x, true)), conj(cof(f, x, false), cof(g, x, false)));
    }

    edge_ptr heuristicMemory(edge_ptr f, edge_ptr g, std::int32_t x)
    {
        std::ifstream statm("/proc/self/statm");
        if (statm.is_open()) {
            long size, resident, shared, text, lib, data, dt;
            statm >> size >> resident >> shared >> text >> lib >> data >> dt;
            statm.close();

            long pageSize = sysconf(_SC_PAGESIZE); // get the page size in bytes
            long rss = resident * pageSize; // resident set size in bytes

            //std::cout << "Physical memory used by process: " << rss / 1024 << " KB" << std::endl;


            if ((rss / 1024) > heuristicAtt){

                return replaceOnesWithExp(f, false, consts[2]);
            } else {
                edge_ptr conj2 = skipLowerVarsConj(f, g, x, false);
                edge_ptr conj1 = skipLowerVarsConj(f, g, x, true);

                return make_branch(x, conj1, conj2);
            }

        } else {
            std::cerr << "Failed to open /proc/self/statm." << std::endl;
            return 0;
        }
    }

    edge_ptr heuristicNodePathCount(edge_ptr f, edge_ptr g, std::int32_t x)
    {
        int amount = node_count() + edge_count();
        //std::cout << amount << "\n";

        if (amount >= heuristicAtt){
            return replaceOnesWithExp(f, false, consts[2]);
        }
        else {
            edge_ptr conj2 = skipLowerVarsConj(f, g, x, false);
            edge_ptr conj1 = skipLowerVarsConj(f, g, x, true);

            return make_branch(x, conj1, conj2);
        }
    }

    edge_ptr heuristicLayer(edge_ptr f, edge_ptr g, std::int32_t x)
    {

        if (f->v->is_const() || f->v->br().x < heuristicAtt){
            if (g->v->is_const() || g->v->br().x < heuristicAtt){
                edge_ptr conj2 = skipLowerVarsConj(f, g, x, false);
                edge_ptr conj1 = skipLowerVarsConj(f, g, x, true);

                return make_branch(x, conj1, conj2);
            }
            else {
                return replaceOnesWithExp(f, false, consts[2]);
            }
        }
        else {
            if (g->v->is_const() || g->v->br().x < heuristicAtt){
                return replaceOnesWithExp(g, false, consts[2]);
            }
            else {
                return consts[2];
            }
        }
    }

    edge_ptr heuristicRandom(edge_ptr f, edge_ptr g, std::int32_t x)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(1, 1000000);
        int randomNumber = dist(gen);

        if (randomNumber <= 1000000 - heuristicAtt) {
            edge_ptr conj2 = skipLowerVarsConj(f, g, x, false);
            edge_ptr conj1 = skipLowerVarsConj(f, g, x, true);

            return make_branch(x, conj1, conj2);
        }

        //lo becomes extension
        else if (randomNumber <= 1000000 - (heuristicAtt/2)){
            edge_ptr conj = skipLowerVarsConj(f, g, x, true);
            return make_branch(x, conj, consts[2]);

            // hi becomes extension
        } else {
            edge_ptr conj = skipLowerVarsConj(f, g, x, false);
            return make_branch(x, consts[2], conj);
        }
    }


    edge_ptr skipLowerVarsConj(edge_ptr f, edge_ptr g, std::int32_t x, bool varA)
    {
        auto cofR = cof(g, x, varA);
        if (cofR->w < 0 && (!f->v || f->v->br().x != x)){
            return cofR;
        } else {
            auto  cofL = cof(f, x, varA);

            if (cofL->w < 0 && (!g->v || g->v->br().x != x)){
                return cofL;
            } else {
                return conj(cofL, cofR);
            }
        }
    }

    edge_ptr replaceOnesWithExp(edge_ptr f, bool goesTrue, edge_ptr ex) {
        assert(f);
        assert(ex);

        if (f == consts[2] || f == consts[3]){
            return f;
        }

        if (goesTrue){
            if (f == consts[0]){
                if (ex == consts[2]){
                    return consts[3];
                }
                if (ex == consts[3]){
                    return consts[2];
                }
            }
            if (f == consts[1]){
                return f;
            }
        } else {
            if (f == consts[0]){
                return f;
            }
            if (f == consts[1]){
                return ex;
            }
        }



        if (f->w == false) {
            auto hi = replaceOnesWithExp(f->v->br().hi, goesTrue, ex);
            auto lo = replaceOnesWithExp(f->v->br().lo, goesTrue, ex);
            auto w = lo->w;

            if (w == 1){
                hi = complement(hi);
                lo = complement(lo);
            }

            if (f->w == 1){
                (w == 0) ? w = 1 : w = 0;
            }

            return foa(std::make_shared<bool_edge>(w, foa(std::make_shared<bool_node>(f->v->br().x, hi, lo))));

        }
        else{
            auto hi = replaceOnesWithExp(f->v->br().hi, !goesTrue, ex);
            auto lo = replaceOnesWithExp(f->v->br().lo, !goesTrue, ex);
            auto w = lo->w;

            if (w == 1){
                hi = complement(hi);
                lo = complement(lo);
            }

            if (f->w == 1){
                (w == 0) ? w = 1 : w = 0;
            }

            return foa(std::make_shared<bool_edge>(w, foa(std::make_shared<bool_node>(f->v->br().x, hi, lo))));

        }
    }



    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        return complement(conj(complement(f), complement(g)));
    }

    auto make_branch(std::int32_t const x, edge_ptr hi, edge_ptr lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == lo)  // redundancy rule
        {
            return hi;  // without limitation of generality
        }

        auto const w = lo->w;
        return foa(
            std::make_shared<bool_edge>(w, foa(std::make_shared<bool_node>(x, !w ? std::move(hi) : complement(hi),
                                                                           !w ? std::move(lo) : complement(lo)))));
    }

    [[nodiscard]] auto merge(bool const& val1, bool const& val2) const noexcept -> bool override
    {
        std::cout << "merge WIRD BENUTZT\n";

        return !(val1 == val2);
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        std::cout << "mul WIRD BENUTZT\n";

        assert(f);
        assert(g);

        return conj(f, g);
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;  // means a regular (non-complemented) edge
    }

    auto static transform(std::vector<bhd> const& fs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> gs;
        gs.reserve(fs.size());
        std::transform(fs.begin(), fs.end(), std::back_inserter(gs), [](auto const& g) { return g.f; });

        return gs;
    }

    auto static tmls() -> std::array<edge_ptr, 2>
    {
        // choose the 0-leaf due to complemented edges in order to ensure canonicity
        auto const leaf = std::make_shared<bool_node>(false);

        return std::array<edge_ptr, 2>{std::make_shared<bool_edge>(false, leaf),
                                       std::make_shared<bool_edge>(true, leaf)};
    }
};

auto inline bhd::operator~() const
{
    assert(mgr);

    return bhd{mgr->complement(f), mgr};
}

auto inline bhd::operator&=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);
    return *this;
}

auto inline bhd::operator|=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);
    return *this;
}

auto inline bhd::operator^=(bhd const& rhs) -> bhd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);
    return *this;
}

auto inline bhd::is_zero() const noexcept
{
    assert(mgr);

    return (*this == mgr->zero());
}

auto inline bhd::is_one() const noexcept
{
    assert(mgr);

    return (*this == mgr->one());
}

auto inline bhd::high(bool const weighting) const
{
    assert(mgr);

    return bhd{mgr->high(f, weighting), mgr};
}

auto inline bhd::low(bool const weighting) const
{
    assert(mgr);

    return bhd{mgr->low(f, weighting), mgr};
}

template <typename T, typename... Ts>
auto inline bhd::cof(T const a, Ts... args) const
{
    assert(mgr);

    return bhd{mgr->subfunc(f, a, std::forward<Ts>(args)...), mgr};
}

auto inline bhd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

auto inline bhd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

auto inline bhd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

auto inline bhd::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);
    assert(static_cast<std::int32_t>(as.size()) == mgr->var_count());

    return mgr->eval(f, as);
}

auto inline bhd::has_const(bool const c) const
{
    assert(mgr);

    return mgr->has_const(f, c);
}

auto inline bhd::is_essential(std::int32_t const x) const
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

auto inline bhd::ite(bhd const& g, bhd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);  // transitive property

    return bhd{mgr->ite(f, g.f, h.f), mgr};
}

auto inline bhd::compose(std::int32_t const x, bhd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bhd{mgr->compose(f, x, g.f), mgr};
}

auto inline bhd::restr(std::int32_t const x, bool const a) const
{
    assert(mgr);

    return bhd{mgr->restr(f, x, a), mgr};
}

auto inline bhd::exist(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->exist(f, x), mgr};
}

auto inline bhd::forall(std::int32_t const x) const
{
    assert(mgr);

    return bhd{mgr->forall(f, x), mgr};
}

auto inline bhd::satcount() const
{
    assert(mgr);

    return mgr->satcount(f);
}

auto inline bhd::print() const
{
    assert(mgr);

    mgr->print({*this});
}

auto inline bhd::createExpansionFiles() const{
    assert(mgr);

    for (const auto& entry : std::filesystem::directory_iterator("ExpansionNodes")) {
        std::filesystem::remove_all(entry.path());
    }

    mgr->createExpansionFiles(f);
}

}  // namespace freddy::dd
