#pragma once
#include <string>
#include <stack>
#include <vector>
#include <map>
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
        uint32_t count = 0;
        std::wstring fullname;
        bool topLevel = false;

        ProfilerBlock(const std::wstring& _name, std::wstring _fullname);
        void Stop();
    };
public:
    inline static std::stack<std::shared_ptr<ProfilerBlock>> ProfilerBlockStack{};
    inline static std::map<std::wstring, std::shared_ptr<ProfilerBlock>> ProfilerBlocksByFullname{};

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