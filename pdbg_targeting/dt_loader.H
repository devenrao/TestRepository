#pragma once

#include <string>
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <unistd.h>
// #include <stdexcept>
namespace dtree
{
class DeviceTreeLoader
{
  public:
    DeviceTreeLoader(const std::string& path);
    ~DeviceTreeLoader();

    DeviceTreeLoader(const DeviceTreeLoader&) = delete;
    DeviceTreeLoader& operator=(const DeviceTreeLoader&) = delete;
    DeviceTreeLoader(DeviceTreeLoader&& other) = delete;
    DeviceTreeLoader& operator=(DeviceTreeLoader&& other) = delete;

    void* fdt() const noexcept
    {
        return _fdt;
    }
    size_t size() const noexcept
    {
        return _size;
    }

  private:
    int _fd{-1};
    void* _fdt{nullptr};
    size_t _size{0};
};
} // namespace dtree
