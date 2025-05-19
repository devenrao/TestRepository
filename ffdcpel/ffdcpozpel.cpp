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
namespace sdbus = sdbusplus::common::xyz::openbmc_project::logging;
inline sdbus::Create::FFDCFormat toFFDCFormat(errl::FFDCFormat format)
{
    using FFDCFormat = sdbus::Create::FFDCFormat;

    switch (format)
    {
        case errl::FFDCFormat::JSON:
            return FFDCFormat::JSON;
        case errl::FFDCFormat::CBOR:
            return FFDCFormat::CBOR;
        case errl::FFDCFormat::Text:
            return FFDCFormat::Text;
        case errl::FFDCFormat::Custom:
            return FFDCFormat::Custom;
        default:
            throw std::invalid_argument("Invalid UserDataFormat enum value");
    }
}
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

    // pdbg_target_probe_all(pdbg_target_root());
    std::cout << "ffdcpel before looking for proc target " << std::endl;
    struct pdbg_target* proc;
    using Level =
        sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;
    pdbg_for_each_target("proc", NULL, proc)
    {
        pdbg_target_probe(proc);
        if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
        {
            std::cout << "proc chip not enabled " << std::endl;
            return -1;
        }
        try
        {
            std::cout << "ffdcpel before calling poz_sbe_chipop_failure"
                      << std::endl;
            auto errlHandle = errl::factory::createPozSbeChipOpFailure(proc);
            if (errlHandle && *errlHandle)
            {
                const auto& entries =
                    (*errlHandle)
                        ->getEntries(); // or wherever you call getEntries()
                std::cout << "ffdcpel createSbeBootFailure entries size "
                          << entries.size() << std::endl;
                for (const auto& entryPtr : entries)
                {
                    if (entryPtr) // Always good to check for non-null
                    {
                        const auto& msg = entryPtr->getMessage();
                        const auto& data = entryPtr->getAdditionalData();
                        const auto& filesOpt = entryPtr->getFfdcFiles();
                        std::cout << "ffdcpel createSbeBootFailure msg " << msg
                                  << std::endl;

                        FFDCInfo ffdcInfo;
                        if (filesOpt)
                        {
                            for (const auto& pair : *filesOpt)
                            {
                                const auto& pelFfdc = pair.first;
                                // const CreateFFDCFormat format =
                                // sdbusplus::xyz::openbmc_project::Logging::
                                //         server::Create::FFDCFormat::Custom;
                                ffdcInfo.emplace_back(std::make_tuple(
                                    toFFDCFormat(pelFfdc.format),
                                    pelFfdc.subType, pelFfdc.version,
                                    pelFfdc.fd));

                                std::cout
                                    << "**ffdcpozpel createSbeBootFailure "
                                       "pelFfdc.fd "
                                    << pelFfdc.fd << " subtpe "
                                    << static_cast<uint16_t>(pelFfdc.subType)
                                    << " format "
                                    << static_cast<uint16_t>(pelFfdc.format)
                                    << "pelFfdc.version"
                                    << static_cast<uint16_t>(pelFfdc.version)
                                    << std::endl;
                            }
                        }
                        std::cout << "Calling CreatePELWithFFDCFiles"
                                  << std::endl;
                        auto level = sdbusplus::xyz::openbmc_project::Logging::
                            server::convertForMessage(Level::Informational);
                        auto service = getService(bus, opLoggingInterface,
                                                  loggingObjectPath);
                        std::cout << "service name is " << service.c_str()
                                  << std::endl;
                        auto method = bus.new_method_call(
                            service.c_str(), loggingObjectPath,
                            opLoggingInterface, "CreatePELWithFFDCFiles");
                        std::cout << "calling bus.call method " << std::endl;
                        method.append(msg, level, data, ffdcInfo);
                        auto response = bus.call(method);
                        // reply will be tuple containing bmc log id, platform
                        // log id
                        std::tuple<uint32_t, uint32_t> reply = {0, 0};

                        // parse dbus response into reply
                        response.read(reply);
                        int plid = std::get<1>(
                            reply); // platform log id is tuple "second"
                        std::cout << "ffdcpel createSbeBootFailure plid "
                                  << plid << std::endl;
                    }
                }
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
