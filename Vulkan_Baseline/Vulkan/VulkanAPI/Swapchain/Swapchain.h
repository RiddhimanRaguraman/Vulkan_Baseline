//-----------------------------------------------------------------
// Copyright 2026 by Riddhiman Raguraman
//-----------------------------------------------------------------

#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

// Vulkan + VMA types come from the force-included Framework.h (VULKAN FRAMEWORK section).

//---------------------------------------------------------------------------
// class Swapchain
//
// The swapchain: the ring of images the GPU renders into and the OS presents
// to the surface. This is the object that finally puts pixels on screen.
//
// Create() builds, in order:
//   * the VkSwapchainKHR (from the surface, FIFO/vsync present mode)
//   * the swapchain's color images + one image view each
//   * one render-complete semaphore per image (present synchronization)
//   * a depth image (allocated through VMA) + its image view
//
// It borrows the device / allocator handles so Destroy() can tear all of that
// down in reverse. The color images themselves are OWNED BY the swapchain --
// you never destroy them individually, only the swapchain.
//---------------------------------------------------------------------------

namespace Neelam::vk
{
	class Swapchain
	{
	public:
		//-----------------------------------------------------------------
		// Constructors / Destructors
		//-----------------------------------------------------------------
		Swapchain();
		Swapchain(const Swapchain &) = delete;
		Swapchain &operator = (const Swapchain &) = delete;
		~Swapchain();

		//-----------------------------------------------------------------
		// Lifetime
		//-----------------------------------------------------------------

		// width/height are a hint: on Windows the surface dictates the real
		// size (currentExtent), so they are only used as a fallback.
		void Create(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface,
			VmaAllocator allocator, uint32_t width, uint32_t height,
			VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM,
			VkFormat depthFormat = VK_FORMAT_D32_SFLOAT);

		void Destroy();

		//-----------------------------------------------------------------
		// Accessors
		//-----------------------------------------------------------------
		VkSwapchainKHR GetSwapchain() const;
		uint32_t       GetImageCount() const;
		VkImage        GetImage(uint32_t i) const;
		VkImageView    GetImageView(uint32_t i) const;
		VkSemaphore    GetRenderCompleteSemaphore(uint32_t i) const;
		VkImage        GetDepthImage() const;
		VkImageView    GetDepthImageView() const;
		VkExtent2D     GetExtent() const;
		VkFormat       GetColorFormat() const;
		VkFormat       GetDepthFormat() const;

	private:
		static const uint32_t privMaxImages = 8;

		// Borrowed handles, needed only so Destroy() can release what it made.
		VkDevice     privDevice;
		VmaAllocator privAllocator;

		// The swapchain + its per-image objects.
		VkSwapchainKHR privSwapchain;
		uint32_t       privImageCount;
		VkImage        privImages[privMaxImages];					// owned by the swapchain
		VkImageView    privImageViews[privMaxImages];
		VkSemaphore    privRenderCompleteSemaphores[privMaxImages];

		// Depth buffer (allocated via VMA).
		VkImage        privDepthImage;
		VmaAllocation  privDepthAllocation;
		VkImageView    privDepthImageView;

		// Cached config.
		VkExtent2D     privExtent;
		VkFormat       privColorFormat;
		VkFormat       privDepthFormat;
	};
}

#endif   // SWAPCHAIN_H

// ---  End of File ---
