// weatherTest.cpp : Defines the entry point for the application.
//

#include "weatherTest.h"
#include <time.h>
#include <chrono>

#include <iostream>
#include <unordered_map>
//#include<fstream>

#ifndef WIN32
#include </usr/local/include/sw/redis++/redis++.h>
//#include <sw/redis++/redis++.h>
 using namespace sw::redis;
//#include <redox.hpp>
//#include </usr/local/include/redox.hpp>
//using namespace redox;
#endif

using namespace std;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

string weatherDataPath; 

string resultPath;

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
	printf("start test\n");
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
	return 0;

}

int main(int argc, char* argv[])
{
	string dataName, inputPath;
	int nAnropData, userGivenOK;
	FILE* filpek;

	testing(2);

	cout << "Hello CMake. Test 2" << endl;
	cout << "nArgc " << argc << endl;

	for (int i = 0; i < argc; i++)
		cout << argv[i] << endl;

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
					exitKontrollerat(__LINE__, 0);
				}
			}
			//inputPath = "indataLokesh";
			//resultPath = "resultLokesh";
			//inputPath = argv[1];
			//resultPath = argv[2];
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
			printf("input file '%s'\n", inputPath.c_str());
			printf("result file '%s'\n", dataName.c_str());

			resultPath = splitFilename(dataName);
			printf("result path '%s'\n", resultPath.c_str());
			//dataName = resultPath;// +"/result.json";
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

#ifndef WIN32
			auto redis = Redis("tcp://127.0.0.1:6379/1");
			// std::cout << redis.ping() << std::endl;

			auto val = redis.get("optimizer_database_weather:icetk0");
			if (val) {
				std::cout << "Tjoho!! Got an answer from icetk0" << std::endl;
				//std::ofstream out("out.txt");
				//std::streambuf* coutbuf = std::cout.rdbuf(); //save old buf
				//std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
				freopen("output.txt", "w", stdout);
				std::cout << *val << std::endl;
			}
			else
				std::cout << "ERROR! No value from icetk0" << std::endl;
			exit(0);

			using Attrs = std::vector<std::pair<std::string, std::string>>;

			// You can also use std::unordered_map, if you don't care the order of attributes:
			// using Attrs = std::unordered_map<std::string, std::string>;

			Attrs attrs = { {"f1", "v1"}, {"f2", "v2"} };
			auto id = redis.xadd("key", "*", attrs.begin(), attrs.end());

			using Item = std::pair<std::string, Optional<Attrs>>;
			using ItemStream = std::vector<Item>;

			std::unordered_map<std::string, ItemStream> result;
			auto id2 = "$";
			//redis.xread("optimizer_database_weather:icetk0", id2, 10, std::inserter(result, result.end()));
			redis.xread("optimizer_database_weather:icetk0", id, 10, std::inserter(result, result.end()));
			printf("size of result %d\n", result.size());
			redis.xread("optimizer_database_weather:icetk0", id, 10000000, std::inserter(result, result.end()));
			printf("size of result %d\n", result.size());


			std::cout << "\nIterate and print key-value pairs using C++17 structured binding:\n";
			for (const auto& [key, value] : result) {
				std::cout << "Key:[" << key << "] Value:[\n";
				for (auto i : value) {
					auto [a, b] = i;
					std::cout << a;
					std::cout << " .. ";
					//std::cout << b;
					std::cout << "\n";
				}
			}

			auto val2 = redis.get("optimizer_database_weather:icetk0");
			if (val2) {
				std::cout << "Tjoho!! Got an answer from icetk0" << std::endl;
				//std::ofstream out("out.txt");
				//std::streambuf* coutbuf = std::cout.rdbuf(); //save old buf
				//std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
				freopen("output.txt", "w", stdout);
				std::cout << *val2 << std::endl;
			}
			else
				std::cout << "ERROR! No value from icetk0" << std::endl;

			//redis.set("testKey", "testValue");
			//auto value = redis.get("testKey");
			//if (value) {
			//	std::cout << "TjohoLiten" << std::endl;
			//	std::cout << *value << std::endl;
			//}else
			//	std::cout << "ERROR! No value from testKey" << std::endl;



			//Redox rdx;
			//if (!rdx.connect("localhost", 6379))
			//	printf("ERROR! Could not connect to redox\n");
			//else {
			//	cout << "Hello, " << rdx.get("hello") << endl;
			//	rdx.disconnect();
			//}

			exit(0);
#endif


			printf("Calling voyageOpt with input '%s' and output '%s'\n", inputPath.c_str(), dataName.c_str());
			auto tid0 = std::chrono::high_resolution_clock::now();
			if(inputPath != "-")
				voyageOpt(inputPath, dataName);

			auto tid1 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
			printf("voyageOpt took %.3lf\n", fp_ms);
			errlog("voyageOpt took %.3lf\n", fp_ms);
		}
		else {
			printf("%d arguments read, should be two\n", argc);
			inputPath = "testIndata";
			resultPath = "testResults";
			filpek = fopen("test.txt", "w");
			fprintf(filpek, "testing\n");
			fclose(filpek);
			printf("testFinal\n");
			printf("Calling voyageOpt with arguments %s and %s\n", inputPath.c_str(), resultPath.c_str());
			voyageOpt_old(inputPath);
			printf("All done. give 'weatherDataPath dataName' or 'weatherDataPath dataName nAnropData' if you want to test more\n");
		}
	}
	return 0;
}
