#include <iostream>
#include <zip.h>
#include <json/json.h>
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

/**
 * This function loads the header json file
 * stored within a Zip archive
 * 
 * @param[in] zfolder a pointer to an opened zip archive
 * @param[out] header the JSONCpp root of the header. NULL on error.
 *  
 * */
Json::Value load_header(zip_t *zfolder);
std::ostream &operator<<(std::ostream &out, const TrxFile &TrxFile);