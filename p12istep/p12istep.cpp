// This application uses test data writes to a file and invokes
// D-Bus method CreatePELWithFFDCFiles
// This application is used to validate logging/sbe_ffdc_handler code
#include <attributes_info.H>
#include <libphal.H>
#include <string.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <vector>
extern "C" {
#include "libpdbg.h"
}
#include <iplfacade.H>

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

static int print_target(struct pdbg_target* target, void* /* priv */)
{
    const char* targetname = pdbg_target_name(target);
    if (targetname)
    {
        std::cout << "DEV 1238 pdbg target name is " << targetname << std::endl;
    }
    char tgtPhysDevPath[64];
    if (pdbg_target_get_attribute(target, "ATTR_PHYS_DEV_PATH", 1, 64,
                                  tgtPhysDevPath))
    {
        std::cout << "DEV 1238 target devpath " << tgtPhysDevPath << std::endl;
    }
    const char* targetPath = pdbg_target_path(target);
    if (targetPath)
    {
        std::cout << "DEV 1238 pdbg target path is " << targetPath << std::endl;
    }

    size_t len;
    const char* compatible =
        (const char*)pdbg_target_property(target, "compatible", &len);
    if (compatible)
    {
        std::cout << "DEV 1238 pdbg compatible is " << compatible << std::endl;
    }

    const char* classname = pdbg_target_class_name(target);
    if (classname)
    {
        std::cout << "DEV 1238 pdbg target class is " << classname << std::endl;
    }
    return 0;
}

inline struct pdbg_target* get_pib_target(struct pdbg_target* target)
{
    struct pdbg_target* pib = NULL;

    /* We assume each processor chip contains one and only one pib */
    pdbg_for_each_target("pib", target, pib)
    {
        break;
    }

    if (!pib)
    {
        std::cout << "No pib target associated with target %s"
                  << pdbg_target_path(target) << std::endl;
        return NULL;
    }

    return pib;
}
int main()
{
    constexpr auto devtree = "/tmp/p12_fake_2.dtb";
    // std::cout << "enter DEV 1238 main application " << std::endl;

    // PDBG_DTB environment variable set to CEC device tree path
    if (setenv("PDBG_DTB", devtree, 1))
    {
        std::cerr << "Failed to set PDBG_DTB: " << strerror(errno) << std::endl;
        return 0;
    }

    pdbg_set_backend(PDBG_BACKEND_SBEFIFO, NULL);
    std::cout << "calling pdbg_targets_init " << std::endl;
    // initialize the targeting system
    if (!pdbg_targets_init(NULL))
    {
        std::cerr << "pdbg_targets_init failed" << std::endl;
        return 0;
    }

    pdbg_set_loglevel(PDBG_DEBUG);
    pdbg_set_logfunc(pdbgLogCallback);

    std::cout << " DEV 1238 trying to loop through all the hub chips "
              << std::endl;

    struct pdbg_target* root = pdbg_target_root();
    // int level =0;
    // pdbg_target_traverse(root, print_target, &level);

    std::cout << "NOW LOOKING FOR hub chip " << std::endl;
    struct pdbg_target* hubchip;
    pdbg_for_each_target("hubchip", NULL, hubchip)
    {
        std::cout << "hubchip found " << std::endl;
        pdbg_target_probe(hubchip);
        if (pdbg_target_status(hubchip) != PDBG_TARGET_ENABLED)
        {
            std::cout << "hubchip chip not enabled " << std::endl;
            return -1;
        }
        pdbg_target* pib = get_pib_target(hubchip);
        pdbg_target_probe(pib);
        if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
        {
            std::cout << "pib target for hub chip not enabled " << std::endl;
            return -1;
        }
        std::cout << "invoking sbe_istep 0 0 HWP" << std::endl;
        int ret = sbe_istep(pib, 0, 0);
        if (ret == 0)
        {
            std::cout << "succes in call to sbe_istep"
                      << std::endl;
        }
        break;
    }

    sleep(100);
    return 0;
}
