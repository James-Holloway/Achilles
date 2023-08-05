#include "Profiling.h"
#include "Helpers.h"

Profiling::ProfilerBlock::ProfilerBlock(const std::wstring& _name, std::wstring _fullname) : name(_name), fullname(_fullname)
{
    start = std::chrono::steady_clock::now();
}

void Profiling::ProfilerBlock::Stop()
{
    auto end = std::chrono::steady_clock::now();
    duration += (end - start).count() * 1e-6f; // nano to milliseconds
    completed = true;
    count++;
}

void Profiling::BeginBlock(const std::wstring& name)
{
    std::wstring fullname = L"";
    bool topLevel = false;
    if (ProfilerBlockStack.size() > 0)
    {
        fullname += ProfilerBlockStack.top()->fullname + L".";
    }
    else
    {
        topLevel = true;
    }
    fullname += name;

    // Attempt to get a previously existing block
    auto iter = ProfilerBlocksByFullname.find(fullname);
    if (iter != ProfilerBlocksByFullname.end()) // block already exists
    {
        std::shared_ptr<ProfilerBlock> block = iter->second;
        block->start = std::chrono::steady_clock::now();
        ProfilerBlockStack.push(block);
    }
    else // no block found, create a new one
    {
        std::shared_ptr<ProfilerBlock> block = std::make_shared<ProfilerBlock>(name, fullname);
        block->topLevel = topLevel;
        ProfilerBlockStack.push(block);
        ProfilerBlocksByFullname[fullname] = block;
    }
}

void Profiling::EndBlock()
{
    std::shared_ptr<ProfilerBlock> block = ProfilerBlockStack.top();
    ProfilerBlockStack.pop();
    if (block == nullptr)
        return;
    block->Stop();
}

void Profiling::ClearFrame()
{
    if (ProfilerShouldPrint)
    {
        Print();
        ProfilerShouldPrint = false;
    }

    ProfilerBlocksByFullname.clear();
}

void Profiling::Print()
{
    OutputDebugStringW(L"Profiler block printing:\n");
    double totalDuration = 0;
    for (auto iter : ProfilerBlocksByFullname)
    {
        auto block = iter.second;
        if (block->completed)
        {
            if (block->count <= 1)
                OutputDebugStringWFormatted(L"%s - %.2fms\n", block->fullname.c_str(), block->duration);
            else
            {
                double average = block->duration / block->count;
                OutputDebugStringWFormatted(L"%s - Total %.2fms, Average %.2fms (%i)\n", block->fullname.c_str(), block->duration, average, block->count);
            }

            if (block->topLevel)
                totalDuration += block->duration;
        }
    }
    OutputDebugStringWFormatted(L"\nTotal: %.2fms\n", totalDuration);
    OutputDebugStringW(L"Profiler block printing end.\n\n");
}

ScopedTimer::ScopedTimer(const std::wstring& name)
{
#if defined(_DEBUG) || defined(_UNOPTIMIZED)
    Profiling::BeginBlock(name);
#endif
}

ScopedTimer::~ScopedTimer()
{
#if defined(_DEBUG) || defined(_UNOPTIMIZED)
    Profiling::EndBlock();
#endif
}
