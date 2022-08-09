#include "pch.h"
#include <cstdio>
#include <iostream>
#include<fstream>
//#include "stdlib.h"
//#include "string.h"
#include <cmath>
//#include"vector_gdal.cpp"
#include "json.hpp"
//#include "ogrsf_frmts.h"
#include <cstring>
//#include <boost/iostreams/filter/zlib.hpp>
#include <zlib.h>

#include "redisDef.h"
#include <sstream>
#include <time.h>

using json = nlohmann::json;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

strModel model;
extern string resultPath;
extern long long MAXVARDE_NATVERK;
int printGlobal = 0;

int freeMemory() {
	int i;

	for (i = 0; i < model.nNoder; i++) {
		free(model.Noder[i].UtNod);
		free(model.Noder[i].UtNodCost);
		free(model.Noder[i].outArcNr);
	}
	free(model.Noder);

	exit(0);
	return 0;
}

int checkMinnesAnvandning(int rad)
{
	int varde = 1;
#ifdef WIN32
	varde = _CrtCheckMemory();
#endif
	if (varde != 1)
		errlog("Error! Minnesbugg identifierad pa rad %d\n", rad);
	return varde;
}


inline bool exists_test3(char* name) {
	struct stat buffer;
	return (stat(name, &buffer) == 0);
}

void 	initModelStatusValues() {
	model.status.weatherHistoryOpenFile_fail = 0;
}

string splitFilename(string namn, int alt) {
	string resultat;
	size_t found;
	found = namn.find_last_of("/\\");
	if (alt == 0)
		resultat = namn.substr(0, found);
	else
		resultat = namn.substr(found + 1, namn.size());
	return resultat;
}

int callRaster()
{
	// create object of Geotiff class
	// Raster tiff((const char*)"trakt_contour.tif");
	Raster raster; //  (const char*)"WW3_NCEP.grb");

	// dump out array (band) dimensions of Geotiff data  
	int* dims = raster.GetDimensions();
	cout << dims[0] << " " << dims[1] << " " << dims[2] << endl;

	// output a value from 2D array  
	//float** rasterBandData = raster.GetRasterBand(1);
	float* rasterBandData;
	rasterBandData = (float*)malloc(raster.Get_nRows() * raster.Get_nCols() * sizeof(float));
	raster.GetRasterBand_ny(1, rasterBandData);
	cout << "value at row 10, column 30: " << rasterBandData[30 + raster.Get_nCols() * 10] << endl;

	// call other methods, like get the name of the Geotiff
	// passed-in, its length, and its projection string 
	cout << raster.GetFileName() << endl;
	cout << strlen(raster.GetFileName()) << endl;
	cout << raster.GetProjection() << endl;

	// dump out the Geotransform (6 element array of doubles) 
	double* gt = raster.GetGeoTransform();
	cout << gt[0] << " " << gt[1] << " " << gt[2] << " " << gt[3] << " " << gt[4] << " " << gt[5] << endl;

	// dump out Geotiff band NoData value (often it is -9999.0)
	//cout << "No data value: " << raster.GetNoDataValue() << endl;




	return 0;
}

int writePointsToGeojson(char* pszFilename, spherical::Point* point, int nPkter)
{
	errlog("ERROR! Implement writePointsToGeojson\n");
	/*
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	int nSHPType, nAllocPkter, i, iPos;
	int startPos, slutPos, pktFoljdNr, i3, arcNr, arcNr2;
	double* x, * y, * z;
	//const char *pszFilename = "pkterShape";

	nSHPType = SHPT_POINTZ;

	hSHPHandle = SHPCreate(pszFilename, nSHPType);

	x = (double*)malloc(2 * sizeof(double));
	y = (double*)malloc(2 * sizeof(double));
	z = (double*)malloc(2 * sizeof(double));

	for (iPos = 0; iPos < nPkter; iPos++)
	{
		// kopiera delen av punktfoljden som anvands, dess xyz
		y[0] = point[iPos].latitude().degrees();
		x[0] = point[iPos].longitude().degrees();
		z[0] = 0;

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			1, x, y, z, NULL); //  m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);
	}

	SHPClose(hSHPHandle);

	DBFHandle	hDBF;
	hDBF = DBFCreate(pszFilename);
	if (hDBF == NULL)
	{
		printf("DBFCreate(%s) failed.\n", pszFilename);
		exit(2);
	}
	if (DBFAddField(hDBF, "pktNr", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "y_row_lat", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "x_col_lon", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);

	for (iPos = 0; iPos < nPkter; iPos++)
	{
		// kopiera delen av punktfoljden som anvands, dess xyz
		x[0] = point[iPos].latitude().degrees();
		y[0] = point[iPos].longitude().degrees();
		DBFWriteIntegerAttribute(hDBF, iPos, 0, iPos);
		DBFWriteDoubleAttribute(hDBF, iPos, 1, point[iPos].latitude().degrees());
		DBFWriteDoubleAttribute(hDBF, iPos, 2, point[iPos].longitude().degrees());
	}
	DBFClose(hDBF);

	write_copyAtoB(pszFilename, (char*)"prj", (char*)"wgs84Def.prj", (char*)"w");
	test2(2);
	*/

	return 0;
}

int writeAllNodesToGeojson(char* pszFilename)
{
	int forsta = 1;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/allNodes.geojson", model.params.resultPath.c_str());
	filpekG = fopen(namn, "w"); 
	initGeoJsonFil(filpekG, "allPhysicalNodes");

	for (int i = 0; i < model.network.nPhysicalLevels; i++) {
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;

			if (forsta != 1)
				fprintf(filpekG, ", ");
			else
				forsta = 0;
			fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level1\":%d, \"nodPos1\":%d},\n",
				i, i1);
			fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n",
				model.network.physicalLev[i].point[i1].longitude().degrees(),
				model.network.physicalLev[i].point[i1].latitude().degrees());
		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	/*
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	int nSHPType, nAllocPkter, i, iPos;
	int startPos, slutPos, pktFoljdNr, i3, arcNr, arcNr2;
	double* x, * y, * z;
	//const char *pszFilename = "pkterShape";

	nSHPType = SHPT_POINTZ;

	hSHPHandle = SHPCreate(pszFilename, nSHPType);

	x = (double*)malloc(2 * sizeof(double));
	y = (double*)malloc(2 * sizeof(double));
	z = (double*)malloc(2 * sizeof(double));

	iPos = 0;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			// kopiera delen av punktfoljden som anvands, dess xyz
			if (model.network.physicalLev[i].allowedPoint[i1] == 1) {
				y[0] = model.network.physicalLev[i].point[i1].latitude().degrees();
				x[0] = model.network.physicalLev[i].point[i1].longitude().degrees();
				z[0] = 0;

				psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
					1, x, y, z, NULL); //  m);
				SHPWriteObject(hSHPHandle, -1, psShape);
				SHPDestroyObject(psShape);
				iPos++;
			}
		}
	}

	SHPClose(hSHPHandle);

	DBFHandle	hDBF;
	hDBF = DBFCreate(pszFilename);
	if (hDBF == NULL)
	{
		printf("DBFCreate(%s) failed.\n", pszFilename);
		exit(2);
	}
	if (DBFAddField(hDBF, "pktNr", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "y_row_lat", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "x_col_lon", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);

	iPos = 0;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {

			if (model.network.physicalLev[i].allowedPoint[i1] == 1) {
				DBFWriteIntegerAttribute(hDBF, iPos, 0, iPos);
				DBFWriteDoubleAttribute(hDBF, iPos, 1, model.network.physicalLev[i].point[i1].latitude().degrees());
				DBFWriteDoubleAttribute(hDBF, iPos, 2, model.network.physicalLev[i].point[i1].longitude().degrees());
				iPos++;
			}
		}
	}
	DBFClose(hDBF);

	write_copyAtoB(pszFilename, (char*)"prj", (char*)"wgs84Def.prj", (char*)"w");
	*/

	return 0;
}

int writeAllArcsToGeojson(char* pszFilename)
{
	int forsta = 1, nod2, nextLevel;
	double distance;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/allArcs.geojson", model.params.resultPath.c_str());
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "allPhysicalArcs");

	for (int i = 0; i < model.network.nPhysicalLevels; i++) {
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;
			if (i == 3 && i1 > 56)
				i = i;
			for (int i2 = 0; i2 < model.network.physicalLev[i].nOutNodes[i1]; i2++) {
				nod2 = model.network.physicalLev[i].outNode[i1][i2];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2];
				if (nextLevel < 0)
					continue; // ej implementerat channels yet, do that!!!!

				if (forsta != 1)
					fprintf(filpekG, ", ");
				else
					forsta = 0;
				distance = model.network.physicalLev[i].point[i1].distanceTo(model.network.physicalLev[nextLevel].point[nod2]);
				fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level1\":%d, \"nodPos1\":%d, \"arcPos\":%d, \"level2\":%d, \"nodPos2\":%d, \"distance\":%.3lf},\n",
					i, i1, i2, nextLevel, nod2, distance);
				fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[ [%.4lf,%.4lf], [%.4lf,%.4lf] ]]}}\n",
					model.network.physicalLev[i].point[i1].longitude().degrees(),
					model.network.physicalLev[i].point[i1].latitude().degrees(),
					model.network.physicalLev[nextLevel].point[nod2].longitude().degrees(),
					model.network.physicalLev[nextLevel].point[nod2].latitude().degrees());
			}
		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	errlog("ERROR! Add channels to the plotted arcs\n");


	/*
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	int nSHPType, nAllocPkter, i, iPos, nPkter, cNr;
	int startPos, slutPos, i1, i2, i2b, ib, firstPoint, lastPoint;
	double* x, * y, * z, distance;
	//const char *pszFilename = "pkterShape";

	nSHPType = SHPT_ARCZ;

	hSHPHandle = SHPCreate(pszFilename, nSHPType);

	x = (double*)malloc(model.network.nMaxNodesInPath * sizeof(double));
	y = (double*)malloc(model.network.nMaxNodesInPath * sizeof(double));
	z = (double*)malloc(model.network.nMaxNodesInPath * sizeof(double));

	iPos = 0;
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			y[0] = model.network.physicalLev[i].point[i1].latitude().degrees();
			x[0] = model.network.physicalLev[i].point[i1].longitude().degrees();
			z[0] = 0;
			for (int i2 = 0; i2 < model.network.physicalLev[i].nOutNodes[i1]; i2++) {
				i2b = model.network.physicalLev[i].outNode[i1][i2];
				ib = model.network.physicalLev[i].outLevel[i1][i2];
				if (ib >= 0) {
					if (i1 == model.params.preferredPathOrtoPos[i] && i2b == model.params.preferredPathOrtoPos[ib] && i + 1 == ib) {
						nPkter = 1;
						for (int i3 = 0; i3 < model.network.physicalLev[i].npreferredPathPoints; i3++) {
							y[nPkter] = model.network.physicalLev[i].preferredPathPoint[i3].latitude().degrees();
							x[nPkter] = model.network.physicalLev[i].preferredPathPoint[i3].longitude().degrees();
							z[nPkter] = 0;
							nPkter++;
						}
					}
					else {
						y[1] = model.network.physicalLev[ib].point[i2b].latitude().degrees();
						x[1] = model.network.physicalLev[ib].point[i2b].longitude().degrees();
						z[1] = 0;
						nPkter = 2;
					}
				}
				else {
					y[1] = model.network.channel[-ib - 1].point[i2b].latitude().degrees();
					x[1] = model.network.channel[-ib - 1].point[i2b].longitude().degrees();
					z[1] = 0;
					nPkter = 2;
				}
				psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
					nPkter, x, y, z, NULL); //  m);
				SHPWriteObject(hSHPHandle, -1, psShape);
				SHPDestroyObject(psShape);
				iPos++;
			}
		}
	}
	for (i = 0; i < model.network.nUsedChannels; i++) {
		cNr = model.network.usedChannel[i];
		nPkter = 0;
		for (i1 = 0; i1 < model.network.channel[cNr].nPoints; i1++) {
			//			if (model.network.channel[cNr].usedPoint[i1] = 1) {
			y[nPkter] = model.network.channel[cNr].point[i1].latitude().degrees();
			x[nPkter] = model.network.channel[cNr].point[i1].longitude().degrees();
			z[nPkter] = 0;
			nPkter++;
			//			}
		}
		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			nPkter, x, y, z, NULL); //  m);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);
		iPos++;
		y[0] = y[nPkter - 1];
		x[0] = x[nPkter - 1];
		z[0] = 0;
		for (int i2 = 0; i2 < model.network.channel[cNr].nOutNodes[0]; i2++) {
			i2b = model.network.channel[cNr].outNode[0][i2];
			ib = model.network.channel[cNr].outLevel[0][i2];
			y[1] = model.network.physicalLev[ib].point[i2b].latitude().degrees();
			x[1] = model.network.physicalLev[ib].point[i2b].longitude().degrees();
			z[1] = 0;
			psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
				2, x, y, z, NULL); //  m);
			SHPWriteObject(hSHPHandle, -1, psShape);
			SHPDestroyObject(psShape);
			iPos++;
		}
	}

	SHPClose(hSHPHandle);

	DBFHandle	hDBF;
	hDBF = DBFCreate(pszFilename);
	if (hDBF == NULL)
	{
		printf("DBFCreate(%s) failed.\n", pszFilename);
		exit(2);
	}
	if (DBFAddField(hDBF, "lat1", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "lon1", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "lat2", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "lon2", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "distance", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "startLevel", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "endLevel", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);

	iPos = 0;
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			for (int i2 = 0; i2 < model.network.physicalLev[i].nOutNodes[i1]; i2++) {
				i2b = model.network.physicalLev[i].outNode[i1][i2];
				DBFWriteDoubleAttribute(hDBF, iPos, 0, model.network.physicalLev[i].point[i1].latitude().degrees());
				DBFWriteDoubleAttribute(hDBF, iPos, 1, model.network.physicalLev[i].point[i1].longitude().degrees());
				i2b = model.network.physicalLev[i].outNode[i1][i2];
				ib = model.network.physicalLev[i].outLevel[i1][i2];
				if (ib >= 0) {
					y[1] = model.network.physicalLev[ib].point[i2b].latitude().degrees();
					x[1] = model.network.physicalLev[ib].point[i2b].longitude().degrees();
					distance = model.network.physicalLev[i].point[i1].distanceTo(model.network.physicalLev[ib].point[i2b]) / 1000.0;
				}
				else {
					y[1] = model.network.channel[-ib - 1].point[i2b].latitude().degrees();
					x[1] = model.network.channel[-ib - 1].point[i2b].longitude().degrees();
					distance = model.network.physicalLev[i].point[i1].distanceTo(model.network.channel[-ib - 1].point[i2b]) / 1000.0;
				}
				DBFWriteDoubleAttribute(hDBF, iPos, 2, y[1]);
				DBFWriteDoubleAttribute(hDBF, iPos, 3, x[1]);
				DBFWriteDoubleAttribute(hDBF, iPos, 4, distance);
				DBFWriteIntegerAttribute(hDBF, iPos, 5, i);
				DBFWriteIntegerAttribute(hDBF, iPos, 6, ib);
				iPos++;
			}
		}
	}
	for (i = 0; i < model.network.nUsedChannels; i++) {
		cNr = model.network.usedChannel[i];
		nPkter = 0;
		firstPoint = 0;
		lastPoint = model.network.channel[cNr].nPoints - 1;
		//		for (i1 = 0; i1 < model.network.channel[cNr].nPoints; i1++) {
		//			if (model.network.channel[cNr].usedPoint[i1] = 1) {
		//				if (firstPoint == -1)
		//					firstPoint = i1;
		//				lastPoint = i1;
		//			}
		//		}
		DBFWriteDoubleAttribute(hDBF, iPos, 0, model.network.channel[cNr].point[firstPoint].latitude().degrees());
		DBFWriteDoubleAttribute(hDBF, iPos, 1, model.network.channel[cNr].point[firstPoint].longitude().degrees());
		DBFWriteDoubleAttribute(hDBF, iPos, 2, model.network.channel[cNr].point[lastPoint].latitude().degrees());
		DBFWriteDoubleAttribute(hDBF, iPos, 3, model.network.channel[cNr].point[lastPoint].longitude().degrees());
		DBFWriteDoubleAttribute(hDBF, iPos, 4, model.network.channel[cNr].distanceFromStart[lastPoint] -
			model.network.channel[cNr].distanceFromStart[firstPoint]);
		DBFWriteIntegerAttribute(hDBF, iPos, 5, -i - 1);
		DBFWriteIntegerAttribute(hDBF, iPos, 6, -i - 1);
		iPos++;

		for (int i2 = 0; i2 < model.network.channel[cNr].nOutNodes[0]; i2++) {
			DBFWriteDoubleAttribute(hDBF, iPos, 0, model.network.channel[cNr].point[lastPoint].latitude().degrees());
			DBFWriteDoubleAttribute(hDBF, iPos, 1, model.network.channel[cNr].point[lastPoint].longitude().degrees());
			i2b = model.network.channel[cNr].outNode[0][i2];
			ib = model.network.channel[cNr].outLevel[0][i2];
			DBFWriteDoubleAttribute(hDBF, iPos, 2, model.network.physicalLev[ib].point[i2b].latitude().degrees());
			DBFWriteDoubleAttribute(hDBF, iPos, 3, model.network.physicalLev[ib].point[i2b].longitude().degrees());
			distance = model.network.channel[cNr].point[lastPoint].distanceTo(model.network.physicalLev[ib].point[i2b]) / 1000.0;
			DBFWriteDoubleAttribute(hDBF, iPos, 4, distance);
			DBFWriteIntegerAttribute(hDBF, iPos, 5, -i - 1);
			DBFWriteIntegerAttribute(hDBF, iPos, 6, ib);
			iPos++;
		}

	}

	DBFClose(hDBF);

	write_copyAtoB(pszFilename, (char*)"prj", (char*)"wgs84Def.prj", (char*)"w");

	free(x);
	free(y);
	free(z);
	*/

	return 0;
}


int writeKorridorToGeojson(char* pszFilename)
{
	errlog("ERROR! Implement writeKorridorToGeojson\n");
	/*
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	int nSHPType, nAllocPkter, i, iPos, nPkter, cNr, pos;
	int startPos, slutPos, i1, i2, i2b, ib, firstPoint, lastPoint;
	double* x, * y, * z, distance;
	//const char *pszFilename = "pkterShape";

	nSHPType = SHPT_ARCZ;

	hSHPHandle = SHPCreate(pszFilename, nSHPType);

	x = (double*)malloc(model.network.nPhysicalLevels * sizeof(double));
	y = (double*)malloc(model.network.nPhysicalLevels * sizeof(double));
	z = (double*)malloc(model.network.nPhysicalLevels * sizeof(double));

	iPos = 0;
	for (pos = 0; pos < 2; pos++) {
		for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
			if (pos == 0)
				i1 = 0;
			else
				i1 = model.network.physicalLev[i].nPoints - 1;
			y[i] = model.network.physicalLev[i].point[i1].latitude().degrees();
			x[i] = model.network.physicalLev[i].point[i1].longitude().degrees();
			z[i] = 0;
		}
		y[i] = model.preferredPath.point[model.preferredPath.nPoints - 1].latitude().degrees();
		x[i] = model.preferredPath.point[model.preferredPath.nPoints - 1].longitude().degrees();
		z[i] = 0;

		psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
			i + 1, x, y, z, NULL);
		SHPWriteObject(hSHPHandle, -1, psShape);
		SHPDestroyObject(psShape);
		iPos++;
	}

	SHPClose(hSHPHandle);

	DBFHandle	hDBF;
	hDBF = DBFCreate(pszFilename);
	if (hDBF == NULL)
	{
		printf("DBFCreate(%s) failed.\n", pszFilename);
		exit(2);
	}
	if (DBFAddField(hDBF, "from", FTString, 40, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "to", FTString, 40, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "type", FTString, 40, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "corridor", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);

	iPos = 0;
	for (pos = 0; pos < 2; pos++) {

		DBFWriteStringAttribute(hDBF, iPos, 0, model.params.fromHarbour.c_str());
		DBFWriteStringAttribute(hDBF, iPos, 1, model.params.toHarbour.c_str());
		DBFWriteStringAttribute(hDBF, iPos, 2, model.params.type.c_str());
		DBFWriteIntegerAttribute(hDBF, iPos, 3, pos + 1);
		iPos++;
	}
	DBFClose(hDBF);

	write_copyAtoB(pszFilename, (char*)"prj", (char*)"wgs84Def.prj", (char*)"w");

	free(x);
	free(y);
	free(z);

	FILE* filPek3 = fopen("result_json.json", "w");
	fprintf(filPek3, "{\n");
	fprintf(filPek3, "\t\"solutionShape\": \"%s\"}\n", pszFilename);
	fclose(filPek3);
	*/

	return 0;
}


int readGivenSolutionPath()
{
	FILE* filPek;
	int antal, i, level, nodPos, i1, speedSetting;
	int prevLevel, prevNodPos, prevSpeedSetting, tNu;
	char objects[10][CHAR_ALLOC];
	dataStr* data;

	// float** fuelRaster = model.fuelGeographyMapRaster[0].GetRasterBand(1);

	data = (dataStr*)malloc(sizeof(dataStr));

	filPek = fopen(model.params.readSolPathFile.c_str(), "r");
	if (filPek == NULL) {
		errlog("ERROR! Didn't manage to open %s. I quit\n", model.params.readSolPathFile.c_str());
		exit(0);
	}

	antal = get_data_objects_till_EOL_semkol(objects, data, filPek);
	model.nBVArcs = 0;
	tNu = 0;
	for (i = 0; i < model.network.nPhysicalLevels + 2; i++) {

		antal = get_data_objects_till_EOL_semkol(objects, data, filPek);
		if (antal != 4)
			break;
		level = char_to_int(objects[0]);
		nodPos = char_to_int(objects[1]);
		for (i1 = 0; i1 < model.params.nShip_speedSettings; i1++) {
			if (strcmp(objects[2], model.params.ship_speedSettingID[i1]) == 0)
				break;
		}
		if (i1 >= model.params.nShip_speedSettings) {
			errlog("ERROR! Speedseeting %s given in %s is not defined. I use speedsetting %s.\n", objects[2],
				model.params.readSolPathFile.c_str(), model.params.ship_speedSettingID[0]);
			speedSetting = 0;
		}
		else
			speedSetting = i1;
		if (level >= model.network.nPhysicalLevels - 1) {
			errlog("ERROR! Physical level %d given in %s but max level is %d. I quit.\n",
				level, model.params.readSolPathFile.c_str(), model.network.nPhysicalLevels - 2);
			exit(0);
		}
		if (level >= 0) {
			if (nodPos < 0 || nodPos >= model.network.physicalLev[level].nPoints) {
				errlog("ERROR! NodPos %d is given for physical level %d given in %s but nodPos must be 0 - %d. I quit.\n",
					nodPos, level, model.params.readSolPathFile.c_str(), model.network.physicalLev[level].nPoints - 1);
				exit(0);
			}
		}
		else {
			if (nodPos < 0 || nodPos >= 2) {
				errlog("ERROR! NodPos %d is given for channel %d given in %s but nodPos must be 0 - 1. I quit.\n",
					nodPos, -level - 1, model.params.readSolPathFile.c_str());
				exit(0);
			}
		}
		if (i > 0) {
			for (i1 = 0; i1 < model.nArcs; i1++) {
				if (model.arc[i1].fromLevel == prevLevel && model.arc[i1].toLevel == level &&
					model.arc[i1].fromPointNr == prevNodPos && model.arc[i1].toPointNr == nodPos &&
					model.arc[i1].speedSetting == prevSpeedSetting && model.arc[i1].fromTime == tNu) {
					model.BVArc[model.nBVArcs] = i1;
					(model.nBVArcs)++;
					tNu = model.arc[i1].toTime;
					break;
				}
			}
			if (i1 >= model.nArcs) {
				i1 = try_addBage_fromPath(prevLevel, level, prevNodPos, nodPos, prevSpeedSetting, tNu);//, model.rasterData.fuelGeography);
				if (i1 >= 0) {
					model.BVArc[model.nBVArcs] = i1;
					(model.nBVArcs)++;
					tNu = model.arc[i1].toTime;
				}
				else {
					errlog("ERROR! Could not find arc between level %d and %d given in %s. I quit\n",
						prevLevel, level, model.params.readSolPathFile.c_str());
					exit(0);
				}
			}
		}

		prevLevel = level;
		prevNodPos = nodPos;
		prevSpeedSetting = speedSetting;

	}
	if (i > 0) {
		level = model.network.nPhysicalLevels - 1;
		nodPos = 0;
		for (i1 = 0; i1 < model.nArcs; i1++) {
			if (model.arc[i1].fromLevel == prevLevel && model.arc[i1].toLevel == level &&
				model.arc[i1].fromPointNr == prevNodPos && model.arc[i1].toPointNr == nodPos &&
				model.arc[i1].speedSetting == prevSpeedSetting && model.arc[i1].fromTime == tNu) {
				model.BVArc[model.nBVArcs] = i1;
				(model.nBVArcs)++;
				tNu = model.arc[i1].toTime;
				break;
			}
		}
		if (i1 >= model.nArcs) {
			i1 = try_addBage_fromPath(prevLevel, level, prevNodPos, nodPos, prevSpeedSetting, tNu); // , model.rasterData.fuelGeography);
			if (i1 >= 0) {
				model.BVArc[model.nBVArcs] = i1;
				(model.nBVArcs)++;
				tNu = model.arc[i1].toTime;
			}
			else {
				errlog("ERROR! Could not find arc between level %d and %d given in %s. I quit\n",
					prevLevel, level, model.params.readSolPathFile.c_str());
				exit(0);
			}
		}
	}

	// add one more arc in the end as it's supposed to go to a super sink. The arcNr doesn't matter
	model.BVArc[model.nBVArcs] = 0;
	(model.nBVArcs)++;

	return 0;
}

int initGeoJsonFil(FILE* filpek, const char* namn) {

	fprintf(filpek, "{\n");
	fprintf(filpek, "\"type\": \"FeatureCollection\",\n");
	// fprintf(filpek, "\"name\": \"%s\",\n", namn);
	fprintf(filpek, "\"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } },\n");
	fprintf(filpek, "\"features\": [\n");

	return 0;
}

int writeSolutionPathToGeoJson(char* filename, int resAlt)
{
	int i, iPos, nPkter, nArcs, ii3;
	int arcNr, lev1, lev2, pointNr1, pointNr2, * nSpeedSettingUsed, nSpeedChanges = 0;
	double* x, * y, * z;
	struct tm tmBas;
	FILE* filpekG;
	time_t rawtime;

	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = 0;
	tmBas.tm_sec = 0;
	//timeNu = mktime(&tmBas);

	int nAlloc = model.network.nMaxNodesInPath, prefPath;

	x = (double*)malloc(nAlloc * sizeof(double));
	y = (double*)malloc(nAlloc * sizeof(double));
	z = (double*)malloc(nAlloc * sizeof(double));
	nSpeedSettingUsed = (int*)calloc(model.params.nShip_speedSettings, sizeof(int));

	FILE* filPek, * filPek2, * filPek3;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s.csv", filename);
	filPek = fopen(namn, "w");
	sprintf(namn, "%s/solPath.txt", model.params.resultPath.c_str());
	filPek2 = fopen(namn, "w");
	sprintf(namn, "%s/result_json.json", model.params.resultPath.c_str());
	if (resAlt == 0) {
		filPek3 = fopen(namn, "w");
		fprintf(filPek3, "{\n");
		fprintf(filPek3, "\t\"solutionShape\": \"/%s\",\n", filename);
	}
	else {
		filPek3 = fopen(namn, "a+");
	}
	fprintf(filPek2, "level;nodPos;speedSetting;arcNr(for_information_only);nod1(info);nod2(info)\n");

	sprintf(filename, "%s.geojson", filename);
	filpekG = fopen(filename, "w");

	if (filpekG == NULL)
	{
		printf("Faile to open file %s for writing.\n", filename);
		exit(2);
	}

	initGeoJsonFil(filpekG, "testRutt");

	fprintf(filPek, "arcPos\tspeedSetting\tdistance\ttime\tfuelBase\tsafetyBase\tchannelCost\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
		"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\tnodNr1\tnodNr2\n");

	double time = 0, fuel = 0, safety = 0, totCost = 0, distance = 0, channelCost = 0;
	double fuelLSMGO = 0, fuelVLSFO = 0, hurricane = 0, distanceTp, distNu, distTmp;// , stability = 0;
	double bowSlamming = 0, greenWater = 0, dynStability = 0, iceCoverage = 0, feasibleSafety = 0;
	int ii, nTp, nAdded, ii2;
	spherical::Point pointLast, pointFinal;

	nArcs = 0;
	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
	{
		// kopiera delen av punktfoljden som anvands, dess xyz
		arcNr = model.BVArc[iPos];
		if (arcNr == 559126)
			arcNr = arcNr;
		lev1 = model.arc[arcNr].fromLevel;
		lev2 = model.arc[arcNr].toLevel;
		pointNr1 = model.arc[arcNr].fromPointNr;
		pointNr2 = model.arc[arcNr].toPointNr;
		if (iPos > 0) {
			if (model.arc[arcNr].speedSetting != model.arc[model.BVArc[iPos - 1]].speedSetting)
				nSpeedChanges++;
		}
		(nSpeedSettingUsed[model.arc[arcNr].speedSetting])++;

		nTp = model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
		if (nTp == 0)
			nTp = 1;
		distanceTp = 1000.0 * model.arc[arcNr].distance / nTp;
		nAdded = 0;

		if (lev1 < 0 && lev2 < 0) {
			nPkter = 0;
			distNu = 0;
			for (i = 0; i < model.network.channel[-lev1 - 1].nPoints; i++) {
				if (i > 0) {
					distTmp = model.network.channel[-lev1 - 1].point[i - 1].distanceTo(model.network.channel[-lev1 - 1].point[i]);
					for (ii = 0; ii < nTp; ii++) {
						if (distNu + distTmp >= distanceTp || (i == model.network.channel[-lev1 - 1].nPoints - 1 && distNu + distTmp >= distanceTp * 0.95)) {
							// identifiera pkten dar distTmp + distNu = distanceTp
							pointLast = pointLast.destinationPoint(distanceTp - distNu, pointLast.bearingTo(model.network.channel[-lev1 - 1].point[i]));
							y[nPkter] = pointLast.latitude().degrees();
							x[nPkter] = pointLast.longitude().degrees();
							z[nPkter] = 0;
							nPkter++;

							// spara pkten dar distTmp + distNu = distanceTp
							if (nArcs > 0)
								fprintf(filpekG, ",\n");
							distance += model.arc[arcNr].distance / nTp;
							time += model.arc[arcNr].time / nTp;
							fuel += model.arc[arcNr].fuelBase / nTp;
							fuelLSMGO += model.arc[arcNr].fuelLSMGO / nTp;
							fuelVLSFO += model.arc[arcNr].fuelVLSFO / nTp;
							safety += model.arc[arcNr].safetyBase / nTp;
							hurricane += model.arc[arcNr].safetyHurricane / nTp;
							bowSlamming += model.arc[arcNr].safetyBowSlam / nTp;
							greenWater += model.arc[arcNr].safetyGreenWater / nTp;
							dynStability += model.arc[arcNr].safetyDynStability / nTp;
							iceCoverage += model.arc[arcNr].iceCoverCost / nTp;
							feasibleSafety += (double)(model.arc[arcNr].feasibleSafety) / nTp;
							//stability += model.arc[arcNr].safetyStability / nTp;
//							channelCost += model.arc[arcNr].channelCost / nTp;
							totCost += model.arc[arcNr].totCost / nTp;
							fprintf(filpekG, "{ \"type\": \"Feature\", \"properties\": {\n");
							fprintf(filpekG, "\"arcPos\": %d, \"distance\": %.3lf, \"time\": %.3lf,\n", nArcs++, model.arc[arcNr].distance / nTp,
								model.arc[arcNr].time / nTp);
							fprintf(filpekG, "\"fuelBase\": %.2lf, \"safetyBase\": %.3lf, \"weightCost\": %.3lf,\n", model.arc[arcNr].fuelBase / nTp,
								model.arc[arcNr].safetyBase / nTp, model.arc[arcNr].totCost / nTp);
							fprintf(filpekG, "\"fromLevel\": %d, \"fromPointNr\": %d, \"fromTimeInterval\": %d,\n", model.arc[arcNr].fromLevel,
								model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime + ii);
							fprintf(filpekG, "\"toLevel\": %d, \"toPointNr\": %d, \"toTimeInterval\": %d,\n", model.arc[arcNr].toLevel,
								model.arc[arcNr].toPointNr, model.arc[arcNr].fromTime + ii + 1);
							fprintf(filpekG, "\"rpm_setting\": %.2lf, \"nodNr1\": %d, \"nodNr2\": %d,\n",
								model.functions.rpm[model.arc[arcNr].speedSetting],
								model.arc[arcNr].nodNr1, model.arc[arcNr].nodNr2);

							tmBas.tm_hour += model.arc[arcNr].fromTime + ii;
							mktime(&tmBas);
							sprintf(namn, "%d", tmBas.tm_year + 1900);
							if (tmBas.tm_mon + 1 < 10)
								sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
							else
								sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
							if (tmBas.tm_mday < 10)
								sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
							else
								sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
							if (tmBas.tm_hour < 10)
								sprintf(namn, "%sT0%d", namn, tmBas.tm_hour);
							else
								sprintf(namn, "%sT%d", namn, tmBas.tm_hour);
							if (tmBas.tm_min < 10)
								sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
							else
								sprintf(namn, "%s:%d", namn, tmBas.tm_min);
							if (tmBas.tm_sec < 10)
								sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
							else
								sprintf(namn, "%s:%d", namn, tmBas.tm_sec);
							fprintf(filpekG, "\"channelCost\": %.2lf, \"startTime\": \"%s\", ", model.arc[arcNr].channelCost / nTp, namn);

							tmBas.tm_hour += 1; // model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
							mktime(&tmBas);
							sprintf(namn, "%d", tmBas.tm_year + 1900);
							if (tmBas.tm_mon + 1 < 10)
								sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
							else
								sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
							if (tmBas.tm_mday < 10)
								sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
							else
								sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
							if (tmBas.tm_hour < 10)
								sprintf(namn, "%s 0%d", namn, tmBas.tm_hour);
							else
								sprintf(namn, "%s %d", namn, tmBas.tm_hour);
							if (tmBas.tm_min < 10)
								sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
							else
								sprintf(namn, "%s:%d", namn, tmBas.tm_min);
							if (tmBas.tm_sec < 10)
								sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
							else
								sprintf(namn, "%s:%d", namn, tmBas.tm_sec);

							fprintf(filpekG, "\"endTime\": \"%s\",\n", namn);
							tmBas.tm_hour -= model.arc[arcNr].fromTime + ii + 1; // .toTime;

							fprintf(filpekG, "\"arcNr\": %d }, \"geometry\": { \"type\": \"MultiLineString\",\n", arcNr);
							fprintf(filpekG, "\"coordinates\": [ [ ");
							for (ii2 = 0; ii2 < nPkter; ii2++) {
								fprintf(filpekG, "[ %lf, %lf, 0.0 ]", x[ii2], y[ii2]);
								if (ii2 < nPkter - 1)
									fprintf(filpekG, ",\n");
								else
									fprintf(filpekG, " ] ] } }");
							}

							nAdded++;
							if (nAdded >= nTp)
								break;
							distTmp -= distanceTp - distNu;
							distNu = 0;
							nPkter = 0;
							y[nPkter] = pointLast.latitude().degrees();
							x[nPkter] = pointLast.longitude().degrees();
							z[nPkter] = 0;
							nPkter++;
						}
						else {
							distNu += distTmp;
							break;
						}
					}
				}
				y[nPkter] = model.network.channel[-lev1 - 1].point[i].latitude().degrees();
				x[nPkter] = model.network.channel[-lev1 - 1].point[i].longitude().degrees();
				pointLast = spherical::Point(y[nPkter], x[nPkter]);
				z[nPkter] = 0;
				nPkter++;
			}
		}
		else {
			prefPath = 0;
			if (lev1 >= 0 && lev2 >= 0) {
				if (pointNr1 == model.params.preferredPathOrtoPos[lev1] &&
					pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1) {
					y[0] = model.network.physicalLev[lev1].point[pointNr1].latitude().degrees();
					x[0] = model.network.physicalLev[lev1].point[pointNr1].longitude().degrees();
					pointLast = spherical::Point(y[0], x[0]);
					nPkter = 1;
					distNu = 0;
					ii3 = 0;
					for (int i3 = 0; i3 < model.network.physicalLev[lev1].npreferredPathPoints; i3++) {
						distTmp = pointLast.distanceTo(model.network.physicalLev[lev1].preferredPathPoint[i3]);
						for (ii = 0; ii < nTp; ii++) {
							if (distNu + distTmp >= distanceTp || (i3 == model.network.physicalLev[lev1].npreferredPathPoints - 1 && distNu + distTmp >= distanceTp * 0.95)) {
								// identifiera pkten dar distTmp + distNu = distanceTp
								pointLast = pointLast.destinationPoint(distanceTp - distNu, pointLast.bearingTo(model.network.physicalLev[lev1].preferredPathPoint[i3]));
								y[nPkter] = pointLast.latitude().degrees();
								x[nPkter] = pointLast.longitude().degrees();
								z[nPkter] = 0;
								nPkter++;

								// spara pkten dar distTmp + distNu = distanceTp
								if (nArcs > 0)
									fprintf(filpekG, ",\n");
								distance += model.arc[arcNr].distance / nTp;
								time += model.arc[arcNr].time / nTp;
								fuel += model.arc[arcNr].fuelBase / nTp;
								fuelLSMGO += model.arc[arcNr].fuelLSMGO / nTp;
								fuelVLSFO += model.arc[arcNr].fuelVLSFO / nTp;
								safety += model.arc[arcNr].safetyBase / nTp;
								hurricane += model.arc[arcNr].safetyHurricane / nTp;
								bowSlamming += model.arc[arcNr].safetyBowSlam / nTp;
								greenWater += model.arc[arcNr].safetyGreenWater / nTp;
								dynStability += model.arc[arcNr].safetyDynStability / nTp;
								iceCoverage += model.arc[arcNr].iceCoverCost / nTp;
								feasibleSafety += (double)(model.arc[arcNr].feasibleSafety) / nTp;
								//stability += model.arc[arcNr].safetyStability / nTp;
								channelCost += model.arc[arcNr].channelCost / nTp;
								totCost += model.arc[arcNr].totCost / nTp;
								fprintf(filpekG, "{ \"type\": \"Feature\", \"properties\": {\n");
								fprintf(filpekG, "\"arcPos\": %d, \"distance\": %.3lf, \"time\": %.3lf,\n", nArcs++, model.arc[arcNr].distance / nTp,
									model.arc[arcNr].time / nTp);
								fprintf(filpekG, "\"fuelBase\": %.2lf, \"safetyBase\": %.3lf, \"weightCost\": %.3lf,\n", model.arc[arcNr].fuelBase / nTp,
									model.arc[arcNr].safetyBase / nTp, model.arc[arcNr].totCost / nTp);
								fprintf(filpekG, "\"fromLevel\": %d, \"fromPointNr\": %d, \"fromTimeInterval\": %d,\n", model.arc[arcNr].fromLevel,
									model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime + ii);
								fprintf(filpekG, "\"toLevel\": %d, \"toPointNr\": %d, \"toTimeInterval\": %d,\n", model.arc[arcNr].toLevel,
									model.arc[arcNr].toPointNr, model.arc[arcNr].fromTime + ii + 1);
								fprintf(filpekG, "\"rpm_setting\": %.2lf, \"nodNr1\": %d, \"nodNr2\": %d,\n",
									model.functions.rpm[model.arc[arcNr].speedSetting],
									model.arc[arcNr].nodNr1, model.arc[arcNr].nodNr2);

								tmBas.tm_hour += model.arc[arcNr].fromTime + ii3;
								mktime(&tmBas);
								sprintf(namn, "%d", tmBas.tm_year + 1900);
								if (tmBas.tm_mon + 1 < 10)
									sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
								else
									sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
								if (tmBas.tm_mday < 10)
									sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
								else
									sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
								if (tmBas.tm_hour < 10)
									sprintf(namn, "%sT0%d", namn, tmBas.tm_hour);
								else
									sprintf(namn, "%sT%d", namn, tmBas.tm_hour);
								if (tmBas.tm_min < 10)
									sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
								else
									sprintf(namn, "%s:%d", namn, tmBas.tm_min);
								if (tmBas.tm_sec < 10)
									sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
								else
									sprintf(namn, "%s:%d", namn, tmBas.tm_sec);
								fprintf(filpekG, "\"channelCost\": %.2lf, \"startTime\": \"%s\", ", model.arc[arcNr].channelCost / nTp, namn);

								tmBas.tm_hour += 1; // model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
								mktime(&tmBas);
								sprintf(namn, "%d", tmBas.tm_year + 1900);
								if (tmBas.tm_mon + 1 < 10)
									sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
								else
									sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
								if (tmBas.tm_mday < 10)
									sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
								else
									sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
								if (tmBas.tm_hour < 10)
									sprintf(namn, "%s 0%d", namn, tmBas.tm_hour);
								else
									sprintf(namn, "%s %d", namn, tmBas.tm_hour);
								if (tmBas.tm_min < 10)
									sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
								else
									sprintf(namn, "%s:%d", namn, tmBas.tm_min);
								if (tmBas.tm_sec < 10)
									sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
								else
									sprintf(namn, "%s:%d", namn, tmBas.tm_sec);

								fprintf(filpekG, "\"endTime\": \"%s\",\n", namn);
								tmBas.tm_hour -= model.arc[arcNr].fromTime + ii3 + 1; // .toTime;

								fprintf(filpekG, "\"arcNr\": %d }, \"geometry\": { \"type\": \"MultiLineString\",\n", arcNr);
								fprintf(filpekG, "\"coordinates\": [ [ ");
								for (ii2 = 0; ii2 < nPkter; ii2++) {
									fprintf(filpekG, "[ %lf, %lf, 0.0 ]", x[ii2], y[ii2]);
									if (ii2 < nPkter - 1)
										fprintf(filpekG, ",\n");
									else
										fprintf(filpekG, " ] ] } }");
								}
								nAdded++;
								if (nAdded >= nTp)
									break;
								distTmp -= distanceTp - distNu;
								ii3++;
								distNu = 0;
								nPkter = 0;
								y[nPkter] = pointLast.latitude().degrees();
								x[nPkter] = pointLast.longitude().degrees();
								z[nPkter] = 0;
								nPkter++;
							}
							else {
								distNu += distTmp;
								break;
							}
						}
						y[nPkter] = model.network.physicalLev[lev1].preferredPathPoint[i3].latitude().degrees();
						x[nPkter] = model.network.physicalLev[lev1].preferredPathPoint[i3].longitude().degrees();
						pointLast = spherical::Point(y[nPkter], x[nPkter]);
						z[nPkter] = 0;
						nPkter++;
					}
					prefPath = 1;
				}
			}
			if (prefPath == 0) {
				if (lev1 >= 0) {
					y[0] = model.network.physicalLev[lev1].point[pointNr1].latitude().degrees();
					x[0] = model.network.physicalLev[lev1].point[pointNr1].longitude().degrees();
				}
				else {
					y[0] = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1].latitude().degrees();
					x[0] = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1].longitude().degrees();
				}
				z[0] = 0;
				if (lev2 >= 0) {
					y[1] = model.network.physicalLev[lev2].point[pointNr2].latitude().degrees();
					x[1] = model.network.physicalLev[lev2].point[pointNr2].longitude().degrees();
				}
				else {
					y[1] = model.network.channel[-lev2 - 1].point[0].latitude().degrees();
					x[1] = model.network.channel[-lev2 - 1].point[0].longitude().degrees();
				}
				pointLast = spherical::Point(y[0], x[0]);
				pointFinal = spherical::Point(y[1], x[1]);
				z[1] = 0;
				nPkter = 2;
				for (ii = 0; ii < nTp; ii++) {
					y[0] = pointLast.latitude().degrees();
					x[0] = pointLast.longitude().degrees();
					pointLast = pointLast.destinationPoint(distanceTp, pointLast.bearingTo(pointFinal));
					y[1] = pointLast.latitude().degrees();
					x[1] = pointLast.longitude().degrees();

					if (nArcs > 0)
						fprintf(filpekG, ",\n");
					distance += model.arc[arcNr].distance / nTp;
					time += model.arc[arcNr].time / nTp;
					fuel += model.arc[arcNr].fuelBase / nTp;
					fuelLSMGO += model.arc[arcNr].fuelLSMGO / nTp;
					fuelVLSFO += model.arc[arcNr].fuelVLSFO / nTp;
					safety += model.arc[arcNr].safetyBase / nTp;
					hurricane += model.arc[arcNr].safetyHurricane / nTp;
					bowSlamming += model.arc[arcNr].safetyBowSlam / nTp;
					greenWater += model.arc[arcNr].safetyGreenWater / nTp;
					dynStability += model.arc[arcNr].safetyDynStability / nTp;
					iceCoverage += model.arc[arcNr].iceCoverCost / nTp;
					feasibleSafety += (double)(model.arc[arcNr].feasibleSafety) / nTp;
					//stability += model.arc[arcNr].safetyStability / nTp;
					channelCost += model.arc[arcNr].channelCost / nTp;
					totCost += model.arc[arcNr].totCost / nTp;
					fprintf(filpekG, "{ \"type\": \"Feature\", \"properties\": {\n");
					fprintf(filpekG, "\"arcPos\": %d, \"distance\": %.3lf, \"time\": %.3lf,\n", nArcs++, model.arc[arcNr].distance / nTp,
						model.arc[arcNr].time / nTp);
					fprintf(filpekG, "\"fuelBase\": %.2lf, \"safetyBase\": %.3lf, \"weightCost\": %.3lf,\n", model.arc[arcNr].fuelBase / nTp,
						model.arc[arcNr].safetyBase / nTp, model.arc[arcNr].totCost / nTp);
					fprintf(filpekG, "\"fromLevel\": %d, \"fromPointNr\": %d, \"fromTimeInterval\": %d,\n", model.arc[arcNr].fromLevel,
						model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime + ii);
					fprintf(filpekG, "\"toLevel\": %d, \"toPointNr\": %d, \"toTimeInterval\": %d,\n", model.arc[arcNr].toLevel,
						model.arc[arcNr].toPointNr, model.arc[arcNr].fromTime + ii + 1);
					fprintf(filpekG, "\"rpm_setting\": %.2lf, \"nodNr1\": %d, \"nodNr2\": %d,\n",
						model.functions.rpm[model.arc[arcNr].speedSetting],
						model.arc[arcNr].nodNr1, model.arc[arcNr].nodNr2);

					tmBas.tm_hour += model.arc[arcNr].fromTime + ii;
					mktime(&tmBas);
					sprintf(namn, "%d", tmBas.tm_year + 1900);
					if (tmBas.tm_mon + 1 < 10)
						sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
					else
						sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
					if (tmBas.tm_mday < 10)
						sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
					else
						sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
					if (tmBas.tm_hour < 10)
						sprintf(namn, "%sT0%d", namn, tmBas.tm_hour);
					else
						sprintf(namn, "%sT%d", namn, tmBas.tm_hour);
					if (tmBas.tm_min < 10)
						sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
					else
						sprintf(namn, "%s:%d", namn, tmBas.tm_min);
					if (tmBas.tm_sec < 10)
						sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
					else
						sprintf(namn, "%s:%d", namn, tmBas.tm_sec);
					fprintf(filpekG, "\"channelCost\": %.2lf, \"startTime\": \"%s\", ", model.arc[arcNr].channelCost / nTp, namn);

					tmBas.tm_hour += 1; // model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
					mktime(&tmBas);
					sprintf(namn, "%d", tmBas.tm_year + 1900);
					if (tmBas.tm_mon + 1 < 10)
						sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
					else
						sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
					if (tmBas.tm_mday < 10)
						sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
					else
						sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
					if (tmBas.tm_hour < 10)
						sprintf(namn, "%s 0%d", namn, tmBas.tm_hour);
					else
						sprintf(namn, "%s %d", namn, tmBas.tm_hour);
					if (tmBas.tm_min < 10)
						sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
					else
						sprintf(namn, "%s:%d", namn, tmBas.tm_min);
					if (tmBas.tm_sec < 10)
						sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
					else
						sprintf(namn, "%s:%d", namn, tmBas.tm_sec);

					fprintf(filpekG, "\"endTime\": \"%s\",\n", namn);
					tmBas.tm_hour -= model.arc[arcNr].fromTime + ii + 1; // .toTime;

					fprintf(filpekG, "\"arcNr\": %d }, \"geometry\": { \"type\": \"MultiLineString\",\n", arcNr);
					fprintf(filpekG, "\"coordinates\": [ [ ");
					for (ii2 = 0; ii2 < nPkter; ii2++) {
						fprintf(filpekG, "[ %lf, %lf, 0.0 ]", x[ii2], y[ii2]);
						if (ii2 < nPkter - 1)
							fprintf(filpekG, ",\n");
						else
							fprintf(filpekG, " ] ] } }");
					}
				}
			}
		}
	}
	fprintf(filpekG, "\n]\n}");
	fclose(filpekG);

	fprintf(filPek, "total\tcombined\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
		distance, time, fuel, safety, channelCost, totCost);
	fprintf(filPek, "\nobj_weights\ntime\tfuel\tsafety\n%lf\t%lf\t%lf\n",
		model.params.weightTime, 1.0,
		model.params.weightSafety.base);
	//printf("\nobj_weights\ntime\tfuel\tsafety\n%.2lf\t%.2lf\t%.2lf\n",
	//	model.params.weightTime, model.params.weightFuel.base,
	//	model.params.weightSafety.base);
	//printf("results\ndist\t%.2lf\ntime\t%.2lf\nfuel\t%.2lf\nsafety\t%.2lf\nchannelCost\t%.2lf\ntotCost\t%.2lf\n",
	//	distance, time, fuel, safety, channelCost, totCost);
	if (resAlt == 0) {
		fprintf(filPek3, "\t\"objective\":{\"totCost\":%.2lf,\n", totCost);
		fprintf(filPek3, "\t\t\"dist\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", distance, 0.0, 0.0);
		fprintf(filPek3, "\t\t\"time\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", time, model.params.weightTime, time * model.params.weightTime * model.params.priceTime);
		fprintf(filPek3, "\t\t\"fuel\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf, \n\t\t\t\"sub\":{\n", fuel, 1.0, fuel);
		fprintf(filPek3, "\t\t\t\"VLSFO\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", fuelVLSFO, model.params.weightFuel.vlsfo,
			fuelVLSFO * model.params.weightFuel.vlsfo* model.params.priceFuel.vlsfo);
		fprintf(filPek3, "\t\t\t\"LSMGO\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf}\n\t\t\t}\n\t\t},", fuelLSMGO, model.params.weightFuel.lsmgo,
			fuelLSMGO * model.params.weightFuel.lsmgo* model.params.priceFuel.lsmgo);

		fprintf(filPek3, "\t\t\"safety\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf, \n\t\t\t\"sub\":{\n", safety, model.params.weightSafety.base, safety * model.params.weightSafety.base);
		fprintf(filPek3, "\t\t\t\"hurricane\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", 
			hurricane, model.params.weightSafety.hurricane, hurricane * model.params.weightSafety.hurricane);
		fprintf(filPek3, "\t\t\t\"bowSlamming\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n",
			bowSlamming, model.params.weightSafety.bowSlam,
			bowSlamming* model.params.weightSafety.bowSlam);
		fprintf(filPek3, "\t\t\t\"greenWater\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n",
			greenWater, model.params.weightSafety.greenWater,
			greenWater* model.params.weightSafety.greenWater);
		fprintf(filPek3, "\t\t\t\"dynamicStability\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n",
			dynStability, model.params.weightSafety.dynamicStability,
			dynStability* model.params.weightSafety.dynamicStability);
		fprintf(filPek3, "\t\t\t\"iceCoverage\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n",
			iceCoverage, 1,
			iceCoverage);
		fprintf(filPek3, "\t\t\t\"feasibleSafety\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n",
			feasibleSafety, model.params.weightSafety.feasibleSafety,
			feasibleSafety* model.params.weightSafety.feasibleSafety);
		//fprintf(filPek3, "\t\t\t\"waves\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", 0.0, model.params.weightSafety.waves, 0.0);
		//fprintf(filPek3, "\t\t\t\"stability\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf}\n", stability, model.params.weightSafety.stability,
		//	stability * model.params.weightSafety.stability);
		fprintf(filPek3, "\t\t\t}\n\t\t},\n");
		fprintf(filPek3, "\t\t\"channel\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf}\n", channelCost, 1.0, channelCost);
		fprintf(filPek3, "\t},\n");
	}
	else
		fprintf(filPek3, "\t\"eval_dist\": %.2lf,\n\t\"eval_time\": %.2lf,\n\t\"eval_fuel\": %.2lf,\n\t\"eval_safety\": %.2lf,\n\t\"eval_totCost\": %.2lf,\n",
			distance, time, fuel, safety, totCost);

	for (i = 0; i < model.params.nShip_speedSettings; i++) {
		if (nSpeedSettingUsed[i] > 0) {
			if (resAlt == 0)
				fprintf(filPek3, "\t\"no of times using rpm_setting %.2lf\": %d,\n", model.functions.rpm[i], nSpeedSettingUsed[i]);
			else
				fprintf(filPek3, "\t\"eval: no of times using rpm_setting %.2lf\": %d,\n", model.functions.rpm[i], nSpeedSettingUsed[i]);
			fprintf(filPek, "used rpm_setting %.2lf %d times\n", model.functions.rpm[i], nSpeedSettingUsed[i]);
			//printf("used speedSetting %s %d times\n", model.params.ship_speedSettingID[i], nSpeedSettingUsed[i]);
		}
	}
	if (resAlt == 0)
		fprintf(filPek3, "\t\"no of times the speed setting is changed\": %d\n", nSpeedChanges);
	else
		fprintf(filPek3, "\t\"eval: no of times the speed setting is changed\": %d\n", nSpeedChanges);
	fprintf(filPek, "The speed settings are changed %d times during the trip\n", nSpeedChanges);
	//printf("The speed settings are changed %d times during the trip\n", nSpeedChanges);

	fclose(filPek);
	fclose(filPek2);
	if (resAlt == 1 || model.params.readSolPathFile == "")
		fprintf(filPek3, "}\n");
	fclose(filPek3);

	return 0;
}

double identifyForecastType(double tidTot) {
	int tidInt;
	double forecastType = 0;

	tidInt = (int)(tidTot / model.weather[model.functions.pos_current_u].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_current_u].nTimeIntervals_forecast)
		forecastType += 1;
	tidInt = (int)(tidTot / model.weather[model.functions.pos_current_v].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_current_v].nTimeIntervals_forecast)
		forecastType += 1;

	tidInt = (int)(tidTot / model.weather[model.functions.pos_wind_u].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_wind_u].nTimeIntervals_forecast)
		forecastType += 1;
	tidInt = (int)(tidTot / model.weather[model.functions.pos_wind_v].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_wind_v].nTimeIntervals_forecast)
		forecastType += 1;

	tidInt = (int)(tidTot / model.weather[model.functions.pos_waveHeight].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast)
		forecastType += 1;
	tidInt = (int)(tidTot / model.weather[model.functions.pos_wavePeriod].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_wavePeriod].nTimeIntervals_forecast)
		forecastType += 1;
	tidInt = (int)(tidTot / model.weather[model.functions.pos_waveDirection].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_waveDirection].nTimeIntervals_forecast)
		forecastType += 1;

	tidInt = (int)(tidTot / model.weather[model.functions.pos_iceThickness].timeIntervall_h);
	if (tidInt >= model.weather[model.functions.pos_iceThickness].nTimeIntervals_forecast)
		forecastType += 1;

	return forecastType;
}

void evalWeatherDataAlongArc(int arcNr, int startSlutArc, double timeExact) {
	int i;
	double windSpeed_x, windSpeed_y, waveDir_y, waveDir_x, dist, tidTot, distNu;
	double stormVarde, uCurrent, vCurrent;
	double currentDirection, currentSpeed, baseGroundSpeed, calmWaterSpeed, uWind, vWind;
	double speedDiffWindWave, rel_windSpeed, rel_windDir, speedOverGround, timeArc;
	double fuelConsumption, fuelUsage, windDirection, windSpeed2, windSpeed, waveHeight, wavePeriod;
	double rel_waveDir, iceCover, waveDirection, speedDiffWind, speedDiffWave;

	model.functions.valuesNow.current = 0;
	model.functions.valuesNow.windSpeed = 0;
	windSpeed_x = 0;
	windSpeed_y = 0;
	model.functions.valuesNow.waveHeight = 0;
	model.functions.valuesNow.wavePeriod = 0;
	waveDir_x = 0;
	waveDir_y = 0;
	model.functions.valuesNow.forecastType = 0;
	model.functions.valuesNow.worstStormValue = 0;
	model.functions.valuesNow.iceCover_max = 0;
	model.functions.valuesNow.bowSlamming_max = 0;
	model.functions.valuesNow.greenWater_max = 0;
	model.functions.valuesNow.dynamicStability_max = 0;

	model.functions.valuesNow.relWindDir = 0;
	model.functions.valuesNow.relWaveDir = 0;


	dist = 0;

	if (startSlutArc == 0) {
		tidTot = timeExact;

		calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting);
		if (printGlobal == 1) {
			printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, model.arc[arcNr].speedSetting, calmWaterSpeed,
				model.weatherFunctions.nCheckPoints);
		}

		for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
			distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
			dist += distNu;

			//uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
			//vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

			stormVarde = getStormValue((int)(tidTot), model.weatherFunctions.point[i]);
			if (stormVarde > model.functions.valuesNow.worstStormValue)
				model.functions.valuesNow.worstStormValue = stormVarde;

			uCurrent = getVariableValue(model.functions.pos_current_u, i, tidTot);
			vCurrent = getVariableValue(model.functions.pos_current_v, i, tidTot);

			if (uCurrent < 1000 && vCurrent < 1000) {
				currentDirection = atan2(vCurrent, uCurrent);
				currentSpeed = sqrt(pow(uCurrent, 2) + pow(vCurrent, 2));
			}
			else {
				currentDirection = 0;
				currentSpeed = 0;
			}

			baseGroundSpeed = eval_baseGroundSpeed(calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
				currentDirection, currentSpeed);
			if (printGlobal == 1) {
				printf("checkP %d vCurrent %.3lf uCurrent %.3lf, currentDirection %.3lf currentSpeed %.3lf baseGroundSpeed %.3lf\n", i, vCurrent,
					uCurrent, currentDirection, currentSpeed, baseGroundSpeed);
			}


			uWind = getVariableValue(model.functions.pos_wind_u, i, tidTot);
			vWind = getVariableValue(model.functions.pos_wind_v, i, tidTot);
			if (uWind < 1000 && vWind < 1000) {
				windDirection = atan2(vWind, uWind);
				windSpeed2 = pow(uWind, 2) + pow(vWind, 2);
				windSpeed = sqrt(windSpeed2);

				rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
					windDirection, windSpeed, &rel_windDir);
				if (printGlobal == 1) {
					printf("checkP %d vWind %.3lf uWind %.3lf, windDirection %.3lf windSpeed %.3lf rel_windSpeed %.3lf rel_windDir %.3lf\n", i, vWind,
						uWind, windDirection, windSpeed, rel_windSpeed, rel_windDir);
				}
				//model.functions.valuesNow.worstStabilityValue += distNu * rel_windSpeed / 10000.0;
			}
			else {
				windSpeed = 0;
				rel_windDir = 0;
			}

			waveHeight = getVariableValue(model.functions.pos_waveHeight, i, tidTot);
			if (waveHeight > 100)
				waveHeight = 0;
			wavePeriod = getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
			if (wavePeriod > 1000)
				wavePeriod = 0;
			waveDirection = getVariableValue(model.functions.pos_waveDirection, i, tidTot);
			if (waveDirection > 1000)
				waveDirection = 0;
			rel_waveDir = (waveDirection - model.weatherFunctions.vesselBearing[i] / 180.0 * M_PI); // / model.functions.nWaveDir;
			if (printGlobal == 1) {
				printf("checkP %d waveDirection %.3lf rel_waveDir %.3lf\n", i, waveDirection, rel_waveDir);
			}

			speedDiffWind = lookup_speedDiffWindTable(calmWaterSpeed, rel_windSpeed, rel_windDir);
			speedDiffWave = lookup_speedDiffWaveTable(calmWaterSpeed, waveHeight, wavePeriod, rel_waveDir);
			speedDiffWindWave = speedDiffWind + speedDiffWave;
			//speedDiffWindWave = lookup_speedDiffWindWaveTable(rel_windSpeed, rel_windDir, waveHeight, wavePeriod, rel_waveDir) * (
			//	calmWaterSpeed / model.functions.weatherFactorsTable_shipSpeed);
			speedOverGround = baseGroundSpeed * model.params.knots_to_km - speedDiffWindWave; // in km/h
			timeArc = distNu / speedOverGround; // in hours

			//printf("dist %.2lf calmWaterSpeed %.2lf worstStormValue %.2lf vesselBearing %.2lf currDir %.2lf currSpeed %.2lf uCurr %.2lf vCurr %.2lf baseGroundSpeed %.2lf rel_windSpeed %.2lf waveHeight %.2lf wavePeriod %.2lf rel_waveDir %.2lf speedDiffWindWave %.2lf speedOverGround %.2lf timeArc %.2lf\n",
			//	dist, calmWaterSpeed, *worstStormValue, model.weatherFunctions.vesselBearing[i],
			//	currentDirection, currentSpeed, uCurrent, vCurrent, baseGroundSpeed, rel_windSpeed, waveHeight, wavePeriod,
			//	rel_waveDir, speedDiffWindWave, speedOverGround, timeArc);

			fuelConsumption = eval_fuelConsumption(model.arc[arcNr].speedSetting);
			fuelUsage = fuelConsumption * timeArc;

			if (printGlobal == 1) {
				printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf fuelCons %.3lf\n",
					speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu, fuelConsumption);
			}

			model.functions.valuesNow.forecastType += identifyForecastType(tidTot) * timeArc;

			tidTot += timeArc;

			iceCover = getVariableValue(model.functions.pos_iceThickness, i, tidTot);
			if (iceCover > 1000)
				iceCover = 0;
			if (iceCover > model.functions.valuesNow.iceCover_max)
				model.functions.valuesNow.iceCover_max = iceCover;

			eval_safety(rel_windSpeed, rel_windDir, waveHeight, wavePeriod, iceCover);
			if (model.functions.valuesNow.bowSlam > model.functions.valuesNow.bowSlamming_max)
				model.functions.valuesNow.bowSlamming_max = model.functions.valuesNow.bowSlam;
			if (model.functions.valuesNow.greenWater > model.functions.valuesNow.greenWater_max)
				model.functions.valuesNow.greenWater_max = model.functions.valuesNow.greenWater;
			if (model.functions.valuesNow.dynamicStability > model.functions.valuesNow.dynamicStability_max)
				model.functions.valuesNow.dynamicStability_max = model.functions.valuesNow.dynamicStability;

			model.functions.valuesNow.current += timeArc * (baseGroundSpeed - calmWaterSpeed);
			model.functions.valuesNow.windSpeed += timeArc * windSpeed;
			model.functions.valuesNow.waveHeight += timeArc * waveHeight;
			model.functions.valuesNow.wavePeriod += timeArc * wavePeriod;

			//printf("arcNr %d pos %d baseGroundSpeed %.2lf calmWaterSpeed %.2lf timeArc %.2lf currentAcc %.2lf\n", arcNr, i,
			//	baseGroundSpeed, calmWaterSpeed, timeArc, model.functions.valuesNow.current);
			windSpeed_x += timeArc * windSpeed * cos(rel_windDir);
			windSpeed_y += timeArc * windSpeed * sin(rel_windDir);
			waveDir_x += timeArc * waveHeight * cos(rel_waveDir);
			waveDir_y += timeArc * waveHeight * sin(rel_waveDir);

		}

		if (tidTot > 0) {
			model.functions.valuesNow.current /= tidTot;
			model.functions.valuesNow.windSpeed /= tidTot;
			model.functions.valuesNow.waveHeight /= tidTot;
			model.functions.valuesNow.wavePeriod /= tidTot;
			model.functions.valuesNow.forecastType /= (tidTot * 8);
		}
		model.functions.valuesNow.relWindDir = atan2(windSpeed_y, windSpeed_x) * 180.0 / M_PI;
		if (model.functions.valuesNow.relWindDir < 0)
			model.functions.valuesNow.relWindDir = -model.functions.valuesNow.relWindDir;
		//printf("wind_y %.2lf wind_x %.2lf relWindDir %.2lf\n",
		//	windSpeed_y, windSpeed_x, model.functions.valuesNow.relWindDir);
		model.functions.valuesNow.relWaveDir = atan2(waveDir_y, waveDir_x) * 180.0 / M_PI;
		if (model.functions.valuesNow.relWaveDir < 0)
			model.functions.valuesNow.relWaveDir = -model.functions.valuesNow.relWaveDir;
	}
}


void addPositionDataToReport(FILE* filpekG, int posReport, int arcNr, int startSlutArc, double* timeExact, std::string solName) {
	int lev1, lev2, pointNr1, pointNr2, timmar, minuter, sekunder;
	struct tm tmBas;
	double x, y, bearing, speedOnGround;
	spherical::Point p1, p2;

	if (posReport > 0)
		fprintf(filpekG, ",\n ");

	lev1 = model.arc[arcNr].fromLevel;
	lev2 = model.arc[arcNr].toLevel;
	pointNr1 = model.arc[arcNr].fromPointNr;
	pointNr2 = model.arc[arcNr].toPointNr;

	int prefPath = 0;
	if (lev1 >= 0 && lev2 >= 0) {
		if (pointNr1 == model.params.preferredPathOrtoPos[lev1] && pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1)
			prefPath = 1;
	}

	if (lev1 >= 0)
		p1 = model.network.physicalLev[lev1].point[pointNr1];
	else
		p1 = model.network.channel[-lev1 - 1].point[0];
	if (lev2 >= 0)
		p2 = model.network.physicalLev[lev2].point[pointNr2];
	else
		p2 = model.network.channel[-lev2 - 1].point[0];

	if (lev1 >= 0) {
		p1 = model.network.physicalLev[lev1].point[pointNr1];
		if (lev2 >= 0) {
			p2 = model.network.physicalLev[lev2].point[pointNr2];
			if (startSlutArc == 0) {
				if (prefPath == 1)
					calcWeatherPosAlongpreferredPathArc(p1, lev1);
				else
					calcWeatherPosAlongArc(p1, p2);
			}
		}
		else {
			p2 = model.network.channel[-lev2 - 1].point[0];
			if (startSlutArc == 0)
				calcWeatherPosAlongArc(p1, p2);
		}
	}
	else {
		if (lev2 >= 0) {
			p2 = model.network.physicalLev[lev2].point[pointNr2];
			p1 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
			if (startSlutArc == 0)
				calcWeatherPosAlongArc(p1, p2);
		}
		else {
			p1 = model.network.channel[-lev1 - 1].point[0];
			p2 = model.network.physicalLev[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
			if (startSlutArc == 0)
				calcWeatherPosAlongChannel(-lev1 - 1);
			//model.network.channel[-thisLevel - 1].point[pointPos],
			//	model.network.channel[-nextLevel-1].point[outNodePos]);
		}
	}

	if (startSlutArc == 0) {
		y = p1.latitude().degrees();
		x = p1.longitude().degrees();
	}
	else {
		y = p2.latitude().degrees();
		x = p2.longitude().degrees();
	}
	bearing = p1.bearingTo(p2);
	speedOnGround = model.arc[arcNr].distance / model.arc[arcNr].time;

	char* startTime = (char*)malloc(256 * sizeof(char));
	time_t rawtime;
	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	timmar = (int)(*timeExact); // *24;
	tmBas.tm_hour = model.params.startHour + timmar;
	minuter = (int)((*timeExact - timmar) * 60.0);
	tmBas.tm_min = minuter;
	sekunder = (int)((*timeExact - timmar - minuter / 60.0) * 60.0);
	tmBas.tm_sec = sekunder;
	mktime(&tmBas);
	fixReadableDate(tmBas, startTime);

	evalWeatherDataAlongArc(arcNr, startSlutArc, *timeExact);

	(*timeExact) += model.arc[arcNr].time;

	fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"dateUTC\":\"%s\",\n",
		startTime);
	fprintf(filpekG, "\"solutionID\":\"%s\",\n",
		solName.c_str());
	if (startSlutArc == 0) {
		fprintf(filpekG, "    \"hours\":%lf, \"distance_km\":%lf,\n",
			model.arc[arcNr].time, model.arc[arcNr].distance);
		fprintf(filpekG, "    \"Position_lat_lon\":\"%.3lf, %.3lf\", \"bearing\":%.0lf,\n",
			y, x, bearing);
		fprintf(filpekG, "    \"speedSetting\":%d, \"speedOnGround_km_h\":%.2lf, \"rpm\":%.2lf,\n",
			model.arc[arcNr].speedSetting, speedOnGround, model.functions.rpm[model.arc[arcNr].speedSetting]);
		fprintf(filpekG, "    \"fuelConsumption_ton\":%.3lf, \"current_km_h\":%.3lf,\n",
			model.arc[arcNr].fuelLSMGO + model.arc[arcNr].fuelVLSFO, model.functions.valuesNow.current);
		fprintf(filpekG, "    \"windSpeed_m_s\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
			model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir);
		fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
			model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
			model.functions.valuesNow.relWaveDir);

		fprintf(filpekG, "    \"max bow slamming p\":%.3lf, \"max green water p\":%.3lf,\n    \"max dynamic instability\":%.3lf,\n",
			model.functions.valuesNow.bowSlamming_max, model.functions.valuesNow.greenWater_max,
			model.functions.valuesNow.dynamicStability_max);
		fprintf(filpekG, "    \"max iceCover\":%.3lf,  \"worstStormValue\":%.3lf, \"forecastType\":%lf},\n",
			model.functions.valuesNow.iceCover_max, model.functions.valuesNow.worstStormValue, model.functions.valuesNow.forecastType);
	}
	else{
		fprintf(filpekG, "    \"hours\":%lf, \"distance_km\":%lf,\n", 0.0, 0.0);
		fprintf(filpekG, "    \"Position_lat_lon\":\"%.3lf, %.3lf\", \"bearing\":%.0lf,\n",
			y, x, bearing);
		fprintf(filpekG, "    \"speedSetting\":%d, \"speedOnGround_km_h\":%.2lf, \"rpm\":%.2lf,\n",
			-1, 0, 0);
		fprintf(filpekG, "    \"fuelConsumption_ton\":%.3lf, \"current_km_h\":%.3lf,\n",
			0, model.functions.valuesNow.current);
		fprintf(filpekG, "    \"windSpeed_m_s\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
			model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir);
		fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
			model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
			model.functions.valuesNow.relWaveDir);

		fprintf(filpekG, "    \"max bow slamming p\":%.3lf, \"max green water p\":%.3lf,\n    \"max dynamic instability\":%.3lf,\n",
			0.0, 0.0, 0.0);
		fprintf(filpekG, "    \"max iceCover\":%.3lf,  \"worstStormValue\":%.3lf, \"forecastType\":%lf},\n",
			0.0, 0.0, 0.0);
	}

	fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", x, y);
}

int writeSolutionToJson(string filename, int resAlt)
{
	int nAllocPkter, i, iPos, nPkter, nArcs, ii3;
	int arcNr, lev1, lev2, pointNr1, pointNr2, timeInt, * nSpeedSettingUsed, nSpeedChanges = 0;
	double* x, * y;
	struct tm tmBas;
	FILE* filpekG;
	time_t rawtime;

	//printf("here 1\n");
	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = 0;
	tmBas.tm_sec = 0;
	//timeNu = mktime(&tmBas);

	int nAlloc = model.network.nMaxNodesInPath * model.nBVArcs, prefPath;

	x = (double*)malloc(nAlloc * sizeof(double));
	y = (double*)malloc(nAlloc * sizeof(double));
	nSpeedSettingUsed = (int*)calloc(model.params.nShip_speedSettings, sizeof(int));

	//printf("here 2\n");
	FILE* filPek, * filPek2;
	std::string solName;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	//sprintf(namn, "%s.csv", filename);
	sprintf(namn, "%s/resSolution%d.csv", model.params.resultPath.c_str(), resAlt);
	filPek = fopen(namn, "w");
	sprintf(namn, "%s/solPath%d.txt", model.params.resultPath.c_str(), resAlt);
	std::string linePath = "";
	filPek2 = fopen(namn, "w");
	if (resAlt == 0) {
		filpekG = fopen(filename.c_str(), "w"); // "result_json.json", "w");
		if (filpekG == NULL)
		{
			printf("Faile to open file %s for writing.\n", filename.c_str());
			errlog("Faile to open file %s for writing.\n", filename.c_str());
			exitKontrollerat(__LINE__);
		}
		//fprintf(filpekG, "{\n\"solutions\":[\n");
		initGeoJsonFil(filpekG, "result_path");
		//fprintf(filpekG, "\"solutionID\":\"base\",\n");
		solName = "base";
		linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";

		//fprintf(filpekG, "{ \"type\": \"Feature\",\n");
		//fprintf(filpekG, "\"geometry\": { \"type\": \"MultiLineString\",\n");
		//fprintf(filpekG, "\"coordinates\": [ [\n");

		//fprintf(filPek3, "{\n");
		//fprintf(filPek3, "\t\"solutionShape\": \"/%s\",\n", filename.c_str());
	}
	else {
		filpekG = fopen(filename.c_str(), "a+"); // "result_json.json", "w");
		solName = "solAlt_" + to_string(resAlt);
		linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";
		fprintf(filpekG, ",\n");
	}

	fprintf(filPek2, "level;nodPos;speedSetting;arcNr(for_information_only);nod1(info);nod2(info)\n");

	fprintf(filPek, "arcPos\tspeedSetting\tdistance\ttime\tfuelBase\tsafetyBase\tchannelCost\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
		"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\tnodNr1\tnodNr2\n");

	double time = 0, fuel = 0, safety = 0, totCost = 0, distance = 0, channelCost = 0;
	double fuelLSMGO = 0, fuelVLSFO = 0, hurricane = 0, distanceTp, distNu, distTmp;// , stability = 0;
	double bowSlamming = 0, greenWater = 0, dynStability = 0, iceCoverage = 0, feasibleSafety = 0, timeExact = 0;
	int ii, nTp, nAdded, ii2, posIreport;
	spherical::Point pointLast, pointFinal;

	//printf("here 3\n");
	nArcs = 0;
	arcNr = -1;
	nPkter = 0;
	posIreport = 0;
	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
	{
		// kopiera delen av punktfoljden som anvands, dess xyz
		arcNr = model.BVArc[iPos];
		if (arcNr == 559126)
			arcNr = arcNr;
		lev1 = model.arc[arcNr].fromLevel;
		lev2 = model.arc[arcNr].toLevel;
		pointNr1 = model.arc[arcNr].fromPointNr;
		pointNr2 = model.arc[arcNr].toPointNr;
		if (iPos > 0) {
			if (model.arc[arcNr].speedSetting != model.arc[model.BVArc[iPos - 1]].speedSetting)
				nSpeedChanges++;
		}
		(nSpeedSettingUsed[model.arc[arcNr].speedSetting])++;

		nTp = model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
		if (nTp == 0)
			nTp = 1;
		distanceTp = 1000.0 * model.arc[arcNr].distance / nTp;
		nAdded = 0;
		addPositionDataToReport(filpekG, posIreport++, arcNr, 0, &timeExact, solName);


		if (iPos == 31)
			iPos = iPos;
		if (lev1 < 0 && lev2 < 0) {
			distNu = 0;
			for (i = 0; i < model.network.channel[-lev1 - 1].nPoints; i++) {
				if (i > 0) {
					distTmp = model.network.channel[-lev1 - 1].point[i - 1].distanceTo(model.network.channel[-lev1 - 1].point[i]);
					for (ii = 0; ii < nTp; ii++) {
						if (distNu + distTmp >= distanceTp || (i == model.network.channel[-lev1 - 1].nPoints - 1 && distNu + distTmp >= distanceTp * 0.95)) {
							// identifiera pkten dar distTmp + distNu = distanceTp
							pointLast = pointLast.destinationPoint(distanceTp - distNu, pointLast.bearingTo(model.network.channel[-lev1 - 1].point[i]));
							if (nPkter == 0) {
								y[nPkter] = pointLast.latitude().degrees();
								x[nPkter] = pointLast.longitude().degrees();
								nPkter++;
							}
							// spara pkten dar distTmp + distNu = distanceTp
							distance += model.arc[arcNr].distance / nTp;
							time += model.arc[arcNr].time / nTp;
							fuel += model.arc[arcNr].fuelBase / nTp;
							fuelLSMGO += model.arc[arcNr].fuelLSMGO / nTp;
							fuelVLSFO += model.arc[arcNr].fuelVLSFO / nTp;
							safety += model.arc[arcNr].safetyBase / nTp;
							hurricane += model.arc[arcNr].safetyHurricane / nTp;
							bowSlamming += model.arc[arcNr].safetyBowSlam / nTp;
							greenWater += model.arc[arcNr].safetyGreenWater / nTp;
							dynStability += model.arc[arcNr].safetyDynStability / nTp;
							iceCoverage += model.arc[arcNr].iceCoverCost / nTp;
							feasibleSafety += (double)(model.arc[arcNr].feasibleSafety) / nTp;
							//stability += model.arc[arcNr].safetyStability / nTp;
							channelCost += model.arc[arcNr].channelCost / nTp;
							totCost += model.arc[arcNr].totCost / nTp;

							nAdded++;
							if (nAdded >= nTp)
								break;
							distTmp -= distanceTp - distNu;
							distNu = 0;
							//nPkter = 0;
							if (nPkter >= nAlloc) {
								nAlloc += 1000;
								x = (double*)realloc(x, nAlloc * sizeof(double));
								y = (double*)realloc(y, nAlloc * sizeof(double));
							}
							y[nPkter] = pointLast.latitude().degrees();
							x[nPkter] = pointLast.longitude().degrees();
							//z[nPkter] = 0;
							nPkter++;

						}
						else {
							distNu += distTmp;
							break;
						}
					}
				}
				if (nPkter >= nAlloc) {
					nAlloc += 1000;
					x = (double*)realloc(x, nAlloc * sizeof(double));
					y = (double*)realloc(y, nAlloc * sizeof(double));
				}
				y[nPkter] = model.network.channel[-lev1 - 1].point[i].latitude().degrees();
				x[nPkter] = model.network.channel[-lev1 - 1].point[i].longitude().degrees();
				pointLast = spherical::Point(y[nPkter], x[nPkter]);
				//z[nPkter] = 0;
				nPkter++;
			}
		}
		else {
			prefPath = 0;
			if (lev1 >= 0 && lev2 >= 0) {
				if (pointNr1 == model.params.preferredPathOrtoPos[lev1] &&
					pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1) {
					if (nPkter == 0) {
						y[nPkter] = model.network.physicalLev[lev1].point[pointNr1].latitude().degrees();
						x[nPkter] = model.network.physicalLev[lev1].point[pointNr1].longitude().degrees();
						//z[nPkter] = 0;
						nPkter++;
					}
					pointLast = spherical::Point(y[nPkter-1], x[nPkter-1]);
					//nPkter = 1;
					distNu = 0;
					ii3 = 0;
					for (int i3 = 0; i3 < model.network.physicalLev[lev1].npreferredPathPoints; i3++) {
						distTmp = pointLast.distanceTo(model.network.physicalLev[lev1].preferredPathPoint[i3]);
						for (ii = 0; ii < nTp; ii++) {
							if (distNu + distTmp >= distanceTp || (i3 == model.network.physicalLev[lev1].npreferredPathPoints - 1 && distNu + distTmp >= distanceTp * 0.95)) {
								// identifiera pkten dar distTmp + distNu = distanceTp
								pointLast = pointLast.destinationPoint(distanceTp - distNu, pointLast.bearingTo(model.network.physicalLev[lev1].preferredPathPoint[i3]));
								//y[nPkter] = pointLast.latitude().degrees();
								//x[nPkter] = pointLast.longitude().degrees();
								//z[nPkter] = 0;
								//nPkter++;

								// spara pkten dar distTmp + distNu = distanceTp
								distance += model.arc[arcNr].distance / nTp;
								time += model.arc[arcNr].time / nTp;
								fuel += model.arc[arcNr].fuelBase / nTp;
								fuelLSMGO += model.arc[arcNr].fuelLSMGO / nTp;
								fuelVLSFO += model.arc[arcNr].fuelVLSFO / nTp;
								safety += model.arc[arcNr].safetyBase / nTp;
								hurricane += model.arc[arcNr].safetyHurricane / nTp;
								bowSlamming += model.arc[arcNr].safetyBowSlam / nTp;
								greenWater += model.arc[arcNr].safetyGreenWater / nTp;
								dynStability += model.arc[arcNr].safetyDynStability / nTp;
								iceCoverage += model.arc[arcNr].iceCoverCost / nTp;
								feasibleSafety += (double)(model.arc[arcNr].feasibleSafety) / nTp;
								//stability += model.arc[arcNr].safetyStability / nTp;
								channelCost += model.arc[arcNr].channelCost / nTp;
								totCost += model.arc[arcNr].totCost / nTp;

								nAdded++;
								if (nAdded >= nTp)
									break;
								distTmp -= distanceTp - distNu;
								ii3++;
								distNu = 0;
								//nPkter = 0;
								if (nPkter >= nAlloc) {
									nAlloc += 1000;
									x = (double*)realloc(x, nAlloc * sizeof(double));
									y = (double*)realloc(y, nAlloc * sizeof(double));
								}
								y[nPkter] = pointLast.latitude().degrees();
								x[nPkter] = pointLast.longitude().degrees();
								//z[nPkter] = 0;
								nPkter++;
							}
							else {
								distNu += distTmp;
								break;
							}
						}
						if (nPkter >= nAlloc) {
							nAlloc += 1000;
							x = (double*)realloc(x, nAlloc * sizeof(double));
							y = (double*)realloc(y, nAlloc * sizeof(double));
						}
						y[nPkter] = model.network.physicalLev[lev1].preferredPathPoint[i3].latitude().degrees();
						x[nPkter] = model.network.physicalLev[lev1].preferredPathPoint[i3].longitude().degrees();
						pointLast = spherical::Point(y[nPkter], x[nPkter]);
						//z[nPkter] = 0;
						nPkter++;
					}
					prefPath = 1;
				}
			}
			if (prefPath == 0) {
				if (nPkter == 0) {
					if (nPkter >= nAlloc) {
						nAlloc += 1000;
						x = (double*)realloc(x, nAlloc * sizeof(double));
						y = (double*)realloc(y, nAlloc * sizeof(double));
					}
					if (lev1 >= 0) {
						y[nPkter] = model.network.physicalLev[lev1].point[pointNr1].latitude().degrees();
						x[nPkter] = model.network.physicalLev[lev1].point[pointNr1].longitude().degrees();
					}
					else {
						y[nPkter] = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1].latitude().degrees();
						x[nPkter] = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1].longitude().degrees();
					}
					//z[nPkter] = 0;
					nPkter++;
				}
				if (nPkter >= nAlloc) {
					nAlloc += 1000;
					x = (double*)realloc(x, nAlloc * sizeof(double));
					y = (double*)realloc(y, nAlloc * sizeof(double));
				}
				if (lev2 >= 0) {
					y[nPkter] = model.network.physicalLev[lev2].point[pointNr2].latitude().degrees();
					x[nPkter] = model.network.physicalLev[lev2].point[pointNr2].longitude().degrees();
				}
				else {
					y[nPkter] = model.network.channel[-lev2 - 1].point[0].latitude().degrees();
					x[nPkter] = model.network.channel[-lev2 - 1].point[0].longitude().degrees();
				}
				//z[nPkter] = 0;
				nPkter++;
				pointLast = spherical::Point(y[nPkter-2], x[nPkter-2]);
				pointFinal = spherical::Point(y[nPkter-1], x[nPkter-1]);
				for (ii = 0; ii < nTp; ii++) {
					//y[nPkter] = pointLast.latitude().degrees();
					//x[nPkter] = pointLast.longitude().degrees();
					//nPkter++;
					pointLast = pointLast.destinationPoint(distanceTp, pointLast.bearingTo(pointFinal));
					if (nPkter >= nAlloc) {
						nAlloc += 1000;
						x = (double*)realloc(x, nAlloc * sizeof(double));
						y = (double*)realloc(y, nAlloc * sizeof(double));
					}
					y[nPkter] = pointLast.latitude().degrees();
					x[nPkter] = pointLast.longitude().degrees();
					nPkter++;

					distance += model.arc[arcNr].distance / nTp;
					time += model.arc[arcNr].time / nTp;
					fuel += model.arc[arcNr].fuelBase / nTp;
					fuelLSMGO += model.arc[arcNr].fuelLSMGO / nTp;
					fuelVLSFO += model.arc[arcNr].fuelVLSFO / nTp;
					safety += model.arc[arcNr].safetyBase / nTp;
					hurricane += model.arc[arcNr].safetyHurricane / nTp;
					bowSlamming += model.arc[arcNr].safetyBowSlam / nTp;
					greenWater += model.arc[arcNr].safetyGreenWater / nTp;
					dynStability += model.arc[arcNr].safetyDynStability / nTp;
					iceCoverage += model.arc[arcNr].iceCoverCost / nTp;
					feasibleSafety += (double)(model.arc[arcNr].feasibleSafety) / nTp;
					//stability += model.arc[arcNr].safetyStability / nTp;
					channelCost += model.arc[arcNr].channelCost / nTp;
					totCost += model.arc[arcNr].totCost / nTp;

				}
			}
		}
	}
	//printf("here 3b\n");

	if(arcNr >= 0)
		addPositionDataToReport(filpekG, posIreport++, arcNr, 1, &timeExact, solName);

	if(posIreport > 0)
		fprintf(filpekG, ", ");
	fprintf(filpekG, "%s", linePath.c_str());
	for (ii2 = 0; ii2 < nPkter; ii2++) {
		if (ii2 > 0)
			fprintf(filpekG, ", ");
		fprintf(filpekG, "[ %lf, %lf, 0.0 ]\n", x[ii2], y[ii2]);
	}


	//printf("here 3c\n");

	fprintf(filpekG, "\n]]},\n\"properties\": {\n");

	double averSpeed;
	char* startTime, * endTime;
	startTime = (char*)malloc(256 * sizeof(char));
	endTime = (char*)malloc(256 * sizeof(char));
	fixReadableDate(tmBas, startTime);
	tmBas.tm_hour += time;
	mktime(&tmBas);
	fixReadableDate(tmBas, endTime);

	fprintf(filpekG, "\"solutionID\": \"%s\", \"ID\": \"routeInfo\", \"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n", 
		solName.c_str(), startTime, endTime);
	fprintf(filpekG, ", \"fuelConsumption_ton\": %.3lf, \"VLSFO\": %.3lf, \"LSMGO\": %.3lf",
		fuelVLSFO + fuelLSMGO, fuelVLSFO, fuelLSMGO);
	if (time > 0)
		averSpeed = distance / time / model.params.knots_to_km;
	else
		averSpeed = 0;

	fprintf(filpekG, ", \"average speed\": %.3lf, \"safety\": %.3lf, \"channel cost\": %.3lf, \"total cost\": %.3lf",
		averSpeed, safety, channelCost, totCost);
	fprintf(filpekG, ", \"totalDistance_km\": %.3lf, \"totalTime_h\": %.3lf}}\n", distance, time);

	//fprintf(filpekG, "]\n}");

	fclose(filpekG);

	fprintf(filPek, "total\tcombined\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
		distance, time, fuel, safety, channelCost, totCost);
	fprintf(filPek, "\nobj_weights\ntime\tfuel\tsafety\n%lf\t%lf\t%lf\n",
		model.params.weightTime, 1.0,
		model.params.weightSafety.base);

	//printf("\nobj_weights\ntime\tfuel\tsafety\n%.2lf\t%.2lf\t%.2lf\n",
	//	model.params.weightTime, model.params.weightFuel.base,
	//	model.params.weightSafety.base);
	printf("\nobj_weights\ntime\t\t%.2lf\tcost_h\t%.2lf\nfuel VLSFO\t%.2lf\tcost_ton\t%.2lf\nfuel LSMGO\t%.2lf\tcost_ton\t%.2lf\n",
		model.params.weightTime, model.params.priceTime, 
		model.params.weightFuel.vlsfo, model.params.priceFuel.vlsfo,
		model.params.weightFuel.lsmgo, model.params.priceFuel.lsmgo);
	printf("hurricane\t%.2lf\nbowSlamming\t%.2lf\ngreenWater\t%.2lf\ndynamicStability\t\t%.2lf\n",
		model.params.weightSafety.hurricane, model.params.weightSafety.bowSlam,
		model.params.weightSafety.greenWater, model.params.weightSafety.dynamicStability);
	printf("feasibleSafety\t%.2lf\niceCoverCost_fix\t%.2lf\niceCoverCost_thickness\t%.2lf\n",
		model.params.weightSafety.feasibleSafety, model.params.weightSafety.iceCoverCost_fix,
		model.params.weightSafety.iceCoverCost_thickness);
	printf("results\ndist\t%.2lf\ntime\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\nfuel\t%.2lf\tVLSFO\t%.2lf\tLSMGO\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\nsafety\t%.2lf\tobj\t%.2lf\nchannelCost\t%.2lf\ntotObjValue\t%.2lf\n",
		distance, time, model.params.priceTime * time, model.params.weightTime* model.params.priceTime* time,
		fuelVLSFO + fuelVLSFO, fuelVLSFO, fuelLSMGO, fuelVLSFO * model.params.priceFuel.vlsfo+ fuelLSMGO * model.params.priceFuel.lsmgo,
		fuelVLSFO* model.params.weightFuel.vlsfo * model.params.priceFuel.vlsfo + fuelLSMGO * model.params.weightFuel.lsmgo * model.params.priceFuel.lsmgo,
		safety, model.params.weightSafety.base * safety, 
		channelCost, totCost); 

	for (i = 0; i < model.params.nShip_speedSettings; i++) {
		if (nSpeedSettingUsed[i] > 0) {
			fprintf(filPek, "used rpm_setting %.2lf %d times\n", model.functions.rpm[i], nSpeedSettingUsed[i]);
			printf("used rpm_setting %.2lf %d times\n", model.functions.rpm[i], nSpeedSettingUsed[i]);
		}
	}
	fprintf(filPek, "The speed settings are changed %d times during the trip\n", nSpeedChanges);
	printf("The speed settings are changed %d times during the trip\n", nSpeedChanges);

	fclose(filPek);
	fclose(filPek2);

	return 0;
}

int fixReadableDate(struct tm tmBas, char* namn) {
	sprintf(namn, "%d", tmBas.tm_year + 1900);
	if (tmBas.tm_mon + 1 < 10)
		sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
	else
		sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
	if (tmBas.tm_mday < 10)
		sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
	else
		sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
	if (tmBas.tm_hour < 10)
		sprintf(namn, "%sT0%d", namn, tmBas.tm_hour);
	else
		sprintf(namn, "%sT%d", namn, tmBas.tm_hour);
	if (tmBas.tm_min < 10)
		sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
	else
		sprintf(namn, "%s:%d", namn, tmBas.tm_min);
	if (tmBas.tm_sec < 10)
		sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
	else
		sprintf(namn, "%s:%d", namn, tmBas.tm_sec);
	
	return 0;
}

time_t make_gmtime_now() {
	struct tm tmBas = { std::time(0) };
//#ifdef WIN32
//	time_t rawtime = _mkgmtime(&tmBas);
//#endif
//#ifndef WIN32
//	time_t rawtime = timegm(&tmBas);
//#endif
	//cout << "nSeconds " << tmBas.tm_sec << "\n";
	return tmBas.tm_sec;
	//return rawtime;
}

time_t make_gmtime(strParams* params) {
	struct tm tmBas = { 0 };
	tmBas.tm_year = params->startYear - 1900;
	tmBas.tm_mon = params->startMonth_nr - 1; // sep
	tmBas.tm_mday = params->startDay_nr;
	tmBas.tm_hour = params->startHour; // 0;
	tmBas.tm_min = params->startMinute;
	tmBas.tm_sec = 0;
	mktime(&tmBas);
#ifdef WIN32
	time_t rawtime = _mkgmtime(&tmBas);
#endif
#ifndef WIN32
	time_t rawtime = timegm(&tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}

int getTimeZoneDiff(std::string tidpkt, int* nMinDiff) {
	int pos = tidpkt.find_last_of(" ");
	char*  zone = str_alloc_cpy(tidpkt.substr(pos + 1, tidpkt.length() - pos).c_str());
	int nHourDiff, i;

	for (i = 0; i < model.params.nTimeZones; i++) {
		if (strcmp(zone, model.params.timeZone[i].name) == 0) {
			*nMinDiff = model.params.timeZone[i].nMinutesDiff;
			nHourDiff = model.params.timeZone[i].nHoursDiff;
			break;
		}
	}
	if (i >= model.params.nTimeZones) {
		errlog("ERROR! Time zone %s in storms not defined. Add it in file . Assuming UTC\n", zone);
		*nMinDiff = 0;
		nHourDiff = 0;
	}

	free(zone);
	return nHourDiff;
}

long long make_gmtime_fromStormDateTime(std::string tidpkt) {
	struct tm tmBas = { 0 };
	int hour, nHoursDiffTZ = 0, nMinDiffTZ = 0, nValuesHour;

	nHoursDiffTZ = getTimeZoneDiff(tidpkt, &nMinDiffTZ);

	tmBas.tm_year = std::stoi(tidpkt.substr(0, 4)) - 1900;
	tmBas.tm_mon = std::stoi(tidpkt.substr(5, 2)) - 1; // sep
	tmBas.tm_mday = std::stoi(tidpkt.substr(8, 2));
	if (tidpkt.substr(12, 1) == ":")
		nValuesHour = 1;
	else
		nValuesHour = 2;
	hour = std::stoi(tidpkt.substr(11, nValuesHour));
	//printf("genDateTimeFromStorm %s sub %s\n", 
	//	tidpkt.c_str(), tidpkt.substr(15 + nValuesHour, 2).c_str());
	if (tidpkt.substr(15 + nValuesHour, 2) == "PM") {
		if (hour < 12)
			hour += 12;
	}
	else {
		if (hour == 12)
			hour = 0;
	}
	tmBas.tm_hour = hour + nHoursDiffTZ;
	tmBas.tm_min = std::stoi(tidpkt.substr(12 + nValuesHour, 2)) + nMinDiffTZ;
	tmBas.tm_sec = 0;
	mktime(&tmBas);
#ifdef WIN32
	time_t rawtime = _mkgmtime(&tmBas);
#endif
#ifndef WIN32
	time_t rawtime = timegm(&tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}

time_t getFirstSecondOfDay(long long seconds) {
	time_t sec = seconds;
	struct tm* tmBas = gmtime(&sec);
	mktime(tmBas);
	tmBas->tm_hour = 0;
	tmBas->tm_min = 0;
	tmBas->tm_sec = 0;
#ifdef WIN32
	time_t rawtime = _mkgmtime(tmBas);
#endif
#ifndef WIN32
	time_t rawtime = timegm(tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}

int getManadDagFranUTCSeconds(long long seconds, int* dag) {
	time_t sec = seconds;
	struct tm* tmBas = gmtime(&sec);
	mktime(tmBas);
	*dag = tmBas->tm_mday;
	return tmBas->tm_mon + 1;
}

string stringDateFromUTCSeconds(long long seconds) {
	time_t sec = seconds;
	struct tm* tmBas = gmtime(&sec);
	mktime(tmBas);
	char date_string[100];
	strftime(date_string, 50, "%B %d, %Y %T", tmBas);
	string resultat = date_string;
	return resultat;
}

int writeSolutionToJson_dummy(string resultName)
{
	int i;
	int nSpeedChanges = 0;
	double* y;
	struct tm tmBas;
	FILE* filpekG;
	time_t rawtime;

	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900; 
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = 0;
	tmBas.tm_sec = 0;
	//timeNu = mktime(&tmBas);

	//string namn = resultPath;// +"/result.json";
	filpekG = fopen(resultName.c_str(), "w");
	if (filpekG == NULL)
	{
		printf("Faile to open file %s for writing.\n", resultName.c_str());
		errlog("Faile to open file %s for writing.\n", resultName.c_str());
		exitKontrollerat(__LINE__);
	}

	initGeoJsonFil(filpekG, "result_path");

	double time = 0, dist = 0;
	spherical::Point pointLast, pointFinal;


	fprintf(filpekG, "{ \"type\": \"Feature\",\n");
	fprintf(filpekG, "\"geometry\": { \"type\": \"MultiLineString\",\n"); 
	fprintf(filpekG, "\"coordinates\": [ [ ");
	dist = 0;
	time = 0;
	for (i = 0; i < model.solutionPath.nPoints; i++) {
		if (i > 0) {
			fprintf(filpekG, ",\n");
			dist += model.solutionPath.point[i-1].distanceTo(model.solutionPath.point[i]) / 1000.0;
		}
		fprintf(filpekG, "[ %lf, %lf ]", model.solutionPath.point[i].longitude().degrees(),
			model.solutionPath.point[i].latitude().degrees());
	}
	fprintf(filpekG, " ] ] },\n");
	fprintf(filpekG, "\"properties\": {\n");
	time = dist / (model.params.knots_to_km * model.params.shipSpeed_average);

	char* startTime, * endTime;
	startTime = (char*)malloc(256 * sizeof(char));
	endTime = (char*)malloc(256 * sizeof(char));

	fixReadableDate(tmBas, startTime);
	tmBas.tm_hour += time;
	mktime(&tmBas);
	fixReadableDate(tmBas, endTime);

	fprintf(filpekG, "\"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n", startTime, endTime);
	fprintf(filpekG, ", \"averageSpeed_knots\": %.3lf, \"totalDistance_km\": %.3lf, \"totalTime_h\": %.3lf}}\n", model.params.shipSpeed_average,
		dist, time);

	fprintf(filpekG, "]\n}");
	fclose(filpekG);

	return 0;
}

/*
int adderaPartArcWeatherShp(SHPHandle hSHPHandle, int nSHPType, double* x, double* y, double* z, int nPkter) {
	SHPObject* psShape;
	psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
		nPkter, x, y, z, NULL); //  m);
	SHPWriteObject(hSHPHandle, -1, psShape);
	SHPDestroyObject(psShape);

	return 0;
}

int adderaPartArcWeatherDbf(DBFHandle hDBF, int iPos, int arcNr, int tidPos, struct tm tmBas, char* namn)
{

	DBFWriteIntegerAttribute(hDBF, iPos, 0, arcNr);
	DBFWriteDoubleAttribute(hDBF, iPos, 1, model.arc[arcNr].distance);
	DBFWriteDoubleAttribute(hDBF, iPos, 2, model.arc[arcNr].fromTime + tidPos);
	tmBas.tm_hour += model.arc[arcNr].fromTime + tidPos;
	mktime(&tmBas);
	sprintf(namn, "%d", tmBas.tm_year + 1900);
	if (tmBas.tm_mon + 1 < 10)
		sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
	else
		sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
	if (tmBas.tm_mday < 10)
		sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
	else
		sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
	if (tmBas.tm_hour < 10)
		sprintf(namn, "%sT0%d", namn, tmBas.tm_hour);
	else
		sprintf(namn, "%sT%d", namn, tmBas.tm_hour);
	if (tmBas.tm_min < 10)
		sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
	else
		sprintf(namn, "%s:%d", namn, tmBas.tm_min);
	if (tmBas.tm_sec < 10)
		sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
	else
		sprintf(namn, "%s:%d", namn, tmBas.tm_sec);
	DBFWriteStringAttribute(hDBF, iPos, 3, namn);
	tmBas.tm_hour -= model.arc[arcNr].fromTime + tidPos;


	return 0;
}
*/

int writeSolutionPathForWeatherToGeojson(char* pszFilename)
{
	errlog("ERROR! Implement writeSolutionPathForWeatherToGeojson\n");
	/*
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	int nSHPType, nAllocPkter, i, iPos, levTo, nPkter, cNr;
	int arcNr, lev1, lev2, pointNr1, pointNr2, timeInt, * nSpeedSettingUsed, nSpeedChanges = 0;
	int tid, tidPos, i1;
	double* x, * y, * z, lat, lon, dist, distInt, distNu, distToNext, bearing;

	spherical::Point p1, p2, pEnd;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	DBFHandle	hDBF;

	nSHPType = SHPT_ARCZ;
	hSHPHandle = SHPCreate(pszFilename, nSHPType);

	hDBF = DBFCreate(pszFilename);
	if (hDBF == NULL)
	{
		printf("DBFCreate(%s) failed.\n", pszFilename);
		exit(2);
	}
	struct tm tmBas;
	time_t rawtime, timeNu;
	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = 0;
	tmBas.tm_sec = 0;
	//timeNu = mktime(&tmBas);


	int nAlloc = model.network.nMaxNodesInPath, prefPath;

	x = (double*)malloc(nAlloc * sizeof(double));
	y = (double*)malloc(nAlloc * sizeof(double));
	z = (double*)malloc(nAlloc * sizeof(double));

	if (DBFAddField(hDBF, "arcPos", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "distance", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "timeDbl", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "time", FTString, 20, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);

	iPos = 0;
	for (int i0 = 0; i0 < model.nBVArcs - 1; i0++)
	{
		// kopiera delen av punktfoljden som anvands, dess xyz
		arcNr = model.BVArc[i0];
		lev1 = model.arc[arcNr].fromLevel;
		lev2 = model.arc[arcNr].toLevel;
		pointNr1 = model.arc[arcNr].fromPointNr;
		pointNr2 = model.arc[arcNr].toPointNr;
		dist = model.arc[arcNr].distance;
		tid = model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
		tidPos = 0;
		distInt = dist * 1000 / tid;
		distNu = 0;
		if (lev1 < 0 && lev2 < 0) {
			nPkter = 0;
			for (i = 0; i < model.network.channel[-lev1 - 1].nPoints; i++) {
				if (i > 0) {
					distToNext = p1.distanceTo(model.network.channel[-lev1 - 1].point[i]);
					for (i1 = 0; i1 < 100; i1++) {
						if (distNu + distToNext >= distInt) {
							bearing = p1.bearingTo(model.network.channel[-lev1 - 1].point[i]);
							p2 = p1.destinationPoint(distInt - distNu, bearing);
							// addera bage p1 till p2
							y[nPkter] = p2.latitude().degrees();
							x[nPkter] = p2.longitude().degrees();
							z[nPkter] = 0;
							nPkter++;
							adderaPartArcWeatherShp(hSHPHandle, nSHPType, x, y, z, nPkter);
							adderaPartArcWeatherDbf(hDBF, iPos, arcNr, tidPos, tmBas, namn);
							iPos++;
							tidPos++;
							distToNext += distNu - distInt;
							distNu = 0;
							nPkter = 0;
							y[nPkter] = p2.latitude().degrees();
							x[nPkter] = p2.longitude().degrees();
							z[nPkter] = 0;
							nPkter++;
							p1 = p2;
						}
						else {
							distNu += distToNext;
							break;
						}
					}
				}
				p1 = model.network.channel[-lev1 - 1].point[i];
				y[nPkter] = p1.latitude().degrees();
				x[nPkter] = p1.longitude().degrees();
				z[nPkter] = 0;
				nPkter++;
			}
			if (distNu > 10) {
				adderaPartArcWeatherShp(hSHPHandle, nSHPType, x, y, z, nPkter);
				adderaPartArcWeatherDbf(hDBF, iPos, arcNr, tidPos, tmBas, namn);
				iPos++;
			}
		}
		else {
			prefPath = 0;
			if (lev1 >= 0 && lev2 >= 0) {
				if (pointNr1 == model.params.preferredPathOrtoPos[lev1] &&
					pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1) {
					y[0] = model.network.physicalLev[lev1].point[pointNr1].latitude().degrees();
					x[0] = model.network.physicalLev[lev1].point[pointNr1].longitude().degrees();
					nPkter = 1;
					p1 = model.network.physicalLev[lev1].point[pointNr1];
					for (int i3 = 0; i3 < model.network.physicalLev[lev1].npreferredPathPoints; i3++) {
						distToNext = p1.distanceTo(model.network.physicalLev[lev1].preferredPathPoint[i3]);
						for (i1 = 0; i1 < 100; i1++) {
							if (distNu + distToNext >= distInt) {
								bearing = p1.bearingTo(model.network.physicalLev[lev1].preferredPathPoint[i3]);
								p2 = p1.destinationPoint(distInt - distNu, bearing);
								// addera bage p1 till p2
								y[nPkter] = p2.latitude().degrees();
								x[nPkter] = p2.longitude().degrees();
								z[nPkter] = 0;
								nPkter++;
								adderaPartArcWeatherShp(hSHPHandle, nSHPType, x, y, z, nPkter);
								adderaPartArcWeatherDbf(hDBF, iPos, arcNr, tidPos, tmBas, namn);
								iPos++;
								tidPos++;
								distToNext += distNu - distInt;
								distNu = 0;
								nPkter = 0;
								y[nPkter] = p2.latitude().degrees();
								x[nPkter] = p2.longitude().degrees();
								z[nPkter] = 0;
								nPkter++;
								p1 = p2;
							}
							else {
								distNu += distToNext;
								break;
							}
						}
						p1 = model.network.physicalLev[lev1].preferredPathPoint[i3];
						y[nPkter] = p1.latitude().degrees();
						x[nPkter] = p1.longitude().degrees();
						z[nPkter] = 0;
						nPkter++;
					}
					if (distNu > 10) {
						adderaPartArcWeatherShp(hSHPHandle, nSHPType, x, y, z, nPkter);
						adderaPartArcWeatherDbf(hDBF, iPos, arcNr, tidPos, tmBas, namn);
						iPos++;
					}
					prefPath = 1;
				}
			}
			if (prefPath == 0) {
				if (lev1 >= 0) {
					p1 = model.network.physicalLev[lev1].point[pointNr1];
				}
				else {
					p1 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
				}
				z[0] = 0;
				if (lev2 >= 0) {
					pEnd = model.network.physicalLev[lev2].point[pointNr2];
				}
				else {
					pEnd = model.network.channel[-lev2 - 1].point[0];
				}
				z[1] = 0;
				nPkter = 0;
				y[nPkter] = p1.latitude().degrees();
				x[nPkter] = p1.longitude().degrees();
				z[nPkter] = 0;
				nPkter++;

				distToNext = p1.distanceTo(pEnd);
				for (i1 = 0; i1 < 100; i1++) {
					if (distNu + distToNext >= distInt) {
						bearing = p1.bearingTo(pEnd);
						p2 = p1.destinationPoint(distInt - distNu, bearing);
						// addera bage p1 till p2
						y[nPkter] = p2.latitude().degrees();
						x[nPkter] = p2.longitude().degrees();
						z[nPkter] = 0;
						nPkter++;
						adderaPartArcWeatherShp(hSHPHandle, nSHPType, x, y, z, nPkter);
						adderaPartArcWeatherDbf(hDBF, iPos, arcNr, tidPos, tmBas, namn);
						iPos++;
						tidPos++;
						distToNext += distNu - distInt;
						distNu = 0;
						nPkter = 0;
						y[nPkter] = p2.latitude().degrees();
						x[nPkter] = p2.longitude().degrees();
						z[nPkter] = 0;
						nPkter++;
						p1 = p2;
					}
					else {
						distNu += distToNext;
						break;
					}
				}
				p1 = pEnd;
				y[nPkter] = p1.latitude().degrees();
				x[nPkter] = p1.longitude().degrees();
				z[nPkter] = 0;
				nPkter++;
			}
			if (distNu > 10) {
				adderaPartArcWeatherShp(hSHPHandle, nSHPType, x, y, z, nPkter);
				adderaPartArcWeatherDbf(hDBF, iPos, arcNr, tidPos, tmBas, namn);
				iPos++;
			}
		}
	}

	SHPClose(hSHPHandle);
	DBFClose(hDBF);

	write_copyAtoB(pszFilename, (char*)"prj", (char*)"wgs84Def.prj", (char*)"w");
	*/

	return 0;
}


int writeNodeWeatherDataToGeojson(char* pszFilename)
{
	errlog("ERROR! Implement writeNodeWeatherDataToGeojson\n");
	/*
	SHPHandle	hSHPHandle;
	SHPObject* psShape;
	int nSHPType, i, iPos, i1, nNivaer, level1, pos1, i2, t, endT;
	int minFromStart;
	double* x, * y, * z, distIntervall, distNu, stormVarde;
	double uWind, vWind, windDirection, windSpeed, crossWind, headWind, tailWind;
	double uCurrent, vCurrent, currentDirection, currentSpeed, crossCurrent, headCurrent, tailCurrent;
	double waveHeight, wavePeriod;
	char* namn;
	spherical::Point p0, pEnd;
	struct tm tmBas;
	time_t rawtime, timeNu;
	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = 0;
	tmBas.tm_sec = 0;
	//timeNu = mktime(&tmBas);


	namn = (char*)malloc(256 * sizeof(char));

	nSHPType = SHPT_POINTZ;

	hSHPHandle = SHPCreate(pszFilename, nSHPType);
	DBFHandle	hDBF;
	hDBF = DBFCreate(pszFilename);
	if (hDBF == NULL)
	{
		printf("DBFCreate(%s) failed.\n", pszFilename);
		exit(2);
	}
	if (DBFAddField(hDBF, "pointNr", FTInteger, 8, 0) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "time", FTString, 20, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "hurricane", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "windSpeed", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "crossWind", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "headWind", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "tailWind", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "currentSpeed", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "crossCurrent", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "headCurrent", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "tailCurrent", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "waveHeight", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);
	if (DBFAddField(hDBF, "wavePeriod", FTDouble, 15, 3) == -1) errlog("Failed to add field to shapefile at row %d\n", __LINE__);

	int nAlloc = model.network.nMaxNodesInPath, prefPath;

	x = (double*)malloc(nAlloc * sizeof(double));
	y = (double*)malloc(nAlloc * sizeof(double));
	z = (double*)malloc(nAlloc * sizeof(double));
	z[0] = 0;

	endT = 0;
	for (i = 0; i < model.network.physicalLev[model.network.nPhysicalLevels - 1].nPoints; i++) {
		for (i1 = 0; i1 < model.network.physicalLev[model.network.nPhysicalLevels - 1].nTimeIntervals[i]; i1++) {
			if (endT < model.network.physicalLev[model.network.nPhysicalLevels - 1].timeInterval[i][i1])
				endT = model.network.physicalLev[model.network.nPhysicalLevels - 1].timeInterval[i][i1];
		}
	}
	iPos = 0;
	for (i = 1; i < model.network.nPhysicalLevels; i++) {
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1 += 3) {
			if (model.network.physicalLev[i].minDistPrevNode[i1] < 0.9e10) {
				//nNivaer = model.network.physicalLev[i].minDistPrevNode[i1] / 100.0;// 50.0;
				//if (nNivaer < 1)
				//	nNivaer = 1;
				//distIntervall = model.network.physicalLev[i].minDistPrevNode[i1] / nNivaer;
				distNu = 0;
				level1 = model.network.physicalLev[i].minDistPrevNode_level[i1];
				pos1 = model.network.physicalLev[i].minDistPrevNode_pos[i1];
				p0 = model.network.physicalLev[level1].point[pos1];
				pEnd = model.network.physicalLev[i].point[i1];
				calcWeatherPosAlongArc(p0, pEnd);
				for (i2 = 0; i2 < model.weatherFunctions.nCheckPoints - 1; i2 += 3) {
					for (t = 0; t < endT; t += 3) {
						if (iPos >= 71796)
							iPos = iPos;
						stormVarde = getStormValue(t, model.weatherFunctions.point[i2]);

						uWind = getVariableValue(0, i2, t);
						vWind = getVariableValue(1, i2, t);
						if (uWind < 1000 && vWind < 1000) {
							//		errlog("checkP %d latPos %d lonPos %d timePos_uWind %d uWind %.2lf\n", i, latPos,
							//			lonPos, timePos_uWind, uWind);
							//		errlog("checkP %d latPos %d lonPos %d timePos_vWind %d vWind %.2lf\n", i, latPos,
							//			lonPos, timePos_vWind, vWind);
							windDirection = atan2(vWind, uWind);
							windSpeed = sqrt(pow(uWind, 2) + pow(vWind, 2));
							crossWind = abs(sin(windDirection - model.weatherFunctions.vesselBearing[i2] * M_PI / 180) * windSpeed);
							headWind = cos(windDirection - model.weatherFunctions.vesselBearing[i2] * M_PI / 180) * windSpeed;
							if (headWind < 0) {
								tailWind = -headWind;
								headWind = 0;
							}
							else {
								tailWind = 0;
							}
						}
						else {
							tailWind = 0;
							headWind = 0;
							crossWind = 0;
						}
						uCurrent = getVariableValue(2, i2, t);
						vCurrent = getVariableValue(3, i2, t);
						//		errlog("checkP %d latPos %d lonPos %d timePos_uCurrent %d uCurrent %.2lf\n", i, latPos,
						//			lonPos, timePos_uCurrent, uCurrent);
						//		errlog("checkP %d latPos %d lonPos %d timePos_vCurrent %d vCurrent %.2lf\n", i, latPos,
						//			lonPos, timePos_vCurrent, vCurrent);
						if (uCurrent < 1000 && vCurrent < 1000) {
							currentDirection = atan2(vCurrent, uCurrent);
							currentSpeed = sqrt(pow(uCurrent, 2) + pow(vCurrent, 2));
							crossCurrent = abs(sin(currentDirection - model.weatherFunctions.vesselBearing[i2] * M_PI / 180) * currentSpeed);
							headCurrent = cos(currentDirection - model.weatherFunctions.vesselBearing[i2] * M_PI / 180) * currentSpeed;
							if (headCurrent < 0) {
								tailCurrent = -headCurrent;
								headCurrent = 0;
							}
							else {
								tailCurrent = 0;
							}
						}
						else {
							headCurrent = 0;
							tailCurrent = 0;
							crossCurrent = 0;
							currentSpeed = 0;
						}

						waveHeight = getVariableValue(4, i2, t);
						if (waveHeight > 100)
							waveHeight = 0;
						wavePeriod = getVariableValue(5, i2, t);
						if (wavePeriod > 1000)
							wavePeriod = 0;


						y[0] = model.weatherFunctions.point[i2].latitude().degrees();
						x[0] = model.weatherFunctions.point[i2].longitude().degrees();

						psShape = SHPCreateObject(nSHPType, -1, 0, NULL, NULL,
							1, x, y, z, NULL); //  m);
						SHPWriteObject(hSHPHandle, -1, psShape);
						SHPDestroyObject(psShape);

						DBFWriteIntegerAttribute(hDBF, iPos, 0, iPos);

						minFromStart = t * 60;
						tmBas.tm_min += minFromStart;
						mktime(&tmBas);
						sprintf(namn, "%d", tmBas.tm_year + 1900);
						if (tmBas.tm_mon + 1 < 10)
							sprintf(namn, "%s-0%d", namn, tmBas.tm_mon + 1);
						else
							sprintf(namn, "%s-%d", namn, tmBas.tm_mon + 1);
						if (tmBas.tm_mday < 10)
							sprintf(namn, "%s-0%d", namn, tmBas.tm_mday);
						else
							sprintf(namn, "%s-%d", namn, tmBas.tm_mday);
						if (tmBas.tm_hour < 10)
							sprintf(namn, "%sT0%d", namn, tmBas.tm_hour);
						else
							sprintf(namn, "%sT%d", namn, tmBas.tm_hour);
						if (tmBas.tm_min < 10)
							sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
						else
							sprintf(namn, "%s:%d", namn, tmBas.tm_min);
						if (tmBas.tm_sec < 10)
							sprintf(namn, "%s:0%d", namn, tmBas.tm_sec);
						else
							sprintf(namn, "%s:%d", namn, tmBas.tm_sec);
						DBFWriteStringAttribute(hDBF, iPos, 1, namn);
						tmBas.tm_min -= minFromStart;

						DBFWriteDoubleAttribute(hDBF, iPos, 2, stormVarde);
						DBFWriteDoubleAttribute(hDBF, iPos, 3, windSpeed);
						DBFWriteDoubleAttribute(hDBF, iPos, 4, crossWind);
						DBFWriteDoubleAttribute(hDBF, iPos, 5, headWind);
						DBFWriteDoubleAttribute(hDBF, iPos, 6, tailWind);
						DBFWriteDoubleAttribute(hDBF, iPos, 7, currentSpeed);
						DBFWriteDoubleAttribute(hDBF, iPos, 8, crossCurrent);
						DBFWriteDoubleAttribute(hDBF, iPos, 9, headCurrent);
						DBFWriteDoubleAttribute(hDBF, iPos, 10, tailCurrent);
						DBFWriteDoubleAttribute(hDBF, iPos, 11, waveHeight);
						DBFWriteDoubleAttribute(hDBF, iPos, 12, wavePeriod);

						iPos++;
					}



				}
			}
		}
	}

	SHPClose(hSHPHandle);
	DBFClose(hDBF);

	write_copyAtoB(pszFilename, (char*)"prj", (char*)"wgs84Def.prj", (char*)"w");
	*/

	return 0;
}

/*
int loadpreferredPath()
{
	DBFHandle	hDBF;
	SHPHandle	hSHP;
	int iRecord, j, iPart;

	model.preferredPath.nPoints = 0;

	char* namn2;
	namn2 = (char*)malloc(256 * sizeof(char));
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.preferredPath.c_str());
	//	hSHP = SHPOpen("path0.shp", "rb");
	//hSHP = SHPOpen(model.params.preferredPath.c_str(), "rb");
	hSHP = SHPOpen(namn2, "rb");
	if (hSHP == NULL)
	{
		printf("SHPOpen(%s,\"r\") failed.\n", model.params.preferredPath.c_str());
		errlog("ERROR! Could not open %s.shp. I quit!\n", model.params.preferredPath.c_str());
		exit(2);
	}
	//hDBF = DBFOpen(model.params.preferredPath.c_str(), "rb");
	hDBF = DBFOpen(namn2, "rb");
	if (hDBF == NULL)
	{
		printf("DBFOpen(%s,\"r\") failed.\n", model.params.preferredPath.c_str());
		errlog("ERROR! Could not open %s.dbf. I quit!\n", model.params.preferredPath.c_str());
		exit(2);
	}

	if (DBFGetFieldCount(hDBF) == 0)
	{
		printf("There are no fields in this table!\n");
		exit(3);
	}

	int nRecords = DBFGetRecordCount(hDBF);
	int nVertices;
	double xVal;

	for (iRecord = 0; iRecord < nRecords; iRecord++)
	{
		SHPObject* psShape;

		psShape = SHPReadObject(hSHP, iRecord);

		nVertices = psShape->nVertices;
		if (model.preferredPath.nPoints == 0)
			model.preferredPath.point = (spherical::Point*)malloc(nVertices * sizeof(spherical::Point));
		else
			model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point,
				(model.preferredPath.nPoints + nVertices) * sizeof(spherical::Point));
		if (psShape == NULL)
		{
			errlog("ERROR! Unable to read shape %d, terminating object reading.\n",
				iRecord);
			break;
		}

		//			errlog("%d %s\n", i, SHPTypeName(psShape->nSHPType));

		for (j = 0; j < nVertices; j++)
		{
			const char* pszPartType = "";

			if (j == 0 && psShape->nParts > 0)
				pszPartType = SHPPartTypeName(psShape->panPartType[0]);
			xVal = psShape->padfX[j];
			if (xVal > 180)
				xVal -= 360;
			if (xVal < -180)
				xVal += 360;
			model.preferredPath.point[model.preferredPath.nPoints] = spherical::Point(psShape->padfY[j], xVal);
			(model.preferredPath.nPoints)++;

		}
		SHPDestroyObject(psShape);

	}
	DBFClose(hDBF);
	SHPClose(hSHP);


	return 0;
}
*/

int loadChannels()
{
	model.network.nChannels = 0;
	model.network.nMaxNodesInPath = 2;

/*
	DBFHandle	hDBF;
	SHPHandle	hSHP;
	int iRecord, j, iPart;



	if (model.params.channelsName == "") {
		errlog("OBS! No channels are given.\n");
		return 0;
	}

	char* namn2;
	namn2 = (char*)malloc(256 * sizeof(char));
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.channelsName.c_str());
	//hSHP = SHPOpen(model.params.channelsName.c_str(), "rb");
	hSHP = SHPOpen(namn2, "rb");
	if (hSHP == NULL)
	{
		printf("SHPOpen(%s,\"r\") failed.\n", model.params.channelsName.c_str());
		errlog("ERROR! Could not open %s.shp. I quit!\n", model.params.channelsName.c_str());
		exit(2);
	}
	//hDBF = DBFOpen(model.params.channelsName.c_str(), "rb");
	hDBF = DBFOpen(namn2, "rb");
	if (hDBF == NULL)
	{
		printf("DBFOpen(%s,\"r\") failed.\n", model.params.channelsName.c_str());
		errlog("ERROR! Could not open %s.dbf. I quit!\n", model.params.channelsName.c_str());
		exit(2);
	}

	if (DBFGetFieldCount(hDBF) == 0)
	{
		printf("There are no fields in this table!\n");
		exit(3);
	}

	int i, colExtraCost = -1, colExtraTime = -1;
	int		nWidth, nDecimals;
	char	szTitle[12];
	DBFFieldType extraCost_eType, extraTime_eType;
	for (i = 0; i < DBFGetFieldCount(hDBF); i++)
	{
		DBFFieldType	eType;
		// const char* pszTypeName;
		char chNativeType;

		chNativeType = DBFGetNativeFieldType(hDBF, i);

		eType = DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);
		if (strcmp(szTitle, "extraCost") == 0) {
			if (eType == FTInvalid)
				errlog("ERROR! Invalid type for field %s in file %s, I don't use it.\n", szTitle, model.params.channelsName.c_str());
			else {
				colExtraCost = i;
				extraCost_eType = eType;
			}
		}
		if (strcmp(szTitle, "extraTime") == 0) {
			if (eType == FTInvalid)
				errlog("ERROR! Invalid type for field %s in file %s, I don't use it.\n", szTitle, model.params.channelsName.c_str());
			else {
				colExtraTime = i;
				extraTime_eType = eType;
			}
		}
	}

	int nRecords = DBFGetRecordCount(hDBF);
	int nVertices;
	double distance, xVal;

	model.network.channel = (strNodeSeq*)malloc(nRecords * sizeof(strNodeSeq));

	for (iRecord = 0; iRecord < nRecords; iRecord++)
	{
		SHPObject* psShape;

		psShape = SHPReadObject(hSHP, iRecord);

		nVertices = psShape->nVertices;
		model.network.channel[iRecord].point = (spherical::Point*)malloc(nVertices * sizeof(spherical::Point));
		model.network.channel[iRecord].allowedPoint = (int*)malloc(nVertices * sizeof(int));
		model.network.channel[iRecord].usedPoint = (int*)malloc(nVertices * sizeof(int));
		model.network.channel[iRecord].distanceFromStart = (double*)malloc(nVertices * sizeof(double));
		if (colExtraCost == -1) {
			model.network.channel[iRecord].extraCostChannel = 0;
		}
		else {
			if (extraCost_eType == FTString)
				model.network.channel[iRecord].extraCostChannel = char_to_doubleConst(DBFReadStringAttribute(hDBF, iRecord, colExtraCost));
			else if (extraCost_eType == FTInteger)
				model.network.channel[iRecord].extraCostChannel = DBFReadIntegerAttribute(hDBF, iRecord, colExtraCost);
			else if (extraCost_eType == FTDouble)
				model.network.channel[iRecord].extraCostChannel = DBFReadDoubleAttribute(hDBF, iRecord, colExtraCost);
		}
		if (colExtraTime == -1) {
			model.network.channel[iRecord].extraTimeChannel = 0;
		}
		else {
			if (extraCost_eType == FTString)
				model.network.channel[iRecord].extraTimeChannel = char_to_doubleConst(DBFReadStringAttribute(hDBF, iRecord, colExtraCost));
			else if (extraCost_eType == FTInteger)
				model.network.channel[iRecord].extraTimeChannel = DBFReadIntegerAttribute(hDBF, iRecord, colExtraCost);
			else if (extraCost_eType == FTDouble)
				model.network.channel[iRecord].extraTimeChannel = DBFReadDoubleAttribute(hDBF, iRecord, colExtraCost);
		}

		if (psShape == NULL)
		{
			errlog("ERROR! Unable to read shape %d, terminating object reading.\n",
				iRecord);
			break;
		}

		//			errlog("%d %s\n", i, SHPTypeName(psShape->nSHPType));

		for (j = 0; j < nVertices; j++)
		{
			const char* pszPartType = "";

			if (j == 0 && psShape->nParts > 0)
				pszPartType = SHPPartTypeName(psShape->panPartType[0]);

			xVal = psShape->padfX[j];
			if (xVal > 180)
				xVal -= 360;
			model.network.channel[iRecord].point[j] = spherical::Point(psShape->padfY[j], xVal);
			model.network.channel[iRecord].allowedPoint[j] = 1;
			if (j == 0)
				model.network.channel[iRecord].distanceFromStart[j] = 0;
			else
				model.network.channel[iRecord].distanceFromStart[j] = model.network.channel[iRecord].distanceFromStart[j - 1] +
				model.network.channel[iRecord].point[j - 1].distanceTo(model.network.channel[iRecord].point[j]) / 1000.0;
		}
		model.network.channel[iRecord].nPoints = j;
		model.network.channel[iRecord].nOutNodes = 0;
		if (model.network.nMaxNodesInPath < j)
			model.network.nMaxNodesInPath = j;
		SHPDestroyObject(psShape);

	}
	DBFClose(hDBF);
	SHPClose(hSHP);

	model.network.nChannels = nRecords;
	
	model.network.usedChannel = (int*)malloc(nRecords * sizeof(int));
	*/

	return 0;
}
/*
int callVectorTest()
{

	Vector vector((const char*)"path0.geojson");
	//Vector vector((const char*)"path0.shp");

	return 0;
}
*/

int callJsonTest()
{
	json j2 = {
  {"pi", 3.141},
  {"happy", true},
  {"name", "Niels"},
  {"nothing", nullptr},
  {"answer", {
	{"everything", 42}
  }},
  {"list", {1, 0, 2}},
  {"object", {
	{"currency", "USD"},
	{"value", 42.99}
  }}
	};

	std::cout << j2.dump(4) << std::endl;

	for (json::iterator it = j2.begin(); it != j2.end(); ++it) {
		std::cout << *it << '\n';
	}
	std::cout << "size " << j2.size() << '\n';
	std::cout << "test1 " << j2["pi"];
	std::cout << "test2 " << j2["pi2"];



	std::ofstream fil("test.json");
	fil << j2;

	return 0;
}

int loadParams_theRestOld(strParams* params)
{
	int i, vardeInt, i0;

	std::ifstream fil;
	char* namn;

	params->knots_to_km = 1.852;
	params->shipSpeed_average = 22;
	//params->mapPhysicalFileName = std::string();
	params->mapPhysicalAFileName = std::string();
	params->mapPhysicalBFileName = std::string();
	//params->physicalMapRasterPos = 0;
	//params->mapFuelGeographyFileName = std::string();
	params->mapFuelGeographyAFileName = std::string();
	params->mapFuelGeographyBFileName = std::string();

	model.weather = NULL;
	params->preferredPath = "";
	params->corridorPath = "";
	params->preferredPath_followExactOK = 1;
	params->channelsName = "";
	params->readSolPathFile = "";
	params->solutionFileName = "solution";
	params->nHours_changeCourseInterval = 12;
	params->ortoDist_nPointsPerHour = 2;
	params->maxDiffTimeFastSlow = 48;
	params->nPkterOrto = 11;
	params->epsilon = 0.000001;
	params->save_weatherNodes = 0;
	params->max_changeDirection = 1;
	//params->lengthIntervall = 1;
	//params->dist_checkOKroute = 1;
	//params->variableFileName = "variablesInfo.json";
	model.params.useStandardWeather = 0;
	params->physicalMap_noDataValue = 9999;
	params->speedSettings_addOnlyCheapestArcs = 1;
	params->runAlt = 0;
	params->startDelay_h = 0;

	//params->storm_windUBD = 64;
	//params->storm_1dist_ahead = 200;
	//params->storm_2dist_ahead = 500;
	//params->storm_1costInside_ahead = 1e10;
	//params->storm_1cost_ahead = 100;
	//params->storm_2cost_ahead = 1;
	//params->storm_1dist_behind = 120;
	//params->storm_2dist_behind = 200;
	//params->storm_1costInside_behind = 1e10;
	//params->storm_1cost_behind = 100;
	//params->storm_2cost_behind = 1;

	params->startYear = 2018;
	params->startMonth_nr = 9; // sep
	params->startDay_nr = 1;
	params->startHour = 0;
	params->maxDeviationPreferred_km = 500;

	params->minSpeedDiffWeatherFactor = -10;
	params->maxSpeedDiffWeatherFactor = 8;
	params->minSpeedDiffCurrent = -10;
	params->maxSpeedDiffCurrent = 10;
	errlog("OBS! minSpeedDiffWeatherFactor set to %.3lf\n", params->minSpeedDiffWeatherFactor);
	errlog("OBS! maxSpeedDiffWeatherFactor set to %.3lf\n", params->maxSpeedDiffWeatherFactor);
	errlog("OBS! minSpeedDiffCurrent set to %.3lf\n", params->minSpeedDiffCurrent);
	errlog("OBS! maxSpeedDiffCurrent set to %.3lf\n", params->maxSpeedDiffCurrent);

	params->longestRouteDays_history = -1;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/file_params.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(exists_test3(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		exit(0);
	}
	printf("opens %s\n", namn);
	fil.open(namn);
	
	json data, dataSpeed, dataVar, dataIt;
	try{
		fil >> data;
	}
	catch(...){
		errlog("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		exitKontrollerat(__LINE__);
	}

	if (!data["knots_to_km"].is_null())
		params->knots_to_km = data["knots_to_km"];
	//if (!data["mapPhysicalFileName"].is_null()) {
	//	params->mapPhysicalFileName = data["mapPhysicalFileName"];
	//}
	if (!data["mapPhysicalBFileName"].is_null()) {
		params->mapPhysicalBFileName = data["mapPhysicalBFileName"];
	}
	if (!data["mapPhysicalAFileName"].is_null()) {
		params->mapPhysicalAFileName = data["mapPhysicalAFileName"];
	}
	//if (!data["mapPhysicalRasterPos"].is_null()) {
	//	params->physicalMapRasterPos = data["mapPhysicalRasterPos"];
	//}
	if (!data["physicalMap_noDataValue"].is_null())
		params->physicalMap_noDataValue = data["physicalMap_noDataValue"];
	//if (!data["mapFuelGeographyFileName"].is_null()) {
	//	params->mapFuelGeographyFileName = data["mapFuelGeographyFileName"];
	//}
	if (!data["mapFuelGeographyAFileName"].is_null()) {
		params->mapFuelGeographyAFileName = data["mapFuelGeographyAFileName"];
	}
	if (!data["mapFuelGeographyBFileName"].is_null()) {
		params->mapFuelGeographyBFileName = data["mapFuelGeographyBFileName"];
	}

	//if (!data["speedSettings_addOnlyCheapestArcs"].is_null()) {
	//	errlog("ERROR! Not using speedSettings_addOnlyCheapestArcs\n");
	//	// params->speedSettings_addOnlyCheapestArcs = data["speedSettings_addOnlyCheapestArcs"];
	//}
	if (!data["readSolPathFile"].is_null()) {
		params->readSolPathFile = data["readSolPathFile"];
	}
	if (!data["epsilon"].is_null())
		params->epsilon = data["epsilon"];
	if (!data["save_weatherNodes"].is_null())
		params->save_weatherNodes = data["save_weatherNodes"];
	//if (!data["lengthIntervall"].is_null())
	//	params->lengthIntervall = data["lengthIntervall"];
	//if (!data["dist_checkOKroute"].is_null())
	//	params->dist_checkOKroute = data["dist_checkOKroute"];
	//if (!data["variableFileName"].is_null())
	//	params->variableFileName = data["variableFileName"];
	if (!data["useStandardWeather"].is_null())
		params->useStandardWeather = data["useStandardWeather"];

	if (!data["nHours_changeCourseInterval"].is_null())
		params->nHours_changeCourseInterval = data["nHours_changeCourseInterval"];
	if (!data["maxDiffTimeFastSlow_h"].is_null())
		params->maxDiffTimeFastSlow = data["maxDiffTimeFastSlow_h"];
	if (!data["nPkterOrto"].is_null()) {
		params->nPkterOrto = data["nPkterOrto"];
		vardeInt = (int)(params->nPkterOrto / 2.0);
		if (vardeInt * 2 == params->nPkterOrto)
			(params->nPkterOrto)++; // must be an uneven number of orthogonal points
	}
	if (!data["nMinutesBetweenPkterOrto"].is_null()) {
		if (data["nMinutesBetweenPkterOrto"] < 1)
			params->ortoDist_nPointsPerHour = 60.0;
		else
			params->ortoDist_nPointsPerHour = 60.0 / (double)(data["nMinutesBetweenPkterOrto"]);
	}

	if (!data["max_changeDirection"].is_null())
		params->max_changeDirection = data["max_changeDirection"];
	if (!data["longestRouteDays_history"].is_null())
		params->longestRouteDays_history = data["longestRouteDays_history"];



	//checkMinnesAnvandning(__LINE__);
	if (!data["timeZones"].is_null()) {
		dataVar = data["timeZones"];
		params->timeZone = (strTimeZones*)malloc((int)dataVar.size() * sizeof(strTimeZones));
		//printf("\n####\nalloc %d timeZones\n", (int)dataVar.size());
		i0 = 0;
		for (auto it = dataVar.begin(); it != dataVar.end(); ++it) {
			dataIt = it.value();
			if (!dataIt["zoneID"].is_null()) {
				//cout << "timezone " << dataIt["zoneID"];
				//cout << "i0a " << i0;
				std::string namn = dataIt["zoneID"];
				params->timeZone[i0].name = str_alloc_cpy(namn.c_str());
				//cout << "timezone " << dataIt["zoneID"];
			}
			else {
				errlog("ERROR! no zoneID for a given timeZone in file_params.json, I skip this one\n");
				continue;
			}
			if (!dataIt["nHoursDiff_UTC"].is_null())
				params->timeZone[i0].nHoursDiff = dataIt["nHoursDiff_UTC"];
			else {
				errlog("ERROR! no nHoursDiff_UTC given for timeZone %s in file_params.json, I set it to 0\n", params->timeZone[i0].name);
				params->timeZone[i0].nHoursDiff = 0;
			}
			if (!dataIt["nMinutesDiff_UTC"].is_null())
				params->timeZone[i0].nMinutesDiff = dataIt["nMinutesDiff_UTC"];
			else {
				errlog("ERROR! no nMinutesDiff_UTC given for timeZone %s in file_params.json, I set it to 0\n", params->timeZone[i0].name);
				params->timeZone[i0].nMinutesDiff = 0;
			}
			i0++;
		}
		params->nTimeZones = i0;
	}
	else {
		errlog("ERROR! No time zones defined in file_params.json, I assume all storms are given in UTC zone date/time\n");
		params->nTimeZones = 0;
	}
	//checkMinnesAnvandning(__LINE__);

	if (!data["solutionFileName"].is_null())
		params->solutionFileName = data["solutionFileName"];




	/*
	if (!data["speedSettingParameters"].is_null()) {
		// addera vektor...
		//dataSpeed = data["ship_speedSettings"];
		params->nShip_speedSettings = 1; // dataSpeed.size();
		params->ship_speedSettingID = (char**)malloc(params->nShip_speedSettings * sizeof(char*));
		i = 0;
		//for (auto it = dataSpeed.begin(); it != dataSpeed.end(); ++it) {
		//	std::string dataIt = it.value();
		//	params->ship_speedSettingID[i] = str_alloc_cpy(dataIt.c_str());
		char* speedSetting;
		speedSetting = (char*)malloc(256 * sizeof(char));
		sprintf(speedSetting, "speed_%d", (int)(params->shipSpeed_average));
		params->ship_speedSettingID[i] = str_alloc_cpy(speedSetting);
		free(speedSetting);
			i++;
		//}
		// params->ship_speedSettingNr = (int*)malloc(params->nShip_speedSettings * sizeof(int));
		json dataSetting = data["speedSettingParameters"];
		json dataIt;
		std::string namn, namnBas;
		int i2;
		model.weatherFunctions.nFunctions = 3;
		model.weatherFunctions.funcVal = (double*)malloc(model.weatherFunctions.nFunctions * sizeof(double));
		model.weatherFunctions.param = (double***)calloc(params->nShip_speedSettings, sizeof(double**));


		//errlog("ERROR?? Is function to calculate speed given in km/h?? Otherwise fix that.\n");
		// params->ship_speedSettings[i] *= params->knots_to_km;
		for (auto it = dataSetting.begin(); it != dataSetting.end(); ++it) {
			dataIt = it.value();
			namn = dataIt["settingID"];
			for (i = 0; i < params->nShip_speedSettings; i++) {
				if (strcmp(namn.c_str(), params->ship_speedSettingID[i]) == 0)
					break;
			}
			if (i >= params->nShip_speedSettings)
				continue; // this speedsetting is not used
			model.weatherFunctions.param[i] = (double**)malloc(model.weatherFunctions.nFunctions * sizeof(double*));
			for (i2 = 0; i2 < model.weatherFunctions.nFunctions; i2++)
				model.weatherFunctions.param[i][i2] = (double*)calloc(21, sizeof(double));


			for (i2 = 0; i2 < model.weatherFunctions.nFunctions; i2++) {
				if (i2 == 0)
					namnBas = "r_";
				else {
					if (i2 == 1)
						namnBas = "f_";
					else
						namnBas = "s_";
				}
				for (int i1 = 0; i1 < 21; i1++) {
					namn = namnBas + std::to_string(i1);
					if (!dataIt[namn].is_null())
						model.weatherFunctions.param[i][i2][i1] = dataIt[namn];
				}
			}
		}
		for (i = 0; i < params->nShip_speedSettings; i++) {
			if (model.weatherFunctions.param[i] == 0) {
				errlog("ERROR! ship_speedSetting %s is not given parameters for in params.json. I quit.\n",
					model.params.ship_speedSettingID[i]);
				exit(0);
			}
		}
	}
	else {
		errlog("ERROR! I could not read ship_speedSettings and/or speedSettingParameters in params.json. I quit.\n");
		exit(0);
	}
	*/

	if (!data["storms"].is_null()) {
		json dataStorms = data["storms"];
		json dataIt;
		int nAllocStorms;
		std::string namn;

		model.nStorms = 0;
		nAllocStorms = (int)dataStorms.size();
		model.storms = (strStorm*)malloc(nAllocStorms * sizeof(strStorm));
		for (auto it = dataStorms.begin(); it != dataStorms.end(); ++it) {
			dataIt = it.value();
			//if (!data["mapFuelGeographyFileName"].is_null()) {
			//	params->mapFuelGeographyFileName = data["mapFuelGeographyFileName"];
			//}
			if (!dataIt["fileName"].is_null()) {
				namn = dataIt["fileName"];
				model.storms[model.nStorms].fileName = str_alloc_cpy(namn.c_str());
				(model.nStorms)++;
			}
			else
				errlog("ERROR! Could not read file name of a storm\n");
		}
	}

	/*
	if (!data["storm_windUBD_kt"].is_null())
		params->storm_windUBD = data["storm_windUBD_kt"];
	if (!data["storm_1dist_ahead_NM"].is_null())
		params->storm_1dist_ahead = data["storm_1dist_ahead_NM"];
	if (!data["storm_2dist_ahead_NM"].is_null())
		params->storm_2dist_ahead = data["storm_2dist_ahead_NM"];
	if (!data["storm_1costInside_ahead"].is_null())
		params->storm_1costInside_ahead = data["storm_1costInside_ahead"];
	if (!data["storm_1cost_ahead"].is_null())
		params->storm_1cost_ahead = data["storm_1cost_ahead"];
	if (!data["storm_2cost_ahead"].is_null())
		params->storm_2cost_ahead = data["storm_2cost_ahead"];

	if (!data["storm_1dist_behind_NM"].is_null())
		params->storm_1dist_behind = data["storm_1dist_behind_NM"];
	if (!data["storm_2dist_behind_NM"].is_null())
		params->storm_2dist_behind = data["storm_2dist_behind_NM"];
	if (!data["storm_1costInside_behind"].is_null())
		params->storm_1costInside_behind = data["storm_1costInside_behind"];
	if (!data["storm_1cost_behind"].is_null())
		params->storm_1cost_behind = data["storm_1cost_behind"];
	if (!data["storm_2cost_behind"].is_null())
		params->storm_2cost_behind = data["storm_2cost_behind"];
		*/

	fil.close();

	if (params->longestRouteDays_history == -1) {
		params->longestRouteDays_history = 30;
		errlog("ERROR! LongestRouteDays_history not set, I use %d days\n", params->longestRouteDays_history);
	}

	double ortoDist = model.params.shipSpeed_average * 1000 / model.params.ortoDist_nPointsPerHour;
	int nPkterOrtoOld = model.params.nPkterOrto;
	model.params.nPkterOrto = (int)(2 * model.params.maxDeviationPreferred_km * 1000 / ortoDist);
	if (2 * model.params.maxDeviationPreferred_km * 1000 / ortoDist > model.params.nPkterOrto)
		(model.params.nPkterOrto)++;
	if (model.params.nPkterOrto % 2 == 0)
		(model.params.nPkterOrto)++;
	if (model.params.nPkterOrto != nPkterOrtoOld)
		errlog("OBS! Changes nPkterOrto from %d to %d beacuse of maxDeviationPreferred_km given as %.2lf km\n",
			nPkterOrtoOld, model.params.nPkterOrto, model.params.maxDeviationPreferred_km);


	errlog("objective weights:\n\ttime: %.2lf\n\tfuel: %.2lf\n\tsafety: %.2lf\n",
		params->weightTime, 1.0, params->weightSafety.base);
	return 0;
}

int loadParams_new(strParams* params)
{
	int i;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind;


	std::ifstream fil;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s", model.params.indataPathName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(exists_test3(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		exitKontrollerat(__LINE__);
	}
	printf("opens %s\n", namn);
	fil.open(namn);

	json data, dataGeo, dataGeo2, dataFeature, dataProp, dataGeo3, dataCoord;
	json dataIt, dataIt2;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu;
	double xVal, yVal;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		exitKontrollerat(__LINE__);
	}

	if (!data["maxDeviationPreferred_km"].is_null())
		params->maxDeviationPreferred_km = data["maxDeviationPreferred_km"];

	if (!data["startYear"].is_null())
		params->startYear = data["startYear"];
	if (!data["startMonth_nr"].is_null())
		params->startMonth_nr = data["startMonth_nr"];
	if (!data["startDay_nr"].is_null())
		params->startDay_nr = data["startDay_nr"];
	if (!data["startHour"].is_null())
		params->startHour = data["startHour"];
	if (!data["startMinute"].is_null())
		params->startMinute = data["startMinute"];

	if (!data["shipDraft"].is_null()) {
		params->shipDraft = data["shipDraft"];
		errlog("Loaded shipDraft %.3lf\n", params->shipDraft);
	}
	else {
		params->shipDraft = 10.03;
		errlog("ERROR! Load shipDraft, default now %.3lf\n", params->shipDraft);
	}
	if (!data["freeBoard"].is_null()) {
		params->freeBoard2 = data["freeBoard"];
		params->freeBoard2 *= params->freeBoard2;
		errlog("Loaded freeBoard %.3lf\n", sqrt(params->freeBoard2));
	}
	else {
		params->freeBoard2 = 4.39 * 4.39;
		errlog("ERROR! Load freeBoard, default now %.3lf\n", sqrt(params->freeBoard2));
	}
	if (!data["shipLength"].is_null()) {
		params->shipLength = data["shipLength"];
		errlog("Loaded shipLength %.3lf\n", params->shipLength);
	}
	else {
		params->shipLength = 177;
		errlog("ERROR! Load shipLength, default now %.3lf\n", params->shipLength);
	}

	params->UTC_secondsStart = make_gmtime(params);
	//printf("%jd seconds since the epoch began\n", (intmax_t)(params->UTC_secondsStart));
	printf("Do the planning for the dateTime %s", asctime(gmtime(&(params->UTC_secondsStart))));


	if (!data["preferredPath_followExactOK"].is_null())
		params->preferredPath_followExactOK = data["preferredPath_followExactOK"];

	if (!data["shipSpeed"].is_null())
		params->shipSpeed_average = data["shipSpeed"];

	if (!data["shipSpeedSettings"].is_null()) {
		json dataSpeed = data["shipSpeedSettings"];
		params->nShip_speedSettings = dataSpeed.size();
		model.functions.rpm = (double*)malloc(params->nShip_speedSettings * sizeof(double));
		i = 0;
		for (auto it = dataSpeed.begin(); it != dataSpeed.end(); ++it) {
			json dataSpeed2 = it.value();
			model.functions.rpm[i++] = dataSpeed2["rpm"];
		}
	}
	else {
		params->nShip_speedSettings = 1; // 15, 20, 25
		model.functions.rpm = (double*)malloc(params->nShip_speedSettings * sizeof(double));
		model.functions.rpm[0] = 80.0;
	}

	if (data["geoData"].is_null()) {
		errlog("ERROR! No geoData tag in input.json. I quit\n");
		exitKontrollerat(__LINE__);
	}
	dataGeo = data["geoData"];
	if (!dataGeo["features"].is_null()) {
		model.preferredPath.minX = 180;
		model.preferredPath.maxX = -180;

		dataGeo2 = dataGeo["features"];
		i = 0;
		for (auto it = dataGeo2.begin(); it != dataGeo2.end(); ++it) {
			dataFeature = it.value();
			if (dataFeature["properties"].is_null()) {
				printf("ERROR! No properties for a feature in geoData. I skip this one\n");
				errlog("ERROR! No properties for a feature in geoData. I skip this one\n");
				continue; // no properties exists for this one, cannot be a preferred path
			}
			dataProp = dataFeature["properties"];
			string namnNu = dataProp["type"];
			if (namnNu != "preferredPath") {
				printf("ERROR! Not the name preferredPath of type for a property in geoData. I skip this one\n");
				errlog("ERROR! Not the name preferredPath of type for a property in geoData. I skip this one\n");
				continue; //not a preferred path
			}
			dataGeo3 = dataFeature["geometry"];
			if (dataGeo3["coordinates"].is_null()) {
				errlog("ERROR! No coordinates given for the prefered path. I quit!\n");
				printf("ERROR! No coordinates given for the prefered path. I quit!\n");
				exitKontrollerat(__LINE__);
			}
			dataCoord = dataGeo3["coordinates"];

			i = 0;
			xValOld = -999999;
			yValOld = -999999;
			for (auto it = dataCoord.begin(); it != dataCoord.end(); ++it) {
				dataIt = it.value();
				nPointsNu = (int)dataIt.size();
				if (nAlloc == 0) {
					nAlloc = nPointsNu;
					model.preferredPath.point = (spherical::Point*)malloc(nAlloc * sizeof(spherical::Point));
				}
				else {
					if (nPointsTot + nPointsNu >= nAlloc) {
						nAlloc += nPointsNu;
						model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point,
							nAlloc * sizeof(spherical::Point));
					}
				}
				for (auto it2 = dataIt.begin(); it2 != dataIt.end(); ++it2) {
					dataIt2 = it2.value();
					i2 = 0;
					for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
						if (i2 == 0) {
							xVal = it3.value();
							if (xVal > 180)
								xVal -= 360;
							if (xVal < -180)
								xVal += 360;
							if (last_x > -998) {
								if (abs(last_x - xVal) > 180) {
									if (last_x > xVal)
										last_x = xVal + 360;
									else
										last_x = xVal - 360;
								}
								else
									last_x = xVal;
							}
							else
								last_x = xVal;
							if (model.preferredPath.minX > last_x)
								model.preferredPath.minX = last_x;
							if (model.preferredPath.maxX < last_x)
								model.preferredPath.maxX = last_x;
						}
						if (i2 == 1)
							yVal = it3.value();
						i2++;
					}
					if (abs(xVal - xValOld) > 0.00001 || abs(yVal - yValOld) > 0.00001) {
						model.preferredPath.point[nPointsTot] = spherical::Point(yVal, xVal);
						xValOld = xVal;
						yValOld = yVal;
						nPointsTot++;
					}
				}
			}
			i++;
			model.preferredPath.nPoints = nPointsTot;
		}
	}

	printf("nPoints in preferredPath %d\n", model.preferredPath.nPoints);
	errlog("nPoints in preferredPath %d\n", model.preferredPath.nPoints);
	if (model.preferredPath.nPoints == 0) {
		printf("ERROR! There must be points in the preferred path. I have nothing to do so I quit!\n");
		errlog("ERROR! There must be points in the preferred path. I have nothing to do so I quit!\n");
		exitKontrollerat(__LINE__);
	}

	if (model.preferredPath.minX < -180) {
		model.preferredPath.minX += 360;
		model.preferredPath.maxX += 360;
	}


	json dataStorm, dataStorm2, dataGeom, dataIt3;
	int i1, pos, stormNr, nAllocStorms;
	
	if (!data["storms"].is_null()) {
		dataStorm = data["storms"];
		if (!dataStorm["features"].is_null()) {
			dataStorm2 = dataStorm["features"];
			model.nStorms = 0;
			nAllocStorms = (int)dataStorm2.size();
			model.storms = (strStorm*)malloc(nAllocStorms * sizeof(strStorm));
			for (auto it = dataStorm2.begin(); it != dataStorm2.end(); ++it) {
				dataFeature = it.value();
				if (dataFeature["properties"].is_null()) {
					printf("ERROR! No properties for a feature in storms. I skip this one\n");
					errlog("ERROR! No properties for a feature in storms. I skip this one\n");
					continue; // no properties exists for this one, cannot be a preferred path
				}
				dataProp = dataFeature["properties"];
				//string namnNu = dataProp["stormID"];
				if (!dataProp["stormID"].is_null()) {
					stormNr = dataProp["stormID"];
					//printf("stormNr %d\n", stormNr);
				}
				else {
					errlog("ERROR! A storm feature is given without the property stormID. I skip this one\n");
					continue;
				}
				for (i = 0; i < model.nStorms; i++) {
					if (stormNr == model.storms[i].stormNr)
						break;
				}
				if (i >= model.nStorms) {
					// new stormID
					model.storms[i].stormNr = stormNr;
					model.storms[i].nAllocFeatures = 15;
					model.storms[i].feature = (strStormFeature*)malloc(model.storms[i].nAllocFeatures * sizeof(strStormFeature));
					(model.nStorms)++;
					model.storms[i].nFeatures = 0;
					model.storms[i].box_minLat = 360;
					model.storms[i].box_maxLat = -360;
					model.storms[i].box_minLon = 360;
					model.storms[i].box_maxLon = -360;

				}
				if (dataFeature["geometry"].is_null()) {
					printf("ERROR! No geometry for a feature in storms. I skip this one\n");
					errlog("ERROR! No geometry for a feature in storms. I skip this one\n");
					continue; // no properties exists for this one, cannot be a preferred path
				}
				dataGeom = dataFeature["geometry"];
				if (dataGeom["coordinates"].is_null()) {
					printf("ERROR! No coordinates for a geometry in storms. I skip this one\n");
					errlog("ERROR! No coordinates for a geometry in storms. I skip this one\n");
					continue; // no properties exists for this one, cannot be a preferred path
				}
				if (dataGeom["type"].is_null()) {
					printf("ERROR! No type for a geometry in storms. I skip this one\n");
					errlog("ERROR! No type for a geometry in storms. I skip this one\n");
					continue; // no properties exists for this one, cannot be a preferred path
				}
				if (dataGeom["type"] != "Point") {
					string namn = dataGeom["type"];
					errlog("ERROR! Geomestry type has to be Point for storms but it is %s. I skip this one\n", namn.c_str());
					continue; // no properties exists for this one, cannot be a preferred path

				}
				if (model.storms[i].nFeatures >= model.storms[i].nAllocFeatures) {
					model.storms[i].nAllocFeatures += 15;
					model.storms[i].feature = (strStormFeature*)realloc(model.storms[i].feature,
						model.storms[i].nAllocFeatures * sizeof(strStormFeature));
				}

				if (dataProp["WindFrontRadius"].is_null()) {
					errlog("ERROR! No WindFrontRadius given for a feature in storms. I set it to 0\n");
					model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize = 0;
				}else
					model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize = dataProp["WindFrontRadius"];
				if (dataProp["WindBackRadius"].is_null()) {
					errlog("ERROR! No WindBackRadius given for a feature in storms. I set it to 0\n");
					model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize = 0;
				}
				else
					model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize = dataProp["WindBackRadius"];

				if (dataProp["WindMaxRadius"].is_null()) {
					if (model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize >
						model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize)
						maxWind = model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize;
					else
						maxWind = model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize;
					errlog("ERROR! No WindMaxRadius given for a feature in storms. I set it to %.2lf (max of front and back wind radius)\n",
						maxWind);
					model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize = maxWind;
				}else
					model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize = dataProp["WindMaxRadius"];
				
				if (dataGeom["coordinates"].is_null()) {
					printf("ERROR! No coordinates in a geometry in storms. I skip this one\n");
					errlog("ERROR! No coordinates in a geometry in storms. I skip this one\n");
					continue; 
				}
				dataCoord = dataGeom["coordinates"];
				pos = 0;
				for (auto it2 = dataCoord.begin(); it2 != dataCoord.end(); ++it2) {
					dataIt2 = it2.value();
					for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
						dataIt3 = it3.value();
						// get midpoint
						if (pos == 0)
							model.storms[i].feature[model.storms[i].nFeatures].lon = dataIt3;
						else
							model.storms[i].feature[model.storms[i].nFeatures].lat = dataIt3;
						pos++;
					}
				}
				worstDegree = model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize / 120; // estimate
				if (model.storms[i].box_minLat > model.storms[i].feature[model.storms[i].nFeatures].lat - worstDegree)
					model.storms[i].box_minLat = model.storms[i].feature[model.storms[i].nFeatures].lat - worstDegree;
				if (model.storms[i].box_maxLat < model.storms[i].feature[model.storms[i].nFeatures].lat + worstDegree)
					model.storms[i].box_maxLat = model.storms[i].feature[model.storms[i].nFeatures].lat + worstDegree;
				if (model.storms[i].box_minLon > model.storms[i].feature[model.storms[i].nFeatures].lon - worstDegree)
					model.storms[i].box_minLon = model.storms[i].feature[model.storms[i].nFeatures].lon - worstDegree;
				if (model.storms[i].box_maxLon < model.storms[i].feature[model.storms[i].nFeatures].lon + worstDegree)
					model.storms[i].box_maxLon = model.storms[i].feature[model.storms[i].nFeatures].lon + worstDegree;
				//printf("%d stormNr %d box %.2lf %.2lf %.2lf %.2lf\n", i, model.storms[i].stormNr,
				//	model.storms[i].feature[model.storms[i].nFeatures].lon - worstDegree,
				//	model.storms[i].feature[model.storms[i].nFeatures].lat - worstDegree,
				//	model.storms[i].feature[model.storms[i].nFeatures].lon + worstDegree,
				//	model.storms[i].feature[model.storms[i].nFeatures].lat + worstDegree);

				model.storms[i].feature[model.storms[i].nFeatures].midPoint = spherical::Point(model.storms[i].feature[model.storms[i].nFeatures].lat,
					model.storms[i].feature[model.storms[i].nFeatures].lon);


				if (dataProp["FLDATELBL"].is_null()) {
					errlog("ERROR! No FLDATELBL (dateTime) given for a feature in storms. I skip this one\n");
					continue;
				}
				string tidpkt = dataProp["FLDATELBL"];
				model.storms[i].feature[model.storms[i].nFeatures].UTCseconds = make_gmtime_fromStormDateTime(tidpkt);
				(model.storms[i].nFeatures)++;
			}
		}
		errlog("ERROR! Add extended storm information when decided what to use. nStorms %d\n", model.nStorms);
	}
	else {
		errlog("OBS! No storms given in input data\n");
		model.nStorms = 0;
	}

	//params->weightFuel.base = 1;
	params->weightFuel.lsmgo = 0;
	params->weightFuel.vlsfo = 0;
	params->weightTime = 1;
	// strSafety* weightSafety;
	params->weightSafety.base = 1000;
	params->weightSafety.hurricane = 0;
	//params->weightSafety.stability = 0;
	//params->weightSafety.lowPressure = 0;
	//params->weightSafety.waves = 0;
	//params->weightSafety.stability = 0;
	params->weightSafety.bowSlam = 0;
	params->weightSafety.greenWater = 0;
	params->weightSafety.dynamicStability = 0;
	params->weightSafety.feasibleSafety = 100000;
	params->weightSafety.iceCoverCost_fix = 10000;
	params->weightSafety.iceCoverCost_thickness = 0;

	params->priceFuel.vlsfo = 500;
	params->priceFuel.lsmgo = 800;
	params->priceTime = 500;

	params->penalties.storm_costInsideInner = 1000000;
	params->penalties.storm_costInsideOuter_kvot = 100;

	if (!data["fuel_price"].is_null()) {
		json dataFuel = data["fuel_price"];
		if (!dataFuel["VLSFO"].is_null())
			params->priceFuel.vlsfo = dataFuel["VLSFO"];
		if (!dataFuel["LSMGO"].is_null())
			params->priceFuel.lsmgo = dataFuel["LSMGO"];
	}
	if (!data["vessel_price"].is_null())
		params->priceTime = data["vessel_price"];

	if (!data["objective"].is_null()) {
		json dataObj = data["objective"];

		if (!dataObj["weight_time"].is_null()) {
			dataIt = dataObj["weight_time"];
			params->weightTime = (double)(dataIt["weight"]) / 100;
		}
		if (!dataObj["weight_fuel"].is_null()) {
			json data3 = dataObj["weight_fuel"];
			//if (!data3["base"].is_null()) {
			//	dataIt2 = data3["base"];
			//	params->weightFuel.base = (double)(dataIt2["weight"]) / 100;
			//}
			if (!data3["VLSFO"].is_null()) {
				dataIt2 = data3["VLSFO"];
				params->weightFuel.vlsfo = (double)(dataIt2["weight"]) / 100;
			}
			if (!data3["LSMGO"].is_null()) {
				dataIt2 = data3["LSMGO"];
				params->weightFuel.lsmgo = (double)(dataIt2["weight"]) / 100;
			}
		}
		if (!dataObj["weight_safety"].is_null()) {
			json data3 = dataObj["weight_safety"];
			if (!data3["base"].is_null()) {
				dataIt2 = data3["base"];
				params->weightSafety.base = (double)(dataIt2["weight"]) / 100;
			}
			if (!data3["hurricane"].is_null()) {
				dataIt2 = data3["hurricane"];
				params->weightSafety.hurricane = (double)(dataIt2["weight"]) / 100;
			}

			if (!data3["bowSlamming"].is_null()) {
				dataIt2 = data3["bowSlamming"];
				params->weightSafety.bowSlam = dataIt2["weight"];
			}
			if (!data3["greenWater"].is_null()) {
				dataIt2 = data3["greenWater"];
				params->weightSafety.greenWater = dataIt2["weight"];
			}
			if (!data3["dynamicStability"].is_null()) {
				dataIt2 = data3["dynamicStability"];
				if (dataIt2["useWeight"] == 1)
					params->weightSafety.dynamicStability = dataIt2["weight"];
			}
			if (!data3["feasibleSafety"].is_null()) {
				dataIt2 = data3["feasibleSafety"];
				if (dataIt2["useWeight"] == 1)
					params->weightSafety.feasibleSafety = dataIt2["weight"];
			}
			if (!data3["iceCoverCost_fix"].is_null()) {
				dataIt2 = data3["iceCoverCost_fix"];
				if (dataIt2["useWeight"] == 1)
					params->weightSafety.iceCoverCost_fix = dataIt2["weight"];
			}
			if (!data3["iceCoverCost_thickness"].is_null()) {
				dataIt2 = data3["iceCoverCost_thickness"];
				if (dataIt2["useWeight"] == 1)
					params->weightSafety.iceCoverCost_thickness = dataIt2["weight"];
			}
		}
	}


	fil.close();




	return 0;
}

/*
void generate_helpTable_windSpeed() {
	int i, pos;
	double maxVarde;

	model.functions.nWindSpeedSkalad = model.functions.windSpeed_max / model.functions.windMagnitude_discreteSize_kts;
	model.functions.rel_windSpeed_kvotIndex = 1 / model.params.knots_to_km / model.functions.windMagnitude_discreteSize_kts;
	model.functions.rel_windSpeedSkalad_ger_index = (int*)malloc(model.functions.nWindSpeedSkalad * sizeof(int));
	pos = 0;
	for (i = 0; i < model.functions.table_niWindSpeed; i++) {
		maxVarde = model.functions.windSpeed_maxVal_array[i] / model.functions.windMagnitude_discreteSize_kts *
			model.params.knots_to_km;
		for (pos; pos < model.functions.nWindSpeedSkalad; pos++) {
			if (pos > maxVarde)
				break;
			model.functions.rel_windSpeedSkalad_ger_index[pos] = i;
		}
	}
	model.functions.rel_windSpeedSkalad_ger_index[model.functions.nWindSpeedSkalad - 1] = model.functions.table_niWindSpeed - 1;

}

void generate_helpTable_waveHeight() {
	int i, pos;
	double maxVarde;

	model.functions.nWaveHeightSkalad = model.functions.waveHeight_max / model.functions.waveHeight_discreteSize_m;
	model.functions.rel_waveHeight_kvotIndex = 1 / model.functions.waveHeight_discreteSize_m;
	model.functions.rel_waveHeightSkalad_ger_index = (int*)malloc(model.functions.nWaveHeightSkalad * sizeof(int));
	pos = 0;
	for (i = 0; i < model.functions.table_niWave; i++) {
		maxVarde = model.functions.waveHeight_maxVal_array[i] / model.functions.waveHeight_discreteSize_m;
		for (pos; pos < model.functions.nWaveHeightSkalad; pos++) {
			if (pos > maxVarde)
				break;
			model.functions.rel_waveHeightSkalad_ger_index[pos] = i;
		}
	}
	model.functions.rel_waveHeightSkalad_ger_index[model.functions.nWaveHeightSkalad - 1] = model.functions.table_niWave - 1;

}


void loadWeatherFactorTable_notUsed(std::string nameTable) {
	FILE* filpek;
	int nAlloc, i, pos, wind, windDirPos, wave, waveDirPos, wavePeriod, antal;
	double varde;

	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), nameTable.c_str());
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open weather factor file %s\n", namn);
		exitKontrollerat(__LINE__);
	}
	free(namn);

	nAlloc = model.functions.table_niWindSpeed * model.functions.table_niWindDir *
		model.functions.table_niWave * model.functions.table_niWaveDir *
		model.functions.table_niWavePeriod;
	model.functions.weatherFactors.table_speedDiff = (double*)calloc(nAlloc, sizeof(double));
	for (i = 0; i < 2 * nAlloc; i++) {
		antal = fscanf(filpek, "%d %d %d %d %d %lf", &wind, &windDirPos, &wave, &waveDirPos, &wavePeriod, &varde);
		if (antal <= 0)
			break;
		if (wind < 0 || wind >= model.functions.table_niWindSpeed) {
			errlog("ERROR! Wind index wrong, is %d must be between 0 - %d, i skip this one\n",
				wind, model.functions.table_niWindSpeed - 1);
			continue;
		}
		if (windDirPos < 0 || windDirPos >= model.functions.table_niWindDir) {
			errlog("ERROR! windDirPos index wrong, is %d must be between 0 - %d, i skip this one\n",
				windDirPos, model.functions.table_niWindDir - 1);
			continue;
		}
		if (wave < 0 || wave >= model.functions.table_niWave) {
			errlog("ERROR! wave index wrong, is %d must be between 0 - %d, i skip this one\n",
				wave, model.functions.table_niWave - 1);
			continue;
		}
		if (waveDirPos < 0 || waveDirPos >= model.functions.table_niWaveDir) {
			errlog("ERROR! waveDirPos index wrong, is %d must be between 0 - %d, i skip this one\n",
				waveDirPos, model.functions.table_niWaveDir - 1);
			continue;
		}
		if (wavePeriod < 0 || wavePeriod >= model.functions.table_niWavePeriod) {
			errlog("ERROR! wavePeriod index wrong, is %d must be between 0 - %d, i skip this one\n",
				wavePeriod, model.functions.table_niWavePeriod - 1);
			continue;
		}
		pos = wind + model.functions.table_niWindSpeed * (windDirPos + model.functions.table_niWindDir * (
			wave + model.functions.table_niWave * (
				waveDirPos + model.functions.table_niWaveDir * wavePeriod)));
		model.functions.table_speedDiff[pos] = varde;
	}
	fclose(filpek);

}
*/

void loadWeatherFactorTableWave(std::string nameTable) {
	FILE* filpek;
	int nAlloc, i, pos, calmWaterSpeedI, waveI, wavePeriodI, waveDirI, antal;
	double varde, wave, wavePeriod, waveDir, calmWaterSpeed;

	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), nameTable.c_str());
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open weather factor table file %s\n", namn);
		exitKontrollerat(__LINE__);
	}

	model.functions.weatherFactors.nWaveHeightIndex = (model.functions.weatherFactors.waveHeight_max -
		model.functions.weatherFactors.waveHeight_min) / model.functions.weatherFactors.waveHeightIndexSize + 1;
	model.functions.weatherFactors.nWavePeriodIndex = (model.functions.weatherFactors.wavePeriod_max -
		model.functions.weatherFactors.wavePeriod_min) / model.functions.weatherFactors.wavePeriodIndexSize + 1;
	model.functions.weatherFactors.nWaveDirIndex = (model.functions.weatherFactors.waveDir_max -
		model.functions.weatherFactors.waveDir_min) / model.functions.weatherFactors.waveDirIndexSize + 1;

	nAlloc = model.functions.weatherFactors.nCalmWaterSpeedIndex * model.functions.weatherFactors.nWaveHeightIndex * 
		model.functions.weatherFactors.nWavePeriodIndex * model.functions.weatherFactors.nWaveDirIndex;
	model.functions.weatherFactors.tableValueWave = (double*)malloc(nAlloc * sizeof(double));
	for (i = 0; i < nAlloc; i++) {
		model.functions.weatherFactors.tableValueWave[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);
	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf\t%lf\t%lf\n", &calmWaterSpeed, &wave, &wavePeriod, &waveDir, &varde);
		if (antal <= 0)
			break;

		calmWaterSpeedI = get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (calmWaterSpeedI < 0) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.calmWaterSpeed_min);
			exitKontrollerat(__LINE__);
		}
		if (calmWaterSpeedI >= model.functions.weatherFactors.nWaveHeightIndex) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.calmWaterSpeed_max);
			exitKontrollerat(__LINE__);
		}
		waveI = get_relWaveHeightIndex(wave, model.functions.weatherFactors, 1);
		if (waveI < 0) {
			errlog("ERROR! wave height %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.waveHeight_min);
			exitKontrollerat(__LINE__);
		}
		if (waveI >= model.functions.weatherFactors.nWaveHeightIndex) {
			errlog("ERROR! waveHeight %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.waveHeight_max);
			exitKontrollerat(__LINE__);
		}
		wavePeriodI = get_relWavePeriodIndex(wavePeriod, model.functions.weatherFactors, 1);
		if (wavePeriodI < 0) {
			errlog("ERROR! wave Period %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.wavePeriod_min);
			exitKontrollerat(__LINE__);
		}
		if (wavePeriodI >= model.functions.weatherFactors.nWavePeriodIndex) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.wavePeriod_max);
			exitKontrollerat(__LINE__);
		}
		// wave direction has to be in radians and is against the ship direction so need to switch it here in order to use it correctly
		waveDirI = get_relWaveDirIndex(M_PI - waveDir * M_PI / 180, model.functions.weatherFactors, 1);
		if (waveDirI < 0) {
			errlog("ERROR! waveDirection %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.waveDir_min);
			exitKontrollerat(__LINE__);
		}
		if (waveDirI >= model.functions.weatherFactors.nWaveDirIndex) {
			errlog("ERROR! waveDirection %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.waveDir_max);
			exitKontrollerat(__LINE__);
		}

		pos = calmWaterSpeedI + model.functions.weatherFactors.nCalmWaterSpeedIndex *
			(waveDirI + model.functions.weatherFactors.nWaveDirIndex *
				(wavePeriodI + model.functions.weatherFactors.nWavePeriodIndex * waveI));
		if (model.functions.weatherFactors.tableValueWave[pos] > -9998)
			errlog("ERROR! More than one value for wave factor table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.weatherFactors.tableValueWave[pos], varde);
		model.functions.weatherFactors.tableValueWave[pos] = varde * model.params.knots_to_km;
	}
	fclose(filpek);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.weatherFactors.tableValueWave[i] < -9998) {
			errlog("ERROR! No value given for wave factor table pos %d. I set it to 0.\n", i);
			model.functions.weatherFactors.tableValueWave[i] = 0;
		}
	}
}

void loadWeatherFactorTableWind(std::string nameTable) {
	FILE* filpek;
	int nAlloc, i, pos, windI, windDirI, calmWaterSpeedI, antal;
	double varde, wind, windDir, calmWaterSpeed;

	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), nameTable.c_str());
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open weather factor table file %s\n", namn);
		exitKontrollerat(__LINE__);
	}

	model.functions.weatherFactors.nCalmWaterSpeedIndex = (model.functions.weatherFactors.calmWaterSpeed_max -
		model.functions.weatherFactors.calmWaterSpeed_min) / model.functions.weatherFactors.calmWaterSpeedIndexSize + 1;
	model.functions.weatherFactors.nWindSpeedIndex = (model.functions.weatherFactors.windSpeed_max -
		model.functions.weatherFactors.windSpeed_min) / model.functions.weatherFactors.windSpeedIndexSize + 1;
	model.functions.weatherFactors.nWindDirIndex = (model.functions.weatherFactors.windDir_max -
		model.functions.weatherFactors.windDir_min) / model.functions.weatherFactors.windDirIndexSize + 1;

	nAlloc = model.functions.weatherFactors.nCalmWaterSpeedIndex * 
		model.functions.weatherFactors.nWindSpeedIndex * model.functions.weatherFactors.nWindDirIndex;
	model.functions.weatherFactors.tableValueWind = (double*)malloc(nAlloc * sizeof(double));
	for (i = 0; i < nAlloc; i++) {
		model.functions.weatherFactors.tableValueWind[i] = -9999;
	}
	printf("nAlloc %d %d %d %d\n", nAlloc, model.functions.weatherFactors.nCalmWaterSpeedIndex,
		model.functions.weatherFactors.nWindSpeedIndex, model.functions.weatherFactors.nWindDirIndex);

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);
	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf\t%lf\n", &calmWaterSpeed, &wind, &windDir, &varde);
		//printf("windfactor table %lf %lf %lf %lf antal %d\n", calmWaterSpeed, wind, windDir, varde, antal);
		if (antal <= 0)
			break;
	
		calmWaterSpeedI = get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (calmWaterSpeedI < 0) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.calmWaterSpeed_min);
			exitKontrollerat(__LINE__);
		}
		if (calmWaterSpeedI >= model.functions.weatherFactors.nCalmWaterSpeedIndex) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.calmWaterSpeed_max);
			exitKontrollerat(__LINE__);
		}
		windI = get_relWindSpeedIndex(wind, model.functions.weatherFactors, 1);
		if (windI < 0) {
			errlog("ERROR! windSpeed %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.windSpeed_min);
			exitKontrollerat(__LINE__);
		}
		if (windI >= model.functions.weatherFactors.nWindSpeedIndex) {
			errlog("ERROR! windSpeed %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.windSpeed_max);
			exitKontrollerat(__LINE__);
		}
		// wind direction has to be in radians and is against the ship direction so need to switch it here in order to use it correctly
		windDirI = get_relWindDirIndex(M_PI - windDir * M_PI / 180, model.functions.weatherFactors, 1);
		if (windDirI < 0) {
			errlog("ERROR! windDirection %lf given in %s/%s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.windDir_min);
			exitKontrollerat(__LINE__);
		}
		if (windDirI >= model.functions.weatherFactors.nWindDirIndex) {
			errlog("ERROR! windDirection %lf given in %s/%s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), nameTable.c_str(), model.functions.weatherFactors.windDir_max);
			exitKontrollerat(__LINE__);
		}


		pos = calmWaterSpeedI + model.functions.weatherFactors.nCalmWaterSpeedIndex *
			(windDirI + model.functions.weatherFactors.nWindDirIndex * windI);
		if (pos == 5430)
			printf("pos %d calmWaterSpeedI %d windDirI %d windI %d calmWaterSpeed %.2lf windDir %.2lf wind %.2lf\n",
				pos, calmWaterSpeedI, windDirI, windI, calmWaterSpeed, windDir, wind);
		if (model.functions.weatherFactors.tableValueWind[pos] > -9998) {
			windI = pos / (model.functions.weatherFactors.nCalmWaterSpeedIndex * model.functions.weatherFactors.nWindDirIndex);
			windDirI = (pos - windI * (model.functions.weatherFactors.nCalmWaterSpeedIndex * model.functions.weatherFactors.nWindDirIndex)) /
				model.functions.weatherFactors.nCalmWaterSpeedIndex;
			calmWaterSpeedI = pos - windI * (model.functions.weatherFactors.nCalmWaterSpeedIndex * model.functions.weatherFactors.nWindDirIndex) -
				windDirI * model.functions.weatherFactors.nCalmWaterSpeedIndex;
			errlog("ERROR! More than one value for wind factor table pos %d, before %lf, now %lf (speed %.2lf wind %.2lf windDir %.2lf). I use the later one.\n", pos,
				model.functions.weatherFactors.tableValueWind[pos], varde, 
				model.functions.weatherFactors.calmWaterSpeed_min + calmWaterSpeedI * model.functions.weatherFactors.calmWaterSpeedIndexSize,
				model.functions.weatherFactors.windSpeed_min + windI * model.functions.weatherFactors.windSpeedIndexSize,
				model.functions.weatherFactors.windDir_min + windDirI * model.functions.weatherFactors.windDirIndexSize);
		}
		model.functions.weatherFactors.tableValueWind[pos] = varde * model.params.knots_to_km;
	}
	fclose(filpek);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.weatherFactors.tableValueWind[i] < -9998) {
			errlog("ERROR! No value given for wind factor table pos %d. I set it to 0.\n", i);
			model.functions.weatherFactors.tableValueWind[i] = 0;
		}
	}
}

void loadDynamicStabilityTable(std::string nameTable) {
	FILE* filpek;
	int nAlloc, i, pos, windI, windDirI, shipSpeedI, antal;
	double varde, wind, windDir, shipSpeed;

	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), nameTable.c_str());
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open dynamic stability table file %s\n", namn);
		exitKontrollerat(__LINE__);
	}

	model.functions.dynStability.nWindSpeedIndex = (model.functions.dynStability.windSpeed_max -
		model.functions.dynStability.windSpeed_min) / model.functions.dynStability.windSpeedIndexSize;
	model.functions.dynStability.nWindDirIndex = (model.functions.dynStability.windDir_max -
		model.functions.dynStability.windDir_min) / model.functions.dynStability.windDirIndexSize;
	model.functions.dynStability.nShipSpeedIndex = (model.functions.dynStability.shipSpeed_max -
		model.functions.dynStability.shipSpeed_min) / model.functions.dynStability.shipSpeedIndexSize;

	nAlloc = model.functions.dynStability.nWindSpeedIndex * model.functions.dynStability.nWindDirIndex *
		model.functions.dynStability.nShipSpeedIndex;
	model.functions.dynStability.tableValue = (double*)malloc(nAlloc * sizeof(double));
	for (i = 0; i < nAlloc; i++) {
		model.functions.dynStability.tableValue[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);

	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf\t%lf", &wind, &windDir, &shipSpeed, &varde);
		if (antal <= 0)
			break;
		windI = get_relWindSpeedIndex(wind, model.functions.dynStability, 1);
		if (windI < 0) {
			errlog("ERROR! windSpeed %lf given in %s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.functions.dynStability.windSpeed_min);
			exitKontrollerat(__LINE__);
		}
		if(windI >= model.functions.dynStability.nWindSpeedIndex){
			errlog("ERROR! windSpeed %lf given in %s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.functions.dynStability.windSpeed_max);
			exitKontrollerat(__LINE__);
		}
		// wind direction has to be in radians and is against the ship direction so need to switch it here in order to use it correctly
		windDirI = get_relWindDirIndex(M_PI - windDir * M_PI / 180, model.functions.dynStability, 1);
		if(windDirI < 0){
			errlog("ERROR! windDirection %lf given in %s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.functions.dynStability.windDir_min);
			exitKontrollerat(__LINE__);
		}
		if (windDirI >= model.functions.dynStability.nWindDirIndex) {
			errlog("ERROR! windDirection %lf given in %s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.functions.dynStability.windDir_max);
			exitKontrollerat(__LINE__);
		}
		shipSpeedI = get_shipSpeedIndex(shipSpeed, model.functions.dynStability, 1);
		if (shipSpeedI < 0) {
			errlog("ERROR! shipSpeed %lf given in %s is less than min %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				shipSpeed, model.params.indataPath.c_str(), model.functions.dynStability.shipSpeed_min);
			exitKontrollerat(__LINE__);
		}
		if (shipSpeedI >= model.functions.dynStability.nShipSpeedIndex) {
			errlog("ERROR! shipSpeed %lf given in %s is more than max %lf given in functions_parameters.json.\nFix and run again, i quit!\n",
				shipSpeed, model.params.indataPath.c_str(), model.functions.dynStability.shipSpeed_max);
			exitKontrollerat(__LINE__);
		}
		pos = shipSpeedI + model.functions.dynStability.nShipSpeedIndex * (windDirI + model.functions.dynStability.nWindDirIndex * windI);
		if(model.functions.dynStability.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for dynamic stability table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.dynStability.tableValue[pos], varde);
		model.functions.dynStability.tableValue[pos] = varde;
	}
	fclose(filpek);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.dynStability.tableValue[i] < -9998) {
			errlog("ERROR! No value given for dynamic stability table pos %d. I set it to 0.\n", i);
			model.functions.dynStability.tableValue[i] = 0;
		}
	}
}

int loadFunctions()
{

	std::ifstream fil;
	char* namn;
	std::string nameTable;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/function_parameters.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(exists_test3(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		exitKontrollerat(__LINE__);
	}
	printf("opens %s\n", namn);
	fil.open(namn);

	json data;

	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		exitKontrollerat(__LINE__);
	}

	model.functions.iceCoverMaxFree = 0;
	//model.functions.iceCoverCost_fix = 100000;
	model.functions.calmWaterSpeed.c0 = 5;
	model.functions.calmWaterSpeed.c1_rpm = 0.1;
	model.functions.calmWaterSpeed.c2_rpm = 0.0001;
	model.functions.fuelConsumption.c0 = 0.5;
	model.functions.fuelConsumption.c1_rpm = 0.01;
	model.functions.fuelConsumption.c2_rpm = 0.00001;
	model.functions.fuelConsumption.c3_rpm = 0.000001;

	//if (!data["nWindDir"].is_null())
	//	model.functions.table_niWindDir = data["nWindDir"];
	//else
	//	model.functions.table_niWindDir = 8;
	//if (!data["nWaveDir"].is_null())
	//	model.functions.table_niWaveDir = data["nWaveDir"];
	//else
	//	model.functions.table_niWaveDir = 8;

	if (!data["calmWaterSpeed"].is_null()) {
		json data2 = data["calmWaterSpeed"];
		if (!data2["c0"].is_null())
			model.functions.calmWaterSpeed.c0 = data2["c0"];
		if (!data2["c1_rpm"].is_null())
			model.functions.calmWaterSpeed.c1_rpm = data2["c1_rpm"];
		if (!data2["c2_rpm"].is_null())
			model.functions.calmWaterSpeed.c2_rpm = data2["c2_rpm"];
	}
	if (!data["fuelConsumption"].is_null()) {
		json data2 = data["fuelConsumption"];
		if (!data2["c0"].is_null())
			model.functions.fuelConsumption.c0 = data2["c0"];
		if (!data2["c1_rpm"].is_null())
			model.functions.fuelConsumption.c1_rpm = data2["c1_rpm"];
		if (!data2["c2_rpm"].is_null())
			model.functions.fuelConsumption.c2_rpm = data2["c2_rpm"];
		if (!data2["c3_rpm"].is_null())
			model.functions.fuelConsumption.c3_rpm = data2["c3_rpm"];
	}

	if (!data["weatherFactors"].is_null()) {
		json data2 = data["weatherFactors"];

		model.functions.weatherFactors.calmWaterSpeed_min = 6;
		model.functions.weatherFactors.calmWaterSpeed_max = 15;
		model.functions.weatherFactors.calmWaterSpeedIndexSize = 1;
		model.functions.weatherFactors.waveHeight_min = 0;
		model.functions.weatherFactors.waveHeight_max = 7.25;
		model.functions.weatherFactors.waveHeightIndexSize = 0.25;
		model.functions.weatherFactors.wavePeriod_min = 4.5;
		model.functions.weatherFactors.wavePeriod_max = 13.5;
		model.functions.weatherFactors.wavePeriodIndexSize = 1;
		model.functions.weatherFactors.waveDir_min = 0;
		model.functions.weatherFactors.waveDir_max = 180;
		model.functions.weatherFactors.waveDirIndexSize = 7.5;
		model.functions.weatherFactors.windSpeed_min = 0;
		model.functions.weatherFactors.windSpeed_max = 30;
		model.functions.weatherFactors.windSpeedIndexSize = 2;
		model.functions.weatherFactors.windDir_min = 0;
		model.functions.weatherFactors.windDir_max = 180;
		model.functions.weatherFactors.windDirIndexSize = 7.5;

		if (!data2["shipSpeed_calmWater_knots"].is_null()) {
			json data3 = data2["shipSpeed_calmWater_knots"];
			if (!data3["minValue"].is_null())
				model.functions.weatherFactors.calmWaterSpeed_min = data3["minValue"];
			else
				errlog("ERROR! No value for weatherFactors->shipSpeed_calmWater_knots->minValue. I set it to default %.3lf\n", model.functions.weatherFactors.calmWaterSpeed_min);
			if (!data3["maxValue"].is_null())
				model.functions.weatherFactors.calmWaterSpeed_max = data3["maxValue"];
			else
				errlog("ERROR! No value for weatherFactors->shipSpeed_calmWater_knots->maxValue. I set it to default %.3lf\n", model.functions.weatherFactors.calmWaterSpeed_max);
			if (!data3["intervallSize"].is_null())
				model.functions.weatherFactors.calmWaterSpeedIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for weatherFactors->shipSpeed_calmWater_knots->intervallSize. I set it to default %.3lf\n", model.functions.weatherFactors.calmWaterSpeedIndexSize);
		}
		else
			errlog("ERROR! No values for weatherFactors->shipSpeed_calmWater_knots.\n");
		if (!data2["significantWaveHeight_m"].is_null()) {
			json data3 = data2["significantWaveHeight_m"];
			if (!data3["minValue"].is_null())
				model.functions.weatherFactors.waveHeight_min = data3["minValue"];
			else
				errlog("ERROR! No value for weatherFactors->significantWaveHeight_m->minValue. I set it to default %.3lf\n", model.functions.weatherFactors.waveHeight_min);
			if (!data3["maxValue"].is_null())
				model.functions.weatherFactors.waveHeight_max = data3["maxValue"];
			else
				errlog("ERROR! No value for weatherFactors->significantWaveHeight_m->maxValue. I set it to default %.3lf\n", model.functions.weatherFactors.waveHeight_max);
			if (!data3["intervallSize"].is_null())
				model.functions.weatherFactors.waveHeightIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for weatherFactors->significantWaveHeight_m->intervallSize. I set it to default %.3lf\n", model.functions.weatherFactors.waveHeightIndexSize);
		}
		else
			errlog("ERROR! No values for weatherFactors->significantWaveHeight_m.\n");

		if (!data2["meanWavePeriod_s"].is_null()) {
			json data3 = data2["meanWavePeriod_s"];
			if (!data3["minValue"].is_null())
				model.functions.weatherFactors.wavePeriod_min = data3["minValue"];
			else
				errlog("ERROR! No value for weatherFactors->meanWavePeriod_s->minValue. I set it to default %.3lf\n", model.functions.weatherFactors.wavePeriod_min);
			if (!data3["maxValue"].is_null())
				model.functions.weatherFactors.wavePeriod_max = data3["maxValue"];
			else
				errlog("ERROR! No value for weatherFactors->meanWavePeriod_s->maxValue. I set it to default %.3lf\n", model.functions.weatherFactors.wavePeriod_max);
			if (!data3["intervallSize"].is_null())
				model.functions.weatherFactors.wavePeriodIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for weatherFactors->meanWavePeriod_s->intervallSize. I set it to default %.3lf\n", model.functions.weatherFactors.wavePeriodIndexSize);
		}
		else
			errlog("ERROR! No values for weatherFactors->meanWavePeriod_s.\n");

		if (!data2["relativeWaveDirection"].is_null()) {
			json data3 = data2["relativeWaveDirection"];
			if (!data3["minValue"].is_null())
				model.functions.weatherFactors.waveDir_min = data3["minValue"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWaveDirection->minValue. I set it to default %.3lf\n", model.functions.weatherFactors.waveDir_min);
			if (!data3["maxValue"].is_null())
				model.functions.weatherFactors.waveDir_max = data3["maxValue"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWaveDirection->maxValue. I set it to default %.3lf\n", model.functions.weatherFactors.waveDir_max);
			if (!data3["intervallSize"].is_null())
				model.functions.weatherFactors.waveDirIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWaveDirection->intervallSize. I set it to default %.3lf\n", model.functions.weatherFactors.waveDirIndexSize);
			// need to scale them to radians instead of degrees...
			model.functions.weatherFactors.waveDir_min *= M_PI / 180;
			model.functions.weatherFactors.waveDir_max *= M_PI / 180;
			model.functions.weatherFactors.waveDirIndexSize *= M_PI / 180;
		}
		else
			errlog("ERROR! No values for weatherFactors->relativeWaveDirection.\n");

		if (!data2["relativeWindSpeed_m_s"].is_null()) {
			json data3 = data2["relativeWindSpeed_m_s"];
			if (!data3["minValue"].is_null())
				model.functions.weatherFactors.windSpeed_min = data3["minValue"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWindSpeed->minValue. I set it to default %.3lf\n", model.functions.weatherFactors.windSpeed_min);
			if (!data3["maxValue"].is_null())
				model.functions.weatherFactors.windSpeed_max = data3["maxValue"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWindSpeed->maxValue. I set it to default %.3lf\n", model.functions.weatherFactors.windSpeed_max);
			if (!data3["intervallSize"].is_null())
				model.functions.weatherFactors.windSpeedIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWindSpeed->intervallSize. I set it to default %.3lf\n", model.functions.weatherFactors.windSpeedIndexSize);
		}
		else
			errlog("ERROR! No values for safety->relativeWindSpeed.\n");

		if (!data2["relativeWindDirection"].is_null()) {
			json data3 = data2["relativeWindDirection"];
			if (!data3["minValue"].is_null())
				model.functions.weatherFactors.windDir_min = data3["minValue"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWindDirection->minValue. I set it to default %.3lf\n", model.functions.weatherFactors.windDir_min);
			if (!data3["maxValue"].is_null())
				model.functions.weatherFactors.windDir_max = data3["maxValue"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWindDirection->maxValue. I set it to default %.3lf\n", model.functions.weatherFactors.windDir_max);
			if (!data3["intervallSize"].is_null())
				model.functions.weatherFactors.windDirIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for weatherFactors->relativeWindDirection->intervallSize. I set it to default %.3lf\n", model.functions.weatherFactors.windDirIndexSize);
			// need to scale them to radians instead of degrees...
			model.functions.weatherFactors.windDir_min *= M_PI / 180;
			model.functions.weatherFactors.windDir_max *= M_PI / 180;
			model.functions.weatherFactors.windDirIndexSize *= M_PI / 180;
		}
		else
			errlog("ERROR! No values for weatherFactors->relativeWindDirection.\n");

		if (!data2["weatherFactorsWind_tableName"].is_null()) {
			nameTable = data2["weatherFactorsWind_tableName"];
			loadWeatherFactorTableWind(nameTable);
		}
		else {
			errlog("ERROR! No weatherFactorsWind_tableName given in function_parameters.json\nI quit!\n");
			exitKontrollerat(__LINE__);
		}
		if (!data2["weatherFactorsWave_tableName"].is_null()) {
			nameTable = data2["weatherFactorsWave_tableName"];
			loadWeatherFactorTableWave(nameTable);
		}
		else {
			errlog("ERROR! No weatherFactorsWave_tableName given in function_parameters.json\nI quit!\n");
			exitKontrollerat(__LINE__);
		}

	}
	else {
	errlog("ERROR! No weatherFactors given in function_parameters.json\nI quit!\n");
	exitKontrollerat(__LINE__);
	}

	
	/*
	if (!data["windMagnitude_discreteSize_kts"].is_null())
		model.functions.windMagnitude_discreteSize_kts = data["windMagnitude_discreteSize_kts"];
	else
		model.functions.windMagnitude_discreteSize_kts = 0.25;
	if (!data["windMagnitudeTable"].is_null()) {
		json data2 = data["windMagnitudeTable"];
		int pos = 0;
		model.functions.table_niWindSpeed = (int)data2.size();
		model.functions.windSpeed_minVal_array = (double*)malloc(model.functions.table_niWindSpeed *
			sizeof(double));
		model.functions.windSpeed_maxVal_array = (double*)malloc(model.functions.table_niWindSpeed *
			sizeof(double));
		for (auto it = data2.begin(); it != data2.end(); ++it) {
			json dataIt = it.value();
			pos = dataIt["index"];
			if (pos < 0 || pos >= model.functions.table_niWindSpeed) {
				errlog("ERROR! Wrong index %d for windMagnitudeTable in function_parameters.json, is %d, must be 0 - %d\n",
					pos, model.functions.table_niWindSpeed);
				exitKontrollerat(__LINE__);
			}
			model.functions.windSpeed_minVal_array[pos] = dataIt["minWind_kts"];
			model.functions.windSpeed_maxVal_array[pos] = dataIt["maxWind_kts"];
		}
		if (model.functions.table_niWindSpeed > 0)
			model.functions.windSpeed_max = model.functions.windSpeed_maxVal_array[model.functions.table_niWindSpeed - 1];
		else
			model.functions.windSpeed_max = 0;
	}
	else {
		model.functions.table_niWindSpeed = 0;
		model.functions.windSpeed_max = 0;
	}
	generate_helpTable_windSpeed();

	if (!data["waveHeight_discreteSize_m"].is_null())
		model.functions.waveHeight_discreteSize_m = data["waveHeight_discreteSize_m"];
	else
		model.functions.waveHeight_discreteSize_m = 0.15;
	if (!data["waveHeightTable"].is_null()) {
		json data2 = data["waveHeightTable"];
		int pos = 0;
		model.functions.table_niWave = (int)data2.size();
		model.functions.waveHeight_minVal_array = (double*)malloc(model.functions.table_niWave *
			sizeof(double));
		model.functions.waveHeight_maxVal_array = (double*)malloc(model.functions.table_niWave *
			sizeof(double));
		for (auto it = data2.begin(); it != data2.end(); ++it) {
			json dataIt = it.value();
			pos = dataIt["index"];
			if (pos < 0 || pos >= model.functions.table_niWave) {
				errlog("ERROR! Wrong index %d for waveHeightTable in function_parameters.json, is %d, must be 0 - %d\n",
					pos, model.functions.table_niWave);
				exitKontrollerat(__LINE__);
			}
			model.functions.waveHeight_minVal_array[pos] = dataIt["minWaveHeight_m"];
			model.functions.waveHeight_maxVal_array[pos] = dataIt["maxWaveHeight_m"];
		}
		if (model.functions.table_niWave > 0)
			model.functions.waveHeight_max = model.functions.waveHeight_maxVal_array[model.functions.table_niWave - 1];
		else
			model.functions.waveHeight_max = 0;
	}
	else {
		model.functions.table_niWave = 0;
		model.functions.waveHeight_max = 0;
	}
	generate_helpTable_waveHeight();
	model.functions.max_wavePeriodSkalad = 1;
	model.functions.rel_wavePeriod_ger_index = (int*)calloc(model.functions.max_wavePeriodSkalad, sizeof(int));

	if (!data["wavePeriodTable"].is_null()) {
		json data2 = data["wavePeriodTable"];
		int pos = 0;
		model.functions.table_niWavePeriod = (int)data2.size();
		model.functions.wavePeriod_minVal_array = (double*)malloc(model.functions.table_niWavePeriod *
			sizeof(double));
		model.functions.wavePeriod_maxVal_array = (double*)malloc(model.functions.table_niWavePeriod *
			sizeof(double));
		for (auto it = data2.begin(); it != data2.end(); ++it) {
			json dataIt = it.value();
			pos = dataIt["index"];
			if (pos < 0 || pos >= model.functions.table_niWavePeriod) {
				errlog("ERROR! Wrong index %d for wavePeriodTable in function_parameters.json, is %d, must be 0 - %d\n",
					pos, model.functions.table_niWavePeriod);
				exitKontrollerat(__LINE__);
			}
			model.functions.wavePeriod_minVal_array[pos] = dataIt["minWavePeriod_s"];
			model.functions.wavePeriod_maxVal_array[pos] = dataIt["maxWavePeriod_s"];
		}
		if(model.functions.table_niWavePeriod > 0)
			model.functions.wavePeriod_max = model.functions.wavePeriod_maxVal_array[model.functions.table_niWavePeriod - 1];
		else
			model.functions.wavePeriod_max = 0;
	}
	else {
		model.functions.table_niWavePeriod = 0;
		model.functions.wavePeriod_max = 0;
	}

	std::string nameTable;
	if (!data["weatherFactor_tableName"].is_null()) {
		nameTable = data["weatherFactor_tableName"];
		loadWeatherFactorTable(nameTable);
	}
	else {
		errlog("ERROR! No weatherFactor_tableName given in function_parameters.json\n");
		exitKontrollerat(__LINE__);
	}
	*/


	if (!data["safety"].is_null()) {
		json data2 = data["safety"];
		if (!data2["iceCoverMaxFree"].is_null())
			model.functions.iceCoverMaxFree = data2["iceCoverMaxFree"];
		else
			errlog("ERROR! No value for iceCoverMaxFree. I set it to default %.3lf\n", model.functions.iceCoverMaxFree);

		model.functions.dynStability.windSpeed_min = 0;
		model.functions.dynStability.windSpeed_max = 30;
		model.functions.dynStability.windSpeedIndexSize = 2;
		model.functions.dynStability.windDir_min = 0;
		model.functions.dynStability.windDir_max = 180;
		model.functions.dynStability.windDirIndexSize = 7.5;
		model.functions.dynStability.shipSpeed_min = 0;
		model.functions.dynStability.shipSpeed_max = 30;
		model.functions.dynStability.shipSpeedIndexSize = 2;

		if (!data2["relativeWindSpeed_m_s"].is_null()) {
			json data3 = data2["relativeWindSpeed_m_s"];
			if (!data3["minValue"].is_null())
				model.functions.dynStability.windSpeed_min = data3["minValue"];
			else
				errlog("ERROR! No value for safety->relativeWindSpeed->minValue. I set it to default %.3lf\n", model.functions.dynStability.windSpeed_min);
			if (!data3["maxValue"].is_null())
				model.functions.dynStability.windSpeed_max = data3["maxValue"];
			else
				errlog("ERROR! No value for safety->relativeWindSpeed->maxValue. I set it to default %.3lf\n", model.functions.dynStability.windSpeed_max);
			if (!data3["intervallSize"].is_null())
				model.functions.dynStability.windSpeedIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for safety->relativeWindSpeed->intervallSize. I set it to default %.3lf\n", model.functions.dynStability.windSpeedIndexSize);
		}
		else
			errlog("ERROR! No values for safety->relativeWindSpeed.\n");

		if (!data2["relativeWindDirection"].is_null()) {
			json data3 = data2["relativeWindDirection"];
			if (!data3["minValue"].is_null())
				model.functions.dynStability.windDir_min = data3["minValue"];
			else
				errlog("ERROR! No value for safety->relativeWindDirection->minValue. I set it to default %.3lf\n", model.functions.dynStability.windDir_min);
			if (!data3["maxValue"].is_null())
				model.functions.dynStability.windDir_max = data3["maxValue"];
			else
				errlog("ERROR! No value for safety->relativeWindDirection->maxValue. I set it to default %.3lf\n", model.functions.dynStability.windDir_max);
			if (!data3["intervallSize"].is_null())
				model.functions.dynStability.windDirIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for safety->relativeWindDirection->intervallSize. I set it to default %.3lf\n", model.functions.dynStability.windDirIndexSize);
			// need to scale them to radians instead of degrees...
			model.functions.dynStability.windDir_min *= M_PI / 180;
			model.functions.dynStability.windDir_max *= M_PI / 180;
			model.functions.dynStability.windDirIndexSize *= M_PI / 180;
		}
		else
			errlog("ERROR! No values for safety->relativeWindDirection.\n");

		if (!data2["shipSpeedOverGround_m_s"].is_null()) {
			json data3 = data2["shipSpeedOverGround_m_s"];
			if (!data3["minValue"].is_null())
				model.functions.dynStability.shipSpeed_min = data3["minValue"];
			else
				errlog("ERROR! No value for safety->shipSpeedOverGround->minValue. I set it to default %.3lf\n", model.functions.dynStability.shipSpeed_min);
			if (!data3["maxValue"].is_null())
				model.functions.dynStability.shipSpeed_max = data3["maxValue"];
			else
				errlog("ERROR! No value for safety->shipSpeedOverGround->maxValue. I set it to default %.3lf\n", model.functions.dynStability.shipSpeed_max);
			if (!data3["intervallSize"].is_null())
				model.functions.dynStability.shipSpeedIndexSize = data3["intervallSize"];
			else
				errlog("ERROR! No value for safety->shipSpeedOverGround->intervallSize. I set it to default %.3lf\n", model.functions.dynStability.shipSpeedIndexSize);
		}
		else
			errlog("ERROR! No values for safety->shipSpeedOverGround.\n");

		if (!data2["dynamicStability_tableName"].is_null()) {
			nameTable = data2["dynamicStability_tableName"];
			loadDynamicStabilityTable(nameTable);
		}
		else {
			errlog("ERROR! No dynamicStability_tableName given in function_parameters.json\nI quit!\n");
			exitKontrollerat(__LINE__);
		}
	}
	else {
		errlog("ERROR! No safety given in function_parameters.json\nI quit!\n");
		exitKontrollerat(__LINE__);
	}





	fil.close();




	return 0;
}

int loadpreferredPathGeojson()
{
	int i;

	std::ifstream fil;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.params.preferredPath.c_str());
	printf("opens %s\n", namn);
	fil.open(namn);
	
	json data, dataGeo, dataCoord;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run voyageOpt again.\n", namn);
		exitKontrollerat(__LINE__);
	}
	
	if (data["geometry"].is_null()) {
		errlog("ERROR! No geometry given for the prefered path. I quit!\n");
		exit(0);
	}
	dataGeo = data["geometry"];
	if (dataGeo["coordinates"].is_null()) {
		errlog("ERROR! No coordinates given for the prefered path. I quit!\n");
		exit(0);
	}
	dataCoord = dataGeo["coordinates"];

	i = 0;
	json dataIt, dataIt2;
	int i2, swapMinMax = 0;
	double xVal, yVal, min_x = 180, max_x = -180, last_x = -999;
	for (auto it = dataCoord.begin(); it != dataCoord.end(); ++it) {
		dataIt = it.value();
		model.preferredPath.nPoints = (int)dataIt.size();
		model.preferredPath.point = (spherical::Point*)malloc(model.preferredPath.nPoints * sizeof(spherical::Point));
		for (auto it2 = dataIt.begin(); it2 != dataIt.end(); ++it2) {
			dataIt2 = it2.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0) {
					xVal = it3.value();
					if (xVal > 180)
						xVal -= 360;
					if (xVal < -180)
						xVal += 360;
				}
				if (i2 == 1)
					yVal = it3.value();
				i2++;
			}
			model.preferredPath.point[i] = spherical::Point(yVal, xVal);
			i++;
		}
	}

	fil.close();

	if(abs(min_x - max_x) > 180)



	errlog("preferredPath nPoints along arc %d\n", model.preferredPath.nPoints);
	return 0;
}

int loadVariables(int alt = 0)
{
	json data, dataVar, dataIt, dataIt2, dataFiles;
	int i1, i0;
	std::string typeName, namn;

	//if (alt == 0)
		namn = model.params.indataPath + "/weather_parameters.json";// +model.params.variableFileName;
	//else
	//	namn = model.params.variableFileName;

	std::ifstream fil(namn);

	if (!fil.is_open()) {
		errlog("ERROR! Could not open the file %s//weather_parameters.json with information about the weather parameters. I quit.\n", model.params.indataPath.c_str());
		exit(0);
	}
	fil >> data;

	model.nWeatherFiles = 0;
	dataVar = data["weather_parameters"];

	model.weather = (strWeather*)malloc((int)dataVar.size() * sizeof(strWeather));
	//printf("\n####\nalloc %d weatherData\n", (int)dataVar.size());
	i0 = 0;
	for (auto it = dataVar.begin(); it != dataVar.end(); ++it) {
		dataIt = it.value();
		namn = dataIt["variableID"];
		model.weather[i0].weatherFileTypeName = str_alloc_cpy(namn.c_str());
		model.weather[i0].timeIntervall_h = dataIt["timeIntervall_h"];
		model.weather[i0].nBlock_x = dataIt["nBlock_x"];
		model.weather[i0].nBlock_y = dataIt["nBlock_y"];
		dataFiles = dataIt["files"];
		model.weather[i0].nFiles = (int)dataFiles.size();
		model.weather[i0].filePos = (strFileWeather*)malloc(model.weather[i0].nFiles * sizeof(strFileWeather));
		model.weather[i0].rasterPos = (Raster*)malloc(model.weather[i0].nFiles * sizeof(Raster));
		i1 = 0;
		for (auto it2 = dataFiles.begin(); it2 != dataFiles.end(); ++it2) {
			dataIt2 = it2.value();
			namn = dataIt2["fileName"];
			model.weather[i0].filePos[i1].fileName = str_alloc_cpy(namn.c_str());
			//model.weather[i0].filePos[i1].fileNameOnly = splitFilename(namn, 1);

			//model.weather[i0].filePos[i1].minX = dataIt2["minLon"];
			//model.weather[i0].filePos[i1].maxX = dataIt2["maxLon"];
			i1++;
		}
		(model.nWeatherFiles)++;
		i0++;
	}

	model.functions.pos_wind_u = -1;
	model.functions.pos_wind_v = -1;
	model.functions.pos_current_u = -1;
	model.functions.pos_current_v = -1;
	model.functions.pos_waveHeight = -1;
	model.functions.pos_wavePeriod = -1;
	model.functions.pos_waveDirection = -1;
	model.functions.pos_iceThickness = -1;

	for (i0 = 0; i0 < model.nWeatherFiles; i0++) {
		if (strcmp(model.weather[i0].weatherFileTypeName, "wind_uComponent") == 0)
			model.functions.pos_wind_u = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "wind_vComponent") == 0)
			model.functions.pos_wind_v = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "current_uComponent") == 0)
			model.functions.pos_current_u = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "current_vComponent") == 0)
			model.functions.pos_current_v = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "waveHeight") == 0)
			model.functions.pos_waveHeight = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "wavePeriod") == 0)
			model.functions.pos_wavePeriod = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "waveDirection") == 0)
			model.functions.pos_waveDirection = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "ice thickness(m)") == 0)
			model.functions.pos_iceThickness = i0;
		printf("weatherFile %d %s\n", i0, model.weather[i0].weatherFileTypeName);
	}

	if (model.functions.pos_wind_u == -1) {
		errlog("ERROR! weather parameter wind_uComponent not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_wind_v == -1) {
		errlog("ERROR! weather parameter wind_vComponent not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_current_u == -1) {
		errlog("ERROR! weather parameter current_uComponent not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_current_v == -1) {
		errlog("ERROR! weather parameter current_vComponent not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_waveHeight == -1) {
		errlog("ERROR! weather parameter waveHeight not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_wavePeriod == -1) {
		errlog("ERROR! weather parameter wavePeriod not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_waveDirection == -1) {
		errlog("ERROR! weather parameter waveDirection not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}
	if (model.functions.pos_iceThickness == -1) {
		errlog("ERROR! weather parameter ice_thickness_m not given. It must exist\n");
		//exitKontrollerat(__LINE__);
	}


	errlog("pos_wind_u %d\n",	model.functions.pos_wind_u);
	errlog("pos_wind_v %d\n", model.functions.pos_wind_v);
	errlog("pos_current_u %d\n", model.functions.pos_current_u);
	errlog("pos_current_v %d\n", model.functions.pos_current_v);
	errlog("pos_waveHeight %d\n", model.functions.pos_waveHeight);
	errlog("pos_wavePeriod %d\n", model.functions.pos_wavePeriod);
	errlog("pos_waveDirection %d\n", model.functions.pos_waveDirection);
	errlog("pos_iceThickness %d\n", model.functions.pos_iceThickness);


	return 0;
}

int roundUp(double varde) {
	int heltal = (int)varde;
	if (heltal < varde)
		heltal++;
	return heltal;
}

double get_colDblFromWeatherFile(int weatherNr, double lon)
{
	double colDbl, tmpLon = lon;
	if (lon < model.weather[weatherNr].minX)
		lon += 360;
	//	if (lon < model.weather[weatherNr].rasterPos[*nr].Get_minLongitude() ||
	//		lon >= model.weather[weatherNr].rasterPos[*nr].Get_maxLongitude()) {
	//		*nr = get_correctWeatherFile(weatherNr, lon, *nr);
		//model.weatherFunctions.lastFileNr[weatherNr] = *nr;
	//}

		//colDbl = (lon - model.weather[weatherNr].rasterPos[*nr].Get_minLongitude()) / model.weather[weatherNr].rasterPos[*nr].Get_sizeCol();
	colDbl = (lon - model.weather[weatherNr].minX) / model.weather[weatherNr].size_col;
	if (colDbl < 0)
		colDbl += model.weather[weatherNr].nCols;
	if (colDbl < 0 || colDbl >= model.weather[weatherNr].nCols) {
		if (colDbl < -0.1 || colDbl > model.weather[weatherNr].nCols + 0.1) {
			errlog("ERROR! This should not happen. Fix it!! colDbl %.3lf nCols %d weatherNr %d lon %.3lf minLon %.3lf minX %.3lf maxLon %.3lf tmpLon %.3lf\n",
				colDbl, model.weather[weatherNr].nCols, weatherNr, lon,
				model.weather[weatherNr].minX, model.weather[weatherNr].minX,
				model.weather[weatherNr].maxX, tmpLon);
			exit(0);
		}
		else {
			if (colDbl < 0)
				colDbl = 0;
			else
				colDbl = model.weather[weatherNr].nCols - 0.1;
		}
	}


	return colDbl;
}

void redisTestRead() {
	std::string keyID;
	json mData;
	int nBlockRows, nBlockCols, ii, xBlockStart, xBlockEnd, yBlockStart, yBlockEnd;
	int i1, i2, forsta, nBands, pos, pos2, i3, i4, i5;
	size_t nAlloc;
	float* arrFloat, lat, lon, rowDbl, colDbl;
	FILE* filtmp;

//#ifndef WIN32
	auto redis = Redis("tcp://127.0.0.1:6379/1");
	auto tid0 = std::chrono::high_resolution_clock::now();

	//printf("testA\n");
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		keyID.assign(model.weather[ii].weatherFileTypeName);
		keyID.append(":metaData");
		//printf("testAa\n");
		auto reply = redis.get(keyID);
		//printf("testAb\n");
		if (reply) {
			auto val = nlohmann::json::parse(*reply);
			model.weather[ii].nCols = val["nCols"];
			model.weather[ii].nRows = val["nRows"];
			model.weather[ii].nTimeIntervals_forecast = val["nTimeIntervals_forecast"];
			model.weather[ii].nTimeIntervals = val["nTimeIntervals"];
			model.weather[ii].size_col = val["size_col"];
			model.weather[ii].size_row = val["size_row"];
			model.weather[ii].minX = val["minX"];
			model.weather[ii].maxX = val["maxX"];
			model.weather[ii].minY = val["minY"];
			model.weather[ii].maxY = val["maxY"];
			nBlockRows = val["nBlockRows"];
			printf("nBlockRows %d\n", nBlockRows);
			if (model.weather[ii].nBlock_y < roundUp((double)(model.weather[ii].nRows / nBlockRows))) {
				model.weather[ii].nBlock_y = roundUp((double)(model.weather[ii].nRows / nBlockRows));
				printf("OBS OBS changes model.weather[ii].nBlock_y to %d\n", model.weather[ii].nBlock_y);
			}
			nBlockCols = val["nBlockCols"];
			xBlockStart = 0;
			xBlockEnd = model.weather[ii].nBlock_x - 1;// roundUp(model.weather[ii].nCols / nBlockCols);
			yBlockStart = 0;
			yBlockEnd = model.weather[ii].nBlock_y - 1;// roundUp(model.weather[ii].nRows / nBlockRows);
		}
		else {
			errlog("ERROR! Failed to read metaData from redis for weather variable %s\n",
				model.weather[ii].weatherFileTypeName);
			exitKontrollerat(__LINE__);
		}
		//printf("testAc\n");

		nBands = model.weather[ii].nTimeIntervals;
		model.weather[ii].valueCell = (float**)malloc(nBands * sizeof(float*));
		nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
		for (int i2 = 0; i2 < nBands; i2++) {
			model.weather[ii].valueCell[i2] = (float*)malloc(nAlloc * sizeof(float));
		}
		nAlloc = nBands * nBlockRows * nBlockCols;


		auto tid1a = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_msa = tid1a - tid0;
		printf("read first part of redis data from %s took %.3lf\n", model.weather[ii].weatherFileTypeName, fp_msa);
		if (ii > 0)
			delete arrFloat;
		//printf("testA nAlloc %d\n", nAlloc);

		forsta = 1;
		for (i1 = yBlockStart; i1 <= yBlockEnd; i1++) {
			for (i2 = xBlockStart; i2 <= xBlockEnd; i2++) {
				pos = i2 + model.weather[ii].nBlock_x * i1;
				keyID.assign(model.weather[ii].weatherFileTypeName);
				keyID.append(":");
				keyID += to_string(pos);
				auto value = redis.get(keyID);
				if (value) {
					if (forsta == 1) {
						arrFloat = new float[value->size()];
						forsta = 0;
					}
					//printf("keyID %s size %d pos %d\n", keyID.c_str(), value->size(), pos);
					//if (pos == 10) {
					//	filtmp = fopen("test2.txt", "w");
					//	putStringIntoArrayFloat(*value, arrFloat, filtmp);
					//	fprintf(filtmp, "\n\n");
					//}
					//else
						memcpy(arrFloat, value->data(), value->size());
					//printf("pos %d %.3lf %.3lf %.3lf %.3lf size %d\n", pos, arrFloat[0], arrFloat[1], arrFloat[384920], arrFloat[384921], value->size());
					//	printf("pos %d\n", pos);
				}
				else
					printf("ERROR! Failed to load keyID %s\n", keyID.c_str());
				//if (pos > 12)
				//	exit(0);
				//printf("test i1 i2 %d %d nBands %d i4 %d %d i5 %d %d\n", i1, i2,
				//	nBands, nBlockRows * i1, nBlockRows * (i1 + 1),
				//	nBlockCols * i2, nBlockCols * (i2 + 1));
				//for (int i = 0; i < nAlloc; i++) {
				//	if (arrFloat[i] < 0.00001) {
				//		printf("pos %d last pos > 0 is %d nAlloc %d", pos, i - 1, nAlloc);
				//		break;
				//	}
				//}
				//for (int i = nAlloc - 1; i >= 0; i--) {
				//	if (arrFloat[i] > 0.00001) {
				//		printf("pos %d last pos > 0 backwards is %d nAlloc %d\n", pos, i + 1, nAlloc);
				//		break;
				//	}
				//}
				//if (pos == 10) {
				//	for (int i = 0; i < nAlloc; i++) {
				//		fprintf(filtmp, "%d:%.3lf\n", i, arrFloat[i]);
				//	}
				//	fprintf(filtmp, "\n\n");
				//	fprintf(filtmp, "%s\n", (*value).c_str());
				//	fclose(filtmp);
				//}
				pos2 = 0;
				
				for (i3 = 0; i3 < nBands; i3++) {
					for (i4 = nBlockRows * i1; i4 < nBlockRows * (i1 + 1); i4++) {
						for (i5 = nBlockCols * i2; i5 < nBlockCols * (i2 + 1); i5++) {
							if (i4 < model.weather[ii].nRows && i5 < model.weather[ii].nCols) {
								model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4] = arrFloat[pos2];
							}
							pos2++;
						}
					}
				}
				
			}
		}

		auto tid1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
		printf("read and translate redis data from %s took %.3lf\n", model.weather[ii].weatherFileTypeName, fp_ms);

		//lat = 74.4812;
		//lon = 116.0802;
		//rowDbl = (model.weather[ii].maxY - lat) / model.weather[ii].size_row;
		//colDbl = get_colDblFromWeatherFile(ii, lon);
		//i4 = (int)rowDbl;
		//i5 = (int)colDbl;
		//for (i3 = 0; i3 < nBands && i3 < 10; i3++) {
		//	printf("%s lon/lat %.2lf %.2lf i345 %d %d %d val %.3lf %.3lf\n", model.weather[ii].weatherFileTypeName, lon, lat, i3, i4, i5,
		//		model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4],
		//		model.weather[ii].valueCell[i3][i5 + 1 + model.weather[ii].nCols * (i4+1)]);
		//}


		//model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]));
	}






//#endif


}

int redisSetKeys_old(std::string inputPath) {
	int ii, i1, i2, i3, i4, i5, xPos1, yPos0, yPos1, nBands;
	size_t nAlloc;
	int nBlockRows, nBlockCols, pos, pos2;
	double maxLong, xPosFrac, size_col, size_row, lat, lon, rowDbl, colDbl;
	float* arrFloat;
	std::string keyID;
	json mData;

	std::list<int> listOfInts;

	stringstream stream, stream2;
	stream.precision(3);
	stream << fixed;
	//stream2.precision(3);
	//stream2 << fixed;

	//printf("test1\n");

	//model.params.variableFileName = inputPath;
	model.params.indataPath = inputPath;
	loadVariables(1);
	//printf("test1b\n");
	Raster test;

	printf("opening redis\n");
	auto redis = Redis("tcp://127.0.0.1:6379/1");

	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		size_col = -1;
		maxLong = -9999;
		for (i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			if (maxLong < model.weather[ii].filePos[i1].maxX)
				maxLong = model.weather[ii].filePos[i1].maxX;
		}
		for (i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			//printf("test1 %d\n", i1);
			model.weather[ii].rasterPos[i1].open(model.weather[ii].filePos[i1].fileName);
			if (i1 == 0) {
				//printf("test1\n");
				size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
				model.weather[ii].size_col = size_col;
				model.weather[ii].minX = model.weather[ii].filePos[i1].minX; // model.weather[ii].rasterPos[i1].Get_minLongitude();
				xPosFrac = (maxLong - model.weather[ii].minX) /
					size_col;
				xPos1 = roundUp(xPosFrac);
				model.weather[ii].maxX = model.weather[ii].minX +
					xPos1 * size_col;
				model.weather[ii].nCols = xPos1 + 1;

				size_row = model.weather[ii].rasterPos[i1].Get_sizeRow();
				model.weather[ii].size_row = size_row;
				yPos0 = 0;
				model.weather[ii].maxY = model.weather[ii].rasterPos[i1].Get_maxLatitude();
				yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
				model.weather[ii].minY = model.weather[ii].maxY - yPos1 * size_row;
				model.weather[ii].nRows = yPos1 + 1;

				nBands = model.weather[ii].rasterPos[i1].Get_nBands();
				model.weather[ii].nTimeIntervals = nBands;
				model.weather[ii].secondsUTC = (long long*)malloc(nBands * sizeof(long long));
				model.weather[ii].valueCell = (float**)malloc(nBands * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (int i2 = 0; i2 < nBands; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc(nAlloc * sizeof(float));
				}
			}
			else {
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
					errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
					errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
			}

			//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
			//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
			//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
			model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0);
			if (ii == 2) {


			}
		}
		//printf("test1d %d\n", ii);
		nBlockRows = roundUp((double)model.weather[ii].nRows / model.weather[ii].nBlock_y);
		nBlockCols = roundUp((double)model.weather[ii].nCols / model.weather[ii].nBlock_x);
		pos = 0;
		nAlloc = nBands * nBlockRows * nBlockCols;
		printf("nAlloc = %d nBands %d nBlockCols/Rows %d %d\n", 
			nAlloc, nBands, nBlockCols, nBlockRows);
		if (nAlloc > 1500000000) {
			nBlockRows = roundUp(1500000.0 / nBands / nBlockCols);
			errlog("ERROR! Too much data per key, I increase the number of y blocks from %d to %d\n",
				model.weather[ii].nBlock_y, roundUp((double)(model.weather[ii].nRows / nBlockRows)));
			printf("ERROR! Too much data per key, I increase the number of y blocks from %d to %d\n",
				model.weather[ii].nBlock_y, roundUp((double)(model.weather[ii].nRows / nBlockRows)));
			model.weather[ii].nBlock_y = roundUp((double)(model.weather[ii].nRows / nBlockRows));
			nAlloc = nBands * nBlockRows * nBlockCols;
		}

		printf("Adding redis keys for weather parameter %s nAlloc %d nBlock xy %d %d\n", 
			model.weather[ii].weatherFileTypeName, nAlloc,
			model.weather[ii].nBlock_x, model.weather[ii].nBlock_y);
		arrFloat = (float*)malloc(nAlloc * sizeof(float));
		for (i1 = 0; i1 < model.weather[ii].nBlock_y; i1++) {
			for (i2 = 0; i2 < model.weather[ii].nBlock_x; i2++) {
				//if(ii >= 2)
				//	printf("block %d %d av %d %d\n", i1, i2,
				//		model.weather[ii].nBlock_y, model.weather[ii].nBlock_x);
				pos2 = 0;
				for (i3 = 0; i3 < nBands; i3++) {
					for (i4 = nBlockRows * i1; i4 < nBlockRows * (i1 + 1); i4++) {
						for (i5 = nBlockCols * i2; i5 < nBlockCols * (i2 + 1); i5++) {
							//if (i3 == 0 && i4 == 194 && i5 == 3701) {
							//	printf("i345 %d %d %d i12 %d %d pos %d pos2 %d val %.4lf\n",
							//		i3, i4, i5, i1, i2, pos, pos2,
							//		model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4]);
							//}
							if (i4 < model.weather[ii].nRows && i5 < model.weather[ii].nCols)
								arrFloat[pos2] = model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4];
							else
								arrFloat[pos2] = -9999.9;
							//if (pos == 10) {
							//	//stream << pos2 << ":" << arrFloat[pos2] << " ";
							//	stream << arrFloat[pos2] << " ";
							//}
							pos2++;
						}
						//if (pos == 0) 
						//	stream << "\n";
					}
				}
				//printf("pos %d %.3lf %.3lf %.3lf %.3lf\n", pos, arrFloat[0], arrFloat[1], arrFloat[384920], arrFloat[384921]);
				//if (ii >= 2)
				//	printf("setting key\n");
				keyID.assign(model.weather[ii].weatherFileTypeName);
				keyID.append(":");
				keyID += to_string(pos);
				//printf("Setting key %s for i3 0 %d i4 %d %d i5 %d %d pos %d\n", keyID.c_str(),
				//	nBands, nBlockRows * i1, i4-1, nBlockCols * i2, i5-1, pos);
				//if (pos == 10) {
				//	redis.set(keyID, stream.str());
				//	printf("pos %d arrFloat[471839] = %.3lf\n", pos, arrFloat[471839]);
				//}
				//else
				//if (ii >= 2)
				//	printf("setting key2\n");
				redis.set(keyID, string_view(reinterpret_cast<const char*>(arrFloat), nAlloc * sizeof(float)));
				//if (ii >= 2)
				//	printf("done\n");
				pos++;
			}
		}
		free(arrFloat);

		printf("ii %d nBands %d\n", ii, nBands);
		if (ii == 2) {
			lat = 74.4812;
			lon = 116.0802;
			rowDbl = (model.weather[ii].maxY - lat) / model.weather[ii].size_row;
			colDbl = get_colDblFromWeatherFile(ii, lon);
			i4 = (int)rowDbl;
			i5 = (int)colDbl;
			for (i3 = 0; i3 < nBands && i3 < 10; i3++) {
				printf("%s lon/lat %.2lf %.2lf i345 %d %d %d val %.4f\n", model.weather[ii].weatherFileTypeName, lon, lat, i3, i4, i5,
					model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4]);
			}
		}

		printf("setting metadata\n");
		keyID.assign(model.weather[ii].weatherFileTypeName);
		keyID.append(":metaData");
		//redis.set(keyID, to_string(model.weather[ii].nCols));
		mData["nCols"] = model.weather[ii].nCols;
		mData["nRows"] = model.weather[ii].nRows;
		mData["size_col"] = model.weather[ii].size_col;
		mData["size_row"] = model.weather[ii].size_row;
		mData["minX"] = model.weather[ii].minX;
		mData["maxX"] = model.weather[ii].maxX;
		mData["minY"] = model.weather[ii].minY;
		mData["maxY"] = model.weather[ii].maxY;
		mData["nBlockRows"] = nBlockRows;
		mData["nBlockCols"] = nBlockCols;
		mData["nTimeIntervals"] = model.weather[ii].nTimeIntervals;
		listOfInts.clear();
		for (int i = 0; i < model.weather[ii].nTimeIntervals; i++)
			listOfInts.push_back(model.weather[ii].secondsUTC[i]);
		printf("metaData\n%s\n", mData.dump().c_str());
		mData["UTCtimes"] = listOfInts;

		//printf("Setting key %s\n", keyID.c_str());
		redis.set(keyID, mData.dump());
		printf("Setting of key %s done\n", keyID.c_str());
		
		if (ii == 2) {
			lat = 9.8;
			lon = 91.96;
			rowDbl = (model.weather[ii].maxY - lat) / model.weather[ii].size_row;
			colDbl = get_colDblFromWeatherFile(ii, lon);
			i4 = (int)rowDbl;
			i5 = (int)colDbl; 
			for (i3 = 0; i3 < nBands && i3 < 10; i3++) {
				printf("%s lon/lat %.2lf %.2lf i345 %d %d %d val %.3lf %.3lf\n", model.weather[ii].weatherFileTypeName, lon, lat, i3, i4, i5,
					model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4],
					model.weather[ii].valueCell[i3][i5 + 1 + model.weather[ii].nCols * (i4 + 1)]);
			}
		}


	}





	return 0;
}

int redisSetKeys(std::string inputPath) {
	int ii, i1, i2, i3, i4, i5, xPos1, yPos0, yPos1, nBands;
	size_t nAlloc;
	int nBlockRows, nBlockCols, pos, pos2, manad, dag;
	double maxLong, xPosFrac, size_col, size_row, lat, lon, rowDbl, colDbl;
	float* arrFloat;
	std::string keyID, histFileName;
	json mData;
	long long nSecondsUTC_last, nSecondsHistory_first, nSecondsHistory_firstStartDay;
	long long lastSecondUTC_needed, nExtraSecondsNeeded, nSecondsUTC_first, secondsNow;
	int nDaysNeeded_history, openOK, zNu, iY, iX, pos1;

	resultPath = inputPath;
	reset_errlog();

	FILE* filcheck = fopen("data/checkWeatherData_tmp.txt", "w");

	FILE* filCheck2 = fopen("data/checkWeather2.txt", "w");

	std::list<int> listOfInts;

	stringstream stream, stream2;
	stream.precision(3);
	stream << fixed;
	//stream2.precision(3);
	//stream2 << fixed;

	//printf("test1\n");

	//model.params.variableFileName = inputPath;

	initModelStatusValues();

	model.params.indataPath = inputPath;
	model.params.errorCode = 0;
	loadParams_theRestOld(&(model.params));
	loadVariables(1);
	//printf("test1b\n");
	Raster test;

	printf("opening redis\n");
	auto redis = Redis("tcp://127.0.0.1:6379/1");

	lastSecondUTC_needed = make_gmtime_now() + model.params.longestRouteDays_history * 3600 * 24;

	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		size_col = -1;
		maxLong = -9999;
		//for (i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
		//	if (maxLong < model.weather[ii].filePos[i1].maxX)
		//		maxLong = model.weather[ii].filePos[i1].maxX;
		//}
		for (i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			//printf("test1 %d\n", i1);
			openOK = model.weather[ii].rasterPos[i1].open(model.weather[ii].filePos[i1].fileName);
			printf("openOK %d\n", openOK);
			if (openOK != 1) {
				errlog("ERROR! Could not open forecast file %s. This one must exist. I quit\n",
					model.weather[ii].filePos[i1].fileName);
				exitKontrollerat(__LINE__);
			}
			nBands = model.weather[ii].rasterPos[i1].Get_nBands();
			model.weather[ii].filePos[i1].minX = model.weather[ii].rasterPos[i1].Get_minLongitude();
			model.weather[ii].filePos[i1].maxX = model.weather[ii].rasterPos[i1].Get_maxLongitude();
			if (maxLong < model.weather[ii].filePos[i1].maxX)
				maxLong = model.weather[ii].filePos[i1].maxX;
			if (i1 == 0) {
				nSecondsUTC_last = model.weather[ii].rasterPos[i1].GetMetaData_nSecondsUTC_last(&nSecondsUTC_first);
				nSecondsHistory_first = nSecondsUTC_last + model.weather[ii].timeIntervall_h * 3600;
				nSecondsHistory_firstStartDay = getFirstSecondOfDay(nSecondsHistory_first);

				nExtraSecondsNeeded = lastSecondUTC_needed - nSecondsHistory_first;
				nDaysNeeded_history = (int)(nExtraSecondsNeeded / 3600 / 24) + 1;

				//printf("test1\n");
				size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
				model.weather[ii].size_col = size_col;
				model.weather[ii].minX = model.weather[ii].filePos[i1].minX; // model.weather[ii].rasterPos[i1].Get_minLongitude();
				xPosFrac = (maxLong - model.weather[ii].minX) /
					size_col;
				xPos1 = roundUp(xPosFrac);
				model.weather[ii].maxX = model.weather[ii].minX +
					xPos1 * size_col;
				model.weather[ii].nCols = xPos1 + 1;

				size_row = model.weather[ii].rasterPos[i1].Get_sizeRow();
				model.weather[ii].size_row = size_row;
				yPos0 = 0;
				model.weather[ii].maxY = model.weather[ii].rasterPos[i1].Get_maxLatitude();
				yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
				model.weather[ii].minY = model.weather[ii].maxY - yPos1 * size_row;
				model.weather[ii].nRows = yPos1 + 1;

				printf("nBands %d nDaysHistory %d\n", nBands, nDaysNeeded_history);
				model.weather[ii].nTimeIntervals_forecast = nBands;
				model.weather[ii].nTimeIntervals = nBands + nDaysNeeded_history;
				model.weather[ii].secondsUTC = (long long*)malloc((nBands + nDaysNeeded_history) * sizeof(long long));
				model.weather[ii].valueCell = (float**)malloc((nBands + nDaysNeeded_history) * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (i2 = 0; i2 < nBands + nDaysNeeded_history; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc(nAlloc * sizeof(float));
					for (int i3 = 0; i3 < nAlloc; i3++)
						model.weather[ii].valueCell[i2][i3] = 9999;
					model.weather[ii].secondsUTC[i2] = -1;
				}
				model.weather[ii].errorCode = 0;
			}
			else {
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001 ||
					abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
					if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
						errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
							model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
					if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
						errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
							model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
					model.weather[ii].errorCode = 3;
					continue;
				}
				if (model.weather[ii].nTimeIntervals_forecast != nBands) {
					errlog("ERROR! Different number of bands for weather parameter %s, %d and %d. Must be the same. I quit\n",
						model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals_forecast, nBands);
					model.weather[ii].errorCode = 2;
					continue;
				}
			}
			//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
			//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
			//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
			model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0);

			for (i2 = 0; i2 < nDaysNeeded_history; i2++) {
				if (i2 == 0)
					secondsNow = nSecondsHistory_first + i2 * 24 * 3600;
				else
					secondsNow = nSecondsHistory_firstStartDay + i2 * 24 * 3600;

				manad = getManadDagFranUTCSeconds(secondsNow, &dag);
				
				histFileName = model.weather[ii].filePos[i1].fileName;
				pos = histFileName.find_last_of("/\\");
				pos1 = histFileName.rfind("_resampled");
				if (pos1 > pos && pos1 < histFileName.size() - 1) {
					// remove this part of the name as it is not part of resampled average
					//printf("pos %d pos1 %d of  %s size %d\n", pos, pos1, histFileName.c_str(), histFileName.size());
					histFileName.erase(pos1, 10);
					//histFileName.erase(pos1 - 12, 10);
					//printf("%s\n", histFileName.c_str());
				}
				histFileName.insert(pos, "/Monthly/");
				pos += 9;
				if (manad < 10) {
					histFileName.insert(pos, "0");
					histFileName.insert(pos + 1, std::to_string(manad));
				}else
					histFileName.insert(pos, std::to_string(manad));
				//pos += 2;
				pos = histFileName.find_last_of(".");
				if (dag < 10) {
					histFileName.insert(pos, "_0");
					histFileName.insert(pos + 2, std::to_string(dag));
				}
				else {
					histFileName.insert(pos, "_");
					histFileName.insert(pos + 1, std::to_string(dag));
				}
				pos += 3;
				if (manad < 10) {
					histFileName.insert(pos, "_0_avg");
					histFileName.insert(pos + 2, std::to_string(manad));
				}
				else {
					histFileName.insert(pos, "__avg");
					histFileName.insert(pos + 1, std::to_string(manad));
				}
				openOK = model.weather[ii].rasterPos[i1].open(histFileName.c_str());
				if (openOK == 1) {
					if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001 ||
						abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
						if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
							errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
								model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
						if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
							errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
								model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
						model.weather[ii].errorCode = 4;
						continue;
					}
					//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
					//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
					//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
					model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), model.weather[ii].nTimeIntervals_forecast + i2);
					printf("history day %d changes secondsUTC from %I64d to %I64d\n",
						model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2], secondsNow);
				}
				else {
					errlog("ERROR! Failed to open %s. I use weather data from the previous loaded file\n",
						histFileName.c_str());
					zNu = model.weather[ii].nTimeIntervals_forecast + i2;
					for (iY = 0; iY < model.weather[ii].nRows; iY++) {
						for (iX = 0; iX < model.weather[ii].nCols; iX++) {
							model.weather[ii].valueCell[zNu][iX + model.weather[ii].nCols * iY] = 
								model.weather[ii].valueCell[zNu-1][iX + model.weather[ii].nCols * iY];
						}
					}

					(model.status.weatherHistoryOpenFile_fail)++;
					printf("ERROR! Failed to open %s, nFailed %d. I use weather data from the previous loaded file. secondsNow %I64d\n",
						histFileName.c_str(), model.status.weatherHistoryOpenFile_fail, secondsNow);
				}
				model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
			}


		}

		fprintf(filcheck, "\nvar %d %s tidsperioder\n", ii, model.weather[ii].weatherFileTypeName);
		for (i1 = 0; i1 < model.weather[ii].nTimeIntervals; i1++) {
			stringDateFromUTCSeconds(model.weather[ii].secondsUTC[i1]);
			fprintf(filcheck, "%d %I64d %s\n", i1, model.weather[ii].secondsUTC[i1],
				stringDateFromUTCSeconds(model.weather[ii].secondsUTC[i1]).c_str());
		}

		if (model.weather[ii].nRows > 5) {
			fprintf(filcheck, "\nvar %d %s tidp 0 rad 5 varden i kolumner\n", ii, model.weather[ii].weatherFileTypeName);
			for (i1 = 0; i1 < model.weather[ii].nCols; i1++) {
				fprintf(filcheck, "%d midp_xy %.3lf %.3lf varde %f\n", i1,
					(i1 + 0.5) * model.weather[ii].size_col + model.weather[ii].minX,
					model.weather[ii].maxY - 4.5 * model.weather[ii].size_row,
					model.weather[ii].valueCell[0][i1 + model.weather[ii].nCols * 4]);
			}
			if (nDaysNeeded_history > 0) {
				fprintf(filcheck, "\nvar %d %s tidp %d (first historic) rad 5 varden i kolumner\n", ii, model.weather[ii].weatherFileTypeName, 
					model.weather[ii].nTimeIntervals_forecast);
				for (i1 = 0; i1 < model.weather[ii].nCols; i1++) {
					fprintf(filcheck, "%d midp_xy %.3lf %.3lf varde %f\n", i1, 
						(i1 + 0.5) * model.weather[ii].size_col + model.weather[ii].minX,
						model.weather[ii].maxY - 4.5 * model.weather[ii].size_row,
						model.weather[ii].valueCell[model.weather[ii].nTimeIntervals_forecast][i1 + model.weather[ii].nCols * 4]);
				}

			}
		}
		//printf("minX %.4lf\n", model.weather[ii].minX);

		//printf("test1d %d\n", ii);
		nBlockRows = roundUp((double)model.weather[ii].nRows / model.weather[ii].nBlock_y);
		nBlockCols = roundUp((double)model.weather[ii].nCols / model.weather[ii].nBlock_x);
		pos = 0;
		nAlloc = model.weather[ii].nTimeIntervals * nBlockRows * nBlockCols;
		printf("nAlloc = %d nTimePeriods %d nBlockCols/Rows %d %d\n",
			nAlloc, model.weather[ii].nTimeIntervals, nBlockCols, nBlockRows);
		if (nAlloc > 1500000000) {
			nBlockRows = roundUp(1500000.0 / nBands / nBlockCols);
			errlog("ERROR! Too much data per key, I increase the number of y blocks from %d to %d\n",
				model.weather[ii].nBlock_y, roundUp((double)(model.weather[ii].nRows / nBlockRows)));
			printf("ERROR! Too much data per key, I increase the number of y blocks from %d to %d\n",
				model.weather[ii].nBlock_y, roundUp((double)(model.weather[ii].nRows / nBlockRows)));
			model.weather[ii].nBlock_y = roundUp((double)(model.weather[ii].nRows / nBlockRows));
			nAlloc = model.weather[ii].nTimeIntervals * nBlockRows * nBlockCols;
		}

		printf("Adding redis keys for weather parameter %s nAlloc %d nBlock xy %d %d\n",
			model.weather[ii].weatherFileTypeName, nAlloc,
			model.weather[ii].nBlock_x, model.weather[ii].nBlock_y);
		arrFloat = (float*)malloc(nAlloc * sizeof(float));
		if (ii == 3)
			fprintf(filCheck2, "ii %d nTimePeriods %d nBlockRows %d nBlockCols %d\n", ii, model.weather[ii].nTimeIntervals, nBlockRows, nBlockCols);
		for (i1 = 0; i1 < model.weather[ii].nBlock_y; i1++) {
			for (i2 = 0; i2 < model.weather[ii].nBlock_x; i2++) {
				//if(ii >= 2)
				//	printf("block %d %d av %d %d\n", i1, i2,
				//		model.weather[ii].nBlock_y, model.weather[ii].nBlock_x);
				pos2 = 0;
				for (i3 = 0; i3 < model.weather[ii].nTimeIntervals; i3++) {
					for (i4 = nBlockRows * i1; i4 < nBlockRows * (i1 + 1); i4++) {
						for (i5 = nBlockCols * i2; i5 < nBlockCols * (i2 + 1); i5++) {
							//if (i3 == 0 && i4 == 194 && i5 == 3701) {
							//	printf("i345 %d %d %d i12 %d %d pos %d pos2 %d val %.4lf\n",
							//		i3, i4, i5, i1, i2, pos, pos2,
							//		model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4]);
							//}
							if (i4 < model.weather[ii].nRows && i5 < model.weather[ii].nCols)
								arrFloat[pos2] = model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4];
							else
								arrFloat[pos2] = -9999.9;
							if (ii == 3 && i1 == 3 && i2 == 7)
								fprintf(filCheck2, "%.3lf ", arrFloat[pos2]);
							//if (pos == 10) {
							//	//stream << pos2 << ":" << arrFloat[pos2] << " ";
							//	stream << arrFloat[pos2] << " ";
							//}
							pos2++;
						}
						if (ii == 3 && i1 == 3 && i2 == 7)
							fprintf(filCheck2, "\n");
						//if (pos == 0) 
						//	stream << "\n";
					}
				}
				//printf("pos %d %.3lf %.3lf %.3lf %.3lf\n", pos, arrFloat[0], arrFloat[1], arrFloat[384920], arrFloat[384921]);
				//if (ii >= 2)
				//	printf("setting key\n");
				keyID.assign(model.weather[ii].weatherFileTypeName);
				keyID.append(":");
				keyID += to_string(pos);
				//printf("Setting key %s for i3 0 %d i4 %d %d i5 %d %d pos %d\n", keyID.c_str(),
				//	nBands, nBlockRows * i1, i4-1, nBlockCols * i2, i5-1, pos);
				//if (pos == 10) {
				//	redis.set(keyID, stream.str());
				//	printf("pos %d arrFloat[471839] = %.3lf\n", pos, arrFloat[471839]);
				//}
				//else
				//if (ii >= 2)
				//	printf("setting key2\n");
				redis.set(keyID, string_view(reinterpret_cast<const char*>(arrFloat), nAlloc * sizeof(float)));
				//if (ii >= 2)
				//	printf("done\n");
				pos++;
			}
		}
		free(arrFloat);

		printf("ii %d nBands %d nTimePeriods %d\n", ii, nBands, model.weather[ii].nTimeIntervals);
		if (ii == 2) {
			lat = 74.4812;
			lon = 116.0802;
			rowDbl = (model.weather[ii].maxY - lat) / model.weather[ii].size_row;
			colDbl = get_colDblFromWeatherFile(ii, lon);
			i4 = (int)rowDbl;
			i5 = (int)colDbl;
			for (i3 = 0; i3 < model.weather[ii].nTimeIntervals && i3 < 10; i3++) {
				printf("%s lon/lat %.2lf %.2lf i345 %d %d %d val %.4f\n", model.weather[ii].weatherFileTypeName, lon, lat, i3, i4, i5,
					model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4]);
			}
		}

		printf("setting metadata\n");
		keyID.assign(model.weather[ii].weatherFileTypeName);
		keyID.append(":metaData");
		//redis.set(keyID, to_string(model.weather[ii].nCols));
		mData["nCols"] = model.weather[ii].nCols;
		mData["nRows"] = model.weather[ii].nRows;
		mData["size_col"] = model.weather[ii].size_col;
		mData["size_row"] = model.weather[ii].size_row;
		mData["minX"] = model.weather[ii].minX;
		mData["maxX"] = model.weather[ii].maxX;
		mData["minY"] = model.weather[ii].minY;
		mData["maxY"] = model.weather[ii].maxY;
		mData["nBlockRows"] = nBlockRows;
		mData["nBlockCols"] = nBlockCols;
		mData["nTimeIntervals"] = model.weather[ii].nTimeIntervals;
		mData["nTimeIntervals_forecast"] = model.weather[ii].nTimeIntervals_forecast;
		listOfInts.clear();
		for (int i = 0; i < model.weather[ii].nTimeIntervals; i++) {
			listOfInts.push_back(model.weather[ii].secondsUTC[i]);
			errlog("%s timeInt %d sec %I64d %s\n",
				model.weather[ii].weatherFileTypeName, i, model.weather[ii].secondsUTC[i],
				stringDateFromUTCSeconds(model.weather[ii].secondsUTC[i]).c_str());
		}

		printf("metaData\n%s\n", mData.dump().c_str());
		mData["UTCtimes"] = listOfInts;

		//printf("Setting key %s\n", keyID.c_str());
		redis.set(keyID, mData.dump());
		printf("Setting of key %s done\n", keyID.c_str());

		if (ii == 2) {
			lat = 9.8;
			lon = 91.96;
			rowDbl = (model.weather[ii].maxY - lat) / model.weather[ii].size_row;
			colDbl = get_colDblFromWeatherFile(ii, lon);
			i4 = (int)rowDbl;
			i5 = (int)colDbl;
			for (i3 = 0; i3 < model.weather[ii].nTimeIntervals && i3 < 10; i3++) {
				printf("%s lon/lat %.2lf %.2lf i345 %d %d %d val %.3lf %.3lf\n", model.weather[ii].weatherFileTypeName, lon, lat, i3, i4, i5,
					model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4],
					model.weather[ii].valueCell[i3][i5 + 1 + model.weather[ii].nCols * (i4 + 1)]);
			}
		}
		if (model.weather[ii].errorCode != 0)
			model.params.errorCode = 1;


	}

	fclose(filcheck);
	fclose(filCheck2);

	if (model.params.errorCode != 0)
		errlog("ERROR! Generation of redis keys failed\n");
	else
		errlog("Generation of redis keys successful\n");

	return 0;
}

/*
int loadStormsData()
{
	int i, nQuadrants, pos;
	std::ifstream fil;
	json data, dataSpeed, dataGeom;
	json dataIt2, dataCoord, dataIt3;
	std::string namn;
	char* namn2;
	namn2 = (char*)malloc(256 * sizeof(char));

	for (i = 0; i < model.nStorms; i++) {
		sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.storms[i].fileName);
		printf("opens %s\n", namn2);
		fil.open(namn2);
		//fil.open(model.storms[i].fileName);
		fil >> data;
		model.storms[i].nFeatures = 0;
		if (!data["features"].is_null()) {
			json dataFeature = data["features"];
			model.storms[i].feature = (strStormFeature*)malloc(dataFeature.size() * sizeof(strStormFeature));
			model.storms[i].nFeatures = 0;
			json dataIt;
			for (auto it = dataFeature.begin(); it != dataFeature.end(); ++it) {
				dataIt = it.value();
				dataGeom = dataIt["geometry"];
				dataCoord = dataGeom["coordinates"];
				for (auto it2 = dataCoord.begin(); it2 != dataCoord.end(); ++it2) {
					dataIt2 = it2.value();
					pos = 0;
					for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
						dataIt3 = it3.value();
						// get midpoint
						if (pos == 0)
							model.storms[i].feature[model.storms[i].nFeatures].lon = dataIt3;
						else
							model.storms[i].feature[model.storms[i].nFeatures].lat = dataIt3;
						pos++;
					}
				}
				model.storms[i].feature[model.storms[i].nFeatures].midPoint = spherical::Point(model.storms[i].feature[model.storms[i].nFeatures].lat,
					model.storms[i].feature[model.storms[i].nFeatures].lon);
				if (!dataIt["properties"].is_null()) {
					json dataProp = dataIt["properties"];
					// get date
					namn = dataProp["Date"];
					model.storms[i].feature[model.storms[i].nFeatures].datum = 6 * model.storms[i].nFeatures; // tid i timmar sedan starten// getDatumFranString(namn);
					// get maxWind
					model.storms[i].feature[model.storms[i].nFeatures].maxWind = dataProp["MaxWind"];
					// get quadrants
					json dataQuad = dataProp["Quadrants"];
					model.storms[i].feature[model.storms[i].nFeatures].quadrant = (strStormQuadr*)malloc(dataQuad.size() * sizeof(strStormQuadr));
					nQuadrants = 0;
					for (auto it2 = dataQuad.begin(); it2 != dataQuad.end(); ++it2) {
						dataIt2 = it2.value();
						// ne, se, sw, nw
						model.storms[i].feature[model.storms[i].nFeatures].quadrant[nQuadrants].NE = dataIt2["NE"];
						model.storms[i].feature[model.storms[i].nFeatures].quadrant[nQuadrants].SE = dataIt2["SE"];
						model.storms[i].feature[model.storms[i].nFeatures].quadrant[nQuadrants].SW = dataIt2["SW"];
						model.storms[i].feature[model.storms[i].nFeatures].quadrant[nQuadrants].NW = dataIt2["NW"];
						// windMaxRadius
						model.storms[i].feature[model.storms[i].nFeatures].quadrant[nQuadrants].windMaxRadius = dataIt2["WindMaxRadius"];
						nQuadrants++;
					}
					model.storms[i].feature[model.storms[i].nFeatures].nQuadrants = nQuadrants;
				}
				(model.storms[i].nFeatures)++;
			}
		}
		fil.close();

		model.storms[i].tidsIntervall = 6; //
		errlog("OBS! Setting storm time intervall to 6 hours, if it can vary then fix this!\n");
		for (pos = 0; pos < model.storms[i].nFeatures; pos++) {
			if (pos < model.storms[i].nFeatures - 1)
				model.storms[i].feature[pos].bearing = model.storms[i].feature[pos].midPoint.bearingTo(model.storms[i].feature[pos + 1].midPoint);
			else {
				if (pos > 0)
					model.storms[i].feature[pos].bearing = model.storms[i].feature[pos - 1].midPoint.finalBearingTo(model.storms[i].feature[pos].midPoint);
				else
					model.storms[i].feature[pos].bearing = 0;
			}
			//errlog("storm %d feature %d maxWind %.1lf\n", i, pos, model.storms[i].feature[pos].maxWind);
			//for (int i2 = 0; i2 < model.storms[i].feature[pos].nQuadrants; i2++)
			//	errlog("\tquadrant %d %.1lf\n", i2, model.storms[i].feature[pos].quadrant[i2].windMaxRadius);
		}
	}

	return 0;
}
*/

/*
int loadWeatherData()
{
	// not used
	json data, dataIt, dataInt, dataFile;
	int nAllocVar, i, nFiles;
	std::string typeName, namn;
	char* namn2;
	namn2 = (char*)malloc(256 * sizeof(char));



	std::ifstream fil;
	for (int i0 = 0; i0 < model.nWeatherFiles; i0++) {
		sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.weather[i0].weatherFileTypeName);
		printf("opens %s\n", namn2);
		fil.open(namn2);
		if (!fil.is_open()) {
			errlog("ERROR! Could not open the file %s with information about weather.\n"
				"It must be possible open it as it has information for at least one of the variables given in %s.\n I quit.\n",
				model.weather[i0].weatherFileTypeName, model.params.variableFileName);
			exit(0);
		}
		fil >> data;
		// model.weather[i0].nElement = data["nElement"];
		model.weather[i0].timeIntervall_h = data["timeIntervall_h"];
		dataInt = data["timeOrder"];
		model.weather[i0].nTimeIntervals = (int)dataInt.size();
		if (model.weather[i0].nTimeIntervals != data["nTimeIntervals"]) {
			errlog("ERROR! wrong number of timeIntervals (%d) given in %s. I use the number used in timeOrder (%d)\n",
				data["nTimeIntervals"], model.weather[i0].nTimeIntervals);
		}
		model.weather[i0].timeOrder = (int*)malloc(model.weather[i0].nTimeIntervals * sizeof(int));
		model.weather[i0].useStandardWeather = model.params.useStandardWeather; // -1 for standard 0, 1 for standard last, 0 for changing forecast
		if (model.weather[i0].useStandardWeather == 1)
			errlog("ERROR OBS! I use the first time periods weather always for weatherType %d\n", i0);
		i = 0;
		for (auto it = dataInt.begin(); it != dataInt.end(); ++it) {
			//dataIt = it.value();
			//std::cout << dataIt.dump() << '\n';
			model.weather[i0].timeOrder[i] = it.value();
			i++;
		}

		model.weather[i0].minX = data["minX"];
		model.weather[i0].minY = data["minY"];
		model.weather[i0].maxX = data["maxX"];
		model.weather[i0].maxY = data["maxY"];
		//dataFile = data["files"];
		//model.weather[i0].nFiles = (int)dataFile.size();
		//model.weather[i0].filePos = (strFileWeather*)malloc(model.weather[i0].nFiles * sizeof(strFileWeather));
		//model.weather[i0].rasterPos = (Raster*)malloc(model.weather[i0].nFiles * sizeof(Raster));
		//model.weather[i0].rasterBandData = (float****)malloc(model.weather[i0].nFiles * sizeof(float***));
		//model.weather[i0].rasterBandDataNy2 = (double***)malloc(model.weather[i0].nFiles * sizeof(double**));
		//nFiles = data["nFiles"];
		//if (model.weather[i0].nFiles != nFiles) {
		//	errlog("ERROR! wrong number of files (%d) given in %s. I use the given files (%d)\n",
		//		nFiles, model.weather[i0].weatherFileTypeName, model.weather[i0].nFiles);
		//}
		//i = 0;
		//for (auto it = dataFile.begin(); it != dataFile.end(); ++it) {
		//	dataIt = it.value();
		//namn = dataIt["fileName"];
		namn = data["fileName"];
		sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), namn.c_str());
		model.weather[i0].fileName = str_alloc_cpy(namn2);
		model.weather[i0].rasterBandData = (float***)calloc(
			model.weather[i0].nElement * model.weather[i0].nTimeIntervals, sizeof(float**));
		fil.close();
	}

	return 0;
}
*/

double calc_haversine_dist_latlon(double lat1, double lon1, double lat2, double lon2)
{
	double R = 6371e3; // metres
	double vinkel1 = lat1 * M_PI / 180; // vinkel and lambda in radians
	double vinkel2 = lat2 * M_PI / 180;
	double delta = (lat2 - lat1) * M_PI / 180;
	double deltalambda = (lon2 - lon1) * M_PI / 180;

	double a = sin(delta / 2) * sin(delta / 2) +
		cos(vinkel1) * cos(vinkel2) *
		sin(deltalambda / 2) * sin(deltalambda / 2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));

	double d = R * c; // in metres
	return d; //  0.0;
}

//int initCoordUsage()
//{
//	for (int i = 0; i < 4; i++)
//		model.network.useLongitudeKvadrant[i] = 0;
//	return 0;
//}
//int updateCoordUsage(spherical::Point point)
//{
//	double longDbl;
//	int longKvadrant;
//	longDbl = 180 + point.longitude().degrees();
//	longDbl /= 90;
//	longKvadrant = (int)longDbl;
//	model.network.useLongitudeKvadrant[longKvadrant] = 1;
//
//	return 0;
//}

//Raster openRaster(const char *namn)
//{
//	Raster raster(namn);
//	return raster;
//}


int test_OpenTheSameRasterMultipleTimesAndRead()
{
	Raster* map;
	int nRows, nCols, i, nCopies = 1000;
	float*** raster;
	FILE* filpek;

	filpek = fopen("tmp_testFil.txt", "w");
	map = (Raster*)malloc(nCopies * sizeof(Raster));
	raster = (float***)malloc(nCopies * sizeof(float**));
	for (i = 0; i < nCopies; i++) {
		//		map[i].open(model.params.mapPhysicalFileName.c_str());
				//map[i].open("OCEANgl_-180_-90.grb");
		map[i].open("OCEANgl_0_90.grb");
		nRows = map[i].Get_nRows();
		nCols = map[i].Get_nCols();
		//fprintf(filpek, "raster %d\n", i);
		//fprintf(filpek, "nRows %d\nnCols %d\n", nRows, nCols);
		//raster[i] = map[i].GetRasterBand(1);
		//fprintf(filpek, "\nraster %d\n", i);
		printf("raster %d\n", i);
		//for (i0 = 50; i0 < 100; i0++) {
		//	fprintf(filpek, "%d", i0);
		//	for (i1 = 50; i1 < 100; i1++) {
		//		fprintf(filpek, "\t%lf", raster[i][i0][i1]);
		//	}
		//	fprintf(filpek, "\n");
		//}
	}
	fclose(filpek);

	exit(0);
	return 0;
}

int eval_stormWithinBoundingBox(int stormNr) {
	if (model.storms[stormNr].box_maxLon < model.boundingBox.xMin) {
		model.storms[stormNr].box_maxLon += 360;
		model.storms[stormNr].box_minLon += 360;
	}
	if (model.storms[stormNr].box_minLon > model.boundingBox.xMax) {
		model.storms[stormNr].box_maxLon -= 360;
		model.storms[stormNr].box_minLon -= 360;
	}
	//printf("%d stormNr %d box %.2lf %.2lf %.2lf %.2lf\narea %.2lf %.2lf %.2lf %.2lf\n", stormNr, model.storms[stormNr].stormNr,
	//	model.storms[stormNr].box_minLon, model.storms[stormNr].box_minLat,
	//	model.storms[stormNr].box_maxLon, model.storms[stormNr].box_maxLat,
	//	model.boundingBox.xMin, model.boundingBox.yMin,
	//	model.boundingBox.xMax, model.boundingBox.yMax);
	if (model.storms[stormNr].box_maxLat <= model.boundingBox.yMin)
		return 0;
	if (model.storms[stormNr].box_maxLon <= model.boundingBox.xMin)
		return 0;
	if (model.storms[stormNr].box_minLat >= model.boundingBox.yMax)
		return 0;
	if (model.storms[stormNr].box_minLon >= model.boundingBox.xMax)
		return 0;

	return 1;
}

int eval_minDistPointToPhysicalLevelPoints(double lon, double lat, int physLev) {
	int i, iMax, iMin, iMitt;
	double dist_min, dist_max, dist_mitt;
	spherical::Point point(lat, lon);
	iMin = 0;
	iMax = model.network.physicalLev[physLev].nPoints - 1;
	dist_min = point.distanceTo(model.network.physicalLev[physLev].point[iMin]);
	dist_max = point.distanceTo(model.network.physicalLev[physLev].point[iMax]);
	if (dist_min < dist_max) {
		iMitt = iMin + 1;
		dist_mitt = point.distanceTo(model.network.physicalLev[physLev].point[iMitt]);
		if (dist_min <= dist_mitt)
			return dist_min;
		iMin = iMitt;
		dist_min = dist_mitt;
	}
	else {
		iMitt = iMax - 1;
		dist_mitt = point.distanceTo(model.network.physicalLev[physLev].point[iMitt]);
		if (dist_max <= dist_mitt)
			return dist_max;
		iMax = iMitt;
		dist_max = dist_mitt;
	}
	for (i = 0; i < model.network.physicalLev[physLev].nPoints; i++) {
		iMitt = (int)(iMax + iMin) / 2;
		dist_mitt = point.distanceTo(model.network.physicalLev[physLev].point[iMitt]);
		if (dist_min < dist_max) {
			dist_max = dist_mitt;
			iMax = iMitt;
		}
		else {
			dist_min = dist_mitt;
			iMin = iMitt;
		}
		if (iMax - iMin <= 1)
			break;
	}
	if (dist_min < dist_max)
		return dist_min / 1000.0;
	else
		return dist_max / 1000.0;
}

void copyStormFeature(strStormFeature fromF, strStormFeature* toF) {
	toF->datum = fromF.datum;
	toF->maxWind = fromF.maxWind;
	toF->lat = fromF.lat;
	toF->lon = fromF.lon;
	toF->midPoint = fromF.midPoint;
	toF->UTCseconds = fromF.UTCseconds;
	toF->outerCircleSize = fromF.outerCircleSize;
	toF->innerCircleForwardSize = fromF.innerCircleForwardSize;
	toF->innerCircleBackwardsSize = fromF.innerCircleBackwardsSize;
}

void sortStormFeaturesTime(int pos) {
	int i, j, nFeaturesUse, swap;
	strStormFeature fTemp;

	for (j = 0; j < model.storms[pos].nFeatures; j++) {
		swap = 0;
		for (i = 0; i < model.storms[pos].nFeatures - 1; i++) {
			if (model.storms[pos].feature[i].UTCseconds > model.storms[pos].feature[i + 1].UTCseconds) {
				copyStormFeature(model.storms[pos].feature[i + 1], &fTemp);
				copyStormFeature(model.storms[pos].feature[i], &(model.storms[pos].feature[i + 1]));
				copyStormFeature(fTemp, &(model.storms[pos].feature[i]));
				swap = 1;
			}
		}
		if (swap == 0)
			break;
	}

	i = 0;
	for (j = 1; j < model.storms[pos].nFeatures; j++) {
		if (model.storms[pos].feature[j].UTCseconds > model.params.UTC_secondsStart) {
			//printf("stormNr %d feature %d ta bort foregaende ty sec > secStart %I64d %I64d foreg %I64d\ndatum\n%s\n%s\n%s\n",
			//	model.storms[pos].stormNr, j, model.storms[pos].feature[j].UTCseconds, model.params.UTC_secondsStart,
			//	model.storms[pos].feature[j - 1].UTCseconds,
			//	stringDateFromUTCSeconds(model.storms[pos].feature[j].UTCseconds).c_str(),
			//	stringDateFromUTCSeconds(model.params.UTC_secondsStart).c_str(),
			//	stringDateFromUTCSeconds(model.storms[pos].feature[j - 1].UTCseconds).c_str());
			i = j - 1;
			break;
		}
	}
	i = 0;
	errlog("ERROR! Remove the above row\n");
	if (i > 0) {
		for (j = i; j < model.storms[pos].nFeatures; j++) {
			copyStormFeature(model.storms[pos].feature[j], &(model.storms[pos].feature[j - i]));
		}
		model.storms[pos].nFeatures -= i;
	}
}

void addInfoToStorms(int pos) {
	int i, nExtraEndHours = 12, tidInt, nAlloc;
	long long maxTid, sekNu;
	double distNu;
	spherical::Point p1, p2;

	model.storms[pos].timeIntervall_h = 1; // 1 hour intervalls
	nAlloc = ((model.storms[pos].feature[model.storms[pos].nFeatures - 1].UTCseconds -
		model.params.UTC_secondsStart) / 3600.0 + nExtraEndHours) / model.storms[pos].timeIntervall_h;
	//printf("%d nFeatures %d UTCSec %I64d startPlanSec %I64d\n", pos,
	//	model.storms[pos].nFeatures, model.storms[pos].feature[model.storms[pos].nFeatures - 1].UTCseconds,
	//	model.params.UTC_secondsStart);
	model.storms[pos].timeIntervalIndex = (int*)malloc(nAlloc * sizeof(int));

	tidInt = 0;
	for (i = 0; i < model.storms[pos].nFeatures; i++) {
		if (i == 0) {
			p2 = spherical::Point(model.storms[pos].feature[i].lat, model.storms[pos].feature[i].lon);
		}
		if (i < model.storms[pos].nFeatures - 1) {
			p1 = p2;
			p2 = spherical::Point(model.storms[pos].feature[i + 1].lat, model.storms[pos].feature[i + 1].lon);
			distNu = p1.distanceTo(p2);
			model.storms[pos].feature[i].distanceToNextPoint = distNu;
			model.storms[pos].feature[i].bearing = p1.bearingTo(p2);
			model.storms[pos].feature[i].hoursToNextPoint = (model.storms[pos].feature[i + 1].UTCseconds -
				model.storms[pos].feature[i].UTCseconds) / 3600.0;
		}
		else {
			model.storms[pos].feature[i].distanceToNextPoint = 0;
			if (i > 0) {
				model.storms[pos].feature[i].bearing = model.storms[pos].feature[i - 1].bearing;
			}else
				model.storms[pos].feature[i].bearing = 0;
			model.storms[pos].feature[i].hoursToNextPoint = 12;
		}
		model.storms[pos].feature[i].tidFromStart_h = (model.storms[pos].feature[i].UTCseconds -
			model.params.UTC_secondsStart) / 3600.0;

		if (i == model.storms[pos].nFeatures - 1)
			maxTid = model.storms[pos].feature[i].UTCseconds + 3600 * nExtraEndHours - 1;
		else
			maxTid = model.storms[pos].feature[i + 1].UTCseconds - 1;

		//printf("stormNr %d i %d start slut %I64d %I64d optStart %I64d\ndatum\n%s\n%s\n%s\n", 
		//	model.storms[pos].stormNr, i, model.storms[pos].feature[i].UTCseconds, maxTid,
		//	model.params.UTC_secondsStart,
		//	stringDateFromUTCSeconds(model.storms[pos].feature[i].UTCseconds).c_str(),
		//	stringDateFromUTCSeconds(maxTid).c_str(),
		//	stringDateFromUTCSeconds(model.params.UTC_secondsStart).c_str());

		for (; tidInt < 100000; tidInt++) {
			if (tidInt >= nAlloc) {
				errlog("ERROR! Too many tidInt compared to allocated for stormPos %d Nr %d. I skip the rest. maxTid %I64d %s\n", pos, 
					model.storms[pos].stormNr, maxTid,
					stringDateFromUTCSeconds(maxTid).c_str());
				break;
			}
			sekNu = (long long)(tidInt * model.storms[pos].timeIntervall_h * 3600 + model.params.UTC_secondsStart);
			if (sekNu <= maxTid)
				model.storms[pos].timeIntervalIndex[tidInt] = i;
			else
				break;
			//printf("stormPos %d nr %d tidInt %d sec %I64d ger stormFeature %d (endtid %I64d)\n",
			//	pos, model.storms[pos].stormNr, tidInt, sekNu, i, maxTid);
		}
	}
	model.storms[pos].nTimeIntervals_maxValue = tidInt - 1;
	//printf("stormPos %d nr %d nAlloc %d tidInt %d (should be the same) nFeatures %d\n", pos,
	//	model.storms[pos].stormNr, nAlloc, tidInt, model.storms[pos].nFeatures);

}

void gen_infoWeatherAroundStorms() {
	int i, i1, deltaTint, i2, i3;
	spherical::Point p1, p2, p3, pOld;
	double bearing, deltaT, tidTot, distTot, distNu;
	double time1, time2, deltaDist, stormVarde, uCurrent, vCurrent;
	double currentSpeed, currentDirection, uWind, vWind, windDirection;
	double windSpeed2, windSpeed, waveHeight, waveDirection, wavePeriod;
	long long seconds;
	double iceCover;
	FILE* filpek = fopen("data/checkStormWeather.txt", "w");
	fprintf(filpek, "stormID;featureNr;lon;lat;UTCseconds;dateTime;tidFromStartOpt;distFeatureNu;stormVal;uCurrent;vCurrent;"
		"currentDirection;currentSpeed;uWind;vWind;windDirection;windSpeed;waveHeight;wavePeriod;waveDirection;iceCover\n");

	for (i = 0; i < model.nStorms; i++) {
		for (i1 = 0; i1 < model.storms[i].nFeatures - 1; i1++) {
			time1 = (model.storms[i].feature[i1].UTCseconds - model.params.UTC_secondsStart) / 3600.0;
			time2 = (model.storms[i].feature[i1 + 1].UTCseconds - model.params.UTC_secondsStart) / 3600.0;
			p1 = model.storms[i].feature[i1].midPoint;
			p2 = model.storms[i].feature[i1 + 1].midPoint;
			bearing = p1.bearingTo(p2);
			distTot = p1.distanceTo(p2);
			deltaT = time2 - time1;
			deltaTint = (int)deltaT + 1;
			deltaDist = distTot / deltaTint;
			deltaT = (time2 - time1) / deltaTint;
			pOld = p1;
			for (i2 = 0; i2 <= deltaTint; i2++) {
				distNu = i2 * deltaDist;
				tidTot = time1 + i2 * deltaT;
				if (tidTot < 0)
					continue;
				seconds = tidTot * 3600 + model.params.UTC_secondsStart;
				p3 = p1.destinationPoint(distNu, bearing);
				calcWeatherPosAlongArc(p3, pOld); // backwards to make sure it's the right weather at the first point, since I only use that one...
				pOld = p3;
				i3 = 0;
				stormVarde = getStormValue(tidTot, model.weatherFunctions.point[i3]);
				uCurrent = getVariableValue(model.functions.pos_current_u, i3, tidTot);
				vCurrent = getVariableValue(model.functions.pos_current_v, i3, tidTot);
				if (uCurrent < 1000 && vCurrent < 1000) {
					currentDirection = atan2(vCurrent, uCurrent);
					currentSpeed = sqrt(pow(uCurrent, 2) + pow(vCurrent, 2));
				}
				else {
					currentDirection = 0;
					currentSpeed = 0;
				}
				uWind = getVariableValue(model.functions.pos_wind_u, i3, tidTot);
				vWind = getVariableValue(model.functions.pos_wind_v, i3, tidTot);
				if (uWind < 1000 && vWind < 1000) {
					windDirection = atan2(vWind, uWind);
					windSpeed2 = pow(uWind, 2) + pow(vWind, 2);
					windSpeed = sqrt(windSpeed2);

					//printf("i %d bearing %.3lf\n", i, model.weatherFunctions.vesselBearing[i]);
					//printf("shipSpeed %.2lf bearing %.2lf wind xy %.2lf %.2lf dir %.2lf rel_windSpeed %.2lf rel_windDir %.2lf\n",
					//	baseGroundSpeed, model.weatherFunctions.vesselBearing[i], uWind, vWind, windDirection * 180 / M_PI,
					//	rel_windSpeed, rel_windDir * 180 / M_PI);

					//model.functions.valuesNow.worstStabilityValue += distNu * rel_windSpeed / 10000.0;
				}
				else {
					windSpeed = 0;
				}
				waveHeight = getVariableValue(model.functions.pos_waveHeight, i3, tidTot);
				wavePeriod = getVariableValue(model.functions.pos_wavePeriod, i3, tidTot);
				waveDirection = getVariableValue(model.functions.pos_waveDirection, i3, tidTot);
				iceCover = getVariableValue(model.functions.pos_iceThickness, i3, tidTot);
				//printf("%d;%d;%.3lf;%.3lf;%I64d;%s;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf\n",
				//	model.storms[i].stormNr, i1, p3.longitude().degrees(), p3.latitude().degrees(),
				//	seconds, stringDateFromUTCSeconds(seconds).c_str(), tidTot, distNu, stormVarde, uCurrent, vCurrent, currentDirection,
				//	currentSpeed, uWind, vWind, windDirection, windSpeed,
				//	waveHeight, wavePeriod, waveDirection, iceCover);
				fprintf(filpek, "%d;%d;%.3lf;%.3lf;%I64d;%s;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf\n",
					model.storms[i].stormNr, i1, p3.longitude().degrees(), p3.latitude().degrees(),
					seconds, stringDateFromUTCSeconds(seconds).c_str(), tidTot, distNu, stormVarde, uCurrent, vCurrent, currentDirection,
					currentSpeed, uWind, vWind, windDirection, windSpeed,
					waveHeight, wavePeriod, waveDirection, iceCover);

			}


		}
	}
	fclose(filpek);
}


void calc_stormsNearby() {
	int i, i1, i2, posUse, stormOK, keepStorm;
	double timeFromStart, dist, minTid, maxTid, minDistToStorm, maxSpeed, minSpeed, speed;

	for (i = 0; i < model.params.nShip_speedSettings; i++) {
		speed = eval_calmWaterSpeed(i);
		if (minSpeed > speed + model.params.minSpeedDiffWeatherFactor + model.params.minSpeedDiffCurrent)
			minSpeed = speed + model.params.minSpeedDiffWeatherFactor + model.params.minSpeedDiffCurrent;
		if(maxSpeed < speed + model.params.maxSpeedDiffWeatherFactor + model.params.maxSpeedDiffCurrent)
			maxSpeed = speed + model.params.maxSpeedDiffWeatherFactor + model.params.maxSpeedDiffCurrent;
	}
	if (minSpeed < 1)
		minSpeed = 1;
	if (maxSpeed > 60)
		maxSpeed = 60;

	posUse = 0;
	printf("nStorms %d minSpeed %.2lf maxSpeed %.2lf\n", model.nStorms, minSpeed, maxSpeed);
	for (i = 0; i < model.nStorms; i++) {
		stormOK = eval_stormWithinBoundingBox(i);
		//printf("%d stormNr %d stormOK %d\n", i, model.storms[i].stormNr, stormOK);
		if (stormOK == 0)
			continue;

		keepStorm = 1;
		errlog("ERROR! Change keepStorm to = 0 above\n");
		for (i1 = 0; i1 < model.storms[i].nFeatures; i1++) {
			// find two closest points on preferred path within time possible, if either plus maxDistFromPath is close enough to outer circle then keep the storm
			timeFromStart = (model.storms[i].feature[i1].UTCseconds - model.params.UTC_secondsStart) / 3600.0;
			if (timeFromStart < 0) {
				if (timeFromStart < 3600 * 12)
					continue; // too early time, ignore
				timeFromStart = 0;
			}
			for (i2 = 0; i2 < model.network.nPhysicalLevels; i2++) {
				dist = model.network.physicalLev[i2].distTot;
				minTid = dist / maxSpeed;
				maxTid = (dist + model.params.maxDeviationPreferred_km) / minSpeed;
				if (minTid > timeFromStart + 6)
					break; // this level is too late and no one later will come in earlier so no need to check more
				if (maxTid < timeFromStart - 6)
					continue; // this level is too early, continue looking at later ones

				minDistToStorm = eval_minDistPointToPhysicalLevelPoints(model.storms[i].feature[i1].lon, model.storms[i].feature[i1].lat, i2);
				//printf("%d stormNr %d feature %d physLev %d minDistToStorm %.2lf, outerCircle %.2lf\n",
				//	i, model.storms[i].stormNr, i1, i2, minDistToStorm,
				//	model.storms[i].feature[i1].outerCircleSize);
				if (minDistToStorm < model.storms[i].feature[i1].outerCircleSize) {
					keepStorm = 1;
					break;
				}
			}
			if (keepStorm == 1)
				break;
		}

		if (keepStorm == 1) {
			if (i > posUse) {
				// copy this storm i to position posUse
				//for(i1 = 0; i1 < model.storms[posUse].nAllocFeatures; i1++)
				//	free(model.storms[posUse].feature);
				//free(model.storms[posUse]);
				model.storms[posUse] = model.storms[i];
			}
			errlog("ERROR! sort the storm features in time order AND only include needed ones AND possibly identify timeperiod for each\n");
			// sort the timeperiods in the storm
			sortStormFeaturesTime(posUse);

			// add bearing and distanceToNextPoint per timeperiod
			addInfoToStorms(posUse);
			posUse++;
		}
		else {
			for (i1 = 0; i1 < model.storms[i].nAllocFeatures; i1++)
				free(model.storms[i].feature);
		}

	}
	model.nStorms = posUse;
	printf("nStormsUse %d\n", model.nStorms);

}

void calc_boundingBoxFromAllNodes() {
	double x, y;
	int i, i1;

	model.boundingBox.xMin = model.preferredPath.minX;
	model.boundingBox.yMin = 90;
	model.boundingBox.xMax = model.preferredPath.maxX;
	model.boundingBox.yMax = -90;

	for (int i = 0; i < model.preferredPath.nPoints; i++) {
		y = model.preferredPath.point[i].latitude().degrees();
		if (y < model.boundingBox.yMin)
			model.boundingBox.yMin = y;
		if (y > model.boundingBox.yMax)
			model.boundingBox.yMax = y;
	}

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;
			y = model.network.physicalLev[i].point[i1].latitude().degrees();
			if (y < model.boundingBox.yMin)
				model.boundingBox.yMin = y;
			if (y > model.boundingBox.yMax)
				model.boundingBox.yMax = y;
			x = model.network.physicalLev[i].point[i1].longitude().degrees();
			if (x < model.preferredPath.minX - 10)
				x += 360;
			if (x > model.preferredPath.maxX + 10)
				x -= 360;
			if (x < model.boundingBox.xMin)
				model.boundingBox.xMin = x;
			if (x > model.boundingBox.xMax)
				model.boundingBox.xMax = x;
		}
	}
	/*
	double delta = model.params.maxDeviationPrefered_km / 120; // max antal grader
	model.boundingBox.xMin -= delta;
	model.boundingBox.xMax += delta;
	model.boundingBox.yMin -= delta;
	model.boundingBox.yMax += delta;
	*/
}

void calc_boundingBoxFrompreferredPath() {
	double y;
	// not used
	model.boundingBox.xMin = model.preferredPath.minX;
	model.boundingBox.yMin = 90;
	model.boundingBox.xMax = model.preferredPath.maxX;
	model.boundingBox.yMax = -90;

	for (int i = 0; i < model.preferredPath.nPoints; i++) {
		y = model.preferredPath.point[i].latitude().degrees();
		if (y < model.boundingBox.yMin)
			model.boundingBox.yMin = y;
		if (y > model.boundingBox.yMax)
			model.boundingBox.yMax = y;
	}

	double delta = model.params.maxDeviationPreferred_km / 120; // max antal grader
	model.boundingBox.xMin -= delta;
	model.boundingBox.xMax += delta;
	model.boundingBox.yMin -= delta;
	model.boundingBox.yMax += delta;

}


int openNeededRasterFiles()
{
	char* namn2;
	namn2 = (char*)malloc(256 * sizeof(char));
	Raster rasterPhysicalMapA, rasterPhysicalMapB, rasterFuelMapA, rasterFuelMapB;
	calc_boundingBoxFromAllNodes();
	calc_stormsNearby();

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalAFileName.c_str());
	rasterPhysicalMapA.open(namn2);
	model.physicalMapA.valueCell = rasterPhysicalMapA.GetRasterBand_intArr(1, &(model.physicalMapA), model.boundingBox);

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalBFileName.c_str());
	rasterPhysicalMapB.open(namn2);
	model.physicalMapB.valueCell = rasterPhysicalMapB.GetRasterBand_intArr(1, &(model.physicalMapB), model.boundingBox);

	//model.fuelGeographyMapRaster = (Raster*)malloc(sizeof(Raster));
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyAFileName.c_str());
	rasterFuelMapA.open(namn2);
	model.fuelMapA.valueCell = rasterFuelMapA.GetRasterBand_intArr(1, &(model.fuelMapA), model.boundingBox);
	//model.fuelGeographyMapRaster[0].open(namn2);

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyBFileName.c_str());
	rasterFuelMapB.open(namn2);
	model.fuelMapB.valueCell = rasterFuelMapB.GetRasterBand_intArr(1, &(model.fuelMapB), model.boundingBox);

	//int i1, i2;
	//for (int i = 0; i < model.nWeatherFiles; i++) {
	//	model.weather[i].rasterPos.open(model.weather[i].fileName);
		//model.weather[i].valueCell = (float**)calloc(model.weather[i].nTimeIntervals, sizeof(float*));
		//for (i2 = 0; i2 < model.weather[i].nTimeIntervals; i2++) {
		//	model.weather[i].valueCell[i2] = model.weather[i].rasterPos.GetRasterBand_realArr(i2 + 1, &(model.weather[i].raster), model.boundingBox);
		//}
	//}

	return 0;
}

int openNeededRasterFiles_test()
{

	calc_boundingBoxFromAllNodes();

	/*
	for (int i = 0; i < model.nWeatherFiles; i++) {
		model.weather[i].rasterPos.open(model.weather[i].fileName);
		model.weather[i].valueCell = model.weather[i].rasterPos.GetRasterBand_realArrAllBands(&(model.weather[i].raster), model.boundingBox);
		//model.weather[i].valueCell = (float**)calloc(model.weather[i].nTimeIntervals, sizeof(float*));
		//for (i2 = 0; i2 < model.weather[i].nTimeIntervals; i2++) {
		//	model.weather[i].valueCell[i2] = model.weather[i].rasterPos.GetRasterBand_realArr(i2 + 1, &(model.weather[i].raster), model.boundingBox);
		//}
	}

	for (int i = 0; i < model.nWeatherFiles; i++) {
		//for (i1 = 0; i1 < model.weather[i].nFiles; i1++) {
		//	if (model.weather[i].nFiles == 1) {
		//model.weather[i].rasterPos[0].open(model.weather[i].filePos[i1].fileName);
		model.weather[i].rasterPos.open(model.weather[i].fileName);
		//	}
		//	else {
		//		if (model.network.useLongitudeKvadrant[i1] == 1)
		//			model.weather[i].rasterPos[i1].open(model.weather[i].filePos[i1].fileName);
		//	}

		//}
	}
	*/

	//	Raster raster;
	//	raster.open((const char*)"WW3_NCEP.grb");
	//	float** rasterBandData = raster.GetRasterBand(0);
	//	double noData = raster.GetNoDataValue();
	//	cout << "value at row 10, column 30: " << rasterBandData[10][30] << endl;
		//Raster testRaster;
		//testRaster.open(model.params.mapPhysicalFileName.c_str());
		//model.physicalMapRaster = (Raster*)malloc(sizeof(Raster));
		//model.physicalMapRaster = testRaster; //  .open(model.params.mapPhysicalFileName.c_str());

	char* namn2;
	Raster rasterPhysicalMapA, rasterPhysicalMapB, rasterFuelMapA, rasterFuelMapB;
	namn2 = (char*)malloc(256 * sizeof(char));
	//sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalFileName.c_str());
	//model.physicalMapRaster.open(namn2);

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalAFileName.c_str());
	rasterPhysicalMapA.open(namn2);

	// calc_boundingBoxFrompreferredPath();

	/*
	model.boundingBox.xMin = -15.44;
	model.boundingBox.yMin = 27.83;
	model.boundingBox.xMax = -14.23; // 38.07;// 
	model.boundingBox.yMax = 29.06;

	model.boundingBox.xMin = -178;
	model.boundingBox.yMin = -84;
	model.boundingBox.xMax = -178; // 38.07;// 
	model.boundingBox.yMax = -80; // 69.27;// 

	model.boundingBox.xMin = -23.1644;
	model.boundingBox.yMin = 15.70772;
	model.boundingBox.xMax = -22.9786; // 38.07;// 
	model.boundingBox.yMax = 15.8983; // 69.27;// 
	*/

	model.physicalMapA.valueCell = rasterPhysicalMapA.GetRasterBand_intArr(1, &(model.physicalMapA), model.boundingBox);

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalBFileName.c_str());
	rasterPhysicalMapB.open(namn2);
	model.physicalMapB.valueCell = rasterPhysicalMapB.GetRasterBand_intArr(1, &(model.physicalMapB), model.boundingBox);

	//model.fuelGeographyMapRaster = (Raster*)malloc(sizeof(Raster));
	//sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyFileName.c_str());
	//model.fuelGeographyMapRaster[0].open(namn2);
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyAFileName.c_str());
	rasterFuelMapA.open(namn2);
	model.fuelMapA.valueCell = rasterFuelMapA.GetRasterBand_intArr(1, &(model.fuelMapA), model.boundingBox);

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyBFileName.c_str());
	rasterFuelMapB.open(namn2);
	model.fuelMapB.valueCell = rasterFuelMapB.GetRasterBand_intArr(1, &(model.fuelMapB), model.boundingBox);


	/*
	float **rasterData = model.physicalMapRaster.GetRasterBand(1);
	int row, col;
	double v1, v2, v3, v4, v5, rowSize, colSize, minLon, maxLat;
	FILE *filDmp;
	filDmp = fopen("valRasterDmp.txt", "w");
	for (row = 0; row < model.physicalMapRaster.Get_nRows(); row++) {
		for (col = 0; col < model.physicalMapRaster.Get_nCols(); col++) {
			rowSize = model.physicalMapRaster.Get_sizeRow();
			colSize = model.physicalMapRaster.Get_sizeCol();
			maxLat = model.physicalMapRaster.Get_maxLatitude();
			minLon = model.physicalMapRaster.Get_minLongitude();
			v1 = maxLat - row *
				rowSize;
			v2 = minLon + col *
				colSize;
			v3 = maxLat - (row + 1) *
				rowSize;
			v4 = minLon + (col + 1) *
				colSize;
			v5 = rasterData[row][col];
			fprintf(filDmp, "row %d col %d latlon %.2lf %.2lf to %.2lf %.2lf val %.3lf\n",
				row, col, v1, v2, v3, v4, v5);
		}
	}
	fclose(filDmp);
	*/

	return 0;
}

double detDistLatLon(double lat1, double lon1, double lat2, double lon2) {
	if (lon1 > 180)
		lon1 -= 360;
	if (lon2 > 180)
		lon2 -= 360;
	spherical::Point p1(lat1, lon1), p2(lat2, lon2);
	return p1.distanceTo(p2);
}


double get_fuelQualityKvot(int thisLevel, int pos1, int nextLevel, int pos2)
{
	int arcOK = 1;
	spherical::Point p1, p2;
	double distECA = 0.0, distOther = 0.0, distTot;

	if (thisLevel >= 0) {
		p1 = model.network.physicalLev[thisLevel].point[pos1];
		if (nextLevel >= 0) {
			p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			p2 = model.network.channel[-nextLevel - 1].point[0];
		}
	}
	else {
		if (nextLevel >= 0) {
			p1 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			p1 = model.network.channel[-thisLevel - 1].point[0];
			p2 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
		}
	}

	get_fuelUseKvotECA(p1.latitude().degrees(), p1.longitude().degrees(), p2.latitude().degrees(), p2.longitude().degrees(), 0, &distECA, &distOther);
	distTot = distECA + distOther;
	if (distTot > 0)
		return distOther / distTot;
	else
		return 0.0;
}

/*
int check_isPhysicalArcOK_old(int startLevel, int slutLevel, spherical::Point p1, spherical::Point p2, double noDataVal)
{
	int arcOK = 1, mittPktPos1, mittPktPos2, level1, level2;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;


	// model.rasterData.physicalMap = model.physicalMapRaster.GetRasterBand(1);
	// float** rasterData = model.physicalMapRaster.GetRasterBand(1);
	double v1, v2, v3, v4, v5, rowSize, colSize, distance;
	double kvot, kvot_r, kvot_c, kvotNu, rowNu, colNu, maxLat, minLon;
	int row, col, riktning_r, riktning_c;

	distance = p1.distanceTo(p2) / 1000.0;

	if (startLevel >= 0 && slutLevel >= 0) {
		level1 = startLevel - 1;
		if (startLevel == slutLevel - 1) {
			level2 = slutLevel + 1;
		}
		else {
			level2 = slutLevel;
		}
	}
	else {
		if (slutLevel < 0) {
			level1 = startLevel - 1;
			level2 = startLevel;
		}
		else {
			level1 = slutLevel;
			level2 = slutLevel + 1;
		}
	}
	if (level1 < 0) {
		level1 = 0;
		if (level2 == 0)
			level2 = 1;
	}
	if (level2 >= model.network.nPhysicalLevels) {
		level2 = model.network.nPhysicalLevels - 1;
		if (level1 >= level2)
			level1 = level2 - 1;
	}
	mittPktPos1 = model.network.physicalLev[level1].nPoints / 2;
	mittPktPos2 = model.network.physicalLev[level2].nPoints / 2;

	if (distance < model.params.basDistArcs * 0.05) // 1)
		return -2; // felaktig bage, addera ej utan titta i nasta niva istallet, hantera sista nivan...

	if (slutLevel - startLevel != 1) {
		if (distance >= model.params.basDistArcs * 1.4)
			return 0; // too far distance to channel
	}
	auto b1 = model.network.physicalLev[level1].point[mittPktPos1].bearingTo(model.network.physicalLev[level2].point[mittPktPos2]);
	auto b2 = p1.bearingTo(p2);
	// modify the bearing
	double bNy = (b1 - b2);
	if (bNy < 0)
		bNy += 360;
	if (bNy >= 360)
		bNy -= 360;
	if (bNy >= 90 && bNy <= 270) {
		return -2; // felaktigt riktad bage, addera ej utan titta i nasta niva istallet, hantera sista nivan...

	}

	row1Dbl = (model.physicalMapRaster.Get_maxLatitude() - p1.latitude().degrees()) / model.physicalMapRaster.Get_sizeRow();
	col1Dbl = (p1.longitude().degrees() - model.physicalMapRaster.Get_minLongitude()) / model.physicalMapRaster.Get_sizeCol();
	if (col1Dbl < 0)
		col1Dbl += model.physicalMapRaster.Get_nCols();
	row2Dbl = (model.physicalMapRaster.Get_maxLatitude() - p2.latitude().degrees()) / model.physicalMapRaster.Get_sizeRow();
	col2Dbl = (p2.longitude().degrees() - model.physicalMapRaster.Get_minLongitude()) / model.physicalMapRaster.Get_sizeCol();
	if (col2Dbl < 0)
		col2Dbl += model.physicalMapRaster.Get_nCols();

	delta_row = row2Dbl - row1Dbl;
	if (delta_row > model.physicalMapRaster.Get_nRows() / 2) {
		delta_row = model.physicalMapRaster.Get_nRows() - delta_row;
	}
	else {
		if (delta_row < -model.physicalMapRaster.Get_nRows() / 2) {
			delta_row = -model.physicalMapRaster.Get_nRows() - delta_row;
		}
	}
	delta_col = col2Dbl - col1Dbl;
	if (delta_col > model.physicalMapRaster.Get_nCols() / 2) {
		delta_col = model.physicalMapRaster.Get_nCols() - delta_col;
	}
	else {
		if (delta_col < -model.physicalMapRaster.Get_nCols() / 2) {
			delta_col = -model.physicalMapRaster.Get_nCols() - delta_col;
		}
	}

	kvot = 0;
	row = (int)row2Dbl;
	col = (int)col2Dbl;
	if (abs(model.rasterData.physicalMap[row][col] - noDataVal) < 0.001) {
		arcOK = 0;
		return arcOK;
	}
	rowNu = row1Dbl;
	colNu = col1Dbl;
	row = (int)rowNu;
	col = (int)colNu;
	if (abs(model.rasterData.physicalMap[row][col] - noDataVal) < 0.001) {
		arcOK = 0;
		return arcOK;
	}
	if (delta_row > 0)
		riktning_r = 1;
	else {
		if (delta_row != 0)
			riktning_r = -1;
		else
			riktning_r = 0;
	}
	if (delta_col > 0)
		riktning_c = 1;
	else {
		if (delta_col != 0)
			riktning_c = -1;
		else
			riktning_c = 0;
	}
	for (int i = 0; i < riktning_r * delta_row + riktning_c * delta_col + 3; i++) {
		if (riktning_r != 0)
			kvot_r = (row + riktning_r - rowNu) / delta_row;
		else
			kvot_r = 10000;
		if (riktning_c != 0)
			kvot_c = (col + riktning_c - colNu) / delta_col;
		else
			kvot_c = 10000;
		kvotNu = min(kvot_r, kvot_c);
		kvot += kvotNu;
		if (kvot > 0.99)
			break;
		rowNu += kvotNu * delta_row;
		if (rowNu < 0) rowNu += model.physicalMapRaster.Get_nRows();
		if (rowNu >= model.physicalMapRaster.Get_nRows()) rowNu -= model.physicalMapRaster.Get_nRows();
		colNu += kvotNu * delta_col;
		if (colNu < 0) colNu += model.physicalMapRaster.Get_nCols();
		if (colNu >= model.physicalMapRaster.Get_nCols()) colNu -= model.physicalMapRaster.Get_nCols();
		row = (int)rowNu;
		col = (int)colNu;
		if (abs(model.rasterData.physicalMap[row][col] - noDataVal) < 0.001) {
			arcOK = 0;
			break;
		}
	}

	return arcOK;
}
*/

int check_nodeIsWithinPhysicalMapRaster(double lat1, double lon1) {
	if (lat1 < model.physicalMapA.minLatitude)
		return 0;
	if (lat1 > model.physicalMapA.maxLatitude)
		return 0;

	return 1;
}

void getRowColDblFromPhysicalMap(Raster::strPhysRaster physicalMap, double lat1, double lon1, double* row1Dbl, double* col1Dbl) {
	if (lon1 < physicalMap.minLongitude)
		lon1 += 360;
	else {
		if (lon1 > physicalMap.maxLongitude)
			lon1 -= 360;
	}
	*row1Dbl = (physicalMap.maxLatitude - lat1) / physicalMap.size_row; // .raster.Get_sizeRow();
	if (*row1Dbl < 0)
		*row1Dbl = 0;
	if (*row1Dbl >= physicalMap.nRows)
		*row1Dbl = physicalMap.nRows - 1;
	*col1Dbl = (lon1 - physicalMap.minLongitude) / physicalMap.size_col; // .raster.Get_sizeCol();
	if (*col1Dbl < 0)
		*col1Dbl = 0;
	if (*col1Dbl >= physicalMap.nCols)
		*col1Dbl = physicalMap.nCols - 1;
}

void getLatLonFromPhysicalMap(Raster::strPhysRaster physicalMap, double* lat1, double* lon1, double row1Dbl, double col1Dbl) {

	*lat1 = physicalMap.maxLatitude - row1Dbl * physicalMap.size_row;
	*lon1 = col1Dbl * physicalMap.size_col + physicalMap.minLongitude;
	if (*lon1 < -180)
		*lon1 += 360;
	else {
		if (*lon1 > 180)
			*lon1 -= 360;
	}
	if (*lat1 < physicalMap.minLatitude)
		*lat1 = physicalMap.minLatitude;
	else {
		if (*lat1 > physicalMap.maxLatitude)
			*lat1 = physicalMap.maxLatitude;
	}
}

int check_feasibleNodeRasterA(double lat1, double lon1) {
	double row1Dbl, col1Dbl;
	int row, col;
	Raster::strPhysRaster physicalMap = model.physicalMapA;
	getRowColDblFromPhysicalMap(physicalMap, lat1, lon1, &row1Dbl, &col1Dbl);
	row = (int)row1Dbl;
	col = (int)col1Dbl;
	if (col >= physicalMap.nCols)
		col -= physicalMap.nCols;
	else {
		if (col < 0)
			col += physicalMap.nCols;
	}
	return physicalMap.valueCell[row * physicalMap.nCols + col];
}


int check_physicalMap_ok(double lat1, double lon1, double lat2, double lon2, int mapAlt) {
	Raster::strPhysRaster physicalMap;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg;
	int row, col, i, isOK, colUse;

	if (mapAlt == 0)
		physicalMap = model.physicalMapB;
	else
		physicalMap = model.physicalMapA;

	getRowColDblFromPhysicalMap(physicalMap, lat1, lon1, &row1Dbl, &col1Dbl);
	getRowColDblFromPhysicalMap(physicalMap, lat2, lon2, &row2Dbl, &col2Dbl);

	delta_row = row2Dbl - row1Dbl;
	if (delta_row > physicalMap.nRows / 2) {
		delta_row = physicalMap.nRows - delta_row;
	}
	else {
		if (delta_row < -physicalMap.nRows / 2) {
			delta_row = -physicalMap.nRows - delta_row;
		}
	}
	delta_col = col2Dbl - col1Dbl;
	if (delta_col > physicalMap.nCols / 2) {
		delta_col = delta_col - physicalMap.nCols;
	}
	else {
		if (delta_col < -physicalMap.nCols / 2) {
			delta_col = physicalMap.nCols + delta_col;
		}
	}

	kvot = 0;
	row = (int)row1Dbl;
	colDbl = col1Dbl;
	rowDbl = row1Dbl;
	col = (int)colDbl;
	//rowDbl_old = row1Dbl;
	//colDbl_old = col1Dbl;

	a0 = 0;
	for (i = 0; i < 10000; i++) {
		if (col >= physicalMap.nCols)
			colUse = col - physicalMap.nCols;
		else {
			if (col < 0)
				colUse = col + physicalMap.nCols;
			else
				colUse = col;
		}
		if (physicalMap.valueCell[row * physicalMap.nCols + colUse] == 0)
			return 0; // arc is not okay

		if (delta_col > 0)
			ac = (col + 1 - col1Dbl) / delta_col;
		else {
			if (delta_col == 0)
				ac = 999999;
			else {
				if (colDbl > col + 0.001)
					ac = (col - col1Dbl) / delta_col;
				else
					ac = (col - 1 - col1Dbl) / delta_col;
			}
		}
		if (delta_row > 0)
			ar = (row + 1 - row1Dbl) / delta_row;
		else {
			if (delta_row == 0)
				ar = 999999;
			else {
				if (rowDbl > row + 0.001)
					ar = (row - row1Dbl) / delta_row;
				else
					ar = (row - 1 - row1Dbl) / delta_row;
			}
		}
		if (ac < ar)
			a1 = ac;
		else
			a1 = ar;
		if (a1 > 1)
			a1 = 1;

		if (physicalMap.valueCell[row * physicalMap.nCols + colUse] == 2) {
			y1 = physicalMap.maxLatitude - (row1Dbl + a0 * delta_row) * physicalMap.size_row;
			x1 = (col1Dbl + a0 * delta_col) * physicalMap.size_col + physicalMap.minLongitude;
			y2 = physicalMap.maxLatitude - (row1Dbl + a1 * delta_row) * physicalMap.size_row;
			x2 = (col1Dbl + a1 * delta_col) * physicalMap.size_col + physicalMap.minLongitude;
			isOK = check_physicalMap_ok(y1, x1, y2, x2, 1);
			if (isOK == 0)
				return 0; // arc is not okay in raster alt
		}
		if (mapAlt == 1)
			mapAlt = mapAlt;
		if (a1 >= 0.9999)
			break;

		colDbl = col1Dbl + a1 * delta_col;
		rowDbl = row1Dbl + a1 * delta_row;

		col = (int)(colDbl + 0.0001 * delta_col);
		if (colDbl < 0)
			colDbl = colDbl;
		row = (int)(rowDbl + 0.0001 * delta_row);
		if (rowDbl < 0)
			rowDbl = rowDbl;

		colForeg = colDbl;
		a0 = a1;
		//rowDbl_old = rowDbl;
		//colDbl_old = colDbl;
	}
	return 1;
}

void get_fuelUseKvotECA(double lat1, double lon1, double lat2, double lon2, int mapAlt, double* distECA, double* distOther) {
	Raster::strPhysRaster fuelMap;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg, dist;
	int row, col, i, colUse;

	if (mapAlt == 0)
		fuelMap = model.fuelMapB;
	else
		fuelMap = model.fuelMapA;

	getRowColDblFromPhysicalMap(fuelMap, lat1, lon1, &row1Dbl, &col1Dbl);
	getRowColDblFromPhysicalMap(fuelMap, lat2, lon2, &row2Dbl, &col2Dbl);

	delta_row = row2Dbl - row1Dbl;
	if (delta_row > fuelMap.nRows / 2) {
		delta_row = fuelMap.nRows - delta_row;
	}
	else {
		if (delta_row < -fuelMap.nRows / 2) {
			delta_row = -fuelMap.nRows - delta_row;
		}
	}
	delta_col = col2Dbl - col1Dbl;
	if (delta_col > fuelMap.nCols / 2) {
		delta_col = delta_col - fuelMap.nCols;
	}
	else {
		if (delta_col < -fuelMap.nCols / 2) {
			delta_col = fuelMap.nCols + delta_col;
		}
	}

	kvot = 0;
	row = (int)row1Dbl;
	colDbl = col1Dbl;
	rowDbl = row1Dbl;
	col = (int)colDbl;
	//rowDbl_old = row1Dbl;
	//colDbl_old = col1Dbl;

	a0 = 0;
	for (i = 0; i < 10000; i++) {
		if (col >= fuelMap.nCols)
			colUse = col - fuelMap.nCols;
		else {
			if (col < 0)
				colUse = col + fuelMap.nCols;
			else
				colUse = col;
		}

		if(delta_col > 0)
			ac = (col + 1 - col1Dbl) / delta_col;
		else {
			if (delta_col == 0)
				ac = 999999;
			else {
				if(colDbl > col + 0.001)
					ac = (col - col1Dbl) / delta_col;
				else
					ac = (col - 1 - col1Dbl) / delta_col;
			}
		}
		if (delta_row > 0)
			ar = (row + 1 - row1Dbl) / delta_row;
		else {
			if (delta_row == 0)
				ar = 999999;
			else {
				if (rowDbl > row + 0.001)
					ar = (row - row1Dbl) / delta_row;
				else
					ar = (row - 1 - row1Dbl) / delta_row;
			}
		}
		if (ac < ar)
			a1 = ac;
		else
			a1 = ar;
		if (a1 > 1)
			a1 = 1;

		y1 = fuelMap.maxLatitude - (row1Dbl + a0 * delta_row) * fuelMap.size_row;
		x1 = (col1Dbl + a0 * delta_col) * fuelMap.size_col + fuelMap.minLongitude;
		y2 = fuelMap.maxLatitude - (row1Dbl + a1 * delta_row) * fuelMap.size_row;
		x2 = (col1Dbl + a1 * delta_col) * fuelMap.size_col + fuelMap.minLongitude;
		if (fuelMap.valueCell[row * fuelMap.nCols + colUse] == 2) {
			get_fuelUseKvotECA(y1, x1, y2, x2, 1, distECA, distOther);
		}
		else {
			dist = detDistLatLon(y1, x1, y2, x2);
			if (fuelMap.valueCell[row * fuelMap.nCols + colUse] == 1) {
				*distECA += dist;
			}
			else
				*distOther += dist;
		}


		if (mapAlt == 1)
			mapAlt = mapAlt;
		if (a1 >= 0.9999)
			break;

		colDbl = col1Dbl + a1 * delta_col;
		rowDbl = row1Dbl + a1 * delta_row;

		col = (int)(colDbl + 0.0001 * delta_col);
		if (colDbl < 0)
			colDbl = colDbl;
		row = (int)(rowDbl + 0.0001 * delta_row);
		if (rowDbl < 0)
			rowDbl = rowDbl;

		colForeg = colDbl;
		a0 = a1;
		//rowDbl_old = rowDbl;
		//colDbl_old = colDbl;
	}
}


int check_isPhysicalArcOK(int startLevel, int slutLevel, spherical::Point p1, spherical::Point p2, double noDataVal)
{
	int arcOK = 1, mittPktPos1, mittPktPos2, level1, level2;


	// model.rasterData.physicalMap = model.physicalMapRaster.GetRasterBand(1);
	// float** rasterData = model.physicalMapRaster.GetRasterBand(1);
	double distance;

	distance = p1.distanceTo(p2) / 1000.0;

	if (startLevel >= 0 && slutLevel >= 0) {
		level1 = startLevel - 1;
		if (startLevel == slutLevel - 1) {
			level2 = slutLevel + 1;
		}
		else {
			level2 = slutLevel;
		}
	}
	else {
		if (slutLevel < 0) {
			level1 = startLevel - 1;
			level2 = startLevel;
		}
		else {
			level1 = slutLevel;
			level2 = slutLevel + 1;
		}
	}
	if (level1 < 0) {
		level1 = 0;
		if (level2 == 0)
			level2 = 1;
	}
	if (level2 >= model.network.nPhysicalLevels) {
		level2 = model.network.nPhysicalLevels - 1;
		if (level1 >= level2)
			level1 = level2 - 1;
	}
	mittPktPos1 = model.network.physicalLev[level1].nPoints / 2;
	mittPktPos2 = model.network.physicalLev[level2].nPoints / 2;

	if (distance < model.params.basDistArcs * 0.05) // 1)
		return -2; // felaktig bage, addera ej utan titta i nasta niva istallet, hantera sista nivan...

	if (slutLevel - startLevel != 1) {
		if (distance >= model.params.basDistArcs * 1.4)
			return 0; // too far distance to channel
	}
	auto b1 = model.network.physicalLev[level1].point[mittPktPos1].bearingTo(model.network.physicalLev[level2].point[mittPktPos2]);
	auto b2 = p1.bearingTo(p2);
	// modify the bearing
	double bNy = (b1 - b2);
	if (bNy < 0)
		bNy += 360;
	if (bNy >= 360)
		bNy -= 360;
	if (bNy >= 90 && bNy <= 270) {
		return -2; // felaktigt riktad bage, addera ej utan titta i nasta niva istallet, hantera sista nivan...

	}

	int isOk = check_physicalMap_ok(p1.latitude().degrees(), p1.longitude().degrees(), 
		p2.latitude().degrees(), p2.longitude().degrees(), 0);

	int ok1;
	if (isOk == 1) {
		ok1 = check_feasibleNodeRasterA(p1.latitude().degrees(), p1.longitude().degrees());
		if (ok1 == 0)
			errlog("ERROR! Node not feasible but arc from it is. Level %d xy %.4lf %.4lf\n",
				level1, p1.longitude().degrees(), p1.latitude().degrees());
		ok1 = check_feasibleNodeRasterA(p2.latitude().degrees(), p2.longitude().degrees());
		if (ok1 == 0)
			errlog("ERROR! Node not feasible but arc to it is. Level %d xy %.4lf %.4lf\n",
				level1, p2.longitude().degrees(), p2.latitude().degrees());
	}

	return isOk;
}

int try_addPhysicalArcsLevel(int thisLevel, int pointPos, int nextLevel, double noDataVal)
{
	int checkNextLevel, i2, arcOK, arcPos, i;
	double distance;

	checkNextLevel = 0;
	if (nextLevel > 0) { // next physical level
		for (i2 = 0; i2 < model.network.physicalLev[nextLevel].nPoints; i2++) {
			if (i2 == 30)
				i2 = i2;
			if (model.network.physicalLev[nextLevel].allowedPoint[i2] == 0)
				continue; // not allowed node
			if (abs(pointPos - model.network.physicalLev[thisLevel].nPoints / 2 - (
				i2 - model.network.physicalLev[nextLevel].nPoints / 2)) > model.params.max_changeDirection &&
				(thisLevel != 0 && nextLevel != model.network.nPhysicalLevels - 1))
				continue; // cannot turn too much...
			if (thisLevel == 33 && pointPos == 30 && i2 == 33)
				i2 = i2;
			if (pointPos != model.params.preferredPathOrtoPos[thisLevel] || i2 != model.params.preferredPathOrtoPos[nextLevel] || nextLevel != thisLevel + 1)
				arcOK = check_isPhysicalArcOK(thisLevel, nextLevel, model.network.physicalLev[thisLevel].point[pointPos],
					model.network.physicalLev[nextLevel].point[i2], noDataVal);
			else
				arcOK = 1;
			if (arcOK == 1) {
				arcPos = model.network.physicalLev[thisLevel].nOutNodes[pointPos];
				model.network.physicalLev[thisLevel].outNode[pointPos][arcPos] = i2;
				model.network.physicalLev[thisLevel].outLevel[pointPos][arcPos] = nextLevel;
				(model.network.physicalLev[thisLevel].nOutNodes[pointPos])++;
			}
			if (arcOK == -2) {
				checkNextLevel = 1;// titta pa nivan efter...
			}
			else {
				if (thisLevel >= 0 && nextLevel >= 0) {
					distance = model.network.physicalLev[thisLevel].point[pointPos].distanceTo(model.network.physicalLev[nextLevel].point[i2]) / 1000.0;
					if (distance < model.network.physicalLev[nextLevel].minDistPrevNode[i2]) {
						model.network.physicalLev[nextLevel].minDistPrevNode[i2] = distance;
						model.network.physicalLev[nextLevel].minDistPrevNode_level[i2] = thisLevel;
						model.network.physicalLev[nextLevel].minDistPrevNode_pos[i2] = pointPos;
					}
				}
			}
		}
	}
	else {
		// connect to channel
		for (i2 = 0; i2 < model.network.nChannels; i2++) {
			if (model.network.channel[i2].allowedPoint[0] == 0)
				continue; // first node in channel not okay
			arcOK = check_isPhysicalArcOK(thisLevel, -i2 - 1, model.network.physicalLev[thisLevel].point[pointPos],
				model.network.channel[i2].point[0], noDataVal);
			if (arcOK == 1) {
				arcPos = model.network.physicalLev[thisLevel].nOutNodes[pointPos];
				model.network.physicalLev[thisLevel].outNode[pointPos][arcPos] = 0;
				model.network.physicalLev[thisLevel].outLevel[pointPos][arcPos] = -i2 - 1;
				(model.network.physicalLev[thisLevel].nOutNodes[pointPos])++;
				for (i = 0; i < model.network.nUsedChannels; i++) {
					if (model.network.usedChannel[i] == i2)
						break;
				}
				if (i >= model.network.nUsedChannels) {
					model.network.usedChannel[i] = i2;
					(model.network.nUsedChannels)++;

					model.network.channel[i2].nOutNodes = (int*)calloc(1, sizeof(int));
					model.network.channel[i2].nArcsToPoint = (int*)calloc(1, sizeof(int));
					model.network.channel[i2].outNode = (int**)malloc(sizeof(int*));
					model.network.channel[i2].outLevel = (int**)malloc(sizeof(int*));
					model.network.channel[i2].outNode[0] = (int*)malloc(model.params.nPkterOrto * 2 * sizeof(int));
					model.network.channel[i2].outLevel[0] = (int*)malloc(model.params.nPkterOrto * 2 * sizeof(int));

					model.network.channel[i2].nodNr_from_pt = (int**)malloc(2 * sizeof(int*));
					model.network.channel[i2].nTimeIntervals = (int*)malloc(2 * sizeof(int));
					model.network.channel[i2].nAllocTimeIntervals = (int*)malloc(2 * sizeof(int));
					model.network.channel[i2].timeInterval = (int**)malloc(2 * sizeof(int*));

					for (int i3 = 0; i3 < 2; i3++) { // start och endnod i channel
						model.network.channel[i2].nTimeIntervals[i3] = 0;
						model.network.channel[i2].nAllocTimeIntervals[i3] = 100;
						model.network.channel[i2].timeInterval[i3] = (int*)malloc(
							model.network.channel[i2].nAllocTimeIntervals[i3] * sizeof(int));
						model.network.channel[i2].nodNr_from_pt[i3] = (int*)malloc(
							model.network.channel[i2].nAllocTimeIntervals[i3] * sizeof(int));
					}
				}
			}
		}
	}

	return checkNextLevel;
}

int try_addPhysicalArcsFromChannel(int cNr, double noDataVal)
{
	int i, arcPos, i1, arcOK, pos;

	for (i = 1; i < model.network.nPhysicalLevels; i++) {
		if (i == 13)
			i = i;
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue; // node not okay
			pos = model.network.channel[cNr].nPoints - 1;
			if (model.network.channel[cNr].allowedPoint[pos] == 0)
				continue; // last node in channel not okay
			arcOK = check_isPhysicalArcOK(-cNr - 1, i, model.network.channel[cNr].point[pos],
				model.network.physicalLev[i].point[i1], model.params.physicalMap_noDataValue);
			if (arcOK == 1) {
				arcPos = model.network.channel[cNr].nOutNodes[0];
				model.network.channel[cNr].outNode[0][arcPos] = i1;
				model.network.channel[cNr].outLevel[0][arcPos] = i;
				(model.network.channel[cNr].nOutNodes[0])++;
			}
		}
	}
	return 0;
}

int addArcsToNetwork()
{
	int i, i1, i2, cNr;
	int checkNextLevel;

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].nOutNodes = (int*)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].nArcsToPoint = (int*)calloc(
			model.network.physicalLev[i].nPoints, sizeof(int));
		//model.network.physicalLev[i].nOutArcs = (int*)malloc(
		//	model.network.physicalLev[i].nPoints * sizeof(int));
		//model.network.physicalLev[i].nAllocOutArcs = (int*)malloc(
		//	model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].outNode = (int**)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		model.network.physicalLev[i].outLevel = (int**)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		//model.network.physicalLev[i].outArc = (strArcInfo**)malloc(
		//	model.network.physicalLev[i].nPoints * sizeof(strArcInfo*));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			model.network.physicalLev[i].nOutNodes[i1] = 0;
			//model.network.physicalLev[i].nOutArcs[i1] = 0;
			//model.network.physicalLev[i].nAllocOutArcs[i1] = 1000;
			if (i < model.network.nPhysicalLevels - 1) {
				model.network.physicalLev[i].outNode[i1] = (int*)malloc(
					model.network.physicalLev[i + 1].nPoints * sizeof(int));
				model.network.physicalLev[i].outLevel[i1] = (int*)malloc(
					model.network.physicalLev[i + 1].nPoints * sizeof(int));
				//model.network.physicalLev[i].outArc[i1] = (strArcInfo*)malloc(
				//	model.network.physicalLev[i].nAllocOutArcs[i1] * sizeof(strArcInfo));
			}
		}
	}


	//FILE* filpek;
	int allowed;
	//filpek = fopen("checkChannelsCorridor.txt", "w");
	for (i2 = 0; i2 < model.network.nChannels; i2++) {
		allowed = 1;
		if (check_isChannelNodePosAllowed(i2, 0) == 0)
			allowed = 0;
		else {
			if (check_isChannelNodePosAllowed(i2, model.network.channel[i2].nPoints - 1) == 0)
				allowed = 0;
		}
		if (allowed == 0) {
			model.network.channel[i2].allowedPoint[0] = allowed;
			model.network.channel[i2].allowedPoint[model.network.channel[i2].nPoints - 1] = allowed;
		}
	}
	//fclose(filpek);

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].minDistPrevNode = (double*)malloc(model.network.physicalLev[i].nPoints * sizeof(double));
		model.network.physicalLev[i].minDistPrevNode_level = (int*)malloc(model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].minDistPrevNode_pos = (int*)malloc(model.network.physicalLev[i].nPoints * sizeof(int));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			model.network.physicalLev[i].minDistPrevNode[i1] = 1e10;
		}
	}

	//model.rasterData.physicalMap = model.physicalMapRaster.GetRasterBand(model.params.physicalMapRasterPos + 1);
	//model.physicalMapA.valueCell = model.physicalMapA.raster.GetRasterBand(1);
	//model.physicalMapB.valueCell = model.physicalMapB.raster.GetRasterBand(1);

	//double noDataVal = model.physicalMapRaster.GetNoDataValue();
	double noDataVal = model.params.physicalMap_noDataValue;
	model.network.nUsedChannels = 0;
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;
			if (i == 3 && i1 == 59)
				i1 = i1;
			for (i2 = i + 1; i2 < model.network.nPhysicalLevels; i2++) {
				if (i == 3 && i1 == 59&&i2==62)
					i1 = i1;
				checkNextLevel = try_addPhysicalArcsLevel(i, i1, i2, noDataVal);
				if (checkNextLevel == 0)
					break;
			}
			// check if arcs can be added to a channel
			checkNextLevel = try_addPhysicalArcsLevel(i, i1, -1, noDataVal);
		}
	}

	for (i = 0; i < model.network.nUsedChannels; i++) {
		cNr = model.network.usedChannel[i];
		try_addPhysicalArcsFromChannel(cNr, noDataVal);
	}

	int nNoder = 0, nBagar = 0;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		for (i2 = 0; i2 < model.network.physicalLev[i].nPoints; i2++) {
			if (model.network.physicalLev[i].allowedPoint[i2] == 0)
				continue; // not allowed node
			nNoder++;
			nBagar += model.network.physicalLev[i].nOutNodes[i2];
		}
	}
	errlog("Fysiskt natverk: %d noder och %d bagar\n", nNoder, nBagar);






	return 0;
}

int movePointToFeasible(int level, int pos) {
	int row, col, isFeasible = 0, i, nCellsOK, colUse;
	double rowDbl, colDbl, col2Dbl, row2Dbl, row1Dbl, col1Dbl;
	double delta_row, delta_col, kvot, a0, ac, ar, a1, dist, bastDist;
	double bastCol, bastRow, lat, lon;

	bastDist = 1e10;
	if (level == 11 && pos == 60)
		pos = pos;
	getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point[pos].latitude().degrees(),
		model.network.physicalLev[level].point[pos].longitude().degrees(), &row1Dbl, &col1Dbl);

	if (level == 1 && pos == 24)
		pos = pos;
	for (int i0 = 0; i0 < 2; i0++) {
		if (i0 == 0) {
			if (pos == 0)
				continue;
			getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point[pos - 1].latitude().degrees(),
				model.network.physicalLev[level].point[pos - 1].longitude().degrees(), &row2Dbl, &col2Dbl);
		}else{
			if (pos + 1 >= model.network.physicalLev[level].nPoints)
				continue;
			getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point[pos + 1].latitude().degrees(),
				model.network.physicalLev[level].point[pos + 1].longitude().degrees(), &row2Dbl, &col2Dbl);
		}
		delta_row = row2Dbl - row1Dbl;
		if (delta_row > model.physicalMapA.nRows / 2) {
			delta_row = model.physicalMapA.nRows - delta_row;
		}
		else {
			if (delta_row < -model.physicalMapA.nRows / 2) {
				delta_row = -model.physicalMapA.nRows - delta_row;
			}
		}
		delta_col = col2Dbl - col1Dbl;
		if (delta_col > model.physicalMapA.nCols / 2) {
			delta_col = delta_col - model.physicalMapA.nCols;
		}
		else {
			if (delta_col < -model.physicalMapA.nCols / 2) {
				delta_col = model.physicalMapA.nCols + delta_col;
			}
		}

		kvot = 0;
		row = (int)row1Dbl;
		colDbl = col1Dbl;
		col = (int)colDbl;

		a0 = 0;
		nCellsOK = 0;
		for (i = 0; i < 10000; i++) {
			if (col >= model.physicalMapA.nCols)
				colUse = col - model.physicalMapA.nCols;
			else {
				if (col < 0)
					colUse = col + model.physicalMapA.nCols;
				else
					colUse = col;
			}
			if (model.physicalMapA.valueCell[row * model.physicalMapA.nCols + colUse] == 1)
				nCellsOK++;
			else
				nCellsOK = 0;
			if (nCellsOK >= 2)
				break;

			if (delta_col > 0)
				ac = (col + 1 - col1Dbl) / delta_col;
			else {
				if (delta_col == 0)
					ac = 999999;
				else {
					ac = (col - 1 - col1Dbl) / delta_col;
				}
			}
			if (delta_row > 0)
				ar = (row + 1 - row1Dbl) / delta_row;
			else {
				if (delta_row == 0)
					ar = 999999;
				else
					ar = (row - row1Dbl) / delta_row;
			}
			if (ac < ar)
				a1 = ac;
			else
				a1 = ar;
			if (a1 > 1)
				a1 = 1;

			if (a1 >= 0.9999)
				break;

			colDbl = col1Dbl + a1 * delta_col;
			rowDbl = row1Dbl + a1 * delta_row;

			col = (int)(colDbl + 0.0001 * delta_col);
			if (colDbl < 0)
				colDbl = colDbl;
			row = (int)(rowDbl + 0.0001 * delta_row);
			if (rowDbl < 0)
				rowDbl = rowDbl;
		}
		if (nCellsOK >= 2) {
			dist = sqrt((colDbl - col1Dbl) * (colDbl - col1Dbl) + (rowDbl - row1Dbl) * (rowDbl - row1Dbl));
			if(dist < bastDist) {
				bastDist = dist;
				bastCol = colDbl;
				bastRow = rowDbl;
			}
		}
	}
	if (bastDist < 99999) {
		getLatLonFromPhysicalMap(model.physicalMapA, &lat, &lon, bastRow, bastCol);
		model.network.physicalLev[level].point[pos] = spherical::Point(lat, lon);

		return 1;
	}
	else
		return 0;
}

int makeSure_feasibleNodes(int level, int mittPos) {
	int i, isFeasible;

	for (i = 0; i < model.network.physicalLev[level].nPoints; i++) {
		if (i == 35)
			i = i;
		if (check_nodeIsWithinPhysicalMapRaster(model.network.physicalLev[level].point[i].latitude().degrees(),
			model.network.physicalLev[level].point[i].longitude().degrees()) == 0) {
			model.network.physicalLev[level].allowedPoint[i] = 0;
			continue;
		}
			
		isFeasible = check_feasibleNodeRasterA(model.network.physicalLev[level].point[i].latitude().degrees(),
			model.network.physicalLev[level].point[i].longitude().degrees());
		if (isFeasible == 0 && model.params.preferredPathOrtoPos[level] != i) {
			isFeasible = movePointToFeasible(level, i);
			model.network.physicalLev[level].allowedPoint[i] = isFeasible;
		}
	}


	return 0;
}

int createPhysicalNetwork(int sparaKorridorEnbart)
{
	int i, nInt, nPkterOrto;
	double dist, distTot, nIntDbl, distInt;
	double ortoDist, bNy, distNu1, distNu2, distNu3;
	double distIntersect, distPrefPath, distNu, distCorridorSegm, distIntersectCorridor;
	int i1, i2, i3;
	spherical::Point pointNu;

	distTot = 0;
	for (i = 1; i < model.preferredPath.nPoints; i++) {
		dist = model.preferredPath.point[i - 1].distanceTo(model.preferredPath.point[i]) / 1000;
		distTot += dist;
	}
	errlog("tot haversine dist of prefered path %lf nPoints in prefPath %d\n", 
		distTot, model.preferredPath.nPoints);

	nIntDbl = distTot / model.params.shipSpeed_average / model.params.nHours_changeCourseInterval;
	errlog("nHoursIntervals: %lf", model.params.nHours_changeCourseInterval);
	nInt = (int)ceil(nIntDbl);
	distInt = distTot / nInt * (1 + model.params.epsilon); // # add a small number so there will be no nodes orthogonal to the last one
	errlog(" nIntDbl %lf nInt %d distInt %lf\n", nIntDbl, nInt, distInt);
	model.params.basDistArcs = distInt;


	int nIntervallPoints, nAllocPoints, posNu;
	spherical::Point* intervallPoint;
	double distKvar, kvot;

	//initCoordUsage();

	intervallPoint = (spherical::Point*)malloc((nInt + 1) * sizeof(spherical::Point));
	nIntervallPoints = 0;
	intervallPoint[nIntervallPoints] = model.preferredPath.point[0];
	nIntervallPoints++;

	model.network.physicalLev = (strNodeSeq*)malloc((nInt + 1) * sizeof(strNodeSeq));
	nAllocPoints = 100;
	model.network.physicalLev[0].preferredPathPoint = (spherical::Point*)malloc(nAllocPoints * sizeof(spherical::Point));
	model.network.physicalLev[0].npreferredPathPoints = 0;
	// add nodes at even distances along given path
	dist = 0;
	posNu = 0;
	for (i = 1; i < model.preferredPath.nPoints; i++) {
		distNu = model.preferredPath.point[i - 1].distanceTo(model.preferredPath.point[i]) / 1000.0;
		distKvar = distNu;
		while (dist + distKvar >= distInt) {
			// add an intervall here
			kvot = (distInt - dist + distNu - distKvar) / distNu;
			auto pMid = model.preferredPath.point[i - 1].intermediatePointTo(
				model.preferredPath.point[i], kvot); // 51.3721°N, 000.7073°E
			intervallPoint[nIntervallPoints] = pMid;
			model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu] = pMid;
			posNu++;
			model.network.physicalLev[nIntervallPoints - 1].npreferredPathPoints = posNu;
			if (posNu + 1 > model.network.nMaxNodesInPath)
				model.network.nMaxNodesInPath = posNu + 1;
			posNu = 0;
			nIntervallPoints++;
			nAllocPoints = 100;
			model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint = (spherical::Point*)malloc(nAllocPoints * sizeof(spherical::Point));
			if (dist > distInt)
				dist -= distInt;
			else {
				distKvar -= distInt - dist;
				dist = 0;
			}
		}
		if (dist + distKvar > 0.1) {
			if (posNu >= nAllocPoints - 1) {
				nAllocPoints += 100;
				model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint = (spherical::Point*)realloc(
					model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint, nAllocPoints * sizeof(spherical::Point));
			}
			model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu] = model.preferredPath.point[i];
			posNu++;
		}
		dist += distKvar;
	}
	if (nIntervallPoints >= nInt + 1)
		errlog("ERROR! Memory problem. Fix this on code row %d\n", __LINE__);
	intervallPoint[nIntervallPoints] = model.preferredPath.point[model.preferredPath.nPoints - 1];
	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu] = intervallPoint[nIntervallPoints];
	posNu++;
	model.network.physicalLev[nIntervallPoints - 1].npreferredPathPoints = posNu;
	if (posNu + 1 > model.network.nMaxNodesInPath)
		model.network.nMaxNodesInPath = posNu + 1;
	nIntervallPoints++;
	//writePointsToShape((char*)"intervallPoints", intervallPoint, nIntervallPoints);
	//writePointsToGeojson((char*)"intervallPoints", intervallPoint, nIntervallPoints);

	ortoDist = model.params.shipSpeed_average * 1000 / model.params.ortoDist_nPointsPerHour;
	FILE* filtmp;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/tmpCheck.txt", model.params.resultPath.c_str());
	filtmp = fopen(namn, "w");
	model.params.preferredPathOrtoPos = (int*)malloc(nIntervallPoints * sizeof(int));

	distTot = 0;
	for (i = 0; i < nIntervallPoints; i++) {
		if (i > 0)
			distTot += intervallPoint[i - 1].distanceTo(intervallPoint[i]) / 1000.0;
		model.network.physicalLev[i].distTot = distTot;

		if (i == nIntervallPoints - 2)
			i = i;
		if (i == 0 || i == nIntervallPoints - 1) {
			model.network.physicalLev[i].nPoints = 0;
			model.network.physicalLev[i].point = (spherical::Point*)malloc(sizeof(spherical::Point));
			model.network.physicalLev[i].allowedPoint = (int*)malloc(sizeof(int));
			model.network.physicalLev[i].point[model.network.physicalLev[i].nPoints] = intervallPoint[i];
			//updateCoordUsage(model.network.physicalLev[i].point[model.network.physicalLev[i].nPoints]);
			(model.network.physicalLev[i].nPoints)++;
			if (model.params.preferredPath_followExactOK == 1)
				model.params.preferredPathOrtoPos[i] = 0;
			else
				model.params.preferredPathOrtoPos[i] = -1;
			for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
				model.network.physicalLev[i].allowedPoint[i1] = 1;
			}
		}
		else {
			model.network.physicalLev[i].point = (spherical::Point*)malloc(model.params.nPkterOrto * sizeof(spherical::Point));
			model.network.physicalLev[i].allowedPoint = (int*)malloc(model.params.nPkterOrto * sizeof(int));
			//\example
			//	Point p1{ 52.205, 0.119 };
			//Point p2{ 48.857, 2.351 };
			// get bearing at the point
			auto b1 = intervallPoint[i - 1].bearingTo(intervallPoint[i]); // 157.9°
			// modify the bearing
			auto b2 = intervallPoint[i - 1].finalBearingTo(intervallPoint[i]); // 157.9°
			// modify the bearing
			bNy = (b1 + b2) / 2 + 90; // + M_PI / 2;
			if (bNy >= 360)
				bNy -= 360;
			//\example
			//	Point p1{ 51.4778, -0.0015 };
			//Point p2 = p1.destinationPoint(7794, 300.7); // 51.5135°N, 000.0983°W


			nPkterOrto = model.params.nPkterOrto;

			for (int i1 = 0; i1 < nPkterOrto; i1++) {
				model.network.physicalLev[i].point[i1] = intervallPoint[i].destinationPoint(
					ortoDist * (i1 - (nPkterOrto - 1) / 2), bNy);
				//updateCoordUsage(model.network.physicalLev[i].point[i1]);
				model.network.physicalLev[i].allowedPoint[i1] = 1;
			}
			model.network.physicalLev[i].nPoints = nPkterOrto;
			if (model.params.preferredPath_followExactOK == 1)
				model.params.preferredPathOrtoPos[i] = (int)(nPkterOrto / 2);
			else
				model.params.preferredPathOrtoPos[i] = -1;

			//writePointsToShape((char*)"shapeTest", model.network.physicalLev[i].point, model.network.physicalLev[i].nPoints);
		}
	}
	model.network.nPhysicalLevels = i;
	fclose(filtmp);

	openNeededRasterFiles();
	for(i = 0; i < model.network.nPhysicalLevels; i++)
		makeSure_feasibleNodes(i, (int)(nPkterOrto / 2));



	if (sparaKorridorEnbart == 1) {
		writeKorridorToGeojson((char*)"CorridorTmp");
		return 0;
	}


	//writeAllNodesToShape((char*)"networkNodes");
	writeAllNodesToGeojson((char*)"networkNodes");

	addArcsToNetwork();

	int saveArcs = 1;
	if (saveArcs == 1)
		writeAllArcsToGeojson((char*)"networkArcs");


	return 0;
}

int adderaNod(int physicalLevel, int pointNr, int timeInterval)
{
	int nAlloc;
	if (model.nNoder >= model.nAllocNoder) {
		model.nAllocNoder += 50000;
		model.Noder = (strNoder*)realloc(model.Noder, model.nAllocNoder * sizeof(strNoder));
	}

	if (physicalLevel >= 0) {
		if (model.params.nPkterOrto > 2 * model.params.max_changeDirection + 1)
			nAlloc = 2 * model.params.max_changeDirection + 1;
		else
			nAlloc = model.params.nPkterOrto;
		nAlloc *= model.params.nShip_speedSettings;
		if (physicalLevel < model.network.nPhysicalLevels)
			model.network.physicalLev[physicalLevel].nodNr_from_pt[pointNr][timeInterval] = model.nNoder;
	}
	else {
		nAlloc = model.params.nPkterOrto * 2;
		nAlloc *= model.params.nShip_speedSettings;
		model.network.channel[-physicalLevel - 1].nodNr_from_pt[pointNr][timeInterval] = model.nNoder;
	}
	model.Noder[model.nNoder].UtNod = (int*)malloc(nAlloc * sizeof(int));
	model.Noder[model.nNoder].UtNodCost = (double*)malloc(nAlloc * sizeof(double));
	model.Noder[model.nNoder].outArcNr = (int*)malloc(nAlloc * sizeof(int));
	//model.Noder[model.nNoder].outArcPos = (int*)malloc(model.params.ortoDist_nPointsPerHour * model.params.nShip_speedSettings * sizeof(int));
	model.Noder[model.nNoder].nAllocUtNoder = nAlloc;
	model.Noder[model.nNoder].nUtNoder = 0;
	model.Noder[model.nNoder].physicalLevel = physicalLevel;
	model.Noder[model.nNoder].pointNr = pointNr;
	model.Noder[model.nNoder].timeInterval = timeInterval;

	(model.nNoder)++;
	return model.nNoder - 1;
}

int adderaArc(int nodNr1, int nodNr2, double cost)
{
	int i;
	if (model.params.speedSettings_addOnlyCheapestArcs == 1) {
		for (i = model.Noder[nodNr1].nUtNoder - 1; i >= 0; i--) {
			if (model.Noder[nodNr1].UtNod[i] == nodNr2) {
				if (cost < model.Noder[nodNr1].UtNodCost[i]) {
					// this speed setting is cheaper than the old one...
					model.Noder[nodNr1].UtNodCost[i] = cost;
					if (cost < 0)
						printf("ERROR cost negative %.2lf\n", cost);
					return model.Noder[nodNr1].outArcNr[i];
				}
				else {
					return -2;
				}
			}
			if (model.Noder[model.Noder[nodNr1].UtNod[i]].pointNr != model.Noder[nodNr2].pointNr)
				break;
		}
	}
	//if (nodNr2 > 90000)
	//	errlog("ERROR! NodNr2 %d\n", nodNr2);

	if (model.Noder[nodNr1].nUtNoder >= model.Noder[nodNr1].nAllocUtNoder) {
		model.Noder[nodNr1].nAllocUtNoder += 20;
		model.Noder[nodNr1].UtNod = (int*)realloc(model.Noder[nodNr1].UtNod, model.Noder[nodNr1].nAllocUtNoder * sizeof(int));
		model.Noder[nodNr1].UtNodCost = (double*)realloc(model.Noder[nodNr1].UtNodCost, model.Noder[nodNr1].nAllocUtNoder * sizeof(double));
		model.Noder[nodNr1].outArcNr = (int*)realloc(model.Noder[nodNr1].outArcNr, model.Noder[nodNr1].nAllocUtNoder * sizeof(int));
	}
	model.Noder[nodNr1].UtNod[model.Noder[nodNr1].nUtNoder] = nodNr2;
	//if (model.nArcs == 93099)
	//	freeMemory();
	model.Noder[nodNr1].UtNodCost[model.Noder[nodNr1].nUtNoder] = cost;
	if (cost < 0)
		printf("ERROR negative cost %.2lf\n", cost);
	model.Noder[nodNr1].outArcNr[model.Noder[nodNr1].nUtNoder] = model.nArcs;
	(model.Noder[nodNr1].nUtNoder)++;
	return -1;
}


int addTimeTo_timeInterval(int levPrev, int levNr, int pointNr, int tidInt)
{
	int i;
	if (levNr >= 0) {
		for (i = 0; i < model.network.physicalLev[levNr].nTimeIntervals[pointNr]; i++) {
			if (tidInt == model.network.physicalLev[levNr].timeInterval[pointNr][i])
				break;
		}
		if (i >= model.network.physicalLev[levNr].nTimeIntervals[pointNr]) {
			if (i >= model.network.physicalLev[levNr].nAllocTimeIntervals[pointNr]) {
				if (levNr == 16)
					levNr = levNr;
				model.network.physicalLev[levNr].nAllocTimeIntervals[pointNr] += 100;
				model.network.physicalLev[levNr].timeInterval[pointNr] = (int*)realloc(
					model.network.physicalLev[levNr].timeInterval[pointNr],
					model.network.physicalLev[levNr].nAllocTimeIntervals[pointNr] * sizeof(int));
				model.network.physicalLev[levNr].nodNr_from_pt[pointNr] = (int*)realloc(
					model.network.physicalLev[levNr].nodNr_from_pt[pointNr],
					model.network.physicalLev[levNr].nAllocTimeIntervals[pointNr] * sizeof(int));
			}
			model.network.physicalLev[levNr].timeInterval[pointNr][i] = tidInt;
			(model.network.physicalLev[levNr].nTimeIntervals[pointNr])++;
			if (levNr == 1)
				levNr = levNr;
			return adderaNod(levNr, pointNr, i);
		}
		return model.network.physicalLev[levNr].nodNr_from_pt[pointNr][i];
	}
	else {
		if (levPrev < 0)
			pointNr = 1; // endpoint in channel
		else
			pointNr = 0; // startpoint in channel
		for (i = 0; i < model.network.channel[-levNr - 1].nTimeIntervals[pointNr]; i++) {
			if (tidInt == model.network.channel[-levNr - 1].timeInterval[pointNr][i])
				break;
		}
		if (i >= model.network.channel[-levNr - 1].nTimeIntervals[pointNr]) {
			if (i >= model.network.channel[-levNr - 1].nAllocTimeIntervals[pointNr]) {
				if (levNr == 16)
					levNr = levNr;
				model.network.channel[-levNr - 1].nAllocTimeIntervals[pointNr] += 100;
				model.network.channel[-levNr - 1].timeInterval[pointNr] = (int*)realloc(
					model.network.channel[-levNr - 1].timeInterval[pointNr],
					model.network.channel[-levNr - 1].nAllocTimeIntervals[pointNr] * sizeof(int));
				model.network.channel[-levNr - 1].nodNr_from_pt[pointNr] = (int*)realloc(
					model.network.channel[-levNr - 1].nodNr_from_pt[pointNr],
					model.network.channel[-levNr - 1].nAllocTimeIntervals[pointNr] * sizeof(int));
			}
			model.network.channel[-levNr - 1].timeInterval[pointNr][i] = tidInt;
			(model.network.channel[-levNr - 1].nTimeIntervals[pointNr])++;
			if (levNr == 1)
				levNr = levNr;
			return adderaNod(levNr, pointNr, i);
		}
		return model.network.channel[-levNr - 1].nodNr_from_pt[pointNr][i];
	}
}

int testCoordValue(int weatherNr, double lon, double lat) {
	double rowDbl, colDbl;
	int row, col, tidp;

	rowDbl = (model.weather[weatherNr].maxY - lat) / model.weather[weatherNr].size_row;
	colDbl = get_colDblFromWeatherFile(weatherNr, lon);
	row = (int)rowDbl;
	col = (int)colDbl;

	for (tidp = 0; tidp < 6; tidp++)
		printf("%s orig lon/lat %.3lf %.3lf size_col/row %.2lf %.2lf value 0 0/1 1 %lf %lf\n",
		model.weather[weatherNr].weatherFileTypeName,
		model.weather[weatherNr].minX, model.weather[weatherNr].maxY,
		model.weather[weatherNr].size_col, model.weather[weatherNr].size_row,
		model.weather[weatherNr].valueCell[tidp][0 * model.weather[weatherNr].nCols + 0],
		model.weather[weatherNr].valueCell[tidp][1 * model.weather[weatherNr].nCols + 1]);

	for (tidp = 0; tidp < 10; tidp++)
		printf("lon/lat %.3lf %.3lf col/row %.2lf %.2lf tidint %d %lf %lf %lf %lf\n",
			lon, lat,
			colDbl, rowDbl, tidp,
			model.weather[weatherNr].valueCell[tidp][row * model.weather[weatherNr].nCols + col],
			model.weather[weatherNr].valueCell[tidp][row * model.weather[weatherNr].nCols + col+1],
			model.weather[weatherNr].valueCell[tidp][row * model.weather[weatherNr].nCols + col+2],
			model.weather[weatherNr].valueCell[tidp][(row+1) * model.weather[weatherNr].nCols + col]);


	return 0;
}

int calcWeatherPosAlongArc(spherical::Point p1, spherical::Point p2)
{
	int i, i1;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, bearingRadians;
	spherical::Point pMid, pTmp;

	if (printGlobal == 1)
		printf("lat/lon p1 %.3lf %.3lf p2 %.3lf %.3lf\n",
			p1.latitude().degrees(), p1.longitude().degrees(),
			p2.latitude().degrees(), p2.longitude().degrees());


	totDist = p1.distanceTo(p2) / 1000;
	dist = model.params.shipSpeed_average;
	distHittils = 0;
	pMid = p1;
	bearing = pMid.bearingTo(p2);
	bearingRadians = (90 - pMid.bearingTo(p2)) * M_PI / 180;
	if (bearingRadians < -M_PI)
		bearingRadians += 2 * M_PI;

	for (i = 0;; i++) {
		if (i >= 98)
			i = i;
		if (i >= model.weatherFunctions.nAllocPoints) {
			model.weatherFunctions.nAllocPoints += 50;
			model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
				model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
			}
		}

		if (printGlobal == 1)
			printf("i %d lat/lon pMid %.3lf %.3lf bearing %.2lf bearingRadians %.2lf distHittils %.2lf totDist %.2lf\n", i,
				pMid.latitude().degrees(), pMid.longitude().degrees(), bearing, bearingRadians, distHittils, totDist);
		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			if (model.weatherFunctions.checkPoint[i].latPos[i1] < 0) {
				errlog("ERROR! latPos %d\n", model.weatherFunctions.checkPoint[i].latPos[i1]);
				model.weatherFunctions.checkPoint[i].latPos[i1] = 0;
			}
			if (model.weatherFunctions.checkPoint[i].latPos[i1] >=  model.weather[i1].nRows) {
				errlog("ERROR! latPos too high %d (max %d)\n", model.weatherFunctions.checkPoint[i].latPos[i1],
					model.weather[i1].nRows - 1);
				model.weatherFunctions.checkPoint[i].latPos[i1] = model.weather[i1].nRows - 1;
			}
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			if (model.weatherFunctions.checkPoint[i].lonPos[i1] < 0) {
				errlog("ERROR! lonPos %d\n", model.weatherFunctions.checkPoint[i].lonPos[i1]);
				model.weatherFunctions.checkPoint[i].lonPos[i1] = 0;
			}
			if (model.weatherFunctions.checkPoint[i].lonPos[i1] >= model.weather[i1].nCols) {
				errlog("ERROR! lonPos too high %d (max %d)\n", model.weatherFunctions.checkPoint[i].lonPos[i1],
					model.weather[i1].nCols - 1);
				model.weatherFunctions.checkPoint[i].lonPos[i1] = model.weather[i1].nCols - 1;
			}
			//model.weatherFunctions.checkPoint[i].fileNr[i1] = nr;
			//			errlog("checkP %d lat %.3lf lon %.3lf weatherf %d row %d col %d\n", i, pMid.latitude().degrees(),
//				pMid.longitude().degrees(), i1, model.weatherFunctions.checkPoint[i].latPos[i1],
//				model.weatherFunctions.checkPoint[i].lonPos[i1]);
		}
		model.weatherFunctions.vesselBearing[i] = bearingRadians;
		//printf("alongArc bearing i %d from %.3lf %.3lf to %.3lf %.3lf is %.2lf\n", i,
		//	pMid.longitude().degrees(), pMid.latitude().degrees(),
		//	p2.longitude().degrees(),
		//	p2.latitude().degrees(), bearing * 180 / M_PI);
		// model.weatherFunctions.vVesselDirection = sin(bearing * M_PI / 180);
		model.weatherFunctions.point[i] = pMid;
		//		errlog("checkP %d lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", i, pMid.latitude().degrees(),
		//			pMid.longitude().degrees(), distHittils, bearing);
		if (distHittils + dist * 1.05 < totDist) {
			model.weatherFunctions.checkPoint[i].distToNextPkt = dist;
			distHittils += dist;
			// bearing = pMid.bearingTo(p2);

			//pTmp = pMid;
			pMid = p1.destinationPoint(distHittils * 1000, bearing);
			//distTmp = calc_haversine_dist_latlon(pTmp.latitude().degrees(),
			//	pTmp.longitude().degrees(), pMid.latitude().degrees(),
			//	pMid.longitude().degrees());
			//distTmp2 = pTmp.distanceTo(pMid);

		}
		else {
			model.weatherFunctions.checkPoint[i].distToNextPkt = totDist - distHittils;
			//distTmp = calc_haversine_dist_latlon(pMid.latitude().degrees(),
			//	pMid.longitude().degrees(), p2.latitude().degrees(),
			//	p2.longitude().degrees());
			//distTmp2 = pMid.distanceTo(p2);
			//			errlog("lastP P2 lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", p2.latitude().degrees(),
//				p2.longitude().degrees(), distHittils, bearing);
			i++;
			break;
		}
	}
	//	errlog("lastP lat %.3lf lon %.3lf totDist %lf\n", p2.latitude().degrees(),
	//		p2.longitude().degrees(), totDist);
	model.weatherFunctions.nCheckPoints = i;
	//writePointsToShape((char*)"checkPoints", model.weatherFunctions.point, i);

	return 0;
}

spherical::Point getNextPointAlongChannel(int cNr, int* posNu, int endPos, double distHittils, double totDist)
{
	spherical::Point p1 = model.network.channel[cNr].point[endPos];
	int i;
	double dist = 0, bearing, distNu;

	for (i = *posNu + 1; i <= endPos; i++) {
		dist += (model.network.channel[cNr].distanceFromStart[i] - model.network.channel[cNr].distanceFromStart[i - 1]);
		if (dist >= totDist) {
			if (dist > totDist + model.params.epsilon) {
				bearing = model.network.channel[cNr].point[i - 1].bearingTo(model.network.channel[cNr].point[i - 1]);
				distNu = model.network.channel[cNr].distanceFromStart[i] - model.network.channel[cNr].distanceFromStart[i - 1] - (dist - totDist);
				p1 = model.network.channel[cNr].point[i - 1].destinationPoint(distNu * 1000.0, bearing);
				*posNu = i - 1;
			}
			else {
				p1 = model.network.channel[cNr].point[i];
				*posNu = i;
			}
			break;
		}
	}
	if (i > endPos)
		p1 = model.network.channel[cNr].point[endPos];
	return p1;
}

spherical::Point getNextPointAlongpreferredPathArc(spherical::Point pLast, int level, int* posNu, double distHittils, double totDist)
{
	spherical::Point pNu = pLast;
	int i;
	double dist = 0, bearing, distNu, distTmp;

	for (i = *posNu + 1; i < model.network.physicalLev[level].npreferredPathPoints; i++) {
		distTmp = pNu.distanceTo(model.network.physicalLev[level].preferredPathPoint[i]) / 1000.0;
		dist += distTmp;
		if (dist >= totDist) {
			if (dist > totDist + model.params.epsilon) {
				bearing = pNu.bearingTo(model.network.physicalLev[level].preferredPathPoint[i]);
				distNu = (distTmp - (dist - totDist));
				pNu = pNu.destinationPoint(distNu * 1000.0, bearing);
				*posNu = i - 1;
			}
			else {
				pNu = model.network.physicalLev[level].preferredPathPoint[i];
				*posNu = i;
			}
			break;
		}
		else
			pNu = model.network.physicalLev[level].preferredPathPoint[i];

	}
	return pNu;
}

int calcWeatherPosAlongChannel(int cNr)
{
	int i, i1, posLast;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl;
	spherical::Point pMid, pTmp;
	int pointPos1, pointPos2;

	pointPos1 = 0;
	pointPos2 = model.network.channel[cNr].nPoints - 1;
	totDist = model.network.channel[cNr].distanceFromStart[pointPos2] - model.network.channel[cNr].distanceFromStart[pointPos1]; //  p1.distanceTo(p2) / 1000;
	dist = model.params.shipSpeed_average;
	distHittils = 0;
	posLast = pointPos1;
	pMid = model.network.channel[cNr].point[posLast];
	bearing = (90 - pMid.bearingTo(model.network.channel[cNr].point[posLast + 1])) * M_PI / 180;
	if (bearing < -M_PI)
		bearing += 2 * M_PI;
	for (i = 0;; i++) {
		if (i >= 98)
			i = i;
		if (i >= model.weatherFunctions.nAllocPoints) {
			model.weatherFunctions.nAllocPoints += 50;
			model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
				model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
			}
		}
		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			//nr = model.weatherFunctions.lastFileNr[i1];
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			//model.weatherFunctions.checkPoint[i].fileNr[i1] = nr;
			//			errlog("checkP %d lat %.3lf lon %.3lf weatherf %d row %d col %d\n", i, pMid.latitude().degrees(),
//				pMid.longitude().degrees(), i1, model.weatherFunctions.checkPoint[i].latPos[i1],
//				model.weatherFunctions.checkPoint[i].lonPos[i1]);
		}
		model.weatherFunctions.vesselBearing[i] = bearing;
		//printf("channel bearing i %d from %.3lf %.3lf to %.3lf %.3lf is %.2lf\n", i,
		//	pMid.longitude().degrees(), pMid.latitude().degrees(),
		//	model.network.channel[cNr].point[posLast + 1].longitude().degrees(),
		//	model.network.channel[cNr].point[posLast + 1].latitude().degrees(), bearing * 180 / M_PI);
		// model.weatherFunctions.vVesselDirection = sin(bearing * M_PI / 180);
		model.weatherFunctions.point[i] = pMid;
		//		errlog("checkP %d lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", i, pMid.latitude().degrees(),
		//			pMid.longitude().degrees(), distHittils, bearing);
		if (distHittils + dist * 1.05 < totDist) {
			model.weatherFunctions.checkPoint[i].distToNextPkt = dist;
			distHittils += dist;
			// bearing = pMid.bearingTo(p2);

			// pMid = p1.destinationPoint(distHittils * 1000, bearing);
			pMid = getNextPointAlongChannel(cNr, &posLast, pointPos2, distHittils, dist);

			//distTmp = calc_haversine_dist_latlon(pTmp.latitude().degrees(),
			//	pTmp.longitude().degrees(), pMid.latitude().degrees(),
			//	pMid.longitude().degrees());
			//distTmp2 = pTmp.distanceTo(pMid);

		}
		else {
			model.weatherFunctions.checkPoint[i].distToNextPkt = totDist - distHittils;
			//distTmp = calc_haversine_dist_latlon(pMid.latitude().degrees(),
			//	pMid.longitude().degrees(), p2.latitude().degrees(),
			//	p2.longitude().degrees());
			//distTmp2 = pMid.distanceTo(p2);
			//			errlog("lastP P2 lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", p2.latitude().degrees(),
//				p2.longitude().degrees(), distHittils, bearing);
			i++;
			break;
		}
	}
	//	errlog("lastP lat %.3lf lon %.3lf totDist %lf\n", p2.latitude().degrees(),
	//		p2.longitude().degrees(), totDist);
	model.weatherFunctions.nCheckPoints = i;
	//writePointsToShape((char*)"checkPoints", model.weatherFunctions.point, i);

	return 0;
}

int calcWeatherPosAlongpreferredPathArc(spherical::Point p1, int level)
{
	int i, i1, posLast;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl;
	spherical::Point pMid, pTmp;

	model.tmpTid3[0] = std::chrono::high_resolution_clock::now();
	totDist = 0;
	pMid = p1;
	for (i = 0; i < model.network.physicalLev[level].npreferredPathPoints; i++) {
		totDist += pMid.distanceTo(model.network.physicalLev[level].preferredPathPoint[i]) / 1000.0;
		if (i < model.network.physicalLev[level].npreferredPathPoints - 1)
			pMid = model.network.physicalLev[level].preferredPathPoint[i];
	}
	dist = model.params.shipSpeed_average;
	distHittils = 0;
	pMid = p1;
	posLast = 0;
	bearing = (90 - pMid.bearingTo(model.network.physicalLev[level].preferredPathPoint[posLast])) * M_PI / 180;
	if (bearing < -M_PI)
		bearing += 2 * M_PI;
	//printf("bearing from %.3lf %.3lf to %.3lf %.3lf is %.2lf\n",
	//	p1.longitude().degrees(), p1.latitude().degrees(),
	//	model.network.physicalLev[level].preferredPathPoint[posLast].longitude().degrees(), 
	//	model.network.physicalLev[level].preferredPathPoint[posLast].latitude().degrees(), bearing);
	posLast = -1;
	for (i = 0;; i++) {
		if (i >= 98)
			i = i;
		if (i >= model.weatherFunctions.nAllocPoints) {
			model.weatherFunctions.nAllocPoints += 50;
			model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
				model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
			}
		}
		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			//nr = model.weatherFunctions.lastFileNr[i1];
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			//model.weatherFunctions.checkPoint[i].fileNr[i1] = nr;
			//			errlog("checkP %d lat %.3lf lon %.3lf weatherf %d row %d col %d\n", i, pMid.latitude().degrees(),
//				pMid.longitude().degrees(), i1, model.weatherFunctions.checkPoint[i].latPos[i1],
//				model.weatherFunctions.checkPoint[i].lonPos[i1]);
		}
		model.weatherFunctions.vesselBearing[i] = bearing;
		//printf("prefPath bearing i %d from %.3lf %.3lf to %.3lf %.3lf is %.2lf\n",i, 
		//	pMid.longitude().degrees(), pMid.latitude().degrees(),
		//	model.network.physicalLev[level].preferredPathPoint[0].longitude().degrees(),
		//	model.network.physicalLev[level].preferredPathPoint[0].latitude().degrees(), bearing * 180 / M_PI);
		// model.weatherFunctions.vVesselDirection = sin(bearing * M_PI / 180);
		model.weatherFunctions.point[i] = pMid;
		//		errlog("checkP %d lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", i, pMid.latitude().degrees(),
		//			pMid.longitude().degrees(), distHittils, bearing);
		if (distHittils + dist * 1.05 < totDist) {
			model.weatherFunctions.checkPoint[i].distToNextPkt = dist;
			distHittils += dist;
			pMid = getNextPointAlongpreferredPathArc(pMid, level, &posLast, distHittils, dist);

		}
		else {
			model.weatherFunctions.checkPoint[i].distToNextPkt = totDist - distHittils;
			//distTmp = calc_haversine_dist_latlon(pMid.latitude().degrees(),
			//	pMid.longitude().degrees(), p2.latitude().degrees(),
			//	p2.longitude().degrees());
			//distTmp2 = pMid.distanceTo(p2);
			//			errlog("lastP P2 lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", p2.latitude().degrees(),
//				p2.longitude().degrees(), distHittils, bearing);
			i++;
			break;
		}
	}
	//	errlog("lastP lat %.3lf lon %.3lf totDist %lf\n", p2.latitude().degrees(),
	//		p2.longitude().degrees(), totDist);
	model.weatherFunctions.nCheckPoints = i;
	//writePointsToShape((char*)"checkPoints", model.weatherFunctions.point, i);
	model.tmpTid3[1] = std::chrono::high_resolution_clock::now();
	model.duration2 += model.tmpTid3[1] - model.tmpTid3[0];

	return 0;
}

/*
double calcTimeCostOld(int t, double speed, int determineWeatherPos, double* cost)
{
	double tidTot = t, distNu, tidTmp, costTmp, costTot = 0;
	double bearing, uWind, vWind, uCurrent, vCurrent, uVessel, vVessel; //  , uSpeed, vSpeed;
	double factorWind, factorCurrent;
	int i, i1, tidInt, tidInt2, latPos, lonPos, fileNr, weatherNr;
	int timePos_uWind, timePos_vWind, timePos_uCurrent, timePos_vCurrent;

	for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
		distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
		uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
		vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

		weatherNr = model.variable[0].weatherNr;
		latPos = model.weatherFunctions.checkPoint[i].latPos[weatherNr];
		lonPos = model.weatherFunctions.checkPoint[i].lonPos[weatherNr];
		//fileNr = model.weatherFunctions.checkPoint[i].fileNr[weatherNr];

		tidInt = (int)(tidTot / model.weather[weatherNr].timeIntervall_h);
		if (tidInt >= model.weather[weatherNr].nTimeIntervals) {
			errlog("ERROR! Too late time interval %d for weatherfile %d (latest %d). Implement a standard weather for the season and use...\n",
				tidInt, weatherNr, model.weather[weatherNr].nTimeIntervals - 1);
			tidInt = 0;
		}
		tidInt2 = model.weather[weatherNr].timeOrder[tidInt];
		timePos_uWind = tidInt2 * model.weather[weatherNr].nElement + model.variable[0].elementPos;
		if (model.weather[weatherNr].rasterBandData[timePos_uWind] == NULL)
			model.weather[weatherNr].rasterBandData[timePos_uWind] =
			model.weather[weatherNr].rasterPos.GetRasterBand(timePos_uWind + 1);
		uWind = model.weather[weatherNr].rasterBandData[timePos_uWind][latPos][lonPos];
		
		timePos_vWind = tidInt2 * model.weather[weatherNr].nElement + model.variable[1].elementPos;
		if (model.weather[weatherNr].rasterBandData[timePos_vWind] == NULL)
			model.weather[weatherNr].rasterBandData[timePos_vWind] =
			model.weather[weatherNr].rasterPos.GetRasterBand(timePos_vWind + 1);
		vWind = model.weather[weatherNr].rasterBandData[timePos_vWind][latPos][lonPos];
	
		errlog("checkP %d latPos %d lonPos %d timePos_uWind %d uWind %.2lf\n", i, latPos,
			lonPos, timePos_uWind, uWind);
		errlog("checkP %d latPos %d lonPos %d timePos_vWind %d vWind %.2lf\n", i, latPos,
			lonPos, timePos_vWind, vWind);


		weatherNr = model.variable[2].weatherNr;
		latPos = model.weatherFunctions.checkPoint[i].latPos[weatherNr];
		lonPos = model.weatherFunctions.checkPoint[i].lonPos[weatherNr];
		//fileNr = model.weatherFunctions.checkPoint[i].fileNr[weatherNr];

		tidInt = (int)(tidTot / model.weather[weatherNr].timeIntervall_h);
		if (tidInt >= model.weather[weatherNr].nTimeIntervals) {
			errlog("ERROR! Too late time interval %d for weatherfile %d (latest %d). Implement a standard weather for the season and use...",
				tidInt, weatherNr, model.weather[weatherNr].nTimeIntervals - 1);
			tidInt = 0;
		}
		tidInt2 = model.weather[weatherNr].timeOrder[tidInt];
		timePos_uCurrent = tidInt2 * model.weather[weatherNr].nElement + model.variable[2].elementPos;
		if (model.weather[weatherNr].rasterBandData[timePos_uCurrent] == NULL)
			model.weather[weatherNr].rasterBandData[timePos_uCurrent] =
			model.weather[weatherNr].rasterPos.GetRasterBand(timePos_uCurrent + 1);
		uCurrent = model.weather[weatherNr].rasterBandData[timePos_uCurrent][latPos][lonPos];
		timePos_vCurrent = tidInt2 * model.weather[weatherNr].nElement + model.variable[3].elementPos;
		if (model.weather[weatherNr].rasterBandData[timePos_vCurrent] == NULL)
			model.weather[weatherNr].rasterBandData[timePos_vCurrent] =
			model.weather[weatherNr].rasterPos.GetRasterBand(timePos_vCurrent + 1);
		vCurrent = model.weather[weatherNr].rasterBandData[timePos_vCurrent][latPos][lonPos];

		errlog("checkP %d latPos %d lonPos %d timePos_uCurrent %d uCurrent %.2lf\n", i, latPos,
			lonPos, timePos_uCurrent, uCurrent);
		errlog("checkP %d latPos %d lonPos %d timePos_vCurrent %d vCurrent %.2lf\n", i, latPos,
			lonPos, timePos_vCurrent, vCurrent);

		tidTmp = distNu / speed;
		costTmp = distNu * (1 + abs(15 - speed) / 10);

		factorWind = 1 + (uWind * uVessel + vWind * vVessel) / 50;
		if (factorWind < 0.8)
			factorWind = 0.8;
		if (factorWind > 1.2)
			factorWind = 1.2;

		factorCurrent = 1 + (uCurrent * uVessel + vCurrent * vVessel) / 1;
		if (factorCurrent < 0.7)
			factorCurrent = 0.7;
		if (factorCurrent > 1.3)
			factorCurrent = 1.3;
		errlog("tidTmp %.2lf costTmp %.2lf efter factor %.2lf %.2lf factors %.3lf %.3lf\n",
			tidTmp, costTmp, tidTmp * factorWind * factorCurrent, costTmp * factorWind * factorCurrent,
			factorWind, factorCurrent);

		tidTmp *= factorWind * factorCurrent;
		costTmp *= factorWind * factorCurrent;
		tidTot += tidTmp;
		costTot += costTmp;
	}
	*cost = costTot;
	errlog("speed %.2lf tidStart %d tidSlut %.2lf cost %.2lf\n",
		speed, t, tidTot, costTot);


	return tidTot;
}
*/

double getVariableValue(int varNr, int checkPointNr, double tidpkt)
{
	//int weatherNr = model.variable[varNr].weatherNr;
	int latPos = model.weatherFunctions.checkPoint[checkPointNr].latPos[varNr];
	int lonPos = model.weatherFunctions.checkPoint[checkPointNr].lonPos[varNr];
	int tidInt, tidIndex;

	//if (model.weather[varNr].useStandardWeather != 0) {
	//	tidInt = -1;
	//	if (model.weather[varNr].useStandardWeather == -1)
	//		tidIndex = 0;
	//	else {
	//		tidIndex = model.weather[varNr].nTimeIntervals - 1;
	//	}
	//}
	//else {
		tidInt = (int)(tidpkt / model.weather[varNr].timeIntervall_h);
		if (tidInt > model.weather[varNr].nTimeIntervals_maxValue)
			tidInt = model.weather[varNr].nTimeIntervals_maxValue;
		tidIndex = model.weather[varNr].timeIntervalIndex[tidInt];
	//}
	//return model.weather[weatherNr].rasterBandData[timePos][latPos][lonPos];
	//printf("getVariableValue varNr %d tidInt %d latPos %d lonPos %d max %d %d\n",
	//	varNr, tidInt, latPos, lonPos, model.weather[varNr].nCols, model.weather[varNr].nRows);
	//printf("varNr %d tidpkt %.2lf secEpoch %.0lf tidInt %d tidIndex %d timeInt_h %.2lf latPos %d lonPos %d posBer %d val %.6lf"
	//	" minMaxLat %.3lf %.3lf minMaxLon %.3lf %.3lf\n", 
	//	varNr, tidpkt, tidpkt * 3600 + model.params.UTC_secondsStart,
	//	tidInt, tidIndex, model.weather[varNr].timeIntervall_h, latPos, lonPos,
	//	latPos * model.weather[varNr].nCols + lonPos,
	//	model.weather[varNr].valueCell[tidIndex][latPos * model.weather[varNr].nCols + lonPos],
	//	model.weather[varNr].maxY - (latPos + 1) * model.weather[varNr].size_row,
	//	model.weather[varNr].maxY - latPos * model.weather[varNr].size_row,
	//	model.weather[varNr].minX + lonPos * model.weather[varNr].size_col, 
	//	model.weather[varNr].minX + (lonPos + 1) * model.weather[varNr].size_col);
		if (printGlobal == 1)
			printf("varNr %d tidIndex %d latPos %d lonPos %d\n", varNr, tidIndex, latPos, lonPos);
	return model.weather[varNr].valueCell[tidIndex][latPos * model.weather[varNr].nCols + lonPos];
}

int getRadierToStorm(strStormQuadr* quadrant, double bearing, int nivaNoGo, int niva34, double* radieNoGo, double* radie34)
{
	int kvadrant;

	if (nivaNoGo >= 0 || niva34 >= 0) {
		kvadrant = (int)(bearing / 90);
		if (kvadrant == 0) {
			if (nivaNoGo >= 0)
				*radieNoGo = quadrant[nivaNoGo].NE;
			else
				*radieNoGo = 0;
			if (niva34 >= 0)
				*radie34 = quadrant[niva34].NE;
			else
				*radie34 = 0;
		}
		else if (kvadrant == 1) {
			if (nivaNoGo >= 0)
				*radieNoGo = quadrant[nivaNoGo].SE;
			else
				*radieNoGo = 0;
			if (niva34 >= 0)
				*radie34 = quadrant[niva34].SE;
			else
				*radie34 = 0;
		}
		else if (kvadrant == 2) {
			if (nivaNoGo >= 0)
				*radieNoGo = quadrant[nivaNoGo].SW;
			else
				*radieNoGo = 0;
			if (niva34 >= 0)
				*radie34 = quadrant[niva34].SW;
			else
				*radie34 = 0;
		}
		else {
			if (nivaNoGo >= 0)
				*radieNoGo = quadrant[nivaNoGo].NW;
			else
				*radieNoGo = 0;
			if (niva34 >= 0)
				*radie34 = quadrant[niva34].NW;
			else
				*radie34 = 0;
		}
	}
	else {
		*radieNoGo = 0;
		*radie34 = 0;
	}

	return 0;
}

/*
double getStormValue_old(int t, spherical::Point point)
{
	int i, pos, riktning, i1, nivaNoGo, niva34;
	double varde = 0, vardeNu, dist, bearingFromStorm, bearingStormMove;
	double bearingDiff, wind, wind2, radieExtraNoGo, radieExtra34;

	for (i = 0; i < model.nStorms; i++) {
		pos = t / model.storms[i].tidsIntervall;
		if (pos < model.storms[i].nFeatures) {
			dist = model.storms[i].feature[pos].midPoint.distanceTo(point) / 1852; // rakna om till nautiska miles...
			if (dist >= model.params.storm_2dist_ahead)
				continue; // too far from storm, no problem
			bearingFromStorm = model.storms[i].feature[pos].midPoint.bearingTo(point);
			bearingStormMove = model.storms[i].feature[pos].bearing;
			bearingDiff = bearingFromStorm - bearingStormMove;
			if (bearingDiff >= 360)
				bearingDiff -= 360;
			if (bearingDiff < 0)
				bearingDiff += 360;
			if (bearingDiff < 90 || bearingDiff > 270)
				riktning = 1; // framfor
			else
				riktning = -1; // bakom

			wind = model.storms[i].feature[pos].maxWind;
			if (wind >= model.params.storm_windUBD - 0.1)
				nivaNoGo = -1;
			else
				nivaNoGo = -2;
			for (i1 = 0; i1 < model.storms[i].feature[pos].nQuadrants; i1++) {
				wind2 = model.storms[i].feature[pos].quadrant[i1].windMaxRadius;
				if (wind2 >= model.params.storm_windUBD - 0.1)
					nivaNoGo = i1;
			}
			niva34 = model.storms[i].feature[pos].nQuadrants - 1;

			getRadierToStorm(model.storms[i].feature[pos].quadrant, bearingFromStorm, nivaNoGo, niva34, &radieExtraNoGo, &radieExtra34);

			if (riktning == 1) {
				if (dist - radieExtraNoGo >= model.params.storm_2dist_ahead)
					continue; // too far from storm, no problem
				if (dist - radieExtraNoGo < model.params.storm_1dist_ahead) {
					return model.params.storm_1costInside_ahead; // A and B
				}
				if (dist - radieExtra34 < 0) { // C
					if (radieExtra34 > radieExtraNoGo)
						vardeNu = model.params.storm_2cost_ahead + model.params.storm_1cost_ahead * (radieExtra34 - dist) / (radieExtra34 - radieExtraNoGo);
					else
						vardeNu = model.params.storm_2cost_ahead + model.params.storm_1cost_ahead;
				}
				else {
					if (model.params.storm_2dist_ahead - (radieExtra34 - radieExtraNoGo) > 0)
						vardeNu = model.params.storm_2cost_ahead * (1 - (dist - radieExtra34) / (model.params.storm_2dist_ahead - (radieExtra34 - radieExtraNoGo)));
					else
						vardeNu = model.params.storm_2cost_ahead;
				}
			}
			else {
				if (dist - radieExtraNoGo >= model.params.storm_2dist_behind)
					continue; // too far from storm, no problem
				if (dist - radieExtraNoGo < model.params.storm_1dist_behind) {
					return model.params.storm_1costInside_behind; // A and B
				}
				if (dist - radieExtra34 < 0) { // C
					if (radieExtra34 > radieExtraNoGo)
						vardeNu = model.params.storm_2cost_behind + model.params.storm_1cost_behind * (radieExtra34 - dist) / (radieExtra34 - radieExtraNoGo);
					else
						vardeNu = model.params.storm_2cost_behind + model.params.storm_1cost_behind;
				}
				else {
					if (model.params.storm_2dist_behind - (radieExtra34 - radieExtraNoGo) > 0)
						vardeNu = model.params.storm_2cost_behind * (1 - (dist - radieExtra34) / (model.params.storm_2dist_behind - (radieExtra34 - radieExtraNoGo)));
					else
						vardeNu = model.params.storm_2cost_behind;
				}
			}
			if (vardeNu > varde)
				varde = vardeNu;

		}

	}



	return varde;
}
*/

double getStormValue(int t, spherical::Point point)
{
	int i, tidIndex, tidInt, posNu;
	double varde = 0, kvot, dist, bearingFromStorm, bearingStormMove;
	double bearingDiff, distanceMove;
	double outerCircleSize, outerCircleNext, innerCircleSize, innerCircleNext;
	spherical::Point pointStorm;


	for (i = 0; i < model.nStorms; i++) {
		tidInt = t / model.storms[i].timeIntervall_h;
		if (tidInt > model.storms[i].nTimeIntervals_maxValue)
			tidInt = model.storms[i].nTimeIntervals_maxValue;
		tidIndex = model.storms[i].timeIntervalIndex[tidInt];
		if (tidIndex < model.storms[i].nFeatures) {
			kvot = (t - model.storms[i].feature[tidIndex].tidFromStart_h) / model.storms[i].feature[tidIndex].hoursToNextPoint;
			bearingStormMove = model.storms[i].feature[tidIndex].bearing;
			if (kvot > 0.05) {
				// distanceMove = model.storms[i].feature[pos].midPoint.distanceTo(model.storms[i].feature[pos + 1].midPoint);
				distanceMove = model.storms[i].feature[tidIndex].distanceToNextPoint * kvot; // .midPoint.distanceTo(model.storms[i].feature[pos + 1].midPoint);
				// bearing = model.storms[i].feature[pos].midPoint.bearingTo(model.storms[i].feature[pos + 1].midPoint));
				pointStorm = model.storms[i].feature[tidIndex].midPoint.destinationPoint(distanceMove, bearingStormMove);
				if (tidIndex < model.storms[i].nFeatures - 1) {
					outerCircleNext = model.storms[i].feature[tidIndex + 1].outerCircleSize;
				}else
					outerCircleNext = model.storms[i].feature[tidIndex].outerCircleSize;
				outerCircleSize = model.storms[i].feature[tidIndex].outerCircleSize * (1 - kvot) + kvot * outerCircleNext;
			}
			else
				pointStorm = model.storms[i].feature[tidIndex].midPoint;
			dist = pointStorm.distanceTo(point); //  / 1852; // rakna om till nautiska miles...
			// dist = model.storms[i].feature[pos].midPoint.distanceTo(point) / 1852; // rakna om till nautiska miles...
			if (dist >= outerCircleSize)
				continue; // too far from storm, no problem
			bearingFromStorm = pointStorm.bearingTo(point);
			bearingDiff = bearingFromStorm - bearingStormMove;
			if (bearingDiff >= 360)
				bearingDiff -= 360;
			if (bearingDiff < 0)
				bearingDiff += 360;
			if (bearingDiff < 90 || bearingDiff > 270) { // ahead of the storm
				if (tidIndex < model.storms[i].nFeatures - 1) {
					innerCircleNext = model.storms[i].feature[tidIndex + 1].innerCircleForwardSize;
				}
				else
					innerCircleNext = model.storms[i].feature[tidIndex].innerCircleForwardSize;
				innerCircleSize = model.storms[i].feature[tidIndex].innerCircleForwardSize * (1 - kvot) + kvot * innerCircleNext;
			}
			else{ // behind the storm
				if (tidIndex < model.storms[i].nFeatures - 1) {
					innerCircleNext = model.storms[i].feature[tidIndex + 1].innerCircleBackwardsSize;
				}
				else
					innerCircleNext = model.storms[i].feature[tidIndex].innerCircleBackwardsSize;
				innerCircleSize = model.storms[i].feature[tidIndex].innerCircleBackwardsSize * (1 - kvot) + kvot * innerCircleNext;
			}
			if (dist <= innerCircleSize) {
				varde += model.params.penalties.storm_costInsideInner;
			}else
				varde += model.params.penalties.storm_costInsideOuter_kvot * (outerCircleSize - dist) / (outerCircleSize - innerCircleSize);
		}
	}

	return varde;
}

double calcTimeCost_notUsed(int t, int speedSettingNr, int determineWeatherPos, double* fuel, double* safety, double* distance, double* worstStormValue, double* worstStabilityValue)
{
	double tidTot = t, distNu, tidTmp, costTot = 0, safetyTot = 0;
	double uWind, vWind, uCurrent, vCurrent;// , uVessel, vVessel; //  , uSpeed, vSpeed;
	double dist = 0;
	double tailWind, headWind, crossWind, windDirection, windSpeed;
	double tailCurrent, headCurrent, crossCurrent, currentDirection, currentSpeed;
	int i, i1;
	double stormVarde, windSpeed2;

	model.tmpTid4[0] = std::chrono::high_resolution_clock::now();
	*worstStormValue = 0;
	*worstStabilityValue = 0;
	for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
		distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
		dist += distNu;
		//uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
		//vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

		stormVarde = getStormValue(t, model.weatherFunctions.point[i]);
		if (stormVarde > *worstStormValue)
			*worstStormValue = stormVarde;


		uWind = getVariableValue(model.functions.pos_wind_u, i, tidTot);
		vWind = getVariableValue(model.functions.pos_wind_v, i, tidTot);
		if (uWind < 1000 && vWind < 1000) {
			//		errlog("checkP %d latPos %d lonPos %d timePos_uWind %d uWind %.2lf\n", i, latPos,
			//			lonPos, timePos_uWind, uWind);
			//		errlog("checkP %d latPos %d lonPos %d timePos_vWind %d vWind %.2lf\n", i, latPos,
			//			lonPos, timePos_vWind, vWind);
			windDirection = atan2(vWind, uWind);
			windSpeed2 = pow(uWind, 2) + pow(vWind, 2);
			windSpeed = sqrt(windSpeed2);

			*worstStabilityValue += distNu * windSpeed2 / 10000.0;

			crossWind = abs(sin(windDirection - model.weatherFunctions.vesselBearing[i] * M_PI / 180) * windSpeed);
			headWind = cos(windDirection - model.weatherFunctions.vesselBearing[i] * M_PI / 180) * windSpeed;
			if (headWind < 0) {
				tailWind = -headWind;
				headWind = 0;
			}
			else {
				tailWind = 0;
			}
		}
		else {
			tailWind = 0;
			headWind = 0;
			crossWind = 0;
		}
		uCurrent = getVariableValue(model.functions.pos_current_u, i, tidTot);
		vCurrent = getVariableValue(model.functions.pos_current_v, i, tidTot);
		//		errlog("checkP %d latPos %d lonPos %d timePos_uCurrent %d uCurrent %.2lf\n", i, latPos,
		//			lonPos, timePos_uCurrent, uCurrent);
		//		errlog("checkP %d latPos %d lonPos %d timePos_vCurrent %d vCurrent %.2lf\n", i, latPos,
		//			lonPos, timePos_vCurrent, vCurrent);
		if (uCurrent < 1000 && vCurrent < 1000) {
			currentDirection = atan2(vCurrent, uCurrent);
			currentSpeed = sqrt(pow(uCurrent, 2) + pow(vCurrent, 2));
			crossCurrent = abs(sin(currentDirection - model.weatherFunctions.vesselBearing[i] * M_PI / 180) * currentSpeed);
			headCurrent = cos(currentDirection - model.weatherFunctions.vesselBearing[i] * M_PI / 180) * currentSpeed;
			if (headCurrent < 0) {
				tailCurrent = -headCurrent;
				headCurrent = 0;
			}
			else {
				tailCurrent = 0;
			}
		}
		else {
			headCurrent = 0;
			tailCurrent = 0;
			crossCurrent = 0;
		}

		//waveHeight = getVariableValue(4, i, tidTot);
		//if (waveHeight > 100)
		//	waveHeight = 0;
		//wavePeriod = getVariableValue(5, i, tidTot);
		//if (wavePeriod > 1000)
		//	wavePeriod = 0;

		for (i1 = 0; i1 < model.weatherFunctions.nFunctions; i1++) {
			model.weatherFunctions.funcVal[i1] = model.weatherFunctions.param[speedSettingNr][i1][0];
			if (tailWind > 0) {
				model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][1] * tailWind +
					model.weatherFunctions.param[speedSettingNr][i1][2] * tailWind * tailWind;
			}
			if (headWind > 0) {
				model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][3] * headWind +
					model.weatherFunctions.param[speedSettingNr][i1][4] * headWind * headWind;
			}
			if (crossWind > 0) {
				model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][5] * crossWind +
					model.weatherFunctions.param[speedSettingNr][i1][6] * crossWind * crossWind;
			}
			if (tailCurrent > 0) {
				model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][7] * tailCurrent +
					model.weatherFunctions.param[speedSettingNr][i1][8] * tailCurrent * tailCurrent;
			}
			if (headCurrent > 0) {
				model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][9] * headCurrent +
					model.weatherFunctions.param[speedSettingNr][i1][10] * headCurrent * headCurrent;
			}
			if (crossCurrent > 0) {
				model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][11] * crossCurrent +
					model.weatherFunctions.param[speedSettingNr][i1][12] * crossCurrent * crossCurrent;
			}
			////model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[i1][13] * speedNu +
			////	model.weatherFunctions.param[i1][14] * speedNu * speedNu;
			////model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[i1][15] * fuelNu +
			////	model.weatherFunctions.param[i1][16] * fuelNu * fuelNu;

			// not using waveHeight and wavePeriod for now...
			//model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][17] * waveHeight +
			//	model.weatherFunctions.param[speedSettingNr][i1][18] * waveHeight * waveHeight;
			//model.weatherFunctions.funcVal[i1] += model.weatherFunctions.param[speedSettingNr][i1][19] * wavePeriod +
			//	model.weatherFunctions.param[speedSettingNr][i1][20] * wavePeriod * wavePeriod;
		}
		if (model.weatherFunctions.funcVal[0] > 0.01)
			tidTmp = distNu / model.weatherFunctions.funcVal[0] / model.params.knots_to_km;
		else
			tidTmp = 999;

		//		errlog("tidTmp %.2lf speed fuel safety %.2lf %.2lf %.2lf\n",
		//			tidTmp, model.weatherFunctions.funcVal[0], model.weatherFunctions.funcVal[1],
		//			model.weatherFunctions.funcVal[2]);

		tidTot += tidTmp;
		costTot += model.weatherFunctions.funcVal[1] * tidTmp / 24;
		safetyTot += model.weatherFunctions.funcVal[2] * tidTmp / 24;
	}
	*distance = dist;
	*fuel = costTot;
	*safety = safetyTot;
	//	errlog("speedSetting %d tidStart %d tidSlut %.2lf cost %.2lf safety %.2lf\n",
	//		speedSettingNr, t, tidTot, costTot, safetyTot);


	model.tmpTid4[1] = std::chrono::high_resolution_clock::now();
	model.duration3 += model.tmpTid4[1] - model.tmpTid4[0];
	return tidTot - t;
}

double eval_calmWaterSpeed(int speedNr) {
	double varde;

	varde = model.functions.calmWaterSpeed.c0
		+ model.functions.calmWaterSpeed.c1_rpm * model.functions.rpm[speedNr]
		+ model.functions.calmWaterSpeed.c2_rpm * model.functions.rpm[speedNr] * model.functions.rpm[speedNr];
	return varde;
}

double eval_fuelConsumption(int speedNr) {
	double varde;

	varde = model.functions.fuelConsumption.c0
		+ model.functions.fuelConsumption.c1_rpm * model.functions.rpm[speedNr]
		+ model.functions.fuelConsumption.c2_rpm * model.functions.rpm[speedNr] * model.functions.rpm[speedNr]
		+ model.functions.fuelConsumption.c3_rpm * model.functions.rpm[speedNr] * model.functions.rpm[speedNr] * model.functions.rpm[speedNr];

	return varde;
	//return model.functions.rpmSetting_gerFuelConsumption[speedNr];
}

double eval_bowSlamming(double waveHeight, double wavePeriod) {
	double p, d, v_cr, waveHeight2, sigma2_0, Tp, wp, kvot;
	double gamma, sigma2_2;
	double PI_2 = 2 * 3.141592654;

	d = model.params.shipDraft;
	v_cr = 0.093 * sqrt(9.8 * model.params.shipLength);
	waveHeight2 = waveHeight * waveHeight;
	sigma2_0 = 1 / 16 * waveHeight2;
	Tp = 1.296 * wavePeriod;
	wp = PI_2 / Tp;
	kvot = Tp / sqrt(waveHeight);
	if(kvot <= 3.6)
		gamma = 5;
	else {
		if (kvot < 5)
			gamma = exp(5.75 - 1.15 * kvot);
		else
			gamma = 1;
	}

	sigma2_2 = sigma2_0 * wp * (11 + gamma) / (5 + gamma);

	p = exp(-v_cr * v_cr * d * d / (2 * sigma2_2 * 2 * sigma2_0));

	return p;
}

double eval_greenWater(double waveHeight) {
	double sigma2_0, p;

	sigma2_0 = 1.0 / 16 * waveHeight * waveHeight;
	p = exp(-model.params.freeBoard2 / (2 * sigma2_0));
	return p;
}

double eval_dynamicStability() {
	double p = 0;
	//R_AA = 0.5 * r_A * A_XV * (windSpeed2 * C_X[windDir] -
	//	speed2_overGround * C_X[0]);
	//P = R_AA / A_XV;
	return p;
}

int getHeightIndex(double height, strFunkData funcData) {
	int index;

	printf("height %lf indexSize %lf nIndex %d\n", height, funcData.waveHeightIndexSize, funcData.nWaveHeightIndex);
	if (height < 0)
		index = 0;
	else {
		index = (int)(height / funcData.waveHeightIndexSize);
		if (index >= funcData.nWaveHeightIndex) {
			index = -1; // funcData.nHeightIndex - 1;
		}
	}
	return index;
}

int getPeriodIndex(double period, strFunkData funcData) {
	int index;

	if (period < 0)
		index = 0;
	else {
		index = (int)(period / funcData.wavePeriodIndexSize);
		if (index >= funcData.nWavePeriodIndex) {
			index = -1; // funcData.nPeriodIndex - 1;
		}
	}
	return index;
}

int getWindSpeedIndex(double wSpeed, strFunkData funcData) {
	int index;

	if (wSpeed < 0)
		index = 0;
	else {
		index = (int)(wSpeed / funcData.windSpeedIndexSize);
		if (index >= funcData.nWindSpeedIndex) {
			index = -1;// funcData.nPeriodIndex - 1;
		}
	}
	return index;
}

int getWindDirectionIndex(double windDir, strFunkData funcData) {
	int index;

	if (windDir < 0)
		index = 0;
	else {
		index = (int)(windDir / funcData.windDirIndexSize);
		if (index >= funcData.nWindDirIndex) {
			index = -1;// funcData.nPeriodIndex - 1;
		}
	}
	return index;
}

double getFromTable_bowSlamming(double waveHeight, double wavePeriod) {
	int heightIndex, periodIndex;

	heightIndex = getHeightIndex(waveHeight, model.functions.bowSlamming);
	periodIndex = getPeriodIndex(wavePeriod, model.functions.bowSlamming);
	if (heightIndex < 0 || periodIndex < 0)
		return 9999.9;
	else
		return model.functions.bowSlamming.tableValue[heightIndex +
			model.functions.bowSlamming.nWaveHeightIndex * periodIndex];
}

double getFromTable_greenWater(double waveHeight) {
	int heightIndex;

	heightIndex = getHeightIndex(waveHeight, model.functions.greenWater);
	if (heightIndex < 0)
		return 9999.9;
	else
		return model.functions.greenWater.tableValue[heightIndex];
}

double getFromTable_dynamicStability(double relWindSpeed, double relWindDirection) {
	int wSpeedIndex, wDirIndex;

	wSpeedIndex = getWindSpeedIndex(relWindSpeed, model.functions.dynStability);
	wDirIndex = getWindDirectionIndex(relWindDirection, model.functions.dynStability);
	if (wSpeedIndex < 0 || wDirIndex < 0)
		return 9999.9;
	else
		return model.functions.dynStability.tableValue[wSpeedIndex +
			model.functions.dynStability.nWindSpeedIndex * wDirIndex];
}

void eval_safety(double windspeed, double windDirection, double waveHeight, 
	double wavePeriod, double iceCover) {
	int feasibleSafety = 1;
	double bowSlam, greenWater, dynStab, iceCost;

	bowSlam = eval_bowSlamming(waveHeight, wavePeriod);
	//bowSlam = getFromTable_bowSlamming(waveHeight, wavePeriod);
	if (model.functions.valuesNow.bowSlam < bowSlam)
		model.functions.valuesNow.bowSlam = bowSlam;
	if (bowSlam > 0.01)
		model.functions.valuesNow.feasibleSafety = 0;
	greenWater = eval_greenWater(waveHeight);
	//greenWater = getFromTable_greenWater(waveHeight);
	if (model.functions.valuesNow.greenWater < greenWater)
		model.functions.valuesNow.greenWater = greenWater;
	if (greenWater > 0.07)
		model.functions.valuesNow.feasibleSafety = 0;
	//dynStab = eval_dynamicStability();
	dynStab = getFromTable_dynamicStability(windspeed, windDirection);
	if (model.functions.valuesNow.dynamicStability < dynStab)
		model.functions.valuesNow.dynamicStability = dynStab;
	if (dynStab >= 1)
		model.functions.valuesNow.feasibleSafety = 0;

	if (iceCover > model.functions.iceCoverMaxFree) {
		iceCost = model.params.weightSafety.iceCoverCost_fix + model.params.weightSafety.iceCoverCost_thickness * (
				iceCover - model.functions.iceCoverMaxFree);
		if (model.functions.valuesNow.iceCoverCost < iceCost)
			model.functions.valuesNow.iceCoverCost = iceCost;
		model.functions.valuesNow.feasibleSafety = 0;
	}

}

double eval_baseGroundSpeed(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed) {
	double rel_currentDir, speed, B, C, sinA, tmp;

	// basic variant, not correct
	// rel_currentDir = currentDir * 180.0 / M_PI - bearing;
	// speed_y = calmWaterSpeed + currentSpeed * cos(rel_currentDir);
	// speed_x = currentSpeed * sin(rel_currentDir);
	// speed = sqrt(speed_y * speed_y - speed_x * speed_x);

	rel_currentDir = abs(currentDir - bearing);
	if (rel_currentDir > M_PI)
		rel_currentDir = 2 * M_PI - rel_currentDir;
	sinA = sin(rel_currentDir);
	if (sinA < 0.01)
		speed = calmWaterSpeed + currentSpeed * cos(rel_currentDir);
	else {
		tmp = currentSpeed * sinA / calmWaterSpeed;
		if (tmp > 1)
			tmp = 1;
		if (tmp < -1)
			tmp = -1;
		B = asin(tmp);
		C = M_PI - B - rel_currentDir;

		speed = calmWaterSpeed * sin(C) / sinA;
	}
	if (speed < 0.0001)
		speed = 0.0001;
	//printf("calmWaterSpeed %.2lf bearing %.2lf currentDir %.2lf currentSpeed %.2lf rel_currentDir %.2lf speed %.2lf sinA %.2lf B %.2lf currentSpeed * sinA / calmWaterSpeed %.2lf\n",
	//	calmWaterSpeed, bearing, currentDir, currentSpeed, rel_currentDir, speed, sinA, B, currentSpeed * sinA / calmWaterSpeed);
	return speed;
}

double eval_relWindSpeed(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir) {
	double rel_speed, x, y, direction;

	//printf("dir ship wind %.2lf %.2lf\n", bearing * 180 / M_PI, windDir * 180 / M_PI);

	//windDir = (windDir - (M_PI + bearing)); // in radians relative to ship bearing
	windDir -= bearing; // in radians relative to ship bearing
	x = windSpeed * cos(windDir) - baseGroundSpeed;
	y = windSpeed * sin(windDir);
	direction = atan2(y, x);
	if (direction < 0)
		direction = -direction;
	if (direction > M_PI)
		direction = 2 * M_PI - direction;
	*rel_windDir = direction;
	rel_speed = sqrt(x * x + y * y);
	//printf("windDir rel ship %.2lf xy rel ship %.2lf %.2lf get rel_wind speed dir %.2lf %.2lf\n",
	//	windDir * 180 / M_PI, x, y, rel_speed, *rel_windDir * 180 / M_PI);
	//printf("relWindSpeed xy %.2lf %.2lf windDir %.2lf bearing %.2lf relWindDir %.2lf windSpeed %.2lf baseGroundSpeed %.2lf\n", 
	//	x, y, windDir, bearing, *rel_windDir, windSpeed, baseGroundSpeed);
	return rel_speed;
}

int get_relWindDirIndex(double rel_windDir, strFunkData funkData, int alt) { // windDir: [-pi, +pi]

	rel_windDir = M_PI - rel_windDir; // need it in opposite direction
	if (rel_windDir < 0)
		rel_windDir = -rel_windDir; // windDir [0, +pi]
	if (rel_windDir > M_PI)
		rel_windDir = 2 * M_PI - rel_windDir;
	int index = (int)((rel_windDir - funkData.windDir_min) / funkData.windDirIndexSize);
	// .*model.functions.table_niWindDir / M_PI;// 180.0;
	// printf("rel_windDir %.2lf index %d min %.2lf size %.3lf\n", rel_windDir, )
	if (index < (rel_windDir - funkData.windDir_min) / funkData.windDirIndexSize - 0.5)
		index++;
	if (index < 0 && alt == 0)
		index = 0;
	else {
		if (index >= funkData.nWindDirIndex && alt == 0)
			index = funkData.nWindDirIndex - 1;
	}
	return index;
}

int get_relWaveDirIndex(double rel_waveDir, strFunkData funkData, int alt) { // windDir: [-pi, +pi]

	rel_waveDir = M_PI - rel_waveDir; // need it in opposite direction
	if (rel_waveDir < -M_PI)
		rel_waveDir += 2 * M_PI;
	if (rel_waveDir > 2 * M_PI)
		rel_waveDir -= 2 * M_PI;
	if (rel_waveDir < 0)
		rel_waveDir = -rel_waveDir; // windDir [0, +pi]
	int index = (int)((rel_waveDir - funkData.waveDir_min) / funkData.waveDirIndexSize);// table_niWaveDir / M_PI;// 180.0;
	if (index < (rel_waveDir - funkData.waveDir_min) / funkData.waveDirIndexSize - 0.5)
		index++;

	if (index < 0 && alt == 0)
		index = 0;
	else {
		if (index >= funkData.nWaveDirIndex && alt == 0)
			index = funkData.nWaveDirIndex - 1;
	}
	return index;
}

int get_relWindSpeedIndex(double rel_windSpeed, strFunkData funkData, int alt) {

	int tmp = (int)((rel_windSpeed - funkData.windSpeed_min) / funkData.windSpeedIndexSize); // *model.functions.rel_windSpeed_kvotIndex;
	//printf("rel_windSpeed %.2lf tmp %d min %.3lf indexSize %.3lf", rel_windSpeed,
	//	tmp, funkData.windSpeed_min, funkData.windSpeedIndexSize);
	if (tmp < (rel_windSpeed - funkData.windSpeed_min) / funkData.windSpeedIndexSize - 0.5)
		tmp++;
	//printf(" tmp2 %d\n", tmp);
	if (tmp >= funkData.nWindSpeedIndex && alt == 0)
		return funkData.nWindSpeedIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

int get_calmWaterSpeedIndex(double speed, strFunkData funkData, int alt) {

	int tmp = (int)((speed - funkData.calmWaterSpeed_min) / funkData.calmWaterSpeedIndexSize);
	//printf("speed %.3lf tmp %d min %.3lf indexSize %.3lf\n", speed, tmp, funkData.calmWaterSpeed_min,
	//	funkData.calmWaterSpeedIndexSize);
	if (tmp < (speed - funkData.calmWaterSpeed_min) / funkData.calmWaterSpeedIndexSize - 0.5)
		tmp++;
	//printf("tmp sedan %d\n", tmp);
	if (tmp >= funkData.nCalmWaterSpeedIndex && alt == 0)
		return funkData.nCalmWaterSpeedIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

int get_shipSpeedIndex(double speed, strFunkData funkData, int alt) {

	int tmp = (int)((speed - funkData.shipSpeed_min) / funkData.shipSpeedIndexSize);
	if (tmp < (speed - funkData.shipSpeed_min) / funkData.shipSpeedIndexSize - 0.5)
		tmp++;
	//printf(" tmp2 %d\n", tmp);
	if (tmp >= funkData.nShipSpeedIndex && alt == 0)
		return funkData.nShipSpeedIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

int get_relWaveHeightIndex(double waveHeight, strFunkData funkData, int alt) {
	int tmp = (int)((waveHeight - funkData.waveHeight_min) / funkData.waveHeightIndexSize); 

	if (tmp < (waveHeight - funkData.waveHeight_min) / funkData.waveHeightIndexSize - 0.5)
		tmp++;
	if (tmp >= funkData.nWaveHeightIndex && alt == 0)
		return funkData.nWaveHeightIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

int get_relWavePeriodIndex(double wavePeriod, strFunkData funkData, int alt) {
	int tmp = (int)((wavePeriod - funkData.wavePeriod_min) / funkData.wavePeriodIndexSize);
	if (tmp < (wavePeriod - funkData.wavePeriod_min) / funkData.wavePeriodIndexSize - 0.5)
		tmp++;
	if (tmp >= funkData.nWavePeriodIndex && alt == 0)
		return funkData.nWavePeriodIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

double lookup_speedDiffWindWaveTable(double rel_windSpeed, double rel_windDir, double waveHeight, double wavePeriod, double rel_waveDir) {
	int iWindDir = get_relWindDirIndex(rel_windDir, model.functions.weatherFactors);
	int iWaveDir = get_relWaveDirIndex(rel_waveDir, model.functions.weatherFactors);
	int iWindSpeed = get_relWindSpeedIndex(rel_windSpeed, model.functions.weatherFactors);
	int iWaveHeight = get_relWaveHeightIndex(waveHeight, model.functions.weatherFactors); // / model.functions.waveHeightDiscr);
	int iWavePeriod = get_relWavePeriodIndex(wavePeriod, model.functions.weatherFactors); // / model.functions.wavePeriodDiscr);
	int pos;

	pos = iWaveDir + model.functions.weatherFactors.nWaveDirIndex * (iWavePeriod + model.functions.weatherFactors.nWavePeriodIndex * (
		iWaveHeight + model.functions.weatherFactors.nWaveHeightIndex * (
			iWindDir + model.functions.weatherFactors.nWindDirIndex * iWindSpeed)));
	return model.functions.weatherFactors.tableValue[pos]; // windSpeed, windDir, wave, wavePeriod, waveDir

}

double lookup_speedDiffWindTable(double calmWaterSpeed, double rel_windSpeed, double rel_windDir) {
	int iWindDir = get_relWindDirIndex(rel_windDir, model.functions.weatherFactors);
	int iWindSpeed = get_relWindSpeedIndex(rel_windSpeed, model.functions.weatherFactors);
	int iCalmWaterSpeed = get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors);
	int pos;

	pos = iCalmWaterSpeed + model.functions.weatherFactors.nCalmWaterSpeedIndex *
		(iWindDir + model.functions.weatherFactors.nWindDirIndex * iWindSpeed);
	return model.functions.weatherFactors.tableValueWind[pos]; // windSpeed, windDir, calmWaterSpeed

}

double lookup_speedDiffWaveTable(double calmWaterSpeed, double waveHeight, double wavePeriod, double rel_waveDir) {
	int iWaveDir = get_relWaveDirIndex(rel_waveDir, model.functions.weatherFactors);
	int iWaveHeight = get_relWaveHeightIndex(waveHeight, model.functions.weatherFactors); // / model.functions.waveHeightDiscr);
	int iWavePeriod = get_relWavePeriodIndex(wavePeriod, model.functions.weatherFactors); // / model.functions.wavePeriodDiscr);
	int iCalmWaterSpeed = get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors);
	int pos;

	pos = iCalmWaterSpeed + model.functions.weatherFactors.nCalmWaterSpeedIndex *
		(iWaveDir + model.functions.weatherFactors.nWaveDirIndex * (iWavePeriod + 
			model.functions.weatherFactors.nWavePeriodIndex * iWaveHeight));
	return model.functions.weatherFactors.tableValueWave[pos]; // wave, wavePeriod, waveDir, calmWaterSpeed

}

double calcArcTimeCost(int t, int speedSettingNr, int determineWeatherPos){
	//, double* fuel, double* safety, double* distance, double* worstStormValue, double* worstStabilityValue)

	double tidTot = t, distNu, fuelTot = 0, safetyTot = 0;
	double uWind, vWind, uCurrent, vCurrent;// , uVessel, vVessel; //  , uSpeed, vSpeed;
	double dist = 0;
	double windDirection, windSpeed;
	double currentDirection, currentSpeed, rel_windSpeed;
	int i;
	double waveHeight, wavePeriod, stormVarde, windSpeed2, waveDirection;
	double calmWaterSpeed, baseGroundSpeed, rel_windDir, rel_waveDir, speedDiffWindWave;
	double speedOverGround, timeArc, fuelConsumption, fuelUsage, iceCover, safetyArc;
	double speedDiffWind, speedDiffWave;

	// a speed setting (rpm) gives a calm water speed (function)
	// calm water speed and its heading plus current and its direction gives baseGroundSpeed and its heading
	// baseGroundSpeed and its heading plus windspeed and its direction gives relative wind speed
	// // (ground speed direction including current or is the current dealt with when choosing the heading) or too small to worry about?
	// relative wind speed and its relative direction plus wave height, wave frequence and wave direction relative to ship heading => table value of real speed loss
	// // right now we only have a table for one ship speed (13.4 knots), I assume it is calm water speed. We need tables for other ship speeds as well or can we scale the table depending on the give calm water speed?
	// // should we interpolate between different Beaufort, different combination of (wave height and frequence), relative wind angle, relative wave angle (then it is a lot of places in the table to check…...)??
	// // should we choose the table entry with the highest of frequence and wave height, or??????????
	// baseGroundSpeed minus/plus real speed loss gives real ship speed over ground
	// real ship speed over ground and length of arc gives time on arc
	// rpm gives fuel consumption per time unit (function), possibly with some weather dependence as well, IVADO decides…
	// time on arc plus fuel consumption per time unit gives total fuel consumption on an arc

	// data problematic with discrete jumps
	// // change in Beaufort value - interpolate
	// // wave direction change from 45 to 56 degrees, same effect if 56 degrees (sideways from the front) or 180 degrees (back). Is that correct?

	// // wave height change – interpolate
	// // how does different load affect speed and fuel consumption?
	// // we need one weather factor table for each ‘main’ type of ship
	// // different calm water ship speeds => more tables?

	// safety is only measured in ice coverage, over a threshold value it is expensive


	model.tmpTid4[0] = std::chrono::high_resolution_clock::now();
	model.functions.valuesNow.worstStormValue = 0;
	//model.functions.valuesNow.worstStabilityValue = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0; // a / b
	model.functions.valuesNow.feasibleSafety = 1;
	calmWaterSpeed = eval_calmWaterSpeed(speedSettingNr);
	if (printGlobal == 1) {
		printf("speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", speedSettingNr, calmWaterSpeed,
			model.weatherFunctions.nCheckPoints);
	}
	for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
		distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
		dist += distNu;

		//printf("test i %d\n", i);

		//uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
		//vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

		//printf("test tt\n");
		stormVarde = getStormValue(t, model.weatherFunctions.point[i]);
		if (stormVarde > model.functions.valuesNow.worstStormValue)
			model.functions.valuesNow.worstStormValue = stormVarde;

		uCurrent = getVariableValue(model.functions.pos_current_u, i, tidTot);
		vCurrent = getVariableValue(model.functions.pos_current_v, i, tidTot);
		if (uCurrent < 1000 && vCurrent < 1000) {
			currentDirection = atan2(vCurrent, uCurrent);
			currentSpeed = sqrt(pow(uCurrent, 2) + pow(vCurrent, 2));
		}
		else {
			currentDirection = 0;
			currentSpeed = 0;
		}

		if (uCurrent > 100000 || 
			(model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u]==9000&&
			model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u]==10&&
				(int)tidTot / model.weather[model.functions.pos_current_u].timeIntervall_h==3))
			printf("currentSpeed %.2lf uCurr %.2lf vCurr %.2lf uCurrPos %d lonPos %d latPos %d "
				"tidInt %d lon/lat %.2lf %.2lf speedSetting %d i %d av %d\n", 
				currentSpeed, uCurrent, vCurrent, 
				model.functions.pos_current_u,
				model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u],
				model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u],
				(int)tidTot / model.weather[model.functions.pos_current_u].timeIntervall_h,
				-model.weather[model.functions.pos_current_u].minX +
				model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u] *
				model.weather[model.functions.pos_current_u].size_col,
				model.weather[model.functions.pos_current_u].maxY - 
				model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u] * 
				model.weather[model.functions.pos_current_u].size_row,
				speedSettingNr, i, model.weatherFunctions.nCheckPoints);

		baseGroundSpeed = eval_baseGroundSpeed(calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
			currentDirection, currentSpeed);
		if (printGlobal == 1) {
			printf("checkP %d vCurrent %.3lf uCurrent %.3lf, currentDirection %.3lf currentSpeed %.3lf baseGroundSpeed %.3lf\n", i, vCurrent,
				uCurrent, currentDirection, currentSpeed, baseGroundSpeed);
		}

		uWind = getVariableValue(model.functions.pos_wind_u, i, tidTot);
		vWind = getVariableValue(model.functions.pos_wind_v, i, tidTot);
		if (uWind < 1000 && vWind < 1000) {
			windDirection = atan2(vWind, uWind);
			windSpeed2 = pow(uWind, 2) + pow(vWind, 2);
			windSpeed = sqrt(windSpeed2);

			//printf("i %d bearing %.3lf\n", i, model.weatherFunctions.vesselBearing[i]);
			rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
				windDirection, windSpeed, &rel_windDir);
			if (printGlobal == 1) {
				printf("checkP %d vWind %.3lf uWind %.3lf, windDirection %.3lf windSpeed %.3lf rel_windSpeed %.3lf rel_windDir %.3lf\n", i, vWind,
					uWind, windDirection, windSpeed, rel_windSpeed, rel_windDir);
			}
			//printf("shipSpeed %.2lf bearing %.2lf wind xy %.2lf %.2lf dir %.2lf rel_windSpeed %.2lf rel_windDir %.2lf\n",
			//	baseGroundSpeed, model.weatherFunctions.vesselBearing[i], uWind, vWind, windDirection * 180 / M_PI,
			//	rel_windSpeed, rel_windDir * 180 / M_PI);

			//model.functions.valuesNow.worstStabilityValue += distNu * rel_windSpeed / 10000.0;
		}
		else {
			windSpeed = 0;
			rel_windDir = 0;
		}

		waveHeight = getVariableValue(model.functions.pos_waveHeight, i, tidTot);
		if (waveHeight > 100)
			waveHeight = 0;
		wavePeriod = getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
		if (wavePeriod > 1000)
			wavePeriod = 0;
		waveDirection = getVariableValue(model.functions.pos_waveDirection, i, tidTot);
		if (waveDirection > 1000)
			waveDirection = 0;
		rel_waveDir = waveDirection * M_PI / 180 - model.weatherFunctions.vesselBearing[i]; // / model.functions.nWaveDir;
		if (rel_waveDir < 0)
			rel_waveDir = -rel_waveDir;
		if (rel_waveDir >= 2 * M_PI)
			rel_waveDir -= 2 * M_PI;
		if (rel_waveDir > M_PI)
			rel_waveDir = 2 * M_PI - rel_waveDir;
		if (printGlobal == 1) {
			printf("checkP %d waves hight %.2lf period %.2lf Direction %.3lf rel_waveDir %.3lf\n", i, 
				waveHeight, wavePeriod, waveDirection, rel_waveDir);
		}

		speedDiffWind = lookup_speedDiffWindTable(calmWaterSpeed, rel_windSpeed, rel_windDir);
		speedDiffWave = lookup_speedDiffWaveTable(calmWaterSpeed, waveHeight, wavePeriod, rel_waveDir);
		speedDiffWindWave = speedDiffWind + speedDiffWave;
		//speedDiffWindWave = lookup_speedDiffWindWaveTable(rel_windSpeed, rel_windDir, waveHeight, wavePeriod, rel_waveDir) * (
		//	calmWaterSpeed / model.functions.weatherFactorsTable_shipSpeed);
		speedOverGround = baseGroundSpeed * model.params.knots_to_km - speedDiffWindWave; // in km/h
		timeArc = distNu / speedOverGround; // in hours

		//printf("dist %.2lf calmWaterSpeed %.2lf worstStormValue %.2lf vesselBearing %.2lf currDir %.2lf currSpeed %.2lf uCurr %.2lf vCurr %.2lf baseGroundSpeed %.2lf rel_windSpeed %.2lf waveHeight %.2lf wavePeriod %.2lf rel_waveDir %.2lf speedDiffWindWave %.2lf speedOverGround %.2lf timeArc %.2lf\n",
		//	dist, calmWaterSpeed, *worstStormValue, model.weatherFunctions.vesselBearing[i],
		//	currentDirection, currentSpeed, uCurrent, vCurrent, baseGroundSpeed, rel_windSpeed, waveHeight, wavePeriod,
		//	rel_waveDir, speedDiffWindWave, speedOverGround, timeArc);

		fuelConsumption = eval_fuelConsumption(speedSettingNr);
		fuelUsage = fuelConsumption * timeArc;
		if (printGlobal == 1) {
			printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf fuelCons %.3lf\n",
				speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu, fuelConsumption);
		}

		tidTot += timeArc;
		fuelTot += fuelUsage;

		iceCover = getVariableValue(model.functions.pos_iceThickness, i, tidTot);
		if (iceCover > 1000)
			iceCover = 0;

		eval_safety(rel_windSpeed, rel_windDir, waveHeight, wavePeriod, iceCover);
		//safetyTot += safetyArc;
		if (speedDiffWind > 98 || speedDiffWave > 98)
			tidTot = 1e10;
	}
	model.functions.valuesNow.distance = dist;
	model.functions.valuesNow.fuel = fuelTot;
	//model.functions.valuesNow.safety = safetyTot;

	//	errlog("speedSetting %d tidStart %d tidSlut %.2lf cost %.2lf safety %.2lf\n",
	//		speedSettingNr, t, tidTot, costTot, safetyTot);


	model.tmpTid4[1] = std::chrono::high_resolution_clock::now();
	model.duration3 += model.tmpTid4[1] - model.tmpTid4[0];
	return tidTot - t;
}

int checkAddBagar_AB(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int* setupCheckPoints, int max_t, int* min_t_nu, int* max_t_nu, double fuelQualityKvot)
{
	// i = thisLevel, i1 = pointPos, i+1 = nextLevel, i2 = outNodePos, i3 = tPos
	int i4, tidInt, nArcsNu = 0, nodNr1, nodNr2, posNy, arcNr, prefPath = 0;
	double tid; // , safety, fuel, distance
	double totCost, channelCost, safety;
	double fuelVLSFO, fuelLSMGO, fuelBase, safetyBase; // , worstStormValue = 0;
	// double worstStabilityValue = 0;

	model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
	if (thisLevel >= 0 && nextLevel >= 0) {
		if (pos1 == model.params.preferredPathOrtoPos[thisLevel] && pos2 == model.params.preferredPathOrtoPos[nextLevel] && thisLevel == nextLevel - 1)
			prefPath = 1;
	}

	for (i4 = 0; i4 < model.params.nShip_speedSettings; i4++) {
		//printf("test i4 %d\n", i4);
		if (*setupCheckPoints == 1) {
			if (printGlobal == 1)
				printf("prefPath %d\n", prefPath);
			if (thisLevel >= 0) {
				if (nextLevel >= 0) {
					if (prefPath == 1)
						calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[thisLevel].point[pos1], thisLevel);
					else
						calcWeatherPosAlongArc(model.network.physicalLev[thisLevel].point[pos1],
							model.network.physicalLev[nextLevel].point[pos2]);
				}
				else {
					calcWeatherPosAlongArc(model.network.physicalLev[thisLevel].point[pos1],
						model.network.channel[-nextLevel - 1].point[0]);
				}
			}
			else {
				if (nextLevel >= 0) {
					calcWeatherPosAlongArc(model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1],
						model.network.physicalLev[nextLevel].point[pos2]);
				}
				else {
					calcWeatherPosAlongChannel(-thisLevel - 1);
					//model.network.channel[-thisLevel - 1].point[pointPos],
					//	model.network.channel[-nextLevel-1].point[outNodePos]);
				}
			}
			//printf("test cc\n");

			//errlog("from %.2lf %.2lf to %.2lf %.2lf\n",
			//	model.network.physicalLev[i - 1].point[i1].latitude().degrees(),
			//	model.network.physicalLev[i - 1].point[i1].longitude().degrees(),
			//	model.network.physicalLev[i].point[i2].latitude().degrees(),
			//	model.network.physicalLev[i].point[i2].longitude().degrees());
			*setupCheckPoints = 0;
		}
		if (thisLevel == 12 && model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] == 107)
			tPos = tPos;
		//printf("test thisLevel %d\n", thisLevel);
		if (thisLevel >= 0) {
			nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
			//printf("test aa\n");
			tid = calcArcTimeCost(model.network.physicalLev[thisLevel].timeInterval[pos1][tPos],
				i4, *setupCheckPoints); // , & fuel, & safety, & distance, & worstStormValue, & worstStabilityValue);
			//printf("test bb\n");
			tidInt = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] + (int)tid;
		}
		else {
			nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
			tid = calcArcTimeCost(model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos],
				i4, *setupCheckPoints); // , & fuel, & safety, & distance, & worstStormValue, & worstStabilityValue);
			tidInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos] + (int)tid;
		}
		//printf("test dd\n");



		if (tidInt <= max_t || prefPath == 1) {
			if (thisLevel < 0 && nextLevel < 0) {
				tid += model.network.channel[-thisLevel - 1].extraTimeChannel;
				channelCost = model.network.channel[-thisLevel - 1].extraCostChannel;
			}
			else
				channelCost = 0;
			totCost = channelCost;
			model.tmpTid5[0] = std::chrono::high_resolution_clock::now();
			nodNr2 = addTimeTo_timeInterval(thisLevel, nextLevel, pos2, tidInt);
			model.tmpTid5[1] = std::chrono::high_resolution_clock::now();
			model.duration4 += model.tmpTid5[1] - model.tmpTid5[0];
			//pos = model.network.physicalLev[i - 1].nOutArcs[i1];
			fuelVLSFO = model.functions.valuesNow.fuel * (1 - fuelQualityKvot);
			fuelLSMGO = model.functions.valuesNow.fuel * fuelQualityKvot;
			fuelBase = fuelVLSFO * model.params.weightFuel.vlsfo * model.params.priceFuel.vlsfo + fuelLSMGO * model.params.weightFuel.lsmgo * model.params.priceFuel.lsmgo;

			safety = model.functions.valuesNow.worstStormValue *
				model.params.weightSafety.hurricane +
				model.functions.valuesNow.bowSlam *
				model.params.weightSafety.bowSlam +
				model.functions.valuesNow.greenWater *
				model.params.weightSafety.greenWater +
				model.functions.valuesNow.dynamicStability *
				model.params.weightSafety.dynamicStability +
				model.functions.valuesNow.feasibleSafety *
				model.params.weightSafety.feasibleSafety +
				model.functions.valuesNow.iceCoverCost;
			//model.functions.valuesNow.worstStabilityValue * model.params.weightSafety.stability;
			safetyBase = safety;

			totCost += model.params.weightTime * model.params.priceTime * tid +
				fuelBase + model.params.weightSafety.base * safety;
			if (totCost < 0) {
				printf("\nchannelCost %.2lf wTime %.2lf pTime %.2lf tid %.2lf wFuel %.2lf fBase %.2lf wSafety %.2lf safety %.2lf totCost %.2lf\n",
					channelCost, model.params.weightTime, model.params.priceTime, tid,
					1.0, fuelBase, model.params.weightSafety.base, safety, totCost);
				printf("thisLevel %d pos1 %d nextLevel %d pos2 %d tPos %d i4 %d\n",
					thisLevel, pos1, nextLevel, pos2, tPos, i4);
				printf("thisLevel %d\n", thisLevel);
				printf("tidInt %d\n",
					model.network.physicalLev[thisLevel].timeInterval[pos1][tPos]);
				printf("cPoint %d\n",
					*setupCheckPoints);
				printf("lon/lat %.2lf %.2lf to lon/lat %.2lf %.2lf\n",
					model.network.physicalLev[thisLevel].point[pos1].longitude().degrees(),
					model.network.physicalLev[thisLevel].point[pos1].latitude().degrees(),
					model.network.physicalLev[nextLevel].point[pos2].longitude().degrees(),
					model.network.physicalLev[nextLevel].point[pos2].latitude().degrees());
			}

			posNy = adderaArc(nodNr1, nodNr2, totCost);
			if (nodNr1 == 44660 && nodNr2 == 52128)
				nodNr1 = nodNr1;
			//if (model.nArcs == 93099)
			//	freeMemory();
			if (posNy == -2)
				continue; // do not add this arc as there is another one thats cheaper between the time nodes
			if (posNy >= 0) {
				model.arc[posNy].speedSetting = i4;
				model.arc[posNy].time = tid;
				model.arc[posNy].fuelBase = fuelBase;
				model.arc[posNy].fuelVLSFO = fuelVLSFO;
				model.arc[posNy].fuelLSMGO = fuelLSMGO;
				model.arc[posNy].distance = model.functions.valuesNow.distance;
				model.arc[posNy].safetyHurricane = model.functions.valuesNow.worstStormValue;
				model.arc[posNy].safetyBowSlam = model.functions.valuesNow.bowSlam;
				model.arc[posNy].safetyGreenWater = model.functions.valuesNow.greenWater;
				model.arc[posNy].safetyDynStability = model.functions.valuesNow.dynamicStability;
				model.arc[posNy].feasibleSafety = model.functions.valuesNow.feasibleSafety;
				model.arc[posNy].iceCoverCost = model.functions.valuesNow.iceCoverCost;
				//model.arc[posNy].safetyStability = worstStabilityValue;
				model.arc[posNy].safetyBase = safety;
				model.arc[posNy].channelCost = channelCost;
				model.arc[posNy].totCost = totCost;
			}
			else {
				arcNr = model.nArcs;
				if (arcNr >= model.nAllocArcs) {
					model.nAllocArcs += 100000;
					model.arc = (strArcInfo*)realloc(model.arc,
						model.nAllocArcs * sizeof(strArcInfo));
				}
				model.arc[arcNr].fromLevel = thisLevel;
				model.arc[arcNr].toLevel = nextLevel;
				model.arc[arcNr].fromPointNr = pos1;
				model.arc[arcNr].toPointNr = pos2;
				if (nextLevel >= 0)
					(model.network.physicalLev[nextLevel].nArcsToPoint[pos2])++;
				else
					(model.network.channel[-nextLevel - 1].nArcsToPoint[pos2])++;
				if (thisLevel >= 0)
					model.arc[arcNr].fromTime = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
				else
					model.arc[arcNr].fromTime = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
				model.arc[arcNr].toTime = tidInt;
				model.arc[arcNr].speedSetting = i4;
				model.arc[arcNr].time = tid;
				model.arc[arcNr].distance = model.functions.valuesNow.distance;
				model.arc[arcNr].fuelBase = fuelBase;
				model.arc[arcNr].fuelVLSFO = fuelVLSFO;
				model.arc[arcNr].fuelLSMGO = fuelLSMGO;
				model.arc[arcNr].safetyHurricane = model.functions.valuesNow.worstStormValue;
				model.arc[arcNr].safetyBowSlam = model.functions.valuesNow.bowSlam;
				model.arc[arcNr].safetyGreenWater = model.functions.valuesNow.greenWater;
				model.arc[arcNr].safetyDynStability = model.functions.valuesNow.dynamicStability;
				model.arc[arcNr].feasibleSafety = model.functions.valuesNow.feasibleSafety;
				model.arc[arcNr].iceCoverCost = model.functions.valuesNow.iceCoverCost;
				//model.arc[arcNr].safetyStability = worstStabilityValue;
				model.arc[arcNr].safetyBase = safety;
				model.arc[arcNr].channelCost = channelCost;
				model.arc[arcNr].totCost = totCost;
				model.arc[arcNr].nodNr1 = nodNr1;
				model.arc[arcNr].nodNr2 = nodNr2;
				model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
				model.nArcs++;
				if (model.nArcs == 203913)
					arcNr = arcNr;
				nArcsNu++;
			}
			if (tidInt < *min_t_nu)
				*min_t_nu = tidInt;
			if (tidInt > *max_t_nu)
				*max_t_nu = tidInt;
		}
	}
	model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
	model.durationCheckAddBagar += model.tmpTid2[1] - model.tmpTid2[0];

	return nArcsNu;
}

int try_addBage_fromPath(int thisLevel, int nextLevel, int pos1, int pos2, int speedSetting, int tPos)
{
	// i = thisLevel, i1 = pointPos, i+1 = nextLevel, i2 = outNodePos, i3 = tPos
	int i4, tidInt, nArcsNu = 0, nodNr1, nodNr2, posNy, arcNr, prefPath = 0;
	double tid; // , safety, fuel, distance, totCost;
	double fuelVLSFO, fuelLSMGO, fuelBase, safetyBase, totCost;
	double worstStormValue; // , worstStabilityValue;

	double fuelQualityKvot = get_fuelQualityKvot(thisLevel, pos1, nextLevel, pos2); //, fuelRaster);

	model.params.speedSettings_addOnlyCheapestArcs = 0;

	if (thisLevel >= 0 && nextLevel >= 0) {
		if (pos1 == model.params.preferredPathOrtoPos[thisLevel] && pos2 == model.params.preferredPathOrtoPos[nextLevel] && thisLevel == nextLevel - 1)
			prefPath = 1;
	}

	i4 = speedSetting;
	if (thisLevel >= 0) {
		if (nextLevel >= 0) {
			if (prefPath == 1)
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[thisLevel].point[pos1], thisLevel);
			else
				calcWeatherPosAlongArc(model.network.physicalLev[thisLevel].point[pos1],
					model.network.physicalLev[nextLevel].point[pos2]);
		}
		else {
			calcWeatherPosAlongArc(model.network.physicalLev[thisLevel].point[pos1],
				model.network.channel[-nextLevel - 1].point[0]);
		}
	}
	else {
		if (nextLevel >= 0) {
			calcWeatherPosAlongArc(model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1],
				model.network.physicalLev[nextLevel].point[pos2]);
		}
		else {
			calcWeatherPosAlongChannel(-thisLevel - 1);
			//model.network.channel[-thisLevel - 1].point[pointPos],
			//	model.network.channel[-nextLevel-1].point[outNodePos]);
		}
	}
	if (thisLevel >= 0) {
		nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
		tid = calcArcTimeCost(model.network.physicalLev[thisLevel].timeInterval[pos1][tPos],
			i4, 1); // , & fuel, & safety, & distance, & worstStormValue, & worstStabilityValue);
		tidInt = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] + (int)tid;
	}
	else {
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
		tid = calcArcTimeCost(model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos],
			i4, 1); // , & fuel, & safety, & distance, & worstStormValue, & worstStabilityValue);
		tidInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos] + (int)tid;
	}

	if (thisLevel < 0 && nextLevel < 0) {
		tid += model.network.channel[-thisLevel - 1].extraTimeChannel;
		totCost = model.network.channel[-thisLevel - 1].extraCostChannel;
	}
	else
		totCost = 0;

	fuelVLSFO = model.functions.valuesNow.fuel * (1 - fuelQualityKvot);
	fuelLSMGO = model.functions.valuesNow.fuel * fuelQualityKvot;
	fuelBase = fuelVLSFO * model.params.weightFuel.vlsfo * model.params.priceFuel.vlsfo + fuelLSMGO * model.params.weightFuel.lsmgo * model.params.priceFuel.lsmgo;

	safetyBase = model.functions.valuesNow.worstStormValue *
		model.params.weightSafety.hurricane +
		model.functions.valuesNow.bowSlam *
		model.params.weightSafety.bowSlam +
		model.functions.valuesNow.greenWater *
		model.params.weightSafety.greenWater +
		model.functions.valuesNow.dynamicStability *
		model.params.weightSafety.dynamicStability +
		model.functions.valuesNow.feasibleSafety *
		model.params.weightSafety.feasibleSafety +
		model.functions.valuesNow.iceCoverCost;

	totCost += model.params.weightTime * model.params.priceTime * tid +
		fuelBase + model.params.weightSafety.base * safetyBase;
	nodNr2 = addTimeTo_timeInterval(thisLevel, nextLevel, pos2, tidInt);
	//pos = model.network.physicalLev[i - 1].nOutArcs[i1];
	posNy = adderaArc(nodNr1, nodNr2, totCost);
	if (posNy < 0) {
		arcNr = model.nArcs;
		if (arcNr >= model.nAllocArcs) {
			model.nAllocArcs += 100000;
			model.arc = (strArcInfo*)realloc(model.arc,
				model.nAllocArcs * sizeof(strArcInfo));
		}
		model.arc[arcNr].fromLevel = thisLevel;
		model.arc[arcNr].toLevel = nextLevel;
		model.arc[arcNr].fromPointNr = pos1;
		model.arc[arcNr].toPointNr = pos2;
		if (nextLevel >= 0)
			(model.network.physicalLev[nextLevel].nArcsToPoint[pos2])++;
		else
			(model.network.channel[-nextLevel - 1].nArcsToPoint[pos2])++;
		if (thisLevel >= 0)
			model.arc[arcNr].fromTime = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
		else
			model.arc[arcNr].fromTime = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
		model.arc[arcNr].toTime = tidInt;
		model.arc[arcNr].speedSetting = i4;
		model.arc[arcNr].time = tid;
		model.arc[arcNr].distance = model.functions.valuesNow.distance;
		model.arc[arcNr].fuelBase = fuelBase;
		model.arc[arcNr].fuelVLSFO = fuelVLSFO;
		model.arc[arcNr].fuelLSMGO = fuelLSMGO;
		model.arc[arcNr].safetyHurricane = model.functions.valuesNow.worstStormValue;
		model.arc[arcNr].safetyBowSlam = model.functions.valuesNow.bowSlam;
		model.arc[arcNr].safetyGreenWater = model.functions.valuesNow.greenWater;
		model.arc[arcNr].safetyDynStability = model.functions.valuesNow.dynamicStability;
		model.arc[arcNr].feasibleSafety = model.functions.valuesNow.feasibleSafety;
		model.arc[arcNr].iceCoverCost = model.functions.valuesNow.iceCoverCost;
		//model.arc[arcNr].safetyStability = worstStabilityValue;
		model.arc[arcNr].safetyBase = safetyBase;
		model.arc[arcNr].totCost = totCost;
		model.arc[arcNr].nodNr1 = nodNr1;
		model.arc[arcNr].nodNr2 = nodNr2;
		model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
		posNy = model.nArcs;
	}

	return posNy;
}

int check_useRaster_longitude(int weatherNr, int filNr) {

	if (model.boundingBox.xMax <= model.weather[weatherNr].filePos[filNr].minX) {
		if (model.boundingBox.xMin + 360 >= model.weather[weatherNr].filePos[filNr].maxX)
			return 0;
	}
	if (model.boundingBox.xMin >= model.weather[weatherNr].filePos[filNr].maxX) {
		if (model.boundingBox.xMax - 360 <= model.weather[weatherNr].filePos[filNr].minX)
			return 0;
	}

	return 1;
}

void loadWeatherFiles() {
	int xPos0, xPos1, yPos0, yPos1, nBands;
	int pos2, pos3, i4, i5, nAlloc, i3, offset_x, offset_y;
	int nCols, nRows, nRows_inBlock, nCols_inBlock;
	int yStartBlock, yStartValue, nY_valueAdd, yEndBlock;
	int xStartBlock, xStartValue, nX_valueAdd, xEndBlock, i;
	int xBlockNr, xBlockUse, yBlockNr, pos, latPos, lonPos, tidInt;
	int nTimeIntervals_forecast_redis, nTimeIntervals_redis, forstaOverT, startT;
	int nDefault, nNoll, nTot;

	long long maxTid, sekNu, offset_x2;
	double min_lon, max_lon, min_lat, max_lat, min_lonUse, max_lonUse;
	double size_col, size_row, xPosFrac, yPosFrac, lat, lon;
	double xPosFrac0, xPosFrac1, yPosFrac0, yPosFrac1;
	float* arrFloat;
	std::string keyID;

	//#ifndef WIN32_AAAAA
	auto redis = Redis("tcp://127.0.0.1:6379/1");
	//#endif

		//errlog("test14\n");
	for (int ii = 0; ii < model.nWeatherFiles; ii++) {
		model.tmpTid[0] = std::chrono::high_resolution_clock::now();
		//printf("\nweather %d variable %s\n", ii, model.weather[ii].weatherFileTypeName);
		//if (ii == 3)
		//	ii = ii;
#ifdef WIN32_AAAAA
		// identifiera vilka raster som behover oppnas, och oppna dem
		size_col = -1;
		for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			if (check_useRaster_longitude(ii, i1) == 1) {
				model.weather[ii].rasterPos[i1].open(model.weather[ii].filePos[i1].fileName);
				if (size_col < 0) {
					size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
					model.weather[ii].size_col = size_col;
					xPosFrac = (model.boundingBox.xMin - model.weather[ii].rasterPos[i1].Get_minLongitude()) / size_col;
					xPos0 = (int)xPosFrac;
					model.weather[ii].minX = model.weather[ii].rasterPos[i1].Get_minLongitude() +
						xPos0 * size_col;
					xPosFrac = (model.boundingBox.xMax - model.weather[ii].minX) /
						size_col;
					xPos1 = roundUp(xPosFrac);
					model.weather[ii].maxX = model.weather[ii].minX +
						xPos1 * size_col;
					model.weather[ii].nCols = xPos1 + 1;

					size_row = model.weather[ii].rasterPos[i1].Get_sizeRow();
					model.weather[ii].size_row = size_row;
					yPosFrac = (model.weather[ii].rasterPos[i1].Get_maxLatitude() - model.boundingBox.yMax) /
						size_row;
					yPos0 = (int)yPosFrac;
					model.weather[ii].maxY = model.weather[ii].rasterPos[i1].Get_maxLatitude() -
						yPos0 * size_row;
					yPosFrac = (model.weather[ii].maxY - model.boundingBox.yMin) /
						size_row;
					yPos1 = roundUp(yPosFrac);
					if (yPos1 >= model.weather[ii].rasterPos[i1].Get_nRows())
						yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
					model.weather[ii].minY = model.weather[ii].maxY -
						yPos1 * size_row;
					model.weather[ii].nRows = yPos1 + 1;

					if (ii == 3)
						ii = ii;
					nBands = model.weather[ii].rasterPos[i1].Get_nBands();
					model.weather[ii].nTimeIntervals = nBands;
					model.weather[ii].secondsUTC = (long long*)malloc(nBands * sizeof(long long));
					model.weather[ii].valueCell = (float**)malloc(nBands * sizeof(float*));
					nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
					for (int i2 = 0; i2 < nBands; i2++) {
						model.weather[ii].valueCell[i2] = (float*)malloc(nAlloc * sizeof(float));
					}
				}
				else {
					if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
						errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
							model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
					if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
						errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
							model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
				}
				if (ii == 3)
					ii = ii;
				model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0);

			}
		}

#else

		keyID.assign(model.weather[ii].weatherFileTypeName);
		keyID.append(":metaData");
		auto reply = redis.get(keyID);
		if (reply) {
			auto val = nlohmann::json::parse(*reply);
			nCols = val["nCols"];
			nRows = val["nRows"];
			nTimeIntervals_forecast_redis = val["nTimeIntervals_forecast"];
			nTimeIntervals_redis = val["nTimeIntervals"];
			size_col = val["size_col"];
			size_row = val["size_row"];
			min_lon = val["minX"];
			max_lon = val["maxX"];
			min_lat = val["minY"];
			max_lat = val["maxY"];
			nRows_inBlock = val["nBlockRows"]; // pnYSize;
			nCols_inBlock = val["nBlockCols"]; // pnXSize;
			json UTC = val["UTCtimes"];
			pos = 0;
			model.weather[ii].secondsUTC = (long long*)malloc(nTimeIntervals_redis * sizeof(long long));
			forstaOverT = -1;
			for (auto it = UTC.begin(); it != UTC.end(); ++it) {
				model.weather[ii].secondsUTC[pos] = it.value();
				if (model.weather[ii].secondsUTC[pos] > model.params.UTC_secondsStart && forstaOverT == -1)
					forstaOverT = pos;
				pos++;

			}
		}
		else {
			errlog("ERROR! Failed to read metaData from redis for weather variable %s\n",
				model.weather[ii].weatherFileTypeName);
			exitKontrollerat(__LINE__);
		}

		if (forstaOverT <= 0) {
			if (forstaOverT == 0) {
				errlog("ERROR! %s Start planning a route at UTC second %I64d but weather data only starts at %I64d, diff %.2lf hours \n",
					model.weather[ii].weatherFileTypeName, model.params.UTC_secondsStart, model.weather[ii].secondsUTC[0],
					(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[0]) / 3600.0);
				startT = 0;
			}
			else {
				errlog("ERROR! Start planning a route at UTC second %I64d but last weather data for parameter %d is %I64d, diff %.2lf hours \n",
					model.params.UTC_secondsStart, model.weather[ii].secondsUTC[pos - 1],
					(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[pos - 1]) / 3600.0);
				startT = pos - 1;
			}
		}else
			startT = forstaOverT - 1;
		if (startT > 0) {
			for (i = startT; i < nTimeIntervals_redis; i++)
				model.weather[ii].secondsUTC[i - startT] = model.weather[ii].secondsUTC[i];
		}

		model.weather[ii].nTimeIntervals_forecast = nTimeIntervals_forecast_redis - startT;
		model.weather[ii].nTimeIntervals = nTimeIntervals_redis - startT;

		// last dateTime plus 24 hours...
		nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather[ii].timeIntervall_h) + 1;
		//nAlloc = (int)((model.weather[ii].nTimeIntervals - model.weather[ii].nTimeIntervals_forecast) * 24 / model.weather[ii].timeIntervall_h);
		//if (model.params.UTC_secondsStart < model.weather[ii].secondsUTC[0]) {
		//	nAlloc += (int)((model.weather[ii].secondsUTC[0] - model.params.UTC_secondsStart) / 3600 / model.weather[ii].timeIntervall_h) + 1;
		//}
		//nAlloc += model.weather[ii].nTimeIntervals_forecast + 10; // a safety buffer of 10 in case we get one or two extra...
		
		//printf("alloc %d for timeIntervalIndex for %s\n", nAlloc, model.weather[ii].weatherFileTypeName);
		model.weather[ii].timeIntervalIndex = (int*)malloc(nAlloc * sizeof(int));

		printf("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
			ii, model.weather[ii].nTimeIntervals,
			model.weather[ii].nTimeIntervals_forecast, model.weather[ii].timeIntervall_h, nAlloc);
		tidInt = 0;
		for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

			if (i == model.weather[ii].nTimeIntervals - 1)
				maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
			else
				maxTid = model.weather[ii].secondsUTC[i + 1] - 1;

			for (; tidInt < 100000; tidInt++) {
				if (tidInt >= nAlloc) {
					printf("ERROR! Too many tidInt compared to allocated for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", ii,
						model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
					errlog("ERROR! Too many tidInt compared to allocated for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", ii,
						model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
					break;
				}
				sekNu = (long long)(tidInt * model.weather[ii].timeIntervall_h * 3600 + model.params.UTC_secondsStart);
				if (sekNu <= maxTid)
					model.weather[ii].timeIntervalIndex[tidInt] = i;
				else
					break;
			}
			//printf("weather %d i %d tidInt %d (over maxTid) sekNu %I64d maxSec %I64d\n", ii, i, tidInt, sekNu, maxTid);
		}
		//printf("tidInt %d nAlloc %d\n", tidInt, nAlloc);

		model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;


		//printf("nCols/Rows %d %d nTidsp %d startTP %d size_col/row %.3lf %.3lf lon %.3lf %.3lf inBlockCols/rows %d %d\n",
		//	nCols, nRows, model.weather[ii].nTimeIntervals, startT, 
		//	size_col, size_row, min_lon, max_lon, nCols_inBlock, nRows_inBlock);
		//printf("bounding mox min/maxX %.3lf %.3lf\n", model.boundingBox.xMin, model.boundingBox.xMax);
		if (model.boundingBox.xMin < min_lon) {
			min_lonUse = min_lon - 360;
			max_lonUse = max_lon - 360;
		}
		else {
			if (model.boundingBox.xMin >= min_lon + 360) {
				min_lonUse = min_lon + 360;
				max_lonUse = max_lon + 360;
			}
			else {
				min_lonUse = min_lon;
				max_lonUse = max_lon;
			}

		}
		//if (max_lon < model.weather[ii].minX) {
		//	min_lonUse = min_lon + 360;
		//	max_lonUse = max_lon + 360;
		//}
		//else {
		//	if (min_lon > model.weather[ii].maxX) {
		//		min_lonUse = min_lon - 360;
		//		max_lonUse = max_lon - 360;
		//	}
		//	else {
		//		min_lonUse = min_lon;
		//		max_lonUse = max_lon;
		//	}
		//}

		model.weather[ii].size_col = size_col;
		model.weather[ii].size_row = size_row;
		nBands = model.weather[ii].nTimeIntervals;

		xPosFrac = (model.boundingBox.xMin - min_lonUse) / size_col;
		xPos0 = (int)xPosFrac;

		// is xPos0 correct if xPosFrac is negative, i.e. -0.5 => -1?
		model.weather[ii].minX = min_lonUse + xPos0 * size_col;
		xPosFrac = (model.boundingBox.xMax - model.weather[ii].minX) / size_col;
		xPos1 = roundUp(xPosFrac);
		model.weather[ii].maxX = model.weather[ii].minX + xPos1 * size_col;
		model.weather[ii].nCols = xPos1 + 1;
		//if (model.weather[ii].maxX > max_lonUse) {
		//	printf("changes maxX %.3lf nCols %d to ", model.weather[ii].maxX, model.weather[ii].nCols);
		//	model.weather[ii].maxX -= (model.weather[ii].nBlock_x * nCols_inBlock - nCols) * size_col;
		//	model.weather[ii].nCols -= model.weather[ii].nBlock_x * nCols_inBlock - nCols;
		//	printf("%.3lf %d\n", model.weather[ii].maxX, model.weather[ii].nCols);
		//}

		//printf("bbox %.2lf %.2lf weather min/max %.2lf %.2lf size_col %.2lf .nCols %d",
		//	model.boundingBox.xMin, model.boundingBox.xMax, model.weather[ii].minX,
		//	model.weather[ii].maxX, size_col, model.weather[ii].nCols);

		yPosFrac = (max_lat - model.boundingBox.yMax) /
			size_row;
		yPos0 = (int)yPosFrac;
		model.weather[ii].maxY = max_lat -
			yPos0 * size_row;
		yPosFrac = (model.weather[ii].maxY - model.boundingBox.yMin) /
			size_row;
		yPos1 = roundUp(yPosFrac);
		if (yPos1 >= nRows)
			yPos1 = nRows - 1;
		model.weather[ii].minY = model.weather[ii].maxY -
			yPos1 * size_row;
		model.weather[ii].nRows = yPos1 + 1;

		model.weather[ii].valueCell = (float**)malloc(nBands * sizeof(float*));
		nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
		//printf("ii %d nCols/nRows %d %d nAlloc %d nBands %d fromToX %.3lf %.3lf size %.3lf\n", ii, 
		//	model.weather[ii].nCols, model.weather[ii].nRows, nAlloc, nBands,
		//	model.boundingBox.xMin, model.boundingBox.xMax, size_col);
		for (int i2 = 0; i2 < nBands; i2++) {
			//printf("bandNr %d\n", i2);
			model.weather[ii].valueCell[i2] = (float*)malloc(nAlloc * sizeof(float));
		}

		nAlloc = nBands * nCols_inBlock * nRows_inBlock;
		if (ii > 0)
			delete arrFloat;
		arrFloat = new float[nAlloc];
		//printf("blockAlloc nBands %d ncols %d nrows %d nAlloc %d\n",
		//	nBands, nCols_inBlock, nRows_inBlock, nAlloc);




		//printf("weather data needed from box min/max xy %.4lf %.4lf %.4lf %.4lf\n",
		//	model.weather[ii].minX, model.weather[ii].minY,
		//	model.weather[ii].maxX, model.weather[ii].maxY);
		//printf("weather data from box min/max xy %.4lf %.4lf %.4lf %.4lf\n",
		//	min_lonUse, min_lat, max_lonUse, max_lat);

		xPosFrac0 = (model.weather[ii].minX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock;
		xPosFrac1 = (model.weather[ii].maxX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock;
		if (model.weather[ii].maxX > max_lonUse) {
			xPosFrac1 = (model.weather[ii].maxX - min_lonUse + (model.weather[ii].nBlock_x * nCols_inBlock - nCols) * size_col) / 
				(max_lonUse - min_lonUse) * nCols / nCols_inBlock;
		}else
			xPosFrac1 = (model.weather[ii].maxX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock;

		//xxx doesnt work;
		//if (model.weather[ii].minX - min_lonUse < 0)
		//	xPosFrac0 = (360 + model.weather[ii].minX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock -
		//	model.weather[ii].nBlock_x;
		//else
		//	xPosFrac0 = (model.weather[ii].minX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock;
		//if (model.weather[ii].maxX - min_lonUse < 0)
		//	xPosFrac0 = (360 + model.weather[ii].maxX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock -
		//	model.weather[ii].nBlock_x;
		//else
		//	xPosFrac1 = (model.weather[ii].maxX - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock;

		xPos0 = int(xPosFrac0);
		xPos1 = int(xPosFrac1);
		printf("xPosFrac0 %.3lf xPos0 %d xPosFrac1 %.3lf xPos1 %d\n",
			xPosFrac0, xPos0, xPosFrac1, xPos1);

		yPosFrac0 = (max_lat - model.weather[ii].maxY) / (max_lat - min_lat) * nRows / nRows_inBlock;
		yPosFrac1 = (max_lat - model.weather[ii].minY) / (max_lat - min_lat) * nRows / nRows_inBlock;
		if (yPosFrac0 < 0) {
			yPos0 = 0;
			yPosFrac0 = 0;
		}
		else
			yPos0 = (int)yPosFrac0;
		if (yPosFrac1 >= model.weather[ii].nBlock_y - 1) {
			yPos1 = model.weather[ii].nBlock_y - 1;
			if (yPosFrac1 >= model.weather[ii].nBlock_y)
				yPosFrac1 = model.weather[ii].nBlock_y - 0.0000001;
		}
		else
			yPos1 = (int)(yPosFrac1);

		//printf("\nread from blockNrs x %d %d y %d %d\n", xPos0, xPos1, yPos0, yPos1);
		for (yBlockNr = yPos0; yBlockNr <= yPos1; yBlockNr++) {
			if (yBlockNr == yPos0) {
				yStartBlock = (yPosFrac0 - yPos0) * nRows_inBlock;
				yStartValue = 0;
			}
			else {
				yStartBlock = 0;
				yStartValue += nY_valueAdd;
			}
			offset_y = yStartValue - yStartBlock;
			if (yBlockNr == yPos1) {
				yEndBlock = (int)((yPosFrac1 - yPos1) * nRows_inBlock) + 1;
				if (yEndBlock > nRows_inBlock)
					yEndBlock = nRows_inBlock;
				//printf("yEndBlock %d yPosFrac1 %.2lf yPos1 %d\n", yEndBlock, yPosFrac1, yPos1);
			}
			else {
				yEndBlock = nRows_inBlock;
				nY_valueAdd = yEndBlock - yStartBlock;
			}

			for (xBlockNr = xPos0; xBlockNr <= xPos1; xBlockNr++) {
				if (xBlockNr < 0)
					xBlockUse = model.weather[ii].nBlock_x + xBlockNr - 1;
				else {
					if (xBlockNr >= model.weather[ii].nBlock_x)
						xBlockUse = xBlockNr - model.weather[ii].nBlock_x;
					else
						xBlockUse = xBlockNr;
				}
				pos = xBlockUse + model.weather[ii].nBlock_x * yBlockNr;
				keyID.assign(model.weather[ii].weatherFileTypeName);
				keyID.append(":");
				keyID += to_string(pos);
				auto value = redis.get(keyID);
				if (value) {
					memcpy(arrFloat, value->data(), value->size());
				}
				else
					printf("ERROR! Failed to load keyID %s\n", keyID.c_str());


				if (xBlockNr == xPos0) {
					xStartBlock = (xPosFrac0 - xPos0) * nCols_inBlock;
					if (xStartBlock < 0)
						xStartBlock += nCols_inBlock;
					xStartValue = 0;
					//printf("xStartBlock %d xStartValue %d offset_x %d\n", xStartBlock, xStartValue, offset_x);
				}
				else {
					xStartBlock = 0;
					xStartValue += nX_valueAdd;
					//printf("xStartBlock %d xStartValue %d offset_x %d nX_valueAdd %d\n", xStartBlock, xStartValue, offset_x,
					//	nX_valueAdd);
				}
				offset_x = xStartValue - xStartBlock;
				//if (xBlockNr >= model.weather[ii].nBlock_x) {
				//	offset_x -= model.weather[ii].nBlock_x * nCols_inBlock - nCols;
				//	offset_x2 = -(model.weather[ii].nBlock_x * nCols_inBlock - nCols);
				//}
				//else
					offset_x2 = 0;


				if (xBlockNr == xPos1) {
					xEndBlock = (int)((xPosFrac1 - xPos1) * nCols_inBlock) + 1;
					if (xEndBlock < 0)
						xEndBlock += nCols_inBlock;
					if (xEndBlock > nCols_inBlock)
						xEndBlock = nCols_inBlock;
				}
				else {
					xEndBlock = nCols_inBlock;
					if (xBlockNr == model.weather[ii].nBlock_x - 1) {
						if (model.weather[ii].nBlock_x * nCols_inBlock > nCols)
							xEndBlock = nCols - (model.weather[ii].nBlock_x - 1) * nCols_inBlock - 1; //  > nCols;
					}
					nX_valueAdd = xEndBlock - xStartBlock;
				}

				//printf("data fran xy block %d %d nXblocks %d start/end x %d %d y %d %d offset xy %d %d startT %d nBands %d\n", 
				//	xBlockUse, yBlockNr, model.weather[ii].nBlock_x,
				//	xStartBlock, xEndBlock, yStartBlock, yEndBlock, offset_x, offset_y, startT, nBands);
				//printf("xBlockNr %d xPos0 %d xPosFrac0 %.4lf nCols_inBlock %d xPosFrac1 %.4lf xPos1 %d\n",
				//	xBlockNr, xPos0, xPosFrac0, nCols_inBlock, xPosFrac1, xPos1);
				//if (ii == 2)
				//	printf("i4 %d %d min/max posCell %d %d i5 %d %d min/max posCell %d %d\n", yStartBlock, yEndBlock - 1,
				//		yStartBlock + offset_y, yEndBlock - 1 + offset_y, xStartBlock, xEndBlock - 1,
				//		xStartBlock + offset_x, xEndBlock - 1 + offset_x);
				//printf("## blockStartX %.3lf valueCellStartX %.3lf blockStartX+360 %.3lf\n",
				//	min_lonUse + xBlockUse * nCols_inBlock * size_col,
				//	model.weather[ii].minX + offset_x * size_col,
				//	min_lonUse + xBlockUse * nCols_inBlock * size_col + 360);
				for (i3 = startT; i3 < nBands; i3++) {
					for (i4 = yStartBlock; i4 < yEndBlock; i4++) {
						for (i5 = xStartBlock; i5 < xEndBlock; i5++) {
							pos2 = i5 + nCols_inBlock * (i4 + nRows_inBlock * i3);
							pos3 = i5 + offset_x + model.weather[ii].nCols * (i4 + offset_y);
							model.weather[ii].valueCell[i3-startT][pos3] = arrFloat[pos2];
							if(ii== 200 && arrFloat[pos2] >9)
								printf("ERROR! too high current %.2lf lon/lat %.2lf %.2lf bandNr %d lon/lat pos %d %d\n",
									arrFloat[pos2], -model.weather[2].minX + (i5 + offset_x) *
									model.weather[2].size_col,
									model.weather[2].maxY -
									(i4 + offset_y) *
									model.weather[2].size_row,
									i3, i5+offset_x, i4+offset_y);
							if(ii ==1000 && (i3 >= 48||pos3>=120||pos3< 0||i3<0))
								printf("ERROR i3 %d pos3 %d i5 %d xStartValue %d i4 %d yStartValue %d nCols %d\n", 
									i3, pos3, i5, xStartValue, i4, yStartValue, model.weather[ii].nCols);
							//if (ii == 1 && i3 == 5 && i4 + yStartValue == 0 && i5 + xStartValue == 9)
							//	printf("\n\n#########\nii %d i3 %d i4 %d i5 %d pos3 %d pos2 %d val %.2lf\n\n",
							//		ii, i3, i4, i5, pos3, pos2, arrFloat[pos2]);
							
						}
					}
				}
				//printf("saved to valueCell pos z %d %d x %d %d y %d %d ftomToX %.3lf %.3lf\n", 0, nBands - 1 - startT,
				//	xStartBlock + offset_x, xEndBlock - 1 + offset_x,
				//	yStartBlock + offset_y, yEndBlock - 1 + offset_y,
				//	min_lonUse + (xBlockNr * nCols_inBlock + xStartBlock + offset_x2) * size_col,
				//	min_lonUse + (xBlockNr * nCols_inBlock + xEndBlock + offset_x2) * size_col);
				//if (ii == 3) {
				//	FILE* filCheck2 = fopen("checkWeather3.txt", "w");
				//	printf("keyID %s\n", keyID.c_str());
				//	for (i3 = 0; i3 < nBands; i3++) {
				//		for (i4 = 0; i4 < nRows_inBlock; i4++) {
				//			for (i5 = 0; i5 < nCols_inBlock; i5++) {
				//				pos2 = i5 + nCols_inBlock * (i4 + nRows_inBlock * i3);
				//				fprintf(filCheck2, "%.3lf ", arrFloat[pos2]);
				//			}
				//			fprintf(filCheck2, "\n");
				//		}
				//	}
				//	fclose(filCheck2);
				//}


			}
		}

#endif

		//testCoordValue(ii, 93.84, 5.98);
		//testCoordValue(ii, 167.6111, 81);
		//testCoordValue(ii, 179.6111, 82);
		//testCoordValue(ii, -179.6111, 83);
		//testCoordValue(ii, -132.39, 84);


		// om olika diskretization pa oppnade raster sa stoppa
		// 



		//model.weather[ii].valueCell = model.weather[ii].rasterPos.GetRasterBand_realArrAllBands(&(model.weather[ii].raster), model.boundingBox);
		//printf("used dim %d %d tid %lf nBands %d\n", model.weather[ii].nRows,
		//	model.weather[ii].nCols, model.durationMilli[ii], model.weather[ii].nTimeIntervals);


		nDefault = 0;
		nNoll = 0;
		nTot = 0;
		for (int ii3 = 0; ii3 < model.weather[ii].nTimeIntervals; ii3++) {
			if (ii == 3 && ii3 == 3)
				ii3 = ii3;
			//model.weather[ii].valueCell[ii3] = model.weather[ii].rasterPos.GetRasterBand_realArr(ii3 + 1, &(model.weather[ii].raster), model.boundingBox);

			//for (int ii4 = 0; ii4 < model.weather[ii].rasterPos.Get_nRows(); ii4++) {
			for (int ii4 = 0; ii4 < model.weather[ii].nRows; ii4++) {
				latPos = ii4;
				//for (int ii5 = 0; ii5 < model.weather[ii].rasterPos.Get_nCols(); ii5++) {
				for (int ii5 = 0; ii5 < model.weather[ii].nCols; ii5++) {
					lonPos = ii5;
					//lat = model.weather[ii].maxY - (latPos + 0.5) * model.weather[ii].rasterPos.Get_sizeRow();
					//lon = (lonPos + 0.5) * model.weather[ii].rasterPos.Get_sizeCol() - model.weather[ii].minX;
					if (model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5] < -10000 ||
						model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5] > 10000) {
						lat = model.weather[ii].maxY - (latPos + 0.5) * model.weather[ii].size_row;
						lon = (lonPos + 0.5) * model.weather[ii].size_col + model.weather[ii].minX;
						errlog("ERROR! value of weather %d %s band %d lon/lat %.3lf %.3lf i4/i5 %d %d nCols/Rows %d %d kvotxy %.3lf %.3lf is %lf\n", ii,
							model.weather[ii].weatherFileTypeName,
							ii3 + 1, lon, lat, ii4, ii5, model.weather[ii].nCols, model.weather[ii].nRows,
							(lon - min_lonUse) / (max_lonUse - min_lonUse) * nCols / nCols_inBlock,
							(max_lat - lat) / (max_lat - min_lat) * nRows / nRows_inBlock,
							model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5]);
					}
					if (model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5] > 9998)
						nDefault++;
					if (abs(model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5]) < 0.0001)
						nNoll++;
					nTot++;
				}
			}
		}
		printf("%d %s kvoter default %.3lf noll %.3lf nVarden %d\n", ii, model.weather[ii].weatherFileTypeName,
			(double)nDefault / nTot, (double)nNoll / nTot, nTot);
		
		//errlog("iicc %d ii2 %d\n", ii, ii2);
		model.tmpTid[1] = std::chrono::high_resolution_clock::now();
		model.durationMilli[ii] += model.tmpTid[1] - model.tmpTid[0];
		(model.nCallsWeatherBand[ii])++;
		model.durationMilliTot += model.tmpTid[1] - model.tmpTid[0];
		errlog("weather %d variable %s nTimeInt %d dim %d %d tid %lf\nminLon %.3lf maxLon %.3lf\nminLat %.3lf maxLat %.3lf\n", ii,
			model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals,
			model.weather[ii].nRows,
			model.weather[ii].nCols,
			model.durationMilli[ii],
			model.weather[ii].minX, model.weather[ii].maxX,
			model.weather[ii].minY, model.weather[ii].maxY);

	}
	printf("all weathedata loaded\n");
	//exit(0);

}

int createTimeArcs()
{
	int i, i1, nAlloc, i2, i3, setupCheckPoints;
	int tidInt, min_t_nu, nArcsTot, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, max_t_nu, nextLevel;
	int cNr;
	double fuel, safety, tid, totCost, fuelQualityKvot;

	// model.rasterData.fuelGeography = model.fuelGeographyMapRaster[0].GetRasterBand(1);
	//float** fuelRaster = model.fuelGeographyMapRaster[0].GetRasterBand(1);

	errlog("OBS! Fixed variables, order and operations. Develop when I know more about the variables\n");

	model.weatherFunctions.nAllocPoints = 50;
	model.weatherFunctions.vesselBearing = (double*)malloc(
		model.weatherFunctions.nAllocPoints * sizeof(double));
	model.weatherFunctions.point = (spherical::Point*)malloc(
		model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
	model.weatherFunctions.checkPoint = (strCheckPkt*)malloc(model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
	for (i = 0; i < model.weatherFunctions.nAllocPoints; i++) {
		//model.weatherFunctions.checkPoint[i].fileNr = (int*)malloc(model.nWeatherFiles * sizeof(int));
		model.weatherFunctions.checkPoint[i].latPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
		model.weatherFunctions.checkPoint[i].lonPos = (int*)malloc(model.nWeatherFiles * sizeof(int));
	}
	//errlog("test1\n");
	//model.weatherFunctions.lastFileNr = (int*)calloc(model.nWeatherFiles, sizeof(int));
	//errlog("test11\n");

	model.nAllocArcs = 100000;
	model.arc = (strArcInfo*)malloc(model.nAllocArcs * sizeof(strArcInfo));

	//errlog("test12\n");
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].nodNr_from_pt = (int**)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		model.network.physicalLev[i].nTimeIntervals = (int*)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].nAllocTimeIntervals = (int*)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].timeInterval = (int**)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		if (i == 0) {
			model.network.physicalLev[i].timeInterval[0] = (int*)malloc(sizeof(int));
			model.network.physicalLev[i].timeInterval[0][0] = model.params.startDelay_h;
			model.network.physicalLev[i].nodNr_from_pt[0] = (int*)malloc(sizeof(int));
			model.network.physicalLev[i].nTimeIntervals[0] = 1;
			model.nAllocNoder = 50000;
			model.Noder = (strNoder*)malloc(model.nAllocNoder * sizeof(strNoder));
			model.nArcs = 0;
			model.nNoder = 0;
			model.network.physicalLev[i].nodNr_from_pt[0][0] = model.nNoder;
			adderaNod(i, 0, 0);
		}
		else {
			for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
				model.network.physicalLev[i].nTimeIntervals[i1] = 0;
				model.network.physicalLev[i].nAllocTimeIntervals[i1] = 100;
				model.network.physicalLev[i].timeInterval[i1] = (int*)malloc(
					model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));
				model.network.physicalLev[i].nodNr_from_pt[i1] = (int*)malloc(
					model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));

			}
		}
	}
	//errlog("test13\n");

	auto tid0c = std::chrono::high_resolution_clock::now();
	model.durationMilliTot = tid0c - tid0c;
	model.durationCheckAddBagar = tid0c - tid0c;
	model.duration1 = tid0c - tid0c;
	model.duration2 = tid0c - tid0c;
	model.duration3 = tid0c - tid0c;
	model.duration4 = tid0c - tid0c;
	model.durationMilli = (std::chrono::duration<double, std::milli> *)malloc(model.nWeatherFiles * sizeof(std::chrono::duration<double, std::milli>));
	model.nCallsWeatherBand = (int*)calloc(model.nWeatherFiles, sizeof(int));
	for(i = 0; i < model.nWeatherFiles; i++)
		model.durationMilli[i] = tid0c - tid0c;

	//float***  dataWeatherFile;
	//dataWeatherFile = (float***)malloc(model.nWeatherFiles * sizeof(float**));

	loadWeatherFiles();
	printf("After loadWeatherFiles\n");

	gen_infoWeatherAroundStorms();

	

#ifdef WIN32
	std::chrono::steady_clock::time_point tid1, tid2, tid3, tid4, tid3b, tid3c, tid3d, tt;
#else
std::chrono::system_clock::time_point tid1, tid2, tid3, tid4, tid3b, tid3c, tid3d, tt;
#endif
	std::chrono::duration<double, std::milli> dur2, dur3, dur4, dur3b, dur3c, dur3d;
	FILE* filpek11;

	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/checkNetworkSize.txt", model.params.resultPath.c_str());

	filpek11 = fopen(namn, "w");

	min_t_nu = 0;
	nArcsTot = 0;
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		//printf("lev %d", i);
		max_t = min_t_nu + model.params.maxDiffTimeFastSlow;
		min_t_nu = 99999;
		max_t_nu = 0;
		n_added_t = 0;
		nArcsNu = 0;
		model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
		tid1 = std::chrono::high_resolution_clock::now();
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			//printf("point %d", i1);
			fprintf(filpek11, "lev %d point %d nOutNodes %d nTimeInt %d\n", i, i1,
				model.network.physicalLev[i].nOutNodes[i1], model.network.physicalLev[i].nTimeIntervals[i1]);
			model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
			if (model.network.physicalLev[i].nArcsToPoint[i1] == 0 && i > 0)
				continue; // no arc to this point so no use to add arcs out
			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];
				//if (i2b > 1) {
				//	printGlobal = 1;
				//}

				if (nextLevel > i + 1)
					i = i;
				setupCheckPoints = 1;
				//printf("test\n");
				fuelQualityKvot = get_fuelQualityKvot(i, i1, nextLevel, i2);
				//printf("test1b\n");
				for (i3 = 0; i3 < model.network.physicalLev[i].nTimeIntervals[i1]; i3++) {
					if (i == 11 && i1 == 42 && nextLevel == 12 && model.network.physicalLev[i].outNode[i1][i2b] == 40 &&
						model.network.physicalLev[i].timeInterval[i1][i3] == 64)
						i = i;
					//freeMemory();

					model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
					model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
					//printf("test1bb\n");
					//if (i >= 10000) {
					//	printf("i %d i1 %d i2b %d i3 %d nextLevel %d\n", i, i1, i2b, i3, nextLevel);
					//	printGlobal = 1;
					//}
					nArcsNu += checkAddBagar_AB(i, i1, nextLevel, i2, i3, &setupCheckPoints, max_t, &min_t_nu, &max_t_nu, fuelQualityKvot);
					//printf("test1bc\n");
					model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
				}
				//printf("test1c\n");
				if (nextLevel < 0) { // add arcs for the channel path
					setupCheckPoints = 1;
					fuelQualityKvot = get_fuelQualityKvot(nextLevel, 0, nextLevel, 1);
					// endPos = model.network.channel[-nextLevel - 1].nOutNodes[0] - 1;
					for (i3 = 0; i3 < model.network.channel[-nextLevel - 1].nTimeIntervals[0]; i3++) {
						model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
						model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
						nArcsNu += checkAddBagar_AB(nextLevel, 0, nextLevel, 1, i3, &setupCheckPoints, 99999, &min_t_nu, &max_t_nu, fuelQualityKvot);
						model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
					}
				}
				//errlog("level %d p1 %d p2 %d nArcsHere %d nArcsTot %d\n", i, i1, i2, 
				//	model.network.physicalLev[i - 1].nOutArcs[i1], nArcsNu);
			}
		}
		model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
		model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
		tid2 = std::chrono::high_resolution_clock::now();
		//printf("test1\n");


		//if (i == 11)
		//	freeMemory();
		for (i1 = 0; i1 < model.network.nUsedChannels; i1++) {
			cNr = model.network.usedChannel[i1];
			for (i2b = 0; i2b < model.network.channel[cNr].nOutNodes[0]; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[0][i2b];
				if (nextLevel != i + 1)
					continue;
				setupCheckPoints = 1;
				fuelQualityKvot = get_fuelQualityKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[0][i2b]);
				for (i3 = 0; i3 < model.network.channel[cNr].nTimeIntervals[1]; i3++) {
					nArcsNu += checkAddBagar_AB(-cNr - 1, 1, nextLevel,
						model.network.channel[cNr].outNode[0][i2b], i3, &setupCheckPoints, max_t, &min_t_nu, &max_t_nu, fuelQualityKvot);
				}
			}
		}
		tid3 = std::chrono::high_resolution_clock::now();
		//printf("test2\n");

		tt = std::chrono::high_resolution_clock::now();
		auto tid1c = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1c - tid0c;
		tid3b = std::chrono::high_resolution_clock::now();
		errlog("level %d min_t %d max_t %d n_added_t %d nArcsAdded %d tidUsed %lf rasterRead %lf checkAddBagar %.2lf other %.2lf prefPath %.2lf dur3 %.2lf dur4 %.2lf\n", i,
			min_t_nu, max_t_nu, n_added_t, nArcsNu, fp_ms, model.durationMilliTot, model.durationCheckAddBagar,
			model.duration1, model.duration2, model.duration3, model.duration4);
		tid3c = std::chrono::high_resolution_clock::now();
		for (int ii = 0; ii < model.nWeatherFiles; ii++) {
			//errlog(" w%d %d %.2lf", ii, model.nCallsWeatherBand[ii], model.durationMilli[ii]);
			model.durationMilli[ii] = tid1c - tid1c;
			model.nCallsWeatherBand[ii] = 0;
		}
		//errlog("\n");
		model.durationMilliTot = tid1c - tid1c;
		model.durationCheckAddBagar = tid1c - tid1c;
		model.duration1 = tid1c - tid1c;
		model.duration2 = tid1c - tid1c;
		model.duration3 = tid1c - tid1c;
		model.duration4 = tid1c - tid1c;
		nArcsTot += nArcsNu;
		tid3d = std::chrono::high_resolution_clock::now();
		printf("\tLevel %d done (of %d). I have %d arcs now.\n",
			i + 1, model.network.nPhysicalLevels - 1, model.nArcs);
		//if (i + 1 == 4) {
		//	printf("\nsaving some arc costs as well\n\n");
		//	for (int ii = 0; ii < model.nArcs && ii < 200; ii++) {
		//		errlog("Arc %d cost %lf\n", ii, model.arc[ii].totCost);
		//	}
		//}
		dur2 = tid2 - tid1;
		dur3 = tid3 - tid1;
		dur3b = tid3b - tid1;
		dur3c = tid3c - tid1;
		dur3d = tid3d - tid1;
		tid4 = std::chrono::high_resolution_clock::now();
		dur4 = tid4 - tid1;
		errlog("dur2 %.3lf dur3 %.3lf dur3b %.3lf dur3c %.3lf dur3d %.3lf dur4 %.3lf\n", dur2, dur3, dur3b, dur3c, dur3d, dur4);

		//if (i == 11)
		//	freeMemory();

		//printf("..done\n");
	}
	fclose(filpek11);
	//freeMemory();

	// add arcs from last node and time to a super sink
	nArcsNu = 0;
	nodNr2 = adderaNod(i + 1, 0, 0);
	i1 = 0;
	for (i3 = 0; i3 < model.network.physicalLev[i].nTimeIntervals[i1]; i3++) {
		nodNr1 = model.network.physicalLev[i].nodNr_from_pt[i1][i3];
		tid = 0;
		fuel = 0;
		safety = 0;
		tidInt = (int)tid;
		totCost = 0;
		posNy = adderaArc(nodNr1, nodNr2, totCost);
		arcNr = model.nArcs;
		if (arcNr >= model.nAllocArcs) {
			model.nAllocArcs += 100000;
			model.arc = (strArcInfo*)realloc(model.arc,
				model.nAllocArcs * sizeof(strArcInfo));
		}
		model.arc[arcNr].fromLevel = i;
		model.arc[arcNr].toLevel = i + 1;
		model.arc[arcNr].fromPointNr = i1;
		model.arc[arcNr].toPointNr = 0;
		model.arc[arcNr].fromTime = model.network.physicalLev[i].timeInterval[i1][i3];
		model.arc[arcNr].toTime = tidInt;
		model.arc[arcNr].speedSetting = 0;
		model.arc[arcNr].time = tid;
		model.arc[arcNr].distance = 0;
		model.arc[arcNr].fuelBase = fuel;
		model.arc[arcNr].fuelVLSFO = 0;
		model.arc[arcNr].fuelLSMGO = 0;
		model.arc[arcNr].safetyHurricane = 0;
		model.arc[arcNr].safetyBowSlam = 0;
		model.arc[arcNr].safetyGreenWater = 0;
		model.arc[arcNr].safetyDynStability = 0;
		model.arc[arcNr].feasibleSafety = 0;
		model.arc[arcNr].iceCoverCost = 0;
		//model.arc[arcNr].safetyStability = 0;
		model.arc[arcNr].safetyBase = safety;
		model.arc[arcNr].totCost = totCost;
		model.arc[arcNr].nodNr1 = nodNr1;
		model.arc[arcNr].nodNr2 = nodNr2;
		model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
		model.nArcs++;
		if (model.nArcs == 203913)
			arcNr = arcNr;
		nArcsNu++;
	}
	nArcsTot += nArcsNu;
	errlog("level last nArcsAdded %d nArcs %d check %d\n", nArcsNu, model.nArcs, nArcsTot);
	printf("creatingTimeArcs done.\n");


	return 0;
}

int saveDijkstraData()
{
	FILE* filPek;
	int i, i1;

	filPek = fopen("dijkstraData.txt", "w");
	fprintf(filPek, "%d\t%d\n", model.nNoder, model.nAllocNoder);
	//	int nAlloc;
	//	if (model.params.nPkterOrto > 2 * model.params.max_changeDirection + 1)
	//		nAlloc = 2 * model.params.max_changeDirection + 1;
	//	else
	//		nAlloc = model.params.nPkterOrto;
	//	nAlloc *= model.params.nShip_speedSettings;
	for (i = 0; i < model.nNoder; i++) {
		fprintf(filPek, "%d\t%d\n", i, model.Noder[i].nUtNoder);
		for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
			fprintf(filPek, "%d\t%d\t%d\t%d\t%lf\n", i, i1, model.Noder[i].UtNod[i1],
				model.Noder[i].outArcNr[i1], model.Noder[i].UtNodCost[i1]);
		}
	}
	fprintf(filPek, "-1\n");
	fprintf(filPek, "%d\t%d\n", model.nArcs, model.nAllocArcs);
	for (i = 0; i < model.nArcs; i++) {
		fprintf(filPek, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", i, model.arc[i].fromPointNr,
			model.arc[i].fromLevel, model.arc[i].toPointNr, model.arc[i].toLevel, model.arc[i].fromTime,
			model.arc[i].speedSetting, model.arc[i].nodNr1, model.arc[i].nodNr2,
			model.arc[i].nodNr1_utNodPos, model.arc[i].distance,
			model.arc[i].time, model.arc[i].fuelBase, model.arc[i].safetyBase, model.arc[i].channelCost,
			model.arc[i].totCost);
	}
	fprintf(filPek, "-1\n");
	fprintf(filPek, "%d\n", model.network.nPhysicalLevels);
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		fprintf(filPek, "%d\t%d\n", i, model.network.physicalLev[i].nPoints);
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			fprintf(filPek, "%d\t%lf\t%lf\n", i1, model.network.physicalLev[i].point[i1].latitude().degrees(),
				model.network.physicalLev[i].point[i1].longitude().degrees());
		}
	}
	fprintf(filPek, "-1\n");

	fclose(filPek);

	return 0;
}


int modify_utNodCost(int alt) {
	int i, i1, arcNr;
	double cost, weightTime, weightFuel, weightSafety, weightDistance;

	weightTime = 0;
	weightFuel = 0;
	weightSafety = 0;
	weightDistance = 0;
	if (alt == 1) {
		weightTime = 100;
	}
	if (alt == 2) {
		weightFuel = 100;
	}
	if (alt == 3) {
		weightSafety = 100;
	}
	if (alt == 4) {
		weightTime = 50;
		weightFuel = 50;
	}
	if (alt == 5) {
		weightTime = 33;
		weightFuel = 33;
		weightSafety = 33;
	}
	if (alt == 6) {
		weightDistance = 100;
	}


	double maxCost = 0;
	for (i = 0; i < model.nNoder; i++) {
		for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
			arcNr = model.Noder[i].outArcNr[i1];
			cost = weightTime * model.params.priceTime * model.arc[arcNr].time +
				weightFuel * model.arc[arcNr].fuelBase + weightSafety * model.arc[arcNr].safetyBase +
				model.arc[arcNr].distance * weightDistance;
			model.Noder[i].UtNodCost[i1] = cost;
			if (cost > maxCost)
				maxCost = cost;
			if (cost < 0) {
				printf("wTime %.2lf price %.2lf time %.2lf wFuel %.2lf fBase %.2lf wSafe %.2lf sBase %.2lf dist %.2lf wDist %.2lf\n", weightTime, model.params.priceTime, model.arc[arcNr].time,
					weightFuel, model.arc[arcNr].fuelBase, weightSafety, model.arc[arcNr].safetyBase,
					model.arc[arcNr].distance, weightDistance);
			}
		}
	}

	if (maxCost > 0)
		model.Dijkstra.FAKTOR_NATVERK = (MAXVARDE_NATVERK / maxCost);

	return 0;
}

int voyageOpt(string inputPath, string resultName)
{

	double dist;
	long long Cost;
	reset_errlog();
	//callRaster();

	FILE* filPek3;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));

	initModelStatusValues();

	model.params.resultPath = splitFilename(resultName);

	sprintf(namn, "%s/result_json.json", model.params.resultPath.c_str());
	filPek3 = fopen(namn, "w");
	fprintf(filPek3, "{\n\t\"solutionShape\": \"ERROR\"\n}\n");
	fclose(filPek3);

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;

	//model.params.resultPath = resultPath;

	printf("Reading data for the problem\n");

	loadParams_theRestOld(&(model.params));
	loadParams_new(&(model.params));


	loadFunctions();

	int testOpenMultipleTimes = 0;
	if (testOpenMultipleTimes == 1) {
		test_OpenTheSameRasterMultipleTimesAndRead();
		exit(0);
	}

	if (model.params.runAlt == 1) {
		//loadpreferredPathGeojson();
		createPhysicalNetwork(1);
		exit(0);
	}

	loadVariables();

	//redisTestRead();
	//exit(0);

	// loadWeatherData();
	//loadStormsData();

	loadChannels();
	//loadpreferredPathGeojson();
	//if (model.params.corridorPath != "")
	//	loadCorridorPath();
	//else
		model.corridorPath.nLines = 0;

	printf("Creating the physical network.\n");
	createPhysicalNetwork(0);


	printf("Creating the time dimension.\n");
	createTimeArcs();


	if (model.nArcs == 0) {
		errlog("ERROR! Number of arcs is %d. No use to solve Dijkstra. Try setting preferredPath_followExactOK = 1. I quit.\n", model.nArcs);
		printf("ERROR! Number of arcs is %d. No use to solve Dijkstra. Try setting preferredPath_followExactOK = 1. I quit.\n", model.nArcs);
		exit(0);
	}

	//freeMemory();


	//model.Dijkstra = (strDijkstra*)malloc(sizeof(strDijkstra));
	//FILE* pek;
	//pek = fopen("checkDijkst0.txt", "w");
	//for (int i = 0; i < model.nNoder; i++) {
	//	for (int i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
	//		fprintf(pek, "i %d i1 %d head %d\n", i, i1, model.Noder[i].UtNod[i1]);
	//	}
	//}
	//fclose(pek);
	printf("setting up data for dijkstra's algorithm\n");
	auto tid0 = std::chrono::high_resolution_clock::now();
	SattUppDijkstraNatverk3(&model);
	auto tid1c = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1c - tid0;
	errlog("sattUppDijkstra took %lf\n", fp_ms);
	int nod1, nod2;
	nod1 = 0;
	nod2 = model.nNoder - 1;
	bool Reached;

	int nExtraOpt = 2;
	string resAltName;

	for (int ii = 0; ii < 1 + nExtraOpt; ii++) {
		if (ii > 0) {
			modify_utNodCost(ii);
			ChangeArcCosts3(&model);
		}
		printf("solving dijkstra's algorithm..");
		AnropDijkstra2(nod1, nod2, &model, &Reached);
		auto tid1c2 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms2 = tid1c2 - tid0;
		errlog("after Dijkstra %lf\n", fp_ms2);
		printf("..done. Obj %I64d\nSaving solution.\n", model.Dijkstra.OptCost);
		if (Reached == true) {
			model.BVArc = (int*)malloc(model.nNoder * sizeof(int));
			model.BVtempNodOrder = (int*)malloc(model.nNoder * sizeof(int));
			dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
			if(ii == 0)
				sprintf(namn, "%s/%s", resultPath.c_str(), model.params.solutionFileName.c_str());
			else
				sprintf(namn, "%s/resObj_%d", resultPath.c_str(), ii);

			//writeSolutionPathToShape(namn, 0);
			writeSolutionPathToGeoJson(namn, 0);

			printf("Saving solution path1\n");
			writeSolutionToJson(resultName, ii);
			//if (ii == 0)
			//	writeSolutionToJson(resultName, 0);
			//else {
			//	resAltName = resultPath;
			//	resAltName.append("//resObjAlt_");
			//	resAltName += to_string(ii);
			//	resAltName.append(".json");
			//	writeSolutionToJson(resAltName, 0);
			//}

			//sink = model.Dijkstra.nodes - model.Dijkstra.node_min + nod2;
			//errlog("dist from %d to %d: %I64d\n", nod1, nod2, sink->dist);

			if (model.params.save_weatherNodes == 1) {
				writeNodeWeatherDataToGeojson((char*)"networkNodesWeather");
				writeSolutionPathForWeatherToGeojson((char*)"networkSPWeather");
			}
			if (model.params.readSolPathFile != "") {
				readGivenSolutionPath();
				//writeSolutionPathToGeojson((char*)"solution_givenPath", 1);
			}
		}
		else {
			errlog("ERROR! Did not manage to find a route from start to finish...\n");
			printf("\nERROR! Did not manage to find a route from start to finish...\n");
		}
	}

	FILE* filpekG = fopen(resultName.c_str(), "a+"); // "result_json.json", "w");
	fprintf(filpekG, "]}\n");
	fclose(filpekG);


	//printf("All done. I quit.\n");
	//auto tid1c3 = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double, std::milli> fp_ms3 = tid1c3 - tid0;
	//errlog("all done %lf\n", fp_ms3);


	//callJsonTest();
	return 0;
}


int generate_solutionPathTest() {
	int i;
	double yNext, yNu, yUse, xNext, xNu, xUse;

	model.solutionPath.point = (spherical::Point*)malloc(model.preferredPath.nPoints * sizeof(spherical::Point));

	for (i = 0; i < model.preferredPath.nPoints; i++) {
		if (i == 0 || i == model.preferredPath.nPoints - 1)
			model.solutionPath.point[i] = spherical::Point(model.preferredPath.point[i].latitude().degrees(),
				model.preferredPath.point[i].longitude().degrees());
		else {
			yNext = model.preferredPath.point[i + 1].latitude().degrees();
			yNu = model.preferredPath.point[i].latitude().degrees();
			if (abs(yNext - yNu) < 0.5)
				yUse = yNu / 2 + yNext / 2;
			else {
				if (yNext > yNu)
					yUse = yNu + 0.25;
				else
					yUse = yNu - 0.25;
			}
			xNext = model.preferredPath.point[i + 1].longitude().degrees();
			xNu = model.preferredPath.point[i].longitude().degrees();
			if (abs(xNext - xNu) < 0.5)
				xUse = xNu / 2 + xNext / 2;
			else {
				if (xNext > xNu)
					xUse = xNu + 0.25;
				else
					xUse = xNu - 0.25;
			}
			model.solutionPath.point[i] = spherical::Point(yUse, xUse);
		}
	}
	model.solutionPath.nPoints = model.preferredPath.nPoints;

	return 0;
}

int voyageOpt_dummy(string inputName, string resultName)
{

	reset_errlog();

	auto tid0 = std::chrono::high_resolution_clock::now();
	//filPek3 = fopen("result_json.json", "w");
	//fprintf(filPek3, "{\n\t\"solutionShape\": \"ERROR\"\n}\n");
	//fclose(filPek3);

	model.params.indataPath = inputName;
	//model.params.resultPath = resultPath;

	printf("Reading data for the problem\n");
	loadParams_new(&(model.params));

	printf("Generate elementary solution path\n");
	generate_solutionPathTest();

	printf("Saving solution path2\n");
	writeSolutionToJson_dummy(resultName);

	printf("All done. I quit.\n");
	auto tid1c3 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms3 = tid1c3 - tid0;
	errlog("all done %lf\n", fp_ms3);
	return 0;
}



int check_isChannelNodePosAllowed(int nr, int pos) {
	int i, i1, i2;
	double distNu, dist1, dist2, dist1b, dist3, minDist, kvot, distTmp, distNu1, distNu3;
	double distCorridorSegm, distIntersectCorridor, distPrefPath, distIntersect, distLimit;
	spherical::Point p1, p2, pMid, pC, pN, pointNu;

	distLimit = model.params.shipSpeed_average / model.params.ortoDist_nPointsPerHour * (model.params.nPkterOrto - 1) / 2;
	minDist = distLimit;
	pC = model.network.channel[nr].point[pos];
	for (i = 1; i < model.preferredPath.nPoints; i++) {
		if (i == 1) {
			p1 = model.preferredPath.point[i - 1];
			dist1 = p1.distanceTo(pC) / 1000.0;
		}
		else {
			p1 = p2;
			dist1 = dist2;
		}
		p2 = model.preferredPath.point[i];
		pMid = p1.midpointTo(p2);

		dist2 = p2.distanceTo(pC) / 1000.0;
		dist3 = pMid.distanceTo(pC) / 1000.0;
		if (dist3 < dist1 || dist3 < dist2) {
			distNu = abs(pC.crossTrackDistanceTo(p1, p2)) / 1000.0;
			if (distNu < minDist) {
				distTmp = pC.alongTrackDistanceTo(p1, p2) / 1000.0; // Returns how far 'this' point is along a path from start-point.
				dist1b = p1.distanceTo(p2) / 1000.0;
				if (distTmp < dist1b) {
					minDist = distNu;
					if (dist1b > 0.1) {
						kvot = distTmp / dist1b;
						pN = p1.intermediatePointTo(p2, kvot);
					}
					else
						pN = p1;
				}
			}
		}
		if (dist1 < dist2) {
			distNu = dist1;
			if (distNu < minDist) {
				minDist = distNu;
				pN = p1;
			}
		}
		else {
			distNu = dist2;
			if (distNu < minDist) {
				minDist = distNu;
				pN = p2;
			}
		}
	}
	//fprintf(filtmp, "nr %d pos %d minDist %.2lf distLimit %.2lf\n", nr, pos, minDist, distLimit);
	if (minDist >= distLimit - 0.001)
		return 0; // too far distance to channel

	// om det finns corridor, path fran narmsta pkt pa pref till pos i channel far ej brytas av corridor
	return 1;
}