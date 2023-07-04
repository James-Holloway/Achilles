#include "Thetis.h"

// Thetis was one of the 50 Nereids (sea nymphs) and mother of Achilles. It's backwards if you think about inheritance

int main()
{
	std::cout << "Hello World!\n";

	std::unique_ptr<Thetis> thetis = std::make_unique<Thetis>(L"Thetis");
	thetis->Initialize();
	thetis->Run();

	return 0;
}
