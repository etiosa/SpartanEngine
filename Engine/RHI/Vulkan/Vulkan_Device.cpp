/*
Copyright(c) 2016-2019 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= IMPLEMENTATION ===============
#include "../RHI_Implementation.h"
#ifdef API_GRAPHICS_VULKAN 
//================================

//= INCLUDES ==================
#include "../RHI_Device.h"
#include "../../Math/Vector4.h"
#include "../../Logging/Log.h"
#include "../../Core/Settings.h"
#include <string>
//=============================

//= NAMESPACES ================
using namespace std;
using namespace Directus::Math;
//=============================

namespace Directus
{
	namespace Vulkan_Device
	{
		inline bool AcquireValidationLayers(const std::vector<const char*>& validationLayers)
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : validationLayers)
			{
				for (const auto& layerProperties : availableLayers)
				{
					if (strcmp(layerName, layerProperties.layerName) == 0)
					{
						return true;
					}
				}
			}

			LOG_ERROR("Validation layer was requested, but not available.");
			return false;
		}

		inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback)
		{
			if (auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))
				return func(instance, pCreateInfo, pAllocator, pCallback);

			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}

		inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) 
		{
			if (auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")) 
			{
				func(instance, callback, pAllocator);
			}
		}

		static inline VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData) 
		{
			Log_Type type	= Log_Info;
			type			= messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT	? Log_Warning	: type;
			type			= messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT		? Log_Error		: type;
			Log::Write("Vulkan: " + string(pCallbackData->pMessage), type);

			return VK_FALSE;
		}

		inline bool isDeviceSuitable(VkPhysicalDevice device) 
		{
			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(device, &deviceProperties);
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || VK_PHYSICAL_DEVICE_TYPE_CPU;
		}

		VkInstance instance;
		VkPhysicalDevice device;
		vector<const char*> validationLayers	= { "VK_LAYER_LUNARG_standard_validation" };
		#ifdef DEBUG
		const bool validationLayerEnabled		= true;
		vector<const char*> extensions			= { "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
		#else
		const bool validationLayerEnabled = false;
		vector<const char*> extensions			= { "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
		#endif
		VkDebugUtilsMessengerEXT callback;
	}

	RHI_Device::RHI_Device(void* drawHandle)
	{
		// Validation layer
		bool validationLayerAvailable = false;
		if (Vulkan_Device::validationLayerEnabled)
		{
			validationLayerAvailable = Vulkan_Device::AcquireValidationLayers(Vulkan_Device::validationLayers);
		}
		
		// Create instance
		{
			VkApplicationInfo appInfo	= {};
			appInfo.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pApplicationName	= ENGINE_VERSION;
			appInfo.applicationVersion	= VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName			= ENGINE_VERSION;
			appInfo.engineVersion		= VK_MAKE_VERSION(1, 0, 0);
			appInfo.apiVersion			= VK_API_VERSION_1_1;

			VkInstanceCreateInfo createInfo		= {};
			createInfo.sType					= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo			= &appInfo;
			createInfo.enabledExtensionCount	= (uint32_t)Vulkan_Device::extensions.size();
			createInfo.ppEnabledExtensionNames	= Vulkan_Device::extensions.data();
			if (validationLayerAvailable) 
			{
				createInfo.enabledLayerCount	= (uint32_t)Vulkan_Device::validationLayers.size();
				createInfo.ppEnabledLayerNames	= Vulkan_Device::validationLayers.data();
			}
			else 
			{
				createInfo.enabledLayerCount = 0;
			}

			auto result = vkCreateInstance(&createInfo, nullptr, &Vulkan_Device::instance);
			if (result != VK_SUCCESS)
			{
				LOG_ERROR("Failed to create instance.");
				return;
			}
		}
		
		// Get available extensions
		{
			uint32_t extensionCount = 0;
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
			for (const auto& extension : extensions)
			{
				LOGF_INFO("Available extension: %s", extension.extensionName);
			}
		}

		// Callback
		if (Vulkan_Device::validationLayerEnabled)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType			= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity	= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType		= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback	= Vulkan_Device::debugCallback;
			createInfo.pUserData		= nullptr; // Optional

			if (Vulkan_Device::CreateDebugUtilsMessengerEXT(Vulkan_Device::instance, &createInfo, nullptr, &Vulkan_Device::callback) != VK_SUCCESS) 
			{
				LOG_ERROR("Failed to setup debug callback");
			}
		}

		// Device
		{
			Vulkan_Device::device	= VK_NULL_HANDLE;
			uint32_t deviceCount	= 0;
			vkEnumeratePhysicalDevices(Vulkan_Device::instance, &deviceCount, nullptr);
			if (deviceCount == 0) 
			{
				LOG_ERROR("Failed to enumerate physical devices.");
				return;
			}
			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(Vulkan_Device::instance, &deviceCount, devices.data());
			
			for (const auto& device : devices) 
			{
				if (Vulkan_Device::isDeviceSuitable(device))
				{
					Vulkan_Device::device = device;
					break;
				}
			}

			if (Vulkan_Device::device == VK_NULL_HANDLE) 
			{
				LOG_ERROR("Failed to find a suitable device.");
				return;
			}
		}

		Settings::Get().m_versionGraphicsAPI = to_string(VK_API_VERSION_1_0);
		LOG_INFO(Settings::Get().m_versionGraphicsAPI);
		m_initialized = true;
	}

	RHI_Device::~RHI_Device()
	{	
		if (Vulkan_Device::validationLayerEnabled) Vulkan_Device::DestroyDebugUtilsMessengerEXT(Vulkan_Device::instance, Vulkan_Device::callback, nullptr);
		vkDestroyInstance(Vulkan_Device::instance, nullptr);
	}

	bool RHI_Device::Draw(unsigned int vertex_count) const
	{
		return true;
	}

	bool RHI_Device::DrawIndexed(const unsigned int index_count, const unsigned int index_offset, const unsigned int vertex_offset) const
	{
		return true;
	}

	bool RHI_Device::ClearRenderTarget(void* render_target, const Vector4& color) const
	{
		return true;
	}

	bool RHI_Device::ClearDepthStencil(void* depth_stencil, const unsigned int flags, const float depth, const unsigned int stencil) const
	{
		return true;
	}

	bool RHI_Device::SetVertexBuffer(const std::shared_ptr<RHI_VertexBuffer>& buffer) const
	{
		return true;
	}

	bool RHI_Device::SetIndexBuffer(const std::shared_ptr<RHI_IndexBuffer>& buffer) const
	{
		return true;
	}

	bool RHI_Device::SetVertexShader(const std::shared_ptr<RHI_Shader>& shader) const
	{
		return true;
	}

	bool RHI_Device::SetPixelShader(const std::shared_ptr<RHI_Shader>& shader) const
	{
		return true;
	}

	bool RHI_Device::SetConstantBuffers(const unsigned int start_slot, const unsigned int buffer_count, void* buffer, const RHI_Buffer_Scope scope) const
	{
		return true;
	}

	bool RHI_Device::SetSamplers(const unsigned int start_slot, const unsigned int sampler_count, void* samplers) const
	{
		return true;
	}

	bool RHI_Device::SetRenderTargets(const unsigned int render_target_count, void* render_targets, void* depth_stencil) const
	{
		return true;
	}

	bool RHI_Device::SetTextures(const unsigned int start_slot, const unsigned int resource_count, void* shader_resources) const
	{
		return true;
	}

	bool RHI_Device::SetViewport(const RHI_Viewport& viewport) const
	{
		return true;
	}

	bool RHI_Device::SetScissorRectangle(const Math::Rectangle& rectangle) const
	{
		return true;
	}

	bool RHI_Device::SetDepthStencilState(const std::shared_ptr<RHI_DepthStencilState>& depth_stencil_state) const
	{
		return true;
	}

	bool RHI_Device::SetBlendState(const std::shared_ptr<RHI_BlendState>& blend_state) const
	{
		return true;
	}

	bool RHI_Device::SetPrimitiveTopology(const RHI_PrimitiveTopology_Mode primitive_topology) const
	{
		return true;
	}

	bool RHI_Device::SetInputLayout(const std::shared_ptr<RHI_InputLayout>& input_layout) const
	{
		return true;
	}

	bool RHI_Device::SetRasterizerState(const std::shared_ptr<RHI_RasterizerState>& rasterizer_state) const
	{
		return true;
	}

	void RHI_Device::EventBegin(const std::string& name)
	{

	}

	void RHI_Device::EventEnd()
	{

	}

	bool RHI_Device::ProfilingCreateQuery(void** query, const RHI_Query_Type type) const
	{
		return true;
	}

	bool RHI_Device::ProfilingQueryStart(void* query_object) const
	{
		return true;
	}

	bool RHI_Device::ProfilingQueryEnd(void* query_object) const
	{
		return true;
	}

	bool RHI_Device::ProfilingGetTimeStamp(void* query_disjoint) const
	{
		return true;
	}

	float RHI_Device::ProfilingGetDuration(void* query_disjoint, void* query_start, void* query_end) const
	{
		return 0.0f;
	}

	void RHI_Device::DetectPrimaryAdapter(RHI_Format format) const
	{

	}
}
#endif