#pragma once

#include "ShaderInclude.h"
#include "shaders/PPBloom.h"

class PostProcessing
{
public:
    // Bloom
    bool EnableBloom = true;
    float BloomThreshold = 4.0f; // Threshold luminacne above which a pixel will start to bloom
    float BloomStrength = 0.1f; // How much bloom is added back into the image
    float BloomUpsampleFactor = 0.65f; // Aka Scatter - Controls the focus of the blur, higher values spreading out more, causing more haze

protected:


public:
    PostProcessing(float width, float height);
    virtual ~PostProcessing();

    virtual void ApplyPostProcessing(std::shared_ptr<Texture> texture);
    virtual void Resize(uint32_t width, uint32_t height);

protected:
    virtual void ApplyBloom(std::shared_ptr<CommandList> commandList, std::shared_ptr<Texture> texture);
};

