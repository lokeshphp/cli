// weatherTest.cpp : Defines the entry point for the application.
//

#include "weatherTest.h"
#include <time.h>
#include <chrono>

#include <iostream>
#include <unordered_map>
#include <sstream>

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

			printf("pass 1\n");
			float number;
			stringstream stream;
			stream.precision(3);
			stream << fixed;
			//testStruct testArray0;
			float* testArray0;
			testArray0 = (float*)malloc(900 * 451 * sizeof(float));

			printf("pass 1b\n");
			int pos = 0;
			for (int i = 0; i < 900; i++) {
				for (int i1 = 0; i1 < 451; i1++) {
					for (int i2 = 0; i2 < 1; i2++) {
						number = i / 100.2 + i1 / 50.34 + i2 / 38.2;
						stream << number << " ";
						//testArray0.varden[pos++] = number;
						testArray0[pos++] = number;
					}
				}
				stream << endl;
			}
			string str = stream.str();
			//freopen("output.txt", "w", stdout);
			//cout << str;

			printf("pass 1c\n");
			float* testArray;
			testArray = (float*)malloc(900 * 451 * sizeof(float));
			putStringIntoArrayFloat(str, testArray);
			printf("pass 1d\n");

			//vector <float> testVec;
			//istringstream ss(str);
			//copy(
			//	istream_iterator <float>(ss),
			//	istream_iterator <float>(),
			//	back_inserter(testVec)
			//);

			/*
			cout << endl << endl;
			int pos = 0;
			for (int i = 0; i < 900; i++) {
				for (int i1 = 0; i1 < 451; i1++) {
					for (int i2 = 0; i2 < 1; i2++) {
						cout << testVec[pos] << " ";
						pos++;
					}
				}
				cout << endl;
			}
			*/


			//exit(0);


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
			if (argc == 2) {
				int i = 1;
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
				printf("input file for redis key generation '%s'\n", inputPath.c_str());
				auto tid0 = std::chrono::high_resolution_clock::now();
				int returnVal = 1;
				if (inputPath != "-")
					returnVal = redisSetKeys(inputPath);
				if (returnVal != 0) {
					errlog("ERROR! Failed to set redis keys for weather\n");
					printf("ERROR! Failed to set redis keys for weather\n");
				}
				else
					printf("Setting of keys done\n");


				auto tid1 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
				printf("redis key generation took %.3lf\n", fp_ms);
				errlog("redis key generation took %.3lf\n", fp_ms);
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
	}
	return 0;
}
