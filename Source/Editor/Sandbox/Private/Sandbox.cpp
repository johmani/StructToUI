#include "Sandbox.h"
#include "ImExtensions/ImExtra.h"
#include "Core/EntryPoint.h"

namespace ImGui {

    template<typename T>
    void Struct(T& type)
    {
        const Meta::Type* metaType = Meta::Sandbox::Type<T>();

        if (!metaType)
            return;

        if (ImField::BeginBlock(metaType->name.data()))
        {
            if (ImGui::BeginTable(metaType->name.data(), 2, ImGuiTableFlags_SizingFixedFit))
            {
                for (const Meta::Field& field : metaType->Fields())
                {
                    Meta::Range range(-FLT_MAX, FLT_MAX);
                    Meta::UI ui = Meta::UI::Default;
                    Meta::Color color;
                    bool hasColor = false;

                    for (const Meta::Attribute& att : field.Attributes())
                    {
                        if (att.type == Meta::Attribute::Type::Range)
                            range = att.range;

                        if (att.type == Meta::Attribute::Type::UI)
                            ui = att.ui;

                        if (att.type == Meta::Attribute::Type::Color)
                        {
                            color = att.color;
                            hasColor = true;
                        }
                    }

                    if (hasColor)
                    {
                        ImGui::PushStyleColor(ImGuiCol_FrameBg,        { color.r       , color.g       , color.b       , color.a });
                        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, { color.r * 0.9f, color.g * 0.9f, color.b * 0.8f, color.a });
                        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  { color.r * 0.8f, color.g * 0.8f, color.b * 0.8f, color.a });
                    }

                    switch (field.type)
                    {
                    case Meta::FieldType::Float:
                    {
                        auto& v = field.Value<float>(type);

                        switch (ui)
                        {
                        case Meta::UI::Default:
                        case Meta::UI::Drag:
                            ImField::DragFloat(field.name.data(), &v, 0.01f, range.min, range.max);
                            break;
                        case Meta::UI::Slider:
                            ImField::SliderFloat(field.name.data(), &v, range.min, range.max);
                            break;
                        case Meta::UI::Text:
                            ImField::Text(field.name.data(), "%.3f", v);
                            break;
                        }
                       
                        break;
                    }
                    case Meta::FieldType::Float2:
                    {
                        auto& v = field.Value<Math::float2>(type);

                        switch (ui)
                        {
                        case Meta::UI::Default:
                        case Meta::UI::Drag:
                            ImField::DragFloat2(field.name.data(), &v.x, 0.01f, range.min, range.max);
                            break;
                        case Meta::UI::Slider:
                            ImField::SliderFloat2(field.name.data(), &v.x, range.min, range.max);
                            break;
                        case Meta::UI::Text:
                            ImField::Text(field.name.data(), "%.3f, %.3f", v.x, v.y);
                            break;
                        }

                        break;
                    }
                    case Meta::FieldType::Float3:
                    {
                        auto& v = field.Value<Math::float3>(type);

                        switch (ui)
                        {
                        case Meta::UI::Default:
                        case Meta::UI::Drag:
                            ImField::DragFloat3(field.name.data(), &v.x, 0.01f, range.min, range.max);
                            break;
                        case Meta::UI::Slider:
                            ImField::SliderFloat3(field.name.data(), &v.x, range.min, range.max);
                            break;
                        case Meta::UI::Text:
                            ImField::Text(field.name.data(), "%.3f, %.3f, %.3f", v.x, v.y, v.z);
                            break;
                        }

                        break;
                    }
                    case Meta::FieldType::Float4:
                    {
                        auto& v = field.Value<Math::float4>(type);

                        switch (ui)
                        {
                        case Meta::UI::Default:
                        case Meta::UI::Drag:
                            ImField::DragFloat4(field.name.data(), &v.x, 0.01f, range.min, range.max);
                            break;
                        case Meta::UI::Slider:
                            ImField::SliderFloat4(field.name.data(), &v.x, range.min, range.max);
                            break;
                        case Meta::UI::Text:
                            ImField::Text(field.name.data(), "%.3f, %.3f, %.3f, %.3f", v.x, v.y, v.z, v.w);
                            break;
                        }

                        break;
                    }
                    case Meta::FieldType::Bool:
                    {
                        auto& v = field.Value<bool>(type);

                        switch (ui)
                        {
                        case Meta::UI::Default:   
                            ImField::Checkbox(field.name.data(), &v);
                            break;
                        case Meta::UI::Drag:   break;
                        case Meta::UI::Slider: break;
                        case Meta::UI::Text:
                            ImField::Text(field.name.data(), "%s", (v ? "true" : "false"));
                            break;
                        default :    
                            ImField::Checkbox(field.name.data(), &v);
                        }

                        break;
                    }
                    }

                    if (hasColor)
                        ImGui::PopStyleColor(3);
                }

                ImGui::EndTable();
            }
        }
        ImField::EndBlock();
    }
}

struct AppLayer : Core::Layer
{
    nvrhi::DeviceHandle device;
    nvrhi::CommandListHandle commandList;

    Sandbox::Entity entity;
    Sandbox::Camera camera;

    void OnUpdate(const Core::FrameInfo& info) override
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

        ImGui::Begin("Auto UI");

        ImGui::Struct(entity);
        ImGui::Struct(camera);

        ImGui::End();
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


