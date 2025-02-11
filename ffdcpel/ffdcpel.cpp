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

extern "C"
{
#include "libpdbg.h"
}
#include <libphal.H>


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

    std::cout << "ffdcpel before looking for proc target " << std::endl;
    struct pdbg_target *proc;
    pdbg_for_each_target("proc", NULL, proc)
    {
		pdbg_target_probe(proc);
		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
		{
			std::cout << "ocmb chip not enabled " << std::endl;
			return -1;
		}
            auto bus = sdbusplus::bus::new_default();
		try
		{
		    std::cout << "ffdcpel before calling createSbeBootFailure " << std::endl;
		    auto errlHandle = errl::factory::createSbeBootFailure();
		    if (errlHandle)
		    {
                const auto& entries = errlHandle.getEntries();  // or wherever you call getEntries()
		        std::cout << "ffdcpel createSbeBootFailure entries size " << entries.size() << std::endl;
                for (const auto& entryPtr : entries)
                {
                    if (entryPtr)  // Always good to check for non-null
                    {
                        const auto& msg = entryPtr->getMessage();
                        auto severity = entryPtr->getSeverity();
                        const auto& data = entryPtr->getAdditionalData();
		                std::cout << "ffdcpel createSbeBootFailure msg " << msg << std::endl;

                        std::unordered_map<std::string, std::string> additionalData;
                        additionalData.emplace("_PID", std::to_string(getpid()));
                        additionalData.emplace("SBE_ERR_MSG", errMsg);
                        for (auto& it : data)
                        {
                            additionalData.emplace(it);
                        }

                        FFDCInfo ffdcInfo;
                        for (const auto& [pelFfdc, std::ignore] : *files)
                        {
                            ffdcInfo.emplace_back(
                                static_cast<sdbusplus::xyz::openbmc_project::Logging::server::Create::FFDCFormat>(pelFfdc.format),
                                pelFfdc.subType,
                                pelFfdc.version,
                                sdbusplus::message::unix_fd{pelFfdc.fd});
		                    std::cout << "ffdcpel createSbeBootFailure pelFfdc.fd 0x{:08x}" << pelFfdc.fd << std::endl;
                        }
                        auto service = getService(bus, opLoggingInterface,
                                                    loggingObjectPath);
                        auto method = bus.new_method_call(service.c_str(), loggingObjectPath,
                                                      opLoggingInterface,
                                                      "CreatePELWithFFDCFiles");
                        method.append(msg, severity, additionalData, ffdcInfo);
                        auto response = bus.call(method);
                        // reply will be tuple containing bmc log id, platform log id
                        std::tuple<uint32_t, uint32_t> reply = {0, 0};

                        // parse dbus response into reply
                        response.read(reply);
                        plid = std::get<1>(reply); // platform log id is tuple "second"
                        std::cout << "ffdcpel createSbeBootFailure plid 0x{:08x}" << plid << std::endl;
                    }
                }		    
            }
        }
        catch(const std::exception& ex)
        {
            std::cout << "Exception raise when creating PEL " << ex.what() << std::endl;
        }
		break;
    }

    sleep(10);
    return 0;
}

