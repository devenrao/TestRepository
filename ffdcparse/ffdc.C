#include <iostream>
#include <vector>
#include <iomanip>
#include <cstdint>
#include <optional>
#include <map>
#include <span>
#include <cstring>
#include <errno.h>   // For EPROTO
#include <endian.h>  // For be16toh, be32toh
#include <format>
#ifndef be16toh
    #define be16toh(x) __builtin_bswap16(x)
#endif
#ifndef be32toh
    #define be32toh(x) __builtin_bswap32(x)
#endif

constexpr uint32_t MAGIC_MASK = 0xFFFF0000;
constexpr uint32_t MAGIC_HEADER = 0xC0DE0000;
constexpr uint16_t FFDC_MAGIC = 0xFBAD;
constexpr uint16_t FFDC_HEADER_LEN = 0x5;

using Slid = uint16_t;
namespace fapi2 {
    enum errlSeverity_t : uint8_t {
        SEV_UNDEFINED = 0,
        SEV_RECOVERED,
        SEV_PREDICTIVE,
        SEV_UNRECOVERABLE,
    };
}
struct FFDCEntry
{
    std::vector<std::byte> data;
    uint32_t fapiRc;
    fapi2::errlSeverity_t severity;
};
using FFDCMap = std::map<Slid, std::vector<FFDCEntry>>;
using FFDCMapOpt = std::optional<FFDCMap>;

constexpr size_t WORD_SIZE = sizeof(uint32_t);
constexpr uint16_t STATUS_RESP_SIZE = 3 * WORD_SIZE;

struct __attribute__((packed)) pozFfdcHeader
{
    uint16_t magicByte;     // 0xFBAD
    uint16_t lengthInWords; // FFDC length in words (N + 5)
    uint16_t seqId;
    uint8_t cmdClass;
    uint8_t cmd;
    uint16_t slid;
    uint8_t severity;
    uint8_t chipId;
    uint32_t fapiRc;
};

constexpr uint32_t MAX_SBE_RESP_SIZE = 0x9999;

union StatusWord
{
    uint32_t full;
    struct
    {   
        uint16_t secondary;
        uint16_t primary;
    };  
};
constexpr uint32_t SBEFIFO_MIN_RESP_LEN = 0x10;

// Dummy logger implementation (replace with your actual logger if needed)
namespace logger {
    template<typename... Args>
    void info(const std::string& fmt, Args&&... args) {
        std::cout << "INFO: " << std::vformat(fmt, std::make_format_args(args...)) << "\n";
    }

    template<typename... Args>
    void error(const std::string& fmt, Args&&... args) {
        std::cerr << "ERROR: " << std::vformat(fmt, std::make_format_args(args...)) << "\n";
    }
}

// Converts vector<uint32_t> to vector<std::byte> in big endian
std::vector<std::byte> convertToBytes(const std::vector<uint32_t>& words) {
    std::vector<std::byte> bytes;
    for (uint32_t word : words) {
        bytes.push_back(static_cast<std::byte>((word >> 24) & 0xFF));
        bytes.push_back(static_cast<std::byte>((word >> 16) & 0xFF));
        bytes.push_back(static_cast<std::byte>((word >> 8) & 0xFF));
        bytes.push_back(static_cast<std::byte>(word & 0xFF));
    }
    return bytes;
}

void printBytes(const std::vector<std::byte>& data, const std::string& label) {
    std::cout << label << " (" << data.size() << " bytes):\n";
    for (size_t i = 0; i < data.size(); ++i) {
        if (i % 16 == 0) std::cout << "\n  ";
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << "\n";
}

void printFFDCMap(const FFDCMap& ffdcMap) {
    for (const auto& [slid, entries] : ffdcMap) {
        std::cout << "SLID: 0x" << std::hex << slid << std::dec
                  << " has " << entries.size() << " entries\n";
        for (const auto& entry : entries) {
            std::cout << "  - fapiRc: 0x" << std::hex << entry.fapiRc
                      << ", severity: " << std::dec << static_cast<int>(entry.severity)
                      << ", data size: " << entry.data.size() << " bytes\n";

            // Ensure size is a multiple of 4
            if (entry.data.size() % 4 != 0) {
                std::cout << "    Warning: data size is not a multiple of 4 bytes\n";
                continue;
            }

            std::cout << "    Data (uint32_t words, big endian):\n";
            for (size_t i = 0; i < entry.data.size(); i += 4) {
                uint32_t word = 0;
                word |= static_cast<uint32_t>(entry.data[i + 0]) << 24;
                word |= static_cast<uint32_t>(entry.data[i + 1]) << 16;
                word |= static_cast<uint32_t>(entry.data[i + 2]) << 8;
                word |= static_cast<uint32_t>(entry.data[i + 3]) << 0;

                if ((i / 4) % 4 == 0) std::cout << "      ";
                std::cout << std::hex << std::setw(8) << std::setfill('0') << word << " ";
                if ((i / 4 + 1) % 4 == 0) std::cout << "\n";
            }
            std::cout << std::dec << "\n";
        }
    }
}
int parseSBEFFDC(const std::vector<std::byte>& buf,
                 size_t offset,
                 size_t endOffset,
                 FFDCMap& ffdcMap);

int parseSBEResponse(const std::vector<std::byte>& buf,
                     std::vector<std::byte>& value,
                     uint16_t& primary,
                     uint16_t& secondary,
                     FFDCMapOpt* ffdc);

void printAsUint32Words(const std::vector<std::byte>& data, const std::string& label) {
    if (data.size() % 4 != 0) {
        std::cerr << label << ": Size is not a multiple of 4 bytes (" << data.size() << " bytes)\n";
        return;
    }

    std::cout << label << " (" << data.size() / 4 << " words):\n";
    for (size_t i = 0; i < data.size(); i += 4) {
        uint32_t word = 0;
        word |= static_cast<uint32_t>(data[i + 0]) << 24;
        word |= static_cast<uint32_t>(data[i + 1]) << 16;
        word |= static_cast<uint32_t>(data[i + 2]) << 8;
        word |= static_cast<uint32_t>(data[i + 3]);

        std::cout << "  0x" << std::hex << std::setw(8) << std::setfill('0') << word << "\n";
    }
    std::cout << std::dec;
}
bool startsWithFBADMagic(const std::vector<std::byte>& value) {
    if (value.size() < 2) return false;

    uint16_t magic = 0;
    magic |= static_cast<uint16_t>(value[0]) << 8;
    magic |= static_cast<uint16_t>(value[1]);

    return magic == 0xFBAD;
}
static const std::vector<uint32_t> ffdcDataRaw1 = {
    0xFBAD0417, 0x0000A801, 0x00040000, 0x02000001, 0x00000000,
    0x7A951049, 0xFF000000, 0x00000004, 0x00041038, 0x00020000,
    0x5342455F, 0x54524143, 0x45000000, 0x00000000, 0x0000300F,
    0x38131000, 0xFE2329AF, 0x23C34600, 0x00000000, 0xFFFFFFFF,
    0xEF73FA8F, 0x00000018, 0x00015FD8, 0x00000000, 0x00000000,
    0x2C8D0101, 0x9D5C3E06, 0x5FD60000, 0x9D5C4311, 0xA6390006,
    0x9F4D53A9, 0x0000DA7A, 0x00000000, 0xD0A40102, 0x9F4D561E,
    0x00000000, 0x00000001, 0xC49C0102, 0x9F4D58DA, 0x91A00006,
    0x9F4D5AF1, 0xC9200000, 0x9F4D6021, 0x00000001, 0x00000000,
    0xEB800102, 0x9F4D61C6, 0x000000A2, 0x00000001, 0x1F770102,
    0x9F62C136, 0x000000A2, 0x00000001, 0xF0F10102, 0x9F62C392,
    0x00000003, 0x00000004, 0x348B0102, 0x9F62C7DA, 0x00000000,
    0x00000000, 0xBA9B0101, 0x9F62D142, 0x9FB10000, 0x9FA14901,
    0x00000000, 0x08011400, 0x137B0102, 0x9FA14B7A, 0x5FFA0000,
    0x9FA15E0D, 0x00000000, 0x00000000, 0x2C8D0101, 0x9FA1647E,
    0x5FD60000, 0x9FA16989, 0xA6390006, 0xA4C52195, 0x0000DA7A,
    0x00000000, 0xD0A40102, 0xA4C5240A, 0x00000000, 0x00000001,
    0xC49C0102, 0xA4C526C6, 0x91A00006, 0xA4C528DD, 0xC9200000,
    0xA4C52E0D, 0x00000001, 0x00000000, 0xEB800102, 0xA4C52FB6,
    0x000000A2, 0x00000001, 0x1F770102, 0xA4EF3B22, 0x000000A2,
    0x00000001, 0xF0F10102, 0xA4EF3D7E, 0x00000003, 0x00000004,
    0x348B0102, 0xA4EF41C6, 0x00000000, 0x00000000, 0xBA9B0101,
    0xA4EF4B2E, 0x9FB10000, 0xA55B0985, 0x00000000, 0x080114D7,
    0x137B0102, 0xA55B0C02, 0x5FFA0000, 0xA55B1E91, 0x00000000,
    0x00000000, 0x2C8D0101, 0xA55B2502, 0x5FD60000, 0xA55B2A0D,
    0xA6390006, 0xA7A4EAC1, 0x0000DA7A, 0x00000000, 0xD0A40102,
    0xA7A4ED36, 0x00000000, 0x00000001, 0xC49C0102, 0xA7A4EFF2,
    0x91A00006, 0xA7A4F209, 0xC9200000, 0xA7A4F73D, 0x00000001,
    0x00000000, 0xEB800102, 0xA7A4F8E2, 0x000000A2, 0x00000001,
    0x1F770102, 0xA7D0E2B6, 0x000000A2, 0x00000001, 0xF0F10102,
    0xA7D0E512, 0x00000003, 0x00000004, 0x348B0102, 0xA7D0E95A,
    0x00000000, 0x00000000, 0xBA9B0101, 0xA7D0F2C6, 0x9FB10000,
    0xA810E1E9, 0x00000000, 0x08011400, 0x137B0102, 0xA810E462,
    0x5FFA0000, 0xA810F6F5, 0x00000000, 0x00000000, 0x2C8D0101,
    0xA810FD66, 0x5FD60000, 0xA8110271, 0xA6390006, 0xAD2D90FD,
    0x0000DA7A, 0x00000000, 0xD0A40102, 0xAD2D9372, 0x00000000,
    0x00000001, 0xC49C0102, 0xAD2D962E, 0x91A00006, 0xAD2D9845,
    0xC9200000, 0xAD2D9D75, 0x00000001, 0x00000000, 0xEB800102,
    0xAD2D9F1A, 0x000000A2, 0x00000001, 0x1F770102, 0xAD4EAE0E,
    0x000000A2, 0x00000001, 0xF0F10102, 0xAD4EB06A, 0x00000003,
    0x00000004, 0x348B0102, 0xAD4EB4B2, 0x00000000, 0x00000000,
    0xBA9B0101, 0xAD4EBE1A, 0x9FB10000, 0xADD57B3D, 0x00000000,
    0x080114D7, 0x137B0102, 0xADD57DBA, 0x5FFA0000, 0xADD59049,
    0x00000000, 0x00000000, 0x2C8D0101, 0xADD596BA, 0x5FD60000,
    0xADD59BC5, 0xA6390006, 0xB010F7F9, 0x0000DA7A, 0x00000000,
    0xD0A40102, 0xB010FA6A, 0x00000000, 0x00000001, 0xC49C0102,
    0xB010FD26, 0x91A00006, 0xB010FF3D, 0xC9200000, 0xB0110471,
    0x00000001, 0x00000000, 0xEB800102, 0xB0110616, 0x000000A2,
    0x00000001, 0x1F770102, 0xB0406D3E, 0x000000A2, 0x00000001,
    0xF0F10102, 0xB0406F9A, 0x00000003, 0x00000004, 0x348B0102,
    0xB04073E2, 0x00000000, 0x00000000, 0xBA9B0101, 0xB0407D4A,
    0x9FB10000, 0xB0AB99F9, 0x00000000, 0x08011400, 0x137B0102,
    0xB0AB9C76, 0x5FFA0000, 0xB0ABAF05, 0x00000000, 0x00000000,
    0x2C8D0101, 0xB0ABB576, 0x5FD60000, 0xB0ABBA81, 0xA6390006,
    0xB2FCCA49, 0x0000DA7A, 0x00000000, 0xD0A40102, 0xB2FCCCBE,
    0x00000000, 0x00000001, 0xC49C0102, 0xB2FCCF7A, 0x91A00006,
    0xB2FCD191, 0xC9200000, 0xB2FCD6C5, 0x00000001, 0x00000000,
    0xEB800102, 0xB2FCD86A, 0x000000A2, 0x00000001, 0x1F770102,
    0xB327454A, 0x000000A2, 0x00000001, 0xF0F10102, 0xB32747A6,
    0x00000003, 0x00000004, 0x348B0102, 0xB3274BEE, 0x00000000,
    0x00000000, 0xBA9B0101, 0xB327555A, 0x9FB10000, 0xB392A375,
    0x00000000, 0x0801100F, 0x137B0102, 0xB392A5F2, 0x5FFA0000,
    0xB392B881, 0x00000000, 0x00000000, 0x2C8D0101, 0xB392BEF2,
    0x5FD60000, 0xB392C3FD, 0xA6390006, 0xB5F70F2D, 0x0000DA7A,
    0x00000000, 0xD0A40102, 0xB5F711A2, 0x00000000, 0x00000001,
    0xC49C0102, 0xB5F7145E, 0x91A00006, 0xB5F71675, 0xC9200000,
    0xB5F71BA5, 0x00000001, 0x00000000, 0xEB800102, 0xB5F71D4A,
    0x000000A2, 0x00000002, 0x1F770102, 0xB6292BAA, 0x000000A2,
    0x00000002, 0xF0F10102, 0xB6292E06, 0x00000003, 0x00000004,
    0x348B0102, 0xB629328E, 0x00000000, 0x00000000, 0xBA9B0101,
    0xB6293BFA, 0x9FB10000, 0xB6F193C9, 0x5FFA0000, 0xB6F1A229,
    0x00000000, 0x00000000, 0x2C8D0101, 0xB6F1A8F6, 0x5FD60000,
    0xB6F1ADFD, 0xA6390006, 0xB8E5554D, 0x0000DA7A, 0x00000000,
    0xD0A40102, 0xB8E557C2, 0x00000000, 0x00000001, 0xC49C0102,
    0xB8E55A7E, 0x91A00006, 0xB8E55C95, 0xC9200000, 0xB8E561C5,
    0x00000001, 0x00000000, 0xEB800102, 0xB8E5636A, 0x000000A2,
    0x00000002, 0x1F770102, 0xB915279A, 0x000000A2, 0x00000002,
    0xF0F10102, 0xB91529F6, 0x00000003, 0x00000004, 0x348B0102,
    0xB9152E7E, 0x00000000, 0x00000000, 0xBA9B0101, 0xB91537E6,
    0x9FB10000, 0xB9EC718D, 0x5FFA0000, 0xB9EC7FE9, 0x00000000,
    0x00000000, 0x2C8D0101, 0xB9EC86BA, 0x5FD60000, 0xB9EC8BC5,
    0xA6390006, 0xBBCC1ECD, 0x0000DA7A, 0x00000000, 0xD0A40102,
    0xBBCC213E, 0x00000000, 0x00000001, 0xC49C0102, 0xBBCC23FA,
    0x91A00006, 0xBBCC2611, 0xC9200000, 0xBBCC2B45, 0x00000001,
    0x00000000, 0xEB800102, 0xBBCC2CEA, 0x000000A2, 0x00000002,
    0x1F770102, 0xBBF7B3D2, 0x000000A2, 0x00000002, 0xF0F10102,
    0xBBF7B62E, 0x00000003, 0x00000004, 0x348B0102, 0xBBF7BAB6,
    0x00000000, 0x00000000, 0xBA9B0101, 0xBBF7C41E, 0x9FB10000,
    0xBCCE4125, 0x5FFA0000, 0xBCCE4F85, 0x00000000, 0x00000000,
    0x2C8D0101, 0xBCCE5656, 0x5FD60000, 0xBCCE5B61, 0xA6390006,
    0xBEA867E9, 0x0000DA7A, 0x00000000, 0xD0A40102, 0xBEA86A5E,
    0x00000000, 0x00000001, 0xC49C0102, 0xBEA86D1A, 0x91A00006,
    0xBEA86F31, 0xC9200000, 0xBEA87461, 0x00000001, 0x00000000,
    0xEB800102, 0xBEA87606, 0x000000A2, 0x00000002, 0x1F770102,
    0xBED408BE, 0x000000A2, 0x00000002, 0xF0F10102, 0xBED40B1A,
    0x00000003, 0x00000004, 0x348B0102, 0xBED40FA2, 0x00000000,
    0x00000000, 0xBA9B0101, 0xBED4190E, 0x9FB10000, 0xBFAA5D9D,
    0x5FFA0000, 0xBFAA6BFD, 0x00000000, 0x00000000, 0x2C8D0101,
    0xBFAA72CA, 0x5FD60000, 0xBFAA77D9, 0xA6390006, 0xC184C3F5,
    0x0000DA7A, 0x00000000, 0xD0A40102, 0xC184C66A, 0x00000000,
    0x00000001, 0xC49C0102, 0xC184C926, 0x91A00006, 0xC184CB3D,
    0xC9200000, 0xC184D071, 0x00000001, 0x00000000, 0xEB800102,
    0xC184D216, 0x000000A2, 0x00000002, 0x1F770102, 0xC1B0CDA2,
    0x000000A2, 0x00000002, 0xF0F10102, 0xC1B0CFFE, 0x00000003,
    0x00000004, 0x348B0102, 0xC1B0D486, 0x00000000, 0x00000000,
    0xBA9B0101, 0xC1B0DDF2, 0x9FB10000, 0xC271A5B5, 0x5FFA0000,
    0xC271B415, 0x00000000, 0x00000000, 0x2C8D0101, 0xC271BAE2,
    0x5FD60000, 0xC271BFF1, 0xA6390006, 0xC4759B01, 0x0000DA7A,
    0x00000000, 0xD0A40102, 0xC4759D76, 0x00000000, 0x00000001,
    0xC49C0102, 0xC475A032, 0x91A00006, 0xC475A249, 0xC9200000,
    0xC475A77D, 0x00000001, 0x00000000, 0xEB800102, 0xC475A922,
    0x000000A2, 0x00000002, 0x1F770102, 0xC496C9CA, 0x000000A2,
    0x00000002, 0xF0F10102, 0xC496CC26, 0x00000003, 0x00000004,
    0x348B0102, 0xC496D0AE, 0x00000000, 0x00000000, 0xBA9B0101,
    0xC496DA1A, 0x9FB10000, 0xC5585CE9, 0x5FFA0000, 0xC5586B45,
    0x00000000, 0x00000000, 0x2C8D0101, 0xC5587216, 0x5FD60000,
    0xC5587721, 0xA6390006, 0xC74DD4D5, 0x0000DA7A, 0x00000000,
    0xD0A40102, 0xC74DD74A, 0x00000000, 0x00000001, 0xC49C0102,
    0xC74DDA06, 0x91A00006, 0xC74DDC1D, 0xC9200000, 0xC74DE14D,
    0x00000001, 0x00000000, 0xEB800102, 0xC74DE2F6, 0x000000A2,
    0x00000002, 0x1F770102, 0xC77CD282, 0x000000A2, 0x00000002,
    0xF0F10102, 0xC77CD4DE, 0x00000003, 0x00000004, 0x348B0102,
    0xC77CD966, 0x00000000, 0x00000000, 0xBA9B0101, 0xC77CE2CE,
    0x9FB10000, 0xC7E91811, 0x5FFA0000, 0xC7E92671, 0x00000000,
    0x00000000, 0x2C8D0101, 0xC7E92D3E, 0x5FD60000, 0xC7E9324D,
    0xA6390006, 0xC9A29B65, 0x0000DA7A, 0x00000000, 0xD0A40102,
    0xC9A29DDA, 0x00000000, 0x00000001, 0xC49C0102, 0xC9A2A096,
    0x91A00006, 0xC9A2A2AD, 0xC9200000, 0xC9A2A7DD, 0x00000001,
    0x00000000, 0xEB800102, 0xC9A2A986, 0x000000A1, 0x00000004,
    0x1F770102, 0xC9CE012A, 0x000000A1, 0x00000004, 0xF0F10102,
    0xC9CE038A, 0x00000003, 0x00000005, 0x348B0102, 0xC9CE0826,
    0x00000000, 0x00000000, 0xBA9B0101, 0xC9CE118E, 0x9FB10000,
    0xCA235E6D, 0x00000002, 0x0000000B, 0xD2940102, 0xCA236116,
    0x00000002, 0x00000000, 0x3DFA0101, 0xCA236356, 0x0000000B,
    0x00000002, 0x3A930102, 0xCA23654A, 0x5FFA0000, 0xCA7CD831,
    0x00000000, 0x00000000, 0x2C8D0101, 0xCA7CDEEE, 0x5FD60000,
    0xCA7CE3F9, 0xA6390006, 0xCD4BAB5D, 0x0000DA7A, 0x00000000,
    0xD0A40102, 0xCD4BADD2, 0x00000000, 0x00000001, 0xC49C0102,
    0xCD4BB08E, 0x91A00006, 0xCD4BB2A5, 0xC9200000, 0xCD4BB7D5,
    0x00000001, 0x00000000, 0xEB800102, 0xCD4BB97E, 0x000000A2,
    0x00000001, 0x1F770102, 0xCD610732, 0x000000A2, 0x00000001,
    0xF0F10102, 0xCD61098E, 0x00000003, 0x00000004, 0x348B0102,
    0xCD610DD6, 0x00000000, 0x00000000, 0xBA9B0101, 0xCD61173E,
    0x9FB10000, 0xCDA0D8F5, 0x00000000, 0x0801102A, 0x137B0102,
    0xCDA0DB72, 0x5FFA0000, 0xCDA0EE01, 0x00000000, 0x00000000,
    0x2C8D0101, 0xCDA0F472, 0x5FD60000, 0xCDA0F97D, 0xA6390006,
    0xCEC49951, 0x0000DA7A, 0x00000000, 0xD0A40102, 0xCEC49BC6,
    0x00000000, 0x00000001, 0xC49C0102, 0xCEC49E82, 0x91A00006,
    0xCEC4A099, 0xC9200000, 0xCEC4A5CD, 0x00000001, 0x00000000,
    0xEB800102, 0xCEC4A772, 0x000000A2, 0x00000002, 0x1F770102,
    0xCED9F81A, 0x000000A2, 0x00000002, 0xF0F10102, 0xCED9FA76,
    0x00000003, 0x00000004, 0x348B0102, 0xCED9FEFE, 0x00000000,
    0x00000000, 0xBA9B0101, 0xCEDA086A, 0x9FB10000, 0xCF4442CD,
    0x5FFA0000, 0xCF44512D, 0x00000000, 0x00000000, 0x2C8D0101,
    0xCF4457FA, 0x5FD60000, 0xCF445D01, 0xA6390006, 0xD3E060DD,
    0x0000DA7A, 0x00000000, 0xD0A40102, 0xD3E06352, 0x00000000,
    0x00000001, 0xC49C0102, 0xD3E0660E, 0x91A00006, 0xD3E06825,
    0xC9200000, 0xD3E06D59, 0x00000001, 0x00000000, 0xEB800102,
    0xD3E06EFE, 0x000000A2, 0x00000001, 0x1F770102, 0xD3F61FA2,
    0x000000A2, 0x00000001, 0xF0F10102, 0xD3F621FE, 0x00000003,
    0x00000004, 0x348B0102, 0xD3F62646, 0x00000000, 0x00000000,
    0xBA9B0101, 0xD3F62FAE, 0x9FB10000, 0xD436D1D5, 0x00000000,
    0x0801102A, 0x137B0102, 0xD436D452, 0x5FFA0000, 0xD436E6E1,
    0x00000000, 0x00000000, 0x2C8D0101, 0xD436ED52, 0x5FD60000,
    0xD436F25D, 0xA6390006, 0xD55B66E5, 0x0000DA7A, 0x00000000,
    0xD0A40102, 0xD55B6956, 0x00000000, 0x00000001, 0xC49C0102,
    0xD55B6C12, 0x91A00006, 0xD55B6E29, 0xC9200000, 0xD55B735D,
    0x00000001, 0x00000000, 0xEB800102, 0xD55B7502, 0x000000A2,
    0x00000002, 0x1F770102, 0xD56ED87E, 0x000000A2, 0x00000002,
    0xF0F10102, 0xD56EDADE, 0x00000003, 0x00000004, 0x348B0102,
    0xD56EDF62, 0x00000000, 0x00000000, 0xBA9B0101, 0xD56EE8CE,
    0x9FB10000, 0xD5D9E4F5, 0x5FFA0000, 0xD5D9F351, 0x00000000,
    0x00000000, 0x2C8D0101, 0xD5D9FA1E, 0x5FD60000, 0xD5D9FF25,
    0xA6390006, 0x82EF2B41, 0x0000DA7A, 0x00000000, 0xD0A40102,
    0x82EF2DB2, 0x00000000, 0x00000001, 0xC49C0102, 0x82EF306E,
    0x91A00006, 0x82EF3285, 0xC9200000, 0x82EF37B9, 0x00000001,
    0x00000000, 0xEB800102, 0x82EF395E, 0x000000C1, 0x00000001,
    0x1F770102, 0x83048E76, 0x000000C1, 0x00000001, 0xF0F10102,
    0x830490D2, 0x00000003, 0x00000005, 0x348B0102, 0x830495A6,
    0x00000000, 0x00000000, 0xBA9B0101, 0x83049F0E, 0x00000010,
    0xFFFE8D90, 0xFFFF99E0, 0xFFFF99C8, 0x84FD0104, 0x8319EBEA,
    0x9FB10000, 0x83835CB5, 0x00077A17, 0x00000000, 0x14350101,
    0x83837B16, 0x000002D0, 0xFFFE8D90, 0xFFFF99C8, 0xFFFF96F0,
    0x84FD0104, 0x83837EEA, 0x000002D0, 0xFFFE8D90, 0xFFFF96F0,
    0xFFFF9418, 0x84FD0104, 0x8383DDDA, 0x007D41D4, 0x00000000,
    0x58670101, 0x838416D2, 0x5FFA0000, 0x83842E19, 0x76010000,
    0x83843699, 0x00000000, 0x00000000, 0x2C8D0101, 0x83843832,
    0x5FD60000, 0x83843D3D, 0xA6390006, 0x652284FD, 0x0000DA7A,
    0x00000000, 0xD0A40102, 0x65228772, 0x00000000, 0x00000001,
    0xC49C0102, 0x65228A2E, 0x91A00006, 0x65228C45, 0xC9200000,
    0x65229175, 0x00000001, 0x00000000, 0xEB800102, 0x6522931A,
    0x000000A8, 0x00000001, 0x1F770102, 0x6537121A, 0x000000A8,
    0x00000001, 0xF0F10102, 0x65371476, 0x00000003, 0x00000004,
    0x348B0102, 0x65371902, 0x00000000, 0x00000000, 0xBA9B0101,
    0x6537226E, 0x00000000, 0x00000000, 0x5B240101, 0x65372736,
    0x9FB10000, 0x654C6645, 0x00000000, 0x00000000, 0x01E50101,
    0x654C687E, 0x5FFA0000, 0x654C6AED, 0x5FFA0000, 0xF0E914B9,
    0x00000000, 0x00000000, 0x2C8D0101, 0xF0FDB0B2, 0x5FD60000,
    0xF128E409, 0xA6390006, 0xF2861F51, 0x0000DA7A, 0x00000000,
    0xD0A40102, 0xF28621C6, 0x00000000, 0x00000001, 0xC49C0102,
    0xF2862482, 0x91A00006, 0xF2862699, 0xC9200000, 0xF2862BC9,
    0x00000001, 0x00000000, 0xEB800102, 0xF2862D72, 0x000000A8,
    0x00000001, 0x1F770102, 0xF29B76B6, 0x000000A8, 0x00000001,
    0xF0F10102, 0xF29B7916, 0x00000003, 0x00000004, 0x348B0102,
    0xF29B7DA2, 0x00000000, 0x00000000, 0xBA9B0101, 0xF29B870A,
    0x00000000, 0x00000000, 0x5B240101, 0xF29B8BD6, 0x9FB10000,
    0xF2B0CAE5, 0x00000000, 0x00000000, 0x01E50101, 0xF2B0CD1E,
    0x5FFA0000, 0xF2B0CF8D, 0xBA9B0101, 0x9CDCA20A, 0x9FB10000,
    0x9D5C2289, 0x00000000, 0x080114D7, 0x137B0102, 0x9D5C2506,
    0x5FFA0000, 0x9D5C3795, 0xC0DEA801, 0x00000000, 0x00000003};

static const std::vector<uint32_t>  getScom = {
  0x9033603F,   0x00000000,   0xC0DEA201,   0x00000000,   0xFBAD0009,   0x0000A201,   0x00014000,   0x00A5BA9B,
  0x00000001,   0x0001000C,   0x00000000,   0x00000000,   0x00000000,   0xFBAD0097,   0x0000A201,   0x00014000,
  0x02000001,   0x00FE002C,   0x1CD7B376,   0x00000000,   0x00000004,   0x00040238,   0x00020000,   0x5342455F,
  0x54524143,   0x45000000,   0x00000000,   0x0000300F,   0x38130200,   0xFE2329AF,   0x17D78400,   0x00000000,
  0xFFFFFFFF,   0xF3A94B4C,   0x00000003,   0x00000200,   0x00000003,   0x00000017,   0x94AC0102,   0xAF018DF2,
  0x0C090001,   0xAF0198C5,   0x5FFA0000,   0xAF019ACD,   0x00000000,   0x00000000,   0x2C8D0101,   0xAF01A18A,
  0x5FD60000,   0xAF01A66D,   0xA6390006,   0xB038EF55,   0x0000DA7A,   0x00000000,   0xD0A40102,   0xB038F1AA,
  0x00000000,   0x00000001,   0xC49C0102,   0xB038F44A,   0x91A00006,   0xB038F641,   0xC9200000,   0xB038FB65,
  0x00000001,   0x00000000,   0xEB800102,   0xB038FD1A,   0x000000A1,   0x00000001,   0x1F770102,   0xB0526AF6,
  0x000000A1,   0x00000001,   0xF0F10102,   0xB0526D5A,   0x00000003,   0x00000005,   0x348B0102,   0xB05271C6,
  0x00000000,   0x00000000,   0xBA9B0101,   0xB0527B3A,   0x9FB10000,   0xB0A338E1,   0x35BB0000,   0xB0A33CD1,
  0x00000003,   0x00000018,   0x94AC0102,   0xB0A33EFA,   0x2CCF0000,   0xB0A3427D,   0xC5770000,   0xB0A351ED,
  0x00000000,   0x00000003,   0x00000009,   0x00000002,   0x57060104,   0xB0A35742,   0x0C090001,   0xB0A35F15,
  0x5FFA0000,   0xB0A3611D,   0x00000000,   0x00000000,   0x2C8D0101,   0xB0A367DA,   0x5FD60000,   0xB0A36CBD,
  0xA6390006,   0x5A7CF211,   0x0000DA7A,   0x00000000,   0xD0A40102,   0x5A7CF466,   0x00000000,   0x00000001,
  0xC49C0102,   0x5A7CF706,   0x91A00006,   0x5A7CF8FD,   0xC9200000,   0x5A7CFE21,   0x00000001,   0x00000000,
  0xEB800102,   0x5A7CFFD6,   0x000000A2,   0x00000001,   0x1F770102,   0x5A89FA4E,   0x000000A2,   0x00000001,
  0xF0F10102,   0x5A89FCB2,   0x00000003,   0x00000004,   0x348B0102,   0x5A8A0126,   0x00000000,   0x00000000,
  0xBA9B0101,   0x5A8A0A9A,   0x9FB10000,   0x5AC3C4E9,   0x00000000,   0x00050009,   0x137B0102,   0x5AC3C74E,
  0xA54C0000,   0x5AC3CEE1,   0xE2BE0000,   0x5AC3D42D,   0xAD430000,   0x5AC3D539,   0x00000290,   0xFFFE0888,
  0xFFFF99E0,   0xFFFF9748,   0x84FD0104,   0x5AC3D896,   0x000000A3, 
};
static const std::vector<uint32_t> ffdcDataRaw0 = {
    0xFBAD0038, 0x0000A801, 0x00040000, 0x02000001, 0x00000000, 0x7A951049,
    0xFF000000, 0x00000004, 0x00041038, 0x00020000, 0x5342455F, 0x54524143,
    0x45000000, 0x00000000, 0x0000300F, 0x38131000, 0xFE2329AF, 0x23C34600,
    0x00000000, 0xFFFFFFFF, 0xEF73FA8F, 0x00000018, 0x00015FD8, 0x00000000,
    0x00000000, 0x2C8D0101, 0x9D5C3E06, 0x5FD60000, 0x9D5C4311, 0xA6390006,
    0x9F4D53A9, 0x0000DA7A, 0x00000000, 0xD0A40102, 0x9F4D561E, 0x00000000,
    0x00000001, 0xC49C0102, 0x9F4D58DA, 0x91A00006, 0x9F4D5AF1, 0xC9200000,
    0x9F4D6021, 0x00000001, 0x00000000, 0xEB800102, 0x9F4D61C6, 0x000000A2,
    0x00000001, 0x1F770102, 0x9F62C136, 0x000000A2, 0x00000001, 0xF0F10102,
    0x9F62C392, 0x00000003, 0xC0DEA801, 0x00000000, 0x00000003};

int main() {
    std::vector<std::byte> inputBytes = convertToBytes(ffdcDataRaw1);
    std::vector<std::byte> value;
    uint16_t primary = 0, secondary = 0;
    FFDCMapOpt ffdcMap = FFDCMap{};

    int rc = parseSBEResponse(inputBytes, value, primary, secondary, &ffdcMap);
    if (rc != 0) {
        std::cerr << "parseSBEResponse failed with rc = " << rc << "\n";
        return rc;
    }

    if (ffdcMap && startsWithFBADMagic(value)) {
        std::cerr << "****Received FFDC as value so going to parse it " << std::endl;
        auto& ffdcMapRef = ffdcMap.value(); // OR *ffdcMap
        //const size_t endOffset = value.size() >= WORD_SIZE ? value.size() - WORD_SIZE : 0;
        const size_t endOffset = value.size();;
        int rcFFDC = parseSBEFFDC(value, 0, endOffset, ffdcMapRef);
        if (rcFFDC != 0) {
            std::cerr << "parseSBEFFDC failed with rc = " << rcFFDC << "\n";
            return rcFFDC;
        }
    }else
    {
        std::cerr << "****FFDC does not start with FBAD value " << std::endl;
    }

    printBytes(value, "Parsed Value");
    std::cout << "Primary RC: 0x" << std::hex << primary << ", Secondary RC: 0x" << secondary << std::dec << "\n";

    if (ffdcMap.has_value()) {
        printFFDCMap(ffdcMap.value());
    } else {
        std::cout << "No FFDC data parsed.\n";
    }

    return 0;
}

// FFDC Package Format
//
//               Byte 0       |   Byte 1      |   Byte 2      |   Byte 3
// ---------------------------------------------------------------------------
// Word 0  :   Magic Bytes: 0xFBAD            | Length in words (N+5)
// Word 1  :   Sequence ID                    | Command Class | Command
// Word 2  :   SLID                           | Severity      | Chip ID
// Word 3  :   Return Code (bits 0–31)
// Word 4  :   FFDC Data – Word 0
// Word 5  :   FFDC Data – Word 1
// ...
// Word N+4:   FFDC Data – Word N
int parseSBEFFDC(const std::vector<std::byte>& buf,
                 size_t offset,
                 size_t endOffset,
                 FFDCMap& ffdcMap)
{
    constexpr size_t HEADER_SIZE = sizeof(pozFfdcHeader);
    logger::info("parseSBEFFDC: offset 0x{:08X} endOffset 0x{:08X} ", offset, endOffset);

    while (offset + HEADER_SIZE <= endOffset)
    {
        const auto* header = reinterpret_cast<const pozFfdcHeader*>(&buf[offset]);

        // Convert to host endianness
        uint16_t magic = be16toh(header->magicByte);
        uint16_t lengthWords = be16toh(header->lengthInWords);
        uint16_t slid = be16toh(header->slid);
        logger::info("parseSBEFFDC: magic 0x{:04X} lengthWords 0x{:04X} slid 0x{:04X} ", magic, lengthWords, slid);
        if (magic != FFDC_MAGIC)
        {
            logger::error("parseSBEFFDC: Expected FBAD magic at offset {}, got 0x{:04X}", offset, magic);
            return EPROTO;
        }

        size_t totalSizeBytes = lengthWords * WORD_SIZE;
        logger::info(
            "parseSBEFFDC: FFDC entry overruns buffer totalSizeBytes=0x{:08x} "
            "lengthWords=0x{:04x}, offset=0x{:08X} offset=0x{:08X}",
            totalSizeBytes, lengthWords, offset, endOffset);
        if (offset + totalSizeBytes > endOffset)
        {
            logger::error(
                "parseSBEFFDC: FFDC entry overruns buffer totalSizeBytes=0x{:08x} "
                "lengthWords=0x{:04x}, offset=0x{:08X} offset=0x{:08X}",
                totalSizeBytes, lengthWords, offset, endOffset);
            return EPROTO;
        }

        const uint32_t fapiRc = be32toh(header->fapiRc);
        const auto severity = static_cast<fapi2::errlSeverity_t>(header->severity);
        logger::info(
            "sbe_get_ffdc parseSBEFFDC slid 0x{:04x} fapiRc 0x{:04x} severity 0x{:04x}",
            slid, fapiRc, static_cast<int>(severity));

        // Copy entire FFDC block (header + payload)
        std::vector<std::byte> ffdcData(buf.begin() + offset, buf.begin() + offset + totalSizeBytes);
        ffdcMap[slid].emplace_back(FFDCEntry{.data = std::move(ffdcData),
                                                .fapiRc = fapiRc,
                                                .severity = severity});

        offset += totalSizeBytes;
    }

    if (offset != endOffset)
    {
        logger::info("parseSBEFFDC: Unparsed leftover bytes: {}", endOffset - offset);
    }

    return 0;
}
int parseSBEResponse(const std::vector<std::byte>& buf,
                     std::vector<std::byte>& value,
                     uint16_t& primary,
                     uint16_t& secondary,
                     FFDCMapOpt* ffdc)
{
    const size_t buflen = buf.size();
    logger::info("sbe_resp_parser: parseSBEResponse");

    if (buflen < SBEFIFO_MIN_RESP_LEN)
    {
        logger::error("parseSBEResponse: buffer too small: {}", buflen);
        return EPROTO;
    }

    // Get distance word (last word)
    if (buflen < WORD_SIZE)
    {
        logger::error("parseSBEResponse: buffer too small for distance word");
        return EPROTO;
    }

    uint32_t distanceToMagic = 0;
    std::memcpy(&distanceToMagic, &buf[buflen - WORD_SIZE], sizeof(distanceToMagic));
    distanceToMagic = be32toh(distanceToMagic);

    logger::info("sbe_resp_parser: distanceToMagic 0x{:x}", distanceToMagic);

    const size_t headerOffset = buflen - (distanceToMagic * WORD_SIZE);
    logger::info("sbe_resp_parser: headerOffset 0x{:x}", headerOffset);

    if (headerOffset + 2 * WORD_SIZE > buflen)
    {
        logger::error("parseSBEResponse: invalid header offset: {}", headerOffset);
        return EPROTO;
    }

    // Validate magic
    uint32_t header = 0;
    std::memcpy(&header, &buf[headerOffset], sizeof(header));
    header = be32toh(header);

    if ((header & MAGIC_MASK) != MAGIC_HEADER)
    {
        logger::error("parseSBEResponse: invalid magic header 0x{:08x}", header);
        return EPROTO;
    }

    // Extract status
    uint32_t status = 0;
    std::memcpy(&status, &buf[headerOffset + WORD_SIZE], sizeof(status));
    status = be32toh(status);

    StatusWord sw{.full = status};
    primary = static_cast<uint16_t>(sw.primary);
    secondary = static_cast<uint16_t>(sw.secondary);

    // Extract value (everything before header)
    logger::info("headerOffset 0x{:x} ", headerOffset);
    value.clear();
    if (headerOffset > 0)
    {
        logger::info("value present inserting into value");
        value.insert(value.end(), buf.begin(), buf.begin() + headerOffset);
    }
    logger::info("value size 0x{:x}", value.size());

    // Extract FFDC
    logger::info("buflen 0x{:x} valuesize 0x{:x} ", buflen, (value.size() + STATUS_RESP_SIZE));
    if (ffdc && ffdc->has_value() && (buflen > (value.size() + STATUS_RESP_SIZE)))
    {
        const size_t offset = headerOffset + 2 * WORD_SIZE;
        const size_t endOffset = buflen - WORD_SIZE; // exclude last word (distance)

        if (offset > endOffset)
        {
            logger::info("***************parseSBEResponse: no FFDC data available");
        }
        else
        {
            logger::info("***************parseSBEResponse: came here to call parseSBEFFDC");
            auto& ffdcMap = ffdc->value();
            const int rc = parseSBEFFDC(buf, offset, endOffset, ffdcMap);
            if (rc)
            {
                logger::error("parseSBEResponse: ffdc invalid format rc 0x{:08x}", rc);
                return rc;
            }
        }
    }
    return 0;
}

