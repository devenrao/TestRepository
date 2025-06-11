#include "target.H"
namespace TARGETING
{
#if __cplusplus >= 202302L
std::generator<TargetPtr> Target::ancestors() const
{
    auto p = _parent;
    while (p)
    {
        co_yield p;
        p = p->_parent;
    }
}
#endif
void Target::addChild(const TargetPtr& child)
{
    child->_parent = shared_from_this();
    _children.push_back(child);
}
} //namespace TARGETING
