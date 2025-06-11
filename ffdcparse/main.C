#include "target.H"
#include "target_service.H"
#include <iostream>
#include "attributetraits.H"
#include "attributestructs.H"
#include "attributeenums.H"
#include <iomanip>
extern "C"
{
#include <libfdt.h>
}
void printHwasState(const TARGETING::HwasState& state)
{
    std::cout << "  present: " << +state.present
              << ", functional: " << +state.functional
              << ", poweredOn: " << +state.poweredOn << "\n";
}

void traverse(const TARGETING::TargetPtr& node)
{
    TARGETING::HwasState hwas{};
    if (node->tryGetAttr<TARGETING::ATTR_HWAS_STATE>(hwas))
    {
        printHwasState(hwas);
    }
    else
    {
        std::cout << "  [ATTR_HWAS_STATE not found]\n";
    }

    TARGETING::AttributeTraits<TARGETING::ATTR_PHYS_DEV_PATH>::Type physDevPath{};
    if (node->tryGetAttr<TARGETING::ATTR_PHYS_DEV_PATH>(physDevPath))
    {
        std::cout << "Phys dev path: " << physDevPath << "\n";
    }

    for (const auto& child : node->getChildren())
    {
        traverse(child);
    }
}

// Helper to get full path of a node
int get_full_path(const void *fdt, int nodeoffset, char *out_path, size_t out_size)
{
    if (nodeoffset < 0 || !out_path || out_size == 0)
        return -1;

    char temp[1024] = {0};

    while (nodeoffset >= 0)
    {
        int len;
        const char *name = fdt_get_name(fdt, nodeoffset, &len);
        if (!name)
            return -1;

        // Prepend the current node name
        char new_path[1024];
        if (strcmp(temp, "") == 0)
            snprintf(new_path, sizeof(new_path), "/%s", name);
        else
            snprintf(new_path, sizeof(new_path), "/%s%s", name, temp);

        strncpy(temp, new_path, sizeof(temp) - 1);
        nodeoffset = fdt_parent_offset(fdt, nodeoffset);
    }

    strncpy(out_path, temp, out_size - 1);
    return 0;
}

// Walk the entire tree and print full paths
void print_all_node_paths(const void *fdt)
{
    int offset = -1;
    char path[1024];

    while ((offset = fdt_next_node(fdt, offset, NULL)) >= 0)
    {
        if (get_full_path(fdt, offset, path, sizeof(path)) == 0)
        {
            printf("Node offset %d: %s\n", offset, path);
        }
        else
        {
            printf("Failed to get path for node %d\n", offset);
        }
    }
}

void printPropertiesOfNode(const void* fdt, const char* nodePath)
{
    int offset = fdt_path_offset(fdt, nodePath);
    if (offset < 0)
    {
        std::cerr << "Node not found: " << nodePath << "\n";
        return;
    }

    std::cout << "Properties for node: " << nodePath << "\n";

    int prop_offset;
    int len = 0;
    const char* name = nullptr;

    fdt_for_each_property_offset(prop_offset, fdt, offset)
    {
        const void* data = fdt_getprop_by_offset(fdt, prop_offset, &name, &len);
        if (!data || !name || len <= 0)
            continue;

        std::cout << "  " << name << " = ";
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        for (int i = 0; i < len; ++i)
        {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << " ";
        }
        std::cout << std::dec << "\n";
    }
}

int main()
{
    try
    {
        auto& ts = TARGETING::TargetService::instance();
        ts.init("/tmp/rainest.dtb");

        // Print all nodes
        //traverse(ts.getTopLevelTarget());
        //print_all_node_paths(ts.getFDT());
        //printPropertiesOfNode(ts.getFDT(), "//backplane0/proc_module0/hub_chip0");
        for (TARGETING::TargetPtr target : ts.getAllTargets())
        {
            TARGETING::AttributeTraits<TARGETING::ATTR_PHYS_DEV_PATH>::Type physDevPath{};
            if (target->tryGetAttr<TARGETING::ATTR_PHYS_DEV_PATH>(physDevPath))
            {
                std::string safePath(physDevPath, strnlen(physDevPath, sizeof(physDevPath)));
                std::cout << "Phys dev path: " << safePath << "\n";
            }
            TARGETING::HwasState hwas{};
            if (target->tryGetAttr<TARGETING::ATTR_HWAS_STATE>(hwas))
            {
                printHwasState(hwas);
            }            
       }
    }
    catch (std::exception& ex)
    {
        std::cout << "exception raised " << ex.what() << std::endl;
    }
}
