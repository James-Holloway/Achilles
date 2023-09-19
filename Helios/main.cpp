#include "Helios.h"

_Use_decl_annotations_ int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    std::unique_ptr<Helios> helios = std::make_unique<Helios>(L"Helios");
    helios->Initialize();
    helios->Run();

    return 0;
}
