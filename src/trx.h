#include <iostream>
#include <zip.h>
#include <string.h>
#include <json/json.h>
#include <algorithm>
#include <variant>
#include <math.h>
#include <Eigen/Core>

using namespace Eigen;

const std::vector<std::string> dtypes({"int8", "int16", "int64", "uint8", "uint16", "uint32", "uint64", "float16", "float32", "float64"});

typedef std::map<std::string, std::variant<int, MatrixXf, RowVectorXi>> Dict;

class TrxFile
{
	// Access specifier
public:
	// Data Members
	Dict header;
	std::vector<float> streamlines;
	std::vector<uint64_t> offsets;
	std::vector<uint32_t> lenghts;

	// Member Functions()
public:
	//TrxFile(int nb_vertices = 0, int nb_streamlines = 0);
	TrxFile(int nb_vertices = 0, int nb_streamlines = 0, Json::Value init_as = NULL);

	static TrxFile _create_trx_from_pointer(Json::Value header, std::map<std::string, std::tuple<int, int>> dict_pointer_size, std::string root_zip = NULL, std::string root = NULL);

private:
	int len();
};

/**
 * Determines data type size
 * Note: need to determine a better solution
 * 
 * @param[in] dtype a string consisting of the extension starting by a .
 * @param[out] size the respective size of the dtype
 *  
 * */
long dtype_size(std::string dtype);
/**
 * Determine whether the extension is a valid extension
 * 
 * 
 * @param[in] ext a string consisting of the extension starting by a .
 * @param[out] is_valid a boolean denoting whether the extension is valid.
 *  
 * */
bool _is_dtype_valid(std::string &ext);

/**
 * This function loads the header json file
 * stored within a Zip archive
 * 
 * @param[in] zfolder a pointer to an opened zip archive
 * @param[out] header the JSONCpp root of the header. NULL on error.
 *  
 * */
Json::Value load_header(zip_t *zfolder);

/**
 * Load the TRX file stored within a Zip archive.
 * 
 * @param[in] path path to Zip archive
 * @param[out] status return 0 if success else 1 
 * 
 * */
int load_from_zip(const char *path);
std::ostream &operator<<(std::ostream &out, const TrxFile &TrxFile);