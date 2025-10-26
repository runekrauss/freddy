#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"                 // config
#include "freddy/detail/manager.hpp"         // detail::manager
#include "freddy/detail/node.hpp"            // detail::intrusive_edge_ptr
#include "freddy/detail/operation/mul.hpp"   // detail::mul
#include "freddy/detail/operation/plus.hpp"  // detail::plus
#include "freddy/expansion.hpp"              // expansion::pD

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <boost/safe_numerics/safe_integer.hpp>  // boost::safe_numerics::safe
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <algorithm>    // std::ranges::transform
#include <array>        // std::array
#include <cassert>      // assert
#include <cmath>        // std::abs
#include <concepts>     // std::integral
#include <cstdint>      // std::int64_t
#include <iostream>     // std::cout
#include <limits>       // std::numeric_limits
#include <numeric>      // std::gcd
#include <ostream>      // std::ostream
#include <ranges>       // std::views::iota
#include <string>       // std::string
#include <string_view>  // hash
#include <type_traits>  // std::is_signed_v
#include <utility>      // std::move
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace std
{

// add a specialization of the standard library's hash function now to enable hashing of safe weight and node values
template <std::integral RawInt>
struct hash<boost::safe_numerics::safe<RawInt>> final
{
    auto operator()(boost::safe_numerics::safe<RawInt> const& val) const noexcept
    {
        return hash<RawInt>{}(val);
    }
};

}  // namespace std

namespace freddy
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

class bmd_manager;

// =====================================================================================================================
// Aliases
// =====================================================================================================================

// used as weight and node value due to numerical interpretations and evaluations
using bmd_int = boost::safe_numerics::safe<std::int64_t>;

// no alias for the underlying type here to avoid "polluting" the namespace
static_assert(std::is_integral_v<boost::safe_numerics::base_type<bmd_int>::type> &&
                  std::is_signed_v<boost::safe_numerics::base_type<bmd_int>::type>,
              "bmd_int must be signed");

// =====================================================================================================================
// Types
// =====================================================================================================================

class bmd final  // (multiplicative) binary moment diagram
{
  public:
    bmd() noexcept = default;  // enable default BMD construction for compatibility with standard containers

    auto operator-() const;

    auto operator*=(bmd const&) -> bmd&;

    auto operator+=(bmd const&) -> bmd&;

    auto operator-=(bmd const&) -> bmd&;

    auto operator~() const;

    auto operator&=(bmd const&) -> bmd&;

    auto operator|=(bmd const&) -> bmd&;

    auto operator^=(bmd const&) -> bmd&;

    friend auto operator*(bmd lhs, bmd const& rhs)
    {
        lhs *= rhs;
        return lhs;
    }

    friend auto operator+(bmd lhs, bmd const& rhs)
    {
        lhs += rhs;
        return lhs;
    }

    friend auto operator-(bmd lhs, bmd const& rhs)
    {
        lhs -= rhs;
        return lhs;
    }

    friend auto operator&(bmd lhs, bmd const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    friend auto operator|(bmd lhs, bmd const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend auto operator^(bmd lhs, bmd const& rhs)
    {
        lhs ^= rhs;
        return lhs;
    }

    friend auto operator==(bmd const& lhs, bmd const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);  // check for the same BMD manager

        return lhs.f == rhs.f;
    }

    friend auto operator!=(bmd const& lhs, bmd const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend auto operator<<(std::ostream& os, bmd const& g) -> std::ostream&
    {
        os << "BMD handle: " << g.f << '\n';
        os << "BMD manager: " << g.mgr;
        return os;
    }

    [[nodiscard]] auto same_node(bmd const& g) const noexcept
    {
        assert(f);
        assert(mgr == g.mgr);  // BMD g is valid in any case

        return f->ch() == g.f->ch();
    }

    [[nodiscard]] auto weight() const noexcept
    {
        assert(f);

        return f->weight();
    }

    [[nodiscard]] auto is_const() const noexcept
    {
        assert(f);

        return f->is_const();
    }

    [[nodiscard]] auto var() const noexcept
    {
        assert(!is_const());

        return f->ch()->br().x;
    }

    [[nodiscard]] auto high() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return bmd{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return bmd{f->ch()->br().lo, mgr};
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    [[nodiscard]] auto is_two() const noexcept;

    template <typename TruthValue, typename... TruthValues>
    auto fn(TruthValue, TruthValues...) const;

    [[nodiscard]] auto eval(std::vector<bool> const&) const;

    [[nodiscard]] auto ite(bmd const&, bmd const&) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto is_essential(var_index) const noexcept;

    [[nodiscard]] auto compose(var_index, bmd const&) const;

    [[nodiscard]] auto restr(var_index, bool) const;

    [[nodiscard]] auto exist(var_index) const;

    [[nodiscard]] auto forall(var_index) const;

    auto dump_dot(std::ostream& = std::cout) const;

  private:
    friend bmd_manager;

    // wrapper is controlled by its BMD manager
    bmd(detail::intrusive_edge_ptr<bmd_int, bmd_int> f, bmd_manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        assert(this->f);
        assert(this->mgr);
    }

    detail::intrusive_edge_ptr<bmd_int, bmd_int> f;  // BMD handle

    bmd_manager* mgr{};  // must be destroyed after this BMD wrapper
};

class bmd_manager final : public detail::manager<bmd_int, bmd_int>
{
  public:
    explicit bmd_manager(struct config const cfg = {}) :
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) because BMD terminals are intrusive
            manager{tmls(), cfg}
    {
        // constants with eternal lifetime
        manager::constant(2, 1, true);
        manager::constant(-1, 1, true);
    }

    auto var(std::string_view lbl = {})
    {
        return bmd{manager::var(expansion::pD, lbl), this};
    }

    auto var(var_index const x) noexcept
    {
        return bmd{manager::var(x), this};
    }

    auto constant(bmd_int const w, bool const keep_alive = false)
    {
        return bmd{manager::constant(w, 1, keep_alive), this};
    }

    auto zero() noexcept
    {
        return bmd{manager::constant(0), this};
    }

    auto one() noexcept
    {
        return bmd{manager::constant(1), this};
    }

    auto two() noexcept
    {
        return bmd{manager::constant(2), this};
    }

    [[nodiscard]] auto size(std::vector<bmd> const& fs) const
    {
        return manager::size(transform(fs));
    }

    [[nodiscard]] auto depth(std::vector<bmd> const& fs) const
    {
        assert(!fs.empty());

        return manager::depth(transform(fs));
    }

    auto unsigned_bin(std::vector<bmd> const& fs)  // unsigned binary encoding
    {
        assert(fs.size() < std::numeric_limits<bmd_int>::digits);  // since weights are represented by bmd_int

        auto sum = manager::constant(0);
        for (auto const i : std::views::iota(0uz, fs.size()))  // LSB...MSB
        {
            auto const w = static_cast<bmd_int>(1uz << i);  // 2^i
            sum = plus(sum, mul(fs[i].f, manager::constant(w, 1, false)));
        }
        return bmd{sum, this};  // sum of weighted bits
    }

    auto twos_complement(std::vector<bmd> const& fs)
    {
        assert(!fs.empty());
        assert(fs.size() <= std::numeric_limits<bmd_int>::digits);

        auto const w = -static_cast<bmd_int>(1uz << (fs.size() - 1));
        return bmd{plus(apply(w, fs.back().f), unsigned_bin({fs.begin(), fs.end() - 1}).f), this};
    }

    auto dump_dot(std::vector<bmd> const& fs, std::vector<std::string> const& outputs = {},
                  std::ostream& os = std::cout) const
    {
        assert(outputs.empty() ? true : outputs.size() == fs.size());

        manager::dump_dot(transform(fs), outputs, os);
    }

  private:
    using raw_int = boost::safe_numerics::base_type<bmd_int>::type;

    friend bmd;

    // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
    static auto tmls() -> std::array<edge_ptr, 2>
    {
        node_ptr const leaf{new node{1}};
        return {edge_ptr{new edge{0, leaf}}, edge_ptr{new edge{1, leaf}}};
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

    static auto transform(std::vector<bmd> const& gs) -> std::vector<edge_ptr>
    {
        std::vector<edge_ptr> fs(gs.size());
        std::ranges::transform(gs, fs.begin(), [](auto const& g) { return g.f; });
        return fs;
    }

    static auto normw(edge_ptr const& f, edge_ptr const& g) noexcept -> bmd_int
    {
        // as there is no risk of overflow in this operation
        auto const fw = static_cast<raw_int>(f->weight());
        auto const gw = static_cast<raw_int>(g->weight());
        return gw < 0 || (fw < 0 && gw == 0) ? -std::gcd(fw, gw) : std::gcd(fw, gw);
    }

    auto neg(edge_ptr const& f)
    {
        assert(f);

        return f == manager::constant(0) ? f : mul(manager::constant(3), f);  // -1f
    }

    auto sub(edge_ptr const& f, edge_ptr const& g)
    {
        return plus(f, neg(g));
    }

    auto antiv(edge_ptr const& f, edge_ptr const& g)
    {
        return sub(plus(f, g), mul(manager::constant(2), mul(f, g)));
    }

    [[nodiscard]] auto agg(bmd_int const& w, bmd_int const& val) const -> bmd_int override
    {
        return w * val;
    }

    auto apply(bmd_int const& w, edge_ptr const& f) -> edge_ptr override
    {
        assert(f);

        if (w == 1)
        {
            return f;
        }
        if (w == 0 || f->weight() == 0)
        {
            return manager::constant(0);
        }
        return uedge(comb(w, f->weight()), f->ch());
    }

    auto branch(var_index const x, edge_ptr&& hi, edge_ptr&& lo) -> edge_ptr override
    {
        assert(x < var_count());
        assert(hi);
        assert(lo);

        if (hi == manager::constant(0))  // redundancy rule
        {
            return lo;
        }

        // normalization
        auto const w = normw(hi, lo);

        assert(w != 0);

        return w != 1 ? uedge(w, unode(x, uedge(hi->weight() / w, hi->ch()), uedge(lo->weight() / w, lo->ch())))
                      : uedge(w, unode(x, std::move(hi), std::move(lo)));
    }

    [[nodiscard]] auto comb(bmd_int const& w1, bmd_int const& w2) const -> bmd_int override
    {
        return w1 * w2;
    }

    auto complement(edge_ptr const& f) -> edge_ptr override
    {
        return sub(manager::constant(1), f);
    }

    auto conj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        return mul(f, g);
    }

    auto disj(edge_ptr const& f, edge_ptr const& g) -> edge_ptr override
    {
        assert(f);
        assert(g);

        if (f == manager::constant(0))
        {
            return g;
        }
        if (g == manager::constant(0))
        {
            return f;
        }
        return sub(plus(f, g), mul(f, g));
    }

    [[nodiscard]] auto merge(bmd_int const& val1, bmd_int const& val2) const -> bmd_int override
    {
        return val1 + val2;
    }

    auto mul(edge_ptr f, edge_ptr g) -> edge_ptr override  // defined over "words"
    {
        assert(f);
        assert(g);

        // base case checks
        if (f == manager::constant(0) || g == manager::constant(0))
        {
            return manager::constant(0);
        }
        if (f->is_const())
        {
            return apply(f->weight(), g);
        }
        if (g->is_const())
        {
            return apply(g->weight(), f);
        }

        // increase chances of reusing previously computed results (rearrange)
        auto const w = f->weight() * g->weight();
        if ((*f->ch())() <= (*g->ch())())  // comparison of hash values
        {
            std::swap(f, g);
        }
        f = uedge(1, f->ch());
        g = uedge(1, g->ch());

        detail::mul op{f, g};
        if (auto const* const entry = cached(op))
        {
            return apply(w, entry->get_result());
        }

        auto const x = top_var(f, g);
        auto hi = plus(mul(cof(f, x, true), cof(g, x, true)),
                       plus(mul(cof(f, x, true), cof(g, x, false)), mul(cof(f, x, false), cof(g, x, true))));
        auto const res = branch(x, std::move(hi), mul(cof(f, x, false), cof(g, x, false)));

        op.set_result(res);
        cache(std::move(op));

        return apply(w, res);
    }

    auto plus(edge_ptr f, edge_ptr g) -> edge_ptr override  // word-level addition
    {
        assert(f);
        assert(g);

        if (f == manager::constant(0))
        {
            return g;
        }
        if (g == manager::constant(0))
        {
            return f;
        }
        if (f->ch() == g->ch())
        {
            auto const sum = f->weight() + g->weight();
            return sum == 0 ? manager::constant(0) : uedge(sum, f->ch());
        }

        // rearrange
        bmd_int w;
        if (std::abs(static_cast<raw_int>(f->weight())) <= std::abs(static_cast<raw_int>(g->weight())))
        {
            std::swap(f, g);
            w = normw(f, g);
        }
        else
        {
            w = normw(g, f);
        }
        f = uedge(f->weight() / w, f->ch());
        g = uedge(g->weight() / w, g->ch());

        detail::plus op{f, g};
        if (auto const* const entry = cached(op))
        {
            return apply(w, entry->get_result());
        }

        auto const x = top_var(f, g);
        auto const res = branch(x, plus(cof(f, x, true), cof(g, x, true)), plus(cof(f, x, false), cof(g, x, false)));

        op.set_result(res);
        cache(std::move(op));

        return apply(w, res);
    }

    [[nodiscard]] auto regw() const noexcept -> bmd_int override
    {
        return 1;
    }
};

inline auto bmd::operator-() const
{
    assert(mgr);

    return bmd{mgr->neg(f), mgr};
}

inline auto bmd::operator*=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->mul(f, rhs.f);

    return *this;
}

inline auto bmd::operator+=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->plus(f, rhs.f);

    return *this;
}

inline auto bmd::operator-=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->sub(f, rhs.f);

    return *this;
}

inline auto bmd::operator~() const
{
    assert(mgr);

    return bmd{mgr->complement(f), mgr};
}

inline auto bmd::operator&=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->conj(f, rhs.f);

    return *this;
}

inline auto bmd::operator|=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->disj(f, rhs.f);

    return *this;
}

inline auto bmd::operator^=(bmd const& rhs) -> bmd&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr->antiv(f, rhs.f);

    return *this;
}

inline auto bmd::is_zero() const noexcept
{
    assert(mgr);

    return *this == mgr->zero();
}

inline auto bmd::is_one() const noexcept
{
    assert(mgr);

    return *this == mgr->one();
}

inline auto bmd::is_two() const noexcept
{
    assert(mgr);

    return *this == mgr->two();
}

template <typename TruthValue, typename... TruthValues>
inline auto bmd::fn(TruthValue const a, TruthValues... as) const
{
    assert(mgr);

    return bmd{mgr->fn(f, a, std::forward<TruthValues>(as)...), mgr};
}

inline auto bmd::eval(std::vector<bool> const& as) const
{
    assert(mgr);

    return mgr->eval(f, as);
}

inline auto bmd::ite(bmd const& g, bmd const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return bmd{mgr->ite(f, g.f, h.f), mgr};
}

inline auto bmd::size() const
{
    assert(mgr);

    return mgr->size({*this});
}

inline auto bmd::depth() const
{
    assert(mgr);

    return mgr->depth({*this});
}

inline auto bmd::path_count() const noexcept
{
    assert(mgr);

    return mgr->path_count(f);
}

inline auto bmd::is_essential(var_index const x) const noexcept
{
    assert(mgr);

    return mgr->is_essential(f, x);
}

inline auto bmd::compose(var_index const x, bmd const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return bmd{mgr->compose(f, x, g.f), mgr};
}

inline auto bmd::restr(var_index const x, bool const a) const
{
    assert(mgr);

    return bmd{mgr->restr(f, x, a), mgr};
}

inline auto bmd::exist(var_index const x) const
{
    assert(mgr);

    return bmd{mgr->exist(f, x), mgr};
}

inline auto bmd::forall(var_index const x) const
{
    assert(mgr);

    return bmd{mgr->forall(f, x), mgr};
}

inline auto bmd::dump_dot(std::ostream& os) const
{
    assert(mgr);

    mgr->dump_dot({*this}, {}, os);
}

}  // namespace freddy
