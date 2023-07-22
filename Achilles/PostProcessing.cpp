#include "PostProcessing.h"
#include "Application.h"

PostProcessing::PostProcessing(float width, float height)
{
    PPBloom::CreateUAVs(width, height);
}

PostProcessing::~PostProcessing()
{
}

void PostProcessing::ApplyPostProcessing(std::shared_ptr<Texture> texture)
{
    std::shared_ptr<CommandQueue> commandQueue = Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    std::shared_ptr<CommandList> commandList = commandQueue->GetCommandList();

    if (EnableBloom)
    {
        ApplyBloom(commandList, texture);
    }
    commandList->FlushResourceBarriers();

    commandQueue->ExecuteCommandList(commandList);
    commandQueue->Flush();
}

void PostProcessing::Resize(uint32_t width, uint32_t height)
{
    PPBloom::ResizeUAVs(width, height);
}

void PostProcessing::ApplyBloom(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture)
{
    PPBloom::ApplyBloom(commandList, texture, BloomThreshold, BloomUpsampleFactor, BloomStrength);
}
