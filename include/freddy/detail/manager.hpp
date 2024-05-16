#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"         // parallel_for
#include "freddy/config.hpp"  // config::vl_size
#include "variable.hpp"       // variable

#include <algorithm>      // std::max_element
#include <array>          // std::array
#include <cassert>        // assert
#include <concepts>       // std::same_as
#include <cstddef>        // std::size_t
#include <cstdint>        // std::int32_t
#include <functional>     // std::hash
#include <memory>         // std::shared_ptr
#include <numeric>        // std::accumulate
#include <optional>       // std::optional
#include <ostream>        // std::ostream
#include <string>         // std::string
#include <string_view>    // std::string_view
#include <type_traits>    // std::underlying_type
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <utility>        // std::pair
#include <vector>         // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

class manager
{
  public:
    manager()
    {
        ct.reserve(config::ct_size);
        tmls[0] = foa(std::make_shared<edge>(0));
        tmls[1] = foa(std::make_shared<edge>(1));
        tmls[2] = foa(std::make_shared<edge>(2));
        tmls[3] = foa(std::make_shared<edge>(-1));
        var2lvl.reserve(config::vl_size);
        vars.reserve(config::vl_size);

        consts.reserve(config::ut_size);
        lvl2var.reserve(config::vl_size);
        vl.reserve(config::vl_size);
    }

    manager(manager const&) = default;

    manager(manager&&) = default;

    auto operator=(manager const&) -> manager& = default;

    auto operator=(manager&&) -> manager& = default;

    auto friend operator<<(std::ostream& s, manager const& mgr) -> std::ostream&
    {
        for (auto const& var : mgr.vl)
        {
            s << "Variable: " << var << '\n';
        }

        s << "Ordering: ";
        for (auto lvl = 0; lvl < static_cast<std::int32_t>(mgr.lvl2var.size()); ++lvl)
        {
            s << mgr.vl[mgr.lvl2var[lvl]].l << ' ';
        }

        s << "\nConstants:\n";
        for (auto i = 0; i < static_cast<std::int32_t>(mgr.consts.bucket_count()); ++i)
        {
            if (mgr.consts.bucket_size(i) > 0)
            {
                s << "| " << i << " | ";
                for (auto it = mgr.consts.begin(i); it != mgr.consts.end(i); ++it)
                {
                    s << *it << '(' << *(*it) << ")[" << it->use_count() << "] ";
                }
                s << "|\n";
            }
        }
        s << "#Constants = " << mgr.consts.size();
        s << "\nOccupancy = " << mgr.consts.load_factor();

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

    [[nodiscard]] auto node_count() const noexcept
    {
        return std::accumulate(vl.begin(), vl.end(), 0, [](auto const sum, auto const& var) {
            return (sum + static_cast<std::int32_t>(var.nt.size()));
        });
    }

    [[nodiscard]] auto edge_count() const noexcept
    {
        return (std::accumulate(
                    vl.begin(), vl.end(), 0,
                    [](auto const sum, auto const& var) { return (sum + static_cast<std::int32_t>(var.et.size())); }) +
                static_cast<std::int32_t>(consts.size()));
    }

    [[nodiscard]] auto var_count() const noexcept
    {
        return static_cast<std::int32_t>(vl.size());
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
        auto nc_min = node_count();
        for (auto x = 0; x < var_count(); ++x)
        {
            auto lvl_min = var2lvl[x];
            auto lvl_stop = sift_down(var2lvl[x], lvl_min, nc_min);
            lvl_stop = sift_up(lvl_stop, lvl_min, nc_min);
            sift(lvl_stop, lvl_min);
        }
    }

    auto gc() noexcept  // garbage collection
    {
        ct.clear();  // to avoid invalid results

        auto cleanup = [](auto& ut) {
            for (auto it = ut.begin(); it != ut.end();)
            {
                if (it->use_count() == 1)
                {  // only the UT references this edge/node => edge/node is dead
                    it = ut.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        };
        for (auto const lvl : lvl2var)
        {  // an increased chance of deleting edges/nodes
            cleanup(vl[lvl].et);
            cleanup(vl[lvl].nt);
        }
        cleanup(consts);
    }

  protected:
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

    struct entry  // for caching
    {
        entry(operation const op, std::shared_ptr<edge> f) :
                op{op},
                f{std::move(f)}
        {
            assert(this->f);
        }

        entry(operation const op, std::shared_ptr<edge> f, std::shared_ptr<edge> g) :
                entry(op, std::move(f))
        {
            assert(g);

            this->g = std::move(g);
        }

        entry(operation const op, std::shared_ptr<edge> f, std::shared_ptr<edge> g, std::shared_ptr<edge> h) :
                entry(op, std::move(f), std::move(g))
        {
            assert(h);

            this->h = std::move(h);
        }

        entry(operation const op, std::shared_ptr<edge> f, std::int32_t const x, std::shared_ptr<edge> g) :
                entry(op, std::move(f), std::move(g))
        {
            assert(x >= 0);

            this->x = x;
        }

        entry(operation const op, std::shared_ptr<edge> f, std::int32_t const x, bool const a) :
                entry(op, std::move(f))
        {
            assert(x >= 0);

            this->x = x;
            this->a = a;
        }

        auto friend operator==(entry const& lhs, entry const& rhs) noexcept
        {
            return (lhs.op == rhs.op && lhs.f == rhs.f && lhs.g == rhs.g && lhs.h == rhs.h && lhs.x == rhs.x &&
                    lhs.a == rhs.a);
        }

        auto friend operator<<(std::ostream& s, entry const& o) -> std::ostream&
        {
            s << '{' << static_cast<std::int32_t>(o.op) << ',' << o.f;

            if (o.x != -1)
            {
                s << ',' << o.x;
            }
            if (o.g)
            {
                s << ',' << o.g;
            }
            if (o.h)
            {
                s << ',' << o.h;
            }
            if (o.a.has_value())
            {
                s << ',' << *o.a;
            }

            s << '}';

            return s;
        }

        operation op;

        std::shared_ptr<edge> f;

        std::shared_ptr<edge> g;

        std::shared_ptr<edge> h;  // for operations such as ITE

        std::int32_t x{-1};  // for substitution

        std::optional<bool> a;  // for restriction
    };

    struct hash_fn  // to suppress warnings
    {
        auto operator()(entry const& o) const noexcept -> std::size_t
        {
            return (std::hash<std::int32_t>()(static_cast<std::underlying_type<operation>::type>(o.op)) ^
                    std::hash<std::shared_ptr<edge>>()(o.f) ^ std::hash<std::shared_ptr<edge>>()(o.g) ^
                    std::hash<std::shared_ptr<edge>>()(o.h) ^ std::hash<std::int32_t>()(o.x) ^
                    std::hash<std::optional<bool>>()(o.a));
        }
    };

    auto make_var(decomposition const t, std::string_view l)
    {
        vl.emplace_back(t, l.empty() ? 'x' + std::to_string(var_count()) : l);
        vars.push_back(
            foa(std::make_shared<edge>(regw(), foa(std::make_shared<node>(var_count() - 1, tmls[1], tmls[0])))));
        var2lvl.push_back(var_count() - 1);
        lvl2var.push_back(var_count() - 1);

        return vars[var_count() - 1];
    }

    auto node_count(std::vector<std::shared_ptr<edge>> fs)
    {
        // (shared) number of nodes
        auto count = std::accumulate(fs.begin(), fs.end(), 0,
                                     [this](auto const sum, auto& f) { return (sum + node_count_rec(f)); });
        unmark();
        return ++count;  // including the leaf
    }

    [[nodiscard]] auto longest_path(std::vector<std::shared_ptr<edge>> const& fs) const
    {
        std::vector<std::int32_t> ds;
        ds.reserve(fs.size());

        for (auto const& f : fs)
        {
            ds.push_back(longest_path_rec(f));
        }

        return *std::max_element(ds.begin(), ds.end());
    }

    auto to_dot(std::vector<std::shared_ptr<edge>> f, std::vector<std::string> outputs, std::ostream& s)
    {
        assert(outputs.empty() ? true : outputs.size() == f.size());

        s << "digraph DD {\n";

        s << 'f' << " [style=invis];\n";
        s << 'c' << " [style=invis];\n";

        if (var_count() == 0)
        {
            s << "f -> c [style=invis];\n";
        }

        for (auto i = 0; i < var_count(); ++i)  // to arrange nodes per level
        {
            if (i == 0)
            {
                s << "f -> x" << lvl2var[i] << " [style=invis];\n";
            }

            s << 'x' << lvl2var[i] << " [shape=plaintext,label=\"" << lvl2var[i] << "\"];\n";

            if (i + 1 < var_count())
            {
                s << 'x' << lvl2var[i] << " -> x" << lvl2var[i + 1] << " [style=invis];\n";
            }
            else
            {
                s << 'x' << lvl2var[i] << " -> c [style=invis];\n";
            }
        }

        for (auto i = 0; i < static_cast<std::int32_t>(f.size()); ++i)
        {
            assert(f[i]);

            if (i == 0)
            {
                s << 'v' << tmls[0]->v << " [shape=box,label=\"" << regw() << "\"];\n";  // terminal is implied
                s << "{ rank=same; c; v" << tmls[0]->v << "; }\n";
            }

            s << 'f' << f[i] << " [label=\"" << (outputs.empty() ? 'f' + std::to_string(i) : outputs[i]) << "\"]\n";
            s << "{ rank=same; f; f" << f[i] << "; }\n";

            s << 'f' << f[i] << " -> v" << f[i]->v << " [style=dotted";
            if (f[i]->w == 0)  // 0 is hidden
            {
                s << "];\n";
            }
            else
            {
                s << ",label=\"" << f[i]->w << "\"];\n";
            }

            if (f[i]->v)
            {
                s << 'v' << f[i]->v << " [shape=circle,label=\"" << vl[f[i]->v->x].l << "\"];\n";
                s << "{ rank=same; x" << f[i]->v->x << "; v" << f[i]->v << "; }\n";

                to_dot(f[i], s);
            }
        }

        s << "}\n";

        unmark();
    }

    auto high(std::shared_ptr<edge> const& f, bool const weighting)
    {
        assert(f);

        return (f->v ? (weighting ? apply(f->w, f->v->hi) : f->v->hi) : tmls[1]);
    }

    auto low(std::shared_ptr<edge> const& f, bool const weighting)
    {
        assert(f);

        return (f->v ? (weighting ? apply(f->w, f->v->lo) : f->v->lo) : tmls[0]);
    }

    template <typename T>
        requires std::same_as<T, bool>
    auto subfunc(std::shared_ptr<edge> const& f, T const a)
    {
        assert(f);

        return (a ? high(f, true) : low(f, true));
    }

    template <typename T, typename... Ts>
        requires std::same_as<T, bool>
    auto subfunc(std::shared_ptr<edge> const& f, T const a, Ts... args)
    {
        assert(f);

        return (a ? subfunc(high(f, true), args...) : subfunc(low(f, true), args...));
    }

    template <typename T>
        requires std::same_as<T, bool>
    auto eval(std::shared_ptr<edge> const& f, T const a)
    {
        assert(f);

        return (a ? high(f, true)->w : low(f, true)->w);
    }

    template <typename T, typename... Ts>
        requires std::same_as<T, bool>
    auto eval(std::shared_ptr<edge> const& f, T const a, Ts... args)
    {
        assert(f);

        return (a ? eval(high(f, true), args...) : eval(low(f, true), args...));
    }

    [[nodiscard]] auto path_count(std::shared_ptr<edge> const& f) const noexcept
    {
        assert(f);

        if (!f->v)
        {
            return 1;
        }

        return (path_count(f->v->hi) + path_count(f->v->lo));  // DFS
    }

    [[nodiscard]] auto is_essential(std::shared_ptr<edge> const& f, std::int32_t const x) const noexcept
    {
        assert(f);
        assert(x < var_count());

        if (!f->v || var2lvl[f->v->x] > var2lvl[x])
        {
            return false;
        }

        return (f->v->x == x || is_essential(f->v->hi, x) || is_essential(f->v->lo, x));
    }

    auto compose(std::shared_ptr<edge> const& f, std::int32_t const x, std::shared_ptr<edge> const& g)
    {
        assert(f);
        assert(x < var_count());
        assert(g);

        if (!f->v || !is_essential(f, x))
        {
            return f;
        }

        auto const cr = ct.find({operation::COMPOSE, f, x, g});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        std::shared_ptr<edge> r;
        std::shared_ptr<edge> hi;
        std::shared_ptr<edge> lo;
        if (f->v->x == x)
        {
            switch (vl[x].t)
            {
                case decomposition::PD:
                    hi = mul(f->v->hi, g);
                    lo = f->v->lo;
                    break;
                case decomposition::S:
                {
                    hi = mul(f->v->hi, g);
                    lo = mul(f->v->lo, complement(g));
                    break;
                }
                default: assert(false);
            }
        }
        else
        {
            switch (vl[f->v->x].t)
            {
                case decomposition::PD:
                    hi = mul(vars[f->v->x], compose(f->v->hi, x, g));
                    lo = compose(f->v->lo, x, g);
                    break;
                case decomposition::S:
                {
                    hi = mul(vars[f->v->x], compose(f->v->hi, x, g));
                    lo = mul(complement(vars[f->v->x]), compose(f->v->lo, x, g));
                    break;
                }
                default: assert(false);
            }
        }
        r = apply(f->w, add(hi, lo));

        // overwrite the currently cached result in case of a collision
        ct.insert_or_assign({operation::COMPOSE, f, x, g}, std::make_pair(r, 0.0));

        return r;
    }

    auto cof(std::shared_ptr<edge> const& f, std::int32_t const x, bool const a)
    {
        assert(f);
        assert(x < var_count());

        if (!f->v || f->v->x != x)
        {
            return ((vl[x].t == decomposition::PD && a) ? tmls[0] : f);
        }

        return (a ? apply(f->w, f->v->hi) : apply(f->w, f->v->lo));
    }

    auto restr(std::shared_ptr<edge> const& f, std::int32_t const x, bool const a)
    {
        assert(f);
        assert(x < var_count());

        if (!is_essential(f, x))
        {
            return f;
        }
        if (f->v->x == x)
        {
            return cof(f, x, a);
        }

        auto const cr = ct.find({operation::RESTR, f, x, a});
        if (cr != ct.end())
        {
            return cr->second.first.lock();
        }

        auto r = apply(f->w, make_branch(f->v->x, restr(f->v->hi, x, a), restr(f->v->lo, x, a)));

        ct.insert_or_assign({operation::RESTR, f, x, a}, std::make_pair(r, 0.0));

        return r;
    }

    auto exist(std::shared_ptr<edge> const& f, std::int32_t const x)
    {
        assert(f);
        assert(x < var_count());

        return disj(restr(f, x, true), restr(f, x, false));
    }

    auto forall(std::shared_ptr<edge> const& f, std::int32_t const x)
    {
        assert(f);
        assert(x < var_count());

        return conj(restr(f, x, true), restr(f, x, false));
    }

    [[nodiscard]] auto top_var(std::shared_ptr<edge> const& f, std::shared_ptr<edge> const& g) const noexcept
    {
        assert(f);
        assert(g);
        assert(f->v || g->v);

        if (!f->v)
        {
            return g->v->x;
        }
        if (!g->v)
        {
            return f->v->x;
        }

        return ((var2lvl[f->v->x] <= var2lvl[g->v->x]) ? f->v->x : g->v->x);
    }

    auto foa(std::shared_ptr<node> v) -> std::shared_ptr<node>
    {
        assert(v);
        assert(v->x < var_count());

        if (vl[v->x].nt.load_factor() >= config::load_factor)
        {  // collision probability is too high => clean up nodes
            auto const old_lf = vl[v->x].nt.load_factor();
            gc();

            if (vl[v->x].nt.load_factor() > old_lf - config::dead_factor)
            {  // too few nodes were deleted => resize this UT
                vl[v->x].nt.reserve(2 * vl[v->x].nt.bucket_count());
            }
        }

        return *vl[v->x].nt.insert(std::move(v)).first;  // in the case of a collision, v is the list head
    }

    auto foa(std::shared_ptr<edge> e) -> std::shared_ptr<edge>
    {
        assert(e);

        float old_lf{};
        if (e->v)
        {
            assert(e->v->x < var_count());

            old_lf = vl[e->v->x].et.load_factor();
            if (old_lf >= config::load_factor)
            {
                gc();

                if (vl[e->v->x].et.load_factor() > old_lf - config::dead_factor)
                {
                    vl[e->v->x].et.reserve(2 * vl[e->v->x].et.bucket_count());
                }
            }

            return *vl[e->v->x].et.insert(std::move(e)).first;
        }

        old_lf = consts.load_factor();
        if (old_lf >= config::load_factor)
        {
            gc();

            if (consts.load_factor() > old_lf - config::dead_factor)
            {
                consts.reserve(2 * consts.bucket_count());
            }
        }

        return *consts.insert(std::move(e)).first;
    }

    // combines two DDs additively
    auto virtual add(std::shared_ptr<edge>, std::shared_ptr<edge>) -> std::shared_ptr<edge> = 0;

    // adjusts a pair weight according to DD type
    auto virtual apply(std::int32_t, std::shared_ptr<edge> const&) -> std::shared_ptr<edge> = 0;

    auto virtual complement(std::shared_ptr<edge> const&) -> std::shared_ptr<edge> = 0;  // computes NOT

    // connects two conjuncts logically (AND)
    auto virtual conj(std::shared_ptr<edge>, std::shared_ptr<edge>) -> std::shared_ptr<edge> = 0;

    // connects two disjuncts logically (OR)
    auto virtual disj(std::shared_ptr<edge> const&, std::shared_ptr<edge> const&) -> std::shared_ptr<edge> = 0;

    // checks if a node is normalized
    [[nodiscard]] auto virtual is_normalized(std::shared_ptr<edge> const&, std::shared_ptr<edge> const&) const
        -> bool = 0;

    // creates/reuses a node and an incoming edge
    auto virtual make_branch(std::int32_t, std::shared_ptr<edge>, std::shared_ptr<edge>) -> std::shared_ptr<edge> = 0;

    // combines two DDs multiplicatively
    auto virtual mul(std::shared_ptr<edge>, std::shared_ptr<edge>) -> std::shared_ptr<edge> = 0;

    auto virtual neg(std::shared_ptr<edge> const&) -> std::shared_ptr<edge> = 0;  // negates a DD

    [[nodiscard]] auto virtual regw() const -> std::int32_t = 0;  // returns the standard weight of an edge

    std::unordered_map<entry, std::pair<std::weak_ptr<edge>, double>, hash_fn> ct;  // numerical results can be cached

    std::array<std::shared_ptr<edge>, 4> tmls;  // these constants are never cleared

    std::vector<std::int32_t> var2lvl;

    std::vector<std::shared_ptr<edge>> vars;  // DD variables are never cleared

  private:
    auto exchange(std::int32_t const lvl)
    {
        assert(lvl < var_count());
        assert(var_count() > 1);

        if (lvl == var_count() - 1)
        {
            return;
        }

        auto const x = lvl2var[lvl];
        auto const y = lvl2var[lvl + 1];

        gc();  // ensure that dead nodes are not swapped

        auto swap_is_needed = [y](auto const& hi, auto const& lo) {
            assert(hi);
            assert(lo);

            if (!hi->v && !lo->v)
            {
                return false;
            }
            if (!hi->v || !lo->v)
            {
                if (hi->v)
                {
                    return (hi->v->x == y);
                }
                return (lo->v->x == y);
            }

            return (hi->v->x == y || lo->v->x == y);
        };

        for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();)
        {
            if (swap_is_needed((*it)->hi, (*it)->lo))
            {
                auto v = *it;
                it = vl[x].nt.erase(it);

                auto hi = make_branch(x, cof(v->hi, y, true), cof(v->lo, y, true));
                auto lo = make_branch(x, cof(v->hi, y, false), cof(v->lo, y, false));
                if (is_normalized(hi, lo))
                {
                    hi = neg(hi);
                    lo = neg(lo);

                    v->m = true;
                }

                v->x = y;
                v->hi = hi;
                v->lo = lo;

                foa(v);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = vl[x].et.begin(); it != vl[x].et.end();)
        {  // swap edges pointing to swapped nodes
            if ((*it)->v->x == y)
            {
                foa(*it);
                it = vl[x].et.erase(it);
            }
            else
            {
                ++it;
            }
        }

        for (const auto& e : vl[y].et)
        {  // adjust edges pointing to marked nodes
            if (e->v->m)
            {
                e->w = -e->w;
            }
        }

        gc();  // clean up possible dead nodes

        unmark();

        std::swap(lvl2var[lvl], lvl2var[lvl + 1]);
        std::swap(var2lvl[x], var2lvl[y]);
    }

    [[nodiscard]] auto longest_path_rec(std::shared_ptr<edge> const& f) const noexcept -> std::int32_t
    {
        assert(f);

        if (!f->v)
        {
            return 0;
        }

        auto const d_hi = longest_path_rec(f->v->hi);
        auto const d_lo = longest_path_rec(f->v->lo);

        return (((d_hi > d_lo) ? d_hi : d_lo) + 1);
    }

    auto node_count_rec(std::shared_ptr<edge>& f) noexcept -> std::int32_t
    {
        assert(f);

        if (!f->v || f->v->m)
        {
            return 0;
        }

        f->v->m = true;

        return (node_count_rec(f->v->hi) + node_count_rec(f->v->lo) + 1);
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

    auto sift_down(std::int32_t lvl, std::int32_t& lvl_min, std::int32_t& nc_min) -> std::int32_t
    {
        assert(lvl < var_count());
        assert(lvl_min < var_count());
        assert(nc_min > 0);

        auto nc_start = 0;
        auto nc_end = 0;

        while (lvl != var_count() - 1 &&
               config::growth_factor * static_cast<float>(nc_start) >= static_cast<float>(nc_end))
        {
            nc_start = node_count();
            exchange(lvl++);
            nc_end = node_count();

            if (nc_end < nc_min)
            {
                lvl_min = lvl;
                nc_min = nc_end;
            }
        }

        return lvl;
    }

    auto sift_up(std::int32_t lvl, std::int32_t& lvl_min, std::int32_t& nc_min) -> std::int32_t
    {
        assert(lvl < var_count());
        assert(lvl_min < var_count());
        assert(nc_min > 0);

        auto nc_start = 0;
        auto nc_end = 0;

        while (--lvl != -1 && config::growth_factor * static_cast<float>(nc_start) >= static_cast<float>(nc_end))
        {
            nc_start = node_count();
            exchange(lvl);
            nc_end = node_count();

            if (nc_end < nc_min)
            {
                lvl_min = lvl;
                nc_min = nc_end;
            }
        }

        return ++lvl;
    }

    auto to_dot(std::shared_ptr<edge>& f, std::ostream& s) const -> void
    {
        assert(f);

        if (!f->v || f->v->m)
        {
            return;
        }

        f->v->m = true;

        auto dump = [&](std::shared_ptr<edge> const& child, std::string_view style) {
            assert(child);
            assert(!style.empty());

            if (child->v)
            {
                s << 'v' << child->v << " [shape=circle,label=\"" << vl[child->v->x].l << "\"];\n";
                s << "{ rank=same; x" << child->v->x << "; v" << child->v << "; }\n";
            }

            s << 'v' << f->v << " -> v" << child->v << " [dir=none,style=" << style;
            if (child->w == 0)
            {
                s << "];\n";
            }
            else
            {
                s << ",label=\"" << child->w << "\"];\n";
            }
        };
        dump(f->v->hi, "solid");
        dump(f->v->lo, "dashed");

        to_dot(f->v->hi, s);
        to_dot(f->v->lo, s);
    }

    auto unmark() -> void  // all nodes
    {
        parallel_for(0, var_count(), [this](auto const i) {
            //std::cout << std::this_thread::get_id() << std::endl;
            for (auto& v : vl[i].nt)
            {
                v->m = false;
            }
        });
    }

    std::unordered_set<std::shared_ptr<edge>, hash, comp> consts;

    std::vector<std::int32_t> lvl2var;

    std::vector<variable> vl;
};

}  // namespace freddy::detail
