// weatherTest.cpp : Defines the entry point for the application.
//

#include "weatherTest.h"
#include <time.h>
#include <chrono>

#include <iostream>
#include <fstream>
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
int SKRIV_UT_NOTHING = 2;
int SEND_POST_REQUEST = 1;
int runAltForecast = 0;
int SPARA_RUN_DATA = 0;

extern strModel model;

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
	findData = givenData.find("--inputAutoRoute=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 17, givenData.size() - 8);
		return 2;
	}
	findData = givenData.find("--outputAutoRoute=");
	if (findData < givenData.size()) {
		*outPath = givenData.substr(findData + 18, givenData.size() - 9);
		return 2;
	}
	findData = givenData.find("--inputStormFix=");
	if (findData < givenData.size()) {
		*inPath = givenData.substr(findData + 16, givenData.size() - 8);
		return 3;
	}
	return 0;

}

int readUserParam(char* argv, std::string* pathTmp) {
	int i, likaPos = -1;
	std::string givenData = argv;


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

/*
struct MemoryStruct {
	char* memory;
	size_t size;
};

static size_t
WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = (struct MemoryStruct*)userp;

	char* ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
*/

int call_api_corridors(std::string resultPath) {
	// download new corridors from api to file inputPath/tmp_corridors.json
	
	int retVal = 0;
	CURL* curl;
	CURLcode res;
	FILE* file;
	char* fileName;


	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
		//if (model.params.url_getCorridors != "")
		// curl_easy_setopt(curl, CURLOPT_URL, "https://optinav-api-beta.tnmservices.ai/api/ivado/get-corridors");
		curl_easy_setopt(curl, CURLOPT_URL, model.params.url_getCorridors.c_str());
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

		char* namn = (char*)malloc(256 * sizeof(char));
		sprintf(namn, "%s/corridors_downloaded.json", resultPath.c_str());
		//const char* namn = "data/corridors_tmp.json";
		FILE* fp = fopen(namn, "wb");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		struct curl_slist* headers = NULL;
		headers = curl_slist_append(headers, "sec-ch-ua-platform: \"Windows\"");
		headers = curl_slist_append(headers, "Referer: https://fleetview2-client.tnmservices.com/");
		headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/138.0.0.0 Safari/537.36");
		headers = curl_slist_append(headers, "sec-ch-ua: \"Not)A;Brand\";v=\"8\", \"Chromium\";v=\"138\", \"Google Chrome\";v=\"138\"");
		headers = curl_slist_append(headers, "Content-Type: application/json");
		headers = curl_slist_append(headers, "sec-ch-ua-mobile: ?0");
		std::string token = load_entire_file(resultPath + "/autoRoute/token_id.txt");
		std::string authorization = "Authorization: Bearer " + token;
		// headers = curl_slist_append(headers, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJhdWQiOiIyIiwianRpIjoiYzA3ZTExOTcwMzY2ZDViM2Q2NDhlMDM4NmI3MTlmYWQ3YWExOWVlYjJkOWMxYzVmM2ZhMDYzZDhjOTc1ODUyMzM3NzNiODJkYTI0YWJjYjgiLCJpYXQiOjE3NjgyMTY4ODIuNzgyOTA1LCJuYmYiOjE3NjgyMTY4ODIuNzgyOTA3LCJleHAiOjE4MzEyODg4ODIuNTAzODE1LCJzdWIiOiI3MCIsInNjb3BlcyI6WyIqIl19.DudizR1tcwVm_dRmYUtf5G3ucU5Kuyx6M90t9XoLruc6iUIepUkqzmIgVaS7wS75p1OU_9WhKXxC11h8a9G-QtwF8BpEpCgJ_RZao2wsxS_jZl6Yofn0KwqwVSDNGik1HKh9d-YMFPk7CuMT0BE5pyQ7wpqEzw86IVEoXvMafEpzWwWPD-FZrUCX6Kn7Ook8voftcBqwv6-CfYzczaOEwx2z2Ff5N_Zvxxt7p9228uMOn7HLWWm-vCGiFI4GNQdWtWpnE7SjeRel66VVyoHIIRFz00gfUBEmMOYCDc7SbrdmKe5vSIRDjeL4CvoWDGVqKfb8P71_se7VaIvgPSZXqX7b0qAGfWVtI8CuVfkiWULp800X9APCVs22-yA1f4lUrXush7i1EsAXDUS5zlE4ZhJzohEAoGa-GC86O9hATl_Mp1kMQb0hKaVmKnYE1uzSVwLe2ESNVez2Qizm9nXBERK2kYByLgpDOsys6vFnutFOtejgMofjaRxB5xKz5cDFJyKo98-7Jr0I57ajkn9UtPiEF6j9tZ0I51xp4gFSZf_fPCJHW61LmQsa8onX6CAWgTfNuqX0UA-I-ms1qTu-KWWzllFru0bxKKfg2dUgmhd2vnjPdcsZY7Ou31GTLBzznJE4VKPq880ccYDMtD9xmWuxA538z6rPBT98zil-hik");
		printf("url api: '%s'\n", model.params.url_getCorridors.c_str());
		//printf("curl authorization: '%s'\n", authorization.c_str());
		headers = curl_slist_append(headers, authorization.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		const char* data = "";
		printf("here1\n");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		printf("here1b\n");
		res = curl_easy_perform(curl);
		printf("here1bb\n");

		fclose(fp);
		printf("here1c\n");
		curl_slist_free_all(headers);
		printf("here1d\n");
	}
	curl_easy_cleanup(curl);

	return retVal;
}

void postRequest(std::string errorMessage, int endProgram) {

	if (endProgram == 1) {
		FILE* filPek3 = fopen(model.params.resultName.c_str(), "w"); // "result_json.json", "w");
		fprintf(filPek3, "{\n\t\"errorMessage\": \"%s\"\n}\n", errorMessage.c_str());
		fclose(filPek3);
	}

	errorMessage.append(", hindcast " + std::to_string(model.params.hindCast));
	
	char hostname[256];
	char username[256];
	int result;
	result = gethostname(hostname, 256);
	if(!result)
		errorMessage.append(", computerName " + std::string(hostname));
	result = getlogin_r(username, 256);
	if (!result)
		errorMessage.append(", user " + std::string(username));

	if(SKRIV_UT_NOTHING <= 1)
		errlog("postRequest: %s", errorMessage.c_str());
	printf("postRequest: %s\n", errorMessage.c_str());

	if (SEND_POST_REQUEST == 1) {
		if (model.params.url_errorEmail_api == "") {
			printf("ERROR! No url given for the api to send emails. I skip this\n");
		}
		else {
			char* namn;
			namn = (char*)malloc2(256 * sizeof(char));
			char* datumNamn = (char*)malloc2(256 * sizeof(char));
			char* namnDir = (char*)malloc2(256 * sizeof(char));

			time_t rawtime;
			time(&rawtime);
			struct tm tmBas = *localtime(&rawtime);
			// struct tm tmBas = { std::time(0) };
			//setTMtime(&tmBas, endTime);
			fixReadableDate_file(tmBas, datumNamn);
			sprintf(namnDir, "%s/postRequestFiles", model.params.resultPath.c_str());
			struct stat sb;
			if (stat(namnDir, &sb) != 0) {
				mkdir(namnDir, 0777);
			}
			sprintf(namn, "%s/postRequestFiles/input_%s", model.params.resultPath.c_str(), datumNamn);
			//sprintf(namn, "%s", model.params.indataPathName.c_str());
			write_copyAtoB(namn, (char*)"json", (char*)model.params.indataPathName.c_str(), (char*)"w");
			errlog("OBS! Sending the following message to POST and saves the input file as %s\n%s.json\n", errorMessage.c_str(),
				namn);

			CURL* curl;
			CURLcode res;
			curl = curl_easy_init();
			if (curl) {
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
				// curl_easy_setopt(curl, CURLOPT_URL, "https://optinav-api-beta.tnmservices.ai/api/weather/notify");
				// curl_easy_setopt(curl, CURLOPT_URL, "https://optinav-a8cbbffregdneudk.eastus-01.azurewebsites.net/api/weather/notify");
				curl_easy_setopt(curl, CURLOPT_URL, model.params.url_errorEmail_api.c_str());
				curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
				curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
				struct curl_slist* headers = NULL;
				//headers = curl_slist_append(headers, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiIsImp0aSI6ImU5ODhjNjk3ZTI1NDA4ZWQzNTMzNjdhZmI4NmFkNzUzYmIyOWFlMWU3NzRmMzNiYWRiMDllZmYyOTdiNjE4ZjlmMDZhOTk3YmU3NWY2ZTM3In0.eyJhdWQiOiIxIiwianRpIjoiZTk4OGM2OTdlMjU0MDhlZDM1MzM2N2FmYjg2YWQ3NTNiYjI5YWUxZTc3NGYzM2JhZGIwOWVmZjI5N2I2MThmOWYwNmE5OTdiZTc1ZjZlMzciLCJpYXQiOjE2NDc1MjE4NDksIm5iZiI6MTY0NzUyMTg0OSwiZXhwIjoxNjc5MDU3ODQ5LCJzdWIiOiIyIiwic2NvcGVzIjpbXX0.UVbHJMid3B_5WyzD5VJ9AA1wllGtlr_aK4JRuQ66jRgSmn0fZGzB6D4Cm97sFUSltHp8cOPfQf0jOTC_sjFz0UoFGckSNrbw0GTwue3h9cduvdSZB7rUB7VgR_0XOL6hOiEgPzBOQU4okDwp52KZ5avZDE8x5PWF76qADJ2_835_9AMOq-myBQwFkysFiohJDZo5GS0MabVilJ58tls94KhX2er_8qj2_SpYGVWUVCCy_FYe8XnVrXOSO7j06LYvtpkR5Lspcp4Z9egDGb-NcqB80x9ilNc1CzzClt1DC1yMUUyTo1Z0162A6vxh5vM0Ly0pEX2r3UNfNDWo4-IDH-BB1aczK-43NTE2yafpPqHklj6FvzhdJAHX3Pht3SBFrHT2IG15yFeCj1fhJB9oHTwLnG4BYOmWwO6FohV5DSEolrFTOLWA1MoOrztN-xx4nmrmM6p53awVrRanNMbwnh6X7qPqS668Kd9ZQmR-EkyYHxEvib1YitOH7smnTFzI2P5Jfymf9K2fti3AyzzLGVa3HCKUHSaHU6yMaLk4ZECqRAcxOaYjQZFFJTqWSyY9weozmR1M-GdGFJ1shI9qqDl9utcCPoZ0-IxsJ8hKoVYT2KmqgAd-9vZLAXB2p_Q0twl1riqMyzg1J2W52HNNv8Mcu3WVZOWpLGjHuiy_O9o");
				//headers = curl_slist_append(headers, "Cookie: XSRF-TOKEN=eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJhdWQiOiIyIiwianRpIjoiNDA3YTc1YTUzNDc3MGU2NDRjOTBlN2E0ZTNiODgyYTEzM2YzMzBlM2QwMjQ5MzcwYzFmYzJmYzYwYzcyMTZiNTBhYmY2OGNmNmFkOWUxNWYiLCJpYXQiOjE2NzkwODQ4NzUuMzAwMjYzLCJuYmYiOjE2NzkwODQ4NzUuMzAwMjY3LCJleHAiOjQ4MzQ3NTg0NzUuMjgwNDQ0LCJzdWIiOiIyIiwic2NvcGVzIjpbIioiXX0.lIY-jGjOFBVk_SkxAyDMeD8HIeZ3bZKb_d4q3N4LM4JJ8lnRYd9O6yFh1x5aTOuOJOyEQbfjQklBjmCl7OQlLqjRilsmp7X9O196tM-44s036MdTq8jkVQHRBrKFK0AqK2v58ZJsrD1fQVMcIZ4694vpHaLJDaCUN9VhOA1hcAATZP7hXs-lbLnP1ajoTLwGkctnaAVfIHvapkcWd1RTSGYBud42WV-CUdVKUYSBP9ej70BK5G0OZJbK5Gtnwqp2CdnOyL-mIWIfjTciu2Mo2YTYnfv6kfBIcSrmWYqVrb5VMCYwn6GS14S6ZycIiEmLL_o1Xvt-E1pG8F1Uvv9vSowrxKHio6ulWxzWroX0YWFNqeAhu0_gdUnafK9kKHPCHvVwjd179oUzz2DbT7OiEwtiMCxy_icF-A-As1YX8c9qTnd1KF1cV7C33eb4_ds5n5d1g6CbcqmhhLitEpXrU5K2yqFd8u_0gkuyMgg7PLYSXYd_lLSJROmqvh_VKziN4xJ_k16Ho3WM2Pwy4akMsbJ567hnNeQ-Kt6lF4JsTrkl0IJ-7L4sPO8T3ehlyn-cvN8FUv5ISt_ohzM-J8GBfZfIAWTbBg9P7D1xNqwszbgpqyBm-4nU5R--iUpkew95WH364SO4uT2DwEg1DgR4zRdoYpPqKtiH1U1CqFUew0o");
				// headers = curl_slist_append(headers, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJhdWQiOiIyIiwianRpIjoiNDA3YTc1YTUzNDc3MGU2NDRjOTBlN2E0ZTNiODgyYTEzM2YzMzBlM2QwMjQ5MzcwYzFmYzJmYzYwYzcyMTZiNTBhYmY2OGNmNmFkOWUxNWYiLCJpYXQiOjE2NzkwODQ4NzUuMzAwMjYzLCJuYmYiOjE2NzkwODQ4NzUuMzAwMjY3LCJleHAiOjQ4MzQ3NTg0NzUuMjgwNDQ0LCJzdWIiOiIyIiwic2NvcGVzIjpbIioiXX0.lIY-jGjOFBVk_SkxAyDMeD8HIeZ3bZKb_d4q3N4LM4JJ8lnRYd9O6yFh1x5aTOuOJOyEQbfjQklBjmCl7OQlLqjRilsmp7X9O196tM-44s036MdTq8jkVQHRBrKFK0AqK2v58ZJsrD1fQVMcIZ4694vpHaLJDaCUN9VhOA1hcAATZP7hXs-lbLnP1ajoTLwGkctnaAVfIHvapkcWd1RTSGYBud42WV-CUdVKUYSBP9ej70BK5G0OZJbK5Gtnwqp2CdnOyL-mIWIfjTciu2Mo2YTYnfv6kfBIcSrmWYqVrb5VMCYwn6GS14S6ZycIiEmLL_o1Xvt-E1pG8F1Uvv9vSowrxKHio6ulWxzWroX0YWFNqeAhu0_gdUnafK9kKHPCHvVwjd179oUzz2DbT7OiEwtiMCxy_icF-A-As1YX8c9qTnd1KF1cV7C33eb4_ds5n5d1g6CbcqmhhLitEpXrU5K2yqFd8u_0gkuyMgg7PLYSXYd_lLSJROmqvh_VKziN4xJ_k16Ho3WM2Pwy4akMsbJ567hnNeQ-Kt6lF4JsTrkl0IJ-7L4sPO8T3ehlyn-cvN8FUv5ISt_ohzM-J8GBfZfIAWTbBg9P7D1xNqwszbgpqyBm-4nU5R--iUpkew95WH364SO4uT2DwEg1DgR4zRdoYpPqKtiH1U1CqFUew0o");
				// headers = curl_slist_append(headers, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJSUzI1NiJ9.eyJhdWQiOiIyIiwianRpIjoiYzA3ZTExOTcwMzY2ZDViM2Q2NDhlMDM4NmI3MTlmYWQ3YWExOWVlYjJkOWMxYzVmM2ZhMDYzZDhjOTc1ODUyMzM3NzNiODJkYTI0YWJjYjgiLCJpYXQiOjE3NjgyMTY4ODIuNzgyOTA1LCJuYmYiOjE3NjgyMTY4ODIuNzgyOTA3LCJleHAiOjE4MzEyODg4ODIuNTAzODE1LCJzdWIiOiI3MCIsInNjb3BlcyI6WyIqIl19.DudizR1tcwVm_dRmYUtf5G3ucU5Kuyx6M90t9XoLruc6iUIepUkqzmIgVaS7wS75p1OU_9WhKXxC11h8a9G-QtwF8BpEpCgJ_RZao2wsxS_jZl6Yofn0KwqwVSDNGik1HKh9d-YMFPk7CuMT0BE5pyQ7wpqEzw86IVEoXvMafEpzWwWPD-FZrUCX6Kn7Ook8voftcBqwv6-CfYzczaOEwx2z2Ff5N_Zvxxt7p9228uMOn7HLWWm-vCGiFI4GNQdWtWpnE7SjeRel66VVyoHIIRFz00gfUBEmMOYCDc7SbrdmKe5vSIRDjeL4CvoWDGVqKfb8P71_se7VaIvgPSZXqX7b0qAGfWVtI8CuVfkiWULp800X9APCVs22-yA1f4lUrXush7i1EsAXDUS5zlE4ZhJzohEAoGa-GC86O9hATl_Mp1kMQb0hKaVmKnYE1uzSVwLe2ESNVez2Qizm9nXBERK2kYByLgpDOsys6vFnutFOtejgMofjaRxB5xKz5cDFJyKo98-7Jr0I57ajkn9UtPiEF6j9tZ0I51xp4gFSZf_fPCJHW61LmQsa8onX6CAWgTfNuqX0UA-I-ms1qTu-KWWzllFru0bxKKfg2dUgmhd2vnjPdcsZY7Ou31GTLBzznJE4VKPq880ccYDMtD9xmWuxA538z6rPBT98zil-hik");
				std::string token = load_entire_file(model.params.resultPath + "/autoRoute/token_id.txt");
				std::string authorization = "Authorization: Bearer " + token;
				printf("url api: '%s'\n", model.params.url_errorEmail_api.c_str());
				//printf("load token from '%s/autoRoute/token_id.txt'\n", model.params.resultPath.c_str());
				//printf("curl authorization: '%s'\n", authorization.c_str());
				headers = curl_slist_append(headers, authorization.c_str());

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
				printf("calling curl_easy_perform:\n\n");
				res = curl_easy_perform(curl);
				printf("\n... done with the call to curl_easy_perform\n\n");
				curl_mime_free(mime);
			}
			curl_easy_cleanup(curl);
		}
	}

	if (endProgram == 1) {
		exitKontrollerat(__LINE__);
	}

}

int main(int argc, char* argv[])
{
	std::string dataName, inputPath, outputPath, weatherPath, pathUse;
	int nAnropData, userGivenOK, problTyp;
	FILE* filpek;

	// testing(2);

	//cout << "Hello CMake. Test 2" << endl;
	//cout << "nArgc " << argc << endl;

	LOGFILE = "logfile.txt";
	//for (int i = 0; i < argc; i++)
	//	cout << argv[i] << endl;

	model.params.checkGribFilesSpecial = 0;
	model.delay.nYears = 0;

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
			if (userGivenOK <= 2) {
				if (userGivenOK == 1)
					inputPath = dataName;
				else
					outputPath = dataName;
				if (problTyp != 1) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 1);
					problTyp = 1; // forecast opt
				}
			}
			else if (userGivenOK <= 4) {
				if (userGivenOK == 3)
					inputPath = dataName;
				else
					outputPath = dataName;
				if (problTyp != 2) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 2);
					problTyp = 2; // auto route
				}
			}
			else if (userGivenOK == 5) {
				weatherPath = dataName;
			}
			else if (userGivenOK == 10) {
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
					inputPath = dataName;
				else
					outputPath = dataName;
				if (problTyp != 3) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 3);
					problTyp = 3; // GribFileTest
				}
			}
			else if (userGivenOK <= 17) {
				if (userGivenOK == 16)
					inputPath = dataName;
				else
					outputPath = dataName;
				if (problTyp != 4) {
					if (problTyp != 0)
						errlog("ERROR! OptiNav called with unknown combination of flags. Before was problTyp %d but now it is %d which I use.\n",
							problTyp, 4);
					problTyp = 4; // CreateDelayFactors
				}
			}
			else if (userGivenOK == 18) {
				inputPath = dataName;
				problTyp = 9;
			}
			else if (userGivenOK == 20) {
				runAltForecast = stoi(dataName);
			}
			else if ((problTyp == 3 || problTyp == 4) && i == 3) {
				node = char_to_int(argv[3]);
			}
			else if (problTyp == 4 && i == 4) {
				manad = char_to_int(argv[4]);
			}
			else {
				errlog("ERROR! Skipping input no %d %s, problType %d\n", i, argv[i], problTyp);
			}
		}
		else {
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

	if (problTyp == 1) {// OptiNav forecast or setRedisKeys
		if (inputPath == "-") {
			errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
			printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
			exitKontrollerat(__LINE__, 0);
		}

		if (outputPath == "-") {
			LOGFILE = "logfile_setRedisKeys.txt";
			printf("input file for redis key generation '%s'\n", inputPath.c_str());
			auto tid0 = std::chrono::high_resolution_clock::now();
			int returnVal = 1;
			if (inputPath != "-") {
				SKRIV_UT_NOTHING = 0;
				printf("pfg innan reset_errlog\n");
				resultPath = inputPath;
				reset_errlog();


				printf("pfg innan saveTablesToSQLite\n");
				returnVal = saveTablesToSQLite(inputPath);
				//returnVal = saveMapsToBinary();

				returnVal = redisSetKeys(inputPath);

				printf("pfg innan updateCorridors\n");
				//printf("\n\ntesting to call the api to update corridors\n");
				updateCorridors(inputPath);
				//exit(0);
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
				voyageOpt(inputPath, outputPath);
			else {
				//if (runAltForecast > 0)
				voyageOpt_fixPartSol(inputPath, outputPath);
				//else
				//	voyageEval_fixSol(inputPath, outputPath);
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
	return 0;
}

int main_old(int argc, char* argv[])
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
					postRequest("ERROR! Could not read user data '" + std::string(argv[i]) + "'. I quit!", 1);
				}
			}
			if (inputPath == "-") {
				errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
				postRequest("ERROR! Did not manage to identify an input name from " + std::string(argv[1]) + 
					" or " + std::string(argv[2]) + ". I quit.", 1);
			}
			if (dataName == "-") {
				errlog0("ERROR! Did not manage to identify a result name from %s or %s. I quit.\n", argv[1], argv[2]);
				printf("ERROR! Did not manage to identify a result name from %s or %s. I quit.\n", argv[1], argv[2]);
				postRequest("ERROR! Did not manage to identify a result name from " + std::string(argv[1]) +
					" or " + std::string(argv[2]) + ". I quit.", 1);
			}
			resultPath = splitFilename(dataName);
			filpek = fopen(dataName.c_str(), "w");
			if (filpek == NULL) {
				errlog0("ERROR! Could not open file %s. Does the directory not exist or am I not allowed to write to that directory? I quit.\n",
					dataName.c_str());
				printf("ERROR! Could not open file %s for writing. Is it locked or does the directory not exist? I quit.\n",
					dataName.c_str());
				postRequest("ERROR! Could not open file " + dataName + " for writing.Is it locked or does the directory not exist ? I quit.", 1);
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
				printf("two arguments\n");
				int i = 1;
				SKRIV_UT_NOTHING = 0;
				inputPath = "-";
				userGivenOK = setUserParam(argv[i], &inputPath, &dataName);
				if (userGivenOK == 0) {
					errlog0("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					printf("ERROR! Could not read user data '%s'. I quit!\n", argv[i]);
					postRequest("ERROR! Could not read user data '" + std::string(argv[i]) + "'. I quit!", 1);
				}
				if (inputPath == "-") {
					errlog0("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
					printf("ERROR! Did not manage to identify an input name from %s or %s. I quit.\n", argv[1], argv[2]);
					postRequest("ERROR2! Did not manage to identify an input name from " + std::string(argv[1]) +
						" or " + std::string(argv[2]) + ". I quit.", 1);
				}



				if (userGivenOK == 1) {
					LOGFILE = "logfile_setRedisKeys.txt";
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
						postRequest("ERROR! Failed to set redis keys for weather", 1);
					}
					else
						printf("Setting of all the keys done\n");


					auto tid1 = std::chrono::high_resolution_clock::now();
					std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
					printf("redis key generation took %.3lf\n", fp_ms);
					errlog("redis key generation took %.3lf\n", fp_ms);
				}
				else {
					// stormFix
					LOGFILE = "logfile_fixStormValues.txt";
					printf("input path for fix of storm values '%s'\n", inputPath.c_str());
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
			}
			else {
				LOGFILE = "logfile_error.txt";
				printf("%d arguments read, should be two\n", argc);
				postRequest("wrong number of arguments calling OptiNav", 1);

			}
		}
	}
	return 0;
}
