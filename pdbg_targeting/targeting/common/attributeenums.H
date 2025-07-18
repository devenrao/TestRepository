#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string_view>
#include <optional>
namespace TARGETING
{

enum ATTRIBUTE_ID
{
    ATTR_FAPI_NAME = 1,
    ATTR_PHYS_BIN_PATH,
    ATTR_CHIP_ID,
    ATTR_CLOCKSTOP_ON_XSTOP,
    ATTR_EC,
    ATTR_HWAS_STATE,
    ATTR_LOCATION_CODE,
    ATTR_NAME,
    ATTR_PHYS_DEV_PATH,
    ATTR_POS,
    ATTR_SPI_BUS_DIV_REF,
    ATTR_TYPE,
    ATTR_PHYS_PATH,
    ATTR_POSITION
};

/**
 *  @brief Enumeration indicating the target's type
 */
enum TYPE
{
    TYPE_NA                                                     = 0x00000000,
    TYPE_SYS                                                    = 0x00000001,
    TYPE_NODE                                                   = 0x00000002,
    TYPE_DIMM                                                   = 0x00000003,
    TYPE_MEMBUF                                                 = 0x00000004,
    TYPE_PROC                                                   = 0x00000005,
    TYPE_EX                                                     = 0x00000006,
    TYPE_CORE                                                   = 0x00000007,
    TYPE_L2                                                     = 0x00000008,
    TYPE_L3                                                     = 0x00000009,
    TYPE_L4                                                     = 0x0000000A,
    TYPE_MCS                                                    = 0x0000000B,
    TYPE_MBA                                                    = 0x0000000D,
    TYPE_XBUS                                                   = 0x0000000E,
    TYPE_ABUS                                                   = 0x0000000F,
    TYPE_PCI                                                    = 0x00000010,
    TYPE_DPSS                                                   = 0x00000011,
    TYPE_APSS                                                   = 0x00000012,
    TYPE_OCC                                                    = 0x00000013,
    TYPE_PSI                                                    = 0x00000014,
    TYPE_FSP                                                    = 0x00000015,
    TYPE_PNOR                                                   = 0x00000016,
    TYPE_OSC                                                    = 0x00000017,
    TYPE_TODCLK                                                 = 0x00000018,
    TYPE_CONTROL_NODE                                           = 0x00000019,
    TYPE_OSCREFCLK                                              = 0x0000001A,
    TYPE_OSCPCICLK                                              = 0x0000001B,
    TYPE_REFCLKENDPT                                            = 0x0000001C,
    TYPE_PCICLKENDPT                                            = 0x0000001D,
    TYPE_NX                                                     = 0x0000001E,
    TYPE_PORE                                                   = 0x0000001F,
    TYPE_PCIESWITCH                                             = 0x00000020,
    TYPE_CAPP                                                   = 0x00000021,
    TYPE_FSI                                                    = 0x00000022,
    TYPE_EQ                                                     = 0x00000023,
    TYPE_MCA                                                    = 0x00000024,
    TYPE_MCBIST                                                 = 0x00000025,
    TYPE_MI                                                     = 0x00000026,
    TYPE_DMI                                                    = 0x00000027,
    TYPE_OBUS                                                   = 0x00000028,
    TYPE_SBE                                                    = 0x0000002A,
    TYPE_PPE                                                    = 0x0000002B,
    TYPE_PERV                                                   = 0x0000002C,
    TYPE_PEC                                                    = 0x0000002D,
    TYPE_PHB                                                    = 0x0000002E,
    TYPE_SYSREFCLKENDPT                                         = 0x0000002F,
    TYPE_MFREFCLKENDPT                                          = 0x00000030,
    TYPE_TPM                                                    = 0x00000031,
    TYPE_SP                                                     = 0x00000032,
    TYPE_UART                                                   = 0x00000033,
    TYPE_PS                                                     = 0x00000034,
    TYPE_FAN                                                    = 0x00000035,
    TYPE_VRM                                                    = 0x00000036,
    TYPE_USB                                                    = 0x00000037,
    TYPE_ETH                                                    = 0x00000038,
    TYPE_PANEL                                                  = 0x00000039,
    TYPE_BMC                                                    = 0x0000003A,
    TYPE_FLASH                                                  = 0x0000003B,
    TYPE_SEEPROM                                                = 0x0000003C,
    TYPE_TMP                                                    = 0x0000003D,
    TYPE_GPIO_EXPANDER                                          = 0x0000003E,
    TYPE_POWER_SEQUENCER                                        = 0x0000003F,
    TYPE_RTC                                                    = 0x00000040,
    TYPE_FANCTLR                                                = 0x00000041,
    TYPE_OBUS_BRICK                                             = 0x00000042,
    TYPE_NPU                                                    = 0x00000043,
    TYPE_MC                                                     = 0x00000044,
    TYPE_TEST_FAIL                                              = 0x00000045,
    TYPE_MFREFCLK                                               = 0x00000046,
    TYPE_SMPGROUP                                               = 0x00000047,
    TYPE_OMI                                                    = 0x00000048,
    TYPE_MCC                                                    = 0x00000049,
    TYPE_OMIC                                                   = 0x0000004A,
    TYPE_OCMB_CHIP                                              = 0x0000004B,
    TYPE_MEM_PORT                                               = 0x0000004C,
    TYPE_I2C_MUX                                                = 0x0000004D,
    TYPE_PMIC                                                   = 0x0000004E,
    TYPE_NMMU                                                   = 0x0000004F,
    TYPE_PAU                                                    = 0x00000050,
    TYPE_IOHS                                                   = 0x00000051,
    TYPE_PAUC                                                   = 0x00000052,
    TYPE_FC                                                     = 0x00000053,
    TYPE_LPCREFCLKENDPT                                         = 0x00000054,
    TYPE_GENERIC_I2C_DEVICE                                     = 0x00000055,
    TYPE_MDS_CTLR                                               = 0x00000056,
    TYPE_DCM                                                    = 0x00000057,
    TYPE_POWER_IC                                               = 0x00000066,
    TYPE_TEMP_SENSOR                                            = 0x00000067,
    TYPE_LAST_IN_RANGE                                          = 0x00000068,
    TYPE_INVALID                                                = 0x0000007F,
};
/** 
 *  @brief Enumeration indicating the target's class
 */ 
enum CLASS
{   
    CLASS_NA                                                    = 0x00000000,
    CLASS_CARD                                                  = 0x00000001,
    CLASS_ENC                                                   = 0x00000002,
    CLASS_CHIP                                                  = 0x00000003,
    CLASS_UNIT                                                  = 0x00000004,
    CLASS_DEV                                                   = 0x00000005,
    CLASS_SYS                                                   = 0x00000006,
    CLASS_LOGICAL_CARD                                          = 0x00000007,
    CLASS_BATTERY                                               = 0x00000008,
    CLASS_LED                                                   = 0x00000009,
    CLASS_SP                                                    = 0x0000000A,
    CLASS_ASIC                                                  = 0x0000000B,
    CLASS_MAX                                                   = 0x0000000C,
    CLASS_INVALID                                               = 0xFFFFFFFF,
}; 
/** 
 *  @brief Enumeration indicating the target's engine type         
 */ 
enum ENGINE_TYPE
{   
    ENGINE_TYPE_NA                                              = 0x00000000,
    ENGINE_TYPE_ENGINE_IIC                                      = 0x00000001,
    ENGINE_TYPE_ENGINE_SCOM                                     = 0x00000002,
    ENGINE_TYPE_INVALID                                         = 0xFFFFFFFF,
}; 

template<ATTRIBUTE_ID A>
constexpr std::optional<std::string_view> tryGetAttrName()
{
    switch (A)
    {
        case ATTR_FAPI_NAME:         return "ATTR_FAPI_NAME";
        case ATTR_PHYS_BIN_PATH:     return "ATTR_PHYS_BIN_PATH";
        case ATTR_CHIP_ID:           return "ATTR_CHIP_ID";
        case ATTR_CLOCKSTOP_ON_XSTOP:return "ATTR_CLOCKSTOP_ON_XSTOP";
        case ATTR_EC:                return "ATTR_EC";
        case ATTR_HWAS_STATE:        return "ATTR_HWAS_STATE";
        case ATTR_LOCATION_CODE:     return "ATTR_LOCATION_CODE";
        case ATTR_NAME:              return "ATTR_NAME";
        case ATTR_PHYS_DEV_PATH:     return "ATTR_PHYS_DEV_PATH";
        case ATTR_POS:               return "ATTR_POS";
        case ATTR_SPI_BUS_DIV_REF:   return "ATTR_SPI_BUS_DIV_REF";
        case ATTR_TYPE:              return "ATTR_TYPE";
        case ATTR_PHYS_PATH:         return "ATTR_PHYS_PATH";
        case ATTR_POSITION:          return "ATTR_POSITION";
        default:                     return std::nullopt;
    }
}
inline std::optional<ATTRIBUTE_ID> tryGetAttrId(std::string_view name)
{
    if (name == "ATTR_FAPI_NAME")         return ATTR_FAPI_NAME;
    else if (name == "ATTR_PHYS_BIN_PATH") return ATTR_PHYS_BIN_PATH;
    else if (name == "ATTR_CHIP_ID")       return ATTR_CHIP_ID;
    else if (name == "ATTR_CLOCKSTOP_ON_XSTOP") return ATTR_CLOCKSTOP_ON_XSTOP;
    else if (name == "ATTR_EC")            return ATTR_EC;
    else if (name == "ATTR_HWAS_STATE")    return ATTR_HWAS_STATE;
    else if (name == "ATTR_LOCATION_CODE") return ATTR_LOCATION_CODE;
    else if (name == "ATTR_NAME")          return ATTR_NAME;
    else if (name == "ATTR_PHYS_DEV_PATH") return ATTR_PHYS_DEV_PATH;
    else if (name == "ATTR_POS")           return ATTR_POS;
    else if (name == "ATTR_SPI_BUS_DIV_REF") return ATTR_SPI_BUS_DIV_REF;
    else if (name == "ATTR_TYPE")          return ATTR_TYPE;
    else if (name == "ATTR_PHYS_PATH")     return ATTR_PHYS_PATH;
    else if (name == "ATTR_POSITION")      return ATTR_POSITION;
    else                              return std::nullopt;
}
} // namespace TARGETING

