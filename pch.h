// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

#ifndef PCH_H
#define PCH_H
#include <string>
#include "sphericalpoint.h"
#include"raster.cpp"
#include "sp.h"
#include <chrono>

#define JSON_TRY_USER if(true)
#define JSON_CATCH_USER(exception) if(false)
#define JSON_THROW_USER(exception)                           \
    {std::clog << "Error in " << __FILE__ << ":" << __LINE__ \
               << " (function " << __FUNCTION__ << ") - "    \
               << (exception).what() << std::endl;           \
	  printf("ERROR in JSON\n"); \
     std::abort();}

#include "json.hpp"
//#include "redisDef.h"

#include <string>

using namespace erkir;
using json = nlohmann::json;
//using namespace std;


// TODO: add headers that you want to pre-compile here

#define CHAR_ALLOC 257
struct dataStr
{
	int length;
	//	char data[257];
	char data[CHAR_ALLOC];
};

struct strFuelType
{
	double price;
	double emissionFactor;
};
struct strFuel
{
	//double base;
	strFuelType aux_noEca;
	strFuelType aux_eca;
	strFuelType main_noEca;
	strFuelType main_eca;
};

struct strSafety
{
	double base;
	double hurricane;
	double bowSlam;
	double greenWater;
	double dynamicStability;
	double feasibleSafety;
	double iceCoverCost_fix;
	double iceCoverCost_thickness;
	//double lowPressure;
	//double waves;
	//double stability;
};

struct strPenalties
{
	double storm_costInsideInner;
	double storm_costInsideOuter_kvot;
};

struct strTimeZones
{
	char* name;
	int nHoursDiff;
	int nMinutesDiff;
};

struct strExtraWeights {
	double weightEmission;
	double weightTime;
	double weightFuel;
	double weightSafetyBase;
	double weightDistance;
	double eta_cost_early;
	char* identifierOpt;
};

struct strVisuell {
	FILE* filVisuell;
	struct tm tmBas;
	char* startTime;
	int pos;
	double oldX;
	double oldY;
};

struct strParams
{
	double delayEjPrefPathArcFactor;
	int delay_onlySolveSP;
	int simuleraTidVisuellt;
	int simulateTimeVisually_nIntHour;

	double tIndexGerH; // omvandling fran tIndex till timmar
	int nTidsperioder_perH; // omvandling fran timmar till tIndex

	double preferredSpeed_calmWater;
	double calmWaterSpeedMin;
	double calmWaterSpeedMax;

	double historicDataFactor_waveHeight;
	double historicDataFactor_windSpeed;
	double historicDataFactor_current;

	double maxDistStartToCorridorConnect;
	//double* preferredPathUseChannelSpeed;
	//int* preferredPathUseChannelConsumption;

	//int nTimeZones;
	//strTimeZones* timeZone;
	std::string indataPath;
	std::string indataPathName;
	std::string resultPath;

	double knots_to_km;
	double shipSpeed_average; // km/h = 20 knots, 1 knot = 1.852 km/h
	//std::string mapPhysicalFileName;
	std::string mapPhysicalBFileName;
	std::string mapPhysicalAFileName;
	//int physicalMapRasterPos;
	std::string mapFuelGeographyAFileName;
	std::string mapFuelGeographyBFileName;
	std::string mapTimeDelayName;
	int nTimeDelayAngles;

	//std::string mapFuelGeographyFileName;
	std::string preferredPath;
	std::string corridorPath;
	std::string channelsName;

	double nHours_changeCourseInterval;
	double ortoDist_nPointsPerHour;
	int nPkterOrto;
	int* preferredPathOrtoPos;
	int* preferredPathStraightLineFeasibleFrom;
	int preferredPath_followExactOK;
	std::string readSolPathFile;
	std::string solutionFileName;

	double epsilon;
	int save_weatherNodes;
	double commercialSpeed;
	double commercialFuel;
	double commercialAllowedVariation;
	double report_minBearingDiff;
	double report_minBearingDiffWpt;

	//double *ship_speedSettings;
	char **ship_speedSettingID;
	// int *ship_speedSettingNr;
	int maxDiffTimeFastSlow; // max time difference between fastest and slowest route
	int maxDiffTimeFastSlow_fas3;
	
	int max_changeDirection;
	int maxDiff_pointNrFas3;
	int nMaxLev_posToDelayedPrefPath;
	int longestRouteDays_history;
	//double lengthIntervall; // length of a time intervall in hours
	//double dist_checkOKroute; // nKm between checks if the route is on land or water, no need to check more often than the pixel size of the map
	
	//std::string variableFileName;

	double weightTime;
	double priceTime;
	strFuel fuel;
	double weightFuel;
	double weightEmission;
	double scaleObjEmission;
	strSafety weightSafety;
	strPenalties penalties;

	int useStandardWeather; // -1 for standard 0, 1 for standard last, 0 for changing forecast

	double basDistArcs;
	double physicalMap_noDataValue;
	//int speedSettings_addOnlyCheapestArcs;

	int runAlt;
	int startDelay_h;
	std::string fromHarbour;
	std::string toHarbour;
	std::string type;

	
	//double storm_windUBD; // storm level 64 kt
	//double storm_1dist_ahead; // first ring ahead, 200 nautiska miles
	//double storm_2dist_ahead; // second ring ahead, 500 nautiska miles
	//double storm_1costInside_ahead; // 1e10
	//double storm_1cost_ahead; // 100
	//double storm_2cost_ahead; // 1

	//double storm_1dist_behind; // first ring ahead, 200 nautiska miles
	//double storm_2dist_behind; // second ring, 120 nautiska miles
	//double storm_1costInside_behind; // 1e10
	//double storm_1cost_behind; // 100
	//double storm_2cost_behind; // 1
	int startYear; // = 2018;
	int startMonth_nr; // = 9; // sep
	int startDay_nr; //  = 1;
	time_t UTC_secondsStart;
	int startHour; // 0
	int startMinute; // 0
	int startHoursSinceMidnight;
	double eta_h;
	double eta_cost_early;
	double eta_cost_late;

	double maxDeviationPreferred_km;

	double shipDraft;
	double shipLength;
	double freeBoard2;

	double minSpeedDiffWeatherFactor;
	double maxSpeedDiffWeatherFactor;
	double minSpeedDiffCurrent;
	double maxSpeedDiffCurrent;

	int errorCode;

	strExtraWeights* extraOptWeights;
};

struct strVariables
{
	int weatherNr;
	int elementPos;
	char *nameID;
};

struct strPath
{
	int nPoints;
	spherical::Point* point;
	double* point_y;
	double* point_x;
	double minX;
	double maxX;
	double startX;
	double totDist;
};

struct strOptPathLevel
{
	double timeArrive;
	int pointNr;
	int speedSettingNr;
	int levelNext;
};

struct strOptPathChannel
{
	double timeArriveNext;
	int speedSettingNrNext;
	int levelNext;
	double timeArriveThrough;
	int speedSettingNrThrough;
};
struct strOptPath
{
	strOptPathLevel* level;
	strOptPathChannel* channel;
};

struct strCorrLines
{
	int nLines;
	int *nPoints;
	spherical::Point** point;
};

struct strArcInfo
{
	int fromPointNr;
	int fromLevel;
	int toLevel;
	int toPointNr;
	int fromTime;
	int toTime;
	int speedSetting;

	//int nodNr1;
	//int nodNr2;
	//int nodNr1_utNodPos;
	//double safetyBowSlam;//
	//double safetyGreenWater;//
	//double safetyDynStability;//
	//int feasibleSafety;
	//double iceCoverCost;//
	//double channelCost;//

	////double safetyStability;
	////double safetyPressure;
	double distance;
	double time;
	double fuelBase;
	double emission;//
	double fuel_aux;//
	double fuel_auxEca;//
	double fuel_noEca;//
	double fuel_eca;//
	double fuelQualityKvot;
	double safetyBase;
	double safetyHurricane;
	double totCost;
};

struct strChannel {
	double extraCostChannel;
	double timeThroughChannel; // if -1 then optimized
	double waitingTime;
	double distance_km;
	double totalConsumption;
	double arrivalTime_h;
	int intArrivalTime_h;
	int intWaitingTime;
	char* ID;

	int earliestStartLevel;
	int latestEndLevel;
	int bastStartLevel;
	int bastEndLevel;
	double* point_y;
	double* point_x;
	strBoundBox boundingBox;

	double factorDelayedPrefPathDuring;
	double factorDelayedPrefPathAfter;

	int nPoints;
	spherical::Point* point;
	//int* allowedPoint;
	int nOutNodes;
	int* outNode;
	//int* outPolyPoint;
	int* outLevel;
	int nArcsToPoint;
	int* nAllocTimeIntervals;
	int* nTimeIntervals;
	int** timeInterval;
	int** nodNr_from_pt;
	double* distanceFromStart;

	//double* polygon_x[2];
	//double* polygon_y[2];
	//strBoundBox polygon_boundingBox[2];
	//double polygon_dist[2];
	//int nPolygonPoints[2];
	//spherical::Point* polygonUse_point[2];
	//double* polygonUse_x[2];
	//double* polygonUse_y[2];
	//int nPolygonUsePoints[2];

};

struct strNodeSeq
{
	int nPoints;
	spherical::Point *point;
	double* point_x;
	double* point_y;
	int *nOutNodes;
	int* nInNodes;
	int** outNode;
	int** outLevel;

	double* minDistPrevNode;
	int* minDistPrevNode_level;
	int* minDistPrevNode_pos;
	//int* nodeConnectedFromChannel;
	int requirePrefPathFeasible;
	double factorDelayedPrefPath;

	//int *nAllocOutArcs;
	//int *nOutArcs;
	//strArcInfo **outArc;
	int *nTimeIntervals;
	int *nAllocTimeIntervals;
	int **timeInterval;
	int **nodNr_from_pt;
	int* nArcsToPoint;

	int* usedPoint;
	int* allowedPoint;
	double distanceFromStartPosMid;
	double distTot;
	spherical::Point* preferredPathPoint;
	int npreferredPathPoints;
	double midTimeArrive;
	int restrictedLevel;
};

struct strNetwork
{
	int nPhysicalLevels;
	strNodeSeq *physicalLev;
	//int useLongitudeKvadrant[4];
	int nMaxNodesInPath;
	int nChannels;
	strChannel* channel; 
	int nPhysicalNodes;
	int nPhysicalArcs;

	//int nUsedChannels;
	//int* usedChannel;

	int nAllocCoords;
	double* xCoord;
	double* yCoord;
	int nCoords;
	double last_x;
	double* startKvot;
	double* endKvot;
	int* posSplitCoord;
	int nMaxSplits;

	//int* arcGen_utilizeStaticWeather_ForOutNodePos;
	//int** arcGen_staticWeatherArcNr_outNodePosSpeed;
	int tidp_startHistoricDataOnly;
};

struct strPhysicalMap
{
public:
	std::vector<Raster> raster;
	double min_latitude;
	double min_longitude;

};

struct strCheckPkt
{
	//double uVesselDirection;
	//double vVesselDirection;
	double distToNextPkt;
	int *latPos;
	int *lonPos;
	int* pos_latLon;
	//int *fileNr;
};

struct strFunc
{
	int nCheckPoints;
	int nAllocPoints;
	strCheckPkt *checkPoint;
	double checkPoints_totDist;
	// int modifiedPoints;
	//spherical::Point *point;
	double* point_lat;
	double* point_lon;
	double *vesselBearing;

	int nFunctions;
	double *funcVal;
	double ***param;

};

struct strCalmWaterFkn {
	double c0;
	double c1_rpm;
	double c2_rpm;
};

struct strFuelConsumptionFkn {
	double c0;
	double c1_rpm;
	double c2_rpm;
	double c3_rpm;
};

struct strTableParam {
	double minValue;
	double maxValue;
	double intervalSize;
	double inv_intervalSize;
	int nIndex;
};

struct strTableTyp {
	//std::string tableID;
	//std::string fileName;
	char* tableID;
	char* fileName;
	double maxWaveHeight;
	strTableParam shipSpeedCalmWater;
	strTableParam windSpeed;
	strTableParam windDirection;
	strTableParam waveHeight;
	strTableParam wavePeriod;
	strTableParam waveDirection;
	strTableParam shipSpeedOverGround;
};


struct strFunkData {
	strTableParam shipSpeedCalmWater;
	strTableParam windSpeed;
	strTableParam windDirection;
	strTableParam waveHeight;
	strTableParam wavePeriod;
	strTableParam waveDirection;
	strTableParam shipSpeedOverGround;
	float* tableValue;
	//double* tableValueWind;
	//double* tableValueWave;

	/*
	int nCalmWaterSpeedIndex;
	double calmWaterSpeedIndexSize;
	double inv_calmWaterSpeedIndexSize;
	double calmWaterSpeed_min;
	double calmWaterSpeed_max;

	int nWaveHeightIndex;
	double waveHeightIndexSize;
	double inv_waveHeightIndexSize;
	double waveHeight_min;
	double waveHeight_max;
	int nWavePeriodIndex;
	double wavePeriodIndexSize;
	double inv_wavePeriodIndexSize;
	double wavePeriod_min;
	double wavePeriod_max;
	int nWaveDirIndex;
	double waveDirIndexSize;
	double inv_waveDirIndexSize;
	double waveDir_min;
	double waveDir_max;

	int nWindSpeedIndex;
	double windSpeedIndexSize;
	double inv_windSpeedIndexSize;
	double windSpeed_min;
	double windSpeed_max;
	int nWindDirIndex;
	double windDirIndexSize;
	double inv_windDirIndexSize;
	double windDir_min;
	double windDir_max;

	int nShipSpeedIndex;
	double shipSpeedIndexSize;
	double inv_shipSpeedIndexSize;
	double shipSpeed_min;
	double shipSpeed_max;
	*/

};

struct strValuesNow {
	double fuel_main;
	double fuel_aux;
	double totFuel_aux;
	double totFuel_main;
	double distance;

	double bowSlam;
	double greenWater;
	double dynamicStability;
	double worstStormValue;
	int feasibleSafety;
	double iceCoverCost;
	double channelCost;

	double current;
	double windSpeed;
	double relWindDir;
	double waveHeight;
	double maxWaveHeight;
	int maxWaveHeight_tp;
	double wavePeriod;
	double relWaveDir;
	double forecastType;
	
	double currentReal;
	double currentDirReal;
	double windReal;
	double windDirReal;
	double waveDirReal;

	double windDirReal_lastKnown;
	double currentReal_lastKnown;
	double waveDirReal_lastKnown;


	double iceCover_max;
	double bowSlamming_max;
	double greenWater_max;
	double dynamicStability_max;

	double speedDiffWind;
	double speedDiffWave;
	double baseGroundSpeed;

	double accumDistance;
	double totDistance;
	int Wpt;
	double bearingOldWpt;
	double calmWaterSpeed;

	double maxDiffTime;
	double minDiffTime;
	int maxDiffTime_level;
	int minDiffTime_level;

	double WindF;
	double WaveF;
	double CurrentF;
	double DelayF;
	double totWindF;
	double totWaveF;
	double totCurrentF;
	double totDelayF;

	int prefPathArc;
};

struct strTables {
	int nBasAlloc;
	int nAllocTableTyp[3];
	int nTableTyp[3];
	strTableTyp* tableTyp[3]; // 0 wind, 1 wave, 2 stability
};

struct strSpeed {
	int nShip_speedSettings;
	double* rpmSetting_gerCalmWaterSpeed;
	double* rpmSetting_gerFuelConsumption_main;
	double* rpmSetting_gerFuelConsumption_aux;
	double* rpm;

	int* settingGerBaseSetting;
};

struct strFunc2 {
	int nShip_speedSettingsBase;
	double* rpmSetting_gerCalmWaterSpeedBase;
	double* rpmSetting_gerFuelConsumption_mainBase;
	double* rpmSetting_gerFuelConsumption_auxBase;
	int speedSetting95MCR_base;
	int speedSetting95MCR_use;

	strSpeed* speedLevel;
	strSpeed* speedChannelOut;
	strSpeed* speedChannel;
	int nAllocShipSpeedsLevel;

	double* varValue;
	double* varValueAverage;

	std::string windTableID;
	std::string waveTableID;
	std::string stabilityTableID;
	int windTableNr;
	int  waveTableNr;
	int stabilityTableNr;
	double maxWaveHeight;
	// std::string bowSlammingTableID;
	// std::string greenWaterTableID;

	//int nWindDir;
	//int nWaveDir;
	//int table_niWaveDir;
	//int table_niWave;
	//int table_niWavePeriod;
	//int table_niWindDir;
	//int table_niWindSpeed;
	//double* table_speedDiff;

	double* rpmBase;
	//strCalmWaterFkn calmWaterSpeed;
	//strFuelConsumptionFkn fuelConsumption;

	//double windSpeed_max;
	//double* windSpeed_minVal_array;
	//double* windSpeed_maxVal_array;

	//double waveHeight_max;
	//double* waveHeight_minVal_array;
	//double* waveHeight_maxVal_array;

	//double wavePeriod_max;
	//double* wavePeriod_minVal_array;
	//double* wavePeriod_maxVal_array;

	//double windMagnitude_discreteSize_kts;
	//double rel_windSpeed_kvotIndex; // 2
	//double max_windSpeed;
	//int nWindSpeedSkalad; // omskalad med kvotIndex
	//int* rel_windSpeedSkalad_ger_index;

	//double waveHeight_discreteSize_m;
	//double rel_waveHeight_kvotIndex; // 2
	//double max_waveHeight;
	//int nWaveHeightSkalad; // omskalad med kvotIndex
	//int* rel_waveHeightSkalad_ger_index;

	//double rel_wavePeriod_kvotIndex; // 2
	//double max_wavePeriod;
	//int max_wavePeriodSkalad; // omskalad med kvotIndex
	//int* rel_wavePeriod_ger_index;

	int pos_wind_u;
	int pos_wind_v;
	int pos_current_u;
	int pos_current_v;
	int pos_waveHeight;
	int pos_wavePeriod;
	int pos_waveDirection;
	int pos_iceThickness;

	// weather factors
	strFunkData windFactor;
	strFunkData waveFactor;

	// safety
	double iceCoverMaxFree;
	//double iceCoverCost_fix;
	//strFunkData bowSlamming; // height + nHeight * period
	//strFunkData greenWater; // height
	strFunkData dynStability; // wSpeed + nWSpeed * wDir

	strValuesNow valuesNow;
};

struct strDijkstra {
	long long minArcLen;
	long long maxArcLen;
	int nNoder;
	int nArcs;
	SP *sp;
	Node *nodes;
	Node *source;
	Node *sink;
	Arc2 *arcs;
	ulong cLevels;
	int logDelta;
	bool doBFS;
	long node_min;
	long long OptCost;
	double FAKTOR_NATVERK;
};

struct strNoder
{
	int nUtNoder;
	int nAllocUtNoder;
	int *UtNod;
	double *UtNodCost;
	int *outArcNr;
	int physicalLevel;
	int pointNr;
	int timeInterval;

};

struct strStormQuadr
{
	double NE;
	double SE;
	double SW;
	double NW;
	double windMaxRadius;
};

struct strStormFeature
{
	double datum;
	double maxWind;
	double lat;
	double lon;
	spherical::Point midPoint;
	double bearing;
	double distanceToNextPoint;
	double outerCircleSize;
	double innerCircleForwardSize;
	double innerCircleBackwardsSize;
	long long UTCseconds;

	double tidFromStart_h;
	double hoursToNextPoint;
	//int nQuadrants;
	//strStormQuadr* quadrant;
};

struct strStorm
{
	char* fileName;
	int nAllocFeatures;
	int nFeatures;
	double timeIntervall_h;
	double inv_timeIntervall_h;
	//int tidsIntervall;
	strStormFeature* feature;
	int nTimeIntervals_maxValue;
	int* timeIntervalIndex;

	//std::string stormID;
	int stormNr;
	char* stormName;
	double closestPointToRoute;

	double box_minLat;
	double box_maxLat;
	double box_minLon;
	double box_maxLon;
};

//struct strRasterData {
//	float** fuelGeography;
//	//float** physicalMap;
//};

struct strStatus {
	int weatherHistoryOpenFile_fail;
};

struct strDBTableInfo {
	double epochCount;
	char* tableID;
	char* textFileName;
};

struct strSQLiteTables {
	int nAlloc;
	int nTables;
	strDBTableInfo* table;
};

struct strSQLiteMap {
	int type;
	double epochCount;
	char* textFileName;
	int nCols;
	int nRows;
	double size_col;
	double size_row;
	double minX;
	double maxX;
	double minY;
	double maxY;
	int nBlockRows;
	int nBlockCols;
};

struct strPointxy {
	double point_y1;
	double point_x1;
	double point_y2;
	double point_x2;
	int direction;
	int arcNr;
};

struct strDelaySP {
	double time;
	double costs;
	double distance;
	int nSlowSpeed;
	int nHighSpeed;
};

struct strDelay {
	//strPointxy* baseNode;
	//strPointxy** neighbour;
	//int nNodes;
	//int* nNodeNeighbours;
	int nGroups;
	strPointxy** group;
	int* nGroupArcs;

	int nYears;
	int* year;
	strDelaySP SPsol;
	int nDiffTimeSol;
	char* delayed_stormFileName;
	int delayed_monthNr;

	int* nStormsYear;
	strStorm** stormsYear;
};


struct strModel
{
	strVisuell timeVisual;
	strDelay delay;
	strOptPath optPath;

	strSQLiteTables* sqliteTables;
	strSQLiteMap* sqliteMap;

	double tmpEpochCount;

	double weather_timeIntervall_h;
	double weather_inv_timeIntervall_h;
	int weather_nTimeIntervals_maxValue;

	strTables tables;
	strStatus status;
	strBoundBox boundingBox;
	// strRasterData rasterData;
	strParams params;
	// int nVariables;
	//strVariables *variable;
	int nWeatherFiles;
	double inv_nWeatherFiles;
	strWeather *weather;
	strWeather delayedGrid;

	int nStorms;
	strStorm* storms;
	strPath preferredPath;
	strPath solutionPath;
	strCorrLines corridorPath;
	strNetwork network;
	//Raster physicalMapRaster;
	Raster::strPhysRaster physicalMapA;
	Raster::strPhysRaster physicalMapB;
	//Raster rasterPhysicalMapA;
	//Raster rasterPhysicalMapB;

	Raster::strPhysRaster fuelMapA;
	Raster::strPhysRaster fuelMapB;
	//Raster rasterFuelMapA;
	//Raster rasterFuelMapB;

	//Raster* fuelGeographyMapRaster;
	strFunc weatherFunctions;
	strFunc2 functions;
	int nNoder;
	int nAllocNoder;
	int nArcs;
	int nAllocArcs;
	strArcInfo *arc;
	strNoder *Noder;
	strDijkstra Dijkstra;

	int nBVArcs;
	int *BVArc;
	int *BVtempNodOrder;

#ifdef WIN32
	std::chrono::steady_clock::time_point timeStart;
	std::chrono::steady_clock::time_point tmpTid[2];
	std::chrono::steady_clock::time_point tmpTid2[10];
	std::chrono::steady_clock::time_point tmpTid3[2];
	std::chrono::steady_clock::time_point tmpTid4[2];
	std::chrono::steady_clock::time_point tmpTid5[2];
#else
	std::chrono::system_clock::time_point timeStart;
	std::chrono::system_clock::time_point tmpTid[2];
	std::chrono::system_clock::time_point tmpTid2[10];
	std::chrono::system_clock::time_point tmpTid3[2];
	std::chrono::system_clock::time_point tmpTid4[2];
	std::chrono::system_clock::time_point tmpTid5[2];
#endif
	std::chrono::duration<double, std::milli> durationMilliTot;
	std::chrono::duration<double, std::milli> *durationMilli;
	int* nCallsWeatherBand;

	std::chrono::duration<double, std::milli> durationCheckAddBagar;
	std::chrono::duration<double, std::milli> durationSetupCheckPoints;
	std::chrono::duration<double, std::milli> durationSetArcValues;
	std::chrono::duration<double, std::milli> durationAdderaArc;
	std::chrono::duration<double, std::milli> durationGenCalc;
	std::chrono::duration<double, std::milli> durationCalcArcTimeCost;

	std::chrono::duration<double, std::milli> durationCalcArcTimeCalmWater;
	std::chrono::duration<double, std::milli> durationCalcArcTimeStorm;
	std::chrono::duration<double, std::milli> durationCalcArcTimeCurrent;
	std::chrono::duration<double, std::milli> durationCalcArcTimeBaseGroundSpeed;
	std::chrono::duration<double, std::milli> durationCalcArcTimeWind;
	std::chrono::duration<double, std::milli> durationCalcArcTimeWave;
	std::chrono::duration<double, std::milli> durationCalcArcTimeFuel;
	std::chrono::duration<double, std::milli> durationCalcArcTimeIceSafety;

	std::chrono::duration<double, std::milli> durationStormDestPoint;
	std::chrono::duration<double, std::milli> durationStormBearingTo;
	std::chrono::duration<double, std::milli> durationCalcArcTimeRelWindSpeed;

	std::chrono::duration<double, std::milli> duration1;
	std::chrono::duration<double, std::milli> duration2;
	std::chrono::duration<double, std::milli> duration3;
	std::chrono::duration<double, std::milli> duration4;
};


int errlog(const char* format, ...);
int errlog0(const char* format, ...);
int reset_errlog();
char *str_alloc_cpy(const char *data);
char* str_alloc_cpyString(std::string data);
int write_copyAtoB(char *filnamnUt, char *filExt, char *filenamnIn, char *mode);

int SattUppDijkstraNatverk3(strModel *model);
int ChangeArcCosts3(strModel* model);
int AnropDijkstra2(int NodA, int NodB, strModel *model, bool *Reached);
double NystaUppBV_MassTest(strModel *model, int Reached, int NodA0, int NodB0, long long *Cost);
int try_addBage_fromPath(int thisLevel, int nextLevel, int pos1, int pos2, int speedSetting, int tPos); // , float** fuelRaster);

double char_to_double(char* object);
double char_to_doubleConst(const char* object);
int char_to_int(char* object);
int get_data_objects_till_EOL_semkol(char objects[][CHAR_ALLOC], dataStr* data, FILE* FilPek);
int check_isChannelNodePosAllowed(int nr, int pos);
int calcWeatherPosAlongArc(spherical::Point p1, spherical::Point p2, int tidp = 0);
double getStormValue(int t, double lat, double lon, int saveStormData = 0);
double getVariableValue(int varNr, int checkPointNr, double tidpkt);

int test2(int a);
int testing(int a);

int voyageOpt(std::string inputName, std::string resultName);
int generateDelayedFactors(std::string inputName, int node);
int exitKontrollerat(int codeLine, int callType = 1);
int writeSolutionToJson(std::string filename, int resAlt, char* namnSol);
std::string splitFilename(std::string namn, int alt = 0);
int fixReadableDate(struct tm tmBas, char* namn);
int initGeoJsonFil(FILE* filpek, const char* namn);
void get_fuelUseKvotECA(double lat1, double lon1, double lat2, double lon2, int mapAlt, double* distECA, double* distOther);

int redisSetKeys(std::string inputPath);
void putStringIntoArrayFloat(std::string strang, float* arrFloat, FILE* filtmp);

double eval_baseGroundSpeed(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed);
double lookup_speedDiffWindWaveTable(double rel_windSpeed, double rel_windDir, double waveHeight, double wavePeriod, double rel_waveDir);
//double eval_fuelConsumption_main(int speedNr);
//double eval_fuelConsumption_aux(int speedNr);
double eval_fuelConsumption_both(int speedNr, double* consumptionAux, int fromLevel, int toLevel);

double eval_relWindSpeed(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir);
void eval_safety(double windspeed, double windDirection, double waveHeight,	double wavePeriod, double iceCover);
int calcWeatherPosAlongpreferredPathArc(spherical::Point p1, int level);
int calcWeatherPosAlongChannel(int cNr);
double eval_calmWaterSpeed(int speedNr, int fromLevel, int toLevel);
double lookup_speedDiffWaveTable(double calmWaterSpeed, double waveHeight, double wavePeriod, double rel_waveDir);
double lookup_speedDiffWindTable(double calmWaterSpeed, double rel_windSpeed, double rel_windDir);

int get_tableIndex(double value, strTableParam param, int alt = 0);
int get_tableIndexDirection(double value, strTableParam param, int alt = 0);
int eval_coordWithinBoundingBox(double lon, double lat);
int check_isCoordFeasiblePhysicalMap(double y, double x);
void setupBoundingBoxFromMapCoords(Raster map);
void getAllVariableValues(int checkPointNr, double tidpkt);
int delayTimeToStartTimeDay(int t, int arrivalTime);
double eval_relWindSpeedExact(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir);
double eval_baseGroundSpeedExact(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed);
int makeSure_feasibleCoordFranLinje(double* y1, double* x1, double* y2, double* x2, int pos);
int roundUp(double varde);
int check_nodeIsWithinPhysicalMapRaster(double lat1, double lon1);
int setUpUsablePointsInPolygonChannel(int cNr, int pos);
int identify_startEndOnChannel(int cNr, int startEnd);
int checkCoordInBoundingBox(double y, double x, strBoundBox bbox);
void updateBoundingBoxWithCoord(strBoundBox* bbox, double y, double x);
void initBoundingBox(strBoundBox* bbox);
void setupUsableSpeedSettings();
int check_translate_xCoord(double* xCoord);
int fixReportDate(struct tm tmBas, char* namn);
void postRequest(std::string errorMessage);
int updateSQLiteAllTablesInfo(int type, int tablePos, int modified);
int saveTablesToSQLite(std::string inputPath);

int testSaveMapToBinaryFile();
//int saveMapsToBinary();
unsigned short* openBinaryMap(int ii, Raster::strPhysRaster* physRaster, strBoundBox boundingBox);

int checkMinnesAnvandning(int rad);

int loadParams_theRestOld(strParams* params);
int loadParams_new(strParams* params);
int loadAllNeededTablesFromSQLite();
int loadVariables(int alt = 0);
int createPhysicalNetwork(int sparaKorridorEnbart, int alt);
int adderaNod(int physicalLevel, int pointNr, int timeInterval);
int check_useRaster_longitude(int weatherNr, int filNr);
int addEndBage(int thisLevel, int pos1, int nextLevel, int i3, int nodNr2);
double estimateLargeCircleDistance_km(double lat1, double lon1, double lat0, double lon0);
int adderaArc(int nodNr1, int nodNr2, double cost);
int addBagar_AB_speedSTid(int thisLevel, int pos1, int nextLevel, int pos2, int* setupCheckPoints, int min_t, int max_t, double fuelQualityKvot);
int addPositionDataToReport(FILE* filpekG, int posReport, int arcNr, int startSlutArc, double* timeExact, std::string solName);
double getCorrect_longitude(double x);
void fixPositionString_latLon(double y, double x, char* namn);
int set_speedSettingsFromBase(strSpeed* speedSetting, int i, int iUse, int iOver = -1, double kvot = 0);
long long getSecondsFromUTC(const char* time);

int testCallWeatherFile();
int roundDown(double varde);

void* malloc2(size_t size);
void* calloc2(size_t count, size_t size);
int loadStormObject(json dataFeature);
int eval_stormWithinBoundingBox(int stormNr);
void sortStormFeaturesTime(int pos);
void addInfoToStorms(int pos);
void calc_stormsNearby_delay();
int plotNodeTimeVisuellt(double time, double x, double y);
int simuleraStormsVisuellt();
double fix_lonPos(double lon);
int getMonthToUseForDelay(double dist);
double get_fuelQualityKvot(int thisLevel, int pos1, int nextLevel, int pos2);






#endif //PCH_H

