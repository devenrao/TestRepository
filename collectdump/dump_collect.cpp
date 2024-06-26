extern "C"
{
#include <libpdbg_sbe.h>
}

#include "create_pel.hpp"
#include "dump_collect.hpp"

#include <ekb/hwpf/fapi2/include/target_types.H>
#include <libphal.H>
#include <phal_exception.H>

#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>
#include <sbe_consts.hpp>
#include <xyz/openbmc_project/Common/File/error.hpp>
#include <xyz/openbmc_project/Common/error.hpp>

#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>

namespace openpower
{
namespace dump
{
namespace sbe_chipop
{
constexpr auto SBEFIFO_CMD_CLASS_INSTRUCTION = 0xA700;
constexpr auto SBEFIFO_CMD_CONTROL_INSN = 0x01;

using Severity = sdbusplus::xyz::openbmc_project::Logging::server::Entry::Level;

void writeDumpFile(const std::filesystem::path& path, const uint32_t id,
                   const uint8_t clockState, const uint8_t chipPos,
                   util::DumpDataPtr& dataPtr, const uint32_t len, bool isOcmb)
{
    using namespace phosphor::logging;
    using namespace sdbusplus::xyz::openbmc_project::Common::Error;
    namespace fileError = sdbusplus::xyz::openbmc_project::Common::File::Error;

    // Filename format: <dump_id>.SbeDataClocks<On/Off>.node0.proc<number>
    std::stringstream ss;
    ss << std::setw(8) << std::setfill('0') << id;
    std::string clockStr = (clockState == SBE::SBE_CLOCK_ON) ? "On" : "Off";
    std::string chipStr = isOcmb ? "ocmb" : "proc";

    // Assuming only node0 is supported now
    auto filename = ss.str() + ".SbeDataClocks" + clockStr + ".node0." +
                    chipStr + std::to_string(chipPos);

    std::filesystem::path dumpPath = path / filename;
    std::ofstream outfile{dumpPath, std::ios::out | std::ios::binary};
    if (!outfile.good())
    {
        using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
        using metadata = xyz::openbmc_project::Common::File::Open;
        // Unable to open the file for writing
        auto err = errno;
        log<level::ERR>(
            std::format(
                "Error opening file to write dump, errno({}), filepath({})",
                err, dumpPath.string())
                .c_str());
        report<Open>(metadata::ERRNO(err), metadata::PATH(dumpPath.c_str()));
        // Just return here, so that the dumps collected from other
        // SBEs can be packaged.
        return;
    }
    outfile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
                       std::ifstream::eofbit);
    try
    {
        outfile.write(reinterpret_cast<char*>(dataPtr.getData()), len);
    }
    catch (std::ofstream::failure& oe)
    {
        using namespace sdbusplus::xyz::openbmc_project::Common::File::Error;
        using metadata = xyz::openbmc_project::Common::File::Write;
        log<level::ERR>(std::format("Failed to write to dump file, "
                                    "errorMsg({}), error({}), filepath({})",
                                    oe.what(), oe.code().value(),
                                    dumpPath.string())
                            .c_str());
        report<Write>(metadata::ERRNO(oe.code().value()),
                      metadata::PATH(dumpPath.c_str()));
        // Just return here so dumps collected from other SBEs can be
        // packaged.
    }
    outfile.close();
}

void collectDumpFromSBE(struct pdbg_target* chip,
                        const std::filesystem::path& path, const uint32_t id,
                        const uint8_t type, const uint8_t clockState,
                        const uint64_t failingUnit)
{
    using namespace phosphor::logging;
    auto chipPos = pdbg_target_index(chip);
    bool isOcmb = is_ody_ocmb_chip(chip);
    std::string chipName = isOcmb ? "ocmb" : "proc";
    log<level::INFO>(std::format("Collect dump from ({})({}) path({}) id({}) "
                                 "type({}) clock({}) failingUnit({})",
                                 chipName, chipPos, path.string(), id, type,
                                 clockState, failingUnit)
                         .c_str());

    util::DumpDataPtr dataPtr;
    uint32_t len = 0;
    uint8_t collectFastArray = 0;
    if (clockState == SBE::SBE_CLOCK_OFF)
    {
        if ((type == SBE::SBE_DUMP_TYPE_HOSTBOOT) ||
            ((type == SBE::SBE_DUMP_TYPE_HARDWARE) && (chipPos == failingUnit)))
        {
            collectFastArray = 1;
        }
    }

    try
    {
        openpower::phal::sbe::getDump(chip, type, clockState, collectFastArray,
                                      dataPtr.getPtr(), &len);
    }
    catch (const openpower::phal::sbeError_t& sbeError)
    {
        if (sbeError.errType() ==
            openpower::phal::exception::SBE_CHIPOP_NOT_ALLOWED)
        {
            // SBE is not ready to accept chip-ops,
            // Skip the request, no additional error handling required.
            log<level::INFO>(std::format("Collect dump: Skipping ({}) dump({}) "
                                         "on ({})({}) clock state({})",
                                         sbeError.what(), type, chipName,
                                         chipPos, clockState)
                                 .c_str());
            return;
        }
        log<level::ERR>(
            std::format(
                "Error in collecting dump dump type({}), clockstate({}), proc "
                "position({}), collectFastArray({}) error({})",
                type, clockState, chipPos, collectFastArray, sbeError.what())
                .c_str());

        auto dumpIsRequired = false;
        openpower::dump::pel::FFDCData pelAdditionalData;
        uint32_t cmd = SBE::SBEFIFO_CMD_CLASS_DUMP | SBE::SBEFIFO_CMD_GET_DUMP;

        pelAdditionalData.emplace_back("SRC6",
                                       std::to_string((chipPos << 16) | cmd));

        std::string event;
        uint32_t logId = 0;
        if (isOcmb)
        {
            event = "org.open_power.OCMB.Error.SbeChipOpFailure";
            if (sbeError.errType() ==
                openpower::phal::exception::SBE_CMD_TIMEOUT)
            {
                event = "org.open_power.OCMB.Error.SbeChipOpTimeout";
                dumpIsRequired = true;
            }
            else if (sbeError.errType() ==
                openpower::phal::exception::SBE_INTERNAL_FFDC_DATA)
            {
                event = "org.open_power.OCMB.Error.SbeInternalFFDCData";
            }
            pelAdditionalData.emplace_back(
                "CHIP_TYPE", std::to_string(fapi2::TARGET_TYPE_OCMB_CHIP));
            logId = openpower::dump::pel::createPOZSbeErrorPEL(
                event, sbeError, pelAdditionalData);
        }
        else
        {
            event = "org.open_power.Processor.Error.SbeChipOpFailure";
            if (sbeError.errType() ==
                openpower::phal::exception::SBE_CMD_TIMEOUT)
            {
                event = "org.open_power.Processor.Error.SbeChipOpTimeout";
                dumpIsRequired = true;
            }
            else if (sbeError.errType() ==
                openpower::phal::exception::SBE_INTERNAL_FFDC_DATA)
            {
                event = "org.open_power.Processor.Error.SbeInternalFFDCData";
            }
            logId = openpower::dump::pel::createSbeErrorPEL(
                event, sbeError, pelAdditionalData, Severity::Error);
        }
        // TODO: requestSBEDump is not yet catered for ody
        if (isOcmb)
        {
            return;
        }
        if (dumpIsRequired)
        {
            // Request SBE Dump
            try
            {
                util::requestSBEDump(chipPos, logId);
            }
            catch (const std::exception& e)
            {
                log<level::ERR>(
                    std::format("SBE Dump request failed, ({})({}) error({})",
                                chipName, chipPos, e.what())
                        .c_str());
            }
        }

        auto primaryProc = false;
        try
        {
            primaryProc = openpower::phal::pdbg::isPrimaryProc(chip);
        }
        catch (const std::exception& e)
        {
            log<level::ERR>(
                std::format("Error checking for primary proc error({})",
                            e.what())
                    .c_str());
        }

        if ((primaryProc) && (type == SBE::SBE_DUMP_TYPE_HOSTBOOT))
        {
            log<level::ERR>("Hostboot dump collection failed on primary, "
                            "aborting colllection");
            throw;
        }
        return;
    }
    writeDumpFile(path, id, clockState, chipPos, dataPtr, len, isOcmb);
}

void collectDump(const uint8_t type, const uint32_t id,
                 const uint64_t failingUnit, const std::filesystem::path& path)
{
    using namespace phosphor::logging;
    log<level::INFO>(
        std::format(
            "Dump collection started type({}) id({}) failingUnit({}), path({})",
            type, id, failingUnit, path.string())
            .c_str());
    struct pdbg_target* target = nullptr;
    auto failed = false;
    // Initialize PDBG
    openpower::phal::pdbg::init();

    std::vector<struct pdbg_target*> targetList;

    pdbg_for_each_class_target("proc", target)
    {
        auto index = pdbg_target_index(target);
        if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED)
        {
            continue;
        }
        if (!openpower::phal::pdbg::isTgtFunctional(target))
        {
            if (openpower::phal::pdbg::isPrimaryProc(target))
            {
                // Primary processor is not functional
                log<level::INFO>(
                    std::format("Primary Processor({}) is not functional",
                                index)
                        .c_str());
            }
            continue;
        }

        // if the dump type is hostboot then call stop instructions
        if (type == openpower::dump::SBE::SBE_DUMP_TYPE_HOSTBOOT)
        {
            try
            {
                openpower::phal::sbe::threadStopProc(target);
            }
            catch (const openpower::phal::sbeError_t& sbeError)
            {
                auto errType = sbeError.errType();
                // Create PEL only for valid SBE reported failures
                if (errType == openpower::phal::exception::SBE_CMD_FAILED)
                {
                    log<level::ERR>(
                        std::format(
                            "Stop instructions failed, "
                            " on proc({}) error({}) error type({}), a "
                            "PELL will be logged",
                            index, sbeError.what(),
                            static_cast<
                                std::underlying_type_t<decltype(errType)>>(
                                errType))
                            .c_str());
                    uint32_t cmd = SBEFIFO_CMD_CLASS_INSTRUCTION |
                                   SBEFIFO_CMD_CONTROL_INSN;
                    // To store additional data about ffdc.
                    openpower::dump::pel::FFDCData pelAdditionalData;
                    // SRC6 : [0:15] chip position
                    //        [16:23] command class,  [24:31] Type
                    pelAdditionalData.emplace_back(
                        "SRC6", std::to_string((index << 16) | cmd));

                    // Create error log.
                    openpower::dump::pel::createSbeErrorPEL(
                        "org.open_power.Processor.Error.SbeChipOpFailure",
                        sbeError, pelAdditionalData,
                        openpower::dump::pel::Severity::Informational);
                }
                else
                {
                    log<level::INFO>(
                        std::format(
                            "Stop instructions failed, "
                            " on proc({}) error({}) error type({})",
                            index, sbeError.what(),
                            static_cast<
                                std::underlying_type_t<decltype(errType)>>(
                                errType))
                            .c_str());
                }
            }
        }
        targetList.push_back(target);
        if (type == openpower::dump::SBE::SBE_DUMP_TYPE_HARDWARE)
        {
            struct pdbg_target* ocmbTarget;
            pdbg_for_each_target("ocmb", target, ocmbTarget)
            {
                if (pdbg_target_probe(ocmbTarget) != PDBG_TARGET_ENABLED)
                {
                    continue;
                }
                if (!is_ody_ocmb_chip(ocmbTarget))
                {
                    continue;
                }
                targetList.push_back(ocmbTarget);
            }
        }
    }

    std::vector<uint8_t> clockStates = {SBE::SBE_CLOCK_ON, SBE::SBE_CLOCK_OFF};
    for (auto cstate : clockStates)
    {
        // Performace dump need to collect only when clocks are ON
        if ((type == SBE::SBE_DUMP_TYPE_PERFORMANCE) &&
            (cstate != SBE::SBE_CLOCK_ON))
        {
            continue;
        }

        std::vector<pid_t> pidList;

        for (pdbg_target* dumpTarget : targetList)
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                log<level::ERR>(
                    "Fork failed while starting hostboot dump collection");
                throw std::runtime_error(
                    "Fork failed while starting hostboot dump collection");
            }
            else if (pid == 0)
            {
                try
                {
                    collectDumpFromSBE(dumpTarget, path, id, type, cstate,
                                       failingUnit);
                }
                catch (const std::runtime_error& error)
                {
                    log<level::ERR>(
                        std::format(
                            "Failed to execute collection, errorMsg({})",
                            error.what())
                            .c_str());
                    std::exit(EXIT_FAILURE);
                }
                std::exit(EXIT_SUCCESS);
            }
            else
            {
                pidList.push_back(std::move(pid));
            }
        }
        for (auto& p : pidList)
        {
            auto status = 0;
            waitpid(p, &status, 0);
            if (WEXITSTATUS(status))
            {
                log<level::ERR>(
                    std::format("Dump collection failed, status({}) pid({})",
                                status, p)
                        .c_str());
                failed = true;
            }
        }
        // Exit if there is a critical failure and collection cannot continue
        // or if the dump collection folder is empty return failure
        if ((failed) || (std::filesystem::is_empty(path)))
        {
            log<level::ERR>("Failed to collect the dump");
            std::exit(EXIT_FAILURE);
        }
        log<level::INFO>(
            std::format("Dump collection completed for clock_state({})", cstate)
                .c_str());
    }
}
} // namespace sbe_chipop
} // namespace dump
} // namespace openpower
