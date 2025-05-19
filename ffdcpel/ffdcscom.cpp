// This application uses test data writes to a file and invokes
// D-Bus method CreatePELWithFFDCFiles
// This application is used to validate logging/sbe_ffdc_handler code
#include <attributes_info.H>
#include <errl_factory.H>
#include <fcntl.h>
#include <libphal.H>
#include <string.h>

#include <cstring>
#include <iostream>
#include <phosphor-logging/elog.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>
#include <tuple>
#include <vector>
#include <xyz/openbmc_project/Logging/Create/server.hpp>
#include <xyz/openbmc_project/Logging/Entry/server.hpp>

extern "C" {
#include "libpdbg.h"
}
#include <sbe_intf.H>
using namespace phosphor::logging;
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
using FFDCInfo = std::vector<std::tuple<
    sdbusplus::xyz::openbmc_project::Logging::server::Create::FFDCFormat,
    uint8_t, uint8_t, sdbusplus::message::unix_fd>>;

constexpr auto loggingObjectPath = "/xyz/openbmc_project/logging";
constexpr auto loggingInterface = "xyz.openbmc_project.Logging.Create";
constexpr auto opLoggingInterface = "org.open_power.Logging.PEL";

std::string getService(sdbusplus::bus::bus& bus, const std::string& intf,
                       const std::string& path)
{
    constexpr auto MAPPER_BUSNAME = "xyz.openbmc_project.ObjectMapper";
    constexpr auto MAPPER_PATH = "/xyz/openbmc_project/object_mapper";
    constexpr auto MAPPER_INTERFACE = "xyz.openbmc_project.ObjectMapper";
    try
    {
        auto mapper = bus.new_method_call(MAPPER_BUSNAME, MAPPER_PATH,
                                          MAPPER_INTERFACE, "GetObject");

        mapper.append(path, std::vector<std::string>({intf}));

        auto mapperResponseMsg = bus.call(mapper);
        std::map<std::string, std::vector<std::string>> mapperResponse;
        mapperResponseMsg.read(mapperResponse);

        if (mapperResponse.empty())
        {
            log<level::ERR>(std::format("Empty mapper response for GetObject "
                                        "interface({}), path({})",
                                        intf, path)
                                .c_str());
            throw std::runtime_error("Empty mapper response for GetObject");
        }
        std::cout << "service name in getService method is "
                  << mapperResponse.begin()->first << std::endl;
        return mapperResponse.begin()->first;
    }
    catch (const sdbusplus::exception::exception& ex)
    {
        log<level::ERR>(std::format("Mapper call failed for GetObject "
                                    "errorMsg({}), path({}), interface({}) ",
                                    ex.what(), path, intf)
                            .c_str());
        throw;
    }
}
int main()
{
    constexpr auto devtree =
        "/var/lib/phosphor-software-manager/pnor/rw/DEVTREE";

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
        std::cerr << "Failed to set PDATA_INFODB: ({})" << strerror(errno)
                  << std::endl;
        return 0;
    }
    pdbg_set_backend(PDBG_BACKEND_SBEFIFO, NULL);

    // initialize the targeting system
    if (!pdbg_targets_init(NULL))
    {
        std::cerr << "pdbg_targets_init failed" << std::endl;
        return 0;
    }

    auto bus = sdbusplus::bus::new_default();

    // set log level and callback function
    pdbg_set_loglevel(PDBG_DEBUG);
    pdbg_set_logfunc(pdbgLogCallback);

    pdbg_target_probe_all(pdbg_target_root());
    std::cout << "ffdcpel before looking for proc target " << std::endl;
    struct pdbg_target* proc;
    pdbg_for_each_target("proc", NULL, proc)
    {
        if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
        {
            std::cout << "ocmb chip not enabled " << std::endl;
            return -1;
        }
        try
        {
            {
                FFDCMapOpt ffdc;
                fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP> ptarget =
                    fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP>(proc);

                auto ret = sbei::getScom(ptarget, 0x1, 0x10012, data, primary,
                                         secondary, &ffdc);
                uint64_t value = std::bit_cast<uint64_t>(data);
                std::cout << std::hex << "0x" << value << std::endl;
                std::cout << "Primary Status  : " << primary << std::endl;
                std::cout << "Secondary Status: " << secondary << std::endl;
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << "Exception raise when creating PEL " << ex.what()
                      << std::endl;
        }
        break;
    }

    std::cout << "came to end of ffdcpel" << std::endl;
    sleep(10);
    return 0;
}
