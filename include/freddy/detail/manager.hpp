#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"            // parallel_for
#include "edge.hpp"              // edge
#include "entry.hpp"             // entry
#include "freddy/config.hpp"     // config::ct_size
#include "freddy/expansion.hpp"  // expansion
#include "freddy/operation.hpp"  // operation
#include "node.hpp"              // node
#include "variable.hpp"          // variable

#include <algorithm>      // std::max_element
#include <array>          // std::array
#include <cassert>        // assert
#include <concepts>       // std::same_as
#include <cstdint>        // std::int32_t
#include <memory>         // std::shared_ptr
#include <numeric>        // std::accumulate
#include <ostream>        // std::ostream
#include <string>         // std::string
#include <string_view>    // std::string_view
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <utility>        // std::pair
#include <vector>         // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

template <typename E, typename V>  // edge weight, node value
class manager
{
  public:  // methods that do not need to be wrapped
    using edge_ptr = std::shared_ptr<edge<E, V>>;

    explicit manager(std::array<edge_ptr, 2> tmls)  // contradiction, tautology
    {
        ec.reserve(config::ut_size);
        lvl2var.reserve(config::vl_size);
        nc.reserve(config::ut_size);
        vl.reserve(config::vl_size);

        vars.reserve(config::vl_size);
        consts.reserve(tmls.size());
        ct.reserve(config::ct_size);
        var2lvl.reserve(config::vl_size);

        for (edge_ptr& tml : tmls)
        {
            nc.insert(tml->v);
            consts.push_back(*ec.insert(std::move(tml)).first);
        }
    }

    manager(manager const&) = default;

    manager(manager&&) noexcept = default;

    auto operator=(manager const&) -> manager& = default;

    auto operator=(manager&&) noexcept -> manager& = default;

    auto friend operator<<(std::ostream& s, manager const& mgr) -> std::ostream&
    {
        for (auto const& var : mgr.vl)
        {
            s << "Variable: " << var << '\n';
        }

        s << "Ordering: ";
        for (auto lvl = 0; lvl < static_cast<std::int32_t>(mgr.lvl2var.size()); ++lvl)
        {
            s << mgr.vl[mgr.lvl2var[lvl]].l;

            if (lvl + 1 < static_cast<std::int32_t>(mgr.lvl2var.size()))
            {  // there is at least one more iteration
                s << " < ";
            }
        }

        auto print = [&s](auto const& uc) {
            for (auto i = 0; i < static_cast<std::int32_t>(uc.bucket_count()); ++i)
            {
                if (uc.bucket_size(i) > 0)
                {
                    s << "| " << i << " | ";
                    for (auto it = uc.begin(i); it != uc.end(i); ++it)
                    {
                        s << *it << *(*it) << '[' << it->use_count() << "] ";
                    }
                    s << "|\n";
                }
            }
        };

        s << "\nEC:\n";
        print(mgr.ec);
        s << "#Edges = " << mgr.ec.size();
        s << "\nOccupancy = " << mgr.ec.load_factor();

        s << "\nNC:\n";
        print(mgr.nc);
        s << "#Nodes = " << mgr.nc.size();
        s << "\nOccupancy = " << mgr.nc.load_factor();

        s << "\nCT:\n";
        for (auto const& [key, val] : mgr.ct)
        {
            s << key << " -> ";
            if (val.first.lock())
            {
                s << val.first.lock();
            }
            else
            {
                s << val.second;
            }
            s << '\n';
        }
        s << "#Entries = " << mgr.ct.size();
        s << "\nOccupancy = " << mgr.ct.load_factor();

        return s;
    }

    virtual ~manager() noexcept = default;

    [[nodiscard]] auto var_count() const noexcept
    {
        return static_cast<std::int32_t>(vl.size());
    }

    [[nodiscard]] auto const_count() const noexcept
    {
        return nc.size();
    }

    [[nodiscard]] auto node_count() const noexcept
    {
        return (std::accumulate(
                    vl.begin(), vl.end(), 0,
                    [](auto const sum, auto const& var) { return (sum + static_cast<std::int32_t>(var.nt.size())); }) +
                static_cast<std::int32_t>(nc.size()));
    }

    [[nodiscard]] auto edge_count() const noexcept
    {
        return (std::accumulate(
                    vl.begin(), vl.end(), 0,
                    [](auto const sum, auto const& var) { return (sum + static_cast<std::int32_t>(var.et.size())); }) +
                ec.size());
    }

    // NOLINTBEGIN
    auto swap(std::int32_t const lvl_x, std::int32_t const lvl_y)
    {
        assert(lvl_x < var_count());
        assert(lvl_y < var_count());

        sift(lvl_x, lvl_y);
        sift((lvl_x < lvl_y) ? lvl_y - 1 : lvl_y + 1, lvl_x);
    }
    // NOLINTEND

    auto reorder()
    {
        auto ncnt_min = node_count();
        for (auto x = 0; x < var_count(); ++x)
        {
            auto lvl_min = var2lvl[x];
            auto lvl_stop = sift_down(var2lvl[x], lvl_min, ncnt_min);
            lvl_stop = sift_up(lvl_stop, lvl_min, ncnt_min);
            sift(lvl_stop, lvl_min);
        }
    }

    auto virtual gc() noexcept -> void  // performance of many EDA tasks depends on garbage collection
    {
        ct.clear();  // to avoid invalid results

        auto cleanup = [](auto& ut) {
            for (auto it = ut.begin(); it != ut.end();)
            {
                if (it->use_count() == 1)
                {  // only the UT references this node/edge => node/edge is dead
                    it = ut.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        };
        for (auto const lvl : lvl2var)
        {  // an increased chance of deleting nodes/edges
            cleanup(vl[lvl].et);
            cleanup(vl[lvl].nt);
        }
        cleanup(ec);
        cleanup(nc);
    }

  protected:  // generally intended for overriding and wrapping methods
    using node_ptr = std::shared_ptr<node<E, V>>;

    auto make_var(expansion const t, std::string_view l)
    {
        assert(consts.size() >= 2);

        vl.emplace_back(t, l.empty() ? 'x' + std::to_string(var_count()) : l);
        vars.push_back(foa(std::make_shared<edge<E, V>>(
            regw(), foa(std::make_shared<node<E, V>>(var_count() - 1, consts[1], consts[0])))));
        var2lvl.push_back(var_count() - 1);
        lvl2var.push_back(var_count() - 1);

        return vars[var_count() - 1];
    }

    auto make_const(E w, V c)
    {
        return foa(std::make_shared<edge<E, V>>(std::move(w), foa(std::make_shared<node<E, V>>(std::move(c)))));
    }

    auto high(edge_ptr const& f, bool const weighting)
    {
        assert(f);
        assert(!f->v->is_const());

        return (weighting ? apply(f->w, f->v->br().hi) : f->v->br().hi);
    }

    auto low(edge_ptr const& f, bool const weighting)
    {
        assert(f);
        assert(!f->v->is_const());

        return (weighting ? apply(f->w, f->v->br().lo) : f->v->br().lo);
    }

    template <typename T>
    requires std::same_as<T, bool>
    auto subfunc(edge_ptr const& f, T const a)
    {
        assert(f);
        assert(!f->v->is_const());

        return (a ? high(f, true) : low(f, true));
    }

    template <typename T, typename... Ts>
    requires std::same_as<T, bool>
    auto subfunc(edge_ptr const& f, T const a, Ts... args)
    {
        assert(f);
        assert(!f->v->is_const());

        return (a ? subfunc(high(f, true), args...) : subfunc(low(f, true), args...));
    }

    [[nodiscard]] auto node_count(std::vector<edge_ptr> const& fs) const
    {
        auto marks = get_marks();

        for (auto const& f : fs)
        {  // (shared) number of nodes
            node_count(f, marks);
        }

        return marks.size();  // including leaves
    }

    [[nodiscard]] auto path_count(edge_ptr const& f) const noexcept -> double  // as results can be very large
    {
        assert(f);

        return (f->v->is_const() ? 1 : path_count(f->v->br().hi) + path_count(f->v->br().lo));  // DFS
    }

    [[nodiscard]] auto longest_path(std::vector<edge_ptr> const& fs) const
    {
        assert(!fs.empty());

        std::vector<std::int32_t> ds(fs.size());

        parallel_for(0, static_cast<std::int32_t>(ds.size()), [&ds, &fs, this](auto const i) {
            //std::cout << std::this_thread::get_id() << std::endl;
            ds[i] = longest_path_rec(fs[i]);
        });

        return (*std::max_element(ds.begin(), ds.end()) - 1);  // due to the root edge
    }

    [[nodiscard]] auto eval(edge_ptr const& f, std::vector<bool> const& as) const
    {
        assert(f);
        assert(static_cast<std::int32_t>(as.size()) == var_count());  // assignment follows the made variable ordering

        return agg(f->w, eval(f->v, as));
    }

    [[nodiscard]] auto has_const(edge_ptr const& f, V const& c) const -> bool
    {
        assert(f);

        return (f->v->is_const() ? f->v->c() == c : has_const(f->v->br().hi, c) || has_const(f->v->br().lo, c));
    }

    [[nodiscard]] auto is_essential(edge_ptr const& f, std::int32_t const x) const -> bool
    {
        assert(f);
        assert(x < var_count());

        return ((f->v->is_const() || var2lvl[f->v->br().x] > var2lvl[x])
                    ? false
                    : f->v->br().x == x || is_essential(f->v->br().hi, x) || is_essential(f->v->br().lo, x));
    }

    auto compose(edge_ptr const& f, std::int32_t const x, edge_ptr const& g)
    {
        assert(f);
        assert(x < var_count());
        assert(g);

        if (f->v->is_const() || !is_essential(f, x))
        {
            return f;
        }

        auto const cr = ct.find({operation::COMPOSE, f, x, g});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        edge_ptr r;
        edge_ptr hi;
        edge_ptr lo;
        if (f->v->br().x == x)
        {
            hi = mul(f->v->br().hi, g);

            switch (vl[x].t)
            {
                case expansion::PD: lo = f->v->br().lo; break;
                case expansion::S:
                {
                    lo = mul(f->v->br().lo, complement(g));
                    break;
                }
                default: assert(false);
            }
        }
        else
        {
            hi = mul(vars[f->v->br().x], compose(f->v->br().hi, x, g));

            switch (vl[f->v->br().x].t)
            {
                case expansion::PD: lo = compose(f->v->br().lo, x, g); break;
                case expansion::S:
                {
                    lo = mul(complement(vars[f->v->br().x]), compose(f->v->br().lo, x, g));
                    break;
                }
                default: assert(false);
            }
        }
        r = apply(f->w, add(hi, lo));

        ct.insert_or_assign({operation::COMPOSE, f, x, g}, std::make_pair(r, 0.0));  // overwrite in case of a collision

        return r;
    }

    auto cof(edge_ptr const& f, std::int32_t const x, bool const a)
    {
        assert(f);
        assert(x < var_count());
        assert(!consts.empty());

        return ((f->v->is_const() || f->v->br().x != x)
                    ? ((vl[x].t == expansion::PD && a) ? consts[0] : f)
                    : (a ? apply(f->w, f->v->br().hi) : apply(f->w, f->v->br().lo)));
    }

    auto restr(edge_ptr const& f, std::int32_t const x, bool const a)
    {
        assert(f);
        assert(x < var_count());

        if (!is_essential(f, x))
        {
            return f;
        }
        if (f->v->br().x == x)
        {
            return cof(f, x, a);
        }

        auto const cr = ct.find({operation::RESTR, f, x, a});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto r = apply(f->w, make_branch(f->v->br().x, restr(f->v->br().hi, x, a), restr(f->v->br().lo, x, a)));

        ct.insert_or_assign({operation::RESTR, f, x, a}, std::make_pair(r, 0.0));

        return r;
    }

    auto exist(edge_ptr const& f, std::int32_t const x)
    {
        assert(f);
        assert(x < var_count());

        return disj(restr(f, x, true), restr(f, x, false));
    }

    auto forall(edge_ptr const& f, std::int32_t const x)
    {
        assert(f);
        assert(x < var_count());

        return conj(restr(f, x, true), restr(f, x, false));
    }

    [[nodiscard]] auto top_var(edge_ptr const& f, edge_ptr const& g) const
    {
        assert(f);
        assert(g);
        assert(!f->v->is_const() || !g->v->is_const());

        if (f->v->is_const())
        {
            return g->v->br().x;
        }
        if (g->v->is_const())
        {
            return f->v->br().x;
        }
        return ((var2lvl[f->v->br().x] <= var2lvl[g->v->br().x]) ? f->v->br().x : g->v->br().x);
    }

    auto foa(node_ptr v) -> node_ptr
    {
        assert(v);

        if (v->is_const())
        {
            ctrl(nc);
            return *nc.insert(std::move(v)).first;  // in case of a collision, "v" is the list head
        }
        ctrl(vl[v->br().x].nt);
        return *vl[v->br().x].nt.insert(std::move(v)).first;
    }

    auto foa(edge_ptr e) -> edge_ptr
    {
        assert(e);

        if (e->v->is_const())
        {
            ctrl(ec);
            return *ec.insert(std::move(e)).first;
        }
        ctrl(vl[e->v->br().x].et);
        return *vl[e->v->br().x].et.insert(std::move(e)).first;
    }

    // many optimizations are possible depending on the DD type
    auto virtual apply(E const& w, edge_ptr const& f) -> edge_ptr
    {
        assert(f);

        return foa(std::make_shared<detail::edge<E, V>>(comb(w, f->w), f->v));
    }

    auto virtual ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr  // as there are different versions
    {
        assert(f);
        assert(g);
        assert(h);

        return disj(conj(f, g), conj(complement(f), h));
    }

    auto virtual to_dot(std::vector<edge_ptr> const& fs, std::vector<std::string> const& outputs, std::ostream& s) const
        -> void
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        s << "digraph DD {\n";
        s << 'f' << " [style=invis];\n";
        s << 'c' << " [style=invis];\n";

        if (var_count() == 0)
        {
            s << "f -> c [style=invis];\n";
        }
        for (auto i = 0; i < var_count(); ++i)  // arrange nodes per level
        {
            if (i == 0)
            {
                s << "f -> x" << lvl2var[i] << " [style=invis];\n";
            }

            s << 'x' << lvl2var[i] << R"( [shape=plaintext,fontname="times italic",label=")" << lvl2var[i] << "\"];\n";

            if (i + 1 < var_count())
            {
                s << 'x' << lvl2var[i] << " -> x" << lvl2var[i + 1] << " [style=invis];\n";
            }
            else
            {
                s << 'x' << lvl2var[i] << " -> c [style=invis];\n";
            }
        }

        auto marks = get_marks();

        for (auto i = 0; i < static_cast<std::int32_t>(fs.size()); ++i)
        {
            assert(fs[i]);

            s << 'f' << fs[i] << R"( [shape=plaintext,fontname="times bold",label=")"
              << (outputs.empty() ? 'f' + std::to_string(i) : outputs[i]) << "\"]\n";
            s << "{ rank=same; f; f" << fs[i] << "; }\n";
            s << 'f' << fs[i] << " -> v" << fs[i]->v << " [label=\" " << fs[i]->w << " \"];\n";

            to_dot(fs[i], marks, s);
        }

        s << "}\n";
    }

    auto virtual add(edge_ptr, edge_ptr) -> edge_ptr = 0;  // combines DDs additively

    [[nodiscard]] auto virtual agg(E const&, V const&) const -> V = 0;  // aggregates an edge weight and a node value

    // adjusts a pair weight according to the DD type
    [[nodiscard]] auto virtual comb(E const&, E const&) const -> E = 0;

    auto virtual complement(edge_ptr const&) -> edge_ptr = 0;  // computes NOT

    auto virtual conj(edge_ptr, edge_ptr) -> edge_ptr = 0;  // connects conjuncts logically (AND)

    auto virtual disj(edge_ptr const&, edge_ptr const&) -> edge_ptr = 0;  // connects disjuncts logically (OR)

    // creates/reuses a node and an incoming edge
    auto virtual make_branch(std::int32_t, edge_ptr, edge_ptr) -> edge_ptr = 0;

    [[nodiscard]] auto virtual merge(V const&, V const&) const -> V = 0;  // evaluates aggregates (subtrees)

    // combines DDs multiplicatively
    auto virtual mul(edge_ptr, edge_ptr) -> edge_ptr = 0;

    [[nodiscard]] auto virtual regw() const -> E = 0;  // returns the regular weight of an edge

    std::vector<edge_ptr> vars;  // DD variables that are never cleared

    std::vector<edge_ptr> consts;  // DD constants that are never cleared

    // to cache operation results
    std::unordered_map<entry<E, V>, std::pair<std::weak_ptr<edge<E, V>>, double>, typename entry<E, V>::hash> ct;

    std::vector<std::int32_t> var2lvl;  // for reordering
  private:
    template <typename T>
    auto ctrl(T& ut) -> void
    {
        if (ut.load_factor() < config::load_factor)
        {
            return;
        }
        // collision probability is too high => clean up nodes/edges
        auto const old_lf = ut.load_factor();
        //gc();

        if (ut.load_factor() > old_lf - config::dead_factor)
        {  // too few nodes were deleted => resize/rehash this UT
            ut.reserve(2 * ut.bucket_count());
        }
    }

    [[nodiscard]] auto eval(node_ptr const& v, std::vector<bool> const& as) const -> V
    {
        assert(v);
        assert(static_cast<std::int32_t>(as.size()) == var_count());

        if (v->is_const())
        {
            return v->c();
        }

        V r{};
        switch (vl[v->br().x].t)
        {
            case expansion::PD:
                r = as[v->br().x] ? merge(eval(v->br().hi, as), eval(v->br().lo, as)) : eval(v->br().lo, as);
                break;
            case expansion::S:
            {
                r = as[v->br().x] ? eval(v->br().hi, as) : eval(v->br().lo, as);
                break;
            }
            default: assert(false);
        }
        return r;
    }

    auto exchange(std::int32_t const lvl)  // with the level below
    {
        assert(lvl < var_count());
        assert(var_count() > 1);

        if (lvl == var_count() - 1)
        {
            return;
        }

        gc();  // ensure that dead nodes are not swapped

        auto const x = lvl2var[lvl];
        auto const y = lvl2var[lvl + 1];

        auto swap_is_needed = [y](auto const& hi, auto const& lo) {
            assert(hi);
            assert(lo);

            if (hi->v->is_const() && lo->v->is_const())
            {
                return false;
            }
            if (hi->v->is_const() || lo->v->is_const())
            {
                return (hi->v->is_const() ? lo->v->br().x == y : hi->v->br().x == y);
            }
            return (hi->v->br().x == y || lo->v->br().x == y);
        };

        for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();)
        {
            if (swap_is_needed((*it)->br().hi, (*it)->br().lo))  // swapping levels is a local operation
            {
                auto v = *it;
                it = vl[x].nt.erase(it);

                auto const hi = make_branch(x, cof(v->br().hi, y, true), cof(v->br().lo, y, true));
                v->br().lo = make_branch(x, cof(v->br().hi, y, false), cof(v->br().lo, y, false));
                v->br().hi = hi;
                v->br().x = y;

                foa(v);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = vl[x].et.begin(); it != vl[x].et.end();)
        {  // swap edges pointing to swapped nodes
            if ((*it)->v->br().x == y)
            {
                foa(*it);
                it = vl[x].et.erase(it);
            }
            else
            {
                ++it;
            }
        }

        gc();  // clean up possible dead nodes

        std::swap(lvl2var[lvl], lvl2var[lvl + 1]);
        std::swap(var2lvl[x], var2lvl[y]);
    }

    [[nodiscard]] auto get_marks() const -> std::unordered_set<node_ptr, hash, comp>
    {
        std::unordered_set<node_ptr, hash, comp> marks;
        marks.max_load_factor(0.7f);
        return marks;
    }

    [[nodiscard]] auto longest_path_rec(edge_ptr const& f) const noexcept -> std::int32_t
    {
        assert(f);

        return (f->v->is_const() ? 1
                                 : (std::max(longest_path_rec(f->v->br().hi), longest_path_rec(f->v->br().lo)) + 1));
    }

    auto node_count(edge_ptr const& f, std::unordered_set<node_ptr, hash, comp>& marks) const -> void
    {
        assert(f);

        if (marks.find(f->v) != marks.end())
        {  // node has already been visited
            return;
        }

        marks.insert(f->v);

        if (!f->v->is_const())
        {  // DD traversal
            node_count(f->v->br().hi, marks);
            node_count(f->v->br().lo, marks);
        }
    }

    auto sift(std::int32_t const lvl_x, std::int32_t const lvl_y) -> void
    {
        assert(lvl_x < var_count());
        assert(lvl_y < var_count());

        if (lvl_x == lvl_y)
        {
            return;
        }

        auto const start = (lvl_x < lvl_y) ? lvl_x : lvl_x - 1;
        auto const stop = (lvl_x < lvl_y) ? lvl_y : lvl_y - 1;
        auto const step = (lvl_x < lvl_y) ? 1 : -1;

        for (auto lvl = start; lvl != stop; lvl += step)
        {
            exchange(lvl);
        }
    }

    auto sift_down(std::int32_t lvl, std::int32_t& lvl_min, std::int32_t& ncnt_min) -> std::int32_t
    {
        assert(lvl < var_count());
        assert(lvl_min < var_count());
        assert(ncnt_min > 0);

        auto ncnt_start = 0;
        auto ncnt_end = 0;
        while (lvl != var_count() - 1 &&
               config::growth_factor * static_cast<float>(ncnt_start) >= static_cast<float>(ncnt_end))
        {
            ncnt_start = node_count();
            exchange(lvl++);
            ncnt_end = node_count();

            if (ncnt_end < ncnt_min)
            {
                lvl_min = lvl;
                ncnt_min = ncnt_end;
            }
        }

        return lvl;
    }

    auto sift_up(std::int32_t lvl, std::int32_t& lvl_min, std::int32_t& ncnt_min) -> std::int32_t
    {
        assert(lvl < var_count());
        assert(lvl_min < var_count());
        assert(ncnt_min > 0);

        auto ncnt_start = 0;
        auto ncnt_end = 0;
        while (--lvl != -1 && config::growth_factor * static_cast<float>(ncnt_start) >= static_cast<float>(ncnt_end))
        {
            ncnt_start = node_count();
            exchange(lvl);
            ncnt_end = node_count();

            if (ncnt_end < ncnt_min)
            {
                lvl_min = lvl;
                ncnt_min = ncnt_end;
            }
        }

        return ++lvl;
    }

    auto to_dot(edge_ptr const& f, std::unordered_set<node_ptr, hash, comp>& marks, std::ostream& s) const -> void
    {
        assert(f);

        if (marks.find(f->v) != marks.end())
        {
            return;
        }
        marks.insert(f->v);

        if (f->v->is_const())
        {
            s << 'v' << f->v << " [shape=box,style=filled,color=brown,fontcolor=white,label=\"" << f->v->c()
              << "\"];\n";
            s << "{ rank=same; c; v" << f->v << "; }\n";

            return;
        }

        s << 'v' << f->v << " [shape=circle,style=filled,color=black,fontcolor=white,label=\"" << vl[f->v->br().x].l
          << "\"];\n";
        s << "{ rank=same; x" << f->v->br().x << "; v" << f->v << "; }\n";
        s << 'v' << f->v << " -> v" << f->v->br().hi->v << " [color=blue,dir=none,label=\" " << f->v->br().hi->w
          << " \"];\n";
        s << 'v' << f->v << " -> v" << f->v->br().lo->v << " [style=dashed,color=red,dir=none,label=\" "
          << f->v->br().lo->w << " \"];\n";

        to_dot(f->v->br().hi, marks, s);
        to_dot(f->v->br().lo, marks, s);
    }

    std::unordered_set<edge_ptr, hash, comp> ec;

    std::vector<std::int32_t> lvl2var;

    std::unordered_set<node_ptr, hash, comp> nc;  // constants

    std::vector<variable<E, V>> vl;
};

}  // namespace freddy::detail
