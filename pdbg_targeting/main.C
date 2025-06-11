#include "target.H"
#include "target_service.H"
#include <iostream>
void traverse(const TARGETING::TargetPtr& node)
{
    std::cout << node->getName() << "\n";
    for (const auto& child : node->getChildren())
        traverse(child);
}

int main()
{
    try
    {
        auto& ts = TARGETING::TargetService::instance();
        ts.init("p12_fake.dtb");

        // Print all nodes
        traverse(ts.getTopLevelTarget());
    }
    catch (std::exception& ex)
    {
        std::cout << "exception raised " << ex.what() << std::endl;
    }
}
