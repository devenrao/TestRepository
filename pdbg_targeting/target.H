#pragma once

#include <generator>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace TARGETING
{
class Target;
using TargetPtr = std::shared_ptr<Target>;
using TargetPtrList = std::vector<TargetPtr>;

class Target : public std::enable_shared_from_this<Target>
{
  public:
    explicit Target(std::string name, int offset) :
        _name(std::move(name)), _offset(offset)
    {}

    static TargetPtr create(std::string name, int offset)
    {
        return std::make_shared<Target>(std::move(name), offset);
    }

    void addChild(const TargetPtr& child);

    [[nodiscard]] const std::vector<TargetPtr>& getChildren() const noexcept
    {
        return _children;
    }

    [[nodiscard]] TargetPtr getParent() const noexcept
    {
        return _parent;
    }

    [[nodiscard]] const std::string& getName() const noexcept
    {
        return _name;
    }

    [[nodiscard]] int getOffset() const noexcept
    {
        return _offset;
    }

    [[nodiscard]] std::generator<TargetPtr> ancestors() const;

  private:
    TargetPtr _parent{nullptr};
    std::string _name;
    int _offset{};
    std::vector<TargetPtr> _children;
};
} // namespace TARGETING
