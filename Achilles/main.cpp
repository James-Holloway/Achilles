#include "stdafx.h"
#include "Achilles.h"

int main()
{
    std::cout << "Hello World!\n";

    std::unique_ptr<Achilles> achilles = std::make_unique<Achilles>(L"Achilles");
    achilles->Run();

    return 0;
}