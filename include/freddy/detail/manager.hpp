#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                      // config
#include "freddy/detail/common.hpp"               // parallel_for
#include "freddy/detail/edge.hpp"                 // detail::edge
#include "freddy/detail/node.hpp"                 // detail::node
#include "freddy/detail/operation.hpp"            // operation
#include "freddy/detail/operation/compose.hpp"    // detail::compose
#include "freddy/detail/operation/has_const.hpp"  // detail::has_const
#include "freddy/detail/operation/restr.hpp"      // detail::restr
#include "freddy/detail/variable.hpp"             // variable
#include "freddy/expansion.hpp"                   // to_string

#include <boost/smart_ptr/intrusive_ptr.hpp>       // boost::intrusive_ptr
#include <boost/unordered/unordered_flat_set.hpp>  // boost::unordered::erase_if

#include <algorithm>    // std::ranges::fold_left
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::ceil
#include <concepts>     // std::same_as
#include <cstddef>      // std::size_t
#include <format>       // std::format
#include <limits>       // std::numeric_limits
#include <memory>       // std::unique_ptr
#include <ostream>      // std::ostream
#include <ranges>       // std::views::iota
#include <stdexcept>    // std::overflow_error
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <type_traits>  // std::is_base_of_v
#include <utility>      // std::pair
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

using namespace std::literals;  // sv

// =====================================================================================================================
// Types
// =====================================================================================================================

template <hashable EWeight, hashable NValue>  // edge weight, node value
class manager
{
  public:
    manager(manager const&) = delete;  // because UTs, among others, constitute the manager

    auto operator=(manager const&) = delete;

    friend auto operator<<(std::ostream& os, manager const& mgr) -> std::ostream&
    {
        auto print_thead = [&os](std::string_view title) {
            os << title << '\n';
            os << std::format("{:-<60}\n", '-');
        };

        auto print_tbody = [&os](auto const& ht, std::string_view prefix) {
            static constexpr auto row_fmt = "{:2} {:35} | {:19}"sv;

            os << std::format(row_fmt, prefix, "#Elements", ht.size()) << '\n';
            os << std::format(row_fmt, prefix, "Max load", ht.max_load()) << '\n';
            os << std::format(row_fmt, prefix, "#Buckets", ht.bucket_count());  // How large do hash tables get?
#ifdef BOOST_UNORDERED_ENABLE_STATS  // output statistics in order to evaluate hash quality and impact
            static constexpr auto row_fmt_f = "{:2} {:35} | {:19.2f}"sv;

            auto const& stats = ht.get_stats();

            os << '\n' << std::format(row_fmt, prefix, "Insertion count", stats.insertion.count) << '\n';
            if (stats.insertion.count != 0)
            {  // operation was performed at least once
                // average number of bucket groups accessed during insertion
                os << std::format(row_fmt_f, prefix, "Avg insertion probe length",
                                  stats.insertion.probe_length.average)  // should be close to 1.0
                   << '\n';

                assert(stats.unsuccessful_lookup.count != 0);

                // average number of elements compared during lookup
                os << std::format(row_fmt_f, prefix, "Avg unsuccessful lookup comparisons",
                                  stats.unsuccessful_lookup.num_comparisons.average)  // should be close to 0.0
                   << '\n';

                os << std::format(row_fmt_f, prefix, "Miss rate",
                                  (static_cast<double>(stats.insertion.count) /
                                   (stats.insertion.count + stats.successful_lookup.count)) *
                                      100)
                   << '\n';

                assert(stats.insertion.count >= ht.size());

                // indicates how well the GC works
                os << std::format(row_fmt, prefix, "Cleaned elements", stats.insertion.count - ht.size()) << '\n';
                os << std::format(row_fmt_f, prefix, "Read/Write",  // How often is an element reused?
                                  static_cast<double>(stats.successful_lookup.count) / stats.insertion.count)
                   << '\n';
            }

            os << std::format(row_fmt, prefix, "Successful lookup count", stats.successful_lookup.count);
            if (stats.successful_lookup.count != 0)
            {
                os << '\n'
                   << std::format(row_fmt_f, prefix, "Avg successful lookup probe length",
                                  stats.successful_lookup.probe_length.average);
                os << '\n'
                   << std::format(row_fmt_f, prefix, "Avg successful lookup comparisons",
                                  stats.successful_lookup.num_comparisons.average);  // should be close to 1.0

                os << '\n'
                   << std::format(row_fmt_f, prefix, "Hit rate",
                                  (static_cast<double>(stats.successful_lookup.count) /
                                   (stats.insertion.count + stats.successful_lookup.count)) *
                                      100);
            }
#endif
        };

        for (auto const x : mgr.lvl2var)  // variable with respect to the current order
        {
            print_thead("Variable \""s + std::string{mgr.vlist[x].label()} + "\" [" +
                        to_string(mgr.vlist[x].decomposition()) + ']');
            print_tbody(mgr.vlist[x].edge_table(), "ET");  // edge table
            os << '\n';
            print_tbody(mgr.vlist[x].node_table(), "NT");  // node table
            os << "\n\n";
        }

        print_thead("Constants");
        print_tbody(mgr.etable, "ET");
        os << '\n';
        print_tbody(mgr.ntable, "NT");
        os << "\n\n";

        print_thead("Operation Cache");
        print_tbody(mgr.ct, "CT");

        return os;
    }

    virtual ~manager() noexcept(std::is_nothrow_destructible_v<edge> && std::is_nothrow_destructible_v<node>) = default;

    [[nodiscard]] auto var_count() const noexcept
    {
        return vlist.size();
    }

    [[nodiscard]] auto const_count() const noexcept
    {
        return ntable.size();
    }

    [[nodiscard]] auto node_count() const noexcept
    {
        return std::ranges::fold_left(vlist, 0uz,
                                      [](std::size_t const sum, auto const& var) { return sum + var.ntable.size(); }) +
               const_count();
    }

    [[nodiscard]] auto edge_count() const noexcept
    {
        return std::ranges::fold_left(vlist, 0uz,
                                      [](std::size_t const sum, auto const& var) { return sum + var.etable.size(); }) +
               etable.size();
    }

    auto swap(var_index const lvl_x, var_index const lvl_y)
    {
        assert(lvl_x < var_count());
        assert(lvl_y < var_count());

        sift(lvl_x, lvl_y);  // from x to y
        sift(lvl_x < lvl_y ? lvl_y - 1 : lvl_y + 1, lvl_x);
    }

    auto reorder()
    {
        std::pair<var_index, decltype(node_count())> min{{}, node_count()};  // (level, nodes)
        for (auto const lvl : var2lvl)
        {
            min.first = lvl;                      // at which the minimum number of nodes was recorded
            auto stop_lvl = sift_down(lvl, min);  // taking into account max_node_growth
            stop_lvl = sift_up(stop_lvl, min);
            sift(stop_lvl, min.first);  // move the variable to the position of the local minimum
        }
    }

    auto gc() noexcept(std::is_nothrow_destructible_v<edge> &&
                       std::is_nothrow_destructible_v<node>)  // garbage collection
    {
        ct.clear();  // to avoid invalid results

        auto cleanup = [](auto& ut) {
            boost::unordered::erase_if(ut, [](auto const& item) {  // using an anti-drift mechanism
                return item->is_dead();
            });
        };

        for (auto const x : lvl2var)
        {  // an increased chance of deleting edges/nodes
            cleanup(vlist[x].etable);
            cleanup(vlist[x].ntable);
        }

        // constants
        cleanup(etable);
        cleanup(ntable);
    }

    // identity-oriented
    [[nodiscard]] auto config() const noexcept -> config const&
    {
        return cfg;
    }

    auto config() noexcept -> struct config&
    {
        return cfg;
    }

  protected:
    using edge = detail::edge<EWeight, NValue>;

    using edge_ptr = detail::edge_ptr<EWeight, NValue>;

    using node = detail::node<EWeight, NValue>;

    using node_ptr = detail::node_ptr<EWeight, NValue>;

    // terminals: [contradiction, tautology]
    manager(std::array<edge_ptr, 2> tmls, struct config const cfg) :
            cfg{cfg}
    {
        if (!this->cfg.heap_mem_limit)
        {
            this->cfg.heap_mem_limit = heap_mem_limit() * 9 / 10;  // 90%
        }

        consts.reserve(this->cfg.utable_size_hint);
        ct.reserve(this->cfg.cache_size_hint);
        etable.reserve(this->cfg.utable_size_hint);
        lvl2var.reserve(this->cfg.init_var_cap);
        ntable.reserve(this->cfg.utable_size_hint);
        var2lvl.reserve(this->cfg.init_var_cap);
        vars.reserve(this->cfg.init_var_cap);
        vlist.reserve(this->cfg.init_var_cap);

        for (auto& tml : tmls)
        {
            assert(tml);

            ntable.insert(tml->v);
            consts.push_back(*etable.insert(std::move(tml)).first);
        }
    }

    manager(manager&&) noexcept(std::is_nothrow_move_constructible_v<edge> &&
                                std::is_nothrow_move_constructible_v<node>) = default;

    auto operator=(manager&&) noexcept(std::is_nothrow_move_constructible_v<edge> &&
                                       std::is_nothrow_move_constructible_v<node>) -> manager& = default;

    auto unode(var_index const x, edge_ptr hi, edge_ptr lo)  // unique node
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        return foa(node{x, std::move(hi), std::move(lo)}, vlist[x].ntable);
    }

    auto unode(NValue c)
    {
        return foa(node{std::move(c)}, ntable);
    }

    auto uedge(EWeight w, node_ptr v)
    {
        assert(v);

        if (v->is_const())
        {
            return foa(edge{std::move(w), std::move(v)}, etable);
        }
        // v is labeled with a variable.
        auto const x = v->inner.x;
        return foa(edge{std::move(w), std::move(v)}, vlist[x].etable);
    }

    template <class Operation>
        requires std::is_base_of_v<operation, std::decay_t<Operation>>
    [[nodiscard]] auto cached(Operation const& op) const noexcept
    {
        auto const entry = ct.find(&op);
        return entry == ct.end() ? nullptr : static_cast<Operation const*>((*entry).get());
    }

    template <class Operation>
        requires std::is_base_of_v<operation, std::decay_t<Operation>>
    auto cache(Operation&& op)
    {
        return static_cast<Operation const*>(
            (*ct.insert(std::make_unique<Operation>(std::forward<Operation>(op))).first).get());
    }

    virtual auto cof(edge_ptr const& f, var_index const x, bool const a) -> edge_ptr
    {
        assert(f);
        assert(x < var_count());

        if (f->is_const() || f->v->inner.x != x)
        {
            if ((vlist[x].t == expansion::pD || vlist[x].t == expansion::nD) &&
                a)  // dependent on two subtrees: f ^ f = 0
            {
                return consts[0];
            }
            return f;
        }
        switch (vlist[x].t)
        {
            case expansion::S: return a ? apply(f->w, f->v->inner.hi) : apply(f->w, f->v->inner.lo);
            case expansion::pD:
            case expansion::nD: return a ? f->v->inner.hi : apply(f->w, f->v->inner.lo);
            default: assert(false); std::unreachable();
        }
    }

    [[nodiscard]] auto top_var(edge_ptr const& f, edge_ptr const& g) const noexcept
    {
        assert(f);
        assert(g);
        assert(!f->is_const() || !g->is_const());

        if (f->is_const())
        {
            return g->v->inner.x;
        }
        if (g->is_const())
        {
            return f->v->inner.x;
        }
        return var2lvl[f->v->inner.x] <= var2lvl[g->v->inner.x] ? f->v->inner.x : g->v->inner.x;
    }

    [[nodiscard]] auto lvl_ge(var_index const x, var_index const y) const noexcept
    {
        assert(x < var2lvl.size());
        assert(y < var2lvl.size());

        return var2lvl[x] >= var2lvl[y];
    }

    // ---- Methods for Overriding and/or Wrapping by DD Types ---------------------------------------------------------

    // aggregates an edge weight and a node value
    [[nodiscard]] virtual auto agg(EWeight const&, NValue const&) const -> NValue = 0;

    // creates/reuses a node and an incoming edge
    virtual auto branch(var_index, edge_ptr&&, edge_ptr&&) -> edge_ptr = 0;

    // adjusts a weight pair based on the DD type
    [[nodiscard]] virtual auto comb(EWeight const&, EWeight const&) const -> EWeight = 0;

    virtual auto complement(edge_ptr const&) -> edge_ptr = 0;  // computes NOT

    virtual auto conj(edge_ptr const&, edge_ptr const&) -> edge_ptr = 0;  // connects conjuncts (AND)

    virtual auto disj(edge_ptr const&, edge_ptr const&) -> edge_ptr = 0;  // connects disjuncts (OR)

    // evaluates aggregates (subtrees)
    [[nodiscard]] virtual auto merge(NValue const&, NValue const&) const -> NValue = 0;

    virtual auto mul(edge_ptr, edge_ptr) -> edge_ptr = 0;  // combines DDs multiplicatively

    virtual auto plus(edge_ptr, edge_ptr) -> edge_ptr = 0;  // combines DDs additively

    [[nodiscard]] virtual auto regw() const -> EWeight = 0;  // returns the regular weight of an edge

    virtual auto apply(EWeight const& w, edge_ptr const& f) -> edge_ptr  // optimizations vary depending on the DD type
    {
        assert(f);

        return uedge(comb(w, f->w), f->v);
    }

    // NOLINTNEXTLINE(performance-unnecessary-value-param) because simplifications may change pointers
    virtual auto ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr
    {
        assert(f);
        assert(g);
        assert(h);

        return disj(conj(f, g), conj(complement(f), h));
    }

    // Changes the decomposition type for a variable at the bottom level.
    // Default implementation handles bit-level DDs with edge weight inversion for nD transitions.
    // Word-level DDs should override this with appropriate logic.
    virtual void change_decomposition_type(var_index const x, expansion const t)
    {
        assert(x < var_count());
        assert(var2lvl[x] == var_count() - 1);  // must be at bottom level
        assert(vlist[x].ntable.size() == 1);    // only one node at bottom

        gc();
        auto const org_t = vlist[x].t;

        if (org_t == t)
        {
            return;
        }

        // Only on change from or to negative Davio are edge adjustments needed
        if (t == expansion::nD || org_t == expansion::nD)
        {
            // Invert all edges pointing to the node at this level
            boost::unordered_flat_set<edge_ptr, detail::hash, detail::equal> et;
            for (auto it = vlist[x].etable.begin(); it != vlist[x].etable.end();)
            {
                auto edge = *it;
                it = vlist[x].etable.erase(it);
                edge->w = agg(edge->w, true);
                et.insert(edge);
            }
            for (auto it = et.begin(); it != et.end();)
            {
                auto edge = *it;
                it = et.erase(it);
                vlist[x].etable.insert(edge);
            }
        }

        vlist[x].t = t;
        gc();
    }

    auto var(expansion const t, std::string_view lbl)
    {
        if (var_count() > std::numeric_limits<var_index>::max())
        {
            throw std::overflow_error{"The maximum number of supported variables has been reached. "
                                      "Consider changing the variable encoding."};
        }

        auto const x = static_cast<var_index>(var_count());

        var2lvl.push_back(x);
        lvl2var.push_back(x);
        vlist.emplace_back(t, lbl.empty() ? "x"s + std::to_string(x) : lbl, cfg.utable_size_hint);
        // For nD (negative Davio), the variable edge needs to be complemented
        if (t == expansion::nD)
        {
            vars.push_back(uedge(agg(regw(), consts[1]->weight()), unode(x, consts[1], consts[0])));
        }
        else
        {
            vars.push_back(uedge(regw(), unode(x, consts[1], consts[0])));
        }

        assert(var2lvl.size() == var_count());
        assert(lvl2var.size() == var_count());
        assert(vars.size() == var_count());

        return vars[x];
    }

    [[nodiscard]] auto var(var_index const x) const noexcept
    {
        assert(x < vars.size());

        return vars[x];
    }

    auto constant(EWeight w, NValue c, bool const keep_alive)
    {
        if (keep_alive)
        {
            consts.push_back(uedge(std::move(w), unode(std::move(c))));

            assert(consts.size() <= etable.size());

            return consts.back();
        }
        return uedge(std::move(w), unode(std::move(c)));
    }

    [[nodiscard]] auto constant(std::size_t const i) const noexcept
    {
        assert(i < consts.size());

        return consts[i];
    }

    auto decomposition(var_index const x) const noexcept
    {
        assert(x < var_count());

        return vlist[x].decomposition();
    }

    template <typename TruthValue>
        requires std::same_as<TruthValue, bool>
    auto fn(edge_ptr const& f, TruthValue const a)
    {
        assert(f);
        assert(!f->is_const());

        return a ? apply(f->w, f->v->inner.hi) : apply(f->w, f->v->inner.lo);
    }

    template <typename TruthValue, typename... TruthValues>
        requires std::same_as<TruthValue, bool>
    auto fn(edge_ptr const& f, TruthValue const a, TruthValues const... as)
    {
        assert(f);
        assert(!f->is_const());

        return a ? fn(apply(f->w, f->v->inner.hi), as...) : fn(apply(f->w, f->v->inner.lo), as...);
    }

    [[nodiscard]] auto eval(edge_ptr const& f, std::vector<bool> const& as) const
    {
        assert(f);
        assert(as.size() == var_count());

        return agg(f->w, eval(f->v, as));  // assignment follows the made variable ordering
    }

    [[nodiscard]] auto size(std::vector<edge_ptr> const& fs) const
    {
        boost::unordered_flat_set<node*, hash, equal> marks;
        marks.reserve(cfg.utable_size_hint);

        for (auto const& f : fs)
        {  // (shared) number of nodes
            assert(f);

            size(f, marks);
        }

        return marks.size();  // including leaves
    }

    [[nodiscard]] auto depth(std::vector<edge_ptr> const& fs) const
    {
        assert(!fs.empty());

        std::vector<var_index> paths(fs.size());

        parallel_for(0uz, fs.size(), [&fs, &paths, this](std::size_t const i) {
            assert(fs[i]);

            // std::cout << std::this_thread::get_id() << std::endl;
            paths[i] = depth(fs[i]);
        });

        return *std::ranges::max_element(paths) - 1;  // due to the root edge
    }

    [[nodiscard]] auto path_count(edge_ptr const& f) const noexcept -> double  // as results can be very large
    {
        assert(f);

        return f->is_const() ? 1 : path_count(f->v->br().hi) + path_count(f->v->br().lo);  // DFS
    }

    auto has_const(edge_ptr const& f, NValue const& c)
    {
        assert(f);

        if (f->is_const())
        {
            return f->v->outer == c;
        }

        detail::has_const op{f, c};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        op.set_result(has_const(f->v->inner.hi, c) || has_const(f->v->inner.lo, c));
        return cache(std::move(op))->get_result();
    }

    [[nodiscard]] auto is_essential(edge_ptr const& f, var_index const x) const noexcept -> bool
    {
        assert(f);
        assert(x < var2lvl.size());

        return f->is_const() || var2lvl[f->v->inner.x] > var2lvl[x]
                   ? false
                   : f->v->inner.x == x || is_essential(f->v->inner.hi, x) || is_essential(f->v->inner.lo, x);
    }

    auto compose(edge_ptr const& f, var_index const x, edge_ptr const& g)
    {
        assert(f);
        assert(x < var_count());
        assert(g);

        if (f->is_const() || !is_essential(f, x))
        {
            return f;
        }

        detail::compose op{f, x, g};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        edge_ptr hi, lo;
        if (f->v->inner.x == x)
        {
            switch (vlist[x].t)
            {
                case expansion::S:
                {
                    hi = mul(f->v->inner.hi, g);
                    lo = mul(f->v->inner.lo, complement(g));
                    break;
                }
                case expansion::pD:
                {
                    hi = mul(f->v->inner.hi, g);
                    lo = f->v->inner.lo;
                    break;
                }
                case expansion::nD:
                {
                    hi = mul(f->v->inner.hi, complement(g));
                    lo = f->v->inner.lo;
                    break;
                }
                default: assert(false); std::unreachable();
            }
        }
        else
        {
            switch (vlist[f->v->inner.x].t)
            {
                case expansion::S:
                {
                    hi = mul(vars[f->v->inner.x], compose(f->v->inner.hi, x, g));
                    lo = mul(complement(vars[f->v->inner.x]), compose(f->v->inner.lo, x, g));
                    break;
                }
                case expansion::pD:
                {
                    hi = mul(vars[f->v->inner.x], compose(f->v->inner.hi, x, g));
                    lo = compose(f->v->inner.lo, x, g);
                    break;
                }
                case expansion::nD:
                {
                    hi = mul(complement(vars[f->v->inner.x]), compose(f->v->inner.hi, x, g));
                    lo = compose(f->v->inner.lo, x, g);
                    break;
                }
                default: assert(false); std::unreachable();
            }
        }

        op.set_result(apply(f->w, plus(hi, lo)));
        return cache(std::move(op))->get_result();
    }

    auto restr(edge_ptr const& f, var_index const x, bool const a)
    {
        assert(f);
        assert(x < var_count());

        if (!is_essential(f, x))
        {
            return f;
        }
        if (f->v->inner.x == x)
        {
            // Restriction semantics differ from cofactor for Davio decompositions
            switch (vlist[x].t)
            {
                case expansion::S: return cof(f, x, a);
                case expansion::pD:
                    // pD: f = f⁰ ⊕ x·f², so f|_{x=0} = f⁰ = lo, f|_{x=1} = f⁰ ⊕ f² = lo ⊕ hi
                    return apply(f->w, a ? plus(f->v->inner.hi, f->v->inner.lo) : f->v->inner.lo);
                case expansion::nD:
                    // nD: f = f¹ ⊕ x̄·f², so f|_{x=0} = f¹ ⊕ f² = lo ⊕ hi, f|_{x=1} = f¹ = lo
                    return apply(f->w, a ? f->v->inner.lo : plus(f->v->inner.hi, f->v->inner.lo));
                default: assert(false); std::unreachable();
            }
        }

        detail::restr op{f, x, a};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        op.set_result(apply(f->w, branch(f->v->inner.x, restr(f->v->inner.hi, x, a), restr(f->v->inner.lo, x, a))));
        return cache(std::move(op))->get_result();
    }

    auto exist(edge_ptr const& f, var_index const x)
    {
        assert(f);
        assert(x < var_count());

        return disj(restr(f, x, true), restr(f, x, false));
    }

    auto forall(edge_ptr const& f, var_index const x)
    {
        assert(f);
        assert(x < var_count());

        return conj(restr(f, x, true), restr(f, x, false));
    }

    auto dump_dot(std::vector<edge_ptr> const& fs, std::vector<std::string> const& outputs, std::ostream& os) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        os << "digraph DD {\n";
        os << "f [style=invis];\n";
        os << "c [style=invis];\n";

        if (var_count() == 0)
        {
            os << "f -> c [style=invis];\n";
        }
        for (auto i = 0uz; i < lvl2var.size(); ++i)  // arrange nodes per level
        {
            if (i == 0)
            {
                os << "f -> x" << lvl2var[i] << " [style=invis];\n";
            }

            os << 'x' << lvl2var[i] << R"( [shape=plaintext,fontname="times italic",label=")" << lvl2var[i] << " ["
               << to_string(vlist[lvl2var[i]].t) << "]\"];\n";

            if (i + 1 < var_count())
            {  // There will be one more iteration.
                os << 'x' << lvl2var[i] << " -> x" << lvl2var[i + 1] << " [style=invis];\n";
            }
            else
            {
                os << 'x' << lvl2var[i] << " -> c [style=invis];\n";
            }
        }

        boost::unordered_flat_set<node*, hash, equal> marks;
        marks.reserve(cfg.utable_size_hint);

        for (auto i = 0uz; i < fs.size(); ++i)
        {
            assert(fs[i]);

            auto const func = "f"s + std::to_string(i);  // to prevent overwriting identical functions

            os << func << R"( [shape=plaintext,fontname="times bold",label=")" << (outputs.empty() ? func : outputs[i])
               << "\"]\n";
            os << "{ rank=same; f; " << func << "; }\n";
            os << func << " -> v" << fs[i]->v << " [label=\" " << fs[i]->w << " \"];\n";

            dump_dot(fs[i], marks, os);
        }

        os << "}\n";
    }

    auto sift(var_index const lvl_x, var_index const lvl_y)
    {
        if (lvl_x == lvl_y)
        {
            return;
        }

        if (lvl_x < lvl_y)
        {
            for (auto const lvl : std::views::iota(lvl_x, lvl_y))  // [lvl_x, lvl_y)
            {
                exchange(lvl);
            }
        }
        else
        {
            for (auto const lvl : std::views::iota(lvl_y, lvl_x) | std::views::reverse)
            {
                exchange(lvl);
            }
        }
    }

    // DTL (Decomposition Type List) sifting: optimizes variable order and decomposition types
    void dtl_sift(std::vector<edge_ptr> const& fs = {})
    {
        auto comp_largest_layer = [this](var_index const x, var_index const y) {
            return vlist[x].ntable.size() > vlist[y].ntable.size();
        };

        gc();

        std::vector<var_index> tmp_vars(var_count());
        for (var_index x = 0; x < var_count(); ++x)
        {
            tmp_vars[x] = x;
        }
        gc();
        std::ranges::sort(tmp_vars, comp_largest_layer);
        for (var_index i = 0; i < var_count(); ++i)
        {
            dtl_sift_single_var(tmp_vars[i], fs);
        }
        gc();
    }

  private:
    using computed_table = boost::unordered_flat_set<std::unique_ptr<operation>, hash, equal>;  // CT

    // Result type for DTL sifting
    struct dtl_sift_result
    {
        var_index x;
        var_index pos;
        std::size_t size;
        expansion exp;
    };

    [[nodiscard]] auto depth(edge_ptr const& f) const noexcept -> var_index
    {
        return f->is_const() ? 1 : std::max(depth(f->v->inner.hi), depth(f->v->inner.lo)) + 1;
    }

    auto dtl_find_smallest_level(dtl_sift_result const& curr_best, expansion const exp, std::vector<edge_ptr> const& fs)
        -> dtl_sift_result
    {
        auto res = curr_best;
        auto const x = curr_best.x;
        move_to_bottom(x);
        change_decomposition_type(x, exp);
        auto const exceeding_size = static_cast<double>(dtl_get_size(fs)) * cfg.max_node_growth;
        for (auto i = var2lvl[x]; i > 0; --i)
        {
            sift(i, i - 1);
            gc();
            auto const current_size = dtl_get_size(fs);
            if (static_cast<double>(current_size) > exceeding_size)
            {
                break;
            }
            if (current_size < res.size)
            {
                res.size = current_size;
                res.pos = var2lvl[x];
                res.exp = vlist[x].t;
            }
        }
        return res;
    }

    [[nodiscard]] auto dtl_get_size(std::vector<edge_ptr> const& fs) const -> std::size_t
    {
        if (fs.empty())
        {
            return node_count();
        }
        return size(fs);
    }

    auto dtl_sift_single_var(var_index const x, std::vector<edge_ptr> const& fs) -> dtl_sift_result
    {
        dtl_sift_result res{.x = x, .pos = var2lvl[x], .size = dtl_get_size(fs), .exp = vlist[x].t};

        // find smallest level for Shannon
        res = dtl_find_smallest_level(res, expansion::S, fs);

        // find smallest level for Positive Davio
        res = dtl_find_smallest_level(res, expansion::pD, fs);

        // find smallest level for Negative Davio
        res = dtl_find_smallest_level(res, expansion::nD, fs);

        // move variable to smallest level with smallest expansion type
        move_to_bottom(x);
        change_decomposition_type(x, res.exp);
        sift(var2lvl[x], res.pos);

        return res;
    }

    auto dump_dot(edge_ptr const& f, boost::unordered_flat_set<node*, hash, equal>& marks, std::ostream& os) const
    {
        if (!marks.insert(f->v.get()).second)
        {
            return;
        }

        if (f->is_const())
        {
            os << 'v' << f->v << " [shape=box,style=filled,color=chocolate,fontcolor=white,label=\"" << f->v->outer
               << "\"];\n";
            os << "{ rank=same; c; v" << f->v << "; }\n";

            return;
        }

        os << 'v' << f->v << " [shape=" << (vlist[f->v->inner.x].t == expansion::S ? "circle" : "octagon,regular=true")
           << ",style=filled,color=black,fontcolor=white,label=\"" << vlist[f->v->inner.x].lbl << "\"];\n";
        os << "{ rank=same; x" << f->v->inner.x << "; v" << f->v << "; }\n";
        os << 'v' << f->v << " -> v" << f->v->inner.hi->v << " [color=blue,dir=none,label=\" " << f->v->inner.hi->w
           << " \"];\n";
        os << 'v' << f->v << " -> v" << f->v->inner.lo->v << " [style=dashed,color=red,dir=none,label=\" "
           << f->v->inner.lo->w << " \"];\n";

        dump_dot(f->v->inner.hi, marks, os);
        dump_dot(f->v->inner.lo, marks, os);
    }

    [[nodiscard]] auto eval(node_ptr const& v, std::vector<bool> const& as) const -> NValue
    {
        if (v->is_const())
        {
            return v->outer;
        }

        NValue res;
        auto const& br = v->inner;
        switch (vlist[br.x].t)
        {
            case expansion::S:
            {
                res = as[br.x] ? eval(br.hi, as) : eval(br.lo, as);
                break;
            }
            case expansion::pD: res = as[br.x] ? merge(eval(br.hi, as), eval(br.lo, as)) : eval(br.lo, as); break;
            case expansion::nD: res = as[br.x] ? eval(br.lo, as) : merge(eval(br.hi, as), eval(br.lo, as)); break;
            default: assert(false); std::unreachable();
        }
        return res;
    }

    auto exchange(var_index const lvl)  // NOLINT(readability-function-cognitive-complexity)
    {
        if (lvl == var_count() - 1)
        {  // level below contains only constants
            return;
        }

        gc();  // ensure that dead nodes are not swapped

        auto const x = lvl2var[lvl];
        auto const y = lvl2var[lvl + 1];

        auto swap_is_needed = [y](edge_ptr const& hi, edge_ptr const& lo) {
            auto const hi_has_y = !hi->is_const() && hi->v->inner.x == y;
            auto const lo_has_y = !lo->is_const() && lo->v->inner.x == y;
            return hi_has_y || lo_has_y;
        };

        // look ahead to determine how many swaps need to be performed
        auto const max_swaps_needed =
            std::ranges::fold_left(vlist[x].ntable, 0uz, [swap_is_needed](std::size_t const sum, auto const& v) {
                return swap_is_needed(v->inner.hi, v->inner.lo) ? sum + 2 : sum;  // two branches must be made per swap
            });
        if (vlist[x].ntable.size() + max_swaps_needed > vlist[x].ntable.max_load())
        {  // prevent rehashing during swapping, as it invalidates iterators
            vlist[x].ntable.rehash(
                std::max(2 * vlist[x].ntable.bucket_count(),
                         static_cast<std::size_t>(std::ceil((vlist[x].ntable.size() + max_swaps_needed) /
                                                            vlist[x].ntable.max_load_factor()))));

            assert(vlist[x].ntable.size() + max_swaps_needed <= vlist[x].ntable.max_load());
        }

        for (auto node_it = vlist[x].ntable.begin(); node_it != vlist[x].ntable.end();)
        {
            if (swap_is_needed((*node_it)->inner.hi, (*node_it)->inner.lo))  // level swap is a local transformation
            {
                auto v = *node_it;  // to ensure existing references are not lost
                node_it = vlist[x].ntable.erase(node_it);

                auto& br = v->inner;
                auto hi = branch(x, cof(br.hi, y, true), cof(br.lo, y, true));
                br.lo = branch(x, cof(br.hi, y, false), cof(br.lo, y, false));
                br.hi = std::move(hi);
                br.x = y;

                if (!vlist[y].ntable.insert(v).second)
                {  // insertion failed (edge case) => same node already exists
                    auto const& orig_v = *vlist[y].ntable.find(v);
                    boost::unordered_flat_set<edge_ptr, hash, equal> tmp;
                    for (auto edge_it = vlist[x].etable.begin(); edge_it != vlist[x].etable.end();)
                    {
                        if ((*edge_it)->v == v)
                        {  // redirect incoming edge of duplicated node to original node
                            auto e = *edge_it;
                            edge_it = vlist[x].etable.erase(edge_it);

                            e->v = orig_v;

                            tmp.insert(e);
                        }
                        else
                        {
                            ++edge_it;
                        }
                    }
                    vlist[x].etable.merge(tmp);
                }
            }
            else
            {
                ++node_it;
            }
        }

        for (auto it = vlist[x].etable.begin(); it != vlist[x].etable.end();)
        {  // reassign edges pointing to swapped nodes
            if ((*it)->v->inner.x == y)
            {
                vlist[y].etable.insert(*it);
                it = vlist[x].etable.erase(it);
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

    template <class T>
    auto foa(T&& obj, unique_table<T>& ut)
    {  // find or add node/edge
        auto const& search = ut.find(&obj);
        if (search == ut.end())
        {
            if (ut.size() == ut.max_load() && heap_usage() > cfg.heap_mem_limit)
            {  // try GC to avoid expensive rehashing
                gc();
            }
            return *ut.insert(boost::intrusive_ptr<T>{new T{std::forward<T>(obj)}}).first;
        }
        return *search;
    }

    [[nodiscard]] auto heap_usage() const noexcept  // estimation
    {
        auto bytes = 0uz;

        // all current variables and constants
        for (auto const& var : vlist)
        {
            bytes += var.etable.size() * sizeof(edge);
            bytes += var.etable.bucket_count() * sizeof(edge_ptr);
            bytes += var.ntable.size() * sizeof(node);
            bytes += var.ntable.bucket_count() * sizeof(node_ptr);
        }
        bytes += etable.size() * sizeof(edge);
        bytes += etable.bucket_count() * sizeof(edge_ptr);
        bytes += const_count() * sizeof(node);
        bytes += ntable.bucket_count() * sizeof(node_ptr);

        assert(var2lvl.capacity() == vars.capacity());
        assert(lvl2var.capacity() == vars.capacity());

        // variables and constants that are kept alive
        bytes += vars.capacity() * sizeof(edge_ptr) + vars.capacity() * sizeof(var_index) * 2;
        bytes += consts.capacity() * sizeof(edge_ptr);

        // cached operations
        bytes += ct.size() * sizeof(operation);
        bytes += ct.bucket_count() * sizeof(std::unique_ptr<operation>);

        return bytes;
    }

    void move_to_bottom(var_index const x)
    {
        sift(var2lvl[x], static_cast<var_index>(var_count() - 1));
    }

    auto sift_down(var_index lvl, std::pair<var_index, std::size_t>& min)
    {
        auto prev_ncount = 0uz;
        auto curr_ncount = 0uz;
        while (lvl + 1 < var_count() && static_cast<decltype(cfg.max_node_growth)>(prev_ncount) * cfg.max_node_growth >=
                                            static_cast<decltype(cfg.max_node_growth)>(curr_ncount))
        {
            prev_ncount = node_count();
            exchange(lvl++);
            curr_ncount = node_count();

            if (curr_ncount < min.second)
            {
                min = {lvl, curr_ncount};
            }
        }

        return lvl;
    }

    auto sift_up(var_index lvl, std::pair<var_index, std::size_t>& min)
    {
        auto prev_ncount = 0uz;
        auto curr_ncount = 0uz;
        while (lvl > 0 && static_cast<decltype(cfg.max_node_growth)>(prev_ncount) * cfg.max_node_growth >=
                              static_cast<decltype(cfg.max_node_growth)>(curr_ncount))
        {
            --lvl;

            prev_ncount = node_count();
            exchange(lvl);
            curr_ncount = node_count();

            if (curr_ncount < min.second)
            {
                min = {lvl, curr_ncount};
            }
        }

        return lvl;
    }

    auto size(edge_ptr const& f, boost::unordered_flat_set<node*, hash, equal>& marks) const
    {
        if (!marks.insert(f->v.get()).second)
        {  // node already visited
            return;
        }

        if (!f->is_const())
        {  // DD traversal
            size(f->v->inner.hi, marks);
            size(f->v->inner.lo, marks);
        }
    }

    struct config cfg;  // configuration settings such as hash table sizes

    std::vector<edge_ptr> consts;  // DD constants that are never cleared

    computed_table ct;  // to cache already computed results of operations

    unique_table<edge> etable;  // edges pointing to constants

    std::vector<var_index> lvl2var;  // for efficient GC

    unique_table<node> ntable;  // constants

    std::vector<var_index> var2lvl;  // for reasons of reordering

    std::vector<edge_ptr> vars;  // DD variables that are never cleared

    std::vector<variable<EWeight, NValue>> vlist;  // variable list
};

}  // namespace freddy::detail
