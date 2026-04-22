#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                     // config
#include "freddy/detail/dd_base.hpp"             // detail::dd_base
#include "freddy/detail/manager.hpp"             // detail::manager
#include "freddy/detail/node.hpp"                // detail::edge_ptr
#include "freddy/detail/operation/antiv.hpp"     // detail::antiv
#include "freddy/detail/operation/conj.hpp"      // detail::conj
#include "freddy/detail/operation/ite.hpp"       // detail::ite
#include "freddy/detail/operation/sharpsat.hpp"  // detail::sharpsat
#include "freddy/expansion.hpp"                  // expansion::nD

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cstdint>      // std::int32_t
#include <iostream>     // std::cout
#include <ostream>      // std::ostream
#include <string>       // std::string
#include <string_view>  // std::string_view
#include <utility>      // std::move
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

class kfdd_manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

class kfdd final : public detail::dd_base<kfdd, bool, bool, kfdd_manager>
{
    friend kfdd_manager;
    friend dd_base;

    // wrapper is controlled by its KFDD manager
    kfdd(detail::edge_ptr<bool, bool> f, kfdd_manager* const mgr) :
            dd_base{std::move(f), mgr}
    {}

  public:
    static constexpr std::string_view LABEL = "KFDD";

    kfdd() noexcept = default;

    auto operator^=(kfdd const&) -> kfdd&;

    friend auto operator^(kfdd lhs, kfdd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    [[nodiscard]] auto is_complemented() const noexcept
    {
        assert(f);

        return f->weight();
    }

    [[nodiscard]] auto sharpsat() const;

    [[nodiscard]] auto manager() const noexcept -> kfdd_manager const&
    {
        return *mgr;
    }
};

class kfdd_manager final : public detail::manager<bool, bool>
{
  public:
    explicit kfdd_manager(struct config const cfg = {}) :
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) because KFDD terminals are intrusive
            manager{tmls(), cfg}
    {}

    auto var(expansion const t = expansion::S, std::string_view lbl = {})
    {
        return kfdd{manager::var(t, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        return kfdd{manager::var(x), this};
    }

    auto zero() noexcept
    {
        return kfdd{constant(0), this};
    }

    auto one() noexcept
    {
        return kfdd{constant(1), this};
    }

    [[nodiscard]] auto size(std::vector<kfdd> const& fs) const
    {
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<kfdd> const& fs) const
    {
        assert(!fs.empty());

        return manager::depth(transform(fs));
    }

    auto dump_dot(std::vector<kfdd> const& fs, std::vector<std::string> const& outputs = {},
                  std::ostream& os = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        manager::dump_dot(transform(fs), outputs, os);
    }

    // DTL sifting wrapper for KFDD-typed vectors
    void dtl_sift()
    {
        manager::dtl_sift({});
    }

    void dtl_sift(std::vector<kfdd> const& fs)
    {
        manager::dtl_sift(transform(fs));
    }

    // Public alias for change_decomposition (compatibility with tests)
    void change_expansion_type(var_index const x, expansion const t)
    {
        change_decomposition(x, t);
    }

  private:
    friend kfdd;

    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        node_ptr const leaf{new node{false}};  // use the 0-leaf to support complemented edges and ensure canonicity
        return {edge_ptr{new edge{false, leaf}}, edge_ptr{new edge{true, leaf}}};
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    static auto transform(std::vector<kfdd> const& gs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        assert(f);
        assert(g);

        if (f == constant(0))
        {
            return g;
        }
        if (g == constant(0))
        {
            return f;
        }
        if (f == constant(1))
        {
            return complement(g);
        }
        if (g == constant(1))
        {
            return complement(f);
        }
        if (f == g)
        {
            return constant(0);
        }
        if (f == complement(g))
        {
            return constant(1);
        }

        detail::antiv op{f, g};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = top_var(f, g);

        op.set_result(branch(x, antiv(cof(f, x, true), cof(g, x, true)), antiv(cof(f, x, false), cof(g, x, false))));
        return cache(std::move(op))->get_result();
    }

    auto sharpsat(edge_ptr const& f)
    {
        assert(f);

        if (f->is_const())
        {
            return f == constant(0) ? 0 : static_cast<double>(1uz << var_count());  // 2^var_count()
        }

        detail::sharpsat op{f};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto count = (sharpsat(f->ch()->br().hi) + sharpsat(f->ch()->br().lo)) / 2;
        if (f->weight())
        {  // complemented edge
            count = static_cast<double>(1uz << var_count()) - count;
        }

        op.set_result(count);
        return cache(std::move(op))->get_result();
    }

    auto simplify(edge_ptr const& f, edge_ptr& g, edge_ptr& h) const noexcept
    {
        if (f == g)
        {  // ite(f, f, h) => ite(f, 1, h)
            g = constant(1);
            return 1;
        }
        if (f == h)
        {  // ite(f, g, f) => ite(f, g, 0)
            h = constant(0);
            return 2;
        }
        if (f->ch() == h->ch() && f->weight() != h->weight())
        {  // ite(f, g, ~f) => ite(f, g, 1)
            h = constant(1);
            return 3;
        }
        if (f->ch() == g->ch() && f->weight() != g->weight())
        {  // ite(f, ~f, h) => ite(f, 0, h)
            g = constant(0);
            return 4;
        }
        return 0;
    }

    auto std_triple(std::int32_t const simpl, edge_ptr& f, edge_ptr& g, edge_ptr& h)
    {
        switch (simpl)
        {
            case 1:
                assert(!h->is_const());

                if (lvl_ge(f->ch()->br().x, h->ch()->br().x))
                {  // ite(f, 1, h) == ite(h, 1, f)
                    std::swap(f, h);
                }
                break;
            case 2:
                assert(!g->is_const());

                if (lvl_ge(f->ch()->br().x, g->ch()->br().x))
                {  // ite(f, g, 0) == ite(g, f, 0)
                    std::swap(f, g);
                }
                break;
            case 3:
                assert(!g->is_const());

                if (lvl_ge(f->ch()->br().x, g->ch()->br().x))
                {  // ite(f, g, 1) == ite(~g, ~f, 1)
                    std::swap(f, g);

                    f = complement(f);
                    g = complement(g);
                }
                break;
            case 4:
                assert(!h->is_const());

                if (lvl_ge(f->ch()->br().x, h->ch()->br().x))
                {  // ite(f, 0, h) == ite(~h, 0, ~f)
                    std::swap(f, h);

                    f = complement(f);
                    h = complement(h);
                }
                break;
            default: assert(false); std::unreachable();
        }
    }

    [[nodiscard]] auto agg(bool const& w, bool const& val) const noexcept -> bool override
    {
        return w != val;  // XOR
    }

    [[nodiscard]] auto comb(bool const& w1, bool const& w2) const noexcept -> bool override
    {
        return w1 != w2;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override  // O(1) instead of O(n) where n is the number of nodes
    {
        assert(f);

        return f->weight() ? uedge(false, f->ch()) : uedge(true, f->ch());
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        // terminal cases
        if (f == constant(0) || g == constant(0))
        {
            return constant(0);
        }
        if (f == constant(1))
        {
            return g;
        }
        if (g == constant(1))
        {
            return f;
        }

        detail::conj op{f, g};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = top_var(f, g);
        auto f_low = cof(f, x, false);
        auto f_high = cof(f, x, true);
        auto g_low = cof(g, x, false);
        auto g_high = cof(g, x, true);

        edge_ptr high;
        edge_ptr low;

        switch (decomposition(x))
        {
            case expansion::S:
                high = conj(f_high, g_high);
                low = conj(f_low, g_low);
                break;
            case expansion::pD:
            case expansion::nD:
                high = antiv(conj(f_high, g_high), antiv(conj(f_low, g_high), conj(g_low, f_high)));
                low = conj(f_low, g_low);
                break;
            default: assert(false); std::unreachable();
        }

        op.set_result(branch(x, std::move(high), std::move(low)));
        return cache(std::move(op))->get_result();
    }

    [[nodiscard]] auto denorm_high(edge_ptr const& f) -> edge_ptr override
    {
        switch (decomposition(f->ch()->br().x))
        {
            case expansion::S: return apply(f->weight(), f->ch()->br().hi);
            case expansion::pD:
            case expansion::nD: return f->ch()->br().hi;
            default: assert(false); std::unreachable();
        }
    }

    [[nodiscard]] auto denorm_low(edge_ptr const& f) -> edge_ptr override
    {
        return apply(f->weight(), f->ch()->br().lo);
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return complement(conj(complement(f), complement(g)));
    }

    [[nodiscard]] auto expanded(edge_ptr const& f, bool const a, expansion const t) const noexcept -> edge_ptr override
    {
        if ((t == expansion::pD || t == expansion::nD) && a)  // dependent on two subtrees: f ^ f = 0
        {
            return constant(0);
        }
        return f;
    }

    auto ite(edge_ptr f, edge_ptr g, edge_ptr h) -> edge_ptr override
    {
        assert(f);
        assert(g);
        assert(h);

        auto const ret = simplify(f, g, h);

        // terminal cases
        if (f == constant(0))
        {
            return h;
        }
        if (f == constant(1) || g == h)
        {
            return g;
        }
        if (h == constant(0) && g == constant(1))
        {
            return f;
        }
        if (g == constant(0) && h == constant(1))
        {
            return complement(f);
        }

        if (ret != 0)
        {
            std_triple(ret, f, g, h);
        }

        detail::ite op{f, g, h};
        if (auto const* const entry = cached(op))
        {
            return entry->get_result();
        }

        auto const x = f->ch()->br().x == top_var(f, g) ? top_var(f, h) : top_var(g, h);

        op.set_result(branch(x, ite(cof(f, x, true), cof(g, x, true), cof(h, x, true)),
                             ite(cof(f, x, false), cof(g, x, false), cof(h, x, false))));
        return cache(std::move(op))->get_result();
    }

    [[nodiscard]] auto merge(bool const& val1, bool const& val2) const noexcept -> bool override
    {
        return val1 != val2;
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return conj(f, g);
    }

    [[nodiscard]] auto norm_high(edge_ptr const& hi, bool, expansion const t) -> edge_ptr override
    {
        switch (t)
        {
            case expansion::S: return complement(hi);
            case expansion::pD:
            case expansion::nD: return hi;
            default: assert(false); std::unreachable();
        }
    }

    [[nodiscard]] auto norm_is_needed(edge_ptr const&, edge_ptr const& lo) const noexcept -> bool override
    {
        return lo->weight();
    }

    [[nodiscard]] auto norm_low(edge_ptr const& lo, bool, expansion) -> edge_ptr override
    {
        return complement(lo);
    }

    [[nodiscard]] auto norm_weight(edge_ptr const&, edge_ptr const& lo) const noexcept -> bool override
    {
        return lo->weight();
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override
    {
        return antiv(f, g);
    }

    [[nodiscard]] auto reduced(edge_ptr const&, edge_ptr const& lo, expansion) noexcept -> edge_ptr override
    {
        return lo;
    }

    [[nodiscard]] auto reducible(edge_ptr const& hi, edge_ptr const& lo, expansion const t) const noexcept
        -> bool override
    {
        switch (t)
        {
            case expansion::S: return hi == lo;
            case expansion::pD:
            case expansion::nD: return hi == constant(0);
            default: assert(false); std::unreachable();
        }
    }

    [[nodiscard]] auto regw() const noexcept -> bool override
    {
        return false;  // means a regular (non-complemented) edge
    }
};

inline auto kfdd::operator^=(kfdd const& rhs) -> kfdd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);

    return *this;
}

inline auto kfdd::sharpsat() const
{
    assert(mgr);

    return mgr->sharpsat(f);
}

}  // namespace freddy
