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

extern "C"
{
#include "libpdbg.h"
}

//000c0af0    
//0x0002,    0x0100,
//Feb 15 10:38:29 rain71bmc phosphor-log-manager[2207]: POZFFDC fapiRc 0x02000001
//0xb200,    0xa8a0,
//0x009f,    0xb509,
//Feb 15 07:55:12 ever6bmc phosphor-log-manager[10933]: libekb - fapiRC recieved 0x9f0009b5
//RC_ERROR_UNSUPPORTED_BY_SBE, 0xe0f4f0
//RC_EXTRACT_SBE_RC_OTP_PIB_ERR, 0x9fb509
//0x9f00,    0x09b5,
//libekb - fapiRC recieved 0x9fb509

std::vector<uint16_t> testFFDC1 = { 0xadfb, 0x1800, 0x0000, 0x01c1, 0x0100, 0x0040, 0x0c00, 0xf00a,
        0x0000, 0x0100, 0x0100, 0x4800, 0x0000, 0xfeff, 0x001c, 0x0000,
        0x0000, 0x0000, 0x0000, 0xfeff, 0x0001, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0100, 0x0000, 0x0000, 0x0000, 0xde00, 0x0000, 0x0200,
        0x0000, 0x0000, 0x0000, 0xadde, 0x0000, 0x0400, 0x0000, 0x0000 };  
        
std::vector<uint16_t> testFFDC2 = { 0xadfb,    0x9700,    0x0000,    0x01c1,    0x0100,    0x0040,    0x0002,    0x0100,
                                    0xfe00,    0x2c00,    0x3d90,    0x8bc4,    0x0003,    0x0000,    0x0000,    0x0400,
                                    0x0400,    0x3802,    0x0200,    0x0000,    0x4253,    0x5f45,    0x5254,    0x4341,
                                    0x0045,    0x0000,    0x0000,    0x0000,    0x0000,    0x0f30,    0x1338,    0x0002,
                                    0x23fe,    0xaf29,    0xd717,    0x0084,    0x0000,    0x0000,    0xffff,    0xffff,
                                    0x62fa,    0x2809,    0x0000,    0x0000,    0x0000,    0x0002,    0x0000,    0x0200,
                                    0x0000,    0x4f00,    0x0000,    0x0701,    0x0000,    0x0000,    0x9066,    0x0301,
                                    0xf805,    0x7efe,    0x0000,    0x0100,    0x0000,    0x0000,    0x0000,    0x0100,
                                    0x0000,    0x0000,    0x7c66,    0x0301,    0xf905,    0x0e09,    0xe8c6,    0x0400,
                                    0xf905,    0x7d09,    0x0000,    0x0400,    0x0000,    0x5601,    0x0000,    0x0600,
                                    0x0000,    0x0000,    0x9066,    0x0301,    0xf905,    0xc609,    0x0000,    0x0100,
                                    0x0000,    0x0100,    0x0000,    0x0100,    0x0000,    0x0000,    0x7c66,    0x0301,
                                    0xf905,    0xf20a,    0xe8c6,    0x0400,    0xf905,    0x610b,    0x0000,    0x0400,
                                    0x0000,    0x5601,    0x0000,    0x0600,    0x0000,    0x0000,    0x9066,    0x0301,
                                    0xf905,    0xaa0b,    0x0000,    0x0100,    0x0000,    0x0200,    0x0000,    0x0100,
                                    0x0000,    0x0000,    0x7c66,    0x0301,    0xf905,    0xd60c,    0xe8c6,    0x0400,
                                    0xf905,    0x450d,    0x0000,    0x0400,    0x0000,    0x5601,    0x0000,    0x0600,
                                    0x0000,    0x0000,    0x9066,    0x0301,    0xf905,    0x8e0d,    0x0000,    0x0100,
                                    0x0000,    0x0300,    0x0000,    0x0100,    0x0000,    0x0000,    0x7c66,    0x0301,
                                    0xf905,    0xba0e,    0xe8c6,    0x0400,    0xf905,    0x290f,    0x0000,    0x0400,
                                    0x0000,    0x5601,    0x0000,    0x0600,    0x0000,    0x0000,    0x9066,    0x0301,
                                    0xf905,    0x720f,    0x0000,    0x0800,    0x0000,    0x0000,    0x4545,    0x0101,
                                    0xf905,    0x8e10,    0xa304,    0x0000,    0xf905,    0xd510,    0xfa5f,    0x0000,
                                    0xf905,    0x9913,    0x0000,    0x0000,    0x0000,    0x0000,    0x8d2c,    0x0101,
                                    0xf905,    0x7a14,    0xd65f,    0x0000,    0xf905,    0x2515,    0x39a6,    0x0600,
                                    0xfa05,    0xf94f,    0x0000,    0x7ada,    0x0000,    0x0000,    0xa4d0,    0x0201,
                                    0xfa05,    0x5250,    0x0000,    0x0000,    0x0000,    0x0100,    0x9cc4,    0x0201,
                                    0xfa05,    0xba50,    0xa091,    0x0600,    0xfa05,    0x0d51,    0x20c9,    0x0000,
                                    0xfa05,    0xb551,    0x0000,    0x0100,    0x0000,    0x0000,    0x80eb,    0x0201,
                                    0xfa05,    0xf651,    0x0000,    0xc100,    0x0000,    0x0100,    0x771f,    0x0201,
                                    0xfa05,    0xbe52,    0x0000,    0xc100,    0x0000,    0x0100,    0xf1f0,    0x0201,
                                    0xfa05,    0x1653,    0x0000,    0x0200,    0x0000,    0x0500,    0x8b34,    0x0201,
                                    0xfa05,    0xf253,    0x0000,    0x0000,    0x0000,    0x0000,    0x9bba,    0x0101,
                                    0xfa05,    0x2655,    0x0000,    0x8000,    0xfeff,    0xa07b,    0xffff,    0xa88d,
                                    0xffff,    0x208d,    0xfd84,    0x0401,    0xfa05,    0x9656,    0xb19f,    0x0000,
                                    0x1206,    0xe9ca,    0x0c00,    0xf00a,    0x0000,    0x0000,    0x3514,    0x0101,
                                    0x1206,    0x02cf,    0x0000,    0xd002,    0xfeff,    0xa07b,    0xffff,    0x208d,
                                    0xffff,    0x488a,    0xfd84,    0x0401,    0x1206,    0x96cf,    0xadfb,    0x1800,
                                    0x0000,    0x01c1,    0x0200,    0x0002,    0x9f00,    0x09b5,    0x0000,    0x0100,
                                    0x0100,    0x4800,    0x0000,    0xfeff,    0x001c,    0x0000,    0x0000,    0x0000,
                                    0x0000,    0xfeff,    0x0001,    0x0000,    0x0000,    0x0200,    0x0000,    0x0100,
                                    0x0000,    0x0000,    0x0000,    0xde00,    0x0000,    0x0200,    0x0000,    0x0000,
                                    0x0000,    0xadde,    0x0000,    0x0400,    0x0000,    0x0000,    0xadde,    0xefbe,
                                    0x0000,    0x0800,    0xfeca,    0xbeba,    0xadde,    0xadde };
                                    
//0x0000,    0x01c1,    0x0200,    0x0002,    0xb200,    0xa8a0,    0x0000,    0x0100,
//0x0000,    0x01c1,    0x0200,    0x0002,    0x009f,    0xb509,    0x0000,    0x0100,
                                    
constexpr uint64_t TARGET_TYPE_OCMB_CHIP = 0x28;
constexpr uint64_t TARGET_TYPE_PROC_CHIP = 0x2;
  
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

std::string getService(sdbusplus::bus_t& bus, const std::string& path,
                     const std::string& interface)
{
  constexpr auto objectMapperName = "xyz.openbmc_project.ObjectMapper";
  constexpr auto objectMapperPath = "/xyz/openbmc_project/object_mapper";

  auto method = bus.new_method_call(objectMapperName, objectMapperPath,
                                    objectMapperName, "GetObject");

  method.append(path);
  method.append(std::vector<std::string>({interface}));

  std::vector<std::pair<std::string, std::vector<std::string>>> response;

  try 
  {
      auto reply = bus.call(method);
      reply.read(response);
      if (response.empty())
      {   
          std::cout << "Error in mapper response for getting path " <<  path << " interface " << interface << std::endl;
          return std::string{};
      }   
  }
  catch (const sdbusplus::exception_t& e)
  {
      std::cout << "Error in mapper response for getting path " <<  path << " interface " << interface << std::endl;
      return std::string{};
  }
  std::cout << "came into getService service " << response[0].first << std::endl;    

  return response[0].first;
}

int getSbeFfdcFd()
{
  	std::string templatePath = "/tmp/libphal-XXXXXX";
  
  	// Generate unique file name, create file, and open it.  The XXXXXX
  	// characters are replaced by mkstemp() to make the file name unique.
  	int fd = mkostemp(templatePath.data(), O_RDWR);
  	if (fd == -1) {
  	    std::cout << "failed to create temporary file " << std::endl;
  	}
  
  	// Update file with input Buffer data
  	auto rc = write(fd, testFFDC2.data(), testFFDC2.size()*sizeof(uint16_t));
    if(rc == -1)
    {
        std::cout << "failed to write data to the file " << std::endl;
    }
    std::cout << "came into getSbeFfdcFd fd " << fd << std::endl;    
    return fd;
}

#define SBEFIFO_CMD_CLASS_DUMP              0xAA00
#define   SBEFIFO_CMD_GET_DUMP              0x01 /* long running */
using FFDCData = std::vector<std::pair<std::string, std::string>>;

int createSbeErrorPEL(sdbusplus::bus_t& bus, struct pdbg_target* target)
{
    std::cout << "came into createSbeErrorPEL " << std::endl;
    std::string event = "org.open_power.OCMB.Error.SbeChipOpFailure";

    std::vector<std::tuple<
          sdbusplus::xyz::openbmc_project::Logging::server::Create::FFDCFormat,
          uint8_t, uint8_t, sdbusplus::message::unix_fd>>
          pelFFDCInfo;
    pelFFDCInfo.emplace_back(
            std::make_tuple(sdbusplus::xyz::openbmc_project::Logging::server::
                        Create::FFDCFormat::Custom,
                        static_cast<uint8_t>(0xCB),
                        static_cast<uint8_t>(0x01), getSbeFfdcFd()));
    FFDCData pelAdditionalData;
    uint32_t cmd = SBEFIFO_CMD_CLASS_DUMP | SBEFIFO_CMD_GET_DUMP;

    uint32_t chipPos;
    pdbg_target_get_attribute(target, "ATTR_FAPI_POS", 4, 1, &chipPos);
    std::cout << "OCMB fapi position is " << chipPos << std::endl;
    pelAdditionalData.emplace_back("SRC6",
                               std::to_string((chipPos << 16) | cmd));
    pelAdditionalData.emplace_back("CHIP_TYPE",
                               std::to_string(TARGET_TYPE_OCMB_CHIP));
    try
    {
        std::string service = getService(bus, "/xyz/openbmc_project/logging",
                                            "org.open_power.Logging.PEL");
        auto method = bus.new_method_call(service.c_str(), "/xyz/openbmc_project/logging",
                                       "org.open_power.Logging.PEL", "CreatePELWithFFDCFiles");
        method.append(event, "xyz.openbmc_project.Logging.Entry.Level.Error", pelAdditionalData, pelFFDCInfo);
        std::cout << "before calling d-bus method CreatePELWithFFDCFiles " << std::endl;
        auto response = bus.call(method);

        // reply will be tuple containing bmc log id, platform log id
        std::tuple<uint32_t, uint32_t> reply = {0, 0};

        std::cout << "before response.read(reply) " << std::endl;
        // parse dbus response into reply
        response.read(reply);
        int plid = std::get<1>(reply); // platform log id is tuple "second"
        std::cout << "after getting plid " << plid << std::endl;
    }
    catch (const sdbusplus::exception_t& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
    }
    return 0;
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

    constexpr uint16_t ODYSSEY_CHIP_ID = 0x60C0;        
    struct pdbg_target *ocmb;
    pdbg_for_each_target("ocmb", NULL, ocmb)
    {
        uint32_t chipId = 0;
        pdbg_target_get_attribute(ocmb, "ATTR_CHIP_ID", 4, 1, &chipId);
    	uint32_t proc = pdbg_target_index(pdbg_target_parent("proc", ocmb));
	    uint64_t val = 0;
	    std::cout << "found ocmb target with index " << pdbg_target_index(ocmb) << std::endl;
	    if(chipId == ODYSSEY_CHIP_ID)
	    {
			pdbg_target_probe(ocmb);
			if (pdbg_target_status(ocmb) != PDBG_TARGET_ENABLED)
			{
				std::cout << "ocmb chip not enabled " << std::endl;	
				return -1;
			}
            std::cout << "calling createSbeErrorPEL "  << pdbg_target_index(ocmb) << std::endl;
			createSbeErrorPEL(bus, ocmb);
			std::cout << "after calling createSbeErrorPEL " << pdbg_target_index(ocmb) << std::endl;
			break;
	    }
    }

    sleep(20);
    return 0;
}

