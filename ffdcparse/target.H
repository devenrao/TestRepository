#pragma once

#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include <generator>

namespace TARGETING
{
class Target;
using TargetPtr = std::shared_ptr<Target>;
using TargetPtrList = std::vector<TargetPtr>;

struct AncestorIterable;
class AncestorIterator;

class Target : public std::enable_shared_from_this<Target>
{
  public:
    explicit Target(std::string name, int offset) :
        name_(std::move(name)), offset_(offset)
    {}

    static TargetPtr create(std::string name, int offset)
    {
        return std::make_shared<Target>(std::move(name), offset);
    }

    void addChild(const TargetPtr& child)
    {
        child->parent_ = shared_from_this();
        children_.push_back(child);
    }

    [[nodiscard]] const std::vector<TargetPtr>& getChildren() const noexcept
    {
        return children_;
    }

    [[nodiscard]] TargetPtr getParent() const noexcept
    {
        return parent_;
    }

    [[nodiscard]] const std::string& getName() const noexcept
    {
        return name_;
    }

    [[nodiscard]] int getOffset() const noexcept
    {
        return offset_;
    }

    [[nodiscard]] std::generator<Ptr> ancestors() const
    {
        auto p = parent_;
        while (p)
        {
            co_yield p;
            p = p->parent_;
        }
    }

    template<ATTRIBUTE_ID A>
    [[nodiscard]] typename AttributeTraits<A>::Type getAttr(ATTRIBUTE_ID* i_pAttrId) const;


    template<ATTRIBUTE_ID A>
    void setAttr(const typename AttributeTraits<A>::Type& i_attrValue);

  private:
    TargetPtr parent_{nullptr};
    std::string name_;
    int offset_{};
    std::vector<TargetPtr> children_;
};

template<ATTRIBUTE_ID A>
[[nodiscard]] typename AttributeTraits<A>::Type Target::getAttr() const
{
    typename AttributeTraits<A>::Type l_attrValue;
    if (!tryGetAttr<A>(l_attrValue))
    {
        targAssert(GET_ATTR, A);
    }
    return l_attrValue;
}

template<ATTRIBUTE_ID A>
void Target::setAttr(const typename AttributeTraits<A>::Type& i_attrValue)
{
    if (!trySetAttr<A>(i_attrValue))
    {
        targAssert(SET_ATTR, A);
    }
}

// For C++23: used std::generator and remove this AncestorIterator
#if 0
class AncestorIterator
{
  public:
    using iterator_category = std::input_iterator_tag;
    using value_type = TargetPtr;
    using difference_type = std::ptrdiff_t;

    explicit AncestorIterator(TargetPtr current) : current_(current) {}

    value_type operator*() const
    {
        return current_;
    }

    AncestorIterator& operator++()
    {
        current_ = current_ ? current_->parent_ : nullptr;
        return *this;
    }

    bool operator==(const AncestorIterator& other) const
    {
        return current_ == other.current_;
    }
    bool operator!=(const AncestorIterator& other) const
    {
        return !(*this == other);
    }

  private:
    TargetPtr current_;
};

struct AncestorIterable
{
    TargetPtr start_;
    AncestorIterator begin() const
    {
        return AncestorIterator(start_->parent_);
    }
    AncestorIterator end() const
    {
        return AncestorIterator(nullptr);
    }
};
#endif
} //TARGETING
