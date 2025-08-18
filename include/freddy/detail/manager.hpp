#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "common.hpp"               // parallel_for
#include "edge.hpp"                 // edge
#include "freddy/config.hpp"        // config::ct_size
#include "freddy/expansion.hpp"     // to_string
#include "freddy/op/compose.hpp"    // op::compose
#include "freddy/op/has_const.hpp"  // op::has_const
#include "freddy/op/restr.hpp"      // op::restr
#include "node.hpp"                 // node
#include "operation.hpp"            // operation
#include "variable.hpp"             // variable

#include <boost/unordered/unordered_flat_set.hpp>  // boost::unordered::erase_if

#include <algorithm>    // std::ranges::max_element
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::ceil
#include <concepts>     // std::same_as
#include <cstddef>      // std::size_t
#include <cstdint>      // std::uint_fast32_t
#include <format>       // std::format
#include <limits>       // std::numeric_limits
#include <memory>       // std::unique_ptr
#include <numeric>      // std::accumulate
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <type_traits>  // std::is_nothrow_destructible_v
#include <utility>      // std::move
#include <vector>       // std::vector
#ifdef __APPLE__
#include <mach/mach.h>   // host_statistics64
#include <sys/sysctl.h>  // CTL_HW
#elifdef __linux__
#include <fstream>  // std::ifstream
#elifdef _WIN32
#define NOMINMAX
// clang-format off
#include <windows.h>
// clang-format on
#include <psapi.h>  // PERFORMANCE_INFORMATION
#endif

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename E, typename V>  // edge weight, node value
class manager
{
  public:  // methods that do not need to be wrapped
    friend auto operator<< <>(std::ostream&, manager<E, V> const&) -> std::ostream&;

    virtual ~manager() noexcept(std::is_nothrow_destructible_v<E> && std::is_nothrow_destructible_v<V>) = default;

    [[nodiscard]] auto var_count() const noexcept
    {
        //assert(vl.size() <= std::numeric_limits<var_index>::max() + static_cast<std::size_t>(1));

        return vl.size();
    }

    [[nodiscard]] auto const_count() const noexcept
    {
        return nc.size();
    }

    [[nodiscard]] auto node_count() const noexcept
    {
        return std::accumulate(vl.begin(), vl.end(), 0uz,
                               [](std::size_t const sum, auto const& var) { return sum + var.nt.size(); }) +
               nc.size();
    }

    [[nodiscard]] auto edge_count() const noexcept
    {
        return std::accumulate(vl.begin(), vl.end(), 0uz,
                               [](std::size_t const sum, auto const& var) { return sum + var.et.size(); }) +
               ec.size();
    }

    auto swap(std::size_t const lvl_x, std::size_t const lvl_y)
    {
        assert(lvl_x < var_count());
        assert(lvl_y < var_count());

        auto const lvl_x2 = static_cast<var_index>(lvl_x);
        auto const lvl_y2 = static_cast<var_index>(lvl_y);

        sift(lvl_x2, lvl_y2);
        sift(lvl_x2 < lvl_y2 ? lvl_y2 - 1 : lvl_y2 + 1, lvl_x2);
    }

    auto reorder()
    {
        auto ncnt_min = node_count();
        for (auto i = decltype(var_count()){0}; i < var_count(); ++i)
        {
            auto lvl_min = var2lvl[i];
            auto lvl_stop = sift_down(var2lvl[i], lvl_min, ncnt_min);
            lvl_stop = sift_up(lvl_stop, lvl_min, ncnt_min);
            sift(lvl_stop, lvl_min);
        }
    }

    auto gc() noexcept  // performance of many EDA tasks depends on garbage collection
    {
        gc(0, static_cast<var_index>(var_count()));
    }

  protected:  // generally intended for overriding and wrapping methods
    using edge_ptr = std::shared_ptr<edge<E, V>>;

    using node_ptr = std::shared_ptr<node<E, V>>;

    static auto constexpr UT_SIZE = 1679;  // default minimum capacity of a UT (per DD level)

    static auto constexpr CT_SIZE = 215039;  // default minimum capacity of the computed table (operation cache)

    // contradiction, tautology
    explicit manager(std::array<edge_ptr, 2> tmls, std::size_t const ut_size = UT_SIZE,
                     std::size_t const ct_size = CT_SIZE) :
            ut_size{ut_size}
    {
        static auto constexpr vl_size = 10;  // minimum capacity of the variable list and dependencies

        consts.reserve(tmls.size());
        ct.reserve(ct_size);
        ec.reserve(ut_size);
        lvl2var.reserve(vl_size);
        nc.reserve(ut_size);
        var2lvl.reserve(vl_size);
        vars.reserve(vl_size);
        vl.reserve(vl_size);

        for (auto& tml : tmls)
        {
            assert(tml);

            nc.insert(tml->v);
            consts.push_back(*ec.insert(std::move(tml)).first);
        }
    }

    manager(manager const&) = default;

    manager(manager&&) noexcept = default;

    auto operator=(manager const&) -> manager& = default;

    auto operator=(manager&&) noexcept -> manager& = default;

    auto make_var(expansion const t, std::string_view l)
    {
        assert(consts.size() >= 2);

        var2lvl.push_back(static_cast<var_index>(var_count()));
        lvl2var.push_back(static_cast<var_index>(var_count()));
        vl.emplace_back(t, l.empty() ? 'x' + std::to_string(var_count()) : l);
        vars.push_back(uedge(regw(), unode(static_cast<var_index>(var_count()) - 1, consts[1], consts[0])));

        return vars[var_count() - 1];
    }

    [[nodiscard]] auto get_var(var_index const i) const noexcept
    {
        assert(i < var_count());

        return vars[i];
    }

    auto make_const(E w, V c, bool const is_imp = false)
    {
        if (is_imp)
        {
            consts.push_back(uedge(std::move(w), unode(std::move(c))));
            return consts.back();
        }
        return uedge(std::move(w), unode(std::move(c)));
    }

    [[nodiscard]] auto get_const(std::size_t const i) const noexcept
    {
        assert(i < consts.size());

        return consts[i];
    }

    [[nodiscard]] auto lvl_ge(var_index const x, var_index const y) const noexcept
    {
        assert(x < var_count());
        assert(y < var_count());

        return var2lvl[x] >= var2lvl[y];
    }

    template <typename T>
    requires std::same_as<T, bool>
    auto fn(edge_ptr const& f, T const a)
    {
        assert(f);
        assert(!f->v->is_const());

        return a ? apply(f->w, f->v->inner.hi) : apply(f->w, f->v->inner.lo);
    }

    template <typename T, typename... Ts>
    requires std::same_as<T, bool>
    auto fn(edge_ptr const& f, T const a, Ts... args)
    {
        assert(f);
        assert(!f->v->is_const());

        return a ? fn(apply(f->w, f->v->inner.hi), args...) : fn(apply(f->w, f->v->inner.lo), args...);
    }

    [[nodiscard]] auto node_count(std::vector<edge_ptr> const& fs) const
    {
        boost::unordered_flat_set<node_ptr, hash, comp> marks;
        marks.reserve(ut_size);

        for (auto const& f : fs)
        {  // (shared) number of nodes
            node_count(f, marks);
        }

        return marks.size();  // including leaves
    }

    [[nodiscard]] auto path_count(edge_ptr const& f) const noexcept -> double  // as results can be very large
    {
        assert(f);

        return f->v->is_const() ? 1 : path_count(f->v->br().hi) + path_count(f->v->br().lo);  // DFS
    }

    [[nodiscard]] auto longest_path(std::vector<edge_ptr> const& fs) const
    {
        assert(!fs.empty());

        std::vector<std::uint_fast32_t> ds(fs.size());

        parallel_for(decltype(ds.size()){0}, ds.size(), [&ds, &fs, this](std::size_t const i) {
            // std::cout << std::this_thread::get_id() << std::endl;
            ds[i] = longest_path_rec(fs[i]);
        });

        return *std::ranges::max_element(ds) - 1;  // due to the root edge
    }

    [[nodiscard]] auto eval(edge_ptr const& f, std::vector<bool> const& as) const
    {
        assert(f);
        assert(std::cmp_equal(as.size(), var_count()));  // assignment follows the made variable ordering

        return agg(f->w, eval(f->v, as));
    }

    [[nodiscard]] auto has_const(edge_ptr const& f, V const& c)
    {
        assert(f);

        if (f->v->is_const())
        {
            return f->v->outer == c;
        }

        op::has_const op{f, c};
        if (auto const* const ent = cached(op))
        {
            return ent->result();
        }

        op.result() = has_const(f->v->inner.hi, c) || has_const(f->v->inner.lo, c);
        return cache(std::move(op))->result();
    }

    [[nodiscard]] auto is_essential(edge_ptr const& f, std::uint_fast32_t const x) const -> bool
    {
        assert(f);
        assert(x < var_count());

        return f->v->is_const() || var2lvl[f->v->inner.x] > var2lvl[x]
                   ? false
                   : f->v->inner.x == x || is_essential(f->v->inner.hi, x) || is_essential(f->v->inner.lo, x);
    }

    auto compose(edge_ptr const& f, std::uint_fast32_t const x, edge_ptr const& g)
    {
        assert(f);
        assert(x < var_count());
        assert(g);

        if (f->v->is_const() || !is_essential(f, x))
        {
            return f;
        }

        op::compose op{f, x, g};
        if (auto const* const ent = cached(op))
        {
            return ent->result();
        }

        edge_ptr hi;
        edge_ptr lo;
        if (f->v->inner.x == x)
        {
            hi = mul(f->v->inner.hi, g);

            switch (vl[x].t)
            {
                case expansion::PD: lo = f->v->inner.lo; break;
                case expansion::S:
                {
                    lo = mul(f->v->inner.lo, complement(g));
                    break;
                }
                default: assert(false); std::unreachable();
            }
        }
        else
        {
            hi = mul(vars[f->v->inner.x], compose(f->v->inner.hi, x, g));

            switch (vl[f->v->inner.x].t)
            {
                case expansion::PD: lo = compose(f->v->inner.lo, x, g); break;
                case expansion::S:
                {
                    lo = mul(complement(vars[f->v->inner.x]), compose(f->v->inner.lo, x, g));
                    break;
                }
                default: assert(false); std::unreachable();
            }
        }

        op.result() = apply(f->w, plus(hi, lo));
        return cache(std::move(op))->result();
    }

    auto cof(edge_ptr const& f, std::uint_fast32_t const x, bool const a)
    {
        assert(f);
        assert(x < var_count());
        assert(!consts.empty());

        if (f->v->is_const() || f->v->inner.x != x)
        {
            if (vl[x].t == expansion::PD && a)  // dependent on two subtrees: f ^ f = 0
            {
                return consts[0];
            }
            return f;
        }
        return a ? apply(f->w, f->v->inner.hi) : apply(f->w, f->v->inner.lo);
    }

    auto restr(edge_ptr const& f, std::uint_fast32_t const x, bool const a)
    {
        assert(f);
        assert(x < var_count());

        if (!is_essential(f, x))
        {
            return f;
        }
        if (f->v->inner.x == x)
        {
            return cof(f, x, a);
        }

        op::restr op{f, x, a};
        if (auto const* const ent = cached(op))
        {
            return ent->result();
        }

        op.result() = apply(f->w, make_branch(f->v->inner.x, restr(f->v->inner.hi, x, a), restr(f->v->inner.lo, x, a)));
        return cache(std::move(op))->result();
    }

    auto exist(edge_ptr const& f, std::uint_fast32_t const x)
    {
        assert(f);
        assert(x < var_count());

        return disj(restr(f, x, true), restr(f, x, false));
    }

    auto forall(edge_ptr const& f, std::uint_fast32_t const x)
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
            return g->v->inner.x;
        }
        if (g->v->is_const())
        {
            return f->v->inner.x;
        }
        return var2lvl[f->v->inner.x] <= var2lvl[g->v->inner.x] ? f->v->inner.x : g->v->inner.x;
    }

    auto unode(std::uint_fast32_t const x, edge_ptr hi, edge_ptr lo)  // unique node
    {
        return foa(node<E, V>{x, std::move(hi), std::move(lo)}, vl[x].nt, var2lvl[x]);
    }

    auto unode(V c)
    {
        return foa(node<E, V>{std::move(c)}, nc, static_cast<var_index>(var_count()));
    }

    auto uedge(E w, node_ptr v)
    {
        assert(v);

        if (v->is_const())
        {
            return foa(edge<E, V>{std::move(w), std::move(v)}, ec, static_cast<var_index>(var_count()));
        }
        // v is labeled by variable
        auto const x = v->inner.x;
        return foa(edge<E, V>{std::move(w), std::move(v)}, vl[x].et, var2lvl[x]);
    }

    template <typename T>
    auto cache(T op)
    {
        return static_cast<T const*>((*ct.insert(std::make_unique<T>(std::move(op))).first).get());
    }

    template <typename T>
    [[nodiscard]] auto cached(T const& op) const noexcept
    {
        auto const cr = ct.find(&op);
        return cr == ct.end() ? nullptr : static_cast<T const*>((*cr).get());
    }

    auto gc(var_index const start_lvl, var_index const end_lvl) noexcept
    {
        assert(start_lvl <= var_count());  // constants can also be taken into account
        assert(end_lvl <= var_count());
        assert(start_lvl <= end_lvl);

        ct.clear();  // to avoid invalid results

        auto cleanup = [](auto& ut) {
            boost::unordered::erase_if(ut, [](auto const& item) {  // using an anti-drift mechanism
                return item.use_count() == 1;  // only the UT references this edge/node => edge/node is dead
            });
        };

        for (auto lvl = start_lvl; lvl <= end_lvl && lvl < var_count(); ++lvl)  // variables
        {  // an increased chance of deleting edges/nodes
            cleanup(vl[lvl2var[lvl]].et);
            cleanup(vl[lvl2var[lvl]].nt);
        }
        if (end_lvl == var_count())  // constants
        {
            cleanup(ec);
            cleanup(nc);
        }
    }

    auto to_dot(std::vector<edge_ptr> const& fs, std::vector<std::string> const& outputs, std::ostream& s) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        s << "digraph DD {\n";
        s << 'f' << " [style=invis];\n";
        s << 'c' << " [style=invis];\n";

        if (var_count() == 0)
        {
            s << "f -> c [style=invis];\n";
        }
        for (auto i = decltype(var_count()){0}; i < var_count(); ++i)  // arrange nodes per level
        {
            if (i == 0)
            {
                s << "f -> x" << lvl2var[i] << " [style=invis];\n";
            }

            s << 'x' << lvl2var[i] << R"( [shape=plaintext,fontname="times italic",label=")" << lvl2var[i] << " ["
              << to_string(vl[lvl2var[i]].t) << "]\"];\n";

            if (i + 1 < var_count())
            {
                s << 'x' << lvl2var[i] << " -> x" << lvl2var[i + 1] << " [style=invis];\n";
            }
            else
            {
                s << 'x' << lvl2var[i] << " -> c [style=invis];\n";
            }
        }

        boost::unordered_flat_set<node_ptr, hash, comp> marks;
        marks.reserve(ut_size);

        for (decltype(fs.size()) i = 0; i < fs.size(); ++i)
        {
            assert(fs[i]);

            auto const func = 'f' + std::to_string(i);  // to prevent overriding of identical functions

            s << func << R"( [shape=plaintext,fontname="times bold",label=")" << (outputs.empty() ? func : outputs[i])
              << "\"]\n";
            s << "{ rank=same; f; " << func << "; }\n";
            s << func << " -> v" << fs[i]->v << " [label=\" " << fs[i]->w << " \"];\n";

            to_dot(fs[i], marks, s);
        }

        s << "}\n";
    }

    // many optimizations are possible depending on the DD type
    virtual auto apply(E const& w, edge_ptr const& f) -> edge_ptr
    {
        assert(f);

        return uedge(comb(w, f->w), f->v);
    }

    virtual auto ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr  // NOLINT(performance-unnecessary-value-param)
    {
        assert(f);
        assert(g);
        assert(h);

        return disj(conj(f, g), conj(complement(f), h));  // as there are different versions
    }

    virtual auto plus(edge_ptr, edge_ptr) -> edge_ptr = 0;  // combines DDs additively

    [[nodiscard]] virtual auto agg(E const&, V const&) const -> V = 0;  // aggregates an edge weight and a node value

    // adjusts a pair weight according to the DD type
    [[nodiscard]] virtual auto comb(E const&, E const&) const -> E = 0;

    virtual auto complement(edge_ptr const&) -> edge_ptr = 0;  // computes NOT

    virtual auto conj(edge_ptr const&, edge_ptr const&) -> edge_ptr = 0;  // connects conjuncts logically (AND)

    virtual auto disj(edge_ptr const&, edge_ptr const&) -> edge_ptr = 0;  // connects disjuncts logically (OR)

    // creates/reuses a node and an incoming edge
    virtual auto make_branch(std::uint_fast32_t, edge_ptr, edge_ptr) -> edge_ptr = 0;

    virtual auto merge(V const&, V const&) const -> V = 0;  // evaluates aggregates (subtrees)

    virtual auto mul(edge_ptr, edge_ptr) -> edge_ptr = 0;  // combines DDs multiplicatively

    [[nodiscard]] virtual auto regw() const -> E = 0;  // returns the regular weight of an edge

  private:
    using computed_table = boost::unordered_flat_set<std::unique_ptr<operation>, hash, comp>;

    [[nodiscard]] auto eval(node_ptr const& v, std::vector<bool> const& as) const -> V
    {
        assert(v);
        assert(std::cmp_equal(as.size(), var_count()));

        if (v->is_const())
        {
            return v->outer;
        }

        V r{};
        auto const br = v->inner;
        switch (vl[br.x].t)
        {
            case expansion::PD: r = as[br.x] ? merge(eval(br.hi, as), eval(br.lo, as)) : eval(br.lo, as); break;
            case expansion::S:
            {
                r = as[br.x] ? eval(br.hi, as) : eval(br.lo, as);
                break;
            }
            default: assert(false); std::unreachable();
        }
        return r;
    }

    auto exchange(std::uint_fast32_t const lvl)  // NOLINT(readability-function-cognitive-complexity)
    {                                            // with the level below
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
                return hi->v->is_const() ? lo->v->inner.x == y : hi->v->inner.x == y;
            }
            return hi->v->inner.x == y || lo->v->inner.x == y;
        };

        // look ahead to determine how many swaps need to be performed
        auto const max_swaps_needed =
            std::accumulate(vl[x].nt.begin(), vl[x].nt.end(), 0, [swap_is_needed](auto const sum, auto const& v) {
                return swap_is_needed(v->inner.hi, v->inner.lo) ? sum + 2 : sum;  // two branches must be made per swap
            });
        if (vl[x].nt.size() + max_swaps_needed > vl[x].nt.max_load())
        {  // prevent rehashing during swapping, as it invalidates iterators
            vl[x].nt.rehash(std::max(2 * vl[x].nt.bucket_count(),
                                     static_cast<std::size_t>(std::ceil((vl[x].nt.size() + max_swaps_needed) /
                                                                        vl[x].nt.max_load_factor()))));

            assert(vl[x].nt.size() + max_swaps_needed <= vl[x].nt.max_load());
        }

        for (auto it = vl[x].nt.begin(); it != vl[x].nt.end();)
        {
            if (swap_is_needed((*it)->inner.hi, (*it)->inner.lo))  // swapping levels is a local operation
            {
                auto v = *it;  // to ensure existing references are not lost
                it = vl[x].nt.erase(it);

                assert(!v->is_const());

                auto& br = v->inner;
                auto const hi = make_branch(x, cof(br.hi, y, true), cof(br.lo, y, true));
                br.lo = make_branch(x, cof(br.hi, y, false), cof(br.lo, y, false));
                br.hi = hi;
                br.x = y;

                if (!vl[y].nt.insert(v).second)
                {  // insertion failed (edge case) => same node already exists
                    auto const& orig_v = *vl[y].nt.find(v);
                    boost::unordered_flat_set<edge_ptr, hash, comp> tmp;
                    for (auto it2 = vl[x].et.begin(); it2 != vl[x].et.end();)
                    {
                        if ((*it2)->v == v)
                        {  // redirect incoming edge of duplicated node to original node
                            auto e = *it2;
                            it2 = vl[x].et.erase(it2);

                            e->v = orig_v;

                            tmp.insert(e);
                        }
                        else
                        {
                            ++it2;
                        }
                    }
                    vl[x].et.merge(tmp);
                }
            }
            else
            {
                ++it;
            }
        }

        for (auto it = vl[x].et.begin(); it != vl[x].et.end();)
        {  // swap edges pointing to swapped nodes
            if ((*it)->v->inner.x == y)
            {
                vl[y].et.insert(*it);

                it = vl[x].et.erase(it);
            }
            else
            {
                ++it;
            }
        }

        std::swap(lvl2var[lvl], lvl2var[lvl + 1]);
        std::swap(var2lvl[x], var2lvl[y]);

        gc();  // clean up possible dead nodes
    }

    template <typename T>
    auto foa(T&& obj, utable<T>& ut, std::uint_fast32_t const lvl)
    {  // find or add node/edge
        assert(lvl <= var_count());

        auto const search = ut.find(&obj);
        if (search == ut.end())
        {
            if (ut.size() == ut.max_load())
            {  // make sure that at least one more node/edge can be inserted
                gc(0, lvl);
                if (ut.load_factor() <= ut.max_load_factor() * (1.0f - 0.75f))
                {
                    ut.rehash(static_cast<std::size_t>(std::ceil(0.5f * ut.bucket_count())));
                }
                else
                {  // too few edges/nodes of this UT were deleted
                    ut.rehash(2 * ut.bucket_count());
                }
            }
            return *ut.insert(std::make_shared<T>(std::forward<T>(obj))).first;
        }
        return *search;
    }

    [[nodiscard]] auto longest_path_rec(edge_ptr const& f) const noexcept -> std::uint_fast32_t
    {
        assert(f);

        return f->v->is_const() ? 1 : std::max(longest_path_rec(f->v->inner.hi), longest_path_rec(f->v->inner.lo)) + 1;
    }

    auto node_count(edge_ptr const& f, boost::unordered_flat_set<node_ptr, hash, comp>& marks) const
    {
        assert(f);

        if (marks.find(f->v) != marks.end())
        {  // node has already been visited
            return;
        }

        marks.insert(f->v);

        if (!f->v->is_const())
        {  // DD traversal
            node_count(f->v->inner.hi, marks);
            node_count(f->v->inner.lo, marks);
        }
    }

    auto sift(var_index const lvl_x, var_index const lvl_y)
    {
        assert(lvl_x < var_count());
        assert(lvl_y < var_count());

        if (lvl_x == lvl_y)
        {
            return;
        }

        auto const start = lvl_x < lvl_y ? lvl_x : lvl_x - 1;
        auto const stop = lvl_x < lvl_y ? lvl_y : lvl_y - 1;
        auto const step = lvl_x < lvl_y ? 1 : -1;

        for (auto lvl = start; lvl != stop; lvl += step)
        {
            exchange(lvl);
        }
    }

    auto sift_down(var_index lvl, var_index& lvl_min, std::size_t& ncnt_min)
    {
        assert(lvl < var_count());
        assert(lvl_min < var_count());

        auto ncnt_start = decltype(node_count()){0};
        auto ncnt_end = decltype(node_count()){0};
        while (lvl != var_count() - 1 && 1.2f * static_cast<float>(ncnt_start) >= static_cast<float>(ncnt_end))
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

    auto sift_up(var_index lvl, var_index& lvl_min, std::size_t& ncnt_min)
    {
        assert(lvl < var_count());
        assert(lvl_min < var_count());

        auto ncnt_start = decltype(node_count()){0};
        auto ncnt_end = decltype(node_count()){0};
        while (lvl-- > 0 && 1.2f * static_cast<float>(ncnt_start) >= static_cast<float>(ncnt_end))
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

    auto to_dot(edge_ptr const& f, boost::unordered_flat_set<node_ptr, hash, comp>& marks, std::ostream& s) const
    {
        assert(f);

        if (marks.find(f->v) != marks.end())
        {
            return;
        }
        marks.insert(f->v);

        if (f->v->is_const())
        {
            s << 'v' << f->v << " [shape=box,style=filled,color=chocolate,fontcolor=white,label=\"" << f->v->outer
              << "\"];\n";
            s << "{ rank=same; c; v" << f->v << "; }\n";

            return;
        }

        s << 'v' << f->v << " [shape=" << (vl[f->v->inner.x].t == expansion::S ? "circle" : "octagon,regular=true")
          << ",style=filled,color=black,fontcolor=white,label=\"" << vl[f->v->inner.x].l << "\"];\n";
        s << "{ rank=same; x" << f->v->inner.x << "; v" << f->v << "; }\n";
        s << 'v' << f->v << " -> v" << f->v->inner.hi->v << " [color=blue,dir=none,label=\" " << f->v->inner.hi->w
          << " \"];\n";
        s << 'v' << f->v << " -> v" << f->v->inner.lo->v << " [style=dashed,color=red,dir=none,label=\" "
          << f->v->inner.lo->w << " \"];\n";

        to_dot(f->v->inner.hi, marks, s);
        to_dot(f->v->inner.lo, marks, s);
    }

    std::vector<edge_ptr> consts;  // DD constants that are never cleared

    computed_table ct;  // to cache operations

    utable<edge<E, V>> ec;

    std::vector<std::uint_fast32_t> lvl2var;

    utable<node<E, V>> nc;  // constants

    std::size_t ut_size;

    std::vector<var_index> var2lvl;  // for reordering

    std::vector<edge_ptr> vars;  // DD variables that are never cleared

    std::vector<variable<E, V>> vl;
};

template <typename E, typename V>
auto operator<<(std::ostream& s, manager<E, V> const& mgr) -> std::ostream&
{
    auto print_thead = [&s](std::string_view title) {
        s << title << '\n';
        s << std::format("{:-<61}\n", '-');
    };

    auto print_tbody = [&s](auto const& ht, auto const& prefix) {
        s << std::format("{:2} {:36} | {:19}\n", prefix, "#Elements", ht.size());
        s << std::format("{:2} {:36} | {:19}\n", prefix, "Max. load", ht.max_load());
        s << std::format("{:2} {:36} | {:19}", prefix, "#Buckets", ht.bucket_count());  // How large do tables get?
#ifdef BOOST_UNORDERED_ENABLE_STATS  // output statistics in order to evaluate hash quality and impact
        auto const& stats = ht.get_stats();

        s << std::format("\n{:2} {:36} | {:19}\n", prefix, "Insertion count", stats.insertion.count);
        if (stats.insertion.count != 0)
        {  // operation was performed at least once
            // average number of bucket groups accessed during insertion (could indicate that tables are too large)
            s << std::format("{:2} {:36} | {:19.2f}\n", prefix, "Insertion probe length",
                             stats.insertion.probe_length.average);  // should be close to 1.0

            assert(stats.unsuccessful_lookup.count != 0);

            // average number of elements compared during lookup
            s << std::format("{:2} {:36} | {:19.2f}\n", prefix, "Unsuccessful lookup comparison count",
                             stats.unsuccessful_lookup.num_comparisons.average);  // should be close to 0.0

            s << std::format("{:2} {:36} | {:19.2f}\n", prefix, "Miss rate",
                             (static_cast<double>(stats.insertion.count) /
                              (stats.insertion.count + stats.successful_lookup.count)) *
                                 100);

            assert(stats.insertion.count >= ht.size());

            // indicates how well the GC works
            s << std::format("{:2} {:36} | {:19}\n", prefix, "Number of cleaned elements",
                             stats.insertion.count - ht.size());
            s << std::format("{:2} {:36} | {:19.2f}\n", prefix, "Read/Write",  // How often is an element reused?
                             static_cast<double>(stats.successful_lookup.count) / stats.insertion.count);
        }

        s << std::format("{:2} {:36} | {:19}", prefix, "Successful lookup count", stats.successful_lookup.count);
        if (stats.successful_lookup.count != 0)
        {
            s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Successful lookup probe length",
                             stats.successful_lookup.probe_length.average);
            s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Successful lookup comparison count",
                             stats.successful_lookup.num_comparisons.average);  // should be close to 1.0

            s << std::format("\n{:2} {:36} | {:19.2f}", prefix, "Hit rate",
                             (static_cast<double>(stats.successful_lookup.count) /
                              (stats.insertion.count + stats.successful_lookup.count)) *
                                 100);
        }
#endif
    };

    for (auto const x : mgr.lvl2var)  // variable with respect to the order
    {
        print_thead(std::string{"Variable \""} + mgr.vl[x].get_l().data() + "\" [" + to_string(mgr.vl[x].t) + ']');
        print_tbody(mgr.vl[x].get_et(), "ET");
        s << '\n';
        print_tbody(mgr.vl[x].get_nt(), "NT");
        s << "\n\n";
    }

    print_thead("Constants");
    print_tbody(mgr.ec, "EC");
    s << '\n';
    print_tbody(mgr.nc, "NC");
    s << "\n\n";

    print_thead("Cache");
    print_tbody(mgr.ct, "CT");

    return s;
}

}  // namespace freddy::detail
