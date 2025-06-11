#pragma once

#include <target.H>
#include <cstdint>
#include <vector>
#include <dtree_loader.H>
namespace TARGETING
{
class Target;

class TargetService
{
  public:
    // Singleton accessor
    static TargetService& instance();

    // Public API
    void init(const std::string& dtbPath);

    [[nodiscard]] TargetPtr getTopLevelTarget() const noexcept
    {
        return _root;
    }

    TargetPtr getNextTarget(const TargetPtr& target) const noexcept;

    void* getFDT() const noexcept
    {
        return _loader ? _loader->fdt() : nullptr;
    }

#if __cplusplus >= 202302L
    std::generator<TargetPtr> getAllTargets(TargetPtr node = nullptr);
#endif
    TargetPtr toTarget(const EntityPath& i_entityPath) const;

  private:
    TargetService() = default;
    ~TargetService() = default;

    TargetService(const TargetService&) = delete;
    TargetService& operator=(const TargetService&) = delete;

    TargetPtr parseNode(const void* fdt, int offset,
                        TargetPtr parent = nullptr);

    size_t size() const noexcept
    {
        return _loader ? _loader->size() : 0;
    }
    [[nodiscard]] bool isInitialized() const noexcept
    {
        return _initialized;
    }
#if __cplusplus >= 202302L
    std::generator<TargetPtr> preOrderTraversal(TargetPtr node) const;
#endif
    TargetPtr _root = nullptr;
    std::unique_ptr<dtree::DeviceTreeLoader> _loader;
    bool _initialized{false};
};
} // namespace TARGETING
