#pragma once

// *********************************************************************************************************************
// Includes
// *********************************************************************************************************************

#include "freddy/config.hpp"       // var_index
#include "freddy/detail/node.hpp"  // detail::edge_ptr

#include <cassert>      // assert
#include <iostream>     // std::cout
#include <ostream>      // std::ostream
#include <type_traits>  // std::is_base_of_v
#include <utility>      // std::forward, std::move
#include <vector>       // std::vector

// *********************************************************************************************************************
// Namespaces
// *********************************************************************************************************************

namespace freddy::detail
{

// =====================================================================================================================
// Forwards
// =====================================================================================================================

template <hashable, hashable>
class manager;

// =====================================================================================================================
// Types
// =====================================================================================================================

template <typename Derived, typename EWeight, typename NValue, typename Manager>
class dd_base
{
  public:
    dd_base() noexcept = default;

    auto operator~() const;

    auto operator&=(Derived const&) -> Derived&;

    auto operator|=(Derived const&) -> Derived&;

    friend auto operator&(Derived lhs, Derived const& rhs)
    {
        lhs &= rhs;
        return lhs;
    }

    friend auto operator|(Derived lhs, Derived const& rhs)
    {
        lhs |= rhs;
        return lhs;
    }

    friend auto operator==(Derived const& lhs, Derived const& rhs) noexcept
    {
        assert(lhs.mgr == rhs.mgr);

        return lhs.f == rhs.f;
    }

    friend auto operator!=(Derived const& lhs, Derived const& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    friend auto operator<<(std::ostream& os, Derived const& g) -> std::ostream&
    {
        os << Derived::LABEL << " handle: " << g.f << '\n';
        os << Derived::LABEL << " manager: " << g.mgr;
        return os;
    }

    [[nodiscard]] auto same_node(Derived const& g) const noexcept
    {
        assert(f);
        assert(mgr == g.mgr);

        return f->ch() == g.f->ch();
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

        return Derived{f->ch()->br().hi, mgr};
    }

    [[nodiscard]] auto low() const noexcept
    {
        assert(mgr);
        assert(!is_const());

        return Derived{f->ch()->br().lo, mgr};
    }

    [[nodiscard]] auto is_zero() const noexcept;

    [[nodiscard]] auto is_one() const noexcept;

    template <typename TruthValue, typename... TruthValues>
    auto fn(TruthValue, TruthValues...) const noexcept;

    [[nodiscard]] auto eval(std::vector<bool> const&) const noexcept;

    [[nodiscard]] auto ite(Derived const&, Derived const&) const;

    [[nodiscard]] auto size() const;

    [[nodiscard]] auto depth() const;

    [[nodiscard]] auto path_count() const noexcept;

    [[nodiscard]] auto is_essential(var_index) const noexcept;

    [[nodiscard]] auto compose(var_index, Derived const&) const;

    [[nodiscard]] auto restr(var_index, bool) const;

    [[nodiscard]] auto exist(var_index) const;

    [[nodiscard]] auto forall(var_index) const;

    auto dump_dot(std::ostream& = std::cout) const;

  protected:
    edge_ptr<EWeight, NValue> f;  // DD handle

    Manager* mgr{};  // must be destroyed after this DD wrapper

    dd_base(detail::edge_ptr<EWeight, NValue> f, Manager* const mgr) :
            f{std::move(f)},
            mgr{mgr}
    {
        static_assert(std::is_base_of_v<manager<EWeight, NValue>, Manager>,
                      "Manager must derive from detail::manager<EWeight, NValue>");
        assert(this->f);
        assert(this->mgr);
    }

  private:
    auto derived() noexcept -> Derived&
    {
        return static_cast<Derived&>(*this);
    }

    auto derived() const noexcept -> Derived const&
    {
        return static_cast<Derived const&>(*this);
    }

    auto mgr_base() const -> manager<EWeight, NValue>*
    {
        return mgr;  // manager outlives DD wrapper and is always mutable
    }
};

// =====================================================================================================================
// Out-of-line definitions
// =====================================================================================================================

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::operator~() const
{
    assert(mgr);

    return Derived{mgr_base()->complement(f), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::operator&=(Derived const& rhs) -> Derived&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr_base()->conj(f, rhs.f);

    return derived();
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::operator|=(Derived const& rhs) -> Derived&
{
    assert(mgr);
    assert(mgr == rhs.mgr);

    f = mgr_base()->disj(f, rhs.f);

    return derived();
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::is_zero() const noexcept
{
    assert(mgr);

    return derived() == mgr->zero();
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::is_one() const noexcept
{
    assert(mgr);

    return derived() == mgr->one();
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
template <typename TruthValue, typename... TruthValues>
inline auto dd_base<Derived, EWeight, NValue, Manager>::fn(TruthValue const a, TruthValues... as) const noexcept
{
    assert(mgr);

    return Derived{mgr_base()->fn(f, a, std::forward<TruthValues>(as)...), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::eval(std::vector<bool> const& as) const noexcept
{
    assert(mgr);

    return mgr_base()->eval(f, as);
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::ite(Derived const& g, Derived const& h) const
{
    assert(mgr);
    assert(mgr == g.mgr);
    assert(g.mgr == h.mgr);

    return Derived{mgr_base()->ite(f, g.f, h.f), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::size() const
{
    assert(mgr);

    return mgr->size({derived()});
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::depth() const
{
    assert(mgr);

    return mgr->depth({derived()});
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::path_count() const noexcept
{
    assert(mgr);

    return mgr_base()->path_count(f);
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::is_essential(var_index const x) const noexcept
{
    assert(mgr);

    return mgr_base()->is_essential(f, x);
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::compose(var_index const x, Derived const& g) const
{
    assert(mgr);
    assert(mgr == g.mgr);

    return Derived{mgr_base()->compose(f, x, g.f), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::restr(var_index const x, bool const a) const
{
    assert(mgr);

    return Derived{mgr_base()->restr(f, x, a), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::exist(var_index const x) const
{
    assert(mgr);

    return Derived{mgr_base()->exist(f, x), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::forall(var_index const x) const
{
    assert(mgr);

    return Derived{mgr_base()->forall(f, x), mgr};
}

template <typename Derived, typename EWeight, typename NValue, typename Manager>
inline auto dd_base<Derived, EWeight, NValue, Manager>::dump_dot(std::ostream& os) const
{
    assert(mgr);

    mgr->dump_dot({derived()}, {}, os);
}

}  // namespace freddy::detail
