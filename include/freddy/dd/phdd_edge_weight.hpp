#ifndef FREDDY_PHDD_EDGE_WEIGHT_HPP
#define FREDDY_PHDD_EDGE_WEIGHT_HPP

#include <memory>

// implements hashing and string view for std::pair<bool, int32_t>

using edge_weight = std::pair<bool,int32_t>;

namespace std
{
    template <>
    struct [[maybe_unused]] hash<edge_weight> {
        auto operator()(const edge_weight &v) const -> std::size_t
        {
            return v.first ?  std::hash<int>()(v.second) ^ (1 << 31) : std::hash<int>()(v.second);
        }
    };

    inline std::ostream & operator << (std::ostream & os, const edge_weight & val)
    {
        os << (val.first ? "neg\n" : "") << val.second;
        return os;
    }
}  // namespace std

#endif  //FREDDY_PHDD_EDGE_WEIGHT_HPP
