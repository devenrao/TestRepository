#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <optional>
#include "attributeenums.H"
#include "attributetraits.H"
#include "target.H"  // where TargetPtr and tryGetAttr live

namespace TARGETING {

using AttrPrinter = std::function<std::optional<std::string>(TargetPtr)>;

#define SUPPORTED_ATTRS         \
    X(ATTR_FAPI_NAME)           \
    X(ATTR_PHYS_DEV_PATH)       \
    X(ATTR_CHIP_ID)             \
    X(ATTR_CLOCKSTOP_ON_XSTOP)  \
    X(ATTR_EC)                  \
    X(ATTR_HWAS_STATE)          \
    X(ATTR_LOCATION_CODE)       \
    X(ATTR_NAME)                \
    X(ATTR_PHYS_BIN_PATH)       \
    X(ATTR_POS)                 \
    X(ATTR_SPI_BUS_DIV_REF)     \
    X(ATTR_TYPE)                \
    X(ATTR_PHYS_PATH)

inline const std::unordered_map<ATTRIBUTE_ID, AttrPrinter>& getAttrPrinters()
{
    static const std::unordered_map<ATTRIBUTE_ID, AttrPrinter> map = {
#define X(attr) \
        { ATTRIBUTE_ID::attr, [](TargetPtr t) -> std::optional<std::string> { \
              typename AttributeTraits<ATTRIBUTE_ID::attr>::Type val{}; \
              if (t->tryGetAttr<ATTRIBUTE_ID::attr>(val)) { \
                  return t->attrValueToString<ATTRIBUTE_ID::attr>(val); \
              } \
              return std::nullopt; \
        }},
        SUPPORTED_ATTRS
#undef X
    };
    return map;
}
} // namespace TARGETING

