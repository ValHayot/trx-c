#include <iostream>
#include <zip.h>
#include <map>
class TrxFile
{
	// Access specifier
public:
	// Data Members
	//std::map<>

	// Member Functions()
public:
	TrxFile();

private:
	int len();
};

std::ostream &operator<<(std::ostream &out, const TrxFile &TrxFile);