#include <iostream>
#include <map>
#include <zip.h>
#include <string.h>
#include <json/json.h>
using namespace std;

Json::Value load_header(zip_t *zfolder)
{
	// load file
	zip_file_t *zh = zip_fopen(zfolder, "header.json", ZIP_FL_UNCHANGED);

	// read data from file in chunks of 255 characters until data is fully loaded
	int buff_len = 255 * sizeof(char);
	char *buffer = (char *)malloc(buff_len);
	char *jstream, *tmpstream;
	jstream = NULL;
	zip_int64_t nbytes;
	while ((nbytes = zip_fread(zh, buffer, buff_len - 1)) > 0)
	{
		buffer[buff_len - 1] = '\0';
		//expand stream for another buffer of size + 255

		if (jstream != NULL)
		{
			tmpstream = (char *)malloc(strlen(jstream) + nbytes + 1);
			strcpy(tmpstream, jstream);
			strncat(tmpstream, buffer, nbytes);
			free(jstream);
		}
		else
		{
			tmpstream = strdup(buffer);
		}

		jstream = strndup(tmpstream, strlen(tmpstream));
		free(tmpstream);
	}

	// convert jstream data into Json map.
	Json::Value root;
	JSONCPP_STRING err;
	Json::CharReaderBuilder builder;

	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if (!reader->parse(jstream, jstream + strlen(jstream), &root,
					   &err))
	{
		std::cout << "error" << std::endl;
		return NULL;
	}

	return root;
}

int main(int argc, char *argv[])
{

	// "Test cases" until i create more formal ones
	int *errorp;
	char *path = strdup(argv[1]);
	zip_t *zf = zip_open(path, 0, errorp);
	Json::Value root = load_header(zf);

	std::cout << "**Reading header**" << std::endl;
	std::cout << "Dimensions: " << root["DIMENSIONS"] << std::endl;
	std::cout << "Number of streamlines: " << root["NB_STREAMLINES"] << std::endl;
	std::cout << "Number of vertices: " << root["NB_VERTICES"] << std::endl;
	std::cout << "Voxel to RASMM: " << root["VOXEL_TO_RASMM"] << std::endl;
}
