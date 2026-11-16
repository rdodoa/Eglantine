// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <optional>

#include <catch2/catch_test_macros.hpp>

#include "common/common_types.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "video_core/vulkan_common/vulkan_memory_allocator.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

// #262: a staging buffer the device had no memory for threw from the GPU thread and aborted.
// Runs on any Vulkan driver (lavapipe in CI) and skips without one.
TEST_CASE("Vulkan buffers: TryCreateBuffer returns a null buffer when no heap can hold it",
          "[vulkan_buffer_oom]") {
    const auto library = Vulkan::OpenLibrary();
    if (!library) {
        SKIP("No Vulkan library");
    }
    Vulkan::vk::InstanceDispatch dld;
    std::optional<Vulkan::vk::Instance> instance;
    try {
        instance.emplace(Vulkan::CreateInstance(*library, dld, VK_API_VERSION_1_1));
    } catch (const Vulkan::vk::Exception&) {
        SKIP("No Vulkan instance");
    }
    std::optional<Vulkan::Device> device;
    for (const VkPhysicalDevice physical : instance->EnumeratePhysicalDevices()) {
        try {
            device.emplace(**instance, Vulkan::vk::PhysicalDevice(physical, dld), VK_NULL_HANDLE,
                           dld);
            break;
        } catch (const Vulkan::vk::Exception&) {
            continue;
        }
    }
    if (!device) {
        SKIP("No Vulkan device");
    }
    INFO(device->GetDriverName() << " " << device->GetModelName());

    const VkPhysicalDeviceMemoryProperties memory =
        device->GetPhysical().GetMemoryProperties().memoryProperties;
    VkDeviceSize largest_heap = 0;
    for (u32 i = 0; i < memory.memoryHeapCount; ++i) {
        largest_heap = (std::max)(largest_heap, memory.memoryHeaps[i].size);
    }
    const VkBufferCreateInfo too_large{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = largest_heap * 2,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    Vulkan::MemoryAllocator allocator(*device);

    REQUIRE_FALSE(allocator.TryCreateBuffer(too_large, Vulkan::MemoryUsage::Upload));
    REQUIRE_THROWS_AS(allocator.CreateBuffer(too_large, Vulkan::MemoryUsage::Upload),
                      Vulkan::vk::Exception);

    VkBufferCreateInfo fits = too_large;
    fits.size = 64 * 1024;
    const Vulkan::vk::Buffer buffer = allocator.TryCreateBuffer(fits, Vulkan::MemoryUsage::Upload);
    REQUIRE(buffer);
    REQUIRE(buffer.IsHostVisible());
}
