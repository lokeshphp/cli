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

using json = nlohmann::json;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

strModel model;
extern string resultPath;
extern long long MAXVARDE_NATVERK;


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

inline bool exists_test3(char* name) {
	struct stat buffer;
	return (stat(name, &buffer) == 0);
}

string splitFilename(string namn) {
	string resultat;
	size_t found;
	found = namn.find_last_of("/\\");
	resultat = namn.substr(0, found);
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
	double fuelLSMGO = 0, fuelVLSFO = 0, hurricane = 0, distanceTp, distNu, distTmp, stability = 0;
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
							stability += model.arc[arcNr].safetyStability / nTp;
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
							fprintf(filpekG, "\"speedSetting\": \"%s\", \"nodNr1\": %d, \"nodNr2\": %d,\n",
								model.params.ship_speedSettingID[model.arc[arcNr].speedSetting],
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
								stability += model.arc[arcNr].safetyStability / nTp;
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
								fprintf(filpekG, "\"speedSetting\": \"%s\", \"nodNr1\": %d, \"nodNr2\": %d,\n",
									model.params.ship_speedSettingID[model.arc[arcNr].speedSetting],
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
					stability += model.arc[arcNr].safetyStability / nTp;
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
					fprintf(filpekG, "\"speedSetting\": \"%s\", \"nodNr1\": %d, \"nodNr2\": %d,\n",
						model.params.ship_speedSettingID[model.arc[arcNr].speedSetting],
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
		model.params.weightTime, model.params.weightFuel.base,
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
		fprintf(filPek3, "\t\t\"fuel\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf, \n\t\t\t\"sub\":{\n", fuel, model.params.weightFuel.base, fuel * model.params.weightFuel.base);
		fprintf(filPek3, "\t\t\t\"VLSFO\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", fuelVLSFO, model.params.weightFuel.vlsfo,
			fuelVLSFO * model.params.weightFuel.vlsfo* model.params.priceFuel.vlsfo);
		fprintf(filPek3, "\t\t\t\"LSMGO\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf}\n\t\t\t}\n\t\t},", fuelLSMGO, model.params.weightFuel.lsmgo,
			fuelLSMGO * model.params.weightFuel.lsmgo* model.params.priceFuel.lsmgo);

		fprintf(filPek3, "\t\t\"safety\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf, \n\t\t\t\"sub\":{\n", safety, model.params.weightSafety.base, safety * model.params.weightSafety.base);
		fprintf(filPek3, "\t\t\t\"hurricane\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", hurricane, model.params.weightSafety.hurricane, hurricane * model.params.weightSafety.hurricane);
		fprintf(filPek3, "\t\t\t\"lowPressure\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", 0.0, model.params.weightSafety.lowPressure, 0.0);
		fprintf(filPek3, "\t\t\t\"waves\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf},\n", 0.0, model.params.weightSafety.waves, 0.0);
		fprintf(filPek3, "\t\t\t\"stability\":{\"value\":%.2lf, \"weight\": %.2lf, \"objAdd\": %.2lf}\n", stability, model.params.weightSafety.stability,
			stability * model.params.weightSafety.stability);
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
				fprintf(filPek3, "\t\"no of times using speedSetting %s\": %d,\n", model.params.ship_speedSettingID[i], nSpeedSettingUsed[i]);
			else
				fprintf(filPek3, "\t\"eval: no of times using speedSetting %s\": %d,\n", model.params.ship_speedSettingID[i], nSpeedSettingUsed[i]);
			fprintf(filPek, "used speedSetting %s %d times\n", model.params.ship_speedSettingID[i], nSpeedSettingUsed[i]);
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

int writeSolutionToJson(string filename, int resAlt)
{
	int nAllocPkter, i, iPos, nPkter, nArcs, ii3;
	int arcNr, lev1, lev2, pointNr1, pointNr2, timeInt, * nSpeedSettingUsed, nSpeedChanges = 0;
	double* x, * y;
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

	int nAlloc = model.network.nMaxNodesInPath * model.nBVArcs, prefPath;

	x = (double*)malloc(nAlloc * sizeof(double));
	y = (double*)malloc(nAlloc * sizeof(double));
	nSpeedSettingUsed = (int*)calloc(model.params.nShip_speedSettings, sizeof(int));

	FILE* filPek, * filPek2;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	//sprintf(namn, "%s.csv", filename);
	sprintf(namn, "%s/resSolution.csv", model.params.resultPath.c_str());
	filPek = fopen(namn, "w");
	sprintf(namn, "%s/solPath.txt", model.params.resultPath.c_str());
	filPek2 = fopen(namn, "w");
	if (resAlt == 0) {
		filpekG = fopen(filename.c_str(), "w"); // "result_json.json", "w");
		if (filpekG == NULL)
		{
			printf("Faile to open file %s for writing.\n", filename.c_str());
			errlog("Faile to open file %s for writing.\n", filename.c_str());
			exitKontrollerat(__LINE__);
		}
		initGeoJsonFil(filpekG, "result_path");
		fprintf(filpekG, "{ \"type\": \"Feature\",\n");
		fprintf(filpekG, "\"geometry\": { \"type\": \"MultiLineString\",\n");
		fprintf(filpekG, "\"coordinates\": [ [\n");

		//fprintf(filPek3, "{\n");
		//fprintf(filPek3, "\t\"solutionShape\": \"/%s\",\n", filename.c_str());
	}
	else
		filpekG = fopen(filename.c_str(), "a+"); // "result_json.json", "w");
	fprintf(filPek2, "level;nodPos;speedSetting;arcNr(for_information_only);nod1(info);nod2(info)\n");

	fprintf(filPek, "arcPos\tspeedSetting\tdistance\ttime\tfuelBase\tsafetyBase\tchannelCost\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
		"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\tnodNr1\tnodNr2\n");

	double time = 0, fuel = 0, safety = 0, totCost = 0, distance = 0, channelCost = 0;
	double fuelLSMGO = 0, fuelVLSFO = 0, hurricane = 0, distanceTp, distNu, distTmp, stability = 0;
	int ii, nTp, nAdded, ii2;
	spherical::Point pointLast, pointFinal;

	nArcs = 0;
	nPkter = 0;
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
							stability += model.arc[arcNr].safetyStability / nTp;
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
								stability += model.arc[arcNr].safetyStability / nTp;
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
					stability += model.arc[arcNr].safetyStability / nTp;
					channelCost += model.arc[arcNr].channelCost / nTp;
					totCost += model.arc[arcNr].totCost / nTp;

				}
			}
		}
	}

	for (ii2 = 0; ii2 < nPkter; ii2++) {
		if (ii2 > 0)
			fprintf(filpekG, ", ");
		fprintf(filpekG, "[ %lf, %lf, 0.0 ]\n", x[ii2], y[ii2]);
	}



	fprintf(filpekG, "\n]]},\n\"properties\": {\n");

	double averSpeed;
	char* startTime, * endTime;
	startTime = (char*)malloc(256 * sizeof(char));
	endTime = (char*)malloc(256 * sizeof(char));
	fixReadableDate(tmBas, startTime);
	tmBas.tm_hour += time;
	mktime(&tmBas);
	fixReadableDate(tmBas, endTime);

	fprintf(filpekG, "\"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n", startTime, endTime);
	fprintf(filpekG, ", \"fuelConsumption_ton\": %.3lf, \"VLSFO\": %.3lf, \"LSMGO\": %.3lf",
		fuelVLSFO + fuelLSMGO, fuelVLSFO, fuelLSMGO);
	if (time > 0)
		averSpeed = distance / time / model.params.knots_to_km;
	else
		averSpeed = 0;

	fprintf(filpekG, ", \"average speed\": %.3lf, \"safety\": %.3lf, \"channel cost\": %.3lf, \"total cost\": %.3lf",
		averSpeed, safety, channelCost, totCost);
	fprintf(filpekG, ", \"totalDistance_km\": %.3lf, \"totalTime_h\": %.3lf}}\n", distance, time);

	fprintf(filpekG, "]\n}");

	fclose(filpekG);

	fprintf(filPek, "total\tcombined\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
		distance, time, fuel, safety, channelCost, totCost);
	fprintf(filPek, "\nobj_weights\ntime\tfuel\tsafety\n%lf\t%lf\t%lf\n",
		model.params.weightTime, model.params.weightFuel.base,
		model.params.weightSafety.base);

	//printf("\nobj_weights\ntime\tfuel\tsafety\n%.2lf\t%.2lf\t%.2lf\n",
	//	model.params.weightTime, model.params.weightFuel.base,
	//	model.params.weightSafety.base);
	printf("\nobj_weights\ntime\t\t%.2lf\tcost_h\t%.2lf\nfuel VLSFO\t%.2lf\tcost_ton\t%.2lf\nfuel LSMGO\t%.2lf\tcost_ton\t%.2lf\n",
		model.params.weightTime, model.params.priceTime, 
		model.params.weightFuel.vlsfo, model.params.priceFuel.vlsfo,
		model.params.weightFuel.lsmgo, model.params.priceFuel.lsmgo);
	printf("hurricane\t%.2lf\nflow pressure\t%.2lf\nstability\t%.2lf\nwaves\t\t%.2lf\n",
		model.params.weightSafety.hurricane, model.params.weightSafety.lowPressure, 
		model.params.weightSafety.stability, model.params.weightSafety.waves);
	printf("results\ndist\t%.2lf\ntime\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\nfuel\t%.2lf\tVLSFO\t%.2lf\tLSMGO\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\nsafety\t%.2lf\tobj\t%.2lf\nchannelCost\t%.2lf\ntotObjValue\t%.2lf\n",
		distance, time, model.params.priceTime * time, model.params.weightTime* model.params.priceTime* time,
		fuelVLSFO + fuelVLSFO, fuelVLSFO, fuelLSMGO, fuelVLSFO * model.params.priceFuel.vlsfo+ fuelLSMGO * model.params.priceFuel.lsmgo,
		fuelVLSFO* model.params.weightFuel.vlsfo * model.params.priceFuel.vlsfo + fuelLSMGO * model.params.weightFuel.lsmgo * model.params.priceFuel.lsmgo,
		safety, model.params.weightSafety.base * safety, 
		channelCost, totCost); 

	for (i = 0; i < model.params.nShip_speedSettings; i++) {
		if (nSpeedSettingUsed[i] > 0) {
			fprintf(filPek, "used speedSetting %s %d times\n", model.params.ship_speedSettingID[i], nSpeedSettingUsed[i]);
			printf("used speedSetting %s %d times\n", model.params.ship_speedSettingID[i], nSpeedSettingUsed[i]);
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

int loadCorridorPath()
{
	model.corridorPath.nLines = 0;

	/*
	DBFHandle	hDBF;
	SHPHandle	hSHP;
	int iRecord, j, iPart;


	char* namn2;
	namn2 = (char*)malloc(256 * sizeof(char));
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.corridorPath.c_str());
	//	hSHP = SHPOpen("path0.shp", "rb");
	hSHP = SHPOpen(namn2, "rb");
	if (hSHP == NULL)
	{
		printf("SHPOpen(%s,\"r\") failed.\n", model.params.corridorPath.c_str());
		errlog("ERROR! Could not open %s.shp. I quit!\n", model.params.corridorPath.c_str());
		exit(2);
	}
	hDBF = DBFOpen(namn2, "rb");
	if (hDBF == NULL)
	{
		printf("DBFOpen(%s,\"r\") failed.\n", model.params.corridorPath.c_str());
		errlog("ERROR! Could not open %s.dbf. I quit!\n", model.params.corridorPath.c_str());
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

	model.corridorPath.point = (spherical::Point**)malloc(nRecords * sizeof(spherical::Point*));
	model.corridorPath.nPoints = (int*)malloc(nRecords * sizeof(int));
	for (iRecord = 0; iRecord < nRecords; iRecord++)
	{
		SHPObject* psShape;

		psShape = SHPReadObject(hSHP, iRecord);

		nVertices = psShape->nVertices;
		model.corridorPath.point[iRecord] = (spherical::Point*)malloc(nVertices * sizeof(spherical::Point));
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
			model.corridorPath.point[model.corridorPath.nLines][j] = spherical::Point(psShape->padfY[j], xVal);
		}
		model.corridorPath.nPoints[iRecord] = nVertices;
		(model.corridorPath.nLines)++;
		SHPDestroyObject(psShape);

	}
	DBFClose(hDBF);
	SHPClose(hSHP);
*/

	return 0;
}

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

int loadParams(strParams* params)
{
	int i, vardeInt;

	params->knots_to_km = 1.852;
	params->shipSpeed_average = 22;
	//params->mapPhysicalFileName = std::string();
	params->mapPhysicalBFileName = std::string();
	params->mapPhysicalAFileName = std::string();
	//params->physicalMapRasterPos = 0;
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
	params->nShip_speedSettings = 0; // 15, 20, 25
	params->max_changeDirection = 1;
	params->lengthIntervall = 1;
	params->dist_checkOKroute = 1;
	params->variableFileName = "variablesInfo.json";
	model.params.useStandardWeather = 0;
	params->physicalMap_noDataValue = 9999;
	params->speedSettings_addOnlyCheapestArcs = 1;
	params->runAlt = 0;
	params->startDelay_h = 0;

	params->storm_windUBD = 64;
	params->storm_1dist_ahead = 200;
	params->storm_2dist_ahead = 500;
	params->storm_1costInside_ahead = 1e10;
	params->storm_1cost_ahead = 100;
	params->storm_2cost_ahead = 1;
	params->storm_1dist_behind = 120;
	params->storm_2dist_behind = 200;
	params->storm_1costInside_behind = 1e10;
	params->storm_1cost_behind = 100;
	params->storm_2cost_behind = 1;
	params->startYear = 2018;
	params->startMonth_nr = 9; // sep
	params->startDay_nr = 1;
	params->startHour = 0;


	std::ifstream fil;
	char* namn;
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
	
	json data, dataSpeed;
	fil >> data;
	if (!data["knots_to_km"].is_null())
		params->knots_to_km = data["knots_to_km"];
	if (!data["shipSpeed_average"].is_null())
		params->shipSpeed_average = data["shipSpeed_average"];
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

	if (!data["speedSettings_addOnlyCheapestArcs"].is_null()) {
		errlog("ERROR! Not using speedSettings_addOnlyCheapestArcs\n");
		// params->speedSettings_addOnlyCheapestArcs = data["speedSettings_addOnlyCheapestArcs"];
	}
	if (!data["readSolPathFile"].is_null()) {
		params->readSolPathFile = data["readSolPathFile"];
	}
	if (!data["epsilon"].is_null())
		params->epsilon = data["epsilon"];
	if (!data["save_weatherNodes"].is_null())
		params->save_weatherNodes = data["save_weatherNodes"];
	if (!data["lengthIntervall"].is_null())
		params->lengthIntervall = data["lengthIntervall"];
	if (!data["dist_checkOKroute"].is_null())
		params->dist_checkOKroute = data["dist_checkOKroute"];
	if (!data["variableFileName"].is_null())
		params->variableFileName = data["variableFileName"];
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

	if (!data["ship_speedSettings"].is_null() && !data["speedSettingParameters"].is_null()) {
		// addera vektor...
		dataSpeed = data["ship_speedSettings"];
		params->nShip_speedSettings = (int)dataSpeed.size();
		params->ship_speedSettingID = (char**)malloc(params->nShip_speedSettings * sizeof(char*));
		i = 0;
		for (auto it = dataSpeed.begin(); it != dataSpeed.end(); ++it) {
			std::string dataIt = it.value();
			params->ship_speedSettingID[i] = str_alloc_cpy(dataIt.c_str());
			i++;
		}
		// params->ship_speedSettingNr = (int*)malloc(params->nShip_speedSettings * sizeof(int));
		json dataSetting = data["speedSettingParameters"];
		json dataIt;
		std::string namn, namnBas;
		int i2;
		model.weatherFunctions.nFunctions = 3;
		model.weatherFunctions.funcVal = (double*)malloc(model.weatherFunctions.nFunctions * sizeof(double));
		model.weatherFunctions.param = (double***)calloc(params->nShip_speedSettings, sizeof(double**));


		errlog("ERROR?? Is function to calculate speed given in km/h?? Otherwise fix that.\n");
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

	if (!data["startYear"].is_null())
		params->startYear = data["startYear"];
	if (!data["startMonth_nr"].is_null())
		params->startMonth_nr = data["startMonth_nr"];
	if (!data["startDay_nr"].is_null())
		params->startDay_nr = data["startDay_nr"];
	if (!data["startHour"].is_null())
		params->startHour = data["startHour"];

	fil.close();

	params->weightFuel.base = 1;
	params->weightFuel.lsmgo = 0;
	params->weightFuel.vlsfo = 0;
	params->weightTime = 0;
	params->weightSafety.base = 0;
	params->weightSafety.hurricane = 0;
	params->weightSafety.stability = 0;
	params->weightSafety.lowPressure = 0;
	params->weightSafety.waves = 0;
	params->weightSafety.stability = 0;

	sprintf(namn, "%s/obj_weights.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	printf("opens %s\n", namn);
	fil.open(namn);
	fil >> data;

	json dataIt, dataIt2;
	if (!data["weight_time"].is_null()) {
		dataIt = data["weight_time"];
		if (dataIt["useWeight"] == 1)
			params->weightTime = dataIt["weight"];
	}
	if (!data["weight_fuel"].is_null()) {
		json data3 = data["weight_fuel"];
		if (!data3["base"].is_null()) {
			dataIt2 = data3["base"];
			if (dataIt2["useWeight"] == 1)
				params->weightFuel.base = dataIt2["weight"];
		}
		if (!data3["VLSFO"].is_null()) {
			dataIt2 = data3["VLSFO"];
			if (dataIt2["useWeight"] == 1)
				params->weightFuel.vlsfo = dataIt2["weight"];
		}
		if (!data3["LSMGO"].is_null()) {
			dataIt2 = data3["LSMGO"];
			if (dataIt2["useWeight"] == 1)
				params->weightFuel.lsmgo = dataIt2["weight"];
		}
	}
	if (!data["weight_safety"].is_null()) {
		json data3 = data["weight_safety"];
		if (!data3["base"].is_null()) {
			dataIt2 = data3["base"];
			if (dataIt2["useWeight"] == 1)
				params->weightSafety.base = dataIt2["weight"];
		}
		if (!data3["hurricane"].is_null()) {
			dataIt2 = data3["hurricane"];
			if (dataIt2["useWeight"] == 1)
				params->weightSafety.hurricane = dataIt2["weight"];
		}
		if (!data3["lowPressure"].is_null()) {
			dataIt2 = data3["lowPressure"];
			if (dataIt2["useWeight"] == 1)
				params->weightSafety.lowPressure = dataIt2["weight"];
		}
		if (!data3["waves"].is_null()) {
			dataIt2 = data3["waves"];
			if (dataIt2["useWeight"] == 1)
				params->weightSafety.waves = dataIt2["weight"];
		}
		if (!data3["stability"].is_null()) {
			dataIt2 = data3["stability"];
			if (dataIt2["useWeight"] == 1)
				params->weightSafety.stability = dataIt2["weight"];
		}
	}
	fil.close();

	std::ifstream fil2;
	sprintf(namn, "%s/qgis_params.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	printf("opens %s\n", namn);
	fil2.open(namn);
	fil2 >> data;
	string testString;

	if (!data["solutionFileName"].is_null())
		params->solutionFileName = data["solutionFileName"];
	if (!data["preferredPath"].is_null()) {
		params->preferredPath = data["preferredPath"];
		size_t i = params->preferredPath.rfind('.', params->preferredPath.length());
		// check that the extension is .geojson
		if (i != std::string::npos) {
			testString = params->preferredPath.substr(i + 1, i + 3);
			if (params->preferredPath.substr(i + 1, i + 3) != "geojson") {
				errlog("ERROR! The prefered path must be given as a geojson (.geojson). I quit.\n");
				exit(0);
			}
			// params->preferredPath = params->preferredPath.substr(0, params->preferredPath.length() - 4);
		}
		else {
			errlog("ERROR! The prefered path must be given as a geojson (.geojson). I quit.\n");
			exit(0);
		}
	}
	if (!data["corridorPath"].is_null()) {
		params->corridorPath = data["corridorPath"];
		size_t i = params->corridorPath.rfind('.', params->corridorPath.length());
		// check that the extension is .geojson
		if (i != std::string::npos) {
			if (params->corridorPath.substr(i + 1, i + 3) != "geojson") {
				errlog("ERROR! The corridor path must be given as a geojson (.geojson). I quit.\n");
				exit(0);
			}
			//params->corridorPath = params->corridorPath.substr(0, params->corridorPath.length() - 4);
		}
		else {
			errlog("ERROR! The corridor path must be given as a geojson (.geojson). I quit.\n");
			exit(0);
		}
	}
	if (!data["preferredPath_followExactOK"].is_null())
		params->preferredPath_followExactOK = data["preferredPath_followExactOK"];
	if (!data["editCorridor"].is_null())
		params->runAlt = data["editCorridor"];
	if (!data["startDelay_h"].is_null()) {
		params->startDelay_h = data["startDelay_h"];
	}
	if (!data["from"].is_null())
		params->fromHarbour = data["from"];
	if (!data["to"].is_null())
		params->toHarbour = data["to"];
	if (!data["type"].is_null())
		params->type = data["type"];

	if (!data["channelsName"].is_null()) {
		params->channelsName = data["channelsName"];
		size_t i = params->channelsName.rfind('.', params->preferredPath.length());
		// check that the extension is .geojson
		if (i != std::string::npos) {
			if (params->channelsName.substr(i + 1, i + 3) != "geojson") {
				errlog("ERROR! The channel paths must be given in a geojson (.geojson). I quit.\n");
				exit(0);
			}
			//params->channelsName = params->channelsName.substr(0, params->channelsName.length() - 4);
		}
		else {
			errlog("ERROR! The channel paths must be given as a geojson (.geojson). I quit.\n");
			exit(0);
		}
	}
	fil2.close();

	errlog("objective weights:\n\ttime: %.2lf\n\tfuel: %.2lf\n\tsafety: %.2lf\n",
		params->weightTime, params->weightFuel.base, params->weightSafety.base);
	return 0;
}

int loadParams_theRestOld(strParams* params)
{
	int i, vardeInt;

	std::ifstream fil;
	char* namn;
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
	
	json data, dataSpeed;
	fil >> data;
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

	if (!data["speedSettings_addOnlyCheapestArcs"].is_null()) {
		errlog("ERROR! Not using speedSettings_addOnlyCheapestArcs\n");
		// params->speedSettings_addOnlyCheapestArcs = data["speedSettings_addOnlyCheapestArcs"];
	}
	if (!data["readSolPathFile"].is_null()) {
		params->readSolPathFile = data["readSolPathFile"];
	}
	if (!data["epsilon"].is_null())
		params->epsilon = data["epsilon"];
	if (!data["save_weatherNodes"].is_null())
		params->save_weatherNodes = data["save_weatherNodes"];
	if (!data["lengthIntervall"].is_null())
		params->lengthIntervall = data["lengthIntervall"];
	if (!data["dist_checkOKroute"].is_null())
		params->dist_checkOKroute = data["dist_checkOKroute"];
	if (!data["variableFileName"].is_null())
		params->variableFileName = data["variableFileName"];
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


		errlog("ERROR?? Is function to calculate speed given in km/h?? Otherwise fix that.\n");
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


	fil.close();

	std::ifstream fil2;
	sprintf(namn, "%s/qgis_params.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	printf("opens %s\n", namn);
	fil2.open(namn);
	fil2 >> data;
	string testString;

	if (!data["solutionFileName"].is_null())
		params->solutionFileName = data["solutionFileName"];
	if (!data["preferredPath"].is_null()) {
		params->preferredPath = data["preferredPath"];
		size_t i = params->preferredPath.rfind('.', params->preferredPath.length());
		// check that the extension is .geojson
		if (i != std::string::npos) {
			testString = params->preferredPath.substr(i + 1, i + 3);
			if (params->preferredPath.substr(i + 1, i + 3) != "geojson") {
				errlog("ERROR! The prefered path must be given as a geojson (.geojson). I quit.\n");
				exit(0);
			}
			// params->preferredPath = params->preferredPath.substr(0, params->preferredPath.length() - 4);
		}
		else {
			errlog("ERROR! The prefered path must be given as a geojson (.geojson). I quit.\n");
			exit(0);
		}
	}
	if (!data["corridorPath"].is_null()) {
		params->corridorPath = data["corridorPath"];
		size_t i = params->corridorPath.rfind('.', params->corridorPath.length());
		// check that the extension is .geojson
		if (i != std::string::npos) {
			if (params->corridorPath.substr(i + 1, i + 3) != "geojson") {
				errlog("ERROR! The corridor path must be given as a geojson (.geojson). I quit.\n");
				exit(0);
			}
			//params->corridorPath = params->corridorPath.substr(0, params->corridorPath.length() - 4);
		}
		else { 
			errlog("ERROR! The corridor path must be given as a geojson (.geojson). I quit.\n");
			exit(0);
		}
	}
	//if (!data["preferredPath_followExactOK"].is_null())
	//	params->preferredPath_followExactOK = data["preferredPath_followExactOK"];
	if (!data["editCorridor"].is_null())
		params->runAlt = data["editCorridor"];
	if (!data["startDelay_h"].is_null()) {
		params->startDelay_h = data["startDelay_h"];
	}
	if (!data["from"].is_null())
		params->fromHarbour = data["from"];
	if (!data["to"].is_null())
		params->toHarbour = data["to"];
	if (!data["type"].is_null())
		params->type = data["type"];

	if (!data["channelsName"].is_null()) {
		params->channelsName = data["channelsName"];
		size_t i = params->channelsName.rfind('.', params->preferredPath.length());
		// check that the extension is .geojson
		if (i != std::string::npos) {
			if (params->channelsName.substr(i + 1, i + 3) != "geojson") {
				errlog("ERROR! The channel paths must be given in a geojson (.geojson). I quit.\n");
				exit(0);
			}
			//params->channelsName = params->channelsName.substr(0, params->channelsName.length() - 4);
		}
		else {
			errlog("ERROR! The channel paths must be given as a geojson (.geojson). I quit.\n");
			exit(0);
		}
	}
	fil2.close();

	double ortoDist = model.params.shipSpeed_average * 1000 / model.params.ortoDist_nPointsPerHour;
	int nPkterOrtoOld = model.params.nPkterOrto;
	model.params.nPkterOrto = 2 * model.params.maxDeviationPrefered_km * 1000 / ortoDist;
	if (2 * model.params.maxDeviationPrefered_km * 1000 / ortoDist > model.params.nPkterOrto)
		(model.params.nPkterOrto)++;
	if (model.params.nPkterOrto % 2 == 0)
		(model.params.nPkterOrto)++;
	if (model.params.nPkterOrto != nPkterOrtoOld)
		errlog("OBS! Changes nPkterOrto from %d to %d beacuse of maxDeviationPrefered_km given as %.2lf km\n",
			nPkterOrtoOld, model.params.nPkterOrto, model.params.maxDeviationPrefered_km);


	errlog("objective weights:\n\ttime: %.2lf\n\tfuel: %.2lf\n\tsafety: %.2lf\n",
		params->weightTime, params->weightFuel.base, params->weightSafety.base);
	return 0;
}

int loadParams_new(strParams* params)
{
	int i;
	double xValOld, yValOld, last_x = -999;

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
	params->nShip_speedSettings = 0; // 15, 20, 25
	params->max_changeDirection = 1;
	params->lengthIntervall = 1;
	params->dist_checkOKroute = 1;
	params->variableFileName = "variablesInfo.json";
	model.params.useStandardWeather = 0;
	params->physicalMap_noDataValue = 9999;
	params->speedSettings_addOnlyCheapestArcs = 1;
	params->runAlt = 0;
	params->startDelay_h = 0;

	params->storm_windUBD = 64;
	params->storm_1dist_ahead = 200;
	params->storm_2dist_ahead = 500;
	params->storm_1costInside_ahead = 1e10;
	params->storm_1cost_ahead = 100;
	params->storm_2cost_ahead = 1;
	params->storm_1dist_behind = 120;
	params->storm_2dist_behind = 200;
	params->storm_1costInside_behind = 1e10;
	params->storm_1cost_behind = 100;
	params->storm_2cost_behind = 1;
	params->startYear = 2018;
	params->startMonth_nr = 9; // sep
	params->startDay_nr = 1;
	params->startHour = 0;
	params->maxDeviationPrefered_km = 500; 


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
	fil >> data;

	if (!data["maxDeviationPrefered_km"].is_null())
		params->maxDeviationPrefered_km = data["maxDeviationPrefered_km"];

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
	if (!data["preferredPath_followExactOK"].is_null())
		params->preferredPath_followExactOK = data["preferredPath_followExactOK"];

	if (!data["shipSpeed"].is_null())
		params->shipSpeed_average = data["shipSpeed"];

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
			if (dataFeature["properties"].is_null())
				continue; // no properties exists for this one, cannot be a preferred path

			dataProp = dataFeature["properties"];
			if (dataProp["type"].is_null())
				continue; // no type exists for this one, cannot be a preferred path

			string namnNu = dataProp["type"];
			if (namnNu != "preferredPath")
				continue; //not a preferred path

			dataGeo3 = dataFeature["geometry"];
			if (dataGeo3["coordinates"].is_null()) {
				errlog("ERROR! No coordinates given for the prefered path. I quit!\n");
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
								}else
									last_x = xVal;
							}else
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

	if (model.preferredPath.minX < -180) {
		model.preferredPath.minX += 360;
		model.preferredPath.maxX += 360;
	}

	params->weightFuel.base = 1;
	params->weightFuel.lsmgo = 0;
	params->weightFuel.vlsfo = 0;
	params->weightTime = 1;
	// strSafety* weightSafety;
	params->weightSafety.base = 1000;
	params->weightSafety.hurricane = 0;
	params->weightSafety.stability = 0;
	params->weightSafety.lowPressure = 0;
	params->weightSafety.waves = 0;
	params->weightSafety.stability = 0;

	params->priceFuel.vlsfo = 500;
	params->priceFuel.lsmgo = 800;
	params->priceTime = 500;

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
			if (!data3["base"].is_null()) {
				dataIt2 = data3["base"];
				if (dataIt2["useWeight"] == 1)
					params->weightFuel.base = (double)(dataIt2["weight"]) / 100;
			}
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
			if (!data3["lowPressure"].is_null()) {
				dataIt2 = data3["lowPressure"];
				params->weightSafety.lowPressure = (double)(dataIt2["weight"]) / 100;
			}
			if (!data3["waves"].is_null()) {
				dataIt2 = data3["waves"];
				params->weightSafety.waves = (double)(dataIt2["weight"]) / 100;
			}
			if (!data3["stability"].is_null()) {
				dataIt2 = data3["stability"];
				params->weightSafety.stability = (double)(dataIt2["weight"]) / 100;
			}
		}
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
	fil >> data;
	
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

int loadVariables()
{
	json data, dataVar, dataIt, dataIt2, dataFiles;
	int i1, i0;
	std::string typeName, namn;

	std::ifstream fil(model.params.indataPath + "/" + model.params.variableFileName);
	if (!fil.is_open()) {
		errlog("ERROR! Could not open the file %s with information about the weather parameters. I quit.\n", model.params.variableFileName.c_str());
		exit(0);
	}
	fil >> data;

	model.nWeatherFiles = 0;
	//nAllocWeather = 5;
	//model.weather = (strWeather*)malloc(nAllocWeather * sizeof(strWeather));

	//std::cout << data.size() << std::endl;
	dataVar = data["weather_parameters"];
	//std::cout << dataVar.size() << std::endl;
	//std::cout << dataVar.dump() << std::endl;
	//model.nVariables = (int)dataVar.size(); //  data.count("variables");
	// model.variable = (strVariables*)malloc(model.nVariables * sizeof(strVariables));
	model.weather = (strWeather*)malloc((int)dataVar.size() * sizeof(strWeather));
	i0 = 0;
	for (auto it = dataVar.begin(); it != dataVar.end(); ++it) {
		dataIt = it.value();
		namn = dataIt["variableID"];
		model.weather[i0].weatherFileTypeName = str_alloc_cpy(namn.c_str());
		model.weather[i0].timeIntervall_h = dataIt["timeIntervall_h"];
		dataFiles = dataIt["files"];
		model.weather[i0].nFiles = (int)dataFiles.size();
		model.weather[i0].filePos = (strFileWeather*)malloc(model.weather[i0].nFiles * sizeof(strFileWeather));
		model.weather[i0].rasterPos = (Raster*)malloc(model.weather[i0].nFiles * sizeof(Raster));
		i1 = 0;
		for (auto it2 = dataFiles.begin(); it2 != dataFiles.end(); ++it2) {
			dataIt2 = it2.value();
			namn = dataIt2["fileName"];
			model.weather[i0].filePos[i1].fileName = str_alloc_cpy(namn.c_str());
			model.weather[i0].filePos[i1].minX = dataIt2["minLon"];
			model.weather[i0].filePos[i1].maxX = dataIt2["maxLon"];
			i1++;
		}
		(model.nWeatherFiles)++;
		i0++;
	}

	return 0;
}

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

	double delta = model.params.maxDeviationPrefered_km / 120; // max antal grader
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

int roundUp(double varde) {
	int heltal = (int)varde;
	if (heltal < varde)
		heltal++;
	return heltal;
}

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
	errlog("tot haversine dist of prefered path %lf\n", distTot);

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
	writePointsToGeojson((char*)"intervallPoints", intervallPoint, nIntervallPoints);

	ortoDist = model.params.shipSpeed_average * 1000 / model.params.ortoDist_nPointsPerHour;
	FILE* filtmp;
	char* namn;
	namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/tmpCheck.txt", model.params.resultPath.c_str());
	filtmp = fopen(namn, "w");
	model.params.preferredPathOrtoPos = (int*)malloc(nIntervallPoints * sizeof(int));

	for (i = 0; i < nIntervallPoints; i++) {
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

			if (model.corridorPath.nLines > 0) {
				auto bearing2 = model.network.physicalLev[i].point[0].bearingTo(model.network.physicalLev[i].point[nPkterOrto - 1]);
				for (i1 = 0; i1 < model.corridorPath.nLines; i1++) {
					for (i2 = 0; i2 < model.corridorPath.nPoints[i1] - 1; i2++) {
						if (i == 29)
							i = i;
						distNu = model.corridorPath.point[i1][i2].distanceTo(model.network.physicalLev[i].point[0]);
						distNu1 = model.corridorPath.point[i1][i2].distanceTo(model.network.physicalLev[i].point[nPkterOrto - 1]);
						distNu2 = model.corridorPath.point[i1][i2 + 1].distanceTo(model.network.physicalLev[i].point[0]);
						distNu3 = model.corridorPath.point[i1][i2 + 1].distanceTo(model.network.physicalLev[i].point[nPkterOrto - 1]);
						if (distNu < 1 || distNu1 < 1 || distNu2 < 1 || distNu3 < 1)
							continue; // point so close to boarder
						auto bearing1 = model.corridorPath.point[i1][i2].bearingTo(model.corridorPath.point[i1][i2 + 1]);
						pointNu = spherical::Point::intersection(model.corridorPath.point[i1][i2], bearing1,
							model.network.physicalLev[i].point[0], bearing2);
						fprintf(filtmp, "i %d i1 %d i2 %d fromTo corr points %.3lf %.3lf %.3lf %.3lf fromTo ortoPoints %.3lf %.3lf %.3lf %.3lf crossingPoint coords %.3lf %.3lf bearingCorridor %.2lf bearingOrto %.2lf pointValid %d\n", i, i1, i2,
							model.corridorPath.point[i1][i2].latitude().degrees(), model.corridorPath.point[i1][i2].longitude().degrees(),
							model.corridorPath.point[i1][i2 + 1].latitude().degrees(), model.corridorPath.point[i1][i2 + 1].longitude().degrees(),
							model.network.physicalLev[i].point[0].latitude().degrees(), model.network.physicalLev[i].point[0].longitude().degrees(),
							model.network.physicalLev[i].point[nPkterOrto - 1].latitude().degrees(), model.network.physicalLev[i].point[nPkterOrto - 1].longitude().degrees(),
							pointNu.latitude().degrees(), pointNu.longitude().degrees(), bearing1, bearing2, pointNu.isValid());
						if (pointNu.isValid()) {
							distCorridorSegm = model.corridorPath.point[i1][i2].distanceTo(model.corridorPath.point[i1][i2 + 1]);
							distIntersectCorridor = model.corridorPath.point[i1][i2].distanceTo(pointNu);
							if (distIntersectCorridor > distCorridorSegm * 1.00001)
								continue; // skarningspunkten ar utanfor korridorssegmentet
							distIntersect = model.network.physicalLev[i].point[0].distanceTo(pointNu);
							distPrefPath = model.network.physicalLev[i].point[0].distanceTo(model.network.physicalLev[i].point[(int)(nPkterOrto / 2)]);
							fprintf(filtmp, "inside distIntersect0 %.3lf distPrefPath %.3lf\n", distIntersect, distPrefPath);
							if (distIntersect < distPrefPath) {
								for (i3 = 0; i3 < nPkterOrto / 2; i3++) {
									distNu = model.network.physicalLev[i].point[0].distanceTo(model.network.physicalLev[i].point[i3]);
									if (distNu < distIntersect)
										model.network.physicalLev[i].allowedPoint[i3] = 0;
									else
										break;
								}
							}
							else {
								for (i3 = nPkterOrto - 1; i3 > nPkterOrto / 2; i3--) {
									distNu = model.network.physicalLev[i].point[0].distanceTo(model.network.physicalLev[i].point[i3]);
									if (distNu > distIntersect)
										model.network.physicalLev[i].allowedPoint[i3] = 0;
									else
										break;
								}
							}


						}
					}
				}
			}

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
	double totDist, dist, distHittils, bearing, rowDbl, colDbl;
	spherical::Point pMid, pTmp;

	totDist = p1.distanceTo(p2) / 1000;
	dist = model.params.shipSpeed_average;
	distHittils = 0;
	pMid = p1;
	bearing = pMid.bearingTo(p2);
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
	bearing = pMid.bearingTo(model.network.channel[cNr].point[posLast + 1]);
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
	bearing = pMid.bearingTo(model.network.physicalLev[level].preferredPathPoint[posLast]);
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
	int tidInt = (int)(tidpkt / model.weather[varNr].timeIntervall_h);
	if (tidInt >= model.weather[varNr].nTimeIntervals || model.weather[varNr].useStandardWeather == 1 ||
		model.weather[varNr].useStandardWeather == -1) {
		if (model.weather[varNr].useStandardWeather == 0)
			errlog("ERROR! Too late time interval %d for weatherfile %d (latest %d). Implement a standard weather for the season and use, I use the last one now...\n",
				tidInt, varNr, model.weather[varNr].nTimeIntervals - 1);
		if (model.weather[varNr].useStandardWeather == -1)
			tidInt = 0;
		else {
			model.weather[varNr].useStandardWeather = 1;
			tidInt = model.weather[varNr].nTimeIntervals - 1;
		}
	}

	//return model.weather[weatherNr].rasterBandData[timePos][latPos][lonPos];
	return model.weather[varNr].valueCell[tidInt][latPos * model.weather[varNr].nCols + lonPos];
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

double getStormValue(int t, spherical::Point point)
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

double calcTimeCost(int t, int speedSettingNr, int determineWeatherPos, double* fuel, double* safety, double* distance, double* worstStormValue, double* worstStabilityValue)
{
	double tidTot = t, distNu, tidTmp, costTot = 0, safetyTot = 0;
	double uWind, vWind, uCurrent, vCurrent, uVessel, vVessel; //  , uSpeed, vSpeed;
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
		uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
		vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

		stormVarde = getStormValue(t, model.weatherFunctions.point[i]);
		if (stormVarde > *worstStormValue)
			*worstStormValue = stormVarde;


		uWind = getVariableValue(0, i, tidTot);
		vWind = getVariableValue(1, i, tidTot);
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
		uCurrent = getVariableValue(2, i, tidTot);
		vCurrent = getVariableValue(3, i, tidTot);
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

double eval_safety(double iceCover) {

	if (iceCover > model.functions.iceCoverMaxFree)
		return model.functions.iceCoverCost_fix;
	else
		return 0.0;
}

double eval_baseGroundSpeed(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed) {
	double rel_currentDir, speed, B, C, sinA;

	// basic variant, not correct
	// rel_currentDir = currentDir * 180.0 / M_PI - bearing;
	// speed_y = calmWaterSpeed + currentSpeed * cos(rel_currentDir);
	// speed_x = currentSpeed * sin(rel_currentDir);
	// speed = sqrt(speed_y * speed_y - speed_x * speed_x);

	rel_currentDir = abs(currentDir - bearing / 180.0 * M_PI);
	if (rel_currentDir > M_PI)
		rel_currentDir = 2 * M_PI - rel_currentDir;
	sinA = sin(rel_currentDir);
	if (sinA < 0.01)
		speed = calmWaterSpeed + currentSpeed * cos(rel_currentDir);
	else {
		B = asin(currentSpeed * sinA / calmWaterSpeed);
		C = M_PI - B - rel_currentDir;
		speed = calmWaterSpeed * sin(C) / sinA;
	}
	return speed;
}

double eval_relWindSpeed(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir) {
	double rel_speed, x, y;

	windDir = (windDir - (180 + bearing) / 180.0 * M_PI); // in radians relative to ship bearing
	x = windSpeed * cos(windDir) - baseGroundSpeed;
	y = sin(windSpeed);
	*rel_windDir = atan2(y, x);
	rel_speed = sqrt(x * x + y * y);
	return rel_speed;
}

int get_relWindDirIndex(double rel_windDir) { // windDir: [-pi, +pi]

	rel_windDir = M_PI - rel_windDir; // need it in opposite direction
	if (rel_windDir < 0)
		rel_windDir = -rel_windDir; // windDir [0, +pi]
	int index = (int)rel_windDir * model.functions.nWindDir / M_PI;// 180.0;

	if (index < 0)
		index = 0;
	else {
		if (index >= model.functions.nWindDir)
			index = model.functions.nWindDir - 1;
	}
	return index;
}

int get_relWaveDirIndex(double rel_waveDir) { // windDir: [-pi, +pi]

	rel_waveDir = M_PI - rel_waveDir; // need it in opposite direction
	if (rel_waveDir < -M_PI)
		rel_waveDir += 2 * M_PI;
	if (rel_waveDir > 2 * M_PI)
		rel_waveDir -= 2 * M_PI;
	if (rel_waveDir < 0)
		rel_waveDir = -rel_waveDir; // windDir [0, +pi]
	int index = (int)rel_waveDir * model.functions.nWaveDir / M_PI;// 180.0;

	if (index < 0)
		index = 0;
	else {
		if (index >= model.functions.nWaveDir)
			index = model.functions.nWaveDir - 1;
	}
	return index;
}

int get_relWindSpeedIndex(double rel_windSpeed) { 

	int tmp = (int)rel_windSpeed * model.functions.rel_windSpeed_kvotIndex;
	if (tmp >= model.functions.max_windSpeedSkalad)
		return model.functions.rel_windSpeed_ger_index[model.functions.max_windSpeedSkalad - 1];
	else
		return model.functions.rel_windSpeed_ger_index[tmp];
}

int get_relWaveHeightIndex(double waveHeight) {
	int tmp = (int)waveHeight * model.functions.rel_waveHeight_kvotIndex;
	if (tmp >= model.functions.max_waveHeightSkalad)
		return model.functions.rel_waveHeight_ger_index[model.functions.max_waveHeightSkalad - 1];
	else
		return model.functions.rel_waveHeight_ger_index[tmp];
}

int get_relWavePeriodIndex(double wavePeriod) {
	int tmp = (int)wavePeriod * model.functions.rel_wavePeriod_kvotIndex;
	if (tmp >= model.functions.max_wavePeriodSkalad)
		return model.functions.rel_wavePeriod_ger_index[model.functions.max_wavePeriodSkalad - 1];
	else
		return model.functions.rel_wavePeriod_ger_index[tmp];
}

double lookup_speedDiffWindWaveTable(double rel_windSpeed, double rel_windDir, double waveHeight, double wavePeriod, double rel_waveDir) {
	int iWindDir = get_relWindDirIndex(rel_windDir);
	int iWaveDir = get_relWaveDirIndex(rel_waveDir);
	int iWindSpeed = get_relWindSpeedIndex(rel_windDir);
	int iWaveHeight = get_relWaveHeightIndex(waveHeight); // / model.functions.waveHeightDiscr);
	int iWavePeriod = get_relWavePeriodIndex(wavePeriod); // / model.functions.wavePeriodDiscr);
	int iWave, pos;

	pos = iWaveDir + model.functions.table_niWaveDir * (iWavePeriod + model.functions.table_niWavePeriod * (
		iWave + model.functions.table_niWave * (
		iWindDir + model.functions.table_niWindDir * iWindSpeed)));
	return model.functions.table_speedDiff[pos]; // windSpeed, windDir, wave, wavePeriod, waveDir

}

double calcArcTimeCost(int t, int speedSettingNr, int determineWeatherPos, double* fuel, double* safety, double* distance, double* worstStormValue, double* worstStabilityValue)
{
	double tidTot = t, distNu, fuelTot = 0, safetyTot = 0;
	double uWind, vWind, uCurrent, vCurrent, uVessel, vVessel; //  , uSpeed, vSpeed;
	double dist = 0;
	double windDirection, windSpeed;
	double currentDirection, currentSpeed, rel_windSpeed;
	int i;
	double waveHeight, wavePeriod, stormVarde, windSpeed2, waveDirection;
	double calmWaterSpeed, baseGroundSpeed, rel_windDir, rel_waveDir, speedDiffWindWave;
	double speedOverGround, timeArc, fuelConsumption, fuelUsage, iceCover, safetyArc;

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
	*worstStormValue = 0;
	*worstStabilityValue = 0;
	for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
		distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
		dist += distNu;

		calmWaterSpeed = eval_calmWaterSpeed(speedSettingNr);

		uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
		vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

		stormVarde = getStormValue(t, model.weatherFunctions.point[i]);
		if (stormVarde > *worstStormValue)
			*worstStormValue = stormVarde;

		uCurrent = getVariableValue(2, i, tidTot);
		vCurrent = getVariableValue(3, i, tidTot);
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

		uWind = getVariableValue(0, i, tidTot);
		vWind = getVariableValue(1, i, tidTot);
		if (uWind < 1000 && vWind < 1000) {
			windDirection = atan2(vWind, uWind);
			windSpeed2 = pow(uWind, 2) + pow(vWind, 2);
			windSpeed = sqrt(windSpeed2);

			rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
				windDirection, windSpeed, &rel_windDir);

			*worstStabilityValue += distNu * rel_windSpeed / 10000.0;
		}
		else {
			windSpeed = 0;
			rel_windDir = 0;
		}

		waveHeight = getVariableValue(4, i, tidTot);
		if (waveHeight > 100)
			waveHeight = 0;
		wavePeriod = getVariableValue(5, i, tidTot);
		if (wavePeriod > 1000)
			wavePeriod = 0;
		waveDirection = getVariableValue(6, i, tidTot);
		if (waveDirection > 1000)
			waveDirection = 0;
		rel_waveDir = (waveDirection - model.weatherFunctions.vesselBearing[i] / 180.0 * M_PI); // / model.functions.nWaveDir;


		speedDiffWindWave = lookup_speedDiffWindWaveTable(rel_windSpeed, rel_windDir, waveHeight, wavePeriod, rel_waveDir);
		speedOverGround = baseGroundSpeed + speedDiffWindWave;
		timeArc = distNu / speedOverGround / model.params.knots_to_km;

		fuelConsumption = eval_fuelConsumption(speedSettingNr);
		fuelUsage = fuelConsumption * timeArc;

		tidTot += timeArc;
		fuelTot += fuelUsage;

		iceCover = getVariableValue(7, i, tidTot);
		if (iceCover > 1000)
			iceCover = 0;

		safetyArc = eval_safety(iceCover);
		safetyTot += safetyArc;
	}
	*distance = dist;
	*fuel = fuelTot;
	*safety = safetyTot;
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
	double tid, safety, fuel, distance, totCost, channelCost;
	double fuelVLSFO, fuelLSMGO, fuelBase, safetyBase, worstStormValue = 0;
	double worstStabilityValue = 0;

	model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
	if (thisLevel >= 0 && nextLevel >= 0) {
		if (pos1 == model.params.preferredPathOrtoPos[thisLevel] && pos2 == model.params.preferredPathOrtoPos[nextLevel] && thisLevel == nextLevel - 1)
			prefPath = 1;
	}

	for (i4 = 0; i4 < model.params.nShip_speedSettings; i4++) {
		if (*setupCheckPoints == 1) {
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

			//errlog("from %.2lf %.2lf to %.2lf %.2lf\n",
			//	model.network.physicalLev[i - 1].point[i1].latitude().degrees(),
			//	model.network.physicalLev[i - 1].point[i1].longitude().degrees(),
			//	model.network.physicalLev[i].point[i2].latitude().degrees(),
			//	model.network.physicalLev[i].point[i2].longitude().degrees());
			*setupCheckPoints = 0;
		}
		if (thisLevel == 12 && model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] == 107)
			tPos = tPos;
		if (thisLevel >= 0) {
			nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
			tid = calcArcTimeCost(model.network.physicalLev[thisLevel].timeInterval[pos1][tPos],
				i4, *setupCheckPoints, &fuel, &safety, &distance, &worstStormValue, &worstStabilityValue);
			tidInt = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] + (int)tid;
		}
		else {
			nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
			tid = calcArcTimeCost(model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos],
				i4, *setupCheckPoints, &fuel, &safety, &distance, &worstStormValue, &worstStabilityValue);
			tidInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos] + (int)tid;
		}



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
			fuelVLSFO = fuel * (1 - fuelQualityKvot);
			fuelLSMGO = fuel * fuelQualityKvot;
			fuelBase = fuelVLSFO * model.params.weightFuel.vlsfo * model.params.priceFuel.vlsfo + fuelLSMGO * model.params.weightFuel.lsmgo * model.params.priceFuel.lsmgo;

			safety = worstStormValue * model.params.weightSafety.hurricane + worstStabilityValue * model.params.weightSafety.stability;
			safetyBase = safety;

			totCost += model.params.weightTime * model.params.priceTime * tid +
				model.params.weightFuel.base * fuelBase + model.params.weightSafety.base * safety;
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
				model.arc[posNy].distance = distance;
				model.arc[posNy].safetyHurricane = worstStormValue;
				model.arc[posNy].safetyStability = worstStabilityValue;
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
				model.arc[arcNr].distance = distance;
				model.arc[arcNr].fuelBase = fuelBase;
				model.arc[arcNr].fuelVLSFO = fuelVLSFO;
				model.arc[arcNr].fuelLSMGO = fuelLSMGO;
				model.arc[arcNr].safetyHurricane = worstStormValue;
				model.arc[arcNr].safetyStability = worstStabilityValue;
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
	double tid, safety, fuel, distance, totCost;
	double fuelVLSFO, fuelLSMGO, fuelBase, safetyBase;
	double worstStormValue, worstStabilityValue;

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
			i4, 1, &fuel, &safety, &distance, &worstStormValue, &worstStabilityValue);
		tidInt = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] + (int)tid;
	}
	else {
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
		tid = calcArcTimeCost(model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos],
			i4, 1, &fuel, &safety, &distance, &worstStormValue, &worstStabilityValue);
		tidInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos] + (int)tid;
	}

	if (thisLevel < 0 && nextLevel < 0) {
		tid += model.network.channel[-thisLevel - 1].extraTimeChannel;
		totCost = model.network.channel[-thisLevel - 1].extraCostChannel;
	}
	else
		totCost = 0;

	fuelVLSFO = fuel * (1 - fuelQualityKvot);
	fuelLSMGO = fuel * fuelQualityKvot;
	fuelBase = fuelVLSFO * model.params.weightFuel.vlsfo * model.params.priceFuel.vlsfo + fuelLSMGO * model.params.weightFuel.lsmgo * model.params.priceFuel.lsmgo;

	safety = worstStormValue * model.params.weightSafety.hurricane + worstStabilityValue * model.params.weightSafety.stability;
	safetyBase = safety;

	totCost += model.params.weightTime * model.params.priceTime * tid +
		model.params.weightFuel.base * fuelBase + model.params.weightSafety.base * safetyBase;
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
		model.arc[arcNr].distance = distance;
		model.arc[arcNr].fuelBase = fuelBase;
		model.arc[arcNr].fuelVLSFO = fuelVLSFO;
		model.arc[arcNr].fuelLSMGO = fuelLSMGO;
		model.arc[arcNr].safetyHurricane = worstStormValue;
		model.arc[arcNr].safetyStability = worstStabilityValue;
		model.arc[arcNr].safetyBase = safetyBase;
		model.arc[arcNr].totCost = totCost;
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
	errlog("test1\n");
	//model.weatherFunctions.lastFileNr = (int*)calloc(model.nWeatherFiles, sizeof(int));
	//errlog("test11\n");

	model.nAllocArcs = 100000;
	model.arc = (strArcInfo*)malloc(model.nAllocArcs * sizeof(strArcInfo));

	errlog("test12\n");
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
	errlog("test13\n");

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

	float***  dataWeatherFile;
	dataWeatherFile = (float***)malloc(model.nWeatherFiles * sizeof(float**));

	int latPos, lonPos, xPos0, xPos1, yPos0, yPos1, nBands;
	double lat, lon, size_col, size_row, xPosFrac, yPosFrac;

	errlog("test14\n");
	for(int ii = 0; ii < model.nWeatherFiles; ii++){
		model.tmpTid[0] = std::chrono::high_resolution_clock::now();
		printf("weather %d variable %s ", ii, model.weather[ii].weatherFileTypeName);
		if (ii == 3)
			ii = ii;
		// identifiera vilka raster som behover oppnas, och oppna dem
		size_col = -1;
		for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			if (check_useRaster_longitude(ii, i1) == 1){
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
				model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]));

			}
		}

		//testCoordValue(ii, 167.6111, 81);
		//testCoordValue(ii, 179.6111, 82);
		//testCoordValue(ii, -179.6111, 83);
		//testCoordValue(ii, -132.39, 84);


		// om olika diskretization pa oppnade raster sa stoppa
		// 



		//model.weather[ii].valueCell = model.weather[ii].rasterPos.GetRasterBand_realArrAllBands(&(model.weather[ii].raster), model.boundingBox);
		//printf("used dim %d %d tid %lf nBands %d\n", model.weather[ii].nRows,
		//	model.weather[ii].nCols, model.durationMilli[ii], model.weather[ii].rasterPos.Get_nBands());


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
					lat = model.weather[ii].maxY - (latPos + 0.5) * model.weather[ii].size_row;
					lon = (lonPos + 0.5) * model.weather[ii].size_col + model.weather[ii].minX;
					if (model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5] < -10000 ||
						model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5] > 10000) {
						errlog("ERROR! value of weather %d %s band %d lon/lat %.3lf %.3lf is %lf\n", ii, model.weather[ii].weatherFileTypeName,
							ii3 + 1, lon, lat, model.weather[ii].valueCell[ii3][ii4 * model.weather[ii].nCols + ii5]);
					}
				}
			}
		}
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
	//printf("all done\n");
	//exit(0);
	i = i;



	

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
		max_t = min_t_nu + model.params.maxDiffTimeFastSlow;
		min_t_nu = 99999;
		max_t_nu = 0;
		n_added_t = 0;
		nArcsNu = 0;
		model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
		tid1 = std::chrono::high_resolution_clock::now();
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			fprintf(filpek11, "lev %d point %d nOutNodes %d nTimeInt %d\n", i, i1,
				model.network.physicalLev[i].nOutNodes[i1], model.network.physicalLev[i].nTimeIntervals[i1]);
			model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
			if (model.network.physicalLev[i].nArcsToPoint[i1] == 0 && i > 0)
				continue; // no arc to this point so no use to add arcs out
			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];

				if (nextLevel > i + 1)
					i = i;
				setupCheckPoints = 1;
				fuelQualityKvot = get_fuelQualityKvot(i, i1, nextLevel, i2);
				for (i3 = 0; i3 < model.network.physicalLev[i].nTimeIntervals[i1]; i3++) {
					if (i == 11 && i1 == 42 && nextLevel == 12 && model.network.physicalLev[i].outNode[i1][i2b] == 40 &&
						model.network.physicalLev[i].timeInterval[i1][i3] == 64)
						i = i;
					//freeMemory();

					model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
					model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
					nArcsNu += checkAddBagar_AB(i, i1, nextLevel, i2, i3, &setupCheckPoints, max_t, &min_t_nu, &max_t_nu, fuelQualityKvot);
					model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
				}
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
		model.arc[arcNr].safetyStability = 0;
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

int voyageOpt_old(string inputPath)
{

	double dist;
	long long Cost;
	reset_errlog();
	//callRaster();

	FILE* filPek3;
	filPek3 = fopen("result_json.json", "w");
	fprintf(filPek3, "{\n\t\"solutionShape\": \"ERROR\"\n}\n");
	fclose(filPek3);

	model.params.indataPath = inputPath;
	//model.params.resultPath = resultPath;

	printf("Reading data for the problem\n");
	loadParams(&(model.params));

	int testOpenMultipleTimes = 0;
	if (testOpenMultipleTimes == 1) {
		test_OpenTheSameRasterMultipleTimesAndRead();
		exit(0);
	}

	if (model.params.runAlt == 1) {
		loadpreferredPathGeojson();
		createPhysicalNetwork(1);
		exit(0);
	}

	loadVariables();
	// loadWeatherData();
	loadStormsData();

	loadChannels();
	loadpreferredPathGeojson();
	if (model.params.corridorPath != "")
		loadCorridorPath();
	else
		model.corridorPath.nLines = 0;

	printf("Creating the physical network.\n");
	createPhysicalNetwork(0);


	printf("Creating the time dimension.\n");
	createTimeArcs();

	//printf("saveDijkstraData..");
	//saveDijkstraData();
	//printf("..\n");

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
	char* namn;
	namn = (char*)malloc(265 * sizeof(char));

	printf("solving dijkstra's algorithm..");
	AnropDijkstra2(nod1, nod2, &model, &Reached);
	auto tid1c2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms2 = tid1c2 - tid0;
	errlog("after Dijkstra %lf\n", fp_ms2);
	printf("..done.\nSaving solution.\n");
	if (Reached == true) {
		model.BVArc = (int*)malloc(model.nNoder * sizeof(int));
		model.BVtempNodOrder = (int*)malloc(model.nNoder * sizeof(int));
		dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
		sprintf(namn, "%s/%s", resultPath.c_str(), model.params.solutionFileName.c_str());
		//writeSolutionPathToShape(namn, 0);
		writeSolutionPathToGeoJson(namn, 0);

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
	//printf("All done. I quit.\n");
	//auto tid1c3 = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double, std::milli> fp_ms3 = tid1c3 - tid0;
	//errlog("all done %lf\n", fp_ms3);


	//callJsonTest();
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

	model.params.resultPath = splitFilename(resultName);

	sprintf(namn, "%s/result_json.json", model.params.resultPath.c_str());
	filPek3 = fopen(namn, "w");
	fprintf(filPek3, "{\n\t\"solutionShape\": \"ERROR\"\n}\n");
	fclose(filPek3);

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);

	//model.params.resultPath = resultPath;

	printf("Reading data for the problem\n");
	loadParams_new(&(model.params));
	loadParams_theRestOld(&(model.params));

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
	// loadWeatherData();
	loadStormsData();

	loadChannels();
	//loadpreferredPathGeojson();
	if (model.params.corridorPath != "")
		loadCorridorPath();
	else
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

	int nExtraOpt = 6;
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

			printf("Saving solution path\n");
			if (ii == 0)
				writeSolutionToJson(resultName, 0);
			else {
				resAltName = resultPath;
				resAltName.append("resObjAlt_");
				resAltName += to_string(ii);
				resAltName.append(".json");
				writeSolutionToJson(resAltName, 0);
			}

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

	printf("Saving solution path\n");
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
	if (model.corridorPath.nLines > 0) {
		auto bearing2 = pN.bearingTo(pC);
		for (i1 = 0; i1 < model.corridorPath.nLines; i1++) {
			for (i2 = 0; i2 < model.corridorPath.nPoints[i1] - 1; i2++) {
				if (i == 29)
					i = i;
				distNu1 = model.corridorPath.point[i1][i2].distanceTo(pC);
				distNu3 = model.corridorPath.point[i1][i2 + 1].distanceTo(pC);
				if (distNu1 < 1 || distNu3 < 1)
					continue; // point so close to boarder
				auto bearing1 = model.corridorPath.point[i1][i2].bearingTo(model.corridorPath.point[i1][i2 + 1]);
				pointNu = spherical::Point::intersection(model.corridorPath.point[i1][i2], bearing1,
					pN, bearing2);
				//fprintf(filtmp, "i %d i1 %d i2 %d fromTo corr points %.3lf %.3lf %.3lf %.3lf fromTo pN-pC %.3lf %.3lf %.3lf %.3lf crossingPoint coords %.3lf %.3lf bearingCorridor %.2lf bearingOrto %.2lf pointValid %d\n", i, i1, i2,
				//	model.corridorPath.point[i1][i2].latitude().degrees(), model.corridorPath.point[i1][i2].longitude().degrees(),
				//	model.corridorPath.point[i1][i2 + 1].latitude().degrees(), model.corridorPath.point[i1][i2 + 1].longitude().degrees(),
				//	pN.latitude().degrees(), pN.longitude().degrees(),
				//	pC.latitude().degrees(), pC.longitude().degrees(),
				//	pointNu.latitude().degrees(), pointNu.longitude().degrees(), bearing1, bearing2, pointNu.isValid());
				if (pointNu.isValid()) {
					distCorridorSegm = model.corridorPath.point[i1][i2].distanceTo(model.corridorPath.point[i1][i2 + 1]);
					distIntersectCorridor = model.corridorPath.point[i1][i2].distanceTo(pointNu);
					if (distIntersectCorridor > distCorridorSegm * 1.00001)
						continue; // skarningspunkten ar utanfor korridorssegmentet
					distIntersect = pN.distanceTo(pointNu);
					distPrefPath = pN.distanceTo(pC);
					//fprintf(filtmp, "inside distCorridorSegm %.2lf distIntersectCorridor %.2lf distIntersect1 %.3lf distPrefPath %.3lf\n", 
					//	distCorridorSegm, distIntersectCorridor, distIntersect, distPrefPath);
					if (distIntersect < distPrefPath) {
						//fprintf(filtmp, "\nOBS corridor cuts path\n");
						return 0; // skar en corridor pa vag fran prefPath till Channel
					}
				}
			}
		}
	}
	return 1;
}