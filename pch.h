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
	double quantity;
};
struct strFuel
{
	//double base;
	strFuelType aux_noEca;
	strFuelType aux_eca;
	strFuelType main_noEca;
	strFuelType main_eca;
	strFuelType extra_fuel;
};

struct strSafety
{
	double base;
	double hurricane;
	double bowSlam;
	double greenWater;
	double dynamicStability;
	double rolling;
	double surfRiding;

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

struct strLegWeights {
	int level1;
	int level2;
	double weightTime;
	double weightFuel;
	double weightEmission;
	strSafety weightSafety;
};

struct strLegProp {
	int prefPath_startPoint;
	int prefPath_endPoint;
	int path_fixed; // 1 if the leg has to be followed exactly
	int path_pos;
	double endNode_waitingTime;
	double endNode_fuelConsumption_main_mpd;
	double endNode_fuelConsumption_aux_mpd;
	int endNode_exact; // 1 if the end node of the leg must be exactly visited

	int physLevelFirst;
	int physLevelLast;
	double basDistArcs;
};

struct strVisuell {
	FILE* filVisuell;
	struct tm tmBas;
	char* startTime;
	int pos;
	double oldX;
	double oldY;
};

struct strLegCommercial {
	double commercialSpeed;
	double commercialFuel;
	double commercialAllowedVariation;

};

struct strParams
{
	std::string url_errorEmail_api;
	std::string url_getCorridors;

	double tss_attractionDistance_km;
	int useSimulering;
	double simulationSpeed_kmh;
	double userSimulation_maxWaveHeight;
	double userSimulation_maxWindSpeed_kmh;
	double userLimit_maxWaveHeight;
	double userLimit_maxWindSpeed_kmh;
	double maxWaveHeight_warning;
	double maxWindSpeed_warning;

	time_t testTime;
	struct tm tmBas;
	char* startTime;
	char* startTime_short;
	char* startTime_full;

	double wayPointHours;

	char* loadWeightsFile;
	int weightID;

	char* weatherDirectory;
	double maxDistBetweenPrefPathPoints;

	int etaFocus_speed;
	int nSpeedSettingDivideIter1;
	int nTidsperioder_perH_iter1;
	int tidp_startHistoricDataOnly_iter1;

	int hindCast;
	int nHindCastMonths;
	int* hindCast_month;
	int* hindCast_year;
	int* hindCast_yearSecDiff;

	int onboard;
	int onboard_currentStatic;
	int* weather_is_current;

	int includeNazanin_safety;
	int ignore_windWave;
	int ignore_current;

	int checkGribFilesSpecial;

	double delayEjPrefPathArcFactor;
	int delay_onlySolveSP;
	int simuleraTidVisuellt;
	int simulateTimeVisually_nIntHour;
	int eta_naraMaxSpeed;

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
	std::string resultName;
	std::string weatherPath;
	std::string tssName;


	double knots_to_km;
	double shipSpeed_average; // km/h = 20 knots, 1 knot = 1.852 km/h
	//std::string mapPhysicalFileName;
	std::string mapPhysicalBFileName;
	std::string mapPhysicalAFileName;
	std::string mapLandSeaBFileName;
	std::string mapLandSeaAFileName;
	//int physicalMapRasterPos;
	//std::string mapFuelGeographyAFileName;
	//std::string mapFuelGeographyBFileName;
	std::string mapTimeDelayName;
	std::string map_currentDelayName[2];
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
	strLegCommercial* legCommercial;
	double report_minBearingDiff;
	double report_minBearingDiffWpt;

	//double *ship_speedSettings;
	char **ship_speedSettingID;
	// int *ship_speedSettingNr;
	int maxDiffTimeFastSlow; // max time difference between fastest and slowest route
	int maxDiffTimeFastSlow_fas3;
	
	int max_changeDirection_base;
	int max_changeDirection_factorStartEnd_base;
	int max_changeDirection;
	int max_changeDirection_factorStartEnd;
	int varyStartEndArcLength;
	int maxDiff_pointNrFas3;
	int nMaxLev_posToDelayedPrefPath;
	int longestRouteDays_history;
	//double lengthIntervall; // length of a time intervall in hours
	//double dist_checkOKroute; // nKm between checks if the route is on land or water, no need to check more often than the pixel size of the map
	
	//std::string variableFileName;

	// double weightTime;
	double priceTime;
	strFuel fuel;

	//double weightFuel;
	//double weightEmission;
	//strSafety weightSafety;
	strLegWeights* legWeights;
	strLegProp* legProperties;
	int nLegs;

	double scaleObjEmission;
	strPenalties penalties;

	double penOverWeatherLimit_fix;
	double penOverMaxWaveHeightLimit_m;
	double penOverMaxWindSpeedLimit_kmh;


	int useStandardWeather; // -1 for standard 0, 1 for standard last, 0 for changing forecast

	double basDistArcs;
	//double physicalMap_noDataValue;
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

	double aim_waypointInterval_h;

	double calmWaterSpeedCompare;
	double calmWaterSpeedCompareUse;
	double fuelCompare;

	double maxDeviationPreferred_km;
	double maxDeviationPreferred2_km;

	double shipDraft;
	double shipLength;
	double freeBoard2;

	double minSpeedDiffWeatherFactor;
	double maxSpeedDiffWeatherFactor;
	double minSpeedDiffCurrent;
	double maxSpeedDiffCurrent;

	int errorCode;
	int failedTime;

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
	double* distToPrevPoint;
};

struct strOptPathLevel
{
	double timeArrive;
	int pointNr;
	int baseSpeedSettingNr;
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
	int outNodePos;
	int toLevel;
	int toPointNr;
	int fromTime;
	int toTime;
	int speedSetting;
	double* extraAreaFactor;
	int corridorNr;

	int nodNr1;
	//int nodNr2;
	int nodNr1_utNodPos;
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
	double speedDiffCurrent;
	double fuelQualityKvot;
	double extraAreaCostKvot;
	double kvotCost;
	double safetyBase;
	double safetyHurricane;
	double totCost;

	double maxWindSpeed;
	double maxWaveHeight;

	double bowSlam;
	double greenWater;
	double dynamicStability;

	double rolling;
	double surfRiding;

};

struct strChannel {
	int include_tssTmp;
	int straightArcFeasible_toChannelFromPrefPath;
	int preferredPathPoint_posConnectTo;
	int straightArcFeasible_fromChannelToPrefPath;
	int preferredPathPoint_posConnectFrom;

	double extraCostChannel;
	double timeThroughChannel; // if -1 then optimized
	double waitingTime;
	double distance_km;
	double totalConsumption;
	double arrivalTime_h;
	int intArrivalTime_h;
	//int intWaitingTime;
	char* ID;
	int type; // 0 - normal corridor, 1 - tss
	double kvotCost; // used to give discount on tss paths
	double kvotMinCost;
	double minCost;
	int keepPrefPath_tss;

	int ECA_type;
	int followExactly;
	double waiting_consumption_main;
	double waiting_consumption_aux;

	int earliestStartLevel;
	int latestEndLevel;
	int bastStartLevel;
	int bastEndLevel;
	int StartLevelOnlyPrefPath;
	int EndLevelOnlyPrefPath;

	int bastStartPointPos;
	int bastEndPointPos;
	double bastStartDist;
	double bastEndDist;

	double* point_y;
	double* point_x;
	strBoundBox boundingBox;

	double factorDelayedPrefPathDuring;
	double factorDelayedPrefPathAfter;

	int midTimeArrive;
	int midTimeFinish;

	int nPoints;
	spherical::Point* point;
	//int* allowedPoint;
	int nOutNodes;
	int nAllocOutNodes;
	int* outNode;
	//int* outPolyPoint;
	int* outLevel;
	int* outRestrictedAreaNr;
	int outRestrictedAreaNr_channel;
	int* outNoNormalArc_useTSS;
	int nArcsToPoint;
	int* nAllocTimeIntervals;
	int* nTimeIntervals;
	int** timeInterval;
	int** nodNr_from_pt;
	double* distanceFromStart;
	double* minCostOutLevel;

	int nConnectTo;
	int nAllocConnectTo;
	int* connectTo_outLevel;
	int* connectTo_outNode;
	int nConnectFrom;
	int nAllocConnectFrom;
	int* connectFrom_outLevel;
	int* connectFrom_outNode;

	int nodDelay[2];
	int nodDelay_prefPath[2];

	int legNr;

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
	int nOutNodesTot;
	int nPoints;
	spherical::Point *point;
	double* point_x;
	double* point_y;
	int* nAllocOutNodes;
	int *nOutNodes;
	int* nInNodes;
	int** outNode;
	int** outLevel;
	int** outRestrictedAreaNr;
	int** outNoNormalArc_useTSS;

	double* minDistPrevNode;
	int* minDistPrevNode_level;
	int* minDistPrevNode_pos;
	//int* nodeConnectedFromChannel;
	int requirePrefPathFeasible;
	double factorDelayedPrefPath;
	int onlyPrefPath;
	int followChannelExactly;

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

	int* nodDelay;
	int* nodDelay_prefPath;

	int legNr;
	double tidWait;
	double bransleWaitMain;
	double bransleWaitAux;
};

struct strNetwork
{
	int* channelOrder;
	int nPhysicalLevels;
	strNodeSeq *physicalLev;
	//int useLongitudeKvadrant[4];
	int nMaxNodesInPath;
	int nChannels;
	int nChannelsTmp;
	int nAllocChannels;
	strChannel* channel;
	strChannel* channelTmp;
	int nPhysicalNodes;
	int nPhysicalArcs;

	//int nUsedChannels;
	//int* usedChannel;

	int nAllocCoords;
	double* xCoord;
	double* yCoord;
	int nCoords;
	strBoundBox boundingbox;
	double last_x;
	double* startKvot;
	double* endKvot;
	int* posSplitCoord;
	int nMaxSplits;

	//int* arcGen_utilizeStaticWeather_ForOutNodePos;
	//int** arcGen_staticWeatherArcNr_outNodePosSpeed;
	int tidp_startHistoricDataOnly;
	int tidp_lastDelayTidp;
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
	double maxWaveHeight_warning;
	double maxWindSpeed;
	double maxWindSpeed_warning;
	strTableParam shipSpeedCalmWater;
	strTableParam windSpeed;
	strTableParam windDirection;
	strTableParam waveHeight;
	strTableParam wavePeriod;
	strTableParam waveDirection;
	strTableParam shipSpeedOverGround;
	strTableParam relShipSpeed;
};


struct strFunkData {
	strTableParam shipSpeedCalmWater;
	strTableParam windSpeed;
	strTableParam windDirection;
	strTableParam waveHeight;
	strTableParam wavePeriod;
	strTableParam waveDirection;
	strTableParam shipSpeedOverGround;
	strTableParam relShipSpeed;

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
	int sparaWaypointPos;
	int sparaWaypoint;
	double deltaArcStart;

	double last_x;
	double last_y;

	double fuel_main;
	double fuel_aux;
	double arcDel_fuel_aux;
	double arcDel_fuel_main;

	double totFuel_aux;
	double totFuel_main;
	double distance;

	double bowSlam;
	double greenWater;
	double dynamicStability;
	double rolling;
	double surfRiding;

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
	double maxWaveHeight_dir;
	double maxWindSpeed;
	int maxWindSpeed_tp;
	double maxWindSpeed_dir;
	double wavePeriod;
	double relWaveDir;
	double forecastType;
	
	double currentReal;
	double currentDirReal;
	double windReal;
	double windDirReal;
	double waveDirReal;

	double windDirReal_lastKnown;
	double windReal_lastKnown;
	double currentReal_lastKnown;
	double waveDirReal_lastKnown;

	double favorableWind[2][3];
	double favorableWave[2][3];
	double favorableWindWave[2][3];

	double iceCover_max;
	double bowSlamming_max;
	double greenWater_max;
	double dynamicStability_max;
	double rolling_max;
	double surfRiding_max;

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

	double favorableWind_h;
	double favorableWave_h;
	double favorableWindWave_h;
	double totFavorableWind_h;
	double totFavorableWave_h;
	double totFavorableWindWave_h;

	double sumWindSpeed;
	double sumRelCurrent;
	double sumCurrent;
	double sumWaveHeight;
	double maxCurrent;
	double sumSpeedOnWater;
	double speedOnWater;

	double obj_fel_maxWindSpeed;
	double obj_fel_maxWaveHeight;

	int prefPathArc;

	double totDistance_movingNoCorridors;
	double totTime_movingNoCorridors;
	double totFuel_mainMovingNoCorridors;
	double totFuel_auxMovingNoCorridors;

	double totCorridorWaitingFuel_mainECA;
	double totCorridorWaitingFuel_mainNonECA;
	double totCorridorWaitingFuel_auxECA;
	double totCorridorWaitingFuel_auxNonECA;
	double totWaitingTime;

	double accumRPM;
	double accumRPM_time;

	double totTimeArcSTW;
	double WindFArc;
	double WaveFArc;
	double CurrentFArc;
	double DelayFArc;

	double compare_route_endTime_h;
	double compare_fuelConsumption_ton;
	double compare_fuelEcaMain;
	double compare_fuelEcaAux;
	double compare_fuelNotEcaMain;
	double compare_fuelNotEcaAux;
	double compare_emission;
	double compare_emissionEcaMain;
	double compare_emissionEcaAux;
	double compare_emissionNotEcaMain;
	double compare_emissionNotEcaAux;
	double compare_averSpeed;
	double compare_totalDistance_kts;
	double compare_totalTime_h;
	double compare_dollar_cost;

	double deltaTid;
	double currentReal_u;
	double currentReal_v;
	double timeArc_current;

	double windSpeedReal_u;
	double windSpeedReal_v;
	double timeArc_wind;

	double waveHeightReal_u;
	double waveHeightReal_v;
	double timeArc_wave;

	double windSpeed_x;
	double windSpeed_y;

	double waveDir_x;
	double waveDir_y;

	double timeCheck;
	double fuel_eca;
	double waitingTime;
	double timeSinceLast;
	double distSinceLast;
	double distSinceLastCalmWater;

	int coords_lastFromLevel;
	int coords_lastToLevel;
	double coords_lastUsedKvot;

	double pressureSurface;
	double pressureAir;
	double precipitation;
	double tempSea;
	double tempAir;
	double cloudCover;
	double mdps;
	double swell;
	double timePressureSurface;
	double timePressureAir;
	double timePrecipitation;
	double timeTempSea;
	double timeTempAir;
	double timeCloudCover;
	double timeMdps;
	double timeSwell;
};

struct strSimulering {
	double penOverWeatherLimit_fix;
	double penOverMaxWaveHeight_m;
	double penOverMaxWindSpeed_kmh;
	double penDeviateSpeed_kmh;
	double penDeviatePrefPath_nodes;
};

struct strTables {
	int nBasAlloc;
	int nAllocTableTyp[8];
	int nTableTyp[8];
	strTableTyp* tableTyp[8]; // 0 wind, 1 wave, 2, fuelFactorMain, 3 stability, 4 bow slamming, 5 green water, 6 rolling, 7 surfRiding
};

struct strSpeed {
	int nShip_speedSettings;
	double* rpmSetting_gerCalmWaterSpeed;
	double* rpmSetting_gerFuelConsumption_main;
	double* rpmSetting_gerFuelConsumption_aux;
	double* rpm;

	int* settingGerBaseSetting;
};

struct strClosePoints {
	int posCoords;
	int posPrefPath;
	double kvotCoords;
	double kvotPrefPath;
	double distance;
};

struct strFunc2 {
	double timeNextWayPoint;
	int* nSpeedSettingsDelay;

	int nShip_speedSettingsBase;
	double* rpmSetting_gerCalmWaterSpeedBase;
	double* rpmSetting_gerFuelConsumption_mainBase;
	double* rpmSetting_gerFuelConsumption_auxBase;
	// int* nShip_speedSettingsDelay;
	double** rpmSetting_gerCalmWaterSpeedDelay;
	double** rpmSetting_gerFuelConsumption_mainDelay;
	double** rpmSetting_gerFuelConsumption_auxDelay;
	int speedSetting95MCR_base;
	int* speedSetting95MCR_use;

	strSpeed* speedLevel;
	strSpeed* speedChannelOut;
	strSpeed* speedChannel;
	int* nAllocShipSpeedsLevel;

	double* varValue;
	double* varValueAverage;

	std::string windTableID;
	std::string waveTableID_orig;
	std::string waveTableID;
	std::string stabilityTableID;
	std::string fuelFactorTableID;
	std::string bowSlammingTableID;
	std::string greenWaterTableID;
	std::string rollingTableID;
	std::string surfRidingTableID;

	int windTableNr;
	int  waveTableNr;
	int stabilityTableNr;
	int fuelFactorTableNr;
	//double maxWaveHeight;
	//double maxWaveHeight_warning;
	//double maxWindSpeed;
	//double maxWindSpeed_warning;

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

	int pos_pressureSurface;
	int pos_pressureAir;
	int pos_precipitation;
	int pos_tempSea;
	int pos_tempAir;
	int pos_cloudCover;
	int pos_mdps;
	int pos_swell;


	// weather factors
	strFunkData windFactor;
	strFunkData waveFactor;
	strFunkData fuelFactorMain;

	// safety
	double iceCoverMaxFree;
	//double iceCoverCost_fix;
	//strFunkData bowSlamming; // height + nHeight * period
	//strFunkData greenWater; // height
	strFunkData dynStability; // wSpeed + nWSpeed * wDir
	strFunkData bowSlamming; // height + period * nHeight
	strFunkData greenWater; // height
	strFunkData rolling;
	strFunkData surfRiding;

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
	double MAX_FAKTOR_NATVERK;
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

	char* stormID;
	// int stormNr;
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
	int *delayed_monthNr;
	int nDelayed_months;

	int* nStormsYear;
	strStorm** stormsYear;

	int* tidpHistorical_ger_delayMapNr;

	int nXinterval;
	int nYinterval;

	int* changedSpeed;
};

struct strDelayToEnd {
	double base_time;
	double changed_time;
	double distance;
	double base_fuel_main_noEca;
	double base_fuel_main_eca;
	double base_fuel_aux_noEca;
	double base_fuel_aux_eca;
	double changed_fuel_main_noEca;
	double changed_fuel_main_eca;
	double changed_fuel_aux_noEca;
	double changed_fuel_aux_eca;

	int nBVArcs;
	int* BVArc;
};

struct strSeaRoutePath {
	int nCoords;
	double* xCoord;
	double* yCoord;
	double costFactorDist;
	double maxDistConnect_km;
};

struct strExtraNoGoBase {
	char* areaID;
	char* fileNameA;
	char* fileNameB;
	double extraCostFactor;
	int nCorridors;
	strSeaRoutePath* corridor;
};

struct strExtraNoGo {
	char* areaID;
	int posBase;

	Raster rasterA;
	Raster rasterB;
	Raster::strPhysRaster mapA;
	Raster::strPhysRaster mapB;
	double extraCostFactor;
};

struct strPolygon {
	int nCoords;
	double* x;
	double* y;
};

struct strPolygonArea{
	char* id;
	double max_speed;
	double main_fuelConsumption;
	int nPolygons;
	strPolygon* polygon;
	OGRPolygon** polygon_GDAL;
};

struct strViaPos {
	int nPoints;
	double* x;
	double* y;
};

struct strAltRutt {
	int nParts;
	strViaPos* sekvens;
	char* routeID;
};

struct strCorridorSoft {
	int nPkter;
	double* xCoord;
	double* yCoord;
	int startNod;
	int endNod;
	double costFactorDist;
	int noGo_posBase;

	double maxDistConnectInside;
	double* distanceFromStart;
	double distance_km;
	spherical::Point* point;
	strBoundBox boundingBox;
	int autoPathNr;
};

struct strParamsAutoRoute {
	double* cost_route;
	double* dist_route;

	int nCorridors_noGoSoft;
	strCorridorSoft* corridors_noGoSoft;

	int nInSet[2];
	int nAllocSet[2];
	int* setCell[2];
	int* setSmallCell[2];

	int routeAlternative;
	char* zone_id;

	double startBas_lon;
	double startBas_lat;
	double endBas_lon;
	double endBas_lat;

	std::string zonesFileName;
	std::string zoneConnectionsFileName;
	int startZone;
	int endZone;

	double* startPoint_lon;
	double* startPoint_lat;
	double* endPoint_lon;
	double* endPoint_lat;
	int startNod;
	int endNod;

	std::string searoutePathsName;
	int newSeaRoutePathData;

	int nCellLevels;
	double* discretizationSizeLevel;
	int* nDiscreteSizeLevel;
	double x_min;
	double y_min;
	int nXbasLevel;
	int nYbasLevel;
	int nCellsBase;
	int nCellsLevel1;
	double factorExtraCover;
	double maxBaseFeasibleCost;

	//int usePenalty_ECA;
	//double eca_penalty;

	std::string mapAutoRoutePhysicalBFileName;
	std::string mapAutoRoutePhysicalAFileName;
	std::string tssName;
	std::string corridorsNameNew;
	std::string corridorsName;

	double tss_attractionDistance_km;

	int nStartSlut;
	strAltRutt* altRutt;
	int nAltRutter;

	double* minLat_lonIndex;
};

struct strAutoCells {
	double arcCost[8]; // 0 horizontal, 1 diagonal up, 2 vertical, 3 diagonal down
	double arcDistance[8];
	int nodeNr[4]; // 0 lowerLeft, 1 lowerRight, 2 upperLeft, 3 upperRight
	strAutoCells* smallerCells;
	double x;
	double y;
	int isLand;
	int nXsmall;
	int nYsmall;
	int smallerCellsType;
	int use;
	int firstUsePathPos;
	int lastUsePathPos;

};

struct strPair {
	char* groupID;
	int nFrom;
	double* xFrom;
	double* yFrom;
	int nTo;
	double* xTo;
	double* yTo;


};

struct strSeaRoute {
	int nSeaRoutePaths;
	strSeaRoutePath* seaRoutePath;
	double* nod_y;
	double* nod_x;

	int nNewPathPairs;
	strPair* pair;

	double** distLat;
	int* nDistLat;
	int* nAllocDistLat;
	int nDistLatAlt;
	double* latVal;
	double costFactorExtraNoGo;

	int* nextSeaRouteNodePos;
};

struct strTss {
	double* xCoord;
	double* yCoord;
	int nCoords;
	int firstTraffCoord;
	int lastTraffCoord;
	int firstTraffPos;
	int lastTraffPos;
	double firstTraffCoordKvot;
	double lastTraffCoordKvot;
	int autoPathNr;
	double kvotCost;
};

struct strAutoPath {
	int* nodNr;
	double* nodCoord_y;
	double* nodCoord_x;
	int nNoder;
	int nAllocNoder;
	int type; // 0 - tss, 1 - corridor, 
			  // 2 - connector to tss/corridor, 
			  // 3 - new connectors to nodes along SP
	double kvotCost;
	//double kvotMinCost;
};

struct strKaoutarData {
	int radNr;
	double x0;
	double y0;
	double x1;
	double y1;
	int shipType;
	double rpm;
	double sog;
	double fuelCons;
	double windDir;
	double windSpeed;
	double waveDir;
	double waveHeight;
	double currDir;
	double currSpeed;
};

struct strKaoutar {
	int nShipTypes;
	char* windTableID;
	char* waveTableID;
	char* stabilityTableID;
	char* fuelFactorTableID;
	int nShip_speedSettingsBase;
	double* rpmBase;
	double* rpmSetting_gerCalmWaterSpeedBase;
	double* rpmSetting_gerFuelConsumption_mainBase;
	double* rpmSetting_gerFuelConsumption_auxBase;
};

struct strFixedArc {
	int arcNr;
	int fromLevel;
	int toLevel;
	int fromPos;
	int toPos;
	double fromTime;
	double toTime;
	int speedSettingBase;
};

struct strIterKaoutar {
	long long UTC_secondsFirstStart;
	int physLevelStart;
	int physLevelStartUse;
	int physPointStart;
	int fixedArcsEnd;
	double startTimeUse;

	double tidpStartIter_h;
	double tidpStartIterArc_h;
	int nLevelsMoveForeward;
	int nForecastRuns;
	int evalAlt;

	strFixedArc* fixedArcs;
	int nFixedArcs;
	FILE* filpek;
	//double fuelUsedSoFar_aux;
	//double fuelUsedSoFar_auxEca;
	//double fuelUsedSoFar_eca;
	//double fuelUsedSoFar_noEca;

};

struct strLegRes {
	double startTidPkt;
	double slutTidPkt;
	double channelCost;
	double totObjCost;

	double accumRPM;
	double accumRPM_time;

	double totWindF;
	double totWaveF;
	double totCurrentF;
	double totDelayF;

	double fuel_eca;
	double fuel_noEca;
	double fuel_aux;
	double fuel_auxEca;
	double distance;
	double dist_eca;
	double totWaitingTime;
	int nCoords;
	int nAllocCoords;
	double* x;
	double* y;

};
struct strResults {
	strLegRes* leg;
	double bowSlam_notAllowed;
	double greenWater_notAllowed;
	double rolling_notAllowed;
	double dynamicStability_notAllowed;
	double surfRiding_notAllowed;
	double maxWaveHeight_notAllowed;
	double maxWindSpeed_notAllowed;
	double hurricane_insideOuterCircle;
	double hurricane_maxCost_insideOuterCircle;
	double hurricane_insideInnerCircle;
	double hurricane_maxCost_insideInnerCircle;

	double bowSlam_aver;
	double bowSlam_0;
	double bowSlam_01;
	double bowSlam_05;
	double bowSlam_2;
	double greenWater_aver;
	double greenWater_0;
	double greenWater_01;
	double greenWater_05;
	double greenWater_2;
	double dynamicStability_aver;
	double dynamicStability_0;
	double dynamicStability_01;
	double dynamicStability_05;
	double dynamicStability_2;

	double rolling_aver;
	double rolling_0;
	double rolling_01;
	double rolling_05;
	double rolling_2;
	double surfRiding_aver;
	double surfRiding_0;
	double surfRiding_01;
	double surfRiding_05;
	double surfRiding_2;

	double stormValue_aver;
	double worstStormValue_max;

	FILE* fileForecast;
	FILE* fileForecast2;
	std::string fileNameForecast;
	int forecastType;
	int forecastTypeOrig;
	int onlyPrefPath_kaoutar;

	double weightTime;
	double weightFuel;
	double weightEmission;
	double weightSafetyBase;
	char* optRunDateTime;
	double fuel;
	double time;
	double fuelCost;
	double timeCost;
	double dist;
	double safety;
	double objCost;
	double totWeatherFactors;

	double iterTotDistStart;
	double iterTotFuelStart;
	double iterTotObjStart;
	double iterTotDollarCostStart;

	double iterWeatherFactorsStart;
	double iterSafetyStart;
};

struct strFilDirs{
	int presentPos;
	char** filPath;
};

struct strWaypoint {
	double tidp;
	double x;
	double y;
	double calmWaterSpeed;
	double direction;
	int type;
	int use;
};

struct strWaypointResults {
	int arcNr;
	char* dateUTC;
	char* full_Date;

	int ii;
	double arc_obj_cost;
	int fromLevel;
	int fromPointNr;
	int fromTime;
	double accumTimeStart_h;

	double diffTime;
	int toLevel;
	int toPointNr;
	int toTime;
	double hours;
	double checkHours;
	double distance_nm;

	double windF;
	double waveF;
	double currentF;
	double delayF;
	double accumDistance_km;
	double accumDistance_last_km;

	char* fixPositionString_latlon;
	double bearing;

	int bearingDiff;
	int nChangeBearingBetween;
	int nChange_lessXdegrees;
	double fuelMain_tot;
	double time_tot;
	double fuelAux_tot;
	double fuelEca;

	int speedSetting;

	double current;
	double calmWaterSpeed;
	double speedOnGround;
	double rpm;
	double windSpeed;
	double relWindDir;
	double waveHeight;
	double wavePeriod;
	double relWaveDir;

	double pressureSurface;
	double pressureAir;
	double precipitation;
	double tempSea;
	double tempAir;
	double cloudCover;
	double mdps;
	double swell;
	double maxWaveHeight;
	double maxWindSpeed;

	double windReal_lastKnown;
	double windDirReal;
	char* windDirReal_letters;
	double currentReal_lastKnown;
	double currentDirReal;
	double waveDirReal_lastKnown;
	double waveDirReal;
	char* waveDir_letters;

	double bowSlam;
	double greenWater;
	double dynamicStability;
	double rolling;
	double surfRiding;

	double iceCover_max;
	double forecastType;

	double worstStormValue;

	double fuelTransit_main;
	double fuelTransit_aux;
	double fuelWaiting_main;
	double fuelWaiting_aux;
	double transitTime;
	double waitingTime;
	double emissionWaiting_main;
	double emissionWaiting_aux;

	double xCoord;
	double yCoord;
};

struct strTempData {
	int nMax_recursive_arcsLevels;
	int* arr_fromLev;
	int* arr_fromNode;
	int* arr_fromPos;
	int* arrBas_fromLev;
	int* arrBas_fromNode;
	int* arrBas_fromPos;
	int* arr_cNr;
};

struct strModel
{
	strTempData temp_data;
	char* nameTmp;
	strWaypointResults waypointResult;
	strWaypoint* waypoint;
	int nWaypoints;
	strFilDirs filDirsForecast;
	strResults results;
	strIterKaoutar iterKaoutar;

	strSimulering simulering;

	strKaoutar* kaoutar;

	int nBVArcsUse;
	int* BVArcUse;

	strParamsAutoRoute paramsAutoRoute;
	strAutoCells* autoRoute;
	strSeaRoute seaRoute;

	int nAutoPaths;
	strAutoPath* autoPath;

	int nTss;
	strTss* tss;
	int nAutoCorridors;
	strTss* autoCorridors;
	int autoRoute_startNod;
	int autoRoute_endNod;

	int nExtraNoGoAreasBase;
	strExtraNoGoBase* extraNoGoAreaBase;
	int nExtraNoGoAreas;
	int nExtraNoGoPolygons;
	strExtraNoGo* extraNoGoArea;
	strPolygonArea* extraNoGoPolygon;

	int nRestrictedAreas;
	strPolygonArea* restrictedArea;

	int nExtraCostAreas;
	strExtraNoGo* extraCostArea;

	double scaledDelay;

	FILE* filpek;
	strDelayToEnd** delayRouteToEnd;
	strDelayToEnd** delayRouteToEnd_channel;

	strDelayToEnd** delayRouteToEnd_prefPath;
	strDelayToEnd** delayRouteToEnd_channel_prefPath;


	int nErrorCoordBB;

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
	strWeather *delayedGrid;
	strWeather* delayedCurrent[2];

	int nStorms;
	int nAllocStorms;

	strStorm* storms;
	strPath preferredPath;
	strPath solutionPath;
	strCorrLines corridorPath;
	strNetwork network;
	//Raster physicalMapRaster;
	Raster::strPhysRaster physicalMapA;
	Raster::strPhysRaster physicalMapB;
	Raster::strPhysRaster physical_lessBuffer_MapA;
	Raster::strPhysRaster physical_lessBuffer_MapB;
	//Raster rasterPhysicalMapA;
	//Raster rasterPhysicalMapB;

	//Raster::strPhysRaster fuelMapA;
	//Raster::strPhysRaster fuelMapB;
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
	int nAllocBVArcs;
	int *BVArc;
	int *BVtempNodOrder;

#ifdef _WIN32
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
char* append_str_alloc_cpyString(char* oldName, std::string data);

int write_copyAtoB(char *filnamnUt, char *filExt, char *filenamnIn, char *mode);

int SattUppDijkstraNatverk3(strModel* model, std::string saveNatvName);
int SattUppDijkstraNatverk3tmp(strModel* model);
int ChangeArcCosts3(strModel* model);
int AnropDijkstra2(int NodA, int NodB, strModel *model, bool *Reached);
double NystaUppBV_MassTest(strModel *model, int Reached, int NodA0, int NodB0, long long *Cost);
double lasInLsngFromFil_MassTest(strModel* model, int Reached, int NodA0, int NodB0, long long* Cost);

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
int voyageOpt_fixPartSol(std::string inputPath, std::string resultName);
int dump_weatherForecasts(std::string inputPath, std::string resultName);

int generateDelayedFactors(std::string inputName, int node, int manad);
int extractGribInfo(std::string inputPath);
int exitKontrollerat(int codeLine, int callType = 1);
std::string splitFilename(std::string namn, int alt = 0);
int fixReadableDate(struct tm tmBas, char* namn);
int fixReadableDate_file(struct tm tmBas, char* namn);
int initGeoJsonFil(FILE* filpek, const char* namn);
void get_fuelUseKvotECA(double lat1, double lon1, double lat2, double lon2, int mapAlt, double* distECA, double* distOther);
void get_UseKvotExtraArea(double lat1, double lon1, double lat2, double lon2, int posExtraArea, int extraType, int mapAlt, double* distArea, double* distOther);

int redisSetKeys(std::string inputPath);
void putStringIntoArrayFloat(std::string strang, float* arrFloat, FILE* filtmp);

double eval_baseGroundSpeed(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed);
double lookup_speedDiffWindWaveTable(double rel_windSpeed, double rel_windDir, double waveHeight, double wavePeriod, double rel_waveDir);
//double eval_fuelConsumption_main(int speedNr);
//double eval_fuelConsumption_aux(int speedNr);
double eval_fuelConsumption_both(int speedNr, double* consumptionAux, int fromLevel, int toLevel, int restrictedAreaNr, double calmWaterSpeed, double fuelFactorMain);

double eval_relWindSpeed(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir);
void eval_safety(int legNr, double iceCover);
void eval_safety_nazanin(int legNr, double shipSpeedOverLand, double shipSpeedRelWater, double windspeed, double absWindDirDiff, double waveHeight,
	double wavePeriod, double relWaveDirection, double iceCover);
int calcWeatherPosAlongpreferredPathArc(spherical::Point p1, int level);
int calcWeatherPosAlongChannel(int cNr);
double eval_calmWaterSpeed(int speedNr, int fromLevel, int toLevel, int restrictedAreaNr);
double lookup_speedDiffWaveTable(double calmWaterSpeed, double waveHeight, double wavePeriod, double rel_waveDir);
double lookup_speedDiffWindTable(double calmWaterSpeed, double rel_windSpeed, double rel_windDir);
double lookup_fuelFactorMainTable(double rel_windSpeed, double rel_windDir, double waveHeight, double rel_waveDir);


int get_tableIndex(double value, strTableParam param, int alt = 0);
int get_tableIndexDirection(double value, strTableParam param, int alt = 0);
int eval_coordWithinBoundingBox(double lon, double lat);
int check_isCoordFeasiblePhysicalMap(double y, double x);
void setupBoundingBoxFromMapCoords(Raster map);
int getAllVariableValues(int checkPointNr, double tidpkt);
int delayTimeToStartTimeDay(int t, int arrivalTime);
double eval_relWindSpeedExact(double baseGroundSpeed, double bearing, double windDir, double windSpeed, double* rel_windDir);
double eval_baseGroundSpeedExact(double calmWaterSpeed, double bearing, double currentDir, double currentSpeed);
int makeSure_feasibleCoordFranLinje(double* y1, double* x1, double* y2, double* x2, int pos);
int roundUp(double varde);
int check_nodeIsWithinPhysicalMapRaster(double lat1, double lon1);
int setUpUsablePointsInPolygonChannel(int cNr, int pos);
int identify_startEndOnChannel(int cNr, int startEnd);
int identify_startEndOnChannel_tss(int cNr, int startEnd);
int checkCoordInBoundingBox(double y, double x, strBoundBox bbox);
void updateBoundingBoxWithCoord(strBoundBox* bbox, double y, double x);
void initBoundingBox(strBoundBox* bbox);
void setupUsableSpeedSettings();
int check_translate_xCoord(double* xCoord);
int fixReportDate(struct tm tmBas, char* namn);
void postRequest(std::string errorMessage, int avsluta);
int updateSQLiteAllTablesInfo(int type, int tablePos, int modified);
int saveTablesToSQLite(std::string inputPath);
int updateCorridors(std::string inputPath);
int call_api_corridors(std::string inputPath);
int load_autoCorridors(int alt);
int loadSave_downloadedCorridors();

int loadFileParams_feasibilityAuto(strParamsAutoRoute* params);


int testSaveMapToBinaryFile();
//int saveMapsToBinary();
unsigned short* openBinaryMap(int ii, Raster::strPhysRaster* physRaster, strBoundBox boundingBox);

int checkMinnesAnvandning(int rad);

int loadParams_theRestOld(strParams* params);
int loadFileParams_feasibilityOptiNav(strParams* params);
int loadParams_new(strParams* params);

int loadAllNeededTablesFromSQLite();
int loadVariables(int alt = 0);
int createPhysicalNetwork(int sparaKorridorEnbart, int alt);
int adderaNod(int physicalLevel, int pointNr, int timeInterval);
int check_useRaster_longitude(int weatherNr, int filNr);
int addEndBage(int thisLevel, int pos1, int nextLevel, int i3, int nodNr2, int i2b);
double estimateLargeCircleDistance_km(double lat1, double lon1, double lat0, double lon0);
double estimateLargeCircleDistance2_km(double lat1, double lon1, double lat0, double lon0);
int adderaArc(int nodNr1, int nodNr2, double cost, int speedSetting);
int addBagar_AB_speedSTid(int thisLevel, int pos1, int nextLevel, int pos2, int i2b, 
	int* setupCheckPoints, int min_t, int max_t, double fuelQualityKvot, double extraAreaCostKvot, int runAlt = 0);
int addPositionDataToReport(FILE* filpekG, int *posReport, int arcNr, int startSlutArc, double* timeExact, std::string solName, int useFixCalmWaterSpeed = 0, int iter = 0);
double getCorrect_longitude(double x);
void fixPositionString_latLon(double y, double x, char* namn);
int set_speedSettingsFromBase(strSpeed* speedSetting, int i, int iUse, int iOver = -1, double kvot = 0, int legNr = 0);
long long getSecondsFromUTC(const char* time);

int testCallWeatherFile();
//int roundDown(double varde);

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
int getMonthsToUseForDelay_new(double dist);
double get_fuelQualityKvot(int thisLevel, int pos1, int nextLevel, int pos2);
double get_extraAreaKvot(int thisLevel, int pos1, int nextLevel, int pos2, int posExtraArea, int extraType);

std::string stringDateFromUTCSeconds(long long seconds);
double get_colDblFromWeatherFile(int weatherNr, double lon);

int solve_SP_delay();
int solve_SP_delayPrefPath();

int testAnrop(strModel* modelDelay, int nod2);
double getSpeedDiff_currentDelayedFromBearing(int fromLevel, int pos1, int toLevel, int pos2, int delayNr, double bearing, double lat, double lon, double calmWaterSpeed = -1.0);
double calcArcTimeCost(int tidInt, int speedSettingNr, int fromLevel, int toLevel, int restrictedAreaNr, double* calmWaterSpeed, double fuelFactorMain = -1.0);
int checkWeatherCoverOK(int xPos, int yPos);
int getClosestSetting_fromBase(int baseSetting, int fromLevel, int toLevel);
int determineBastSpeedDelay_routeToEnd_eta(strDelayToEnd* routeToEnd, double startTidp);
int calcWeatherPosAlongpreferredPathArc_connectChannel(spherical::Point p1, int level1, spherical::Point p2, int level2);
double eval_factorDelayedAlongArc_currSpeedDiff(int thisLevel, int pos1, int nextLevel, int pos2, int tidp, double* speedDiffCurrent, double calmWaterSpeed = -1.0);
double eval_factorDelayedAlongArc(int thisLevel, int pos1, int nextLevel, int pos2, int tidp);
int getBaseSpeedSetting(int speedSetting, int lev1, int lev2);
int getDelayPosFrom_tidp(int tidp);
void getCurrent_fromCurrentDelayed(int delayNr, double lat, double lon, double* uCurrent, double* vCurrent);

bool check_file_exist(char* name);
int openNeededRasterFilesNew(int alt);
int calc_boundingBoxAutoRoute();
int check_physicalMap_ok(double lat1, double lon1, double lat2, double lon2, int mapAlt);
int check_extraNoGoMap_ok(double lat1, double lon1, double lat2, double lon2, int mapAlt, int pos_noGoMap);
int genAutoRoute(std::string inputPath, std::string resultName);
int findAreaIDpos_inBase(char* ID);
int initLookUpTables();
int getCoordFromAutoArc(int arcNr, int fromTo, double* y, double* x);
void getRowColDblFromPhysicalMap(Raster::strPhysRaster physicalMap, double lat1, double lon1, double* row1Dbl, double* col1Dbl);
void getRowColDblFromNoGoMap(Raster::strPhysRaster physicalMap, double lat1, double lon1, double* row1Dbl, double* col1Dbl);
int fixStormFiles(std::string inputPath);
double get_nextKvotHeltal(double x, double xBas, double dx);
int addAutoNodePath(int tssNr, double y, double x);
int addArcsInOutFromPathNode(int nodNr, int tssNr, int prevNodNr, double distPrev, int firstLastNode);
double addAutoArcBetweenPaths(int path1, int posPath1, int path2, int posPath2);
int checkAllocNode(int nodNr);
int checkSameDir(double dY, double dX, double dY2, double dX2);
int evalKaoutarData(std::string inputPath);
int evalSeaRoutePaths(std::string inputPath);
int addSmallerCellsToCell(int pos, int i, int i1, int mustUse = 0);
double getCostKvotFromBadKvots_feasibility(double y1, double x1, double y2, double x2, int includeCostFeasible = 1);
int openNoGoAreas_local(int i, char* namn2);
double check_map_badKvot_auto(double lat1, double lon1, double lat2, double lon2, int mapAlt, int pos_noGoMap, int costArea = 0, double* yBad = NULL, double* xBad = NULL);
void init_tmBas();
int fixReportDateNew();
int get_speedSettingBase(int arcNr);
double eval_absWindDirDiff(double bearing, double windDir);

int voyageEval_fixSol(std::string inputPath, std::string resultName);
int fixReportDate_full(struct tm tmBas, char* namn);
int fixReportDateNew_full();

std::string cleanString(std::string namn);
int determineBastSpeedDelay_routeToEnd_eta_arc(int arcNr, double tidp);
int copyToArcFromDelay(int posDelay, int arcNr, double timeExact, int iter, int fullCalc = 1);


int addArcDelayToGeojson(FILE* filpek, strDelayToEnd* routeToEnd);
int addArcDelayToGeojson(FILE* filpek, strDelayToEnd* routeToEnd);
int addEnBage_AB(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int i4, int* setupCheckPoints, int min_t, int max_t,
	double fuelQualityKvot, double extraAreaCostKvot, int runAlt);
int genArcsTo_delayedPreferredPath(int thisLevel, int pos1, int nextLevel, int pos2, int i2b, int tPos, int nSpeedSettings, double fuelQualityKvot,
	double extraAreaCostKvot, double* delayFactor, double* distArc);
int addEnBage_delayAB(int thisLevel, int pos1, int nextLevel, int pos2, int i2b, int restrictedAreaNr, int tPos, int i4, int min_t, int max_t, double fuelQualityKvot,
	double extraAreaCostKvot, double dist, double delayFactor);
double calcArcTimeCostChannel(int t, int speedSettingNr, int channelNr, int restrictedAreaNr, double* calmWaterSpeed);
double calcDelayedArcTimeCost(int fromLevel, int toLevel, int restrictedAreaNr, int speedSettingNr, double calmWaterSpeed, double fuelFactorMain, double factorDelay, double dist, double speedDiff);
int freeAllNodData();
int getTidIntForecast(double tidTot);
float ApproxAtan(float z);
float ApproxAtan2(float y, float x);
int addTimeTo_timeInterval(int levPrev, int levNr, int pointNr, int tidInt);
double eval_speedDiffCurrent_delayedAlongArc(int thisLevel, int pos1, int nextLevel, int pos2, int tidp, double calmWaterSpeed);
double eval_factorDelayedAlongPath(int level1, int level2, int tidp, double* speedDiffCurrent);
int sparaLastWaypoint(FILE* filpekG, int* posReport, std::string solName);
int writeSolutionToJson(std::string filename, int resAlt, char* namnSol, int iter, int nFinalRoutes = 0);
double evalWeatherDataAlongArcSection(int arcNr, double startKvot, double endKvot, int startSlutArc, double timeExact, double delayFactor, int useFixCalmWaterSpeed);
double calcBearingFromToCoords(double lat1, double lon1, double lat2, double lon2);
double getDiff_anglesDegrees(double x1, double x2);
double identifyForecastType(double tidTot);
float lookUpCos(float x);
float lookUpSin(float x);
int setTMtime(struct tm* tmBas, double time);
void calc_stormsNearby();
time_t getFirstSecondOfDay(long long seconds);
int getManadDagFranUTCSeconds(long long seconds, int* dag);
void getBastSpeedPos(int nSettings, double target, int* indexUnder, int* indexOver, double* kvot);
void getBastConsumptionPos(int nSettings, double target, int* indexUnder, int* indexOver, double* kvot);
double evalWeatherDataAlongArc(int arcNr, int legNr, double timeExact); //  , int speedSettingGiven = -1);
int check_isPhysicalArcOK(int startLevel, int slutLevel, int pos1, int pos2, int allowShortArc = 0, int followPrefPathExact = 0);
void 	initModelStatusValues();
long long make_gmtime_fromDateTimeString(std::string tidpkt, strParams* params = NULL);
int evalDistanceBetweenPrefPathAndChannel(int level1, int level2);
int loadWeights(strParams* params, json dataObj);
void copyAddTableInfo(strTableParam paramFrom, strTableParam* paramTo);
int loadWeatherFactorTableWind(int tableNr);
int  loadDynamicStabilityTable(int tableNr);
int  loadBowSlammingTable(int tableNr);
int  loadGreenWaterTable(int tableNr);
int loadWeatherFactorTableWave(int tableNr);
int  loadFuelFactorMainTable(int tableNr);
int getBastPhysLevelToConnectToChannel(int alt, int cNr);
void addStatisticsSafety(int arcNr);

long long getCostFromDijkstra(strModel* model, int nod);
int get_isWindFavorable(double windSpeed, double rel_windDir);
int get_isSeaFavorable(double waveHeight, double rel_waveDir);

int loadRollingTable(int tableNr);
int loadSurfRidingTable(int tableNr);

//int addBastSpeed_arcDelayed(int thisLevel, int pos1, int nextLevel, int pos2, int tidInt, int nSpeedSettings,
//	double fuelQualityKvot, double extraAreaCostKvot, int addArc = 1);
int addBastSpeed_arcDelayed(int thisLevel, int pos1, int nextLevel, int pos2, int i2b, int tidInt, int nSpeedSettings,
	double fuelQualityKvot, double extraAreaCostKvot, int addArc = 1);

int getLegNrFromLevels(int thisLevel, int nextLevel);
int check_noGoPolygons_ok(double lat1, double lon1, double lat2, double lon2);
int check_feasibleNode_noGo_polygons(double lat, double lon);
int add_custom_noGo_areas(json data);
int testIntersect();
double intersect_kvot(OGRLineString line, OGRPolygon* polygon);
int determine_nSpeedSettingsToUse(int level1, int level2, int restrictedAreaNr);
int get_restrictedAreaNr(int level1, int fromPointNr, int outNodePos, int level2 = -1, int toPointNr = -1);
int addArcsSmallToEndNode(int yPos, int xPos, int level, int nodEnd, int pathNr, int firstLastNode);
int addArcsSmallFromStartNode(int yPos, int xPos, int level, int nodNr, int pathNr, int firstLastNode);
int find_first_intersect(OGRLineString line, OGRPolygon* polygon, double* yBad, double* xBad);

int addChannelArcs(int cNr, int runAlt);
int genArcsToEnd_delayed(int thisLevel, int pos1, int nextLevel, int pos2, int i2b, int tPos, int nSpeedSettings);
int genArcsToEnd_delayed_prefPath(int thisLevel, int pos1, int nextLevel, int pos2, int i2b, int tPos);
void SwapArray(int* Array, int a, int b);

int check_realloc_coords(int nUsed);

double get_totalExtraAreaCostKvot(int thisLevel, int pos1, int nextLevel, int pos2, double* fuelQualityKvot);

/*
restrictedAreaNr = get_restrictedAreaNr(modelDelay.arc[arcNr].fromLevel, modelDelay.arc[arcNr].fromPointNr,
	modelDelay.arc[arcNr].outNodePos);
	restrictedAreaNr = get_restrictedAreaNr(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
		model.arc[arcNr].outNodePos);


	extraAreaCostKvot = get_totalExtraAreaCostKvot(i, i1, nextLevel, i2, &fuelQualityKvot); // only calculate this for physical arcs!!! use model.arc[xx].outNodePos...

	*/


int solve_dijkstras_claude(strModel* model);
int solve_dijkstras_chatGPT(strModel* model);
std::string load_entire_file(const std::string& path);

#endif //PCH_H

