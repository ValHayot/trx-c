#include "trx.h"
#include <fstream>

#include <errno.h>
#define SYSERROR() errno

//#define ZIP_DD_SIG 0x08074b50
//#define ZIP_CD_SIG 0x06054b50
using namespace Eigen;
using namespace std;

long dtype_size(std::string dtype)
{
	if (dtype.compare("int8") == 0 || dtype.compare("uint8") == 0)
		return (long)pow(2, 8);
	else if (dtype.compare("int16") == 0 || dtype.compare("uint16") == 0 || dtype.compare("float16") == 0)
		return (long)pow(2, 16);
	else if (dtype.compare("uint32") || dtype.compare("float32"))
		return (long)pow(2, 32);
	else if (dtype.compare("int64") == 0 || dtype.compare("uint64") == 0 || dtype.compare("float64") == 0)
		return (long)pow(2, 64);
	else
		return 0;
}

bool _is_dtype_valid(std::string &ext)
{
	if (ext.compare("bit") == 0)
		return true;
	else if (std::find(dtypes.begin(), dtypes.end(), ext) != dtypes.end())
		return true;
	return false;
}

Json::Value load_header(zip_t *zfolder)
{
	// load file
	zip_file_t *zh = zip_fopen(zfolder, "header.json", ZIP_FL_UNCHANGED);

	// read data from file in chunks of 255 characters until data is fully loaded
	int buff_len = 255 * sizeof(char);
	char *buffer = (char *)malloc(buff_len);
	//char *jstream, *tmpstream;
	//jstream = NULL;
	std::string jstream = "";
	zip_int64_t nbytes;
	while ((nbytes = zip_fread(zh, buffer, buff_len - 1)) > 0)
	{
		if (buffer != NULL)
		{
			jstream += string(buffer, nbytes);
		}
	}

	// convert jstream data into Json map.
	Json::Value root;
	JSONCPP_STRING err;
	Json::CharReaderBuilder builder;

	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if (!reader->parse(jstream.c_str(), jstream.c_str() + jstream.size(), &root,
					   &err))
	{
		std::cout << "error" << std::endl;
		return 0;
	}

	return root;
}

int load_from_zip(const char *path)
{
	int *errorp;
	zip_t *zf = zip_open(path, 0, errorp);
	Json::Value header = load_header(zf);

	std::map<std::string, std::tuple<int, int>> file_pointer_size;
	int global_pos = 0;
	int mem_address = 0;

	int num_entries = zip_get_num_entries(zf, ZIP_FL_UNCHANGED);

	for (int i = 0; i < num_entries; ++i)
	{
		std::string elem_filename = zip_get_name(zf, i, ZIP_FL_UNCHANGED);

		size_t lastdot = elem_filename.find_last_of(".");

		if (lastdot == std::string::npos)
			continue;
		std::string ext = elem_filename.substr(lastdot + 1, std::string::npos);

		if (ext.compare("bit") == 0)
			ext = "bool";

		// get file stats
		zip_stat_t sb;

		if (zip_stat(zf, elem_filename.c_str(), ZIP_FL_UNCHANGED, &sb) != 0)
		{
			return 1;
		}

		ifstream file(path, ios::binary);
		file.seekg(global_pos);
		mem_address = global_pos;

		unsigned char signature[4] = {0};
		const unsigned char local_sig[4] = {0x50, 0x4b, 0x03, 0x04};
		file.read((char *)signature, sizeof(signature));

		if (memcmp(signature, local_sig, sizeof(signature)) == 0)
		{
			global_pos += 30;
			global_pos += sb.comp_size + elem_filename.size();
		}

		// It's the header, skip it
		if (ext.compare("json") == 0)
			continue;

		if (!_is_dtype_valid(ext))
			continue;

		file_pointer_size[elem_filename] = {mem_address, global_pos};
	}
	return 1;
}

void allocate_file(const std::string &path, const int size)
{
	std::ofstream file(path);
	if (file.is_open())
	{
		std::string s(size, float(0));
		file << s;
		file.flush();
		file.close();
	}
	else
	{
		std::cerr << "Failed to allocate file : " << SYSERROR() << std::endl;
	}
}

//TODO: support FORTRAN ORDERING
//template <typename Derived>
mio::shared_mmap_sink _create_memmap(std::filesystem::path &filename, const MatrixXi &shape, std::string mode, std::string dtype, int offset)
{
	if (dtype.compare("bool") == 0)
	{
		std::filesystem::path ext = "bit";
		filename.replace_extension(ext);
	}

	allocate_file(filename.string(), shape(0) * shape(1));
	//std::error_code error;
	mio::shared_mmap_sink rw_mmap(filename.string(), 0, mio::map_entire_file);

	// should investigate further on creating mmap here rather than returning data pointer
	return rw_mmap;
}

void TrxFile::_initialize_empty_trx(int nb_streamlines, int nb_vertices, TrxFile *init_as)
{

	//TODO: fix as tmpnam has issues with concurrency.
	std::filesystem::path tmp_dir{std::filesystem::temp_directory_path() /= std::tmpnam(nullptr)};
	std::filesystem::create_directory(tmp_dir);
	std::cout << "Temporary folder for memmaps: " << tmp_dir << std::endl;

	this->header["NB_VERTICES"] = nb_vertices;
	this->header["NB_STREAMLINES"] = nb_streamlines;

	if (init_as != NULL)
	{
		this->header["VOXEL_TO_RASMM"] = init_as->header["VOXEL_TO_RASMM"];
		this->header["DIMENSIONS"] = init_as->header["DIMENSIONS"];
	}

	cout << "Initializing positions with dtype: "
		 << "float16" << endl;
	cout << "Initializing offsets with dtype: "
		 << "uint16" << endl;
	cout << "Initializing lengths with dtype: "
		 << "uint32" << endl;

	std::filesystem::path positions_filename;
	positions_filename /= tmp_dir;
	positions_filename /= "positions.3.float16";
	Eigen::MatrixXi m(2, 1);
	m << nb_vertices, 3;
	//new (&trx.streamlines._data)
	this->streamlines.mmap_pos = _create_memmap(positions_filename, m, std::string("w+"), std::string("float16"), 0);
	new (&(this->streamlines._data)) Map<Matrix<half, Dynamic, Dynamic>>(reinterpret_cast<half *>(this->streamlines.mmap_pos.data()), m(0), m(1));

	std::filesystem::path offsets_filename;
	offsets_filename /= tmp_dir;
	offsets_filename /= "offsets.uint16";
	cout << "filesystem path " << offsets_filename << endl;
	m << nb_streamlines, 1;
	this->streamlines.mmap_off = _create_memmap(offsets_filename, m, std::string("w+"), std::string("uint16_t"), 0);
	new (&(this->streamlines._offset)) Map<Matrix<uint16_t, Dynamic, 1>>(reinterpret_cast<uint16_t *>(this->streamlines.mmap_off.data()), m(0), m(1));

	this->streamlines._lengths.resize(nb_streamlines, 0);

	if (init_as != NULL)
	{
		if (init_as->data_per_vertex.size() > 0)
		{
			std::filesystem::path dpv_dirname;
			dpv_dirname /= tmp_dir;
			dpv_dirname /= "dpv";
			std::filesystem::create_directory(dpv_dirname);
		}
		if (init_as->data_per_streamline.size() > 0)
		{
			std::filesystem::path dps_dirname;
			dps_dirname /= tmp_dir;
			dps_dirname /= "dps";
			std::filesystem::create_directory(dps_dirname);
		}

		for (auto const &x : init_as->data_per_vertex)
		{
			int rows, cols;
			Map<Matrix<half, Dynamic, Dynamic>> tmp_as = init_as->data_per_vertex[x.first]._data;
			std::filesystem::path dpv_filename;
			if (tmp_as.cols() == 1 || tmp_as.rows() == 1)
			{
				dpv_filename /= tmp_dir;
				dpv_filename /= "dpv";
				dpv_filename /= x.first + "." + "float16";
				rows = nb_vertices;
				cols = 1;
			}
			else
			{
				rows = nb_vertices;
				cols = tmp_as.cols();

				dpv_filename /= tmp_dir;
				dpv_filename /= "dpv";
				dpv_filename /= x.first + "." + std::to_string(cols) + "." + "float16";
			}

			cout << "Initializing " << x.first << " (dpv) with dtype: "
				 << "float16" << endl;

			Eigen::MatrixXi m(2, 1);
			m << rows, cols;
			this->data_per_vertex[x.first] = ArraySequence();
			this->data_per_vertex[x.first].mmap_pos = _create_memmap(dpv_filename, m, std::string("w+"), std::string("float16"), 0);
			new (&(this->data_per_vertex[x.first]._data)) Map<Matrix<uint16_t, Dynamic, Dynamic>>(reinterpret_cast<uint16_t *>(this->data_per_vertex[x.first].mmap_pos.data()), m(0), m(1));

			this->data_per_vertex[x.first]._offset = this->streamlines._offset;
			this->data_per_vertex[x.first]._lengths = this->streamlines._lengths;
		}

		for (auto const &x : init_as->data_per_streamline)
		{
			string dtype = "float16";
			int rows, cols;
			Map<Matrix<half, Dynamic, Dynamic>> tmp_as = init_as->data_per_streamline[x.first]._matrix;
			std::filesystem::path dps_filename;

			if (tmp_as.rows() == 1 || tmp_as.cols() == 1)
			{
				dps_filename /= tmp_dir;
				dps_filename /= "dps";
				dps_filename /= x.first + "." + dtype;
				rows = nb_streamlines;
			}
			else
			{
				cols = tmp_as.cols();
				rows = nb_streamlines;

				dps_filename /= tmp_dir;
				dps_filename /= "dps";
				dps_filename /= x.first + "." + std::to_string(cols) + "." + dtype;
			}

			cout << "Initializing " << x.first << " (dps) with and dtype: " << dtype << endl;

			Eigen::MatrixXi m(2, 1);
			m << rows, cols;
			this->data_per_streamline[x.first] = MMappedMatrix();
			this->data_per_streamline[x.first].mmap = _create_memmap(dps_filename, m, std::string("w+"), dtype, 0);
			new (&(this->data_per_streamline[x.first]._matrix)) Map<Matrix<half, Dynamic, Dynamic>>(reinterpret_cast<half *>(this->data_per_streamline[x.first].mmap.data()), m(0), m(1));
		}
	}

	this->_uncompressed_folder_handle = tmp_dir;
}

TrxFile::TrxFile(int nb_vertices, int nb_streamlines, Json::Value init_as, std::string reference)
{
	MatrixXf affine(4, 4);
	RowVectorXi dimensions(3);

	if (init_as != 0)
	{
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				affine << init_as["VOXEL_TO_RASMM"][i][j].asFloat();
			}
		}

		for (int i = 0; i < 3; i++)
		{
			dimensions[i] << init_as["DIMENSIONS"][i].asUInt();
		}
	}
	else
	{
		// add logger here
		// eye matrixt
		affine << MatrixXf::Identity(4, 4);
		dimensions << 1, 1, 1;
	}

	if (nb_vertices == 0 && nb_streamlines == 0)
	{
		if (init_as != 0)
		{
			// raise error here
			exit(1);
		}

		// will remove as completely unecessary. using as placeholders
		this->header = {};
		// this->streamlines = ArraySequence();
		this->_uncompressed_folder_handle = "";

		nb_vertices = 0;
		nb_streamlines = 0;
	}
	else if (nb_vertices > 0 && nb_streamlines > 0)
	{
		std::cout << "Preallocating TrxFile with size " << nb_streamlines << " streamlines and " << nb_vertices << " vertices." << std::endl;
		_initialize_empty_trx(nb_streamlines, nb_vertices);

		//this->streamlines._data << half(1);
		this->streamlines._offset << uint16_t(1);
	}

	this->header["VOXEL_TO_RASMM"] = affine;
	this->header["DIMENSIONS"] = dimensions;
	this->header["NB_VERTICES"] = nb_vertices;
	this->header["NB_STREAMLINES"] = nb_streamlines;
}

Dict assignHeader(Json::Value root)
{
	Dict header;
	MatrixXf affine(4, 4);
	RowVectorXi dimensions(3);

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			affine << root["VOXEL_TO_RASMM"][i][j].asFloat();
		}
	}

	for (int i = 0; i < 3; i++)
	{
		dimensions[i] << root["DIMENSIONS"][i].asUInt();
	}
	header["VOXEL_TO_RASMM"] = affine;
	header["DIMENSIONS"] = dimensions;
	header["NB_VERTICES"] = (int)root["NB_VERTICES"].asUInt();
	header["NB_STREAMLINES"] = (int)root["NB_STREAMLINES"].asUInt();

	return header;
}

TrxFile *TrxFile::_create_trx_from_pointer(Json::Value header, std::map<std::string, std::tuple<int, int>> dict_pointer_size, std::string root_zip, std::string root)
{
	TrxFile *trx = new TrxFile();
	trx->header = assignHeader(header);

	// positions, offsets = None, None
	Map<Matrix<half, Dynamic, Dynamic>> positions(NULL, 1, 1);
	Map<Matrix<uint16_t, Dynamic, 1>> offsets(NULL, 1, 1);
	std::filesystem::path filename;

	for (auto const &x : dict_pointer_size)
	{
		std::filesystem::path elem_filename = x.first;
		if (root_zip.size() == 0)
		{
			filename = root_zip;
		}
		else
		{
			filename = elem_filename;
		}

		std::filesystem::path folder = elem_filename.parent_path();

		// _split_ext_with_dimensionality
		std::string basename = elem_filename.filename().string();

		std::string tokens[4];
		int idx, prev_pos, curr_pos;
		prev_pos = 0;
		idx = 0;

		while ((curr_pos = basename.find(".")) != std::string::npos)
		{
			tokens[idx] = basename.substr(prev_pos, curr_pos);
			prev_pos = curr_pos + 1;
			idx++;
		}

		if (idx < 2 || idx > 3)
		{
			throw("Invalid filename.");
		}

		basename = tokens[0];
		std::string ext = "." + tokens[idx - 1];
		int dim;

		if (idx == 2)
		{
			dim = 1;
		}
		else
		{
			dim = std::stoi(tokens[1]);
		}
		_is_dtype_valid(ext);
		// function completed

		if (ext.compare(".bit") == 0)
		{
			ext = ".bool";
		}
	}

	return trx;
}

void get_reference_info(std::string reference, const MatrixXf &affine, const RowVectorXi &dimensions)
{
	if (reference.find(".nii") != std::string::npos)
	{
	}
	else if (reference.find(".trk") != std::string::npos)
	{
		// TODO: Create exception class
		std::cout << "Trk reference not implemented" << std::endl;
		std::exit(1);
	}
	else
	{
		// TODO: Create exception class
		std::cout << "Trk reference not implemented" << std::endl;
		std::exit(1);
	}
}

int main(int argc, char *argv[])
{
	// "Test cases" until i create more formal ones
	int *errorp;
	char *path = strdup(argv[1]);
	zip_t *zf = zip_open(path, 0, errorp);
	Json::Value header = load_header(zf);

	std::cout << "**Reading header**" << std::endl;
	std::cout << "Dimensions: " << header["DIMENSIONS"] << std::endl;
	std::cout << "Number of streamlines: " << header["NB_STREAMLINES"] << std::endl;
	std::cout << "Number of vertices: " << header["NB_VERTICES"] << std::endl;
	std::cout << "Voxel to RASMM: " << header["VOXEL_TO_RASMM"] << std::endl;

	load_from_zip(path);

	TrxFile trx(1, 1);

	std::cout << "Printing trk streamline data value " << trx.streamlines._data(0, 0) << endl;
	std::cout << "Printing trk streamline offset value " << trx.streamlines._offset(0, 0) << endl;
	return 1;
}
