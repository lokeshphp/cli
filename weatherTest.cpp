// weatherTest.cpp : Defines the entry point for the application.
//

#include "weatherTest.h"
#include <time.h>
#include <chrono>

#include <iostream>
#include <unordered_map>
#include <sstream>

#include <curl/curl.h>


 struct testStruct
 {
	 float varden[90000];
 };


using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

std::string weatherDataPath;

std::string resultPath;
std::string LOGFILE;
int SKRIV_UT_NOTHING = 1;
int SEND_POST_REQUEST = 1;

//using namespace std;

int test_OpenTheSameRasterMultipleTimesAndRead(std::string dataName, int nAnropData)
{
	Raster* map;
	int nRows, nCols, i, i1, i0, minV, maxV;
	float*** raster;
	FILE* filpek;
	std::string namn;
	time_t tid0, tid1;

	time(&tid0);
	auto tid0c = std::chrono::high_resolution_clock::now();
	//printf("start test\n");
	namn = weatherDataPath + dataName;
	time(&tid1);
	auto tid1c = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1c - tid0c;
	printf("Trying to open %s. tidNu %.3lf %lf\n", namn.c_str(), difftime(tid1, tid0), fp_ms);
	filpek = fopen(namn.c_str(), "r");
	if (filpek == NULL) {
		time(&tid1);
		printf("Failed to open weatherData %s. I quit.. tidNu %.3lf\n", namn.c_str(), difftime(tid1, tid0));
		return 0;
	}
	else {
		time(&tid1);
		tid1c = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms2 = tid1c - tid0c;
		printf("Succeeded in opening %s. tidNu %.3lf %lf\n", namn.c_str(), difftime(tid1, tid0), fp_ms2);
		fclose(filpek);
	}
	time(&tid1);

	// must initiate a raster first, this is where GDALAllRegister() is called, if only a pointer to Raster is used then this is not happening...

	Raster test;
	tid1c = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms3 = tid1c - tid0c;
	printf("open raster. tidNu %.3lf %lf\n", difftime(tid1, tid0), fp_ms3);
	test.open(namn.c_str());
	nRows = test.Get_nRows();
	nCols = test.Get_nCols();
	fprintf(filpek, "nRows %d\nnCols %d\n", nRows, nCols);


	filpek = fopen("tmp_testFil.txt", "w");
	map = (Raster*)malloc(nAnropData * sizeof(Raster));
	raster = (float***)malloc(nAnropData * sizeof(float**));
	for (i = 0; i < nAnropData; i++) {
		//		map[i].open(model.params.mapPhysicalFileName.c_str());
				//map[i].open("OCEANgl_-180_-90.grb");


		time(&tid1);
		tid1c = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms4 = tid1c - tid0c;
		printf("opening raster %d... tidNu %.3lf %lf\n", i, difftime(tid1, tid0), fp_ms4);
		map[i].open(namn.c_str());
		nRows = map[i].Get_nRows();
		nCols = map[i].Get_nCols();
		maxV = 100;
		minV = 50;
		if (maxV > nRows)
			maxV = nRows;
		if (maxV > nCols)
			maxV = nCols;
		if (maxV <= minV)
			minV = (int)(maxV / 2);
		fprintf(filpek, "nRows %d\nnCols %d\n", nRows, nCols);
		printf("before getRasterBand\n");
		raster[i] = map[i].GetRasterBand(1);
		printf("after getRasterBand\n");
		fprintf(filpek, "\nraster %d\n", i);
		for (i0 = minV; i0 < maxV; i0++) {
			fprintf(filpek, "%d", i0);
			for (i1 = minV; i1 < maxV; i1++) {
				fprintf(filpek, "\t%lf", raster[i][i0][i1]);
			}
			fprintf(filpek, "\n");
		}
	}
	fclose(filpek);
	time(&tid1);
	tid1c = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms5 = tid1c - tid0c;
	printf("double done. tidNu %.3lf %lf\n", difftime(tid1, tid0), fp_ms5);

	exit(0);
	return 0;
}

int exitKontrollerat(int codeLine, int callType) {

	if(callType == 0)
		errlog0("Ending the program on code row %d\n", codeLine);
	else
		errlog("Ending the program on code row %d\n", codeLine);
	exit(0);
	return 0;
}

int setUserParam(char* argv, std::string* inPath, std::string* outPath) {
	int i, likaPos = -1;
	std::string givenData = argv;

	size_t findData;
	findData = givenData.find("--input=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 8, givenData.size() - 8);
		return 1;
	}
	findData = givenData.find("--output=");
	if (findData < givenData.size()) {
		*outPath = givenData.substr(findData + 9, givenData.size() - 9);
		return 1;
	}
	return 0;

}

void putStringIntoArrayFloat(std::string strang, float* arrFloat, FILE* filpek = NULL) {
	int pos = 0, pos2 = 0, negativ = 0, decimal = 0;
	double scale = 10, varde = 0;

	for (int i = 0; i < 10000000; i++) {
		if (strang[i] == ' ' || strang[i] == '\0') {
			if (negativ == 0)
				arrFloat[pos2++] = varde;
			else
				arrFloat[pos2++] = -varde;
			if (filpek != NULL)
				fprintf(filpek, "%d:%.3lf\n", pos2 - 1, varde);
			if (strang[i] == '\0')
				break;
			varde = 0;
			negativ = 0;
			scale = 10;
			continue;
		}
		if (strang[i] == '-') {
			negativ = 1;
			continue;
		}
		if (strang[i] == '.') {
			scale = 0.1;
			continue;
		}

		if (scale > 1)
			varde = strang[i] - '0' + varde * 10;
		else {
			varde += (strang[i] - '0') * scale;
			scale *= 0.1;
		}
	}
}

void putBinaryIntoArrayFloat(std::string strang, float* arrFloat) {
	int pos = 0, pos2 = 0, negativ = 0, decimal = 0;
	double scale = 10, varde = 0;

	for (int i = 0; i < 10000000; i++) {
		if (strang[i] == ' ' || strang[i] == '\0') {
			if(negativ == 0)
				arrFloat[pos2++] = varde;
			else
				arrFloat[pos2++] = -varde;
			if (strang[i] == '\0')
				break;
			varde = 0;
			negativ = 0;
			scale = 10;
			continue;
		}
		if (strang[i] == '-') {
			negativ = 1;
			continue;
		}
		if (strang[i] == '.') {
			scale = 0.1;
			continue;
		}
		
		if(scale > 1)
			varde = strang[i] - '0' + varde * 10;
		else {
			varde += (strang[i] - '0') * scale;
			scale *= 0.1;
		}
	}
}

size_t writeFunction(void* ptr, size_t size, size_t nmemb, std::string* data) {
	data->append((char*)ptr, size * nmemb);
	return size * nmemb;
}


void getRequest() {
/*
	//curl_global_init(CURL_GLOBAL_DEFAULT);
	auto curl = curl_easy_init();
	CURLcode res;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/whoshuu/cpr/contributors?anon=true&key=value");
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(curl, CURLOPT_USERPWD, "user:pass");
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.42.0");
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
		curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

		std::string response_string;
		std::string header_string;
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

		char* url;
		long response_code;
		double elapsed;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
		curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &elapsed);
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

		res = curl_easy_perform(curl);
		// Check for errors 
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
		else {
			printf("response: '%s'\n", response_string.c_str());
			printf("header_string: '%s'\n", header_string.c_str());

		}
		curl_easy_cleanup(curl);
		curl = NULL;
	}
*/
}

void postRequest(std::string errorMessage) {

	if (SEND_POST_REQUEST == 1) {
		CURL* curl;
		CURLcode res;
		curl = curl_easy_init();
		if (curl) {
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(curl, CURLOPT_URL, "https://optinav-api-dev.tnmservices.ai/api/weather/notify");
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
			struct curl_slist* headers = NULL;
			headers = curl_slist_append(headers, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiIsImp0aSI6ImU5ODhjNjk3ZTI1NDA4ZWQzNTMzNjdhZmI4NmFkNzUzYmIyOWFlMWU3NzRmMzNiYWRiMDllZmYyOTdiNjE4ZjlmMDZhOTk3YmU3NWY2ZTM3In0.eyJhdWQiOiIxIiwianRpIjoiZTk4OGM2OTdlMjU0MDhlZDM1MzM2N2FmYjg2YWQ3NTNiYjI5YWUxZTc3NGYzM2JhZGIwOWVmZjI5N2I2MThmOWYwNmE5OTdiZTc1ZjZlMzciLCJpYXQiOjE2NDc1MjE4NDksIm5iZiI6MTY0NzUyMTg0OSwiZXhwIjoxNjc5MDU3ODQ5LCJzdWIiOiIyIiwic2NvcGVzIjpbXX0.UVbHJMid3B_5WyzD5VJ9AA1wllGtlr_aK4JRuQ66jRgSmn0fZGzB6D4Cm97sFUSltHp8cOPfQf0jOTC_sjFz0UoFGckSNrbw0GTwue3h9cduvdSZB7rUB7VgR_0XOL6hOiEgPzBOQU4okDwp52KZ5avZDE8x5PWF76qADJ2_835_9AMOq-myBQwFkysFiohJDZo5GS0MabVilJ58tls94KhX2er_8qj2_SpYGVWUVCCy_FYe8XnVrXOSO7j06LYvtpkR5Lspcp4Z9egDGb-NcqB80x9ilNc1CzzClt1DC1yMUUyTo1Z0162A6vxh5vM0Ly0pEX2r3UNfNDWo4-IDH-BB1aczK-43NTE2yafpPqHklj6FvzhdJAHX3Pht3SBFrHT2IG15yFeCj1fhJB9oHTwLnG4BYOmWwO6FohV5DSEolrFTOLWA1MoOrztN-xx4nmrmM6p53awVrRanNMbwnh6X7qPqS668Kd9ZQmR-EkyYHxEvib1YitOH7smnTFzI2P5Jfymf9K2fti3AyzzLGVa3HCKUHSaHU6yMaLk4ZECqRAcxOaYjQZFFJTqWSyY9weozmR1M-GdGFJ1shI9qqDl9utcCPoZ0-IxsJ8hKoVYT2KmqgAd-9vZLAXB2p_Q0twl1riqMyzg1J2W52HNNv8Mcu3WVZOWpLGjHuiy_O9o");
			headers = curl_slist_append(headers, "Cookie: XSRF-TOKEN=eyJpdiI6IkRyMXNDcmhUcVhMZlFVamZNcysxNHc9PSIsInZhbHVlIjoibkFQcnhPV0RBby9pbVdpNllLdGZld2RJYTZXVjV4UWd6VmJuNERBMTc0T3NoUmhpdVNXcnJGcUZDUFBmQk5lSUFzcWo0QUplVVBsYytxYWRUcWxCbkRjWlF2UUdSbGZFeW1mbEF3ZjcxM3JrY0JqUWtIM2Z5UnI2d1FFWGhTWWIiLCJtYWMiOiJiNGU1NGFhNmYxNzY1ZjMyZjA4NjY0MTQ5Yzc3MzlmZDU4MTU2MTVlODNmZGI5Y2RjZjVkZTIwOWVmZTE0NjcwIiwidGFnIjoiIn0%3D; laravel_session=eyJpdiI6InppTmFLd1R3SE84bjB1d2hLcFg5eVE9PSIsInZhbHVlIjoiajdDNjU4dkpmY2RMVGdacHA0OXdUSGlZTjB4THRNbmFKQ3o5Z3hXL3ZUY1ZScHdwNGpsYXhOVU5sVG9KYUV6QVRNeXBhZTg3UXJXakYyd3I1c1RLUHdtVDNkOWd4MWx3ekpPMHpPOW1VM1J2QlhIcWhsVDNHSzZWclA4ZS9NbnQiLCJtYWMiOiIxNDVhZDNlYmY0YmM3MmNjYjZlNGIyYmFkN2EyMjMyNzVmMDRhNWE4NTg5ZDU2YTYwNWU0ZTkyZTdmOGE1MmMxIiwidGFnIjoiIn0%3D");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_mime* mime;
			curl_mimepart* part;
			mime = curl_mime_init(curl);
			part = curl_mime_addpart(mime);
			curl_mime_name(part, "type");
			curl_mime_data(part, "error", CURL_ZERO_TERMINATED);
			part = curl_mime_addpart(mime);
			curl_mime_name(part, "message");
			curl_mime_data(part, errorMessage.c_str(), CURL_ZERO_TERMINATED);
			curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
			res = curl_easy_perform(curl);
			curl_mime_free(mime);
		}
		curl_easy_cleanup(curl);
	}
}

int main(int argc, char* argv[])
{
	std::string dataName, inputPath;
	int nAnropData, userGivenOK;
	FILE* filpek;

	// testing(2);

	//cout << "Hello CMake. Test 2" << endl;
	//cout << "nArgc " << argc << endl;

	LOGFILE = "logfile.txt";
	//for (int i = 0; i < argc; i++)
	//	cout << argv[i] << endl;

	if (argc == 4) {
		printf("three arguments read, should only be two\n");
		weatherDataPath = argv[1];
		dataName = argv[2];
		try {
			nAnropData = std::stoi(argv[3]);
			if (nAnropData < 0 || nAnropData > 10000) {
				printf("ERROR! nAnropData is %d, must be between 0 and 10000\n", nAnropData);
			}
		}
		catch (...) {
			printf("ERROR! input no 3 is not an integer\n");
			exit(0);
		}

		test_OpenTheSameRasterMultipleTimesAndRead(dataName, nAnropData);
	}
	else {
		inputPath = "-";
		resultPath = "-";
		if(argc == 3){
			for (int i = 1; i < 3; i++) {
				userGivenOK = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					postRequest("ERROR! Could not read user data '" + std::string(argv[i]) + "'. I quit!");
					exitKontrollerat(__LINE__, 0);
				}
			}
			if (inputPath == "-") {
				errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				postRequest("ERROR! Did not manage to identify an input name from " + std::string(argv[1]) + 
					" or " + std::string(argv[2]) + ". I quit.");
				exitKontrollerat(__LINE__, 0);
			}
			if (dataName == "-") {
				errlog0("ERROR! Did not manage to identify a result name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify a result name from %s or %s. I quit.\n", argv[1], argv[2]);
				postRequest("ERROR! Did not manage to identify a result name from " + std::string(argv[1]) +
					" or " + std::string(argv[2]) + ". I quit.");
				exitKontrollerat(__LINE__, 0);
			}
			resultPath = splitFilename(dataName);
			filpek = fopen(dataName.c_str(), "w");
			if (filpek == NULL) {
				errlog0("ERROR! Could not open file %s. Does the directory not exist or am I not allowed to write to that directory? I quit.\n",
					dataName.c_str());
				printf("ERROR! Could not open file %s for writing. Is it locked or does the directory not exist? I quit.\n",
					dataName.c_str());
				postRequest("ERROR! Could not open file " + dataName + " for writing.Is it locked or does the directory not exist ? I quit.");
				exitKontrollerat(__LINE__, 0);
			}
			fprintf(filpek, "{\nerror\n}\n");
			fclose(filpek);

			//printf("pass 1\n");
			//freopen("output.txt", "w", stdout);
			//cout << str;

			printf("Calling OptiNav with input '%s' and output '%s'\n", inputPath.c_str(), dataName.c_str());
			auto tid0 = std::chrono::high_resolution_clock::now();
			if(inputPath != "-")
				voyageOpt(inputPath, dataName);

			auto tid1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
			printf("OptiNav took %.3lf\n", fp_ms);
			errlog("OptiNav took %.3lf\n", fp_ms);
		}
		else {
			if (argc == 2) {
				int i = 1;
				LOGFILE = "logfile_setRedisKeys.txt";
				SKRIV_UT_NOTHING = 0;
				inputPath = "-";
				userGivenOK = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					postRequest("ERROR! Could not read user data '" + std::string(argv[i]) + "'. I quit!");
					exitKontrollerat(__LINE__, 0);
				}
				if (inputPath == "-") {
					errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
					printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
					postRequest("ERROR2! Did not manage to identify an input name from " + std::string(argv[1]) +
						" or " + std::string(argv[2]) + ". I quit.");
					exitKontrollerat(__LINE__, 0);
				}
				printf("input file for redis key generation '%s'\n", inputPath.c_str());
				auto tid0 = std::chrono::high_resolution_clock::now();
				int returnVal = 1;
				if (inputPath != "-") {
					returnVal = saveTablesToSQLite(inputPath);
					returnVal = redisSetKeys(inputPath);
				}
				if (returnVal != 0) {
					errlog("ERROR! Failed to set redis keys for weather\n");
					printf("ERROR! Failed to set redis keys for weather\n");
					postRequest("ERROR! Failed to set redis keys for weather");
					exitKontrollerat(__LINE__, 0);
				}
				else
					printf("Setting of all the keys done\n");


				auto tid1 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
				printf("redis key generation took %.3lf\n", fp_ms);
				errlog("redis key generation took %.3lf\n", fp_ms);
			}
			else {
				LOGFILE = "logfile_error.txt";
				printf("%d arguments read, should be two\n", argc);
				postRequest("wrong number of arguments calling OptiNav");

			}
		}
	}
	return 0;
}
