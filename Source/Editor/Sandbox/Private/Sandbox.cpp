#include "Core/Core.h"
#include "Core/EntryPoint.h"

struct AppLayer : Core::Layer
{
    nvrhi::DeviceHandle device;
    nvrhi::CommandListHandle commandList;

    void OnUpdate(const Core::FrameInfo& info) override
    {
       
    }

    void OnAttach() override
    {
        device = RHI::GetDevice();
        commandList = device->createCommandList();

        Plugins::LoadPluginsInDirectory("Plugins");
    }

    void OnDetach() override
    {

    }

    void OnBegin(const Core::FrameInfo& info) override
    {
        commandList->open();
        nvrhi::utils::ClearColorAttachment(commandList, info.fb, 0, nvrhi::Color(0.1f));
    }

    void OnEnd(const Core::FrameInfo& info) override
    {
        commandList->close();
        device->executeCommandList(commandList);
    }
};

Application::ApplicationContext* Application::CreateApplication(ApplicationCommandLineArgs args)
{
    Application::ApplicationDesc desc;
    desc.commandLineArgs = args;

#ifdef HE_DIST
    if (args.count == 2)
        desc.workingDirectory = std::filesystem::path(args[0]).parent_path();
#endif

#if CORE_DEBUG
    desc.deviceDesc.enableGPUValidation = true;
    desc.deviceDesc.enableDebugRuntime = true;
    desc.deviceDesc.enableNvrhiValidationLayer = true;
#endif

    desc.deviceDesc.enableRayTracingExtensions = true;
    desc.deviceDesc.enableComputeQueue = true;
    desc.deviceDesc.enableCopyQueue = true;
    desc.deviceDesc.api = {
        nvrhi::GraphicsAPI::D3D11,
        nvrhi::GraphicsAPI::D3D12,
        nvrhi::GraphicsAPI::VULKAN,
    };

    desc.windowDesc.title = "Sandbox";
    desc.windowDesc.minWidth = 960;
    desc.windowDesc.minHeight = 540;
    desc.windowDesc.swapChainDesc.swapChainFormat = nvrhi::Format::SRGBA8_UNORM;

    desc.logFile = FileSystem::GetAppDataPath(desc.windowDesc.title) / (desc.windowDesc.title + ".log");

    ApplicationContext* ctx = new ApplicationContext(desc);
    Application::PushLayer(new AppLayer());

    return ctx;
}
