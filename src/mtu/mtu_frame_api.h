#pragma once
#include <vulkan/vulkan.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MTU_VkFrameInfo {
    VkDevice        device;
    VkQueue         graphicsQueue;
    VkCommandBuffer cmdBuffer;

    VkImage         srcImage;
    VkImageView     srcView;

    uint32_t        width;
    uint32_t        height;

    void*           userData;   // optional
} MTU_VkFrameInfo;

typedef void (*MTU_OnPresentFn)(const MTU_VkFrameInfo* frame);

#ifdef __cplusplus
}
#endif