#ifndef FREDDY_PHDD_EDGE_WEIGHT_HPP
#define FREDDY_PHDD_EDGE_WEIGHT_HPP

#include <memory>

// implements hashing and string view for std::pair<bool, int32_t>

namespace std
{
    template <>
    struct [[maybe_unused]] hash<std::pair<bool,int32_t>> {
        auto operator()(const std::pair<bool,int32_t> &v) const -> std::size_t
        {
            return v.first ?  std::hash<int>()(v.second) ^ (1 << 31) : std::hash<int>()(v.second);
        }
    };

    std::ostream & operator << (std::ostream & os, const std::pair<bool,int32_t> & val)
    {
        os << val.first << " ;" << val.second;
        return os;
    }
}  // namespace std

#endif  //FREDDY_PHDD_EDGE_WEIGHT_HPP
