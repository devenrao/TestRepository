#pragma once

#include <libfdt.h>

#include <stdexcept>
#include <string>
/*
class DeviceTreeEditor
{
  public:
    DeviceTreeEditor(void* fdt) : _fdt(fdt) {}

    void setStringProperty(int nodeOffset, const std::string& propName,
                           const std::string& value)
    {
        int rc = fdt_setprop_string(_fdt, nodeOffset, propName.c_str(),
                                    value.c_str());
        if (rc != 0)
            throw std::runtime_error("Failed to set property: " + propName);
    }

    void setUint32Property(int nodeOffset, const std::string& propName,
                           uint32_t value)
    {
        uint32_t beValue = htobe32(value);
        int rc = fdt_setprop(_fdt, nodeOffset, propName.c_str(), &beValue,
                             sizeof(beValue));
        if (rc != 0)
            throw std::runtime_error("Failed to set property: " + propName);
    }

  private:
    void* _fdt;
};
*/
