
// optiNav_offline.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "weatherTest.h"
#include <time.h>
#include <chrono>
#include <direct.h>

#include <iostream>
#include <unordered_map>
#include <sstream>

#include <curl/curl.h>

//#include<fstream>

struct testStruct
{
	float varden[90000];
};

using namespace std;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

string weatherDataPath;

string resultPath;
string LOGFILE;
int SKRIV_UT_NOTHING = 2;
int SEND_POST_REQUEST = 0;
int runAltForecast = 0;
int SPARA_RUN_DATA = 1;

extern strModel model;

int test_OpenTheSameRasterMultipleTimesAndRead(string dataName, int nAnropData)
{
	Raster* map;
	int nRows, nCols, i, i1, i0, minV, maxV;
	float*** raster;
	FILE* filpek;
	string namn;
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

	if (callType == 0)
		errlog0("Ending the program on code row %d\n", codeLine);
	else
		errlog("Ending the program on code row %d\n", codeLine);
	exit(0);
	return 0;
}

int setUserParam(char* argv, string* inPath, string* outPath) {
	int i, likaPos = -1;
	string givenData = argv;


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
	findData = givenData.find("--inputAutoRoute=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 17, givenData.size() - 17);
		return 2;
	}
	findData = givenData.find("--outputAutoRoute=");
	if (findData < givenData.size()) {
		*outPath = givenData.substr(findData + 18, givenData.size() - 18);
		return 2;
	}
	findData = givenData.find("--opt=");
	if (findData < givenData.size()) {
		*outPath = givenData.substr(findData + 6, givenData.size() - 6);
		return 1;
	}
	findData = givenData.find("--test=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 7, givenData.size() - 7);
		return 2;
	}
	findData = givenData.find("--inputStormFix=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 16, givenData.size() - 16);
		return 3;
	}
	findData = givenData.find("--inputKaoutar=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 15, givenData.size() - 15);
		return 4;
	}

	return 0;

}

int readUserParam(char* argv, string* pathTmp) {
	int i, likaPos = -1;
	string givenData = argv;


	size_t findData;
	findData = givenData.find("--input=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 8, givenData.size() - 8);
		return 1;
	}
	findData = givenData.find("--output=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 9, givenData.size() - 9);
		return 2;
	}
	findData = givenData.find("--inputAutoRoute=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 17, givenData.size() - 17);
		return 3;
	}
	findData = givenData.find("--outputAutoRoute=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 18, givenData.size() - 18);
		return 4;
	}
	findData = givenData.find("--weatherDirectory=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 19, givenData.size() - 19);
		return 5;
	}
	findData = givenData.find("--opt=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 6, givenData.size() - 6);
		return 10;
	}
	findData = givenData.find("--test=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 7, givenData.size() - 7);
		return 11;
	}
	findData = givenData.find("--inputStormFix=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 16, givenData.size() - 16);
		return 12;
	}
	findData = givenData.find("--inputKaoutar=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 15, givenData.size() - 15);
		return 13;
	}
	findData = givenData.find("--inputGribFileTest=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 20, givenData.size() - 20);
		return 14;
	}
	findData = givenData.find("--outputGribFileTest=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 21, givenData.size() - 21);
		return 15;
	}
	findData = givenData.find("--inputCreateRaster=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 20, givenData.size() - 20);
		return 16;
	}
	findData = givenData.find("--outputCreateRaster=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 21, givenData.size() - 21);
		return 17;
	}
	findData = givenData.find("--inputSeaRoute=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 16, givenData.size() - 16);
		return 18;
	}

	findData = givenData.find("--forecast=");
	if (findData < givenData.size()) {
		*pathTmp = givenData.substr(findData + 11, givenData.size() - 11);
		return 20;
	}


	return 0;

}

void putStringIntoArrayFloat(string strang, float* arrFloat, FILE* filpek = NULL) {
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

void putBinaryIntoArrayFloat(string strang, float* arrFloat) {
	int pos = 0, pos2 = 0, negativ = 0, decimal = 0;
	double scale = 10, varde = 0;

	for (int i = 0; i < 10000000; i++) {
		if (strang[i] == ' ' || strang[i] == '\0') {
			if (negativ == 0)
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

		if (scale > 1)
			varde = strang[i] - '0' + varde * 10;
		else {
			varde += (strang[i] - '0') * scale;
			scale *= 0.1;
		}
	}
}

void postRequest(std::string errorMessage, int endProgram) {

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	char* datumNamn = (char*)malloc2(256 * sizeof(char));
	char* namnDir = (char*)malloc2(256 * sizeof(char));

	if (endProgram == 1) {
		FILE* filPek3 = fopen(model.params.resultName.c_str(), "w"); // "result_json.json", "w");
		fprintf(filPek3, "{\n\t\"errorMessage\": \"%s\"\n}\n", errorMessage.c_str());
		fclose(filPek3);
	}

	errorMessage.append(" hindcast: " + std::to_string(model.params.hindCast));
	errlog("postRequest: %s", errorMessage.c_str());
	printf("postRequest: %s", errorMessage.c_str());

	if (SEND_POST_REQUEST == 1) {
		time_t rawtime;
		time(&rawtime);
		struct tm tmBas = *localtime(&rawtime);
		// struct tm tmBas = { std::time(0) };
		//setTMtime(&tmBas, endTime);
		fixReadableDate_file(tmBas, datumNamn);
		sprintf(namnDir, "%s/postRequestFiles", model.params.resultPath.c_str());
		struct stat sb;
		if (stat(namnDir, &sb) != 0) {
			_mkdir(namnDir);
		}
		sprintf(namn, "%s/postRequestFiles/input_%s", model.params.resultPath.c_str(), datumNamn);
		//sprintf(namn, "%s", model.params.indataPathName.c_str());
		write_copyAtoB(namn, (char*)"json", (char*)model.params.indataPathName.c_str(), (char*)"w");

		errlog("OBS! Sending the following message to POST and saves the input file as %s\n%s.json\n", errorMessage.c_str(),
			namn);
	}
	if (endProgram == 1) {
		exitKontrollerat(__LINE__);
	}
	/*
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
	*/
}

int call_api_corridors(std::string inputPath) {
	// download new corridors from api to file inputPath/tmp_corridors.json
	
	return 0;
}

//
// test grib files, files to check is given in data/gribFilesToCheck.json
// --inputGribFileTest=data/delay.json --outputGribFileTest=data/res_delay.json 0
// 
// create delay raster
// ..\..\..\runDir
// --inputCreateRaster=data/delay.json --outputCreateRaster=data/res_delay.json 0 11
// 
// run OptiNav
// ..\..\..\weatherTest\testRun\shipping
// forecast
// --input=data/why.json --output=data/res_why.json
// onboard
// --input=data/test_2031.json --output=data/res_onboard2031b.json
// hindCast
// --input=data\hc_2318.json --output=data\res_hc2318.json


// if onboard is used then in input data use "onboard":2, (if 1 then hindcast data will be used after the planning horizon which shouldn't be available for onboard version)

// load weather data to redis
// --input=data/

// input parameters to run autoRoute
// --inputAutoRoute=data\t_autoTest2.json --outputAutoRoute=data/res_autoTest2.json

// input parameters to run fixIndataStorms
// --inputStormFix=data\weather\Hindcast\storms\

// input parameters to generate weather factor for Kaoutar
// --inputKaoutar=data\

// input parameters to generate new seaRoute paths (obs, change minLat in file_paramsAutoRoute.json if close to it to see correct geometry)
// --inputSeaRoute=data\coordsNewSeaRoutes.json

// --forecast=XX, if XX = 0 then not used, 1 (standard forecast - redis), ... add more alternatives
//				  if XX < 0 then evaluate already optimized solutions against a specific weather
//							-1 (standard forecast - redis), -2 (real historical/hindCast weather), 

// --forecast=10000 ger dump forecast values for given dates and points
//            --input=data\test_kaoutar.json --output=data/forecastTest/res_kaoutar.json --forecast=10000

int main(int argc, char* argv[])
{
	string dataName, inputPath, outputPath, weatherPath, pathUse;
	int nAnropData, userGivenOK, problTyp;
	FILE* filpek;
	int retVal = 0;

	//testing(2);
	
	testIntersect();


	//cout << "Hello CMake. Test 2" << endl;
	//cout << "nArgc " << argc << endl;

	LOGFILE = "logfile.txt";
	//for (int i = 0; i < argc; i++)
	//	cout << argv[i] << endl;

	model.params.checkGribFilesSpecial = 0;
	model.delay.nYears = 0;

	//SKRIV_UT_NOTHING = 0;
	LOGFILE = "logfile_base.txt";
	if (argc > 5) {
		argc = 5;
		printf("ERROR! Too many input parameters, is %d but max is 5. I quit!\n", argc);
		exitKontrollerat(__LINE__, 0);
	}

	problTyp = 0;
	weatherPath = "-";
	inputPath = "-";
	outputPath = "-";
	int node = -1;
	int manad = -1;
	for (int i = 1; i < argc; i++) {
		userGivenOK = readUserParam(argv[i], &dataName);
		if (userGivenOK >= 1) {
			if (userGivenOK <= 2) { // "--input=" or "--output="
				if (userGivenOK == 1)
					inputPath = dataName;// "--input="
				else
					outputPath = dataName;// "--output="
				if (problTyp != 1) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 1);
					problTyp = 1; // forecast opt
				}
			}
			else if (userGivenOK <= 4) {
				if (userGivenOK == 3)
					inputPath = dataName; // "--inputAutoRoute="
				else
					outputPath = dataName; // "--outputAutoRoute="
				if (problTyp != 2) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 2);
					problTyp = 2; // auto route
				}
			}
			else if (userGivenOK == 5) {
				weatherPath = dataName; // "--weatherDirectory="
			}
			else if (userGivenOK == 10) { // "--opt="
				// what to do here, --opt??
				// weatherPath = dataName;
			}
			else if (userGivenOK == 11) {
				// what to do here, --test??
				inputPath = dataName;
				problTyp = 5;
			}
			else if (userGivenOK == 12) {
				// what to do here, --inputStormFix??
				inputPath = dataName;
				problTyp = 7;
			}
			else if (userGivenOK == 13) {
				// what to do here, --inputKaoutar??
				// weatherPath = dataName;
				inputPath = dataName;
				problTyp = 8;
			}
			else if (userGivenOK <= 15) {
				if (userGivenOK == 14)
					inputPath = dataName; // "--inputGribFileTest="
				else
					outputPath = dataName; // "--outputGribFileTest="
				if (problTyp != 3) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 3);
					problTyp = 3; // GribFileTest
				}
			}
			else if (userGivenOK <= 17) {
				if (userGivenOK == 16)
					inputPath = dataName; // "--inputCreateRaster=
				else
					outputPath = dataName; // "--outputCreateRaster="
				if (problTyp != 4) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 4);
					problTyp = 4; // CreateDelayFactors
				}

			}
			else if (userGivenOK == 18) { // "--inputSeaRoute="
				inputPath = dataName;
				problTyp = 9;
			}
			else if (userGivenOK == 20) { // "--forecast="
				runAltForecast = stoi(dataName);
			}
			else {
				errlog("ERROR! Skipping input no %d %s, problType %d\n", i, argv[i], problTyp);
			}
		}
		else {
			if ((problTyp == 3 || problTyp == 4) && i == 3) {
				node = char_to_int(argv[3]);
			}
			else if (problTyp == 4 && i == 4) {
				manad = char_to_int(argv[4]);
			}else
				errlog("ERROR2! Skipping input no %d %s, problType %d\n", i, argv[i], problTyp);
		}
	}

	if (weatherPath != "-") {
		model.params.weatherPath = weatherPath;
		if (model.params.weatherPath.back() != '/\\' && model.params.weatherPath.back() != '/')
			model.params.weatherPath.push_back('/\\');
	}
	else {
		if (outputPath == "-") {
			if (inputPath.back() != '/' && inputPath.back() != '\\')
				inputPath += '/';
		}
		pathUse = splitFilename(inputPath);
		model.params.weatherPath = pathUse + "/weather/";
	}
	printf("weatherPath '%s'\n", model.params.weatherPath.c_str());

	if (problTyp == 1) {// OptiNav forecast or setRedisKeys
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
			printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
			exitKontrollerat(__LINE__, 0);
		}

		if (outputPath == "-") {
			resultPath = inputPath;
			SKRIV_UT_NOTHING = 0;
			LOGFILE = "logfile_setRedisKeys.txt";
			reset_errlog();
			printf("input file for redis key generation '%s'\n", inputPath.c_str());
			auto tid0 = std::chrono::high_resolution_clock::now();
			int returnVal = 1;
			if (inputPath != "-") {

				// returnVal = updateCorridors(inputPath);

				returnVal = saveTablesToSQLite(inputPath);
				//returnVal = saveMapsToBinary();

				SKRIV_UT_NOTHING = 0;
				returnVal = redisSetKeys(inputPath);
				printf("cleaning4\n");
			}
			if (returnVal != 0) {
				errlog("ERROR! Failed to set redis keys for weather\n");
				printf("ERROR! Failed to set redis keys for weather\n");
			}
			else
				printf("Setting of all the keys done\n");


			auto tid1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
			printf("cleaning5\n");
			printf("redis key generation took %.3lf\n", fp_ms);
			errlog("redis key generation took %.3lf\n", fp_ms);
			return 0;
		}
		resultPath = splitFilename(outputPath);
		filpek = fopen(outputPath.c_str(), "w");
		if (filpek == NULL) {
			errlog0("ERROR! Could not open file %s. Does the directory not exist or am I not allowed to write to that directory? I quit.\n",
				outputPath.c_str());
			printf("ERROR! Could not open file %s for writing. Is it locked or does the directory not exist? I quit.\n",
				outputPath.c_str());
			exitKontrollerat(__LINE__, 0);
		}
		fprintf(filpek, "{\nerror\n}\n");
		fclose(filpek);

		printf("Calling OptiNav with input '%s' and output '%s'\n", inputPath.c_str(), outputPath.c_str());
		auto tid0 = std::chrono::high_resolution_clock::now();
		if (inputPath != "-") {
			if (runAltForecast == 0)
				retVal = voyageOpt(inputPath, outputPath);
			else {
				if (runAltForecast != 10000)
					retVal = voyageOpt_fixPartSol(inputPath, outputPath);
				else
					retVal = dump_weatherForecasts(inputPath, outputPath);
			}
		}

		auto tid1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
		printf("OptiNav took %.3lf\n", fp_ms);
		errlog("OptiNav took %.3lf\n", fp_ms);

	}
	else if (problTyp == 2) {// autoRoute
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from input flags. I quit.\n");
			printf("ERROR! Did not manage to identify an input name from input flags. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		if (outputPath == "-") {
			errlog0("ERROR! Did not manage to identify a result name from input flags. I quit.\n");
			printf("ERROR! Did not manage to identify a result name from input flags. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		resultPath = splitFilename(outputPath);
		filpek = fopen(outputPath.c_str(), "w");
		if (filpek == NULL) {
			errlog0("ERROR! Could not open file %s. Does the directory not exist or am I not allowed to write to that directory? I quit.\n",
				outputPath.c_str());
			printf("ERROR! Could not open file %s for writing. Is it locked or does the directory not exist? I quit.\n",
				outputPath.c_str());
			exitKontrollerat(__LINE__, 0);
		}
		fprintf(filpek, "{\nerror\n}\n");
		fclose(filpek);

		LOGFILE = "logfile_autoRoute.txt";
		printf("Calling OptiNav-autoRoute with input '%s' and output '%s'\n", inputPath.c_str(), outputPath.c_str());
		auto tid0 = std::chrono::high_resolution_clock::now();
		if (inputPath != "-")
			genAutoRoute(inputPath, outputPath);

		auto tid1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
		printf("OptiNav-autoRoute took %.3lf\n", fp_ms);
		errlog("OptiNav-autoRoute took %.3lf\n", fp_ms);
	}
	else if (problTyp == 3) {// grib info
		if (node == -1) {
			errlog("ERROR! Params to run gribInfo given but no node. I quit\n");
			exitKontrollerat(__LINE__, 0);
		}
		LOGFILE = "logfile_gribInfo.txt";
		SKRIV_UT_NOTHING = 0;

		auto tid0 = std::chrono::high_resolution_clock::now();
		printf("Calling OptiNav to test grib files, input file '%s'\n", inputPath.c_str());
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from input flags. I quit.\n");
			printf("ERROR! Did not manage to identify an input name from input flags. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		else {
			resultPath = splitFilename(outputPath);
			model.params.checkGribFilesSpecial = 1;
			int returnVal = generateDelayedFactors(inputPath, node, 0);

		}
		auto tid1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
		printf("OptiNav to test grib files took %.3lf\n", fp_ms);
		errlog("OptiNav to test grib files took %.3lf\n", fp_ms);
	}
	else if (problTyp == 4) {// generate delay raster
		LOGFILE = "logfile_delayedFactors.txt";
		SKRIV_UT_NOTHING = 0;

		auto tid0 = std::chrono::high_resolution_clock::now();
		printf("Calling OptiNav to calculate delay factors with input '%s'\n", inputPath.c_str());
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
			printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
			exitKontrollerat(__LINE__, 0);
		}
		else {
			resultPath = splitFilename(outputPath);
			int returnVal = generateDelayedFactors(inputPath, node, manad);

		}
		auto tid1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
		printf("OptiNav calculate delay factors took %.3lf\n", fp_ms);
		errlog("OptiNav calculate delay factors took %.3lf\n", fp_ms);
	}
	else if (problTyp == 5) {// test call
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from. I quit.\n");
			printf("ERROR! Did not manage to identify an input name from. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		printf("%s\n", inputPath.c_str());
		return 2;
	}
	else if (problTyp == 7) {// fix stormValues
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from. I quit.\n");
			printf("ERROR! Did not manage to identify an input name from. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		LOGFILE = "logfile_fixStormValues.txt";
		int returnVal = 1;
		if (inputPath != "-") {
			returnVal = fixStormFiles(inputPath);
		}
		if (returnVal != 0) {
			errlog("ERROR! Failed to fix storm files\n");
			printf("ERROR! Failed to fix storm files\n");
		}
		else
			printf("All storm files fixed\n");
	}
	else if (problTyp == 8) {// Kaoutar data
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from. I quit.\n");
			printf("ERROR! Did not manage to identify an input name from. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		LOGFILE = "logfile_kaoutarData.txt";
		int returnVal = 1;
		if (inputPath != "-") {
			returnVal = evalKaoutarData(inputPath);
		}
		if (returnVal != 0) {
			errlog("ERROR! Failed to calculate weather factors for Kaoutar\n");
			printf("ERROR! Failed to calculate weather factors for Kaoutar\n");
		}
		else
			printf("Weather factors for Kaoutar calculated\n");
	}
	else if (problTyp == 9) {// generate new paths to seaRoute
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from. I quit.\n");
			printf("ERROR! Did not manage to identify an input name from. I quit.\n");
			exitKontrollerat(__LINE__, 0);
		}
		LOGFILE = "logfile_seaRouteExtend.txt";
		int returnVal = 1;
		if (inputPath != "-") {
			returnVal = evalSeaRoutePaths(inputPath);
		}
		if (returnVal != 0) {
			errlog("ERROR! Failed to calculate new seaRoute paths\n");
			printf("ERROR! Failed to calculate new seaRoute paths\n");
		}
		else
			printf("New seaRoute paths calculated, saved in autoRoute/newSeaRoutes.txt to be added to seaRoutes\n"
				"and autoRoute/newSeaRoutes.geojson to be visualized\n");
	}

	return retVal;
}

/*
int oldMain(){
	if (argc >= 4) {
		if (argc == 4) {
			inputPath = "-";
			LOGFILE = "logfile_gribInfo.txt";
			SKRIV_UT_NOTHING = 0;

			for (int i = 1; i < 3; i++) {
				userGivenOK[i] = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					exitKontrollerat(__LINE__, 0);
				}
			}
			auto tid0 = std::chrono::high_resolution_clock::now();
			printf("Calling OptiNav to calculate delay factors with input '%s'\n", inputPath.c_str());
			if (inputPath == "-") {
				errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				exitKontrollerat(__LINE__, 0);
			}
			else {
				resultPath = splitFilename(dataName);
				int node = char_to_int(argv[3]);
				model.params.checkGribFilesSpecial = 1;
				int returnVal = generateDelayedFactors(inputPath, node, 0);

			}
			auto tid1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
			printf("OptiNav calculate delay factors took %.3lf\n", fp_ms);
			errlog("OptiNav calculate delay factors took %.3lf\n", fp_ms);

		}
		else {
			inputPath = "-";
			LOGFILE = "logfile_delayedFactors.txt";
			SKRIV_UT_NOTHING = 0;

			for (int i = 1; i < 3; i++) {
				userGivenOK = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					exitKontrollerat(__LINE__, 0);
				}
			}
			auto tid0 = std::chrono::high_resolution_clock::now();
			printf("Calling OptiNav to calculate delay factors with input '%s'\n", inputPath.c_str());
			if (inputPath == "-") {
				errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				exitKontrollerat(__LINE__, 0);
			}
			else {
				resultPath = splitFilename(dataName);
				int node = char_to_int(argv[3]);
				int manad = char_to_int(argv[4]);
				int returnVal = generateDelayedFactors(inputPath, node, manad);

			}
			auto tid1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
			printf("OptiNav calculate delay factors took %.3lf\n", fp_ms);
			errlog("OptiNav calculate delay factors took %.3lf\n", fp_ms);
		}
	}
	else {
		inputPath = "-";
		resultPath = "-";
		if (argc == 3) {
			//SKRIV_UT_NOTHING = 0;

			for (int i = 1; i < 3; i++) {
				userGivenOK = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					exitKontrollerat(__LINE__, 0);
				}
			}
			if (inputPath == "-") {
				errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				exitKontrollerat(__LINE__, 0);
			}
			if (dataName == "-") {
				errlog0("ERROR! Did not manage to identify a result name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify a result name from %s or %s. I quit.\n", argv[1], argv[2]);
				exitKontrollerat(__LINE__, 0);
			}
			resultPath = splitFilename(dataName);
			filpek = fopen(dataName.c_str(), "w");
			if (filpek == NULL) {
				errlog0("ERROR! Could not open file %s. Does the directory not exist or am I not allowed to write to that directory? I quit.\n",
					dataName.c_str());
				printf("ERROR! Could not open file %s for writing. Is it locked or does the directory not exist? I quit.\n",
					dataName.c_str());
				exitKontrollerat(__LINE__, 0);
			}
			fprintf(filpek, "{\nerror\n}\n");
			fclose(filpek);

			//printf("pass 1\n");
			//freopen("output.txt", "w", stdout);
			//cout << str;

			if (userGivenOK == 1) {
				printf("Calling OptiNav with input '%s' and output '%s'\n", inputPath.c_str(), dataName.c_str());
				auto tid0 = std::chrono::high_resolution_clock::now();
				if (inputPath != "-")
					voyageOpt(inputPath, dataName);

				auto tid1 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
				printf("OptiNav took %.3lf\n", fp_ms);
				errlog("OptiNav took %.3lf\n", fp_ms);
			}
			else {
				LOGFILE = "logfile_autoRoute.txt";
				printf("Calling OptiNav-autoRoute with input '%s' and output '%s'\n", inputPath.c_str(), dataName.c_str());
				auto tid0 = std::chrono::high_resolution_clock::now();
				if (inputPath != "-")
					genAutoRoute(inputPath, dataName);

				auto tid1 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
				printf("OptiNav-autoRoute took %.3lf\n", fp_ms);
				errlog("OptiNav-autoRoute took %.3lf\n", fp_ms);

			}
		}
		else {
			if (argc == 2) {
				int i = 1;
				//SKRIV_UT_NOTHING = 0;
				inputPath = "-";
				userGivenOK = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					exitKontrollerat(__LINE__, 0);
				}
				if (inputPath == "-") {
					errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
					printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
					exitKontrollerat(__LINE__, 0);
				}
				if(userGivenOK == 2) {
					printf("%s\n", inputPath.c_str());
					return 2;
				}


				if (userGivenOK == 1) {
					LOGFILE = "logfile_setRedisKeys.txt";
					printf("input file for redis key generation '%s'\n", inputPath.c_str());
					auto tid0 = std::chrono::high_resolution_clock::now();
					int returnVal = 1;
					if (inputPath != "-") {
						returnVal = saveTablesToSQLite(inputPath);
						//returnVal = saveMapsToBinary();

						returnVal = redisSetKeys(inputPath);
					}
					if (returnVal != 0) {
						errlog("ERROR! Failed to set redis keys for weather\n");
						printf("ERROR! Failed to set redis keys for weather\n");
					}
					else
						printf("Setting of all the keys done\n");


					auto tid1 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
					printf("redis key generation took %.3lf\n", fp_ms);
					errlog("redis key generation took %.3lf\n", fp_ms);
				}
				else if (userGivenOK == 3) {
					// stormFix
					LOGFILE = "logfile_fixStormValues.txt";
					int returnVal = 1;
					if (inputPath != "-") {
						returnVal = fixStormFiles(inputPath);
					}
					if (returnVal != 0) {
						errlog("ERROR! Failed to fix storm files\n");
						printf("ERROR! Failed to fix storm files\n");
					}
					else
						printf("All storm files fixed\n");
				}
				else if (userGivenOK == 4) {
					// stormFix
					LOGFILE = "logfile_kaoutarData.txt";
					int returnVal = 1;
					if (inputPath != "-") {
						returnVal = evalKaoutarData(inputPath);
					}
					if (returnVal != 0) {
						errlog("ERROR! Failed to calculate weather factors for Kaoutar\n");
						printf("ERROR! Failed to calculate weather factors for Kaoutar\n");
					}
					else
						printf("Weather factors for Kaoutar calculated\n");
				}
			}
			else {
				LOGFILE = "logfile_error.txt";
				printf("%d arguments read, should be two\n", argc);
			}
		}
	}
	return 0;
}
*/
