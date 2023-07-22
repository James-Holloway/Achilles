#include "Profiling.h"
#include "Helpers.h"

Profiling::ProfilerBlock::ProfilerBlock(const std::wstring& _name, std::wstring _fullname) : name(_name), fullname(_fullname)
{
    start = std::chrono::steady_clock::now();
}

void Profiling::ProfilerBlock::Stop()
{
    auto end = std::chrono::steady_clock::now();
    duration = (end - start).count() * 1e-6f; // nano to milliseconds
    completed = true;
}

void Profiling::BeginBlock(const std::wstring& name)
{
    std::wstring fullname = L"";
    if (ProfilerBlockStack.size() > 0)
    {
        fullname += ProfilerBlockStack.top()->fullname + L".";
    }
    fullname += name;
    std::shared_ptr<ProfilerBlock> block = std::make_shared<ProfilerBlock>(name, fullname);
    ProfilerBlockStack.push(block);
}

void Profiling::EndBlock()
{
    std::shared_ptr<ProfilerBlock> block = ProfilerBlockStack.top();
    ProfilerBlockStack.pop();
    if (block == nullptr)
        return;
    block->Stop();
    ProfilerBlocksThisFrame.push_back(block);
}

void Profiling::ClearFrame()
{
    if (ProfilerShouldPrint)
    {
        Print();
        ProfilerShouldPrint = false;
    }

    ProfilerBlocksLastFrame.swap(ProfilerBlocksThisFrame);
    ProfilerBlocksThisFrame.clear();
}

void Profiling::Print()
{
    OutputDebugStringW(L"Profiler block printing:\n");
    for (auto block : ProfilerBlocksThisFrame)
    {
        if (block->completed)
            OutputDebugStringWFormatted(L"%s - %.1fms\n", block->fullname.c_str(), block->duration);
    }
    OutputDebugStringW(L"Profiler block printing end.\n\n");
}

ScopedTimer::ScopedTimer(const std::wstring& name)
{
#if _DEBUG
    Profiling::BeginBlock(name);
#endif
}

ScopedTimer::~ScopedTimer()
{
#if _DEBUG
    Profiling::EndBlock();
#endif
}
