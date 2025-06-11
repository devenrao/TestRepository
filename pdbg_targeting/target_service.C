#include <target_service.H>
extern "C"
{
#include <libfdt.h>
}
#include <fstream>
#include <generator>
#include <vector>

namespace TARGETING
{
// Recursive generator for pre-order traversal
TargetService& TargetService::instance()
{
    static TargetService service;
    return service;
}

void TargetService::init(const std::string& dtbPath)
{
    if (_initialized)
        return;

    _loader = std::make_unique<dtree::DeviceTreeLoader>(dtbPath);
    void* fdt = _loader->fdt();

    int rootOffset = fdt_path_offset(fdt, "/");
    if (rootOffset < 0)
        throw std::runtime_error("Failed to find root node");

    _root = parseNode(fdt, rootOffset);
    _initialized = true;
}

TargetPtr TargetService::getNextTarget(const TargetPtr& target) const noexcept
{
    bool found = false;

    for (const auto& targetPtr : preOrderTraversal(target))
    {
        if (found)
            return targetPtr;

        if (targetPtr == target)
            found = true;
    }

    return nullptr;
}

TargetPtr TargetService::parseNode(const void* fdt, int offset, TargetPtr parent)
{
    const char* name = fdt_get_name(fdt, offset, nullptr);
    auto node = Target::create(name, offset);

    if (parent)
        parent->addChild(node);

    int child;
    fdt_for_each_subnode(child, fdt, offset) parseNode(fdt, child, node);

    return node;
}

std::generator<TargetPtr> TargetService::preOrderTraversal(TargetPtr node) const
{
    if (!node)
        co_return;

    co_yield node;

    for (const auto& child : node->getChildren())
    {
        for (auto descendant : preOrderTraversal(child))
        {
            co_yield descendant;
        }
    }
}
} // namespace TARGETING
