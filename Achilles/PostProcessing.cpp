#include "PostProcessing.h"
#include "Application.h"

PostProcessing::PostProcessing(float width, float height)
{
    PPBloom::CreateUAVs(width, height);
}

PostProcessing::~PostProcessing()
{
}

void PostProcessing::ApplyPostProcessing(std::shared_ptr<Texture> texture, std::shared_ptr<Texture> presentTexture, bool postProcessingEnable)
{
    std::shared_ptr<CommandQueue> commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

    // Apply present conversion from 16 bit float HDR down to 8 bit unorm SDR
    if (!postProcessingEnable)
    {
        ApplyPresentConversion(commandList, texture, presentTexture);
        commandQueue->ExecuteCommandList(commandList);
        return;
    }

    // Bloom
    if (EnableBloom)
    {
        ApplyBloom(commandList, texture);
    }
    commandList->FlushResourceBarriers();

    // Exposure

    // Tone Mapping
    ApplyToneMapping(commandList, texture, presentTexture);

    // Gamma Correction
    if (EnableGammaCorrection)
    {
        ApplyGammaCorrection(commandList, presentTexture);
    }

    commandQueue->ExecuteCommandList(commandList);
}

void PostProcessing::Resize(uint32_t width, uint32_t height)
{
    PPBloom::ResizeUAVs(width, height);
}

void PostProcessing::ApplyBloom(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture)
{
    ScopedTimer _prof(L"Bloom");
    PPBloom::ApplyBloom(commandList, texture, BloomThreshold, BloomUpsampleFactor, BloomStrength);
}

void PostProcessing::ApplyToneMapping(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, std::shared_ptr<Texture> presentTexture)
{
    ScopedTimer _prof(L"Tone Mapping");
    // Always apply tone mapping to convert down to presentTexture's format. If tone mapping is disabled then pass a none tonemapper
    PPToneMapping::ApplyToneMapping(commandList, texture, presentTexture, EnableToneMapping ? ToneMapper : PPToneMapping::ToneMappers::None);
}

void PostProcessing::ApplyGammaCorrection(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> presentTexture)
{
    ScopedTimer _prof(L"Gamma Correction");
    PPGammaCorrection::ApplyGammaCorrection(commandList, presentTexture, GammaCorrection);
}

void PostProcessing::ApplyPresentConversion(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture, std::shared_ptr<Texture> presentTexture)
{
    ScopedTimer _prof(L"Present Conversion");
    // Tone mapping converts, so let's just use that instead
    PPToneMapping::ApplyToneMapping(commandList, texture, presentTexture, PPToneMapping::ToneMappers::None);
}