#pragma once

#if __cplusplus >= 202302L
#include <generator>
#endif
#include <iterator>
#include <sstream>
#include <iomanip>
#include <memory>
#include <string>
#include <vector>
#include <attributeenums.H>
#include <attributetraits.H>
extern "C"
{
#include <libfdt.h>
}
namespace TARGETING
{
class Target;
using TargetPtr = std::shared_ptr<Target>;

class Target : public std::enable_shared_from_this<Target>
{
  public:
    explicit Target(std::string name, int offset, const void* fdt) :
        _name(std::move(name)), _offset(offset), _fdt(fdt)
    {}

    static TargetPtr create(std::string name, int offset, const void* fdt)
    {
        return std::make_shared<Target>(std::move(name), offset, fdt);
    }

    void addChild(const TargetPtr& child);

    [[nodiscard]] const std::vector<TargetPtr>& getChildren() const noexcept
    {
        return _children;
    }

    [[nodiscard]] TargetPtr getParent() const noexcept
    {
        return _parent;
    }

    [[nodiscard]] const std::string& getName() const noexcept
    {
        return _name;
    }

    [[nodiscard]] int getOffset() const noexcept
    {
        return _offset;
    }
    [[nodiscard]]const void* fdt() const noexcept
    { 
        return _fdt; 
    }
#if __cplusplus >= 202302L
    [[nodiscard]] std::generator<TargetPtr> ancestors() const;
#endif
    template<const TARGETING::ATTRIBUTE_ID A>
    bool tryGetAttr(typename TARGETING::AttributeTraits<A>::Type& o_attrValue) const;

    template <const TARGETING::ATTRIBUTE_ID A>
        std::string attrValueToString(const typename TARGETING::AttributeTraits<A>::Type& val);

  private:
    TargetPtr _parent{nullptr};
    std::string _name;
    int _offset{};
    const void* _fdt;
    std::vector<TargetPtr> _children;
};

template <const ATTRIBUTE_ID A>
std::string Target::attrValueToString(const typename AttributeTraits<A>::Type& val)
{
    using T = typename AttributeTraits<A>::Type;

    if constexpr (std::is_same_v<T, std::string>)
    {
        return val;
    }
    else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>)
    {
        return std::string(val);  // for char[]
    }
    else if constexpr (std::is_same_v<T, std::array<char, 64>>)
    {
        return std::string(val.data());
    }
    else if constexpr (std::is_integral_v<T>)
    {
        return std::to_string(val);
    }
    else if constexpr (std::is_same_v<T, EntityPath>)
    {
        char* cstr = val.toString();  // returns malloc'd C-string
        if (cstr)
        {
            std::string result(cstr);
            free(cstr);               // free the buffer after use
            return result;
        }
        return {};
    }
    else
    {
        // fallback: hex dump
        std::ostringstream oss;
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&val);
        for (size_t i = 0; i < sizeof(T); ++i)
        {
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(bytes[i]) << " ";
        }
        return oss.str();
    }
}

template<const ATTRIBUTE_ID A>
bool Target::tryGetAttr(typename AttributeTraits<A>::Type& o_attrValue) const
{
    const void* l_fdt = fdt();     // FDT blob
    int l_offset = getOffset();      // This target’s node offset

    auto nameOpt = tryGetAttrName<A>();  // Note: A is a template parameter
    if (!nameOpt)
    {
        return false;
    }

    const char* propName = nameOpt->data();  // Convert string_view to const char*

    int len = 0;
    const void* prop = fdt_getprop(l_fdt, l_offset, propName, &len);
    if (!prop || static_cast<size_t>(len) < sizeof(o_attrValue)) 
    {
        return false;
    }

    std::memcpy(&o_attrValue, prop, sizeof(o_attrValue));
    return true;
}
template<>
inline bool Target::tryGetAttr<ATTR_PHYS_PATH>(AttributeTraits<ATTR_PHYS_PATH>::Type& o_attrValue) const
{
    const void* l_fdt = fdt();    // FDT blob
    int l_offset = getOffset();     // Node offset

    auto nameOpt = tryGetAttrName<ATTR_PHYS_PATH>();  // Note: A is a template parameter
    if (!nameOpt)
    {
        return false;
    }
    const char* propName = nameOpt->data();  // Convert string_view to const char*
    
    int len = 0;
    const void* prop = fdt_getprop(l_fdt, l_offset, propName, &len);
    if (!prop || static_cast<size_t>(len) < sizeof(EntityPath)) 
    {
        return false;
    }

    // Use safe C++ assignment (not memcpy)
    o_attrValue = *reinterpret_cast<const EntityPath*>(prop);
    return true;
}

} // namespace TARGETING
