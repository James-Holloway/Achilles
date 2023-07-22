#pragma once
#include <string>
#include <stack>
#include <vector>
#include <chrono>
#include <algorithm>

class Profiling
{
    class ProfilerBlock
    {
    public:
        const std::wstring name;
        double duration = 0.0; // duration in milliseconds
        std::chrono::steady_clock::time_point start;
        bool completed = false;
        std::wstring fullname;

        ProfilerBlock(const std::wstring& _name, std::wstring _fullname);
        void Stop();
    };
public:
    inline static std::stack<std::shared_ptr<ProfilerBlock>> ProfilerBlockStack{};
    inline static std::vector<std::shared_ptr<ProfilerBlock>> ProfilerBlocksThisFrame{};
    inline static std::vector<std::shared_ptr<ProfilerBlock>> ProfilerBlocksLastFrame{};

    inline static bool ProfilerShouldPrint = false;

    static void BeginBlock(const std::wstring& name);
    static void EndBlock();
    static void ClearFrame();
    static void Print();
};

class ScopedTimer
{
public:
    ScopedTimer(const std::wstring& name);
    ~ScopedTimer();
};