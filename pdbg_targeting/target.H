#pragma once

#if __cplusplus >= 202302L
#include <generator>
#endif
#include <attributeenums.H>
#include <attributetraits.H>

#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
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
    [[nodiscard]] const void* getFDT() const noexcept
    {
        return _fdt;
    }
#if __cplusplus >= 202302L
    [[nodiscard]] std::generator<TargetPtr> ancestors() const;
#endif
    template <const TARGETING::ATTRIBUTE_ID A>
    bool tryGetAttr(
        typename TARGETING::AttributeTraits<A>::Type& o_attrValue) const;

    template <const TARGETING::ATTRIBUTE_ID A>
    std::string attrValueToString(
        const typename TARGETING::AttributeTraits<A>::Type& val);

  private:
    TargetPtr _parent{nullptr};
    std::string _name;
    int _offset{};
    const void* _fdt;
    std::vector<TargetPtr> _children;
};

namespace // local use only
{
template <typename T>
bool tryGetAttrHelper(const void* fdt, int offset, std::string_view propName,
                      T& outVal)
{
    int len = 0;
    std::cout << "tryGetAttrHelper prop " << propName << std::endl;
    const void* prop = fdt_getprop(fdt, offset, propName.data(), &len);
    if (!prop || static_cast<size_t>(len) < sizeof(T))
    {
        std::cout << "false tryGetAttrHelper prop " << propName << " len "
                  << static_cast<size_t>(len) << " sizeof(T) " << sizeof(T)
                  << std::endl;
        return false;
    }

    std::memcpy(&outVal, prop, sizeof(T));
    return true;
}

template <>
inline bool tryGetAttrHelper<EntityPath>(
    const void* fdt, int offset, std::string_view propName, EntityPath& outVal)
{
    int len = 0;
    const void* prop = fdt_getprop(fdt, offset, propName.data(), &len);
    if (!prop || static_cast<size_t>(len) < sizeof(EntityPath))
    {
        return false;
    }

    outVal = *reinterpret_cast<const EntityPath*>(prop);
    return true;
}
} // namespace
template <const ATTRIBUTE_ID A>
bool Target::tryGetAttr(typename AttributeTraits<A>::Type& o_attrValue) const
{
    const void* fdt = getFDT(); // FDT blob
    int offset = getOffset();   // Node offset

    auto nameOpt = tryGetAttrName<A>();
    if (!nameOpt)
    {
        return false;
    }

    return TARGETING::tryGetAttrHelper(fdt, offset, *nameOpt, o_attrValue);
}

} // namespace TARGETING
