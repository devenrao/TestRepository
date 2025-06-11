/**
 *  @file targeting/common/entitypath.C
 *
 *  @brief Implementation of the EntityPath class which provides a hierarchical
 *      path structure for identification of targets
 */

//******************************************************************************
// Includes
//******************************************************************************

// STD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <cassert>

// This component
#include <attributeenums.H>
#include <entitypath.H>

namespace TARGETING
{

//******************************************************************************
// EntityPath::EntityPath (Path Type Constructor)
//******************************************************************************

EntityPath::EntityPath(
    const PATH_TYPE i_pathType)
    : iv_type(i_pathType), iv_size(0)
{
    memset(&iv_pathElement[0], 0x00, sizeof(iv_pathElement));
}

//******************************************************************************
// EntityPath::EntityPath (Full Constructor)
//******************************************************************************

EntityPath::EntityPath()
    : iv_type(PATH_NA), iv_size(0)
{
    memset(&iv_pathElement[0], 0x00, sizeof(iv_pathElement));
}

//******************************************************************************
// EntityPath::removeLast
//******************************************************************************

EntityPath& EntityPath::removeLast()
{
    assert(size() >= 1);

    iv_pathElement[size() - 1].type = TYPE_NA;
    iv_pathElement[size() - 1].instance = 0;
    --iv_size;
    return *this;
}

//******************************************************************************
// EntityPath::copyRemoveLast
//******************************************************************************

EntityPath EntityPath::copyRemoveLast() const
{
    EntityPath l_newPath = *this;
    l_newPath.removeLast();
    return l_newPath;
}

//******************************************************************************
// EntityPath::addLast
//******************************************************************************

EntityPath& EntityPath::addLast(
    const TYPE    i_type,
    const uint8_t i_instance)
{
    assert(size() < MAX_PATH_ELEMENTS);

    iv_pathElement[size()].type = i_type;
    iv_pathElement[size()].instance = i_instance;
    ++iv_size;
    return *this;
}

//******************************************************************************
// EntityPath::copyAddLast
//******************************************************************************

EntityPath EntityPath::copyAddLast(
    const TYPE    i_type,
    const uint8_t i_instance) const
{
    EntityPath l_newPath = *this;
    l_newPath.addLast(i_type,i_instance);
    return l_newPath;
}

//******************************************************************************
// EntityPath::operator==
//******************************************************************************

bool EntityPath::operator==(
    const EntityPath &i_rhs) const
{
    return (   (i_rhs.iv_type == iv_type)
            && (i_rhs.iv_size == iv_size)
            && (memcmp(&iv_pathElement[0],
                       &i_rhs.iv_pathElement[0],
                       (sizeof(iv_pathElement[0])*iv_size)) == 0));
}

//******************************************************************************
// EntityPath::operator<
//******************************************************************************

bool EntityPath::operator<(const EntityPath &i_rhs) const
{
    if (iv_type != i_rhs.iv_type)
    {
        return iv_type < i_rhs.iv_type;
    }

    size_t size = std::min(iv_size,i_rhs.iv_size)*sizeof(PathElement);
    int result = memcmp(&iv_pathElement[0], &i_rhs.iv_pathElement[0], size);

    // lhs == rhs
    if ( result == 0 )
    {
        return iv_size < i_rhs.iv_size;
    }
    else
    {
        return ( result < 0 );
    }
}


//******************************************************************************
// EntityPath::equals
//******************************************************************************

bool EntityPath::equals(
    const EntityPath& i_rhs,
    const uint32_t    i_size) const
{
    assert(i_size <= MAX_PATH_ELEMENTS);

    return (   (i_rhs.iv_type == iv_type)
            && (i_size <= i_rhs.size())
            && (i_size <= size())
            && (memcmp(&iv_pathElement[0],
                       &i_rhs.iv_pathElement[0],
                       (sizeof(iv_pathElement[0])*i_size)) == 0));
}

//******************************************************************************
// EntityPath::operator[]
//******************************************************************************

const EntityPath::PathElement& EntityPath::operator[](
    const uint32_t i_index) const
{
    assert(i_index < size());
    return iv_pathElement[i_index];
}

//******************************************************************************
// EntityPath::replace
//******************************************************************************

void EntityPath::replace(
    const uint32_t i_index,
    PathElement i_element)
{
    assert(i_index < size());
    iv_pathElement[i_index] = i_element;
}

//******************************************************************************
// EntityPath::pathElementOfType
//******************************************************************************

const EntityPath::PathElement EntityPath::pathElementOfType(
    const TYPE i_type) const
{
    for( uint32_t x = 0; x < iv_size; x++ )
    {
        if( i_type == iv_pathElement[x].type )
        {
            return iv_pathElement[x];
        }
    }

    PathElement na_path = { TYPE_NA, 0 };
    return na_path;
}

//******************************************************************************
// EntityPath::setType
//******************************************************************************

void EntityPath::setType(
    const PATH_TYPE i_pathType)
{
    iv_type = i_pathType;
}

//******************************************************************************
// EntityPath::pathTypeAsString (DEBUG)
//******************************************************************************

const char* EntityPath::pathTypeAsString() const
{
    switch (iv_type)
    {
        case PATH_DEVICE:
            return "Device";
        case PATH_AFFINITY:
            return "Logical";
        case PATH_PHYSICAL:
            return "Physical";
        case PATH_POWER:
            return "Power";
        default:
            return "Unknown entity path type";
    }
}

//******************************************************************************
// EntityPath::pathElementTypeAsString (DEBUG)
//******************************************************************************

const char* EntityPath::pathElementTypeAsString(
    const TYPE i_type) const
{
#if 0
    switch (i_type)
    {
        case TYPE_NA:
            return "NA";
        case TYPE_SYS:
            return "Sys";
        case TYPE_NODE:
            return "Node";
        case TYPE_DIMM:
            return "DIMM";
        case TYPE_MEMBUF:
            return "Membuf";
        case TYPE_PROC:
            return "Proc";
        case TYPE_FC:
            return "FC";
        case TYPE_EX:
            return "EX";
        case TYPE_CORE:
            return "Core";
        case TYPE_L2:
            return "L2";
        case TYPE_L3:
            return "L3";
        case TYPE_L4:
            return "L4";
        case TYPE_MCS:
            return "MCS";
        case TYPE_MBA:
            return "MBA";
        case TYPE_XBUS:
            return "XBUS";
        case TYPE_ABUS:
            return "ABUS";
        case TYPE_PCI:
            return "PCI";
        case TYPE_DPSS:
            return "DPSS";
        case TYPE_APSS:
            return "APSS";
        case TYPE_OCC:
            return "OCC";
        case TYPE_PSI:
            return "PSI";
        case TYPE_FSP:
            return "FSP";
        case TYPE_PNOR:
            return "PNOR";
        case TYPE_OSC:
            return "OSC";
        case TYPE_MFREFCLK:
            return "MFREFClock";
        case TYPE_TODCLK:
            return "TodClock";
        case TYPE_CONTROL_NODE:
            return "Control Node";
        case TYPE_NX:
            return "NX";
        case TYPE_PORE:
            return "PORE";
        case TYPE_OSCREFCLK:
            return "OSCREFClock";
        case TYPE_OSCPCICLK:
            return "OSCPCIClock";
        case TYPE_REFCLKENDPT:
            return "REFClockEndPoint";
        case TYPE_PCICLKENDPT:
            return "PCIClockEndPoint";
        case TYPE_PCIESWITCH:
             return "PCIESWITCH";
        case TYPE_CAPP:
            return "CAPP";
        case TYPE_FSI:
            return "FSI";
        case TYPE_EQ:
            return "EQ";
        case TYPE_MCA:
            return "MCA";
        case TYPE_MCBIST:
            return "MCBIST";
        case TYPE_MC:
            return "MC";
        case TYPE_MI:
            return "MI";
        case TYPE_DMI:
            return "DMI";
        case TYPE_OBUS:
            return "OBUS";
        case TYPE_OBUS_BRICK:
            return "OBUS_BRICK";
        case TYPE_NPU:
            return "NPU";
        case TYPE_SBE:
            return "SBE";
        case TYPE_PPE:
            return "PPE";
        case TYPE_PERV:
            return "PERV";
        case TYPE_PEC:
            return "PEC";
        case TYPE_PHB:
            return "PHB";
        case TYPE_SYSREFCLKENDPT:
            return "SYSREFCLKENDPT";
        case TYPE_MFREFCLKENDPT:
            return "MFREFCLKENDPT";
        case TYPE_TPM:
            return "TPM";
        case TYPE_SP:
            return "SP";
        case TYPE_UART:
            return "UART";
        case TYPE_PS:
            return "PS";
        case TYPE_FAN:
            return "FAN";
        case TYPE_VRM:
            return "VRM";
        case TYPE_USB:
            return "USB";
        case TYPE_ETH:
            return "ETH";
        case TYPE_PANEL:
            return "PANEL";
        case TYPE_BMC:
            return "BMC";
        case TYPE_FLASH:
            return "FLASH";
        case TYPE_SEEPROM:
            return "SEEPROM";
        case TYPE_TMP:
            return "TMP";
        case TYPE_GPIO_EXPANDER:
            return "GPIO_EXPANDER";
        case TYPE_POWER_SEQUENCER:
            return "POWER_SEQUENCER";
        case TYPE_RTC:
            return "RTC";
        case TYPE_FANCTLR:
            return "FANCTLR";
        case TYPE_SMPGROUP:
            return "SMPGROUP";
        case TYPE_OMI:
            return "OMI";
        case TYPE_OMIC:
            return "OMIC";
        case TYPE_MCC:
            return "MCC";
        case TYPE_OCMB_CHIP:
            return "OCMB_CHIP";
        case TYPE_MEM_PORT:
            return "MEM_PORT";
        case TYPE_I2C_MUX:
            return "I2C_MUX";
        case TYPE_PMIC:
            return "PMIC";
        case TYPE_NMMU:
            return "NMMU";
        case TYPE_PAU:
            return "PAU";
        case TYPE_IOHS:
            return "IOHS";
        case TYPE_PAUC:
            return "PAUC";
        case TYPE_LPCREFCLKENDPT:
            return "LPCREFCLKENDPT";
        case TYPE_GENERIC_I2C_DEVICE:
            return "GENERIC_I2C_DEVICE";
        case TYPE_MDS_CTLR:
            return "MDS_CTLR";
        case TYPE_DCM:
            return "DCM";
        case TYPE_TEMP_SENSOR:
            return "TEMP_SENSOR";
        case TYPE_POWER_IC:
            return "POWER_IC";
        default:
            return "Unknown path type";
    }
#endif //TODO    
}

//******************************************************************************
// EntityPath::toString
//******************************************************************************

char * EntityPath::toString() const
{
    void* l_ptr = NULL;
    char* l_pString = NULL;
    const char* l_ptr1 = NULL;
    size_t l_len1, l_len2;

    l_ptr1 = pathTypeAsString();
    l_len1 = strlen( l_ptr1 ) + 1;  // add 1 for the ':' char
    // allocate extra 1 bytes for the nul char
    l_pString = static_cast<char*>( malloc( l_len1 + 1 ) );
    sprintf( l_pString, "%s:", l_ptr1 );

    for (uint32_t i=0; i < size(); ++i)
    {
        l_ptr1 = pathElementTypeAsString( operator[](i).type );
        l_len2 = strlen( l_ptr1 ) + 1;  // add 1 for '/' char
        // realloc with extra 33 bytes (more than enough)
        // for the %d conversion and the nul char.
        l_ptr = realloc( l_pString, (l_len1 + l_len2 + 33) );
        l_pString = static_cast<char*>( l_ptr );
        // append at the nul char of previous string
        l_len2 = sprintf( l_pString + l_len1, "/%s%d",
                          l_ptr1, operator[](i).instance );
        l_len1 += l_len2;
    }

    return (l_pString);
}
} // End namespace TARGETING
