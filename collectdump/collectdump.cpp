//This application uses test data writes to a file and invokes
//D-Bus method CreatePELWithFFDCFiles
//This application is used to validate logging/sbe_ffdc_handler code
#include <iostream>
#include <vector>
#include <string.h>
#include <attributes_info.H>
#include <libphal.H>
#include <cstring>
#include <sdbusplus/bus.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>
#include <fcntl.h>
#include <libphal.H>
#include <create_pel.hpp>
#include <dump_collect.hpp>
extern "C"
{
#include "libpdbg.h"
}
#include <libphal.H>

#define SBEFIFO_CMD_CLASS_DUMP              0xAA00
#define   SBEFIFO_CMD_GET_DUMP              0x01 /* long running */
using FFDCData = std::vector<std::pair<std::string, std::string>>;
constexpr uint64_t TARGET_TYPE_OCMB_CHIP = 0x28;
constexpr uint16_t ODYSSEY_CHIP_ID = 0x60C0;

using namespace openpower::phal;

void pdbgLogCallback(int, const char* fmt, va_list ap)
{
    va_list vap;
    va_copy(vap, ap);
    std::vector<char> logData(1 + std::vsnprintf(nullptr, 0, fmt, ap));
    std::vsnprintf(logData.data(), logData.size(), fmt, vap);
    va_end(vap);
    std::string logstr(logData.begin(), logData.end());
    std::cout << "PDBG:" << logstr << std::endl;
}

int main()
{
    constexpr auto devtree = "/var/lib/phosphor-software-manager/pnor/rw/DEVTREE";

    // PDBG_DTB environment variable set to CEC device tree path
    if (setenv("PDBG_DTB", devtree, 1))
    {
        std::cerr << "Failed to set PDBG_DTB: " << strerror(errno) << std::endl;
        return 0;
    }

    constexpr auto PDATA_INFODB_PATH = "/usr/share/pdata/attributes_info.db";
    // PDATA_INFODB environment variable set to attributes tool  infodb path
    if (setenv("PDATA_INFODB", PDATA_INFODB_PATH, 1))
    {
        std::cerr << "Failed to set PDATA_INFODB: ({})" << strerror(errno) << std::endl;
        return 0;
    }

    //initialize the targeting system
    if (!pdbg_targets_init(NULL))
    {
        std::cerr << "pdbg_targets_init failed" << std::endl;
        return 0;
    }

    auto bus = sdbusplus::bus::new_default();

    // set log level and callback function
    pdbg_set_loglevel(PDBG_DEBUG);
    pdbg_set_logfunc(pdbgLogCallback);

    struct pdbg_target *ocmb;
    pdbg_for_each_target("ocmb", NULL, ocmb)
    {
	    std::cout << "multiffdc now calling is_ody_ocmb_chip " << std::endl;
	    if(is_ody_ocmb_chip(ocmb))
	    {
			pdbg_target_probe(ocmb);
			if (pdbg_target_status(ocmb) != PDBG_TARGET_ENABLED)
			{
				std::cout << "ocmb chip not enabled " << std::endl;
				return -1;
			}
			try
			{
                uint32_t addr = 0xc0002040;
                uint64_t origval = 0;
                ocmb_getscom(ocmb, addr, &origval);
                uint32_t fapipos = 0;
                pdbg_target_get_attribute(ocmb, "ATTR_FAPI_POS", 4, 1, &fapipos);
                openpower::dump::sbe_chipop::collectDumpFromSBE(ocmb, "/tmp", 0x1, 0x1, 0x1, fapipos);
            }
            catch(const std::exception& ex)
            {
                std::cout << "Exception raise when creating PEL " << ex.what() << std::endl;
            }
			break;
	    }
    }

    sleep(20);
    return 0;
}

