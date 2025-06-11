#pragma once

#include <target.H>

#include <cstdint>
#include <vector>
#include <dt_loader.H>
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

    [[nodiscard]] bool isInitialized() const noexcept
    {
        return _initialized;
    }

    [[nodiscard]] TargetPtr getTopLevelTarget() const noexcept
    {
        return _root;
    }

    TargetPtr getNextTarget(const TargetPtr& target) const noexcept;

    void* fdt() const noexcept
    {
        return _loader->fdt();
    }
    size_t size() const noexcept
    {
        return _loader->size();
    }

  private:
    TargetService() = default;
    ~TargetService() = default;

    TargetService(const TargetService&) = delete;
    TargetService& operator=(const TargetService&) = delete;

    TargetPtr parseNode(const void* fdt, int offset,
                        TargetPtr parent = nullptr);
    std::generator<TargetPtr> preOrderTraversal(TargetPtr node) const;
    TargetPtr _root = nullptr;
    std::unique_ptr<dtree::DeviceTreeLoader> _loader;
    bool _initialized{false};
};

} // namespace TARGETING
