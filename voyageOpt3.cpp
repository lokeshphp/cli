

extern int USE_KVOTKOST;
extern int USE_KVOTCOST_CORRIDORS;
extern double DEFAULT_KVOTMINCOST_TSS;
extern double DEFAULT_KVOTMINCOST_CORRIDORS;
extern double DEFAULT_TSS_ATTACTIONDISTANCE;
extern double MAX_FAKTOR_NATVERK;

#include "pch.h"
#include <cstdio>
#include <iostream>
#include<fstream>
//#include "stdlib.h"
//#include "string.h"
#include <cmath>
//#include"vector_gdal.cpp"
//#include "json.hpp"
//#include "ogrsf_frmts.h"
#include <cstring>
//#include <boost/iostreams/filter/zlib.hpp>
#include <zlib.h>
//#include <filesystem>
//#include<iomanip>

#include "redisDef.h"
#include <sstream>
#include <time.h>

#include <sqlite3.h> 
//#include <sstream>
//#include<iomanip>

#ifdef _WIN32
double M_PI_2 = M_PI / 2;
#endif

using json = nlohmann::json;

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;

strModel model;
strModel modelDelay;
strModel modelDelay_prefPath;

extern std::string resultPath;
extern long long MAXVARDE_NATVERK;
extern int SKRIV_UT_NOTHING;
extern int runAltForecast;


int printGlobal = 0;
int globalCount1 = 0;
int globalCount2 = 0;

int globalCount10 = 0;
int globalCount11 = 0;
int globalCount12 = 0;
int globalFirst = 1;

int delayVersion = 4; // 1 first version, 
					  // 2 second version when not needed to go back to prefPath, 
					  // 3 creating one arc from startHistoricalData to end,
					  // 4 as 3 but only wind and sea in delay map and current on its own,
					  // 5 as 3 but no delay and no current from startHistoricalData to end


double M_PI2 = M_PI * 2;
double LOOKUP_COS_STEP_INV;
double cos_table[20001];
double sin_table[20001];
double atan_table[20001];

double glob_tmpTotDist = 0;

//double maxBearingDiff;
//double sumAbsBearingDiff ;
//double sumBearingDiff;
//double bearingErrorXY[4];
//int nBearingDiff;


bool check_file_exist(char* name) {
	struct stat buffer;
	return (stat(name, &buffer) == 0);
}

void 	initModelStatusValues() {
	model.status.weatherHistoryOpenFile_fail = 0;
	model.functions.valuesNow.deltaArcStart = 0;
	model.results.leg = NULL;
}

std::string splitFilename(std::string namn, int alt) {
	std::string resultat;
	size_t found;
	found = namn.find_last_of("/\\");
	if (alt == 0)
		resultat = namn.substr(0, found);
	else
		resultat = namn.substr(found + 1, namn.size());
	return resultat;
}

int callRaster()
{ // NOT USED!
	// create object of Geotiff class
	// Raster tiff((const char*)"trakt_contour.tif");
	Raster raster; //  (const char*)"WW3_NCEP.grb");

	// dump out array (band) dimensions of Geotiff data  
	int* dims = raster.GetDimensions();
	std::cout << dims[0] << " " << dims[1] << " " << dims[2] << std::endl;

	// output a value from 2D array  
	//float** rasterBandData = raster.GetRasterBand(1);
	float* rasterBandData;
	rasterBandData = (float*)malloc2(raster.Get_nRows() * raster.Get_nCols() * sizeof(float));
	raster.GetRasterBand_ny(1, rasterBandData);
	std::cout << "value at row 10, column 30: " << rasterBandData[30 + raster.Get_nCols() * 10] << std::endl;

	// call other methods, like get the name of the Geotiff
	// passed-in, its length, and its projection string 
	std::cout << raster.GetFileName() << std::endl;
	std::cout << strlen(raster.GetFileName()) << std::endl;
	std::cout << raster.GetProjection() << std::endl;

	// dump out the Geotransform (6 element array of doubles) 
	double* gt = raster.GetGeoTransform();
	std::cout << gt[0] << " " << gt[1] << " " << gt[2] << " " << gt[3] << " " << gt[4] << " " << gt[5] << std::endl;

	// dump out Geotiff band NoData value (often it is -9999.0)
	//cout << "No data value: " << raster.GetNoDataValue() << endl;




	return 0;
}


inline float ApproxCos(float x)
{
	constexpr float tp = 1. / (2. * M_PI);
	x *= tp;
	x -= (.25) + std::floor(x + (.25));
	x *= (16.) * (std::abs(x) - (.5));
#if EXTRA_PRECISION
	x += T(.225) * x * (std::abs(x) - T(1.));
#endif
	return x;
}

inline float ApproxSin(float x)
{
	return ApproxCos(M_PI_2 - x);
}

float sqrt3(const float& n)
{
	static union { int i; float f; } u;
	u.i = 0x2035AD0C + (*(int*)&n >> 1);
	return n / u.f + u.f * 0.25f;
}

float ApproxSqrt(float x)
{
	unsigned int i = *(unsigned int*)&x;

	// adjust bias
	i += 127 << 23;
	// approximation of square root
	i >>= 1;

	return *(float*)&i;
}
// Polynomial approximating arctangenet on the range -1,1.
// Max error < 0.005 (or 0.29 degrees)
float ApproxAtan(float z)
{
	const float n1 = 0.97239411f;
	const float n2 = -0.19194795f;
	return (n1 + n2 * z * z) * z;
}

float lookUpAtan2(float z)
{
	if (z > M_PI2)
		z -= M_PI2;
	if (z < 0)
		z += M_PI2;
	int pos = (int)(z * LOOKUP_COS_STEP_INV);
	//if (pos < 0)
	//	printf("x %lf pos %d\n", x, pos);
	//if(pos > 2000)
	//	printf("x %lf pos %d\n", x, pos);
	return atan_table[pos];

}

float ApproxAtan2(float y, float x)
{
	if (x != 0.0f)
	{
		if (fabsf(x) > fabsf(y))
		{
			const float z = y / x;
			if (x > 0.0)
			{
				// atan2(y,x) = atan(y/x) if x > 0
				return ApproxAtan(z);
			}
			else if (y >= 0.0)
			{
				// atan2(y,x) = atan(y/x) + PI if x < 0, y >= 0
				return ApproxAtan(z) + M_PI;
			}
			else
			{
				// atan2(y,x) = atan(y/x) - PI if x < 0, y < 0
				return ApproxAtan(z) - M_PI;
			}
		}
		else // Use property atan(y/x) = PI/2 - atan(x/y) if |y/x| > 1.
		{
			const float z = x / y;
			if (y > 0.0)
			{
				// atan2(y,x) = PI/2 - atan(x/y) if |y/x| > 1, y > 0
				return -ApproxAtan(z) + M_PI_2;
			}
			else
			{
				// atan2(y,x) = -PI/2 - atan(x/y) if |y/x| > 1, y < 0
				return -ApproxAtan(z) - M_PI_2;
			}
		}
	}
	else
	{
		if (y > 0.0f) // x = 0, y > 0
		{
			return M_PI_2;
		}
		else if (y < 0.0f) // x = 0, y < 0
		{
			return -M_PI_2;
		}
	}
	return 0.0f; // x,y = 0. Could return NaN instead.
}

float lookUpCos(float x)
{
	if (x > M_PI2)
		x -= M_PI2;
	if (x < 0)
		x += M_PI2;
	int pos = (int)(x * LOOKUP_COS_STEP_INV);
	if (pos < 0 || pos > 20000) {
		errlog("ERROR! lookUpCos pos %d x %lf\n", pos, x);
		pos = 0;
	}
	//if (pos < 0)
	//	printf("x %lf pos %d\n", x, pos);
	//if(pos > 2000)
	//	printf("x %lf pos %d\n", x, pos);
	return cos_table[pos];
}

float lookUpSin(float x)
{
	if (x > M_PI2)
		x -= M_PI2;
	if (x < 0)
		x += M_PI2;
	//if (x < -100000 || x > 100000) {
	//	errlog("ERROR! lookUpCost %lf\n", x);
	//	x = 0;
	//}
	int pos = (int)(x * LOOKUP_COS_STEP_INV);
	if (pos < 0 || pos > 20000) {
		errlog("ERROR! lookUpSin pos %d x %lf\n", pos, x);
		pos = 0;
	}
	//if (pos < 0)
	//	printf("x %lf pos %d\n", x, pos);
	//if(pos > 2000)
	//	printf("x %lf pos %d\n", x, pos);
	return sin_table[pos];
}

//inline float lookUpSin(float x) {
//	return lookUpCos(M_PI_2 - x);
//}

double estimateLargeCircleDistance_km(double lat1, double lon1, double lat0, double lon0) {
	double deglen = 110.25, xDiff = lon1 - lon0;
	if (xDiff > 270)
		xDiff -= 360;
	if (xDiff < -270)
		xDiff += 360;
	double x = lat1 - lat0;
	double y = (xDiff)*lookUpCos(lat0 * M_PI / 180);
	//printf("lat0 %.4lf lon0 %.4lf lat1 %.4lf lon1 %.4lf lookUpCost %.4lf val %.4lf val2 %.4lf\n", lat0, lon0, lat1, lon1,
	//	val3, val, val2);
	return deglen * sqrt(x * x + y * y);
}

double estimateLargeCircleDistance2_km(double lat1, double lon1, double lat0, double lon0) {
	double deglen = 12155.06; // 110.25^2
	double xDiff = lon1 - lon0;
	if (xDiff > 270)
		xDiff -= 360;
	if (xDiff < -270)
		xDiff += 360;
	double x = lat1 - lat0;
	double y = (xDiff) * lookUpCos(lat0 * M_PI / 180);
	//printf("lat0 %.4lf lon0 %.4lf lat1 %.4lf lon1 %.4lf lookUpCost %.4lf val %.4lf val2 %.4lf\n", lat0, lon0, lat1, lon1,
	//	val3, val, val2);
	return deglen * (x * x + y * y);
}

void estimateDestinationPointStorm(int stormNr, int t, double kvot, double* lat, double* lon) {
	if (t < model.storms[stormNr].nFeatures - 1) {
		*lat = model.storms[stormNr].feature[t].lat * (1 - kvot) + model.storms[stormNr].feature[t + 1].lat * kvot;
		*lon = model.storms[stormNr].feature[t].lon * (1 - kvot) + model.storms[stormNr].feature[t + 1].lon * kvot;
	}
	else {
		if (t > 0) {
			*lat = model.storms[stormNr].feature[t].lat * (1 + kvot) - model.storms[stormNr].feature[t - 1].lat * kvot;
			*lon = model.storms[stormNr].feature[t].lon * (1 + kvot) - model.storms[stormNr].feature[t - 1].lon * kvot;
		}
		else {
			*lat = model.storms[stormNr].feature[t].lat;
			*lon = model.storms[stormNr].feature[t].lon;
		}
	}
}

double calcBearingFromToCoords(double lat1, double lon1, double lat2, double lon2) {
	spherical::Point p1(lat1, lon1);
	spherical::Point p2(lat2, lon2);
	return p1.bearingTo(p2);
}

double estimateBearingFromToCoords(double lat1, double lon1, double lat2, double lon2) {
	double xDiff = lon2 - lon1;
	if (xDiff > 270)
		xDiff -= 360;
	if (xDiff < -270)
		xDiff += 360;
	double vinkel = 90 - ApproxAtan2(lat2 - lat1, xDiff) * 180 / M_PI;
	if (vinkel < 0)
		vinkel += 360;

	//xDiff = spherical::Point(lat1, lon1).bearingTo(spherical::Point(lat2, lon2));
	//double absDiff= abs(xDiff - vinkel);
	//if (absDiff > maxBearingDiff) {
	//	maxBearingDiff = absDiff;
	//	bearingErrorXY[0] = lat1;
	//	bearingErrorXY[1] = lon1;
	//	bearingErrorXY[2] = lat2;
	//	bearingErrorXY[3] = lon2;
	//}
	//sumAbsBearingDiff += absDiff;
	//sumBearingDiff += xDiff - vinkel;
	//nBearingDiff++;

	return vinkel;
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

	x = (double*)malloc2(2 * sizeof(double));
	y = (double*)malloc2(2 * sizeof(double));
	z = (double*)malloc2(2 * sizeof(double));

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
	namn = (char*)malloc2(256 * sizeof(char));
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
				model.network.physicalLev[i].point_x[i1],
				model.network.physicalLev[i].point_y[i1]);
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

	x = (double*)malloc2(2 * sizeof(double));
	y = (double*)malloc2(2 * sizeof(double));
	z = (double*)malloc2(2 * sizeof(double));

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
	int forsta = 1, nod2, nextLevel, cNr, prefPath, startPos;
	double distance;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/allArcs.geojson", model.params.resultPath.c_str());
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "allPhysicalArcs");

	for (int i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i == 20)
			i = i;
		for (int i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;
			if (i == 3 && i1 ==46)
				i = i;
			for (int i2 = 0; i2 < model.network.physicalLev[i].nOutNodes[i1]; i2++) {
				nod2 = model.network.physicalLev[i].outNode[i1][i2];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2];

				if (model.network.physicalLev[i].outNoNormalArc_useTSS[i1][i2] == 1) {
					//if (model.params.preferredPathStraightLineFeasibleFrom[i] == 1 || (i1 != model.params.preferredPathOrtoPos[i] || nextLevel != i + 1 ||
					//	nod2 != model.params.preferredPathOrtoPos[nextLevel]))
						continue; // do not include this arc as a tss should be used instead.
				}

				if (nextLevel > 0) {
					if (i1 == model.params.preferredPathOrtoPos[i] &&
						nod2 == model.params.preferredPathOrtoPos[nextLevel] && i == nextLevel - 1 &&
						(model.params.preferredPathStraightLineFeasibleFrom[i] == 0 || model.params.max_changeDirection == 0))
						prefPath = 1;
					else
						prefPath = 0;
				}
				else {
					if (i1 == model.params.preferredPathOrtoPos[i] &&
						model.network.channel[-nextLevel - 1].followExactly == 1 &&
						model.network.channel[-nextLevel - 1].bastStartLevel == i &&
						model.network.channel[-nextLevel - 1].straightArcFeasible_toChannelFromPrefPath == 0 && 
						model.network.channel[-nextLevel - 1].preferredPathPoint_posConnectTo >= 0)
						prefPath = 2;
					else
						prefPath = 0;
				}

				if (forsta != 1)
					fprintf(filpekG, ", ");
				else
					forsta = 0;

				if (nextLevel < 0) {
					distance = model.network.physicalLev[i].point[i1].distanceTo(model.network.channel[-nextLevel - 1].point[0]);
					fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level1\":%d, \"nodPos1\":%d, \"arcPos\":%d, \"level2\":%d, \"nodPos2\":%d, \"distance\":%.3lf},\n",
						i, i1, i2, nextLevel, nod2, distance);
					if (prefPath == 0)
						fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[ [%.4lf,%.4lf], [%.4lf,%.4lf] ]]}}\n",
							model.network.physicalLev[i].point_x[i1],
							model.network.physicalLev[i].point_y[i1],
							model.network.channel[-nextLevel - 1].point_x[0],
							model.network.channel[-nextLevel - 1].point_y[0]);
					else {
						// follow the preferred path
						fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[");
						fprintf(filpekG, "[%.4lf,%.3lf]", model.network.physicalLev[i].point[i1].longitude().degrees(),
							model.network.physicalLev[i].point[i1].latitude().degrees());
						for (int i3 = 0; i3 <= model.network.channel[-nextLevel - 1].preferredPathPoint_posConnectTo; i3++) {
							fprintf(filpekG, ", [%.4lf,%.3lf]", model.network.physicalLev[i].preferredPathPoint[i3].longitude().degrees(),
								model.network.physicalLev[i].preferredPathPoint[i3].latitude().degrees());
						}
						fprintf(filpekG, ", [%.4lf,%.4lf]",
							model.network.channel[-nextLevel - 1].point_x[0],
							model.network.channel[-nextLevel - 1].point_y[0]);
						fprintf(filpekG, "]]}}\n");
					}
				}
				else {
					distance = model.network.physicalLev[i].point[i1].distanceTo(model.network.physicalLev[nextLevel].point[nod2]);
					fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level1\":%d, \"nodPos1\":%d, \"arcPos\":%d, \"level2\":%d, \"nodPos2\":%d, \"distance\":%.3lf},\n",
						i, i1, i2, nextLevel, nod2, distance);
					if (prefPath == 0)
						fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[ [%.4lf,%.4lf], [%.4lf,%.4lf] ]]}}\n",
							model.network.physicalLev[i].point_x[i1],
							model.network.physicalLev[i].point_y[i1],
							model.network.physicalLev[nextLevel].point_x[nod2],
							model.network.physicalLev[nextLevel].point_y[nod2]);
					else {
						// follow the preferred path
						fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[");
						fprintf(filpekG, "[%.4lf,%.3lf]", model.network.physicalLev[i].point[i1].longitude().degrees(),
							model.network.physicalLev[i].point[i1].latitude().degrees());
						for (int i3 = 0; i3 < model.network.physicalLev[i].npreferredPathPoints; i3++) {
							fprintf(filpekG, ", [%.4lf,%.3lf]", model.network.physicalLev[i].preferredPathPoint[i3].longitude().degrees(),
								model.network.physicalLev[i].preferredPathPoint[i3].latitude().degrees());
						}
						fprintf(filpekG, "]]}}\n");
					}
				}
			}
		}
		for (int i1 = 0; i1 < model.network.nChannels; i1++) { //  .nUsedChannels; i1++) {
			cNr = i1; // model.network.usedChannel[i1];
			for (int i2b = 0; i2b < model.network.channel[cNr].nOutNodes; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[i2b];
				if (nextLevel != i && nextLevel >= 0)
					continue;
				if(nextLevel < 0 && i > 0)
					continue;
				nod2 = model.network.channel[cNr].outNode[i2b];

				if (model.network.channel[cNr].outNoNormalArc_useTSS[i2b] == 1)
					continue; // do not include this arc as a tss should be used instead.

				if (i1 == 0)
					i1 = i1;
				//if (model.network.channel[cNr].outPolyPoint[i2b] == -1)
				if(nextLevel >= 0)
					distance = model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].distanceTo(
						model.network.physicalLev[nextLevel].point[nod2]);
				else
					distance = model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].distanceTo(
						model.network.channel[-nextLevel - 1].point[nod2]);
				//else
				//	distance = model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].distanceTo(
				//		model.network.channel[cNr].polygonUse_point[1][model.network.channel[cNr].outPolyPoint[i2b]]);

				if (forsta != 1)
					fprintf(filpekG, ", ");
				else
					forsta = 0;
				fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level1\":%d, \"nodPos1\":%d, \"arcPos\":%d, \"level2\":%d, \"nodPos2\":%d, \"distance\":%.3lf},\n",
					-cNr - 1, 0, i2b, nextLevel, nod2, distance);
				if (nextLevel >= 0) {
					if (nod2 == model.params.preferredPathOrtoPos[nextLevel] &&
						model.network.channel[cNr].followExactly == 1 &&
						model.network.channel[cNr].bastEndLevel == nextLevel &&
						model.network.channel[cNr].straightArcFeasible_fromChannelToPrefPath == 0 &&
						model.network.channel[cNr].preferredPathPoint_posConnectFrom >= 0) {
						// follow the preferred path
						fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[");
						fprintf(filpekG, "[%.4lf,%.4lf]",
							model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1],
							model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1]);
						startPos = model.network.channel[cNr].preferredPathPoint_posConnectFrom;
						if (startPos < 0) {
							errlog("ERROR! This shouldn't happen, row %d\n", __LINE__);
							printf("ERROR! This shouldn't happen, row %d\n", __LINE__);
							startPos = 0;
						}
						for (int i3 = startPos;
							i3 < model.network.physicalLev[nextLevel - 1].npreferredPathPoints; i3++) {
							fprintf(filpekG, ", [%.4lf,%.3lf]", model.network.physicalLev[nextLevel - 1].preferredPathPoint[i3].longitude().degrees(),
								model.network.physicalLev[nextLevel - 1].preferredPathPoint[i3].latitude().degrees());
						}
						fprintf(filpekG, ", [%.4lf,%.3lf]", model.network.physicalLev[nextLevel].point[nod2].longitude().degrees(),
							model.network.physicalLev[i].point[nod2].latitude().degrees());
						fprintf(filpekG, "]]}}\n");
					}
					else
					fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[ [%.4lf,%.4lf], [%.4lf,%.4lf] ]]}}\n",
						model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1],
						model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[nextLevel].point_x[nod2],
						model.network.physicalLev[nextLevel].point_y[nod2]);
				}
				else
					fprintf(filpekG, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[ [%.4lf,%.4lf], [%.4lf,%.4lf] ]]}}\n",
						model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1],
						model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1],
						model.network.channel[-nextLevel - 1].point_x[nod2],
						model.network.channel[-nextLevel - 1].point_y[nod2]);
			}
		}
	}

	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	//errlog("ERROR! Add channels to the plotted arcs\n");

	return 0;
}

int writeAllChannelsToGeojson()
{
	int forsta = 1, nod2, nextLevel, cNr, prefPath;
	double distance;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/allChannels.geojson", model.params.resultPath.c_str());
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "allChannels");

	for (int i = 0; i < model.network.nChannels; i++) {
		if (i > 0)
			fprintf(filpekG, ", ");
		fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"cNr\":%d, \"type\":%d, \"nPoints\":%d, \"time\":%.2lf, \"fuel\":%.2lf, "
			"\"earliestStartLevel\":%d, \"latestEndLevel\":%d, \"distance\":%.3lf},\n",
			i, model.network.channel[i].type, model.network.channel[i].nPoints, model.network.channel[i].timeThroughChannel, model.network.channel[i].totalConsumption,
			model.network.channel[i].earliestStartLevel, model.network.channel[i].latestEndLevel, model.network.channel[i].distance_km);
		fprintf(filpekG, "    \"geometry\":{\"type\": \"LineString\", \"coordinates\":[");
		for (int i1 = 0; i1 < model.network.channel[i].nPoints; i1++) {
			if(i1 > 0)
				fprintf(filpekG, ", ");
			fprintf(filpekG, "[%.4lf,%.3lf]", model.network.channel[i].point_x[i1], model.network.channel[i].point_y[i1]);
		}
		fprintf(filpekG, "]}}\n");
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	//errlog("ERROR! Add channels to the plotted arcs\n");

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

int getTidIntForecast(double tidTot) {
	int tidInt;
	tidInt = (int)(tidTot * model.weather_inv_timeIntervall_h);
	if (tidInt > model.weather_nTimeIntervals_maxValue) {
		tidInt = model.weather_nTimeIntervals_maxValue;
	}
	return tidInt;
}

double identifyForecastType(double tidTot) {
	int tidInt;
	double forecastType = 0, tidDiff;
	long long UTC;

	tidInt = (int)(tidTot * model.weather_inv_timeIntervall_h); // ) / model.weather_timeIntervall_h);
	if (tidInt > model.weather_nTimeIntervals_maxValue) {
		if (model.params.hindCast == 1) {
			forecastType += 8;
			return forecastType;
		}
		tidInt = model.weather_nTimeIntervals_maxValue;
	}
	for (int i = 0; i < 8; i++) {
		if (model.params.hindCast == 0) {
			if (model.weather[i].timeIntervalIndex[tidInt] >= model.weather[i].nTimeIntervals_forecast)
				forecastType += 1;
		}
		else {
			if (model.weather[i].timeIntervalIndex[tidInt] >= model.weather[i].nTimeIntervals_forecast)
				forecastType += 1;
			else {
				UTC = (long long)(model.params.UTC_secondsStart + tidTot * 3600);
				tidDiff = (UTC - model.weather[i].secondsUTC[model.weather[i].timeIntervalIndex[tidInt]]) / 3600.0;
				if (tidDiff > model.weather[i].timeIntervall_expected * 1.5)
					forecastType += 0.5;
			}
		}
		//if (model.functions.valuesNow.Wpt == 17) {
		//	printf("i %d tidInt %d, tidIndex %d nTimeInt %d\n", i, tidInt,
		//		model.weather[i].timeIntervalIndex[tidInt], model.weather[i].nTimeIntervals_forecast);
		//}
	}

	if (forecastType > 4)
		forecastType = forecastType;
	return forecastType;
}


void fixPositionString_latLon(double y, double x, char* namn) {
	int heltal, heltal2;
	double absVal;

	absVal = abs(y);
	heltal = absVal;
	heltal2 = (absVal - heltal) * 60.0;
	sprintf(namn, "");
	if(heltal < 10)
		sprintf(namn, "%s0", namn);
	sprintf(namn, "%s%d ", namn, heltal);
	if (heltal2 < 10)
		sprintf(namn, "%s0", namn);
	sprintf(namn, "%s%d", namn, heltal2);
	if (y >= 0)
		sprintf(namn, "%sN, ", namn);
	else
		sprintf(namn, "%sS, ", namn);

	absVal = abs(x);
	heltal = absVal;
	heltal2 = (absVal - heltal) * 60.0;
	if (heltal < 10)
		sprintf(namn, "%s0", namn);
	if (heltal < 100)
		sprintf(namn, "%s0", namn);
	sprintf(namn, "%s%d ", namn, heltal);
	if (heltal2 < 10)
		sprintf(namn, "%s0", namn);
	sprintf(namn, "%s%d", namn, heltal2);
	if (x >= 0)
		sprintf(namn, "%sE", namn);
	else
		sprintf(namn, "%sW", namn);
}



int get_isWindFavorable(double windSpeed, double rel_windDir) {

	if (windSpeed <= 18.52)
		return 1; // 10 knots x 1.852 = 18.52 km/h

	if (windSpeed <= 29.632) { // 16 knots x 1.852 = 29.632 km/h
		if (rel_windDir >= 2.356194)
			return 1; // 135 degrees = 2.356194 radians
	}
	else {
		if (windSpeed <= 50.004) { // 27 knots x 1.852 = 50.004 km/h
			if (rel_windDir >= 2.530727)
				return 1; // 145 degrees = 2.530727 radians
		}
		if(windSpeed > 1000)
			return -1;
	}

	return 0;
}

int get_isSeaFavorable(double waveHeight, double rel_waveDir) {

	if (waveHeight <= 0.7)
		return 1;

	if (waveHeight <= 1.8) {
		if (rel_waveDir >= 2.70526)
			return 1; // 155 degrees = 2.70526 radians
	}
	else {
		if (waveHeight > 100)
			return -1;
	}
		
	return 0;
}


//nSplit = round(timeCheck / 4.0);
//if (nSplit == 0)
//	nSplit = 1; // start pos
// startKvot =  (double)ii / nSplit;
// endKvot = (double)(ii + 1) / nSplit;

/*
int genSplitsArcNew(int arcNr, spherical::Point p1, spherical::Point p2, int prefPath) {
	double wantedTimeLength = 4.0, distNu, wantedDist, kvot;
	int nSplit = 0, nWantedSplits;
	int i, cNr, level, ii;
	spherical::Point p3, pointLast;
	double x, y, bearing, distTmp, calmWaterSpeed, distBas;
	double distLastSplit, distArc = model.arc[arcNr].distance * 1000.0, distTot = 0;

	if (model.network.nCoords + 500 >= model.network.nAllocCoords) {
		model.network.nAllocCoords += 500;
		model.network.xCoord = (double*)realloc(model.network.xCoord, model.network.nAllocCoords * sizeof(double));
		model.network.yCoord = (double*)realloc(model.network.yCoord, model.network.nAllocCoords * sizeof(double));
	}

	//printf("** genSplitsArc arcNr %d from xy %.3lf %.3lf to %.3lf %.3lf prefPath %d\n", arcNr,
	//	p1.longitude().degrees(), p1.latitude().degrees(), p2.longitude().degrees(), p2.latitude().degrees(), prefPath);
	//level = model.arc[arcNr].fromLevel;
	//if (level >= 0) {
	//	for (int i3 = 0; i3 < model.network.physicalLev[level].npreferredPathPoints; i3++) {
	//		printf("prefPath i3 %d lon/lat %.3lf %.3lf\n", i3,
	//			model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees(),
	//			model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees());
	//	}
	//}
	// if pref path

	if (arcNr == 169)
		arcNr = arcNr;

	if (prefPath == 1) {
		nWantedSplits = round(model.arc[arcNr].time / wantedTimeLength);
		if (model.network.nMaxSplits == 1)
			nWantedSplits = 1;

		if (model.arc[arcNr].speedSetting == -1)
			wantedDist = model.arc[arcNr].distance * 1000.0;
		else
			wantedDist = model.arc[arcNr].distance / nWantedSplits * 1000.0;
		level = model.arc[arcNr].fromLevel;
		//printf("--- arcNr %d nWantedSplitsPrefPath %d, wantedDist %.2lf level %d\n", arcNr, nWantedSplits, wantedDist, level);

		model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
		model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
		//printf("/// solPathCoord pos %d added %.3lf %.3lf\n", model.network.nCoords,
		//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
		//printf("distLastSplit %.2lf wantedDist %.2lf\n", distLastSplit, wantedDist);
		model.network.posSplitCoord[nSplit] = model.network.nCoords;
		//printf("## adderar startSplit nr %d fran pos %d i xy %.3lf %.3lf\n", nSplit, model.network.nCoords,
		//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
		model.network.startKvot[nSplit] = 0.0;
		nSplit++;
		distLastSplit = 0;
		(model.network.nCoords)++;
		pointLast = p1;

		for (int i3 = 0; i3 < model.network.physicalLev[level].npreferredPathPoints; i3++) {
			//printf("prefPath i3 %d lon/lat %.3lf %.3lf\n", i3,
			//	model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees(),
			//	model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees());
			distTmp = pointLast.distanceTo(model.network.physicalLev[level].preferredPathPoint[i3]);
			distBas = distTmp;
			distTot += distTmp;
			for (ii = 0; ii < nWantedSplits + 1; ii++) {
				if (distLastSplit + distTmp > 0.5 * wantedDist) {

					if (distBas < wantedDist) {
						// check if bearing direction changed significantly. If not then skip this one.
						diff = getDiff_anglesDegrees(bearingNu, bearingOld);
						if (diff < model.params.report_minBearingDiffWpt)
							break; // too little change in diff so do not save this one as a splitPoint but add the coordinate below

						distLastSplit = 0;
						pointLast = pointLast.destinationPoint(distBas, pointLast.bearingTo(model.network.physicalLev[level].preferredPathPoint[i3]));
					}
					else {
						distLastSplit = 0;
						pointLast = pointLast.destinationPoint(wantedDist, pointLast.bearingTo(model.network.physicalLev[level].preferredPathPoint[i3]));
					}
					// if (nPkter == 0) {
					model.network.yCoord[model.network.nCoords] = pointLast.latitude().degrees();
					model.network.xCoord[model.network.nCoords] = pointLast.longitude().degrees();
					//printf("/// solPathCoord1 pos %d added %.3lf %.3lf\n", model.network.nCoords,
					//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
					//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii %d distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, ii, distTmp, x[nPkter], y[nPkter], __LINE__);
					model.network.posSplitCoord[nSplit] = model.network.nCoords;
					model.network.startKvot[nSplit] = (distTot - distTmp + wantedDist) / distArc;
					model.network.endKvot[nSplit - 1] = (distTot - distTmp + wantedDist) / distArc;
					//printf("## adderar Split nr %d fran pos %d i3 %d ii %d xy %.3lf %.3lf startKvot %.2lf distTot %.2lf distTmp %.2lf distArc %.2lf\n", 
					//	nSplit, model.network.nCoords, i3, ii,
					//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords],
					//	model.network.startKvot[nSplit], distTot, distTmp, distArc);
					nSplit++;
					(model.network.nCoords)++;
					distTmp -= wantedDist;
				}
				else
					break;
			}
			distLastSplit += distTmp;

			if (distTot < distArc - 10) {
				model.network.yCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees();
				model.network.xCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees();
				//printf("/// solPathCoord2 pos %d added %.3lf %.3lf\n", model.network.nCoords,
				//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
				//printf("distLastSplit %.2lf distTmp %.2lf wantedDist %.2lf distTot %.2lf distArc %.2lf\n", distLastSplit, distTmp, wantedDist, distTot, distArc);
				if (distLastSplit >= wantedDist && distTot < distArc - wantedDist * 0.3) {
					model.network.posSplitCoord[nSplit] = model.network.nCoords;
					model.network.startKvot[nSplit] = distTot / distArc;
					model.network.endKvot[nSplit - 1] = distTot / distArc;
					//printf("## adderar split nr %d fran pos %d i xy %.3lf %.3lf distTot %.2lf distArc %.2lf\n", nSplit, model.network.nCoords,
					//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords], distTot, distArc);
					nSplit++;
					distLastSplit = 0;
				}
				(model.network.nCoords)++;
				pointLast = model.network.physicalLev[level].preferredPathPoint[i3];
			}
		}
		model.network.endKvot[nSplit - 1] = 1.0;
		return nSplit;
	}

	// if corridor
	if (model.arc[arcNr].fromLevel < 0 && model.arc[arcNr].toLevel < 0) {
		cNr = -model.arc[arcNr].fromLevel - 1;
		//printf("--- arcNr %d cNr %d, \n", arcNr, cNr);
		distTot = 0;
		if (model.network.channel[cNr].timeThroughChannel > 0) {
			kvot = model.network.channel[cNr].timeThroughChannel / wantedTimeLength;
			wantedDist = model.network.channel[cNr].distance_km * 1000.0; // / kvot;

			//printf("++genSplitsArc kvot %.2lf wantedDist %.2lf timeThroughChannel %.2lf wantedTime %.2lf\n", kvot,
			//	wantedDist, model.network.channel[cNr].timeThroughChannel, wantedTimeLength);
		}
		else {
			if (model.network.nMaxSplits > 1) {
				calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				wantedDist = 1000 * calmWaterSpeed * wantedTimeLength;
				kvot = distArc / wantedDist;
			}
			else {
				kvot = model.network.channel[cNr].timeThroughChannel / wantedTimeLength;
				wantedDist = model.network.channel[cNr].distance_km * 1000.0; // / kvot;
			}
			//printf("++genSplitsArc2 kvot %.2lf wantedDist %.2lf calmWaterSpeed %.2lf\n", kvot,
			//	wantedDist, calmWaterSpeed, wantedTimeLength);
		}


		distLastSplit = wantedDist * 2;
		for (i = 0; i < model.network.channel[cNr].nPoints; i++) {
			if (i > 0) {
				distTmp = (model.network.channel[cNr].distanceFromStart[i] - model.network.channel[cNr].distanceFromStart[i - 1]) * 1000.0;
				distBas = distTmp;
				for (ii = 0; ii < kvot + 1; ii++) {
					//printf("i %d, ii %d distLastSplit %.2lf distTmp %.2lf 2xwantedDist %.2lf\n", i, ii,
					//	distLastSplit, distTmp, 2 * wantedDist);
					if (distLastSplit + distTmp >= 1.7 * wantedDist && distTot + distLastSplit < distArc - 0.5 * wantedDist) {
						//printf("+++add extra point distTot %.2lf wantedDist %.2lf distArc %.2lf\n", distTot, wantedDist, distArc);
						distTot += distLastSplit;
						//printf("--check pointLast %.3lf %.3lf\n", pointLast.longitude().degrees(), pointLast.latitude().degrees());
						if (distBas < wantedDist)
							pointLast = pointLast.destinationPoint(distBas, pointLast.bearingTo(model.network.channel[cNr].point[i]));
						else
							pointLast = pointLast.destinationPoint(wantedDist, pointLast.bearingTo(model.network.channel[cNr].point[i]));
						distLastSplit = 0;
						// if (nPkter == 0) {
						model.network.yCoord[model.network.nCoords] = pointLast.latitude().degrees();
						model.network.xCoord[model.network.nCoords] = pointLast.longitude().degrees();
						//printf("  %d at coord %.3lf %.3lf\n", model.network.nCoords, model.network.xCoord[model.network.nCoords],
						//	model.network.yCoord[model.network.nCoords]);
						//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii %d distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, ii, distTmp, x[nPkter], y[nPkter], __LINE__);
						model.network.posSplitCoord[nSplit] = model.network.nCoords;
						model.network.startKvot[nSplit] = (distTot + wantedDist) / distArc;
						//printf("//sets2 splitPos %d startKvot to %.2lf from distTot %.2lf\n", nSplit, model.network.startKvot[nSplit], distTot);
						model.network.endKvot[nSplit - 1] = (distTot + wantedDist) / distArc;
						//printf("//sets2 splitPos %d endKvot to %.2lf from distTot %.2lf\n", nSplit - 1, model.network.endKvot[nSplit - 1], distTot);
						nSplit++;
						(model.network.nCoords)++;
						distTmp -= wantedDist;
						distTot += wantedDist;
					}
					else
						break;
				}
				distLastSplit += distTmp;
				//distTot += distTmp;
			}
			if (i < model.network.channel[cNr].nPoints - 1) {
				model.network.yCoord[model.network.nCoords] = model.network.channel[cNr].point[i].latitude().degrees();
				model.network.xCoord[model.network.nCoords] = model.network.channel[cNr].point[i].longitude().degrees();
				//printf("add point %d coord %.3lf %.3lf distLastSplit %.2lf distTot %.2lf distArc %.2lf\n",
				//	model.network.nCoords, model.network.xCoord[model.network.nCoords],
				//	model.network.yCoord[model.network.nCoords], distLastSplit, distTot, distArc);
				if (distLastSplit >= wantedDist && (distTot + distLastSplit < distArc - wantedDist * 0.3 || i == 0)) {
					//printf("++use as split points nr %d distLastSplit %.2lf distTot %.2lf distArc %.2lf\n", nSplit, distLastSplit, distTot, distArc);
					model.network.posSplitCoord[nSplit] = model.network.nCoords;
					if (nSplit > 0) {
						distTot += distLastSplit;
						model.network.endKvot[nSplit - 1] = distTot / distArc;
						//printf("//sets splitPos %d endKvot to %.2lf from distTot %.2lf\n", nSplit - 1, model.network.endKvot[nSplit - 1], distTot);
					}
					model.network.startKvot[nSplit] = distTot / distArc;
					//printf("//sets splitPos %d startKvot to %.2lf from distTot %.2lf\n", nSplit, model.network.startKvot[nSplit], distTot);
					nSplit++;
					distLastSplit = 0;
				}
				(model.network.nCoords)++;
				pointLast = model.network.channel[cNr].point[i];
			}
		}
		model.network.endKvot[nSplit - 1] = 1.0;
		return nSplit;
	}

	// else straight arc
	nWantedSplits = round(model.arc[arcNr].time / wantedTimeLength);
	bearing = p1.bearingTo(p2);
	nSplit = nWantedSplits;
	//printf("--- arcNr %d nWantedSplitsStraight %d\n", arcNr, nWantedSplits);
	if (nSplit == 0)
		nSplit = 1;
	if (nSplit > model.network.nMaxSplits)
		nSplit = model.network.nMaxSplits;
	for (i = 0; i < nSplit; i++) {
		model.network.startKvot[i] = (double)i / nSplit;
		model.network.endKvot[i] = (double)(i + 1) / nSplit;
		if (i > 0) {
			p3 = p1.destinationPoint(model.arc[arcNr].distance * 1000.0 * model.network.startKvot[i], bearing);
			x = p3.longitude().degrees();
			y = p3.latitude().degrees();
		}
		else {
			x = p1.longitude().degrees();
			y = p1.latitude().degrees();
		}
		model.network.xCoord[model.network.nCoords] = x;
		model.network.yCoord[model.network.nCoords] = y;
		model.network.posSplitCoord[i] = model.network.nCoords;
		(model.network.nCoords)++;
	}
	return nSplit;

}
*/

int check_realloc_coords(int nUsed) {
	int legNr;

	//	if (nUsed == -1) {
	if(model.results.leg == NULL){
		model.network.nAllocCoords = 500;
		model.network.xCoord = (double*)malloc2(model.network.nAllocCoords * sizeof(double));
		model.network.yCoord = (double*)malloc2(model.network.nAllocCoords * sizeof(double));
		//if (model.results.leg == NULL) {
			model.results.leg = (strLegRes*)malloc(model.params.nLegs * sizeof(strLegRes));
			for (legNr = 0; legNr < model.params.nLegs; legNr++) {
				model.results.leg[legNr].nAllocCoords = 100;
				model.results.leg[legNr].nCoords = 0;
				model.results.leg[legNr].x = (double*)malloc(model.results.leg[legNr].nAllocCoords * sizeof(double));
				model.results.leg[legNr].y = (double*)malloc(model.results.leg[legNr].nAllocCoords * sizeof(double));
			}
		//}
		//else {
		//	for (legNr = 0; model.params.nLegs; legNr++) {
		//		model.results.leg[legNr].x = (double*)malloc(model.network.nAllocCoords * sizeof(double));
		//		model.results.leg[legNr].y = (double*)malloc(model.network.nAllocCoords * sizeof(double));
		//	}
		//}
	}
	else {

		if (nUsed + 500 >= model.network.nAllocCoords) {
			if(nUsed < model.network.nAllocCoords)
				model.network.nAllocCoords += 500;
			else
				model.network.nAllocCoords = nUsed + 500;
			model.network.xCoord = (double*)realloc(model.network.xCoord, model.network.nAllocCoords * sizeof(double));
			model.network.yCoord = (double*)realloc(model.network.yCoord, model.network.nAllocCoords * sizeof(double));
			for (legNr = 0; legNr < model.params.nLegs; legNr++) {
				model.results.leg[legNr].x = (double*)realloc(model.results.leg[legNr].x, model.network.nAllocCoords * sizeof(double));
				model.results.leg[legNr].y = (double*)realloc(model.results.leg[legNr].y, model.network.nAllocCoords * sizeof(double));
			}
		}
	}

	return 0;
}


int addCoordsToPath(int arcNr, spherical::Point p1, spherical::Point p2, int prefPath) {
	double distNu, wantedDist, kvot;
	int i3b, legNr;
	int i, cNr, level, ii, startPos, endPos;
	spherical::Point p3, pointLast;
	double x, y, bearing, distTmp, calmWaterSpeed, distBas;
	double distLastSplit, distArc = model.arc[arcNr].distance * 1000.0, distTot = 0;

	check_realloc_coords(model.network.nCoords);

	if (model.arc[arcNr].fromLevel >= 0)
		legNr = model.network.physicalLev[model.arc[arcNr].fromLevel].legNr;
	else
		legNr = model.network.channel[-model.arc[arcNr].fromLevel - 1].legNr;

	if (prefPath == 1) {
		model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
		model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
		model.results.leg[legNr].y[model.results.leg[legNr].nCoords] = model.network.yCoord[model.network.nCoords];
		model.results.leg[legNr].x[model.results.leg[legNr].nCoords] = model.network.xCoord[model.network.nCoords];
		(model.results.leg[legNr].nCoords)++;
		(model.network.nCoords)++;
		pointLast = p1;

		level = model.arc[arcNr].fromLevel;
		startPos = 0;
		if (model.arc[arcNr].fromLevel < 0) {
			pointLast = p1;
			startPos = model.network.channel[-level - 1].preferredPathPoint_posConnectFrom;
			if (startPos < 0) {
				errlog("ERROR! This shouldn't happen, row %d\n", __LINE__);
				printf("ERROR! This shouldn't happen, row %d\n", __LINE__);
				startPos = 0;
			}
			level = model.arc[arcNr].toLevel - 1;
			distTmp = pointLast.distanceTo(model.network.physicalLev[level].preferredPathPoint[startPos]);
			bearing = p1.bearingTo(model.network.physicalLev[level].preferredPathPoint[startPos]);
			pointLast = model.network.physicalLev[level].preferredPathPoint[startPos];
		}
		endPos = model.network.physicalLev[level].npreferredPathPoints;

		if (model.arc[arcNr].toLevel < 0)
			endPos = model.network.channel[-model.arc[arcNr].toLevel - 1].preferredPathPoint_posConnectTo + 1;

		for (int i3 = startPos; i3 < endPos; i3++) {
			model.network.yCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees();
			model.network.xCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees();
			model.results.leg[legNr].y[model.results.leg[legNr].nCoords] = model.network.yCoord[model.network.nCoords];
			model.results.leg[legNr].x[model.results.leg[legNr].nCoords] = model.network.xCoord[model.network.nCoords];
			(model.results.leg[legNr].nCoords)++;
			(model.network.nCoords)++;
		}
		return 0;
	}

	// if corridor
	if (model.arc[arcNr].fromLevel < 0 && model.arc[arcNr].toLevel < 0 && model.arc[arcNr].fromLevel == model.arc[arcNr].toLevel) {
		cNr = -model.arc[arcNr].fromLevel - 1;
		for (i = 0; i < model.network.channel[cNr].nPoints; i++) {
			if (i < model.network.channel[cNr].nPoints - 1) {
				model.network.yCoord[model.network.nCoords] = model.network.channel[cNr].point[i].latitude().degrees();
				model.network.xCoord[model.network.nCoords] = model.network.channel[cNr].point[i].longitude().degrees();
				model.results.leg[legNr].y[model.results.leg[legNr].nCoords] = model.network.yCoord[model.network.nCoords];
				model.results.leg[legNr].x[model.results.leg[legNr].nCoords] = model.network.xCoord[model.network.nCoords];
				(model.results.leg[legNr].nCoords)++;
				(model.network.nCoords)++;
			}
		}
		return 0;
	}

	// else straight arc
	x = p1.longitude().degrees();
	y = p1.latitude().degrees();
	model.network.xCoord[model.network.nCoords] = x;
	model.network.yCoord[model.network.nCoords] = y;
	model.results.leg[legNr].y[model.results.leg[legNr].nCoords] = model.network.yCoord[model.network.nCoords];
	model.results.leg[legNr].x[model.results.leg[legNr].nCoords] = model.network.xCoord[model.network.nCoords];
	(model.results.leg[legNr].nCoords)++;
	(model.network.nCoords)++;
	return 0;

}


double getDiff_anglesDegrees(double x1, double x2) {
	double diff = x1 - x2;
	if (diff < 0) {
		if (diff >= -360)
			diff += 360;
		else
			diff = 180;
	}
	if (diff > 360)
		diff -= 360;
	if (diff > 180) {
		if (diff <= 360)
			diff = 360 - diff;
	}
	return diff;
}


double getCorrect_longitude(double x) {
	if (model.network.last_x < -999)
		model.network.last_x = x;
	else {
		if (abs(model.network.last_x - x) > 180) {
			if (model.network.last_x > x)
				model.network.last_x = x + 360;
			else
				model.network.last_x = x - 360;
		}
		else
			model.network.last_x = x;
	}
	return model.network.last_x;
}

int setTMtime(struct tm* tmBas, double time) {
	tmBas->tm_year = model.params.startYear - 1900;
	tmBas->tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas->tm_mday = model.params.startDay_nr;
	int hInt = (int)time;
	//tmBas->tm_hour = model.params.startHour + hInt; // 0;
	//tmBas->tm_min = model.params.startMinute + (time - hInt) * 60;
	tmBas->tm_hour = model.params.startHour;
	tmBas->tm_min = model.params.startMinute;
	tmBas->tm_sec = 0;
	time_t test = mktime(tmBas);
	//printf("time zone %d\n", tmBas->tm_isdst);
	//errlog("setTMtime %d %d %d: %d %d %d\n", tmBas->tm_year,
	//	tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d dst %d\n", __LINE__,
			tmBas->tm_year,
			tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec,
			tmBas->tm_isdst);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	tmBas->tm_hour += hInt; // 0;
	tmBas->tm_min += (time - hInt) * 60;
	time_t test2 = mktime(tmBas);
	//errlog("setTMtime %d %d %d: %d %d %d\n", tmBas->tm_year,
	//	tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec);
	checkMinnesAnvandning(__LINE__);
	if (test2 == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d dst %d\n", __LINE__,
			tmBas->tm_year,
			tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec,
			tmBas->tm_isdst);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	return 0;
}

int plotNodeTimeVisuellt(double time, double x, double y) {
	
	if (model.timeVisual.pos == 0) {
		model.timeVisual.oldX = x;
		model.timeVisual.oldY = y;
		(model.timeVisual.pos)++;
	}
	else {
		if (model.timeVisual.pos > 1)
			fprintf(model.timeVisual.filVisuell, ",\n");

		setTMtime(&(model.timeVisual.tmBas), time);
		fixReadableDate(model.timeVisual.tmBas, model.timeVisual.startTime);
		fprintf(model.timeVisual.filVisuell, "{\"type\":\"Feature\", \"properties\":{\n\"pos\":%d, \"startTime\":\"%s\"},\n",
			model.timeVisual.pos++, model.timeVisual.startTime);
		//fprintf(model.timeVisual.filVisuell, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", x, y);
		fprintf(model.timeVisual.filVisuell, "    \"geometry\":{\"type\": \"LineString\", \"coordinates\":["
			"[%.4lf,%.4lf], [%.4lf,%.4lf]]}}\n", model.timeVisual.oldX, model.timeVisual.oldY, x, y);
		model.timeVisual.oldX = x;
		model.timeVisual.oldY = y;
	}
	return 0;
}

double getBearing(double b1, double b2, double factor) {
	double diff = abs(b1 - b2);
	if (diff <= 180)
		return b1 * (1 - factor) + b2 * factor;

	if (b1 < b2)
		b1 += 360;
	else
		b2 += 360;
	diff = b1 * (1 - factor) + b2 * factor;
	if (diff > 360)
		diff -= 360;
	return diff;
}
double oppositDirection(double b1) {
	double b = b1 - 180;
	if (b < 0)
		b += 360;
	return b;
}

double getPartOfBearing(double b, int i, int nSteps) {
	double b1 = b - 90 + 180 * i / (double)nSteps;
	if (b1 < 0)
		b1 += 360;
	if (b1 > 360)
		b1 -= 360;
	return b1;
}

int plotStormTimeVisuellt(double time, double x, double y, double bearing, double r, double speed) {
	spherical::Point p, p2, p0;
	if (model.timeVisual.pos > 1)
		fprintf(model.timeVisual.filVisuell, ",\n");
	(model.timeVisual.pos)++;

	setTMtime(&(model.timeVisual.tmBas), time);
	fixReadableDate(model.timeVisual.tmBas, model.timeVisual.startTime);
	fprintf(model.timeVisual.filVisuell, "{\"type\":\"Feature\", \"properties\":{\n\"pos\":%d, "
		"\"maxWindSpeed\": %.1lf, \"startTime\":\"%s\"},\n",
		model.timeVisual.pos++, speed, model.timeVisual.startTime);

	double b1;
	int nSteps = 9;

	p = spherical::Point(y, x);

	fprintf(model.timeVisual.filVisuell, "    \"geometry\":{\"type\": \"LineString\", \"coordinates\":[");
	for (int i = 0; i <= nSteps; i++) {
		b1 = getPartOfBearing(bearing, i, nSteps);
		if (i == 0) {
			p0 = p.destinationPoint(r * 1000, b1);
			fprintf(model.timeVisual.filVisuell, "[%.4lf,%.4lf]",
				p0.longitude().degrees(), p0.latitude().degrees());
		}
		else {
			p2 = p.destinationPoint(r * 1000, b1);
			fprintf(model.timeVisual.filVisuell, ", [%.4lf,%.4lf]",
				p2.longitude().degrees(), p2.latitude().degrees());
		}
	}
	fprintf(model.timeVisual.filVisuell, ", [%.4lf,%.4lf]]}}\n", 
		p0.longitude().degrees(), p0.latitude().degrees());

	return 0;

}

int plotStormPos(int i, int i1) {

	double time1 = model.storms[i].feature[i1 - 1].tidFromStart_h;
	double timeDiff = model.storms[i].feature[i1].tidFromStart_h - time1;
	double x1 = model.storms[i].feature[i1 - 1].lon;
	double xDiff = model.storms[i].feature[i1].lon - x1;
	double y1 = model.storms[i].feature[i1 - 1].lat;
	double yDiff = model.storms[i].feature[i1].lat - y1;
	double b1 = model.storms[i].feature[i1 - 1].bearing;
	double b2 = model.storms[i].feature[i1].bearing;
	double r1 = model.storms[i].feature[i1 - 1].innerCircleForwardSize;
	double r1Diff = model.storms[i].feature[i1].innerCircleForwardSize - r1;
	double r2 = model.storms[i].feature[i1 - 1].innerCircleBackwardsSize;
	double r2Diff = model.storms[i].feature[i1].innerCircleBackwardsSize - r2;
	double s2 = model.storms[i].feature[i1 - 1].maxWind;
	double sDiff = model.storms[i].feature[i1].maxWind - s2;

	
	int timeInt = (int)timeDiff * model.params.simulateTimeVisually_nIntHour;
	if (timeInt < 2) timeInt = 2;
	timeDiff /= timeInt;
	xDiff /= timeInt;
	yDiff /= timeInt;
	r1Diff /= timeInt;
	r2Diff /= timeInt;
	double bearing;

	int iStart = 1;
	if (i1 == 0) iStart = 0;
	for (int i0 = iStart; i0 < timeInt; i0++) {
		bearing = getBearing(b1, b2, i / timeInt);
		plotStormTimeVisuellt(time1 + timeDiff * i0, x1 + xDiff * i0, y1 + yDiff * i0, bearing,
			r1 + r1Diff * i0, s2 + sDiff * i0);
		bearing = oppositDirection(bearing);
		plotStormTimeVisuellt(time1 + timeDiff * i0, x1 + xDiff * i0, y1 + yDiff * i0, bearing,
			r2 + r2Diff * i0, s2 + sDiff * i0);
	}

	return 0;
}

int simuleraStormsVisuellt() {
	sprintf(model.timeVisual.startTime, "%s/stormsVisuellt.geojson", model.params.indataPath.c_str());
	model.timeVisual.filVisuell = fopen(model.timeVisual.startTime, "w");
	initGeoJsonFil(model.timeVisual.filVisuell, "storms");
	model.timeVisual.pos = 0;

	for (int i = 0; i < model.nStorms; i++) {
		for (int i1 = 1; i1 < model.storms[i].nFeatures; i1++) {
			if (model.storms[i].feature[i1].tidFromStart_h < -12)
				continue;
			plotStormPos(i, i1);
		}
	}
	fprintf(model.timeVisual.filVisuell, "]}\n");
	fclose(model.timeVisual.filVisuell);

	return 0;
}







void init_tmBas() {
	time(&(model.params.testTime));
	model.params.tmBas = *localtime(&(model.params.testTime));
	model.params.startTime = (char*)malloc2(256 * sizeof(char));
	model.params.startTime_short = (char*)malloc2(256 * sizeof(char));
	model.params.startTime_full = (char*)malloc2(256 * sizeof(char));
}



double calcNewTime_changeSpeedSetting_delay(int arcDelay, int speedSettingNu, int tidInt, double* calmWaterSpeedNy) {
	//double calcSpeed = modelDelay.arc[arcDelay].distance / modelDelay.arc[arcDelay].time;
	//double calmWaterSpeed = eval_calmWaterSpeed(modelDelay.arc[arcDelay].speedSetting, -1, -100);
	//double factor = calmWaterSpeed / calcSpeed;
	int restrictedAreaNr = get_restrictedAreaNr(modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].fromPointNr,
		modelDelay.arc[arcDelay].outNodePos);

	*calmWaterSpeedNy = eval_calmWaterSpeed(speedSettingNu,
		modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].toLevel, restrictedAreaNr);
	//double timeArc = modelDelay.arc[arcDelay].distance / calmWaterSpeedNy * factor;

	double factorDelay = 1.0, speedDiffCurrent = 0, waiting = 0, fixTime;
	if (modelDelay.arc[arcDelay].fromLevel >= 0) {
		if (delayVersion < 4)
			factorDelay = eval_factorDelayedAlongArc(modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].fromPointNr,
				modelDelay.arc[arcDelay].toLevel, modelDelay.arc[arcDelay].toPointNr, tidInt);
		else {
			if (delayVersion == 4)
				factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].fromPointNr,
					modelDelay.arc[arcDelay].toLevel, modelDelay.arc[arcDelay].toPointNr, tidInt, &speedDiffCurrent, *calmWaterSpeedNy);
			else
				factorDelay = 1.0;
		}
		if (modelDelay.arc[arcDelay].fromLevel >= 0)
			waiting = model.network.physicalLev[modelDelay.arc[arcDelay].fromLevel].tidWait;
	}
	else {
		if (modelDelay.arc[arcDelay].toLevel < 0) {
			waiting = model.network.channel[-modelDelay.arc[arcDelay].fromLevel - 1].waitingTime;
			fixTime = model.network.channel[-modelDelay.arc[arcDelay].fromLevel - 1].timeThroughChannel;
		}
		else
			fixTime = -1;
		if (fixTime < -0.5)
			factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].fromPointNr,
				modelDelay.arc[arcDelay].toLevel, modelDelay.arc[arcDelay].toPointNr, tidInt, &speedDiffCurrent, *calmWaterSpeedNy);
	}

	double speedNow = *calmWaterSpeedNy / factorDelay + speedDiffCurrent;
	// double timeArcCheck = modelDelay.arc[arcDelay].distance / calmWaterSpeedNy * factor;
	double timeArcCheck = modelDelay.arc[arcDelay].distance / speedNow + waiting;




	return timeArcCheck;
}

double calcNewTime_changeSpeedSetting_delay_prefPath(int arcDelay, int speedSettingNu, int tidInt, double* calmWaterSpeedNy) {
	//double calcSpeed = modelDelay.arc[arcDelay].distance / modelDelay.arc[arcDelay].time;
	//double calmWaterSpeed = eval_calmWaterSpeed(modelDelay.arc[arcDelay].speedSetting, -1, -100);
	//double factor = calmWaterSpeed / calcSpeed;
	*calmWaterSpeedNy = model.params.calmWaterSpeedCompareUse; // eval_calmWaterSpeed(speedSettingNu,
		// modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].toLevel);
	//double timeArc = modelDelay.arc[arcDelay].distance / calmWaterSpeedNy * factor;

	double factorDelay = 1.0, speedDiffCurrent = 0, waiting = 0, fixTime;
	if (modelDelay_prefPath.arc[arcDelay].fromLevel >= 0) {
		factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(modelDelay_prefPath.arc[arcDelay].fromLevel, modelDelay_prefPath.arc[arcDelay].fromPointNr,
			modelDelay_prefPath.arc[arcDelay].toLevel, modelDelay_prefPath.arc[arcDelay].toPointNr, tidInt, &speedDiffCurrent, *calmWaterSpeedNy);
		if (modelDelay.arc[arcDelay].fromLevel >= 0)
			waiting = model.network.physicalLev[modelDelay.arc[arcDelay].fromLevel].tidWait;
	}
	else {
		if (modelDelay_prefPath.arc[arcDelay].toLevel < 0) {
			waiting = model.network.channel[-modelDelay_prefPath.arc[arcDelay].fromLevel - 1].waitingTime;
			fixTime = model.network.channel[-modelDelay_prefPath.arc[arcDelay].fromLevel - 1].timeThroughChannel;
		}
		else
			fixTime = -1;
		if (fixTime < -0.5)
			factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(modelDelay_prefPath.arc[arcDelay].fromLevel, modelDelay_prefPath.arc[arcDelay].fromPointNr,
				modelDelay_prefPath.arc[arcDelay].toLevel, modelDelay_prefPath.arc[arcDelay].toPointNr, tidInt, &speedDiffCurrent, *calmWaterSpeedNy);
	}
	double speedNow = *calmWaterSpeedNy / factorDelay + speedDiffCurrent;
	// double timeArcCheck = modelDelay.arc[arcDelay].distance / calmWaterSpeedNy * factor;
	double timeArcCheck = modelDelay_prefPath.arc[arcDelay].distance / speedNow + waiting;




	return timeArcCheck;
}

int setAllSpeedAltOK_ifPossible_level(int arcDelay, int posDelay){
	int chosenSetting = modelDelay.arc[arcDelay].speedSetting;
	int speedSetting, legNr;
	int lev1 = modelDelay.arc[arcDelay].fromLevel;
	int lev2 = modelDelay.arc[arcDelay].toLevel;
	
	int i, pos;
	strSpeed* speedLevel;

	if(lev1 >= 0)
		legNr = model.network.physicalLev[lev1].legNr;
	else
		legNr = model.network.channel[-lev1 - 1].legNr;

	if (model.functions.nAllocShipSpeedsLevel[legNr] < model.functions.nShip_speedSettingsBase) {
		speedSetting = getClosestSetting_fromBase(chosenSetting, lev1, lev2) + 
			model.delay.changedSpeed[posDelay];
	}
	else {
		if (lev1 >= 0) {
			speedLevel = model.functions.speedLevel;
			pos = lev1;
		}
		else {
			if (lev2 >= 0) {
				speedLevel = model.functions.speedChannelOut;
				pos = -lev1 - 1;
			}
			else {
				speedLevel = model.functions.speedChannel;
				pos = -lev1 - 1;
				if (model.network.channel[pos].timeThroughChannel > 0)
					return chosenSetting; // do not change the number of speed settings for this one
			}
		}
		checkMinnesAnvandning(__LINE__);
		for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
			set_speedSettingsFromBase(&(speedLevel[pos]), i, i);
		}
		speedLevel[pos].nShip_speedSettings = model.functions.nShip_speedSettingsBase;
		speedSetting = chosenSetting + model.delay.changedSpeed[posDelay];
	}
	return speedSetting;
}

int copyToArcFromDelay(int posDelay, int arcNr, double timeExact, int iter, int fullCalc) {
	int arcNu = model.nArcs, arcDelay, thisLevel, pos1, speedSetting, cNr, legNr;
	int restrictedAreaNr;
	double emission, newTime;
	double fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux;
	double fuelQualityKvot, fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, totCost;
	double extraAreaCostKvot, kvotCost, calmWaterSpeed, posDiff;

	thisLevel = model.arc[arcNr].fromLevel;
	if (thisLevel == 77)
		thisLevel = thisLevel;
	pos1 = model.arc[arcNr].fromPointNr;
	if (iter != 1) {// && model.results.onlyPrefPath_kaoutar != 1) {
		if (thisLevel >= 0)
			arcDelay = model.delayRouteToEnd[thisLevel][pos1].BVArc[posDelay];
		else
			arcDelay = model.delayRouteToEnd_channel[-thisLevel - 1][pos1].BVArc[posDelay];
		if (modelDelay.arc[arcDelay].fromLevel == 77)
			arcDelay = arcDelay;
		model.arc[arcNu].fromLevel = modelDelay.arc[arcDelay].fromLevel;
		model.arc[arcNu].toLevel = modelDelay.arc[arcDelay].toLevel;
		model.arc[arcNu].fromPointNr = modelDelay.arc[arcDelay].fromPointNr;
		model.arc[arcNu].outNodePos = modelDelay.arc[arcDelay].outNodePos;

		model.arc[arcNu].toPointNr = modelDelay.arc[arcDelay].toPointNr;
		model.arc[arcNu].distance = modelDelay.arc[arcDelay].distance;
		if (fullCalc == 0)
			return 0; // don't need all the other stuff
		//model.arc[arcNu].speedSetting = modelDelay.arc[arcDelay].speedSetting + model.delay.changedSpeed[posDelay];

		//speedSetting = getClosestSetting_fromBase(modelDelay.arc[arcDelay].speedSetting,
		//	modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].toLevel) + model.delay.changedSpeed[posDelay];
		speedSetting = setAllSpeedAltOK_ifPossible_level(arcDelay, posDelay);
		checkMinnesAnvandning(__LINE__);

		newTime = calcNewTime_changeSpeedSetting_delay(arcDelay, speedSetting, (int)(round((timeExact + model.functions.valuesNow.deltaArcStart) / model.params.tIndexGerH)), &calmWaterSpeed);
		fuelQualityKvot = modelDelay.arc[arcDelay].fuelQualityKvot;
		extraAreaCostKvot = modelDelay.arc[arcDelay].extraAreaCostKvot;
		kvotCost = modelDelay.arc[arcDelay].kvotCost;
	}
	else {
		if (thisLevel >= 0)
			arcDelay = model.delayRouteToEnd_prefPath[thisLevel][pos1].BVArc[posDelay];
		else
			arcDelay = model.delayRouteToEnd_channel_prefPath[-thisLevel - 1][pos1].BVArc[posDelay];
		model.arc[arcNu].fromLevel = modelDelay_prefPath.arc[arcDelay].fromLevel;
		model.arc[arcNu].toLevel = modelDelay_prefPath.arc[arcDelay].toLevel;
		model.arc[arcNu].fromPointNr = modelDelay_prefPath.arc[arcDelay].fromPointNr;
		model.arc[arcNu].outNodePos = modelDelay_prefPath.arc[arcDelay].outNodePos;
		model.arc[arcNu].toPointNr = modelDelay_prefPath.arc[arcDelay].toPointNr;
		model.arc[arcNu].distance = modelDelay_prefPath.arc[arcDelay].distance;
		if (fullCalc == 0)
			return 0; // don't need all the other stuff
		//model.arc[arcNu].speedSetting = modelDelay.arc[arcDelay].speedSetting + model.delay.changedSpeed[posDelay];
		speedSetting = 0;
		newTime = calcNewTime_changeSpeedSetting_delay_prefPath(arcDelay, speedSetting, (int)(round((timeExact + model.functions.valuesNow.deltaArcStart) / model.params.tIndexGerH)), &calmWaterSpeed);

		fuelQualityKvot = modelDelay_prefPath.arc[arcDelay].fuelQualityKvot;
		extraAreaCostKvot = modelDelay_prefPath.arc[arcDelay].extraAreaCostKvot;
		kvotCost = modelDelay_prefPath.arc[arcDelay].kvotCost;
	}

	model.arc[arcNu].speedSetting = speedSetting;
//	if (model.delay.changedSpeed[posDelay] != 0) {
		model.arc[arcNu].time = newTime;

		//speedSetting = getClosestSetting_fromBase(modelDelay.arc[arcDelay].speedSetting,
		//	modelDelay.arc[arcDelay].fromLevel, modelDelay.arc[arcDelay].toLevel);
		//fuelConsumption_main = eval_fuelConsumption_both(speedSetting + model.delay.changedSpeed[posDelay], &fuelConsumption_aux,
		//	model.arc[arcNu].fromLevel, model.arc[arcNu].toLevel);
		restrictedAreaNr = get_restrictedAreaNr(model.arc[arcNu].fromLevel, model.arc[arcNu].fromPointNr,
			model.arc[arcNu].outNodePos);
		fuelConsumption_main = eval_fuelConsumption_both(speedSetting, &fuelConsumption_aux,
			model.arc[arcNu].fromLevel, model.arc[arcNu].toLevel, restrictedAreaNr, calmWaterSpeed, 1.0);
		fuelUsage_aux = fuelConsumption_aux * newTime;
		fuelUsage_main = fuelConsumption_main * newTime;
		if (model.arc[arcNu].toLevel < 0 && model.arc[arcNu].fromLevel < 0) {
			cNr = -model.arc[arcNu].toLevel - 1;
			if (model.network.channel[cNr].totalConsumption >= 0) {
				// fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_use, &fuelConsumption_aux, -1, -100);
				//fuelUsage_main = model.network.channel[cNr].totalConsumption;
				// fuelUsage_aux = fuelConsumption_aux * newTime;
			}
			fuelUsage_main -= fuelConsumption_main * model.network.channel[cNr].waitingTime;
			fuelUsage_main += model.network.channel[cNr].waiting_consumption_main;
			fuelUsage_aux -= fuelConsumption_aux * model.network.channel[cNr].waitingTime;
			fuelUsage_aux += model.network.channel[cNr].waiting_consumption_aux;
		}
		if (model.arc[arcNu].fromLevel >= 0) {
			fuelUsage_main -= fuelConsumption_main * model.network.physicalLev[model.arc[arcNu].fromLevel].tidWait;
			//fuelUsage_aux -= fuelConsumption_aux * model.network.physicalLev[model.arc[arcNu].fromLevel].tidWait;
		}

		//if (iter != 1) {
		//}
		//else {
		//	fuelQualityKvot = modelDelay_prefPath.arc[arcDelay].fuelQualityKvot;
		//	extraAreaCostKvot = modelDelay_prefPath.arc[arcDelay].extraAreaCostKvot;
		//	model.arc[arcNu].distance = modelDelay_prefPath.arc[arcDelay].distance;
		//}
		model.arc[arcNu].fuelQualityKvot = fuelQualityKvot;
		model.arc[arcNu].extraAreaCostKvot = extraAreaCostKvot;
		model.arc[arcNu].maxWindSpeed = 0;
		model.arc[arcNu].maxWaveHeight = 0;

		model.arc[arcNu].kvotCost = kvotCost;
		fuel_eca = fuelUsage_main * (1 - fuelQualityKvot);
		fuel_noEca = fuelUsage_main * fuelQualityKvot;
		fuel_aux = fuelUsage_aux * fuelQualityKvot;
		fuel_auxEca = fuelUsage_aux * (1 - fuelQualityKvot);
		fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
			fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price);

		emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
			fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

		legNr = getLegNrFromLevels(model.arc[arcNu].fromLevel, model.arc[arcNu].toLevel);
		totCost = model.params.legWeights[legNr].weightTime * model.params.priceTime * newTime +
			model.params.legWeights[legNr].weightFuel * fuelBase + emission * model.params.legWeights[legNr].weightEmission * model.params.scaleObjEmission;

		if (model.params.useSimulering == 1) {
			//if (model.functions.valuesNow.maxWaveHeight > model.params.user_maxWaveHeight)
			//	totCost += model.simulering.penOverWeatherLimit_fix +
			//	(model.functions.valuesNow.maxWaveHeight - model.params.user_maxWaveHeight) * model.simulering.penOverMaxWaveHeight_m;
			//if (model.functions.valuesNow.maxWindSpeed > model.params.user_maxWindSpeed_kmh)
			//	totCost += model.simulering.penOverWeatherLimit_fix +
			//	(model.functions.valuesNow.maxWindSpeed - model.params.user_maxWindSpeed_kmh) * model.simulering.penOverMaxWindSpeed_kmh;
			if (thisLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[thisLevel] - pos1);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (model.arc[arcNr].toLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[model.arc[arcNr].toLevel] - model.arc[arcNr].toPointNr);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (model.params.simulationSpeed_kmh > 0)
				totCost += abs(calmWaterSpeed - model.params.simulationSpeed_kmh) * model.simulering.penDeviateSpeed_kmh;
		}

		totCost *= (1 + extraAreaCostKvot);
		if (USE_KVOTKOST == 1) {
			if (USE_KVOTCOST_CORRIDORS == 1)
				totCost *= kvotCost;
			else
				totCost -= kvotCost;
		}

		model.arc[arcNu].fuelBase = fuelBase;
		model.arc[arcNu].fuel_aux = fuel_aux;
		model.arc[arcNu].fuel_auxEca = fuel_auxEca;
		model.arc[arcNu].fuel_eca = fuel_eca;
		model.arc[arcNu].fuel_noEca = fuel_noEca;
		model.arc[arcNu].totCost = totCost;
	//}
	//else {
	//	model.arc[arcNu].time = modelDelay.arc[arcDelay].time;
	//	model.arc[arcNu].fuelBase = modelDelay.arc[arcDelay].fuelBase;
	//	model.arc[arcNu].fuel_aux = modelDelay.arc[arcDelay].fuel_aux;
	//	model.arc[arcNu].fuel_auxEca = modelDelay.arc[arcDelay].fuel_auxEca;
	//	model.arc[arcNu].fuel_eca = modelDelay.arc[arcDelay].fuel_eca;
	//	model.arc[arcNu].fuel_noEca = modelDelay.arc[arcDelay].fuel_noEca;
	//	model.arc[arcNu].totCost = modelDelay.arc[arcDelay].totCost;

	//	emission = model.arc[arcNu].fuel_aux * model.params.fuel.aux_noEca.emissionFactor + model.arc[arcNu].fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
	//		model.arc[arcNu].fuel_eca * model.params.fuel.main_eca.emissionFactor + model.arc[arcNu].fuel_noEca * model.params.fuel.main_noEca.emissionFactor;
	//}
	if (arcNu == 272303)
		arcNu = arcNu;

	model.arc[arcNu].emission = emission;
	model.arc[arcNu].maxWaveHeight = 0;
	model.arc[arcNu].maxWindSpeed = 0;

	model.arc[arcNu].safetyBase = 0;
	model.arc[arcNu].safetyHurricane = 0;
	model.arc[arcNu].bowSlam = 0;
	model.arc[arcNu].greenWater = 0;
	model.arc[arcNu].dynamicStability = 0;
	model.arc[arcNu].rolling = 0;
	model.arc[arcNu].surfRiding = 0;

	model.arc[arcNu].fromTime = (int)(round(timeExact / model.params.tIndexGerH));
	model.arc[arcNu].toTime = (int)(round(model.arc[arcNu].fromTime + model.arc[arcNu].time / model.params.tIndexGerH));

	return 0;
}

int determineBastSpeedDelay_routeToEnd_eta_arc(int arcNr, double tidp) {
	int thisLevel = model.arc[arcNr].fromLevel;
	int pos1 = model.arc[arcNr].fromPointNr;
	if (thisLevel >= 0)
		determineBastSpeedDelay_routeToEnd_eta(&(model.delayRouteToEnd[thisLevel][pos1]), tidp);
	else
		determineBastSpeedDelay_routeToEnd_eta(&(model.delayRouteToEnd_channel[-thisLevel - 1][pos1]), tidp);
	return 0;
}


void addStatisticsSafety(int arcNr) {
	double dist = model.arc[arcNr].distance / model.params.knots_to_km;

	if (model.params.includeNazanin_safety >= 1) {
		if (model.arc[arcNr].bowSlam > 0.999)
			model.results.bowSlam_notAllowed += dist;
		model.results.bowSlam_aver += model.arc[arcNr].bowSlam;
		if (model.arc[arcNr].bowSlam > 0.00001) {
			(model.results.bowSlam_0)++;// += dist;
			if (model.arc[arcNr].bowSlam > 0.01) {
				(model.results.bowSlam_01)++;// += dist;
				if (model.arc[arcNr].bowSlam > 0.05) {
					(model.results.bowSlam_05)++;// += dist;
					if (model.arc[arcNr].bowSlam > 0.2) {
						(model.results.bowSlam_2)++;// += dist;
					}
				}
			}
		}

		if (model.arc[arcNr].greenWater > 0.999)
			model.results.greenWater_notAllowed += dist;
		model.results.greenWater_aver += model.arc[arcNr].greenWater;
		if (model.arc[arcNr].greenWater > 0.00001) {
			(model.results.greenWater_0)++;
			if (model.arc[arcNr].greenWater > 0.01) {
				(model.results.greenWater_01)++;// += dist;
				if (model.arc[arcNr].greenWater > 0.05) {
					(model.results.greenWater_05)++; // += dist;
					if (model.arc[arcNr].greenWater > 0.2) {
						(model.results.greenWater_2)++; // += dist;
					}
				}
			}
		}

		if (model.arc[arcNr].dynamicStability > 0.999)
			model.results.dynamicStability_notAllowed += dist;
		model.results.dynamicStability_aver += model.arc[arcNr].dynamicStability;
		if (model.arc[arcNr].dynamicStability > 0.00001) {
			(model.results.dynamicStability_0)++;// += dist;
			if (model.arc[arcNr].dynamicStability > 0.01) {
				(model.results.dynamicStability_01)++;// += dist;
				if (model.arc[arcNr].dynamicStability > 0.05) {
					(model.results.dynamicStability_05)++;// += dist;
					if (model.arc[arcNr].dynamicStability > 0.2) {
						(model.results.dynamicStability_2)++;// += dist;
					}
				}
			}
		}

		if (model.arc[arcNr].rolling > 0.999)
			model.results.rolling_notAllowed += dist;
		model.results.rolling_aver += model.arc[arcNr].rolling;
		if (model.arc[arcNr].rolling > 0.00001) {
			(model.results.rolling_0)++;
			if (model.arc[arcNr].rolling > 0.01) {
				(model.results.rolling_01)++;// += dist;
				if (model.arc[arcNr].rolling > 0.05) {
					(model.results.rolling_05)++; // += dist;
					if (model.arc[arcNr].rolling > 0.2) {
						(model.results.rolling_2)++; // += dist;
					}
				}
			}
		}

		if (model.arc[arcNr].surfRiding > 0.999)
			model.results.surfRiding_notAllowed += dist;
		model.results.surfRiding_aver += model.arc[arcNr].surfRiding;
		if (model.arc[arcNr].surfRiding > 0.00001) {
			(model.results.surfRiding_0)++;
			if (model.arc[arcNr].surfRiding > 0.01) {
				(model.results.surfRiding_01)++;// += dist;
				if (model.arc[arcNr].surfRiding > 0.05) {
					(model.results.surfRiding_05)++; // += dist;
					if (model.arc[arcNr].surfRiding > 0.2) {
						(model.results.surfRiding_2)++; // += dist;
					}
				}
			}
		}
	}
	model.results.stormValue_aver += model.arc[arcNr].safetyHurricane; // *dist;
	if (model.arc[arcNr].safetyHurricane > model.results.worstStormValue_max)
		model.results.worstStormValue_max = model.arc[arcNr].safetyHurricane;

	if (model.arc[arcNr].safetyHurricane > 0.001) {
		if (model.arc[arcNr].safetyHurricane > 100.001) {
			model.results.hurricane_insideInnerCircle += dist;
			if (model.results.hurricane_maxCost_insideInnerCircle < model.arc[arcNr].safetyHurricane)
				model.results.hurricane_maxCost_insideInnerCircle = model.arc[arcNr].safetyHurricane;
		}
		else {
			model.results.hurricane_insideOuterCircle += dist;
			if (model.results.hurricane_maxCost_insideOuterCircle < model.arc[arcNr].safetyHurricane)
				model.results.hurricane_maxCost_insideOuterCircle = model.arc[arcNr].safetyHurricane;
		}
	}

	if (model.arc[arcNr].maxWaveHeight > model.params.userLimit_maxWaveHeight)
		model.results.maxWaveHeight_notAllowed += dist;
	if (model.arc[arcNr].maxWindSpeed > model.params.userLimit_maxWindSpeed_kmh)
		model.results.maxWindSpeed_notAllowed += dist;

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

	//printf("date/time2 %s\n", namn);
	return 0;
}

int fixReadableDate_file(struct tm tmBas, char* namn) {
	sprintf(namn, "%d", tmBas.tm_year + 1900);
	if (tmBas.tm_mon + 1 < 10)
		sprintf(namn, "%s_0%d", namn, tmBas.tm_mon + 1);
	else
		sprintf(namn, "%s_%d", namn, tmBas.tm_mon + 1);
	if (tmBas.tm_mday < 10)
		sprintf(namn, "%s_0%d", namn, tmBas.tm_mday);
	else
		sprintf(namn, "%s_%d", namn, tmBas.tm_mday);
	if (tmBas.tm_hour < 10)
		sprintf(namn, "%s_0%d", namn, tmBas.tm_hour);
	else
		sprintf(namn, "%s_%d", namn, tmBas.tm_hour);
	if (tmBas.tm_min < 10)
		sprintf(namn, "%s_0%d", namn, tmBas.tm_min);
	else
		sprintf(namn, "%s_%d", namn, tmBas.tm_min);
	if (tmBas.tm_sec < 10)
		sprintf(namn, "%s_0%d", namn, tmBas.tm_sec);
	else
		sprintf(namn, "%s_%d", namn, tmBas.tm_sec);

	//printf("date/time2 %s\n", namn);
	return 0;
}

int fixReportDate(struct tm tmBas, char* namn) {

	if (tmBas.tm_mday < 10)
		sprintf(namn, "0%d-", tmBas.tm_mday);
	else
		sprintf(namn, "%d-", tmBas.tm_mday);
	if (tmBas.tm_mon == 0)
		sprintf(namn, "%sJan", namn);
	else if (tmBas.tm_mon == 1)
		sprintf(namn, "%sFeb", namn);
	else if (tmBas.tm_mon == 2)
		sprintf(namn, "%sMar", namn);
	else if (tmBas.tm_mon == 3)
		sprintf(namn, "%sApr", namn);
	else if (tmBas.tm_mon == 4)
		sprintf(namn, "%sMay", namn);
	else if (tmBas.tm_mon == 5)
		sprintf(namn, "%sJun", namn);
	else if (tmBas.tm_mon == 6)
		sprintf(namn, "%sJul", namn);
	else if (tmBas.tm_mon == 7)
		sprintf(namn, "%sAug", namn);
	else if (tmBas.tm_mon == 8)
		sprintf(namn, "%sSep", namn);
	else if (tmBas.tm_mon == 9)
		sprintf(namn, "%sOct", namn);
	else if (tmBas.tm_mon == 10)
		sprintf(namn, "%sNov", namn);
	else
		sprintf(namn, "%sDec", namn);
	if (tmBas.tm_hour < 10)
		sprintf(namn, "%s 0%d", namn, tmBas.tm_hour);
	else
		sprintf(namn, "%s %d", namn, tmBas.tm_hour);
	if (tmBas.tm_min < 10)
		sprintf(namn, "%s:0%d", namn, tmBas.tm_min);
	else
		sprintf(namn, "%s:%d", namn, tmBas.tm_min);
	return 0;
}

int fixReportDate_full(struct tm tmBas, char* namn) {

	sprintf(namn, "%d-", tmBas.tm_year + 1900);
	if (tmBas.tm_mon + 1 < 10)
		sprintf(namn, "%s0%d-", namn, tmBas.tm_mon + 1);
	else
		sprintf(namn, "%s%d-", namn, tmBas.tm_mon + 1);

	if (tmBas.tm_mday < 10)
		sprintf(namn, "%s0%dT", namn, tmBas.tm_mday);
	else
		sprintf(namn, "%s%dT", namn, tmBas.tm_mday);
	if (tmBas.tm_hour < 10)
		sprintf(namn, "%s0%d:", namn, tmBas.tm_hour);
	else
		sprintf(namn, "%s%d:", namn, tmBas.tm_hour);
	if (tmBas.tm_min < 10)
		sprintf(namn, "%s0%d:00", namn, tmBas.tm_min);
	else
		sprintf(namn, "%s%d:00", namn, tmBas.tm_min);

	return 0;
}

int fixReportDateNew() {

	if (model.params.tmBas.tm_mday < 10)
		sprintf(model.params.startTime, "0%d-", model.params.tmBas.tm_mday);
	else
		sprintf(model.params.startTime, "%d-", model.params.tmBas.tm_mday);
	if (model.params.tmBas.tm_mon == 0)
		sprintf(model.params.startTime, "%sJan", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 1)
		sprintf(model.params.startTime, "%sFeb", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 2)
		sprintf(model.params.startTime, "%sMar", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 3)
		sprintf(model.params.startTime, "%sApr", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 4)
		sprintf(model.params.startTime, "%sMay", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 5)
		sprintf(model.params.startTime, "%sJun", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 6)
		sprintf(model.params.startTime, "%sJul", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 7)
		sprintf(model.params.startTime, "%sAug", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 8)
		sprintf(model.params.startTime, "%sSep", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 9)
		sprintf(model.params.startTime, "%sOct", model.params.startTime);
	else if (model.params.tmBas.tm_mon == 10)
		sprintf(model.params.startTime, "%sNov", model.params.startTime);
	else
		sprintf(model.params.startTime, "%sDec", model.params.startTime);
	if (model.params.tmBas.tm_hour < 10)
		sprintf(model.params.startTime, "%s 0%d", model.params.startTime, model.params.tmBas.tm_hour);
	else
		sprintf(model.params.startTime, "%s %d", model.params.startTime, model.params.tmBas.tm_hour);
	if (model.params.tmBas.tm_min < 10)
		sprintf(model.params.startTime, "%s:0%d", model.params.startTime, model.params.tmBas.tm_min);
	else
		sprintf(model.params.startTime, "%s:%d", model.params.startTime, model.params.tmBas.tm_min);
	return 0;
}

int fixReportDateNew_full() {

	sprintf(model.params.startTime, "%d-", model.params.tmBas.tm_year + 1900);
	if (model.params.tmBas.tm_mon + 1 < 10)
		sprintf(model.params.startTime, "%s0%d-", model.params.startTime, model.params.tmBas.tm_mon + 1);
	else
		sprintf(model.params.startTime, "%s%d-", model.params.startTime, model.params.tmBas.tm_mon + 1);

	if (model.params.tmBas.tm_mday < 10)
		sprintf(model.params.startTime, "%s0%dT", model.params.startTime, model.params.tmBas.tm_mday);
	else
		sprintf(model.params.startTime, "%s%dT", model.params.startTime, model.params.tmBas.tm_mday);
	if (model.params.tmBas.tm_hour < 10)
		sprintf(model.params.startTime, "%s0%d:", model.params.startTime, model.params.tmBas.tm_hour);
	else
		sprintf(model.params.startTime, "%s%d:", model.params.startTime, model.params.tmBas.tm_hour);
	if (model.params.tmBas.tm_min < 10)
		sprintf(model.params.startTime, "%s0%d:00", model.params.startTime, model.params.tmBas.tm_min);
	else
		sprintf(model.params.startTime, "%s%d:00", model.params.startTime, model.params.tmBas.tm_min);
	return 0;
}

long long make_gmtime_fromStormDateTime(std::string tidpkt) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	int hour, nMinDiffTZ = 0, nValuesHour;

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
		if (tidpkt.substr(15 + nValuesHour, 2) == "AM") {
			if (hour == 12)
				hour = 0;
		}
	}
	tmBas.tm_hour = hour;
	tmBas.tm_min = std::stoi(tidpkt.substr(12 + nValuesHour, 2)) + nMinDiffTZ;
	tmBas.tm_sec = 0;
	time_t test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
#ifdef _WIN32
	time_t rawtime = _mkgmtime(&tmBas);
#endif
#ifndef _WIN32
	time_t rawtime = timegm(&tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}

long long make_gmtime_fromDateTimeString(std::string tidpkt, strParams* params) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	int hour, nHoursDiffTZ = 0, nMinDiffTZ = 0, nValuesHour;

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
	int length = tidpkt.size();
	if (length >= 17 + nValuesHour) {
		if (tidpkt.substr(15 + nValuesHour, 2) == "PM") {
			if (hour < 12)
				hour += 12;
		}
		else {
			if (tidpkt.substr(15 + nValuesHour, 2) == "AM") {
				if (hour == 12)
					hour = 0;
			}
		}
	}
	tmBas.tm_hour = hour;
	tmBas.tm_min = std::stoi(tidpkt.substr(12 + nValuesHour, 2));
	tmBas.tm_sec = 0; // std::stoi(tidpkt.substr(15 + nValuesHour, 2));

	time_t test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}

	if (params != NULL) {
		params->startYear = tmBas.tm_year + 1900;
		params->startMonth_nr = tmBas.tm_mon + 1;
		params->startDay_nr = tmBas.tm_mday;
		params->startHour = tmBas.tm_hour;
		params->startMinute = tmBas.tm_min;
	}

#ifdef _WIN32
	//printf("time %s", tidpkt.c_str());
	if (params != NULL)
		printf(" %d-%d-%d:%d %d",
			params->startYear, params->startMonth_nr, params->startDay_nr,
			params->startHour, params->startMinute);
	printf("\n");
	time_t rawtime = _mkgmtime(&tmBas);
#endif
#ifndef _WIN32
	time_t rawtime = timegm(&tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}


void loadSet_startDateTime(json data, strParams* params) { // not used
	std::string tidpkt = data;
	params->UTC_secondsStart = make_gmtime_fromDateTimeString(tidpkt, params);
}




time_t getFirstSecondOfDay(long long seconds) {
	time_t sec = seconds;
	struct tm* tmBas = gmtime(&sec);
	time_t test = mktime(tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas->tm_year,
			tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}

	tmBas->tm_hour = 0;
	tmBas->tm_min = 0;
	tmBas->tm_sec = 0;
#ifdef _WIN32
	time_t rawtime = _mkgmtime(tmBas);
#endif
#ifndef _WIN32
	time_t rawtime = timegm(tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}

int getManadDagFranUTCSeconds(long long seconds, int* dag) {
	time_t sec = seconds;
	struct tm* tmBas = gmtime(&sec);
	time_t test = mktime(tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas->tm_year,
			tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	*dag = tmBas->tm_mday;
	return tmBas->tm_mon + 1;
}

std::string stringDateFromUTCSeconds(long long seconds) {
	time_t sec = seconds;
	struct tm* tmBas = gmtime(&sec);
	time_t test = mktime(tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d seconds given %I64d\n", __LINE__,
			tmBas->tm_year,
			tmBas->tm_mon, tmBas->tm_mday, tmBas->tm_hour, tmBas->tm_min, tmBas->tm_sec, seconds);
		std::cout << "time_t " << sec << std::endl;
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	char date_string[100];
	strftime(date_string, 50, "%B %d, %Y %T", tmBas);
	std::string resultat = date_string;
	return resultat;
}

int identifyClosestPointInPhysicalLevel(int level, double x, double y) {
	int i, minPos = -1, basPos, battre;
	double minDist = 1e20, dist;

	basPos = model.params.preferredPathOrtoPos[level];
	if (basPos < 0)
		basPos = -basPos - 1;
	minDist = estimateLargeCircleDistance2_km(y, x, model.network.physicalLev[level].point_y[basPos], model.network.physicalLev[level].point_x[basPos]);
	minPos = basPos;

	for (i = 0; i < model.network.physicalLev[level].nPoints / 2; i++) {
		battre = 0;
		if (basPos - i >= 0) {
			dist = estimateLargeCircleDistance2_km(y, x, model.network.physicalLev[level].point_y[basPos - i], model.network.physicalLev[level].point_x[basPos - i]);
			if (dist < minDist) {
				minDist = dist;
				minPos = basPos - i;
			}
			battre = 1;
		}
		if (basPos + i < model.network.physicalLev[level].nPoints ) {
			dist = estimateLargeCircleDistance2_km(y, x, model.network.physicalLev[level].point_y[basPos + i], model.network.physicalLev[level].point_x[basPos + i]);
			if (dist < minDist) {
				minDist = dist;
				minPos = basPos + i;
			}
			battre = 1;
		}
		if (battre == 0)
			break;
	}
	return minPos;
}

double getDiff_angles(double a1, double a2, int alt = 0) {
	double diff;
	
	if(alt == 0)
		diff = a2 - a1;
	else
		diff = a2 - (a1 - M_PI);

	if (diff < -M_PI)
		diff += M_PI2;
	if (diff > M_PI)
		diff -= M_PI2;
	return diff;
}

double getDirChannelTmp(int cNr, int alt) {
	double x1, y1, x2, y2, channelDir, distKrav = 10;
	int i, nCoords;

	if (alt == 0) {
		x1 = model.network.channelTmp[cNr].point_x[0];
		y1 = model.network.channelTmp[cNr].point_y[0];
		for (i = 1; i < model.network.channelTmp[cNr].nPoints - 1; i++) {
			if (model.network.channelTmp[cNr].distanceFromStart[i] > distKrav)
				break;
		}
		x2 = model.network.channelTmp[cNr].point_x[i];
		y2 = model.network.channelTmp[cNr].point_y[i];
		channelDir = atan2(y2 - y1, x2 - x1);
	}
	else {
		nCoords = model.network.channelTmp[cNr].nPoints;
		x2 = model.network.channelTmp[cNr].point_x[nCoords - 1];
		y2 = model.network.channelTmp[cNr].point_y[nCoords - 1];

		for (i = nCoords - 2; i > 0; i--) {
			if (model.network.channelTmp[cNr].distanceFromStart[nCoords - 1] - model.network.channelTmp[cNr].distanceFromStart[i] > distKrav)
				break;
		}
		x1 = model.network.channelTmp[cNr].point_x[i];
		y1 = model.network.channelTmp[cNr].point_y[i];
		channelDir = atan2(y2 - y1, x2 - x1);
	}
	return channelDir;
}

double getDirChannel(int cNr, int alt) {
	double x1, y1, x2, y2, channelDir, distKrav = 10;
	int i, nCoords;

	if (alt == 0) {
		x1 = model.network.channel[cNr].point_x[0];
		y1 = model.network.channel[cNr].point_y[0];
		for (i = 1; i < model.network.channel[cNr].nPoints - 1; i++) {
			if (model.network.channel[cNr].distanceFromStart[i] > distKrav)
				break;
		}
		x2 = model.network.channel[cNr].point_x[i];
		y2 = model.network.channel[cNr].point_y[i];
		channelDir = atan2(y2 - y1, x2 - x1);
	}
	else {
		nCoords = model.network.channel[cNr].nPoints;
		x2 = model.network.channel[cNr].point_x[nCoords - 1];
		y2 = model.network.channel[cNr].point_y[nCoords - 1];

		for (i = nCoords - 2; i > 0; i--) {
			if (model.network.channel[cNr].distanceFromStart[nCoords - 1] - model.network.channel[cNr].distanceFromStart[i] > distKrav)
				break;
		}
		x1 = model.network.channel[cNr].point_x[i];
		y1 = model.network.channel[cNr].point_y[i];
		channelDir = atan2(y2 - y1, x2 - x1);
	}
	return channelDir;
}

int getConnectDirectionStartEndCorridor_ok(int cNr, int alt) {
	int nCoords, returnLevel, i;
	double x, y, x1, y1, x2, y2, channelDir, distKrav = 10;
	double connectDir, diffAngle, angleKrav = M_PI * 2 / 3.0;

	return 1;
	if (alt == 0) {
		x1 = model.network.channel[cNr].point_x[0];
		y1 = model.network.channel[cNr].point_y[0];
		x = model.network.physicalLev[0].point_x[0];
		y = model.network.physicalLev[0].point_y[0];

		for (i = 1; i < model.network.channel[cNr].nPoints - 1; i++) {
			if (model.network.channel[cNr].distanceFromStart[i] > distKrav)
				break;
		}
		x2 = model.network.channel[cNr].point_x[i];
		y2 = model.network.channel[cNr].point_y[i];
		channelDir = atan2(y2 - y1, x2 - x1);
		connectDir = atan2(y1 - y, x1 - x);
		diffAngle = getDiff_angles(channelDir, connectDir);
		if (abs(diffAngle) > angleKrav)
			returnLevel = -1;
		else
			returnLevel = 1;
	}
	else {
		nCoords = model.network.channel[cNr].nPoints;
		x2 = model.network.channel[cNr].point_x[nCoords - 1];
		y2 = model.network.channel[cNr].point_y[nCoords - 1];
		x = model.network.physicalLev[model.network.nPhysicalLevels - 1].point_x[0];
		y = model.network.physicalLev[model.network.nPhysicalLevels - 1].point_y[0];

		for (i = model.network.channel[cNr].nPoints - 2; i > 0; i--) {
			if (model.network.channel[cNr].distanceFromStart[nCoords - 1] - model.network.channel[cNr].distanceFromStart[i] > distKrav)
				break;
		}
		x1 = model.network.channel[cNr].point_x[i];
		y1 = model.network.channel[cNr].point_y[i];
		channelDir = atan2(y2 - y1, x2 - x1);
		connectDir = atan2(y - y2, x - x2);
		diffAngle = getDiff_angles(channelDir, connectDir);
		if (abs(diffAngle) > angleKrav)
			returnLevel = -1;
		else
			returnLevel = 1;
	}

	return returnLevel;
}

int getBastPhysLevelToConnectToChannel(int alt, int cNr) {
	int i, posMid, firstPos = -1, lastPos = -1, bast = -1, bast2 = -1;
	int bastLevel, returnLevel, startPoint, bastPoint, nCoords, bastPos;
	double dist, bastDist = 1e20, bastDist2 = 1e20, x1;
	double x, y, y1;

	identify_startEndOnChannel(cNr, alt);
	if (cNr == 8)
		cNr = cNr;
	if (alt == 0) {
		x = model.network.channel[cNr].point_x[0];
		y = model.network.channel[cNr].point_y[0];
	}
	else {
		nCoords = model.network.channel[cNr].nPoints;
		x = model.network.channel[cNr].point_x[nCoords - 1];
		y = model.network.channel[cNr].point_y[nCoords - 1];
	}

	// startSlut, 0 start, 1 end
	check_translate_xCoord(&x);


	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i == 164)
			i = i;
		posMid = (int)model.network.physicalLev[i].nPoints / 2;

		x1 = model.network.physicalLev[i].point_x[posMid];
		check_translate_xCoord(&x1);
		dist = estimateLargeCircleDistance2_km(y, x, model.network.physicalLev[i].point_y[posMid], x1);
		//printf("in getBastPhysLevelToConnectToChannel cNr %d alt %d xy %.3lf %.3lf %.3lf %.3lf i %d dist %.3lf maxDev %.3lf\n",
		//	cNr, alt, x, y, x1, model.network.physicalLev[i].point_y[posMid],
		//	i, dist, model.params.maxDeviationPreferred_km);
		if (dist <= model.params.maxDeviationPreferred2_km) {
			if (firstPos == -1)
				firstPos = i;
			//if (startPos == 0)
			//	break;
			//traffEnd = 1;
			lastPos = i;
			if (dist < bastDist) {
				bastDist2 = bastDist;
				bast2 = bast;
				bastDist = dist;
				bast = i;
			}
			else {
				if (dist < bastDist2) {
					bastDist2 = dist;
					bast2 = i;
				}
			}
		}
		//else {
			//if (traffEnd == 1) {
			//	lastPos = i - 1;
			//	break;
			//}
		//}
	}
	if (bast == -1 || bast2 == -1) {
		if (alt == 0)
			model.network.channel[cNr].bastStartLevel = -1;
		else
			model.network.channel[cNr].bastEndLevel = -1;
		model.network.channel[cNr].legNr = 0;

		printf("ERROR! Could not find a connection to corridor %d alt %d. bast %d bast2 %d\n", cNr, alt, bast, bast2);
		errlog("ERROR! Could not find a connection to corridor %d alt %d. bast %d bast2 %d\n", cNr, alt, bast, bast2);
		return -1;
	}

	double dirChannel = getDirChannel(cNr, alt);
	double connectDir, diffAngle, angleKrav = M_PI * 11/ 20; // slightly more than 90 degrees

	if (alt == 0) { // start of channel
		if (bast2 > bast || bast2 == -1) {
			//printf("inGetBastPhysLevelToConnectToChannel bast %d bast2 %d bastDist %.2lf bastDist2 %.2lf distFromStartPosMid %.2lf %.2lf\n",
			//	bast, bast2, bastDist, bastDist2, model.network.physicalLev[bast].distanceFromStartPosMid,
			//	model.network.physicalLev[bast2].distanceFromStartPosMid);
			bastLevel = bast;
			//if (bast == 0 && bast2 > 0) {
			//	if (bastDist2 > model.network.physicalLev[bast2].distanceFromStartPosMid - model.network.physicalLev[bast].distanceFromStartPosMid) {
			//		identify_startOnChannel(cNr);
			//	}
			//}
		}
		else
			bastLevel = bast2;
		if (bastLevel == 0) {
			if (getConnectDirectionStartEndCorridor_ok(cNr, alt) == -1)
				bastLevel = -1;
			// sets the bastLevel to -1 if the direction is completely wrong
		}
		model.network.channel[cNr].bastStartLevel = bastLevel;
		if (bastLevel >= 0)
			model.network.channel[cNr].legNr = model.network.physicalLev[bastLevel].legNr;
		else
			model.network.channel[cNr].legNr = model.network.channel[-bastLevel - 1].legNr;
		//if (model.network.channel[cNr].followExactly == 1)
		//	model.network.physicalLev[bastLevel].followChannelExactly = 1;

		bastDist = 1e20;
		for (i = -1; i < model.network.physicalLev[bastLevel].npreferredPathPoints; i++) {
			if (i < 0) {
				x1 = model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]];
				y1 = model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]];
			}
			else {
				x1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].longitude().degrees();
				y1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].latitude().degrees();
			}
			check_translate_xCoord(&x1);
			dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
			if (bastDist > dist) {
				if (dist < 1) {
					bastDist = dist;
					bastPos = i;
				}
				else {
					connectDir = atan2(y - y1, x - x1);
					diffAngle = getDiff_angles(dirChannel, connectDir, alt);
					if (abs(diffAngle) < angleKrav) {
						bastDist = dist;
						bastPos = i;
					}
				}
			}
		}
		if (bastDist < 0.9e20) {
			//if (bastPos >= 0)
			//	bastPos--;
			model.network.channel[cNr].preferredPathPoint_posConnectTo = bastPos;
		}else
			model.network.channel[cNr].preferredPathPoint_posConnectTo = -1;
		model.network.channel[cNr].bastStartDist = bastDist;
		returnLevel = firstPos;
	}
	else { // end of channel
		//printf("-------bast %d bast2 %d\n", bast, bast2);
		if (bast2 > bast)
			bastLevel = bast2;
		else
			bastLevel = bast;
		if (bastLevel > model.network.nPhysicalLevels - 1)
			bastLevel = model.network.nPhysicalLevels - 1;
		if (bastLevel == model.network.nPhysicalLevels - 1) {
			if (getConnectDirectionStartEndCorridor_ok(cNr, alt) == -1)
				bastLevel = -1;
			// sets the bastLevel to -1 if the direction is completely wrong
		}
		model.network.channel[cNr].bastEndLevel = bastLevel;

		if (bastLevel > 0)
			bastLevel--;
		bastDist = 1e20;
		for (i = -1; i < model.network.physicalLev[bastLevel].npreferredPathPoints; i++) {
			if (i < 0) {
				x1 = model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]];
				y1 = model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]];
			}
			else {
				x1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].longitude().degrees();
				y1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].latitude().degrees();
			}
			check_translate_xCoord(&x1);
			dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
			if (bastDist > dist) {
				if (dist < 1) {
					bastDist = dist;
					bastPos = i;
				}
				else {
					connectDir = atan2(y - y1, x - x1);
					diffAngle = getDiff_angles(dirChannel, connectDir, alt);
					if (abs(diffAngle) < angleKrav) {
						bastDist = dist;
						bastPos = i;
					}
				}
			}
		}
		if (bastDist < 0.9e20) {
			if (bastPos < model.network.physicalLev[bastLevel].npreferredPathPoints - 1)
				bastPos++;
			model.network.channel[cNr].preferredPathPoint_posConnectFrom = bastPos;

			if(model.network.channel[cNr].bastStartLevel >= 0 && model.network.channel[cNr].followExactly == 1)
				model.network.physicalLev[model.network.channel[cNr].bastStartLevel].followChannelExactly = 1;
		}
		else {
			model.network.channel[cNr].preferredPathPoint_posConnectFrom = -1;
		}

		//printf("-------bastLevel %d\n", bastLevel);
		returnLevel = lastPos;
	}
	//printf("corridor %d alt %d. bast %d bast2 %d bastLevel %d returnLevel %d\n", cNr, alt, bast, bast2, bastLevel, returnLevel);

	//bastPoint = identifyClosestPointInPhysicalLevel(bastLevel, x, y);
	//startPoint = bastPoint - model.params.max_changeDirection;
	//if (startPoint < 0)
	//	startPoint = 0;
	//for (i = startPoint; i < bastPoint + model.params.max_changeDirection && i < model.network.physicalLev[bastLevel].nPoints; i++)
	//	model.network.physicalLev[bastLevel].nodeConnectedFromChannel[i] = 1;
	//printf("corridor %d alt %d. bastPoint %d startPoint %d nUpDown %d\n", cNr, alt, bastPoint, startPoint, model.params.max_changeDirection);

	return returnLevel;

	//printf("dist %.3lf distFromStartPrev %.3lf startPos %d traffEnd %d i %d nPhysLev %d\n", dist, distFromStartPrev,
	//	startPos, traffEnd, i, model.network.nPhysicalLevels);
	//if (startPos == 0) {
	//	if (i >= model.network.nPhysicalLevels)
	//		return -1; // no physicalLev close enough to the coordinate
	//}
	//else {
	//	if (i >= model.network.nPhysicalLevels) {
	//		if (traffEnd == 0)
	//			return -1;
	//		return model.network.nPhysicalLevels - 1;
	//	}
	//}
	//if (model.network.channel[cNr].allowedPoint[pos] == 0)
	//	continue; // last node in channel not okay

	//return i;


}

double findClosestPointKvotAlongLine(double yp, double xp, double yL1, double xL1, double yL2, double xL2, double* y, double* x) {
	double vx, vy, ux, uy, kvot, kvot1;
	vx = xL2 - xL1;
	vy = yL2 - yL1;
	ux = xL1 - xp;
	uy = yL1 - yp;
	kvot = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
	if (kvot < 0)
		kvot1 = 0;
	else {
		if (kvot > 1)
			kvot1 = 1;
		else
			kvot1 = kvot;
	}
	*x = xL1 * (1 - kvot1) + kvot1 * xL2;
	*y = yL1 * (1 - kvot1) + kvot1 * yL2;
	//dist = estimateLargeCircleDistance_km(yp1, xp1, yp2, xp2);
	return kvot;
}

int check_isStraightLineFeasible(double y1, double x1, double y2, double x2)
{

	int isOk = check_physicalMap_ok(y1, x1, y2, x2, 0);

	if (isOk == 1) {
		for (int i = 0; i < model.nExtraNoGoAreas; i++) {
			isOk = check_extraNoGoMap_ok(y1, x1, y2, x2, 0, i);
			if (isOk == 0)
				break;
		}
		if (isOk == 1) {
			isOk = check_noGoPolygons_ok(y1, x1, y2, x2);
		}
	}

	return isOk;
}

int findClosestNextPointAlongPrefPath(int cNr, int posTss, int levelPrefP, int riktning, int* levelNy, int* posPrefP) {
	int i, i1, startP, lastLev = levelPrefP, lastPos = 0, okAngleBefore = 0;
	double x1, y1, x, y, dist, lastDist = 1e20, bastDist = 1e20;
	double dirChannel, connectDir, diffAngle, angleKrav = M_PI * 11 / 20; // slightly more than 90 degrees


	*levelNy = -1;
	x = model.network.channel[cNr].point_x[posTss];
	y = model.network.channel[cNr].point_y[posTss];
	dirChannel = getDirChannel(cNr, riktning);

	startP = 0;
	if (riktning == 1) {
		if (levelPrefP > 0) {
			levelPrefP--;
			startP = model.network.physicalLev[levelPrefP].npreferredPathPoints - 1;
		}
		else
			startP = 0;
		for (i = levelPrefP; i >= 0; i--) {
			for (i1 = startP; i1 >= 0; i1--) {
				x1 = model.network.physicalLev[i].preferredPathPoint[i1].longitude().degrees();
				y1 = model.network.physicalLev[i].preferredPathPoint[i1].latitude().degrees();
				dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
				if (dist < 1) {
					*levelNy = lastLev;
					*posPrefP = lastPos;
					return 0;
				}
				connectDir = atan2(y1 - y, x1 - x);
				diffAngle = getDiff_angles(dirChannel, connectDir, riktning);
				//if (dist < 1 || dist > lastDist || (abs(diffAngle) < angleKrav && okAngleBefore == 1)) {
				//	if (dist < bastDist) {
				//		bastDist = dist;
				//		*levelNy = lastLev;
				//		*posPrefP = lastPos;
				//		if (dist < 1)
				//			return 0;
				//	}
				//}
				lastLev = i;
				lastPos = i1;
				lastDist = dist;
				//if (abs(diffAngle) >= angleKrav)
				//	okAngleBefore = 1;
				if (abs(diffAngle) >= angleKrav && dist < bastDist) {
					bastDist = dist;
					*levelNy = i;
					*posPrefP = i1;
				}
			}
			if (*levelNy != -1) {
				return 0;
			}
			if(i > 0)
				startP = model.network.physicalLev[i - 1].npreferredPathPoints - 1;
		}
	}
	else {
		if (levelPrefP > 0) {
			levelPrefP--;
			startP = model.network.physicalLev[levelPrefP].npreferredPathPoints - 1;
		}
		else
			startP = 0;
		for (i = levelPrefP; i < model.network.nPhysicalLevels; i++) {
			for (i1 = startP; i1 < model.network.physicalLev[i].npreferredPathPoints; i1++) {
				x1 = model.network.physicalLev[i].preferredPathPoint[i1].longitude().degrees();
				y1 = model.network.physicalLev[i].preferredPathPoint[i1].latitude().degrees();
				dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
				if (dist < 1) {
					*levelNy = lastLev;
					*posPrefP = lastPos;
					return 0;
				}
				connectDir = atan2(y1 - y, x1 - x);
				diffAngle = getDiff_angles(dirChannel, connectDir, riktning);
				//if (dist < 1 || dist > lastDist || (abs(diffAngle) < angleKrav && okAngleBefore == 1)) {
				//	bastDist = dist;
				//	*levelNy = lastLev;
				//	*posPrefP = lastPos;
				//	return 0;
				//}
				lastLev = i;
				lastPos = i1;
				lastDist = dist;
				//if (abs(diffAngle) >= angleKrav)
				//	okAngleBefore = 1;
				if (abs(diffAngle) >= angleKrav && dist < bastDist) {
					bastDist = dist;
					*levelNy = i;
					*posPrefP = i1;
				}
			}
			if (*levelNy != -1) {
				return 0;
			}
			startP = 0;
		}
	}
	return -1;
}

int extendTssAlongPrefPath(int cNr, int posTss, int* levelPrefP, int alt) {
	int i, level1, posPP, i1, nAlloc, startP, pos;
	double totDist;
	// find next point (closest to posTss point, along prefPath => level + pos
	if (cNr == 8)
		alt = alt;
	findClosestNextPointAlongPrefPath(cNr, posTss, *levelPrefP, alt, &level1, &startP);
	if (level1 < 0 || level1 >= model.network.nPhysicalLevels - 1) {
		*levelPrefP = -1;
		return -1;
	}

	nAlloc = model.network.channel[cNr].nPoints;
	// add the points and update the tss
	if (alt == 1) {
		nAlloc += model.network.physicalLev[level1].npreferredPathPoints - startP;
		model.network.channel[cNr].point_x = (double*)realloc(model.network.channel[cNr].point_x, nAlloc * sizeof(double));
		model.network.channel[cNr].point_y = (double*)realloc(model.network.channel[cNr].point_y, nAlloc * sizeof(double));
		model.network.channel[cNr].distanceFromStart = (double*)realloc(model.network.channel[cNr].distanceFromStart, nAlloc * sizeof(double));
		model.network.channel[cNr].point = (spherical::Point*)realloc(model.network.channel[cNr].point, nAlloc * sizeof(spherical::Point));
		pos = model.network.channel[cNr].nPoints;
		totDist = model.network.channel[cNr].distance_km;
		for (i1 = startP; i1 < model.network.physicalLev[level1].npreferredPathPoints; i1++) {
			model.network.channel[cNr].point_x[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].longitude().degrees();
			model.network.channel[cNr].point_y[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].latitude().degrees();
			model.network.channel[cNr].point[pos] = spherical::Point(model.network.channel[cNr].point_y[pos], model.network.channel[cNr].point_x[pos]);
			totDist += model.network.channel[cNr].point[pos].distanceTo(model.network.channel[cNr].point[pos - 1]) / 1000.0;
			model.network.channel[cNr].distanceFromStart[pos] = totDist;
			pos++;
		}
		*levelPrefP = level1 + 1;
		model.network.channel[cNr].distance_km = model.network.channel[cNr].distanceFromStart[pos - 1];
		model.network.channel[cNr].nPoints = pos;
		model.network.channel[cNr].bastEndPointPos = 0;
		model.network.channel[cNr].preferredPathPoint_posConnectFrom = -1;
		model.network.channel[cNr].bastEndDist = 0;
	}
	else {
		nAlloc += startP + 2;
		model.network.channel[cNr].point_x = (double*)realloc(model.network.channel[cNr].point_x, nAlloc * sizeof(double));
		model.network.channel[cNr].point_y = (double*)realloc(model.network.channel[cNr].point_y, nAlloc * sizeof(double));
		model.network.channel[cNr].distanceFromStart = (double*)realloc(model.network.channel[cNr].distanceFromStart, nAlloc * sizeof(double));
		model.network.channel[cNr].point = (spherical::Point*)realloc(model.network.channel[cNr].point, nAlloc * sizeof(spherical::Point));
		pos = startP + 2;
		for (i1 = model.network.channel[cNr].nPoints - 1; i1 >= 0; i1--) {
			model.network.channel[cNr].point_x[i1 + pos] = model.network.channel[cNr].point_x[i1];
			model.network.channel[cNr].point_y[i1 + pos] = model.network.channel[cNr].point_y[i1];
			model.network.channel[cNr].point[i1 + pos] = spherical::Point(model.network.channel[cNr].point_y[i1 + pos], model.network.channel[cNr].point_x[i1 + pos]);
			model.network.channel[cNr].distanceFromStart[i1 + pos] = model.network.channel[cNr].distanceFromStart[i1];
		}
		totDist = 0;
		pos = 0;
		model.network.channel[cNr].point_x[pos] = model.network.physicalLev[level1].point_x[model.params.preferredPathOrtoPos[level1]];
		model.network.channel[cNr].point_y[pos] = model.network.physicalLev[level1].point_y[model.params.preferredPathOrtoPos[level1]];
		model.network.channel[cNr].point[pos] = spherical::Point(model.network.channel[cNr].point_y[pos], model.network.channel[cNr].point_x[pos]);
		*levelPrefP = level1;
		pos++;
		for (i1 = 0; i1 <= startP; i1++) {
			model.network.channel[cNr].point_x[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].longitude().degrees();
			model.network.channel[cNr].point_y[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].latitude().degrees();
			model.network.channel[cNr].point[pos] = spherical::Point(model.network.channel[cNr].point_y[pos], model.network.channel[cNr].point_x[pos]);
			totDist += model.network.channel[cNr].point[pos].distanceTo(model.network.channel[cNr].point[pos - 1]) / 1000.0;
			model.network.channel[cNr].distanceFromStart[pos] = totDist;
			pos++;
		}
		model.network.channel[cNr].nPoints += startP + 2;
		for (i1 = startP + 2; i1 < model.network.channel[cNr].nPoints; i1++) {
			if (i1 == startP + 2)
				totDist += model.network.channel[cNr].point[i1].distanceTo(model.network.channel[cNr].point[i1 - 1]) / 1000.0;
			model.network.channel[cNr].distanceFromStart[i1] += totDist;
		}
		model.network.channel[cNr].distance_km = model.network.channel[cNr].distanceFromStart[i1 - 1];
		model.network.channel[cNr].bastStartPointPos = 0;
		model.network.channel[cNr].preferredPathPoint_posConnectTo = -1;
		model.network.channel[cNr].bastStartDist = 1e20;
		model.network.channel[cNr].bastStartDist = 0;
	}

	return 0;
}

int extendTssAlongPrefPath_old(int cNr, int posTss, int* levelPrefP, int alt) {
	int i, level1, posPP, i1, nAlloc, startP, pos;
	double totDist;
	// find next point (closest to posTss point, along prefPath => level + pos
	findClosestNextPointAlongPrefPath(cNr, posTss, *levelPrefP, alt, &level1, &startP);

	nAlloc = model.network.channelTmp[cNr].nPoints;
	// add the points and update the tss
	if (alt == 1) {
		nAlloc += model.network.physicalLev[level1].npreferredPathPoints - startP;
		model.network.channelTmp[cNr].point_x = (double*)realloc(model.network.channelTmp[cNr].point_x, nAlloc * sizeof(double));
		model.network.channelTmp[cNr].point_y = (double*)realloc(model.network.channelTmp[cNr].point_y, nAlloc * sizeof(double));
		model.network.channelTmp[cNr].distanceFromStart = (double*)realloc(model.network.channelTmp[cNr].distanceFromStart, nAlloc * sizeof(double));
		model.network.channelTmp[cNr].point = (spherical::Point*)realloc(model.network.channelTmp[cNr].point, nAlloc * sizeof(spherical::Point));
		pos = model.network.channelTmp[cNr].nPoints;
		totDist = model.network.channelTmp[cNr].distance_km;
		for (i1 = startP; i1 < model.network.physicalLev[level1].npreferredPathPoints; i1++) {
			model.network.channelTmp[cNr].point_x[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].longitude().degrees();
			model.network.channelTmp[cNr].point_y[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].latitude().degrees();
			model.network.channelTmp[cNr].point[pos] = spherical::Point(model.network.channelTmp[cNr].point_y[pos], model.network.channelTmp[cNr].point_x[pos]);
			totDist += model.network.channelTmp[cNr].point[pos].distanceTo(model.network.channelTmp[cNr].point[pos - 1]) / 1000.0;
			model.network.channelTmp[cNr].distanceFromStart[pos] = totDist;
			pos++;
		}
		*levelPrefP = level1 + 1;
		model.network.channelTmp[cNr].distance_km = model.network.channelTmp[cNr].distanceFromStart[pos - 1];
		model.network.channelTmp[cNr].nPoints = pos;
		model.network.channelTmp[cNr].bastEndPointPos= 0;
		model.network.channelTmp[cNr].preferredPathPoint_posConnectFrom = -1;
		model.network.channelTmp[cNr].bastEndDist = 0;
	}
	else {
		nAlloc += startP + 2;
		model.network.channelTmp[cNr].point_x = (double*)realloc(model.network.channelTmp[cNr].point_x, nAlloc * sizeof(double));
		model.network.channelTmp[cNr].point_y = (double*)realloc(model.network.channelTmp[cNr].point_y, nAlloc * sizeof(double));
		model.network.channelTmp[cNr].distanceFromStart = (double*)realloc(model.network.channelTmp[cNr].distanceFromStart, nAlloc * sizeof(double));
		model.network.channelTmp[cNr].point = (spherical::Point*)realloc(model.network.channelTmp[cNr].point, nAlloc * sizeof(spherical::Point));
		pos = startP + 2;
		for (i1 = model.network.channelTmp[cNr].nPoints - 1; i1 >= 0; i1--) {
			model.network.channelTmp[cNr].point_x[i1 + pos] = model.network.channelTmp[cNr].point_x[i1];
			model.network.channelTmp[cNr].point_y[i1 + pos] = model.network.channelTmp[cNr].point_y[i1];
			model.network.channelTmp[cNr].point[i1 + pos] = spherical::Point(model.network.channelTmp[cNr].point_y[i1 + pos], model.network.channelTmp[cNr].point_x[i1 + pos]);
			model.network.channelTmp[cNr].distanceFromStart[i1 + pos] = model.network.channelTmp[cNr].distanceFromStart[i1];
		}
		totDist = 0;
		pos = 0;
		model.network.channelTmp[cNr].point_x[pos] = model.network.physicalLev[level1].point_x[model.params.preferredPathOrtoPos[level1]];
		model.network.channelTmp[cNr].point_y[pos] = model.network.physicalLev[level1].point_y[model.params.preferredPathOrtoPos[level1]];
		model.network.channelTmp[cNr].point[pos] = spherical::Point(model.network.channelTmp[cNr].point_y[pos], model.network.channelTmp[cNr].point_x[pos]);
		*levelPrefP = level1;
		pos++;
		for (i1 = 0; i1 <= startP; i1++) {
			model.network.channelTmp[cNr].point_x[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].longitude().degrees();
			model.network.channelTmp[cNr].point_y[pos] = model.network.physicalLev[level1].preferredPathPoint[i1].latitude().degrees();
			model.network.channelTmp[cNr].point[pos] = spherical::Point(model.network.channelTmp[cNr].point_y[pos], model.network.channelTmp[cNr].point_x[pos]);
			totDist += model.network.channelTmp[cNr].point[pos].distanceTo(model.network.channelTmp[cNr].point[pos - 1]) / 1000.0;
			model.network.channelTmp[cNr].distanceFromStart[pos] = totDist;
			pos++;
		}
		model.network.channelTmp[cNr].nPoints += startP + 2;
		for (i1 = startP + 2; i1 < model.network.channelTmp[cNr].nPoints; i1++) {
			if (i1 == startP + 2)
				totDist += model.network.channelTmp[cNr].point[i1].distanceTo(model.network.channelTmp[cNr].point[i1 - 1]) / 1000.0;
			model.network.channelTmp[cNr].distanceFromStart[i1] += totDist;
		}
		model.network.channelTmp[cNr].distance_km = model.network.channelTmp[cNr].distanceFromStart[i1 - 1];
		model.network.channelTmp[cNr].bastStartPointPos = 0;
		model.network.channelTmp[cNr].preferredPathPoint_posConnectTo = -1;
		model.network.channelTmp[cNr].bastStartDist = 0;
	}

	return 0;
}

int check_feasibleToClosestPointInTss(int pos, int cNr, int* posTss, double* kvotRet, int riktning) {
	int i, isAllowed = -10;
	double kvot, xPrefP = model.network.physicalLev[pos].point_x[model.params.preferredPathOrtoPos[pos]]; // model.preferredPath.point_x[pos], distOK;
	double xTss, yTss, yPrefP = model.network.physicalLev[pos].point_y[model.params.preferredPathOrtoPos[pos]]; // model.preferredPath.point_y[pos];
	double distOK;

	if (riktning == -1) {
		for (i = *posTss; i < model.network.channelTmp[cNr].nPoints - 1; i++) {
			kvot = findClosestPointKvotAlongLine(yPrefP, xPrefP, model.network.channelTmp[cNr].point_y[i], model.network.channelTmp[cNr].point_x[i],
				model.network.channelTmp[cNr].point_y[i + 1], model.network.channelTmp[cNr].point_x[i + 1], &yTss, &xTss);
			if (kvot > 0.99) {
				if (i < model.network.channelTmp[cNr].nPoints - 2)
					continue; // closer to next tss point
				else
					kvot = 1;
			}

			isAllowed = check_isStraightLineFeasible(yPrefP, xPrefP, yTss, xTss);
			*posTss = i;
			*kvotRet = kvot;
			if (isAllowed == 0 && i == model.network.channelTmp[cNr].nPoints - 2)
				isAllowed = -2;
			return isAllowed;
		}
	}
	else {
		for (i = *posTss; i >= 1; i--) {
			kvot = findClosestPointKvotAlongLine(yPrefP, xPrefP, model.network.channelTmp[cNr].point_y[i], model.network.channelTmp[cNr].point_x[i],
				model.network.channelTmp[cNr].point_y[i - 1], model.network.channelTmp[cNr].point_x[i - 1], &yTss, &xTss);
			if (kvot > 0.99) {
				if (i > 1)
					continue; // closer to next tss point
				else
					kvot = 1;
			}

			isAllowed = check_isStraightLineFeasible(yPrefP, xPrefP, yTss, xTss);
			*posTss = i;
			*kvotRet = kvot;
			if (isAllowed == 0 && i == 1)
				isAllowed = -2;
			return isAllowed;
		}
	}

	isAllowed = check_isStraightLineFeasible(yPrefP, xPrefP, yTss, xTss);
	*posTss = i;
	*kvotRet = 0.0;
	return isAllowed;
}

int getBastPhysLevelToConnectToChannel_tss(int alt, int cNr) {
	int i, posMid, firstPos = -1, lastPos = -1, bast = -1, bast2 = -1;
	int isAllowed, posTss, pos;
	int bastLevel, returnLevel, startPoint, bastPoint, nCoords, bastPos;
	double dist, bastDist = 1e20, bastDist2 = 1e20, x1;
	double x, y, y1, kvot, distOK, deltaDist = 0, dirChannel;
	double connectDir, diffAngle, angleKrav = M_PI * 11 / 20; // slightly more than 90 degrees

	identify_startEndOnChannel_tss(cNr, alt);

	if (alt == 0) {
		x = model.network.channelTmp[cNr].point_x[0];
		y = model.network.channelTmp[cNr].point_y[0];
	}
	else {
		nCoords = model.network.channelTmp[cNr].nPoints;
		x = model.network.channelTmp[cNr].point_x[nCoords - 1];
		y = model.network.channelTmp[cNr].point_y[nCoords - 1];
	}
	if (cNr == 2)
		cNr = cNr;
	dirChannel = getDirChannelTmp(cNr, alt);

	// startSlut, 0 start, 1 end
	check_translate_xCoord(&x);


	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		posMid = (int)model.network.physicalLev[i].nPoints / 2;

		x1 = model.network.physicalLev[i].point_x[posMid];
		check_translate_xCoord(&x1);
		dist = estimateLargeCircleDistance2_km(y, x, model.network.physicalLev[i].point_y[posMid], x1);
		if (dist <= model.params.maxDeviationPreferred2_km) {
			if (dist < bastDist) {
				if (dist < 1) {
					bastDist = dist;
					bast = i;
				}else{
					connectDir = atan2(y - model.network.physicalLev[i].point_y[posMid], x - x1);
					diffAngle = getDiff_angles(dirChannel, connectDir, alt);
					if (abs(diffAngle) < angleKrav) {
						bastDist = dist;
						bast = i;
					}
				}
			}
		}
	}
	if (bast == -1) {
		if(alt == 0)
			model.network.channelTmp[cNr].bastStartLevel = -1;
		else
			model.network.channelTmp[cNr].bastEndLevel = -1;
		printf("ERROR! Could not find a connection to corridor %d alt %d. bast %d\n", cNr, alt, bast);
		errlog("ERROR! Could not find a connection to corridor %d alt %d. bast %d\n", cNr, alt, bast);
		return -1;
	}
	if (alt == 0) { // start of channel
		bastLevel = bast;

		bastDist = 1e20;
		for (i = -1; i < model.network.physicalLev[bastLevel].npreferredPathPoints; i++) {
			if (i < 0) {
				x1 = model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]];
				y1 = model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]];
			}
			else {
				x1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].longitude().degrees();
				y1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].latitude().degrees();
			}
			check_translate_xCoord(&x1);
			dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
			if (bastDist > dist) {
				if (dist < 1) {
					bastDist = dist;
					bast = i;
				}
				else {
					connectDir = atan2(y - y1, x - x1);
					diffAngle = getDiff_angles(dirChannel, connectDir, alt);
					if (abs(diffAngle) < angleKrav) {
						bastDist = dist;
						bast = i;
					}
				}
			}
		}
		if (bastDist < 0.9e20) {
			//if (bastPos >= 0)
			//	bastPos--;
			model.network.channelTmp[cNr].preferredPathPoint_posConnectTo = bast;
			model.network.channelTmp[cNr].bastStartPointPos = bast;
			model.network.channelTmp[cNr].bastStartDist = bastDist;
		}
		returnLevel = bastLevel;

		isAllowed = check_isStraightLineFeasible(model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]],
			model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]], 
			model.network.channelTmp[cNr].point_y[0], model.network.channelTmp[cNr].point_x[0]);

		if (isAllowed == 0) {
		//	// if straight line to tss not feasible
		//	posTss = 0;
		//	for (i = bastLevel + 1; i < model.network.nPhysicalLevels; i++) {
		//		// find first prefPath section where the path is feasible		
		//		isAllowed = check_feasibleToClosestPointInTss(i, cNr, &posTss, &kvot, -1);
		//		if (isAllowed == 1 || isAllowed == -2)
		//			break;
		//	}
		//	if (isAllowed == 1) {
		//		pos = 0;
		//		bastLevel = i;
		//		if (posTss > model.network.channelTmp[cNr].nPoints - 2) {
		//			posTss = model.network.channelTmp[cNr].nPoints - 2;
		//			kvot = 0.0;
		//		}

		//		for (i = posTss; i < model.network.channelTmp[cNr].nPoints; i++) {
		//			if (i == posTss && kvot > 0.01) {
		//				model.network.channelTmp[cNr].point_x[pos] = model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]];
		//				model.network.channelTmp[cNr].point_y[pos] = model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]];
		//				model.network.channelTmp[cNr].point[pos] = spherical::Point(model.network.channelTmp[cNr].point_y[pos], model.network.channelTmp[cNr].point_x[pos]);
		//			}
		//			else {
		//				model.network.channelTmp[cNr].point_x[pos] = model.network.channelTmp[cNr].point_x[i];
		//				model.network.channelTmp[cNr].point_y[pos] = model.network.channelTmp[cNr].point_y[i];
		//				model.network.channelTmp[cNr].point[pos] = model.network.channelTmp[cNr].point[i];
		//			}
		//			if (i > posTss)
		//				model.network.channelTmp[cNr].distanceFromStart[pos] = model.network.channelTmp[cNr].distanceFromStart[i] - deltaDist;
		//			else {
		//				deltaDist = model.network.channelTmp[cNr].distanceFromStart[i] -
		//					model.network.channelTmp[cNr].point[pos].distanceTo(model.network.channelTmp[cNr].point[i + 1]) / 1000.0;
		//			}
		//			pos++;
		//		}
		//		model.network.channelTmp[cNr].nPoints = pos;
		//		model.network.channelTmp[cNr].distance_km = model.network.channelTmp[cNr].distanceFromStart[pos - 1];
				model.network.channelTmp[cNr].StartLevelOnlyPrefPath = 1;
		//	}
		//	if (isAllowed == -2) {
				// extend the tss to next node along prefPath
			posTss = 0;
				// extendTssAlongPrefPath(cNr, posTss, &bastLevel, alt);
		//	}
		}
		model.network.channelTmp[cNr].bastStartLevel = bastLevel;
		if (bastLevel >= 0)
			model.network.channelTmp[cNr].legNr = model.network.physicalLev[bastLevel].legNr;
		else
			model.network.channelTmp[cNr].legNr = model.network.channelTmp[-bastLevel - 1].legNr;

		returnLevel = bastLevel;
	}
	else { // end of channel
		bastLevel = bast;
		if (bastLevel >= model.network.nPhysicalLevels - 1) {
			bastLevel = model.network.nPhysicalLevels - 1;
		}

		if (bastLevel > 0)
			bastLevel--;
		bastDist = 1e20;
		for (i = 0; i <= model.network.physicalLev[bastLevel].npreferredPathPoints; i++) {
			if (i == model.network.physicalLev[bastLevel].npreferredPathPoints) {
				x1 = model.network.physicalLev[bastLevel+1].point_x[model.params.preferredPathOrtoPos[bastLevel+1]];
				y1 = model.network.physicalLev[bastLevel+1].point_y[model.params.preferredPathOrtoPos[bastLevel+1]];
			}
			else {
				x1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].longitude().degrees();
				y1 = model.network.physicalLev[bastLevel].preferredPathPoint[i].latitude().degrees();
			}
			check_translate_xCoord(&x1);
			dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
			if (bastDist > dist) {
				if (dist < 1) {
					bastDist = dist;
					bast = i;
				}
				else {
					connectDir = atan2(y - y1, x - x1);
					diffAngle = getDiff_angles(dirChannel, connectDir, alt);
					if (abs(diffAngle) < angleKrav) {
						bastDist = dist;
						bast = i;
					}
				}
			}
		}
		if (bastDist < 0.9e20) {
			if (bast == model.network.physicalLev[bastLevel].npreferredPathPoints) {
				bastLevel++;
				bast = -1;
			}
			else
				bastLevel++;
			model.network.channelTmp[cNr].preferredPathPoint_posConnectFrom = bast;
			model.network.channelTmp[cNr].bastEndPointPos = bast;
			model.network.channelTmp[cNr].bastEndDist = bastDist;
		}

		//printf("-------bastLevel %d\n", bastLevel);
		returnLevel = bastLevel;

		posTss = model.network.channelTmp[cNr].nPoints - 1;
		isAllowed = check_isStraightLineFeasible(model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]], 
			model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]],
			model.network.channelTmp[cNr].point_y[posTss], model.network.channelTmp[cNr].point_x[posTss]);

		if (isAllowed == 0) {
			// if straight line to tss not feasible

			//posTss = model.network.channelTmp[cNr].nPoints - 1;
			//for (i = bastLevel - 1; i >= 0; i--) {
			//	// find first prefPath section where the path is feasible		
			//	isAllowed = check_feasibleToClosestPointInTss(i, cNr, &posTss, &kvot, 1);
			//	if (isAllowed == 1 || isAllowed == -2)
			//		break;
			//}
			//if (isAllowed == 1) {
			//	pos = 0;
			//	bastLevel = i;
			//	if (posTss < 1) {
			//		posTss = 1;
			//		kvot = 0.0;
			//	}

			//	pos = posTss;
			//	if (kvot > 0.01) {
			//		model.network.channelTmp[cNr].point_x[pos] = model.network.physicalLev[bastLevel].point_x[model.params.preferredPathOrtoPos[bastLevel]];
			//		model.network.channelTmp[cNr].point_y[pos] = model.network.physicalLev[bastLevel].point_y[model.params.preferredPathOrtoPos[bastLevel]];
			//		model.network.channelTmp[cNr].point[pos] = spherical::Point(model.network.channelTmp[cNr].point_y[pos], model.network.channelTmp[cNr].point_x[pos]);
			//		model.network.channelTmp[cNr].distanceFromStart[pos] = model.network.channelTmp[cNr].distanceFromStart[pos - 1] +
			//			model.network.channelTmp[cNr].point[pos].distanceTo(model.network.channelTmp[cNr].point[pos - 1]) / 1000.0;
			//	}
			//	model.network.channelTmp[cNr].nPoints = posTss + 1;
			//	model.network.channelTmp[cNr].distance_km = model.network.channelTmp[cNr].distanceFromStart[pos];
			model.network.channelTmp[cNr].EndLevelOnlyPrefPath = 1;
			//}
			//if (isAllowed == -2) {
				// extend the tss to next node along prefPath
				// extendTssAlongPrefPath(cNr, posTss, &bastLevel, alt);
			//}
		}
		model.network.channelTmp[cNr].bastEndLevel = bastLevel;
		returnLevel = bastLevel;
	}
	return returnLevel;

}

/*
double getDistToBoundingBoxChannelPolygon(strBoundBox bbox, double lat, double lon) {
	double dist1, dist2, dist3, dist4;
	double minX, minY, minDist, minDist2, minX2, minY2;

	//printf("bbox %.3lf %.3lf %.3lf %.3lf\n", bbox.xMin, bbox.yMin, bbox.xMax, bbox.yMax);
	if (lat >= bbox.yMin && lat <= bbox.yMax && lon >= bbox.xMin && lon <= bbox.xMax)
		return 0.0;

	dist1 = estimateLargeCircleDistance_km(lat, lon, bbox.yMin, bbox.xMin);
	dist2 = estimateLargeCircleDistance_km(lat, lon, bbox.yMin, bbox.xMax);
	if (dist1 <= dist2) {
		minDist = dist1;
		minX = bbox.xMin;
		minY = bbox.yMin;
		minDist2 = dist2;
		minX2 = bbox.xMax;
		minY2 = bbox.yMin;
	}
	else {
		minDist = dist2;
		minX = bbox.xMax;
		minY = bbox.yMin;
		minDist2 = dist1;
		minX2 = bbox.xMin;
		minY2 = bbox.yMin;
	}
	dist3 = estimateLargeCircleDistance_km(lat, lon, bbox.yMax, bbox.xMax);
	if (dist3 < minDist) {
		minDist2 = minDist;
		minX2 = minX;
		minY2 = minY;
		minDist = dist3;
		minX = bbox.xMax;
		minY = bbox.yMax;
	}
	else {
		if (dist3 < minDist2) {
			minDist2 = dist3;
			minX2 = bbox.xMax;
			minY2 = bbox.yMax;
		}
	}
	dist4 = estimateLargeCircleDistance_km(lat, lon, bbox.yMax, bbox.xMin);
	if (dist4 < minDist) {
		minDist2 = minDist;
		minX2 = minX;
		minY2 = minY;
		minDist = dist4;
		minX = bbox.xMin;
		minY = bbox.yMax;
	}
	else {
		if (dist4 < minDist2) {
			minDist2 = dist4;
			minX2 = bbox.xMin;
			minY2 = bbox.yMax;
		}
	}

	if (lat < minY && lat < minY2 || lat > minY && lat > minY2) {
		if (lon < minX && lon < minX2 || lon > minX && lon > minX2)
			return minDist;
		else {
			dist1 = estimateLargeCircleDistance_km(lat, lon, minY, lon);
			return dist1;
		}
	}
	else {
		dist1 = estimateLargeCircleDistance_km(lat, lon, lat, minX);
		return dist1;
	}

}

int getBastPhysLevelToConnectToChannelPolygon(int cNr, int startPos, int corridorPos) {
	int i, posMid, traffEnd;
	double dist, distFromStartPrev, deltaDist;

	// startSlut, 0 start, 1 end

	dist = 0;
	if (startPos > 0)
		distFromStartPrev = model.network.physicalLev[startPos - 1].distanceFromStartPosMid;
	else
		distFromStartPrev = 0;

	traffEnd = 0;
	for (i = startPos; i < model.network.nPhysicalLevels; i++) {
		posMid = (int)model.network.physicalLev[i].nPoints / 2;
		deltaDist = model.network.physicalLev[i].distanceFromStartPosMid - distFromStartPrev;
		if (dist < model.params.maxDeviationPreferred_km + deltaDist) {
			dist = getDistToBoundingBoxChannelPolygon(model.network.channel[cNr].polygon_boundingBox[corridorPos],
				model.network.physicalLev[i].point_y[posMid], model.network.physicalLev[i].point_x[posMid]);
			//printf("corridor %d corridorPos %d i %d dist %.2lf maxDev %.2lf\n", cNr, corridorPos, i, dist,
			//	model.params.maxDeviationPreferred_km);
			if (dist <= model.params.maxDeviationPreferred_km) {
				if (corridorPos == 0)
					break;
				traffEnd = 1;
			}
			else {
				if (traffEnd == 1) {
					break;
				}
			}
			distFromStartPrev = model.network.physicalLev[i].distanceFromStartPosMid;
		}
	}
	//printf("dist %.3lf distFromStartPrev %.3lf startPos %d traffEnd %d i %d nPhysLev %d\n", dist, distFromStartPrev,
	//	startPos, traffEnd, i, model.network.nPhysicalLevels);
	if (startPos == 0) {
		if (i >= model.network.nPhysicalLevels)
			return -1; // no physicalLev close enough to the coordinate
	}
	else {
		if (i >= model.network.nPhysicalLevels) {
			if (traffEnd == 0)
				return -1;
			return model.network.nPhysicalLevels - 1;
		}
	}
	//if (model.network.channel[cNr].allowedPoint[pos] == 0)
	//	continue; // last node in channel not okay

	return i;


}
*/

double identify_minDist_channelToPrefPath(int cNr, int startEnd, int* minPosBast, double* distBastPrev2, double* distBastNext2) {
	double minDist = 1e20, x, y, distPrev = -1, dist, x0;
	int i0, i, minPos = 0, start_i0, direction;

	if (startEnd == 0) {
		start_i0 = 0;
		direction = 1;
	}
	else {
		start_i0 = model.network.nPhysicalLevels - 1;
		direction = -1;
	}

	for (i0 = start_i0; i0 >= 0 && i0 < model.network.nPhysicalLevels; i0 += direction) {
		x = model.network.physicalLev[i0].point_x[model.params.preferredPathOrtoPos[i0]];
		y = model.network.physicalLev[i0].point_y[model.params.preferredPathOrtoPos[i0]];
		for (i = 0; i < model.network.channel[cNr].nPoints; i++) {
			x0 = model.network.channel[cNr].point_x[i];
			check_translate_xCoord(&x0);
			dist = estimateLargeCircleDistance2_km(y, x, model.network.channel[cNr].point_y[i], x0);
			if (minDist > dist) {
				minDist = dist;
				*distBastPrev2 = distPrev;
				minPos = i;
			}
			distPrev = dist;
			if (minPos == i - 1)
				*distBastNext2 = dist;
		}
		if (minDist < 50000) {
			break; // close enough to pref path...
		}
	}
	*minPosBast = minPos;
	minDist = sqrt(minDist);

	return minDist;
}

int identify_startEndOnChannel(int cNr, int startEnd) {
	int i, minPos, posUse, ok;
	double minDist = 1e20, dist, x, y, distBastPrev2, distPrev = -1, distBastNext2 = -1, x0, x1, x2;
	double dist2, cosC1, cosC2, minPrev, minNext, dist1, distFromPoint, bearing, cosBefore = -10, cosAfter = -10;
	double distOld, deltaDist, timeOld, timeNew;
	spherical::Point p1;

	// if startpunkt within corridor and dist < x to corridor => cut the corridors length, start at a position after startpunkt
	if (startEnd == 0) {
		x = model.network.physicalLev[0].point_x[0];
		y = model.network.physicalLev[0].point_y[0];
	}
	else {
		x = model.network.physicalLev[model.network.nPhysicalLevels - 1].point_x[0];
		y = model.network.physicalLev[model.network.nPhysicalLevels - 1].point_y[0];
	}

	//ok = checkCoordInBoundingBox(y, x, model.network.channel[cNr].boundingBox);
	//if (ok == 0)
	//	return 0;

	minDist = identify_minDist_channelToPrefPath(cNr, startEnd, &minPos, &distBastPrev2, &distBastNext2);

	//for (i = 0; i < model.network.channel[cNr].nPoints; i++) {
	//	x0 = model.network.channel[cNr].point_x[i];
	//	check_translate_xCoord(&x0);
	//	dist = estimateLargeCircleDistance2_km(y, x, model.network.channel[cNr].point_y[i], x0);
	//	if (minDist > dist) {
	//		minDist = dist;
	//		distBastPrev2 = distPrev;
	//		minPos = i;
	//	}
	//	distPrev = dist;
	//	if (minPos == i - 1)
	//		distBastNext2 = dist;
	//}
	//printf("ident_startEndOnChannel startEnd %d minDist %.2lf minPos %d distBastPrev %.2lf distBastNext %.2lf nPoints %d\n",
	//	startEnd, minDist, minPos, distBastPrev, distBastNext, model.network.channel[cNr].nPoints);

	if (minPos > 0) {
		x1 = model.network.channel[cNr].point_x[minPos - 1];
		check_translate_xCoord(&x1);
		//x2 = model.network.channel[cNr].point_x[minPos + 1];
		//check_translate_xCoord(&x2);
		x0 = model.network.channel[cNr].point_x[minPos];
		check_translate_xCoord(&x0);
		dist1 = estimateLargeCircleDistance_km(model.network.channel[cNr].point_y[minPos - 1], x1,
			model.network.channel[cNr].point_y[minPos], x0);
		//dist2 = estimateLargeCircleDistance_km(model.network.channel[cNr].point_y[minPos + 1], x2,
		//	model.network.channel[cNr].point_y[minPos], x0);

		// cosC1 = cosPrev
		if (minDist * dist1 > 0.00001)
			cosBefore = (-distBastPrev2 + dist1 * dist1 + minDist * minDist) / (2 * minDist * dist1);
		else
			cosBefore = (-distBastPrev2 + dist1 * dist1 + minDist * minDist) / (2 * 0.00001);

		if (cosBefore > 1)
			cosBefore = 1;
		if (cosBefore < -1)
			cosBefore = -1;
		if (cosBefore > 0)
			minPrev = minDist * lookUpSin(acos(cosBefore));
		else {
			minPrev = minDist;
			if (minPos == model.network.channel[cNr].nPoints - 1)
				return 0; // connection after the corridor ends, keep the corridor as it is
		}

		//if(distBastPrev * dist1 > 0.00001)
		//	cosC1 = (distBastPrev * distBastPrev + dist1 * dist1 - minDist * minDist) / (2 * distBastPrev * dist1);
		//else
		//	cosC1 = (distBastPrev * distBastPrev + dist1 * dist1 - minDist * minDist) / (2 * 0.00001);
		//if (cosC1 > 1)
		//	cosC1 = 1;
		//if (cosC1 < -1)
		//	cosC1 = -1;
		//minPrev = distBastPrev * sin(acos(cosC1));
		//if (minPrev < minDist - dist1)
		//	minPrev = minDist - dist1;

		//printf("minDist %.2lf dist1 %.2lf distBastPrev %.2lf cosBefore %.2lf cosC1 %.2lf\n",
		//	minDist, dist1, distBastPrev, cosBefore, cosC1);
	}
	else {
		minPrev = 1e10;
	}
	if (minPos < model.network.channel[cNr].nPoints - 1) {
		//x1 = model.network.channel[cNr].point_x[minPos - 1];
		//check_translate_xCoord(&x1);
		x2 = model.network.channel[cNr].point_x[minPos + 1];
		check_translate_xCoord(&x2);
		x0 = model.network.channel[cNr].point_x[minPos];
		check_translate_xCoord(&x0);
		dist2 = estimateLargeCircleDistance_km(model.network.channel[cNr].point_y[minPos + 1], x2,
			model.network.channel[cNr].point_y[minPos], x0);

		if (minDist * dist2 > 0.00001)
			cosAfter = (-distBastNext2 + dist2 * dist2 + minDist * minDist) / (2 * minDist * dist2);
		else
			cosAfter = (-distBastNext2 + dist2 * dist2 + minDist * minDist) / (2 * 0.00001);

		if (cosAfter > 1)
			cosAfter = 1;
		if (cosAfter < -1)
			cosAfter = -1;
		if (cosAfter > 0)
			minNext = minDist * lookUpSin(acos(cosAfter));
		else {
			if (minPos == 0)
				return 0; // connection before the corridor start, keep the corridor as it is
			minNext = minDist;
		}

		//if (distBastNext * dist2 > 0.00001)
		//	cosC2 = (distBastNext * distBastNext + dist2 * dist2 - minDist * minDist) / (2 * distBastNext * dist2);
		//else
		//	cosC2 = (distBastNext * distBastNext + dist2 * dist2 - minDist * minDist) / (2 * 0.00001);
		//if (cosC2 > 1)
		//	cosC2 = 1;
		//if (cosC2 < -1)
		//	cosC2 = -1;

		//if (dist2 - distBastNext * cosC2 < 0)
		//	minNext = minDist;
		//else
		//	minNext = distBastNext * sin(acos(cosC2));
		//if (minNext < minDist - dist2)
		//	minNext = minDist - dist2;

		//printf("minDist %.2lf dist2 %.2lf distBastNext %.2lf cosAfter %.2lf cosC2 %.2lf\n",
		//	minDist, dist2, distBastNext, cosAfter, cosC2);
	}
	else {
		minNext = 1e10;
	}

	//printf("minPrev %.2lf minNext %.2lf\n", minPrev, minNext);

	if (minPrev >= 9e9 && minNext >= 9e9)
		return 0;

	if (startEnd == 0) {
		if (minDist < 0.01 && minPos == 0)
			return 0; // no need to do anything as it is the right node
	}
	else {
		if (minDist < 0.01 && minPos == model.network.channel[cNr].nPoints - 1)
			return 0; // no need to do anything as it is the right node
	}

	if (minPrev <= minNext) {
		if (minPrev > model.params.maxDistStartToCorridorConnect) {
			errlog("OBS! Not shortening the corridor at startEnd %d, distPrev %.2lf km must be <= %.2lf\n",
				startEnd, minPrev, model.params.maxDistStartToCorridorConnect);
			return 0; // too far away from the channel
		}
		distFromPoint = dist1 - cosBefore * minDist;
		posUse = minPos - 1;
		if (cosBefore < 0) {
			p1 = model.network.channel[cNr].point[posUse + 1];
			distFromPoint = -1;
		}
	}
	else {
		if (minNext > model.params.maxDistStartToCorridorConnect) {
			errlog("OBS! Not shortening the corridor at startEnd %d, distNext %.2lf km must be <= %.2lf\n",
				startEnd, minNext, model.params.maxDistStartToCorridorConnect);
			return 0; // too far away from the channel
		}
		distFromPoint = cosAfter * minDist;
		posUse = minPos;
		if (cosAfter <= 0) {
			p1 = model.network.channel[cNr].point[posUse];
		}
	}

	//if (cosBefore >= cosAfter) {
	//	if (minPrev > model.params.maxDistStartToCorridorConnect) {
	//		errlog("OBS! Not shortening the corridor at startEnd %d, distPrev %.2lf km must be <= %.2lf\n",
	//			startEnd, minPrev, model.params.maxDistStartToCorridorConnect);
	//		return 0; // too far away from the channel
	//	}
	//	distFromPoint = distBastPrev * cosC1;
	//	posUse = minPos - 1;
	//}
	//else {
	//	if (minNext > model.params.maxDistStartToCorridorConnect) {
	//		errlog("OBS! Not shortening the corridor at startEnd %d, distNext %.2lf km must be <= %.2lf\n",
	//			startEnd, minNext, model.params.maxDistStartToCorridorConnect);
	//		return 0; // too far away from the channel
	//	}
	//	distFromPoint = dist2 - distBastNext * cosC2;
	//	posUse = minPos;
	//}
	if (distFromPoint > 0) {
		bearing = model.network.channel[cNr].point[posUse].bearingTo(model.network.channel[cNr].point[posUse + 1]);
		p1 = model.network.channel[cNr].point[posUse].destinationPoint(distFromPoint * 1000, bearing);
	}
	distOld = model.network.channel[cNr].distance_km;
	if (startEnd == 0)
		deltaDist = p1.distanceTo(model.network.channel[cNr].point[posUse + 1]) / 1000.0 - model.network.channel[cNr].distanceFromStart[posUse + 1];
	else
		deltaDist = -distOld - p1.distanceTo(model.network.channel[cNr].point[posUse + 1]) / 1000.0 + model.network.channel[cNr].distanceFromStart[posUse + 1];

	//printf("deltaDist %.2lf distFromStart_posUse+1 %.2lf p1_distTo_posUse+1 %.2lf posUse %d posUse+1_xy %.3lf %.3lf\n", deltaDist,
	//	model.network.channel[cNr].distanceFromStart[posUse + 1], p1.distanceTo(model.network.channel[cNr].point[posUse + 1]),
	//	posUse, model.network.channel[cNr].point[posUse + 1].longitude().degrees(), model.network.channel[cNr].point[posUse + 1].latitude().degrees());
	timeOld = model.network.channel[cNr].timeThroughChannel;
	if (timeOld < -0.1)
		timeNew = timeOld;
	else
		timeNew = timeOld * (distOld + deltaDist) / distOld;

	if (startEnd == 0)
		errlog("update data for corridor. Before it started at %.3lf %.3lf. It is shortened and now starts at %.3lf %.3lf.\n"
			"The time through the corridor is changed from %.2lf to %.2lf since the new distance of the corridor is %.2lf compared to %.2lf before, same for pilot cost and fuel consumption\n",
			model.network.channel[cNr].point_x[0], model.network.channel[cNr].point_y[0],
			p1.longitude().degrees(), p1.latitude().degrees(), timeOld, timeNew, distOld + deltaDist, distOld);
	else
		errlog("update data for corridor. Before it ended at %.3lf %.3lf. It is shortened and now ends at %.3lf %.3lf.\n"
			"The time through the corridor is changed from %.2lf to %.2lf since the new distance of the corridor is %.2lf compared to %.2lf before, same for pilot cost and fuel consumption\n",
			model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1], model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1],
			p1.longitude().degrees(), p1.latitude().degrees(), timeOld, timeNew, distOld + deltaDist, distOld);

	model.network.channel[cNr].distance_km = distOld + deltaDist;
	model.network.channel[cNr].timeThroughChannel = timeNew;
	model.network.channel[cNr].extraCostChannel = model.network.channel[cNr].extraCostChannel * (distOld + deltaDist) / distOld;
	model.network.channel[cNr].totalConsumption = model.network.channel[cNr].totalConsumption * (distOld + deltaDist) / distOld;
	if (startEnd == 0) {
		model.network.channel[cNr].waitingTime = 0;
		model.network.channel[cNr].waiting_consumption_main = 0;
		model.network.channel[cNr].waiting_consumption_aux = 0;
		model.network.channel[cNr].waitingTime = 0; // .intWaitingTime = 0;
		model.network.channel[cNr].arrivalTime_h = -1;
		model.network.channel[cNr].intArrivalTime_h = -1;

		model.network.channel[cNr].point[0] = p1;
		model.network.channel[cNr].point_x[0] = p1.longitude().degrees();
		model.network.channel[cNr].point_y[0] = p1.latitude().degrees();

		//printf("new Point lon/lat %.3lf %.3lf instead of pos %d %.3lf %.3lf\n", model.network.channel[cNr].point_x[0],
		//	model.network.channel[cNr].point_y[0], posUse, model.network.channel[cNr].point_x[posUse],
		//	model.network.channel[cNr].point_y[posUse]);

		for (i = posUse + 1; i < model.network.channel[cNr].nPoints; i++) {
			model.network.channel[cNr].point[i - posUse] = model.network.channel[cNr].point[i];
			model.network.channel[cNr].point_x[i - posUse] = model.network.channel[cNr].point_x[i];
			model.network.channel[cNr].point_y[i - posUse] = model.network.channel[cNr].point_y[i];
			if (i == posUse + 1)
				model.network.channel[cNr].distanceFromStart[i - posUse] =
				model.network.channel[cNr].point[0].distanceTo(model.network.channel[cNr].point[1]) / 1000.0;
			else
				model.network.channel[cNr].distanceFromStart[i - posUse] = model.network.channel[cNr].distanceFromStart[i - posUse - 1] +
				model.network.channel[cNr].distanceFromStart[i] - model.network.channel[cNr].distanceFromStart[i - 1];
		}
		model.network.channel[cNr].nPoints -= posUse;
	}
	else {
		model.network.channel[cNr].point[posUse + 1] = p1;
		model.network.channel[cNr].point_x[posUse + 1] = p1.longitude().degrees();
		model.network.channel[cNr].point_y[posUse + 1] = p1.latitude().degrees();

		errlog("new endPoint %d lon/lat %.3lf %.3lf\n", posUse + 1, model.network.channel[cNr].point_x[posUse + 1],
			model.network.channel[cNr].point_y[posUse + 1]);

		model.network.channel[cNr].distanceFromStart[posUse + 1] = model.network.channel[cNr].distanceFromStart[posUse] +
			model.network.channel[cNr].point[posUse].distanceTo(model.network.channel[cNr].point[posUse + 1]) / 1000.0;

		model.network.channel[cNr].nPoints = posUse + 2;
	}


	//printf("nPoints efter %d\n", model.network.channel[cNr].nPoints);
	//printf("pos %d xy %.3lf %.3lf\n", model.network.channel[cNr].nPoints - 1,
	//	model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1],
	//	model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1]);
	//printf("pos %d xy %.3lf %.3lf\n", model.network.channel[cNr].nPoints - 2,
	//	model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 2],
	//	model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 2]);


	return 1;
}

int identify_startEndOnChannel_tss(int cNr, int startEnd) {
	int i, minPos, posUse, ok;
	double minDist = 1e20, dist, x, y, distBastPrev2, distPrev = -1, distBastNext2 = -1, x0, x1, x2;
	double dist2, cosC1, cosC2, minPrev, minNext, dist1, distFromPoint, bearing, cosBefore = -10, cosAfter = -10;
	double distOld, deltaDist, timeOld, timeNew;
	spherical::Point p1;

	// if startpunkt within corridor and dist < x to corridor => cut the corridors length, start at a position after startpunkt
	if (startEnd == 0) {
		x = model.network.physicalLev[0].point_x[0]; 
		y = model.network.physicalLev[0].point_y[0];
	}
	else {
		x = model.network.physicalLev[model.network.nPhysicalLevels-1].point_x[0];
		y = model.network.physicalLev[model.network.nPhysicalLevels - 1].point_y[0];
	}

	//ok = checkCoordInBoundingBox(y, x, model.network.channel[cNr].boundingBox);
	//if (ok == 0)
	//	return 0;

	for (i = 0; i < model.network.channelTmp[cNr].nPoints; i++) {
		x0 = model.network.channelTmp[cNr].point_x[i];
		check_translate_xCoord(&x0);
		dist = estimateLargeCircleDistance2_km(y, x, model.network.channelTmp[cNr].point_y[i], x0);
		if (minDist > dist) {
			minDist = dist;
			distBastPrev2 = distPrev;
			minPos = i;
		}
		distPrev = dist;
		if (minPos == i - 1)
			distBastNext2 = dist;
	}
	//printf("ident_startEndOnChannel startEnd %d minDist %.2lf minPos %d distBastPrev %.2lf distBastNext %.2lf nPoints %d\n",
	//	startEnd, minDist, minPos, distBastPrev, distBastNext, model.network.channel[cNr].nPoints);

	minDist = sqrt(minDist);
	if (minPos > 0) {
		x1 = model.network.channelTmp[cNr].point_x[minPos - 1];
		check_translate_xCoord(&x1);
		//x2 = model.network.channel[cNr].point_x[minPos + 1];
		//check_translate_xCoord(&x2);
		x0 = model.network.channelTmp[cNr].point_x[minPos];
		check_translate_xCoord(&x0);
		dist1 = estimateLargeCircleDistance_km(model.network.channelTmp[cNr].point_y[minPos - 1], x1,
			model.network.channelTmp[cNr].point_y[minPos], x0);
		//dist2 = estimateLargeCircleDistance_km(model.network.channel[cNr].point_y[minPos + 1], x2,
		//	model.network.channel[cNr].point_y[minPos], x0);
		
		// cosC1 = cosPrev
		if(minDist * dist1 > 0.00001)
			cosBefore = (-distBastPrev2 + dist1 * dist1 + minDist * minDist) / (2 * minDist * dist1);
		else
			cosBefore = (-distBastPrev2 + dist1 * dist1 + minDist * minDist) / (2 * 0.00001);

		if (cosBefore > 1)
			cosBefore = 1;
		if (cosBefore < -1)
			cosBefore = -1;
		if (cosBefore > 0)
			minPrev = minDist * lookUpSin(acos(cosBefore));
		else {
			minPrev = minDist;
			if (minPos == model.network.channelTmp[cNr].nPoints - 1)
				return 0; // connection after the corridor ends, keep the corridor as it is
		}

		//if(distBastPrev * dist1 > 0.00001)
		//	cosC1 = (distBastPrev * distBastPrev + dist1 * dist1 - minDist * minDist) / (2 * distBastPrev * dist1);
		//else
		//	cosC1 = (distBastPrev * distBastPrev + dist1 * dist1 - minDist * minDist) / (2 * 0.00001);
		//if (cosC1 > 1)
		//	cosC1 = 1;
		//if (cosC1 < -1)
		//	cosC1 = -1;
		//minPrev = distBastPrev * sin(acos(cosC1));
		//if (minPrev < minDist - dist1)
		//	minPrev = minDist - dist1;

		//printf("minDist %.2lf dist1 %.2lf distBastPrev %.2lf cosBefore %.2lf cosC1 %.2lf\n",
		//	minDist, dist1, distBastPrev, cosBefore, cosC1);
	}
	else {
		minPrev = 1e10;
	}
	if (minPos < model.network.channelTmp[cNr].nPoints - 1) {
		//x1 = model.network.channel[cNr].point_x[minPos - 1];
		//check_translate_xCoord(&x1);
		x2 = model.network.channelTmp[cNr].point_x[minPos + 1];
		check_translate_xCoord(&x2);
		x0 = model.network.channelTmp[cNr].point_x[minPos];
		check_translate_xCoord(&x0);
		dist2 = estimateLargeCircleDistance_km(model.network.channelTmp[cNr].point_y[minPos + 1], x2,
			model.network.channelTmp[cNr].point_y[minPos], x0);

		if (minDist * dist2 > 0.00001)
			cosAfter = (-distBastNext2 + dist2 * dist2 + minDist * minDist) / (2 * minDist * dist2);
		else
			cosAfter = (-distBastNext2 + dist2 * dist2 + minDist * minDist) / (2 * 0.00001);

		if (cosAfter > 1)
			cosAfter = 1;
		if (cosAfter < -1)
			cosAfter = -1;
		if (cosAfter > 0)
			minNext = minDist * lookUpSin(acos(cosAfter));
		else {
			if (minPos == 0)
				return 0; // connection before the corridor start, keep the corridor as it is
			minNext = minDist;
		}

		//if (distBastNext * dist2 > 0.00001)
		//	cosC2 = (distBastNext * distBastNext + dist2 * dist2 - minDist * minDist) / (2 * distBastNext * dist2);
		//else
		//	cosC2 = (distBastNext * distBastNext + dist2 * dist2 - minDist * minDist) / (2 * 0.00001);
		//if (cosC2 > 1)
		//	cosC2 = 1;
		//if (cosC2 < -1)
		//	cosC2 = -1;

		//if (dist2 - distBastNext * cosC2 < 0)
		//	minNext = minDist;
		//else
		//	minNext = distBastNext * sin(acos(cosC2));
		//if (minNext < minDist - dist2)
		//	minNext = minDist - dist2;

		//printf("minDist %.2lf dist2 %.2lf distBastNext %.2lf cosAfter %.2lf cosC2 %.2lf\n",
		//	minDist, dist2, distBastNext, cosAfter, cosC2);
	}
	else {
		minNext = 1e10;
	}

	//printf("minPrev %.2lf minNext %.2lf\n", minPrev, minNext);

	if (minPrev >= 9e9 && minNext >= 9e9)
		return 0;

	if (startEnd == 0) {
		if (minDist < 0.01 && minPos == 0)
			return 0; // no need to do anything as it is the right node
	}
	else {
		if (minDist < 0.01 && minPos == model.network.channelTmp[cNr].nPoints - 1)
			return 0; // no need to do anything as it is the right node
	}

	if (minPrev <= minNext) {
		if (minPrev > model.params.maxDistStartToCorridorConnect) {
			errlog("OBS! Not shortening the corridor at startEnd %d, distPrev %.2lf km must be <= %.2lf\n",
				startEnd, minPrev, model.params.maxDistStartToCorridorConnect);
			return 0; // too far away from the channel
		}
		distFromPoint = dist1 - cosBefore * minDist;
		posUse = minPos - 1;
		if (cosBefore < 0) {
			p1 = model.network.channelTmp[cNr].point[posUse + 1];
			distFromPoint = -1;
		}
	}
	else {
		if (minNext > model.params.maxDistStartToCorridorConnect) {
			errlog("OBS! Not shortening the corridor at startEnd %d, distNext %.2lf km must be <= %.2lf\n",
				startEnd, minNext, model.params.maxDistStartToCorridorConnect);
			return 0; // too far away from the channel
		}
		distFromPoint = cosAfter * minDist;
		posUse = minPos;
		if (cosAfter <= 0) {
			p1 = model.network.channelTmp[cNr].point[posUse];
		}
	}

	//if (cosBefore >= cosAfter) {
	//	if (minPrev > model.params.maxDistStartToCorridorConnect) {
	//		errlog("OBS! Not shortening the corridor at startEnd %d, distPrev %.2lf km must be <= %.2lf\n",
	//			startEnd, minPrev, model.params.maxDistStartToCorridorConnect);
	//		return 0; // too far away from the channel
	//	}
	//	distFromPoint = distBastPrev * cosC1;
	//	posUse = minPos - 1;
	//}
	//else {
	//	if (minNext > model.params.maxDistStartToCorridorConnect) {
	//		errlog("OBS! Not shortening the corridor at startEnd %d, distNext %.2lf km must be <= %.2lf\n",
	//			startEnd, minNext, model.params.maxDistStartToCorridorConnect);
	//		return 0; // too far away from the channel
	//	}
	//	distFromPoint = dist2 - distBastNext * cosC2;
	//	posUse = minPos;
	//}
	if (distFromPoint > 0) {
		bearing = model.network.channelTmp[cNr].point[posUse].bearingTo(model.network.channelTmp[cNr].point[posUse + 1]);
		p1 = model.network.channelTmp[cNr].point[posUse].destinationPoint(distFromPoint * 1000, bearing);
	}
	distOld = model.network.channelTmp[cNr].distance_km;
	if(startEnd == 0)
		deltaDist = p1.distanceTo(model.network.channelTmp[cNr].point[posUse + 1]) / 1000.0 - model.network.channelTmp[cNr].distanceFromStart[posUse + 1];
	else
		deltaDist = -distOld - p1.distanceTo(model.network.channelTmp[cNr].point[posUse + 1]) / 1000.0 + model.network.channelTmp[cNr].distanceFromStart[posUse + 1];

	//printf("deltaDist %.2lf distFromStart_posUse+1 %.2lf p1_distTo_posUse+1 %.2lf posUse %d posUse+1_xy %.3lf %.3lf\n", deltaDist,
	//	model.network.channel[cNr].distanceFromStart[posUse + 1], p1.distanceTo(model.network.channel[cNr].point[posUse + 1]),
	//	posUse, model.network.channel[cNr].point[posUse + 1].longitude().degrees(), model.network.channel[cNr].point[posUse + 1].latitude().degrees());
	timeOld = model.network.channelTmp[cNr].timeThroughChannel;
	if (timeOld < -0.1)
		timeNew = timeOld;
	else
		timeNew = timeOld * (distOld + deltaDist) / distOld;

	if(startEnd == 0)
		errlog("update data for corridor. Before it started at %.3lf %.3lf. It is shortened and now starts at %.3lf %.3lf.\n"
			"The time through the corridor is changed from %.2lf to %.2lf since the new distance of the corridor is %.2lf compared to %.2lf before, same for pilot cost and fuel consumption\n",
			model.network.channelTmp[cNr].point_x[0], model.network.channelTmp[cNr].point_y[0],
			p1.longitude().degrees(), p1.latitude().degrees(), timeOld, timeNew, distOld + deltaDist, distOld);
	else
		errlog("update data for corridor. Before it ended at %.3lf %.3lf. It is shortened and now ends at %.3lf %.3lf.\n"
			"The time through the corridor is changed from %.2lf to %.2lf since the new distance of the corridor is %.2lf compared to %.2lf before, same for pilot cost and fuel consumption\n",
			model.network.channelTmp[cNr].point_x[model.network.channelTmp[cNr].nPoints - 1], 
			model.network.channelTmp[cNr].point_y[model.network.channelTmp[cNr].nPoints - 1],
			p1.longitude().degrees(), p1.latitude().degrees(), timeOld, timeNew, distOld + deltaDist, distOld);

	model.network.channelTmp[cNr].distance_km = distOld + deltaDist;
	model.network.channelTmp[cNr].timeThroughChannel = timeNew;
	model.network.channelTmp[cNr].extraCostChannel = model.network.channelTmp[cNr].extraCostChannel * (distOld + deltaDist) / distOld;
	model.network.channelTmp[cNr].totalConsumption = model.network.channelTmp[cNr].totalConsumption * (distOld + deltaDist) / distOld;
	if (startEnd == 0) {
		model.network.channelTmp[cNr].waitingTime = 0;
		model.network.channelTmp[cNr].waiting_consumption_main = 0;
		model.network.channelTmp[cNr].waiting_consumption_aux = 0;
		model.network.channelTmp[cNr].waitingTime = 0; // .intWaitingTime = 0;
		model.network.channelTmp[cNr].arrivalTime_h = -1;
		model.network.channelTmp[cNr].intArrivalTime_h = -1;

		model.network.channelTmp[cNr].point[0] = p1;
		model.network.channelTmp[cNr].point_x[0] = p1.longitude().degrees();
		model.network.channelTmp[cNr].point_y[0] = p1.latitude().degrees();

		//printf("new Point lon/lat %.3lf %.3lf instead of pos %d %.3lf %.3lf\n", model.network.channel[cNr].point_x[0],
		//	model.network.channel[cNr].point_y[0], posUse, model.network.channel[cNr].point_x[posUse],
		//	model.network.channel[cNr].point_y[posUse]);

		for (i = posUse + 1; i < model.network.channelTmp[cNr].nPoints; i++) {
			model.network.channelTmp[cNr].point[i - posUse] = model.network.channelTmp[cNr].point[i];
			model.network.channelTmp[cNr].point_x[i - posUse] = model.network.channelTmp[cNr].point_x[i];
			model.network.channelTmp[cNr].point_y[i - posUse] = model.network.channelTmp[cNr].point_y[i];
			if (i == posUse + 1)
				model.network.channelTmp[cNr].distanceFromStart[i - posUse] =
				model.network.channelTmp[cNr].point[0].distanceTo(model.network.channelTmp[cNr].point[1]) / 1000.0;
			else
				model.network.channelTmp[cNr].distanceFromStart[i - posUse] = model.network.channelTmp[cNr].distanceFromStart[i - posUse - 1] +
				model.network.channelTmp[cNr].distanceFromStart[i] - model.network.channelTmp[cNr].distanceFromStart[i - 1];
		}
		model.network.channelTmp[cNr].nPoints -= posUse;
	}
	else {
		model.network.channelTmp[cNr].point[posUse + 1] = p1;
		model.network.channelTmp[cNr].point_x[posUse + 1] = p1.longitude().degrees();
		model.network.channelTmp[cNr].point_y[posUse + 1] = p1.latitude().degrees();

		errlog("new endPoint %d lon/lat %.3lf %.3lf\n", posUse + 1, model.network.channelTmp[cNr].point_x[posUse + 1],
			model.network.channelTmp[cNr].point_y[posUse + 1]);

		model.network.channelTmp[cNr].distanceFromStart[posUse + 1] = model.network.channelTmp[cNr].distanceFromStart[posUse] +
			model.network.channelTmp[cNr].point[posUse].distanceTo(model.network.channelTmp[cNr].point[posUse + 1]) / 1000.0;

		model.network.channelTmp[cNr].nPoints = posUse + 2;
	}


	//printf("nPoints efter %d\n", model.network.channel[cNr].nPoints);
	//printf("pos %d xy %.3lf %.3lf\n", model.network.channel[cNr].nPoints - 1,
	//	model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1],
	//	model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1]);
	//printf("pos %d xy %.3lf %.3lf\n", model.network.channel[cNr].nPoints - 2,
	//	model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 2],
	//	model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 2]);


	return 1;
}

int checkChannels() {
	int cNr, nCoords, pos0, pos1, i1, pos;


	//for (int i = 0; i < model.network.nPhysicalLevels - 1; i++) {
	//	if (i == 0) {
	//		model.network.physicalLev[i].distanceFromStartPosMid = 0;
	//		pos = (int)model.network.physicalLev[i].nPoints / 2;
	//	}
	//	else
	//		pos = pos1;
	//	pos1 = (int)model.network.physicalLev[i + 1].nPoints / 2;
	//	model.network.physicalLev[i + 1].distanceFromStartPosMid = model.network.physicalLev[i].distanceFromStartPosMid +
	//		model.network.physicalLev[i].point[pos].distanceTo(model.network.physicalLev[i + 1].point[pos1]) / 1000.0;
	//}


	for (cNr = 0; cNr < model.network.nChannels; cNr++) {

		model.network.channel[cNr].straightArcFeasible_toChannelFromPrefPath = 1;
		model.network.channel[cNr].straightArcFeasible_fromChannelToPrefPath = 1;

		//for (i1 = 0; i1 < 2; i1++) {
			//if (model.network.channel[cNr].nPolygonPoints[i1] == 0)
			//	model.network.channel[cNr].nPolygonUsePoints[i1] = 0;
			//else
			//	model.network.channel[cNr].nPolygonUsePoints[i1] = setUpUsablePointsInPolygonChannel(cNr, i1);
		model.network.channel[cNr].nAllocOutNodes = model.params.nPkterOrto * 2;
		model.network.channel[cNr].outNode = (int*)malloc(model.network.channel[cNr].nAllocOutNodes * sizeof(int));
		model.network.channel[cNr].outLevel = (int*)malloc(model.network.channel[cNr].nAllocOutNodes * sizeof(int));
		model.network.channel[cNr].outRestrictedAreaNr = (int*)malloc(model.network.channel[cNr].nAllocOutNodes * sizeof(int));
		model.network.channel[cNr].outNoNormalArc_useTSS = (int*)calloc(model.network.channel[cNr].nAllocOutNodes, sizeof(int));
		
		model.network.channel[cNr].nConnectTo = 0;
		model.network.channel[cNr].nAllocConnectTo = 100;
		model.network.channel[cNr].connectTo_outLevel = (int*)malloc(model.network.channel[cNr].nAllocConnectTo * sizeof(int));
		model.network.channel[cNr].connectTo_outNode = (int*)malloc(model.network.channel[cNr].nAllocConnectTo * sizeof(int));
		model.network.channel[cNr].nConnectFrom = 0;
		model.network.channel[cNr].nAllocConnectFrom = 100;
		model.network.channel[cNr].connectFrom_outLevel = (int*)malloc(model.network.channel[cNr].nAllocConnectFrom * sizeof(int));
		model.network.channel[cNr].connectFrom_outNode = (int*)malloc(model.network.channel[cNr].nAllocConnectFrom * sizeof(int));


		if (model.network.channel[cNr].type == 1) {
			// tss
			continue; // only normal channels need to be checked, not tss 
		}
	
		//}
		nCoords = model.network.channel[cNr].nPoints;

		//if (model.network.channel[cNr].nPolygonUsePoints[0] == 0) { 
		if (eval_coordWithinBoundingBox(model.network.channel[cNr].point_x[0], model.network.channel[cNr].point_y[0]) == 0) {
			errlog("OBS! Corridor %d, startpoint %.3lf %.3lf not within bounding box of preferred path (%.3lf %.3lf %.3lf %.3lf)\n", cNr,
				model.network.channel[cNr].point_x[0], model.network.channel[cNr].point_y[0],
				model.boundingBox.xMin, model.boundingBox.yMin,
				model.boundingBox.xMax, model.boundingBox.yMax);
		}
		// check if start and end point of the channel is in preferred path's allowed area, no => skip
		pos0 = getBastPhysLevelToConnectToChannel(0, cNr);
		if (pos0 < 0) {
			printf("ERROR! Corridor %d, startpoint %.3lf %.3lf does not give a position in the physical network\n", cNr,
				model.network.channel[cNr].point_x[0], model.network.channel[cNr].point_y[0]);
			errlog("ERROR! Corridor %d, startpoint %.3lf %.3lf does not give a position in the physical network\n", cNr,
				model.network.channel[cNr].point_x[0], model.network.channel[cNr].point_y[0]);
			//continue;
		}

		// validate start and end node as valid in feasible network
		//if (check_isCoordFeasiblePhysicalMap(model.network.channel[cNr].point_y[0],
		//	model.network.channel[cNr].point_x[0]) == 0) {
		//	errlog("ERROR! channel starts at a position lat/lon %.3lf %.3lf that is not allowed in the feasible map so we can never use it\n",
		//		model.network.channel[cNr].point_y[0], model.network.channel[cNr].point_x[0]);
		//	continue;
		//}
		//}
		//else {
		//	pos0 = getBastPhysLevelToConnectToChannelPolygon(cNr, 0, 0);
		//}
		//if (model.network.channel[cNr].nPolygonUsePoints[1] == 0) {
		//for (int ii = 0; ii < nCoords; ii++)
		//	printf("cNr %d ii %d xy %.3lf %.3lf\n", cNr, ii, model.network.channel[cNr].point_x[ii], model.network.channel[cNr].point_y[ii]);

		if (eval_coordWithinBoundingBox(model.network.channel[cNr].point_x[nCoords - 1], model.network.channel[cNr].point_y[nCoords - 1]) == 0) {
			errlog("OBS! Corridor %d, endpoint %.3lf %.3lf not within bounding box of preferred path (%.3lf %.3lf %.3lf %.3lf)\n", cNr,
				model.network.channel[cNr].point_x[nCoords - 1], model.network.channel[cNr].point_y[nCoords - 1],
				model.boundingBox.xMin, model.boundingBox.yMin,
				model.boundingBox.xMax, model.boundingBox.yMax);
			// continue;
		}
		// check if start and end point of the channel is in preferred path's allowed area, no => skip
		pos1 = getBastPhysLevelToConnectToChannel(1, cNr); // pos0 + 1);

		if (pos1 < 0 || pos0 < 0) {
			if(pos1 < 0)
				errlog("ERROR! Corridor %d, endpoint %.3lf %.3lf does not give a position in the physical network\n", cNr,
					model.network.channel[cNr].point_x[nCoords - 1], model.network.channel[cNr].point_y[nCoords - 1]);
			model.network.channel[cNr].bastStartLevel = -1;
			model.network.channel[cNr].bastEndLevel = -1;

			continue;
		}

		int legNr1, legNr2, levelNr1, levelNr2;
		levelNr1 = model.network.channel[cNr].bastStartLevel;
		if (levelNr1 >= 0)
			legNr1 = model.network.physicalLev[levelNr1].legNr;
		else
			legNr1 = model.network.channel[-levelNr1 - 1].legNr;
		levelNr2 = model.network.channel[cNr].bastEndLevel;
		if (levelNr2 >= 0)
			legNr2 = model.network.physicalLev[levelNr2].legNr;
		else
			legNr2 = model.network.channel[-levelNr2 - 1].legNr;
		if (legNr1 != legNr2 && 
			!(legNr1 == legNr2 - 1 && model.params.legProperties[legNr1].endNode_exact != 1)) {
			// different legs then not allowed channel, it is okay if the legs follow each other and no exact node requirement on legNr1
			if(model.network.channel[cNr].bastStartLevel >= 0)
				model.network.physicalLev[model.network.channel[cNr].bastStartLevel].followChannelExactly = 0;
			model.network.channel[cNr].bastStartLevel = -1;
			model.network.channel[cNr].bastEndLevel = -1;
			model.network.channel[cNr].earliestStartLevel = 10000;
			model.network.channel[cNr].latestEndLevel = -1;
			continue;
		}

		// validate start and end node as valid in feasible network
		//if (check_isCoordFeasiblePhysicalMap(model.network.channel[cNr].point_y[nCoords - 1],
		//	model.network.channel[cNr].point_x[nCoords - 1]) == 0) {
		//	errlog("ERROR! channel ends at a position lat/lon %.3lf %.3lf that is not allowed in the feasible map so we can never use it\n",
		//		model.network.channel[cNr].point_y[nCoords - 1], model.network.channel[cNr].point_x[nCoords - 1]);
		//	continue;
		//}
		//}
		//else {
		//	pos1 = getBastPhysLevelToConnectToChannelPolygon(cNr, pos0, 1);
		//}

		if (pos0 - 2 < 0)
			model.network.channel[cNr].earliestStartLevel = 0;
		else
			model.network.channel[cNr].earliestStartLevel = pos0 - 2;
		if (pos1 + 2 >= model.network.nPhysicalLevels)
			model.network.channel[cNr].latestEndLevel = model.network.nPhysicalLevels - 1;
		else
			model.network.channel[cNr].latestEndLevel = pos1 + 2;

		if (model.network.channel[cNr].type == 0) {
			if (model.network.channel[cNr].bastStartLevel >= 0) {
				for (i1 = model.network.channel[cNr].bastStartLevel; i1 < model.network.channel[cNr].bastEndLevel; i1++) {
					model.network.physicalLev[i1].requirePrefPathFeasible = 1;
				}
			}
			errlog("corridor %d bound start/end %d %d bast start/end %d %d (sets requirePrefPathFeasible to 1 between these)\n", cNr,
				model.network.channel[cNr].earliestStartLevel, model.network.channel[cNr].latestEndLevel,
				model.network.channel[cNr].bastStartLevel, model.network.channel[cNr].bastEndLevel);
		}else
			errlog("corridor %d bound start/end %d %d bast start/end %d %d (NOT setting requirePrefPathFeasible to 1 between these as it is tss)\n", cNr,
				model.network.channel[cNr].earliestStartLevel, model.network.channel[cNr].latestEndLevel,
				model.network.channel[cNr].bastStartLevel, model.network.channel[cNr].bastEndLevel);
	}


	int i, j, pos_i, pos_i1;
	model.network.channelOrder = (int*)malloc(model.network.nChannels * sizeof(int));
	for (i = 0; i < model.network.nChannels; i++) {
		model.network.channelOrder[i] = i;
	}

	for (j = 0; j < model.network.nChannels; j++) {
		for (i = 0; i < model.network.nChannels - 1; i++) {
			pos_i = model.network.channelOrder[i];
			pos_i1 = model.network.channelOrder[i + 1];
			if (model.network.channel[pos_i].bastStartLevel > model.network.channel[pos_i1].bastStartLevel ||
				(model.network.channel[pos_i].bastStartLevel == model.network.channel[pos_i1].bastStartLevel &&
					model.network.channel[pos_i].preferredPathPoint_posConnectTo > model.network.channel[pos_i1].preferredPathPoint_posConnectTo) ||
				(model.network.channel[pos_i].bastStartLevel == model.network.channel[pos_i1].bastStartLevel &&
					model.network.channel[pos_i].preferredPathPoint_posConnectTo == model.network.channel[pos_i1].preferredPathPoint_posConnectTo &&
					model.network.channel[pos_i].bastStartDist > model.network.channel[pos_i1].bastStartDist))
				SwapArray(model.network.channelOrder, i, i + 1);
		}
	}
	//for (j = 0; j < model.network.nChannels; j++) {
	//	pos_i = model.network.channelOrder[j];
	//		printf("i %d cNr %d level %d posPref %d dist %.2lf\n", j, pos_i,
	//			model.network.channel[pos_i].bastStartLevel,
	//			model.network.channel[pos_i].preferredPathPoint_posConnectTo,
	//			model.network.channel[pos_i].bastStartDist);
	//}

	return 0;
}

int eval_bboxesOverlap() {

	if (model.network.boundingbox.xMax < model.boundingBox.xMin) {
		model.network.boundingbox.xMin += 360;
		model.network.boundingbox.xMax += 360;
	}
	else {
		if (model.network.boundingbox.xMin > model.boundingBox.xMax) {
			model.network.boundingbox.xMin -= 360;
			model.network.boundingbox.xMax -= 360;
		}
	}

	if (model.network.boundingbox.xMin >= model.boundingBox.xMax || model.network.boundingbox.xMax <= model.boundingBox.xMin ||
		model.network.boundingbox.yMin >= model.boundingBox.yMax || model.network.boundingbox.yMax <= model.boundingBox.yMin)
		return 0;


	return 1;

}


double findClosestPrefPathPointToCoord2(double y, double x, int* posPref, int riktning) {
	int i, minPos;
	double dist, minDist2 = 1e20;

	if (riktning == -1) {
		for (i = *posPref; i >= 0; i--) {
			dist = estimateLargeCircleDistance2_km(y, x, model.preferredPath.point_y[i], model.preferredPath.point_x[i]);
			if (minDist2 > dist) {
				minDist2 = dist;
				minPos = i;
				if (minDist2 < 0.001)
					break;
			}
		}
	}
	else {
		for (i = *posPref; i < model.preferredPath.nPoints; i++) {
			if (i == 64)
				i = i;
			dist = estimateLargeCircleDistance2_km(y, x, model.preferredPath.point_y[i], model.preferredPath.point_x[i]);
			if (minDist2 > dist) {
				minDist2 = dist;
				minPos = i;
				if (minDist2 < 0.001)
					break;
			}
		}
	}
	*posPref = minPos;
	return minDist2;
}


double findPointClose(int* posCoord, int* posPrefBas, int riktning) {
	int i, iStart = *posCoord, minDistKrav2 = 5 * 5;

	
	double minDist2 = 1e20, dist;
	int posPref, min_i, min_posPref, minOK_i = -1, minOK_posPref;
	if (riktning == -1) {
		if (iStart >= model.network.nCoords)
			iStart = model.network.nCoords - 1;
		posPref = model.preferredPath.nPoints - 1;
		for (i = iStart; i >= 0; i--) {
			dist = findClosestPrefPathPointToCoord2(model.network.yCoord[i], model.network.xCoord[i], &posPref, riktning);
			if (minDist2 > dist || dist < 0.001) {
				minDist2 = dist;
				min_i = i;
				min_posPref = posPref;
			}
			if (dist < minDistKrav2) {
				minOK_i = i;
				minOK_posPref = posPref;
			}
			else {
				if (minDist2 < minDistKrav2)
					break;
			}
		}
	}
	else {
		if (iStart < 0)
			iStart = 0;
		posPref = 0;
		for (i = iStart; i < model.network.nCoords; i++) {
			dist = findClosestPrefPathPointToCoord2(model.network.yCoord[i], model.network.xCoord[i], &posPref, riktning);
			if (minDist2 > dist || dist < 0.001) {
				minDist2 = dist;
				min_i = i;
				min_posPref = posPref;
			}
			if (dist < minDistKrav2) {
				minOK_i = i;
				minOK_posPref = posPref;
			}
			else {
				if (minDist2 < minDistKrav2)
					break;
			}
		}
	}
	*posCoord = min_i;
	*posPrefBas = min_posPref;
	return minDist2;
}


void getLineEqCoef(double y1, double x1, double y2, double x2, double* a, double* b, double* c) {
	if (x2 - x1 != 0) {
		*a = (y2 - y1) / (x2 - x1);
		*b = -1;
		*c = -(*a) * x2 - (*b) * y2;
	}
	else {
		*a = 1;
		*c = -x2;
		*b = 0;
	}

}

double getClosestPointLines(double y1a, double x1a, double y1b, double x1b, double y2a, double x2a, double y2b, double x2b, double* kvot1, double* kvot2) {
	double a1, b1, c1, a2, b2, c2;

	getLineEqCoef(y1a, x1a, y1b, x1b, &a1, &b1, &c1);
	getLineEqCoef(y2a, x2a, y2b, x2b, &a2, &b2, &c2);

	double kvot = (a1 * b2 - a2 * b1), xIntersect, yIntersect;
	double xp1, yp1, xp2, yp2, dist, vx, vy, ux, uy;
	double dist1, dist2, kvota, kvotb;

	if (abs(kvot) > 0.0000001) {
		xIntersect = (b1 * c2 - b2 * c1) / kvot;
		yIntersect = (c1 * a2 - c2 * a1) / kvot;

		if (x1a - x1b != 0)
			*kvot1 = (xIntersect - x1a) / (x1b - x1a);
		else
			*kvot1 = (yIntersect - y1a) / (y1b - y1a);
		if (x2a - x2b != 0)
			*kvot2 = (xIntersect - x2a) / (x2b - x2a);
		else
			*kvot2 = (yIntersect - y2a) / (y2b - y2a);

		int changed1 = 1;
		if (*kvot1 < 0)
			*kvot1 = 0;
		else {
			if (*kvot1 > 1)
				*kvot1 = 1;
			else
				changed1 = 0;
		}

		int changed2 = 1;
		if (*kvot2 < 0)
			*kvot2 = 0;
		else {
			if (*kvot2 > 1)
				*kvot2 = 1;
			else
				changed2 = 0;
		}

		if (changed1 == 1 && changed2 == 0) {
			//xp1 = x1a * (*kvot1) + (1 - (*kvot1)) * x1b;
			//yp1 = y1a * (*kvot1) + (1 - (*kvot1)) * y1b;
			xp1 = x1b * (*kvot1) + (1 - (*kvot1)) * x1a;
			yp1 = y1b * (*kvot1) + (1 - (*kvot1)) * y1a;
			vx = x2b - x2a;
			vy = y2b - y2a;
			ux = x2a - xp1;
			uy = y2a - yp1;
			*kvot2 = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
			xp2 = x2b * (*kvot2) + (1 - (*kvot2)) * x2a;
			yp2 = y2b * (*kvot2) + (1 - (*kvot2)) * y2a;
			dist = estimateLargeCircleDistance_km(yp1, xp1, yp2, xp2);
		}
		else {
			if (changed2 == 1 && changed1 == 0) {
				//xp2 = x2a * (*kvot2) + (1 - (*kvot2)) * x2b;
				//yp2 = y2a * (*kvot2) + (1 - (*kvot2)) * y2b;
				xp2 = x2b * (*kvot2) + (1 - (*kvot2)) * x2a;
				yp2 = y2b * (*kvot2) + (1 - (*kvot2)) * y2a;
				vx = x1b - x1a;
				vy = y1b - y1a;
				ux = x1a - xp2;
				uy = y1a - yp2;
				*kvot1 = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
				xp1 = x1b * (*kvot1) + (1 - (*kvot1)) * x1a;
				yp1 = y1b * (*kvot1) + (1 - (*kvot1)) * y1a;
				dist = estimateLargeCircleDistance_km(yp1, xp1, yp2, xp2);
			}
			else {
				if (changed1 == 0 && changed2 == 0)
					dist = 0.0;
				else {
					xp1 = x1b * (*kvot1) + (1 - (*kvot1)) * x1a;
					yp1 = y1b * (*kvot1) + (1 - (*kvot1)) * y1a;
					xp2 = x2b * (*kvot2) + (1 - (*kvot2)) * x2a;
					yp2 = y2b * (*kvot2) + (1 - (*kvot2)) * y2a;
					dist = estimateLargeCircleDistance_km(yp1, xp1, yp2, xp2);
				}
			}
		}
	}
	else {

		dist1 = estimateLargeCircleDistance2_km(y1a, x1a, y1b, y1b);
		dist2 = estimateLargeCircleDistance2_km(y2a, x2a, y2b, y2b);
		if (dist1 <= dist2) {
			vx = x2b - x2a;
			vy = y2b - y2a;
			ux = x2a - x1a;
			uy = y2a - y1a;
			kvota = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
			ux = x2a - x1b;
			uy = y2a - y1b;
			kvotb = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
			if (kvota >= 0 && kvota <= 1) {
				*kvot1 = 0.0;
				*kvot2 = kvota;
			}
			else {
				if (kvotb >= 0 && kvotb <= 1) {
					*kvot1 = 1.0;
					*kvot2 = kvotb;
				}
				else {
					if (kvota < 0) {
						if (kvota < kvotb)
							*kvot1 = 1.0;
						else
							*kvot1 = 0.0;
						*kvot2 = 0.0;
					}
					else {
						if (kvota > kvotb)
							*kvot1 = 1.0;
						else
							*kvot1 = 0.0;
						*kvot2 = 1.0;
					}
				}
			}
		}
		else {
			vx = x1b - x1a;
			vy = y1b - y1a;
			ux = x1a - x2a;
			uy = y1a - y2a;
			kvota = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
			ux = x1a - x2b;
			uy = y1a - y2b;
			kvotb = -(vx * ux + vy * uy) / (vx * vx + vy * vy);
			if (kvota >= 0 && kvota <= 1) {
				*kvot2 = 0.0;
				*kvot1 = kvota;
			}
			else {
				if (kvotb >= 0 && kvotb <= 1) {
					*kvot2 = 1.0;
					*kvot1 = kvotb;
				}
				else {
					if (kvota < 0) {
						if (kvota < kvotb)
							*kvot2 = 1.0;
						else
							*kvot2 = 0.0;
						*kvot1 = 0.0;
					}
					else {
						if (kvota > kvotb)
							*kvot2 = 1.0;
						else
							*kvot2 = 0.0;
						*kvot1 = 1.0;
					}
				}
			}

		}
		xp1 = x1b * (*kvot1) + (1 - (*kvot1)) * x1a;
		yp1 = y1b * (*kvot1) + (1 - (*kvot1)) * y1a;
		xp2 = x2b * (*kvot2) + (1 - (*kvot2)) * x2a;
		yp2 = y2b * (*kvot2) + (1 - (*kvot2)) * y2a;
		dist = estimateLargeCircleDistance_km(yp1, xp1, yp2, xp2);
	}

	return dist;
}

int comparePoints_equal(strClosePoints* p1, strClosePoints* p2) {
	if (p1->posCoords == p2->posCoords && abs(p1->kvotCoords - p2->kvotCoords) < 0.0001)
		return 1;

	if (p1->posCoords == p2->posCoords - 1 && abs(p1->kvotCoords - p2->kvotCoords - 1) < 0.0001)
		return 1;

	if (p1->posCoords == p2->posCoords + 1 && abs(p1->kvotCoords - p2->kvotCoords + 1) < 0.0001)
		return 1;

	return 0;
}

int getNextPointCoords(strClosePoints* point, double riktning, double* y, double* x, int update) {
	double kvot;
	int pos;

	if (riktning < 0) {
		if (point->kvotCoords > 0.01) {
			kvot = point->kvotCoords + riktning * point->kvotCoords;
			pos = point->posCoords;
		}
		else {
			kvot = 1 + riktning;
			pos = point->posCoords - 1;
			if (pos < 0)
				pos = 0;
		}
	}
	else {
		kvot = point->kvotCoords + riktning * (1 - point->kvotCoords);
		if (kvot > 0.99) {
			kvot = 0.0;
			pos = point->posCoords + 1;
			if (pos >= model.network.nCoords)
				pos = model.network.nCoords - 1;
		}
		else
			pos = point->posCoords;
	}

	if (update == 1) {
		point->posCoords = pos;
		point->kvotCoords = kvot;
		return 0;
	}

	if (pos < model.network.nCoords - 1) {
		*x = model.network.xCoord[pos] * (1 - kvot) + kvot * model.network.xCoord[pos + 1];
		*y = model.network.yCoord[pos] * (1 - kvot) + kvot * model.network.yCoord[pos + 1];
	}
	else {
		*x = model.network.xCoord[pos];
		*y = model.network.yCoord[pos];
	}

	return 0;
}

int getNextPointPrefPath(strClosePoints* point, double riktning, double* y, double* x, int update) {
	double kvot;
	int pos;

	if (riktning < 0) {
		if (point->kvotPrefPath > 0.01) {
			kvot = point->kvotPrefPath + riktning * point->kvotPrefPath;
			pos = point->posPrefPath;
		}
		else {
			kvot = 1 + riktning;
			pos = point->posPrefPath - 1;
			if (pos < 0)
				pos = 0;
		}
	}
	else {
		kvot = point->kvotPrefPath + riktning * (1 - point->kvotPrefPath);
		if (kvot > 0.99) {
			kvot = 0.0;
			pos = point->posPrefPath + 1;
			if (pos >= model.preferredPath.nPoints)
				pos = model.preferredPath.nPoints - 1;
		}
		else
			pos = point->posPrefPath;
	}

	if (update == 1) {
		point->posPrefPath = pos;
		point->kvotPrefPath = kvot;
		return 0;
	}

	if (pos < model.preferredPath.nPoints - 1) {
		*x = model.preferredPath.point_x[pos] * (1 - kvot) + kvot * model.preferredPath.point_x[pos + 1];
		*y = model.preferredPath.point_y[pos] * (1 - kvot) + kvot * model.preferredPath.point_y[pos + 1];
	}
	else {
		*x = model.preferredPath.point_x[pos];
		*y = model.preferredPath.point_y[pos];
	}

	return 0;
}

int findPreviousNext_closePoint_prefPathToCoords(strClosePoints* point, int riktning) {
	double maxDist0 = 1.0, distNu, distNu1, distNu2, dist1, dist2, maxDist2 = 25.0;
	double y1a, y1b, y2a, y2b, x1a, x1b, x2a, x2b;

	// get previous point in tss
	getNextPointCoords(point, riktning, &y1b, &x1b, 0);
	// get previous point in prefPath
	getNextPointPrefPath(point, riktning, &y2b, &x2b, 0);
	// printf("tss %.3lf %.3lf prefPath %.3lf %.3lf\n", y1b, x1b, y2b, x2b);
	distNu1 = estimateLargeCircleDistance_km(y1b, x1b, y2b, x2b);
	if (distNu1 > maxDist0) {
		getNextPointCoords(point, 0, &y1a, &x1a, 0);
		getNextPointPrefPath(point, 0, &y2a, &x2a, 0);
		dist1 = estimateLargeCircleDistance_km(y1a, x1a, y1b, x1b);
		dist2 = estimateLargeCircleDistance_km(y2a, x2a, y2b, x2b);
		distNu2 = estimateLargeCircleDistance_km(y1a, x1a, y2a, x2a);
		// identifiera den som ar mest begransande i hur langt man kan ga
		// ga sa langt
		// om inte for langt mellan punkterna sa uppdatera punkten och return 1, annars return 0
		if (dist1 <= dist2) {
			if (dist2 < 0.000001) {
				dist1 = 0.000001;
				dist2 = 0.000001;
			}
			getNextPointPrefPath(point, riktning * dist1 / dist2, &y2b, &x2b, 0);
			distNu = estimateLargeCircleDistance_km(y1b, x1b, y2b, x2b);
			if (distNu > maxDist2 || (distNu > dist1 && distNu > maxDist0)) {
				if (distNu < distNu2 + dist1 * 0.25 + 0.01) {
					getNextPointCoords(point, riktning, &y1b, &x1b, 1);
					getNextPointPrefPath(point, riktning * dist1 / dist2, &y2b, &x2b, 1);
				}
				return 0;
			}
			getNextPointCoords(point, riktning, &y1b, &x1b, 1);
			getNextPointPrefPath(point, riktning * dist1 / dist2, &y2b, &x2b, 1);
		}
		else {
			getNextPointCoords(point, riktning * dist2 / dist1, &y1b, &x1b, 0);
			distNu = estimateLargeCircleDistance_km(y1b, x1b, y2b, x2b);
			if (distNu > maxDist2 || (distNu > dist2 && distNu > maxDist0)) {
				if (distNu < distNu2 + dist2 * 0.25 + 0.01) {
					getNextPointCoords(point, riktning * dist2 / dist1, &y1b, &x1b, 1);
					getNextPointPrefPath(point, riktning, &y2b, &x2b, 1);
				}
				return 0;
			}
			getNextPointPrefPath(point, riktning, &y2b, &x2b, 1);
			getNextPointCoords(point, riktning * dist2 / dist1, &y1b, &x1b, 1);
		}
	}
	else {
		getNextPointCoords(point, riktning, &y1b, &x1b, 1);
		getNextPointPrefPath(point, riktning, &y2b, &x2b, 1);
	}

	return 1;
}


int findClosePoints_prefPathToCoords(strClosePoints* closePoints) {
	int i, i1, minPos_i, minPos_i1, min_i, min_i1;
	double dist, minDist = 1e20, minKvot_i, minKvot_i1, kvot_i, kvot_i1;
	double minDistKrav2 = 30; // changed pfg 20250530 from  1;

	for (i = 0; i < model.network.nCoords; i++) {
		i1 = 0;
		if (i == 37)
			i = i;
		dist = findClosestPrefPathPointToCoord2(model.network.yCoord[i], model.network.xCoord[i], &i1, 1);
		//for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
		//	dist = estimateLargeCircleDistance2_km(model.network.yCoord[i], model.network.xCoord[i],
		//		model.network.physicalLev[i1].point_y[model.params.preferredPathOrtoPos[i1]],
		//		model.network.physicalLev[i1].point_x[model.params.preferredPathOrtoPos[i1]]);
		if (minDist > dist) {
			minDist = dist;
			minPos_i = i;
			minPos_i1 = i1;
			if (minDist < minDistKrav2)
				break;
		}
	}

	if (minPos_i < 1)
		minPos_i = 1;
	if (minPos_i1 < 1)
		minPos_i1 = 1;
	minDist = 1e20;
	int maxPos_i = minPos_i, maxPos_i1 = minPos_i1;
	if (maxPos_i >= model.network.nCoords - 2)
		maxPos_i = model.network.nCoords - 2;
	if (maxPos_i1 >= model.preferredPath.nPoints - 2)
		maxPos_i1 = model.preferredPath.nPoints - 2;
	for (i = minPos_i - 1; i <= maxPos_i; i++) {
		for (i1 = minPos_i1 - 1; i1 <= maxPos_i1; i1++) {
			dist = getClosestPointLines(model.network.yCoord[i], model.network.xCoord[i], model.network.yCoord[i + 1], model.network.xCoord[i + 1],
				model.preferredPath.point_y[i1], model.preferredPath.point_x[i1],
				model.preferredPath.point_y[i1 + 1], model.preferredPath.point_x[i1 + 1], &kvot_i, &kvot_i1);
			if (minDist > dist) {
				minDist = dist;
				min_i = i;
				min_i1 = i1;
				minKvot_i = kvot_i;
				minKvot_i1 = kvot_i1;
			}
		}
	}


	if (minDist > minDistKrav2)
		return 0; // not close enough to prefPath

	closePoints->posCoords = min_i;
	closePoints->posPrefPath = min_i1;
	closePoints->kvotCoords = minKvot_i;
	closePoints->kvotPrefPath = minKvot_i1;
	closePoints->distance = minDist;

	return 1;
}



int copyClosePoints(strClosePoints* pTo, strClosePoints pFrom) {
	pTo->posCoords = pFrom.posCoords;
	pTo->posPrefPath = pFrom.posPrefPath;
	pTo->kvotCoords = pFrom.kvotCoords;
	pTo->kvotPrefPath = pFrom.kvotPrefPath;
	return 0;
}

int comparePathsNetworkCoords(strClosePoints* firstPoints, strClosePoints* lastPoints) {
	int i, i1, i1Start, nIter, minPos_i, minPos_i1;
	double dist, minDist, maxDist2 = 200 * 200, minDistKrav2 = 5 * 5;
	strClosePoints closePoints;
	int newPointsFound, distOK, nMaxIter;

	distOK = findClosePoints_prefPathToCoords(&closePoints);
	if (distOK == 0)
		return distOK;

	copyClosePoints(firstPoints, closePoints);

	nMaxIter = model.network.nCoords + model.network.nPhysicalLevels;

	for (i = 0; i < nMaxIter; i++) {
		newPointsFound = findPreviousNext_closePoint_prefPathToCoords(firstPoints, -1);
		if (newPointsFound == 0)
			break;
	}
	copyClosePoints(lastPoints, closePoints);
	for (i = 0; i < nMaxIter; i++) {
		if (i == 5)
			i = i;
		newPointsFound = findPreviousNext_closePoint_prefPathToCoords(lastPoints, 1);
		if (newPointsFound == 0)
			break;
	}

	if (comparePoints_equal(firstPoints, lastPoints) == 1)
		return 0; // do not add the tss as it is probably in the wrong direction

	return 1;

}

std::string CURRENT_TSS_NAME = "";

int checkTssRightArea(strClosePoints* firstPoints, strClosePoints* lastPoints) {
	int useTss = 0, pos0, pos1;

	// check if bbox in
	useTss = eval_bboxesOverlap();
	if (useTss == 0) {
		errlog("tss '%s' NOT used: bounding box does not overlap the preferred path bounding box\n", CURRENT_TSS_NAME.c_str());
		return useTss; // not using this tss as its bounding box is not overlapping prefPath bounding box
	}

	useTss = comparePathsNetworkCoords( firstPoints, lastPoints);
	if (useTss == 0) {
		errlog("tss '%s' NOT used: not close enough to the preferred path (max %.1lf km) or wrong direction\n",
			CURRENT_TSS_NAME.c_str(), 30.0);
		return useTss; // not close enough to prefPath
	}



	return useTss;
}


int checkIfSwapChannelsTmp(int i, int i1) {
	if (model.network.channelTmp[i].bastStartLevel > model.network.channelTmp[i1].bastStartLevel)
		return 1;
	if (model.network.channelTmp[i].bastStartLevel < model.network.channelTmp[i1].bastStartLevel)
		return 0;

	// samma bastStartLevel
	if (model.network.channelTmp[i].bastStartPointPos > model.network.channelTmp[i1].bastStartPointPos)
		return 1;
	if (model.network.channelTmp[i].bastStartPointPos < model.network.channelTmp[i1].bastStartPointPos)
		return 0;

	// samma bastStartPointPos
	if (model.network.channelTmp[i].bastStartDist > model.network.channelTmp[i1].bastStartDist + 0.001)
		return 1;

	if (model.network.channelTmp[i].bastStartDist < model.network.channelTmp[i1].bastStartDist - 0.001)
		return 0;

	if (model.network.channelTmp[i].distance_km > model.network.channelTmp[i1].distance_km + 0.001)
		return 1;


	return 0;
}

void swapOrder(int* order, int i, int i1) {
	int tmp = order[i];
	order[i] = order[i1];
	order[i1] = tmp;
}

void copyChannelFromTmp(int cNr, int pos) {
	int i;

	model.network.channel[cNr].point_x = (double*)malloc(model.network.channelTmp[pos].nPoints * sizeof(double));
	model.network.channel[cNr].point_y = (double*)malloc(model.network.channelTmp[pos].nPoints * sizeof(double));
	model.network.channel[cNr].distanceFromStart = (double*)malloc(model.network.channelTmp[pos].nPoints * sizeof(double));
	model.network.channel[cNr].point = (spherical::Point*)malloc(model.network.channelTmp[pos].nPoints * sizeof(spherical::Point));

	for (i = 0; i < model.network.channelTmp[pos].nPoints; i++) {
		model.network.channel[cNr].point_x[i] = model.network.channelTmp[pos].point_x[i];
		model.network.channel[cNr].point_y[i] = model.network.channelTmp[pos].point_y[i];
		model.network.channel[cNr].point[i] = spherical::Point(model.network.channel[cNr].point_y[i], model.network.channel[cNr].point_x[i]);
		model.network.channel[cNr].distanceFromStart[i] = model.network.channelTmp[pos].distanceFromStart[i];
	}
	model.network.channel[cNr].nPoints = model.network.channelTmp[pos].nPoints;
	model.network.channel[cNr].distance_km = model.network.channelTmp[pos].distance_km;

	model.network.channel[cNr].StartLevelOnlyPrefPath = model.network.channelTmp[pos].StartLevelOnlyPrefPath;
	model.network.channel[cNr].EndLevelOnlyPrefPath = model.network.channelTmp[pos].EndLevelOnlyPrefPath;

	if (cNr == 8)
		cNr = cNr;
	model.network.channel[cNr].bastStartLevel = model.network.channelTmp[pos].bastStartLevel;
	model.network.channel[cNr].legNr = model.network.channelTmp[pos].legNr;
	model.network.channel[cNr].bastEndLevel = model.network.channelTmp[pos].bastEndLevel;
	model.network.channel[cNr].bastStartPointPos = model.network.channelTmp[pos].bastStartPointPos;
	model.network.channel[cNr].bastStartDist = model.network.channelTmp[pos].bastStartDist;
	model.network.channel[cNr].bastEndPointPos = model.network.channelTmp[pos].bastEndPointPos;
	model.network.channel[cNr].bastEndDist = model.network.channelTmp[pos].bastEndDist;
	model.network.channel[cNr].earliestStartLevel = model.network.channelTmp[pos].earliestStartLevel;
	model.network.channel[cNr].latestEndLevel = model.network.channelTmp[pos].latestEndLevel;
	model.network.channel[cNr].kvotCost = model.network.channelTmp[pos].kvotCost;
	model.network.channel[cNr].kvotMinCost = model.network.channelTmp[pos].kvotMinCost;
	model.network.channel[cNr].preferredPathPoint_posConnectFrom = model.network.channelTmp[pos].preferredPathPoint_posConnectFrom;
	model.network.channel[cNr].preferredPathPoint_posConnectTo = model.network.channelTmp[pos].preferredPathPoint_posConnectTo;
	model.network.channel[cNr].keepPrefPath_tss = model.network.channelTmp[pos].keepPrefPath_tss;

}


int check_if_include_tss(int* order, int i, int nC) {
	int i1, pos1, pos2, include_tss = 1;
	pos1 = order[i];

	if (i < nC - 1) {
		pos2 = order[i + 1];
		if(model.network.channelTmp[pos1].bastStartLevel == model.network.channelTmp[pos2].bastStartLevel &&
			model.network.channelTmp[pos1].bastStartPointPos == model.network.channelTmp[pos2].bastStartPointPos &&
			abs(model.network.channelTmp[pos1].bastStartDist - model.network.channelTmp[pos2].bastStartDist) < 0.001)
			include_tss = 0;
	}
	for (i1 = i - 1; i1 >= 0; i1--) {
		pos2 = order[i1];
		if (model.network.channelTmp[pos2].include_tssTmp == 1 &&
			model.network.channelTmp[pos1].bastEndLevel == model.network.channelTmp[pos2].bastEndLevel &&
			model.network.channelTmp[pos1].bastEndPointPos == model.network.channelTmp[pos2].bastEndPointPos &&
			abs(model.network.channelTmp[pos1].bastEndDist - model.network.channelTmp[pos2].bastEndDist) < 0.001) {
			include_tss = 0;
			break;
		}
	}

	return include_tss;
}

int sortChannelsInOrder(int* order, int nC) {
	int i, include_tss;
	int cNr = model.network.nChannels;

	if (nC + model.network.nChannels > model.network.nAllocChannels) {
		model.network.nAllocChannels = nC + model.network.nChannels;
		model.network.channel = (strChannel*)realloc(model.network.channel, model.network.nAllocChannels * sizeof(strChannel));
	}

	for (i = 0; i < nC; i++) {
		include_tss = check_if_include_tss(order, i, nC);
		model.network.channelTmp[order[i]].include_tssTmp = include_tss;
	}

	for (i = 0; i < nC; i++) {
		if (model.network.channelTmp[order[i]].include_tssTmp == 0) {
			errlog("tss channelTmp %d (levels %d-%d) NOT included: another tss connects at the same start or end point\n",
				order[i], model.network.channelTmp[order[i]].bastStartLevel, model.network.channelTmp[order[i]].bastEndLevel);
			continue; // do not include this tss, pref path follows another one longer
		}

		copyChannelFromTmp(cNr, order[i]);

		model.network.channel[cNr].type = 1;
		model.network.channel[cNr].straightArcFeasible_toChannelFromPrefPath = 1;
		model.network.channel[cNr].straightArcFeasible_fromChannelToPrefPath = 1;

		model.network.channel[cNr].nOutNodes = 0;
		model.network.channel[cNr].outNode = (int*)malloc2(model.params.nPkterOrto * 2 * sizeof(int));
		model.network.channel[cNr].outLevel = (int*)malloc2(model.params.nPkterOrto * 2 * sizeof(int));
		model.network.channel[cNr].outRestrictedAreaNr = (int*)malloc2(model.params.nPkterOrto * 2 * sizeof(int));
		model.network.channel[cNr].outNoNormalArc_useTSS = (int*)calloc(model.params.nPkterOrto * 2, sizeof(int));
		
		model.network.channel[cNr].extraCostChannel = 0.0;
		model.network.channel[cNr].ID = str_alloc_cpy("tss");
		model.network.channel[cNr].timeThroughChannel = -1.0;
		model.network.channel[cNr].waitingTime = 0.0;
		model.network.channel[cNr].waiting_consumption_main = 0.0;
		model.network.channel[cNr].waiting_consumption_aux = 0.0;
		model.network.channel[cNr].totalConsumption = -1.0;
		model.network.channel[cNr].ECA_type = -1;
		model.network.channel[cNr].followExactly = 0;
		model.network.channel[cNr].arrivalTime_h = -1.0;
		model.network.channel[cNr].intArrivalTime_h = -1;
		model.network.channel[cNr].factorDelayedPrefPathAfter = 0;
		model.network.channel[cNr].factorDelayedPrefPathDuring = 0;

		model.network.channel[cNr].midTimeArrive = 0;
		model.network.channel[cNr].midTimeFinish = 0;

		model.network.channel[cNr].nodNr_from_pt = (int**)malloc2(2 * sizeof(int*));
		model.network.channel[cNr].nTimeIntervals = (int*)malloc2(2 * sizeof(int));
		model.network.channel[cNr].nAllocTimeIntervals = (int*)malloc2(2 * sizeof(int));
		model.network.channel[cNr].timeInterval = (int**)malloc2(2 * sizeof(int*));

		for (int i3 = 0; i3 < 2; i3++) { // start och endnod i channel
			model.network.channel[cNr].nTimeIntervals[i3] = 0;
			model.network.channel[cNr].nAllocTimeIntervals[i3] = 100;
			model.network.channel[cNr].timeInterval[i3] = (int*)malloc2(
				model.network.channel[cNr].nAllocTimeIntervals[i3] * sizeof(int));
			model.network.channel[cNr].nodNr_from_pt[i3] = (int*)malloc2(
				model.network.channel[cNr].nAllocTimeIntervals[i3] * sizeof(int));
		}


		cNr++;
	}
	model.network.nChannels = cNr;
	return 0;
}

int sort_tssChannels(int nC) {
	int i, j;

	//printf("innan sort tss channels\n");
	//for (i = 0; i < nC; i++) {
	//	printf("cNr %d bast level pos dist %d %d %.2lf nPkter %d xy %.3lf %.3lf %.3lf %.3lf\n",
	//		i, model.network.channelTmp[i].bastStartLevel, model.network.channelTmp[i].bastStartPointPos,
	//		model.network.channelTmp[i].bastStartDist, model.network.channelTmp[i].nPoints,
	//		model.network.channelTmp[i].point_x[0], model.network.channelTmp[i].point_y[0],
	//		model.network.channelTmp[i].point_x[model.network.channelTmp[i].nPoints - 1],
	//		model.network.channelTmp[i].point_y[model.network.channelTmp[i].nPoints - 1]);
	//}

	int* order = (int*)malloc(nC * sizeof(int));
	for (j = 0; j < nC; j++) {
		//printf("tss %d nPkter %d last_x %lf\n", j, model.network.channelTmp[j].nPoints,
		//	model.network.channelTmp[j].point_x[model.network.channelTmp[j].nPoints - 1]);
		order[j] = j;
	}
	for (j = 0; j < nC; j++) {
		for (i = 0; i < nC - 1; i++) {
			if(checkIfSwapChannelsTmp(order[i], order[i + 1]) == 1){
				swapOrder(order, i, i + 1);
			}
		}
	}

	//for (i = 0; i < nC; i++)
	//	printf("new order %d %d\n", i, order[i]);

	sortChannelsInOrder(order, nC);

	//printf("\nefter sort tss channels\n");
	//for (i = 0; i < nC; i++) {
	//	printf("cNr %d type %d bast level pos dist %d %d %.2lf nPkter %d xy %.3lf %.3lf %.3lf %.3lf\n",
	//		i, model.network.channel[i].type, model.network.channel[i].bastStartLevel, model.network.channel[i].bastStartPointPos,
	//		model.network.channel[i].bastStartDist, model.network.channel[i].nPoints,
	//		model.network.channel[i].point_x[0], model.network.channel[i].point_y[0],
	//		model.network.channel[i].point_x[model.network.channel[i].nPoints - 1],
	//		model.network.channel[i].point_y[model.network.channel[i].nPoints - 1]);
	//}

	return 0;
}


int addSplitTss(int* cNrUse, double kvotCost, double kvotMinCost, strClosePoints firstPoints, strClosePoints lastPoints, int keepPrefPath) {

	// add the tss from firstPoints to lastPoints
	int nAlloc = lastPoints.posCoords - firstPoints.posCoords + 2, pos;

	// split the channel about every 8 hours;
	// I'm not splitting the tss for now...
	// for all splits before the last one, set
	// model.network.channel[cNr].connectToNextChannel = 1;
	// for the last one, set model.network.channel[cNr].connectToNextChannel = 0;
	// for all splits except the first one, set
	// model.network.channel[cNr].connectToPrevChannel = 1;
	// for the first one, set model.network.channel[cNr].connectToPrevChannel = 0;
	// for all tss channels set model.network.channel[cNr].type = 1;
	// 
	// for normal channels, model.network.channel[cNr].connectToNextChannel = 0;
	// for normal channels, model.network.channel[cNr].connectToPrevChannel = 0;
	// for normal channels, model.network.channel[cNr].type = 0;

	int cNr = *cNrUse, i;
	model.network.channelTmp[cNr].point_x = (double*)malloc(nAlloc * sizeof(double));
	model.network.channelTmp[cNr].point_y = (double*)malloc(nAlloc * sizeof(double));
	model.network.channelTmp[cNr].point = (spherical::Point*)malloc(nAlloc * sizeof(spherical::Point));
	model.network.channelTmp[cNr].distanceFromStart = (double*)malloc2(nAlloc * sizeof(double));

	if (cNr == 9)
		cNr = cNr;
	pos = 0;
	double distance;
	for (i = firstPoints.posCoords; i < lastPoints.posCoords + 2; i++) {
		if (i == firstPoints.posCoords) {
			if (firstPoints.kvotCoords > 0) {
				model.network.channelTmp[cNr].point_x[pos] = model.preferredPath.point_x[firstPoints.posPrefPath];
				model.network.channelTmp[cNr].point_y[pos] = model.preferredPath.point_y[firstPoints.posPrefPath];
			}
			else {
				model.network.channelTmp[cNr].point_x[pos] = model.network.xCoord[i];
				model.network.channelTmp[cNr].point_y[pos] = model.network.yCoord[i];
			}
		}
		else {
			if (i == lastPoints.posCoords)
				i = i;
			if (i == lastPoints.posCoords + 1) {
				if (lastPoints.kvotCoords > 0) {
					model.network.channelTmp[cNr].point_x[pos] = model.preferredPath.point_x[lastPoints.posPrefPath];
					model.network.channelTmp[cNr].point_y[pos] = model.preferredPath.point_y[lastPoints.posPrefPath];
				}
				else
					continue;
			}
			else {
				model.network.channelTmp[cNr].point_x[pos] = model.network.xCoord[i];
				model.network.channelTmp[cNr].point_y[pos] = model.network.yCoord[i];
			}
		}
		model.network.channelTmp[cNr].point[pos] = spherical::Point(model.network.channelTmp[cNr].point_y[pos], model.network.channelTmp[cNr].point_x[pos]);
		if (pos == 0)
			distance = 0;
		else
			distance += model.network.channelTmp[cNr].point[pos - 1].distanceTo(model.network.channelTmp[cNr].point[pos]) / 1000.0;
		model.network.channelTmp[cNr].distanceFromStart[pos] = distance;
		//printf("cNr %d pos %d (i %d) xy %.3lf %.3lf\n", cNr, pos, i, model.network.channelTmp[cNr].point_y[pos],
		//	model.network.channelTmp[cNr].point_x[pos]);
		pos++;
	}
	model.network.channelTmp[cNr].nPoints = pos;
	model.network.channelTmp[cNr].distance_km = distance;
	if (cNr == 4)
		cNr = cNr;

	model.network.channelTmp[cNr].StartLevelOnlyPrefPath = 0;
	model.network.channelTmp[cNr].EndLevelOnlyPrefPath = 0;

	model.network.channelTmp[cNr].bastStartPointPos = -2;
	model.network.channelTmp[cNr].bastStartDist = 1e10;
	model.network.channelTmp[cNr].bastEndPointPos = -2;
	model.network.channelTmp[cNr].bastEndDist = 1e10;

	int pos0 = getBastPhysLevelToConnectToChannel_tss(0, cNr);
	if (pos0 < 0) {
		errlog("tss '%s' NOT used: no physical level found to connect the tss start to\n", CURRENT_TSS_NAME.c_str());
		return -1; // not an interesting tss
	}
	int pos1 = getBastPhysLevelToConnectToChannel_tss(1, cNr);
	if (pos1 < 0) {
		errlog("tss '%s' NOT used: no physical level found to connect the tss end to\n", CURRENT_TSS_NAME.c_str());
		return -1; // not an interesting tss
	}

	int legNr1, legNr2, levelNr1, levelNr2;
	levelNr1 = model.network.channelTmp[cNr].bastStartLevel;
	if (levelNr1 >= 0)
		legNr1 = model.network.physicalLev[levelNr1].legNr;
	else
		legNr1 = model.network.channel[-levelNr1 - 1].legNr;
	levelNr2 = model.network.channelTmp[cNr].bastEndLevel;
	if (levelNr2 >= 0)
		legNr2 = model.network.physicalLev[levelNr2].legNr;
	else
		legNr2 = model.network.channel[-levelNr2 - 1].legNr;
	if (legNr1 != legNr2) {
		errlog("tss '%s' NOT used: start connects to leg %d (level %d) but end to leg %d (level %d)\n",
			CURRENT_TSS_NAME.c_str(), legNr1, levelNr1, legNr2, levelNr2);
		model.network.channelTmp[cNr].bastStartLevel = -1;
		model.network.channelTmp[cNr].bastEndLevel = -1;
		return -1; // not an interesting tss as it connects to different legs.
	}

	errlog("tss '%s' ADDED as channelTmp %d: levels %d-%d (pointPos %d-%d), %.1lf km, %d points\n",
		CURRENT_TSS_NAME.c_str(), cNr, model.network.channelTmp[cNr].bastStartLevel,
		model.network.channelTmp[cNr].bastEndLevel, model.network.channelTmp[cNr].bastStartPointPos,
		model.network.channelTmp[cNr].bastEndPointPos, model.network.channelTmp[cNr].distance_km,
		model.network.channelTmp[cNr].nPoints);

	if (pos1 <= pos0)
		pos0 = pos0;

	if (pos0 < 0)
		model.network.channelTmp[cNr].earliestStartLevel = 0;
	else
		model.network.channelTmp[cNr].earliestStartLevel = pos0; // -2;

	// model.network.physicalLev[model.network.channelTmp[cNr].earliestStartLevel].onlyPrefPath = 1; //pfg 20250529


	if (pos1 >= model.network.nPhysicalLevels)
		model.network.channelTmp[cNr].latestEndLevel = model.network.nPhysicalLevels - 1;
	else
		model.network.channelTmp[cNr].latestEndLevel = pos1; // +2;

	model.network.channelTmp[cNr].kvotCost = kvotCost;
	model.network.channelTmp[cNr].kvotMinCost = kvotMinCost;
	model.network.channelTmp[cNr].keepPrefPath_tss = keepPrefPath;


	*cNrUse = cNr + 1;
	return 0;


}

int ok_connect_tss_to_channel(int cNr, int alt) {
	int cNr2, pos, arcOK = 0, pos2;
	if (alt == 0) {
		pos = 0;
		for (cNr2 = 0; cNr2 < model.network.nChannels; cNr2++) {
			if (cNr == cNr2)
				continue;
			pos2 = model.network.channel[cNr2].nPoints - 1;
			arcOK = check_isPhysicalArcOK(-cNr2 - 1, -cNr - 1, pos2, pos, 1);
			if (arcOK == 1)
				break;
		}
	}
	else {
		pos = model.network.channel[cNr].nPoints - 1;
		for (cNr2 = 0; cNr2 < model.network.nChannels; cNr2++) {
			if (cNr == cNr2)
				continue;
			pos2 = 0;
			arcOK = check_isPhysicalArcOK(-cNr - 1, -cNr2 - 1, pos, pos2, 1);
			if (arcOK == 1)
				break;
		}
	}
	return arcOK;
}


int check_tss_startSlutLevelOnlyPrefPath() {
	int cNr, isAllowed, posTss, bastLevel;

	for (cNr = 0; cNr < model.network.nChannels; cNr++) {
		if (model.network.channel[cNr].type == 0)
			continue; // only fix tss
		if (cNr == 8)
			cNr = cNr;
		if (model.network.channel[cNr].StartLevelOnlyPrefPath == 1) {
			isAllowed = ok_connect_tss_to_channel(cNr, 0);
			if (isAllowed == 0) {
				posTss = 0;
				bastLevel = model.network.channel[cNr].bastStartLevel;
				extendTssAlongPrefPath(cNr, posTss, &bastLevel, 0);
				model.network.channel[cNr].bastStartLevel = bastLevel;
			}
			else
				model.network.channel[cNr].StartLevelOnlyPrefPath = 0;
		}

		if (model.network.channel[cNr].EndLevelOnlyPrefPath == 1) {
			isAllowed = ok_connect_tss_to_channel(cNr, 1);
			if (isAllowed == 0) {
				posTss = model.network.channel[cNr].nPoints - 1;
				bastLevel = model.network.channel[cNr].bastEndLevel;
				extendTssAlongPrefPath(cNr, posTss, &bastLevel, 1);
				model.network.channel[cNr].bastEndLevel = bastLevel;
			}
			else
				model.network.channel[cNr].EndLevelOnlyPrefPath = 0;
		}
		int legNr1, legNr2, levelNr1, levelNr2;
		levelNr1 = model.network.channel[cNr].bastStartLevel;
		if (levelNr1 >= 0)
			legNr1 = model.network.physicalLev[levelNr1].legNr;
		else
			legNr1 = model.network.channel[-levelNr1 - 1].legNr;
		levelNr2 = model.network.channel[cNr].bastEndLevel;
		if (levelNr2 >= 0)
			legNr2 = model.network.physicalLev[levelNr2].legNr;
		else
			legNr2 = model.network.channel[-levelNr2 - 1].legNr;
		if (legNr1 != legNr2) {
			model.network.channel[cNr].bastStartLevel = -1;
			model.network.channel[cNr].bastEndLevel = -1;
		}
	}
	return 0;
}


int load_tss_optiNav()
{
	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	double kvotCost, default_kvotCost = 0.01;
	double kvotMinCost;

	if (USE_KVOTCOST_CORRIDORS == 0)
		default_kvotCost = 0; //  DEFAULT_KVOTMINCOST_TSS; // cannot be used because of min cost issues

	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.params.tssName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist but given in input data as the tss to load in OptiNav. I continue without tss\n", 0);
		model.nTss = 0;
		return 0;
	}
	//printf("opens %s\n", namn);
	fil.open(namn);

	int cNr, pos2, useTss, nAllocLast = -1, keepPrefPath;
	json data, geom, coords, dataIt2, prop;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu, pos, posBase;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + std::string(namn) + " is not valid. I continue without tss.", 0);
		fil.close();
		model.nTss = 0;
		return 0;
	}
	fil.close();

	if (data["features"].is_null()) {
		postRequest("ERROR! no features in tss file. I continue without tss.", 0);
		model.nTss = 0;
		return 0;
	}
	json data2 = data["features"];

	cNr = 0; // model.network.nChannels;
	strClosePoints firstPoints, lastPoints;

	int nAllocChannelsTmp = 10;
	model.network.channelTmp = (strChannel*)malloc(nAllocChannelsTmp * sizeof(strChannel));
	nAlloc = data2.size();
	pos = 0;
	for (auto it = data2.begin(); it != data2.end(); ++it) {
		if (cNr >= nAllocChannelsTmp) {
			nAllocChannelsTmp += 10;
			model.network.channelTmp = (strChannel*)realloc(model.network.channelTmp, nAllocChannelsTmp * sizeof(strChannel));
		}

		pos++;
		json dataNu = it.value();
		CURRENT_TSS_NAME = std::to_string(pos);
		if (!(dataNu["properties"].is_null())) {
			json propNu = dataNu["properties"];
			if (!(propNu["id_name"].is_null()))
				CURRENT_TSS_NAME = propNu["id_name"];
		}
		if (dataNu["geometry"].is_null()) {
			errlog("ERROR! tss %d do not have a geometry. I skip this one\n", pos);
			continue;
		}
		geom = dataNu["geometry"];
		if (geom["coordinates"].is_null()) {
			errlog("ERROR! tss %d has geometry but no coordinates. I skip this one\n", pos);
			continue;
		}
		coords = geom["coordinates"];

		check_realloc_coords(coords.size());

		initBoundingBox(&(model.network.boundingbox));

		pos2 = 0;
		if (pos == 38)
			pos = pos;
		for (auto it2 = coords.begin(); it2 != coords.end(); ++it2) {
			dataIt2 = it2.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0) {
					model.network.xCoord[pos2] = it3.value();
				}
				else {
					model.network.yCoord[pos2] = it3.value();
				}
				i2++;
			}
			updateBoundingBoxWithCoord(&(model.network.boundingbox), model.network.yCoord[pos2], model.network.xCoord[pos2]);
			pos2++;
		}
		model.network.nCoords = pos2;
		if (pos == 69)
			pos = pos;
		useTss = checkTssRightArea(&firstPoints, &lastPoints);
		if (useTss != 0)
			pos = pos;
		if (useTss == 1) {
			keepPrefPath = 0;
			if (!(dataNu["properties"].is_null())) {
				prop = dataNu["properties"];
				if (!(prop["kvotCost"].is_null()))
					kvotCost = prop["kvotCost"];
				else
					kvotCost = default_kvotCost;
				if (!(prop["keepPrefPath"].is_null()))
					keepPrefPath = prop["keepPrefPath"];
			}
			if (USE_KVOTCOST_CORRIDORS == 0) {
				kvotMinCost = kvotCost; //  0.0; // kvotCost;
				kvotCost = 0.0;
			}else
				kvotMinCost = 0.0;

			addSplitTss(&cNr, kvotCost, kvotMinCost, firstPoints, lastPoints, keepPrefPath);
			// printf("cNr %d origPos %d\n\n", cNr - 1, pos);
			// cNr++;
		}
	}
	free(namn);




	// set other channel data

	sort_tssChannels(cNr);

	check_tss_startSlutLevelOnlyPrefPath();

	// model.network.nChannels = cNr;


	return 0;
}




int checkCoordInBoundingBox(double y, double x, strBoundBox bbox) {
	if (y >= bbox.yMin && y <= bbox.yMax && x >= bbox.xMin && x <= bbox.xMax)
		return 1;
	else
		return 0;
}

void updateBoundingBoxWithCoord(strBoundBox* bbox, double y, double x) {
	double dist;
	if (y < bbox->yMin)
		bbox->yMin = y;
	if (bbox->xMin < 9998) {
		dist = abs(bbox->xMin - x);
		if (dist > 180) {
			if (bbox->xMin - x < -180)
				x -= 360;
			else
				x += 360;
		}
	}

	if (x < bbox->xMin)
		bbox->xMin = x;
	if (y > bbox->yMax)
		bbox->yMax = y;
	if (x > bbox->xMax)
		bbox->xMax = x;
}

void initBoundingBox(strBoundBox* bbox) {
	bbox->yMin = 9999;
	bbox->xMin = 9999;
	bbox->yMax = -9999;
	bbox->xMax = -9999;
}


/*
int setUpUsablePointsInPolygonChannel(int cNr, int pos) {
	int i2, firstAllowed, nAlloc, startPos;
	double dist = 0, x1Bas, y1Bas, distSinceLastTot;
	int nFPoints = 0, nPkterNu, i1, forsta, isFeasible;
	double distSinceLast, distUse, punktDist, x1, y1, x2, y2, bearing, x2Bas, y2Bas;
	spherical::Point p1, p2, p3;

	if (model.network.channel[cNr].nPolygonPoints > 0) {
		model.network.channel[cNr].polygon_boundingBox[pos].xMax = -9999;
		model.network.channel[cNr].polygon_boundingBox[pos].yMax = -9999;
		model.network.channel[cNr].polygon_boundingBox[pos].xMin = 9999;
		model.network.channel[cNr].polygon_boundingBox[pos].yMin = 9999;

		nFPoints = 0;
		punktDist = model.params.shipSpeed_average * 1000 / model.params.ortoDist_nPointsPerHour; // / 2;

		nAlloc = 2 * model.network.channel[cNr].nPolygonPoints[pos] + model.network.channel[cNr].polygon_dist[pos] * 1000 / punktDist;
		model.network.channel[cNr].polygonUse_point[pos] = (spherical::Point*)malloc2(nAlloc * sizeof(spherical::Point));
		model.network.channel[cNr].polygonUse_x[pos] = (double*)malloc2(nAlloc * sizeof(double));
		model.network.channel[cNr].polygonUse_y[pos] = (double*)malloc2(nAlloc * sizeof(double));
		//printf("nAlloc %d\n", nAlloc);

		startPos = 0;
		for (i2 = 0; i2 < model.network.channel[cNr].nPolygonPoints[pos]; i2++) {
			y1 = model.network.channel[cNr].polygon_y[pos][i2];
			x1 = model.network.channel[cNr].polygon_x[pos][i2];
			if (check_nodeIsWithinPhysicalMapRaster(y1, x1) == 0) {
				errlog("ERROR! cNr %d pos %d coord %d %.3lf %.3lf not in physical map. I skip this one\n", cNr, pos,
					i2, x1, y1);
				continue;
			}
			p1 = spherical::Point(y1, x1);
			break;
		}
		startPos = i2 + 1;

		y1Bas = y1;
		x1Bas = x1;
		firstAllowed = 0;
		forsta = 1;
		distSinceLastTot = 0;
		for (i2 = startPos; i2 < model.network.channel[cNr].nPolygonPoints[pos]; i2++) {
			y2 = model.network.channel[cNr].polygon_y[pos][i2];
			x2 = model.network.channel[cNr].polygon_x[pos][i2];
			p2 = spherical::Point(y2, x2);
			bearing = p1.bearingTo(p2);
			//printf("i2 %d coords from %.3lf %.3lf to %.3lf %.3lf punktDist %.3lf bearing %.3lf\n", i2,
			//	x1, y1, x2, y2, punktDist, bearing);
			if (check_nodeIsWithinPhysicalMapRaster(y2, x2) == 1) {
				distSinceLast = p1.distanceTo(p2);// estimateLargeCircleDistance_km(lastX, lastY,
					//model.network.yCoord[nCoords], model.network.xCoord[nCoords]);
				if (distSinceLast + distSinceLastTot < 5.0 && i2 < model.network.channel[cNr].nPolygonPoints[pos] - 1) {
					distSinceLastTot += distSinceLast;
					continue;
				}
				else
					distSinceLastTot = 0;
				nPkterNu = roundUp(distSinceLast / punktDist);
				distUse = distSinceLast / nPkterNu;
				//printf("i2 %d coords from %.3lf %.3lf to %.3lf %.3lf nPkterNu %d distUse %.3lf distSinceLast %.3lf bearing %.3lf\n", i2,
				//	x1, y1, x2, y2, nPkterNu, distUse, distSinceLast, bearing);
				for (i1 = 1; i1 <= nPkterNu; i1++) {
					if (forsta == 1) {
						isFeasible = makeSure_feasibleCoordFranLinje(&y1, &x1, &y2, &x2, 0);
						//printf("forsta isFeasible %d xy %.3lf %.3lf\n", isFeasible, x1, y1);
						if (isFeasible == 1) {
							model.network.channel[cNr].polygonUse_x[pos][nFPoints] = x1;
							model.network.channel[cNr].polygonUse_y[pos][nFPoints] = y1;
							model.network.channel[cNr].polygonUse_point[pos][nFPoints] = spherical::Point(y1, x1);
							updateBoundingBoxCorridorPolygon(&(model.network.channel[cNr].polygon_boundingBox[pos]), y1, x1);
							//printf("forsta isFeasible %d xy %.3lf %.3lf\n", isFeasible, 
							//	model.network.channel[cNr].polygonUse_x[pos][nFPoints], model.network.channel[cNr].polygonUse_y[pos][nFPoints]);
							nFPoints++;
							forsta = 0;
							if (i2 == 1 && i1 == 1) {
								if (abs((x1 - x1Bas) * (x1 - x1Bas) + (y1 - y1Bas) * (y1 - y1Bas)) < 0.0001)
									firstAllowed = 1;
							}
						}
						x1 = x1Bas;
						y1 = y1Bas;
					}

					if (i2 == model.network.channel[cNr].nPolygonPoints[pos] - 1 && i1 == nPkterNu && firstAllowed == 1)
						continue; // do not include the last point as the first point is allowed and the same
					if (i1 < nPkterNu) {
						p3 = p1.destinationPoint(distUse * i1, bearing);
						y2Bas = p3.latitude().degrees();
						x2Bas = p3.longitude().degrees();
					}
					else {
						y2Bas = model.network.channel[cNr].polygon_y[pos][i2];
						x2Bas = model.network.channel[cNr].polygon_x[pos][i2];
					}
					x2 = x2Bas;
					y2 = y2Bas;
					isFeasible = makeSure_feasibleCoordFranLinje(&y1, &x1, &y2, &x2, 1);
					//printf("i1 %d isFeasible %d xy %.3lf %.3lf\n", i1, isFeasible, x2, y2);
					if (isFeasible == 1) {
						model.network.channel[cNr].polygonUse_x[pos][nFPoints] = x2;
						model.network.channel[cNr].polygonUse_y[pos][nFPoints] = y2;
						model.network.channel[cNr].polygonUse_point[pos][nFPoints] = spherical::Point(y2, x2);
						updateBoundingBoxCorridorPolygon(&(model.network.channel[cNr].polygon_boundingBox[pos]), y2, x2);
						nFPoints++;
						forsta = 0;
					}
					x1 = x2Bas;
					y1 = y2Bas;

				}
			}
			else
				errlog("ERROR! cNr %d pos %d coord %d %.3lf %.3lf not in physical map. I skip this one\n", cNr, pos, i2,
					x2, y2);
			p1 = p2;
			x1 = x2Bas;
			y1 = y2Bas;
			x1Bas = x1;
			y1Bas = y1;
		}
	}
	//printf("saved points %d\n", nFPoints);
	for (i2 = 0; i2 < nFPoints; i2++)
		errlog("cNr %d, pos %d i2 %d xy %.3lf %.3lf\n", cNr, pos, i2, model.network.channel[cNr].polygonUse_x[pos][i2], model.network.channel[cNr].polygonUse_y[pos][i2]);

	//printf("bounding box for corridor %d pos %d %.3lf %.3lf %.3lf %.3lf\n", cNr, pos,
	//	model.network.channel[cNr].polygon_boundingBox[pos].xMin,
	//	model.network.channel[cNr].polygon_boundingBox[pos].yMin,
	//	model.network.channel[cNr].polygon_boundingBox[pos].xMax,
	//	model.network.channel[cNr].polygon_boundingBox[pos].yMax);


	return nFPoints;

}
*/

/*
int loadPolygonToChannel(json dataCoord, int cNr, int pos) {
	json dataIt, dataIt2;
	int nCoords = 0, i2, nAlloc;
	double dist = 0;
	errlog("ERROR! OBS! Change channel xCoords to same as prefered path in loadPolygonToChannel\n");

	// read the coordinates
	nAlloc = dataCoord.size();
	model.network.channel[cNr].polygon_x[pos] = (double*)malloc2(nAlloc * sizeof(double));
	model.network.channel[cNr].polygon_y[pos] = (double*)malloc2(nAlloc * sizeof(double));
	for (auto it = dataCoord.begin(); it != dataCoord.end(); ++it) {
		dataIt = it.value();
		i2 = 0;
		for (auto it2 = dataIt.begin(); it2 != dataIt.end(); ++it2) {
			if (i2 == 0) {
				model.network.channel[cNr].polygon_x[pos][nCoords] = it2.value();
			}
			else if (i2 == 1)
				model.network.channel[cNr].polygon_y[pos][nCoords] = it2.value();
			i2++;
		}
		//printf("%d coords %.3lf %.3lf\n", nCoords, model.network.channel[cNr].polygon_x[pos][nCoords], model.network.channel[cNr].polygon_y[pos][nCoords]);
		if(nCoords > 0)
			dist += estimateLargeCircleDistance_km(model.network.channel[cNr].polygon_y[pos][nCoords - 1], model.network.channel[cNr].polygon_x[pos][nCoords - 1],
				model.network.channel[cNr].polygon_y[pos][nCoords], model.network.channel[cNr].polygon_x[pos][nCoords]);
		//printf("nCoords %d distNu %.3lf\n", nCoords, dist);
		nCoords++;
	}
	model.network.channel[cNr].polygon_dist[pos] = dist;
	printf("polygon for corridor cNr %d pos %d nCoords %d polygon dist %.3lf\n", cNr, pos, nCoords, dist);

	return nCoords;
}
*/



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
	int vardeInt, pos;

	std::ifstream fil;
	std::string namnString;
	char* namn;

	params->nTidsperioder_perH = params->nTidsperioder_perH_iter1;
	params->tIndexGerH = 1;
	params->knots_to_km = 1.852;
	params->maxDistBetweenPrefPathPoints = 250 * params->knots_to_km;
	params->shipSpeed_average = 22;
	//params->mapPhysicalFileName = std::string();
	params->mapPhysicalAFileName = std::string();
	params->mapPhysicalBFileName = std::string();
	//params->physicalMapRasterPos = 0;
	//params->mapFuelGeographyFileName = std::string();
	//params->mapFuelGeographyAFileName = std::string();
	//params->mapFuelGeographyBFileName = std::string();

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
	params->max_changeDirection_factorStartEnd = 3;
	params->varyStartEndArcLength = 1;
	//params->lengthIntervall = 1;
	//params->dist_checkOKroute = 1;
	//params->variableFileName = "variablesInfo.json";
	model.params.useStandardWeather = 0;
	//params->physicalMap_noDataValue = 9999;
	//params->speedSettings_addOnlyCheapestArcs = 1;
	params->runAlt = 0;
	params->startDelay_h = 0;

	params->nMaxLev_posToDelayedPrefPath = 3;
	errlog("ERROR! Hard coded nMaxLev_posToDelayedPrefPath %d, ie number of physical level before route must be on pref path with historical data\n",
		params->nMaxLev_posToDelayedPrefPath);
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
	params->startMinute = 0;
	params->maxDeviationPreferred_km = 500;

	params->historicDataFactor_current = 0.8;
	params->historicDataFactor_windSpeed = 0.5;
	params->historicDataFactor_waveHeight = 0.5;

	params->minSpeedDiffWeatherFactor = -10;
	params->maxSpeedDiffWeatherFactor = 8;
	params->minSpeedDiffCurrent = -10;
	params->maxSpeedDiffCurrent = 10;
	errlog("OBS! minSpeedDiffWeatherFactor set to %.3lf\n", params->minSpeedDiffWeatherFactor);
	errlog("OBS! maxSpeedDiffWeatherFactor set to %.3lf\n", params->maxSpeedDiffWeatherFactor);
	errlog("OBS! minSpeedDiffCurrent set to %.3lf\n", params->minSpeedDiffCurrent);
	errlog("OBS! maxSpeedDiffCurrent set to %.3lf\n", params->maxSpeedDiffCurrent);

	params->penOverWeatherLimit_fix = 1e12;
	params->penOverMaxWaveHeightLimit_m = 1e11;
	params->penOverMaxWindSpeedLimit_kmh = 1e10;

	params->longestRouteDays_history = -1;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/file_params.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		exit(0);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);
	
	json data, dataSpeed, dataVar, dataIt;
	try{
		fil >> data;
	}
	catch(...){
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}

	if (!data["SKRIV_UT_NOTHING"].is_null()) {
		SKRIV_UT_NOTHING = data["SKRIV_UT_NOTHING"];
		//if (SKRIV_UT_NOTHING < 2) {
		//	reset_errlog();
		//}
	}
	if (!data["MAX_FAKTOR_NATVERK"].is_null()) {
		MAX_FAKTOR_NATVERK = data["MAX_FAKTOR_NATVERK"];
	}

	if (!data["url_errorEmail_api"].is_null()) {
		params->url_errorEmail_api = data["url_errorEmail_api"];
	}
	else
		params->url_errorEmail_api = "";

	if (!data["url_getCorridors"].is_null()) {
		params->url_getCorridors = data["url_getCorridors"];
	}
	else
		params->url_getCorridors = "";

	if (!data["includeNazanin_safety"].is_null())
		params->includeNazanin_safety = data["includeNazanin_safety"];
	else
		params->includeNazanin_safety = 0;


	if (!data["ignore_windWave"].is_null())
		params->ignore_windWave = data["ignore_windWave"];
	else
		params->ignore_windWave = 0;
	if (!data["ignore_current"].is_null())
		params->ignore_current = data["ignore_current"];
	else
		params->ignore_current = 0;

	if (!data["delayVersion"].is_null()) {
		// loadSet_startDateTime(data["startDateTime", params]);
		delayVersion = data["delayVersion"];
		errlog("ERROR? OBS! Setting delayVersion to %d in file_params.json\n", delayVersion);
	}

	if (!data["startDateTime"].is_null()) {
		// loadSet_startDateTime(data["startDateTime", params]);
		params->UTC_secondsStart = make_gmtime_fromDateTimeString(data["startDateTime"], params);
	}

	if (!data["eta_penalty_early"].is_null())
		params->eta_cost_early = data["eta_penalty_early"];
	else
		params->eta_cost_early = 1000;
	if (!data["eta_penalty_late"].is_null())
		params->eta_cost_late = data["eta_penalty_late"];
	else
		params->eta_cost_late = 1000;

	if (!data["penOverWeatherLimit_fix"].is_null())
		model.simulering.penOverWeatherLimit_fix = data["penOverWeatherLimit_fix"];
	else
		model.simulering.penOverWeatherLimit_fix = 1e10;
	if (!data["penOverMaxWaveHeight_m"].is_null())
		model.simulering.penOverMaxWaveHeight_m = data["penOverMaxWaveHeight_m"];
	else
		model.simulering.penOverMaxWaveHeight_m = 1e7;
	if (!data["penOverMaxWindSpeed_kmh"].is_null())
		model.simulering.penOverMaxWindSpeed_kmh = data["penOverMaxWindSpeed_kmh"];
	else
		model.simulering.penOverMaxWindSpeed_kmh = 1e6;
	if (!data["penDeviateSpeed_kmh"].is_null())
		model.simulering.penDeviateSpeed_kmh = data["penDeviateSpeed_kmh"];
	else
		model.simulering.penDeviateSpeed_kmh = 1e4;
	if (!data["penDeviatePrefPath_nodes"].is_null())
		model.simulering.penDeviatePrefPath_nodes = data["penDeviatePrefPath_nodes"];
	else
		model.simulering.penDeviatePrefPath_nodes = 2e4;


	if (!data["knots_to_km"].is_null())
		params->knots_to_km = data["knots_to_km"];

	if (!data["mapTimeDelay"].is_null()) {
		params->mapTimeDelayName = data["mapTimeDelay"];
	}
	else {
		errlog("ERROR! No time delay maps given in file_params.json. It must exist. I quit.\n");
		postRequest("ERROR! No time delay maps given in file_params.json. It must exist. I quit.", 1);
	}
	if (delayVersion == 4) {
		if (!data["map_currentDelayName0"].is_null()) {
			params->map_currentDelayName[0] = data["map_currentDelayName0"];
		}
		else {
			errlog("ERROR! No map_currentDelayName0 for delay given in file_params.json. It must exist. I quit.\n");
			postRequest("ERROR! No map_currentDelayName0 for delay given in file_params.json. It must exist. I quit.", 1);
		}
		if (!data["map_currentDelayName1"].is_null()) {
			params->map_currentDelayName[1] = data["map_currentDelayName1"];
		}
		else {
			errlog("ERROR! No map_currentDelayName1 for delay given in file_params.json. It must exist. I quit.\n");
			postRequest("ERROR! No map_currentDelayName1 for delay given in file_params.json. It must exist. I quit.", 1);
		}
	}
	if (!data["delayNotPreferredPathArcFactor"].is_null()) {
		params->delayEjPrefPathArcFactor = data["delayNotPreferredPathArcFactor"];
	}
	else {
		params->delayEjPrefPathArcFactor = 1;
	}


	//if (!data["speedSettings_addOnlyCheapestArcs"].is_null()) {
	//	errlog("ERROR! Not using speedSettings_addOnlyCheapestArcs\n");
	//	// params->speedSettings_addOnlyCheapestArcs = data["speedSettings_addOnlyCheapestArcs"];
	//}
	if (!data["readSolPathFile"].is_null()) {
		params->readSolPathFile = data["readSolPathFile"];
	}
	if (!data["historicDataFactor_current"].is_null())
		params->historicDataFactor_current = data["historicDataFactor_current"];
	if (!data["historicDataFactor_windSpeed"].is_null())
		params->historicDataFactor_windSpeed = data["historicDataFactor_windSpeed"];
	if (!data["historicDataFactor_waveHeight"].is_null())
		params->historicDataFactor_waveHeight = data["historicDataFactor_waveHeight"];
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

	if (!data["report_minBearingDiff"].is_null())
		params->report_minBearingDiff = data["report_minBearingDiff"];
	else
		params->report_minBearingDiffWpt = 8;
	if (!data["report_minBearingDiffWpt"].is_null())
		params->report_minBearingDiffWpt = data["report_minBearingDiffWpt"];
	else
		params->report_minBearingDiffWpt = 5;

	if (!data["nHours_changeCourseInterval"].is_null())
		params->nHours_changeCourseInterval = data["nHours_changeCourseInterval"];
	if (!data["maxDiffTimeFastSlow_h"].is_null())
		params->maxDiffTimeFastSlow = data["maxDiffTimeFastSlow_h"];
	if (!data["maxDiffTimeFastSlow_fas3"].is_null())
		params->maxDiffTimeFastSlow_fas3 = data["maxDiffTimeFastSlow_fas3"];
	else
		params->maxDiffTimeFastSlow_fas3 = 8;
	if (!data["nPkterOrto"].is_null()) {
		params->nPkterOrto = data["nPkterOrto"];
		vardeInt = (int)(params->nPkterOrto / 2.0);
		if (vardeInt * 2 == params->nPkterOrto)
			(params->nPkterOrto)++; // must be an uneven number of orthogonal points
	}
	if (!data["maxDeviationPreferred_km"].is_null()) {
		params->maxDeviationPreferred_km = data["maxDeviationPreferred_km"];
	}
	if (!data["nMinutesBetweenPkterOrto"].is_null()) {
		if (data["nMinutesBetweenPkterOrto"] < 1)
			params->ortoDist_nPointsPerHour = 60.0;
		else
			params->ortoDist_nPointsPerHour = 60.0 / (double)(data["nMinutesBetweenPkterOrto"]);
	}

	if (!data["max_changeDirection"].is_null())
		params->max_changeDirection = data["max_changeDirection"];
	if (!data["max_changeDirection_factorStartEnd"].is_null())
		params->max_changeDirection_factorStartEnd = data["max_changeDirection_factorStartEnd"];
	if (!data["varyStartEndArcLength"].is_null())
		params->varyStartEndArcLength = data["varyStartEndArcLength"];
	if (!data["maxDiff_pointNrFas3"].is_null())
		params->maxDiff_pointNrFas3 = data["maxDiff_pointNrFas3"];
	else
		params->maxDiff_pointNrFas3 = 2;
	if (!data["longestRouteDays_history"].is_null())
		params->longestRouteDays_history = data["longestRouteDays_history"];



	if (!data["solutionFileName"].is_null())
		params->solutionFileName = data["solutionFileName"];


	errlog("historicDataFactor_current %.3lf\nhistoricDataFactor_windSpeed %.3lf\nhistoricDataFactor_waveHeight %.3lf\n",
		params->historicDataFactor_current, params->historicDataFactor_windSpeed, params->historicDataFactor_waveHeight);

	if (!data["storms"].is_null()) {
		json dataStorms = data["storms"];
		json dataIt;
		int nAllocStorms;
		std::string namn;

		model.nStorms = 0;
		model.nameTmp = (char*)malloc(256 * sizeof(char));
		nAllocStorms = (int)dataStorms.size();
		model.storms = (strStorm*)malloc2(nAllocStorms * sizeof(strStorm));
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

	//errlog("objective weights:\n\ttime: %.2lf\n\tfuel: %.2lf\n\tsafety: %.2lf\n",
	//	params->weightTime, 1.0, params->weightSafety.base);
	return 0;
}

int loadFileParams_feasibilityOptiNav(strParams* params)
{
	int i, closestI, pos2, minLon, maxLon;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux, minLat;


	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	sprintf(namn, "%s/file_paramsFeasibility.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist.I quit\n", 1);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);

	json data, dataGeo, dataGeo2, dataFeature, dataProp, dataGeo3, dataCoord;
	json dataIt, dataIt2, dataIt3;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu, pos, posBase, i1;
	double xVal, yVal;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav-autoRoute again.", 1);
	}

	if (!data["mapPhysicalBFileName"].is_null()) {
		params->mapPhysicalBFileName = data["mapPhysicalBFileName"];
	}
	if (!data["mapPhysicalAFileName"].is_null()) {
		params->mapPhysicalAFileName = data["mapPhysicalAFileName"];
	}

	if (!data["mapLandSeaBFileName"].is_null()) {
		params->mapLandSeaBFileName = data["mapLandSeaBFileName"];
	}
	if (!data["mapLandSeaAFileName"].is_null()) {
		params->mapLandSeaAFileName = data["mapLandSeaAFileName"];
	}

	params->tssName = std::string();
	if (!data["tss"].is_null()) {
		params->tssName = data["tss"];
	}
	else {
		postRequest("ERROR! No field tss in file_paramsFeasibility.json. I use no TSS.", 0);
		params->tssName = "-";
	}

	if (!data["tss_attractionDistance_nm"].is_null()) {
		params->tss_attractionDistance_km = data["tss_attractionDistance_nm"];
		params->tss_attractionDistance_km *= 1.852;
		if (params->tss_attractionDistance_km < 0.001)
			params->tss_attractionDistance_km = 0.001;
	}
	else
		params->tss_attractionDistance_km = DEFAULT_TSS_ATTACTIONDISTANCE * 1.852;


	//if (!data["physicalMap_noDataValue"].is_null())
	//	params->physicalMap_noDataValue = data["physicalMap_noDataValue"];

	//if (!data["mapFuelGeographyAFileName"].is_null()) {
	//	params->mapFuelGeographyAFileName = data["mapFuelGeographyAFileName"];
	//}
	//if (!data["mapFuelGeographyBFileName"].is_null()) {
	//	params->mapFuelGeographyBFileName = data["mapFuelGeographyBFileName"];
	//}

	std::string namnString;
	if (!data["optionalExtraAreas"].is_null()) {
		json dataExtra = data["optionalExtraAreas"];
		model.nExtraNoGoAreasBase = dataExtra.size();
		model.extraNoGoAreaBase = (strExtraNoGoBase*)malloc(model.nExtraNoGoAreasBase * sizeof(strExtraNoGoBase));
		pos = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			namnString = dataNu["areaID"];
			model.extraNoGoAreaBase[pos].areaID = str_alloc_cpy(namnString.c_str());
			namnString = dataNu["fileNameA"];
			model.extraNoGoAreaBase[pos].fileNameA = str_alloc_cpy(namnString.c_str());
			namnString = dataNu["fileNameB"];
			model.extraNoGoAreaBase[pos].fileNameB = str_alloc_cpy(namnString.c_str());
			if (!dataNu["extraCostFactor"].is_null())
				model.extraNoGoAreaBase[pos].extraCostFactor = dataNu["extraCostFactor"];
			else {
				model.extraNoGoAreaBase[pos].extraCostFactor = 2.0;
				errlog("OBS! No extraCostFactor given for %s, I set it to %.2lf\n", namnString.c_str(), model.extraNoGoAreaBase[pos].extraCostFactor);
			}

			pos++;
		}
	}

	fil.close();

	return 0;
}

std::string cleanString(std::string namn) {
	std::string resultat;
	std::string subNamn = ",";
	size_t pos = std::string::npos;

	resultat = namn;
	// Search for the substring in string in a loop untill nothing is found
	while ((pos = resultat.find(subNamn)) != std::string::npos)
	{
		// If found then erase it from string
		resultat.erase(pos, subNamn.length());
	}

	for (int i = 0; i < resultat.length(); i++) {
		if (resultat[i] == '-')
			resultat[i] = '_';
	}

	return resultat;
}


int loadStormObject(json dataFeature) {

	if (dataFeature["properties"].is_null()) {
		printf("ERROR! No properties for a feature in storms. I skip this one\n");
		errlog("ERROR! No properties for a feature in storms. I skip this one\n");
		return -1; // no properties exists for this one, cannot be a preferred path
	}
	json dataProp = dataFeature["properties"];
	int stormNr;
	std::string stormID;//  = dataProp["stormID"];
	if (!dataProp["stormID"].is_null()) {
		if (dataProp["stormID"].type() == json::value_t::string) {
			stormID = dataProp["stormID"];
			sprintf(model.nameTmp, "%s", stormID.c_str());
		}
		else {
			stormNr = dataProp["stormID"];
			sprintf(model.nameTmp, "%d", stormNr);
		}
		//printf("stormNr %d\n", stormNr);
	}
	else {
		errlog("ERROR! A storm feature is given without the property stormID. I skip this one\n");
		return -1;
	}
	int i;
	for (i = 0; i < model.nStorms; i++) {
		if (strcmp(model.nameTmp, model.storms[i].stormID) == 0)
			break;
	}
	if (i >= model.nStorms) {
		if (i >= model.nAllocStorms) {
			model.nAllocStorms += 50;
			model.storms = (strStorm*)realloc(model.storms , model.nAllocStorms * sizeof(strStorm));
		}
		// new stormID
		model.storms[i].stormID = str_alloc_cpy(model.nameTmp);
		model.storms[i].nAllocFeatures = 15;
		model.storms[i].feature = (strStormFeature*)malloc2(model.storms[i].nAllocFeatures * sizeof(strStormFeature));
		//errlog("storm %d alloc %d features\n", i, model.storms[i].nAllocFeatures);
		model.storms[i].nFeatures = 0;
		(model.nStorms)++;
		model.storms[i].box_minLat = 360;
		model.storms[i].box_maxLat = -360;
		model.storms[i].box_minLon = 360;
		model.storms[i].box_maxLon = -360;
		model.storms[i].closestPointToRoute = 1e10;
		if (!dataProp["STORMNAME"].is_null()) {
			std::string stormName = dataProp["STORMNAME"];
			model.storms[i].stormName = str_alloc_cpy(stormName.c_str());
		}
		else {
			model.storms[i].stormName = str_alloc_cpy("unknown");
			errlog("ERROR! StormID %d is missing field STORMNAME. I set it to unknown\n", stormNr);
		}
	}
	if (dataFeature["geometry"].is_null()) {
		printf("ERROR! No geometry for a feature in storms. I skip this one\n");
		errlog("ERROR! No geometry for a feature in storms. I skip this one\n");
		return -1; // no properties exists for this one, cannot be a preferred path
	}
	json dataGeom = dataFeature["geometry"];
	if (dataGeom["coordinates"].is_null()) {
		printf("ERROR! No coordinates for a geometry in storms. I skip this one\n");
		errlog("ERROR! No coordinates for a geometry in storms. I skip this one\n");
		return -1; // no properties exists for this one, cannot be a preferred path
	}
	if (dataGeom["type"].is_null()) {
		printf("ERROR! No type for a geometry in storms. I skip this one\n");
		errlog("ERROR! No type for a geometry in storms. I skip this one\n");
		return -1; // no properties exists for this one, cannot be a preferred path
	}
	if (dataGeom["type"] != "Point") {
		std::string namn = dataGeom["type"];
		errlog("ERROR! Geomestry type has to be Point for storms but it is %s. I skip this one\n", namn.c_str());
		return -1; // no properties exists for this one, cannot be a preferred path

	}
	if (model.storms[i].nFeatures >= model.storms[i].nAllocFeatures) {
		model.storms[i].nAllocFeatures += 15;
		model.storms[i].feature = (strStormFeature*)realloc(model.storms[i].feature,
			model.storms[i].nAllocFeatures * sizeof(strStormFeature));
		//errlog("storm %d realloc %d features\n", i, model.storms[i].nAllocFeatures);
	}

	if (dataProp["WindFrontRadius"].is_null()) {
		errlog("ERROR! No WindFrontRadius given for a feature in storms. I set it to 0\n");
		model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize = 0;
	}
	else
		model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize = dataProp["WindFrontRadius"];
	if (dataProp["WindBackRadius"].is_null()) {
		errlog("ERROR! No WindBackRadius given for a feature in storms. I set it to 0\n");
		model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize = 0;
	}
	else
		model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize = dataProp["WindBackRadius"];

	double maxWind;
	if (dataProp["WindMaxRadius"].is_null()) {
		if (model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize >
			model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize)
			maxWind = model.storms[i].feature[model.storms[i].nFeatures].innerCircleForwardSize;
		else
			maxWind = model.storms[i].feature[model.storms[i].nFeatures].innerCircleBackwardsSize;
		errlog("ERROR! No WindMaxRadius given for a feature in storms. I set it to %.2lf (max of front and back wind radius)\n",
			maxWind);
		model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize = maxWind;
	}
	else
		model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize = dataProp["WindMaxRadius"];

	if (dataProp["MAXWIND"].is_null()) {
		model.storms[i].feature[model.storms[i].nFeatures].maxWind = -1;
	}
	else
		model.storms[i].feature[model.storms[i].nFeatures].maxWind = dataProp["MAXWIND"];

	if (dataGeom["coordinates"].is_null()) {
		printf("ERROR! No coordinates in a geometry in storms. I skip this one\n");
		errlog("ERROR! No coordinates in a geometry in storms. I skip this one\n");
		return -1;
	}
	json dataIt2, dataIt3, dataCoord = dataGeom["coordinates"];
	int pos = 0;
	for (auto it2 = dataCoord.begin(); it2 != dataCoord.end(); ++it2) {
		dataIt2 = it2.value();
		for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
			dataIt3 = it3.value();
			// get midpoint
			if (pos == 0) {
				model.storms[i].feature[model.storms[i].nFeatures].lon = dataIt3;
				if (model.storms[i].feature[model.storms[i].nFeatures].lon > 180)
					model.storms[i].feature[model.storms[i].nFeatures].lon -= 360;
			}
			else
				model.storms[i].feature[model.storms[i].nFeatures].lat = dataIt3;
			pos++;
		}
	}
	double worstDegree = model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize / 120; // estimate
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

	if (model.storms[i].feature[model.storms[i].nFeatures].lon > 180)
		model.storms[i].feature[model.storms[i].nFeatures].lon -= 180;
	model.storms[i].feature[model.storms[i].nFeatures].midPoint = spherical::Point(model.storms[i].feature[model.storms[i].nFeatures].lat,
		model.storms[i].feature[model.storms[i].nFeatures].lon);


	if (dataProp["FLDATELBL"].is_null()) {
		errlog("ERROR! No FLDATELBL (dateTime) given for a feature in storms. I skip this one\n");
		return -1;
	}
	std::string tidpkt = dataProp["FLDATELBL"];
	model.storms[i].feature[model.storms[i].nFeatures].UTCseconds = make_gmtime_fromStormDateTime(tidpkt);
	//errlog("storm %d xy %.3lf %.3lf %s t_h %.2lf outerCircleSize %.2lf\n", i, model.storms[i].feature[model.storms[i].nFeatures].lon,
	//	model.storms[i].feature[model.storms[i].nFeatures].lat, tidpkt.c_str(), 
	//	(model.storms[i].feature[model.storms[i].nFeatures].UTCseconds - model.params.UTC_secondsStart) / 3600.0,
	//	model.storms[i].feature[model.storms[i].nFeatures].outerCircleSize);
	(model.storms[i].nFeatures)++;



	return 0;
}

int findAreaIDpos_inBaseOld(std::string ID) {
	int i;
	for (i = 0; i < model.nExtraNoGoAreasBase; i++) {
		if (strcmp(ID.c_str(), model.extraNoGoAreaBase[i].areaID) == 0)
			break;
	}
	if (i < model.nExtraNoGoAreasBase)
		return i;
	else
		return -1;
}

int findAreaIDpos_inBase(char* ID) {
	int i;

	//if (strcmp(ID, "custom") == 0)
	//	return -2; // custom polygon area
	for (i = 0; i < model.nExtraNoGoAreasBase; i++) {
		if (strcmp(ID, model.extraNoGoAreaBase[i].areaID) == 0)
			break;
	}
	if (i < model.nExtraNoGoAreasBase)
		return i;
	else
		return -1;
}

int loadWeights_leg(strParams* params, int legNr, json dataObj) {
	json dataIt, dataIt2;

	if (!dataObj["path_pos"].is_null()) {
		if (params->legProperties[legNr].path_pos != dataObj["path_pos"])
			postRequest("ERROR! path_pos " + std::string(dataObj["path_pos"]) + 
				" in objective is different for leg " + std::to_string(legNr + 1) + 
				" than in geoData " + std::to_string(params->legProperties[legNr].path_pos), 1);
	}
	else {
		if (params->legProperties[legNr].path_pos != -1) {
			postRequest("ERROR! path_pos in objective is not given for leg " + std::to_string(legNr + 1) + 
				" but is " + std::to_string(params->legProperties[legNr].path_pos) + " in geoData", 1);
		}
	}

	if (!dataObj["weight_time"].is_null()) {
		dataIt = dataObj["weight_time"];
		params->legWeights[legNr].weightTime = (double)(dataIt["weight"]); // / 100;
	}
	if (!dataObj["weightID"].is_null()) {
		params->weightID = (int)(dataObj["weightID"]); // / 100;
	}
	else
		params->weightID = 0;
	if (!dataObj["weight_fuel"].is_null()) {
		dataIt = dataObj["weight_fuel"];
		params->legWeights[legNr].weightFuel = (double)(dataIt["weight"]); // / 100;
	}
	if (!dataObj["weight_emission"].is_null()) {
		dataIt = dataObj["weight_emission"];
		params->legWeights[legNr].weightEmission = (double)(dataIt["weight"]); // / 100;
	}
	if (!dataObj["weight_safety"].is_null()) {
		json data3 = dataObj["weight_safety"];
		if (!data3["base"].is_null()) {
			dataIt2 = data3["base"];
			params->legWeights[legNr].weightSafety.base = (double)(dataIt2["weight"]); // / 100;
		}
		if (!data3["hurricane"].is_null()) {
			dataIt2 = data3["hurricane"];
			params->legWeights[legNr].weightSafety.hurricane = (double)(dataIt2["weight"]); // / 100;
		}

		if (!data3["bowSlamming"].is_null()) {
			dataIt2 = data3["bowSlamming"];
			params->legWeights[legNr].weightSafety.bowSlam = dataIt2["weight"];
		}
		if (!data3["greenWater"].is_null()) {
			dataIt2 = data3["greenWater"];
			params->legWeights[legNr].weightSafety.greenWater = dataIt2["weight"];
		}
		if (!data3["dynamicStability"].is_null()) {
			dataIt2 = data3["dynamicStability"];
			//if (dataIt2["useWeight"] == 1)
				params->legWeights[legNr].weightSafety.dynamicStability = dataIt2["weight"];
		}
		if (!data3["rolling"].is_null()) {
			dataIt2 = data3["rolling"];
			params->legWeights[legNr].weightSafety.rolling = dataIt2["weight"];
		}
		if (!data3["surfRiding"].is_null()) {
			dataIt2 = data3["surfRiding"];
			params->legWeights[legNr].weightSafety.surfRiding = dataIt2["weight"];
		}
		if (!data3["feasibleSafety"].is_null()) {
			dataIt2 = data3["feasibleSafety"];
			//if (dataIt2["useWeight"] == 1)
				params->legWeights[legNr].weightSafety.feasibleSafety = dataIt2["weight"];
		}
		if (!data3["iceCoverCost_fix"].is_null()) {
			dataIt2 = data3["iceCoverCost_fix"];
			//if (dataIt2["useWeight"] == 1)
				params->legWeights[legNr].weightSafety.iceCoverCost_fix = dataIt2["weight"];
		}
		if (!data3["iceCoverCost_thickness_m"].is_null()) {
			dataIt2 = data3["iceCoverCost_thickness_m"];
			//if (dataIt2["useWeight"] == 1)
				params->legWeights[legNr].weightSafety.iceCoverCost_thickness = dataIt2["weight"];
		}
	}
	return 0;
}

int loadWeights(strParams* params, json dataObj) {
	int legNr = 0;
	json dataFeature;
	if (dataObj.is_array()) {
		for (auto it = dataObj.begin(); it != dataObj.end(); ++it) {
			dataFeature = it.value();
			loadWeights_leg(params, legNr, dataFeature);
			legNr++;
		}
	}
	else
		loadWeights_leg(params, legNr, dataObj);

	return 0;
}


void calc_stormsNearby_delay() {
	int i, i1, i2, posUse, stormOK, keepStorm;
	double timeFromStart, dist, minTid, maxTid, minDistToStorm, maxSpeed = 0, minSpeed = 9999, speed;

	posUse = 0;
	for (i = 0; i < model.nStorms; i++) {
		stormOK = eval_stormWithinBoundingBox(i);
		if (stormOK == 0) {
			free(model.storms[i].feature);
			continue;
		}

		if (i > posUse) {
			model.storms[posUse] = model.storms[i];
		}
		// errlog("ERROR! sort the storm features in time order AND only include needed ones AND possibly identify timeperiod for each\n");
		// sort the timeperiods in the storm
		sortStormFeaturesTime(posUse);

		// add bearing and distanceToNextPoint per timeperiod
		addInfoToStorms(posUse);
		posUse++;
	}
	model.nStorms = posUse;
	errlog("nStormsUse %d\n", model.nStorms);

}




void copyAddTableInfo(strTableParam paramFrom, strTableParam* paramTo) {
	paramTo->maxValue = paramFrom.maxValue;
	paramTo->minValue = paramFrom.minValue;
	paramTo->intervalSize = paramFrom.intervalSize;
	paramTo->inv_intervalSize = 1 / paramFrom.intervalSize;
	paramTo->nIndex = (int)((paramTo->maxValue + 0.0001 - paramTo->minValue) * paramTo->inv_intervalSize) + 1;
}


int loadWeatherFactorTableWave(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, calmWaterSpeedI, waveI, wavePeriodI, waveDirI, antal;
	double varde, wave, wavePeriod, waveDir, calmWaterSpeed;

	auto tid0 = std::chrono::high_resolution_clock::now();
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open wave factor table file %s\n", namn);
		postRequest("ERROR! Could not open wave factor table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[1][tableNr].shipSpeedCalmWater, &(model.functions.waveFactor.shipSpeedCalmWater));
	copyAddTableInfo(model.tables.tableTyp[1][tableNr].waveHeight, &(model.functions.waveFactor.waveHeight));
	copyAddTableInfo(model.tables.tableTyp[1][tableNr].wavePeriod, &(model.functions.waveFactor.wavePeriod));
	copyAddTableInfo(model.tables.tableTyp[1][tableNr].waveDirection, &(model.functions.waveFactor.waveDirection));
	model.params.userLimit_maxWaveHeight = model.tables.tableTyp[1][tableNr].maxWaveHeight;
	model.params.userLimit_maxWindSpeed_kmh = model.tables.tableTyp[0][tableNr].maxWindSpeed;
	model.params.maxWaveHeight_warning = model.tables.tableTyp[1][tableNr].maxWaveHeight_warning;
	nAlloc = model.functions.waveFactor.shipSpeedCalmWater.nIndex * model.functions.waveFactor.waveHeight.nIndex *
		model.functions.waveFactor.wavePeriod.nIndex * model.functions.waveFactor.waveDirection.nIndex;
	if (model.functions.waveFactor.tableValue != NULL)
		free(model.functions.waveFactor.tableValue);
	model.functions.waveFactor.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.waveFactor.tableValue[i] = -9999;
	}

	//printf("waveFactor calmWaterSpeed nIndex %d minVal %.3lf interval %.3lf maxVal %.3lf\n", 
	//	model.functions.waveFactor.shipSpeedCalmWater.nIndex, model.functions.waveFactor.shipSpeedCalmWater.minValue,
	//	model.functions.waveFactor.shipSpeedCalmWater.intervalSize, model.functions.waveFactor.shipSpeedCalmWater.maxValue);

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

		calmWaterSpeedI = get_tableIndex(calmWaterSpeed, model.functions.waveFactor.shipSpeedCalmWater, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (calmWaterSpeedI < 0) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.shipSpeedCalmWater.minValue);
			postRequest("ERROR! calmWaterSpeed " + std::to_string(calmWaterSpeed) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.waveFactor.shipSpeedCalmWater.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (calmWaterSpeedI >= model.functions.waveFactor.shipSpeedCalmWater.nIndex) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.shipSpeedCalmWater.maxValue);
			postRequest("ERROR! calmWaterSpeed " + std::to_string(calmWaterSpeed) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.waveFactor.shipSpeedCalmWater.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveI = get_tableIndex(wave, model.functions.waveFactor.waveHeight, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveI < 0) {
			errlog("ERROR! wave %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.waveHeight.minValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.waveFactor.waveHeight.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveI >= model.functions.waveFactor.waveHeight.nIndex) {
			errlog("ERROR! wave %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.waveHeight.maxValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.waveFactor.waveHeight.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveDirI = get_tableIndexDirection(waveDir, model.functions.waveFactor.waveDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveDirI < 0) {
			errlog("ERROR! waveDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.waveDirection.minValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.waveFactor.waveDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveDirI >= model.functions.waveFactor.waveDirection.nIndex) {
			errlog("ERROR! waveDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.waveDirection.maxValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.waveFactor.waveDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		wavePeriodI = get_tableIndex(wavePeriod, model.functions.waveFactor.wavePeriod, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (wavePeriodI < 0) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.wavePeriod.minValue);
			postRequest("ERROR! wavePeriod " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.waveFactor.wavePeriod.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (wavePeriodI >= model.functions.waveFactor.wavePeriod.nIndex) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[1][tableNr].fileName,
				model.functions.waveFactor.wavePeriod.maxValue);
			postRequest("ERROR! wavePeriod " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[1][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.waveFactor.wavePeriod.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}

		pos = calmWaterSpeedI + model.functions.waveFactor.shipSpeedCalmWater.nIndex *
			(waveDirI + model.functions.waveFactor.waveDirection.nIndex *
				(wavePeriodI + model.functions.waveFactor.wavePeriod.nIndex * waveI));
		if (model.functions.waveFactor.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for wave factor table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.waveFactor.tableValue[pos], varde);
		model.functions.waveFactor.tableValue[pos] = varde * model.params.knots_to_km;
	}
	fclose(filpek);
	printf("check of knots_to_km %.3lf\n", model.params.knots_to_km);

	auto tid1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
	printf("loading %s took %.3lf\n", model.tables.tableTyp[1][tableNr].fileName, fp_ms);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.waveFactor.tableValue[i] < -9998) {
			errlog("ERROR! No value given for wave factor table pos %d. I set it to 0.\n", i);
			model.functions.waveFactor.tableValue[i] = 0;
		}
	}

	return nAlloc;
}

int loadRollingTable(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, relShipSpeedI, waveI, wavePeriodI, waveDirI, antal;
	double varde, wave, wavePeriod, waveDir, relShipSpeed;

	auto tid0 = std::chrono::high_resolution_clock::now();
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open wave factor table file %s\n", namn);
		postRequest("ERROR! Could not open wave factor table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[6][tableNr].relShipSpeed, &(model.functions.rolling.relShipSpeed));
	copyAddTableInfo(model.tables.tableTyp[6][tableNr].waveHeight, &(model.functions.rolling.waveHeight));
	copyAddTableInfo(model.tables.tableTyp[6][tableNr].wavePeriod, &(model.functions.rolling.wavePeriod));
	copyAddTableInfo(model.tables.tableTyp[6][tableNr].waveDirection, &(model.functions.rolling.waveDirection));
	nAlloc = model.functions.rolling.relShipSpeed.nIndex * model.functions.rolling.waveHeight.nIndex *
		model.functions.rolling.wavePeriod.nIndex * model.functions.rolling.waveDirection.nIndex;
	if (model.functions.rolling.tableValue != NULL)
		free(model.functions.rolling.tableValue);
	model.functions.rolling.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.rolling.tableValue[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);
	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf\t%lf\t%lf\n", &relShipSpeed, &wave, &wavePeriod, &waveDir, &varde);
		if (antal <= 0)
			break;

		relShipSpeedI = get_tableIndex(relShipSpeed, model.functions.rolling.relShipSpeed, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (relShipSpeedI < 0) {
			errlog("ERROR! relShipSpeed %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				relShipSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.relShipSpeed.minValue);
			postRequest("ERROR! relShipSpeed " + std::to_string(relShipSpeed) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.rolling.relShipSpeed.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (relShipSpeedI >= model.functions.rolling.relShipSpeed.nIndex) {
			errlog("ERROR! relShipSpeed %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				relShipSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.relShipSpeed.maxValue);
			postRequest("ERROR! relShipSpeed " + std::to_string(relShipSpeed) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.rolling.relShipSpeed.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveI = get_tableIndex(wave, model.functions.rolling.waveHeight, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveI < 0) {
			errlog("ERROR! wave %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.waveHeight.minValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.rolling.waveHeight.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveI >= model.functions.rolling.waveHeight.nIndex) {
			errlog("ERROR! wave %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.waveHeight.maxValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.rolling.waveHeight.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveDirI = get_tableIndexDirection(waveDir, model.functions.rolling.waveDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveDirI < 0) {
			errlog("ERROR! waveDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.waveDirection.minValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.rolling.waveDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveDirI >= model.functions.rolling.waveDirection.nIndex) {
			errlog("ERROR! waveDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.waveDirection.maxValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.rolling.waveDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		wavePeriodI = get_tableIndex(wavePeriod, model.functions.rolling.wavePeriod, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (wavePeriodI < 0) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.wavePeriod.minValue);
			postRequest("ERROR! wavePeriod " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.rolling.wavePeriod.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (wavePeriodI >= model.functions.rolling.wavePeriod.nIndex) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[6][tableNr].fileName,
				model.functions.rolling.wavePeriod.maxValue);
			postRequest("ERROR! wavePeriod " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[6][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.rolling.wavePeriod.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}

		pos = relShipSpeedI + model.functions.rolling.relShipSpeed.nIndex *
			(waveDirI + model.functions.rolling.waveDirection.nIndex *
				(wavePeriodI + model.functions.rolling.wavePeriod.nIndex * waveI));
		if (model.functions.rolling.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for rolling table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.rolling.tableValue[pos], varde);
		model.functions.rolling.tableValue[pos] = varde;
	}
	fclose(filpek);

	auto tid1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
	printf("loading %s took %.3lf\n", model.tables.tableTyp[6][tableNr].fileName, fp_ms);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.rolling.tableValue[i] < -9998) {
			errlog("ERROR! No value given for rolling table pos %d. I set it to 0.\n", i);
			model.functions.rolling.tableValue[i] = 0;
		}
	}

	return nAlloc;
}

int loadSurfRidingTable(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, relShipSpeedI, waveI, wavePeriodI, waveDirI, antal, nr = 7;
	double varde, wave, wavePeriod, waveDir, relShipSpeed;

	auto tid0 = std::chrono::high_resolution_clock::now();
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open wave factor table file %s\n", namn);
		postRequest("ERROR! Could not open wave factor table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].relShipSpeed, &(model.functions.surfRiding.relShipSpeed));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].waveHeight, &(model.functions.surfRiding.waveHeight));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].wavePeriod, &(model.functions.surfRiding.wavePeriod));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].waveDirection, &(model.functions.surfRiding.waveDirection));
	nAlloc = model.functions.surfRiding.relShipSpeed.nIndex * model.functions.surfRiding.waveHeight.nIndex *
		model.functions.surfRiding.wavePeriod.nIndex * model.functions.surfRiding.waveDirection.nIndex;
	if (model.functions.surfRiding.tableValue != NULL)
		free(model.functions.surfRiding.tableValue);
	model.functions.surfRiding.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.surfRiding.tableValue[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	free(namn);
	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf\t%lf\t%lf\n", &relShipSpeed, &wave, &wavePeriod, &waveDir, &varde);
		if (antal <= 0)
			break;

		relShipSpeedI = get_tableIndex(relShipSpeed, model.functions.surfRiding.relShipSpeed, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (relShipSpeedI < 0) {
			errlog("ERROR! relShipSpeed %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				relShipSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.relShipSpeed.minValue);
			postRequest("ERROR! relShipSpeed " + std::to_string(relShipSpeed) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.surfRiding.relShipSpeed.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (relShipSpeedI >= model.functions.surfRiding.relShipSpeed.nIndex) {
			errlog("ERROR! relShipSpeed %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				relShipSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.relShipSpeed.maxValue);
			postRequest("ERROR! relShipSpeed " + std::to_string(relShipSpeed) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.surfRiding.relShipSpeed.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveI = get_tableIndex(wave, model.functions.surfRiding.waveHeight, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveI < 0) {
			errlog("ERROR! wave %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.waveHeight.minValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.surfRiding.waveHeight.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveI >= model.functions.surfRiding.waveHeight.nIndex) {
			errlog("ERROR! wave %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.waveHeight.maxValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.surfRiding.waveHeight.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveDirI = get_tableIndexDirection(waveDir, model.functions.surfRiding.waveDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveDirI < 0) {
			errlog("ERROR! waveDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.waveDirection.minValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.surfRiding.waveDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveDirI >= model.functions.surfRiding.waveDirection.nIndex) {
			errlog("ERROR! waveDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.waveDirection.maxValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.surfRiding.waveDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}

		wavePeriodI = get_tableIndex(wavePeriod, model.functions.surfRiding.wavePeriod, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (wavePeriodI < 0) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.wavePeriod.minValue);
			postRequest("ERROR! wavePeriod " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.surfRiding.wavePeriod.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (wavePeriodI >= model.functions.surfRiding.wavePeriod.nIndex) {
			errlog("ERROR! wavePeriod %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.surfRiding.wavePeriod.maxValue);
			postRequest("ERROR! wavePeriod " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.surfRiding.wavePeriod.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}


		pos = relShipSpeedI + model.functions.surfRiding.relShipSpeed.nIndex *
			(waveDirI + model.functions.surfRiding.waveDirection.nIndex * 
				(wavePeriodI + model.functions.surfRiding.wavePeriod.nIndex * waveI));
		if (model.functions.surfRiding.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for surfRiding table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.surfRiding.tableValue[pos], varde);
		model.functions.surfRiding.tableValue[pos] = varde;
	}
	fclose(filpek);

	auto tid1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
	printf("loading %s took %.3lf\n", model.tables.tableTyp[nr][tableNr].fileName, fp_ms);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.surfRiding.tableValue[i] < -9998) {
			errlog("ERROR! No value given for surfRiding table pos %d. I set it to 0.\n", i);
			model.functions.surfRiding.tableValue[i] = 0;
		}
	}

	return nAlloc;
}



static int callbackIfTableExists(void* veryUsed, int argc, char** argv, char** azColName) {
	int i;
	int* svar = (int*)veryUsed;
	*svar = argc;
	printf("argc %d\n", argc);
	return 0;
}





template <typename TP>
std::time_t to_time_t(TP tp)
{
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}



int updateSQLiteAllTablesInfo(int type, int tablePos, int modified) {

	char* namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/shipTables/all_tables.db", model.params.indataPath.c_str());

	sqlite3* db;
	char* zErrMsg = 0;
	sqlite3_stmt* query;

	int rc = sqlite3_open(namn, &db);
	if (rc) {
		errlog("ERROR. Can't open database %s: %s\n", namn, sqlite3_errmsg(db));
		postRequest("ERROR! Could not open the database " + std::string(namn) + ". Is it possibly locked by another application. Close it and run the redis update again", 1);
	}
	else {
		errlog("Opened database successfully\n");
	}

	std::string sql;
	if(modified == 1)
		sql = "UPDATE all_tables SET epochCount = " + std::to_string(model.tmpEpochCount) + ", textFileName = '" + 
		std::string(model.tables.tableTyp[type][tablePos].fileName) + "' WHERE tableID = '" +
		std::string(model.tables.tableTyp[type][tablePos].tableID) + "' AND tableType = " +
		std::to_string(type) + ";";
	else
		sql = "INSERT INTO all_tables VALUES ('" +
		std::string(model.tables.tableTyp[type][tablePos].tableID) + "', " +
		std::to_string(type) + ", '" +
		std::string(model.tables.tableTyp[type][tablePos].fileName) + "', " +
		std::to_string(model.tmpEpochCount) + ");";

	if (sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg)) {
		printf("error failed to modify the database table all_tables: %s\n", zErrMsg);
		return 0;
	}
	sqlite3_close(db);


	return 0;
}


/*
int checkIfModifiedMap(int type) {

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	if (type == 1) {
		sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalAFileName.c_str());
	}
	else {
		sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyAFileName.c_str());
	}

	if (strcmp(model.sqliteMap[type].textFileName, namn) != 0) {
		free(model.sqliteMap[type].textFileName);
		model.sqliteMap[type].textFileName = str_alloc_cpy(namn);
		free(namn);
		return 1; // new name for the textfilename so have to load it
	}

#ifdef _WIN32
	try {
		const auto fileTime = std::filesystem::last_write_time(namn);
		const auto ticks = fileTime.time_since_epoch().count() - 1.33e17;
		model.tmpEpochCount = ticks;
	}
	catch (...) {
		printf("ERROR! Last modified date of %s given in file_params.json could not be read. Does it exist\n", namn);
		postRequest("ERROR! Last modified date of " + std::string(namn) + " given in file_params.json could not be read. Does it exist?", 1);
	}
#else
	struct stat result; 
	if (stat(namn, &result) != 0)
	{
		printf("ERROR! Failed to get stats from file %s\n", namn);
		postRequest("ERROR! Last modified date of " + std::string(namn) + " given in file_params.json could not be read. Does it exist?", 1);
	}
	auto ticks = result.st_mtime;
	model.tmpEpochCount = ticks;
#endif // 


	printf("model.tmpEpochCount %lf\n", model.tmpEpochCount);
	free(namn);

	if (model.sqliteMap[type].epochCount < model.tmpEpochCount - 10)
		return 1;
	else
		return 0;
}
*/

int updateSQLiteMap(int type) {
	char* namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/shipTables/all_tables.db", model.params.indataPath.c_str());

	sqlite3* db;
	char* zErrMsg = 0;
	sqlite3_stmt* query;

	int rc = sqlite3_open(namn, &db);
	if (rc) {
		errlog("ERROR. Can't open database %s: %s\n", namn, sqlite3_errmsg(db));
		postRequest("ERROR! Could not open the database " + std::string(namn) + ". Is it possibly locked by another application. Close it and run the redis update again", 1);
	}
	else {
		errlog("Opened database successfully\n");
	}

	std::string sql;
	sql = "UPDATE maps SET epochCount = " + std::to_string(model.tmpEpochCount) + ", textFileName = '" + std::string(model.sqliteMap[type].textFileName) + 
		"', nCols = " + std::to_string(model.sqliteMap[type].nCols) +
		", nRows = " + std::to_string(model.sqliteMap[type].nRows) +
		", size_col = " + std::to_string(model.sqliteMap[type].size_col) +
		", size_row = " + std::to_string(model.sqliteMap[type].size_row) +
		", minX = " + std::to_string(model.sqliteMap[type].minX) +
		", maxX = " + std::to_string(model.sqliteMap[type].maxX) +
		", minY = " + std::to_string(model.sqliteMap[type].minY) +
		", maxY = " + std::to_string(model.sqliteMap[type].maxY) +
		", nBlockRows = " + std::to_string(model.sqliteMap[type].nBlockRows) +
		", nBlockCols = " + std::to_string(model.sqliteMap[type].nBlockCols) +
		" WHERE mapNr = " +
		std::to_string(type) + ";";
	
	if (sqlite3_exec(db, sql.c_str(), NULL, 0, &zErrMsg)) {
		std::cout << sql << std::endl;
		printf("error failed to modify the database table maps: %s\n", zErrMsg);
		postRequest("ERROR! Failed to modify the database table maps in database " + std::string(namn) + ". Is it possibly locked by another application. Close it and run the redis update again", 1);
		return 0;
	}
	sqlite3_close(db);

	return 0;
}

/*
int saveMapsToBinary() {
	char* zErrMsg = 0;
	int i, modified;
	int nBlockRows, nBlockCols, nAlloc;
	unsigned short* arrShortInt;

	char* namn = (char*)malloc2(256 * sizeof(char));

	for (i = 0; i < 2; i++) {
		modified = checkIfModifiedMap(i);
		if (modified == 0)
			continue; // this file is not modified so no need to update it

		Raster rasterMapA;
		Raster::strPhysRaster physRaster;
		//load mapA
		if (i == 1) {
			sprintf(namn, "%s/mapPhysicalA.bin", model.params.indataPath.c_str());
		}
		else {
			sprintf(namn, "%s/mapFuelA.bin", model.params.indataPath.c_str());
		}

		printf("opens '%s'\n", model.sqliteMap[i].textFileName);
		rasterMapA.open(model.sqliteMap[i].textFileName);
		physRaster.valueCell = rasterMapA.GetRasterBand_intArr2(1, &(physRaster));
		auto tid0 = std::chrono::high_resolution_clock::now();

		std::ofstream wf(namn, std::ios::out | std::ios::binary);
		if (!wf) {
			std::cout << "Cannot open file!" << std::endl;
			postRequest("ERROR! Cannot open file " + std::string(namn) + " for writing binary. I quit");
			exitKontrollerat(__LINE__);
		}

		nBlockRows = roundUp((double)physRaster.nRows / physRaster.nBlock_y);
		nBlockCols = roundUp((double)physRaster.nCols / physRaster.nBlock_x);
		nAlloc = nBlockRows * nBlockCols;

		printf("saving map %s as binary, nAlloc %d nBlock xy %d %d nBlockCols/Rows %d %d\n",
			model.sqliteMap[i].textFileName, nAlloc,
			physRaster.nBlock_x, physRaster.nBlock_y, nBlockCols, nBlockRows);
		arrShortInt = (unsigned short*)malloc2(nAlloc * sizeof(unsigned short));

		int pos, i1, i2, i4, i5, pos2;
		pos = 0;
		for (i1 = 0; i1 < physRaster.nBlock_y; i1++) {
			for (i2 = 0; i2 < physRaster.nBlock_x; i2++) {
				pos2 = 0;
				for (i4 = nBlockRows * i1; i4 < nBlockRows * (i1 + 1); i4++) {
					for (i5 = nBlockCols * i2; i5 < nBlockCols * (i2 + 1); i5++) {
						if (i4 < physRaster.nRows && i5 < physRaster.nCols) {
							arrShortInt[pos2] = physRaster.valueCell[i5 + physRaster.nCols * i4];
						}
						else
							arrShortInt[pos2] = 0;
						pos2++;
					}
				}
				wf.write(reinterpret_cast<const char*>(arrShortInt), nAlloc * sizeof(unsigned short));
				pos++;
			}
		}
		wf.close();
		if (!wf.good()) {
			std::cout << "Error occurred at writing time!" << std::endl;
			postRequest("ERROR occured at binary writing time of file " + std::string(namn) + ". I quit");
			exitKontrollerat(__LINE__);
		}
		free(arrShortInt);

		model.sqliteMap[i].nCols = physRaster.nCols;
		model.sqliteMap[i].nRows = physRaster.nRows;
		model.sqliteMap[i].size_col = physRaster.size_col;
		model.sqliteMap[i].size_row = physRaster.size_row;
		model.sqliteMap[i].minX = physRaster.minLongitude;
		model.sqliteMap[i].maxX = physRaster.maxLongitude;
		model.sqliteMap[i].minY = physRaster.minLatitude;
		model.sqliteMap[i].maxY = physRaster.maxLatitude;
		model.sqliteMap[i].nBlockRows = roundUp(physRaster.nRows / 10.0);
		model.sqliteMap[i].nBlockCols = roundUp(physRaster.nCols / 10.0);
		updateSQLiteMap(i);
		auto tid1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
		printf("write binary %s took %.3lf\n", namn, fp_ms);

	}


	return 0;
}
*/

int loadWeatherFactorTableWind(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, windI, windDirI, calmWaterSpeedI, antal;
	double varde, wind, windDir, calmWaterSpeed;

	auto tid0 = std::chrono::high_resolution_clock::now();
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName);

	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open wind factor table file %s\n", namn);
		postRequest("ERROR! Could not open wind factor table file " + std::string(namn), 1);
	}


	copyAddTableInfo(model.tables.tableTyp[0][tableNr].shipSpeedCalmWater, &(model.functions.windFactor.shipSpeedCalmWater));
	copyAddTableInfo(model.tables.tableTyp[0][tableNr].windSpeed, &(model.functions.windFactor.windSpeed));
	copyAddTableInfo(model.tables.tableTyp[0][tableNr].windDirection, &(model.functions.windFactor.windDirection));
	model.params.maxWindSpeed_warning = model.tables.tableTyp[0][tableNr].maxWindSpeed_warning;
	printf("windFactor dimensions %d %d %d\n", model.functions.windFactor.shipSpeedCalmWater.nIndex,
		model.functions.windFactor.windSpeed.nIndex, model.functions.windFactor.windDirection.nIndex);

	nAlloc = model.functions.windFactor.shipSpeedCalmWater.nIndex * model.functions.windFactor.windSpeed.nIndex * 
		model.functions.windFactor.windDirection.nIndex;
	if (model.functions.windFactor.tableValue != NULL)
		free(model.functions.windFactor.tableValue);
	model.functions.windFactor.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.windFactor.tableValue[i] = -9999;
	}
	printf("nAlloc %d %d %d %d\n", nAlloc, model.functions.windFactor.shipSpeedCalmWater.nIndex, 
		model.functions.windFactor.windSpeed.nIndex, model.functions.windFactor.windDirection.nIndex);

	//printf("windFactor calmWaterSpeed nIndex %d minVal %.3lf interval %.3lf maxVal %.3lf\n", 
	//	model.functions.windFactor.shipSpeedCalmWater.nIndex, model.functions.windFactor.shipSpeedCalmWater.minValue,
	//	model.functions.windFactor.shipSpeedCalmWater.intervalSize, model.functions.windFactor.shipSpeedCalmWater.maxValue);


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
	
		calmWaterSpeedI = get_tableIndex(calmWaterSpeed, model.functions.windFactor.shipSpeedCalmWater, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (calmWaterSpeedI < 0) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName, 
				model.functions.windFactor.shipSpeedCalmWater.minValue);
			postRequest("ERROR2! calmWaterSpeed " + std::to_string(calmWaterSpeed) + " given in " +
				std::string(model.tables.tableTyp[0][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.windFactor.shipSpeedCalmWater.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (calmWaterSpeedI >= model.functions.windFactor.shipSpeedCalmWater.nIndex) {
			errlog("ERROR! calmWaterSpeed %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				calmWaterSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName, 
				model.functions.windFactor.shipSpeedCalmWater.maxValue);
			postRequest("ERROR2! calmWaterSpeed " + std::to_string(calmWaterSpeed) + " given in " +
				std::string(model.tables.tableTyp[0][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.windFactor.shipSpeedCalmWater.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		windI = get_tableIndex(wind, model.functions.windFactor.windSpeed, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (windI < 0) {
			errlog("ERROR! wind %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName,
				model.functions.windFactor.windSpeed.minValue);
			postRequest("ERROR2! wind " + std::to_string(wind) + " given in " +
				std::string(model.tables.tableTyp[0][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.windFactor.windSpeed.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (windI >= model.functions.windFactor.windSpeed.nIndex) {
			errlog("ERROR! wind %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName,
				model.functions.windFactor.windSpeed.maxValue);
			postRequest("ERROR2! wind " + std::to_string(wind) + " given in " +
				std::string(model.tables.tableTyp[0][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.windFactor.windSpeed.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		windDirI = get_tableIndexDirection(windDir, model.functions.windFactor.windDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (windDirI < 0) {
			errlog("ERROR! windDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName,
				model.functions.windFactor.windDirection.minValue);
			postRequest("ERROR2! windDir " + std::to_string(windDir) + " given in " +
				std::string(model.tables.tableTyp[0][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.windFactor.windDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (windDirI >= model.functions.windFactor.windDirection.nIndex) {
			errlog("ERROR! windDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.tables.tableTyp[0][tableNr].fileName,
				model.functions.windFactor.windDirection.maxValue);
			postRequest("ERROR2! windDir " + std::to_string(windDir) + " given in " +
				std::string(model.tables.tableTyp[0][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.windFactor.windDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}

		pos = calmWaterSpeedI + model.functions.windFactor.shipSpeedCalmWater.nIndex *
			(windDirI + model.functions.windFactor.windDirection.nIndex * windI);
		if (model.functions.windFactor.tableValue[pos] > -9998) {
			windI = pos / (model.functions.windFactor.shipSpeedCalmWater.nIndex* model.functions.windFactor.windDirection.nIndex);
			windDirI = (pos - windI * (model.functions.windFactor.shipSpeedCalmWater.nIndex * model.functions.windFactor.windDirection.nIndex)) /
				model.functions.windFactor.shipSpeedCalmWater.nIndex;
			calmWaterSpeedI = pos - windI * (model.functions.windFactor.shipSpeedCalmWater.nIndex * model.functions.windFactor.windDirection.nIndex) -
				windDirI * model.functions.windFactor.shipSpeedCalmWater.nIndex;
			errlog("ERROR! More than one value for wind factor table pos %d, before %lf, now %lf (speed %.2lf wind %.2lf windDir %.2lf). I use the later one.\n", pos,
				model.functions.windFactor.tableValue[pos], varde,
				model.functions.windFactor.shipSpeedCalmWater.minValue + calmWaterSpeedI * model.functions.windFactor.shipSpeedCalmWater.intervalSize,
				model.functions.windFactor.windSpeed.minValue + windI * model.functions.windFactor.windSpeed.intervalSize,
				model.functions.windFactor.windDirection.minValue + windDirI * model.functions.windFactor.windDirection.intervalSize);
		}
		model.functions.windFactor.tableValue[pos] = varde * model.params.knots_to_km;
	}
	fclose(filpek);

	auto tid1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
	printf("loading %s took %.3lf\n", model.tables.tableTyp[0][tableNr].fileName, fp_ms);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.windFactor.tableValue[i] < -9998) {
			errlog("ERROR! No value given for wind factor table pos %d. I set it to 0.\n", i);
			model.functions.windFactor.tableValue[i] = 0;
		}
	}

	return nAlloc;
}

int  loadDynamicStabilityTable(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, windI, windDirI, shipSpeedI, antal, nr = 3;
	double varde, wind, windDir, shipSpeed;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open dynamic stability table file %s\n", namn);
		postRequest("ERROR! Could not open dynamic stability table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].windSpeed, &(model.functions.dynStability.windSpeed));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].windDirection, &(model.functions.dynStability.windDirection));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].shipSpeedOverGround, &(model.functions.dynStability.shipSpeedOverGround));

	if (model.functions.dynStability.tableValue != NULL)
		free(model.functions.dynStability.tableValue);
	nAlloc = model.functions.dynStability.windSpeed.nIndex * model.functions.dynStability.windDirection.nIndex *
		model.functions.dynStability.shipSpeedOverGround.nIndex;
	model.functions.dynStability.tableValue = (float*)malloc2(nAlloc * sizeof(float));
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
		windI = get_tableIndex(wind, model.functions.dynStability.windSpeed, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (windI < 0) {
			errlog("ERROR! wind %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.dynStability.windSpeed.minValue);
			postRequest("ERROR3! wind " + std::to_string(wind) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.dynStability.windSpeed.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (windI >= model.functions.dynStability.windSpeed.nIndex) {
			errlog("ERROR! wind %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.dynStability.windSpeed.maxValue);
			postRequest("ERROR3! wind " + std::to_string(wind) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.dynStability.windSpeed.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		windDirI = get_tableIndexDirection(windDir, model.functions.dynStability.windDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (windDirI < 0) {
			errlog("ERROR! windDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.dynStability.windDirection.minValue);
			postRequest("ERROR3! windDir " + std::to_string(windDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.dynStability.windDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (windDirI >= model.functions.dynStability.windDirection.nIndex) {
			errlog("ERROR! windDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.dynStability.windDirection.maxValue);
			postRequest("ERROR3! windDir " + std::to_string(windDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.dynStability.windDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		shipSpeedI = get_tableIndex(shipSpeed, model.functions.dynStability.shipSpeedOverGround, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (shipSpeedI < 0) {
			errlog("ERROR! shipSpeedOverGround %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				shipSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.dynStability.shipSpeedOverGround.minValue);
			postRequest("ERROR3! shipSpeedOverGround " + std::to_string(shipSpeed) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.dynStability.shipSpeedOverGround.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (shipSpeedI >= model.functions.dynStability.shipSpeedOverGround.nIndex) {
			errlog("ERROR! shipSpeedOverGround %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				shipSpeed, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.dynStability.shipSpeedOverGround.maxValue);
			postRequest("ERROR3! shipSpeedOverGround " + std::to_string(shipSpeed) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.dynStability.shipSpeedOverGround.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		pos = shipSpeedI + model.functions.dynStability.shipSpeedOverGround.nIndex *
			(windDirI + model.functions.dynStability.windDirection.nIndex * windI);
		if (model.functions.dynStability.tableValue[pos] > -9998)
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

	return nAlloc;
}

int  loadFuelFactorMainTable(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, windI, windDirI, shipSpeedI, antal, nr = 2, waveI, waveDirI;
	double varde, wind, windDir, wave, waveDir;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open fuel factor table file %s\n", namn);
		postRequest("ERROR! Could not open fuel factor table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].windSpeed, &(model.functions.fuelFactorMain.windSpeed));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].windDirection, &(model.functions.fuelFactorMain.windDirection));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].waveHeight, &(model.functions.fuelFactorMain.waveHeight));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].waveDirection, &(model.functions.fuelFactorMain.waveDirection));

	if (model.functions.fuelFactorMain.tableValue != NULL)
		free(model.functions.fuelFactorMain.tableValue);
	nAlloc = model.functions.fuelFactorMain.windSpeed.nIndex * model.functions.fuelFactorMain.windDirection.nIndex *
		model.functions.fuelFactorMain.waveHeight.nIndex * model.functions.fuelFactorMain.waveDirection.nIndex;
	model.functions.fuelFactorMain.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.fuelFactorMain.tableValue[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);

	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf\t%lf\t%lf", &wind, &windDir, &wave, &waveDir, &varde);
		if (antal <= 0)
			break;
		windI = get_tableIndex(wind, model.functions.fuelFactorMain.windSpeed, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (windI < 0) {
			errlog("ERROR! wind %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.windSpeed.minValue);
			postRequest("ERROR3! wind " + std::to_string(wind) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.fuelFactorMain.windSpeed.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (windI >= model.functions.fuelFactorMain.windSpeed.nIndex) {
			errlog("ERROR! wind %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wind, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.windSpeed.maxValue);
			postRequest("ERROR3! wind " + std::to_string(wind) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.fuelFactorMain.windSpeed.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		windDirI = get_tableIndexDirection(windDir, model.functions.fuelFactorMain.windDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (windDirI < 0) {
			errlog("ERROR! windDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.windDirection.minValue);
			postRequest("ERROR3! windDir " + std::to_string(windDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.fuelFactorMain.windDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (windDirI >= model.functions.fuelFactorMain.windDirection.nIndex) {
			errlog("ERROR! windDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				windDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.windDirection.maxValue);
			postRequest("ERROR3! windDir " + std::to_string(windDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.fuelFactorMain.windDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveI = get_tableIndex(wave, model.functions.fuelFactorMain.waveHeight, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveI < 0) {
			errlog("ERROR! wave %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.waveHeight.minValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.fuelFactorMain.waveHeight.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveI >= model.functions.fuelFactorMain.waveHeight.nIndex) {
			errlog("ERROR! wave %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wave, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.waveHeight.maxValue);
			postRequest("ERROR! wave " + std::to_string(wave) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.fuelFactorMain.waveHeight.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		waveDirI = get_tableIndexDirection(waveDir, model.functions.fuelFactorMain.waveDirection, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (waveDirI < 0) {
			errlog("ERROR! waveDir %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.waveDirection.minValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.fuelFactorMain.waveDirection.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (waveDirI >= model.functions.fuelFactorMain.waveDirection.nIndex) {
			errlog("ERROR! waveDir %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveDir, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.fuelFactorMain.waveDirection.maxValue);
			postRequest("ERROR! waveDir " + std::to_string(waveDir) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.fuelFactorMain.waveDirection.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}

		pos = windDirI + model.functions.fuelFactorMain.windDirection.nIndex * (windI +
			model.functions.fuelFactorMain.windSpeed.nIndex * (waveDirI +
				model.functions.fuelFactorMain.waveDirection.nIndex * waveI));
		if (model.functions.fuelFactorMain.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for dynamic stability table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.fuelFactorMain.tableValue[pos], varde);
		model.functions.fuelFactorMain.tableValue[pos] = varde;
	}
	fclose(filpek);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.fuelFactorMain.tableValue[i] < -9998) {
			errlog("ERROR! No value given for fuel factor table pos %d. I set it to 0.\n", i);
			model.functions.fuelFactorMain.tableValue[i] = 0;
		}
	}

	return nAlloc;
}

int  loadBowSlammingTable(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, heightI, periodI, antal, nr = 4;
	double varde, waveHeight, wavePeriod;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open bow slamming table file %s\n", namn);
		postRequest("ERROR! Could not open bow slamming table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].waveHeight, &(model.functions.bowSlamming.waveHeight));
	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].wavePeriod, &(model.functions.bowSlamming.wavePeriod));

	if (model.functions.bowSlamming.tableValue != NULL)
		free(model.functions.bowSlamming.tableValue);
	nAlloc = model.functions.bowSlamming.waveHeight.nIndex * model.functions.bowSlamming.wavePeriod.nIndex;
	model.functions.bowSlamming.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.bowSlamming.tableValue[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);

	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf\t%lf", &waveHeight, &wavePeriod, &varde);
		if (antal <= 0)
			break;
		heightI = get_tableIndex(waveHeight, model.functions.bowSlamming.waveHeight, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (heightI < 0) {
			errlog("ERROR! wave height %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveHeight, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.bowSlamming.waveHeight.minValue);
			postRequest("ERROR3! wave height " + std::to_string(waveHeight) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.bowSlamming.waveHeight.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (heightI >= model.functions.bowSlamming.waveHeight.nIndex) {
			continue;
			errlog("ERROR! wave height %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveHeight, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.bowSlamming.waveHeight.maxValue);
			postRequest("ERROR3! wave height " + std::to_string(waveHeight) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.bowSlamming.waveHeight.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		periodI = get_tableIndex(wavePeriod, model.functions.bowSlamming.wavePeriod, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (periodI < 0) {
			errlog("ERROR! wave period %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.bowSlamming.wavePeriod.minValue);
			postRequest("ERROR3! wave period " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.bowSlamming.wavePeriod.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (periodI >= model.functions.bowSlamming.wavePeriod.nIndex) {
			errlog("ERROR! wave period %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				wavePeriod, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.bowSlamming.wavePeriod.maxValue);
			postRequest("ERROR3! wave period " + std::to_string(wavePeriod) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.bowSlamming.wavePeriod.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		pos = periodI + model.functions.bowSlamming.wavePeriod.nIndex * heightI;
		if (model.functions.bowSlamming.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for bow slamming table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.bowSlamming.tableValue[pos], varde);
		model.functions.bowSlamming.tableValue[pos] = varde;
	}
	fclose(filpek);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.bowSlamming.tableValue[i] < -9998) {
			errlog("ERROR! No value given for bow slamming table pos %d. I set it to 0.\n", i);
			model.functions.bowSlamming.tableValue[i] = 0;
		}
	}

	return nAlloc;
}

int  loadGreenWaterTable(int tableNr) {
	FILE* filpek;
	int nAlloc, i, pos, heightI, antal, nr = 5;
	double varde, waveHeight;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName);
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		errlog("ERROR! Could not open bow slamming table file %s\n", namn);
		postRequest("ERROR! Could not open bow slamming table file " + std::string(namn), 1);
	}

	copyAddTableInfo(model.tables.tableTyp[nr][tableNr].waveHeight, &(model.functions.greenWater.waveHeight));

	if (model.functions.greenWater.tableValue != NULL)
		free(model.functions.greenWater.tableValue);
	nAlloc = model.functions.greenWater.waveHeight.nIndex;
	model.functions.greenWater.tableValue = (float*)malloc2(nAlloc * sizeof(float));
	for (i = 0; i < nAlloc; i++) {
		model.functions.greenWater.tableValue[i] = -9999;
	}

	antal = fscanf(filpek, "%s\t", namn);
	antal = fscanf(filpek, "%s\n", namn);
	free(namn);

	for (i = 0; i < nAlloc; i++) {
		antal = fscanf(filpek, "%lf\t%lf", &waveHeight, &varde);
		if (antal <= 0)
			break;
		heightI = get_tableIndex(waveHeight, model.functions.greenWater.waveHeight, 1); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors, 1);
		if (heightI < 0) {
			errlog("ERROR! wave height %lf given in %s/%s is less than min %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveHeight, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.greenWater.waveHeight.minValue);
			postRequest("ERROR3! wave height " + std::to_string(waveHeight) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is less than min " +
				std::to_string(model.functions.greenWater.waveHeight.minValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		if (heightI >= model.functions.greenWater.waveHeight.nIndex) {
			errlog("ERROR! wave height %lf given in %s/%s is more than max %lf given in table_parameters.json.\nFix and run again, i quit!\n",
				waveHeight, model.params.indataPath.c_str(), model.tables.tableTyp[nr][tableNr].fileName,
				model.functions.greenWater.waveHeight.maxValue);
			postRequest("ERROR3! wave height " + std::to_string(waveHeight) + " given in " +
				std::string(model.tables.tableTyp[nr][tableNr].fileName) + " is more than max " +
				std::to_string(model.functions.greenWater.waveHeight.maxValue) +
				" given in table_parameters.json. Fix and run again, i quit!", 1);
		}
		pos = heightI;
		if(model.functions.greenWater.tableValue[pos] > -9998)
			errlog("ERROR! More than one value for green water table pos %d, before %lf, now %lf. I use the later one.\n", pos,
				model.functions.greenWater.tableValue[pos], varde);
		model.functions.greenWater.tableValue[pos] = varde;
	}
	fclose(filpek);

	for (i = 0; i < nAlloc; i++) {
		if (model.functions.greenWater.tableValue[i] < -9998) {
			errlog("ERROR! No value given for green water table pos %d. I set it to 0.\n", i);
			model.functions.greenWater.tableValue[i] = 0;
		}
	}

	return nAlloc;
}


void getBastSpeedPos(int nSettings, double target, int* indexUnder, int* indexOver, double* kvot) {
	int i, posUnder = -1, posOver = -1;
	double valUnder, valOver, minValUnder = 9999, minValOver = 9999, diff;

	for (i = 0; i < nSettings; i++) {
		if (target > model.functions.rpmSetting_gerCalmWaterSpeedBase[i]) {
			valUnder = target - model.functions.rpmSetting_gerCalmWaterSpeedBase[i];
			if (minValUnder > valUnder) {
				minValUnder = valUnder;
				posUnder = i;
			}
		}
		else {
			valOver = model.functions.rpmSetting_gerCalmWaterSpeedBase[i] - target;
			if (minValOver > valOver) {
				minValOver = valOver;
				posOver = i;
			}
		}
	}
	if (posUnder == -1) {
		*indexUnder = posOver;
		*indexOver = -1;
		*kvot = -1;
		if (minValOver > 0.001)
			errlog("ERROR! A speed of %.3lf knots is given but the lowest speed given for this digital ship is %.3lf which I will use\n",
				target / 1.852, (target + minValOver) / 1.852);
	}
	else if (posOver == -1) {
		*indexUnder = posUnder;
		*indexOver = -1;
		*kvot = -1;
		if (minValUnder > 0.001)
			errlog("ERROR! A speed of %.3lf knots is given but the highest speed given for this digital ship is %.3lf which I will use\n",
				target / 1.852, (target - minValUnder) / 1.852);
	}
	else {
		*indexUnder = posUnder;
		*indexOver = posOver;
		diff = model.functions.rpmSetting_gerCalmWaterSpeedBase[posOver] - model.functions.rpmSetting_gerCalmWaterSpeedBase[posUnder];
		if (diff < 0.0001) {
			*indexOver = -1;
			*kvot = -1;
		}
		else {
			*kvot = (target - model.functions.rpmSetting_gerCalmWaterSpeedBase[posUnder]) / diff;
		}
	}
	//printf("BastSpeedPos target %.2lf indexUnder/Over %d %d kvot %.3lf nSettings %d\n", target/1.852,
	//	*indexUnder, *indexOver, *kvot, nSettings);
}

void getBastConsumptionPos(int nSettings, double target, int* indexUnder, int* indexOver, double* kvot) {
	int i, posUnder = -1, posOver = -1;
	double valUnder, valOver, minValUnder = 9999, minValOver = 9999, fuel, diff;

	for (i = 0; i < nSettings; i++) {
		fuel = model.functions.rpmSetting_gerFuelConsumption_mainBase[i]; // only main engine +model.functions.rpmSetting_gerFuelConsumption_auxBase[i];
		if (target > fuel) {
			valUnder = target - fuel;
			if (minValUnder > valUnder) {
				minValUnder = valUnder;
				posUnder = i;
			}
		}
		else {
			valOver = fuel - target;
			if (minValOver > valOver) {
				minValOver = valOver;
				posOver = i;
			}
		}
	}
	if (posUnder == -1) {
		*indexUnder = posOver;
		*indexOver = -1;
		*kvot = -1;
		if(minValOver > 0.001)
			errlog("ERROR! A consumption of %.3lf mpd is given but the lowest consumption given for this digital ship is %.3lf (main) which I will use\n",
				target * 24.0, (target + minValOver) * 24.0);
	}
	else if (posOver == -1) {
		*indexUnder = posUnder;
		*indexOver = -1;
		*kvot = -1;
		if (minValUnder > 0.001)
			errlog("ERROR! A consumption of %.3lf mpd is given but the higest consumption given for this digital ship is %.3lf (main) which I will use\n",
				target * 24.0, (target - minValUnder) * 24.0);
	}
	else {
		*indexUnder = posUnder;
		*indexOver = posOver;
		diff = model.functions.rpmSetting_gerFuelConsumption_mainBase[posOver] -
			(model.functions.rpmSetting_gerFuelConsumption_mainBase[posUnder]);
		if (diff < 0.0001) {
			*indexOver = -1;
			*kvot = -1;
		}
		else {
			*kvot = (target - (model.functions.rpmSetting_gerFuelConsumption_mainBase[posUnder])) / diff;
		}
	}
	//printf("BastConsumptionPos target %.2lf indexUnder/Over %d %d kvot %.3lf\n", target,
	//	*indexUnder, *indexOver, *kvot);
}

int set_speedSettingsFromBase(strSpeed* speedSetting, int i, int iUse, int iOver, double kvot, int legNr) {
	if (iOver == -1) {
		if (speedSetting != NULL) {
			speedSetting->rpm[i] = model.functions.rpmBase[iUse];
			speedSetting->rpmSetting_gerCalmWaterSpeed[i] = model.functions.rpmSetting_gerCalmWaterSpeedBase[iUse];
			speedSetting->rpmSetting_gerFuelConsumption_main[i] = model.functions.rpmSetting_gerFuelConsumption_mainBase[iUse];
			speedSetting->rpmSetting_gerFuelConsumption_aux[i] = model.functions.rpmSetting_gerFuelConsumption_auxBase[iUse];
			speedSetting->settingGerBaseSetting[i] = iUse;
		}
		else {
			model.functions.rpmSetting_gerCalmWaterSpeedDelay[legNr][i] = model.functions.rpmSetting_gerCalmWaterSpeedBase[iUse];
			model.functions.rpmSetting_gerFuelConsumption_mainDelay[legNr][i] = model.functions.rpmSetting_gerFuelConsumption_mainBase[iUse];
			model.functions.rpmSetting_gerFuelConsumption_auxDelay[legNr][i] = model.functions.rpmSetting_gerFuelConsumption_auxBase[iUse];
		}
	}
	else {
		if (speedSetting != NULL) {
			speedSetting->rpm[i] = model.functions.rpmBase[iUse] * (1 - kvot) + model.functions.rpmBase[iOver] * kvot;
			speedSetting->rpmSetting_gerCalmWaterSpeed[i] = model.functions.rpmSetting_gerCalmWaterSpeedBase[iUse] * (1 - kvot) + model.functions.rpmSetting_gerCalmWaterSpeedBase[iOver] * kvot;
			speedSetting->rpmSetting_gerFuelConsumption_main[i] = model.functions.rpmSetting_gerFuelConsumption_mainBase[iUse] * (1 - kvot) + model.functions.rpmSetting_gerFuelConsumption_mainBase[iOver] * kvot;
			speedSetting->rpmSetting_gerFuelConsumption_aux[i] = model.functions.rpmSetting_gerFuelConsumption_auxBase[iUse] * (1 - kvot) + model.functions.rpmSetting_gerFuelConsumption_auxBase[iOver] * kvot;
			if (kvot <= 0.5)
				speedSetting->settingGerBaseSetting[i] = iUse;
			else
				speedSetting->settingGerBaseSetting[i] = iOver;
		}
		else {
			model.functions.rpmSetting_gerCalmWaterSpeedDelay[legNr][i] = model.functions.rpmSetting_gerCalmWaterSpeedBase[iUse] * (1 - kvot) + model.functions.rpmSetting_gerCalmWaterSpeedBase[iOver] * kvot;
			model.functions.rpmSetting_gerFuelConsumption_mainDelay[legNr][i] = model.functions.rpmSetting_gerFuelConsumption_mainBase[iUse] * (1 - kvot) + model.functions.rpmSetting_gerFuelConsumption_mainBase[iOver] * kvot;
			model.functions.rpmSetting_gerFuelConsumption_auxDelay[legNr][i] = model.functions.rpmSetting_gerFuelConsumption_auxBase[iUse] * (1 - kvot) + model.functions.rpmSetting_gerFuelConsumption_auxBase[iOver] * kvot;
		}
	}
	return 0;
}


int getBaseSpeedSetting(int speedSetting, int lev1, int lev2) {
	int speedNr;
	if (lev1 >= 0)
		speedNr = model.functions.speedLevel[lev1].settingGerBaseSetting[speedSetting];
	else {
		if (lev2 >= 0)
			speedNr = model.functions.speedChannelOut[-lev1 - 1].settingGerBaseSetting[speedSetting];
		else
			speedNr = model.functions.speedChannel[-lev1 - 1].settingGerBaseSetting[speedSetting];
	}
	return speedNr;
}


int getClosestSetting_fromBase(int baseSetting, int fromLevel, int toLevel) {
	int i;

	strSpeed* speedSetting;

	if (fromLevel >= 0)
		speedSetting = &(model.functions.speedLevel[fromLevel]);
	else {
		if (toLevel >= 0)
			speedSetting = &(model.functions.speedChannelOut[-fromLevel - 1]);
		else
			speedSetting = &(model.functions.speedChannel[-fromLevel - 1]);
	}

	for (i = 0; i < speedSetting->nShip_speedSettings; i++) {
		if (baseSetting <= speedSetting->settingGerBaseSetting[i])
			break;
	}
	if (i >= speedSetting->nShip_speedSettings)
		return speedSetting->nShip_speedSettings - 1;
	if (i == 0)
		return i;
	if (baseSetting - speedSetting->settingGerBaseSetting[i - 1] <= speedSetting->settingGerBaseSetting[i] - baseSetting)
		return i - 1;
	else
		return i;
}

int getNextTraff_speedPos(int lastKvotTraff, int minPos, int maxPos, double minSpeed, double speedInterval, double kvotTraff) {
	int i, bastPos = model.functions.nShip_speedSettingsBase - 1;
	double kvotNu, bastFel = 2.0;

	for (i = lastKvotTraff + 1; i < model.functions.nShip_speedSettingsBase; i++) {
		kvotNu = (model.functions.rpmSetting_gerCalmWaterSpeedBase[i] - minSpeed) / speedInterval;
		if (abs(kvotNu - kvotTraff) < bastFel) {
			bastFel = abs(kvotNu - kvotTraff);
			bastPos = i;
		}
		if (kvotNu >= kvotTraff)
			break;
	}
	return bastPos;
}

void setupUsableSpeedSettings() {
	int *nAlloc, minPos = 0, maxPos = 0, midPos, add95, iUse, i, indexUnder, indexOver = -1, i1;
	int nextTraff, legNr;
	double min_rpm, max_rpm, midVal, diff, min_diff, delta, target, kvot;

	model.functions.speedLevel = (strSpeed*)malloc2(model.network.nPhysicalLevels * sizeof(strSpeed));
	model.functions.speedChannel = (strSpeed*)malloc2(model.network.nChannels * sizeof(strSpeed));
	model.functions.speedChannelOut = (strSpeed*)malloc2(model.network.nChannels * sizeof(strSpeed));

	model.functions.rpmSetting_gerCalmWaterSpeedDelay = (double**)malloc(model.params.nLegs * sizeof(double*));
	model.functions.rpmSetting_gerFuelConsumption_mainDelay = (double**)malloc(model.params.nLegs * sizeof(double*));
	model.functions.rpmSetting_gerFuelConsumption_auxDelay = (double**)malloc(model.params.nLegs * sizeof(double*));
	for (legNr = 0; legNr < model.params.nLegs; legNr++) {
		model.functions.rpmSetting_gerCalmWaterSpeedDelay[legNr] = (double*)malloc(model.functions.nShip_speedSettingsBase * sizeof(double));
		model.functions.rpmSetting_gerFuelConsumption_mainDelay[legNr] = (double*)malloc(model.functions.nShip_speedSettingsBase * sizeof(double));
		model.functions.rpmSetting_gerFuelConsumption_auxDelay[legNr] = (double*)malloc(model.functions.nShip_speedSettingsBase * sizeof(double));
	}
	model.functions.nSpeedSettingsDelay = (int*)calloc(model.params.nLegs, sizeof(int));

	double maxSpeed = 0, minSpeed = 1e10, speedInterval;

	int* nShip_speedSettings = (int*)malloc(model.params.nLegs * sizeof(int));
	nAlloc = (int*)malloc(model.params.nLegs * sizeof(int));
	model.functions.nAllocShipSpeedsLevel = (int*)malloc(model.params.nLegs * sizeof(int));
	model.functions.speedSetting95MCR_use = (int*)malloc(model.params.nLegs * sizeof(int));
	//model.functions.nShip_speedSettingsDelay = (int*)malloc(model.params.nLegs * sizeof(int));

	for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
		if (minSpeed > model.functions.rpmSetting_gerCalmWaterSpeedBase[i]) {
			minSpeed = model.functions.rpmSetting_gerCalmWaterSpeedBase[i];
			minPos = i;
		}
		if (maxSpeed < model.functions.rpmSetting_gerCalmWaterSpeedBase[i]) {
			maxSpeed = model.functions.rpmSetting_gerCalmWaterSpeedBase[i];
			maxPos = i;
		}
	}
	speedInterval = maxSpeed - minSpeed;
	if (speedInterval < 0.01)
		speedInterval = 0.01;

	for (legNr = 0; legNr < model.params.nLegs; legNr++) {
		model.functions.nSpeedSettingsDelay[legNr] = 1;
		if (model.params.legCommercial[legNr].commercialAllowedVariation < 0 && model.results.forecastTypeOrig <= 1000) {
			// not commercial
			nShip_speedSettings[legNr] = model.params.nSpeedSettingDivideIter1 + 2;
			nAlloc[legNr] = model.functions.nShip_speedSettingsBase; //  5;
		}
		else {
			// commercial version
			if (model.params.legCommercial[legNr].commercialAllowedVariation < 0.001 || model.results.forecastTypeOrig > 1000) {
				model.params.legCommercial[legNr].commercialAllowedVariation = 0;
				nShip_speedSettings[legNr] = 1;
				nAlloc[legNr] = 1;
			}
			else {
				nShip_speedSettings[legNr] = 5;
				nAlloc[legNr] = model.functions.nShip_speedSettingsBase;
			}
		}
		model.functions.nAllocShipSpeedsLevel[legNr] = nAlloc[legNr];
		if (nShip_speedSettings[legNr] > model.functions.nShip_speedSettingsBase) {
			nShip_speedSettings[legNr] = model.functions.nShip_speedSettingsBase;
		}

		model.functions.speedSetting95MCR_use[legNr] = model.functions.speedSetting95MCR_base;
		if (model.functions.speedSetting95MCR_use[legNr] < nShip_speedSettings[legNr] && (model.params.legCommercial[legNr].commercialAllowedVariation >= 0 ||
			model.results.forecastTypeOrig > 1000))
			model.functions.speedSetting95MCR_use[legNr] = nShip_speedSettings[legNr];
	}

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		legNr = model.network.physicalLev[i].legNr;
		model.functions.speedLevel[i].rpm = (double*)malloc2(nAlloc[legNr] * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerCalmWaterSpeed = (double*)malloc2(nAlloc[legNr] * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerFuelConsumption_main = (double*)malloc2(nAlloc[legNr] * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerFuelConsumption_aux = (double*)malloc2(nAlloc[legNr] * sizeof(double));
		model.functions.speedLevel[i].settingGerBaseSetting = (int*)malloc2(nAlloc[legNr] * sizeof(int));
		model.functions.speedLevel[i].nShip_speedSettings = nShip_speedSettings[legNr];
	}

	int nAllocNu, usedLevel, usedLevelChannel;
	for (i = 0; i < model.network.nChannels; i++) {
		legNr = model.network.channel[i].legNr;
		nAllocNu = model.functions.nShip_speedSettingsBase;
		if (nAllocNu <= model.functions.speedSetting95MCR_use[legNr])
			nAllocNu = model.functions.speedSetting95MCR_use[legNr] + 1;

		model.functions.speedChannel[i].rpm = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannel[i].rpmSetting_gerCalmWaterSpeed = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannel[i].rpmSetting_gerFuelConsumption_main = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannel[i].rpmSetting_gerFuelConsumption_aux = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannel[i].settingGerBaseSetting = (int*)malloc2(nAllocNu * sizeof(int));
		model.functions.speedChannel[i].nShip_speedSettings = nShip_speedSettings[legNr];

		model.functions.speedChannelOut[i].rpm = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannelOut[i].rpmSetting_gerCalmWaterSpeed = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannelOut[i].rpmSetting_gerFuelConsumption_main = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannelOut[i].rpmSetting_gerFuelConsumption_aux = (double*)malloc2(nAllocNu * sizeof(double));
		model.functions.speedChannelOut[i].settingGerBaseSetting = (int*)malloc2(nAllocNu * sizeof(int));
		model.functions.speedChannelOut[i].nShip_speedSettings = nShip_speedSettings[legNr];
	}

	double consumption, speed, kvotTraff;

	for (legNr = 0; legNr < model.params.nLegs; legNr++) {
		if (model.params.legCommercial[legNr].commercialAllowedVariation < 0 && model.results.forecastTypeOrig <= 1000) {
			errlog("leg %d Not commercial opt, using %d speed settings of %d", legNr,
				nShip_speedSettings[legNr], model.functions.nShip_speedSettingsBase);

			//model.functions.nShip_speedSettingsDelay[legNr] = model.functions.nShip_speedSettingsBase;
			i = 0;

			//addBastSpeed_arcDelayed(int thisLevel, int pos1, int nextLevel, int pos2, int tidInt, int nSpeedSettings, double fuelQualityKvot, double extraAreaCostKvot) {
			//	minSpeed = model.functions.rpmSetting_gerCalmWaterSpeedBase[i];
			kvotTraff = 1.0 / (double)model.params.nSpeedSettingDivideIter1;
			nextTraff = getNextTraff_speedPos(0, minPos, maxPos, minSpeed, speedInterval, kvotTraff);
			//nextTraff = roundUp(model.functions.nShip_speedSettingsBase / (double)model.params.nSpeedSettingDivideIter1);
			//if (nextTraff == maxPos)
			//	nextTraff--;

			if (model.params.useSimulering == 1) {
				target = model.params.simulationSpeed_kmh;
				getBastSpeedPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
			}
			else
				indexUnder = -1;

			for (iUse = 0; iUse < model.functions.nShip_speedSettingsBase; iUse++) {
				set_speedSettingsFromBase(NULL, iUse, iUse, -1, 0., legNr);
				model.functions.nSpeedSettingsDelay[legNr] = iUse + 1;
				if (iUse == minPos || iUse == maxPos || iUse == nextTraff || iUse == indexUnder) {
					if (iUse == nextTraff) {
						kvotTraff += 1.0 / (double)model.params.nSpeedSettingDivideIter1;
						nextTraff = getNextTraff_speedPos(nextTraff, minPos, maxPos, minSpeed, speedInterval, kvotTraff);
						//nextTraff += roundUp(model.functions.nShip_speedSettingsBase / (double)model.params.nSpeedSettingDivideIter1);
					}
					for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
						if (model.network.physicalLev[i1].legNr != legNr)
							continue;
						if (iUse == indexUnder && kvot >= 0)
							set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, indexUnder, indexOver, kvot);
						else
							set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, iUse);
						if (i1 == 0)
							errlog(" %d %.2lf", i, model.functions.speedLevel[i1].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
					}
					for (i1 = 0; i1 < model.network.nChannels; i1++) {
						if (model.network.channel[i1].legNr != legNr)
							continue;
						if (model.network.channel[i1].timeThroughChannel > 0) {
							if (i == 0) {
								consumption = model.network.channel[i1].totalConsumption / model.network.channel[i1].timeThroughChannel;
								speed = model.network.channel[i1].distance_km / model.network.channel[i1].timeThroughChannel;
								set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, iUse);
								model.functions.speedChannel[i1].nShip_speedSettings = 1;
								model.functions.speedChannel[i1].rpmSetting_gerCalmWaterSpeed[i] = speed;
								model.functions.speedChannel[i1].rpmSetting_gerFuelConsumption_main[i] = consumption;
							}
						}
						if (iUse == indexUnder && kvot >= 0) {
							if (model.network.channel[i1].timeThroughChannel <= 0)
								set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, indexUnder, indexOver, kvot);
							set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), i, indexUnder, indexOver, kvot);
						}
						else {
							if (model.network.channel[i1].timeThroughChannel <= 0)
								set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, iUse);
							set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), i, iUse);
							if (iUse == maxPos) {
								if (model.network.channel[i1].timeThroughChannel <= 0)
									set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), iUse, iUse);
								set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), iUse, iUse);
							}
						}
					}
					i++;
				}
			}
			errlog(" knots\n");
			model.functions.speedSetting95MCR_base = maxPos;

			if (i < nShip_speedSettings[legNr]) {
				nShip_speedSettings[legNr] = i;
				for (i = 0; i < model.network.nPhysicalLevels; i++) {
					if (model.network.physicalLev[i].legNr != legNr)
						continue;
					model.functions.speedLevel[i].nShip_speedSettings = nShip_speedSettings[legNr];
				}
				for (i = 0; i < model.network.nChannels; i++) {
					if (model.network.channel[i].legNr != legNr)
						continue;
					if (model.functions.speedChannel[i].nShip_speedSettings > nShip_speedSettings[legNr])
						model.functions.speedChannel[i].nShip_speedSettings = nShip_speedSettings[legNr];
					if (model.functions.speedChannelOut[i].nShip_speedSettings > nShip_speedSettings[legNr])
						model.functions.speedChannelOut[i].nShip_speedSettings = nShip_speedSettings[legNr];
				}
			}

		}
		else {
			// commercial version or fix speed forecast
			// model.functions.nShip_speedSettingsDelay[legNr] = nShip_speedSettings[legNr];

			usedLevel = -1;
			usedLevelChannel = -1;
			if (model.results.forecastTypeOrig > 1000) {
				// fix speed forecast
				iUse = (int)(model.results.forecastTypeOrig / 1000) - 1;
				if (iUse >= model.functions.nShip_speedSettingsBase)
					iUse = model.functions.nShip_speedSettingsBase - 1;
				i = 0;
				set_speedSettingsFromBase(NULL, i, iUse, -1, 0., legNr);
				model.functions.nSpeedSettingsDelay[legNr] = i + 1;
				for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
					if (model.network.physicalLev[i1].legNr != legNr)
						continue;
					set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, iUse);
					usedLevel = i1;
				}
				for (i1 = 0; i1 < model.network.nChannels; i1++) {
					if (model.network.channel[i1].legNr != legNr)
						continue;
					if (model.network.channel[i1].timeThroughChannel > 0) {
						consumption = model.network.channel[i1].totalConsumption / model.network.channel[i1].timeThroughChannel;
						speed = model.network.channel[i1].distance_km / model.network.channel[i1].timeThroughChannel;
						set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, iUse);
						model.functions.speedChannel[i1].nShip_speedSettings = 1;
						model.functions.speedChannel[i1].rpmSetting_gerCalmWaterSpeed[i] = speed;
						model.functions.speedChannel[i1].rpmSetting_gerFuelConsumption_main[i] = consumption;
					}
					else {
						set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, iUse);
					}
					usedLevelChannel = i1;
					set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), i, iUse);
				}
				i++;

				iUse = model.functions.speedSetting95MCR_use[legNr];
				for (i1 = 0; i1 < model.network.nChannels; i1++) {
					if (model.network.channel[i1].legNr != legNr)
						continue;
					set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), iUse, model.functions.speedSetting95MCR_base);
					set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), iUse, model.functions.speedSetting95MCR_base);
				}

				if (usedLevel >= 0) {
					errlog("OBS! leg %d fix speed, using %d speed settings with allowedVariation %.2lf Speed %.2lf fuel %.2lf, with data interpolated from the speed/fuel table by Trung, calmwaterspeeds",
						legNr, model.functions.speedLevel[usedLevel].nShip_speedSettings, model.params.legCommercial[legNr].commercialAllowedVariation, 
						model.params.legCommercial[legNr].commercialSpeed, model.params.legCommercial[legNr].commercialFuel);
					for (i = 0; i < model.functions.speedLevel[usedLevel].nShip_speedSettings; i++)
						errlog(" %d %.2lf", i, model.functions.speedLevel[usedLevel].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
				}
				else {
					if (usedLevelChannel >= 0) {
						errlog("OBS! leg %d fix speed (only channel), using %d speed settings with allowedVariation %.2lf Speed %.2lf fuel %.2lf, with data interpolated from the speed/fuel table by Trung, calmwaterspeeds",
							legNr, model.functions.speedChannel[usedLevelChannel].nShip_speedSettings, model.params.legCommercial[legNr].commercialAllowedVariation,
							model.params.legCommercial[legNr].commercialSpeed, model.params.legCommercial[legNr].commercialFuel);
						for (i = 0; i < model.functions.speedChannel[usedLevelChannel].nShip_speedSettings; i++)
							errlog(" %d %.2lf", i, model.functions.speedChannel[usedLevelChannel].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
					}
					else
						postRequest("ERROR! leg " + std::to_string(legNr) + " has no used speed level. I quit", 1);
				}

			}
			else {
				// commercial version

				for (i = 0; i < nShip_speedSettings[legNr]; i++) {
					if (i == 0)
						delta = 1 - model.params.legCommercial[legNr].commercialAllowedVariation / 100.0;
					else if (i == 1)
						delta = 1 - model.params.legCommercial[legNr].commercialAllowedVariation / 100.0 / 2.0;
					else if (i == 2)
						delta = 1;
					else if (i == 3)
						delta = 1 + model.params.legCommercial[legNr].commercialAllowedVariation / 100.0 / 2.0;
					else
						delta = 1 + model.params.legCommercial[legNr].commercialAllowedVariation / 100.0;
					if (model.params.legCommercial[legNr].commercialSpeed > 0) {
						target = model.params.legCommercial[legNr].commercialSpeed * model.params.knots_to_km * delta;
						getBastSpeedPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
					}
					else {
						target = model.params.legCommercial[legNr].commercialFuel * delta / 24.0;
						getBastConsumptionPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
					}

					if (kvot < 0) {
						set_speedSettingsFromBase(NULL, i, indexUnder, -1, 0, legNr);
						model.functions.nSpeedSettingsDelay[legNr] = i + 1;
						for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
							if (model.network.physicalLev[i1].legNr != legNr)
								continue;
							set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, indexUnder);
							// set_speedSettingsFromBase(NULL, i, indexUnder);
							usedLevel = i1;
						}
						for (i1 = 0; i1 < model.network.nChannels; i1++) {
							if (model.network.channel[i1].legNr != legNr)
								continue;
							if (model.network.channel[i1].timeThroughChannel > 0) {
								if (i == 0) {
									consumption = model.network.channel[i1].totalConsumption / model.network.channel[i1].timeThroughChannel;
									speed = model.network.channel[i1].distance_km / model.network.channel[i1].timeThroughChannel;
									set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, i);
									model.functions.speedChannel[i1].nShip_speedSettings = 1;
									model.functions.speedChannel[i1].rpmSetting_gerCalmWaterSpeed[i] = speed;
									model.functions.speedChannel[i1].rpmSetting_gerFuelConsumption_main[i] = consumption;
								}
							}
							else {
								set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, indexUnder);
							}
							set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), i, indexUnder);
							usedLevelChannel = i1;
						}
					}
					else {
						set_speedSettingsFromBase(NULL, i, indexUnder, indexOver, kvot, legNr);
						model.functions.nSpeedSettingsDelay[legNr] = i + 1;
						for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
							if (model.network.physicalLev[i1].legNr != legNr)
								continue;
							set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, indexUnder, indexOver, kvot);
							usedLevel = i1;
						}
						for (i1 = 0; i1 < model.network.nChannels; i1++) {
							if (model.network.channel[i1].legNr != legNr)
								continue;
							if (model.network.channel[i1].timeThroughChannel > 0) {
								if (i == 0) {
									consumption = model.network.channel[i1].totalConsumption / model.network.channel[i1].timeThroughChannel;
									speed = model.network.channel[i1].distance_km / model.network.channel[i1].timeThroughChannel;
									set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, i);
									model.functions.speedChannel[i1].nShip_speedSettings = 1;
									model.functions.speedChannel[i1].rpmSetting_gerCalmWaterSpeed[i] = speed;
									model.functions.speedChannel[i1].rpmSetting_gerFuelConsumption_main[i] = consumption;
								}
							}
							else {
								set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), i, indexUnder, indexOver, kvot);
							}
							set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), i, indexUnder, indexOver, kvot);
							usedLevelChannel = i1;
						}
					}
				}

				iUse = model.functions.speedSetting95MCR_use[legNr];
				for (i1 = 0; i1 < model.network.nChannels; i1++) {
					if (model.network.channel[i1].legNr != legNr)
						continue;
					set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), iUse, model.functions.speedSetting95MCR_base);
					set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), iUse, model.functions.speedSetting95MCR_base);
					usedLevelChannel = i1;
				}

				if(usedLevel >= 0){
					errlog("OBS! leg %d commercial opt, using %d speed settings with allowedVariation %.2lf Speed %.2lf fuel %.2lf, with data interpolated from the speed/fuel table by Trung, calmwaterspeeds",
						legNr, model.functions.speedLevel[usedLevel].nShip_speedSettings, model.params.legCommercial[legNr].commercialAllowedVariation, 
						model.params.legCommercial[legNr].commercialSpeed, model.params.legCommercial[legNr].commercialFuel);
					for (i = 0; i < model.functions.speedLevel[usedLevel].nShip_speedSettings; i++)
						errlog(" %d %.2lf", i, model.functions.speedLevel[usedLevel].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
				}
				else {
					if (usedLevelChannel >= 0) {
						errlog("OBS! leg %d commercial opt (only channel), using %d speed settings with allowedVariation %.2lf Speed %.2lf fuel %.2lf, with data interpolated from the speed/fuel table by Trung, calmwaterspeeds",
							legNr, model.functions.speedChannel[usedLevelChannel].nShip_speedSettings, model.params.legCommercial[legNr].commercialAllowedVariation,
							model.params.legCommercial[legNr].commercialSpeed, model.params.legCommercial[legNr].commercialFuel);
						for (i = 0; i < model.functions.speedChannel[usedLevelChannel].nShip_speedSettings; i++)
							errlog(" %d %.2lf", i, model.functions.speedChannel[usedLevelChannel].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
					}
					else
						postRequest("ERROR! legB " + std::to_string(legNr) + " has no used speed level. I quit", 1);
				}
			}
			errlog(" knots, fuel consumption main/aux ");
			if (usedLevel >= 0) {
				for (i = 0; i < model.functions.speedLevel[usedLevel].nShip_speedSettings; i++)
					errlog(" %d %.2lf %.2lf", i, model.functions.speedLevel[usedLevel].rpmSetting_gerFuelConsumption_main[i] * 24.0,
						model.functions.speedLevel[usedLevel].rpmSetting_gerFuelConsumption_aux[i] * 24.0);
			}
			else {
				if (usedLevelChannel >= 0) {
					for (i = 0; i < model.functions.speedChannel[usedLevelChannel].nShip_speedSettings; i++)
					errlog(" %d %.2lf %.2lf", i, model.functions.speedChannel[usedLevelChannel].rpmSetting_gerFuelConsumption_main[i] * 24.0,
						model.functions.speedChannel[usedLevelChannel].rpmSetting_gerFuelConsumption_aux[i] * 24.0);
				}
				else
					postRequest("ERROR! legC " + std::to_string(legNr) + " has no used speed level. I quit", 1);
			}
			errlog(" mpd\n");
		}
	}



	double averSpeed = 0;
	minSpeed = 1e10;
	maxSpeed = 0;
	for (i = 0; i < model.functions.speedLevel[0].nShip_speedSettings; i++) {
		averSpeed += model.functions.speedLevel[0].rpmSetting_gerCalmWaterSpeed[i];
		if (minSpeed > model.functions.speedLevel[0].rpmSetting_gerCalmWaterSpeed[i])
			minSpeed = model.functions.speedLevel[0].rpmSetting_gerCalmWaterSpeed[i];
		if (maxSpeed < model.functions.speedLevel[0].rpmSetting_gerCalmWaterSpeed[i])
			maxSpeed = model.functions.speedLevel[0].rpmSetting_gerCalmWaterSpeed[i];
	}

	if (model.params.calmWaterSpeedCompare > 0) {
		if (minSpeed > model.params.calmWaterSpeedCompare) {
			errlog("ERROR! calmWaterSpeedCompared is %.3lf but minSpeed given in shipSpeedSettings is %.3lf, I use the later one\n",
				model.params.calmWaterSpeedCompare / model.params.knots_to_km, minSpeed / model.params.knots_to_km);
			model.params.calmWaterSpeedCompare = minSpeed;
		}
		if (maxSpeed < model.params.calmWaterSpeedCompare) {
			errlog("ERROR! calmWaterSpeedCompared is %.3lf but maxSpeed given in shipSpeedSettings is %.3lf, I use the later one\n",
				model.params.calmWaterSpeedCompare / model.params.knots_to_km, maxSpeed / model.params.knots_to_km);
			model.params.calmWaterSpeedCompare = maxSpeed;
		}
		model.params.preferredSpeed_calmWater = model.params.calmWaterSpeedCompare;
		model.params.calmWaterSpeedCompareUse = model.params.calmWaterSpeedCompare;
		errlog("OBS! Setting preferred speed to calmWaterSpeedCompared: %.2lf knots (it is modified if eta is given)\n",
			model.params.preferredSpeed_calmWater / model.params.knots_to_km);
	}
	else {
		model.params.preferredSpeed_calmWater = averSpeed / model.functions.speedLevel[0].nShip_speedSettings;
		errlog("OBS! Setting preferred speed to average of all speed settings right now: %.2lf knots (it is modified if eta is given)\n",
			model.params.preferredSpeed_calmWater / model.params.knots_to_km);
		model.params.calmWaterSpeedCompareUse = model.params.preferredSpeed_calmWater;
	}
	model.params.calmWaterSpeedMin = minSpeed;
	model.params.calmWaterSpeedMax = maxSpeed;
	//errlog("ERROR! ERROR! Modify so the rpeferredSpeed_calmWater uses the objective when calculated, ie min time -> high speed, min emission -> most economical speed aso\n");


}


int readAddParameterInfoForTable(json data, strTableParam* param, std::string namn) {
	int nError = 0;

	if (!data[namn].is_null()) {
		json dataNu = data[namn];
		if (!dataNu["minValue"].is_null()) {
			param->minValue = dataNu["minValue"];
		}
		else {
			errlog("ERROR! minValue is missing for parameter %s\n", namn.c_str());
			nError = 1;
		}
		if (!dataNu["maxValue"].is_null()) {
			param->maxValue = dataNu["maxValue"];
		}
		else {
			errlog("ERROR! maxValue is missing for parameter %s\n", namn.c_str());
			nError = 1;
		}
		if (!dataNu["intervalSize"].is_null()) {
			param->intervalSize = dataNu["intervalSize"];
		}
		else {
			errlog("ERROR! minValue is missing for parameter %s\n", namn.c_str());
			nError = 1;
		}
	}
	else {
		errlog("ERROR! parameter %s is missing for table\n", namn.c_str());
		nError = 1;
	}

	if (nError == 0) {
		param->inv_intervalSize = 1 / param->intervalSize;
		param->nIndex = (int)((param->maxValue - param->minValue) * param->inv_intervalSize) + 1;
		//printf("nIndex %d float %.3lf\n", param->nIndex, ((param->maxValue - param->minValue) * param->inv_intervalSize) + 1);
		if(param->nIndex < ((param->maxValue - param->minValue) * param->inv_intervalSize) + 0.001)
			(param->nIndex)++;
	}

	return nError;
}


static int callbackDB2(void* data, int argc, char** argv, char** azColName) {
	int i;
	fprintf(stderr, "%s: ", (const char*)data);

	//for (i = 0; i < argc; i++) {
	//	printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	//}

	//printf("\n");
	return 0;
}


int loadMapsSQLite() {
	sqlite3* db;
	char* zErrMsg = 0;
	int rc;
	sqlite3_stmt* query;
	const char* data = "Callback function called";
	FILE* filpek;
	//filpek = fopen("tmpTable.txt", "w");
	int retval, pos;
	int count;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/shipTables/all_tables.db", model.params.indataPath.c_str());	rc = sqlite3_open(namn, &db);
	if (rc) {
		postRequest("ERROR! Could not open the database " + std::string(namn) + ". Is it possibly locked by another application. Close it and run the redis update again", 1);
	}
	else {
		//fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	std::string sql = "SELECT * from maps;";
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &query, NULL) != SQLITE_OK) {
		printf("error executing query: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	model.sqliteMap = (strSQLiteMap*)malloc2(2 * sizeof(strSQLiteMap));
	count = 0;
	int type;
	while (1) {
		retval = sqlite3_step(query);

		if (retval == SQLITE_ROW) {
			type = (uint32_t)sqlite3_column_int(query, 0);

			if (type >= 2) {
				errlog("ERROR! Too many maps in db table maps, type %d.\n", type);
				postRequest("ERROR! Too many maps in db table maps, type " + std::to_string(type), 1);
			}
			model.sqliteMap[type].textFileName = str_alloc_cpy((char*)sqlite3_column_text(query, 1));
			model.sqliteMap[type].epochCount = (double)sqlite3_column_double(query, 2);
			model.sqliteMap[type].nCols = (double)sqlite3_column_int(query, 3);
			model.sqliteMap[type].nRows = (double)sqlite3_column_int(query, 4);
			model.sqliteMap[type].size_col = (double)sqlite3_column_double(query, 5);
			model.sqliteMap[type].size_row = (double)sqlite3_column_double(query, 6);
			model.sqliteMap[type].minX = (double)sqlite3_column_double(query, 7);
			model.sqliteMap[type].maxX = (double)sqlite3_column_double(query, 8);
			model.sqliteMap[type].minY = (double)sqlite3_column_double(query, 9);
			model.sqliteMap[type].maxY = (double)sqlite3_column_double(query, 10);
			model.sqliteMap[type].nBlockRows = (double)sqlite3_column_int(query, 11);
			model.sqliteMap[type].nBlockCols = (double)sqlite3_column_int(query, 12);
		}
		else if (retval == SQLITE_DONE) {
			/* all done */
			//printf("search maps: row processing done, %u rows processed\n", count);
			break;
		}
		else {
			/* error of some sort */
			printf("Error search: error during row processing: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(query);
			errlog("Error search: error during row processing: %s\n", sqlite3_errmsg(db));
			postRequest("Error search: error during row processing: " + std::string(sqlite3_errmsg(db)), 1);
			return 0;
		}
		count++;
	}
	//fclose(filpek);

	sqlite3_finalize(query);
	sqlite3_close(db);

	if (count != 2) {
		postRequest("ERROR! Wrong number of maps in table maps, must be 2 but is " + std::to_string(count), 1);
	}
	return 0;
}



int loadUsedTables() { // not used

	loadWeatherFactorTableWind(model.functions.windTableNr);
	loadWeatherFactorTableWave(model.functions.waveTableNr);
	loadDynamicStabilityTable(model.functions.stabilityTableNr);
	return 0;
}

int loadpreferredPathGeojson()
{
	int i;

	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.params.preferredPath.c_str());
	//printf("opens %s\n", namn);
	fil.open(namn);
	
	json data, dataGeo, dataCoord;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
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
		model.preferredPath.point = (spherical::Point*)malloc2(model.preferredPath.nPoints * sizeof(spherical::Point));
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

char* str_alloc_cpy_hindCastWeatherData(std::string data) //const char* data)
{
	char* dataAdd;

	size_t firstindex = data.find_last_of("/");
	size_t lastindex2 = data.find("_resampled");
	size_t lastindex = data.find_last_of(".");
	if (lastindex2 < lastindex)
		lastindex = lastindex2;
	std::string rawname = data.substr(firstindex + 1, lastindex - firstindex - 1);

	int langd = rawname.length() + 1;
	dataAdd = (char*)malloc(langd * sizeof(char));
	if (dataAdd == NULL) {
		fprintf(stdout, "out of memory at line %d\n", __LINE__);
	}
	strcpy(dataAdd, rawname.c_str());
	return dataAdd;
}



int loadVariables(int alt)
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
		postRequest("ERROR! Could not open the file " + model.params.indataPath + "//weather_parameters.json with information about the weather parameters. I quit.\n", 1);
	}
	fil >> data;

	if (!data["delayed_stormFileName"].is_null()) {
		namn = data["delayed_stormFileName"];
		model.delay.delayed_stormFileName = str_alloc_cpy(namn.c_str());
	}
	else
		model.delay.delayed_stormFileName = NULL;

	//if (!data["delayed_monthNr"].is_null()) {
	//	model.delay.delayed_monthNr = data["delayed_monthNr"];
	//}
	//else
	//	model.delay.delayed_monthNr = -1;
	if (!data["simulateTimeVisually"].is_null())
		model.params.simuleraTidVisuellt = data["simulateTimeVisually"];
	else
		model.params.simuleraTidVisuellt = 0;
	if (!data["simulateTimeVisually_nIntHour"].is_null())
		model.params.simulateTimeVisually_nIntHour = data["simulateTimeVisually_nIntHour"];
	else
		model.params.simulateTimeVisually_nIntHour = 6;


	if (!data["timeIntervall_h"].is_null()) {
		model.weather_timeIntervall_h = data["timeIntervall_h"];
		model.weather_inv_timeIntervall_h = 1 / model.weather_timeIntervall_h;
	}
	else {
		errlog("ERROR! timeIntervall_h not given in file %s. Fix this and run again. I quit\n", namn.c_str());
		postRequest("ERROR! timeIntervall_h not given in file " + namn + ". Fix this and run again.I quit", 1);
	}

	model.nWeatherFiles = 0;
	dataVar = data["weather_parameters"];
	char* namnAll = (char*)malloc2(256 * sizeof(char));

	model.functions.pos_wind_u = -1;
	model.functions.pos_wind_v = -1;
	model.functions.pos_current_u = -1;
	model.functions.pos_current_v = -1;
	model.functions.pos_waveHeight = -1;
	model.functions.pos_wavePeriod = -1;
	model.functions.pos_waveDirection = -1;
	model.functions.pos_iceThickness = -1;

	model.functions.pos_pressureSurface = -1;
	model.functions.pos_pressureAir = -1;
	model.functions.pos_precipitation = -1; 
	model.functions.pos_tempSea = -1;
	model.functions.pos_tempAir = -1;
	model.functions.pos_cloudCover = -1;
	model.functions.pos_mdps = -1;
	model.functions.pos_swell = -1;

	int nAllocValues = dataVar.size();
	model.weather = (strWeather*)malloc2((int)nAllocValues * sizeof(strWeather));
	model.params.weather_is_current = (int*)calloc((int)nAllocValues, sizeof(int));
	//printf("\n####\nalloc %d weatherData\n", (int)dataVar.size());
	i0 = 0;
	for (auto it = dataVar.begin(); it != dataVar.end(); ++it) {
		dataIt = it.value();
		namn = dataIt["variableID"];
		model.weather[i0].weatherFileTypeName = str_alloc_cpy(namn.c_str());

		model.weather[i0].defaultValueMore = 1;
		if (strcmp(model.weather[i0].weatherFileTypeName, "wind_uComponent") == 0)
			model.functions.pos_wind_u = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "wind_vComponent") == 0)
			model.functions.pos_wind_v = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "current_uComponent") == 0) {
			model.functions.pos_current_u = i0;
			model.params.weather_is_current[i0] = 1;
		}
		if (strcmp(model.weather[i0].weatherFileTypeName, "current_vComponent") == 0) {
			model.functions.pos_current_v = i0;
			model.params.weather_is_current[i0] = 1;
		}
		if (strcmp(model.weather[i0].weatherFileTypeName, "waveHeight") == 0)
			model.functions.pos_waveHeight = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "wavePeriod") == 0)
			model.functions.pos_wavePeriod = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "waveDirection") == 0)
			model.functions.pos_waveDirection = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "ice thickness(m)") == 0)
			model.functions.pos_iceThickness = i0;

		if (strcmp(model.weather[i0].weatherFileTypeName, "pressureSurface_prmsl") == 0) {
			model.functions.pos_pressureSurface = i0;
			model.weather[i0].defaultValueMore = 0;
		}
		if (strcmp(model.weather[i0].weatherFileTypeName, "pressureAir_gh") == 0)
			model.functions.pos_pressureAir = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "precipitation_tp") == 0)
			model.functions.pos_precipitation = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "temperatureSea_wtmp") == 0)
			model.functions.pos_tempSea = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "temperatureAir_tp") == 0)
			model.functions.pos_tempAir = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "cloudCover_tcc") == 0)
			model.functions.pos_cloudCover = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "mdps") == 0)
			model.functions.pos_mdps = i0;
		if (strcmp(model.weather[i0].weatherFileTypeName, "swell") == 0)
			model.functions.pos_swell = i0;
		//printf("weatherFile %d %s\n", i0, model.weather[i0].weatherFileTypeName);

		model.weather[i0].nBlock_x = dataIt["nBlock_x"];
		model.weather[i0].nBlock_y = dataIt["nBlock_y"];
		if(!dataIt["timeIntervall_expected_h"].is_null())
			model.weather[i0].timeIntervall_expected = dataIt["timeIntervall_expected_h"];
		else
			model.weather[i0].timeIntervall_expected = -1;

		model.weather[i0].nBlock_y = dataIt["nBlock_y"];
		dataFiles = dataIt["files"];
		model.weather[i0].nFiles = (int)dataFiles.size();
		model.weather[i0].filePos = (strFileWeather*)malloc2(model.weather[i0].nFiles * sizeof(strFileWeather));
		model.weather[i0].rasterPos = (Raster*)malloc2(model.weather[i0].nFiles * sizeof(Raster));
		model.weather[i0].valueCell = NULL;
		i1 = 0;
		for (auto it2 = dataFiles.begin(); it2 != dataFiles.end(); ++it2) {
			dataIt2 = it2.value();
			namn = dataIt2["fileName"];
			if (model.params.hindCast == 0 && runAltForecast != -2) {
				//if (model.params.weatherDirectory != NULL) {
				//	sprintf(namnAll, "%s/%s", model.params.weatherDirectory, namn.c_str());
				//}
				//else {
				//	sprintf(namnAll, "%s/%s", model.params.indataPath.c_str(), namn.c_str());
				//}
				if (model.params.onboard_currentStatic == 0 || model.params.weather_is_current[i0] != 1) {
					if (runAltForecast >= 1) {
						model.weather[i0].filePos[i1].fileName = str_alloc_cpy_hindCastWeatherData(namn);
					}
					else {
						sprintf(namnAll, "%s%s", model.params.weatherPath.c_str(), namn.c_str());
						model.weather[i0].filePos[i1].fileName = str_alloc_cpy(namnAll);
					}
				}
				else {
					model.weather[i0].filePos[i1].fileName = str_alloc_cpy_hindCastWeatherData(namn);
				}
			}
			else {
				model.weather[i0].filePos[i1].fileName = str_alloc_cpy_hindCastWeatherData(namn);
				//sprintf(namnAll, "%s%s", model.params.weatherPath.c_str(), str_alloc_cpy_hindCastWeatherData(namn));
				//model.weather[i0].filePos[i1].fileName = str_alloc_cpy(namnAll);
			}
			//printf("-- weatheFilePath %s\n", model.weather[i0].filePos[i1].fileName);
			//model.weather[i0].filePos[i1].fileNameOnly = splitFilename(namn, 1);

			//model.weather[i0].filePos[i1].minX = dataIt2["minLon"];
			//model.weather[i0].filePos[i1].maxX = dataIt2["maxLon"];
			i1++;
		}
		(model.nWeatherFiles)++;
		i0++;
	}

	free(namnAll);
	model.inv_nWeatherFiles = 1.0 / model.nWeatherFiles;

	model.functions.varValue = (double*)malloc2(model.nWeatherFiles * sizeof(double));
	model.functions.varValueAverage = (double*)malloc2(model.nWeatherFiles * sizeof(double));

	if (model.functions.pos_wind_u == -1) {
		errlog("ERROR! weather parameter wind_uComponent not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter wind_uComponent not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_wind_v == -1) {
		errlog("ERROR! weather parameter wind_vComponent not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter wind_vComponent not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_current_u == -1) {
		errlog("ERROR! weather parameter current_uComponent not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter current_uComponent not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_current_v == -1){// }&& model.params.onboard != 2) { // not needed for onboard pfg 20250209
		errlog("ERROR! weather parameter current_vComponent not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter current_vComponent not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_waveHeight == -1) {
		errlog("ERROR! weather parameter waveHeight not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter waveHeight not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_wavePeriod == -1) {
		errlog("ERROR! weather parameter wavePeriod not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter wavePeriod not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_waveDirection == -1) {
		errlog("ERROR! weather parameter waveDirection not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter waveDirection not given in weather_parameters.json. It must exist", 1);
	}
	if (model.functions.pos_iceThickness == -1) {
		errlog("ERROR! weather parameter ice_thickness_m not given in weather_parameters.json. It must exist\n");
		postRequest("ERROR! weather parameter ice_thickness_m not given in weather_parameters.json. It must exist", 1);
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

int roundDown(double varde) {
	int heltal = (int)varde;
	if (heltal > varde)
		heltal--;
	return heltal;
}


int roundUp(double varde) {
	int heltal = (int)varde;
	if (heltal < varde)
		heltal++;
	return heltal;
}

int roundUpApprox(double varde) {
	int heltal = (int)varde;
	if (heltal < varde - 0.01)
		heltal++;
	return heltal;
}

double get_colDblFromWeatherFile(int weatherNr, double lon)
{
	double colDbl, tmpLon = lon;
	strWeather gridData;
	if (weatherNr >= 0)
		gridData = model.weather[weatherNr];
	else {
		if (weatherNr == -1)
			gridData = model.delayedGrid[0];
		else
			gridData = model.delayedCurrent[-weatherNr - 2][0];
	}

	if (lon < gridData.minX) {
		if (gridData.maxX - gridData.minX < 355) {
			if (lon < gridData.minX - 20)
				lon += 360;
		}
		else
			lon += 360;
	}
	else {
		if (lon > gridData.maxX) {
			if (gridData.maxX - gridData.minX < 355) {
				if (lon > gridData.maxX + 20)
					lon -= 360;
			}
			else
				lon -= 360;
		}
	}

	colDbl = (lon - gridData.minX) / gridData.size_col;
	if (colDbl < 0)
		colDbl += gridData.nCols;
	if (colDbl < 0 || colDbl >= gridData.nCols) {
		if (colDbl < -0.1 || colDbl > gridData.nCols + 0.1) {
			errlog("ERROR! This should not happen. Fix it!! colDbl %.3lf nCols %d weatherNr %d lon %.3lf minLon %.3lf minX %.3lf maxLon %.3lf tmpLon %.3lf\n",
				colDbl, gridData.nCols, weatherNr, lon,
				gridData.minX, gridData.minX,
				gridData.maxX, tmpLon);
			exit(0);
		}
		else {
			if (colDbl < 0)
				colDbl = 0;
			else
				colDbl = gridData.nCols - 0.1;
		}
	}


	return colDbl;
}


//void test4(Raster rasterMapA) {
//	Raster::strPhysRaster physRaster;
//	physRaster.valueCell = rasterMapA.GetRasterBand_intArr(1, &(physRaster), model.boundingBox);
//}

/*
void test3(Raster rasterMapA) {
	Raster::strPhysRaster physRaster;
	model.boundingBox.xMin = -180.0;
	model.boundingBox.yMin = -85.051;
	model.boundingBox.xMax = 180.0;
	model.boundingBox.yMax = 85.051;

	physRaster.valueCell = rasterMapA.GetRasterBand_intArr(1, &(physRaster), model.boundingBox);

}

void test2() {
	Raster rasterMapA;
	Raster::strPhysRaster physRaster;
	rasterMapA.open("data/Raster_A-Global-v2.tif");

	model.boundingBox.xMin = -180.0;
	model.boundingBox.yMin = -85.051;
	model.boundingBox.xMax = 180.0;
	model.boundingBox.yMax = 85.051;

	physRaster.valueCell = rasterMapA.GetRasterBand_intArr(1, &(physRaster), model.boundingBox);

}
void testAnropRedisMap() {
	printf("opening redis\n");
	auto redis = Redis("tcp://127.0.0.1:6379/1");
	std::string redisTest;
	try {
		redisTest = redis.ping();
		if (redisTest != "PONG") {
			errlog("ERROR! Redis is not running on the server. Start it and try again\n");
			printf("ERROR! Redis is not running on the server. Start it and try again\n");
			exitKontrollerat(__LINE__);
		}
	}
	catch (...) {
		errlog("ERROR! Redis is not running on the server. Start it and try again\n");
		printf("ERROR! Redis is not running on the server. Start it and try again\n");
		exitKontrollerat(__LINE__);
	}

	Raster rasterMapA;
	Raster::strPhysRaster physRaster;
	rasterMapA.open("data/Raster_A-Global-v2.tif");

	model.boundingBox.xMin = -180.0;
	model.boundingBox.yMin = -85.051;
	model.boundingBox.xMax = 180.0;
	model.boundingBox.yMax = 85.051;

	physRaster.valueCell = rasterMapA.GetRasterBand_intArr(1, &(physRaster), model.boundingBox);


}
*/

int updateCorridors(std::string inputPath) {
	// download new corridors from api to file inputPath/tmp_corridors.json
	// reset_errlog();

	if (model.params.url_getCorridors != "") {
		int retVal = call_api_corridors(inputPath);
		printf("pfg efter call_api_corridors\n");
		if (retVal != 0)
			return -1;
	}
	else {
		printf("ERROR! No url given for getCorridors api\n");
		return -1;
	}

	model.params.indataPath = inputPath;
	model.params.resultPath = inputPath;

	// test the file tmp_corridors.json if there are corridors in it
	loadFileParams_feasibilityAuto(&(model.paramsAutoRoute));
	printf("pfg efter loadFileParams_feasibilityAuto\n");
	
	// printf("skipping loadFileParams_feasibilityAuto\n");
	loadSave_downloadedCorridors();
	printf("pfg efter loadSave_downloadedCorridors\n");

	/*
	if (model.nAutoCorridors < 2) {
		postRequest("ERROR! Updating corridors failed (call to api). I use the old corridors\n", 0);
		return -1;
	}
	char* filUt, * filIn;
	filUt = (char*)malloc(256 * sizeof(char));
	filIn = (char*)malloc(256 * sizeof(char));
	// if it is then save it to usable corridors for autoRoute
	sprintf(filUt, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.corridorsNameNew.c_str());
	sprintf(filIn, "%s/tmp_corridors.json", model.params.indataPath.c_str());
	write_copyAtoB(filUt, (char*)"-", filIn, (char*)"w");

	free(filUt);
	free(filIn);
	*/

	return 0;
}




int initGeoJsonFil2(FILE* filpek, const char* namn) {

	fprintf(filpek, "{\n");
	fprintf(filpek, "\"type\": \"FeatureCollection\",\n");
	fprintf(filpek, "\"crs\": { \"type\": \"name\", \"properties\": { \"name\": \"urn:ogc:def:crs:OGC:1.3:CRS84\" } },\n");
	fprintf(filpek, "\"features\": [\n");

	return 0;
}

int getNrFromDate(std::string datum, int startPos, int length) {
	int nr;
	std::string manad = datum.substr(startPos, length);
	nr = std::stoi(manad);
	return nr;
}

int setRadiusFromMaxWind(double maxWind, double* r1, double* r2, double* r3) {
	if (maxWind < 28)
		return -1;
	if (maxWind <= 33.5) {
		*r1 = 85;
	}
	else if (maxWind <= 40.5) {
		*r1 = 170;
	}
	else if (maxWind <= 47.5) {
		*r1 = 225;
	}
	else if (maxWind <= 52.5) {
		*r1 = 250;
	}
	else if (maxWind <= 57.5) {
		*r1 = 300;
	}
	else if (maxWind <= 63.5) {
		*r1 = 325;
	}
	else if (maxWind <= 74.5) {
		*r1 = 350;
	}
	else if (maxWind <= 85.5) {
		*r1 = 400;
	}
	else if (maxWind <= 101.5) {
		*r1 = 450;
	}
	else {
		*r1 = 500;
	}

	*r2 = *r1;
	*r3 = *r1; //backwards
	return 0;
}


std::string addDataToString(std::string namnStorm, double lat, double lon, int* nr, int stormNr, double maxWind, std::string datum, double radius1, double radius2, double radius3)
{
	std::string data = "{\"properties\": {\n";
	data += "\"LAT\": " + std::to_string(lat) + ",\n";
	data += "\"LON\": " + std::to_string(lon) + ",\n";
	data += "\"MAXWIND\": " + std::to_string(maxWind) + ",\n";
	data += "\"FLDATELBL\": \"" + datum + "\",\n";
	(*nr)++;
	data += "\"STORMNAME\":\"" + namnStorm + "\",\n";
	data += "\"stormID\":\"" + std::to_string(stormNr) + "\",\n";
	data += "\"WindMaxRadius\": " + std::to_string(radius1) + ",\n";
	data += "\"WindFrontRadius\": " + std::to_string(radius2) + ",\n";
	data += "\"WindBackRadius\": " + std::to_string(radius3) + "},\n";
	data += "\"geometry\": {\"type\": \"Point\",\n\"coordinates\": [" + std::to_string(lon);
	data += ", " + std::to_string(lat) + "]}}\n";

	return data;
}


bool is_number(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

int getNumberFromString(std::string namn, int* heltal) {
	int isInteger;

	if (is_number(namn)) {
		isInteger = 1;
		*heltal = stoi(namn);
	}
	else
		isInteger = 0;
	return isInteger;
}

int fixStormFiles(std::string inputPath) {

	std::ifstream fil;
	char* namn, * namnNu;
	namn = (char*)malloc(256 * sizeof(char));
	namnNu = (char*)malloc(256 * sizeof(char));
	json data, data2, data3;
	std::string datum, stringPrev, stringNu, nummerString;
	FILE* filPrev, * filNu, * filNext = NULL;
	//FILE* filPrev2, * filNu2, * filNext2 = NULL;
	int monthNu, taMedStormInfo;
	int nextMonth, i, manadNu, yearNu, antal, dagNu, stormNr = 0;
	char** monthText;
	std::string stormInfo, stormPrev, stormTmp;
	//std::string stormInfo2, stormPrev2, stormTmp2;
	double lat, lon, radius1, radius2, radius3, maxWind;
	int langd, year, month, isInteger, i1;

	int filTyp = 1; // 0 original, 1 works 2024/01/04, string for Wind

	// loop over all months with storms
	filPrev = NULL;
	filNu = NULL;
	//filPrev2 = NULL;
	//filNu2 = NULL;
	int firstStorm = 0, firstStormNext = 0, nRowsNu, nInfoToday = 0, nInfoPrev = 0, posNu;
	std::string namnStorm;
	int startYear = 2020, endYear, monthNext, yearNext;

	time_t rawtime;
	time(&rawtime);
	struct tm tmBas = *localtime(&rawtime);
	time_t test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
		endYear = 2030;
	}
	endYear = tmBas.tm_year + 1900;
	monthNu = tmBas.tm_mon + 1;

	int monthLastFew = monthNu - 4, yearLastFew = endYear;
	if (monthNu < 1) {
		monthNu += 12;
		yearLastFew--;
	}
	monthNu++;
	if (monthNu > 12) {
		endYear++;
		monthNu = 1;
	}


	monthNu = 13;
	for (year = startYear; year <= endYear; monthNu++) {
		if (monthNu > 12) {
			monthNu = 1;
			year++;
		}

		if (monthNu < 10)
			sprintf(namn, "%s%d-0%d.json", inputPath.c_str(), year, monthNu);
		else
			sprintf(namn, "%s%d-%d.json", inputPath.c_str(), year, monthNu);

		if (!check_file_exist(namn))
			continue; // no file to check, go to the next

		if (filNu == NULL) {
			if (monthNu < 10)
				sprintf(namnNu, "%sstorms_0%d_%d.json", inputPath.c_str(), monthNu, year);
			else
				sprintf(namnNu, "%sstorms_%d_%d.json", inputPath.c_str(), monthNu, year);

			if (check_file_exist(namnNu) && (year < yearLastFew || (year == yearLastFew && monthNu < monthLastFew))) {
				printf("skips %s as it already exists\n", namnNu);
				filNu = NULL;
			}
			else {
				filNu = fopen(namnNu, "w");
				fprintf(filNext, "[\n");
				printf("Saves %s\n", namnNu);
			}
		}

		monthNext = monthNu + 1;
		if (monthNext > 12) {
			yearNext = year + 1;
			monthNext = 1;
		}
		else
			yearNext = year;

		if (monthNext < 10)
			sprintf(namnNu, "%sstorms_0%d_%d.json", inputPath.c_str(), monthNext, yearNext);
		else
			sprintf(namnNu, "%sstorms_%d_%d.json", inputPath.c_str(), monthNext, yearNext);

		if (check_file_exist(namnNu) && (yearNext < yearLastFew || (yearNext == yearLastFew && monthNext < monthLastFew))) {
			printf("skips %s as it already exists\n", namnNu);
			filNext = NULL;
		}
		else {
			filNext = fopen(namnNu, "w");
			fprintf(filNext, "[\n");
			printf("Saves %s\n", namnNu);
		}

		if (filNext == NULL && filNu == NULL)
			continue; // don't need to open this file as it already exists

		fil.open(namn);
		fil >> data;

		for (auto it = data.begin(); it != data.end(); ++it) {
			json dataSpeed2 = it.value();
			nextMonth = 0;
			namnStorm = dataSpeed2["Name"];
			data2 = dataSpeed2["StormForecast"];
			stormPrev = "";
			stormInfo = "";
			//stormPrev2 = "";
			//stormInfo2 = "";
			nRowsNu = 0;
			for (auto it2 = data2.begin(); it2 != data2.end(); ++it2) {
				json dataSpeed3 = it2.value();
				datum = dataSpeed3["ForecastDate"];
				manadNu = getNrFromDate(datum, 5, 2);
				dagNu = getNrFromDate(datum, 8, 2);
				if (manadNu != monthNu) {
					nextMonth = 1;
				}

				if (dataSpeed3["Wind"].type() == json::value_t::string) {
					nummerString = dataSpeed3["Wind"];
					maxWind = std::stof(nummerString);
				}
				else
					maxWind = dataSpeed3["Wind"];
				json dataPos = dataSpeed3["Position"];

				if (filTyp == 0) {
					lat = dataPos["LatitudeNormalized"];
					lon = dataPos["LongitudeNormalized"];
				}
				else {
					if (dataPos["Latitude"].type() == json::value_t::string) {
						nummerString = dataPos["Latitude"];
						lat = std::stof(nummerString);
					}
					else
						lat = dataPos["Latitude"];
					if (dataPos["Longitude"].type() == json::value_t::string) {
						nummerString = dataPos["Longitude"];
						lon = std::stof(nummerString);
					}
					else
						lon = dataPos["Longitude"];
				}

				taMedStormInfo = setRadiusFromMaxWind(maxWind, &radius1, &radius2, &radius3);
				if (taMedStormInfo == -1)
					continue;
				if (abs(lon - 112.12) < 0.001)
					lon = lon;
				stormTmp = addDataToString(namnStorm, lat, lon, &posNu, stormNr, maxWind, datum, radius1, radius2, radius3);
				//stormTmp2 = addDataToString2(lat, lon, posNu - 1, stormNr, maxWind, datum, radius1, radius2, radius3);
				//if (nInfoToday > 0 && nRowsNu > 0)
				if (nRowsNu > 0) {
					stormInfo += ",\n";
					//stormInfo2 += ",\n";
				}
				stormInfo += stormTmp;
				//stormInfo2 += stormTmp2;
				nInfoToday++;
				if (dagNu < 10 && manadNu == monthNu && filPrev != NULL) {
					if (nInfoPrev > 0) {
						stormPrev += ",\n";
						//stormPrev2 += ",\n";
					}
					stormPrev += stormTmp;
					//stormPrev2 += stormTmp2;
					nInfoPrev++;
				}
				nRowsNu++;
			}
			if (stormPrev != "") {
				fprintf(filPrev, "%s", stormPrev.c_str());
				//fprintf(filPrev2, "%s", stormPrev2.c_str());
			}
			if (nRowsNu > 0) {
				if (firstStorm > 0 && filNu != NULL) {
					fprintf(filNu, ",\n");
					//fprintf(filNu2, ",\n");
				}
				firstStorm++;
				if (filNu != NULL)
					fprintf(filNu, "%s", stormInfo.c_str());
				//fprintf(filNu2, "%s", stormInfo2.c_str());
				if (nextMonth == 1 && filNext != NULL) {
					if (firstStormNext > 0) {
						fprintf(filNext, ",\n");
						//fprintf(filNext2, ",\n");
					}
					fprintf(filNext, "%s", stormInfo.c_str());
					//fprintf(filNext2, "%s", stormInfo2.c_str());
					firstStormNext++;
				}
			}
			stormNr++;
		}
		fil.close();
		if (filPrev != NULL) {
			fprintf(filPrev, "]\n");
			fclose(filPrev);
			//fprintf(filPrev2, "]}\n");
			//fclose(filPrev2);
		}
		filPrev = filNu;
		//filPrev2 = filNu2;
		nInfoPrev = nInfoToday;
		filNu = filNext;
		//filNu2 = filNext2;
		nInfoToday = firstStormNext;
		firstStorm = firstStormNext;
		firstStormNext = 0;
	}
	if (filNu != NULL) {
		fprintf(filNu, "]\n");
		fclose(filNu);
	}
	if (filPrev != NULL) {
		fprintf(filPrev, "]\n");
		fclose(filPrev);
		//fprintf(filPrev2, "]}\n");
		//fclose(filPrev2);
	}
	return 0;
}


double calc_haversine_dist_latlon(double lat1, double lon1, double lat2, double lon2)
{
	double R = 6371e3; // metres
	double vinkel1 = lat1 * M_PI / 180; // vinkel and lambda in radians
	double vinkel2 = lat2 * M_PI / 180;
	double delta = (lat2 - lat1) * M_PI / 180;
	double deltalambda = (lon2 - lon1) * M_PI / 180;

	double a = lookUpSin(delta / 2) * lookUpSin(delta / 2) +
		lookUpCos(vinkel1) * lookUpCos(vinkel2) *
		lookUpSin(deltalambda / 2) * lookUpSin(deltalambda / 2);
	double c = 2 * atan2(sqrt(a), sqrt(1 - a));

	double d = R * c; // in metres
	return d; //  0.0;
}

int test_OpenTheSameRasterMultipleTimesAndRead()
{
	Raster* map;
	int nRows, nCols, i, nCopies = 1000;
	float*** raster;
	FILE* filpek;

	filpek = fopen("tmp_testFil.txt", "w");
	map = (Raster*)malloc2(nCopies * sizeof(Raster));
	raster = (float***)malloc2(nCopies * sizeof(float**));
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

int eval_coordWithinBoundingBox(double lon, double lat) {
	if (lon < model.boundingBox.xMin) {
		if (model.boundingBox.xMax - model.boundingBox.xMin < 355) {
			if (lon < model.boundingBox.xMin - 20)
				lon += 360;
		}
		else
			lon += 360;
	}
	else if (lon > model.boundingBox.xMax) {
		if (model.boundingBox.xMax - model.boundingBox.xMin < 355) {
			if (lon > model.boundingBox.xMax + 20)
				lon -= 360;
		}
		else
			lon -= 360;
	}
	//printf("eval_coordWithinBoundingBox coords %.3lf %.3lf box %.2lf %.2lf %.2lf %.2lf\n\n", lon, lat,
	//	model.boundingBox.xMin, model.boundingBox.yMin,
	//	model.boundingBox.xMax, model.boundingBox.yMax);
	if (lat < model.boundingBox.yMin)
		return 0;
	if (lon <= model.boundingBox.xMin)
		return 0;
	if (lat >= model.boundingBox.yMax)
		return 0;
	if (lon >= model.boundingBox.xMax)
		return 0;

	return 1;
}

int check_translate_xCoordArray(double* xCoord, int nCoords) {
	int i, translate = 0;

	for (i = 0; i < nCoords; i++) {
		if (xCoord[i] < model.boundingBox.xMin) {
			if (model.boundingBox.xMax - model.boundingBox.xMin < 355) {
				if (xCoord[i] < model.boundingBox.xMin - 20) {
					xCoord[i] += 360;
					translate = 1;
				}
			}
			else {
				xCoord[i] += 360;
				translate = 1;
			}
		}
		else if (xCoord[i] > model.boundingBox.xMax) {
			if (model.boundingBox.xMax - model.boundingBox.xMin < 355) {
				if (xCoord[i] > model.boundingBox.xMax + 20) {
					xCoord[i] -= 360;
					translate = 1;
				}
			}
			else {
				xCoord[i] -= 360;
				translate = 1;
			}
		}
	}
	return translate;
}

int check_translate_xCoord(double* xCoord) {
	int translate = 0;

	if (*xCoord < model.boundingBox.xMin) {
		if (model.boundingBox.xMax - model.boundingBox.xMin < 355) {
			if (*xCoord < model.boundingBox.xMin - 20) {
				*xCoord += 360;
				translate = 1;
			}
		}
		else {
			*xCoord += 360;
			translate = 1;
		}
	}
	else if (*xCoord > model.boundingBox.xMax) {
		if (model.boundingBox.xMax - model.boundingBox.xMin < 355) {
			if (*xCoord > model.boundingBox.xMax + 20) {
				*xCoord -= 360;
				translate = 1;
			}
		}
		else {
			*xCoord -= 360;
			translate = 1;
		}
	}
	return translate;
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
	//i = 0;
	//errlog("ERROR! Remove the above row\n");
	if (i > 0) {
		for (j = i; j < model.storms[pos].nFeatures; j++) {
			copyStormFeature(model.storms[pos].feature[j], &(model.storms[pos].feature[j - i]));
			//errlog("storm %d copy from %d to %d\n", pos, j, j-1);

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
	model.storms[pos].inv_timeIntervall_h = 1 / model.storms[pos].timeIntervall_h;
	nAlloc = ((model.storms[pos].feature[model.storms[pos].nFeatures - 1].UTCseconds -
		model.params.UTC_secondsStart) / 3600.0 + nExtraEndHours + 2) / model.storms[pos].timeIntervall_h;
	if (nAlloc < 1) {
		errlog("ERROR! StormPos %d stormID %s starts too early so shouldn't be part of the optimization. nFeatures %d.\n", pos,
			model.storms[pos].stormID, model.storms[pos].nFeatures);
		nAlloc = 1;
	}
	//printf("%d nFeatures %d UTCSec %I64d startPlanSec %I64d nAlloc %d\n", pos,
	//	model.storms[pos].nFeatures, model.storms[pos].feature[model.storms[pos].nFeatures - 1].UTCseconds,
	//	model.params.UTC_secondsStart, nAlloc);
	model.storms[pos].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

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
		else {
			// maxTid = model.storms[pos].feature[i + 1].UTCseconds - 1;
			maxTid = (model.storms[pos].feature[i].UTCseconds + model.storms[pos].feature[i + 1].UTCseconds) / 2;
		}
		//printf("stormNr %d i %d start slut %I64d %I64d optStart %I64d\ndatum\n%s\n%s\n%s\n", 
		//	model.storms[pos].stormNr, i, model.storms[pos].feature[i].UTCseconds, maxTid,
		//	model.params.UTC_secondsStart,
		//	stringDateFromUTCSeconds(model.storms[pos].feature[i].UTCseconds).c_str(),
		//	stringDateFromUTCSeconds(maxTid).c_str(),
		//	stringDateFromUTCSeconds(model.params.UTC_secondsStart).c_str());

		for (; tidInt < 100000; tidInt++) {
			if (tidInt >= nAlloc) {
				errlog("ERROR! Too many tidInt compared to allocated for stormPos %d ID %s. I skip the rest. maxTid %I64d %s\n", pos, 
					model.storms[pos].stormID, maxTid,
					stringDateFromUTCSeconds(maxTid).c_str());
				break;
			}
			sekNu = (long long)(tidInt * model.storms[pos].timeIntervall_h * 3600 + model.params.UTC_secondsStart);
			//printf("  tidInt %d sekNu %.3lf stormTid %.3lf\n", tidInt, (sekNu - model.params.UTC_secondsStart )/ 3600.0, 
			//	(maxTid - model.params.UTC_secondsStart ) / 3600.0);
			if (sekNu <= maxTid)
				model.storms[pos].timeIntervalIndex[tidInt] = i;
			else
				break;
			//printf("stormPos %d nr %d tidInt %d sec %I64d ger stormFeature %d (endtid %I64d)\n",
			//	pos, model.storms[pos].stormNr, tidInt, sekNu, i, maxTid);
		}
	}
	if (tidInt < 1) {
		errlog("ERROR! StormPos %d stormID %s tidInt %d, I set it to 1 but this storm should not be included\n",
			pos, model.storms[pos].stormID, tidInt);
		tidInt = 1;
		model.storms[pos].timeIntervalIndex[tidInt - 1] = 0;
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

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	FILE* filpek = NULL;

	if (SKRIV_UT_NOTHING == 0) {
		sprintf(namn, "%s/checkStormWeather.txt", model.params.resultPath.c_str());
		filpek = fopen(namn, "w");
		free(namn);
		fprintf(filpek, "stormID;featureNr;lon;lat;UTCseconds;dateTime;tidFromStartOpt;distFeatureNu;stormVal;uCurrent;vCurrent;"
			"currentDirection;currentSpeed;uWind;vWind;windDirection;windSpeed;waveHeight;wavePeriod;waveDirection;iceCover\n");
	}
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
				stormVarde = getStormValue(tidTot, model.weatherFunctions.point_lat[i3], model.weatherFunctions.point_lon[i3]); // model.weatherFunctions.point[i3]);
				uCurrent = getVariableValue(model.functions.pos_current_u, i3, tidTot);
				vCurrent = getVariableValue(model.functions.pos_current_v, i3, tidTot);
				if (uCurrent < 1000 && vCurrent < 1000) {
					currentDirection = atan2(vCurrent, uCurrent);
					currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
					//if (tidTot >= model.weather[model.functions.pos_current_u].tidpHistoricalWeather)
					//	currentSpeed *= model.params.historicDataFactor_current;
				}
				else {
					currentDirection = 0;
					currentSpeed = 0;
				}
				uWind = getVariableValue(model.functions.pos_wind_u, i3, tidTot);
				vWind = getVariableValue(model.functions.pos_wind_v, i3, tidTot);
				if (uWind < 1000 && vWind < 1000) {
					windDirection = atan2(vWind, uWind);
					windSpeed2 = uWind * uWind + vWind * vWind;
					windSpeed = sqrt(windSpeed2);
					//if (tidTot >= model.weather[model.functions.pos_wind_u].tidpHistoricalWeather)
					//	windSpeed *= model.params.historicDataFactor_windSpeed;

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
				//if (tidTot >= model.weather[model.functions.pos_waveHeight].tidpHistoricalWeather)
				//	waveHeight *= model.params.historicDataFactor_waveHeight;
				wavePeriod = getVariableValue(model.functions.pos_wavePeriod, i3, tidTot);
				waveDirection = getVariableValue(model.functions.pos_waveDirection, i3, tidTot);
				iceCover = getVariableValue(model.functions.pos_iceThickness, i3, tidTot);
				//printf("%d;%d;%.3lf;%.3lf;%I64d;%s;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf\n",
				//	model.storms[i].stormNr, i1, p3.longitude().degrees(), p3.latitude().degrees(),
				//	seconds, stringDateFromUTCSeconds(seconds).c_str(), tidTot, distNu, stormVarde, uCurrent, vCurrent, currentDirection,
				//	currentSpeed, uWind, vWind, windDirection, windSpeed,
				//	waveHeight, wavePeriod, waveDirection, iceCover);
				if (SKRIV_UT_NOTHING == 0) {
					fprintf(filpek, "%s;%d;%.3lf;%.3lf;%I64d;%s;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf;%.3lf\n",
						model.storms[i].stormID, i1, p3.longitude().degrees(), p3.latitude().degrees(),
						seconds, stringDateFromUTCSeconds(seconds).c_str(), tidTot, distNu, stormVarde, uCurrent, vCurrent, currentDirection,
						currentSpeed, uWind, vWind, windDirection, windSpeed,
						waveHeight, wavePeriod, waveDirection, iceCover);
				}
			}


		}
	}
	if (SKRIV_UT_NOTHING == 0) {
		fclose(filpek);
	}
}


void calc_stormsNearby() {
	int i, i1, i2, posUse, stormOK, keepStorm;
	double timeFromStart, dist, minTid, maxTid, minDistToStorm, maxSpeed = 0, minSpeed = 9999, speed;

	for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
		speed = eval_calmWaterSpeed(i, -1, -100, -1);
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
	errlog("nStorms %d minSpeed %.2lf maxSpeed %.2lf\n", model.nStorms, minSpeed, maxSpeed);
	for (i = 0; i < model.nStorms; i++) {
		stormOK = eval_stormWithinBoundingBox(i);
		//printf("%d stormNr %d stormOK %d\n", i, model.storms[i].stormNr, stormOK);
		if (stormOK == 0)
			continue;

		keepStorm = 0;
		//errlog("ERROR! Change keepStorm to = 0 above\n");
		for (i1 = 0; i1 < model.storms[i].nFeatures; i1++) {
			// find two closest points on preferred path within time possible, if either plus maxDistFromPath is close enough to outer circle then keep the storm
			timeFromStart = (model.storms[i].feature[i1].UTCseconds - model.params.UTC_secondsStart) / 3600.0;
			//printf("timeFromStart %.2lf storm %d i1 %d UTCstorm %.2lf optStart %.2lf\n", 
			//	timeFromStart, i, i1, model.storms[i].feature[i1].UTCseconds/3600.0, model.params.UTC_secondsStart/3600.0);
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
				//printf("minDistToStorm %.2lf maxDist %.2lf\n", minDistToStorm, model.storms[i].feature[i1].outerCircleSize);
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
			// errlog("ERROR! sort the storm features in time order AND only include needed ones AND possibly identify timeperiod for each\n");
			// sort the timeperiods in the storm
			sortStormFeaturesTime(posUse);

			// add bearing and distanceToNextPoint per timeperiod
			addInfoToStorms(posUse);
			posUse++;
		}
		else {
			//errlog("free storm i %d all nAlloc %d features\n", i, model.storms[i].nAllocFeatures);
			//for (i1 = 0; i1 < model.storms[i].nAllocFeatures; i1++) {
				free(model.storms[i].feature);
			//}
		}

	}
	model.nStorms = posUse;
	errlog("nStormsUse %d\n", model.nStorms);

}

void calc_boundingBoxFromAllNodes(int alt) {
	double x, y;
	int i, i1;

	if (alt == 0) {
		model.boundingBox.xMin = model.preferredPath.minX;
		model.boundingBox.yMin = 90;
		model.boundingBox.xMax = model.preferredPath.maxX;
		model.boundingBox.yMax = -90;
	}

	for (int i = 0; i < model.preferredPath.nPoints; i++) {
		y = model.preferredPath.point_y[i];
		if (y < model.boundingBox.yMin)
			model.boundingBox.yMin = y;
		if (y > model.boundingBox.yMax)
			model.boundingBox.yMax = y;
	}

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;
			y = model.network.physicalLev[i].point_y[i1];
			if (y < model.boundingBox.yMin)
				model.boundingBox.yMin = y;
			if (y > model.boundingBox.yMax)
				model.boundingBox.yMax = y;
			x = model.network.physicalLev[i].point_x[i1];


			if (x < model.preferredPath.minX) {
				if (model.preferredPath.maxX - model.preferredPath.minX < 355) {
					if (x < model.preferredPath.minX - 20)
						x += 360;
				}
				else
					x += 360;
			}
			if (x > model.preferredPath.maxX) {
				if (model.preferredPath.maxX - model.preferredPath.minX < 355) {
					if (x > model.preferredPath.maxX + 20)
						x -= 360;
				}
				else
					x -= 360;
			}
			//if (x > model.preferredPath.maxX + 20)
			//	x -= 360;
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

void setupBoundingBoxFromMapCoords(Raster map) {
	model.boundingBox.xMin = map.Get_minLongitude();
	model.boundingBox.yMin = map.Get_minLatitude();
	model.boundingBox.xMax = map.Get_maxLongitude(); 
	model.boundingBox.yMax = map.Get_maxLatitude();;
}

void calc_boundingBoxFrompreferredPath(int alt) {
	double y, distTmp;
	int i1, nIntervall;
	spherical::Point p1;


	if (alt == 0) {
		model.boundingBox.xMin = model.preferredPath.minX;
		model.boundingBox.yMin = 90;
		model.boundingBox.xMax = model.preferredPath.maxX;
		model.boundingBox.yMax = -90;
		errlog("boundingBox in calc_boundingBoxFrompreferredPath alt %d set to min/max %.3lf %.3lf\n", alt,
			model.boundingBox.xMin, model.boundingBox.xMax);
		// }

		for (int i = 0; i < model.preferredPath.nPoints; i++) {
			y = model.preferredPath.point_y[i];
			if (y < model.boundingBox.yMin)
				model.boundingBox.yMin = y;
			if (y > model.boundingBox.yMax)
				model.boundingBox.yMax = y;
			if (model.preferredPath.distToPrevPoint[i] > 300) {
				nIntervall = (int)(model.preferredPath.distToPrevPoint[i] / 200.0);
				distTmp = model.preferredPath.distToPrevPoint[i] / (nIntervall + 1) * 1000;

				for (i1 = 0; i1 < nIntervall; i1++) {
					p1 = model.preferredPath.point[i - 1].destinationPoint(distTmp * (i1 + 1), model.preferredPath.point[i - 1].bearingTo(model.preferredPath.point[i]));
					y = p1.latitude().degrees();
					if (y < model.boundingBox.yMin)
						model.boundingBox.yMin = y;
					if (y > model.boundingBox.yMax)
						model.boundingBox.yMax = y;
				}
			}
		}
		errlog("boundingBox in calc_boundingBoxFrompreferredPath alt %d after prefPath points min/max %.3lf %.3lf\n", alt,
			model.boundingBox.xMin, model.boundingBox.xMax);


		model.params.maxDeviationPreferred_km = model.params.shipSpeed_average / model.params.ortoDist_nPointsPerHour * (model.params.nPkterOrto - 1) / 2;
		double deltaY = model.params.maxDeviationPreferred_km / 111 + 0.3; // 120; max antal grader
		model.boundingBox.yMin -= deltaY;
		model.boundingBox.yMax += deltaY;
		double abs_yMax = abs(model.boundingBox.yMax);
		double abs_yMin = abs(model.boundingBox.yMin);
		double abs_y;
		if (abs_yMax > abs_yMin)
			abs_y = abs_yMax;
		else
			abs_y = abs_yMin;
		double deltaX = model.params.maxDeviationPreferred_km / 110.25 / lookUpCos(abs_y * M_PI / 180) + 0.3; // 120; max antal grader
		model.boundingBox.xMin -= deltaX;
		model.boundingBox.xMax += deltaX;
		errlog("boundingBox in calc_boundingBoxFrompreferredPath alt %d after deltaX min/max %.3lf %.3lf\n", alt,
			model.boundingBox.xMin, model.boundingBox.xMax);
	}

}


int openPhysicalMapAB_local() {
	char* namn2;
	Raster rasterPhysicalMapA, rasterPhysicalMapB;
	namn2 = (char*)malloc2(256 * sizeof(char));

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalAFileName.c_str());

	auto tid1 = std::chrono::high_resolution_clock::now();
	rasterPhysicalMapA.open(namn2);
	model.physicalMapA.valueCell = rasterPhysicalMapA.GetRasterBand_intArrTest(1, &(model.physicalMapA), model.boundingBox);
	if (model.physicalMapA.valueCell == NULL) {
		errlog("ERROR! Failed to load physical map %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load physical map " + std::string(namn2) + ". Must be datatype Byte", 1);
	}
	auto tid2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms2 = tid2 - tid1;
	//if (SKRIV_UT_NOTHING == 0) {
	//	printf("read rasterTest map took %.3lf\n", fp_ms2);
	//	printf("-- Time after loading physicalMapA %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	//}

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapPhysicalBFileName.c_str());

	rasterPhysicalMapB.open(namn2);
	//printf("-- Time2c %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	model.physicalMapB.valueCell = rasterPhysicalMapB.GetRasterBand_intArrTest(1, &(model.physicalMapB), model.boundingBox);
	if (model.physicalMapB.valueCell == NULL) {
		errlog("ERROR! Failed to load physical map %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load physical map " + std::string(namn2) + ". Must be datatype Byte", 1);
	}
	free(namn2);

	return 0;
}

int openPhysicalMapAB_lessBuffer_local() {
	char* namn2;
	Raster rasterPhysical_lessBuffer_MapA, rasterPhysical_lessBuffer_MapB;
	namn2 = (char*)malloc2(256 * sizeof(char));

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.mapAutoRoutePhysicalAFileName.c_str());
	rasterPhysical_lessBuffer_MapA.open(namn2);
	//printGlobal = 1;
	model.physical_lessBuffer_MapA.valueCell = rasterPhysical_lessBuffer_MapA.GetRasterBand_intArrTest(1, &(model.physical_lessBuffer_MapA), model.boundingBox);
	//printGlobal = 0;
	if (model.physicalMapA.valueCell == NULL) {
		errlog("ERROR! Failed to load physical map %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load physical map " + std::string(namn2) + ". Must be datatype Byte", 1);
	}

	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.mapAutoRoutePhysicalBFileName.c_str());
	rasterPhysical_lessBuffer_MapB.open(namn2);
	//printf("-- Time2c %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	model.physical_lessBuffer_MapB.valueCell = rasterPhysical_lessBuffer_MapB.GetRasterBand_intArrTest(1, &(model.physical_lessBuffer_MapB), model.boundingBox);
	if (model.physical_lessBuffer_MapB.valueCell == NULL) {
		errlog("ERROR! Failed to load physical map %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load physical map " + std::string(namn2) + ". Must be datatype Byte", 1);
	}

	free(namn2);

	return 0;
}

int openNoGoAreas_local(int i, char* namn2) {
	//printf("opens %s\n", namn2);
	model.extraNoGoArea[i].rasterA.open(namn2);
	model.extraNoGoArea[i].mapA.valueCell = model.extraNoGoArea[i].rasterA.GetRasterBand_intArrTest(1, &(model.extraNoGoArea[i].mapA), model.boundingBox);
	if (model.extraNoGoArea[i].mapA.valueCell == NULL) {
		errlog("ERROR! Failed to load extra noGo mapA %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load extra noGo mapA " + std::string(namn2) + ". Must be datatype Byte", 1);
	}
	if (i >= model.nExtraNoGoAreas)
		sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapLandSeaBFileName.c_str());
	else
		sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].fileNameB);
	model.extraNoGoArea[i].rasterB.open(namn2);
	model.extraNoGoArea[i].mapB.valueCell = model.extraNoGoArea[i].rasterA.GetRasterBand_intArrTest(1, &(model.extraNoGoArea[i].mapB), model.boundingBox);
	if (model.extraNoGoArea[i].mapB.valueCell == NULL) {
		errlog("ERROR! Failed to load extra noGo mapB %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load extra noGo mapB " + std::string(namn2) + ". Must be datatype Byte", 1);
	}

	return 0;
}

int openExtraCostAreas_local(int i, char* namn2) {
	//printf("opens %s\n", namn2);
	model.extraCostArea[i].rasterA.open(namn2);
	model.extraCostArea[i].mapA.valueCell = model.extraCostArea[i].rasterA.GetRasterBand_intArrTest(1, &(model.extraCostArea[i].mapA), model.boundingBox);
	if (model.extraCostArea[i].mapA.valueCell == NULL) {
		errlog("ERROR! Failed to load extra cost area mapA %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load extra cost area mapA " + std::string(namn2) + ". Must be datatype Byte", 1);
	}
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.extraNoGoAreaBase[model.extraCostArea[i].posBase].fileNameB);
	model.extraCostArea[i].rasterB.open(namn2);
	model.extraCostArea[i].mapB.valueCell = model.extraCostArea[i].rasterA.GetRasterBand_intArrTest(1, &(model.extraCostArea[i].mapB), model.boundingBox);
	if (model.extraCostArea[i].mapB.valueCell == NULL) {
		errlog("ERROR! Failed to load extra cost area mapB %s. Must be datatype Byte. I quit\n", namn2);
		postRequest("ERROR!Failed to load extra cost area mapB " + std::string(namn2) + ". Must be datatype Byte", 1);
	}

	return 0;
}

//int openFuelMaps_local() {
//	char* namn2;
//	Raster rasterFuelMapA, rasterFuelMapB;
//	namn2 = (char*)malloc2(256 * sizeof(char));

//	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyAFileName.c_str());
//	rasterFuelMapA.open(namn2);
//	model.fuelMapA.valueCell = rasterFuelMapA.GetRasterBand_intArrTest(1, &(model.fuelMapA), model.boundingBox);
//	if (model.fuelMapA.valueCell == NULL) {
//		errlog("ERROR! Failed to load fuel map %s. Must be datatype Byte. I quit\n", namn2);
//		postRequest("ERROR!Failed to load fuel map " + std::string(namn2) + ". Must be datatype Byte", 1);
//	}
//	//if (SKRIV_UT_NOTHING == 0) 
//	//	printf("-- Time after loading fuelMapA %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

//	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapFuelGeographyBFileName.c_str());
//	rasterFuelMapB.open(namn2);
//	//printf("-- Time2g %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
//	//printGlobal = 1;
//	model.fuelMapB.valueCell = rasterFuelMapB.GetRasterBand_intArrTest(1, &(model.fuelMapB), model.boundingBox);
//	if (model.fuelMapA.valueCell == NULL) {
//		errlog("ERROR! Failed to load fuel map %s. Must be datatype Byte. I quit\n", namn2);
//		postRequest("ERROR!Failed to load fuel map " + std::string(namn2) + ". Must be datatype Byte", 1);
//	}

//	free(namn2);

//	return 0;
//}

int makeSureChannelsInBoundingBox() {
	double xMinBas, yMinBas, xMaxBas, yMaxBas, xTrans;
	int i, pos;

	xMinBas = model.boundingBox.xMin;
	xMaxBas = model.boundingBox.xMax;
	yMinBas = model.boundingBox.yMin;
	yMaxBas = model.boundingBox.yMax;

	int* useChannel = (int*)malloc(model.network.nChannels * sizeof(int));
	for (i = 0; i < model.network.nChannels; i++) {
		useChannel[i] = 1;
		xTrans = 0;
		if (model.boundingBox.xMin > model.network.channel[i].boundingBox.xMax + 5)
			xTrans += 360;
		if (model.boundingBox.xMax < model.network.channel[i].boundingBox.xMin - 5)
			xTrans -= 360;
		if (model.network.channel[i].boundingBox.xMin + xTrans < model.boundingBox.xMin - 5)
			useChannel[i] = 0;
		if (model.network.channel[i].boundingBox.xMax + xTrans > model.boundingBox.xMax + 5)
			useChannel[i] = 0;
		if (model.network.channel[i].boundingBox.yMin < model.boundingBox.yMin - 5)
			useChannel[i] = 0;
		if (model.network.channel[i].boundingBox.yMax > model.boundingBox.yMax + 5)
			useChannel[i] = 0;

		if (useChannel[i] == 1) {
			if (model.network.channel[i].boundingBox.xMin + xTrans < xMinBas)
				xMinBas = model.network.channel[i].boundingBox.xMin + xTrans;
			if (model.network.channel[i].boundingBox.xMax + xTrans > xMaxBas)
				xMaxBas = model.network.channel[i].boundingBox.xMax + xTrans;
			if (model.network.channel[i].boundingBox.yMin < yMinBas)
				yMinBas = model.network.channel[i].boundingBox.yMin;
			if (model.network.channel[i].boundingBox.yMax > yMaxBas)
				yMaxBas = model.network.channel[i].boundingBox.yMax;
		}
	}

	pos = 0;
	for (i = 0; i < model.network.nChannels; i++) {
		if (useChannel[i] == 1) {
			if (i > pos)
				model.network.channel[pos] = model.network.channel[i];
			pos++;
		}
	}
	model.network.nChannels = pos;

	model.boundingBox.xMin = xMinBas;
	model.boundingBox.xMax = xMaxBas;
	model.boundingBox.yMin = yMinBas;
	model.boundingBox.yMax = yMaxBas;


	return 0;
}

double get_bast_longitude_fit_to_physicalMapA(double min_x, double max_x) {
	double mid, delta_x = 0, diff1, diff2;

	mid = (min_x + max_x) / 2.0;
	if (mid > model.physicalMapA.maxLongitude) {
		if (mid - 360 < model.physicalMapA.minLongitude) {
			diff1 = mid - model.physicalMapA.maxLongitude;
			diff2 = model.physicalMapA.minLongitude - (mid - 360);
			if (diff1 > diff2)
				delta_x = -360;
		}
		else
			delta_x = -360;
	}
	else {
		if (mid < model.physicalMapA.minLongitude) {
			if (mid + 360 > model.physicalMapA.maxLongitude) {
				diff1 = model.physicalMapA.minLongitude - mid;
				diff2 = mid + 360 - model.physicalMapA.maxLongitude;
				if (diff1 > diff2)
					delta_x = 360;
			}
			else
				delta_x = 360;
		}
	}

	return delta_x;
}

int setup_noGo_polygons_GDAL() {
	int i, i1, i2;
	double min_x, max_x, delta_x;
	// create the GDAL polygons
	for (i = 0; i < model.nExtraNoGoPolygons; i++) {
		model.extraNoGoPolygon[i].polygon_GDAL = (OGRPolygon**)malloc(model.extraNoGoPolygon[i].nPolygons * sizeof(OGRPolygon*));
		for (i1 = 0; i1 < model.extraNoGoPolygon[i].nPolygons; i1++) {
			model.extraNoGoPolygon[i].polygon_GDAL[i1] = new OGRPolygon();
			OGRLinearRing* ring = new OGRLinearRing();
			min_x = 1e10;
			max_x = -1e10;
			for (i2 = 0; i2 < model.extraNoGoPolygon[i].polygon[i1].nCoords; i2++) {
				if (min_x > model.extraNoGoPolygon[i].polygon[i1].x[i2])
					min_x = model.extraNoGoPolygon[i].polygon[i1].x[i2];
				if (max_x < model.extraNoGoPolygon[i].polygon[i1].x[i2])
					max_x = model.extraNoGoPolygon[i].polygon[i1].x[i2];
			}
			delta_x = get_bast_longitude_fit_to_physicalMapA(min_x, max_x);

			for (i2 = 0; i2 < model.extraNoGoPolygon[i].polygon[i1].nCoords; i2++) {
				ring->addPoint(model.extraNoGoPolygon[i].polygon[i1].y[i2], model.extraNoGoPolygon[i].polygon[i1].x[i2] + delta_x);
			}
			if (model.extraNoGoPolygon[i].polygon[i1].x[i2 - 1] != model.extraNoGoPolygon[i].polygon[i1].x[0] ||
				model.extraNoGoPolygon[i].polygon[i1].y[i2 - 1] != model.extraNoGoPolygon[i].polygon[i1].y[0]) {
				postRequest("ERROR! useOptionalExtraNoGoAreas given extraAreaID custom customAreaID " + std::string(model.extraNoGoPolygon[i].id) + " the first and last coordinates are not the same, I close the ring", 0);
				ring->closeRings();
			}
			//(model.extraNoGoPolygon[i].polygon_GDAL[i1])->addRingDirectly(ring);
			(model.extraNoGoPolygon[i].polygon_GDAL[i1])->addRing(ring);
		}
	}


	return 0;
}

int setup_restrictedArea_polygons_GDAL() {
	int i, i1, i2;
	double min_x, max_x, delta_x;
	// create the GDAL polygons
	for (i = 0; i < model.nRestrictedAreas; i++) {
		model.restrictedArea[i].polygon_GDAL = (OGRPolygon**)malloc(model.restrictedArea[i].nPolygons * sizeof(OGRPolygon*));
		for (i1 = 0; i1 < model.restrictedArea[i].nPolygons; i1++) {
			model.restrictedArea[i].polygon_GDAL[i1] = new OGRPolygon();
			OGRLinearRing* ring = new OGRLinearRing();
			min_x = 1e10;
			max_x = -1e10;
			for (i2 = 0; i2 < model.restrictedArea[i].polygon[i1].nCoords; i2++) {
				if (min_x > model.restrictedArea[i].polygon[i1].x[i2])
					min_x = model.restrictedArea[i].polygon[i1].x[i2];
				if (max_x < model.restrictedArea[i].polygon[i1].x[i2])
					max_x = model.restrictedArea[i].polygon[i1].x[i2];
			}
			delta_x = get_bast_longitude_fit_to_physicalMapA(min_x, max_x);

			for (i2 = 0; i2 < model.restrictedArea[i].polygon[i1].nCoords; i2++) {
				ring->addPoint(model.restrictedArea[i].polygon[i1].y[i2], model.restrictedArea[i].polygon[i1].x[i2] + delta_x);
			}
			if (model.restrictedArea[i].polygon[i1].x[i2 - 1] != model.restrictedArea[i].polygon[i1].x[0] ||
				model.restrictedArea[i].polygon[i1].y[i2 - 1] != model.restrictedArea[i].polygon[i1].y[0]) {
				postRequest("ERROR! restrictedArea given id " + std::string(model.restrictedArea[i].id) + " the first and last coordinates are not the same, I close the ring", 0);
				ring->closeRings();
			}
			(model.restrictedArea[i].polygon_GDAL[i1])->addRing(ring);
		}
	}

	return 0;
}

int openNeededRasterFilesNew(int alt)
{
	char* namn2;
	namn2 = (char*)malloc2(256 * sizeof(char));
	//Raster rasterFuelMapA, rasterFuelMapB, rasterTimeDelay;
	int i, i1, i2;

	if (alt > -10) {
		if (model.params.varyStartEndArcLength == 0)
			calc_boundingBoxFromAllNodes(alt);
		else
			calc_boundingBoxFrompreferredPath(alt);
		// calc_stormsNearby();
	}
	else {
		if(alt == -10)
			calc_boundingBoxAutoRoute(); // not needed anymore, calculating this from solution to searoute
	}
	makeSureChannelsInBoundingBox();

	openPhysicalMapAB_local();

	int nExtra = model.nExtraNoGoAreas;
	if (alt <= -10)
		nExtra++; // it's the land map

	checkMinnesAnvandning(__LINE__);
	//printf("nExtra %d\n", nExtra);
	for (i = 0; i < nExtra; i++) {
		//printf("nExtra igen %d\n", nExtra);

		if(i >= model.nExtraNoGoAreas)
			sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.params.mapLandSeaAFileName.c_str());
		else
			sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].fileNameA);

		checkMinnesAnvandning(__LINE__);
		openNoGoAreas_local(i, namn2);
	}

	if (alt == -11) {
		openPhysicalMapAB_lessBuffer_local();
	}
	checkMinnesAnvandning(__LINE__);

	setup_noGo_polygons_GDAL();
	setup_restrictedArea_polygons_GDAL();

	if (alt == -10)
		return 0;

	nExtra = model.nExtraCostAreas;

	checkMinnesAnvandning(__LINE__);
	//printf("nExtraCost areas %d\n", nExtra);
	for (i = 0; i < nExtra; i++) {
		sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.extraNoGoAreaBase[model.extraCostArea[i].posBase].fileNameA);

		checkMinnesAnvandning(__LINE__);
		openExtraCostAreas_local(i, namn2);
	}


	// openFuelMaps_local();


	return 0;
}

double detDistLatLon(double lat1, double lon1, double lat2, double lon2) {
	if (lon1 > 180)
		lon1 -= 360;
	if (lon1 < -180)
		lon1 += 360;
	if (lon2 > 180)
		lon2 -= 360;
	if (lon2 < -180)
		lon2 += 360;
	spherical::Point p1(lat1, lon1), p2(lat2, lon2);
	return p1.distanceTo(p2);
}


double get_fuelQualityKvot(int thisLevel, int pos1, int nextLevel, int pos2)
{
	int arcOK = 1;
	//spherical::Point p1, p2;
	double distECA = 0.0, distOther = 0.0, distTot;
	double x1, y1, x2, y2;


	if (thisLevel >= 0) {
		x1 = model.network.physicalLev[thisLevel].point_x[pos1];
		y1 = model.network.physicalLev[thisLevel].point_y[pos1];
		if (nextLevel >= 0) {
			x2 = model.network.physicalLev[nextLevel].point_x[pos2];
			y2 = model.network.physicalLev[nextLevel].point_y[pos2];
			//if (thisLevel == 38 && pos1 == 45 && nextLevel == 39 && pos2 == 42) {
			//	//printGlobal = 1;
			//	printf("check xy %.3lf %.3lf %.3lf %.3lf\n", x1, y1, x2, y2);
			//}
			//p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			x2 = model.network.channel[-nextLevel - 1].point_x[0];
			y2 = model.network.channel[-nextLevel - 1].point_y[0];
			//p2 = model.network.channel[-nextLevel - 1].point[0];
		}
	}
	else {
		if (nextLevel >= 0) {
			x1 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
			y1 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
			//p1 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			x2 = model.network.physicalLev[nextLevel].point_x[pos2];
			y2 = model.network.physicalLev[nextLevel].point_y[pos2];
			//p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			if (thisLevel == nextLevel) {
				if (model.network.channel[-thisLevel - 1].ECA_type >= 0)
					return 1 - model.network.channel[-thisLevel - 1].ECA_type; // return the given ECA type if a corridor

				// if no given value, then use the ECA map
				x1 = model.network.channel[-thisLevel - 1].point_x[0];
				y1 = model.network.channel[-thisLevel - 1].point_y[0];
				//p1 = model.network.channel[-thisLevel - 1].point[0];
				x2 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
				y2 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
				//p2 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			}
			else {
				x1 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
				y1 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
				x2 = model.network.channel[-nextLevel - 1].point_x[0];
				y2 = model.network.channel[-nextLevel - 1].point_y[0];
			}
		}
	}

	get_fuelUseKvotECA(y1, x1, y2, x2, 0, &distECA, &distOther);
	// get_fuelUseKvotECA(p1.latitude().degrees(), p1.longitude().degrees(), p2.latitude().degrees(), p2.longitude().degrees(), 0, &distECA, &distOther);
	distTot = distECA + distOther;

	if (distTot > 0)
		return distOther / distTot;
	else
		return 0.0;
}

double get_extraAreaKvot(int thisLevel, int pos1, int nextLevel, int pos2, int posExtraArea, int extraType)
{
	int arcOK = 1;
	//spherical::Point p1, p2;
	double distArea = 0.0, distOther = 0.0, distTot;
	double x1, y1, x2, y2;


	if (thisLevel >= 0) {
		x1 = model.network.physicalLev[thisLevel].point_x[pos1];
		y1 = model.network.physicalLev[thisLevel].point_y[pos1];
		if (nextLevel >= 0) {
			x2 = model.network.physicalLev[nextLevel].point_x[pos2];
			y2 = model.network.physicalLev[nextLevel].point_y[pos2];
			//if (thisLevel == 38 && pos1 == 45 && nextLevel == 39 && pos2 == 42) {
			//	//printGlobal = 1;
			//	printf("check xy %.3lf %.3lf %.3lf %.3lf\n", x1, y1, x2, y2);
			//}
			//p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			x2 = model.network.channel[-nextLevel - 1].point_x[0];
			y2 = model.network.channel[-nextLevel - 1].point_y[0];
			//p2 = model.network.channel[-nextLevel - 1].point[0];
		}
	}
	else {
		if (nextLevel >= 0) {
			x1 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
			y1 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
			//p1 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			x2 = model.network.physicalLev[nextLevel].point_x[pos2];
			y2 = model.network.physicalLev[nextLevel].point_y[pos2];
			//p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			if (thisLevel == nextLevel) {
				if (model.network.channel[-thisLevel - 1].ECA_type >= 0)
					return model.network.channel[-thisLevel - 1].ECA_type; // return the given ECA type if a corridor

				// if no given value, then use the ECA map
				x1 = model.network.channel[-thisLevel - 1].point_x[0];
				y1 = model.network.channel[-thisLevel - 1].point_y[0];
				//p1 = model.network.channel[-thisLevel - 1].point[0];
				x2 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
				y2 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
				//p2 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			}
			else {
				x1 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
				y1 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
				x2 = model.network.channel[-nextLevel - 1].point_x[0];
				y2 = model.network.channel[-nextLevel - 1].point_y[0];

			}
		}
	}

	get_UseKvotExtraArea(y1, x1, y2, x2, posExtraArea, extraType, 0, &distArea, &distOther);
	// get_fuelUseKvotECA(p1.latitude().degrees(), p1.longitude().degrees(), p2.latitude().degrees(), p2.longitude().degrees(), 0, &distECA, &distOther);
	distTot = distArea + distOther;

	if (distTot > 0)
		return distArea / distTot;
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
	if (lon1 < physicalMap.minLongitude) {
		if (physicalMap.maxLongitude - physicalMap.minLongitude < 355) {
			if (lon1 < physicalMap.minLongitude - 20)
				lon1 += 360;
		}
		else
			lon1 += 360;
	}
	else {
		if (lon1 > physicalMap.maxLongitude) {
			if (physicalMap.maxLongitude - physicalMap.minLongitude < 355) {
				if (lon1 > physicalMap.maxLongitude + 20)
					lon1 -= 360;
			}
			else
				lon1 -= 360;
		}
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

void getRowColDblFromNoGoMap(Raster::strPhysRaster physicalMap, double lat1, double lon1, double* row1Dbl, double* col1Dbl) {
	if (lon1 < physicalMap.minLongitude) {
		if (physicalMap.maxLongitude - physicalMap.minLongitude < 355) {
			if (lon1 < physicalMap.minLongitude - 20)
				lon1 += 360;
		}
		else
			lon1 += 360;
	}
	else {
		if (lon1 > physicalMap.maxLongitude) {
			if (physicalMap.maxLongitude - physicalMap.minLongitude < 355) {
				if (lon1 > physicalMap.maxLongitude + 20)
					lon1 -= 360;
			}
			else
				lon1 -= 360;
		}
	}
	*row1Dbl = (physicalMap.maxLatitude - lat1) / physicalMap.size_row; // .raster.Get_sizeRow();
	*col1Dbl = (lon1 - physicalMap.minLongitude) / physicalMap.size_col; // .raster.Get_sizeCol();
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

double fix_longitude(double lon) {
	if (lon < model.physicalMapA.minLongitude) {
		if (model.physicalMapA.maxLongitude - model.physicalMapA.minLongitude < 355) {
			if (lon < model.physicalMapA.minLongitude - 20)
				lon += 360;
		}
		else
			lon += 360;
	}
	else {
		if (lon > model.physicalMapA.maxLongitude) {
			if (model.physicalMapA.maxLongitude - model.physicalMapA.minLongitude < 355) {
				if (lon > model.physicalMapA.maxLongitude + 20)
					lon -= 360;
			}
			else
				lon -= 360;
		}
	}
	return lon;
}

int check_feasibleNode_noGo_polygons(double lat, double lon) {
	int i, pos_noGoPoly;

	// lon = fix_longitude(lon);

	OGRPoint point(lat, lon);

	for (pos_noGoPoly = 0; pos_noGoPoly < model.nExtraNoGoPolygons; pos_noGoPoly++) {
		for (i = 0; i < model.extraNoGoPolygon[pos_noGoPoly].nPolygons; i++) {
			if ((model.extraNoGoPolygon[pos_noGoPoly].polygon_GDAL[i])->Contains(&point))
				break;
		}
		if (i < model.extraNoGoPolygon[pos_noGoPoly].nPolygons)
			break;
	}

	if (pos_noGoPoly < model.nExtraNoGoPolygons)
		return 0;
	return 1;
}



int check_noGoPolygons_ok(double lat1, double lon1, double lat2, double lon2) {
	int i, pos_noGoPoly;

	OGRLineString line;
	line.addPoint(lat1, lon1);
	line.addPoint(lat2, lon2);

	for (pos_noGoPoly = 0; pos_noGoPoly < model.nExtraNoGoPolygons; pos_noGoPoly++) {
		for (i = 0; i < model.extraNoGoPolygon[pos_noGoPoly].nPolygons; i++) {
			if (line.Intersects(model.extraNoGoPolygon[pos_noGoPoly].polygon_GDAL[i]))
				break;
		}
		if (i < model.extraNoGoPolygon[pos_noGoPoly].nPolygons)
			break;
	}

	if (pos_noGoPoly < model.nExtraNoGoPolygons)
		return 0;
	return 1;
}

int getMost_restrictedAreaCoords(double lat1, double lon1, double lat2, double lon2) {
	int i, pos, minPos = -1;
	double min_speed = 1e10;

	OGRLineString line;
	line.addPoint(lat1, lon1);
	line.addPoint(lat2, lon2);

	for (pos = 0; pos < model.nRestrictedAreas; pos++) {
		for (i = 0; i < model.restrictedArea[pos].nPolygons; i++) {
			if (line.Intersects(model.restrictedArea[pos].polygon_GDAL[i])) {
				break;
			}
		}
		if (i < model.restrictedArea[pos].nPolygons) {
			if (min_speed > model.restrictedArea[pos].max_speed) {
				minPos = pos;
				min_speed = model.restrictedArea[pos].max_speed;
			}
		}
	}

	return minPos;
}

OGRLineString create_ogrLine_straight(double lat1, double lon1, double lat2, double lon2) {

	OGRLineString line;
	line.addPoint(lat1, lon1);
	line.addPoint(lat2, lon2);
	return line;
}


OGRLineString create_ogrLine_channel(int cNr, double lat1, double lon1, double lat2, double lon2) {

	OGRLineString line;
	int startPos = 0, endPos;
	double x, y;

	for (int i = 0; i < model.network.channel[cNr].nPoints; i++) {
		line.addPoint(model.network.channel[cNr].point[i].latitude().degrees(), model.network.channel[cNr].point[i].longitude().degrees());
	}
	if(model.network.channel[cNr].nPoints < 2)
		line.addPoint(model.network.channel[cNr].point[0].latitude().degrees(), model.network.channel[cNr].point[0].longitude().degrees());

	return line;
}

OGRLineString create_ogrLine_prefPath(int lev1, int lev2, double lat1, double lon1, double lat2, double lon2) {

	OGRLineString line;
	int startPos = 0, endPos;
	double x, y;

	int level = lev1;
	startPos = 0;
	if (lev1 < 0) {
		startPos = model.network.channel[-level - 1].preferredPathPoint_posConnectFrom;
		if (startPos < 0) {
			errlog("ERROR! This shouldn't happen, row %d\n", __LINE__);
			printf("ERROR! This shouldn't happen, row %d\n", __LINE__);
			startPos = 0;
		}
		level = lev2 - 1;
		// pointLast = model.network.physicalLev[level].preferredPathPoint[startPos];
	}

	endPos = model.network.physicalLev[level].npreferredPathPoints;

	if (lev2 < 0)
		endPos = model.network.channel[-lev2 - 1].preferredPathPoint_posConnectTo + 1;

	if (endPos <= startPos)
		endPos = startPos + 1;

	line.addPoint(lat1, lon1);
	int nPkter = 1;
	for (int i3 = startPos; i3 < endPos; i3++) {
		y = model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees();
		x = model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees();
		if (abs(y - lat1) + abs(x - lon1) > 0.00001) {
			line.addPoint(y, x);
			nPkter++;
		}
	}
	if (abs(y - lat2) + abs(x - lon2) > 0.00001) {
		line.addPoint(lat2, lon2);
		nPkter++;
	}
	if(nPkter < 2)
		line.addPoint(y, x);

	return line;
}

int getMost_restrictedAreaLine(OGRLineString line) {
	int i, pos, minPos = -1;
	double min_speed = 1e10;

	for (pos = 0; pos < model.nRestrictedAreas; pos++) {
		for (i = 0; i < model.restrictedArea[pos].nPolygons; i++) {
			if (line.Intersects(model.restrictedArea[pos].polygon_GDAL[i])) {
				break;
			}
		}
		if (i < model.restrictedArea[pos].nPolygons) {
			if (min_speed > model.restrictedArea[pos].max_speed) {
				minPos = pos;
				min_speed = model.restrictedArea[pos].max_speed;
			}
		}
	}

	return minPos;
}

int check_extraNoGoMap_ok(double lat1, double lon1, double lat2, double lon2, int mapAlt, int pos_noGoMap) {
	Raster::strPhysRaster physicalMap;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg;
	int row, col, i, isOK, colUse, posOK;

	if (mapAlt == 0)
		physicalMap = model.extraNoGoArea[pos_noGoMap].mapB;
	else
		physicalMap = model.extraNoGoArea[pos_noGoMap].mapA;

	getRowColDblFromNoGoMap(physicalMap, lat1, lon1, &row1Dbl, &col1Dbl);
	getRowColDblFromNoGoMap(physicalMap, lat2, lon2, &row2Dbl, &col2Dbl);

	if ((row1Dbl < 0 && row2Dbl < 0) || (col1Dbl < 0 && col2Dbl < 0))
		return 1; // outside the area so all good
	if ((row1Dbl >= physicalMap.nRows && row2Dbl >= physicalMap.nRows ) || (col1Dbl >= physicalMap.nCols && col2Dbl >= physicalMap.nCols))
		return 1; // outside the area so all good

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
		//if (col >= physicalMap.nCols)
		//	colUse = col - physicalMap.nCols;
		//else {
		//	if (col < 0)
		//		colUse = col + physicalMap.nCols;
		//	else
		//		colUse = col;
		//}
		colUse = col;
		if (colUse >= 0 && row >= 0 && colUse < physicalMap.nCols && row < physicalMap.nRows)
			posOK = 1;
		else
			posOK = 0;
		if(posOK == 1){
			if (physicalMap.valueCell[row * physicalMap.nCols + colUse] == 1)
				return 0; // arc is not okay
		}
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

		if (posOK == 1) {
			if (physicalMap.valueCell[row * physicalMap.nCols + colUse] == 2) {
				y1 = physicalMap.maxLatitude - (row1Dbl + a0 * delta_row) * physicalMap.size_row;
				x1 = (col1Dbl + a0 * delta_col) * physicalMap.size_col + physicalMap.minLongitude;
				y2 = physicalMap.maxLatitude - (row1Dbl + a1 * delta_row) * physicalMap.size_row;
				x2 = (col1Dbl + a1 * delta_col) * physicalMap.size_col + physicalMap.minLongitude;
				isOK = check_extraNoGoMap_ok(y1, x1, y2, x2, 1, pos_noGoMap);
				if (isOK == 0)
					return 0; // arc is not okay in raster alt
			}
		}
		//if (mapAlt == 1)
		//	mapAlt = mapAlt;
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
		fuelMap = model.extraCostArea[0].mapB; // model.fuelMapB;
	else
		fuelMap = model.extraCostArea[0].mapA; // model.fuelMapA;

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

		y1 = fuelMap.maxLatitude - (row1Dbl + a0 * delta_row) * fuelMap.size_row;
		x1 = (col1Dbl + a0 * delta_col) * fuelMap.size_col + fuelMap.minLongitude;
		y2 = fuelMap.maxLatitude - (row1Dbl + a1 * delta_row) * fuelMap.size_row;
		x2 = (col1Dbl + a1 * delta_col) * fuelMap.size_col + fuelMap.minLongitude;
		if (fuelMap.valueCell[row * fuelMap.nCols + colUse] == 2) {
			get_fuelUseKvotECA(y1, x1, y2, x2, 1, distECA, distOther);
			if (printGlobal == 1)
				printf("i %d fuelMapVal alt %d val 2 distECA %.2lf distOther %.2lf\n", i, mapAlt, *distECA, *distOther);
		}
		else {
			dist = detDistLatLon(y1, x1, y2, x2);
			if (fuelMap.valueCell[row * fuelMap.nCols + colUse] == 1) {
				*distECA += dist;
				if (printGlobal == 1)
					printf("i %d fuelMapVal alt %d val 1 row %d col %d nCols %d nRows %d distECA %.2lf distOther %.2lf\n", i, mapAlt, row, colUse,
						fuelMap.nCols, fuelMap.nRows, *distECA, *distOther);
			}
			else {
				*distOther += dist;
				if (printGlobal == 1)
					printf("i %d fuelMapVal alt %d val 0 distECA %.2lf distOther %.2lf\n", i, mapAlt, *distECA, *distOther);
			}
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

void get_UseKvotExtraArea(double lat1, double lon1, double lat2, double lon2, int posExtraArea, int extraType, int mapAlt, double* distArea, double* distOther) {
	Raster::strPhysRaster extraAreaMap;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg, dist;
	int row, col, i, colUse;

	if (extraType == 0) { // noGo area
		if (mapAlt == 0)
			extraAreaMap = model.extraNoGoArea[posExtraArea].mapB; // model.fuelMapB;
		else
			extraAreaMap = model.extraNoGoArea[posExtraArea].mapA; // model.fuelMapA;
	}
	else { // extra cost area
		if (mapAlt == 0)
			extraAreaMap = model.extraCostArea[posExtraArea].mapB; // model.fuelMapB;
		else
			extraAreaMap = model.extraCostArea[posExtraArea].mapA; // model.fuelMapA;
	}
	getRowColDblFromPhysicalMap(extraAreaMap, lat1, lon1, &row1Dbl, &col1Dbl);
	getRowColDblFromPhysicalMap(extraAreaMap, lat2, lon2, &row2Dbl, &col2Dbl);

	delta_row = row2Dbl - row1Dbl;
	if (delta_row > extraAreaMap.nRows / 2) {
		delta_row = extraAreaMap.nRows - delta_row;
	}
	else {
		if (delta_row < -extraAreaMap.nRows / 2) {
			delta_row = -extraAreaMap.nRows - delta_row;
		}
	}
	delta_col = col2Dbl - col1Dbl;
	if (delta_col > extraAreaMap.nCols / 2) {
		delta_col = delta_col - extraAreaMap.nCols;
	}
	else {
		if (delta_col < -extraAreaMap.nCols / 2) {
			delta_col = extraAreaMap.nCols + delta_col;
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
		if (col >= extraAreaMap.nCols)
			colUse = col - extraAreaMap.nCols;
		else {
			if (col < 0)
				colUse = col + extraAreaMap.nCols;
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

		y1 = extraAreaMap.maxLatitude - (row1Dbl + a0 * delta_row) * extraAreaMap.size_row;
		x1 = (col1Dbl + a0 * delta_col) * extraAreaMap.size_col + extraAreaMap.minLongitude;
		y2 = extraAreaMap.maxLatitude - (row1Dbl + a1 * delta_row) * extraAreaMap.size_row;
		x2 = (col1Dbl + a1 * delta_col) * extraAreaMap.size_col + extraAreaMap.minLongitude;
		if (extraAreaMap.valueCell[row * extraAreaMap.nCols + colUse] == 2) {
			get_UseKvotExtraArea(y1, x1, y2, x2, posExtraArea, extraType, 1, distArea, distOther);
			if(printGlobal == 1)
				printf("i %d extraAreaMap nr %d alt %d val 2 distArea %.2lf distOther %.2lf\n", i, posExtraArea, mapAlt, *distArea, *distOther);
		}
		else {
			dist = detDistLatLon(y1, x1, y2, x2);
			if (extraAreaMap.valueCell[row * extraAreaMap.nCols + colUse] == 1) {
				*distArea += dist;
				if (printGlobal == 1)
					printf("i %d extraAreaMapVal nr %d alt %d val 1 row %d col %d nCols %d nRows %d distArea %.2lf distOther %.2lf\n", 
						i, posExtraArea, mapAlt, row, colUse,
						extraAreaMap.nCols, extraAreaMap.nRows, *distArea, *distOther);
			}
			else {
				*distOther += dist;
				if (printGlobal == 1)
					printf("i %d extraAreaMapVal nr %d alt %d val 0 distArea %.2lf distOther %.2lf\n", i, posExtraArea, mapAlt, *distArea, *distOther);
			}
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


int check_isPhysicalArcOK_old2(int startLevel, int slutLevel, spherical::Point p1, spherical::Point p2, double noDataVal)
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

int check_isPhysicalArcOK(int startLevel, int slutLevel, int pos1, int pos2, int allowShortArc, int followPrefPathExact)
{
	int arcOK = 1, mittPktPos1, mittPktPos2, level1, level2, connectChannels = 0, posEnd2;
	double x1, y1, x2, y2;

	// model.rasterData.physicalMap = model.physicalMapRaster.GetRasterBand(1);
	// float** rasterData = model.physicalMapRaster.GetRasterBand(1);
	double distance, b1;

	if (startLevel >= 0 && slutLevel >= 0) {
		x1 = model.network.physicalLev[startLevel].point_x[pos1];
		y1 = model.network.physicalLev[startLevel].point_y[pos1];
		x2 = model.network.physicalLev[slutLevel].point_x[pos2];
		y2 = model.network.physicalLev[slutLevel].point_y[pos2];
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
			if (startLevel >= 0) {
				x1 = model.network.physicalLev[startLevel].point_x[pos1];
				y1 = model.network.physicalLev[startLevel].point_y[pos1];
				//if (posPoly >= 0) {
				//	x2 = model.network.channel[-slutLevel - 1].polygonUse_x[pos2][posPoly];
				//	y2 = model.network.channel[-slutLevel - 1].polygonUse_y[pos2][posPoly];
				//}
				//else {
				if (pos2 == 1)
					pos2 = model.network.channel[-slutLevel - 1].nPoints - 1;
				x2 = model.network.channel[-slutLevel - 1].point_x[pos2];
				y2 = model.network.channel[-slutLevel - 1].point_y[pos2];
				//}
				level1 = startLevel - 1;
				level2 = startLevel;
			}
			else {
				if (pos1 == 1)
					pos1 = model.network.channel[-startLevel - 1].nPoints - 1;
				x1 = model.network.channel[-startLevel - 1].point_x[pos1];
				y1 = model.network.channel[-startLevel - 1].point_y[pos1];
				if (startLevel != slutLevel) {
					x2 = model.network.channel[-slutLevel - 1].point_x[pos2];
					y2 = model.network.channel[-slutLevel - 1].point_y[pos2];
				}
				else {
					x2 = model.network.channel[-slutLevel - 1].point_x[model.network.channel[-slutLevel - 1].nPoints - 1];
					y2 = model.network.channel[-slutLevel - 1].point_y[model.network.channel[-slutLevel - 1].nPoints - 1];
				}
				connectChannels = 1;
			}
		}
		else {
			if (pos1 == 1)
				pos1 = model.network.channel[-startLevel - 1].nPoints - 1;
			x1 = model.network.channel[-startLevel - 1].point_x[pos1];
			y1 = model.network.channel[-startLevel - 1].point_y[pos1];
			x2 = model.network.physicalLev[slutLevel].point_x[pos2];
			y2 = model.network.physicalLev[slutLevel].point_y[pos2];
			level1 = slutLevel;
			level2 = slutLevel + 1;
		}
	}

	distance = estimateLargeCircleDistance_km(y1, x1, y2, x2); // p1.distanceTo(p2) / 1000.0;
	if (distance < model.params.basDistArcs * 0.05 && allowShortArc == 0) // 1)
		return -2; // felaktig bage, addera ej utan titta i nasta niva istallet, hantera sista nivan...

	if (connectChannels == 0) {
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

		if (slutLevel - startLevel != 1) {
			if (distance >= model.params.basDistArcs * 1.4)
				return 0; // too far distance to channel
		}

		mittPktPos1 = model.network.physicalLev[level1].nPoints / 2;
		mittPktPos2 = model.network.physicalLev[level2].nPoints / 2;

		b1 = estimateBearingFromToCoords(model.network.physicalLev[level1].point_y[mittPktPos1],
			model.network.physicalLev[level1].point_x[mittPktPos1], model.network.physicalLev[level2].point_y[mittPktPos2],
			model.network.physicalLev[level2].point_x[mittPktPos2]);
	}
	else {
		if (distance >= model.params.basDistArcs * 1.4)
			return 0; // too far distance between channel
		posEnd2 = model.network.channel[-slutLevel - 1].nPoints - 1;
		b1 = estimateBearingFromToCoords(model.network.channel[-startLevel - 1].point_y[0], model.network.channel[-startLevel - 1].point_x[0],
			model.network.channel[-slutLevel - 1].point_y[posEnd2], model.network.channel[-slutLevel - 1].point_x[posEnd2]);
	}
	//if (startLevel < 0)
	//	printf("level1/2 %d %d slutLevel %d mittPktPos %d %d distance %.3lf basDistArcs %.3lf %.3lf %.3lf %.3lf %.3lf pos1 %d nPkterChannel %d\n",
	//		level1, level2, slutLevel, mittPktPos1, mittPktPos2, distance, model.params.basDistArcs, x1, y1, x2, y2, pos1,
	//		model.network.channel[-startLevel - 1].nPoints);

	// auto b1 = model.network.physicalLev[level1].point[mittPktPos1].bearingTo(model.network.physicalLev[level2].point[mittPktPos2]);
	double b2 = estimateBearingFromToCoords(y1, x1, y2, x2);
	// auto b2 = p1.bearingTo(p2);
	// modify the bearing
	double bNy = (b1 - b2);
	if (bNy < 0)
		bNy += 360;
	if (bNy >= 360)
		bNy -= 360;

	//if (startLevel < 0)
	//	printf("bNy %.3lf\n", bNy);

	if (distance < 0.01 && slutLevel < 0)
		return 1;

	if (bNy >= 90 && bNy <= 270 & distance > 1.0) {
		return -2; // felaktigt riktad bage, addera ej utan titta i nasta niva istallet, hantera sista nivan...

	}

	int isOk = check_physicalMap_ok(y1, x1, y2, x2, 0);

	int i1;
	double y1a, x1a, y1b, x1b;
	if (isOk == 1) {
		for (int i = 0; i < model.nExtraNoGoAreas; i++) {
			if (pos1 == model.params.preferredPathOrtoPos[startLevel] && pos2 == model.params.preferredPathOrtoPos[slutLevel] && slutLevel == startLevel + 1 && followPrefPathExact == 1) {
				y1a = y1;
				x1a = x1;
				for (i1 = 0; i1 < model.network.physicalLev[startLevel].npreferredPathPoints; i1++) {
					if (i1 < model.network.physicalLev[startLevel].npreferredPathPoints) {
						y1b = model.network.physicalLev[startLevel].preferredPathPoint[i1].latitude().degrees();
						x1b = model.network.physicalLev[startLevel].preferredPathPoint[i1].longitude().degrees();
					}
					else {
						y1b = y2;
						x1b = x2;
					}
					isOk = check_extraNoGoMap_ok(y1a, x1a, y1b, x1b, 0, i);
					if (isOk == 0)
						break;
					y1a = y1b;
					x1a = x1b;
				}
				if (i1 < model.network.physicalLev[startLevel].npreferredPathPoints + 1)
					break;
			}
			else
				isOk = check_extraNoGoMap_ok(y1, x1, y2, x2, 0, i);
			if (isOk == 0)
				break;
		}
		if (isOk == 1) {
			if (pos1 == model.params.preferredPathOrtoPos[startLevel] && pos2 == model.params.preferredPathOrtoPos[slutLevel] && followPrefPathExact == 1) {
				y1a = y1;
				x1a = x1;
				for (i1 = 0; i1 < model.network.physicalLev[startLevel].npreferredPathPoints + 1; i1++) {
					if (i1 < model.network.physicalLev[startLevel].npreferredPathPoints) {
						y1b = model.network.physicalLev[startLevel].preferredPathPoint[i1].latitude().degrees();
						x1b = model.network.physicalLev[startLevel].preferredPathPoint[i1].longitude().degrees();
					}
					else {
						y1b = y2;
						x1b = x2;
					}
					isOk = check_noGoPolygons_ok(y1a, x1a, y1b, x1b);
					if (isOk == 0)
						break;
					y1a = y1b;
					x1a = x1b;
				}
			}
			else
				isOk = check_noGoPolygons_ok(y1, x1, y2, x2);
		}
		if (isOk == 0)
			isOk = -1;
	}

	//int isOk = check_physicalMap_ok(y1, x1, y2, x2p1.latitude().degrees(), p1.longitude().degrees(),
//	p2.latitude().degrees(), p2.longitude().degrees(), 0);

	if (printGlobal == 1)
		printf("++check_isPhysicalArcOK xy %.3lf %.3lf %.3lf %.3lf isOk %d isOk2 %d\n", x1, y1, x2, y2, isOk);
	//int ok1;
	//if (isOk == 1) {
	//	ok1 = check_feasibleNodeRasterA(y1, x1);
	//	// ok1 = check_feasibleNodeRasterA(p1.latitude().degrees(), p1.longitude().degrees());
	//	if (ok1 == 0)
	//		errlog("ERROR! Node not feasible but arc from it is. Level %d xy %.4lf %.4lf\n",
	//			level1, x1, y1);
	//	ok1 = check_feasibleNodeRasterA(y2, x2);
	//	// ok1 = check_feasibleNodeRasterA(p2.latitude().degrees(), p2.longitude().degrees());
	//	if (ok1 == 0)
	//		errlog("ERROR! Node not feasible but arc to it is. Level %d xy %.4lf %.4lf\n",
	//			level1, x2, y2);
	//}

	return isOk;
}

int getMostRestrictedArea(int startLevel, int slutLevel, int pos1, int pos2, int allowShortArc = 0)
{
	int areaNr, level1, level2;
	double x1, y1, x2, y2;
	int prefPath = 0;

	if (startLevel >= 0 && slutLevel >= 0) {
		if (pos1 == model.params.preferredPathOrtoPos[startLevel] && pos2 == model.params.preferredPathOrtoPos[slutLevel] && startLevel == slutLevel - 1
			&& (model.params.preferredPathStraightLineFeasibleFrom[startLevel] == 0 || model.params.max_changeDirection == 0)) {
			prefPath = 1;
		}
		x1 = model.network.physicalLev[startLevel].point_x[pos1];
		y1 = model.network.physicalLev[startLevel].point_y[pos1];
		x2 = model.network.physicalLev[slutLevel].point_x[pos2];
		y2 = model.network.physicalLev[slutLevel].point_y[pos2];
		level1 = startLevel - 1;
		if (startLevel == slutLevel - 1) {
			level2 = slutLevel + 1;
		}
		else {
			level2 = slutLevel;
		}
	}
	else {
		if (startLevel >= 0) {
			if ((model.network.channel[-slutLevel - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0) &&
				pos1 == model.params.preferredPathOrtoPos[startLevel] && model.network.channel[-slutLevel - 1].preferredPathPoint_posConnectTo >= 0)
				prefPath = 1;
		}
		if (slutLevel >= 0) {
			if ((model.network.channel[-startLevel - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0) &&
				pos2 == model.params.preferredPathOrtoPos[slutLevel] && model.network.channel[-startLevel - 1].preferredPathPoint_posConnectFrom >= 0)
				prefPath = 1;
		}

		if (slutLevel < 0) {
			if (startLevel >= 0) {
				x1 = model.network.physicalLev[startLevel].point_x[pos1];
				y1 = model.network.physicalLev[startLevel].point_y[pos1];
				//if (posPoly >= 0) {
				//	x2 = model.network.channel[-slutLevel - 1].polygonUse_x[pos2][posPoly];
				//	y2 = model.network.channel[-slutLevel - 1].polygonUse_y[pos2][posPoly];
				//}
				//else {
				if (pos2 == 1)
					pos2 = model.network.channel[-slutLevel - 1].nPoints - 1;
				x2 = model.network.channel[-slutLevel - 1].point_x[pos2];
				y2 = model.network.channel[-slutLevel - 1].point_y[pos2];
				//}
				level1 = startLevel - 1;
				level2 = startLevel;
			}
			else {
				if (pos1 == 1)
					pos1 = model.network.channel[-startLevel - 1].nPoints - 1;
				x1 = model.network.channel[-startLevel - 1].point_x[pos1];
				y1 = model.network.channel[-startLevel - 1].point_y[pos1];
				if(startLevel != slutLevel) {
					x2 = model.network.channel[-slutLevel - 1].point_x[pos2];
					y2 = model.network.channel[-slutLevel - 1].point_y[pos2];
				}
				else {
					x2 = model.network.channel[-slutLevel - 1].point_x[model.network.channel[-slutLevel - 1].nPoints - 1];
					y2 = model.network.channel[-slutLevel - 1].point_y[model.network.channel[-slutLevel - 1].nPoints - 1];
				}
			}
		}
		else {
			if (pos1 == 1)
				pos1 = model.network.channel[-startLevel - 1].nPoints - 1;
			x1 = model.network.channel[-startLevel - 1].point_x[pos1];
			y1 = model.network.channel[-startLevel - 1].point_y[pos1];
			x2 = model.network.physicalLev[slutLevel].point_x[pos2];
			y2 = model.network.physicalLev[slutLevel].point_y[pos2];
			level1 = slutLevel;
			level2 = slutLevel + 1;
		}
	}

	OGRLineString line;
	if(startLevel < 0 && startLevel == slutLevel)
		line = create_ogrLine_channel(-startLevel - 1, y1, x1, y2, x2);
	else {
		if (prefPath == 0)
			line = create_ogrLine_straight(y1, x1, y2, x2);
		else
			line = create_ogrLine_prefPath(startLevel, slutLevel, y1, x1, y2, x2);
	}
	areaNr = getMost_restrictedAreaLine(line);



	return areaNr;
}

int check_isPhysicalArcOK_alongPrefPath(int startLevel, int slutLevel, int pos1, int pos2, double noDataVal)
{
	int i, isOk;
	double x1, y1, x2, y2;

	// model.rasterData.physicalMap = model.physicalMapRaster.GetRasterBand(1);
	// float** rasterData = model.physicalMapRaster.GetRasterBand(1);
	double distance;

	for (i = 0; i <= model.network.physicalLev[startLevel].npreferredPathPoints; i++) {
		if (i == 0) {
			x1 = model.network.physicalLev[startLevel].point_x[pos1];
			y1 = model.network.physicalLev[startLevel].point_y[pos1];
		}
		else {
			x1 = x2;
			y1 = y2;
		}
		if (i < model.network.physicalLev[startLevel].npreferredPathPoints) {
			x2 = model.network.physicalLev[startLevel].preferredPathPoint[i].longitude().degrees();
			y2 = model.network.physicalLev[startLevel].preferredPathPoint[i].latitude().degrees();
		}
		else {
			x2 = model.network.physicalLev[slutLevel].point_x[pos2];
			y2 = model.network.physicalLev[slutLevel].point_y[pos2];
		}
		isOk = check_physicalMap_ok(y1, x1, y2, x2, 0);
		//printf("check_isPhysicalArcOK_alongPrefPath xy %.3lf %.3lf %.3lf %.3lf isOk %d\n",
		//	x1, y1, x2, y2, isOk);
		if (isOk == 0)
			break;
	}

	return isOk;
}

int identify_onlyPrefPathAllowedOnPhysicalLevel() {
	int i, i1, pos, nInArcsTot;

	for (i = model.network.nPhysicalLevels - 1; i >= 1; i--) {
		nInArcsTot = 0;
		for (pos = 0; pos < model.network.physicalLev[i].nPoints; pos++)
			nInArcsTot += model.network.physicalLev[i].nInNodes[pos];
		//if(nInArcsTot < 2)
		//	printf("level %d nInArcsPhysical %d\n", i, nInArcsTot);
		// 
		//pos = model.params.preferredPathOrtoPos[i];
		//if (pos == -1)
		//	continue; // pref path not always allowed so I can skip this step
		//if (model.network.physicalLev[i].nInNodes[pos] == 1) {
		//	// only pref path allowed to this node
		//}
	}
	//printf("all done with identifying\n");

	return 0;
}

int try_addPhysicalArcsLevel(int thisLevel, int pointPos, int nextLevel)
{
	int checkNextLevel, i2, arcOK, arcPos, i3;
	double distance;
	int nChangeFactor;

	checkNextLevel = 0;
	if (thisLevel == 3 && pointPos == 45)
		thisLevel = thisLevel;
	if (nextLevel > 0) { // next physical level
		if (nextLevel == 20)
			nextLevel = nextLevel;
		for (i2 = 0; i2 < model.network.physicalLev[nextLevel].nPoints; i2++) {

			if (i2 == 30)
				i2 = i2;
			if (model.network.physicalLev[nextLevel].allowedPoint[i2] == 0)
				continue; // not allowed node

			if (model.network.physicalLev[thisLevel].onlyPrefPath == 1) {
				if (model.params.preferredPathOrtoPos[nextLevel] != i2)
					continue; // only pref path ok
			}

			//if (abs(pointPos - model.network.physicalLev[thisLevel].nPoints / 2 - (
			//	i2 - model.network.physicalLev[nextLevel].nPoints / 2)) > model.params.max_changeDirection &&
			//	(thisLevel != 0 && nextLevel != model.network.nPhysicalLevels - 1))
			//	continue; // cannot turn too much...

			if (thisLevel == 0)
				nChangeFactor = model.params.max_changeDirection_factorStartEnd; // 2;
			else {
				if (model.network.physicalLev[thisLevel].restrictedLevel == 1 || model.network.physicalLev[nextLevel].restrictedLevel == 1)
					nChangeFactor = model.params.max_changeDirection_factorStartEnd; // 3; // 2;
				else
					nChangeFactor = 1;
			}

			if (abs(pointPos - model.network.physicalLev[thisLevel].nPoints / 2 - (
				i2 - model.network.physicalLev[nextLevel].nPoints / 2)) > model.params.max_changeDirection * nChangeFactor)
				continue; // cannot turn too much...

			if (thisLevel == 2 && pointPos == 23 && i2 == 29)
				i2 = i2;
			arcOK = 1;
			if (pointPos != model.params.preferredPathOrtoPos[thisLevel] || i2 != model.params.preferredPathOrtoPos[nextLevel] || 
				nextLevel != thisLevel + 1){
				arcOK = check_isPhysicalArcOK(thisLevel, nextLevel, pointPos, i2); // not a preferred path
			}
			else {
				if (model.network.physicalLev[thisLevel].requirePrefPathFeasible == 1) {
					arcOK = check_isPhysicalArcOK(thisLevel, nextLevel, pointPos, i2, 0, 1); // a preferred path but through a channel
					//printf("prefPath physical lev %d arcOK %d reqFeasible %d isFeasible %d\n",
					//	thisLevel, arcOK, model.network.physicalLev[thisLevel].requirePrefPathFeasible, arcOK);
				}
			}
			if (arcOK == 1) {
				if (nextLevel == 18 && i2 == 31)
					i2 = i2;
				arcPos = model.network.physicalLev[thisLevel].nOutNodes[pointPos];
				if (arcPos >= model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos]) {
					model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] += 5;
					model.network.physicalLev[thisLevel].outNode[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outNode[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					model.network.physicalLev[thisLevel].outLevel[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outLevel[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					for (int ii = model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] - 5; ii < model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos]; ii++)
						model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos][ii] = 0;
				}
				model.network.physicalLev[thisLevel].outNode[pointPos][arcPos] = i2;
				model.network.physicalLev[thisLevel].outLevel[pointPos][arcPos] = nextLevel;
				//model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos][arcPos] = getMostRestrictedArea(thisLevel, nextLevel, pointPos, i2);
				(model.network.physicalLev[thisLevel].nOutNodes[pointPos])++;
				(model.network.physicalLev[thisLevel].nOutNodesTot)++;
				(model.network.physicalLev[nextLevel].nInNodes[i2])++;
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
		if (thisLevel == 16)
			thisLevel = thisLevel;
 		for (i2 = 0; i2 < model.network.nChannels; i2++) {
			if(thisLevel==9 && pointPos==34)
				checkMinnesAnvandning(__LINE__);
			if (model.network.channel[i2].type == 0) { // normal corridor
				if (thisLevel < model.network.channel[i2].earliestStartLevel ||
					thisLevel > model.network.channel[i2].latestEndLevel)
					continue; // wrong area so cannot connect to this channel
			}
			else {
				// tss 
				if (thisLevel != model.network.channel[i2].earliestStartLevel)
					continue; // wrong area so cannot connect to this channel
				if (model.network.channel[i2].StartLevelOnlyPrefPath == 1 && model.params.preferredPathOrtoPos[thisLevel] != pointPos)
					continue; // only prefpath can attach to this tss
			}
			//if (model.network.channel[i2].allowedPoint[0] == 0)
			//	continue; // first node in channel not okay

			//if(model.network.channel[i2].nPolygonUsePoints[0] == 0){
			if (model.network.channel[i2].type == 0)
				i2 = i2;
			if (thisLevel == 9 && pointPos == 34)
				checkMinnesAnvandning(__LINE__);
			arcOK = check_isPhysicalArcOK(thisLevel, -i2 - 1, pointPos, 0, 1);
			if (thisLevel == 9 && pointPos == 34)
				checkMinnesAnvandning(__LINE__);
			if (arcOK == 1) {
				arcPos = model.network.physicalLev[thisLevel].nOutNodes[pointPos];
				if (arcPos >= model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos]) {
					model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] += 5;
					model.network.physicalLev[thisLevel].outNode[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outNode[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					model.network.physicalLev[thisLevel].outLevel[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outLevel[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos] = (int*)realloc(
						model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
					for (int ii = model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] - 5; ii < model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos]; ii++)
						model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos][ii] = 0;
				}
				model.network.physicalLev[thisLevel].outNode[pointPos][arcPos] = 0;
				model.network.physicalLev[thisLevel].outLevel[pointPos][arcPos] = -i2 - 1;
				//model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos][arcPos] = getMostRestrictedArea(thisLevel, -i2 - 1, pointPos, 0, 1);
				if (thisLevel == 9 && pointPos == 34)
					checkMinnesAnvandning(__LINE__);
				if (model.network.channel[i2].type != 0) { // tss
					if (model.network.channel[i2].nConnectFrom >= model.network.channel[i2].nAllocConnectFrom) {
						model.network.channel[i2].nAllocConnectFrom += 100;
						model.network.channel[i2].connectFrom_outLevel = (int*)realloc(
							model.network.channel[i2].connectFrom_outLevel, model.network.channel[i2].nAllocConnectFrom * sizeof(int));
						model.network.channel[i2].connectFrom_outNode = (int*)realloc(
							model.network.channel[i2].connectFrom_outNode, model.network.channel[i2].nAllocConnectFrom * sizeof(int));
					}
					model.network.channel[i2].connectFrom_outLevel[model.network.channel[i2].nConnectFrom] = thisLevel;
					model.network.channel[i2].connectFrom_outNode[model.network.channel[i2].nConnectFrom] = pointPos;
					if (thisLevel == 9 && pointPos == 34)
						checkMinnesAnvandning(__LINE__);
					(model.network.channel[i2].nConnectFrom)++;
				}
				if (thisLevel == 9 && pointPos == 34)
					checkMinnesAnvandning(__LINE__);
				(model.network.physicalLev[thisLevel].nOutNodes[pointPos])++;
				(model.network.physicalLev[thisLevel].nOutNodesTot)++;
				if (thisLevel == 9 && pointPos == 34)
					checkMinnesAnvandning(__LINE__);
			}
			else {
				if (model.params.preferredPathOrtoPos[thisLevel] == pointPos && model.network.channel[i2].bastStartLevel == thisLevel &&
					model.network.channel[i2].type == 0) {
					model.network.channel[i2].straightArcFeasible_toChannelFromPrefPath = 0;
					// add an arc from pref path to channel since the pref path needs to be able to use the channel if it passes it
					arcPos = model.network.physicalLev[thisLevel].nOutNodes[pointPos];
					if (arcPos >= model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos]) {
						model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] += 5;
						model.network.physicalLev[thisLevel].outNode[pointPos] = (int*)realloc(
							model.network.physicalLev[thisLevel].outNode[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
						model.network.physicalLev[thisLevel].outLevel[pointPos] = (int*)realloc(
							model.network.physicalLev[thisLevel].outLevel[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
						model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos] = (int*)realloc(
							model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
						model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos] = (int*)realloc(
							model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos], model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] * sizeof(int));
						for (int ii = model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos] - 5; ii < model.network.physicalLev[thisLevel].nAllocOutNodes[pointPos]; ii++)
							model.network.physicalLev[thisLevel].outNoNormalArc_useTSS[pointPos][ii] = 0;
					}

					model.network.physicalLev[thisLevel].outNode[pointPos][arcPos] = 0;
					model.network.physicalLev[thisLevel].outLevel[pointPos][arcPos] = -i2 - 1;
					//model.network.physicalLev[thisLevel].outRestrictedAreaNr[pointPos][arcPos] = getMostRestrictedArea(thisLevel, -i2 - 1, pointPos, 0, 1);
					model.network.channel[i2].connectFrom_outLevel[model.network.channel[i2].nConnectFrom] = thisLevel;
					model.network.channel[i2].connectFrom_outNode[model.network.channel[i2].nConnectFrom] = pointPos;
					(model.network.channel[i2].nConnectFrom)++;
					(model.network.physicalLev[thisLevel].nOutNodes[pointPos])++;
					(model.network.physicalLev[thisLevel].nOutNodesTot)++;
					if (thisLevel == 9 && pointPos == 34)
						checkMinnesAnvandning(__LINE__);
				}
			}
		
		}
	}
	if (thisLevel == 9 && pointPos == 34)
		checkMinnesAnvandning(__LINE__);

	return checkNextLevel;
}

int try_addPhysicalArcsFromChannel(int toLevel)
{
	int i1, arcOK, pos, cNr, arcPos, i3;

	for (cNr = 0; cNr < model.network.nChannels; cNr++) {
		if (model.network.channel[cNr].type == 0) { // normal corridor
			if (model.network.channel[cNr].earliestStartLevel > toLevel)
				continue;
			if (model.network.channel[cNr].latestEndLevel < toLevel)
				continue;
		}
		else {
			// tss
			if (cNr == 1)
				cNr = cNr;
			if (model.network.channel[cNr].latestEndLevel != toLevel) {
				if (model.network.channel[cNr].latestEndLevel != toLevel - 1 || model.network.channel[cNr].bastEndDist > model.params.basDistArcs * 0.3)
					continue;
			}
		}
		pos = model.network.channel[cNr].nPoints - 1;
		if (model.network.channel[cNr].type == 0)
			cNr = cNr;

		for (i1 = 0; i1 < model.network.physicalLev[toLevel].nPoints; i1++) {
			if (model.network.physicalLev[toLevel].allowedPoint[i1] == 0)
				continue; // node not okay

			if (model.network.channel[cNr].type == 1) { // tss
				if (model.network.channel[cNr].EndLevelOnlyPrefPath == 1 && model.params.preferredPathOrtoPos[toLevel] != i1)
					continue; // only prefpath can attach to this tss
			}

			if (model.params.max_changeDirection == 0 && model.params.preferredPathOrtoPos[toLevel] != i1)
				continue; // must go along pref path

			//if (model.network.channel[cNr].allowedPoint[pos] == 0)
			//	continue; // last node in channel not okay

			//if (model.network.channel[i2].nPolygonUsePoints[0] == 0) {
			arcOK = check_isPhysicalArcOK(-cNr - 1, toLevel, pos, i1, 1);

			if (toLevel == 18 && i1 == 31)
				i1 = i1;
			if (arcOK == 1) {
				//printf("--add arc to physLev %d pointPos %d from channel %d\n", toLevel, i1, cNr);
				arcPos = model.network.channel[cNr].nOutNodes;
				if (arcPos >= model.network.channel[cNr].nAllocOutNodes) {
					model.network.channel[cNr].nAllocOutNodes += 50;
					model.network.channel[cNr].outNode = (int*)realloc(model.network.channel[cNr].outNode, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
					model.network.channel[cNr].outLevel = (int*)realloc(model.network.channel[cNr].outLevel, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
					model.network.channel[cNr].outRestrictedAreaNr = (int*)realloc(model.network.channel[cNr].outRestrictedAreaNr, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
					model.network.channel[cNr].outNoNormalArc_useTSS = (int*)realloc(model.network.channel[cNr].outNoNormalArc_useTSS, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
					for (int ii = model.network.channel[cNr].nAllocOutNodes - 50; ii < model.network.channel[cNr].nAllocOutNodes; ii++)
						model.network.channel[cNr].outNoNormalArc_useTSS[ii] = 0;
				}
				model.network.channel[cNr].outNode[arcPos] = i1;
				//model.network.channel[cNr].outPolyPoint[arcPos] = -1;
				model.network.channel[cNr].outLevel[arcPos] = toLevel;
				//model.network.channel[cNr].outRestrictedAreaNr[arcPos] = getMostRestrictedArea(-cNr - 1, toLevel, pos, i1);

				if (model.network.channel[cNr].type != 0) { // tss
					if (model.network.channel[cNr].nConnectTo >= model.network.channel[cNr].nAllocConnectTo) {
						model.network.channel[cNr].nAllocConnectTo += 100;
						model.network.channel[cNr].connectTo_outLevel = (int*)realloc(
							model.network.channel[cNr].connectTo_outLevel, model.network.channel[cNr].nAllocConnectTo * sizeof(int));
						model.network.channel[cNr].connectTo_outNode = (int*)realloc(
							model.network.channel[cNr].connectTo_outNode, model.network.channel[cNr].nAllocConnectTo * sizeof(int));
					}
					model.network.channel[cNr].connectTo_outLevel[model.network.channel[cNr].nConnectTo] = toLevel;
					model.network.channel[cNr].connectTo_outNode[model.network.channel[cNr].nConnectTo] = i1;
					(model.network.channel[cNr].nConnectTo)++;
				}

				(model.network.channel[cNr].nOutNodes)++;
				(model.network.physicalLev[toLevel].nInNodes[i1])++;
				if (toLevel >= 11)
					checkMinnesAnvandning(__LINE__);
			}
			else {
				//printf("++ortoPos %d i1 %d bastEndLevel %d toLevel %d\n", 
				//	model.params.preferredPathOrtoPos[toLevel], i1, model.network.channel[cNr].bastEndLevel, toLevel);
				if (model.params.preferredPathOrtoPos[toLevel] == i1 && model.network.channel[cNr].bastEndLevel == toLevel) {
					// add an arc from channel to pref path since the pref path needs to be able to use the channel if it passes it
					//printf("--add arc2 to physLev %d pointPos %d from channel %d\n", toLevel, i1, cNr);
					model.network.channel[cNr].straightArcFeasible_fromChannelToPrefPath = 0;
					arcPos = model.network.channel[cNr].nOutNodes;
					if (arcPos >= model.network.channel[cNr].nAllocOutNodes) {
						model.network.channel[cNr].nAllocOutNodes += 50;
						model.network.channel[cNr].outNode = (int*)realloc(model.network.channel[cNr].outNode, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
						model.network.channel[cNr].outLevel = (int*)realloc(model.network.channel[cNr].outLevel, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
						model.network.channel[cNr].outRestrictedAreaNr = (int*)realloc(model.network.channel[cNr].outRestrictedAreaNr, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
						model.network.channel[cNr].outNoNormalArc_useTSS = (int*)realloc(model.network.channel[cNr].outNoNormalArc_useTSS, model.network.channel[cNr].nAllocOutNodes * sizeof(int));
						for (int ii = model.network.channel[cNr].nAllocOutNodes - 50; ii < model.network.channel[cNr].nAllocOutNodes; ii++)
							model.network.channel[cNr].outNoNormalArc_useTSS[ii] = 0;
					}
					model.network.channel[cNr].outNode[arcPos] = i1;
					//model.network.channel[cNr].outPolyPoint[arcPos] = -1;
					model.network.channel[cNr].outLevel[arcPos] = toLevel;
					//model.network.channel[cNr].outRestrictedAreaNr[arcPos] = getMostRestrictedArea(-cNr - 1, toLevel, pos, i1);

					if (model.network.channel[cNr].type != 0) { // tss
						if (model.network.channel[cNr].nConnectTo >= model.network.channel[cNr].nAllocConnectTo) {
							model.network.channel[cNr].nAllocConnectTo += 100;
							model.network.channel[cNr].connectTo_outLevel = (int*)realloc(
								model.network.channel[cNr].connectTo_outLevel, model.network.channel[cNr].nAllocConnectTo * sizeof(int));
							model.network.channel[cNr].connectTo_outNode = (int*)realloc(
								model.network.channel[cNr].connectTo_outNode, model.network.channel[cNr].nAllocConnectTo * sizeof(int));
						}
						model.network.channel[cNr].connectTo_outLevel[model.network.channel[cNr].nConnectTo] = toLevel;
						model.network.channel[cNr].connectTo_outNode[model.network.channel[cNr].nConnectTo] = i1;
						(model.network.channel[cNr].nConnectTo)++;
					}

					(model.network.channel[cNr].nOutNodes)++;
					(model.network.physicalLev[toLevel].nInNodes[i1])++;
				}
			}

		}
	}
	return 0;
}

int try_addPhysicalArcsBetweenChannels()
{
	int i1, arcOK, pos, cNr1, cNr2, arcPos, i3;

	for (cNr1 = 0; cNr1 < model.network.nChannels; cNr1++) {
		for (cNr2 = 0; cNr2 < model.network.nChannels; cNr2++) {
			if (cNr1 == cNr2)
				continue;
			if (model.network.channel[cNr1].bastEndLevel >= 0 && model.network.channel[cNr2].bastStartLevel >= 0) {
				if (model.network.physicalLev[model.network.channel[cNr1].bastEndLevel].legNr !=
					model.network.physicalLev[model.network.channel[cNr2].bastStartLevel].legNr &&
					model.params.legProperties[model.network.physicalLev[model.network.channel[cNr1].bastEndLevel].legNr].endNode_exact == 1)
					continue; // different legs, don't connect the tss if the end point of first leg has to be visited exactly
			}
			else
				continue; // could not figure out the levels, I skip the connection

			pos = model.network.channel[cNr1].nPoints - 1;
			if (cNr1 == 7 && cNr2 == 6)
				cNr1 = cNr1;
			if (model.network.channel[cNr1].bastEndLevel > model.network.channel[cNr2].bastEndLevel ||
				model.network.channel[cNr1].bastStartLevel > model.network.channel[cNr2].bastStartLevel)
				continue; // going backwards...
			arcOK = check_isPhysicalArcOK(-cNr1 - 1, -cNr2 - 1, pos, 0, 1);
			if (arcOK == 1) {
				//printf("--add arc to physLev %d pointPos %d from channel %d\n", toLevel, i1, cNr);
				arcPos = model.network.channel[cNr1].nOutNodes;
				if (arcPos >= model.network.channel[cNr1].nAllocOutNodes) {
					model.network.channel[cNr1].nAllocOutNodes += 50;
					model.network.channel[cNr1].outNode = (int*)realloc(model.network.channel[cNr1].outNode, model.network.channel[cNr1].nAllocOutNodes * sizeof(int));
					model.network.channel[cNr1].outLevel = (int*)realloc(model.network.channel[cNr1].outLevel, model.network.channel[cNr1].nAllocOutNodes * sizeof(int));
					model.network.channel[cNr1].outRestrictedAreaNr = (int*)realloc(model.network.channel[cNr1].outRestrictedAreaNr, model.network.channel[cNr1].nAllocOutNodes * sizeof(int));
					model.network.channel[cNr1].outNoNormalArc_useTSS = (int*)realloc(model.network.channel[cNr1].outNoNormalArc_useTSS, model.network.channel[cNr1].nAllocOutNodes * sizeof(int));
					for (int ii = model.network.channel[cNr1].nAllocOutNodes - 50; ii < model.network.channel[cNr1].nAllocOutNodes; ii++)
						model.network.channel[cNr1].outNoNormalArc_useTSS[ii] = 0;
				}
				model.network.channel[cNr1].outNode[arcPos] = 0;
				//model.network.channel[cNr].outPolyPoint[arcPos] = -1;
				model.network.channel[cNr1].outLevel[arcPos] = -cNr2 - 1;
				//model.network.channel[cNr1].outRestrictedAreaNr[arcPos] = getMostRestrictedArea(-cNr1 - 1, -cNr2 - 1, pos, 0, 1);

				if (model.network.channel[cNr1].type != 0) { // tss
					if (model.network.channel[cNr1].nConnectTo >= model.network.channel[cNr1].nAllocConnectTo) {
						model.network.channel[cNr1].nAllocConnectTo += 100;
						model.network.channel[cNr1].connectTo_outLevel = (int*)realloc(
							model.network.channel[cNr1].connectTo_outLevel, model.network.channel[cNr1].nAllocConnectTo * sizeof(int));
						model.network.channel[cNr1].connectTo_outNode = (int*)realloc(
							model.network.channel[cNr1].connectTo_outNode, model.network.channel[cNr1].nAllocConnectTo * sizeof(int));
					}
					model.network.channel[cNr1].connectTo_outLevel[model.network.channel[cNr1].nConnectTo] = -cNr2 - 1;
					model.network.channel[cNr1].connectTo_outNode[model.network.channel[cNr1].nConnectTo] = 0;
					(model.network.channel[cNr1].nConnectTo)++;
				}
				if (model.network.channel[cNr2].type != 0) { // tss
					if (model.network.channel[cNr2].nConnectFrom >= model.network.channel[cNr2].nAllocConnectFrom) {
						model.network.channel[cNr2].nAllocConnectFrom += 100;
						model.network.channel[cNr2].connectFrom_outLevel = (int*)realloc(
							model.network.channel[cNr2].connectFrom_outLevel, model.network.channel[cNr2].nAllocConnectFrom * sizeof(int));
						model.network.channel[cNr2].connectFrom_outNode = (int*)realloc(
							model.network.channel[cNr2].connectFrom_outNode, model.network.channel[cNr2].nAllocConnectFrom * sizeof(int));
					}
					model.network.channel[cNr2].connectFrom_outLevel[model.network.channel[cNr2].nConnectFrom] = -cNr1 - 1;
					model.network.channel[cNr2].connectFrom_outNode[model.network.channel[cNr2].nConnectFrom] = 1;
					(model.network.channel[cNr2].nConnectFrom)++;
				}

				(model.network.channel[cNr1].nOutNodes)++;
			}
		}
	}
	return 0;
}

int identify_mostRestrictedAreaArcs() {
	int i, i1, i2, pos2, lev2, cNr, pos, toLevel;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if(i==28 && i1 ==35)
				i = i;
			for (i2 = 0; i2 < model.network.physicalLev[i].nOutNodes[i1]; i2++) {
				pos2 = model.network.physicalLev[i].outNode[i1][i2];
				lev2 = model.network.physicalLev[i].outLevel[i1][i2];
				if (pos2 == 34)
					pos2 = pos2;
				if (lev2 >= 0)
					model.network.physicalLev[i].outRestrictedAreaNr[i1][i2] = getMostRestrictedArea(i, lev2, i1, pos2);
				else
					model.network.physicalLev[i].outRestrictedAreaNr[i1][i2] = getMostRestrictedArea(i, lev2, i1, pos2, 1);
			}
		}
	}

	for (cNr = 0; cNr < model.network.nChannels; cNr++) {
		model.network.channel[cNr].outRestrictedAreaNr_channel = getMostRestrictedArea(-cNr - 1, -cNr - 1, 0, 1);
		pos = model.network.channel[cNr].nPoints - 1;
		for (i1 = 0; i1 < model.network.channel[cNr].nOutNodes; i1++) {
			toLevel = model.network.channel[cNr].outLevel[i1];
			if (toLevel >= 0)
				model.network.channel[cNr].outRestrictedAreaNr[i1] = getMostRestrictedArea(-cNr - 1, toLevel, pos, model.network.channel[cNr].outNode[i1]);
			else
				model.network.channel[cNr].outRestrictedAreaNr[i1] = getMostRestrictedArea(-cNr - 1, toLevel, pos, 0, 1);
		}
	}

	return 0;
}

int isOtherTssCloser(int fromLev, int fromNode, int arcPos) {
	double x0, y0, x1, y1, xBas = 0, yBas = 0, distNu, distBas;
	int nOutNodes, i, pos, toLev, outLevel, toNode;
	if (fromLev >= 0) {
		x0 = model.network.physicalLev[fromLev].point_x[fromNode];
		y0 = model.network.physicalLev[fromLev].point_y[fromNode];
		nOutNodes = model.network.physicalLev[fromLev].nOutNodes[fromNode];
	}
	else {
		x0 = model.network.channel[-fromLev - 1].point_x[model.network.channel[-fromLev - 1].nPoints - 1];
		y0 = model.network.channel[-fromLev - 1].point_y[model.network.channel[-fromLev - 1].nPoints - 1];
		nOutNodes = model.network.channel[-fromLev - 1].nOutNodes;
	}

	for (i = -1; i < nOutNodes; i++) {
		if (i == arcPos)
			continue;
		if (i == -1)
			pos = arcPos;
		else
			pos = i;
		if (fromLev >= 0)
			outLevel = model.network.physicalLev[fromLev].outLevel[fromNode][pos];
		else
			outLevel = model.network.channel[-fromLev - 1].outLevel[pos];
		if (fromLev >= 0) {
			toLev = model.network.physicalLev[fromLev].outLevel[fromNode][pos];
			toNode = model.network.physicalLev[fromLev].outNode[fromNode][pos];
		}
		else {
			toLev = model.network.channel[-fromLev - 1].outLevel[pos];
			toNode = model.network.channel[-fromLev - 1].outNode[pos];
		}
		if (toLev >= 0)
			continue; // not a tss
		x1 = model.network.channel[-toLev - 1].point_x[toNode];
		y1 = model.network.channel[-toLev - 1].point_y[toNode];
		distNu = estimateLargeCircleDistance_km(y0, x0, y1, x1);
		if (i == -1) {
			distBas = distNu;
			yBas = y1;
			xBas = x1;
		}
		else {
			if (distNu > distBas)
				continue; // only interested in other tss that starts before
			distNu += model.network.channel[-toLev - 1].distance_km;
			x1 = model.network.channel[-toLev - 1].point_x[model.network.channel[-toLev - 1].nPoints - 1];
			y1 = model.network.channel[-toLev - 1].point_y[model.network.channel[-toLev - 1].nPoints - 1];
			distNu += estimateLargeCircleDistance_km(y1, x1, yBas, xBas);
			if (distNu < distBas + model.params.tss_attractionDistance_km)
				return 1;
		}
	}



	return 0;
}

// Detailed tss debugging, both off by default as they log per arc and per evaluated bypass.
int DIAG_TSS_CNR = -1; // tss channel number to log every bypass decision and blocked arc for, -1 = off
int DIAG_TSS_STARTLEVEL = -1; // set DIAG_TSS_CNR automatically for the tss starting at this level, -1 = off
int DIAG_LEVEL_DUMP = -1; // dump the arcs from this physical level to the next one, -1 = off

// When a bypass chain around a tss is too long, all arcs of the chain used to be blocked, also the
// arcs after the tss has ended. Those arcs are not a bypass of the tss and are shared with routes that
// have nothing to do with the tss, so blocking them removes far more than intended (it can leave nodes
// without any outgoing arc). Only arcs inside the level span of the tss are blocked now.
// Set "tss_blockBeyondSpan":1 in the input json to get the old behaviour back.
int TSS_BLOCK_BEYOND_SPAN = 0;
int CURRENT_TSS_BASTENDLEVEL = -1; // bastEndLevel of the tss being processed in identify_arcsNoUse_tss

int recursive_arcsLevels(int niva, int fromLev, int fromNode, int toLevEnd, int toNodeEnd, double dist_max, double distTot, int prefPathStart, int nivaBas, int keepPrefPath) { // not used
	int i, toLev, toNode, arcPos, i1, nOutNodes, outLevel, maxLevel;
	double distNu, x0, y0, x1, y1;

	if (fromLev >= 0) {
		x0 = model.network.physicalLev[fromLev].point_x[fromNode];
		y0 = model.network.physicalLev[fromLev].point_y[fromNode];
		nOutNodes = model.network.physicalLev[fromLev].nOutNodes[fromNode];
	}
	else {
		x0 = model.network.channel[-fromLev - 1].point_x[model.network.channel[-fromLev - 1].nPoints - 1];
		y0 = model.network.channel[-fromLev - 1].point_y[model.network.channel[-fromLev - 1].nPoints - 1];
		nOutNodes = model.network.channel[-fromLev - 1].nOutNodes;
	}
	if(toLevEnd >= 0)
		maxLevel = toLevEnd;
	else
		maxLevel = model.network.channel[-toLevEnd - 1].bastStartLevel;

	for (i = 0; i < nOutNodes; i++) {
		if (fromLev >= 0) 
			outLevel = model.network.physicalLev[fromLev].outLevel[fromNode][i];
		else
			outLevel = model.network.channel[-fromLev - 1].outLevel[i];
		if (outLevel <= maxLevel) {
			if (fromLev >= 0) {
				if (model.network.physicalLev[fromLev].outLevel[fromNode][i] == toLevEnd &&
					model.network.physicalLev[fromLev].outNode[fromNode][i] != toNodeEnd)
					continue; // not the right end node so skip this one
				toLev = model.network.physicalLev[fromLev].outLevel[fromNode][i];
				if (toLev < 0 && toLevEnd != toLev)
					continue; // do not connect to other channels
				toNode = model.network.physicalLev[fromLev].outNode[fromNode][i];
			}
			else {
				if (model.network.channel[-fromLev - 1].outLevel[i] == toLevEnd &&
					model.network.channel[-fromLev - 1].outNode[i] != toNodeEnd)
					continue; // not the right end node so skip this one
				toLev = model.network.channel[-fromLev - 1].outLevel[i];
				if (toLev < 0 && toLevEnd != toLev)
					continue; // do not connect to other channels
				toNode = model.network.channel[-fromLev - 1].outNode[i];
			}
			if (toLev >= 0) {
				x1 = model.network.physicalLev[toLev].point_x[toNode];
				y1 = model.network.physicalLev[toLev].point_y[toNode];
			}
			else {
				x1 = model.network.channel[-toLev - 1].point_x[toNode];
				y1 = model.network.channel[-toLev - 1].point_y[toNode];
			}
			distNu = estimateLargeCircleDistance_km(y0, x0, y1, x1);
			// distTot += distNu;
			model.temp_data.arr_fromLev[niva] = fromLev;
			model.temp_data.arr_fromNode[niva] = fromNode;
			model.temp_data.arr_fromPos[niva] = i;
			//printf("fromLev %d fromNode %d fromPos %d nAlloc %d nAlloc2 %d\n", fromLev, fromNode, i,
			//	model.network.physicalLev[fromLev].nPoints, model.network.physicalLev[fromLev + 1].nPoints);
			if (outLevel != toLevEnd && niva + 1 < model.temp_data.nMax_recursive_arcsLevels &&
				prefPathStart == 1) {
				recursive_arcsLevels(niva + 1, toLev, toNode, toLevEnd, toNodeEnd, dist_max, distTot + distNu, prefPathStart, nivaBas, keepPrefPath);
			}
			else {
				// done, compare the costs
				if (DIAG_TSS_CNR >= 0 && toLevEnd == toLev && toNodeEnd == toNode) {
					errlog("      [tss %d] bypass ending at level %d node %d: %.1lf km vs threshold %.1lf km -> %s\n",
						DIAG_TSS_CNR, toLev, toNode, distTot + distNu, dist_max,
						(distTot + distNu >= dist_max) ? "BLOCKED" : "allowed (shorter than tss)");
				}
				if (distTot + distNu >= dist_max && toLevEnd == toLev && toNodeEnd == toNode) {
					// do not use this set of arcs as they are too close to tss distance, set to not be used
					for (i1 = 0; i1 < niva + 1; i1++) {
						fromLev = model.temp_data.arr_fromLev[i1];
						fromNode = model.temp_data.arr_fromNode[i1];
						arcPos = model.temp_data.arr_fromPos[i1];
						if (fromLev == 9 && fromNode == 26 && arcPos >= 4)
							i1 = i1;
						if (i1 < nivaBas && fromLev == model.temp_data.arrBas_fromLev[i1] &&
							fromNode == model.temp_data.arrBas_fromNode[i1] && arcPos == model.temp_data.arrBas_fromPos[i1])
							continue; // used in the tss route
						if (TSS_BLOCK_BEYOND_SPAN == 0 && fromLev >= 0 && CURRENT_TSS_BASTENDLEVEL >= 0 &&
							fromLev >= CURRENT_TSS_BASTENDLEVEL)
							continue; // arc starts after the tss has ended, it is not a bypass of this tss
						if (fromLev >= 0) {
							toLev = model.network.physicalLev[fromLev].outLevel[fromNode][arcPos];
							if (toLev < 0) {
								int otherCloser = isOtherTssCloser(fromLev, fromNode, arcPos);
								if(otherCloser == 0)
									continue; // do not remove connection to channel
							}
							toNode = model.network.physicalLev[fromLev].outNode[fromNode][arcPos];
							if (fromLev == 15 && fromNode == 23 && toLev == 16 && toNode == 23)
								toNode = toNode;
							if (fromLev == 33 && fromNode == 23 &&
								model.network.physicalLev[fromLev].outLevel[fromNode][arcPos] == 34 &&
								model.network.physicalLev[fromLev].outNode[fromNode][arcPos] == 23)
								arcPos = arcPos;

							if (model.params.preferredPathOrtoPos[fromLev] == fromNode &&
								toLev >= 0 && model.params.preferredPathOrtoPos[toLev] == toNode) {
								if (fromNode == 25 && toLev == 28 && toNode == 23)
									toNode = toNode;
								// require to follow pref path exactly
								model.params.preferredPathStraightLineFeasibleFrom[fromLev] = 0;
								if (model.network.physicalLev[fromLev].outNoNormalArc_useTSS[fromNode][arcPos] == 1)
									model.network.physicalLev[fromLev].outNoNormalArc_useTSS[fromNode][arcPos] = 0;
								continue; // do not remove arcs along prefPath for certain tss (given for the tss)
							}

							//if ((keepPrefPath == 1 || model.params.preferredPathStraightLineFeasibleFrom[fromLev] == 0) && 
							//	model.params.preferredPathOrtoPos[fromLev] == fromNode &&
							//	toLev >= 0 && model.params.preferredPathOrtoPos[toLev] == toNode) {
							//	if (fromNode == 25 && toLev == 28 && toNode == 23)
							//		toNode = toNode;
							//	// require to follow pref path exactly
							//	model.params.preferredPathStraightLineFeasibleFrom[fromLev] = 0;
							//	if (model.network.physicalLev[fromLev].outRestrictedAreaNr[fromNode][arcPos] == -2)
							//		model.network.physicalLev[fromLev].outRestrictedAreaNr[fromNode][arcPos] = -1;
							//	continue; // do not remove arcs along prefPath for certain tss (given for the tss)
							//}
							if (fromLev == 4)
								toNode = toNode;
							if (DIAG_TSS_CNR >= 0 && model.network.physicalLev[fromLev].outNoNormalArc_useTSS[fromNode][arcPos] == 0)
								errlog("        [tss %d] blocking arc level %d node %d -> level %d node %d (chain step %d of %d, chain ends at level %d node %d)\n",
									DIAG_TSS_CNR, fromLev, fromNode, toLev, toNode, i1, niva, toLevEnd, toNodeEnd);
							model.network.physicalLev[fromLev].outNoNormalArc_useTSS[fromNode][arcPos] = 1;
						}
						// do not remove connection from channel here, it's done elsewhere
						//else
						//	model.network.channel[-fromLev - 1].outRestrictedAreaNr[arcPos] = -2;
						// 
						//printf("niva_i1 %d arc not OK lev %d fromNode %d arcPos %d nAllocTimeInt %d\n", i1, fromLev, fromNode, arcPos,
						//	model.network.physicalLev[fromLev].nPoints);
					}
				}
			}
		}
	}


	return 0;
}

int identify_arcsNoUse_tss_levels(int level, int cNr, double dist_tss_ext, int fromLev, int fromNode, int pos2, int prefPathStart, int keepPrefPath) { // not used 
	int toLev, i1, i2, toNode, toLev2, toNode2, fromLev2, fromNode2, pos_prefPathEnd, keepPrefPath2;
	double y0, x0, y1, x1, dist, x2, y2;

	toLev = model.network.channel[cNr].connectTo_outLevel[pos2];
	toNode = model.network.channel[cNr].connectTo_outNode[pos2];

	// totDist = p1.distanceTo(p2) / 1000;
	if (toLev >= 0) {
		x1 = model.network.physicalLev[toLev].point_x[toNode];
		y1 = model.network.physicalLev[toLev].point_y[toNode];
	}
	else {
		x1 = model.network.channel[-toLev - 1].point_x[toNode];
		y1 = model.network.channel[-toLev - 1].point_y[toNode];
		if(keepPrefPath == 0)
			keepPrefPath = model.network.channel[-toLev - 1].keepPrefPath_tss;
	}
	x0 = model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1];
	y0 = model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1];
	dist = estimateLargeCircleDistance_km(y0, x0, y1, x1);
	dist_tss_ext += dist;

	model.temp_data.arrBas_fromLev[level] = fromLev;
	model.temp_data.arrBas_fromNode[level] = fromNode;
	model.temp_data.arrBas_fromPos[level] = pos2;

	recursive_arcsLevels(0, fromLev, fromNode, toLev, toNode, dist_tss_ext, 0.0, prefPathStart, level, keepPrefPath);

	if (toLev == fromLev + 1 && fromLev >= 0 && prefPathStart == 1) { // preferably add a check that the tss is close to the end of the level as well
		// check one leavel after as well along preferred path
		pos_prefPathEnd = -1;
		for (i1 = 0; i1 < model.network.physicalLev[toLev].nOutNodes[toNode]; i1++) {
			toLev2 = model.network.physicalLev[toLev].outLevel[toNode][i1];
			toNode2 = model.network.physicalLev[toLev].outNode[toNode][i1];
			if (toLev2 >= 0) {
				x2 = model.network.physicalLev[toLev2].point_x[toNode2];
				y2 = model.network.physicalLev[toLev2].point_y[toNode2];
				dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				model.temp_data.arrBas_fromLev[level + 1] = toLev;
				model.temp_data.arrBas_fromNode[level + 1] = toNode;
				model.temp_data.arrBas_fromPos[level + 1] = i1;
				recursive_arcsLevels(0, fromLev, fromNode, toLev2, toNode2, dist_tss_ext + dist, 0.0, prefPathStart, level + 1, keepPrefPath);
				if (model.params.preferredPathOrtoPos[toLev2] == toNode2)
					pos_prefPathEnd = i1;
			}
		}

		// check one leavel before as well along preferred path
		if (pos_prefPathEnd >= 0) {
			toLev2 = model.network.physicalLev[toLev].outLevel[toNode][pos_prefPathEnd];
			toNode2 = model.network.physicalLev[toLev].outNode[toNode][pos_prefPathEnd];
			x2 = model.network.physicalLev[toLev2].point_x[toNode2];
			y2 = model.network.physicalLev[toLev2].point_y[toNode2];
			dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
			model.temp_data.arrBas_fromLev[level + 1] = toLev;
			model.temp_data.arrBas_fromNode[level + 1] = toNode;
			model.temp_data.arrBas_fromPos[level + 1] = i1;
			dist_tss_ext += dist;

			x1 = model.network.channel[cNr].point_x[0];
			y1 = model.network.channel[cNr].point_y[0];
			if (fromLev >= 0) {
				x0 = model.network.physicalLev[fromLev].point_x[fromNode];
				y0 = model.network.physicalLev[fromLev].point_y[fromNode];
			}
			else {
				x0 = model.network.channel[-fromLev - 1].point_x[model.network.channel[-fromLev - 1].nPoints - 1];
				y0 = model.network.channel[-fromLev - 1].point_y[model.network.channel[-fromLev - 1].nPoints - 1];
			}
			dist = estimateLargeCircleDistance_km(y0, x0, y1, x1);
			dist_tss_ext -= dist;

			for (i1 = 0; i1 < model.network.channel[cNr].nConnectFrom; i1++) {
				fromLev2 = model.network.channel[cNr].connectFrom_outLevel[i1];
				fromNode2 = model.network.channel[cNr].connectFrom_outNode[i1];
				keepPrefPath2 = keepPrefPath;
				if (fromLev2 >= 0) {
					x0 = model.network.physicalLev[fromLev2].point_x[fromNode2];
					y0 = model.network.physicalLev[fromLev2].point_y[fromNode2];
				}
				else {
					x0 = model.network.channel[-fromLev2 - 1].point_x[model.network.channel[-fromLev2 - 1].nPoints - 1];
					y0 = model.network.channel[-fromLev2 - 1].point_y[model.network.channel[-fromLev2 - 1].nPoints - 1];
					if (keepPrefPath == 0)
						keepPrefPath2 = model.network.channel[-fromLev2 - 1].keepPrefPath_tss;
				}
				dist = estimateLargeCircleDistance_km(y0, x0, y1, x1);

				model.temp_data.arrBas_fromLev[level] = fromLev2;
				model.temp_data.arrBas_fromNode[level] = fromNode2;
				model.temp_data.arrBas_fromPos[level] = i1;
				recursive_arcsLevels(0, fromLev2, fromNode2, toLev2, toNode2, dist_tss_ext + dist, 0.0, prefPathStart, level + 1, keepPrefPath2);
			}

		}


		for (i1 = 0; i1 < model.network.physicalLev[toLev].nOutNodes[toNode]; i1++) {
			toLev2 = model.network.physicalLev[toLev].outLevel[toNode][i1];
			toNode2 = model.network.physicalLev[toLev].outNode[toNode][i1];
			if (toLev2 >= 0) {
				x2 = model.network.physicalLev[toLev2].point_x[toNode2];
				y2 = model.network.physicalLev[toLev2].point_y[toNode2];
				dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				model.temp_data.arrBas_fromLev[level + 1] = toLev;
				model.temp_data.arrBas_fromNode[level + 1] = toNode;
				model.temp_data.arrBas_fromPos[level + 1] = i1;
				recursive_arcsLevels(0, fromLev, fromNode, toLev2, toNode2, dist_tss_ext + dist, 0.0, prefPathStart, level + 1, keepPrefPath);
			}
		}


	}

	return 0;
}

int recursive_arcsNoUse_tss_tss(int cNr, double dist_tss_ext, int fromLev, int fromNode, int pos2, int cNrLevel, int prefPathStart, int keepPrefPath) { // not used
	int toLev, i1, i2, toNode, cNr2;
	double y0, x0, y1, x1, dist;

	if (pos2 >= 0)
		cNr2 = -model.network.channel[cNr].connectTo_outLevel[pos2] - 1;
	else
		cNr2 = cNr;
	if (cNr2 < 0)
		return 0; // only continue if it is a corridor
	if (model.network.channel[cNr2].type == 0)
		return 0; // not a tss

	for (i1 = 0; i1 < cNrLevel; i1++) {
		if (cNr2 == model.temp_data.arr_cNr[i1])
			return 0; // do not go back to a corridor that has already been checked
	}

	if (keepPrefPath == 0)
		keepPrefPath = model.network.channel[cNr2].keepPrefPath_tss;

	if (model.network.channel[cNr2].bastEndLevel > CURRENT_TSS_BASTENDLEVEL)
		CURRENT_TSS_BASTENDLEVEL = model.network.channel[cNr2].bastEndLevel; // chained tss, the span reaches further

	if (pos2 >= 0) {
		identify_arcsNoUse_tss_levels(cNrLevel - 1, cNr, dist_tss_ext, fromLev, fromNode, pos2, prefPathStart, keepPrefPath);
		x0 = model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1];
		y0 = model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1];
		x1 = model.network.channel[cNr2].point_x[0];
		y1 = model.network.channel[cNr2].point_y[0];
		dist = estimateLargeCircleDistance_km(y0, x0, y1, x1);
		dist_tss_ext += dist;
	}

	dist_tss_ext += model.network.channel[cNr2].distance_km;
	dist_tss_ext -= model.params.tss_attractionDistance_km;

	for (i2 = 0; i2 < model.network.channel[cNr2].nConnectTo; i2++) {
		if (model.network.channel[cNr2].connectTo_outLevel[i2] >= 0) {
			identify_arcsNoUse_tss_levels(cNrLevel - 1, cNr2, dist_tss_ext, fromLev, fromNode, i2, prefPathStart, keepPrefPath);
		}
		else {
			// connected to another channel .. handle this
			model.temp_data.arr_cNr[cNrLevel] = cNr2;
			recursive_arcsNoUse_tss_tss(cNr2, dist_tss_ext, fromLev, fromNode, i2, cNrLevel + 1, prefPathStart, keepPrefPath);
		}
		if(cNr==5)
			checkMinnesAnvandning(__LINE__);
	}
	return 0;
}

int remove_all_arcs_nonPrefPath_level(int lev, int alt) {
	int i1, i2b, i2, nextLev;

	for (i1 = 0; i1 < model.network.physicalLev[lev].nPoints; i1++) {
		for (i2b = 0; i2b < model.network.physicalLev[lev].nOutNodes[i1]; i2b++) {
			i2 = model.network.physicalLev[lev].outNode[i1][i2b];
			nextLev = model.network.physicalLev[lev].outLevel[i1][i2b];
			if (i1 == model.params.preferredPathOrtoPos[lev] && nextLev == lev + 1 &&
				i2 == model.params.preferredPathOrtoPos[nextLev])
				continue; // keep the preferred path
			if (alt == 0 && (nextLev < 0))
				continue; // for the first level, keep the arcs going to prefpath or going to corridors
			//if (alt == 0 && (nextLev == lev + 1 && i2 == model.params.preferredPathOrtoPos[nextLev] || nextLev < 0))
			//	continue; // for the first level, keep the arcs going to prefpath or going to corridors
			//if (alt == 2 && i1 == model.params.preferredPathOrtoPos[lev])
			//	continue; // for the level before tss connection, keep the arcs going from prefpath
			model.network.physicalLev[lev].outNoNormalArc_useTSS[i1][i2b] = 1;
		}
	}
	//if(alt == 1)
		model.params.preferredPathStraightLineFeasibleFrom[lev] = 0;


	return 0;
}

int count_blockedArcs_tss() {
	int lev, i1, i2, n = 0;
	for (lev = 0; lev < model.network.nPhysicalLevels; lev++) {
		for (i1 = 0; i1 < model.network.physicalLev[lev].nPoints; i1++) {
			for (i2 = 0; i2 < model.network.physicalLev[lev].nOutNodes[i1]; i2++) {
				if (model.network.physicalLev[lev].outNoNormalArc_useTSS[i1][i2] == 1)
					n++;
			}
		}
	}
	return n;
}

int identify_arcsNoUse_tss() {
	int cNr, i1, i2, fromLev, fromNode, prefPathStart, keepPrefPath, connected_to_tss;
	int i_start, i_end, alt;
	double dist_tss_ext, x0, y0, x1, y1, dist;
	int nBlockedBefore, nBlockedAfter;

	model.temp_data.nMax_recursive_arcsLevels = 4;
	model.temp_data.arr_fromLev = (int*)malloc(model.temp_data.nMax_recursive_arcsLevels * sizeof(int));
	model.temp_data.arr_fromNode = (int*)malloc(model.temp_data.nMax_recursive_arcsLevels * sizeof(int));
	model.temp_data.arr_fromPos = (int*)malloc(model.temp_data.nMax_recursive_arcsLevels * sizeof(int));
	model.temp_data.arrBas_fromLev = (int*)malloc(model.temp_data.nMax_recursive_arcsLevels * sizeof(int));
	model.temp_data.arrBas_fromNode = (int*)malloc(model.temp_data.nMax_recursive_arcsLevels * sizeof(int));
	model.temp_data.arrBas_fromPos = (int*)malloc(model.temp_data.nMax_recursive_arcsLevels * sizeof(int));

	model.temp_data.arr_cNr = (int*)malloc(model.network.nChannels * sizeof(int));

	for (cNr = 0; cNr < model.network.nChannels; cNr++) {
		if (model.network.channel[cNr].type == 0)
			continue; // not a tss
		nBlockedBefore = count_blockedArcs_tss();
		CURRENT_TSS_BASTENDLEVEL = model.network.channel[cNr].bastEndLevel;
		if (DIAG_TSS_STARTLEVEL >= 0)
			DIAG_TSS_CNR = (model.network.channel[cNr].bastStartLevel == DIAG_TSS_STARTLEVEL) ? cNr : -1;
		errlog("tss channel %d: levels %d-%d, %.1lf km, %d connectFrom, %d connectTo, attractionDist %.1lf km\n",
			cNr, model.network.channel[cNr].bastStartLevel, model.network.channel[cNr].bastEndLevel,
			model.network.channel[cNr].distance_km, model.network.channel[cNr].nConnectFrom,
			model.network.channel[cNr].nConnectTo, model.params.tss_attractionDistance_km);
		i_start = model.network.channel[cNr].bastStartLevel;
		i_end = model.network.channel[cNr].bastEndLevel;
		if (i_start >= 0 && i_start < model.network.nPhysicalLevels &&
			i_end >= 0 && i_end < model.network.nPhysicalLevels) {
			for (i1 = i_start; i1 <= i_end; i1++) {
				if (model.network.physicalLev[i1].nOutNodesTot < 2) {
					break;
				}
			}
			if (i1 <= i_end) {
				alt = 0;
				for (i1 = i_start; i1 < i_end; i1++) {
					if (model.network.physicalLev[i1].nOutNodesTot >= 2) {
						if (i1 == i_end - 1)
							alt = 2;
						remove_all_arcs_nonPrefPath_level(i1, alt);
						model.network.physicalLev[i1].nOutNodesTot = -model.network.physicalLev[i1].nOutNodesTot;
					}
					alt = 1;
				}
				errlog("    tss channel %d: forced prefPath on levels %d-%d, blocked arcs now %d (was %d)\n",
					cNr, i_start, i_end - 1, count_blockedArcs_tss(), nBlockedBefore);
				continue; // do not need to do any other removal of arcs for this tss so go to next one
			}
			errlog("    tss channel %d: NOT forcing prefPath (all levels %d-%d have >= 2 out-arcs), using distance comparison instead\n",
				cNr, i_start, i_end);
		}


		keepPrefPath = model.network.channel[cNr].keepPrefPath_tss;

		dist_tss_ext = 0;
		//recursive_arcsNoUse_tss_tss(cNr, dist_tss_ext, -cNr - 1, 1, -1, 1, 1);
		model.temp_data.arr_cNr[0] = cNr;
		if (cNr == 7)
			cNr = cNr;
		for (i1 = 0; i1 < model.network.channel[cNr].nConnectFrom; i1++) {
			fromLev = model.network.channel[cNr].connectFrom_outLevel[i1];
			fromNode = model.network.channel[cNr].connectFrom_outNode[i1];
			if(fromLev >= 0 && model.params.preferredPathOrtoPos[fromLev] != fromNode)
				prefPathStart = 0;
			else
				prefPathStart = 1;
			dist_tss_ext = model.network.channel[cNr].distance_km;
			dist_tss_ext -= model.params.tss_attractionDistance_km;
			if (fromLev >= 0) {
				x0 = model.network.physicalLev[fromLev].point_x[fromNode];
				y0 = model.network.physicalLev[fromLev].point_y[fromNode];
			}
			else {
				x0 = model.network.channel[-fromLev - 1].point_x[model.network.channel[-fromLev - 1].nPoints - 1];
				y0 = model.network.channel[-fromLev - 1].point_y[model.network.channel[-fromLev - 1].nPoints - 1];
			}
			x1 = model.network.channel[cNr].point_x[0];
			y1 = model.network.channel[cNr].point_y[0];
			dist = estimateLargeCircleDistance_km(y0, x0, y1, x1);
			dist_tss_ext += dist;
			errlog("    tss channel %d: from level %d node %d, a bypass is blocked only if it is >= %.1lf km "
				"(tss %.1lf km + connect %.1lf km - attraction %.1lf km), max %d arcs in the bypass chain\n",
				cNr, fromLev, fromNode, dist_tss_ext, model.network.channel[cNr].distance_km, dist,
				model.params.tss_attractionDistance_km, model.temp_data.nMax_recursive_arcsLevels);

			for (i2 = 0; i2 < model.network.channel[cNr].nConnectTo; i2++) {
				if (model.network.channel[cNr].connectTo_outLevel[i2] >= 0) {
					identify_arcsNoUse_tss_levels(0, cNr, dist_tss_ext, fromLev, fromNode, i2, prefPathStart, keepPrefPath);
				}
				else {
					// connected to another channel .. handle this
					model.temp_data.arr_cNr[0] = cNr;
					recursive_arcsNoUse_tss_tss(cNr, dist_tss_ext, fromLev, fromNode, i2, 1, prefPathStart, keepPrefPath);


				}

			}
		}

		connected_to_tss = 0;
		for (i2 = 0; i2 < model.network.channel[cNr].nOutNodes; i2++) {
			if (model.network.channel[cNr].outLevel[i2] < 0) {
				connected_to_tss = 1;
				break;
			}
		}
		if (connected_to_tss == 1) {
			for (i2 = 0; i2 < model.network.channel[cNr].nOutNodes; i2++) {
				if (model.network.channel[cNr].outLevel[i2] >= 0) {
					model.network.channel[cNr].outNoNormalArc_useTSS[i2] = 1;
				}
			}
		}

		nBlockedAfter = count_blockedArcs_tss();
		errlog("    tss channel %d: blocked %d bypass arcs (total blocked now %d)\n",
			cNr, nBlockedAfter - nBlockedBefore, nBlockedAfter);
	}

	// summary: how many out-arcs survive per level, and how many nodes are left without any way onwards
	int nTotAll = 0, nBlockedAll = 0, nDeadEndAll = 0;
	errlog("=== tss arc blocking summary (tss_blockBeyondSpan %d) ===\n", TSS_BLOCK_BEYOND_SPAN);
	for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
		int nTot = 0, nBlocked = 0, nDeadEnd = 0;
		for (i2 = 0; i2 < model.network.physicalLev[i1].nPoints; i2++) {
			int nNode = 0, nNodeBlocked = 0;
			for (int i3 = 0; i3 < model.network.physicalLev[i1].nOutNodes[i2]; i3++) {
				nNode++;
				if (model.network.physicalLev[i1].outNoNormalArc_useTSS[i2][i3] == 1)
					nNodeBlocked++;
			}
			nTot += nNode;
			nBlocked += nNodeBlocked;
			if (nNode > 0 && nNode == nNodeBlocked)
				nDeadEnd++; // node had arcs but all of them are blocked
		}
		nTotAll += nTot;
		nBlockedAll += nBlocked;
		nDeadEndAll += nDeadEnd;
		if (nBlocked > 0)
			errlog("level %d: %d of %d out-arcs blocked by tss logic, %d remain, %d nodes left with no arc out (prefPath node is %d)\n",
				i1, nBlocked, nTot, nTot - nBlocked, nDeadEnd, model.params.preferredPathOrtoPos[i1]);
	}
	errlog("TOTAL: %d of %d physical out-arcs blocked by tss logic, %d remain, %d nodes left with no arc out\n",
		nBlockedAll, nTotAll, nTotAll - nBlockedAll, nDeadEndAll);

	// detailed dump of the arcs from DIAG_LEVEL_DUMP to the next level
	if (DIAG_LEVEL_DUMP >= 0 && DIAG_LEVEL_DUMP < model.network.nPhysicalLevels) {
		i1 = DIAG_LEVEL_DUMP;
		errlog("--- arcs from level %d to level %d (prefPath node %d -> %d) ---\n", i1, i1 + 1,
			model.params.preferredPathOrtoPos[i1], model.params.preferredPathOrtoPos[i1 + 1]);
		for (i2 = 0; i2 < model.network.physicalLev[i1].nPoints; i2++) {
			int nToNext = 0, nToNextBlocked = 0;
			for (int i3 = 0; i3 < model.network.physicalLev[i1].nOutNodes[i2]; i3++) {
				if (model.network.physicalLev[i1].outLevel[i2][i3] != i1 + 1)
					continue;
				nToNext++;
				if (model.network.physicalLev[i1].outNoNormalArc_useTSS[i2][i3] == 1)
					nToNextBlocked++;
			}
			if (nToNext > 0 || model.network.physicalLev[i1].allowedPoint[i2] == 1)
				errlog("  node %d (allowed %d): %d arcs to level %d, %d blocked, %d remain\n",
					i2, model.network.physicalLev[i1].allowedPoint[i2], nToNext, i1 + 1,
					nToNextBlocked, nToNext - nToNextBlocked);
		}
	}


	return 0;
}

int addArcsToNetwork()
{
	int i, i1, i2, cNr;
	int checkNextLevel;

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].nOutNodesTot = 0;
		model.network.physicalLev[i].nOutNodes = (int*)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].nInNodes = (int*)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].nArcsToPoint = (int*)calloc2(
			model.network.physicalLev[i].nPoints, sizeof(int));
		//model.network.physicalLev[i].nOutArcs = (int*)malloc2(
		//	model.network.physicalLev[i].nPoints * sizeof(int));
		//model.network.physicalLev[i].nAllocOutArcs = (int*)malloc2(
		//	model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].nAllocOutNodes = (int*)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].outNode = (int**)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		model.network.physicalLev[i].outLevel = (int**)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		model.network.physicalLev[i].outRestrictedAreaNr = (int**)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		model.network.physicalLev[i].outNoNormalArc_useTSS = (int**)malloc(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		//model.network.physicalLev[i].outArc = (strArcInfo**)malloc2(
		//	model.network.physicalLev[i].nPoints * sizeof(strArcInfo*));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			model.network.physicalLev[i].nOutNodes[i1] = 0;
			if(i == 0)
				model.network.physicalLev[i].nInNodes[i1] = 1;
			else
				model.network.physicalLev[i].nInNodes[i1] = 0;
			//model.network.physicalLev[i].nOutArcs[i1] = 0;
			//model.network.physicalLev[i].nAllocOutArcs[i1] = 1000;
			if (i == 9 && i1 == 34)
				i = i;
			if (i < model.network.nPhysicalLevels - 1) {
				model.network.physicalLev[i].nAllocOutNodes[i1] = model.network.physicalLev[i + 1].nPoints + 2;
				model.network.physicalLev[i].outNode[i1] = (int*)malloc(
					model.network.physicalLev[i].nAllocOutNodes[i1] * sizeof(int));
				model.network.physicalLev[i].outLevel[i1] = (int*)malloc(
					model.network.physicalLev[i].nAllocOutNodes[i1] * sizeof(int));
				model.network.physicalLev[i].outRestrictedAreaNr[i1] = (int*)malloc(
					model.network.physicalLev[i].nAllocOutNodes[i1] * sizeof(int));
				model.network.physicalLev[i].outNoNormalArc_useTSS[i1] = (int*)calloc(
					model.network.physicalLev[i].nAllocOutNodes[i1], sizeof(int));
				//model.network.physicalLev[i].outArc[i1] = (strArcInfo*)malloc2(
				//	model.network.physicalLev[i].nAllocOutArcs[i1] * sizeof(strArcInfo));
			}
		}
	}

	//FILE* filpek;
	//int allowed;
	//filpek = fopen("checkChannelsCorridor.txt", "w");
	//for (i2 = 0; i2 < model.network.nChannels; i2++) {
	//	allowed = 1;
	//	if (check_isChannelNodePosAllowed(i2, 0) == 0)
	//		allowed = 0;
	//	else {
	//		if (check_isChannelNodePosAllowed(i2, model.network.channel[i2].nPoints - 1) == 0)
	//			allowed = 0;
	//	}
	//	if (allowed == 0) {
	//		model.network.channel[i2].allowedPoint[0] = allowed;
	//		model.network.channel[i2].allowedPoint[model.network.channel[i2].nPoints - 1] = allowed;
	//	}
	//}
	//fclose(filpek);

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].minDistPrevNode = (double*)malloc2(model.network.physicalLev[i].nPoints * sizeof(double));
		model.network.physicalLev[i].minDistPrevNode_level = (int*)malloc2(model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].minDistPrevNode_pos = (int*)malloc2(model.network.physicalLev[i].nPoints * sizeof(int));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			model.network.physicalLev[i].minDistPrevNode[i1] = 1e10;
		}
	}

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i == 0 || i == model.network.nPhysicalLevels - 1)
			model.network.physicalLev[i].restrictedLevel = 1;
		else
			model.network.physicalLev[i].restrictedLevel = 0;
	}


	int pos1, pos2, arcOK;
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		pos1 = model.params.preferredPathOrtoPos[i];
		if (pos1 < 0)
			pos1 = -pos1 - 1;
		pos2 = model.params.preferredPathOrtoPos[i + 1];
		if (pos2 < 0)
			pos2 = -pos2 - 1;
		arcOK = check_isPhysicalArcOK(i, i + 1, pos1, pos2); // not a preferred path
		if (arcOK <= 0)
			model.network.physicalLev[i + 1].restrictedLevel = 1;
		else
			break;
	}
	for (i = model.network.nPhysicalLevels - 2; i > 0; i--) {
		pos1 = model.params.preferredPathOrtoPos[i];
		if (pos1 < 0)
			pos1 = -pos1 - 1;
		pos2 = model.params.preferredPathOrtoPos[i + 1];
		if (pos2 < 0)
			pos2 = -pos2 - 1;
		arcOK = check_isPhysicalArcOK(i, i + 1, pos1, pos2); // not a preferred path
		if (arcOK <= 0)
			model.network.physicalLev[i].restrictedLevel = 1;
		else
			break;
	}

	if (SKRIV_UT_NOTHING == 0)
		printf("nCorridors %d\n", model.network.nChannels);
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i >= 24)
			i = i;
		//printf("level %d\n", i);
		//if (i >= 14)
		//	printf("test\n");
		// add arcs from the channels, but NOT from preferred path
		if(i >= model.network.nPhysicalLevels - 3)
			i = i;
		try_addPhysicalArcsFromChannel(i);
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.params.preferredPathOrtoPos[i] == i1)
				i = i;
			if (model.network.physicalLev[i].allowedPoint[i1] == 0)
				continue;
			if (model.network.physicalLev[i].nInNodes[i1] == 0 && model.params.preferredPathOrtoPos[i] != i1) {// && model.network.physicalLev[i].nodeConnectedFromChannel[i1] == 0)
				continue; // no physical arcs to this node so no use to look for arcs out from it...
			}

			if ((model.network.physicalLev[i].onlyPrefPath != 1 || model.params.preferredPathOrtoPos[i] == i1) && model.network.physicalLev[i].followChannelExactly == 0) {
				for (i2 = i + 1; i2 < model.network.nPhysicalLevels; i2++) { // not if onlyPrefPath whole physical
					if (i == 5 && i1 == 28 && i2 == 62)
						i1 = i1;
					checkNextLevel = try_addPhysicalArcsLevel(i, i1, i2);
					if (checkNextLevel == 0)
						break;
					if (model.network.physicalLev[i].onlyPrefPath == 1 || model.network.physicalLev[i2].onlyPrefPath == 1)
						break;
				}
			}
			// check if arcs can be added to a channel, but NOT from preferred path
			checkNextLevel = try_addPhysicalArcsLevel(i, i1, -1);
		}
	}
	try_addPhysicalArcsBetweenChannels();

	// identify the levels where only the preferred path is allowed ***** and where the corridor cannot be utilized
	// then extend the corridor if the pref path is not allowed till other arcs attach
	identify_onlyPrefPathAllowedOnPhysicalLevel();

	//errlog("OBS! I don't close arcs because of close to tss as it doesn't work\n");
	int nNoder = 0, nBagar = 0, pointNr1, pointNr2;
	errlog("Fysiskt natverk: %d noder och %d bagar\n", nNoder, nBagar);
	
	model.params.preferredPathStraightLineFeasibleFrom = (int*)malloc2(model.network.nPhysicalLevels * sizeof(int));
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.params.preferredPathStraightLineFeasibleFrom[i] = 1;
		// printf("level %d nArcsOut %d\n", i, model.network.physicalLev[i].nOutNodesTot);
	}
	identify_arcsNoUse_tss();

	//for (i = 0; i < model.network.nChannels; i++){//  .nUsedChannels; i++) {
	//	cNr = i; // model.network.usedChannel[i];
	//	try_addPhysicalArcsFromChannel(cNr, noDataVal);
	//}


	double maxDist = 0, dist;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i == 38 || i == 43)
			i = i;
		for (i2 = 0; i2 < model.network.physicalLev[i].nPoints; i2++) {
			if (model.network.physicalLev[i].allowedPoint[i2] == 0)
				continue; // not allowed node
			nNoder++;
			nBagar += model.network.physicalLev[i].nOutNodes[i2];
		}

		if (i < model.network.nPhysicalLevels - 1) {
			pointNr1 = model.params.preferredPathOrtoPos[i];
			if (pointNr1 < 0)
				pointNr1 = (int)model.network.physicalLev[i].nPoints / 2;
			pointNr2 = model.params.preferredPathOrtoPos[i + 1];
			if (pointNr2 < 0)
				pointNr2 = (int)model.network.physicalLev[i+1].nPoints / 2;
			if (model.network.physicalLev[i].onlyPrefPath == 1)
				arcOK = 0;
			else
				arcOK = check_isPhysicalArcOK(i, i + 1, pointNr1, pointNr2);
			if (arcOK <= 0)
				model.params.preferredPathStraightLineFeasibleFrom[i] = 0;
		}
	}
	model.network.nPhysicalNodes = nNoder;
	model.network.nPhysicalArcs = nBagar;
	errlog("Fysiskt natverk: %d noder och %d bagar\n", nNoder, nBagar);

	identify_mostRestrictedAreaArcs();


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
	getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point_y[pos],
		model.network.physicalLev[level].point_x[pos], &row1Dbl, &col1Dbl);
	//getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point[pos].latitude().degrees(),
	//	model.network.physicalLev[level].point[pos].longitude().degrees(), &row1Dbl, &col1Dbl);

	if (level == 1 && pos == 24)
		pos = pos;
	for (int i0 = 0; i0 < 2; i0++) {
		if (i0 == 0) {
			if (pos == 0)
				continue;
			getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point_y[pos - 1],
				model.network.physicalLev[level].point_x[pos - 1], &row2Dbl, &col2Dbl);
		}
		else {
			if (pos + 1 >= model.network.physicalLev[level].nPoints)
				continue;
			getRowColDblFromPhysicalMap(model.physicalMapA, model.network.physicalLev[level].point_y[pos + 1],
				model.network.physicalLev[level].point_x[pos + 1], &row2Dbl, &col2Dbl);
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
			if (model.physicalMapA.valueCell[row * model.physicalMapA.nCols + colUse] == 1 &&
				check_feasibleNode_noGo_polygons(model.physicalMapA.maxLatitude - (row + 0.5) * model.physicalMapA.size_col,
					model.physicalMapA.minLongitude + (col + 0.5) * model.physicalMapA.size_col) == 1)
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
			if (dist < bastDist) {
				bastDist = dist;
				bastCol = colDbl;
				bastRow = rowDbl;
			}
		}
	}
	if (bastDist < 99999) {
		getLatLonFromPhysicalMap(model.physicalMapA, &lat, &lon, bastRow, bastCol);
		model.network.physicalLev[level].point[pos] = spherical::Point(lat, lon);
		model.network.physicalLev[level].point_x[pos] = lon;
		model.network.physicalLev[level].point_y[pos] = lat;

		return 1;
	}
	else
		return 0;
}

int moveCoordToFeasibleAlongLine(double* y1, double* x1, double y2, double x2) {
	int row, col, isFeasible = 0, i, nCellsOK, colUse;
	double rowDbl, colDbl, col2Dbl, row2Dbl, row1Dbl, col1Dbl;
	double delta_row, delta_col, kvot, a0, ac, ar, a1, dist, bastDist;
	double bastCol, bastRow, lat, lon;

	lat = *y1;
	lon = *x1;

	bastDist = 1e10;
	getRowColDblFromPhysicalMap(model.physicalMapA, lat, lon, &row1Dbl, &col1Dbl);
	getRowColDblFromPhysicalMap(model.physicalMapA, y2, x2, &row2Dbl, &col2Dbl);

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

	if (a1 < 0.9999){
		getLatLonFromPhysicalMap(model.physicalMapA, &lat, &lon, rowDbl, colDbl);
		*y1 = lat;
		*x1 = lon;
		return 1;
	}
	else
		return 0;
}

int makeSure_feasibleNodes(int level, int mittPos) {
	int i, isFeasible;

	for (i = 0; i < model.network.physicalLev[level].nPoints; i++) {
		if (i == 45)
			i = i;
		if (check_nodeIsWithinPhysicalMapRaster(model.network.physicalLev[level].point_y[i],
			model.network.physicalLev[level].point_x[i]) == 0) {
			model.network.physicalLev[level].allowedPoint[i] = 0;
			continue;
		}

		isFeasible = check_feasibleNodeRasterA(model.network.physicalLev[level].point_y[i],
			model.network.physicalLev[level].point_x[i]);
		if (isFeasible == 1 && model.params.preferredPathOrtoPos[level] != i)
			isFeasible = check_feasibleNode_noGo_polygons(model.network.physicalLev[level].point_y[i],
				model.network.physicalLev[level].point_x[i]);

		if (isFeasible == 0 && model.params.preferredPathOrtoPos[level] != i) {
			isFeasible = movePointToFeasible(level, i);
			model.network.physicalLev[level].allowedPoint[i] = isFeasible;
		}
	}


	return 0;
}

int makeSure_feasibleCoordFranLinje(double* y1, double* x1, double* y2, double* x2, int pos) {
	int isFeasible;
	double xUse, yUse, xOther, yOther;

	if (pos == 0) {
		xUse = *x1;
		yUse = *y1;
		xOther = *x2;
		yOther = *y2;
	}
	else {
		xUse = *x2;
		yUse = *y2;
		xOther = *x1;
		yOther = *y1;
	}
	isFeasible = check_nodeIsWithinPhysicalMapRaster(yUse, xUse);
	if (isFeasible == 0)
		return -1; // coordinate outside physical map, do not use it

	if (check_feasibleNodeRasterA(yUse, xUse) == 0) {
		// node not feasible, move it
		isFeasible = moveCoordToFeasibleAlongLine(&yUse, &xUse, yOther, xOther);
		if (isFeasible == 0)
			return -1;
	}

	if (pos == 0) {
		*x1 = xUse;
		*y1 = yUse;
	}
	else {
		*x2 = xUse;
		*y2 = yUse;
	}

	return 1;
}

int check_isCoordFeasiblePhysicalMap(double y, double x) {

	if (check_nodeIsWithinPhysicalMapRaster(y, x) == 0)
		return 0;
			
	return check_feasibleNodeRasterA(y, x);
}

int getMonthFromHoursSinceRouteStart(double hoursAfterStart) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	int timmar = (int)(hoursAfterStart); // *24;
	tmBas.tm_hour = model.params.startHour + timmar;
	int minuter = (int)((hoursAfterStart - timmar) * 60.0);
	tmBas.tm_min = model.params.startMinute + minuter;
	tmBas.tm_sec = 0;
	time_t test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	return tmBas.tm_mon + 1;
}


int getMonthsToUseForDelay_new(double dist) {
	int i, pos, dayStart, dayEnd;
	double tripHours = dist / model.params.shipSpeed_average;
	int midMonth = getMonthFromHoursSinceRouteStart(model.network.tidp_startHistoricDataOnly + tripHours / 2);
	if (SKRIV_UT_NOTHING == 0)
		errlog("getMonthsToUseForDelay dist %.2lf speedAver %.2lf tidp_startHist %d tripHours %.2lf midMonth %d\n",
			dist, model.params.shipSpeed_average, model.network.tidp_startHistoricDataOnly, tripHours, midMonth);
	//double midHistorical = model.network.tidp_startHistoricDataOnly + (tripHours - model.network.tidp_startHistoricDataOnly) / 2;
	int nMonths = 1;

	if (SKRIV_UT_NOTHING == 0)
		errlog("nMonths %d\n", nMonths);

	model.delay.delayed_monthNr = (int*)malloc(nMonths * sizeof(int));
	pos = 0;
	model.delay.delayed_monthNr[pos++] = midMonth;

	return nMonths;
	//return getMonthFromHoursSinceRouteStart(midHistorical); // create a date from startTime plus midHistorical
}

int getMonthsToUseForDelay_old(double dist) {
	int i, pos;
	double tripHours = dist / model.params.shipSpeed_average;
	double tripHoursLongest = dist / model.params.shipSpeed_average * 2;
	int startMonth = getMonthFromHoursSinceRouteStart(model.network.tidp_startHistoricDataOnly);
	int endMonth = getMonthFromHoursSinceRouteStart(model.network.tidp_startHistoricDataOnly + tripHours) + 1;
	if (endMonth > 12)
		endMonth -= 12;
	int endMonth2 = getMonthFromHoursSinceRouteStart(model.network.tidp_startHistoricDataOnly + tripHoursLongest);
	if (SKRIV_UT_NOTHING == 0)
		errlog("getMonthsToUseForDelay dist %.2lf speedAver %.2lf tidp_startHist %d tripHours %.2lf tripHoursLongest %.2lf startMonth %d endMonth %d endMonth2 %d\n",
			dist, model.params.shipSpeed_average, model.network.tidp_startHistoricDataOnly, tripHours, tripHoursLongest, startMonth, endMonth, endMonth2);
	if (endMonth2 == endMonth - 1 || (endMonth2 == 12 && endMonth == 1))
		endMonth = endMonth2;
	//double midHistorical = model.network.tidp_startHistoricDataOnly + (tripHours - model.network.tidp_startHistoricDataOnly) / 2;
	int nMonths = endMonth - startMonth + 1;
	if (nMonths < 0)
		nMonths += 12;

	if (SKRIV_UT_NOTHING == 0)
		errlog("nMonths %d\n", nMonths);

	model.delay.delayed_monthNr = (int*)malloc(nMonths * sizeof(int));
	pos = 0;
	if (endMonth < startMonth) {
		for (i = startMonth; i <= 12; i++)
			model.delay.delayed_monthNr[pos++] = i;
		for (i = 1; i <= endMonth; i++)
			model.delay.delayed_monthNr[pos++] = i;
	}
	else {
		for (i = startMonth; i <= endMonth; i++)
			model.delay.delayed_monthNr[pos++] = i;
	}
	return nMonths;
	//return getMonthFromHoursSinceRouteStart(midHistorical); // create a date from startTime plus midHistorical
}

int check_isStraightLineStartFeasible(double y1, double x1, double y2, double x2, double *distOK)
{
	int arcOK = 1, mittPktPos1, mittPktPos2, level1, level2, row, col, colUse;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl;
	double distance;

	distance = estimateLargeCircleDistance_km(y1, x1, y2, x2); // p1.distanceTo(p2) / 1000.0;

	getRowColDblFromPhysicalMap(model.physicalMapA, y1, x1, &row1Dbl, &col1Dbl);
	row = (int)row1Dbl;
	col = (int)col1Dbl;
	if (col >= model.physicalMapA.nCols)
		colUse = col - model.physicalMapA.nCols;
	else {
		if (col < 0)
			colUse = col + model.physicalMapA.nCols;
		else
			colUse = col;
	}
	if (model.physicalMapA.valueCell[row * model.physicalMapA.nCols + colUse] == 1) {
		*distOK = 0;
		return 2; // startpoint is okay
	}

	getRowColDblFromPhysicalMap(model.physicalMapA, y2, x2, &row2Dbl, &col2Dbl);
	row = (int)row2Dbl;
	col = (int)col2Dbl;
	if (col >= model.physicalMapA.nCols)
		colUse = col - model.physicalMapA.nCols;
	else {
		if (col < 0)
			colUse = col + model.physicalMapA.nCols;
		else
			colUse = col;
	}
	if (model.physicalMapA.valueCell[row * model.physicalMapA.nCols + colUse] == 0) {
		*distOK = 0;
		return 0; // start and endpoint not okay
	}

	double colDbl = col2Dbl;
	double rowDbl = row2Dbl;
	double delta_row = row1Dbl - row2Dbl;
	if (delta_row > model.physicalMapA.nRows / 2) {
		delta_row = model.physicalMapA.nRows - delta_row;
	}
	else {
		if (delta_row < -model.physicalMapA.nRows / 2) {
			delta_row = -model.physicalMapA.nRows - delta_row;
		}
	}
	double delta_col = col1Dbl - col2Dbl;
	if (delta_col > model.physicalMapA.nCols / 2) {
		delta_col = delta_col - model.physicalMapA.nCols;
	}
	else {
		if (delta_col < -model.physicalMapA.nCols / 2) {
			delta_col = model.physicalMapA.nCols + delta_col;
		}
	}
	double kvot = 0;

	double a0 = 0, ok_a0 = 0, ac, ar, a1, xNy, yNy;
	int i;
	for (i = 0; i < 10000; i++) {
		if (col >= model.physicalMapA.nCols)
			colUse = col - model.physicalMapA.nCols;
		else {
			if (col < 0)
				colUse = col + model.physicalMapA.nCols;
			else
				colUse = col;
		}
		if (model.physicalMapA.valueCell[row * model.physicalMapA.nCols + colUse] == 0)
			break;
		ok_a0 = a0;
		if (delta_col > 0)
			ac = (col + 1 - col2Dbl) / delta_col;
		else {
			if (delta_col == 0)
				ac = 999999;
			else {
				if (colDbl > col + 0.001)
					ac = (col - col2Dbl) / delta_col;
				else
					ac = (col - 1 - col2Dbl) / delta_col;
			}
		}
		if (delta_row > 0)
			ar = (row + 1 - row2Dbl) / delta_row;
		else {
			if (delta_row == 0)
				ar = 999999;
			else {
				if (rowDbl > row + 0.001)
					ar = (row - row2Dbl) / delta_row;
				else
					ar = (row - 1 - row2Dbl) / delta_row;
			}
		}
		if (ac < ar)
			a1 = ac;
		else
			a1 = ar;
		if (a1 > 1)
			a1 = 1;

		if (a1 >= 0.9999)
			break;

		colDbl = col2Dbl + a1 * delta_col;
		rowDbl = row2Dbl + a1 * delta_row;

		col = (int)(colDbl + 0.0001 * delta_col);
		if (colDbl < 0)
			colDbl = colDbl;
		row = (int)(rowDbl + 0.0001 * delta_row);
		if (rowDbl < 0)
			rowDbl = rowDbl;

		a0 = a1;
	}

	colDbl = col2Dbl + ok_a0 * delta_col;
	rowDbl = row2Dbl + ok_a0 * delta_row;

	getLatLonFromPhysicalMap(model.physicalMapA, &yNy, &xNy, rowDbl, colDbl);
	*distOK = estimateLargeCircleDistance_km(y2, x2, yNy, xNy);

	return 1;
}

int identifyStartEndAllowed(double distInt, double* startDistBad, double* endDistBad) {
	double dist = 0, prevDistOK = 0, minKravOKdist = 5.0, distOK, distNu;
	int i, isAllowed;
	spherical::Point newPoint;

	for (i = 1; i < model.preferredPath.nPoints; i++) {
		if (dist >= distInt - 1) {
			*startDistBad = distInt;
			break;
		}
		distNu = model.preferredPath.point[i - 1].distanceTo(model.preferredPath.point[i]) / 1000.0;
		if (dist + distNu > distInt) {
			newPoint = model.preferredPath.point[i - 1].destinationPoint((distInt - dist) * 1000.0, model.preferredPath.point[i - 1].bearingTo(model.preferredPath.point[i]));
			isAllowed = check_isStraightLineStartFeasible(model.preferredPath.point_y[i-1], model.preferredPath.point_x[i-1],
				newPoint.latitude().degrees(), newPoint.longitude().degrees(), &distOK);
		}
		else
			isAllowed = check_isStraightLineStartFeasible(model.preferredPath.point_y[i - 1], model.preferredPath.point_x[i - 1],
				model.preferredPath.point_y[i], model.preferredPath.point_x[i], &distOK);

		if (isAllowed > 0) {
			if (isAllowed == 2) { // point i - 1 is allowed
				if (i == 1)
					*startDistBad = 0;
				else {
					if (prevDistOK + distNu >= minKravOKdist)
						*startDistBad = dist + minKravOKdist - prevDistOK;
					else {
						prevDistOK += distNu;
						dist += distNu;
						continue;
					}
				}
				break;
			}
			else {
				// point i - 1 is not allowed but the next point is
				if (prevDistOK + distOK >= minKravOKdist) {
					if (dist + distNu > distInt)
						*startDistBad = distInt - (prevDistOK + distOK) + minKravOKdist;
					else
						*startDistBad = dist + distNu - (prevDistOK + distOK) + minKravOKdist;
					break;
				}
				else
					prevDistOK += distOK;
			}
		}
		dist += distNu;
	}

	dist = 0;
	prevDistOK = 0;
	for (i = model.preferredPath.nPoints - 1; i > 0; i--) {
		if (dist >= distInt - 0.01) {
			*endDistBad = distInt;
			break;
		}
		distNu = model.preferredPath.point[i].distanceTo(model.preferredPath.point[i - 1]) / 1000.0;
		if (dist + distNu > distInt) {
			newPoint = model.preferredPath.point[i].destinationPoint((distInt - dist) * 1000.0, model.preferredPath.point[i].bearingTo(model.preferredPath.point[i - 1]));
			isAllowed = check_isStraightLineStartFeasible(model.preferredPath.point_y[i], model.preferredPath.point_x[i],
				newPoint.latitude().degrees(), newPoint.longitude().degrees(), &distOK);
		}
		else
			isAllowed = check_isStraightLineStartFeasible(model.preferredPath.point_y[i], model.preferredPath.point_x[i],
				model.preferredPath.point_y[i - 1], model.preferredPath.point_x[i - 1], &distOK);

		if (isAllowed > 0) {
			if (isAllowed == 2) { // point i is allowed
				if (i == model.preferredPath.nPoints - 1)
					*endDistBad = 0;
				else {
					if (prevDistOK + distNu >= minKravOKdist)
						*endDistBad = dist + minKravOKdist - prevDistOK;
					else {
						prevDistOK += distNu;
						dist += distNu;
						continue;
					}
				}
				break;
			}
			else {
				// point i is not allowed but the previous point is
				if (prevDistOK + distOK >= minKravOKdist) {
					if(dist + distNu > distInt)
						*endDistBad = distInt - (prevDistOK + distOK) + minKravOKdist;
					else
						*endDistBad = dist + distNu - (prevDistOK + distOK) + minKravOKdist;
					break;
				}
				else
					minKravOKdist += distOK;
			}
		}
		dist += distNu;
	}

	return 0;
}


int checkPrefPath_throughExtraNoGo() {
	int i, arcOK, startProblem = 1, i1, lastProblem = 0;
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		if (i == model.network.nPhysicalLevels - 2)
			i = i;
		if (i >= 40)
			i = i;
		arcOK = check_isPhysicalArcOK(i, i + 1, model.params.preferredPathOrtoPos[i], model.params.preferredPathOrtoPos[i + 1]); // not a preferred path
		if (arcOK == -1 || (startProblem == 1 && arcOK == 0) || ((arcOK == 0 || arcOK == -2) && i == model.network.nPhysicalLevels - 2)) {
			if (startProblem == 0) {
				// if pref path feasible but through extra noGo and not start/end of leg then
				model.network.physicalLev[i].requirePrefPathFeasible = 1;
				lastProblem = 1;
			}
			if ((model.network.physicalLev[i].legNr < model.network.physicalLev[i + 1].legNr &&
				model.params.legProperties[model.network.physicalLev[i].legNr].endNode_exact == 1) ||
				i == model.network.nPhysicalLevels - 2){
				for (i1 = i; i1 >= 0; i1--) {
					if (model.network.physicalLev[i1].requirePrefPathFeasible == 0) {
						arcOK = check_isPhysicalArcOK(i1, i1 + 1, model.params.preferredPathOrtoPos[i1], model.params.preferredPathOrtoPos[i1 + 1]); // not a preferred path
						if(arcOK == 1)
							break;
					}
					model.network.physicalLev[i1].requirePrefPathFeasible = 0;
				}
				startProblem = 1;
			}
		}
		else {
			if (arcOK == 0) {
				if (lastProblem == 1) {
					if ((model.network.physicalLev[i].legNr < model.network.physicalLev[i + 1].legNr &&
						model.params.legProperties[model.network.physicalLev[i].legNr].endNode_exact == 1) ||
						i == model.network.nPhysicalLevels - 2) {
						for (i1 = i; i1 >= 0; i1--) {
							if (model.network.physicalLev[i1].requirePrefPathFeasible == 0) {
								arcOK = check_isPhysicalArcOK(i1, i1 + 1, model.params.preferredPathOrtoPos[i1], model.params.preferredPathOrtoPos[i1 + 1]); // not a preferred path
								if (arcOK == 1)
									break;
							}
							model.network.physicalLev[i1].requirePrefPathFeasible = 0;
						}
						startProblem = 1;
						lastProblem = 0;
					}
				}
			}
			else {
				lastProblem = 0;
				startProblem = 0;
			}
		}
	}


	return 0;
}


int createPhysicalNetwork(int sparaKorridorEnbart, int alt)
{
	int i, nInt, nPkterOrto = -1, i1, endPos, nIntLoc, roundKvot, nodExact;
	double dist, distTot, nIntDbl, distInt, distIntUse, distIntTot;
	double ortoDist, bNy, badDist;
	double distNu, startDistBad, endDistBad;
	spherical::Point pointNu;

	distTot = 0;
	model.preferredPath.distToPrevPoint = (double*)calloc(model.preferredPath.nPoints, sizeof(double));
	for (i = 1; i < model.preferredPath.nPoints; i++) {
		dist = model.preferredPath.point[i - 1].distanceTo(model.preferredPath.point[i]) / 1000;
		model.preferredPath.distToPrevPoint[i] = dist;
		distTot += dist;
		//printf("from %d to %d %.4lf %.4lf to %.4lf %.4lf dist %.3lf %.3lf\n", i - 1, i,
		//	model.preferredPath.point_y[i - 1], model.preferredPath.point_x[i - 1],
		//	model.preferredPath.point_y[i], model.preferredPath.point_x[i], dist, distTot);
	}
	errlog("tot haversine dist of prefered path %lf nPoints in prefPath %d\n",
		distTot, model.preferredPath.nPoints);

	checkMinnesAnvandning(__LINE__);
	model.preferredPath.totDist = distTot;

	if (model.params.varyStartEndArcLength == 1)
		openNeededRasterFilesNew(alt);

	errlog("nHoursIntervals: %lf\n", model.params.nHours_changeCourseInterval);

	nIntDbl = distTot / model.params.shipSpeed_average / model.params.nHours_changeCourseInterval;
	nIntLoc = (int)ceil(nIntDbl);
	distInt = distTot / nIntLoc * (1 + model.params.epsilon); // # add a small number so there will be no nodes orthogonal to the last one
	//errlog("is identifyStartEndAllowed() below needed for legs. I have removed it now.\n");
	if (model.params.varyStartEndArcLength == 1)
		identifyStartEndAllowed(distInt, &startDistBad, &endDistBad);
	else {
		startDistBad = 0;
		endDistBad = 0;
	}

	distIntTot = 0;
	nInt = 0;
	for (i = 0; i < model.params.nLegs; i++) {
		distNu = 0;
		if (i < model.params.nLegs - 1)
			endPos = model.params.legProperties[i + 1].prefPath_startPoint;
		else
			endPos = model.params.legProperties[i].prefPath_endPoint;
		for (i1 = model.params.legProperties[i].prefPath_startPoint + 1; i1 <= endPos; i1++)
			distNu += model.preferredPath.distToPrevPoint[i1];
		nIntDbl = distNu / model.params.shipSpeed_average / model.params.nHours_changeCourseInterval;
		nIntLoc = (int)ceil(nIntDbl);
		distInt = distNu / nIntLoc * (1 + model.params.epsilon); // # add a small number so there will be no nodes orthogonal to the last one
		//if (i < model.params.nLegs - 1)
		//	distInt--; // since last node will be used for the next leg, not correct, I add one more in the end
		errlog("legNr %d nIntDbl %lf nInt %d distInt %lf\n", i, nIntDbl, nIntLoc, distInt);

		if ((startDistBad > 0.005 && i == 0) || (endDistBad > 0.005 && i == model.params.nLegs - 1)) {
			if (i == 0)
				badDist = startDistBad;
			else
				badDist = 0;
			if (i == model.params.nLegs - 1)
				badDist += endDistBad;
			nIntDbl = (distNu - badDist) / model.params.shipSpeed_average / model.params.nHours_changeCourseInterval;
			nIntLoc = (int)ceil(nIntDbl);
			if (nIntLoc < 1)
				nIntLoc = 1;
			distInt = (distNu - badDist) / nIntLoc * (1 + model.params.epsilon); // # add a small number so there will be no nodes orthogonal to the last one
			if (distInt < model.params.epsilon)
				distInt = model.params.epsilon;
			if (startDistBad > 0 && i == 0)
				nIntLoc++;
			if (endDistBad > 0 && i == model.params.nLegs - 1)
				nIntLoc++;
			errlog("recalculate distInt, nIntDbl %lf nInt %d distInt %lf, startDistBad %.2lf endDistBad %.2lf\n",
				nIntDbl, nIntLoc, distInt, startDistBad, endDistBad);
		}

		model.params.legProperties[i].physLevelFirst = nInt;
		model.params.legProperties[i].basDistArcs = distInt;
		distIntTot += distInt;
		nInt += nIntLoc;
		model.params.legProperties[i].physLevelLast = nInt - 1;

	}

	model.params.basDistArcs = distIntTot;

	int nIntervallPoints, nAllocPoints, posNu, legNr;
	spherical::Point* intervallPoint;
	double distKvar, kvot;

	//initCoordUsage();

	intervallPoint = (spherical::Point*)malloc2((nInt + 1) * sizeof(spherical::Point));
	nIntervallPoints = 0;
	intervallPoint[nIntervallPoints] = model.preferredPath.point[0];
	nIntervallPoints++;

	model.network.physicalLev = (strNodeSeq*)malloc2((nInt + 1) * sizeof(strNodeSeq));
	model.delay.nDelayed_months = 0;
	nAllocPoints = 10;
	model.network.physicalLev[0].preferredPathPoint = (spherical::Point*)malloc(nAllocPoints * sizeof(spherical::Point));
	model.network.physicalLev[0].npreferredPathPoints = 0;
	model.network.physicalLev[0].distanceFromStartPosMid = 0;
	model.network.physicalLev[0].legNr = 0;

	// add nodes at even distances along given path
	dist = 0;
	posNu = 0;
	legNr = 0;
	for (i = 1; i < model.preferredPath.nPoints; i++) {
		if (i >= 55)
			i = i;
		distNu = model.preferredPath.point[i - 1].distanceTo(model.preferredPath.point[i]) / 1000.0;
		distKvar = distNu;

		if (nIntervallPoints - 1 > model.params.legProperties[legNr].physLevelLast)
			legNr++;
		distIntUse = model.params.legProperties[legNr].basDistArcs; // distInt;
		if (nIntervallPoints == 1) {
			if (startDistBad > 0.001) {
				distIntUse = startDistBad;
			}
		}
		while (dist + distKvar >= distIntUse && nIntervallPoints < nInt) {
			// add an intervall here
			kvot = (distIntUse - dist + distNu - distKvar) / distNu;
			if (kvot < 0.0001) {
				kvot = 0;
				roundKvot = 1;
			}
			else
				roundKvot = 0;
			auto pMid = model.preferredPath.point[i - 1].intermediatePointTo(
				model.preferredPath.point[i], kvot); // 51.3721°N, 000.7073°E
			intervallPoint[nIntervallPoints] = pMid;
			if (posNu <= nAllocPoints) {
				nAllocPoints += 10;
				model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint = (spherical::Point*)realloc(
					model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint, nAllocPoints * sizeof(spherical::Point));
			}
			model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu] = pMid;
			model.network.physicalLev[nIntervallPoints].distanceFromStartPosMid = model.network.physicalLev[nIntervallPoints - 1].distanceFromStartPosMid + distIntUse;

			//printf("-- i %d level %d adding pref path point %d %.3lf %.3lf dist %.2lf distKvar %.2lf distInt %.2lf\n", i, nIntervallPoints - 1, posNu,
			//	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu].longitude().degrees(),
			//	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu].latitude().degrees(), dist, distKvar, distInt);
			posNu++;
			model.network.physicalLev[nIntervallPoints - 1].npreferredPathPoints = posNu;
			model.network.physicalLev[nIntervallPoints - 1].legNr = legNr;

			if (posNu + 1 > model.network.nMaxNodesInPath)
				model.network.nMaxNodesInPath = posNu + 1;
			posNu = 0;
			nIntervallPoints++;

			if (nIntervallPoints >= 6)
				nIntervallPoints = nIntervallPoints;
			nAllocPoints = 10;
			model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint = (spherical::Point*)malloc(nAllocPoints * sizeof(spherical::Point));
			//distIntUse = distInt;
			if (dist > distIntUse)
				dist -= distIntUse;
			else {
				if(roundKvot == 0)
					distKvar -= distIntUse - dist;
				dist = 0;
			}
			if (nIntervallPoints - 1 > model.params.legProperties[legNr].physLevelLast) {
				legNr++;
			}
			distIntUse = model.params.legProperties[legNr].basDistArcs; // distInt;
		}
		if (dist + distKvar > 0.1) {
			if (posNu >= nAllocPoints - 1) {
				nAllocPoints += 10;
				model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint = (spherical::Point*)realloc(
					model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint, nAllocPoints * sizeof(spherical::Point));
			}
			model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu] = model.preferredPath.point[i];
			model.network.physicalLev[nIntervallPoints - 1].legNr = legNr;
			//printf("-- i %d level %d adding pref path point2 %d %.3lf %.3lf dist %.2lf distKvar %.2lf\n", i, nIntervallPoints - 1, posNu,
			//	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu].longitude().degrees(),
			//	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu].latitude().degrees(), dist, distKvar);
			posNu++;
		}
		dist += distKvar;
	}
	if (nIntervallPoints >= nInt + 1)
		errlog("ERROR! Memory problem. Fix this on code row %d\n", __LINE__);
	intervallPoint[nIntervallPoints] = model.preferredPath.point[model.preferredPath.nPoints - 1];
	model.network.physicalLev[nIntervallPoints].distanceFromStartPosMid = model.network.physicalLev[nIntervallPoints - 1].distanceFromStartPosMid + dist;

	//model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu] = intervallPoint[nIntervallPoints];
	//printf("-- i xx level %d adding pref path point3 %d %.3lf %.3lf dist %.2lf distKvar %.2lf\n", nIntervallPoints - 1, posNu,
	//	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu].longitude().degrees(),
	//	model.network.physicalLev[nIntervallPoints - 1].preferredPathPoint[posNu].latitude().degrees(), dist, distKvar);
	//posNu++;
	model.network.physicalLev[nIntervallPoints - 1].npreferredPathPoints = posNu;
	if (posNu + 1 > model.network.nMaxNodesInPath)
		model.network.nMaxNodesInPath = posNu + 1;
	nIntervallPoints++;
	//writePointsToShape((char*)"intervallPoints", intervallPoint, nIntervallPoints);
	//writePointsToGeojson((char*)"intervallPoints", intervallPoint, nIntervallPoints);

	ortoDist = model.params.shipSpeed_average * 1000 / model.params.ortoDist_nPointsPerHour;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	model.params.preferredPathOrtoPos = (int*)malloc2(nIntervallPoints * sizeof(int));

	// printf("-- Time1 %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	distTot = 0;
	if (nIntervallPoints > 1)
		model.network.physicalLev[nIntervallPoints - 1].legNr = model.network.physicalLev[nIntervallPoints - 2].legNr;
	else {
		model.network.physicalLev[nIntervallPoints - 1].legNr = 0;
	}
	for (i = 0; i < nIntervallPoints; i++) {
		if (i == 41)
			i = i;
		if (i > 0)
			distTot += intervallPoint[i - 1].distanceTo(intervallPoint[i]) / 1000.0;
		model.network.physicalLev[i].distTot = distTot;
		model.network.physicalLev[i].requirePrefPathFeasible = 0;
		model.network.physicalLev[i].factorDelayedPrefPath = 0;
		legNr = model.network.physicalLev[i].legNr;
		if(model.params.legProperties[legNr].path_fixed == 1)
			model.network.physicalLev[i].onlyPrefPath = 1;
		else {
			model.network.physicalLev[i].onlyPrefPath = 0;
		}
		model.network.physicalLev[i].followChannelExactly = 0;

		nodExact = 0;
		if (i > 0 && model.network.physicalLev[i].onlyPrefPath == 0 && model.network.physicalLev[i - 1].onlyPrefPath == 0) {
			if (model.network.physicalLev[i - 1].legNr < legNr &&
				model.params.legProperties[model.network.physicalLev[i - 1].legNr].endNode_exact == 1)
				nodExact = 1;
		}

		if (i == nIntervallPoints - 2)
			i = i;

		if (i > 0 && model.network.physicalLev[i - 1].legNr < legNr) {
			model.network.physicalLev[i].tidWait = model.params.legProperties[model.network.physicalLev[i - 1].legNr].endNode_waitingTime;
			model.network.physicalLev[i].bransleWaitMain = model.params.legProperties[model.network.physicalLev[i - 1].legNr].endNode_waitingTime *
				model.params.legProperties[model.network.physicalLev[i - 1].legNr].endNode_fuelConsumption_main_mpd / 24.0;
			model.network.physicalLev[i].bransleWaitAux = model.params.legProperties[model.network.physicalLev[i - 1].legNr].endNode_waitingTime *
				model.params.legProperties[model.network.physicalLev[i - 1].legNr].endNode_fuelConsumption_aux_mpd / 24.0;
		}
		else {
			model.network.physicalLev[i].tidWait = 0.0;
			model.network.physicalLev[i].bransleWaitMain = 0.0;
			model.network.physicalLev[i].bransleWaitAux = 0.0;
		}

		if (i == 0 || i == nIntervallPoints - 1 || model.network.physicalLev[i].onlyPrefPath == 1 || nodExact == 1) {
			model.network.physicalLev[i].nPoints = 0;
			model.network.physicalLev[i].point = (spherical::Point*)malloc2(sizeof(spherical::Point));
			model.network.physicalLev[i].allowedPoint = (int*)malloc2(sizeof(int));
			model.network.physicalLev[i].point_x = (double*)malloc2(sizeof(double));
			model.network.physicalLev[i].point_y = (double*)malloc2(sizeof(double));
			model.network.physicalLev[i].point[model.network.physicalLev[i].nPoints] = intervallPoint[i];
			model.network.physicalLev[i].point_x[model.network.physicalLev[i].nPoints] = intervallPoint[i].longitude().degrees();
			model.network.physicalLev[i].point_y[model.network.physicalLev[i].nPoints] = intervallPoint[i].latitude().degrees();
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
			model.network.physicalLev[i].point = (spherical::Point*)malloc2(model.params.nPkterOrto * sizeof(spherical::Point));
			model.network.physicalLev[i].allowedPoint = (int*)malloc2(model.params.nPkterOrto * sizeof(int));
			model.network.physicalLev[i].point_x = (double*)malloc2(model.params.nPkterOrto * sizeof(double));
			model.network.physicalLev[i].point_y = (double*)malloc2(model.params.nPkterOrto * sizeof(double));
			//\example
			//	Point p1{ 52.205, 0.119 };
			//Point p2{ 48.857, 2.351 };
			// get bearing at the point

			//auto b1 = intervallPoint[i - 1].bearingTo(intervallPoint[i]); // 157.9°
			// modify the bearing
			//auto b2 = intervallPoint[i - 1].finalBearingTo(intervallPoint[i]); // 157.9°
			// modify the bearing
			// bNy = (b1 + b2) / 2 + 90; // + M_PI / 2;

			//bNy = estimateBearingFromToCoords(intervallPoint[i - 1].latitude().degrees(), 
			//	intervallPoint[i - 1]. longitude().degrees(),
			//	intervallPoint[i].latitude().degrees(), intervallPoint[i].longitude().degrees()) + 90;
			// new try 220830
			bNy = estimateBearingFromToCoords(intervallPoint[i - 1].latitude().degrees(),
				intervallPoint[i - 1].longitude().degrees(),
				intervallPoint[i + 1].latitude().degrees(), intervallPoint[i + 1].longitude().degrees()) + 90;


			if (bNy >= 360)
				bNy -= 360;
			//\example
			//	Point p1{ 51.4778, -0.0015 };
			//Point p2 = p1.destinationPoint(7794, 300.7); // 51.5135°N, 000.0983°W


			nPkterOrto = model.params.nPkterOrto;

			for (int i1 = 0; i1 < nPkterOrto; i1++) {
				model.network.physicalLev[i].point[i1] = intervallPoint[i].destinationPoint(
					ortoDist * (i1 - (nPkterOrto - 1) / 2), bNy);
				if (i==4&& i1 == 37)
					i1 = i1;
				model.network.physicalLev[i].point_x[i1] = model.network.physicalLev[i].point[i1].longitude().degrees();
				model.network.physicalLev[i].point_y[i1] = model.network.physicalLev[i].point[i1].latitude().degrees();
				//updateCoordUsage(model.network.physicalLev[i].point[i1]);
				model.network.physicalLev[i].allowedPoint[i1] = 1;
			}
			model.network.physicalLev[i].nPoints = nPkterOrto;
			if (model.params.preferredPath_followExactOK == 1)
				model.params.preferredPathOrtoPos[i] = (int)(nPkterOrto / 2);
			else
				model.params.preferredPathOrtoPos[i] = -(int)(nPkterOrto / 2) - 1;

			//writePointsToShape((char*)"shapeTest", model.network.physicalLev[i].point, model.network.physicalLev[i].nPoints);

			//model.network.physicalLev[i].point[model.network.physicalLev[i].nPoints] = intervallPoint[i];
			//model.network.physicalLev[i].point_x[model.network.physicalLev[i].nPoints] = intervallPoint[i].longitude().degrees();
			//model.network.physicalLev[i].point_y[model.network.physicalLev[i].nPoints] = intervallPoint[i].latitude().degrees();
		}
		//printf("--physicalLevel %d prefPathPos %d nPkter %d xy %.3lf %.3lf checkX %.3lf pos %d nAlloc %d\n", i, model.params.preferredPathOrtoPos[i],
		//	model.network.physicalLev[i].nPoints, model.network.physicalLev[i].point_x[model.params.preferredPathOrtoPos[i]],
		//	model.network.physicalLev[i].point_y[model.params.preferredPathOrtoPos[i]],
		//	model.network.physicalLev[i].point[model.params.preferredPathOrtoPos[i]].longitude().degrees(),
		//	model.network.physicalLev[i].nPoints, model.params.nPkterOrto);
	}
	model.network.nPhysicalLevels = i;
	//printf("-- Time2 %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

	//for(i = 0; i < model.network.nPhysicalLevels; i++)
	//	printf("lev %d nPrefPathPoints %d\n", i, model.network.physicalLev[i].npreferredPathPoints);

	if (model.params.varyStartEndArcLength == 0)
		openNeededRasterFilesNew(alt);

	for(i = 0; i < model.network.nPhysicalLevels; i++)
		makeSure_feasibleNodes(i, (int)(nPkterOrto / 2));


	//printf("-- Time4 %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

	load_tss_optiNav();

	// loadChannels();
	checkChannels();
	checkPrefPath_throughExtraNoGo();

	addArcsToNetwork();
	checkMinnesAnvandning(__LINE__);

	if (model.params.hindCast == 0)
		calc_stormsNearby();

	if (SKRIV_UT_NOTHING == 0){
		writeAllChannelsToGeojson();
		//writeAllNodesToShape((char*)"networkNodes");
		writeAllNodesToGeojson((char*)"networkNodes");
		writeAllArcsToGeojson((char*)"networkArcs");
	}
	checkMinnesAnvandning(__LINE__);

	return 0;
}

int adderaNod(int physicalLevel, int pointNr, int timeInterval)
{
	int nAlloc;
	if (model.nNoder >= model.nAllocNoder) {
		model.nAllocNoder += 50000;
		model.Noder = (strNoder*)realloc(model.Noder, model.nAllocNoder * sizeof(strNoder));
		for (int i0 = model.nAllocNoder - 50000; i0 < model.nAllocNoder; i0++)
			model.Noder[i0].UtNod = NULL;
	}

	if (physicalLevel >= 0) {
		if (model.params.nPkterOrto > 2 * model.params.max_changeDirection + 1)
			nAlloc = 2 * model.params.max_changeDirection + 1;
		else
			nAlloc = model.params.nPkterOrto;
		if (physicalLevel < model.network.nPhysicalLevels) {
			model.network.physicalLev[physicalLevel].nodNr_from_pt[pointNr][timeInterval] = model.nNoder;
			nAlloc *= model.functions.speedLevel[physicalLevel].nShip_speedSettings;
		}
		else {
			nAlloc *= model.functions.speedLevel[model.network.nPhysicalLevels - 1].nShip_speedSettings;
		}
	}
	else {
		nAlloc = model.params.nPkterOrto * 2;
		if (pointNr == 0)
			nAlloc *= model.functions.speedChannel[-physicalLevel - 1].nShip_speedSettings;
		else
			nAlloc *= model.functions.speedChannelOut[-physicalLevel - 1].nShip_speedSettings;
		model.network.channel[-physicalLevel - 1].nodNr_from_pt[pointNr][timeInterval] = model.nNoder;
	}
	if (model.nNoder == 86)
		nAlloc = nAlloc;
	if (model.Noder[model.nNoder].UtNod == NULL) {
		model.Noder[model.nNoder].UtNod = (int*)malloc2(nAlloc * sizeof(int));
		model.Noder[model.nNoder].UtNodCost = (double*)malloc2(nAlloc * sizeof(double));
		model.Noder[model.nNoder].outArcNr = (int*)malloc2(nAlloc * sizeof(int));
		//model.Noder[model.nNoder].outArcPos = (int*)malloc2(model.params.ortoDist_nPointsPerHour * model.params.nShip_speedSettings * sizeof(int));
		model.Noder[model.nNoder].nAllocUtNoder = nAlloc;
	}
	model.Noder[model.nNoder].nUtNoder = 0;
	model.Noder[model.nNoder].physicalLevel = physicalLevel;
	model.Noder[model.nNoder].pointNr = pointNr;
	model.Noder[model.nNoder].timeInterval = timeInterval;
	//if (model.nNoder == 10145 || model.nNoder == 11863)
	//	errlog("error: nod %d level %d pointNr %d timeInt %d tidp %d\n", model.nNoder, physicalLevel, pointNr, timeInterval,
	//		model.network.physicalLev[physicalLevel].timeInterval[pointNr][timeInterval]);

	(model.nNoder)++;
	return model.nNoder - 1;
}

int adderaArc(int nodNr1, int nodNr2, double cost, int speedSetting)
{
	int i;

	if (model.nArcs == 92494)
		model.nArcs = model.nArcs;
	if (nodNr1 == 3 && nodNr2 == 39)
		nodNr1 = nodNr1;
	for (i = model.Noder[nodNr1].nUtNoder - 1; i >= 0; i--) {
		if (model.Noder[nodNr1].UtNod[i] == nodNr2) {
			if (model.params.eta_h < 0 || model.params.etaFocus_speed == 0 ||
				model.arc[model.Noder[nodNr1].outArcNr[i]].speedSetting == speedSetting) {
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
			else {
				// lower speed is better: model.params.etaFocus_speed = -1;
				// higher speed is better: model.params.etaFocus_speed = 1;
				if (model.params.etaFocus_speed == 1 && speedSetting > model.arc[model.Noder[nodNr1].outArcNr[i]].speedSetting) {
					model.Noder[nodNr1].UtNodCost[i] = cost;
					if (cost < 0)
						printf("ERROR cost negative %.2lf\n", cost);
					return model.Noder[nodNr1].outArcNr[i];
				}
				if (model.params.etaFocus_speed == -1 && speedSetting < model.arc[model.Noder[nodNr1].outArcNr[i]].speedSetting) {
					model.Noder[nodNr1].UtNodCost[i] = cost;
					if (cost < 0)
						printf("ERROR cost negative %.2lf\n", cost);
					return model.Noder[nodNr1].outArcNr[i];
				}
				return -2;
			}
		}
		if (model.Noder[model.Noder[nodNr1].UtNod[i]].pointNr != model.Noder[nodNr2].pointNr)
			break;
	}

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
	if (model.nArcs == 41034)
		model.nArcs = model.nArcs;
	model.Noder[nodNr1].outArcNr[model.Noder[nodNr1].nUtNoder] = model.nArcs;
	(model.Noder[nodNr1].nUtNoder)++;
	return -1;
}

int adderaArcDelay_felHall(int nodNr1, int nodNr2, double cost, int speedSetting)
{
	int i;
	if (nodNr1 == 3 && nodNr2==39)
		nodNr1 = nodNr1;
	for (i = modelDelay.Noder[nodNr1].nUtNoder - 1; i >= 0; i--) {
		if (modelDelay.Noder[nodNr1].UtNod[i] == nodNr2) {
			if (cost < modelDelay.Noder[nodNr1].UtNodCost[i]) {
				// this speed setting is cheaper than the old one...
				modelDelay.Noder[nodNr1].UtNodCost[i] = cost;
				if (cost < 0)
					printf("ERROR cost negative %.2lf\n", cost);
				return modelDelay.Noder[nodNr1].outArcNr[i];
			}
			else {
				return -2;
			}
		}
		if (modelDelay.Noder[modelDelay.Noder[nodNr1].UtNod[i]].pointNr != modelDelay.Noder[nodNr2].pointNr)
			break;
	}

	if (modelDelay.Noder[nodNr1].nUtNoder >= modelDelay.Noder[nodNr1].nAllocUtNoder) {
		modelDelay.Noder[nodNr1].nAllocUtNoder += 50;
		modelDelay.Noder[nodNr1].UtNod = (int*)realloc(modelDelay.Noder[nodNr1].UtNod, modelDelay.Noder[nodNr1].nAllocUtNoder * sizeof(int));
		modelDelay.Noder[nodNr1].UtNodCost = (double*)realloc(modelDelay.Noder[nodNr1].UtNodCost, modelDelay.Noder[nodNr1].nAllocUtNoder * sizeof(double));
		modelDelay.Noder[nodNr1].outArcNr = (int*)realloc(modelDelay.Noder[nodNr1].outArcNr, modelDelay.Noder[nodNr1].nAllocUtNoder * sizeof(int));
	}
	modelDelay.Noder[nodNr1].UtNod[modelDelay.Noder[nodNr1].nUtNoder] = nodNr2;
	modelDelay.Noder[nodNr1].UtNodCost[modelDelay.Noder[nodNr1].nUtNoder] = cost;
	if (cost < 0)
		printf("ERROR negative cost %.2lf\n", cost);
	if (modelDelay.nArcs == 41034)
		modelDelay.nArcs = modelDelay.nArcs;
	modelDelay.Noder[nodNr1].outArcNr[modelDelay.Noder[nodNr1].nUtNoder] = modelDelay.nArcs;
	(modelDelay.Noder[nodNr1].nUtNoder)++;
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
			if (levNr == 1400 && pointNr == 25&&i==37)
				printf("levPrev %d levNr %d pointNr %d i %d tidInt %d\n", levPrev, levNr, pointNr, i, tidInt);

			(model.network.physicalLev[levNr].nTimeIntervals[pointNr])++;
			if (levNr == 1)
				levNr = levNr;
			return adderaNod(levNr, pointNr, i);
		}
		return model.network.physicalLev[levNr].nodNr_from_pt[pointNr][i];
	}
	else {
		if (levPrev < 0 && levPrev == levNr)
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
			if (levNr == -4 && pointNr == 1)
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

	for (tidp = 0; tidp < 6 && tidp < model.weather[weatherNr].nTimeIntervals; tidp++)
		printf("%s orig lon/lat %.3lf %.3lf size_col/row %.2lf %.2lf value 0 0/1 1 %lf %lf\n",
		model.weather[weatherNr].weatherFileTypeName,
		model.weather[weatherNr].minX, model.weather[weatherNr].maxY,
		model.weather[weatherNr].size_col, model.weather[weatherNr].size_row,
		model.weather[weatherNr].valueCell[tidp][0 * model.weather[weatherNr].nCols + 0],
		model.weather[weatherNr].valueCell[tidp][1 * model.weather[weatherNr].nCols + 1]);

	for (tidp = 0; tidp < 20 && tidp < model.weather[weatherNr].nTimeIntervals; tidp++)
		printf("lon/lat %.3lf %.3lf col/row %.2lf %.2lf tidint %d %lf %lf %lf %lf\n",
			lon, lat,
			colDbl, rowDbl, tidp,
			model.weather[weatherNr].valueCell[tidp][row * model.weather[weatherNr].nCols + col],
			model.weather[weatherNr].valueCell[tidp][row * model.weather[weatherNr].nCols + col+1],
			model.weather[weatherNr].valueCell[tidp][row * model.weather[weatherNr].nCols + col+2],
			model.weather[weatherNr].valueCell[tidp][(row+1) * model.weather[weatherNr].nCols + col]);


	return 0;
}

int evalWeatherPosAlongLine(spherical::Point p1, spherical::Point p2, int* checkPointNr, double* distHittils, double bearing, double dist, double totDist) {
	int i, i1;
	double rowDbl, colDbl;

	spherical::Point pMid = p1;
	double bearingRadians;
	// bearingRadians = (90 - bearing) * M_PI / 180;
	bearingRadians = bearing;

	for (i = *checkPointNr;; i++) {
		if (i >= 98)
			i = i;
		if (i >= model.weatherFunctions.nAllocPoints) {
			model.weatherFunctions.nAllocPoints += 50;
			model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lat = (double*)realloc(model.weatherFunctions.point_lat,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lon = (double*)realloc(model.weatherFunctions.point_lon,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			//model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
			//	model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			}
		}

		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			if (model.weatherFunctions.checkPoint[i].latPos[i1] < 0) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! latPos %d\n", model.weatherFunctions.checkPoint[i].latPos[i1]);
					postRequest("ERROR! latPos " + std::to_string(model.weatherFunctions.checkPoint[i].latPos[i1]), 0);
					printf("\n\n\n\n\n\n\n############################################################\nERROR! latPos %d\n", model.weatherFunctions.checkPoint[i].latPos[i1]);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].latPos[i1] = 0;
			}
			if (model.weatherFunctions.checkPoint[i].latPos[i1] >= model.weather[i1].nRows) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! latPos too high %d (max %d) lat lon %.4lf %.4lf maxY %.4lf minX %.4lf nArcs %d\n", model.weatherFunctions.checkPoint[i].latPos[i1],
						model.weather[i1].nRows - 1, pMid.latitude().degrees(), pMid.longitude().degrees(),
						model.weather[i1].maxY, model.weather[i1].minX, model.nArcs);
					postRequest("ERROR! latPos too high " + std::to_string(model.weatherFunctions.checkPoint[i].latPos[i1]) +
						" max " + std::to_string(model.weather[i1].nRows - 1), 0);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].latPos[i1] = model.weather[i1].nRows - 1;
			}
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			if (model.weatherFunctions.checkPoint[i].lonPos[i1] < 0) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! lonPos %d\n", model.weatherFunctions.checkPoint[i].lonPos[i1]);
					postRequest("ERROR! lonPos " + std::to_string(model.weatherFunctions.checkPoint[i].lonPos[i1]), 0);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].lonPos[i1] = 0;
			}
			if (model.weatherFunctions.checkPoint[i].lonPos[i1] >= model.weather[i1].nCols) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! lonPos too high %d (max %d)\n", model.weatherFunctions.checkPoint[i].lonPos[i1],
						model.weather[i1].nCols - 1);
					postRequest("ERROR! lonPos too high " + std::to_string(model.weatherFunctions.checkPoint[i].lonPos[i1]) +
						" (max " + std::to_string(model.weather[i1].nCols - 1), 0);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].lonPos[i1] = model.weather[i1].nCols - 1;
			}
			model.weatherFunctions.checkPoint[i].pos_latLon[i1] = model.weatherFunctions.checkPoint[i].latPos[i1] *
				model.weather[i1].nCols + model.weatherFunctions.checkPoint[i].lonPos[i1];
		}
		model.weatherFunctions.vesselBearing[i] = bearingRadians;
		model.weatherFunctions.point_lat[i] = pMid.latitude().degrees();
		model.weatherFunctions.point_lon[i] = pMid.longitude().degrees();
		//		errlog("checkP %d lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", i, pMid.latitude().degrees(),
		//			pMid.longitude().degrees(), distHittils, bearing);
		if (*distHittils + dist * 1.05 < totDist) {
			model.weatherFunctions.checkPoint[i].distToNextPkt = dist;
			(*distHittils) += dist;
			// bearing = pMid.bearingTo(p2);

			//pTmp = pMid;
			pMid = p1.destinationPoint(*distHittils * 1000, bearing);
			//distTmp = calc_haversine_dist_latlon(pTmp.latitude().degrees(),
			//	pTmp.longitude().degrees(), pMid.latitude().degrees(),
			//	pMid.longitude().degrees());
			//distTmp2 = pTmp.distanceTo(pMid);

		}
		else {
			model.weatherFunctions.checkPoint[i].distToNextPkt = totDist - (*distHittils);
			(*distHittils) = totDist;
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
	*checkPointNr = i;

	return 0;
}

int calcWeatherPosAlongArc(spherical::Point p1, spherical::Point p2, int tidp)
{
	int i, i1;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, bearingRadians;
	spherical::Point pMid, pTmp;
	double x0, y0, x2, y2;

	y0 = p1.latitude().degrees();
	x0 = p1.longitude().degrees();
	y2 = p2.latitude().degrees();
	x2 = p2.longitude().degrees();
	if (printGlobal == 1)
		printf("lat/lon p1 %.3lf %.3lf p2 %.3lf %.3lf\n",
			y0, x0, y2, x2);


	//totDist = estimateLargeCircleDistance_km(y0, x0, y2, x2); // p1.distanceTo(p2) / 1000;
	totDist = p1.distanceTo(p2) / 1000;
	model.functions.valuesNow.distance = totDist;
	if (printGlobal == 1)
		printf("totDist  %.3lf\n", totDist);

//	if (tidp < 7 * 24000) {
		//dist = model.params.shipSpeed_average;
//		if (tidp < 3 * 24 || tidp >= 7 * 24) {
			dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.weather[0].size_row, x0);
			//printf("dist from %.3lf %.3lf to %.3lf %.3lf is %.3lf\n",
			//	y0, x0, y0 + model.weather[0].size_row, x0, dist);
			globalCount10++;
//		}
//		else {
//			dist = totDist / 2;
//			globalCount11++;
//		}
//	}
//	else {
//		dist = totDist;
//		globalCount12++;
//	}
	//printf("tidp %d dist i cal %.4lf\n", tidp, dist);
	distHittils = 0;
	pMid = p1;
	//bearing = estimateBearingFromToCoords(y0, x0, y2, x2);
	bearing = pMid.bearingTo(p2);
	bearingRadians = (90 - bearing) * M_PI / 180;
	if (bearingRadians < -M_PI)
		bearingRadians += 2 * M_PI;

	for (i = 0;; i++) {
		if (i >= 98)
			i = i;
		if (i >= model.weatherFunctions.nAllocPoints) {
			model.weatherFunctions.nAllocPoints += 50;
			model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lat = (double*)realloc(model.weatherFunctions.point_lat,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lon = (double*)realloc(model.weatherFunctions.point_lon,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			//model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
			//	model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			}
		}

		if (printGlobal == 1)
			printf("i %d lat/lon pMid %.3lf %.3lf bearing %.2lf bearingRadians %.2lf distHittils %.2lf totDist %.2lf exact %.2lf\n", i,
				pMid.latitude().degrees(), pMid.longitude().degrees(), bearing, bearingRadians, distHittils, totDist,
				p1.distanceTo(p2) / 1000);

		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			if (model.weatherFunctions.checkPoint[i].latPos[i1] < 0) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! latPos %d\n", model.weatherFunctions.checkPoint[i].latPos[i1]);
					postRequest("ERROR! latPos " + std::to_string(model.weatherFunctions.checkPoint[i].latPos[i1]), 0);
					printf("\n\n\n\n\n\n\n############################################################\nERROR! latPos %d\n", model.weatherFunctions.checkPoint[i].latPos[i1]);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].latPos[i1] = 0;
			}
			if (model.weatherFunctions.checkPoint[i].latPos[i1] >=  model.weather[i1].nRows) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! latPos too high %d (max %d) lat lon %.4lf %.4lf maxY %.4lf minX %.4lf nArcs %d\n", model.weatherFunctions.checkPoint[i].latPos[i1],
						model.weather[i1].nRows - 1, pMid.latitude().degrees(), pMid.longitude().degrees(),
						model.weather[i1].maxY, model.weather[i1].minX, model.nArcs);
					postRequest("ERROR! latPos too high " + std::to_string(model.weatherFunctions.checkPoint[i].latPos[i1]) +
						" max " + std::to_string(model.weather[i1].nRows - 1), 0);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].latPos[i1] = model.weather[i1].nRows - 1;
			}
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			if (model.weatherFunctions.checkPoint[i].lonPos[i1] < 0) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! lonPos %d\n", model.weatherFunctions.checkPoint[i].lonPos[i1]);
					postRequest("ERROR! lonPos " + std::to_string(model.weatherFunctions.checkPoint[i].lonPos[i1]), 0);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].lonPos[i1] = 0;
			}
			if (model.weatherFunctions.checkPoint[i].lonPos[i1] >= model.weather[i1].nCols) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! lonPos too high %d (max %d)\n", model.weatherFunctions.checkPoint[i].lonPos[i1],
					model.weather[i1].nCols - 1);
					postRequest("ERROR! lonPos too high " + std::to_string(model.weatherFunctions.checkPoint[i].lonPos[i1]) +
						" (max " + std::to_string(model.weather[i1].nCols - 1), 0);
					model.nErrorCoordBB = 1;
				}
				model.weatherFunctions.checkPoint[i].lonPos[i1] = model.weather[i1].nCols - 1;
			}
			model.weatherFunctions.checkPoint[i].pos_latLon[i1] = model.weatherFunctions.checkPoint[i].latPos[i1] *
				model.weather[i1].nCols + model.weatherFunctions.checkPoint[i].lonPos[i1];
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
		//model.weatherFunctions.point[i] = pMid;
		model.weatherFunctions.point_lat[i] = pMid.latitude().degrees();
		model.weatherFunctions.point_lon[i] = pMid.longitude().degrees();
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
	//model.weatherFunctions.modifiedPoints = 0;
	 model.weatherFunctions.checkPoints_totDist = totDist;
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
	dist = estimateLargeCircleDistance_km(model.network.channel[cNr].point_y[0], model.network.channel[cNr].point_x[0],
		model.network.channel[cNr].point_y[0] + model.weather[0].size_row, model.network.channel[cNr].point_x[0]);
	distHittils = 0;
	posLast = pointPos1;
	pMid = model.network.channel[cNr].point[posLast];
	bearing = (90 - model.network.channel[cNr].point[0].bearingTo(model.network.channel[cNr].point[pointPos2])) * M_PI / 180;
	//bearing = (90 - estimateBearingFromToCoords(model.network.channel[cNr].point_y[0], model.network.channel[cNr].point_x[0],
	//	model.network.channel[cNr].point_y[pointPos2], model.network.channel[cNr].point_x[pointPos2])) * M_PI / 180;
	if (bearing < -M_PI)
		bearing += 2 * M_PI;
	for (i = 0;; i++) {
		if (i >= 98)
			i = i;
		if (i >= model.weatherFunctions.nAllocPoints) {
			model.weatherFunctions.nAllocPoints += 50;
			model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lat = (double*)realloc(model.weatherFunctions.point_lat,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lon = (double*)realloc(model.weatherFunctions.point_lon,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			//model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
			//	model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			}
		}
		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			//nr = model.weatherFunctions.lastFileNr[i1];
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			model.weatherFunctions.checkPoint[i].pos_latLon[i1] = model.weatherFunctions.checkPoint[i].latPos[i1] *
				model.weather[i1].nCols + model.weatherFunctions.checkPoint[i].lonPos[i1];
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
		//model.weatherFunctions.point[i] = pMid;
		model.weatherFunctions.point_lat[i] = pMid.latitude().degrees();
		model.weatherFunctions.point_lon[i] = pMid.longitude().degrees();
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
	//model.weatherFunctions.modifiedPoints = 0;
	model.weatherFunctions.checkPoints_totDist = totDist;
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
	if (printGlobal == 1)
		errlog("level %d nprefPoints %d p1 %.3lf %.3lf\n", level,
			model.network.physicalLev[level].npreferredPathPoints, p1.longitude().degrees(), p1.latitude().degrees());
	for (i = 0; i < model.network.physicalLev[level].npreferredPathPoints; i++) {
		//if (printGlobal == 1)
		//	printf("i %d innan totDist %.3lf\n", i, totDist);
		totDist += pMid.distanceTo(model.network.physicalLev[level].preferredPathPoint[i]) / 1000.0;
		//if (i < model.network.physicalLev[level].npreferredPathPoints - 1)
		pMid = model.network.physicalLev[level].preferredPathPoint[i];
	}
	//dist = model.params.shipSpeed_average;
	double x0, y0;
	if (model.network.physicalLev[level].npreferredPathPoints == 0)
		errlog("ERROR! no npreferredPathPoints but trying to use the first one for level %d\n", level);
	//printf("level %d npreferredPathPoints %d\n", level, model.network.physicalLev[level].npreferredPathPoints);
	y0 = model.network.physicalLev[level].preferredPathPoint[0].latitude().degrees();
	x0 = model.network.physicalLev[level].preferredPathPoint[0].longitude().degrees();
	dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.weather[0].size_row, x0);
	distHittils = 0;
	bearing = (90 - p1.bearingTo(pMid)) * M_PI / 180;
	pMid = p1;
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
			model.weatherFunctions.point_lat = (double*)realloc(model.weatherFunctions.point_lat,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			model.weatherFunctions.point_lon = (double*)realloc(model.weatherFunctions.point_lon,
				model.weatherFunctions.nAllocPoints * sizeof(double));
			//model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
			//	model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
			model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
				model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
			for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
				//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				model.weatherFunctions.checkPoint[i1].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			}
		}
		for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
			//nr = model.weatherFunctions.lastFileNr[i1];
			rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
			model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
			model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
			model.weatherFunctions.checkPoint[i].pos_latLon[i1] = model.weatherFunctions.checkPoint[i].latPos[i1] *
				model.weather[i1].nCols + model.weatherFunctions.checkPoint[i].lonPos[i1];
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
		//model.weatherFunctions.point[i] = pMid;
		model.weatherFunctions.point_lat[i] = pMid.latitude().degrees();
		model.weatherFunctions.point_lon[i] = pMid.longitude().degrees();
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
	//model.weatherFunctions.modifiedPoints = 0;
	model.weatherFunctions.checkPoints_totDist = totDist;
	//writePointsToShape((char*)"checkPoints", model.weatherFunctions.point, i);
	model.tmpTid3[1] = std::chrono::high_resolution_clock::now();
	model.duration2 += model.tmpTid3[1] - model.tmpTid3[0];

	return 0;
}

int calcWeatherPosAlongpreferredPathArc_connectChannel(spherical::Point p1, int level1, spherical::Point p2, int level2)
{
	int i, i1, posLast, posEnd, level;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, totDist1, totDist2 = 0;
	spherical::Point pMid, pTmp;

	model.tmpTid3[0] = std::chrono::high_resolution_clock::now();
	pMid = p1;

	if (level1 >= 0) {
		level = level1;
		posLast = -1;
		posEnd = model.network.channel[-level2 - 1].preferredPathPoint_posConnectTo;
		if(posEnd < 0)
			totDist = model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]].distanceTo(p2) / 1000;
		else {
			totDist = model.network.physicalLev[level].preferredPathPoint[posEnd].distanceTo(p2) / 1000;
			posEnd++; // must include the last point as well on the preferred path
		}
		totDist2 = totDist;
	}
	else {
		level = level2 - 1;
		posLast = model.network.channel[-level1 - 1].preferredPathPoint_posConnectFrom;
		posEnd = model.network.physicalLev[level].npreferredPathPoints;
		if (posLast >= 0 && posLast < posEnd)
			pMid = model.network.physicalLev[level].preferredPathPoint[posLast];
		else
			pMid = model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]];
		totDist = p1.distanceTo(pMid) / 1000;
		totDist1 = totDist;
	}

	for (i = posLast + 1; i < posEnd; i++) {
		if (printGlobal == 1)
			printf("i %d innan totDist %.3lf\n", i, totDist);
		totDist += pMid.distanceTo(model.network.physicalLev[level].preferredPathPoint[i]) / 1000.0;
		//if (i < model.network.physicalLev[level].npreferredPathPoints - 1)
		pMid = model.network.physicalLev[level].preferredPathPoint[i];
	}
	//dist = model.params.shipSpeed_average;
	double x0, y0;
	if (model.network.physicalLev[level].npreferredPathPoints == 0)
		errlog("ERROR! no npreferredPathPoints but trying to use the first one for level %d\n", level);
	//printf("level %d npreferredPathPoints %d\n", level, model.network.physicalLev[level].npreferredPathPoints);

	y0 = pMid.latitude().degrees();
	x0 = pMid.longitude().degrees();
	dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.weather[0].size_row, x0);
	distHittils = 0;
	bearing = (90 - p1.bearingTo(p2)) * M_PI / 180;
	pMid = p1;
	if (bearing < -M_PI)
		bearing += 2 * M_PI;
	//printf("bearing from %.3lf %.3lf to %.3lf %.3lf is %.2lf\n",
	//	p1.longitude().degrees(), p1.latitude().degrees(),
	//	model.network.physicalLev[level].preferredPathPoint[posLast].longitude().degrees(), 
	//	model.network.physicalLev[level].preferredPathPoint[posLast].latitude().degrees(), bearing);
	//posLast = -1;

	int checkPointNr = 0;
	if (level1 < 0) { // start in channel, first part straight to first pref path node
		evalWeatherPosAlongLine(pMid, model.network.physicalLev[level].preferredPathPoint[posLast + 1], &checkPointNr, &distHittils, bearing, dist, totDist1);
		if (posLast >= 0)
			pMid = model.network.physicalLev[level].preferredPathPoint[posLast];
		else
			pMid = model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]];
	}

	if (posEnd > posLast + 1) {
		for (i = checkPointNr;; i++) {
			if (i >= 98)
				i = i;
			if (i >= model.weatherFunctions.nAllocPoints) {
				model.weatherFunctions.nAllocPoints += 50;
				model.weatherFunctions.vesselBearing = (double*)realloc(model.weatherFunctions.vesselBearing,
					model.weatherFunctions.nAllocPoints * sizeof(double));
				model.weatherFunctions.point_lat = (double*)realloc(model.weatherFunctions.point_lat,
					model.weatherFunctions.nAllocPoints * sizeof(double));
				model.weatherFunctions.point_lon = (double*)realloc(model.weatherFunctions.point_lon,
					model.weatherFunctions.nAllocPoints * sizeof(double));
				//model.weatherFunctions.point = (spherical::Point*)realloc(model.weatherFunctions.point,
				//	model.weatherFunctions.nAllocPoints * sizeof(spherical::Point));
				model.weatherFunctions.checkPoint = (strCheckPkt*)realloc(model.weatherFunctions.checkPoint,
					model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
				for (i1 = model.weatherFunctions.nAllocPoints - 50; i1 < model.weatherFunctions.nAllocPoints; i1++) {
					//model.weatherFunctions.checkPoint[i1].fileNr = (int*)malloc2(model.nWeatherFiles * sizeof(int));
					model.weatherFunctions.checkPoint[i1].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
					model.weatherFunctions.checkPoint[i1].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
					model.weatherFunctions.checkPoint[i1].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
				}
			}
			for (i1 = 0; i1 < model.nWeatherFiles; i1++) {
				//nr = model.weatherFunctions.lastFileNr[i1];
				rowDbl = (model.weather[i1].maxY - pMid.latitude().degrees()) / model.weather[i1].size_row;
				colDbl = get_colDblFromWeatherFile(i1, pMid.longitude().degrees());
				model.weatherFunctions.checkPoint[i].latPos[i1] = (int)rowDbl;
				model.weatherFunctions.checkPoint[i].lonPos[i1] = (int)colDbl;
				model.weatherFunctions.checkPoint[i].pos_latLon[i1] = model.weatherFunctions.checkPoint[i].latPos[i1] *
					model.weather[i1].nCols + model.weatherFunctions.checkPoint[i].lonPos[i1];
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
			//model.weatherFunctions.point[i] = pMid;
			model.weatherFunctions.point_lat[i] = pMid.latitude().degrees();
			model.weatherFunctions.point_lon[i] = pMid.longitude().degrees();
			//		errlog("checkP %d lat %.3lf lon %.3lf distNu %lf bearing %.2lf\n", i, pMid.latitude().degrees(),
			//			pMid.longitude().degrees(), distHittils, bearing);
			if (distHittils + dist * 1.05 < totDist - totDist2) {
				model.weatherFunctions.checkPoint[i].distToNextPkt = dist;
				distHittils += dist;
				pMid = getNextPointAlongpreferredPathArc(pMid, level, &posLast, distHittils, dist);

			}
			else {
				model.weatherFunctions.checkPoint[i].distToNextPkt = totDist - totDist2 - distHittils;
				distHittils = totDist - totDist2;
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
	}
	else
		i = checkPointNr;
	//	errlog("lastP lat %.3lf lon %.3lf totDist %lf\n", p2.latitude().degrees(),
	//		p2.longitude().degrees(), totDist);

	if (level2 < 0) { // from pref path to channel, straight line from last node in pref path to channel
		if (posEnd < 1)
			evalWeatherPosAlongLine(model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]], p2, &i, &distHittils, bearing, dist, totDist);
		else
			evalWeatherPosAlongLine(model.network.physicalLev[level].preferredPathPoint[posEnd - 1], p2, &i, &distHittils, bearing, dist, totDist);
	}

	model.weatherFunctions.nCheckPoints = i;
	//model.weatherFunctions.modifiedPoints = 0;
	model.weatherFunctions.checkPoints_totDist = totDist;
	//writePointsToShape((char*)"checkPoints", model.weatherFunctions.point, i);
	model.tmpTid3[1] = std::chrono::high_resolution_clock::now();
	model.duration2 += model.tmpTid3[1] - model.tmpTid3[0];

	return 0;
}


int getDirectionAndFactorsDelayedGridFromBearing(double direction, int* dir1, double* factor1, int* dir2, double* factor2) {
	double kvot;
	kvot = direction * model.delayedGrid[0].nTimeIntervals / 360; // M_PI2;
	*dir1 = int(kvot);
	*factor1 = 1 - (kvot - *dir1);
	*factor2 = 1 - *factor1;
	if (*dir1 < model.delayedGrid[0].nTimeIntervals - 1)
		*dir2 = *dir1 + 1;
	else {
		*dir2 = 0;
		*dir1 = model.delayedGrid[0].nTimeIntervals - 1;
	}

	return 0;
}


int getDelayPosFrom_tidp(int tidp) {

	if (tidp * model.params.tIndexGerH < model.network.tidp_startHistoricDataOnly)
		return 0;
	if (tidp * model.params.tIndexGerH > model.network.tidp_lastDelayTidp)
		return model.delay.nDelayed_months - 1;

	int tidPos = tidp * model.params.tIndexGerH - model.network.tidp_startHistoricDataOnly;
	return model.delay.tidpHistorical_ger_delayMapNr[tidPos];
}


double eval_factorDelayedAlongPath(int level1, int level2, int tidp, double* speedDiffCurrent)
{
	int i, pos_latLon, dir1, dir2, posLast, pointPos2, pos1, delayNr, pos2;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, delay = 0;
	double factor1, factor2, direction, speedDiff = 0;
	spherical::Point p1, pMid;
	double x0, y0;

	if (level1 >= 0) {
		pos1 = model.params.preferredPathOrtoPos[level1];
		if (pos1 < 0)
			pos1 = -pos1 - 1;
		p1 = model.network.physicalLev[level1].point[pos1];
		totDist = 0;
		pMid = p1;
		for (i = 0; i < model.network.physicalLev[level1].npreferredPathPoints; i++) {
			if (printGlobal == 1)
				printf("i %d innan totDist %.3lf\n", i, totDist);
			totDist += pMid.distanceTo(model.network.physicalLev[level1].preferredPathPoint[i]) / 1000.0;
			//if (i < model.network.physicalLev[level].npreferredPathPoints - 1)
			pMid = model.network.physicalLev[level1].preferredPathPoint[i];
		}
		if (model.network.physicalLev[level1].npreferredPathPoints == 0)
			errlog("ERROR! no npreferredPathPoints but trying to use the first one for level %d\n", level1);
	}
	else {
		p1 = model.network.channel[-level1-1].point[0];
		pos1 = 1;
		totDist = model.network.channel[-level1 - 1].distance_km;
		pointPos2 = model.network.channel[-level1 - 1].nPoints - 1;
	}
	if (level2 >= 0) {
		pos2 = model.params.preferredPathOrtoPos[level2];
		if (pos2 < 0)
			pos2 = -pos2 - 1;
	}
	else {
		pos2 = 0;
	}

	y0 = p1.latitude().degrees();
	x0 = p1.longitude().degrees();

	dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.delayedGrid[0].size_row, x0);
	distHittils = 0;
	direction = 90 - p1.bearingTo(pMid);// )* M_PI / 180;
	if (direction < 0)
		direction += 360; // 2 * M_PI;
	getDirectionAndFactorsDelayedGridFromBearing(direction, &dir1, &factor1, &dir2, &factor2);
	pMid = p1;
	posLast = -1;
	delayNr = getDelayPosFrom_tidp(tidp);
	for (i = 0;; i++) {
		rowDbl = (model.delayedGrid[0].maxY - pMid.latitude().degrees()) / model.delayedGrid[0].size_row;
		colDbl = get_colDblFromWeatherFile(-1, pMid.longitude().degrees());
		pos_latLon = (int)rowDbl * model.delayedGrid[0].nCols + (int)colDbl;

		if (delayVersion == 5)
			delay += 1;
		else
			delay += 1 + (model.delayedGrid[delayNr].valueCell[dir1][pos_latLon] * factor1 + model.delayedGrid[delayNr].valueCell[dir2][pos_latLon] * factor2 - 1) * model.scaledDelay;
		if (delayVersion == 4)
			speedDiff += getSpeedDiff_currentDelayedFromBearing(level1, pos1, level2, pos2, delayNr, direction, pMid.latitude().degrees(), pMid.longitude().degrees());

		if (distHittils + dist * 1.05 < totDist) {
			distHittils += dist;
			if(level1 >= 0)
				pMid = getNextPointAlongpreferredPathArc(pMid, level1, &posLast, distHittils, dist);
			else
				pMid = getNextPointAlongChannel(-level1 - 1, &posLast, pointPos2, distHittils, dist);
		}
		else {
			i++;
			break;
		}
	}
	model.network.physicalLev[level1].factorDelayedPrefPath = delay / i;
	*speedDiffCurrent = speedDiff / i;
	
	return model.network.physicalLev[level1].factorDelayedPrefPath;
}

double eval_factorDelayedAlongArc(int thisLevel, int pos1, int nextLevel, int pos2, int tidp)
{
	int i, pos_latLon, dir1, dir2, posLast, delayNr;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, delay = 0;
	double factor1, factor2, direction;
	spherical::Point p1, p2, pMid;

	if (thisLevel >= 0) {
		p1 = model.network.physicalLev[thisLevel].point[pos1];
	}
	else {
		p1 = model.network.channel[-thisLevel - 1].point[pos1];
	}
	if (nextLevel >= 0) {
		p2 = model.network.physicalLev[nextLevel].point[pos2];
	}
	else {
		p2 = model.network.channel[-nextLevel - 1].point[pos2];
	}
	totDist = p1.distanceTo(p2) / 1000;

	double x0, y0, x2, y2;
	y0 = p1.latitude().degrees();
	x0 = p1.longitude().degrees();
	y2 = p2.latitude().degrees();
	x2 = p2.longitude().degrees();

	dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.delayedGrid[0].size_row, x0);

	distHittils = 0;
	pMid = p1;
	bearing = estimateBearingFromToCoords(y0, x0, y2, x2); // pMid.bearingTo(p2);
	double bearingRadians = (90 - bearing);// *M_PI / 180;
	//if (bearingRadians < -M_PI)
	//	bearingRadians += M_PI2;
	if (bearingRadians < 0)
		direction = bearingRadians + 360;// M_PI2;
	else
		direction = bearingRadians;

	getDirectionAndFactorsDelayedGridFromBearing(direction, &dir1, &factor1, &dir2, &factor2);

	pMid = p1;
	posLast = -1;
	delayNr = getDelayPosFrom_tidp(tidp);
	for (i = 0;; i++) {
		rowDbl = (model.delayedGrid[0].maxY - pMid.latitude().degrees()) / model.delayedGrid[0].size_row;
		colDbl = get_colDblFromWeatherFile(-1, pMid.longitude().degrees());
		pos_latLon = (int)rowDbl * model.delayedGrid[0].nCols + (int)colDbl;

		delay += 1 + (model.delayedGrid[delayNr].valueCell[dir1][pos_latLon] * factor1 + model.delayedGrid[delayNr].valueCell[dir2][pos_latLon] * factor2 - 1)* model.scaledDelay;
		if (distHittils + dist * 1.05 < totDist) {
			distHittils += dist;
			pMid = p1.destinationPoint(distHittils * 1000, bearing);

		}
		else {
			i++;
			break;
		}
	}
	double factor = delay / i;

	return factor;
}


void getCurrent_fromCurrentDelayed(int delayNr, double lat, double lon, double* uCurrent, double* vCurrent) {
	int i, pos_latLon, row, col;

	for (i = 0; i < 2; i++) {
		row = (int)((model.delayedCurrent[i][0].maxY - lat) / model.delayedCurrent[i][0].size_row);
		col = (int)(get_colDblFromWeatherFile(-2 - i, lon));
		if (row < 0 || row >= model.delayedCurrent[i][0].nRows || col < 0 || col >= model.delayedCurrent[i][0].nCols) {
			errlog("ERROR! wrong row/col %d %d max %d %d for getCurrent i %d row %d, I set it to 0\n",
				row, col, model.delayedCurrent[i][0].nRows, model.delayedCurrent[i][0].nCols, i, __LINE__);
			if (i == 0)
				*uCurrent = 0;
			else
				*vCurrent = 0;
			continue;
		}
		pos_latLon = row * model.delayedCurrent[i][0].nCols + col;
		if (i == 0)
			*uCurrent = model.delayedCurrent[i][delayNr].valueCell[0][pos_latLon];
		else
			*vCurrent = model.delayedCurrent[i][delayNr].valueCell[0][pos_latLon];
	}
}


double getSpeedDiff_currentDelayedFromBearing(int fromLevel, int pos1, int toLevel, int pos2, int delayNr, double bearing_grader, double lat, double lon, double calmWaterSpeed) {
	double currentDirection, currentSpeed, baseGroundSpeed;
	int i, pos_latLon, row, col, restrictedAreaNr;
	double uCurrent, vCurrent;

	for (i = 0; i < 2; i++) {
		row = (int)((model.delayedCurrent[i][0].maxY - lat) / model.delayedCurrent[i][0].size_row);
		col = (int)(get_colDblFromWeatherFile(-2 - i, lon));
		if (row < 0 || row >= model.delayedCurrent[i][0].nRows || col < 0 || col >= model.delayedCurrent[i][0].nCols) {
			errlog("ERROR! wrong row/col %d %d max %d %d for getCurrent i %d row %d, I set it to 0\n",
				row, col, model.delayedCurrent[i][0].nRows, model.delayedCurrent[i][0].nCols, i, __LINE__);
			if (i == 0)
				uCurrent = 0;
			else
				vCurrent = 0;
			continue;
		}
		pos_latLon = row * model.delayedCurrent[i][0].nCols + col;
		if (i == 0)
			uCurrent = model.delayedCurrent[i][delayNr].valueCell[0][pos_latLon];
		else
			vCurrent = model.delayedCurrent[i][delayNr].valueCell[0][pos_latLon];
	}

	if (uCurrent < 1000 && vCurrent < 1000) {
		currentDirection = ApproxAtan2(vCurrent, uCurrent);
		currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
	}
	else {
		currentDirection = 0;
		currentSpeed = 0;
	}

	if (calmWaterSpeed < -0.5) {
		restrictedAreaNr = get_restrictedAreaNr(fromLevel, pos1, -1, toLevel, pos2);
		calmWaterSpeed = eval_calmWaterSpeed(-1, fromLevel, toLevel, restrictedAreaNr);
	}

	baseGroundSpeed = eval_baseGroundSpeed(calmWaterSpeed, bearing_grader / 180 * M_PI,
		currentDirection, currentSpeed);
	return baseGroundSpeed - calmWaterSpeed;
}

double eval_factorDelayedAlongArc_currSpeedDiff(int thisLevel, int pos1, int nextLevel, int pos2, int tidp, double* speedDiffCurrent, double calmWaterSpeed)
{
	int i, pos_latLon, dir1, dir2, posLast, delayNr;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, delay = 0, speedDiff = 0;
	double factor1, factor2, direction;
	spherical::Point p1, p2, pMid;

	if (thisLevel >= 0) {
		p1 = model.network.physicalLev[thisLevel].point[pos1];
	}
	else {
		p1 = model.network.channel[-thisLevel - 1].point[pos1];
	}
	if (nextLevel >= 0) {
		p2 = model.network.physicalLev[nextLevel].point[pos2];
	}
	else {
		p2 = model.network.channel[-nextLevel - 1].point[pos2];
	}
	totDist = p1.distanceTo(p2) / 1000;

	double x0, y0, x2, y2;
	y0 = p1.latitude().degrees();
	x0 = p1.longitude().degrees();
	y2 = p2.latitude().degrees();
	x2 = p2.longitude().degrees();

	dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.delayedGrid[0].size_row, x0);

	distHittils = 0;
	pMid = p1;
	bearing = estimateBearingFromToCoords(y0, x0, y2, x2); // pMid.bearingTo(p2);
	double bearingRadians = (90 - bearing);// *M_PI / 180;
	//if (bearingRadians < -M_PI)
	//	bearingRadians += M_PI2;
	if (bearingRadians < 0)
		direction = bearingRadians + 360;// M_PI2;
	else
		direction = bearingRadians;

	getDirectionAndFactorsDelayedGridFromBearing(direction, &dir1, &factor1, &dir2, &factor2);

	pMid = p1;
	posLast = -1;
	delayNr = getDelayPosFrom_tidp(tidp);
	for (i = 0;; i++) {
		rowDbl = (model.delayedGrid[0].maxY - pMid.latitude().degrees()) / model.delayedGrid[0].size_row;
		colDbl = get_colDblFromWeatherFile(-1, pMid.longitude().degrees());
		pos_latLon = (int)rowDbl * model.delayedGrid[0].nCols + (int)colDbl;

		if (delayVersion == 5)
			delay += 1;
		else
			delay += 1 + (model.delayedGrid[delayNr].valueCell[dir1][pos_latLon] * factor1 + model.delayedGrid[delayNr].valueCell[dir2][pos_latLon] * factor2 - 1) * model.scaledDelay;
		if (delayVersion == 4)
			speedDiff += getSpeedDiff_currentDelayedFromBearing(thisLevel, pos1, nextLevel, pos2, delayNr, bearingRadians, pMid.latitude().degrees(), pMid.longitude().degrees(), calmWaterSpeed);


		if (distHittils + dist * 1.05 < totDist) {
			distHittils += dist;
			if (i == 25)
				i = i;
			pMid = p1.destinationPoint(distHittils * 1000, bearing);

		}
		else {
			i++;
			break;
		}
	}
	double factor = delay / i;

	*speedDiffCurrent = speedDiff / i;

	return factor;
}

double eval_speedDiffCurrent_delayedAlongArc(int thisLevel, int pos1, int nextLevel, int pos2, int tidp, double calmWaterSpeed)
{
	int i, pos_latLon, dir1, dir2, posLast, delayNr;
	double totDist, dist, distHittils, bearing, rowDbl, colDbl, delay = 0, speedDiff = 0;
	double factor1, factor2, direction;
	spherical::Point p1, p2, pMid;

	if (thisLevel >= 0) {
		p1 = model.network.physicalLev[thisLevel].point[pos1];
	}
	else {
		p1 = model.network.channel[-thisLevel - 1].point[pos1];
	}
	if (nextLevel >= 0) {
		p2 = model.network.physicalLev[nextLevel].point[pos2];
	}
	else {
		p2 = model.network.channel[-nextLevel - 1].point[pos2];
	}
	totDist = p1.distanceTo(p2) / 1000;

	double x0, y0, x2, y2;
	y0 = p1.latitude().degrees();
	x0 = p1.longitude().degrees();
	y2 = p2.latitude().degrees();
	x2 = p2.longitude().degrees();

	dist = estimateLargeCircleDistance_km(y0, x0, y0 + model.delayedGrid[0].size_row, x0);

	distHittils = 0;
	pMid = p1;
	bearing = estimateBearingFromToCoords(y0, x0, y2, x2); // pMid.bearingTo(p2);
	double bearingRadians = (90 - bearing);// *M_PI / 180;
	//if (bearingRadians < -M_PI)
	//	bearingRadians += M_PI2;
	if (bearingRadians < 0)
		direction = bearingRadians + 360;// M_PI2;
	else
		direction = bearingRadians;

	pMid = p1;
	posLast = -1;
	delayNr = getDelayPosFrom_tidp(tidp);
	for (i = 0;; i++) {
		rowDbl = (model.delayedGrid[0].maxY - pMid.latitude().degrees()) / model.delayedGrid[0].size_row;
		colDbl = get_colDblFromWeatherFile(-1, pMid.longitude().degrees());
		pos_latLon = (int)rowDbl * model.delayedGrid[0].nCols + (int)colDbl;

		speedDiff += getSpeedDiff_currentDelayedFromBearing(thisLevel, pos1, nextLevel, pos2, delayNr, bearingRadians, pMid.latitude().degrees(), pMid.longitude().degrees(), calmWaterSpeed);
		if (distHittils + dist * 1.05 < totDist) {
			distHittils += dist;
			pMid = p1.destinationPoint(distHittils * 1000, bearing);
		}
		else {
			i++;
			break;
		}
	}
	return  speedDiff / i;
}


double getVariableValue(int varNr, int checkPointNr, double tidpkt)
{
	//int weatherNr = model.variable[varNr].weatherNr;
	int latPos = model.weatherFunctions.checkPoint[checkPointNr].latPos[varNr];
	int lonPos = model.weatherFunctions.checkPoint[checkPointNr].lonPos[varNr];
	int tidInt, tidIndex;

	globalCount1++;


	//if (model.weather[varNr].useStandardWeather != 0) {
	//	tidInt = -1;
	//	if (model.weather[varNr].useStandardWeather == -1)
	//		tidIndex = 0;
	//	else {
	//		tidIndex = model.weather[varNr].nTimeIntervals - 1;
	//	}
	//}
	//else {
	tidInt = (int)(tidpkt * model.weather_inv_timeIntervall_h);
	if (tidInt > model.weather_nTimeIntervals_maxValue)
		tidInt = model.weather_nTimeIntervals_maxValue;
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
		printf("varNr %d tidIndex %d latPos %d lonPos %d value %.3lf\n", varNr, tidIndex, latPos, lonPos,
			model.weather[varNr].valueCell[tidIndex][latPos * model.weather[varNr].nCols + lonPos]);
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

double getStormValuePrecis(int t, double lat, double lon, int saveStormData)
{
	int i, tidIndex, tidInt, nara = 0;
	double varde = 0, kvot, dist, bearingFromStorm, bearingStormMove;
	double bearingDiff, distanceMove;
	double outerCircleSize, outerCircleNext, innerCircleSize, innerCircleNext;
	spherical::Point pointStorm;
	double latStorm, lonStorm;

	for (i = 0; i < model.nStorms; i++) {
		tidInt = t * model.storms[i].inv_timeIntervall_h;
		if (tidInt > model.storms[i].nTimeIntervals_maxValue)
			tidInt = model.storms[i].nTimeIntervals_maxValue;
		//printf("storm %d t %d tidInt %d inv_timeInt %.3lf maxVal %d\n", 
		//	i, t, tidInt, model.storms[i].inv_timeIntervall_h, model.storms[i].nTimeIntervals_maxValue);
		//printf("index %d\n", model.storms[i].timeIntervalIndex[tidInt]);
		tidIndex = model.storms[i].timeIntervalIndex[tidInt];
		if (tidIndex < model.storms[i].nFeatures) {
			if (tidIndex == 0) {
				if(t < model.storms[i].feature[tidIndex].tidFromStart_h - 6)
					continue; // too long before the storm starts to be valid
			}
			kvot = (t - model.storms[i].feature[tidIndex].tidFromStart_h) / model.storms[i].feature[tidIndex].hoursToNextPoint;
			bearingStormMove = model.storms[i].feature[tidIndex].bearing;
			if (kvot > 0.05) {
				// distanceMove = model.storms[i].feature[pos].midPoint.distanceTo(model.storms[i].feature[pos + 1].midPoint);
				//distanceMove = model.storms[i].feature[tidIndex].distanceToNextPoint * kvot; // .midPoint.distanceTo(model.storms[i].feature[pos + 1].midPoint);
				// bearing = model.storms[i].feature[pos].midPoint.bearingTo(model.storms[i].feature[pos + 1].midPoint));
				//model.tmpTid2[7] = std::chrono::high_resolution_clock::now();
				// pointStorm = model.storms[i].feature[tidIndex].midPoint.destinationPoint(distanceMove, bearingStormMove);
				estimateDestinationPointStorm(i, tidIndex, kvot, &latStorm, &lonStorm);

				//model.durationStormDestPoint += std::chrono::high_resolution_clock::now() - model.tmpTid2[7];
				if (tidIndex < model.storms[i].nFeatures - 1) {
					outerCircleNext = model.storms[i].feature[tidIndex + 1].outerCircleSize;
				}else
					outerCircleNext = model.storms[i].feature[tidIndex].outerCircleSize;
				outerCircleSize = model.storms[i].feature[tidIndex].outerCircleSize * (1 - kvot) + kvot * outerCircleNext;
			}
			else {
				//pointStorm = model.storms[i].feature[tidIndex].midPoint;
				latStorm = model.storms[i].feature[tidIndex].lat;
				lonStorm = model.storms[i].feature[tidIndex].lon;
				outerCircleSize = model.storms[i].feature[tidIndex].outerCircleSize;
			}

			//model.tmpTid2[7] = std::chrono::high_resolution_clock::now();
			//dist = pointStorm.distanceTo(point) / 1000.0; //  / 1852; // rakna om till nautiska miles...
			//dist = estimateLargeCircleDistance(pointStorm.latitude().degrees(), pointStorm.longitude().degrees(),
			//	point.latitude().degrees(), point.longitude().degrees()); //  / 1852; // rakna om till nautiska miles...
			dist = estimateLargeCircleDistance_km(latStorm, lonStorm, lat, lon);

			//if(distExact < 100)
			//	printf("compare dists %.2lf %.2lf diff %.2lf\n", distExact, dist, distExact - dist);
			// dist = model.storms[i].feature[pos].midPoint.distanceTo(point) / 1852; // rakna om till nautiska miles...
			if (dist >= outerCircleSize) {
				//model.durationStormBearingTo += std::chrono::high_resolution_clock::now() - model.tmpTid2[7];
				//globalCount1++;
				if (dist < outerCircleSize * 1.5)
					nara = 1;
				continue; // too far from storm, no problem
			}
			bearingFromStorm = estimateBearingFromToCoords(latStorm, lonStorm, lat, lon);// pointStorm.bearingTo(point);
			//model.durationStormBearingTo += std::chrono::high_resolution_clock::now() - model.tmpTid2[7];
			//globalCount2++;
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
			if (printGlobal == 1)
				printf("storm %d innerCircleSize %.2lf dist %.2lf outerCircleSize %.2lf storm %.3lf %.3lf rutt %.3lf %.3lf\n"
					"tidskvot %.2lf tRutt %d prevStormT %.2lf hoursToNextStormT %.2lf\n", i, innerCircleSize, dist, outerCircleSize,
					lonStorm, latStorm, lon, lat, kvot, t, model.storms[i].feature[tidIndex].tidFromStart_h, model.storms[i].feature[tidIndex].hoursToNextPoint);
			if (dist <= innerCircleSize) {
				if (t < model.network.tidp_startHistoricDataOnly && model.network.tidp_startHistoricDataOnly < 999998)
					varde += model.params.penalties.storm_costInsideInner * (1.0 + (model.network.tidp_startHistoricDataOnly - t) /
						model.network.tidp_startHistoricDataOnly);
				else
					varde += model.params.penalties.storm_costInsideInner;
				varde += (innerCircleSize - dist) * 100000;
				if (varde > 2 * model.params.penalties.storm_costInsideInner)
					varde = 2 * model.params.penalties.storm_costInsideInner;
			}else
				varde += model.params.penalties.storm_costInsideOuter_kvot * (outerCircleSize - dist) / (outerCircleSize - innerCircleSize);
			if (saveStormData == 1) {
				if (dist < model.storms[i].closestPointToRoute) {
					model.storms[i].closestPointToRoute = dist;
					if (SKRIV_UT_NOTHING == 0)
						errlog("storm %d dist %.2lf cost %.2lf tidp %d\n", i, dist, varde, t);
				}
			}
		}
	}
	if (abs(varde) < 1 && nara == 1)
		varde = -1;

	return varde;
}

double getStormValue(int t, double lat, double lon, int saveStormData)
{
	double varde, varde1;
	varde = getStormValuePrecis(t, lat, lon, saveStormData);
	if (abs(varde) > 0.9) {
		if (t - 1 >= 0) {
			varde1 = getStormValuePrecis(t - 1, lat, lon, saveStormData);
			if (varde1 > varde)
				varde = varde1;
		}
		varde1 = getStormValuePrecis(t + 1, lat, lon, saveStormData);
		if (varde1 > varde)
			varde = varde1;
		if (varde < 0)
			varde = 0;
	}
	return varde;
}

int determine_nSpeedSettingsToUse(int level1, int level2, int restrictedAreaNr) {
	int nSettings, speedNr;
	double maxSpeed;
	if (level1 >= 0) {
		if (level2 == -1000) {
			nSettings = model.functions.nSpeedSettingsDelay[level1];
			if (restrictedAreaNr >= 0) {
				maxSpeed = model.restrictedArea[restrictedAreaNr].max_speed;
				for (speedNr = 0; speedNr < nSettings; speedNr++) {
					if (maxSpeed <= model.functions.rpmSetting_gerCalmWaterSpeedDelay[level1][speedNr])
						break;
				}
				if (speedNr < nSettings)
					nSettings = speedNr + 1;
			}
		}
		else {
			nSettings = model.functions.speedLevel[level1].nShip_speedSettings;
			if (restrictedAreaNr >= 0) {
				maxSpeed = model.restrictedArea[restrictedAreaNr].max_speed;
				for (speedNr = 0; speedNr < nSettings; speedNr++) {
					if (maxSpeed <= model.functions.speedLevel[level1].rpmSetting_gerCalmWaterSpeed[speedNr])
						break;
				}
				if (speedNr < nSettings)
					nSettings = speedNr + 1;
			}
		}
	}
	else {
		if (level2 >= 0) {
			nSettings = model.functions.speedChannelOut[-level1 - 1].nShip_speedSettings;
			if (restrictedAreaNr >= 0) {
				maxSpeed = model.restrictedArea[restrictedAreaNr].max_speed;
				for (speedNr = 0; speedNr < nSettings; speedNr++) {
					if (maxSpeed <= model.functions.speedChannelOut[-level1 - 1].rpmSetting_gerCalmWaterSpeed[speedNr])
						break;
				}
				if (speedNr < nSettings)
					nSettings = speedNr + 1;
			}
		}
		else {
			nSettings = model.functions.speedChannel[-level1 - 1].nShip_speedSettings;
			if (restrictedAreaNr >= 0) {
				maxSpeed = model.restrictedArea[restrictedAreaNr].max_speed;
				for (speedNr = 0; speedNr < nSettings; speedNr++) {
					if (maxSpeed <= model.functions.speedChannel[-level1 - 1].rpmSetting_gerCalmWaterSpeed[speedNr])
						break;
				}
				if (speedNr < nSettings)
					nSettings = speedNr + 1;
			}
		}
	}
	return nSettings;
}


double eval_calmWaterSpeed(int speedNr, int fromLevel, int toLevel, int restrictedAreaNr) {
	double varde;

	if (speedNr < 0) {
		varde = -1;
		varde = model.params.preferredSpeed_calmWater;
		if (fromLevel < 0 && toLevel < 0) {
			if (model.network.channel[-fromLevel - 1].timeThroughChannel >= 0)
				varde = model.network.channel[-fromLevel - 1].distance_km /
				model.network.channel[-fromLevel - 1].timeThroughChannel;
		}
		if (varde < 0) {
			printf("ERROR! wrong fix speed before/after a channel for fromLevel %d toLevel %d\n",
				fromLevel, toLevel);
			errlog("ERROR! wrong fix speed before/after a channel for fromLevel %d toLevel %d\n",
				fromLevel, toLevel);
		}
		//printf("speed %.2lf arcNr %d\n", varde, arcNr);
	}
	else {
		if (toLevel == -1000)
			varde = model.functions.rpmSetting_gerCalmWaterSpeedDelay[fromLevel][speedNr];
		else {
			if (fromLevel >= 0)
				varde = model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed[speedNr];
			else {
				if (toLevel >= 0)
					varde = model.functions.speedChannelOut[-fromLevel - 1].rpmSetting_gerCalmWaterSpeed[speedNr];
				else {
					if (toLevel != -100)
						varde = model.functions.speedChannel[-fromLevel - 1].rpmSetting_gerCalmWaterSpeed[speedNr];
					else
						varde = model.functions.rpmSetting_gerCalmWaterSpeedBase[speedNr];
				}
			}
		}
		//varde = model.functions.rpmSetting_gerCalmWaterSpeed[speedNr];// .calmWaterSpeed.c0
		//+ model.functions.calmWaterSpeed.c1_rpm * model.functions.rpm[speedNr]
		//+ model.functions.calmWaterSpeed.c2_rpm * model.functions.rpm[speedNr] * model.functions.rpm[speedNr];
	}

	if (restrictedAreaNr >= 0) {
		if (varde > model.restrictedArea[restrictedAreaNr].max_speed)
			varde = model.restrictedArea[restrictedAreaNr].max_speed;
	}

	return varde;
}

int get_speedSettingBase(int arcNr) {
	int fromLevel = model.arc[arcNr].fromLevel;
	int toLevel = model.arc[arcNr].toLevel;
	int speedNr = model.arc[arcNr].speedSetting;
	int speedNrBase = -1;

	if (fromLevel >= 0)
		speedNrBase = model.functions.speedLevel[fromLevel].settingGerBaseSetting[speedNr];
	else {
		if (model.network.channel[-fromLevel - 1].timeThroughChannel > 0)
			return -1; // fix time in channel
		if (toLevel >= 0)
			speedNrBase = model.functions.speedChannelOut[-fromLevel - 1].settingGerBaseSetting[speedNr];
		else
			speedNrBase = model.functions.speedChannel[-fromLevel - 1].settingGerBaseSetting[speedNr];
	}
	return speedNrBase;
}

//double eval_calmWaterSpeed_base(int speedNr) {
//	double varde;
//
//	varde = model.functions.rpmSetting_gerCalmWaterSpeedBase[speedNr];
//	return varde;
//}

double eval_fuelConsumption_both(int speedNr, double* consumptionAux, int fromLevel, int toLevel, int restrictedAreaNr, double calmWaterSpeed, double fuelFactorMain) {
	double vardeMain, vardeAux, factor;

	if (speedNr < 0) {
		//factor = -1;
		//if (fromLevel >= 0) {
		//	if (model.params.preferredPathUseChannelConsumption[fromLevel] >= 0)
		//		factor = model.network.channel[model.params.preferredPathUseChannelConsumption[fromLevel]].totalConsumption;
		//}
		//else {
		//	if (toLevel >= 0) {
		//		if (model.params.preferredPathUseChannelConsumption[toLevel] >= 0)
		//			factor = model.network.channel[model.params.preferredPathUseChannelConsumption[toLevel]].totalConsumption;
		//	}
		//}
		//if (factor < 0) {
		//	printf("ERROR! wrong fix speed before/after a channel for fromLevel %d toLevel %d\n",
		//		fromLevel, toLevel);
		//	errlog("ERROR! wrong fix speed before/after a channel for fromLevel %d toLevel %d\n",
		//		fromLevel, toLevel);
		//	factor = 1;
		//}
		if (model.delay.nDelayed_months == 0) {
			factor = 1;
			vardeMain = model.functions.rpmSetting_gerFuelConsumption_mainBase[model.functions.speedSetting95MCR_base] * factor;
			vardeAux = model.functions.rpmSetting_gerFuelConsumption_auxBase[model.functions.speedSetting95MCR_base];// .functions.fuelConsumption.c0
		}
		else {
			factor = fuelFactorMain;
			vardeMain = model.functions.rpmSetting_gerFuelConsumption_mainBase[model.functions.speedSetting95MCR_base] * factor;
			vardeAux = model.functions.rpmSetting_gerFuelConsumption_auxBase[model.functions.speedSetting95MCR_base];// .functions.fuelConsumption.c0
		}
	}
	else {
		if (fromLevel >= 0) {
			if (toLevel == -1000) {
				vardeMain = model.functions.rpmSetting_gerFuelConsumption_mainDelay[fromLevel][speedNr];// .functions.fuelConsumption.c0
				vardeAux = model.functions.rpmSetting_gerFuelConsumption_auxDelay[fromLevel][speedNr];// .functions.fuelConsumption.c0
			}
			else {
				vardeMain = model.functions.speedLevel[fromLevel].rpmSetting_gerFuelConsumption_main[speedNr];// .functions.fuelConsumption.c0
				vardeAux = model.functions.speedLevel[fromLevel].rpmSetting_gerFuelConsumption_aux[speedNr];// .functions.fuelConsumption.c0
			}
		}
		else {
			if (toLevel >= 0) {
				vardeMain = model.functions.speedChannelOut[-fromLevel - 1].rpmSetting_gerFuelConsumption_main[speedNr];// .functions.fuelConsumption.c0
				vardeAux = model.functions.speedChannelOut[-fromLevel - 1].rpmSetting_gerFuelConsumption_aux[speedNr];// .functions.fuelConsumption.c0
			}
			else {
				if (toLevel != -100) {
					if (model.network.channel[-fromLevel - 1].totalConsumption >= 0)
						vardeMain = model.network.channel[-fromLevel - 1].totalConsumption / model.network.channel[-fromLevel - 1].timeThroughChannel;
					else
						vardeMain = model.functions.speedChannel[-fromLevel - 1].rpmSetting_gerFuelConsumption_main[speedNr];// .functions.fuelConsumption.c0
					vardeAux = model.functions.speedChannel[-fromLevel - 1].rpmSetting_gerFuelConsumption_aux[speedNr];// .functions.fuelConsumption.c0
				}
				else {
					vardeMain = model.functions.rpmSetting_gerFuelConsumption_mainBase[speedNr];// .functions.fuelConsumption.c0
					vardeAux = model.functions.rpmSetting_gerFuelConsumption_auxBase[speedNr];// .functions.fuelConsumption.c0
				}
			}
		}
		if (restrictedAreaNr >= 0){
			if (calmWaterSpeed >= model.restrictedArea[restrictedAreaNr].max_speed - 0.001 && model.restrictedArea[restrictedAreaNr].main_fuelConsumption > -0.5)
				vardeMain = model.restrictedArea[restrictedAreaNr].main_fuelConsumption;
		}

	//+ model.functions.fuelConsumption.c1_rpm * model.functions.rpm[speedNr]
		//+ model.functions.fuelConsumption.c2_rpm * model.functions.rpm[speedNr] * model.functions.rpm[speedNr]
		//+ model.functions.fuelConsumption.c3_rpm * model.functions.rpm[speedNr] * model.functions.rpm[speedNr] * model.functions.rpm[speedNr];
	}
	*consumptionAux = vardeAux;
	return vardeMain * fuelFactorMain;
	//return model.functions.rpmSetting_gerFuelConsumption[speedNr];
}

//double eval_fuelConsumption_aux(int speedNr) {
//	return model.functions.rpmSetting_gerFuelConsumption_aux[speedNr];
//}

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

/*
int getHeightIndex(double height, strFunkData funcData) {
	int index;

	printf("height %lf indexSize %lf nIndex %d\n", height, funcData.waveHeightIndexSize, funcData.nWaveHeightIndex);
	if (height < 0)
		index = 0;
	else {
		index = (int)(height * funcData.inv_waveHeightIndexSize);
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
		index = (int)(period * funcData.inv_wavePeriodIndexSize);
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
		index = (int)(wSpeed * funcData.inv_windSpeedIndexSize);
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
		index = (int)(windDir * funcData.inv_windDirIndexSize);
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
*/

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
	sinA = lookUpSin(rel_currentDir);
	if (sinA < 0.01)
		speed = calmWaterSpeed + currentSpeed * lookUpCos(rel_currentDir);
	else {
		tmp = currentSpeed * sinA / calmWaterSpeed;
		if (tmp > 1)
			tmp = 1;
		if (tmp < -1)
			tmp = -1;
		B = asin(tmp);
		C = M_PI - B - rel_currentDir;

		speed = calmWaterSpeed * lookUpSin(C) / sinA;
	}
	if (speed > calmWaterSpeed + currentSpeed)
		speed = calmWaterSpeed + currentSpeed;
	if (speed < calmWaterSpeed - currentSpeed)
		speed = calmWaterSpeed - currentSpeed;
	if (speed < 0.0001)
		speed = 0.0001;
	//printf("calmWaterSpeed %.2lf bearing %.2lf currentDir %.2lf currentSpeed %.2lf rel_currentDir %.2lf speed %.2lf sinA %.2lf B %.2lf currentSpeed * sinA / calmWaterSpeed %.2lf\n",
	//	calmWaterSpeed, bearing, currentDir, currentSpeed, rel_currentDir, speed, sinA, B, currentSpeed * sinA / calmWaterSpeed);
	return speed;
}

double eval_baseGroundSpeedExact(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed) {
	double rel_currentDir, speed, B, C, sinA, tmp;

	// basic variant, not correct
	// rel_currentDir = currentDir * 180.0 / M_PI - bearing;
	// speed_y = calmWaterSpeed + currentSpeed * cos(rel_currentDir);
	// speed_x = currentSpeed * sin(rel_currentDir);
	// speed = sqrt(speed_y * speed_y - speed_x * speed_x);

	rel_currentDir = abs(currentDir - bearing);
	if (rel_currentDir > M_PI)
		rel_currentDir = 2 * M_PI - rel_currentDir;
	sinA = lookUpSin(rel_currentDir);
	if (sinA < 0.01)
		speed = calmWaterSpeed + currentSpeed * lookUpCos(rel_currentDir);
	else {
		tmp = currentSpeed * sinA / calmWaterSpeed;
		if (tmp > 1)
			tmp = 1;
		if (tmp < -1)
			tmp = -1;
		B = asin(tmp);
		C = M_PI - B - rel_currentDir;

		speed = calmWaterSpeed * lookUpSin(C) / sinA;
		//printf("// currDir %.3lf currentSpeed %.3lf calmSpeed %.3lf bearing %.3lf rel_currDir %.3lf tmp %.3lf B %.3lf C %.3lf speedNew %.3lf\n",
		//	currentDir, currentSpeed, calmWaterSpeed, bearing, rel_currentDir, tmp, B, C, speed);
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

	windDir -= bearing; // in radians relative to ship bearing
	// x = windSpeed * cos(windDir) - baseGroundSpeed;
	x = baseGroundSpeed - windSpeed * lookUpCos(windDir); // since 0 radians is head on and PI is from the back
	// y = windSpeed * sin(windDir);
	y = windSpeed * lookUpSin(windDir);
	//direction = atan2(y, x);
	direction = ApproxAtan2(y, x);
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

double eval_relWindSpeedExact(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir) {
	double rel_speed, x, y, direction;

	//printf("dir ship wind %.2lf %.2lf\n", bearing * 180 / M_PI, windDir * 180 / M_PI);

	//printf("-- bearing %.4lf windDir %.4lf", bearing, windDir);
	windDir -= bearing; // in radians relative to ship bearing
	// x = windSpeed * cos(windDir) - baseGroundSpeed;
	x = baseGroundSpeed - windSpeed * lookUpCos(windDir); // since 0 radians is head on and PI is from the back
	y = windSpeed * sin(windDir);
	direction = atan2(y, x);
	//direction = ApproxAtan2(y, x);
	if (direction < 0)
		direction = -direction;
	if (direction > M_PI)
		direction = 2 * M_PI - direction;
	*rel_windDir = direction;
	rel_speed = sqrt(x * x + y * y);
	//printf(" after %.4lf speed %.4lf x %.4lf y %.4lf dir %.4lf relSpeed %.4lf\n",
	//	windDir, baseGroundSpeed, x, y, direction, rel_speed);

	//printf("windDir rel ship %.2lf xy rel ship %.2lf %.2lf get rel_wind speed dir %.2lf %.2lf\n",
	//	windDir * 180 / M_PI, x, y, rel_speed, *rel_windDir * 180 / M_PI);
	//printf("relWindSpeed xy %.2lf %.2lf windDir %.2lf bearing %.2lf relWindDir %.2lf windSpeed %.2lf baseGroundSpeed %.2lf\n", 
	//	x, y, windDir, bearing, *rel_windDir, windSpeed, baseGroundSpeed);
	return rel_speed;
}

double eval_absWindDirDiff(double bearing, double windDir) {

	windDir -= bearing; // in radians relative to ship bearing
	windDir = M_PI - windDir;
	// obs, PI radians when ship direction and wind direction are the same
	//      0 when ship direction and wind direction are opposites
	if (windDir < 0) {
		windDir += M_PI2;
		if (windDir < 0)
			windDir += M_PI2;
	}
	else {
		if (windDir >= M_PI2) {
			windDir -= M_PI2;
			if (windDir >= M_PI2)
				windDir -= M_PI2;
		}
	}
	if (windDir >= M_PI)
		windDir = M_PI2 - windDir; // if it is from the left or from the right doesn't matter
	//windDir *= 180 / M_PI;
	return windDir;
}

double eval_relWindSpeedApprox(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir) {
	float rel_speed, x, y, direction;

	windDir -= bearing; // in radians relative to ship bearing
	x = windSpeed * ApproxCos(windDir) - baseGroundSpeed;
	y = windSpeed * ApproxSin(windDir);
	direction = ApproxAtan2(y, x);
	if (direction < 0)
		direction = -direction;
	if (direction > M_PI)
		direction = 2 * M_PI - direction;
	*rel_windDir = direction;
	rel_speed = sqrt(x * x + y * y);
	return rel_speed;
}

double eval_relWindSpeedLookUp(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir) {
	float rel_speed, x, y, direction;

	//printf("dir ship wind %.2lf %.2lf\n", bearing * 180 / M_PI, windDir * 180 / M_PI);

	//windDir = (windDir - (M_PI + bearing)); // in radians relative to ship bearing
	windDir -= bearing; // in radians relative to ship bearing
	x = windSpeed * lookUpCos(windDir) - baseGroundSpeed;
	y = windSpeed * lookUpSin(windDir);
	direction = ApproxAtan2(y, x);
	if (direction < 0)
		direction = -direction;
	if (direction > M_PI)
		direction = 2 * M_PI - direction;
	*rel_windDir = direction;
	//rel_speed = sqrt(x * x + y * y);
	rel_speed = ApproxSqrt(x * x + y * y);
	//rel_speed = sqrt3(x * x + y * y);
	//rel_speed = (x+y);
	//printf("windDir rel ship %.2lf xy rel ship %.2lf %.2lf get rel_wind speed dir %.2lf %.2lf\n",
	//	windDir * 180 / M_PI, x, y, rel_speed, *rel_windDir * 180 / M_PI);
	//printf("relWindSpeed xy %.2lf %.2lf windDir %.2lf bearing %.2lf relWindDir %.2lf windSpeed %.2lf baseGroundSpeed %.2lf\n", 
	//	x, y, windDir, bearing, *rel_windDir, windSpeed, baseGroundSpeed);
	return rel_speed;
}

/*
int get_relWindDirIndex(double rel_windDir, strFunkData funkData, int alt) { // windDir: [-pi, +pi]

	rel_windDir = M_PI - rel_windDir; // need it in opposite direction
	if (rel_windDir < 0)
		rel_windDir = -rel_windDir; // windDir [0, +pi]
	if (rel_windDir > M_PI)
		rel_windDir = 2 * M_PI - rel_windDir;
	int index = (int)((rel_windDir - funkData.windDir_min) * funkData.inv_windDirIndexSize);
	// .*model.functions.table_niWindDir / M_PI;// 180.0;
	// printf("rel_windDir %.2lf index %d min %.2lf size %.3lf\n", rel_windDir, )
	if (index < (rel_windDir - funkData.windDir_min) * funkData.inv_windDirIndexSize - 0.5)
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
	int index = (int)((rel_waveDir - funkData.waveDir_min) * funkData.inv_waveDirIndexSize);// table_niWaveDir / M_PI;// 180.0;
	if (index < (rel_waveDir - funkData.waveDir_min) * funkData.inv_waveDirIndexSize - 0.5)
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

	int tmp = (int)((rel_windSpeed - funkData.windSpeed_min) * funkData.inv_windSpeedIndexSize); // *model.functions.rel_windSpeed_kvotIndex;
	//printf("rel_windSpeed %.2lf tmp %d min %.3lf indexSize %.3lf", rel_windSpeed,
	//	tmp, funkData.windSpeed_min, funkData.windSpeedIndexSize);
	if (tmp < (rel_windSpeed - funkData.windSpeed_min) * funkData.inv_windSpeedIndexSize - 0.5)
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
*/

int get_tableIndex(double value, strTableParam param, int alt) {
	int tmp = (int)((value - param.minValue) * param.inv_intervalSize);
	if (tmp < (value - param.minValue) * param.inv_intervalSize - 0.5)
		tmp++;
	//if (tmp < 0)
	//	printf("tmp %d value %lf minValue %lf inv_interval %lf\n",
	//		tmp, value, param.minValue, param.inv_intervalSize);
	if (tmp >= param.nIndex && alt == 0)
		return param.nIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

int get_tableIndexDirection(double value, strTableParam param, int alt) {
	//double dirUse = M_PI - value;
	//if (dirUse < 0)
	//	dirUse = -dirUse;
	//if (dirUse > M_PI)
	//	dirUse = M_PI2 - dirUse;

	int tmp = (int)((value - param.minValue) * param.inv_intervalSize);
	if (tmp < (value - param.minValue) * param.inv_intervalSize - 0.5)
		tmp++;
	if (tmp >= param.nIndex && alt == 0)
		return param.nIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}

/*
int get_calmWaterSpeedIndex(double speed, strFunkData funkData, int alt) {

	int tmp = (int)((speed - funkData.calmWaterSpeed_min) * funkData.inv_calmWaterSpeedIndexSize);
	//printf("speed %.3lf tmp %d min %.3lf indexSize %.3lf\n", speed, tmp, funkData.calmWaterSpeed_min,
	//	funkData.calmWaterSpeedIndexSize);
	if (tmp < (speed - funkData.calmWaterSpeed_min) * funkData.inv_calmWaterSpeedIndexSize - 0.5)
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

	int tmp = (int)((speed - funkData.shipSpeed_min) * funkData.inv_shipSpeedIndexSize);
	if (tmp < (speed - funkData.shipSpeed_min) * funkData.inv_shipSpeedIndexSize - 0.5)
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
	int tmp = (int)((waveHeight - funkData.waveHeight_min) * funkData.inv_waveHeightIndexSize); 

	if (tmp < (waveHeight - funkData.waveHeight_min) * funkData.inv_waveHeightIndexSize - 0.5)
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
	int tmp = (int)((wavePeriod - funkData.wavePeriod_min) * funkData.inv_wavePeriodIndexSize);
	if (tmp < (wavePeriod - funkData.wavePeriod_min) * funkData.inv_wavePeriodIndexSize - 0.5)
		tmp++;
	if (tmp >= funkData.nWavePeriodIndex && alt == 0)
		return funkData.nWavePeriodIndex - 1;
	else {
		if (tmp < 0 && alt == 0)
			tmp = 0;
		return tmp;
	}
}
*/

double lookup_speedDiffWindTable(double calmWaterSpeed, double rel_windSpeed, double rel_windDir) {
	int iWindDir = get_tableIndexDirection(rel_windDir, model.functions.windFactor.windDirection); // get_relWindDirIndex(rel_windDir, model.functions.weatherFactors);
	int iWindSpeed = get_tableIndex(rel_windSpeed, model.functions.windFactor.windSpeed); // get_relWindSpeedIndex(rel_windSpeed, model.functions.weatherFactors);
	int iCalmWaterSpeed = get_tableIndex(calmWaterSpeed, model.functions.windFactor.shipSpeedCalmWater); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors);
	int pos;

	pos = iCalmWaterSpeed + model.functions.windFactor.shipSpeedCalmWater.nIndex *
		(iWindDir + model.functions.windFactor.windDirection.nIndex * iWindSpeed);
	if (printGlobal == 1)
		errlog("** wind table index dir %d windspeed %d baseShipSpeed %d baseShipSpeed %.3lf pos %d value %.4lf\n", 
			iWindDir, iWindSpeed, iCalmWaterSpeed, calmWaterSpeed, pos,
			model.functions.windFactor.tableValue[pos]);
	//printf("pos %d max %d\n", pos, model.functions.windFactor.shipSpeedCalmWater.nIndex * 
	//	model.functions.windFactor.windDirection.nIndex * model.functions.windFactor.windSpeed.nIndex);
	return model.functions.windFactor.tableValue[pos]; // windSpeed, windDir, calmWaterSpeed

}

double lookup_speedDiffWaveTable(double calmWaterSpeed, double waveHeight, double wavePeriod, double rel_waveDir) {
	int iWaveDir = get_tableIndexDirection(rel_waveDir, model.functions.waveFactor.waveDirection); // get_relWaveDirIndex(rel_waveDir, model.functions.weatherFactors);
	int iWaveHeight = get_tableIndex(waveHeight, model.functions.waveFactor.waveHeight); // get_relWaveHeightIndex(waveHeight, model.functions.weatherFactors); // / model.functions.waveHeightDiscr);
	int iWavePeriod = get_tableIndex(wavePeriod, model.functions.waveFactor.wavePeriod); // get_relWavePeriodIndex(wavePeriod, model.functions.weatherFactors); // / model.functions.wavePeriodDiscr);
	int iCalmWaterSpeed = get_tableIndex(calmWaterSpeed, model.functions.waveFactor.shipSpeedCalmWater); // get_calmWaterSpeedIndex(calmWaterSpeed, model.functions.weatherFactors);
	int pos;

	if (printGlobal == 1)
		printf("## wave table index dir %d height %d period %d speed %d speed %.3lf\n", iWaveDir, iWaveHeight, iWavePeriod, iCalmWaterSpeed, calmWaterSpeed);
	pos = iCalmWaterSpeed + model.functions.waveFactor.shipSpeedCalmWater.nIndex *
		(iWaveDir + model.functions.waveFactor.waveDirection.nIndex * (iWavePeriod +
			model.functions.waveFactor.wavePeriod.nIndex * iWaveHeight));
	return model.functions.waveFactor.tableValue[pos]; // wave, wavePeriod, waveDir, calmWaterSpeed

}

double lookup_fuelFactorMainTable(double rel_windSpeed, double rel_windDir, double waveHeight, double rel_waveDir) {
	int iWindDir = get_tableIndexDirection(rel_windDir, model.functions.fuelFactorMain.windDirection);
	int iWindSpeed = get_tableIndex(rel_windSpeed, model.functions.fuelFactorMain.windSpeed);
	int iWaveDir = get_tableIndexDirection(rel_waveDir, model.functions.fuelFactorMain.waveDirection);
	int iWaveHeight = get_tableIndex(waveHeight, model.functions.fuelFactorMain.waveHeight);
	int pos;

	if (printGlobal == 1)
		printf("** fuelFactor table index wind dir %d windspeed %d wave dir %d wave height %d\n", iWindDir, iWindSpeed, iWaveDir, iWaveHeight);
	pos = iWindDir + model.functions.fuelFactorMain.windDirection.nIndex * (iWindSpeed +
		model.functions.fuelFactorMain.windSpeed.nIndex * (iWaveDir +
			model.functions.fuelFactorMain.waveDirection.nIndex * iWaveHeight));
	return model.functions.fuelFactorMain.tableValue[pos];
}


int getHoursSinceMidnight(int t) {
	return (t + model.params.startHoursSinceMidnight) - 24 * (int)((t + model.params.startHoursSinceMidnight) / 24);
}

int delayTimeToStartTimeDay(int t, int arrivalTime) {
	//printf("Eval delayTimeToStartTimeDay from t %d arrivalTime %d\n", t, arrivalTime);
	int hoursSinceMidnight = getHoursSinceMidnight(t);
	//printf("hoursSinceMidnight  %d\n", hoursSinceMidnight);
	if (hoursSinceMidnight <= arrivalTime)
		return t + arrivalTime - hoursSinceMidnight;
	else
		return t + arrivalTime + 24 - hoursSinceMidnight;
}

void 					anropNonsenseFunction(int arcNr) {
	spherical::Point p1, p2;
	if (model.arc[arcNr].toLevel >= 0)
		p2 = model.network.physicalLev[model.arc[arcNr].toLevel].point[model.arc[arcNr].toPointNr];
	else
		p2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point[model.network.channel[-model.arc[arcNr].toLevel - 1].nPoints - 1];
	if (model.arc[arcNr].fromLevel >= 0)
		p1 = model.network.physicalLev[model.arc[arcNr].fromLevel].point[model.arc[arcNr].fromPointNr];
	else {
		if (model.arc[arcNr].toLevel >= 0)
			p1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point[model.network.channel[-model.arc[arcNr].fromLevel - 1].nPoints - 1];
		else
			p1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point[0];
	}
	printf("arcNr %d xy %.3lf %.3lf to %.3lf %.3lf dist %.3lf koll %.3lf tid %.3lf (error if corridor since not straight)\n"
		"levels %d %d points %d %d speedSetting %d fromTime %d\n",
		arcNr, p1.longitude().degrees(), p1.latitude().degrees(),
		p2.longitude().degrees(), p2.latitude().degrees(), model.arc[arcNr].distance,
		p1.distanceTo(p2) / 1000.0, model.arc[arcNr].time, model.arc[arcNr].fromLevel, 
		model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].toPointNr,
		model.arc[arcNr].speedSetting, model.arc[arcNr].fromTime);
}


int saveSP_delay(int iter) { // not used
	FILE* filpek;
	int i, i1, forsta = 1;
	strDelayToEnd* routeToEnd;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/delayArcs.geojson", model.params.resultPath.c_str());
	filpek = fopen(namn, "w");
	initGeoJsonFil(filpek, "delayArcs");

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (model.delayRouteToEnd[i] == NULL)
			continue;
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			routeToEnd = &(model.delayRouteToEnd[i][i1]);

			if (routeToEnd->base_time < -0.5)
				continue;

			if (forsta != 1)
				fprintf(filpek, ", ");
			else
				forsta = 0;
			fprintf(filpek, "  {\"type\":\"Feature\", \"properties\":{\"fromLevel\":%d, \"fromNodPos\":%d, \"nArcs\":%d, \"time\":%.3lf},\n",
				i, i1, routeToEnd->nBVArcs, routeToEnd->base_time + routeToEnd->changed_time);
			fprintf(filpek, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[");
			addArcDelayToGeojson(filpek, routeToEnd);
			fprintf(filpek, "]]}}\n");
		}
	}

	for (i = 0; i < model.network.nChannels; i++) {
		if (model.delayRouteToEnd_channel[i] == NULL)
			continue;
		for (i1 = 0; i1 < 2; i1++) {
			routeToEnd = &(model.delayRouteToEnd_channel[i][i1]);

			if (routeToEnd->base_time < -0.5)
				continue;

			if (forsta != 1)
				fprintf(filpek, ", ");
			else
				forsta = 0;
			fprintf(filpek, "  {\"type\":\"Feature\", \"properties\":{\"fromLevel\":%d, \"fromNodPos\":%d, \"nArcs\":%d, \"time\":%.3lf},\n",
				-i - 1, i1, routeToEnd->nBVArcs, routeToEnd->base_time + routeToEnd->changed_time);
			fprintf(filpek, "    \"geometry\":{\"type\": \"MultiLineString\", \"coordinates\":[[");
			addArcDelayToGeojson(filpek, routeToEnd);
			fprintf(filpek, "]]}}\n");
		}
	}

	fprintf(filpek, "]}\n");
	fclose(filpek);

	return 0;
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

