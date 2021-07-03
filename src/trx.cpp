#include <iostream>
#include <map>
#include <zip.h>
#include <string.h>
#include <json/json.h>
using namespace std;

int main(int argc, char *argv[])
{
	cout << "test" << endl;
	int *errorp;
	char *path = strdup(argv[1]);
	zip_t *zf = zip_open(path, 0, errorp);
	zip_file_t *zh = zip_fopen(zf, "header.json", ZIP_FL_UNCHANGED);

	int buff_len = 20 * sizeof(char);
	char *buffer = (char *)malloc(buff_len);
	char *jstream, *tmpstream;
	jstream = NULL;
	zip_int64_t nbytes;
	while ((nbytes = zip_fread(zh, buffer, buff_len - 1)) > 0)
	{
		buffer[buff_len - 1] = '\0';
		//expand stream for another buffer of size 255

		if (jstream != NULL)
		{
			// std::cout << strlen(jstream) << "-" << strlen(buffer) << "-" << nbytes << '\n';
			tmpstream = (char *)malloc(strlen(jstream) + nbytes + 1);
			strcpy(tmpstream, jstream);
			strncat(tmpstream, buffer, nbytes);
			free(jstream);
		}
		else
		{
			// std::cout << "buff" << strlen(buffer) << "-" << nbytes << '\n';
			tmpstream = strdup(buffer);
		}

		jstream = strndup(tmpstream, strlen(tmpstream));
		// std::cout << strlen(jstream) << std::endl;
		free(tmpstream);
	}
	cout << jstream << endl;
	Json::Value root;
	JSONCPP_STRING err;
	Json::CharReaderBuilder builder;

	const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	if (!reader->parse(jstream, jstream + strlen(jstream), &root,
					   &err))
	{
		std::cout << "error" << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << root["NB_VERTICES"].asString() << std::endl;
}
