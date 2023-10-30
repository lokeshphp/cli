
#include "pch.h"
//#include <time.h>
//#include <cmath>
#include<fstream>

//extern double cos_table[20001];
//extern double sin_table[20001];
//extern double atan_table[20001];
//extern double LOOKUP_COS_STEP_INV;
extern strModel model;
extern std::string resultPath;
extern int SKRIV_UT_NOTHING;
double DIST_SPLIT = 250.0;

int nMAX_ITER = 3;
int nMAX_ADD_NODES;


strModel modelSea;

int loadParams_autoRoute(strParamsAutoRoute* params)
{
	int i, closestI, pos2, minLon, maxLon;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux, minLat, distFromStart, minX, maxX, minY, maxY, x, y;


	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s", model.params.indataPathName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist but given as input data to OptiNav-autoRoute.I quit\n", 1);
	}
	printf("opens %s\n", namn);
	fil.open(namn);

	json data, dataGeo, dataGeo2, dataFeature, dataProp, dataGeo3, dataCoord;
	json dataIt, dataIt2, dataIt3;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu, pos, posBase, i1, nAllocSoft;
	double xVal, yVal;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav-autoRoute again.", 1);
	}

	if (!data["SKRIV_UT_NOTHING"].is_null()) {
		SKRIV_UT_NOTHING = data["SKRIV_UT_NOTHING"];
		if (SKRIV_UT_NOTHING < 2)
			reset_errlog();
	}

	if (!data["startPoint_lon"].is_null())
		params->startBas_lon = data["startPoint_lon"];
	else
		params->startBas_lon = -9999;
	if (!data["startPoint_lat"].is_null())
		params->startBas_lat = data["startPoint_lat"];
	else
		params->startBas_lat = -9999;
	if (!data["endPoint_lon"].is_null())
		params->endBas_lon = data["endPoint_lon"];
	else
		params->endBas_lon = -9999;
	if (!data["endPoint_lat"].is_null())
		params->endBas_lat = data["endPoint_lat"];
	else
		params->endBas_lat = -9999;
	if (!data["newSeaRoutePathData"].is_null())
		params->newSeaRoutePathData = data["newSeaRoutePathData"];
	else
		params->newSeaRoutePathData = 0;

	if (!data["routeAlternative"].is_null())
		params->routeAlternative = data["routeAlternative"];
	else
		params->routeAlternative = -1;

	if (!data["useOptionalExtraNoGoAreas"].is_null()) {
		json dataExtra = data["useOptionalExtraNoGoAreas"];
		model.nExtraNoGoAreas = dataExtra.size();
		model.extraNoGoArea = (strExtraNoGo*)malloc((model.nExtraNoGoAreas + 1) * sizeof(strExtraNoGo));
		pos = 0;
		nAllocSoft = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			std::string namnID = dataNu["extraAreaID"];
			model.extraNoGoArea[pos].areaID = str_alloc_cpy(namnID.c_str());
			posBase = findAreaIDpos_inBase(model.extraNoGoArea[pos].areaID);
			if (posBase < 0) {
				errlog("ERROR! extra noGoAreaID %s is not defined in file_params.json. Add this area. I ignore it for now.\n",
					model.extraNoGoArea[pos].areaID);
				postRequest("ERROR! extra noGoAreaID " + std::string(model.extraNoGoArea[pos].areaID) + " is not defined in file_paramsFeasibility.json. Add this area. I ignore it for now and keep running.", 0);
				continue;
			}
			model.extraNoGoArea[pos].posBase = posBase;
			nAllocSoft += model.extraNoGoAreaBase[model.extraNoGoArea[pos].posBase].nCorridors;
			pos++;
		}
		model.nExtraNoGoAreas = pos;


		model.paramsAutoRoute.nCorridors_noGoSoft = 0;
		if (nAllocSoft > 0) {
			model.paramsAutoRoute.corridors_noGoSoft = (strCorridorSoft*)malloc(nAllocSoft * sizeof(strCorridorSoft));
			int nNu = 0, antal;
			for (i = 0; i < pos; i++) {
				for (i1 = 0; i1 < model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].nCorridors; i1++) {
					model.paramsAutoRoute.corridors_noGoSoft[nNu].costFactorDist = model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].corridor[i1].costFactorDist;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].maxDistConnectInside = model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].corridor[i1].maxDistConnect_km;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].noGo_posBase = model.extraNoGoArea[i].posBase;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].startNod = -1;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].endNod = -1;
					antal = model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].corridor[i1].nCoords;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].nPkter = antal;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].xCoord = (double*)malloc(antal * sizeof(double));
					model.paramsAutoRoute.corridors_noGoSoft[nNu].yCoord = (double*)malloc(antal * sizeof(double));
					model.paramsAutoRoute.corridors_noGoSoft[nNu].distanceFromStart = (double*)malloc(antal * sizeof(double));
					model.paramsAutoRoute.corridors_noGoSoft[nNu].point = (spherical::Point*)malloc(antal * sizeof(spherical::Point));

					distFromStart = 0;
					minX = 360;
					maxX = -360;
					minY = 90;
					maxY = -90;
					for (i2 = 0; i2 < antal; i2++) {
						x = model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].corridor[i1].xCoord[i2];
						y = model.extraNoGoAreaBase[model.extraNoGoArea[i].posBase].corridor[i1].yCoord[i2];
						model.paramsAutoRoute.corridors_noGoSoft[nNu].xCoord[i2] = x;
						model.paramsAutoRoute.corridors_noGoSoft[nNu].yCoord[i2] = y;
						model.paramsAutoRoute.corridors_noGoSoft[nNu].point[i2] = spherical::Point(y, x);
						if (i2 > 0)
							distFromStart += estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[i2 - 1],
								model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[i2 - 1], y, x);
						model.paramsAutoRoute.corridors_noGoSoft[nNu].distanceFromStart[i2] = distFromStart;
						if (x < minX)
							minX = x;
						if (x > maxX)
							maxX = x;
						if (y < minY)
							minY = y;
						if (y > maxY)
							maxY = y;
					}
					model.paramsAutoRoute.corridors_noGoSoft[nNu].distance_km = distFromStart;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].boundingBox.yMin = minY;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].boundingBox.yMax = maxY;
					if (maxX < -180) {
						minX += 360;
						maxX += 360;
					}
					else {
						if (minX > 180) {
							minX -= 360;
							maxX -= 360;
						}
					}
					model.paramsAutoRoute.corridors_noGoSoft[nNu].boundingBox.xMin = minX;
					model.paramsAutoRoute.corridors_noGoSoft[nNu].boundingBox.xMax = maxX;
					nNu++;
				}
			}
			model.paramsAutoRoute.nCorridors_noGoSoft = nNu;
		}

	}
	else {
		model.nExtraNoGoAreas = 0;
		model.extraNoGoArea = (strExtraNoGo*)malloc((model.nExtraNoGoAreas + 1) * sizeof(strExtraNoGo));
		model.paramsAutoRoute.nCorridors_noGoSoft = 0;
	}

	if (!data["useOptionalExtraCostAreas"].is_null()) {
		json dataExtra = data["useOptionalExtraCostAreas"];
		model.nExtraCostAreas = dataExtra.size();
		model.extraCostArea = (strExtraNoGo*)malloc((model.nExtraCostAreas + 1) * sizeof(strExtraNoGo));
		pos = 0;

		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			std::string namnID = dataNu["extraAreaID"];
			model.extraCostArea[pos].areaID = str_alloc_cpy(namnID.c_str()); 

			posBase = findAreaIDpos_inBase(model.extraCostArea[pos].areaID);
			if (posBase < 0) {
				errlog("ERROR! extra costAreaID %s is not defined in file_paramsFeasibility.json. Add this area. I ignore it for now.\n",
					model.extraCostArea[pos].areaID);
				postRequest("ERROR! extra cost AreaID " + std::string(model.extraCostArea[pos].areaID) + " is not defined in file_paramsFeasibility.json. Add this area. I ignore it for now and keep running.", 0);
				continue;
			}
			model.extraCostArea[pos].posBase = posBase;

			if (!dataNu["extraCostFactor"].is_null()) {
				model.extraCostArea[pos].extraCostFactor = dataNu["extraCostFactor"];
			}
			else {
				model.extraCostArea[pos].extraCostFactor = model.extraNoGoAreaBase[posBase].extraCostFactor;
			}
			pos++;
		}
		model.nExtraCostAreas = pos;
	}
	else {
		model.nExtraCostAreas = 0;
		model.extraCostArea = (strExtraNoGo*)malloc((model.nExtraCostAreas + 1) * sizeof(strExtraNoGo));
	}

	fil.close();

	if (params->startBas_lon < -360 || params->startBas_lon > 360 || params->startBas_lat < -90 || params->startBas_lat > 90) {
		postRequest("ERROR! startCoord not given correctly in " + std::string(namn) + ". Fix it and run OptiNav - autoRoute again.\n", 1);
	}
	if (params->endBas_lon < -360 || params->endBas_lon > 360 || params->endBas_lat < -90 || params->endBas_lat > 90) {
		postRequest("ERROR! endCoord not given correctly in " + std::string(namn) + ". Fix it and run OptiNav - autoRoute again.\n", 1);
	}
	return 0;
}

int loadFileParams_autoRoute(strParamsAutoRoute* params)
{ // not used...
	int i, closestI, pos2, minLon, maxLon;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux, minLat;


	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	sprintf(namn, "%s/file_paramsAutoRoute.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist but given as input data to OptiNav-autoRoute.I quit\n", 1);
	}
	printf("opens %s\n", namn);
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

	//if (!data["eca_penalty"].is_null())
	//	params->eca_penalty = data["eca_penalty"];
	//else {
	//	params->eca_penalty = 2.0;
	//}
	//if (params->usePenalty_ECA == 0)
	//	params->eca_penalty = 0.0;

	if (!data["altRouteZones"].is_null())
		params->zonesFileName = data["altRouteZones"];
	else {
		postRequest("ERROR! No altRouteZones in input file, no alternative routes will be used.", 0);
		params->zonesFileName = "";
	}
	if (!data["zoneConnections"].is_null())
		params->zoneConnectionsFileName = data["zoneConnections"];
	else {
		postRequest("ERROR! No zoneConnections in input file, no alternative routes will be used.", 0);
		params->zoneConnectionsFileName = "";
	}

	if (!data["searoutePathFile"].is_null())
		params->searoutePathsName = data["searoutePathFile"];
	else {
		postRequest("ERROR! No searoutePaths, they must exist when running autoRoute. I quit!", 1);
	}

	if (!data["mapAutoRoutePhysicalBFileName"].is_null()) {
		params->mapAutoRoutePhysicalBFileName = data["mapAutoRoutePhysicalBFileName"];
	}
	else {
		postRequest("ERROR! No field mapAutoRoutePhysicalBFileName in the autoRoute input file. It must exists. I quit!", 1);
	}
	if (!data["mapAutoRoutePhysicalAFileName"].is_null()) {
		params->mapAutoRoutePhysicalAFileName = data["mapAutoRoutePhysicalAFileName"];
	}
	else {
		postRequest("ERROR! No field mapAutoRoutePhysicalAFileName in the autoRoute input file. It must exists. I quit!", 1);
	}
	if (!data["autoRoute_tss"].is_null()) {
		params->tssName = data["autoRoute_tss"];
	}
	else {
		postRequest("ERROR! No field autoRoute_tss in the autoRoute input file. I use no TSS.", 0);
		params->tssName = "-";
	}
	if (!data["autoRoute_corridors"].is_null()) {
		params->corridorsName = data["autoRoute_corridors"];
	}
	else {
		postRequest("ERROR! No field autoRoute_corridors in the autoRoute input file. I use no corridors.", 0);
		params->corridorsName = "-";
	}

	//if (!data["usePenalty_ECA"].is_null()) {
	//	params->usePenalty_ECA = data["usePenalty_ECA"];
	//	if (params->usePenalty_ECA < 0 || params->usePenalty_ECA > 1) {
	//		postRequest("ERROR! Wrong value of the field usePenalty_ECA. It must be 0 or 1 but is " + std::to_string(params->usePenalty_ECA) + ".I use 1.", 0);
	//		params->usePenalty_ECA = 1;
	//	}
	//}
	//else {
	//	params->usePenalty_ECA = 1;
	//}

	model.paramsAutoRoute.minLat_lonIndex = (double*)malloc(360 * sizeof(double));
	for (i1 = 0; i1 < 360; i1++)
		model.paramsAutoRoute.minLat_lonIndex[0] = -90;
	if (!data["limitSouth"].is_null()) {
		json dataExtra = data["limitSouth"];
		pos = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			minLat = dataNu["minLat"];
			minLon = roundDown(dataNu["minLon"]) + 180;
			maxLon = roundDown(dataNu["maxLon"]) + 180;
			if (minLon < 0)
				minLon = 0;
			if (maxLon > 359)
				maxLon = 359;
			for (i1 = minLon; i1 <= maxLon; i1++) {
				model.paramsAutoRoute.minLat_lonIndex[i1] = minLat;
			}
			pos++;
		}
	}

	std::string namnString;
	if (!data["optionalExtraNoGoAreas"].is_null()) {
		json dataExtra = data["optionalExtraNoGoAreas"];
		model.nExtraNoGoAreasBase = dataExtra.size();
		model.extraNoGoAreaBase = (strExtraNoGoBase*)malloc(model.nExtraNoGoAreasBase * sizeof(strExtraNoGoBase));
		pos = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			namnString = dataNu["noGoAreaID"];
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

int loadFileParams_feasibilityAuto(strParamsAutoRoute* params)
{
	int i, closestI, pos2, minLon, maxLon;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux, minLat;

	params->nCellLevels = 2;
	params->discretizationSizeLevel = (double*)malloc(params->nCellLevels * sizeof(double));
	params->nDiscreteSizeLevel = (int*)malloc(params->nCellLevels * sizeof(int));
	params->discretizationSizeLevel[0] = 2.0; //  1.5; // 3.0; // degrees
	params->nDiscreteSizeLevel[1] = 15; // km 12;
	params->discretizationSizeLevel[1] = params->discretizationSizeLevel[0] / params->nDiscreteSizeLevel[1]; // degrees
	params->factorExtraCover = 6;// 10; //  2;
	params->maxBaseFeasibleCost = 1000;
	params->mapAutoRoutePhysicalAFileName = std::string();
	params->mapAutoRoutePhysicalBFileName = std::string();
	params->tssName = std::string();
	params->zonesFileName = std::string();
	params->zoneConnectionsFileName = std::string();

	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	sprintf(namn, "%s/file_paramsFeasibility.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist.I quit\n", 1);
	}
	printf("opens %s\n", namn);
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

	//if (!data["eca_penalty"].is_null())
	//	params->eca_penalty = data["eca_penalty"];
	//else {
	//	params->eca_penalty = 2.0;
	//}
	//if (params->usePenalty_ECA == 0)
	//	params->eca_penalty = 0.0;

	if (!data["mapPhysicalBFileName"].is_null()) {
		model.params.mapPhysicalBFileName = data["mapPhysicalBFileName"];
	}
	if (!data["mapPhysicalAFileName"].is_null()) {
		model.params.mapPhysicalAFileName = data["mapPhysicalAFileName"];
	}

	if (!data["mapLandSeaBFileName"].is_null()) {
		model.params.mapLandSeaBFileName = data["mapLandSeaBFileName"];
	}
	if (!data["mapLandSeaAFileName"].is_null()) {
		model.params.mapLandSeaAFileName = data["mapLandSeaAFileName"];
	}

	//if (!data["physicalMap_noDataValue"].is_null())
	//	model.params.physicalMap_noDataValue = data["physicalMap_noDataValue"];


	if (!data["altRouteZones"].is_null())
		params->zonesFileName = data["altRouteZones"];
	else {
		postRequest("ERROR! No altRouteZones in file_paramsFeasibility.json, no alternative routes will be used.", 0);
		params->zonesFileName = "";
	}
	if (!data["zoneConnections"].is_null())
		params->zoneConnectionsFileName = data["zoneConnections"];
	else {
		postRequest("ERROR! No zoneConnections in file_paramsFeasibility.json, no alternative routes will be used.", 0);
		params->zoneConnectionsFileName = "";
	}

	if (!data["searoutePathFile"].is_null())
		params->searoutePathsName = data["searoutePathFile"];
	else {
		postRequest("ERROR! No searoutePaths, they must exist when running autoRoute. I quit!", 1);
	}

	if (!data["mapAutoRoutePhysicalBFileName"].is_null()) {
		params->mapAutoRoutePhysicalBFileName = data["mapAutoRoutePhysicalBFileName"];
	}
	else {
		postRequest("ERROR! No field mapAutoRoutePhysicalBFileName in file_paramsFeasibility.json. It must exists. I quit!", 1);
	}
	if (!data["mapAutoRoutePhysicalAFileName"].is_null()) {
		params->mapAutoRoutePhysicalAFileName = data["mapAutoRoutePhysicalAFileName"];
	}
	else {
		postRequest("ERROR! No field mapAutoRoutePhysicalAFileName in file_paramsFeasibility.json. It must exists. I quit!", 1);
	}
	if (!data["tss"].is_null()) {
		params->tssName = data["tss"];
	}
	else {
		postRequest("ERROR! No field tss file_paramsFeasibility.json. I use no TSS.", 0);
		params->tssName = "-";
	}
	if (!data["autoRoute_corridors"].is_null()) {
		params->corridorsName = data["autoRoute_corridors"];
	}
	else {
		postRequest("ERROR! No field autoRoute_corridors in file_paramsFeasibility.json. I use no corridors.", 0);
		params->corridorsName = "-";
	}

	//if (!data["usePenalty_ECA"].is_null()) {
	//	params->usePenalty_ECA = data["usePenalty_ECA"];
	//	if (params->usePenalty_ECA < 0 || params->usePenalty_ECA > 1) {
	//		postRequest("ERROR! Wrong value of the field usePenalty_ECA. It must be 0 or 1 but is " + std::to_string(params->usePenalty_ECA) + ".I use 1.", 0);
	//		params->usePenalty_ECA = 1;
	//	}
	//}
	//else {
	//	params->usePenalty_ECA = 1;
	//}

	model.paramsAutoRoute.minLat_lonIndex = (double*)malloc(360 * sizeof(double));
	for (i1 = 0; i1 < 360; i1++)
		model.paramsAutoRoute.minLat_lonIndex[0] = -90;
	if (!data["limitSouth"].is_null()) {
		json dataExtra = data["limitSouth"];
		pos = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			minLat = dataNu["minLat"];
			minLon = roundDown(dataNu["minLon"]) + 180;
			maxLon = roundDown(dataNu["maxLon"]) + 180;
			if (minLon < 0)
				minLon = 0;
			if (maxLon > 359)
				maxLon = 359;
			for (i1 = minLon; i1 <= maxLon; i1++) {
				model.paramsAutoRoute.minLat_lonIndex[i1] = minLat;
			}
			pos++;
		}
	}

	if (!data["extraCostFactorNoGoSeaRoutes"].is_null()) {
		modelSea.seaRoute.costFactorExtraNoGo = data["extraCostFactorNoGoSeaRoutes"];
	}
	else
		modelSea.seaRoute.costFactorExtraNoGo = 20.0;

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
			if (!dataNu["corridors"].is_null()) {
				int pos1, posUse, nCoords;
				json dataGeom, dataLine, dataPaths;
				
				dataPaths = dataNu["corridors"];
				model.extraNoGoAreaBase[pos].nCorridors = dataPaths.size();
				model.extraNoGoAreaBase[pos].corridor = (strSeaRoutePath*)malloc((model.extraNoGoAreaBase[pos].nCorridors) * sizeof(strSeaRoutePath));
				pos1 = 0;
				posUse = 0;
				for (auto it2 = dataPaths.begin(); it2 != dataPaths.end(); ++it2) {
					json dataNu2 = it2.value();
					if (dataNu2["geometry"].is_null()) {
						errlog("ERROR! No geometry for corridor %d in optionaExtraAreas %s\n", pos1, namnString.c_str());
						pos1++;
						continue;
					}
					if (!dataNu2["properties"].is_null()) {
						dataProp = dataNu2["properties"];
						if (!dataProp["costFactor"].is_null())
							model.extraNoGoAreaBase[pos].corridor[pos1].costFactorDist = dataProp["costFactor"];
						else
							model.extraNoGoAreaBase[pos].corridor[pos1].costFactorDist = 1.0;
						if (!dataProp["maxDistConnectInside_nm"].is_null()) {
							model.extraNoGoAreaBase[pos].corridor[pos1].maxDistConnect_km = dataProp["maxDistConnectInside_nm"];
							model.extraNoGoAreaBase[pos].corridor[pos1].maxDistConnect_km *= 1.852;
						}
						else
							model.extraNoGoAreaBase[pos].corridor[pos1].maxDistConnect_km = 120 * 1.852;
					}
					else {
						model.extraNoGoAreaBase[pos].corridor[pos1].costFactorDist = 1.0;
						model.extraNoGoAreaBase[pos].corridor[pos1].maxDistConnect_km = 120 * 1.852;
					}
					dataGeom = dataNu2["geometry"];
					if (dataGeom["coordinates"].is_null()) {
						errlog("ERROR! No coordinates in geometry for corridor %d in optionaExtraAreas %s\n", pos1, namnString.c_str());
						pos1++;
						continue;
					}
					dataLine = dataGeom["coordinates"];

					nCoords = dataLine.size();
					model.extraNoGoAreaBase[pos].corridor[pos1].xCoord = (double*)malloc(nCoords * sizeof(double));
					model.extraNoGoAreaBase[pos].corridor[pos1].yCoord = (double*)malloc(nCoords * sizeof(double));
					nCoords = 0;
					for (auto it4 = dataLine.begin(); it4 != dataLine.end(); ++it4) {
						dataIt2 = it4.value();
						i2 = 0;
						for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
							if (i2 == 0) {
								model.extraNoGoAreaBase[pos].corridor[posUse].xCoord[nCoords] = it3.value();
							}
							else if (i2 == 1)
								model.extraNoGoAreaBase[pos].corridor[posUse].yCoord[nCoords] = it3.value();
							i2++;
						}
						nCoords++;
					}
					model.extraNoGoAreaBase[pos].corridor[posUse].nCoords = nCoords;
					posUse++;
					pos1++;
				}
				model.extraNoGoAreaBase[pos].nCorridors = posUse;
			}else
				model.extraNoGoAreaBase[pos].nCorridors = 0;

			pos++;
		}
	}


	fil.close();

	return 0;
}

int loadStartEndPairsSeaRoutes(std::string inputPath)
{
	int i, closestI, pos2, minLon, maxLon;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux, minLat;


	std::ifstream fil;


	errlog("trying to open %s\n", inputPath.c_str());
	fil.open(inputPath.c_str());

	std::string namnStr;
	json data;
	json dataIt, dataIt2;
	int i2, nCoords;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + inputPath + " is not valid.Fix it and run OptiNav-seaRoutePaths again.", 1);
	}

	model.seaRoute.nNewPathPairs = data.size();
	model.seaRoute.pair = (strPair*)malloc(model.seaRoute.nNewPathPairs * sizeof(strPair));

	int nr = 0;
	for (auto it = data.begin(); it != data.end(); ++it) {
		json dataNu = it.value();
		namnStr = dataNu["groupID"];
		model.seaRoute.pair[nr].groupID = str_alloc_cpyString(namnStr);
		dataIt = dataNu["fromCoords"];
		model.seaRoute.pair[nr].nFrom = dataIt.size();
		model.seaRoute.pair[nr].xFrom = (double*)malloc(model.seaRoute.pair[nr].nFrom * sizeof(double));
		model.seaRoute.pair[nr].yFrom = (double*)malloc(model.seaRoute.pair[nr].nFrom * sizeof(double));

		nCoords = 0;
		for (auto it2 = dataIt.begin(); it2 != dataIt.end(); ++it2) {
			dataIt2 = it2.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0) {
					model.seaRoute.pair[nr].xFrom[nCoords] = it3.value();
				}
				else if (i2 == 1)
					model.seaRoute.pair[nr].yFrom[nCoords] = it3.value();
				i2++;
			}
			nCoords++;
		}

		dataIt = dataNu["toCoords"];
		model.seaRoute.pair[nr].nTo = dataIt.size();
		model.seaRoute.pair[nr].xTo = (double*)malloc(model.seaRoute.pair[nr].nTo * sizeof(double));
		model.seaRoute.pair[nr].yTo = (double*)malloc(model.seaRoute.pair[nr].nTo * sizeof(double));

		nCoords = 0;
		for (auto it2 = dataIt.begin(); it2 != dataIt.end(); ++it2) {
			dataIt2 = it2.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0) {
					model.seaRoute.pair[nr].xTo[nCoords] = it3.value();
				}
				else if (i2 == 1)
					model.seaRoute.pair[nr].yTo[nCoords] = it3.value();
				i2++;
			}
			nCoords++;
		}
		nr++;
	}

	fil.close();

	return 0;
}

void getCoordOnTss(strTss* path, int pos, double kvot, double* y, double* x) {
	*y = path->yCoord[pos] * kvot + path->yCoord[pos - 1] * (1 - kvot);
	*x = path->xCoord[pos] * kvot + path->xCoord[pos - 1] * (1 - kvot);
}

void get_xy_fromModelSeaBVArcsPos(int i, double* y1, double* x1) {
	int arcNr, nod;
	if (i <= 0) {
		if (i == -1) {
			*x1 = model.paramsAutoRoute.startPoint_lon[0];
			*y1 = model.paramsAutoRoute.startPoint_lat[0];
		}
		else {
			arcNr = modelSea.BVArcUse[i];
			nod = modelSea.arc[arcNr].fromPointNr;
			*y1 = modelSea.seaRoute.nod_y[nod];
			*x1 = modelSea.seaRoute.nod_x[nod];
		}
	}
	else {
		if (i < modelSea.nBVArcsUse) {
			arcNr = modelSea.BVArcUse[i];
			nod = modelSea.arc[arcNr].toPointNr;
			*y1 = modelSea.seaRoute.nod_y[nod];
			*x1 = modelSea.seaRoute.nod_x[nod];
		}
		else {
			*x1 = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
			*y1 = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
		}
	}
	if (*x1 < model.paramsAutoRoute.x_min - 20)
		*x1 += 360;
	else {
		if (*x1 > model.boundingBox.xMax + 20)
			*x1 -= 360;
	}
}

double getDist_flat(double x1, double y1, double x2, double y2) {
	if (x1 > x2 + 180)
		x2 += 360;
	else {
		if (x1 < x2 - 180)
			x2 -= 360;
	}
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

int getClosestPathPos(int pos1, int pos2, double y, double x, double* kvot) {
	int i, min_i, arcNr, nod;
	double minDist = 1e10, dist, x1, y1;
	double segmentLength, dist1, dist2, factor1, factor2, xTmp, yTmp, xTmp2, yTmp2, y0, x0, x2, y2;

	for (i = pos1 - 1; i <= pos2; i++) {
		get_xy_fromModelSeaBVArcsPos(i, &y1, &x1);
		dist = estimateLargeCircleDistance2_km(y, x, y1, x1);
		if (dist < minDist) {
			minDist = dist;
			min_i = i;
		}
	}

	get_xy_fromModelSeaBVArcsPos(min_i, &y1, &x1);
	minDist = getDist_flat(x, y, x1, y1);

	get_xy_fromModelSeaBVArcsPos(min_i, &y1, &x1);
	dist1 = 1e10;
	if (min_i > -1) {
		get_xy_fromModelSeaBVArcsPos(min_i - 1, &y0, &x0);
		segmentLength = getDist_flat(x0, y0, x1, y1);//  (estimateLargeCircleDistance_km(x0, y0, x1, y1);
		factor1 = ((x - x0) * (x1 - x0) + (y - y0) * (y1 - y0)) / (segmentLength * segmentLength);
		if (factor1 > 0 && factor1 < 1) {
			xTmp = x0 + factor1 * (x1 - x0);
			yTmp = y0 + factor1 * (y1 - y0);
			//dist1 = estimateLargeCircleDistance_km(x, y, xTmp, yTmp);
			dist1 = getDist_flat(x, y, xTmp, yTmp);
		}
	}
	dist2 = 1e10;
	if (min_i < modelSea.nBVArcs) {
		get_xy_fromModelSeaBVArcsPos(min_i + 1, &y2, &x2);
		segmentLength = getDist_flat(x1, y1, x2, y2);//estimateLargeCircleDistance_km(x1, y1, x2, y2);
		factor2 = ((x - x1) * (x2 - x1) + (y - y1) * (y2 - y1)) / (segmentLength * segmentLength);
		if (factor2 > 0 && factor2 < 1) {
			xTmp2 = x1 + factor2 * (x2 - x1);
			yTmp2 = y1 + factor2 * (y2 - y1);
			//dist2 = estimateLargeCircleDistance_km(y, x, yTmp2, xTmp2);
			dist2 = getDist_flat(x, y, xTmp2, yTmp2);
		}
	}
	if (dist1 < dist2) {
		if (dist1 < minDist) {
			*kvot = factor1;
			return min_i - 1;
		}
		else {
			*kvot = 0;
			return min_i;
		}
	}
	else {
		if (dist2 < minDist) {
			*kvot = factor2;
		}
		else {
			*kvot = 0;
		}
		return min_i;
	}
}

int checkIfPathInUsedCells(strTss* path, int dirAlt) {
	int i, traff, xPos, yPos, pos, i0, firstTraff = -1, lastTraff = -1, firstTraffPos, lastTraffPos;
	double yPosDbl, xPosDbl, xNu, yNu, dx, dy, kvot, next_xKvot, next_yKvot, yPosDbl_prev, xPosDbl_prev;
	double firstTraffKvot, lastTraffKvot, firstTraffPos_endKvot, lastTraffPos_startKvot;
	double tss_y1, tss_x1, tss_y2, tss_x2, pathKvotStart, pathKvotEnd;
	int firstPathPos, lastPathPos, closestPathPosStart, closestPathPosEnd, lastPos;

	traff = 0;
	lastPos = -1;

	int fix_x = 0;
	if (path->xCoord[0] < model.paramsAutoRoute.x_min - 20 && path->xCoord[path->nCoords - 1] < model.paramsAutoRoute.x_min - 20)
		fix_x = 360;
	if (path->xCoord[0] > model.boundingBox.xMax + 20 && path->xCoord[path->nCoords - 1] > model.boundingBox.xMax + 20)
		fix_x = -360;

	if (fix_x != 0) {
		for (i0 = 0; i0 < path->nCoords; i0++) {
			path->xCoord[i0] += fix_x;
		}
	}

	for (i0 = 0; i0 < path->nCoords; i0++) {
		if (i0 == 5)
			i0 = i0;
		if (i0 == 0 && (abs(path->xCoord[i0] + 9.966667) < 0.0001))
			i0 = i0;
		yPosDbl = (path->yCoord[i0] - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		xPosDbl = (path->xCoord[i0] - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		if (i0 > 0) {
			if ((yPosDbl < 0 && yPosDbl_prev < 0) || (xPosDbl < 0 && xPosDbl_prev < 0) ||
				(yPosDbl > model.paramsAutoRoute.nYbasLevel) && (yPosDbl_prev > model.paramsAutoRoute.nYbasLevel) ||
				(xPosDbl > model.paramsAutoRoute.nXbasLevel) && (xPosDbl_prev > model.paramsAutoRoute.nXbasLevel)) {
				// go to next point as both these are outside the interesting area
			}
			else {
				// kolla om en neagtiv o en i ok omrade sa fortsatt for den kommer in i tillatet omrade. Kolla kvotBerakningen;
				dx = xPosDbl - xPosDbl_prev;
				dy = yPosDbl - yPosDbl_prev;

				xNu = xPosDbl_prev;
				yNu = yPosDbl_prev;
				kvot = 0;
				for (i = 0;; i++) {
					xPos = (int)xNu;
					yPos = (int)yNu;
					if (xPos >= 0 && yPos >= 0 && xPos < model.paramsAutoRoute.nXbasLevel &&
						yPos < model.paramsAutoRoute.nYbasLevel) {
						pos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
						if (model.autoRoute[pos].use == 1) {
							if (firstTraff == -1) {
								firstTraff = i0;
								firstTraffPos = pos;
								firstTraffKvot = kvot;
							}
							if (firstTraffPos == pos)
								firstTraffPos_endKvot = kvot;
							lastTraff = i0;
							lastTraffKvot = kvot;
							lastTraffPos = pos;
							if (lastPos != pos)
								lastTraffPos_startKvot = kvot;
						}
						lastPos = pos;
					}
					if (kvot > 0.9995)
						break;

					if (xNu < 0 || xNu >= model.paramsAutoRoute.nXbasLevel)
						xNu = xNu;
					next_xKvot = get_nextKvotHeltal(xNu, xPosDbl_prev, dx);
					if (yNu < 0 || yNu >= model.paramsAutoRoute.nYbasLevel)
						yNu = yNu;
					next_yKvot = get_nextKvotHeltal(yNu, yPosDbl_prev, dy);
					if (next_xKvot < next_yKvot)
						kvot = next_xKvot;
					else
						kvot = next_yKvot;
					kvot += 0.001;
					if (kvot > 1)
						kvot = 1;
					xNu = xPosDbl_prev + kvot * dx;
					yNu = yPosDbl_prev + kvot * dy;
				}
			}
		}
		yPosDbl_prev = yPosDbl;
		xPosDbl_prev = xPosDbl;

	}
	if (firstTraff >= 0) {
		//if (firstTraffPos != lastTraffPos && (firstTraffPos_endKvot < lastTraffPos_startKvot + 0.01 || firstTraff < lastTraff)) {
		//	// can look further in on the tss as it is longer
		//	getCoordOnTss(nTss, firstTraff, firstTraffPos_endKvot, &tss_y1, &tss_x1);
		//	getCoordOnTss(nTss, lastTraff, lastTraffPos_startKvot, &tss_y2, &tss_x2);
		//}
		//else {
			getCoordOnTss(path, firstTraff, firstTraffKvot, &tss_y1, &tss_x1);
			getCoordOnTss(path, lastTraff, lastTraffKvot, &tss_y2, &tss_x2);
		//}
		firstPathPos = model.autoRoute[firstTraffPos].firstUsePathPos;
		lastPathPos = model.autoRoute[firstTraffPos].lastUsePathPos;
		closestPathPosStart = getClosestPathPos(firstPathPos, lastPathPos, tss_y1, tss_x1, &pathKvotStart);
		firstPathPos = model.autoRoute[lastTraffPos].firstUsePathPos;
		lastPathPos = model.autoRoute[lastTraffPos].lastUsePathPos;
		closestPathPosEnd = getClosestPathPos(firstPathPos, lastPathPos, tss_y2, tss_x2, &pathKvotEnd);
		if (closestPathPosStart < closestPathPosEnd || (closestPathPosStart == closestPathPosEnd && pathKvotStart < pathKvotEnd)) {
			path->firstTraffCoord = firstTraff;
			path->lastTraffCoord = lastTraff;
			path->firstTraffCoordKvot = firstTraffKvot;
			path->lastTraffCoordKvot = lastTraffKvot;
			path->firstTraffPos = firstTraffPos;
			path->lastTraffPos = lastTraffPos;
			return 1;
		}
		else {
			if (dirAlt == 1 && (closestPathPosStart > closestPathPosEnd || (closestPathPosStart == closestPathPosEnd && pathKvotStart > pathKvotEnd))) {
				path->firstTraffCoord = path->nCoords - lastTraff;
				path->lastTraffCoord = path->nCoords - firstTraff;
				path->firstTraffCoordKvot = 1 - lastTraffKvot;
				path->lastTraffCoordKvot = 1 - firstTraffKvot;
				path->firstTraffPos = lastTraffPos;
				path->lastTraffPos = firstTraffPos;
				return 2;
			}
			else
				return 0;
		}
	}
	else
		return 0;
}

int load_tss()
{
	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	double kvotCost, default_kvotCost = 0.01;
	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.tssName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist but given in input data as the tss to load in OptiNav-autoRoute.I continue without tss\n", 0);
		model.nTss = 0;
		return 0;
	}
	printf("opens %s\n", namn);
	fil.open(namn);

	int nTss, pos2, useTss;
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

	nAlloc = data2.size();
	model.tss = (strTss*)malloc(nAlloc * sizeof(strTss));
	pos = 0;
	nTss = 0;
	for (auto it = data2.begin(); it != data2.end(); ++it) {
		pos++;
		json dataNu = it.value();
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

		model.tss[nTss].nCoords = coords.size();
		model.tss[nTss].xCoord = (double*)malloc(model.tss[nTss].nCoords * sizeof(double));
		model.tss[nTss].yCoord = (double*)malloc(model.tss[nTss].nCoords * sizeof(double));
		pos2 = 0;
		for (auto it2 = coords.begin(); it2 != coords.end(); ++it2) {
			dataIt2 = it2.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0)
					model.tss[nTss].xCoord[pos2] = it3.value();
				else
					model.tss[nTss].yCoord[pos2] = it3.value();
				i2++;
			}
			pos2++;
		}
		model.tss[nTss].nCoords = pos2;

		if (pos == 4)
			pos = pos;
		if (abs(model.tss[nTss].xCoord[0] - 11.3) < 0.1 && abs(model.tss[nTss].yCoord[0] - 37.35) < 0.1)
			pos = pos;
		useTss = checkIfPathInUsedCells(&(model.tss[nTss]), 0);
		if (useTss != 0)
			pos = pos;
		if (useTss == 1) {
			if (!(dataNu["properties"].is_null())) {
				prop = dataNu["properties"];
				if (!(prop["kvotCost"].is_null()))
					kvotCost = prop["kvotCost"];
				else
					kvotCost = default_kvotCost;
			}
			model.tss[nTss].kvotCost = kvotCost;
			nTss++;
		}
		else {
			free(model.tss[nTss].xCoord);
			free(model.tss[nTss].yCoord);
		}
	}
	model.nTss = nTss;
	free(namn);

	return 0;
}


int autoCorridor_create_oppositeDirection(int nAutoCorridors) {

	int pos, nCoords = model.autoCorridors[nAutoCorridors].nCoords;
	double* x, * y;
	x = (double*)malloc(nCoords * sizeof(double));
	y = (double*)malloc(nCoords * sizeof(double));
	pos = nCoords - 1;
	for (int i = 0; i < nCoords; i++) {
		x[i] = model.autoCorridors[nAutoCorridors].xCoord[pos];
		y[i] = model.autoCorridors[nAutoCorridors].yCoord[pos];
		pos--;
	}
	for (int i = 0; i < nCoords; i++) {
		model.autoCorridors[nAutoCorridors].xCoord[i] = x[i];
		model.autoCorridors[nAutoCorridors].yCoord[i] = y[i];
		pos--;
	}
	free(x);
	free(y);
	return 0;
}

int load_autoCorridors()
{
	std::ifstream fil;
	char* namn;
	std::string namnStr;
	namn = (char*)malloc2(256 * sizeof(char));
	double kvotCost, default_kvotCost = 0.5;
	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.corridorsName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist but given in input data as the corridors to load in OptiNav-autoRoute.I continue without corridors\n", 0);
		model.nAutoCorridors = 0;
		return 0;
	}
	printf("opens %s\n", namn);
	fil.open(namn);

	int nAutoCorridors, pos2, useCorridor;
	json data, geom, coords, dataIt2, prop;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu, pos, oneWay;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + std::string(namn) + " is not valid. I continue without corridors.", 0);
		fil.close();
		model.nAutoCorridors = 0;
		return 0;
	}
	fil.close();

	if (data["features"].is_null()) {
		postRequest("ERROR! no features in corridors file. I continue without corridors.", 0);
		model.nAutoCorridors = 0;
		return 0;
	}
	json data2 = data["features"];

	nAlloc = 2 * data2.size();
	model.autoCorridors = (strTss*)malloc(nAlloc * sizeof(strTss));
	pos = 0;
	nAutoCorridors = 0;
	for (auto it = data2.begin(); it != data2.end(); ++it) {
		pos++;
		json dataNu = it.value();
		if (dataNu["geometry"].is_null()) {
			errlog("ERROR! corridor %d do not have a geometry. I skip this one\n", pos);
			continue;
		}
		geom = dataNu["geometry"];
		if (geom["coordinates"].is_null()) {
			errlog("ERROR! corridor %d has geometry but no coordinates. I skip this one\n", pos);
			continue;
		}
		coords = geom["coordinates"];

		model.autoCorridors[nAutoCorridors].nCoords = coords.size();
		model.autoCorridors[nAutoCorridors].xCoord = (double*)malloc(model.autoCorridors[nAutoCorridors].nCoords * sizeof(double));
		model.autoCorridors[nAutoCorridors].yCoord = (double*)malloc(model.autoCorridors[nAutoCorridors].nCoords * sizeof(double));
		pos2 = 0;
		for (auto it2 = coords.begin(); it2 != coords.end(); ++it2) {
			dataIt2 = it2.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0)
					model.autoCorridors[nAutoCorridors].xCoord[pos2] = it3.value();
				else
					model.autoCorridors[nAutoCorridors].yCoord[pos2] = it3.value();
				i2++;
			}
			pos2++;
		}
		model.autoCorridors[nAutoCorridors].nCoords = pos2;

		if (pos == 6)
			pos = pos;
		if (abs(model.autoCorridors[nAutoCorridors].xCoord[0] - 11.3) < 0.1 && abs(model.autoCorridors[nAutoCorridors].yCoord[0] - 37.35) < 0.1)
			pos = pos;
		useCorridor = checkIfPathInUsedCells(&(model.autoCorridors[nAutoCorridors]), 1);

		if (useCorridor > 0) { // 1 forward, 2 backwards, 3 both - not used...
			oneWay = 0;
			if (!(dataNu["properties"].is_null())) {
				prop = dataNu["properties"];
				if (!(prop["kvotCost"].is_null()))
					kvotCost = prop["kvotCost"];
				else
					kvotCost = default_kvotCost;
				if (!(prop["oneWay"].is_null())) {
					if (prop["oneWay"] == "yes")
						oneWay = 1;
					else {
						if (prop["oneWay"] != "no") {
							namnStr = prop["oneWay"];
							errlog("ERROR! Corridor has oneWay = %s, must be 'yes' or 'no'\n", namnStr.c_str());
						}
					}
				}
			}
			else
				kvotCost = default_kvotCost;

			if (oneWay == 0 || useCorridor == 1) {
				if(useCorridor == 2)
					autoCorridor_create_oppositeDirection(nAutoCorridors);
				model.autoCorridors[nAutoCorridors].kvotCost = kvotCost;
				nAutoCorridors++;
			}
			else {
				free(model.autoCorridors[nAutoCorridors].xCoord);
				free(model.autoCorridors[nAutoCorridors].yCoord);
			}
		}
		else {
			free(model.autoCorridors[nAutoCorridors].xCoord);
			free(model.autoCorridors[nAutoCorridors].yCoord);
		}
	}
	model.nAutoCorridors = nAutoCorridors;
	free(namn);

	return 0;
}

double getCostFactorArea(double y1, double x1, double y2, double x2, int* xIndexReturn) {
	double factor = 1.0;
	int xIndex = roundDown(x1 + 180);
	if (xIndex < 0)
		xIndex += 360;
	if (xIndex > 359)
		xIndex -= 360;
	if (y1 < model.paramsAutoRoute.minLat_lonIndex[xIndex]) {
		factor *= 1.1 + (model.paramsAutoRoute.minLat_lonIndex[xIndex] - y1) * 0.02;
	}
	xIndex = roundDown(x2 + 180);
	if (xIndex < 0)
		xIndex += 360;
	if (xIndex > 359)
		xIndex -= 360;
	if (y2 < model.paramsAutoRoute.minLat_lonIndex[xIndex]) {
		if(factor > 1.01)
			factor *= 1.2 + (model.paramsAutoRoute.minLat_lonIndex[xIndex] - y2) * 0.02;
		else
			factor *= 1.1 + (model.paramsAutoRoute.minLat_lonIndex[xIndex] - y2) * 0.02;
		if (factor > 2)
			factor = 2.0;
	}
	*xIndexReturn = xIndex;

	return factor;
}


int addArcBetweenNodes(int nod1, int nod2, double dist) {
	int i, xIndex;
	double costFactorArea;
	if (nod1 == 273 && nod2 == 272)
		nod1 = nod1;
	for (i = 0; i < modelSea.Noder[nod1].nUtNoder; i++) {
		if (modelSea.Noder[nod1].UtNod[i] == nod2)
			break;
	}
	if (i < modelSea.Noder[nod1].nUtNoder) {
		if(abs(dist - modelSea.Noder[nod1].UtNodCost[i]) > 0.1)
			errlog("ERROR? Two arcs between the same nodes %d and %d yx %lf %lf %lf %lf dists %.2lf %.2lf, I use the shorter one\n", nod1, nod2,
				modelSea.seaRoute.nod_y[nod1], modelSea.seaRoute.nod_x[nod1], 
				modelSea.seaRoute.nod_y[nod2], modelSea.seaRoute.nod_x[nod2], dist, modelSea.Noder[nod1].UtNodCost[i]);
		if (dist < modelSea.Noder[nod1].UtNodCost[i])
			modelSea.Noder[nod1].UtNodCost[i] = dist;
		return modelSea.Noder[nod1].outArcNr[i];
	}
	else {
		if (i >= modelSea.Noder[nod1].nAllocUtNoder) {
			modelSea.Noder[nod1].nAllocUtNoder += 10;
			modelSea.Noder[nod1].UtNod = (int*)realloc(modelSea.Noder[nod1].UtNod, modelSea.Noder[nod1].nAllocUtNoder * sizeof(int));
			modelSea.Noder[nod1].UtNodCost = (double*)realloc(modelSea.Noder[nod1].UtNodCost, modelSea.Noder[nod1].nAllocUtNoder * sizeof(double));
			modelSea.Noder[nod1].outArcNr = (int*)realloc(modelSea.Noder[nod1].outArcNr, modelSea.Noder[nod1].nAllocUtNoder * sizeof(int));
		}
		modelSea.Noder[nod1].UtNod[i] = nod2;
		modelSea.Noder[nod1].UtNodCost[i] = dist;
		modelSea.Noder[nod1].outArcNr[i] = modelSea.nArcs;
		(modelSea.Noder[nod1].nUtNoder)++;
		if (modelSea.nArcs >= modelSea.nAllocArcs) {
			modelSea.nAllocArcs += 10000;
			modelSea.arc = (strArcInfo*)realloc(modelSea.arc, modelSea.nAllocArcs * sizeof(strArcInfo));
		}
		modelSea.arc[modelSea.nArcs].fromPointNr = nod1;
		modelSea.arc[modelSea.nArcs].toPointNr = nod2;
		modelSea.arc[modelSea.nArcs].corridorNr = -1;

		if (modelSea.nArcs == 15877 || modelSea.nArcs == 15875)
			nod2 = nod2;
		//costFactorArea = getCostFactorArea(modelSea.seaRoute.nod_y[nod1], modelSea.seaRoute.nod_x[nod1],
		//	modelSea.seaRoute.nod_y[nod2], modelSea.seaRoute.nod_x[nod2], &xIndex);
		//modelSea.arc[modelSea.nArcs].totCost = dist * costFactorArea;
		modelSea.arc[modelSea.nArcs].distance = dist;
		(modelSea.nArcs)++;
		return modelSea.nArcs - 1;
	}
}


double getDistSeaRoute(double y1, double x1, double y2, double x2) {
	if (x1 > 90 && x2 < -90)
		x2 += 360;
	else {
		if (x1 < -90 && x2 > 90)
			x1 += 360;
	}
	return sqrt((y1 - y2) * (y1 - y2) + (x1 - x2) * (x1 - x2));
}

int identify_nearestNode_toSearoutes(double y, double x, double* minDist) {
	int i, min_i = -1;
	double useDist = 1e10, dist;
	for (i = 0; i < modelSea.nNoder; i++) {
		dist = getDistSeaRoute(y, x, modelSea.seaRoute.nod_y[i], modelSea.seaRoute.nod_x[i]);
		if (dist < useDist) {
			useDist = dist;
			min_i = i;
		}
	}
	*minDist = useDist;
	return min_i;
}

int addSeaRouteNode(double y, double x) {
	int nodNr = modelSea.nNoder;
	if (nodNr >= modelSea.nAllocNoder) {
		modelSea.nAllocNoder += 10000;
		modelSea.Noder = (strNoder*)realloc(modelSea.Noder, modelSea.nAllocNoder * sizeof(strNoder));
		modelSea.seaRoute.nod_y = (double*)realloc(modelSea.seaRoute.nod_y, modelSea.nAllocNoder * sizeof(double));
		modelSea.seaRoute.nod_x = (double*)realloc(modelSea.seaRoute.nod_x, modelSea.nAllocNoder * sizeof(double));
	}
	if (x < -179.9999)
		x += 360;
	if (x > 180)
		x = 180;
	modelSea.seaRoute.nod_y[nodNr] = y;
	modelSea.seaRoute.nod_x[nodNr] = x;
	modelSea.Noder[nodNr].nUtNoder = 0;
	modelSea.Noder[nodNr].nAllocUtNoder = 10;
	modelSea.Noder[nodNr].UtNod = (int*)malloc(modelSea.Noder[nodNr].nAllocUtNoder * sizeof(int));
	modelSea.Noder[nodNr].UtNodCost = (double*)malloc(modelSea.Noder[nodNr].nAllocUtNoder * sizeof(double));
	modelSea.Noder[nodNr].outArcNr = (int*)malloc(modelSea.Noder[nodNr].nAllocUtNoder * sizeof(int));

	(modelSea.nNoder)++;

	return nodNr;
}

int get_nodFrom_coords(double y, double x) {
	int i;
	double dist;
	if (x < -179.9999)
		x += 360;
	if (x > 180)
		x = 180;
	for (i = 0; i < modelSea.nNoder; i++) {
		dist = getDistSeaRoute(y, x, modelSea.seaRoute.nod_y[i], modelSea.seaRoute.nod_x[i]);
		if (dist < 0.001) {
			if (dist >= 0.00001)
				errlog("dist %lf yx %lf %lf\n", dist, y, x);
			break;
		}
	}
	if (i >= modelSea.nNoder) {
		i = addSeaRouteNode(y, x);
	}
	return i;
}

double estDistCoordToBoundingBoxCorridorSeaRoute(double y, double x, strBoundBox bbox) {
	double dist;

	if (x < bbox.xMin) {
		if (y < bbox.yMin)
			dist = estimateLargeCircleDistance_km(y, x, bbox.yMin, bbox.xMin);
		else {
			if (y > bbox.yMax)
				dist = estimateLargeCircleDistance_km(y, x, bbox.yMax, bbox.xMin);
			else
				dist = estimateLargeCircleDistance_km(y, x, y, bbox.xMin);
		}
	}
	else {
		if (x > bbox.xMax) {
			if (y < bbox.yMin)
				dist = estimateLargeCircleDistance_km(y, x, bbox.yMin, bbox.xMax);
			else {
				if (y > bbox.yMax)
					dist = estimateLargeCircleDistance_km(y, x, bbox.yMax, bbox.xMax);
				else
					dist = estimateLargeCircleDistance_km(y, x, y, bbox.xMax);
			}
		}
		else {
			if (y < bbox.yMin)
				dist = estimateLargeCircleDistance_km(y, x, bbox.yMin, x);
			else {
				if (y > bbox.yMax)
					dist = estimateLargeCircleDistance_km(y, x, bbox.yMax, x);
				else
					dist = 0; // estimateLargeCircleDistance_km(y, x, y, x);
			}
		}
	}
	return dist;
}


int identifyMod_startEndOnCorridorSeaRoute(int cNr, int startEnd) {
	int i, minPos, posUse, pktNr, nod1, nod2;
	double minDist2 = 1e20, dist, x, y, distBastPrev2, distPrev2 = -1, distBastNext2 = -1, x0, x1, x2;
	double dist2, cosC1, cosC2, minPrev, minNext, dist1, distFromPoint, bearing, cosBefore = -10, cosAfter = -10;
	double distOld, deltaDist, timeOld, timeNew, estMinDist, distPath, minDist;
	spherical::Point p1;

	// if startpunkt within corridor and dist < x to corridor => cut the corridors length, start at a position after startpunkt
	if (startEnd == 0) {
		// only first set of start/end can be used for this
		x = model.paramsAutoRoute.startBas_lon;
		y = model.paramsAutoRoute.startBas_lat;
	}
	else {
		// only first set of start/end can be used for this
		x = model.paramsAutoRoute.endBas_lon;
		y = model.paramsAutoRoute.endBas_lat;
	}

	estMinDist = estDistCoordToBoundingBoxCorridorSeaRoute(y, x, model.paramsAutoRoute.corridors_noGoSoft[cNr].boundingBox);
	if (estMinDist >= model.paramsAutoRoute.corridors_noGoSoft[cNr].maxDistConnectInside)
		return 0; // too far to the start/end, no need to modify the corridor

	for (i = 0; i < model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter; i++) {
		x0 = model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[i];
		check_translate_xCoord(&x0);
		dist = estimateLargeCircleDistance2_km(y, x, model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[i], x0);
		if (minDist2 > dist) {
			minDist2 = dist;
			distBastPrev2 = distPrev2;
			minPos = i;
		}
		distPrev2 = dist;
		if (minPos == i - 1)
			distBastNext2 = dist;
	}
	minDist = sqrt(minDist2);

	//printf("ident_startEndOnChannel startEnd %d minDist %.2lf minPos %d distBastPrev %.2lf distBastNext %.2lf nPoints %d\n",
	//	startEnd, minDist, minPos, distBastPrev, distBastNext, model.network.channel[cNr].nPoints);


	if (minPos > 0) {
		x1 = model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[minPos - 1];
		check_translate_xCoord(&x1);
		x0 = model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[minPos];
		check_translate_xCoord(&x0);
		dist1 = estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[minPos - 1], x1,
			model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[minPos], x0);

		if (minDist * dist1 > 0.00001)
			cosBefore = (-distBastPrev2 + dist1 * dist1 + minDist2) / (2 * minDist * dist1);
		else
			cosBefore = (-distBastPrev2 + dist1 * dist1 + minDist2) / (2 * 0.00001);

		if (cosBefore > 1)
			cosBefore = 1;
		if (cosBefore < -1)
			cosBefore = -1;
		if (cosBefore > 0)
			minPrev = minDist * sin(acos(cosBefore));
		else {
			minPrev = minDist;
			if (minPos == model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter - 1)
				return 0; // connection after the corridor ends, keep the corridor as it is
		}
	}
	else {
		minPrev = 1e10;
	}
	if (minPos < model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter - 1) {
		x2 = model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[minPos + 1];
		check_translate_xCoord(&x2);
		x0 = model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[minPos];
		check_translate_xCoord(&x0);
		dist2 = estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[minPos + 1], x2,
			model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[minPos], x0);

		if (minDist * dist2 > 0.00001)
			cosAfter = (-distBastNext2 + dist2 * dist2 + minDist2) / (2 * minDist * dist2);
		else
			cosAfter = (-distBastNext2 + dist2 * dist2 + minDist2) / (2 * 0.00001);

		if (cosAfter > 1)
			cosAfter = 1;
		if (cosAfter < -1)
			cosAfter = -1;
		if (cosAfter > 0)
			minNext = minDist * sin(acos(cosAfter));
		else {
			if (minPos == 0)
				return 0; // connection before the corridor start, keep the corridor as it is
			minNext = minDist;
		}

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
		if (minDist < 0.01 && minPos == model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter - 1)
			return 0; // no need to do anything as it is the right node
	}

	if (minPrev <= minNext) {
		if (minPrev > model.paramsAutoRoute.corridors_noGoSoft[cNr].maxDistConnectInside) {
			errlog("OBS! Not shortening the corridor noGoSoft at startEnd %d, distPrev %.2lf km must be <= %.2lf\n",
				startEnd, minPrev, model.paramsAutoRoute.corridors_noGoSoft[cNr].maxDistConnectInside);
			return 0; // too far away from the channel
		}
		distFromPoint = dist1 - cosBefore * minDist;
		posUse = minPos - 1;
		if (cosBefore < 0) {
			p1 = model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse + 1];
			//model.network.channel[cNr].point[posUse + 1];
			distFromPoint = -1;
		}
	}
	else {
		if (minNext > model.paramsAutoRoute.corridors_noGoSoft[cNr].maxDistConnectInside) {
			errlog("OBS! Not shortening the corridor noGoSoftat startEnd %d, distNext %.2lf km must be <= %.2lf\n",
				startEnd, minNext, model.paramsAutoRoute.corridors_noGoSoft[cNr].maxDistConnectInside);
			return 0; // too far away from the channel
		}
		distFromPoint = cosAfter * minDist;
		posUse = minPos;
		if (cosAfter <= 0) {
			p1 = model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse];
			// model.network.channel[cNr].point[posUse];
		}
	}

	if (distFromPoint > 0) {
		bearing = model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse].bearingTo(model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse + 1]);
		p1 = model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse].destinationPoint(distFromPoint * 1000, bearing);
	}
	distOld = model.paramsAutoRoute.corridors_noGoSoft[cNr].distance_km;
	if (startEnd == 0)
		deltaDist = p1.distanceTo(model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse + 1]) / 1000.0 - model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[posUse + 1];
	else
		deltaDist = -distOld - p1.distanceTo(model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse + 1]) / 1000.0 + model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[posUse + 1];

	//printf("deltaDist %.2lf distFromStart_posUse+1 %.2lf p1_distTo_posUse+1 %.2lf posUse %d posUse+1_xy %.3lf %.3lf\n", deltaDist,
	//	model.network.channel[cNr].distanceFromStart[posUse + 1], p1.distanceTo(model.network.channel[cNr].point[posUse + 1]),
	//	posUse, model.network.channel[cNr].point[posUse + 1].longitude().degrees(), model.network.channel[cNr].point[posUse + 1].latitude().degrees());
	//timeOld = model.network.channel[cNr].timeThroughChannel;
	//if (timeOld < -0.1)
	//	timeNew = timeOld;
	//else
	//	timeNew = timeOld * (distOld + deltaDist) / distOld;

	if (startEnd == 0)
		errlog("update data for corridor noGoSoft. Before it started at %.3lf %.3lf. It is shortened and now starts at %.3lf %.3lf.\n",
			model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[0], model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[0],
			p1.longitude().degrees(), p1.latitude().degrees());
	else
		errlog("update data for corridor noGoSoft. Before it ended at %.3lf %.3lf. It is shortened and now ends at %.3lf %.3lf.\n",
			model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter - 1], 
			model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter - 1],
			p1.longitude().degrees(), p1.latitude().degrees());

	model.paramsAutoRoute.corridors_noGoSoft[cNr].distance_km = distOld + deltaDist;
	if (startEnd == 0) {
		model.paramsAutoRoute.corridors_noGoSoft[cNr].point[0] = p1;
		model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[0] = p1.longitude().degrees();
		model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[0] = p1.latitude().degrees();

		//printf("new Point lon/lat %.3lf %.3lf instead of pos %d %.3lf %.3lf\n", model.network.channel[cNr].point_x[0],
		//	model.network.channel[cNr].point_y[0], posUse, model.network.channel[cNr].point_x[posUse],
		//	model.network.channel[cNr].point_y[posUse]);

		for (i = posUse + 1; i < model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter; i++) {
			model.paramsAutoRoute.corridors_noGoSoft[cNr].point[i - posUse] = model.paramsAutoRoute.corridors_noGoSoft[cNr].point[i];
			model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[i - posUse] = model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[i];
			model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[i - posUse] = model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[i];
			if (i == posUse + 1)
				model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[i - posUse] =
				model.paramsAutoRoute.corridors_noGoSoft[cNr].point[0].distanceTo(model.paramsAutoRoute.corridors_noGoSoft[cNr].point[1]) / 1000.0;
			else
				model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[i - posUse] = model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[i - posUse - 1] +
				model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[i] - model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[i - 1];
		}
		model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter -= posUse;

		pktNr = 0;
		nod2 = addSeaRouteNode(model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[pktNr], model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[pktNr]);
		model.paramsAutoRoute.corridors_noGoSoft[cNr].startNod = nod2;

		nod1 = model.paramsAutoRoute.startNod;
		if (nod1 < 0) {
			nod1 = addSeaRouteNode(y, x);
			model.paramsAutoRoute.startNod = nod1;
		}

	}
	else {
		model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse + 1] = p1;
		model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[posUse + 1] = p1.longitude().degrees();
		model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[posUse + 1] = p1.latitude().degrees();

		errlog("new endPoint %d lon/lat %.3lf %.3lf\n", posUse + 1, model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[posUse + 1],
			model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[posUse + 1]);

		model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[posUse + 1] = model.paramsAutoRoute.corridors_noGoSoft[cNr].distanceFromStart[posUse] +
			model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse].distanceTo(model.paramsAutoRoute.corridors_noGoSoft[cNr].point[posUse + 1]) / 1000.0;

		model.paramsAutoRoute.corridors_noGoSoft[cNr].nPkter = posUse + 2;

		pktNr = posUse + 1;
		nod1 = addSeaRouteNode(model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[pktNr], model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[pktNr]);
		model.paramsAutoRoute.corridors_noGoSoft[cNr].endNod = nod1;

		nod2 = model.paramsAutoRoute.endNod;
		if (nod2 < 0) {
			nod2 = addSeaRouteNode(y, x);
			model.paramsAutoRoute.endNod = nod2;
		}

	}

	//if (modelSea.Noder[nod1].nUtNoder >= modelSea.Noder[nod1].nAllocUtNoder) {
	(modelSea.Noder[nod1].nAllocUtNoder)++;
	modelSea.Noder[nod1].UtNod = (int*)realloc(modelSea.Noder[nod1].UtNod, modelSea.Noder[nod1].nAllocUtNoder * sizeof(int));
	modelSea.Noder[nod1].UtNodCost = (double*)realloc(modelSea.Noder[nod1].UtNodCost, modelSea.Noder[nod1].nAllocUtNoder * sizeof(double));
	modelSea.Noder[nod1].outArcNr = (int*)realloc(modelSea.Noder[nod1].outArcNr, modelSea.Noder[nod1].nAllocUtNoder * sizeof(int));
	//}

	if (modelSea.nArcs >= modelSea.nAllocArcs) {
		modelSea.nAllocArcs += 100;
		modelSea.arc = (strArcInfo*)realloc(modelSea.arc, modelSea.nAllocArcs * sizeof(strArcInfo));
	}

	modelSea.arc[modelSea.nArcs].fromPointNr = nod1;
	modelSea.arc[modelSea.nArcs].toPointNr = nod2;
	modelSea.arc[modelSea.nArcs].extraAreaFactor = (double*)calloc(model.nExtraNoGoAreasBase, sizeof(double));
	modelSea.arc[modelSea.nArcs].corridorNr = -1;
	distPath = estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[cNr].yCoord[pktNr],
		model.paramsAutoRoute.corridors_noGoSoft[cNr].xCoord[pktNr], y, x);
	modelSea.arc[modelSea.nArcs].distance = distPath;
	modelSea.arc[modelSea.nArcs].totCost = distPath * model.paramsAutoRoute.corridors_noGoSoft[cNr].costFactorDist;

	(modelSea.nArcs)++;



	//printf("nPoints efter %d\n", model.network.channel[cNr].nPoints);
	//printf("pos %d xy %.3lf %.3lf\n", model.network.channel[cNr].nPoints - 1,
	//	model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 1],
	//	model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 1]);
	//printf("pos %d xy %.3lf %.3lf\n", model.network.channel[cNr].nPoints - 2,
	//	model.network.channel[cNr].point_x[model.network.channel[cNr].nPoints - 2],
	//	model.network.channel[cNr].point_y[model.network.channel[cNr].nPoints - 2]);


	return 1;
}


int load_searoutes()
{

	modelSea.nAllocNoder = 10000;
	modelSea.Noder = (strNoder*)malloc(modelSea.nAllocNoder * sizeof(strNoder));
	modelSea.seaRoute.nod_y = (double*)malloc(modelSea.nAllocNoder * sizeof(double));
	modelSea.seaRoute.nod_x = (double*)malloc(modelSea.nAllocNoder * sizeof(double));
	modelSea.nNoder = 0;
	modelSea.nAllocArcs = 10000;
	modelSea.arc = (strArcInfo*)malloc(modelSea.nAllocArcs * sizeof(strArcInfo));
	modelSea.nArcs = 0;

	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/autoRoute/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.searoutePathsName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		postRequest(std::string(namn) + " does not exist. To run autoRoute it has to.I quit\n", 1);
	}
	printf("opens %s\n", namn);
	fil.open(namn);

	json data, dataPaths, dataNu, dataLine, dataCoordinate, dataGeom, dataIt2;
	int nCoords, n2coords = 0, nMaxCoords = 0, i2, posUse = 0, pos = 0, nodNu, nodPrev;
	double dist, xNu;
	spherical::Point p1, p2;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav-autoRoute again.", 1);
	}

	if (!data["features"].is_null()) {
		json dataPaths = data["features"];
		model.seaRoute.nSeaRoutePaths = dataPaths.size();
		model.seaRoute.seaRoutePath = (strSeaRoutePath*)malloc((model.seaRoute.nSeaRoutePaths) * sizeof(strSeaRoutePath));
		pos = 0;
		posUse = 0;
		for (auto it = dataPaths.begin(); it != dataPaths.end(); ++it) {
			dataNu = it.value();
			if (dataNu["geometry"].is_null()) {
				errlog("ERROR! No geometry for pathNr %d in searoutes\n", pos);
				pos++;
				continue;
			}
			dataGeom = dataNu["geometry"];
			if (dataGeom["coordinates"].is_null()) {
				errlog("ERROR! No coordinates in geometry for pathNr %d in searoutes\n", pos);
				pos++;
				continue;
			}
			dataLine = dataGeom["coordinates"];

			nCoords = dataLine.size();
			model.seaRoute.seaRoutePath[posUse].xCoord = (double*)malloc(nCoords * sizeof(double));
			model.seaRoute.seaRoutePath[posUse].yCoord = (double*)malloc(nCoords * sizeof(double));
			if (nCoords == 2)
				n2coords++;
			else {
				if (nCoords > nMaxCoords)
					nMaxCoords = nCoords;
			}
			nCoords = 0;
			nodPrev = -2;
			for (auto it = dataLine.begin(); it != dataLine.end(); ++it) {
				dataIt2 = it.value();
				i2 = 0;
				for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
					if (i2 == 0) {
						model.seaRoute.seaRoutePath[posUse].xCoord[nCoords] = it3.value();
					}
					else if (i2 == 1)
						model.seaRoute.seaRoutePath[posUse].yCoord[nCoords] = it3.value();
					i2++;
				}
				nodNu = get_nodFrom_coords(model.seaRoute.seaRoutePath[posUse].yCoord[nCoords], model.seaRoute.seaRoutePath[posUse].xCoord[nCoords]);
				if (nodNu == nodPrev)
					continue; // same node then don't need to save it...
				xNu = model.seaRoute.seaRoutePath[posUse].xCoord[nCoords];
				if (pos == 131)
					pos = pos;
				if (xNu < -180)
					xNu += 360;
				if (xNu > 180)
					xNu -= 360;
				p2 = spherical::Point(model.seaRoute.seaRoutePath[posUse].yCoord[nCoords], fix_lonPos(xNu));
				nCoords++;
				if (nCoords >= 2) {
					dist = estimateLargeCircleDistance_km(modelSea.seaRoute.nod_y[nodPrev], modelSea.seaRoute.nod_x[nodPrev],
					 	modelSea.seaRoute.nod_y[nodNu], modelSea.seaRoute.nod_x[nodNu]);
					//dist = p1.distanceTo(p2) / 1000.0;
					addArcBetweenNodes(nodPrev, nodNu, dist);
					addArcBetweenNodes(nodNu, nodPrev, dist);
				}
				nodPrev = nodNu;
				p1 = p2;
			}
			model.seaRoute.seaRoutePath[posUse].nCoords = nCoords;
			posUse++;
			pos++;
		}
		model.seaRoute.nSeaRoutePaths = posUse;
	}
	else {
		postRequest("ERROR! json file " + std::string(namn) + " has no paths in it.Fix it and run OptiNav-autoRoute again.", 1);
	}

	fil.close();

	printf("nSeaRoutePaths %d\nn2coords %d\nnMaxCoords %d\n", posUse, n2coords, nMaxCoords);
	printf("nNodes %d nArcs %d\n", modelSea.nNoder, modelSea.nArcs);

	sprintf(namn, "%s/autoRoute/seaRoute_nodes.txt", model.params.indataPath.c_str());
	FILE* filpek;
	int i, i1;
	double kvot;
	filpek = fopen(namn, "w");
	fprintf(filpek, "%d\n", modelSea.nNoder);
	for (i = 0; i < modelSea.nNoder; i++) {
		fprintf(filpek, "%d %d %.4lf %.4lf\n", i, modelSea.Noder[i].nUtNoder, modelSea.seaRoute.nod_y[i], modelSea.seaRoute.nod_x[i]);
	}
	fclose(filpek);

	sprintf(namn, "%s/autoRoute/seaRoute_arcs.txt", model.params.indataPath.c_str());
	filpek = fopen(namn, "w");
	fprintf(filpek, "%d\n", modelSea.nArcs);
	fprintf(filpek, "%d\n", model.nExtraNoGoAreasBase);

	model.boundingBox.xMin = -180;
	model.boundingBox.xMax = 180;
	model.boundingBox.yMin = -75;
	model.boundingBox.yMax = 80;

	char* namn2;
	namn2 = (char*)malloc2(256 * sizeof(char));
	model.extraNoGoArea = (strExtraNoGo*)malloc((model.nExtraNoGoAreasBase) * sizeof(strExtraNoGo));
	for (i = 0; i < model.nExtraNoGoAreasBase; i++) {
		if (model.extraNoGoAreaBase[i].extraCostFactor > -0.99) {
			model.extraNoGoArea[i].posBase = i;
			sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.extraNoGoAreaBase[i].fileNameA);
			checkMinnesAnvandning(__LINE__);
			openNoGoAreas_local(i, namn2);
		}
	}

	int nod1, nod2;
	for (i = 0; i < modelSea.nArcs; i++) {
		nod1 = modelSea.arc[i].fromPointNr;
		nod2 = modelSea.arc[i].toPointNr;
		modelSea.arc[i].totCost = modelSea.arc[i].distance;
		fprintf(filpek, "%d %d %d %.4lf %.4lf", i, nod1, nod2,
			modelSea.arc[i].totCost, modelSea.arc[i].distance);
		for (i1 = 0; i1 < model.nExtraNoGoAreasBase; i1++) {
			if (model.extraNoGoAreaBase[i1].extraCostFactor > -0.99) {
				kvot = 1 - check_map_badKvot_auto(modelSea.seaRoute.nod_y[nod1],
					modelSea.seaRoute.nod_x[nod1],
					modelSea.seaRoute.nod_y[nod2],
					modelSea.seaRoute.nod_x[nod2], 0, i1, 0);
				fprintf(filpek, " %lf", kvot);
			}
			else
				fprintf(filpek, " -1.0");
		}
		fprintf(filpek, "\n");
	}
	fclose(filpek);

	postRequest("Saved new data for searoutes. Run again with newSeaRoutePathData != 1", 1);

	return 0;
}

int load_saved_searoutes() {
	FILE* filpek;
	int i, iTmp, antal, nod1, nod2, xIndex, nExtraAreas, i1;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	double costFactorArea, factor;

	sprintf(namn, "%s/autoRoute/seaRoute_nodes.txt", model.params.indataPath.c_str());
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		postRequest("ERROR! could not open seaRoute_nodes.txt. Run with newSeaRoutePathData first. I quit", 1);
	}

	fscanf(filpek, "%d\n", &(modelSea.nNoder));
	modelSea.nAllocNoder = modelSea.nNoder;
	modelSea.Noder = (strNoder*)malloc(modelSea.nAllocNoder * sizeof(strNoder));
	modelSea.seaRoute.nod_y = (double*)malloc(modelSea.nAllocNoder * sizeof(double));
	modelSea.seaRoute.nod_x = (double*)malloc(modelSea.nAllocNoder * sizeof(double));

	for (i = 0; i < modelSea.nNoder; i++) {
		antal = fscanf(filpek, "%d %d %lf %lf", &iTmp, &(modelSea.Noder[i].nUtNoder), 
			&(modelSea.seaRoute.nod_y[i]), &(modelSea.seaRoute.nod_x[i]));
		if (antal != 4) {
			postRequest("ERROR! failed to read from seaRoute_nodes.txt. i " + std::to_string(i) + " antal " + std::to_string(antal), 1);
		}

		modelSea.Noder[i].nAllocUtNoder = modelSea.Noder[i].nUtNoder;
		modelSea.Noder[i].UtNod = (int*)malloc(modelSea.Noder[i].nAllocUtNoder * sizeof(int));
		modelSea.Noder[i].UtNodCost = (double*)malloc(modelSea.Noder[i].nAllocUtNoder * sizeof(double));
		modelSea.Noder[i].outArcNr = (int*)malloc(modelSea.Noder[i].nAllocUtNoder * sizeof(int));
		modelSea.Noder[i].nUtNoder = 0;

	}
	fclose(filpek);

	sprintf(namn, "%s/autoRoute/seaRoute_arcs.txt", model.params.indataPath.c_str());
	filpek = fopen(namn, "r");
	if (filpek == NULL) {
		postRequest("ERROR! could not open seaRoute_arcs.txt. Run with newSeaRoutePathData first. I quit", 1);
	}
	fscanf(filpek, "%d\n", &(modelSea.nArcs));
	modelSea.nAllocArcs = modelSea.nArcs;
	modelSea.arc = (strArcInfo*)malloc(modelSea.nAllocArcs * sizeof(strArcInfo));

	fscanf(filpek, "%d\n", &nExtraAreas);
	if (nExtraAreas != model.nExtraNoGoAreasBase) {
		postRequest("ERROR! Run first autoRoute with newSeaRoutePathData = 1, as it has changed", 1);
	}

	for (i = 0; i < modelSea.nArcs; i++) {
		antal = fscanf(filpek, "%d %d %d %lf %lf\n", &iTmp, &(nod1),
			&(nod2), &(modelSea.arc[i].totCost), &(modelSea.arc[i].distance));

		modelSea.arc[i].fromPointNr = nod1;
		modelSea.arc[i].toPointNr = nod2;
		modelSea.arc[i].corridorNr = -1;
		//costFactorArea = getCostFactorArea(modelSea.seaRoute.nod_y[modelSea.arc[i].fromPointNr], modelSea.seaRoute.nod_x[modelSea.arc[i].fromPointNr],
		//	modelSea.seaRoute.nod_y[modelSea.arc[i].toPointNr], modelSea.seaRoute.nod_x[modelSea.arc[i].toPointNr], &xIndex);
		//modelSea.arc[i].totCost = modelSea.arc[i].distance * costFactorArea;
		if (antal != 5) {
			postRequest("ERROR! failed to read from seaRoute_arcs.txt. i " + std::to_string(i) + " antal " + std::to_string(antal), 1);
		}
		if (modelSea.Noder[nod1].nUtNoder >= modelSea.Noder[nod1].nAllocUtNoder) {
			errlog("ERROR! Too many outnodes for nod %d compared to given in seRoute_arcs.txt, %d\n", nod1, modelSea.Noder[nod1].nUtNoder);
		}

		modelSea.arc[i].extraAreaFactor = (double*)malloc(model.nExtraNoGoAreasBase * sizeof(double));
		for (i1 = 0; i1 < model.nExtraNoGoAreasBase; i1++) {
			antal = fscanf(filpek, " %lf", &factor);
			if (antal != 1)
				postRequest("ERROR! failed to read from seRoute_arcs.txt. i " + std::to_string(i) + " antal " + std::to_string(antal), 1);
			modelSea.arc[i].extraAreaFactor[i1] = factor;
		}
	}
	fclose(filpek);
	printf("nNodes %d nArcs %d\n", modelSea.nNoder, modelSea.nArcs);
	return 0;
}

int findClosestSeaRouteNode(double x, double y, double* dist) {
	double minDist = 1e10, distNu;
	int nodNr = -1, i;

	for (i = 0; i < modelSea.nNoder; i++) {
		distNu = getDist_flat(modelSea.seaRoute.nod_x[i], modelSea.seaRoute.nod_y[i], x, y);
		if (distNu < minDist) {
			minDist = distNu;
			nodNr = i;
		}
	}
	*dist = minDist;
	return nodNr;
}

int generateCorridorSeaRouteArcs() {
	int i, nod1, nod2, pktNr, i1;
	double dist, distPath;

	modelSea.nAllocArcs += 3 * model.paramsAutoRoute.nCorridors_noGoSoft; // worst case is to generate arcs from start and end nod and between them for each corridor
	modelSea.arc = (strArcInfo*)realloc(modelSea.arc, modelSea.nAllocArcs * sizeof(strArcInfo));

	printf("\n\n****************************************\n\nERROR! ERROR kapa corridorer om start/slut inom!\n\n*****************************\n");
	errlog("\n\n****************************************\n\nERROR! ERROR kapa corridorer om start/slut inom!\n\n*****************************\n");
	// add logics for;
	//	om start / slutpkt ar inom extraNoGo och denna extraNoGo har corridor->koll om narmsta pkten pa corridoren ligger nara;
	//	om ej endpunkt pa corridor och ligger nara->kapa corridoren samt skapa seaRoute arc till corridoren fran start / slutpkten;

	// identifiera om start och/eller slutpunkt inom corridorens noGo omrade
	//   om ja, hitta narmsta punkt pa corridoren om tillrackligt nara sa  bryt korridoren dar (endpunkt) och 
	//   lagg till en nod dar samt en bage mellan start/slutpkt och noden pa korridoren

	// lagg till bagar for corridorerna
	// identifiera narmsta nod till respektive start/endpkt pa corridorer (ska finna nagon nara), ej for de som modifierats ovan

	for (i = 0; i < model.paramsAutoRoute.nCorridors_noGoSoft; i++) {
		identifyMod_startEndOnCorridorSeaRoute(i, 0); // startPkt of autoRoute
		identifyMod_startEndOnCorridorSeaRoute(i, 1); // endPkt of autoRoute

		if (model.paramsAutoRoute.corridors_noGoSoft[i].startNod == -1) { // ej modifierad pkt
			nod1 = findClosestSeaRouteNode(model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[0], model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[0], &dist);
			// model.paramsAutoRoute.corridors_noGoSoft[i].startNod = nod1;
		}
		else
			nod1 = model.paramsAutoRoute.corridors_noGoSoft[i].startNod;
		pktNr = model.paramsAutoRoute.corridors_noGoSoft[i].nPkter - 1;
		if (model.paramsAutoRoute.corridors_noGoSoft[i].endNod == -1) { // ej modifierad pkt
			nod2 = findClosestSeaRouteNode(model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[pktNr], model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[pktNr], &dist);
			// model.paramsAutoRoute.corridors_noGoSoft[i].endNod = nod2;
		}
		else
			nod2 = model.paramsAutoRoute.corridors_noGoSoft[i].endNod;

		//if (modelSea.Noder[nod1].nUtNoder >= modelSea.Noder[nod1].nAllocUtNoder) {
		(modelSea.Noder[nod1].nAllocUtNoder)++;
		modelSea.Noder[nod1].UtNod = (int*)realloc(modelSea.Noder[nod1].UtNod, modelSea.Noder[nod1].nAllocUtNoder * sizeof(int));
		modelSea.Noder[nod1].UtNodCost = (double*)realloc(modelSea.Noder[nod1].UtNodCost, modelSea.Noder[nod1].nAllocUtNoder * sizeof(double));
		modelSea.Noder[nod1].outArcNr = (int*)realloc(modelSea.Noder[nod1].outArcNr, modelSea.Noder[nod1].nAllocUtNoder * sizeof(int));
		//}
		
		if (modelSea.nArcs >= modelSea.nAllocArcs) {
			modelSea.nAllocArcs += 100;
			modelSea.arc = (strArcInfo*)realloc(modelSea.arc, modelSea.nAllocArcs * sizeof(strArcInfo));
		}

		modelSea.arc[modelSea.nArcs].fromPointNr = nod1;
		modelSea.arc[modelSea.nArcs].toPointNr = nod2;
		modelSea.arc[modelSea.nArcs].extraAreaFactor = (double*)calloc(model.nExtraNoGoAreasBase, sizeof(double));
		modelSea.arc[modelSea.nArcs].corridorNr = i;
		distPath = 0;
		for (i1 = 0; i1 < pktNr; i1++) {
			distPath += estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[i1],
				model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[i1],
				model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[i1 + 1],
				model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[i1 + 1]);
		}
		modelSea.arc[modelSea.nArcs].distance = distPath;
		modelSea.arc[modelSea.nArcs].totCost = distPath * model.paramsAutoRoute.corridors_noGoSoft[i].costFactorDist;


		(modelSea.nArcs)++;
	}

	return 0;
}

int updateCostsSeaRouteArcs() {
	int i, xIndex, nod1, nod2, i1;
	double costFactorArea, costKvot;

	// modelSea.seaRoute.costFactorExtraNoGo = 20.0;

	if (model.paramsAutoRoute.nCorridors_noGoSoft > 0) {
		generateCorridorSeaRouteArcs();
	}

	for (i = 0; i < modelSea.nArcs; i++) {
		if (i == 17523)
			i = i;
		//if(i >= 17503)
		//	checkMinnesAnvandning(__LINE__);
		nod1 = modelSea.arc[i].fromPointNr;
		nod2 = modelSea.arc[i].toPointNr;
		if (modelSea.arc[i].corridorNr == -1) {
			costFactorArea = getCostFactorArea(modelSea.seaRoute.nod_y[nod1], modelSea.seaRoute.nod_x[nod1],
				modelSea.seaRoute.nod_y[nod2], modelSea.seaRoute.nod_x[nod2], &xIndex);
			costKvot = 1.0;
			for (i1 = 0; i1 < model.nExtraNoGoAreas; i1++) {
				if (model.extraNoGoArea[i1].posBase >= 0)
					costKvot += modelSea.seaRoute.costFactorExtraNoGo *
					modelSea.arc[i].extraAreaFactor[model.extraNoGoArea[i1].posBase];
			}
			for (i1 = 0; i1 < model.nExtraCostAreas; i1++) {
				if (model.extraCostArea[i1].extraCostFactor > 0)
					costKvot += model.extraCostArea[i1].extraCostFactor *
					modelSea.arc[i].extraAreaFactor[model.extraCostArea[i1].posBase];
			}
			modelSea.arc[i].totCost = modelSea.arc[i].distance * costFactorArea * costKvot;
		}
		modelSea.Noder[nod1].UtNod[modelSea.Noder[nod1].nUtNoder] = nod2;
		modelSea.Noder[nod1].UtNodCost[modelSea.Noder[nod1].nUtNoder] = modelSea.arc[i].totCost;
		modelSea.Noder[nod1].outArcNr[modelSea.Noder[nod1].nUtNoder] = i;
		(modelSea.Noder[nod1].nUtNoder)++;
	}
	return 0;
}



int calc_boundingBoxAutoRoute() { // not needed anymore
	double minX, minY, maxX, maxY, kvot;

	if (model.paramsAutoRoute.startPoint_lon[0] < model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1]) {
		minX = model.paramsAutoRoute.startPoint_lon[0];
		maxX = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
	}
	else {
		maxX = model.paramsAutoRoute.startPoint_lon[0];
		minX = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
	}
	if (model.paramsAutoRoute.startPoint_lat[0] < model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1]) {
		minY = model.paramsAutoRoute.startPoint_lat[0];
		maxY = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
	}
	else {
		maxY = model.paramsAutoRoute.startPoint_lat[0];
		minY = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
	}
	minX -= model.paramsAutoRoute.discretizationSizeLevel[0] / 2;
	kvot = (maxX - minX) / model.paramsAutoRoute.discretizationSizeLevel[0];
	kvot -= (int)kvot;
	if (kvot < 0.25)
		minX += kvot * model.paramsAutoRoute.discretizationSizeLevel[0];
	else {
		if (kvot > 0.75)
			minX -= (1 - kvot) * model.paramsAutoRoute.discretizationSizeLevel[0];
	}
	minY -= model.paramsAutoRoute.discretizationSizeLevel[0] / 2;
	kvot = (maxY - minY) / model.paramsAutoRoute.discretizationSizeLevel[0];
	kvot -= (int)kvot;
	if (kvot < 0.25)
		minY += kvot * model.paramsAutoRoute.discretizationSizeLevel[0];
	else {
		if (kvot > 0.75)
			minY -= (1 - kvot) * model.paramsAutoRoute.discretizationSizeLevel[0];
	}


	double distGrader = model.paramsAutoRoute.discretizationSizeLevel[0]; // / 81;
	model.paramsAutoRoute.x_min = minX - model.paramsAutoRoute.factorExtraCover * distGrader;
	model.paramsAutoRoute.y_min = minY - model.paramsAutoRoute.factorExtraCover * distGrader;
	//int intTmp = (int)(model.paramsAutoRoute.x_min * 10);
	//double floatTmp = (double)intTmp / 10;
	int intTmp = (int)(model.paramsAutoRoute.x_min / model.paramsAutoRoute.discretizationSizeLevel[0]);
	double floatTmp = (double)intTmp * model.paramsAutoRoute.discretizationSizeLevel[0];
	if (model.paramsAutoRoute.x_min < floatTmp)
		model.paramsAutoRoute.x_min = floatTmp - model.paramsAutoRoute.discretizationSizeLevel[0]; // 0.1;
	else
		model.paramsAutoRoute.x_min = floatTmp;

	//intTmp = (int)(model.paramsAutoRoute.y_min * 10);
	//floatTmp = (double)intTmp / 10;
	intTmp = (int)(model.paramsAutoRoute.y_min / model.paramsAutoRoute.discretizationSizeLevel[0]);
	floatTmp = (double)intTmp * model.paramsAutoRoute.discretizationSizeLevel[0];
	if (model.paramsAutoRoute.y_min < floatTmp)
		model.paramsAutoRoute.y_min = floatTmp - model.paramsAutoRoute.discretizationSizeLevel[0]; // 0.1;
	else
		model.paramsAutoRoute.y_min = floatTmp;



	if (model.paramsAutoRoute.y_min < -90)
		model.paramsAutoRoute.y_min = -90;

	model.boundingBox.xMin = model.paramsAutoRoute.x_min;
	model.boundingBox.yMin = model.paramsAutoRoute.y_min;

	model.boundingBox.xMax = maxX + (model.paramsAutoRoute.factorExtraCover + 1) * distGrader; // to make room for full cells
	model.boundingBox.yMax = maxY + (model.paramsAutoRoute.factorExtraCover + 1) * distGrader;
	if (model.boundingBox.yMax > 90)
		model.boundingBox.yMax = 90;

	return 0;
}

int check_isLandOnly_auto(double lat1, double lon1, double lat2, double lon2) {
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg;
	int row, col, i, isOK, colUse;

	getRowColDblFromPhysicalMap(model.physical_lessBuffer_MapB, lat1, lon1, &row1Dbl, &col1Dbl);
	getRowColDblFromPhysicalMap(model.physical_lessBuffer_MapB, lat2, lon2, &row2Dbl, &col2Dbl);

	delta_row = row2Dbl - row1Dbl;
	if (delta_row > model.physical_lessBuffer_MapB.nRows / 2) {
		delta_row = model.physical_lessBuffer_MapB.nRows - delta_row;
	}
	else {
		if (delta_row < -model.physical_lessBuffer_MapB.nRows / 2) {
			delta_row = -model.physical_lessBuffer_MapB.nRows - delta_row;
		}
	}
	delta_col = col2Dbl - col1Dbl;
	if (delta_col > model.physical_lessBuffer_MapB.nCols / 2) {
		delta_col = delta_col - model.physical_lessBuffer_MapB.nCols;
	}
	else {
		if (delta_col < -model.physical_lessBuffer_MapB.nCols / 2) {
			delta_col = model.physical_lessBuffer_MapB.nCols + delta_col;
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
		if (col >= model.physical_lessBuffer_MapB.nCols)
			colUse = col - model.physical_lessBuffer_MapB.nCols;
		else {
			if (col < 0)
				colUse = col + model.physical_lessBuffer_MapB.nCols;
			else
				colUse = col;
		}
		if (model.physical_lessBuffer_MapB.valueCell[row * model.physical_lessBuffer_MapB.nCols + colUse] != 0)
			return 0; // arc is not only on land

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

int checkIsCompleteLand(double y1, double x1, double y2, double x2) {
	int isLand = 1;
	//return 0;

	isLand = check_isLandOnly_auto(y1, x1, y1, x2); // horizontal
	if (isLand == 0)
		return isLand;
	isLand = check_isLandOnly_auto(y1, x1, y2, x1); // vertical
	if (isLand == 0)
		return isLand;
	isLand = check_isLandOnly_auto(y1, x1, y2, x2); // diagonal up
	if (isLand == 0)
		return isLand;
	isLand = check_isLandOnly_auto(y2, x1, y1, x2); // diagonal down
	if (isLand == 0)
		return isLand;

	return isLand;
}

double check_map_badKvot_auto(double lat1, double lon1, double lat2, double lon2, int mapAlt, int pos_noGoMap, int costArea) {
	Raster::strPhysRaster physicalMap;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg, kvotOK = 0, kvotOKTmp;
	int row, col, i, isOK, colUse, posOK;
	int nVardenOnMap = 0;

	if (pos_noGoMap >= 0) {
		if (costArea == 0) {
			if (mapAlt == 0)
				physicalMap = model.extraNoGoArea[pos_noGoMap].mapB;
			else
				physicalMap = model.extraNoGoArea[pos_noGoMap].mapA;
		}
		else {
			if (mapAlt == 0)
				physicalMap = model.extraCostArea[pos_noGoMap].mapB;
			else
				physicalMap = model.extraCostArea[pos_noGoMap].mapA;
		}
	}
	else {
		if (pos_noGoMap == -1) {
			if (mapAlt == 0)
				physicalMap = model.physicalMapB;
			else
				physicalMap = model.physicalMapA;
		}
		else {
			if (pos_noGoMap == -2) {
				if (mapAlt == 0)
					physicalMap = model.physical_lessBuffer_MapB;
				else
					physicalMap = model.physical_lessBuffer_MapA;
			}
			else {
				// eca
				errlog("ERROR! Should never come here, code row %d\n", __LINE__);
				//if (mapAlt == 0)
				//	physicalMap = model.fuelMapB;
				//else
				//	physicalMap = model.fuelMapA;

			}
		}
	}

	if (pos_noGoMap == -1) {
		getRowColDblFromPhysicalMap(physicalMap, lat1, lon1, &row1Dbl, &col1Dbl);
		getRowColDblFromPhysicalMap(physicalMap, lat2, lon2, &row2Dbl, &col2Dbl);
	}
	else {
		getRowColDblFromNoGoMap(physicalMap, lat1, lon1, &row1Dbl, &col1Dbl);
		getRowColDblFromNoGoMap(physicalMap, lat2, lon2, &row2Dbl, &col2Dbl);
		if ((row1Dbl < 0 && row2Dbl < 0) || (col1Dbl < 0 && col2Dbl < 0))
			return 1; // outside the area so all good
		if ((row1Dbl >= physicalMap.nRows && row2Dbl >= physicalMap.nRows) || (col1Dbl >= physicalMap.nCols && col2Dbl >= physicalMap.nCols))
			return 1; // outside the area so all good
	}
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
		//if (posOK == 1) {
		//	if (physicalMap.valueCell[row * physicalMap.nCols + colUse] == 0)
		//		return 0; // arc is not okay
		//}
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
				kvotOKTmp = check_map_badKvot_auto(y1, x1, y2, x2, 1, pos_noGoMap, costArea);
				kvotOK += kvotOKTmp;
				//if (isOK == 0)
				//	return 0; // arc is not okay in raster alt
			}
			else {
				kvotOK += physicalMap.valueCell[row * physicalMap.nCols + colUse];

			}
			nVardenOnMap++;
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

	if (mapAlt == 0) {
		if (pos_noGoMap >= 0) {
			if (nVardenOnMap > 0)
				return 1 - kvotOK / nVardenOnMap;
			else
				return 0.0;
		}
		else
			return 1 - kvotOK / (i + 1); // feasibility map
	}
	else {
		if (pos_noGoMap >= 0) {
			if (nVardenOnMap > 0)
				return kvotOK / nVardenOnMap;
			else
				return 1.0;
		}
		else
			return kvotOK / (i + 1); // feasibility map
	}
}

int evalCostArc_old(strAutoCells* cell, int pos1, double y1, double x1, double y2, double x2) {
	double cost, dist;

	int isOk = check_physicalMap_ok(y1, x1, y2, x2, 0);

	if (isOk == 1) {
		for (int i = 0; i < model.nExtraNoGoAreas; i++) {
			isOk = check_extraNoGoMap_ok(y1, x1, y2, x2, 0, i);
			if (isOk == 0)
				break;
		}
	}
	dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
	cell->arcDistance[pos1] = dist;

	if (isOk == 0) {
		isOk = check_extraNoGoMap_ok(y1, x1, y2, x2, 0, model.nExtraNoGoAreas); // the last extra map is for land!!!
		if (isOk == 0)
			cost = dist * 10000;
		else
			cost = dist * 100;
	}
	else
		cost = dist;
	cell->arcCost[pos1] = cost;
	return 0;
}

double getCostKvotFromBadKvots_feasibility(double y1, double x1, double y2, double x2, int includeCostFeasible) {
	double kvotBad, kvotBad2, costKvot, kvotBadExtra;
	double kvotBad0;
	if (includeCostFeasible == 1) {
		kvotBad0 = check_map_badKvot_auto(y1, x1, y2, x2, 0, -1, 0); // physical map 12nm buffer
		if (kvotBad0 > 0)
			kvotBad = check_map_badKvot_auto(y1, x1, y2, x2, 0, -2, 0); // physical map 2nm buffer
		else
			kvotBad = 0;
		kvotBadExtra = 0;
		if (kvotBad < 1) {
			for (int i = 0; i < model.nExtraNoGoAreas; i++) {
				kvotBadExtra += 1 - check_map_badKvot_auto(y1, x1, y2, x2, 0, i, 0);
				//if (kvotBadExtra >= 1)
				//	break;
			}
		}

		if (kvotBad > 0)
			kvotBad2 = check_map_badKvot_auto(y1, x1, y2, x2, 0, model.nExtraNoGoAreas, 0); // the last extra map is for land!!!
		else
			kvotBad2 = 0;

		costKvot = (1 + kvotBad0 + 1000 * (kvotBad + kvotBadExtra) + 100000 * kvotBad2);
	}
	else
		costKvot = 1.0;

	//if (kvotBad0 > 0 || kvotBad > 0) {
	//	if (kvotBad > 0)
	//		kvotBad2 = check_map_badKvot_auto(y1, x1, y2, x2, 0, model.nExtraNoGoAreas); // the last extra map is for land!!!
	//	else
	//		kvotBad2 = 0;
	//	costKvot = (1 + kvotBad0 + 1000 * kvotBad + 100000 * kvotBad2);
	//}
	//else
	//	costKvot = 1.0;

	double extraCostKvot = 0, kvot;
	for (int i = 0; i < model.nExtraCostAreas; i++) {
		kvot = 1 - check_map_badKvot_auto(y1, x1, y2, x2, 0, i, 1);
		extraCostKvot += kvot * model.extraCostArea[i].extraCostFactor;
	}
	costKvot += extraCostKvot;
	return costKvot;

	//double kvotECA;
	//if (model.paramsAutoRoute.eca_penalty > 1) {
	//	kvotECA = 1 - check_map_badKvot_auto(y1, x1, y2, x2, 0, -3); // eca penalty
	//	costKvot += (model.paramsAutoRoute.eca_penalty - 1) * kvotECA;
	//}
	//return costKvot;
}

double evalCostArc(double y1b, double x1b, double y2b, double x2b, double* distRes) {
	double cost, dist, kvotBad2, totCost = 0, totDist, costFactorArea;
	double kvotBad, x1, y1, x2, y2, splitDist, costKvot, totDist2;
	totDist = estimateLargeCircleDistance_km(y1b, x1b, y2b, x2b);
	double costFactorAreaBas;
	int nInt, i, xIndex;
	spherical::Point p1, p2, p3;

	x1 = x1b;
	y1 = y1b;
	x2 = x2b;
	y2 = y2b;

	if (model.nArcs >= 652749)
		model.nArcs = model.nArcs;
	// split the arc as it is too long
	nInt = roundUp(totDist / DIST_SPLIT);
	if (nInt > 1) {
		p1 = spherical::Point(y1, fix_lonPos(x1));
		p2 = spherical::Point(y2, fix_lonPos(x2));
		totDist = p1.distanceTo(p2) / 1000.0;
		splitDist = totDist / nInt * 1000.0;
		costFactorAreaBas = getCostFactorArea(y1, x1, y2, x2, &xIndex);
	}
	else
		costFactorAreaBas = 2;

	totDist2 = 0;
	for (i = 0; i < nInt; i++) {
		if (nInt > 1) {
			if (i < nInt - 1) {
				p3 = p1.destinationPoint(splitDist * (i + 1), p1.bearingTo(p2));
				y2 = p3.latitude().degrees();
				x2 = p3.longitude().degrees();
			}
			else {
				y2 = y2b;
				x2 = x2b;
			}
		}
		costFactorArea = getCostFactorArea(y1, x1, y2, x2, &xIndex);
		if (costFactorAreaBas < 1.00001) {
			if (costFactorArea >= 1.00001) {
				if (y2 < model.paramsAutoRoute.minLat_lonIndex[xIndex]) {
					y2 = model.paramsAutoRoute.minLat_lonIndex[xIndex];
					costFactorArea = 1.0;
				}
			}
		}
		
		dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
		costKvot = getCostKvotFromBadKvots_feasibility(y1, x1, y2, x2);
		totCost += dist * costKvot * costFactorArea;
		totDist2 += dist;

		// totDist += dist;
		x1 = x2;
		y1 = y2;
	}

	//if (totDist2 != totDist && totDist > 2) {
	//	totCost *= totDist / totDist2;
	//}

	*distRes = totDist2;
	return totCost;
}

int evalCostArc_cells(strAutoCells* cell, int pos1, double y1, double x1, double y2, double x2) {
	double dist;
	double cost = evalCostArc(y1, x1, y2, x2, &dist);
	if (dist > 1000)
		dist = dist;
	cell->arcDistance[pos1] = dist;
	cell->arcCost[pos1] = cost;
	return 0;
}

int evalCostArc2(double y1, double x1, double y2, double x2, double* cost, double* dist) {
	double costFactorArea, costKvot;
	int xIndex;

	*dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
	costKvot = getCostKvotFromBadKvots_feasibility(y1, x1, y2, x2);
	costFactorArea = getCostFactorArea(y1, x1, y2, x2, &xIndex);
	*cost = *dist * costKvot * costFactorArea;

	return 0;
}

strAutoCells* addSmallerCells(strAutoCells* thisCell, double xBas, double yBas, int levelTo) {

	int i, nAlloc, i1, i2, pos, posTmp;
	double x, y, yTmp, xTmp;
	// check if already smaller level then do nothing
	if (thisCell->smallerCells != NULL)
		return thisCell->smallerCells;

	strAutoCells* cells = thisCell->smallerCells;
	nAlloc = model.paramsAutoRoute.nDiscreteSizeLevel[levelTo] * model.paramsAutoRoute.nDiscreteSizeLevel[levelTo];
	if (cells == NULL) {
		cells = (strAutoCells*)malloc(nAlloc * sizeof(strAutoCells));
		for (i = 0; i < nAlloc; i++) {
			cells[i].smallerCells = NULL;
			cells[i].smallerCellsType = 0;
			cells[i].use = 0;
		}
	}

	pos = 0;
	for (i = 0; i < thisCell->nYsmall; i++) {
		for (i1 = 0; i1 < thisCell->nXsmall; i1++) {
			x = xBas + i1 * model.paramsAutoRoute.discretizationSizeLevel[levelTo];
			y = yBas + i * model.paramsAutoRoute.discretizationSizeLevel[levelTo];
			pos = i1 + model.paramsAutoRoute.nDiscreteSizeLevel[levelTo] * i;
			if (pos == 31)
				pos = pos;
			cells[pos].x = x;
			cells[pos].y = y;
			evalCostArc_cells(&(cells[pos]), 0, y, x, y, x + model.paramsAutoRoute.discretizationSizeLevel[levelTo]); // horizontal
			evalCostArc_cells(&(cells[pos]), 1, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[levelTo], x + model.paramsAutoRoute.discretizationSizeLevel[levelTo]); // diagonal
			evalCostArc_cells(&(cells[pos]), 2, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[levelTo], x); // vertical
			x += model.paramsAutoRoute.discretizationSizeLevel[levelTo];
			evalCostArc_cells(&(cells[pos]), 3, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[levelTo], x - model.paramsAutoRoute.discretizationSizeLevel[levelTo]); // diagonal

			if (pos == 11)
				pos = pos;
			if (levelTo < model.paramsAutoRoute.nCellLevels - 1) {
				for (i2 = 0; i2 < 4; i2++) {
					if (cells[pos].arcCost[i2] > model.paramsAutoRoute.maxBaseFeasibleCost) {
						break;
					}
				}
				if (i2 < 4) {
					cells[pos].smallerCells = addSmallerCells(&(cells[pos]), x, y, levelTo + 1);
					cells[pos].smallerCellsType = 1;
					//if (cells[pos].arcCost[0] > model.paramsAutoRoute.maxBaseFeasibleCost) {
					//	yTmp = y - model.paramsAutoRoute.discretizationSizeLevel[levelTo];
					//	posTmp = i1 + model.paramsAutoRoute.nXbasLevel * (i - 1);
					//	cells[posTmp].smallerCells = addSmallerCells(cells[posTmp].smallerCells, x, yTmp, levelTo + 1);
					//}
					//if (cells[pos].arcCost[2] > model.paramsAutoRoute.maxBaseFeasibleCost) {
					//	xTmp = x - model.paramsAutoRoute.discretizationSizeLevel[levelTo];
					//	cells[pos - 1].smallerCells = addSmallerCells(cells[pos - 1].smallerCells, xTmp, y, levelTo + 1);
					//}
				}
			}
		}
	}

	return cells;
}

double getUsable_x(double x) {
	if (x < model.paramsAutoRoute.x_min)
		x += 360;
	if (x > model.paramsAutoRoute.x_min + 360)
		x -= 360;
	return x;
}


int identifyCellFromPoint(double y, double x, int* yPos, int* xPos, int* level) {

	*yPos = (y - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
	x = getUsable_x(x);
	*xPos = (x - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
	*level = 0; // OBS check more levels later on...
	return 0;
}


int writeAllAutoNodesToGeojson(int iter)
{
	int forsta = 1, pos1, i3, i4;
	strAutoCells* cells;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/autoNodesBig_%d.geojson", model.params.indataPath.c_str(), iter);
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "allPhysicalNodesBig");

	int i, i1, pos = 0, level, small;
	double x, y, xNy, yNy;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (model.autoRoute[pos].use > 0) {
				if (forsta != 1)
					fprintf(filpekG, ", ");
				else
					forsta = 0;
				fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"xPos\":%d, \"yPos\":%d},\n",
					0, model.autoRoute[pos].nodeNr[0], pos, i1, i);
				x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
				y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
				fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", x, y);
			}
			pos++;
		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	sprintf(namn, "%s/autoNodesSmall_%d.geojson", model.params.indataPath.c_str(), iter);
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "nodesSmall");

	forsta = 1;
	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
			y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
			if (model.autoRoute[pos].smallerCellsType > 0) {
				cells = model.autoRoute[pos].smallerCells;
				level = 1;
				small = 0;
				for (i3 = 0; i3 < model.autoRoute[pos].nYsmall; i3++) {
					for (i4 = 0; i4 < model.autoRoute[pos].nXsmall; i4++) {
						//if (i3 == 0 && i4 == 0)
						//	continue; // no need to save this node as it is already saved from earlier level
						if (model.autoRoute[pos].smallerCellsType == 4 && i3 > 0 && i4 > 0)
							continue; // only the start row or column used
						if (forsta != 1)
							fprintf(filpekG, ", ");
						else
							forsta = 0;
						fprintf(filpekG, "{\"type\":\"Feature\", \"properties\":{\"level\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"cellPos2\":%d, \"xPos\":%d, \"yPos\":%d, \"use\":%d},\n",
							level, model.autoRoute[pos].smallerCells[i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3].nodeNr[0],
							pos, i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3, i4, i3, model.autoRoute[pos].smallerCells[small].use);
						xNy = x + i4 * model.paramsAutoRoute.discretizationSizeLevel[level];
						yNy = y + i3 * model.paramsAutoRoute.discretizationSizeLevel[level];
						fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", xNy, yNy);
						small++;
					}
				}

			}
			pos++;
		}
	}

	for (i = 0; i < model.nAutoPaths; i++) {
		for (i1 = 0; i1 < model.autoPath[i].nNoder; i1++) {
			fprintf(filpekG, ", ");
			fprintf(filpekG, "{\"type\":\"Feature\", \"properties\":{\"level\":%d, \"type\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"cellPos2\":%d, \"xPos\":%d, \"yPos\":%d},\n",
				2, model.autoPath[i].type, model.autoPath[i].nodNr[i1], i, i1, -1, -1);
			xNy = model.autoPath[i].nodCoord_x[i1];
			yNy = model.autoPath[i].nodCoord_y[i1];
			fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", xNy, yNy);

		}
	}

	fprintf(filpekG, "]}\n");
	fclose(filpekG);


	return 0;
}

int writeAllPathNodesToGeojson(int nPaths)
{
	int forsta = 1, pos1, i3, i4;
	strAutoCells* cells;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/pathNodes.geojson", model.params.indataPath.c_str());
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "pathNodes");

	int i, i1, pos = 0, level;
	double x, y, xNy, yNy;
	//for (i = 0; i < model.nAutoPaths; i++) {
	for (i = 0; i < nPaths; i++) {
		// printf("i %d av %d nNoder %d\n", i, nPaths, model.autoPath[i].nNoder);
		for (i1 = 0; i1 < model.autoPath[i].nNoder; i1++) {
			if (forsta != 1)
				fprintf(filpekG, ", ");
			else
				forsta = 0;
			fprintf(filpekG, "{\"type\":\"Feature\", \"properties\":{\"level\":%d, \"type\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"cellPos2\":%d, \"xPos\":%d, \"yPos\":%d},\n",
				2, model.autoPath[i].type, model.autoPath[i].nodNr[i1], i, i1, -1, -1);
			xNy = model.autoPath[i].nodCoord_x[i1];
			yNy = model.autoPath[i].nodCoord_y[i1];
			fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", xNy, yNy);

		}
	}

	fprintf(filpekG, "]}\n");
	fclose(filpekG);


	return 0;
}

void getCoordFromTss(strTss path, int pos, double kvot, double* y, double* x) {
	double y1, y2, x1, x2;
	y1 = path.yCoord[pos];
	x1 = path.xCoord[pos];
	if (kvot > 0.0001) {
		y2 = path.yCoord[pos + 1];
		x2 = path.xCoord[pos + 1];
		*y = y1 * (1 - kvot) + y2 * kvot;
		*x = x1 * (1 - kvot) + x2 * kvot;
	}
	else {
		*y = y1;
		*x = x1;
	}
}

int generate_nodes_along_pathSegment(int pathNr, int initStart, int coordPos, double y1, double x1, double y2, double x2) {
	double dist, distRef, nIntDbl, dx, dy, x, y, prevX, prevY;
	int nInt, i, startI, prevNodNr, nodNr;

	dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
	distRef = estimateLargeCircleDistance_km(y1, x1, y1, x1 + model.paramsAutoRoute.discretizationSizeLevel[1]);
	nIntDbl = dist / distRef;
	if (nIntDbl < 0.001)
		return 0; // too close points, don't add any nodes
	nInt = roundUp(nIntDbl);
	dx = x2 - x1;
	dy = y2 - y1;

	if (initStart == 1) {
		// if (coordPos == path.firstTraffCoord) {
		startI = 0;
		prevNodNr = -1;
		dist = -1;
	}
	else {
		startI = 1;
		prevNodNr = model.autoPath[pathNr].nodNr[model.autoPath[pathNr].nNoder];
	}
	for (i = 0; i <= nInt; i++) {
		x = x1 + dx * i / nInt;
		y = y1 + dy * i / nInt;
		if (i < startI) {
			nodNr = model.autoPath[pathNr].nodNr[model.autoPath[pathNr].nNoder - 1];
		}
		else {
			nodNr = addAutoNodePath(pathNr, y, x); // pos, 3, posSmall);
			if (prevNodNr >= 0)
				dist = estimateLargeCircleDistance_km(prevY, prevX, y, x);
			else
				dist = 0;
			if (nodNr == 13920)
				nodNr = nodNr;
			addArcsInOutFromPathNode(nodNr, pathNr, prevNodNr, dist); // nodNr, pos, i, i1, i3, i4, posSmall);
		}
		prevNodNr = nodNr;
		prevX = x;
		prevY = y;
	}


	return 1;
}

int addArcs_tss() {
	int i, i1, pathNr = 0, nAlloc;
	double x1 = -1, y1 = -1, x2, y2;

	nAlloc = model.nTss + model.nAutoCorridors + model.paramsAutoRoute.nStartSlut + model.paramsAutoRoute.nCorridors_noGoSoft - 1;
	model.autoPath = (strAutoPath*)malloc(nAlloc * sizeof(strAutoPath));

	for (i = 0; i < model.nTss; i++) {
		model.tss[i].autoPathNr = pathNr;
		model.autoPath[pathNr].nAllocNoder = 100;
		model.autoPath[pathNr].nodNr = (int*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(int));
		model.autoPath[pathNr].nodCoord_y = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nodCoord_x = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nNoder = 0;
		model.autoPath[pathNr].type = 0;
		model.autoPath[pathNr].kvotCost = model.tss[i].kvotCost; // 0.01;

		if (i == 11)
			i = i;
		for (i1 = model.tss[i].firstTraffCoord - 1; i1 <= model.tss[i].lastTraffCoord; i1++) {
			if (i1 == model.tss[i].firstTraffCoord - 1)
				getCoordFromTss(model.tss[i], i1, model.tss[i].firstTraffCoordKvot, &y2, &x2);
			else {
				if (i1 == model.tss[i].lastTraffCoord)
					getCoordFromTss(model.tss[i], i1 - 1, model.tss[i].lastTraffCoordKvot, &y2, &x2);
				else
					getCoordFromTss(model.tss[i], i1, 0.0, &y2, &x2);
			}
			if (i1 >= model.tss[i].firstTraffCoord) {
				if (i1 == model.tss[i].firstTraffCoord)
					generate_nodes_along_pathSegment(model.tss[i].autoPathNr, 1, i1, y1, x1, y2, x2);
				else
					generate_nodes_along_pathSegment(model.tss[i].autoPathNr, 0, i1, y1, x1, y2, x2);
			}
			x1 = x2;
			y1 = y2;

		}
		pathNr++;
	}
	model.nAutoPaths = pathNr;

	return 0;
}

int addArcs_corridors() {
	int i, i1, pathNr = model.nAutoPaths, firstTraff;
	double x1 = -1, y1 = -1, x2, y2;

	for (i = 0; i < model.nAutoCorridors; i++) {
		model.autoCorridors[i].autoPathNr = pathNr;
		model.autoPath[pathNr].nAllocNoder = 100;
		model.autoPath[pathNr].nodNr = (int*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(int));
		model.autoPath[pathNr].nodCoord_y = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nodCoord_x = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nNoder = 0;
		model.autoPath[pathNr].type = 1;
		model.autoPath[pathNr].kvotCost = model.autoCorridors[i].kvotCost; // 0.01;

		if (i == 11)
			i = i;
		firstTraff = 0;
		for (i1 = model.autoCorridors[i].firstTraffCoord - 1; i1 <= model.autoCorridors[i].lastTraffCoord; i1++) {
			if (i1 == model.autoCorridors[i].firstTraffCoord - 1)
				getCoordFromTss(model.autoCorridors[i], i1, model.autoCorridors[i].firstTraffCoordKvot, &y2, &x2);
			else {
				if (i1 == model.autoCorridors[i].lastTraffCoord)
					getCoordFromTss(model.autoCorridors[i], i1 - 1, model.autoCorridors[i].lastTraffCoordKvot, &y2, &x2);
				else
					getCoordFromTss(model.autoCorridors[i], i1, 0.0, &y2, &x2);
			}
			if (i1 >= model.autoCorridors[i].firstTraffCoord) {
				if (firstTraff == 0) {
					//if (i1 == model.autoCorridors[i].firstTraffCoord)
					firstTraff = generate_nodes_along_pathSegment(model.autoCorridors[i].autoPathNr, 1, i1, y1, x1, y2, x2);
				}
				else
					generate_nodes_along_pathSegment(model.autoCorridors[i].autoPathNr, 0, i1, y1, x1, y2, x2);
			}
			x1 = x2;
			y1 = y2;

		}
		pathNr++;
	}
	model.nAutoPaths = pathNr;

	return 0;
}

int addArcs_corridors_noGoSoft() {
	int i, i1, pathNr = model.nAutoPaths;
	int posEnd, nodNr, pos, pktNr, nodNr2;
	double dist, cost;
	double x1 = -1, y1 = -1, x2, y2;

	for (i = 0; i < model.paramsAutoRoute.nCorridors_noGoSoft; i++) {
		model.paramsAutoRoute.corridors_noGoSoft[i].autoPathNr = pathNr;
		model.autoPath[pathNr].nAllocNoder = 100;
		model.autoPath[pathNr].nodNr = (int*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(int));
		model.autoPath[pathNr].nodCoord_y = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nodCoord_x = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nNoder = 0;
		model.autoPath[pathNr].type = 2;
		model.autoPath[pathNr].kvotCost = model.paramsAutoRoute.corridors_noGoSoft[i].costFactorDist; // model.autoCorridors[i].kvotCost; // 0.01;

		for (i1 = 1; i1 < model.paramsAutoRoute.corridors_noGoSoft[i].nPkter; i1++) {
			if (i1 == 1) {
				x1 = model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[i1 - 1];
				y1 = model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[i1 - 1];
			}
			x2 = model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[i1];
			y2 = model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[i1];
			generate_nodes_along_pathSegment(model.paramsAutoRoute.corridors_noGoSoft[i].autoPathNr, 0, i1, y1, x1, y2, x2);
			y1 = y2;
			x1 = x2;
		}

		if (model.paramsAutoRoute.corridors_noGoSoft[i].startNod != -1) {
			// addera bage fran startNod pa autoRoute till starNod i corridoren
			if (model.nArcs >= model.nAllocArcs) {
				model.nAllocArcs += 1000000;
				model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
			}

			posEnd = model.paramsAutoRoute.nCellsBase;
			nodNr = model.autoRoute[posEnd].nodeNr[0];
			pos = model.Noder[nodNr].nUtNoder;
			if (pos >= model.Noder[nodNr].nAllocUtNoder) {
				model.Noder[nodNr].nAllocUtNoder += 8;
				model.Noder[nodNr].UtNod = (int*)realloc(model.Noder[nodNr].UtNod, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
				model.Noder[nodNr].UtNodCost = (double*)realloc(model.Noder[nodNr].UtNodCost, model.Noder[nodNr].nAllocUtNoder * sizeof(double));
				model.Noder[nodNr].outArcNr = (int*)realloc(model.Noder[nodNr].outArcNr, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
			}
			nodNr2 = model.autoPath[pathNr].nodNr[0];
			model.Noder[nodNr].UtNod[pos] = nodNr2;
			model.Noder[nodNr].outArcNr[pos] = model.nArcs;
			model.Noder[nodNr].nUtNoder = pos + 1;

			model.arc[model.nArcs].fromLevel = posEnd;
			model.arc[model.nArcs].fromPointNr = 0;
			model.arc[model.nArcs].fromTime = -1;
			model.arc[model.nArcs].toLevel = -1;
			model.arc[model.nArcs].toPointNr = pathNr;
			model.arc[model.nArcs].toTime = 0;
			dist = estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[0],
				model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[0], 
				model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0]);
			model.arc[model.nArcs].distance = dist;
			cost = model.paramsAutoRoute.corridors_noGoSoft[i].costFactorDist * dist;
			model.arc[model.nArcs].totCost = cost;
			model.Noder[nodNr].UtNodCost[pos] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
			(model.nArcs)++;
		}

		if (model.paramsAutoRoute.corridors_noGoSoft[i].endNod != -1) {
			// addera bage fran endnod i corridoren till endnode pa autoRoute 
			if (model.nArcs >= model.nAllocArcs) {
				model.nAllocArcs += 1000000;
				model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
			}

			posEnd = model.paramsAutoRoute.nCellsBase + 1;

			pktNr = model.autoPath[pathNr].nNoder - 1;
			nodNr = model.autoPath[pathNr].nodNr[pktNr];
			// nodNr = model.paramsAutoRoute.corridors_noGoSoft[i].endNod;
			pos = model.Noder[nodNr].nUtNoder;
			if (pos >= model.Noder[nodNr].nAllocUtNoder) {
				model.Noder[nodNr].nAllocUtNoder += 2;
				model.Noder[nodNr].UtNod = (int*)realloc(model.Noder[nodNr].UtNod, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
				model.Noder[nodNr].UtNodCost = (double*)realloc(model.Noder[nodNr].UtNodCost, model.Noder[nodNr].nAllocUtNoder * sizeof(double));
				model.Noder[nodNr].outArcNr = (int*)realloc(model.Noder[nodNr].outArcNr, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
			}
			model.Noder[nodNr].UtNod[pos] = model.autoRoute[posEnd].nodeNr[0];
			model.Noder[nodNr].outArcNr[pos] = model.nArcs;
			model.Noder[nodNr].nUtNoder = pos + 1;

			model.arc[model.nArcs].fromLevel = -1;
			model.arc[model.nArcs].fromPointNr = pathNr;
			model.arc[model.nArcs].fromTime = pktNr;
			model.arc[model.nArcs].toLevel = posEnd;
			model.arc[model.nArcs].toPointNr = 0;
			model.arc[model.nArcs].toTime = -1;
			pktNr = model.paramsAutoRoute.corridors_noGoSoft[i].nPkter - 1;
			dist = estimateLargeCircleDistance_km(model.paramsAutoRoute.corridors_noGoSoft[i].yCoord[pktNr],
				model.paramsAutoRoute.corridors_noGoSoft[i].xCoord[pktNr],
				model.paramsAutoRoute.endPoint_lat[0], model.paramsAutoRoute.endPoint_lon[0]);
			model.arc[model.nArcs].distance = dist;
			cost = model.paramsAutoRoute.corridors_noGoSoft[i].costFactorDist * dist;
			model.arc[model.nArcs].totCost = cost;
			model.Noder[nodNr].UtNodCost[pos] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
			(model.nArcs)++;
		}

		pathNr++;
	}
	model.nAutoPaths = pathNr;

	return 0;
}

int addArcs_viaPaths(int ruttAlt) {
	int i, i1, pathNr = model.nAutoPaths;
	double x1 = -1, y1 = -1, x2, y2;

	for (i = 0; i < model.paramsAutoRoute.nStartSlut - 1; i++) {
		if (model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints < 2)
			continue; // not a path

		if (i < model.nTss)
			model.tss[i].autoPathNr = pathNr;
		else
			model.autoCorridors[i - model.nTss].autoPathNr = pathNr;
		model.autoPath[pathNr].nAllocNoder = 100;
		model.autoPath[pathNr].nodNr = (int*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(int));
		model.autoPath[pathNr].nodCoord_y = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nodCoord_x = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nNoder = 0;
		model.autoPath[pathNr].type = 10;
		model.autoPath[pathNr].kvotCost = 0.5;

		for (i1 = 0; i1 < model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints; i1++) {
			x2 = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].x[i1];
			y2 = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].y[i1];
			if (i1 > 0) {
				if (i1 < 2)
					generate_nodes_along_pathSegment(pathNr, 1, i1, y1, x1, y2, x2);
				else
					generate_nodes_along_pathSegment(pathNr, 0, i1, y1, x1, y2, x2);
			}
			x1 = x2;
			y1 = y2;
		}
		pathNr++;
	}
	model.nAutoPaths = pathNr;

	return 0;
}

double getMinGradRectangles(double a1, double a2, double b1, double b2) {
	double min1 = a1, max1 = a1, min2 = b1, max2 = b1, minGrad;

	if (min1 > a2)
		min1 = a2;
	else
		max1 = a2;
	if (min2 > b2)
		min2 = b2;
	else
		max2 = b2;
	if (max2 <= min1)
		minGrad = min1 - max2;
	else{
		if (min2 >= max1)
			minGrad = min2 - max1;
		else
			minGrad = 0.0;
	}
	return minGrad;
}

double getClosestPossibleDistStartSlutPaths(int path1, int path2){
	double minGrad_x, minGrad_y;
	int pos1, pos2, minGrad;

	pos1 = model.autoPath[path1].nNoder - 1;
	pos2 = model.autoPath[path2].nNoder - 1;
	minGrad_x = getMinGradRectangles(model.autoPath[path1].nodCoord_x[0], model.autoPath[path1].nodCoord_x[pos1],
		model.autoPath[path2].nodCoord_x[0], model.autoPath[path2].nodCoord_x[pos2]);
	minGrad_y = getMinGradRectangles(model.autoPath[path1].nodCoord_y[0], model.autoPath[path1].nodCoord_y[pos1],
		model.autoPath[path2].nodCoord_y[0], model.autoPath[path2].nodCoord_y[pos2]);
	if (minGrad_x > minGrad_y)
		return minGrad_x;
	else
		return minGrad_y;
}

int addArcs_betweenPaths() {
	int i, i1, i2, i3, last_i2;
	double distGrad, bastDist, bastDistNu, dist, x1, y1;

	for (i = 0; i < model.nAutoPaths; i++) {
		if (model.autoPath[i].type >= 10) {
			for (i1 = 0; i1 < i; i1++) {
				distGrad = getClosestPossibleDistStartSlutPaths(i, i1);
				if (distGrad < 0.25) { // close enough
					bastDist = 1e10;
					for (i2 = 0; i2 < model.autoPath[i].nNoder; i2++) {
						x1 = model.autoPath[i].nodCoord_x[i2];
						y1 = model.autoPath[i].nodCoord_y[i2];
						bastDistNu = 1e10;
						for (i3 = model.autoPath[i1].nNoder - 1; i3 >= 0; i3--) {
							dist = getDistSeaRoute(y1, x1, model.autoPath[i1].nodCoord_y[i3], model.autoPath[i1].nodCoord_x[i3]);
							if (dist < bastDistNu) {
								bastDistNu = dist;
							}
							if (dist < bastDist) {
								bastDist = dist;
							}
							if (dist < 0.25) {
								// add arc as it is close enough...
								addAutoArcBetweenPaths(i, i2, i1, i3);
								addAutoArcBetweenPaths(i1, i3, i, i2);
							}
							else {
								if (dist > bastDistNu * 1.5)
									break;
							}
						}
						if (bastDistNu > bastDist * 1.5 && bastDistNu > 0.25)
							break;
					}
					last_i2 = i2;

					bastDist = 1e10;
					for (i2 = model.autoPath[i].nNoder - 1; i2 > last_i2; i2--) {
						x1 = model.autoPath[i].nodCoord_x[i2];
						y1 = model.autoPath[i].nodCoord_y[i2];
						bastDistNu = 1e10;
						for (i3 = 0; i3 < model.autoPath[i1].nNoder; i3++) {
							dist = getDistSeaRoute(y1, x1, model.autoPath[i1].nodCoord_y[i3], model.autoPath[i1].nodCoord_x[i3]);
							if (dist < bastDistNu) {
								bastDistNu = dist;
							}
							if (dist < bastDist) {
								bastDist = dist;
							}
							if (dist < 0.25) {
								// add arc as it is close enough...
								addAutoArcBetweenPaths(i, i2, i1, i3);
								addAutoArcBetweenPaths(i1, i3, i, i2);
							}
							else {
								if (dist > bastDistNu * 1.5)
									break;
							}
						}
						if (bastDistNu > bastDist * 1.5 && bastDistNu > 0.25)
							break;
					}

				}
			}
		}
	}

	return 0;
}

int save_tss_geojson(int ruttAlt) {
	int i, i1, forsta = 1;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/autoArcs_tssActive.geojson", model.params.indataPath.c_str());
	filpekG = fopen(namn, "w");
	free(namn);
	initGeoJsonFil(filpekG, "tss_active");

	double x, y, kvot;

	for (i = 0; i < model.nTss; i++) {
		if (forsta == 0)
			fprintf(filpekG, ", ");
		else
			forsta = 0;
		fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
		for (i1 = model.tss[i].firstTraffCoord - 1; i1 <= model.tss[i].lastTraffCoord; i1++) {
			if (i1 == model.tss[i].firstTraffCoord - 1)
				getCoordFromTss(model.tss[i], i1, model.tss[i].firstTraffCoordKvot, &y, &x);
			else {
				if (i1 == model.tss[i].lastTraffCoord)
					getCoordFromTss(model.tss[i], i1 - 1, model.tss[i].lastTraffCoordKvot, &y, &x);
				else
					getCoordFromTss(model.tss[i], i1, 0.0, &y, &x);
				fprintf(filpekG, ", ");
			}
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
		}
		fprintf(filpekG, "\n]]},\n\"properties\": {\n");
		fprintf(filpekG, "\"type\": 0, \"tssNr\": %d, \"pos1\": %d,\"kvot1\": %.3lf,\n\"pos2\": %d, \"kvot2\": %.3lf,\"posCell1\": %d,\n\"posCell2\": %d}}\n",
			i, model.tss[i].firstTraffCoord, model.tss[i].firstTraffCoordKvot, model.tss[i].lastTraffCoord,
			model.tss[i].lastTraffCoordKvot, model.tss[i].firstTraffPos, model.tss[i].lastTraffPos);
	}

	for (i = 0; i < model.paramsAutoRoute.nStartSlut - 1; i++) {
		if (forsta == 0)
			fprintf(filpekG, ", ");
		else
			forsta = 0;
		if (model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints > 1) {
			fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");

			for (i1 = 0; i1 < model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints; i1++){
				if(i1 > 0)
					fprintf(filpekG, ", ");
				x = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].x[i1];
				y = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].y[i1];
				fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			}
			fprintf(filpekG, "\n]]},\n\"properties\": {\n");
			fprintf(filpekG, "\"type\": 1, \"tssNr\": %d, \"pos1\": %d, \"pos2\": %d}}\n",
				-i - 1, i, model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints);
		}
		else {
			fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"Point\",\n\"coordinates\": \n");
			i1 = 0;
			x = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].x[i1];
			y = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].y[i1];
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			fprintf(filpekG, "\n},\n\"properties\": {\n");
			fprintf(filpekG, "\"type\": 2, \"tssNr\": %d, \"pos1\": %d, \"pos2\": %d}}\n",
				-i - 1, i, model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints);
		}
	}

	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	return 0;
}

int writeAllAutoArcsToGeojson(int iter)
{
	int i, i1, arcNr, forsta = 1;
	strAutoCells* cells;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/autoArcsBig_%d.geojson", model.params.indataPath.c_str(), iter);
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "allArcs");

	double x, y;
	model.network.last_x = -1000;

	for (i = 0; i < model.nNoder; i++) {
		if (model.Noder[i].nUtNoder > 8)
			i = i;
		for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
			if (i1 > 8)
				i1 = i1;
			arcNr = model.Noder[i].outArcNr[i1];
			if (model.arc[arcNr].fromTime >= 0 && model.arc[arcNr].fromLevel >= 0)
				continue; // short arc
			if (forsta == 1)
				forsta = 0;
			else
				fprintf(filpekG, ", ");
			fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
			if (arcNr == 133910)
				arcNr = arcNr;
			if (model.Noder[i].UtNod[i1] == 18322)
				i = i;
			getCoordFromAutoArc(arcNr, 0, &y, &x);
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			getCoordFromAutoArc(arcNr, 1, &y, &x);
			fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			fprintf(filpekG, "\n]]},\n\"properties\": {\n");
			fprintf(filpekG, "\"fromNode\": %d, \"toNode\": %d,\"arcNr\": %d,\n\"fromLevel\": %d, \"fromPointNr\": %d,\"fromTime\": %d,\n\"toLevel\": %d, \"toPointNr\": %d,\"toTime\": %d,\n\"cost\": %lf,\n\"distance\": %lf}}\n",
				i, model.Noder[i].UtNod[i1],
				arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
				model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime, model.arc[arcNr].totCost, model.arc[arcNr].distance);
		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	sprintf(namn, "%s/autoArcsSmall_%d.geojson", model.params.indataPath.c_str(), iter);
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "allArcs");

	forsta = 1;
	for (i = 0; i < model.nNoder; i++) {
		if (model.Noder[i].nUtNoder > 8)
			i = i;
		for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
			if (i1 > 8)
				i1 = i1;
			arcNr = model.Noder[i].outArcNr[i1];
			if (model.arc[arcNr].fromTime == -1)
				continue; // long arc
			if (forsta == 1)
				forsta = 0;
			else
				fprintf(filpekG, ", ");
			fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
			if (arcNr == 133910)
				arcNr = arcNr;
			if (model.Noder[i].UtNod[i1] == 18322)
				i = i;
			getCoordFromAutoArc(arcNr, 0, &y, &x);
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			getCoordFromAutoArc(arcNr, 1, &y, &x);
			fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			fprintf(filpekG, "\n]]},\n\"properties\": {\n");
			fprintf(filpekG, "\"fromNode\": %d, \"toNode\": %d,\"arcNr\": %d,\n\"fromLevel\": %d, \"fromPointNr\": %d,\"fromTime\": %d,\n\"toLevel\": %d, \"toPointNr\": %d,\"toTime\": %d,\n\"cost\": %lf,\n\"distance\": %lf}}\n",
				i, model.Noder[i].UtNod[i1],
				arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
				model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime, model.arc[arcNr].totCost, model.arc[arcNr].distance);
		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	return 0;
}

int isArcConnectedToPath(int arcNr) {
	if (model.arc[arcNr].fromTime >= 0 && model.arc[arcNr].fromLevel < 0)
		return 1;
	if (model.arc[arcNr].toTime >= 0 && model.arc[arcNr].toLevel < 0)
		return 1;

	return 0;
}

int writeAllPathArcsToGeojson()
{
	int i, i1, arcNr, forsta = 1;
	strAutoCells* cells;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/pathArcs.geojson", model.params.indataPath.c_str());
	filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "pathArcs");

	double x, y;
	model.network.last_x = -1000;

	forsta = 1;
	for (i = 0; i < model.nNoder; i++) {
		if (i == 10859)
			i = i;
		if (model.Noder[i].nUtNoder > 8)
			i = i;
		for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
			if (i1 > 8)
				i1 = i1;
			arcNr = model.Noder[i].outArcNr[i1];

			if (isArcConnectedToPath(arcNr) == 0)
				continue; 


			if (forsta == 1)
				forsta = 0;
			else
				fprintf(filpekG, ", ");
			fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
			if (arcNr == 133910)
				arcNr = arcNr;
			if (model.Noder[i].UtNod[i1] == 18322)
				i = i;
			getCoordFromAutoArc(arcNr, 0, &y, &x);
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			getCoordFromAutoArc(arcNr, 1, &y, &x);
			fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			fprintf(filpekG, "\n]]},\n\"properties\": {\n");
			fprintf(filpekG, "\"fromNode\": %d, \"toNode\": %d,\"arcNr\": %d,\n\"fromLevel\": %d, \"fromPointNr\": %d,\"fromTime\": %d,\n\"toLevel\": %d, \"toPointNr\": %d,\"toTime\": %d,\n\"cost\": %lf,\n\"distance\": %lf}}\n",
				i, model.Noder[i].UtNod[i1],
				arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
				model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime, model.arc[arcNr].totCost, model.arc[arcNr].distance);
		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	return 0;
}

int set_nXY_smallCells(strAutoCells* cell) {

	if (cell->smallerCellsType == 1 || cell->smallerCellsType == 4) {
		cell->nXsmall = model.paramsAutoRoute.nDiscreteSizeLevel[1];
		cell->nYsmall = model.paramsAutoRoute.nDiscreteSizeLevel[1];
	}
	else {
		cell->nXsmall = 1;
		cell->nYsmall = 1;
		if (cell->smallerCellsType == 2)
			cell->nXsmall = model.paramsAutoRoute.nDiscreteSizeLevel[1];
		if (cell->smallerCellsType == 3)
			cell->nYsmall = model.paramsAutoRoute.nDiscreteSizeLevel[1];
	}
	return 0;
}


int get_nextHeltal(double x, double dx) {
	int heltal;
	heltal = (int)x;
	if (x < 0 && heltal > x)
		heltal--;
	if (dx >= 0) {
		if (heltal < x)
			heltal++;
	}
	return heltal;
}

double get_nextKvotHeltal(double x, double xBas, double dx) {
	int nextHeltal = get_nextHeltal(x, dx);
	double kvot;
	if (abs(dx) > 0.0001)
		kvot = (nextHeltal - xBas) / dx;
	else
		kvot = 1e10;
	return kvot;
}

void addSmallCellToSearchSet(int cell, int cellSmall, int posTmp) {
	int pos = model.paramsAutoRoute.nInSet[posTmp];
	model.paramsAutoRoute.setCell[posTmp][pos] = cell;
	model.paramsAutoRoute.setSmallCell[posTmp][pos] = cellSmall;
	(model.paramsAutoRoute.nInSet[posTmp])++;
}

int addNeighboursToCell(int cell, int cellSmall, int valNu, int posTmp) {
	int i, i1, yNu, xNu, cellBas, cellNu, cellSmallNu;
	int yPos0 = cell / model.paramsAutoRoute.nXbasLevel;
	int xPos0 = cell - yPos0 * model.paramsAutoRoute.nXbasLevel;

	int yPos = cellSmall / model.autoRoute[cell].nXsmall;
	int xPos = cellSmall - yPos * model.autoRoute[cell].nXsmall;
	for (i = -1; i <= 1; i++) {
		yNu = yPos + i;
		cellBas = cell;
		if (yNu < 0) {
			if (yPos0 == 0)
				continue; // outside area
			yNu += model.autoRoute[cell].nYsmall;
			cellBas -= model.paramsAutoRoute.nXbasLevel;
		}
		else {
			if (yNu >= model.autoRoute[cell].nYsmall) {
				if (yPos0 >= model.paramsAutoRoute.nYbasLevel - 1)
					continue; // outside area
				yNu -= model.autoRoute[cell].nYsmall;
				cellBas += model.paramsAutoRoute.nXbasLevel;
			}
		}
		for (i1 = -1; i1 <= 1; i1++) {
			if (i == 0 && i1 == 0)
				continue;
			xNu = xPos + i1;
			cellNu = cellBas;
			if (xNu < 0) {
				if (xPos0 == 0)
					continue; // outside area
				xNu += model.autoRoute[cell].nXsmall;
				cellNu--;
			}
			else {
				if (xNu >= model.autoRoute[cell].nXsmall) {
					if (xPos0 >= model.paramsAutoRoute.nXbasLevel - 1)
						continue; // outside area
					xNu -= model.autoRoute[cell].nXsmall;
					cellNu++;
				}
			}
			if (model.autoRoute[cellNu].use == 1 && model.autoRoute[cellNu].isLand == 0) {
				cellSmallNu = xNu + model.autoRoute[cell].nXsmall * yNu;
				if (model.autoRoute[cellNu].smallerCells[cellSmallNu].use == 0) {
					model.autoRoute[cellNu].smallerCells[cellSmallNu].use = valNu;
					addSmallCellToSearchSet(cellNu, cellSmallNu, posTmp);
				}
			}
		}
	}
	return 0;
}

int get_xyPos_fromAutoRoutePos(int cell, int* xPos, int* yPos) {

	// 			pos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	int pos1 = (int)(cell / model.paramsAutoRoute.nXbasLevel);
	*yPos = pos1;
	*xPos = cell - pos1 * model.paramsAutoRoute.nXbasLevel;

	return 0;
}

int addSmallCellsClosestToSeaRoute(int cellOld, double y0, double x0, int cell, double y1, double x1, int iPos, int* nCellsUsed) {
	double xNu, yNu, dx, dy, kvot = 0.0, next_xKvot, next_yKvot;
	int i, xPos, yPos, cellUse, cellSmall, yPosNu, xPosNu;

	y0 *= model.autoRoute[cell].nYsmall;
	y1 *= model.autoRoute[cell].nYsmall;
	x0 *= model.autoRoute[cell].nXsmall;
	x1 *= model.autoRoute[cell].nXsmall;

	dx = x1 - x0;
	dy = y1 - y0;
	xNu = x0;
	yNu = y0;

	if (cell == 1031)
		cell = cell;

	for (i = 0;; i++) {
		xPos = round(xNu);
		yPos = round(yNu);
		cellUse = cell;
		if (xPos < 0 || yPos < 0 || xPos >= model.autoRoute[cell].nXsmall ||
			yPos >= model.autoRoute[cell].nYsmall)
		{
			if (xPos < 0) {
				cellUse--;
				xPos += model.autoRoute[cell].nXsmall;
				if (xPos < 0)
					xPos = 0;
			}
			else {
				if (xPos >= model.autoRoute[cell].nXsmall) {
					cellUse++;
					xPos -= model.autoRoute[cell].nXsmall;
					if(xPos >= model.autoRoute[cell].nXsmall)
						xPos = model.autoRoute[cell].nXsmall - 1;
				}
			}
			if (yPos < 0) {
				cellUse -= model.paramsAutoRoute.nXbasLevel;
				yPos += model.autoRoute[cell].nYsmall;
				if (yPos < 0)
					yPos = 0;
			}
			else {
				if (yPos >= model.autoRoute[cell].nYsmall) {
					cellUse += model.paramsAutoRoute.nXbasLevel;
					yPos -= model.autoRoute[cell].nYsmall;
					if (yPos >= model.autoRoute[cell].nYsmall)
						yPos = model.autoRoute[cell].nYsmall - 1;
				}
			}
		}

		if (model.autoRoute[cellUse].use == 0) {
			model.autoRoute[cellUse].firstUsePathPos = iPos;
			model.autoRoute[cellUse].use = 1;
			(*nCellsUsed)++;

			get_xyPos_fromAutoRoutePos(cellUse, &xPosNu, &yPosNu);
			addSmallerCellsToCell(cellUse, yPosNu, xPosNu);
		}
		if (model.autoRoute[cellUse].smallerCellsType > 0) {
			cellSmall = xPos + model.autoRoute[cell].nXsmall * yPos;
			if (cellUse == 26 && cellSmall == 108)
				xPos = xPos;
			if (model.autoRoute[cellUse].smallerCells[cellSmall].use == 0) {
				model.autoRoute[cellUse].smallerCells[cellSmall].use = 1;
				addSmallCellToSearchSet(cellUse, cellSmall, 0);
			}
		}
		//}
		//else
		//	errlog("Skipping cell %d, not used yet...\n", cellUse);

		if (kvot > 0.9995)
			break;

		next_xKvot = get_nextKvotHeltal(xNu, x0, dx);
		next_yKvot = get_nextKvotHeltal(yNu, y0, dy);
		if (next_xKvot < next_yKvot)
			kvot = next_xKvot;
		else
			kvot = next_yKvot;
		kvot += 0.001;
		if (kvot > 1)
			kvot = 1;
		xNu = x0 + kvot * dx;
		yNu = y0 + kvot * dy;

	}
	return 0;
}

int addSmallerCellsToCell(int pos, int i, int i1) {
	int isLand, i2;
	double x, y;

	if (pos == 654)
		pos = pos;
	model.autoRoute[pos].smallerCells = NULL;
	model.autoRoute[pos].smallerCellsType = 0;
	if (model.autoRoute[pos].use == 1) {
		x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
		y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
		model.autoRoute[pos].x = x;
		model.autoRoute[pos].y = y;
		if (pos == 20)
			pos = pos;
		isLand = checkIsCompleteLand(y, x, y + model.paramsAutoRoute.discretizationSizeLevel[0], x + model.paramsAutoRoute.discretizationSizeLevel[0]);
		model.autoRoute[pos].isLand = isLand;
		evalCostArc_cells(&(model.autoRoute[pos]), 0, y, x, y, x + model.paramsAutoRoute.discretizationSizeLevel[0]); // horizontal
		evalCostArc_cells(&(model.autoRoute[pos]), 1, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[0], x + model.paramsAutoRoute.discretizationSizeLevel[0]); // diagonal up
		evalCostArc_cells(&(model.autoRoute[pos]), 2, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[0], x); // vertical
		evalCostArc_cells(&(model.autoRoute[pos]), 3, y, x + model.paramsAutoRoute.discretizationSizeLevel[0], y + model.paramsAutoRoute.discretizationSizeLevel[0], x); // diagonal down

		if (pos == 23)
			pos = pos;
		if (isLand == 0) {
			if (0 < model.paramsAutoRoute.nCellLevels - 1) {
				for (i2 = 0; i2 < 4; i2++) {
					if (model.autoRoute[pos].arcCost[i2] > model.paramsAutoRoute.maxBaseFeasibleCost) {
						break;
					}
				}
				//if (i2 < 4 || posCellStart == pos || posCellEnd == pos) {
				model.autoRoute[pos].smallerCellsType = 1;
				set_nXY_smallCells(&(model.autoRoute[pos]));
				model.autoRoute[pos].smallerCells = addSmallerCells(&(model.autoRoute[pos]), x, y, 1);
				//}
			}
		}
	}

	return 0;
}

int createCells_new() {
	openNeededRasterFilesNew(-11);

	// model.paramsAutoRoute.x_min = model.boundingBox.xMin;
	model.paramsAutoRoute.nXbasLevel = roundUp((model.boundingBox.xMax - model.paramsAutoRoute.x_min + model.paramsAutoRoute.discretizationSizeLevel[0] * 0.5) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	// model.paramsAutoRoute.y_min = model.boundingBox.yMin;
	model.paramsAutoRoute.nYbasLevel = roundUp((model.boundingBox.yMax - model.paramsAutoRoute.y_min + model.paramsAutoRoute.discretizationSizeLevel[0] * 0.5) / model.paramsAutoRoute.discretizationSizeLevel[0]);

	int iPos, arcNr, nod, i, i1, pos, i2, posTmp, posCellStart, posCellEnd, yPos, xPos, level, isLand;
	double x, y, xTmp, yTmp, yPosDbl, xPosDbl, dx, dy, xNu, yNu, kvot, next_xKvot, next_yKvot, yPosDbl_prev, xPosDbl_prev;
	int nAlloc = (model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel + 2);
	int lastPos, nCellsUsed = 0;
	double yPosDbl_prev0, xPosDbl_prev0;
	model.autoRoute = (strAutoCells*)malloc(nAlloc * sizeof(strAutoCells)); // +2 since start and end node as well
	for (i = 0; i < nAlloc; i++) {
		model.autoRoute[i].use = 0;
		model.autoRoute[i].smallerCellsType = 0;
		model.autoRoute[i].smallerCells = NULL;
	}

	nAlloc = modelSea.Dijkstra.OptCost / model.paramsAutoRoute.discretizationSizeLevel[0] * 3;
	i = 0;
	if (nAlloc > model.paramsAutoRoute.nAllocSet[i]) {
		if (model.paramsAutoRoute.nAllocSet[i] == 0) {
			model.paramsAutoRoute.nAllocSet[i] = nAlloc;
			model.paramsAutoRoute.setCell[i] = (int*)malloc(nAlloc * sizeof(int));
			model.paramsAutoRoute.setSmallCell[i] = (int*)malloc(nAlloc * sizeof(int));
		}
		else {
			model.paramsAutoRoute.nAllocSet[i] = nAlloc;
			model.paramsAutoRoute.setCell[i] = (int*)realloc(model.paramsAutoRoute.setCell[i], nAlloc * sizeof(int));
			model.paramsAutoRoute.setSmallCell[i] = (int*)realloc(model.paramsAutoRoute.setSmallCell[i], nAlloc * sizeof(int));
		}
	}
	model.paramsAutoRoute.nInSet[i] = 0;


	lastPos = -1;
	for (iPos = -1; iPos <= modelSea.nBVArcsUse; iPos++)
	{
		if (iPos < 0) {
			x = model.paramsAutoRoute.startPoint_lon[0];
			y = model.paramsAutoRoute.startPoint_lat[0];
		}
		else {
			if (iPos < modelSea.nBVArcsUse) {
				arcNr = modelSea.BVArcUse[iPos];
				nod = modelSea.arc[arcNr].toPointNr;
				y = modelSea.seaRoute.nod_y[nod];
				x = modelSea.seaRoute.nod_x[nod];
			}
			else {
				x = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
				y = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
			}
		}
		if (iPos == 36)
			iPos = iPos;
		if (x < 13)
			x = x;
		yPosDbl = (y - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		x = getUsable_x(x);
		xPosDbl = (x - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		if (iPos >= 0) {
			dx = xPosDbl - xPosDbl_prev;
			dy = yPosDbl - yPosDbl_prev;

			xNu = xPosDbl_prev;
			yNu = yPosDbl_prev;
			yPosDbl_prev0 = yNu;
			xPosDbl_prev0 = xNu;
			kvot = 0;
			if (iPos == 18)
				y = y;
			for (i = 0;; i++) {
				xPos = (int)xNu;
				yPos = (int)yNu;
				pos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
				if (model.autoRoute[pos].use == 0) {
					model.autoRoute[pos].firstUsePathPos = iPos;
					model.autoRoute[pos].use = 1;
					nCellsUsed++;
					addSmallerCellsToCell(pos, yPos, xPos);
				}
				model.autoRoute[pos].lastUsePathPos = iPos;

				if (model.autoRoute[pos].smallerCellsType > 0) {
					if(lastPos >= 0)
						addSmallCellsClosestToSeaRoute(lastPos, yPosDbl_prev0 - yPos, xPosDbl_prev0 - xPos, pos, yNu - yPos, xNu - xPos, iPos, &nCellsUsed);
					lastPos = pos;
					yPosDbl_prev0 = yNu;
					xPosDbl_prev0 = xNu;

					if (xNu - xPos < 0.5 && xPos > 0) {
						pos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * yPos;
						if (model.autoRoute[pos].use == 0) {
							model.autoRoute[pos].firstUsePathPos = iPos;
							model.autoRoute[pos].use = 1;
							nCellsUsed++;
							addSmallerCellsToCell(pos, yPos, xPos - 1);
						}
						model.autoRoute[pos].lastUsePathPos = iPos;

						if (yNu - yPos < 0.5 && yPos > 0) {
							// diagonally below left
							pos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
							if (model.autoRoute[pos].use == 0) {
								model.autoRoute[pos].firstUsePathPos = iPos;
								model.autoRoute[pos].use = 1;
								nCellsUsed++;
								addSmallerCellsToCell(pos, yPos - 1, xPos - 1);
							}
							model.autoRoute[pos].lastUsePathPos = iPos;
						}
						else {
							// diagonally above left
							if (yNu - yPos > 0.5 && yPos < model.paramsAutoRoute.nYbasLevel - 1) {
								pos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
								if (model.autoRoute[pos].use == 0) {
									model.autoRoute[pos].firstUsePathPos = iPos;
									model.autoRoute[pos].use = 1;
									nCellsUsed++;
									addSmallerCellsToCell(pos, yPos + 1, xPos - 1);
								}
								model.autoRoute[pos].lastUsePathPos = iPos;
							}
						}

					}
					else {
						if (xNu - xPos > 0.5 && xPos < model.paramsAutoRoute.nXbasLevel - 1) {
							pos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * yPos;
							if (model.autoRoute[pos].use == 0) {
								model.autoRoute[pos].firstUsePathPos = iPos;
								model.autoRoute[pos].use = 1;
								nCellsUsed++;
								addSmallerCellsToCell(pos, yPos, xPos + 1);
							}
							model.autoRoute[pos].lastUsePathPos = iPos;

							if (yNu - yPos < 0.5 && yPos > 0) {
								// diagonally below right
								pos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
								if (model.autoRoute[pos].use == 0) {
									model.autoRoute[pos].firstUsePathPos = iPos;
									model.autoRoute[pos].use = 1;
									nCellsUsed++;
									addSmallerCellsToCell(pos, yPos - 1, xPos + 1);
								}
								model.autoRoute[pos].lastUsePathPos = iPos;
							}
							else {
								// diagonally above right
								if (yNu - yPos > 0.5 && yPos < model.paramsAutoRoute.nYbasLevel - 1) {
									pos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
									if (model.autoRoute[pos].use == 0) {
										model.autoRoute[pos].firstUsePathPos = iPos;
										model.autoRoute[pos].use = 1;
										nCellsUsed++;
										addSmallerCellsToCell(pos, yPos + 1, xPos + 1);
									}
									model.autoRoute[pos].lastUsePathPos = iPos;
								}
							}
						}
					}
					if (yNu - yPos < 0.5 && yPos > 0) {
						pos = xPos + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
						if (model.autoRoute[pos].use == 0) {
							model.autoRoute[pos].firstUsePathPos = iPos;
							model.autoRoute[pos].use = 1;
							nCellsUsed++;
							addSmallerCellsToCell(pos, yPos - 1, xPos);
						}
						model.autoRoute[pos].lastUsePathPos = iPos;
					}
					else {
						if (yNu - yPos > 0.5 && yPos < model.paramsAutoRoute.nYbasLevel - 1) {
							pos = xPos + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
							if (model.autoRoute[pos].use == 0) {
								model.autoRoute[pos].firstUsePathPos = iPos;
								model.autoRoute[pos].use = 1;
								nCellsUsed++;
								addSmallerCellsToCell(pos, yPos + 1, xPos);
							}
							model.autoRoute[pos].lastUsePathPos = iPos;
						}
					}

				}
				if (kvot > 0.9995)
					break;

				next_xKvot = get_nextKvotHeltal(xNu, xPosDbl_prev, dx);
				next_yKvot = get_nextKvotHeltal(yNu, yPosDbl_prev, dy);
				if (next_xKvot < next_yKvot)
					kvot = next_xKvot;
				else
					kvot = next_yKvot;
				kvot += 0.001;
				if (kvot > 1)
					kvot = 1;
				xNu = xPosDbl_prev + kvot * dx;
				yNu = yPosDbl_prev + kvot * dy;

			}

		}
		yPosDbl_prev = yPosDbl;
		xPosDbl_prev = xPosDbl;
	}

	nAlloc = nCellsUsed * model.paramsAutoRoute.nDiscreteSizeLevel[1] * model.paramsAutoRoute.nDiscreteSizeLevel[1];
	for (i = 0; i < 2; i++) {
		if (nAlloc > model.paramsAutoRoute.nAllocSet[i]) {
			if (model.paramsAutoRoute.nAllocSet[i] == 0) {
				model.paramsAutoRoute.nAllocSet[i] = nAlloc;
				model.paramsAutoRoute.setCell[i] = (int*)malloc(nAlloc * sizeof(int));
				model.paramsAutoRoute.setSmallCell[i] = (int*)malloc(nAlloc * sizeof(int));
			}
			else {
				model.paramsAutoRoute.nAllocSet[i] = nAlloc;
				model.paramsAutoRoute.setCell[i] = (int*)realloc(model.paramsAutoRoute.setCell[i], nAlloc * sizeof(int));
				model.paramsAutoRoute.setSmallCell[i] = (int*)realloc(model.paramsAutoRoute.setSmallCell[i], nAlloc * sizeof(int));
			}
		}
	}

	int posNu, posNext;
	posNu = 0;
	for (i = 2; i < 1000; i++) {
		if (posNu == 0)
			posNext = 1;
		else
			posNext = 0;
		model.paramsAutoRoute.nInSet[posNext] = 0;
		for (i1 = 0; i1 < model.paramsAutoRoute.nInSet[posNu]; i1++) {
			addNeighboursToCell(model.paramsAutoRoute.setCell[posNu][i1],
				model.paramsAutoRoute.setSmallCell[posNu][i1], i, posNext);
		}
		if (model.paramsAutoRoute.nInSet[posNext] == 0)
			break;
		if (posNu == 0)
			posNu = 1;
		else
			posNu = 0;
	}

	/*
	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (pos == 171)
				pos = pos;
			model.autoRoute[pos].smallerCells = NULL;
			model.autoRoute[pos].smallerCellsType = 0;
			if(model.autoRoute[pos].use == 1){
				x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
				y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
				model.autoRoute[pos].x = x;
				model.autoRoute[pos].y = y;
				if (pos == 20)
					pos = pos;
				isLand = checkIsCompleteLand(y, x, y + model.paramsAutoRoute.discretizationSizeLevel[0], x + model.paramsAutoRoute.discretizationSizeLevel[0]);
				model.autoRoute[pos].isLand = isLand;
				evalCostArc_cells(&(model.autoRoute[pos]), 0, y, x, y, x + model.paramsAutoRoute.discretizationSizeLevel[0]); // horizontal
				evalCostArc_cells(&(model.autoRoute[pos]), 1, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[0], x + model.paramsAutoRoute.discretizationSizeLevel[0]); // diagonal up
				evalCostArc_cells(&(model.autoRoute[pos]), 2, y, x, y + model.paramsAutoRoute.discretizationSizeLevel[0], x); // vertical
				evalCostArc_cells(&(model.autoRoute[pos]), 3, y, x + model.paramsAutoRoute.discretizationSizeLevel[0], y + model.paramsAutoRoute.discretizationSizeLevel[0], x); // diagonal down

				if (pos == 23)
					pos = pos;
				if (isLand == 0) {
					if (0 < model.paramsAutoRoute.nCellLevels - 1) {
						for (i2 = 0; i2 < 4; i2++) {
							if (model.autoRoute[pos].arcCost[i2] > model.paramsAutoRoute.maxBaseFeasibleCost) {
								break;
							}
						}
						//if (i2 < 4 || posCellStart == pos || posCellEnd == pos) {
							model.autoRoute[pos].smallerCellsType = 1;
							set_nXY_smallCells(&(model.autoRoute[pos]));
							model.autoRoute[pos].smallerCells = addSmallerCells(&(model.autoRoute[pos]), x, y, 1);
							//}
					}
				}
			}
			pos++;
		}
	}
	*/


	return 0;
}

int addAutoNode(int cellNr, int posIcell, int cellNr2) {

	if (model.nNoder >= model.nAllocNoder) {
		model.nAllocNoder += 100000;
		model.Noder = (strNoder*)realloc(model.Noder, model.nAllocNoder * sizeof(strNoder));
	}

	model.Noder[model.nNoder].physicalLevel = cellNr;
	model.Noder[model.nNoder].pointNr = posIcell;
	model.Noder[model.nNoder].timeInterval = cellNr2;
	model.Noder[model.nNoder].nAllocUtNoder = 0;
	model.Noder[model.nNoder].nUtNoder = 0;
	if (cellNr2 < 0)
		model.autoRoute[cellNr].nodeNr[posIcell] = model.nNoder;
	else
		model.autoRoute[cellNr].smallerCells[cellNr2].nodeNr[posIcell] = model.nNoder;

	(model.nNoder)++;
	return model.nNoder - 1;
}

int addAutoNodePath(int pathNr, double y, double x) {
	int posIpath;
	if (model.nNoder >= model.nAllocNoder) {
		model.nAllocNoder += 100000;
		model.Noder = (strNoder*)realloc(model.Noder, model.nAllocNoder * sizeof(strNoder));
	}

	posIpath = model.autoPath[pathNr].nNoder;
	if (posIpath>= model.autoPath[pathNr].nAllocNoder) {
		model.autoPath[pathNr].nAllocNoder += 100;
		model.autoPath[pathNr].nodNr = (int*)realloc(model.autoPath[pathNr].nodNr, model.autoPath[pathNr].nAllocNoder * sizeof(int));
		model.autoPath[pathNr].nodCoord_y = (double*)realloc(model.autoPath[pathNr].nodCoord_y, model.autoPath[pathNr].nAllocNoder * sizeof(double));
		model.autoPath[pathNr].nodCoord_x = (double*)realloc(model.autoPath[pathNr].nodCoord_x, model.autoPath[pathNr].nAllocNoder * sizeof(double));
	}
	model.Noder[model.nNoder].physicalLevel = -1;
	model.Noder[model.nNoder].pointNr = pathNr;
	model.Noder[model.nNoder].timeInterval = posIpath;
	model.Noder[model.nNoder].nAllocUtNoder = 0;
	model.Noder[model.nNoder].nUtNoder = 0;
	model.autoPath[pathNr].nodNr[posIpath] = model.nNoder;

	model.autoPath[pathNr].nodCoord_y[posIpath] = y;
	model.autoPath[pathNr].nodCoord_x[posIpath] = x;

	(model.autoPath[pathNr].nNoder)++;

	(model.nNoder)++;
	return model.nNoder - 1;
}

//double addAutoArc(int cellPos1, int posTmp1, int level1, int cellPos2, int cellPos3, int posTmp2, int level2, int posUse) {
double addAutoArc(int cellPos1, int posTmp1, int cellPos2, int posTmp2, int posUse) {
	double dist, cost;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 100000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	if (model.nArcs == 385)
		model.nArcs = model.nArcs;
	int cellUse;
	if (cellPos1 >= model.paramsAutoRoute.nCellsBase || cellPos2 >= model.paramsAutoRoute.nCellsBase) {
		if (cellPos1 >= cellPos2)
			cellUse = cellPos1;
		else
			cellUse = cellPos2;
	}
	else {
		if (cellPos1 < cellPos2)
			cellUse = cellPos1;
		else
			cellUse = cellPos2;
	}

	model.arc[model.nArcs].fromLevel = cellPos1;
	model.arc[model.nArcs].fromPointNr = posTmp1;
	model.arc[model.nArcs].fromTime = -1;

	model.arc[model.nArcs].toLevel = cellPos2;
	model.arc[model.nArcs].toTime = -1;
	dist = model.autoRoute[cellUse].arcDistance[posUse];
	cost = model.autoRoute[cellUse].arcCost[posUse];

	model.arc[model.nArcs].toPointNr = posTmp2;
	model.arc[model.nArcs].distance = dist;
	model.arc[model.nArcs].totCost = cost;

	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;
	return cost;
}

double addAutoArcSmall(int cellPos1, int cellSmall1, int posTmp1, int cellPos2, int cellSmall2, int posTmp2, int posUse) {
	double dist, cost;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	if (model.nArcs == 15472)
		model.nArcs = model.nArcs;
	int cellUse, cellSmallUse;
	if (cellPos1 >= model.paramsAutoRoute.nCellsBase || cellPos2 >= model.paramsAutoRoute.nCellsBase) {
		if (cellPos1 >= cellPos2) {
			cellUse = cellPos1;
			cellSmallUse = 0;
		}
		else {
			cellUse = cellPos2;
			cellSmallUse = 0;
		}
		dist = model.autoRoute[cellUse].arcDistance[posUse];
		cost = 1.001 * model.autoRoute[cellUse].arcCost[posUse];
	}
	else {
		if (cellPos1 < cellPos2) {
			cellUse = cellPos1;
			cellSmallUse = cellSmall1;
		}
		else {
			cellUse = cellPos2;
			cellSmallUse = cellSmall2;
		}
		dist = model.autoRoute[cellUse].smallerCells[cellSmallUse].arcDistance[posUse];
		cost = 1.001 * model.autoRoute[cellUse].smallerCells[cellSmallUse].arcCost[posUse];
	}
	if (model.nArcs == 354521 || model.nArcs == 368139)
		cellUse = cellUse;
	cost += model.autoRoute[cellPos2].smallerCells[cellSmall2].use * 0.1; // 10.0; //  0.0001;
	model.arc[model.nArcs].fromLevel = cellPos1;
	model.arc[model.nArcs].fromPointNr = posTmp1;
	model.arc[model.nArcs].fromTime = cellSmall1;
	model.arc[model.nArcs].toLevel = cellPos2;
	model.arc[model.nArcs].toTime = cellSmall2;

	//if (model.nArcs == 1297 || model.nArcs == 366)
	//	cost *= 1000;

	model.arc[model.nArcs].toPointNr = posTmp2;
	model.arc[model.nArcs].distance = dist;
	model.arc[model.nArcs].totCost = cost;

	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;
	return cost;
}

double addAutoArcSmallPath(int pathNr, int posItss, int prev_posItss, int cellPos, int cellSmall, int posTmp, double dist, int direction, double costBas) {
	double cost, kvotCost = 1.0, costFactorArea, x2, y2, x1, y1;
	int xIndex;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	if (model.nArcs == 600202)
		model.nArcs = model.nArcs;

	if (direction == -1) { // in to tss
		if (prev_posItss >= 0) {
			model.arc[model.nArcs].fromLevel = -1;
			model.arc[model.nArcs].fromPointNr = pathNr;
			model.arc[model.nArcs].fromTime = prev_posItss;
			if (cellPos == -1)
				kvotCost = model.autoPath[pathNr].kvotCost;
			else
				kvotCost = 1.0;
			//if (model.autoPath[pathNr].type == 0)
			//	kvotCost = 0.01;
			//else {
			//	if (model.autoPath[pathNr].type == 1)
			//		kvotCost = 0.5;
			//	else
			//		kvotCost = 1.0;
			//}
		}
		else {
			model.arc[model.nArcs].fromLevel = cellPos;
			model.arc[model.nArcs].fromPointNr = posTmp;
			model.arc[model.nArcs].fromTime = cellSmall;
		}
		model.arc[model.nArcs].toLevel = -1;
		model.arc[model.nArcs].toPointNr = pathNr;
		model.arc[model.nArcs].toTime = posItss;
	}
	else {
		model.arc[model.nArcs].fromLevel = -1;
		model.arc[model.nArcs].fromPointNr = pathNr;
		model.arc[model.nArcs].fromTime = posItss;
		model.arc[model.nArcs].toLevel = cellPos;
		model.arc[model.nArcs].toTime = cellSmall;
		model.arc[model.nArcs].toPointNr = posTmp;
	}
	model.arc[model.nArcs].distance = dist;
	cost = kvotCost * costBas;
	getCoordFromAutoArc(model.nArcs, 0, &y1, &x1);
	getCoordFromAutoArc(model.nArcs, 1, &y2, &x2);
	if (model.autoPath[pathNr].type != 11)
		costFactorArea = getCostFactorArea(y1, x1, y2, x2, &xIndex);
	else
		costFactorArea = 1;
	model.arc[model.nArcs].totCost = cost * costFactorArea;

	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;
	return cost;
}

double addAutoArcBetweenPaths(int path1, int posPath1, int path2, int posPath2){
	double cost, kvotCost = 1.0, costFactorArea, x2, y2, x1, y1;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	int nodNr = model.autoPath[path1].nodNr[posPath1];
	int nodNr2 = model.autoPath[path2].nodNr[posPath2];
	int nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodNr2;
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;

	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	model.arc[model.nArcs].fromLevel = -1;
	model.arc[model.nArcs].fromPointNr = path1;
	model.arc[model.nArcs].fromTime = posPath1;
	model.arc[model.nArcs].toLevel = -1;
	model.arc[model.nArcs].toPointNr = path2;
	model.arc[model.nArcs].toTime = posPath2;

	if(model.autoPath[path1].kvotCost < model.autoPath[path2].kvotCost)
		kvotCost = model.autoPath[path2].kvotCost;
	else
		kvotCost = model.autoPath[path1].kvotCost;
	if (model.nArcs == 108942)
		path1 = path1;
	//if (model.autoPath[path2].type == 0)
	//	kvotCost = 0.01;
	//else {
	//	if (model.autoPath[path2].type == 1)
	//		kvotCost = 0.5;
	//	else
	//		kvotCost = 1.0;
	//}

	double dist;
	y1 = model.autoPath[path1].nodCoord_y[posPath1];
	x1 = model.autoPath[path1].nodCoord_x[posPath1];
	y2 = model.autoPath[path2].nodCoord_y[posPath2];
	x2 = model.autoPath[path2].nodCoord_x[posPath2];
	if (model.nArcs == 83502 || model.nArcs == 83501)
		path1 = path1;
	cost = evalCostArc(y1, x1, y2, x2, &dist);
	model.arc[model.nArcs].distance = dist;
	cost *= kvotCost;
	model.arc[model.nArcs].totCost = cost;

	model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	model.Noder[nodNr].nUtNoder = nodPos;

	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;

	return 0;
}

int checkAllocNode(int nodNr) {
	if (model.Noder[nodNr].nAllocUtNoder == 0) {
		model.Noder[nodNr].nAllocUtNoder = 9;
		model.Noder[nodNr].UtNod = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		model.Noder[nodNr].UtNodCost = (double*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(double));
		model.Noder[nodNr].outArcNr = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
	}
	else {
		if (model.Noder[nodNr].nUtNoder >= model.Noder[nodNr].nAllocUtNoder) {
			model.Noder[nodNr].nAllocUtNoder += 8;
			model.Noder[nodNr].UtNod = (int*)realloc(model.Noder[nodNr].UtNod, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
			model.Noder[nodNr].UtNodCost = (double*)realloc(model.Noder[nodNr].UtNodCost, model.Noder[nodNr].nAllocUtNoder * sizeof(double));
			model.Noder[nodNr].outArcNr = (int*)realloc(model.Noder[nodNr].outArcNr, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		}
	}
	return 0;
}

int addAutoArcExtra(int arcNr1, int arcNr2, int nod1, int nod2, double dist, double cost) {
	int nodPos;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	if (model.nArcs >= 506637)
		model.nArcs = model.nArcs;

	model.arc[model.nArcs].fromLevel = model.arc[arcNr1].fromLevel;
	model.arc[model.nArcs].fromPointNr = model.arc[arcNr1].fromPointNr;
	model.arc[model.nArcs].fromTime = model.arc[arcNr1].fromTime;
	model.arc[model.nArcs].toLevel = model.arc[arcNr2].toLevel;
	model.arc[model.nArcs].toPointNr = model.arc[arcNr2].toPointNr;
	model.arc[model.nArcs].toTime = model.arc[arcNr2].toTime;
	model.arc[model.nArcs].distance = dist;
	model.arc[model.nArcs].totCost = cost;

	checkAllocNode(nod1);
	nodPos = model.Noder[nod1].nUtNoder;
	model.Noder[nod1].UtNod[nodPos] = nod2;
	model.Noder[nod1].outArcNr[nodPos] = model.nArcs;
	model.Noder[nod1].UtNodCost[nodPos++] = cost;
	model.Noder[nod1].nUtNoder = nodPos;

	if (model.nArcs == 83502 || model.nArcs == 83501)
		nod1 = nod1;

	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;
	return 0;
}

double addAutoArcSmall2(int cellPos1, int cellSmall1, int posTmp1, int cellPos2, int cellSmall2, int posTmp2, double cost, double dist) {
	double costUse;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	if (model.nArcs == 15472)
		model.nArcs = model.nArcs;
	//dist = model.autoRoute[cellUse].arcDistance[posUse];
	costUse = 1.001 * cost; // model.autoRoute[cellUse].arcCost[posUse];

	model.arc[model.nArcs].fromLevel = cellPos1;
	model.arc[model.nArcs].fromPointNr = posTmp1;
	model.arc[model.nArcs].fromTime = cellSmall1;
	model.arc[model.nArcs].toLevel = cellPos2;
	model.arc[model.nArcs].toTime = cellSmall2;

	//if (model.nArcs == 1297 || model.nArcs == 366)
	//	cost *= 1000;

	model.arc[model.nArcs].toPointNr = posTmp2;
	model.arc[model.nArcs].distance = dist;
	model.arc[model.nArcs].totCost = cost;

	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;
	return cost;
}


double getPosDiagonalForDistCost(int cellNr, int yPos1, int xPos1, int cellSmall1, int yPos2, int xPos2, int direction, int* cellNrUse, int* cellSmallUse, int* nodPosUse, double* dist) {
	double x2, y2, y1, x1, cost;
	int xPosUse1, xPosUse2, yPosUse1, yPosUse2;

	xPosUse2 = xPos2;
	yPosUse2 = yPos2;
	xPosUse1 = xPos1;
	yPosUse1 = yPos1;

	if (direction == 0) { // right
		*nodPosUse = 1;
	}
	if (direction == 1) { // right up
		*nodPosUse = 3;
	}
	if (direction == 2) { // up
		*nodPosUse = 2;
	}
	if (direction == 3) { // left up
		xPosUse2 -= 1;
		*nodPosUse = 2;
	}
	if (direction == 4) { // left 
		xPosUse2 -= 1;
		*nodPosUse = 0;
	}
	if (direction == 5) { // left down
		xPosUse2 -= 1;
		yPosUse2 -= 1;
		*nodPosUse = 0;
	}
	if (direction == 6) { // down
		yPosUse2 -= 1;
		*nodPosUse = 0;
	}
	if (direction == 7) { // right down
		yPosUse2 -= 1;
		*nodPosUse = 1;
	}

	if (direction == 8) { // right right up
		xPosUse2 += 2;
		yPosUse2 += 1;
		*nodPosUse = 0;
	}
	if (direction == 9) { // right up up
		xPosUse2 += 1;
		yPosUse2 += 2;
		*nodPosUse = 0;
	}
	if (direction == 10) { // left up up
		xPosUse2 -= 1;
		yPosUse2 += 2;
		*nodPosUse = 0;
	}
	if (direction == 11) { // left left up
		xPosUse2 -= 2;
		yPosUse2 += 1;
		*nodPosUse = 0;
	}
	if (direction == 12) { // left left down
		xPosUse2 -= 2;
		yPosUse2 -= 1;
		*nodPosUse = 0;
	}
	if (direction == 13) { // left down down
		xPosUse2 -= 1;
		yPosUse2 -= 2;
		*nodPosUse = 0;
	}
	if (direction == 14) { // right down down
		xPosUse2 += 1;
		yPosUse2 -= 2;
		*nodPosUse = 0;
	}
	if (direction == 15) { // right right down
		xPosUse2 += 2;
		yPosUse2 -= 1;
		*nodPosUse = 0;
	}

	if (xPosUse2 < 0) {
		xPosUse1--;
		if (xPosUse1 < 0) {
			*cellNrUse = -1;
			return 0;
		}
		xPosUse2 += model.paramsAutoRoute.nDiscreteSizeLevel[1];
	}
	if (xPosUse2 >= model.paramsAutoRoute.nDiscreteSizeLevel[1]) { // next cell
		xPosUse1++;
		if (xPosUse1 >= model.paramsAutoRoute.nXbasLevel) {
			*cellNrUse = -1;
			return 0;
		}
		xPosUse2 -= model.paramsAutoRoute.nDiscreteSizeLevel[1];
	}
	if (yPosUse2 < 0) { // next cell
		yPosUse1--;
		if (yPosUse1 < 0) {
			*cellNrUse = -1;
			return 0;
		}
		yPosUse2 += model.paramsAutoRoute.nDiscreteSizeLevel[1];
	}
	if (yPosUse2 >= model.paramsAutoRoute.nDiscreteSizeLevel[1]) { // next cell
		yPosUse1++;
		if (yPosUse1 >= model.paramsAutoRoute.nYbasLevel) {
			*cellNrUse = -1;
			return 0;
		}
		yPosUse2 -= model.paramsAutoRoute.nDiscreteSizeLevel[1];
	}


	*cellNrUse = xPosUse1 + model.paramsAutoRoute.nXbasLevel * yPosUse1;
	if (model.autoRoute[*cellNrUse].smallerCellsType != 1) {
		*cellNrUse = -1;
		return 0;
	}

	*cellSmallUse = xPosUse2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPosUse2;
	if (*cellNrUse > cellNr || (*cellNrUse == cellNr && *cellSmallUse >= cellSmall1)) {
		if (direction >= 8)
			direction -= 4;
		if (direction >= 4) {
			y1 = model.autoRoute[cellNr].smallerCells[cellSmall1].y;
			x1 = model.autoRoute[cellNr].smallerCells[cellSmall1].x;
			y2 = model.autoRoute[*cellNrUse].y + yPosUse2 * model.paramsAutoRoute.discretizationSizeLevel[1];
			x2 = model.autoRoute[*cellNrUse].x + xPosUse2 * model.paramsAutoRoute.discretizationSizeLevel[1];
			if (direction >= 8)
				direction -= 4;
			evalCostArc_cells(&(model.autoRoute[cellNr].smallerCells[cellSmall1]), direction, y1, x1, y2, x2);
		}
		*dist = model.autoRoute[cellNr].smallerCells[cellSmall1].arcDistance[direction];
		cost = model.autoRoute[cellNr].smallerCells[cellSmall1].arcCost[direction];
	}
	else {
		if (direction >= 8) {
			direction -= 4;
			if (direction >= 8)
				direction -= 4;
		}
		else {
			if (direction >= 4)
				direction -= 4;
		}
		*dist = model.autoRoute[*cellNrUse].smallerCells[*cellSmallUse].arcDistance[direction];
		cost = model.autoRoute[*cellNrUse].smallerCells[*cellSmallUse].arcCost[direction];
	}
	return cost;
}


int addArcsOutFromNodeBase(int nodNr, int cellPos, int i, int i1) {
	int nodPos;
	double cost;
	strAutoCells* cells;

	if (nodNr == 7)
		nodNr = nodNr;

	cells = &(model.autoRoute[cellPos]);

	nodPos = model.Noder[nodNr].nUtNoder;
	if (model.Noder[nodNr].nAllocUtNoder == 0) {
		model.Noder[nodNr].nAllocUtNoder = 9;
		model.Noder[nodNr].UtNod = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		model.Noder[nodNr].UtNodCost = (double*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(double));
		model.Noder[nodNr].outArcNr = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
	}
	else {
		if (nodPos < model.Noder[nodNr].nAllocUtNoder + 9) {
			model.Noder[nodNr].nAllocUtNoder += 8;
			model.Noder[nodNr].UtNod = (int*)realloc(model.Noder[nodNr].UtNod, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
			model.Noder[nodNr].UtNodCost = (double*)realloc(model.Noder[nodNr].UtNodCost, model.Noder[nodNr].nAllocUtNoder * sizeof(double));
			model.Noder[nodNr].outArcNr = (int*)realloc(model.Noder[nodNr].outArcNr, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		}
	}

	// right
	model.Noder[nodNr].UtNod[nodPos] = cells->nodeNr[1];
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(cellPos, 0, cellPos, 1, 0);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	// diagonal up right
	model.Noder[nodNr].UtNod[nodPos] = cells->nodeNr[3];
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(cellPos, 0, cellPos, 3, 1);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	// up
	model.Noder[nodNr].UtNod[nodPos] = cells->nodeNr[2];
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(cellPos, 0, cellPos, 2, 2);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	if (i1 > 0) {
		if (model.autoRoute[cellPos - 1].use == 1) {
			// diagonal up left
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos - 1].nodeNr[2];
			cost = addAutoArc(cellPos, 0, cellPos - 1, 2, 3);
			model.Noder[nodNr].UtNodCost[nodPos] = cost;
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
			// left
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos - 1].nodeNr[0];
			cost = addAutoArc(cellPos, 0, cellPos - 1, 0, 0);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos - 1].arcCost[0];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
		}
		if (i > 0) {
			if (model.autoRoute[cellPos - 1 - model.paramsAutoRoute.nXbasLevel].use == 1) {
				// diagonal down left
				model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos - 1 - model.paramsAutoRoute.nXbasLevel].nodeNr[0];
				cost = addAutoArc(cellPos, 0, cellPos - 1 - model.paramsAutoRoute.nXbasLevel, 0, 1);
				model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos - 1 - model.paramsAutoRoute.nXbasLevel].arcCost[1];
				model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
				nodPos++;
			}
		}
	}
	if (i > 0) {
		if (model.autoRoute[cellPos - model.paramsAutoRoute.nXbasLevel].use == 1) {
			// down
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos - model.paramsAutoRoute.nXbasLevel].nodeNr[0];
			cost = addAutoArc(cellPos, 0, cellPos - model.paramsAutoRoute.nXbasLevel, 0, 2);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos - model.paramsAutoRoute.nXbasLevel].arcCost[2];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
			// diagonal down right
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos - model.paramsAutoRoute.nXbasLevel].nodeNr[1];
			cost = addAutoArc(cellPos, 0, cellPos - model.paramsAutoRoute.nXbasLevel, 1, 3);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos - model.paramsAutoRoute.nXbasLevel].arcCost[3];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
		}
	}
	model.Noder[nodNr].nUtNoder = nodPos;

	return 0;
}

int addArcsOutFromNodeSmall(int nodNr, int cellPos, int iBas, int i1Bas, int i, int i1, int cellPos2) {
	int nodPos, cellPosTmp, cellPosTmp2;
	double cost;
	strAutoCells* cells;

	if (nodNr == 1929)
		nodNr = nodNr;

	cells = &(model.autoRoute[cellPos].smallerCells[cellPos2]);

	nodPos = model.Noder[nodNr].nUtNoder;
	if (model.Noder[nodNr].nAllocUtNoder == 0) {
		model.Noder[nodNr].nAllocUtNoder = 9;
		model.Noder[nodNr].UtNod = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		model.Noder[nodNr].UtNodCost = (double*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(double));
		model.Noder[nodNr].outArcNr = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
	}
	else {
		if (nodPos < model.Noder[nodNr].nAllocUtNoder + 9) {
			model.Noder[nodNr].nAllocUtNoder += 8;
			model.Noder[nodNr].UtNod = (int*)realloc(model.Noder[nodNr].UtNod, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
			model.Noder[nodNr].UtNodCost = (double*)realloc(model.Noder[nodNr].UtNodCost, model.Noder[nodNr].nAllocUtNoder * sizeof(double));
			model.Noder[nodNr].outArcNr = (int*)realloc(model.Noder[nodNr].outArcNr, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		}
	}

	if (nodNr == 78)
		nodNr = nodNr;
	// right
	if (model.nArcs >= 131258)
		nodPos = nodPos;
	if (model.autoRoute[cellPos].smallerCellsType == 1 || (model.autoRoute[cellPos].smallerCellsType == 2 || model.autoRoute[cellPos].smallerCellsType == 4) && i == 0) {
		model.Noder[nodNr].UtNod[nodPos] = cells->nodeNr[1];
		model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
		cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2, 1, 0);
		model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	}
	// diagonal up right
	if (model.autoRoute[cellPos].smallerCellsType == 1) {
		model.Noder[nodNr].UtNod[nodPos] = cells->nodeNr[3];
		model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
		cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2, 3, 1);
		model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	}
	// up
	if (model.autoRoute[cellPos].smallerCellsType == 1 || (model.autoRoute[cellPos].smallerCellsType == 3 || model.autoRoute[cellPos].smallerCellsType == 4) && i1 == 0) {
		model.Noder[nodNr].UtNod[nodPos] = cells->nodeNr[2];
		model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
		cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2, 2, 2);
		model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	}
	if (i1 > 0) {
		// diagonal up left
		if (model.autoRoute[cellPos].smallerCellsType == 1) {
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2 - 1].nodeNr[2]; // cells->smallerCells[cellPos2 - 1].nodeNr[2];
			cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2 - 1, 2, 3);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1].arcCost[3];// cells->smallerCells[cellPos2 - 1].arcCost[3];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
		}
		// left
		model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2 - 1].nodeNr[0]; // cells->smallerCells[cellPos2 - 1].nodeNr[0];
		cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2 - 1, 0, 0);
		model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1].arcCost[0]; // cells->smallerCells[cellPos2 - 1].arcCost[0];
		model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
		nodPos++;
		if (i > 0) {
			// diagonal down left
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].nodeNr[0]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].nodeNr[0];
			cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1], 0, 1);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
		}
		else { // i == 0
			// diagonal down left
			if (iBas > 0) {
				cellPosTmp = cellPos - model.paramsAutoRoute.nXbasLevel;
				if (cellPosTmp >= 0) {
					if (model.autoRoute[cellPosTmp].smallerCellsType == 1) {
						cellPosTmp2 = i1 - 1 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * (model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1);
						model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2].nodeNr[0];
						cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2, 0, 1);
						model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1];
						model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
						nodPos++;
					}
				}
			}
		}
	}
	else { // i1 == 0
		cellPosTmp = cellPos - 1;
		if (cellPosTmp >= 0 && i1Bas > 0) {
			cellPosTmp2 = (model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1) + model.paramsAutoRoute.nDiscreteSizeLevel[1] * i;
			if (model.autoRoute[cellPosTmp].smallerCellsType == 1) {
				// diagonal up left
				model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2].nodeNr[2]; // cells->smallerCells[cellPos2 - 1].nodeNr[2];
				cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2, 2, 1);
				model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1].arcCost[3];// cells->smallerCells[cellPos2 - 1].arcCost[3];
				model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
				nodPos++;
			}
			if (model.autoRoute[cellPosTmp].smallerCellsType == 1 || ((model.autoRoute[cellPosTmp].smallerCellsType == 2 || model.autoRoute[cellPosTmp].smallerCellsType == 4) && i == 0)) {
				// left
				model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2].nodeNr[0]; // cells->smallerCells[cellPos2 - 1].nodeNr[0];
				cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2, 0, 0);
				model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1].arcCost[0]; // cells->smallerCells[cellPos2 - 1].arcCost[0];
				model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
				nodPos++;
			}
			if (i > 0) {
				if (model.autoRoute[cellPosTmp].smallerCellsType == 1) {
					// diagonal down left
					model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].nodeNr[0]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].nodeNr[0];
					cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2 - model.paramsAutoRoute.nDiscreteSizeLevel[1], 0, 1);
					model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1];
					model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
					nodPos++;
				}
			}
			else { // i == 0 and i1 == 0
				// diagonal down left
				cellPosTmp = cellPos - 1 - model.paramsAutoRoute.nXbasLevel;
				if (cellPosTmp >= 0 && iBas > 0) {
					if (model.autoRoute[cellPosTmp].smallerCellsType == 1) {
						cellPosTmp2 = model.paramsAutoRoute.nDiscreteSizeLevel[1] * model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1;
						model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2].nodeNr[0];
						cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2, 0, 1);
						model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1];
						model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
						nodPos++;
					}
				}
			}
		}

	}
	if (i > 0) {
		// down
		cellPosTmp2 = cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1];
		model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPosTmp2].nodeNr[0]; // cells->smallerCells[cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].nodeNr[0];
		cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPosTmp2, 0, 2);
		model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[2]; // cells->smallerCells[cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[2];
		model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
		nodPos++;
		if (model.autoRoute[cellPos].smallerCellsType == 1) {
			// diagonal down right
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPosTmp2].nodeNr[1]; // cells->smallerCells[cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].nodeNr[1];
			cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPosTmp2, 1, 3);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[3]; // cells->smallerCells[cellPos2 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[3];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
			nodPos++;
		}
	}
	else { // i == 0
		cellPosTmp = cellPos - model.paramsAutoRoute.nXbasLevel;
		if (cellPosTmp >= 0) {
			if (model.autoRoute[cellPosTmp].smallerCellsType == 1 || ((model.autoRoute[cellPosTmp].smallerCellsType == 3 || model.autoRoute[cellPosTmp].smallerCellsType == 4) && i1 == 0)) {
				// down
				cellPosTmp2 = i1 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * (model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1);
				model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2].nodeNr[0];
				cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2, 0, 2);
				model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1];
				model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
				nodPos++;
			}
			if (model.autoRoute[cellPosTmp].smallerCellsType == 1) {
				// diagonal down right
				model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPosTmp].smallerCells[cellPosTmp2].nodeNr[1];
				cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPosTmp, cellPosTmp2, 1, 3);
				model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[cellPos].smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1]; // cells->smallerCells[cellPos2 - 1 - model.paramsAutoRoute.nDiscreteSizeLevel[1]].arcCost[1];
				model.Noder[nodNr].outArcNr[nodPos] = model.nArcs - 1;
				nodPos++;
			}
		}

	}
	model.Noder[nodNr].nUtNoder = nodPos;

	return 0;
}

int addArcsInOutFromPathNode(int nodNr, int pathNr, int prevNodNr, double distPrev) {
	int nodPos = 0, posIpath, yPos, xPos, yPos2, xPos2, cellPos, cellSmall, i, nodNr2, nodPos2;
	double cost, y, x, y0, x0, y1, x1, yLocal, xLocal, dist, costKvot, cost2;

	if (nodNr == 1929)
		nodNr = nodNr;

	posIpath = model.autoPath[pathNr].nNoder - 1;

	if (prevNodNr >= 0) {
		nodPos = model.Noder[prevNodNr].nUtNoder;
		model.Noder[prevNodNr].UtNod[nodPos] = nodNr;
		model.Noder[prevNodNr].outArcNr[nodPos] = model.nArcs;

		cost = addAutoArcSmallPath(pathNr, posIpath, posIpath - 1, -1, -1, -1, distPrev, -1, distPrev);
		model.Noder[prevNodNr].UtNodCost[nodPos++] = cost;
		model.Noder[prevNodNr].nUtNoder = nodPos;

	}

	if (model.Noder[nodNr].nAllocUtNoder == 0) {
		model.Noder[nodNr].nAllocUtNoder = 5;
		model.Noder[nodNr].UtNod = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		model.Noder[nodNr].UtNodCost = (double*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(double));
		model.Noder[nodNr].outArcNr = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
	}
	else {
		if (nodPos <= model.Noder[nodNr].nAllocUtNoder + 5) {
			model.Noder[nodNr].nAllocUtNoder += 5;
			model.Noder[nodNr].UtNod = (int*)realloc(model.Noder[nodNr].UtNod, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
			model.Noder[nodNr].UtNodCost = (double*)realloc(model.Noder[nodNr].UtNodCost, model.Noder[nodNr].nAllocUtNoder * sizeof(double));
			model.Noder[nodNr].outArcNr = (int*)realloc(model.Noder[nodNr].outArcNr, model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		}
	}

	nodPos = model.Noder[nodNr].nUtNoder;
	y = model.autoPath[pathNr].nodCoord_y[posIpath];
	x = model.autoPath[pathNr].nodCoord_x[posIpath];
	yPos = (int)((y - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	x = getUsable_x(x);
	xPos = (int)((x - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	yLocal = y - yPos * model.paramsAutoRoute.discretizationSizeLevel[0] - model.paramsAutoRoute.y_min;
	xLocal = x - xPos * model.paramsAutoRoute.discretizationSizeLevel[0] - model.paramsAutoRoute.x_min;
	yPos2 = (int)(yLocal / model.paramsAutoRoute.discretizationSizeLevel[1]);
	xPos2 = (int)(xLocal / model.paramsAutoRoute.discretizationSizeLevel[1]);

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	if (model.autoRoute[cellPos].use == 0 || model.autoRoute[cellPos].smallerCellsType == 0) {
		return 0; // not used cell so path is not close enought or is completely on land, skip it
	}

	cellSmall = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;
	y0 = model.autoRoute[cellPos].y + yPos2 * model.paramsAutoRoute.discretizationSizeLevel[1];
	x0 = model.autoRoute[cellPos].x + xPos2 * model.paramsAutoRoute.discretizationSizeLevel[1];

	// addera bagar till de fyra hornen i cellen;
	for (i = 0; i < 4; i++) {
		if (i == 0) {
			y1 = y0;
			x1 = x0;
		}
		if (i == 1) { // right
			y1 = y0;
			x1 = x0 + model.paramsAutoRoute.discretizationSizeLevel[1];
		}
		if (i == 2) { // up
			y1 = y0 + model.paramsAutoRoute.discretizationSizeLevel[1];
			x1 = x0;
		}
		if (i == 3) { // diagonal up right
			y1 = y0 + model.paramsAutoRoute.discretizationSizeLevel[1];
			x1 = x0 + model.paramsAutoRoute.discretizationSizeLevel[1];
		}


		costKvot = getCostKvotFromBadKvots_feasibility(y1, x1, y, x);
		if (costKvot < 2.001 || model.autoPath[pathNr].type >= 10) { // only add allowed arcs
			dist = estimateLargeCircleDistance_km(y1, x1, y, x);
			cost = dist * costKvot;

			nodNr2 = model.autoRoute[cellPos].smallerCells[cellSmall].nodeNr[i];
			nodPos2 = model.Noder[nodNr2].nUtNoder;
			checkAllocNode(nodNr2);
			model.Noder[nodNr2].UtNod[nodPos2] = nodNr;
			model.Noder[nodNr2].outArcNr[nodPos2] = model.nArcs;
			cost2 = addAutoArcSmallPath(pathNr, posIpath, -1, cellPos, cellSmall, i, dist, -1, cost);
			model.Noder[nodNr2].UtNodCost[nodPos2++] = cost2;
			model.Noder[nodNr2].nUtNoder = nodPos2;

			model.Noder[nodNr].UtNod[nodPos] = nodNr2;
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
			cost2 = addAutoArcSmallPath(pathNr, posIpath, -1, cellPos, cellSmall, i, dist, 1, cost);
			model.Noder[nodNr].UtNodCost[nodPos++] = cost2;
		}
	}
	model.Noder[nodNr].nUtNoder = nodPos;

	return 0;
}

int addArcsOutFromNodeSmall2(int nodNr, int cellPos, int iBas, int i1Bas, int i, int i1, int cellPos2) {
	int nodPos, cellPosTmp, cellPosTmp2, cellNrUse, cellSmallUse, nodPosUse, direction;
	double cost, dist;

	nodPos = model.Noder[nodNr].nUtNoder;
	if (model.Noder[nodNr].nAllocUtNoder == 0) {
		model.Noder[nodNr].nAllocUtNoder = 17;
		model.Noder[nodNr].UtNod = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
		model.Noder[nodNr].UtNodCost = (double*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(double));
		model.Noder[nodNr].outArcNr = (int*)malloc(model.Noder[nodNr].nAllocUtNoder * sizeof(int));
	}

	// right
	//model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[1];
	//model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	//cost = addAutoArcSmall(cellPos, cellPos2, 0, cellPos, cellPos2, 1, 0);
	//model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	//if (nodNr == 624 || nodNr == 599)
	//	checkMinnesAnvandning(__LINE__);

	// 0 right 
	// 1 diagonal right
	// 2 up
	// 3 diagonal up left
	// 4 left
	// 5 diagonal down left
	// 6 down
	// 7 diagonal down right
	// 8 diagonal up right right 
	// 9 diagonal up up right
	// 10 left up up
	// 11 left left up
	// 12 left left down
	// 13 left down down
	// 14 right down down
	// 15 right right down
	if (cellPos == 28 && cellPos2 == 0)
		i1 = i1;
	for (direction = 0; direction < 16; direction++) {
		if (direction == 15)
			i = i;
		cost = getPosDiagonalForDistCost(cellPos, iBas, i1Bas, cellPos2, i, i1, direction, &cellNrUse, &cellSmallUse, &nodPosUse, &dist);
		if (cellNrUse >= 0) {
			model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellNrUse].smallerCells[cellSmallUse].nodeNr[nodPosUse]; // cells->nodeNr[3];
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
			cost = addAutoArcSmall2(cellPos, cellPos2, 0, cellNrUse, cellSmallUse, nodPosUse, cost, dist);
			model.Noder[nodNr].UtNodCost[nodPos++] = cost;
		}
	}

	model.Noder[nodNr].nUtNoder = nodPos;

	return 0;
}

int addArcsFromStartNode(int yPos, int xPos, int level) {
	int nodPos, posEnd, posTmp, cellPos, nodNr;
	double y, x, cost;

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	nodNr = model.nNoder - 1;

	model.Noder[nodNr].nAllocUtNoder = 9;
	model.Noder[nodNr].UtNod = (int*)malloc(9 * sizeof(int));
	model.Noder[nodNr].outArcNr = (int*)malloc(9 * sizeof(int));
	model.Noder[nodNr].UtNodCost = (double*)malloc(9 * sizeof(double));

	nodPos = 0;


	posEnd = model.paramsAutoRoute.nCellsBase;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.startPoint_lat[0];
	model.autoRoute[posEnd].x = model.paramsAutoRoute.startPoint_lon[0];
	posTmp = 0;// 0, down left
	y = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	x = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0],
		y, x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 0, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 1, down right
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0],
		y, x + model.paramsAutoRoute.discretizationSizeLevel[0]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 1, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 2, up left
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0],
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 2, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 3, up right
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0],
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 3, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	return 0;
}

int addArcsSmallFromStartNode(int yPos, int xPos, int level) {
	int nodPos, posEnd, posTmp, cellPos, nodNr, yPos2, xPos2, cellPos2, i, i1, pos, posNod;
	double y, x, cost, x1, y1, dist;

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	yPos2 = (model.paramsAutoRoute.startPoint_lat[0] - model.paramsAutoRoute.y_min - yPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	x = getUsable_x(model.paramsAutoRoute.startPoint_lon[0]);
	xPos2 = (x - model.paramsAutoRoute.x_min - xPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	//cellPos2 = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;

	nodNr = model.nNoder - 2;

	model.Noder[nodNr].nAllocUtNoder = 16;
	model.Noder[nodNr].UtNod = (int*)malloc(16 * sizeof(int));
	model.Noder[nodNr].outArcNr = (int*)malloc(16 * sizeof(int));
	model.Noder[nodNr].UtNodCost = (double*)malloc(16 * sizeof(double));

	nodPos = 0;


	posEnd = model.paramsAutoRoute.nCellsBase;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.startPoint_lat[0];
	model.autoRoute[posEnd].x = model.paramsAutoRoute.startPoint_lon[0];
	//y = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0] + yPos2 * model.paramsAutoRoute.discretizationSizeLevel[1];
	//x = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0] + xPos2 * model.paramsAutoRoute.discretizationSizeLevel[1];

	int startY, startX, i1Use, iUse;
	pos = 0;
	startY = yPos2 - 1;
	if (startY < 0)
		startY = 0;
	startX = xPos2 - 1;
	if (startX < 0)
		startX = 0;

	for (i = startY; i < yPos2 + 3; i++) {
		for (i1 = startX; i1 < xPos2 + 3; i1++) {
			posNod = 0;
			//if (i1 < 0) {
			//	posNod += 1;
			//}
			//if (i < 0) {
			//	posNod += 2;
			//}
			if (i1 >= model.autoRoute[cellPos].nXsmall) {
				if (i1 > model.autoRoute[cellPos].nXsmall)
					continue; // too high up
				posNod += 1;
				i1Use = i1 - 1;
			}
			else
				i1Use = i1;
			if (i >= model.autoRoute[cellPos].nYsmall) {
				if (i > model.autoRoute[cellPos].nYsmall)
					continue; // too high up
				posNod += 2;
				iUse = i - 1;
			}
			else
				iUse = i;

			cellPos2 = i1Use + model.paramsAutoRoute.nDiscreteSizeLevel[1] * iUse;
			y1 = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0] + i * model.paramsAutoRoute.discretizationSizeLevel[1];
			x1 = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0] + i1 * model.paramsAutoRoute.discretizationSizeLevel[1];
			model.Noder[nodNr].UtNod[pos] = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posNod];
			evalCostArc2(model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0],
				y1, x1, &cost, &dist);
			model.Noder[nodNr].outArcNr[pos] = model.nArcs;
			cost = addAutoArcSmall2(posEnd, -1, 0, cellPos, cellPos2, posNod, cost, dist);
			model.Noder[nodNr].UtNodCost[pos] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
			pos++;
		}
	}
	model.Noder[nodNr].nUtNoder = pos;
	model.autoRoute[posEnd].nodeNr[0] = nodNr;

	
	nodNr = model.nNoder - 1;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}
	model.arc[model.nArcs].fromLevel = posEnd;
	model.arc[model.nArcs].fromPointNr = -10;
	model.arc[model.nArcs].fromTime = -10;
	model.arc[model.nArcs].toLevel = posEnd;
	model.arc[model.nArcs].toTime = -10;
	model.arc[model.nArcs].toPointNr = -10;
	model.arc[model.nArcs].distance = 0.0;
	model.arc[model.nArcs].totCost = 0;
	model.Noder[nodNr].nAllocUtNoder = 1;
	model.Noder[nodNr].UtNod = (int*)malloc(sizeof(int));
	model.Noder[nodNr].UtNodCost = (double*)malloc(sizeof(double));
	model.Noder[nodNr].outArcNr = (int*)malloc(sizeof(int));
	model.Noder[nodNr].outArcNr[0] = model.nArcs;
	model.Noder[nodNr].UtNod[0] = nodNr - 1;
	model.Noder[nodNr].UtNodCost[0] = model.arc[model.nArcs].totCost;
	model.Noder[nodNr].nUtNoder = 1;
	(model.nArcs)++;
	if (model.nArcs == 866980)
		model.nArcs = model.nArcs;


	return 0;
}

int addArcsToEndNode(int yPos, int xPos, int level) {
	int nodPos, posEnd, posTmp, cellPos, nodNr, nodEnd;
	double y, x, cost;

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	nodEnd = model.nNoder - 1;
	model.Noder[nodEnd].nUtNoder = 0;

	posEnd = model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel + 1;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
	model.autoRoute[posEnd].x = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];

	nodPos = 0;
	// connect to all four corners of the cell

	posTmp = 0;// 0, down left
	y = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	x = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	nodNr = model.autoRoute[cellPos].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, y, x, 
		model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
		model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(cellPos, 0, posEnd, 0, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	// 1, down right
	nodNr = model.autoRoute[cellPos].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp,
		y, x + model.paramsAutoRoute.discretizationSizeLevel[0], 
		model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
		model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(cellPos, 1, posEnd, 0, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	// 2, up left
	nodNr = model.autoRoute[cellPos].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp,
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x, 
		model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
		model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(cellPos, 2, posEnd, 0, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	// 3, up right
	nodNr = model.autoRoute[cellPos].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp,
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x, 
		model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
		model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs; 
	cost = addAutoArc(cellPos, 3, posEnd, 0, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;
	return 0;
}

int addArcsSmallToEndNode(int yPos, int xPos, int level) {
	int nodPos, posEnd, posTmp, cellPos, nodNr, nodEnd, cellPos2, yPos2, xPos2, i, i1, pos, posNod;
	double y, x, cost, x1, y1, dist;

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	yPos2 = (model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1] - model.paramsAutoRoute.y_min - yPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	x = getUsable_x(model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1]);
	xPos2 = (x - model.paramsAutoRoute.x_min - xPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	//cellPos2 = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;

	nodEnd = model.nNoder - 1;
	model.Noder[nodEnd].nUtNoder = 0;

	posEnd = model.paramsAutoRoute.nCellsBase + 1;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
	model.autoRoute[posEnd].x = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
	model.autoRoute[posEnd].nodeNr[0] = nodEnd;

	int startY, startX, i1Use, iUse;
	pos = 0;
	startY = yPos2 - 1;
	if (startY < 0)
		startY = 0;
	startX = xPos2 - 1;
	if (startX < 0)
		startX = 0;

	for (i = startY; i < yPos2 + 3; i++) {
		for (i1 = startX; i1 < xPos2 + 3; i1++) {
			posNod = 0;
			//if (i1 < 0) {
			//	posNod += 1;
			//}
			//if (i < 0) {
			//	posNod += 2;
			//}
			if (i1 >= model.autoRoute[cellPos].nXsmall) {
				if (i1 > model.autoRoute[cellPos].nXsmall)
					continue; // too high up
				posNod += 1;
				i1Use = i1 - 1;
			}
			else
				i1Use = i1;
			if (i >= model.autoRoute[cellPos].nYsmall) {
				if (i > model.autoRoute[cellPos].nYsmall)
					continue; // too high up
				posNod += 2;
				iUse = i - 1;
			}
			else
				iUse = i;
			cellPos2 = i1Use + model.paramsAutoRoute.nDiscreteSizeLevel[1] * iUse;
			y1 = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0] + i * model.paramsAutoRoute.discretizationSizeLevel[1];
			x1 = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0] + i1 * model.paramsAutoRoute.discretizationSizeLevel[1];
			nodNr = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posNod];
			checkAllocNode(nodNr);
			nodPos = model.Noder[nodNr].nUtNoder;
			model.Noder[nodNr].UtNod[nodPos] = nodEnd; //  model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posNod];
			evalCostArc2(y1, x1, model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
				model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1], &cost, &dist);
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
			cost = addAutoArcSmall2(cellPos, cellPos2, posNod, posEnd, -1, 0, cost, dist);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
			model.Noder[nodNr].nUtNoder = nodPos + 1;
			pos++;
		}
	}

	return 0;
}


int freeSmallerCells(strAutoCells* cell) {


	free(cell->smallerCells);
	cell->smallerCells = NULL;

	return 0;
}


int createCellNetwork() {
	int i, i1, level, pos, posTmp, i3, i4;
	int nodNr, nodPos, yPos, xPos;
	int cellPos, posSmall, posTmpNu;
	double y, x, xNy, yNy;
	strAutoCells* cells;

	int addSmallerCells = 1;

	model.paramsAutoRoute.nCellsBase = model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel;
	model.paramsAutoRoute.nCellsLevel1 = model.paramsAutoRoute.nDiscreteSizeLevel[1] * model.paramsAutoRoute.nDiscreteSizeLevel[1];
	model.nAllocNoder = 8 * model.paramsAutoRoute.nCellsBase;
	model.nAllocArcs = model.nAllocNoder * 8;

	model.Noder = (strNoder*)malloc(model.nAllocNoder * sizeof(strNoder));
	model.arc = (strArcInfo*)malloc(model.nAllocArcs * sizeof(strArcInfo));

	model.nArcs = 0;
	model.nNoder = 0;
	
	int i2;
	level = 0;
	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (pos == 9)
				pos = pos;
			if (model.autoRoute[pos].use == 1) {
				//if (model.nNoder >= model.nAllocNoder - 4) {
				//	model.nAllocNoder += 100000;
				//	model.Noder = (strNoder*)realloc(model.Noder, model.nAllocNoder * sizeof(strNoder));
				//}
				for (i2 = 0; i2 < 4; i2++)
					model.autoRoute[pos].nodeNr[i2] = -1;

				if (i > 0) {
					posTmp = pos - model.paramsAutoRoute.nXbasLevel;
					if (model.autoRoute[posTmp].use == 1) {
						model.autoRoute[pos].nodeNr[0] = model.autoRoute[posTmp].nodeNr[2];
						model.autoRoute[pos].nodeNr[1] = model.autoRoute[posTmp].nodeNr[3];
					}
				}
				if (i1 > 0) {
					posTmp = pos - 1;
					if (model.autoRoute[posTmp].use == 1) {
						if (model.autoRoute[pos].nodeNr[0] == -1) {
							model.autoRoute[pos].nodeNr[0] = model.autoRoute[posTmp].nodeNr[1];
						}
						model.autoRoute[pos].nodeNr[2] = model.autoRoute[posTmp].nodeNr[3];
					}
				}

				for (i2 = 0; i2 < 4; i2++) {
					if (model.autoRoute[pos].nodeNr[i2] == -1)
						addAutoNode(pos, i2, -1);
				}

				nodNr = model.autoRoute[pos].nodeNr[0];
				if (nodNr == 7)
					nodNr = nodNr;
				if (pos == 179)
					pos = pos;
				// addArcsOutFromNodeBase(nodNr, pos, i, i1);

				if (pos == 86)
					pos = pos;
				// add nodes and arcs from smallerCells...
				if (model.autoRoute[pos].smallerCellsType != 0 && addSmallerCells == 1) {
					cells = model.autoRoute[pos].smallerCells;
					level = 1;
					// set_nXY_smallCells(&(model.autoRoute[pos]));
					for (i3 = 0; i3 < model.autoRoute[pos].nYsmall; i3++) {
						for (i4 = 0; i4 < model.autoRoute[pos].nXsmall; i4++) {
							if (model.autoRoute[pos].smallerCellsType == 4 && i3 != 0 && i4 != 0)
								continue;
							posSmall = i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3;
							if (posSmall == 132)
								i3 = i3;
							if (pos == 11 && i3 == 5)
								i3 = i3;

							if (i3 > 0) {
								posTmp = posSmall - model.paramsAutoRoute.nDiscreteSizeLevel[level];
								model.autoRoute[pos].smallerCells[posSmall].nodeNr[0] = model.autoRoute[pos].smallerCells[posTmp].nodeNr[2];
								model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = model.autoRoute[pos].smallerCells[posTmp].nodeNr[3];
							}
							else { // i3 == 0
								if (i4 == 0)
									model.autoRoute[pos].smallerCells[posSmall].nodeNr[0] = model.autoRoute[pos].nodeNr[0]; // get node from corser network
								else
									model.autoRoute[pos].smallerCells[posSmall].nodeNr[0] = model.autoRoute[pos].smallerCells[posSmall - 1].nodeNr[1];
								if (pos >= model.paramsAutoRoute.nXbasLevel) {
									if (model.autoRoute[pos - model.paramsAutoRoute.nXbasLevel].use == 1) {
										if (model.autoRoute[pos - model.paramsAutoRoute.nXbasLevel].smallerCellsType == 1) {
											posTmpNu = i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * (model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1); // top small cell in lower big cell
											model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = model.autoRoute[pos - model.paramsAutoRoute.nXbasLevel].smallerCells[posTmpNu].nodeNr[3];
										}
										else {
											if (i4 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1)
												addAutoNode(pos, 1, posSmall);
											else
												model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = model.autoRoute[pos].nodeNr[1];
										}
									}
									else {
										if (i4 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1)
											addAutoNode(pos, 1, posSmall);
										else
											model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = model.autoRoute[pos].nodeNr[1];
									}
								}
								else {
									if (i4 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1)
										addAutoNode(pos, 1, posSmall);
									else
										model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = model.autoRoute[pos].nodeNr[1];

								}
							}
							if (i4 > 0) {
								model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = model.autoRoute[pos].smallerCells[posSmall - 1].nodeNr[3];
								// addAutoNode(pos, 2, posSmall);
							}
							else { // i4 == 0
								if (i3 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1) {
									if (pos > 0) {
										if (model.autoRoute[pos - 1].use == 1) {
											if (model.autoRoute[pos - 1].smallerCellsType == 1) {
												posTmpNu = (model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1) + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3;
												model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = model.autoRoute[pos - 1].smallerCells[posTmpNu].nodeNr[3];
											}
											else
												addAutoNode(pos, 2, posSmall);
										}else
											addAutoNode(pos, 2, posSmall);
									}
									else
										addAutoNode(pos, 2, posSmall);
								}
								else
									model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = model.autoRoute[pos].nodeNr[2];
							}
							if (i4 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1 || i3 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1)
								addAutoNode(pos, 3, posSmall);
							else
								model.autoRoute[pos].smallerCells[posSmall].nodeNr[3] = model.autoRoute[pos].nodeNr[3];

							nodNr = model.autoRoute[pos].smallerCells[posSmall].nodeNr[0];
							if (nodNr == 7)
								nodNr = nodNr;
							addArcsOutFromNodeSmall(nodNr, pos, i, i1, i3, i4, posSmall);
						}
					}
				}
			}
			pos++;
		}
	}
	// add nod and arcs for the start of the route
	identifyCellFromPoint(model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0], &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);
	addAutoNode(cellPos, -level - 1, -1);
	checkMinnesAnvandning(__LINE__);
	
	// addArcsFromStartNode(yPos, xPos, level);
	addArcsSmallFromStartNode(yPos, xPos, level);
	
	// connect to all four corners of the cell

	// add nod and arcs for the end of the route
	identifyCellFromPoint(model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
		model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1], &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);
	
	// addArcsToEndNode(yPos, xPos, level);
	addArcsSmallToEndNode(yPos, xPos, level);
	model.autoRoute_startNod = model.nNoder - 2;
	model.autoRoute_endNod = model.nNoder - 1;
	
	return 0;

}

int getNodFromOtherCell(int cellNr, int yPos1, int xPos1, int yPos2, int xPos2, int nodPos) {
	int posTmp, cellTmp;
	if (nodPos == 0) {
		if (xPos2 > 0) {
			posTmp = (xPos2 - 1) + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;
			return model.autoRoute[cellNr].smallerCells[posTmp].nodeNr[1];
		}
		// xPos2 = 0
		if (yPos2 > 0) {
			posTmp = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * (yPos2 - 1);
			return model.autoRoute[cellNr].smallerCells[posTmp].nodeNr[2];
		}
		// xPos2 == 0 && yPos2 == 0
		if (xPos1 > 0) {
			cellTmp = cellNr - 1;
			if (model.autoRoute[cellTmp].smallerCellsType == 1) {
				posTmp = model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1;
				return model.autoRoute[cellTmp].smallerCells[posTmp].nodeNr[1];
			}
			if (yPos1 > 0) {
				cellTmp = cellNr - 1 - model.paramsAutoRoute.nXbasLevel;
				if (model.autoRoute[cellTmp].smallerCellsType == 1) {
					posTmp = model.paramsAutoRoute.nDiscreteSizeLevel[1] * model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1;
					return model.autoRoute[cellTmp].smallerCells[posTmp].nodeNr[3];
				}
			}
		}
		if (yPos1 > 0) {
			cellTmp = cellNr - model.paramsAutoRoute.nXbasLevel;
			if (model.autoRoute[cellTmp].smallerCellsType == 1) {
				posTmp = model.paramsAutoRoute.nDiscreteSizeLevel[1] * (model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1);
				return model.autoRoute[cellTmp].smallerCells[posTmp].nodeNr[2];
			}
		}
		return addAutoNode(cellNr, nodPos, xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2);
	}
	if (nodPos == 1) {
		if (yPos2 > 0) {
			posTmp = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * (yPos2 - 1);
			return model.autoRoute[cellNr].smallerCells[posTmp].nodeNr[3];
		}
		// yPos2 = 0
		if (yPos1 > 0) {
			cellTmp = cellNr - model.paramsAutoRoute.nXbasLevel;
			if (model.autoRoute[cellTmp].smallerCellsType == 1) {
				posTmp = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * (model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1);
				return model.autoRoute[cellTmp].smallerCells[posTmp].nodeNr[3];
			}
		}
		return addAutoNode(cellNr, nodPos, xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2);
	}
	if (nodPos == 2) {
		if (xPos2 > 0) {
			return addAutoNode(cellNr, nodPos, xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2);
		}
		// xPos2 = 0
		if (xPos1 > 0) {
			cellTmp = cellNr - 1;
			if (model.autoRoute[cellTmp].smallerCellsType == 1) {
				posTmp = model.paramsAutoRoute.nDiscreteSizeLevel[1] - 1 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;
				return model.autoRoute[cellTmp].smallerCells[posTmp].nodeNr[3];
			}
		}
		return addAutoNode(cellNr, nodPos, xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2);
	}

	return -1;
}

int createCellNetwork2() {
	int i, i1, level, pos, posTmp, i3, i4;
	int nodNr, nodPos, yPos, xPos;
	int cellPos, posSmall, posTmpNu, nAlloc;
	double y, x, xNy, yNy;

	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (model.autoRoute[pos].smallerCellsType > 0 && model.autoRoute[pos].smallerCells != NULL) {
				freeSmallerCells(&(model.autoRoute[pos]));
			}
			pos++;
		}
	}

	model.paramsAutoRoute.nDiscreteSizeLevel[1] *= 2;
	model.paramsAutoRoute.discretizationSizeLevel[1] /= 2;
	model.paramsAutoRoute.nCellsLevel1 = model.paramsAutoRoute.nDiscreteSizeLevel[1] * model.paramsAutoRoute.nDiscreteSizeLevel[1];

	model.nArcs = 0;
	model.nNoder = 0;

	nAlloc = model.paramsAutoRoute.nDiscreteSizeLevel[1] * model.paramsAutoRoute.nDiscreteSizeLevel[1];
	level = 0;
	level = 1;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			pos = i1 + model.paramsAutoRoute.nXbasLevel * i;
			if (model.autoRoute[pos].smallerCellsType == 0)
				continue;

			model.autoRoute[pos].nYsmall = model.paramsAutoRoute.nDiscreteSizeLevel[1];
			model.autoRoute[pos].nXsmall = model.paramsAutoRoute.nDiscreteSizeLevel[1];
			// model.autoRoute[pos].smallerCells = (strAutoCells*)malloc(nAlloc * sizeof(strAutoCells));
			x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
			y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
			if (pos == 18)
				pos = pos;
			model.autoRoute[pos].smallerCells = addSmallerCells(&(model.autoRoute[pos]), x, y, 1);

			if (pos == 43)
				pos = pos;
			// add nodes and arcs from smallerCells...
			// set_nXY_smallCells(&(model.autoRoute[pos]));
			for (i3 = 0; i3 < model.autoRoute[pos].nYsmall; i3++) {
				for (i4 = 0; i4 < model.autoRoute[pos].nXsmall; i4++) {
					posSmall = i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3;
					if (pos == 43 && posSmall == 125)
						pos = pos;
					if (pos == 27 && posSmall == 1)
						pos = pos;
					if (pos == 27)
						i3 = i3;

					if (i3 > 0) {
						posTmp = posSmall - model.paramsAutoRoute.nDiscreteSizeLevel[level];
						model.autoRoute[pos].smallerCells[posSmall].nodeNr[0] = model.autoRoute[pos].smallerCells[posTmp].nodeNr[2];
						model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = model.autoRoute[pos].smallerCells[posTmp].nodeNr[3];
					}
					else { // i3 == 0
						model.autoRoute[pos].smallerCells[posSmall].nodeNr[0] = getNodFromOtherCell(pos, i, i1, i3, i4, 0);
						model.autoRoute[pos].smallerCells[posSmall].nodeNr[1] = getNodFromOtherCell(pos, i, i1, i3, i4, 1);
					}
					if (i4 > 0) {
						model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = model.autoRoute[pos].smallerCells[posSmall - 1].nodeNr[3];
					}
					else { // i4 == 0
						model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = getNodFromOtherCell(pos, i, i1, i3, i4, 2);
					}
					//if (i4 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1 || i3 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1)
						addAutoNode(pos, 3, posSmall);
				}
			}
		}
	}

	level = 1;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			pos = i1 + model.paramsAutoRoute.nXbasLevel * i;
			if (model.autoRoute[pos].smallerCellsType == 0)
				continue;

			if (pos == 18)
				pos = pos;

			// add nodes and arcs from smallerCells...
			for (i3 = 0; i3 < model.autoRoute[pos].nYsmall; i3++) {
				for (i4 = 0; i4 < model.autoRoute[pos].nXsmall; i4++) {
					posSmall = i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3;
					if (pos == 11 && i3 == 5)
						i3 = i3;
					nodNr = model.autoRoute[pos].smallerCells[posSmall].nodeNr[0];
					if (nodNr == 2)
						nodNr = nodNr;
					if (i3 == 1 && i4 == 22)
						i3 = i3;
					addArcsOutFromNodeSmall2(nodNr, pos, i, i1, i3, i4, posSmall);
					//checkMinnesAnvandning(__LINE__);
				}
			}
			//checkMinnesAnvandning(__LINE__);
		}
	}


	// add nod and arcs for the start of the route
	identifyCellFromPoint(model.paramsAutoRoute.startPoint_lat[0], model.paramsAutoRoute.startPoint_lon[0], &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);
	addAutoNode(cellPos, -level - 1, -1);
	checkMinnesAnvandning(__LINE__);

	// addArcsFromStartNode(yPos, xPos, level);
	addArcsSmallFromStartNode(yPos, xPos, level);

	// connect to all four corners of the cell

	// add nod and arcs for the end of the route
	identifyCellFromPoint(model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1], 
		model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1], &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);

	// addArcsToEndNode(yPos, xPos, level);
	addArcsSmallToEndNode(yPos, xPos, level);

	int saveNodes = 0;
	if (saveNodes == 1)
		writeAllAutoNodesToGeojson(2);

	int saveArcs = 0;
	if (saveArcs == 1)
		writeAllAutoArcsToGeojson(2);
	checkMinnesAnvandning(__LINE__);


	return 0;

}


int getCoordFromAutoArc(int arcNr, int fromTo, double* y, double* x) {
	int posCell, posTmp, level, yPos, xPos;

	if (fromTo == 0) {
		posCell = model.arc[arcNr].fromLevel;
		posTmp = model.arc[arcNr].fromPointNr;
		level = model.arc[arcNr].fromTime;
	}
	else {
		posCell = model.arc[arcNr].toLevel;
		posTmp = model.arc[arcNr].toPointNr;
		level = model.arc[arcNr].toTime;
	}
	if (posCell == 654)
		posCell = posCell;
	if (level < 0) {
		*y = model.autoRoute[posCell].y;
		*x = model.autoRoute[posCell].x;
		if (level == -1) {
			if (posTmp == 1)
				*x += model.paramsAutoRoute.discretizationSizeLevel[0];
			if (posTmp == 2)
				*y += model.paramsAutoRoute.discretizationSizeLevel[0];
			if (posTmp == 3) {
				*y += model.paramsAutoRoute.discretizationSizeLevel[0];
				*x += model.paramsAutoRoute.discretizationSizeLevel[0];
			}
		}
	}
	else {
		if (posCell >= 0) {
			*y = model.autoRoute[posCell].smallerCells[level].y;
			*x = model.autoRoute[posCell].smallerCells[level].x;
			if (posTmp == 1)
				*x += model.paramsAutoRoute.discretizationSizeLevel[1];
			if (posTmp == 2)
				*y += model.paramsAutoRoute.discretizationSizeLevel[1];
			if (posTmp == 3) {
				*y += model.paramsAutoRoute.discretizationSizeLevel[1];
				*x += model.paramsAutoRoute.discretizationSizeLevel[1];
			}
		}
		else {
			*y = model.autoPath[posTmp].nodCoord_y[level];
			*x = model.autoPath[posTmp].nodCoord_x[level];
		}
	}

	return 0;
}
int getNodFromArcNr(int arcNr, int fromTo, int* tssNod) {
	int posCell, posTmp, level, nodNr;

	if (fromTo == 0) {
		posCell = model.arc[arcNr].fromLevel;
		posTmp = model.arc[arcNr].fromPointNr;
		level = model.arc[arcNr].fromTime;
	}
	else {
		posCell = model.arc[arcNr].toLevel;
		posTmp = model.arc[arcNr].toPointNr;
		level = model.arc[arcNr].toTime;
	}

	if (posCell < 0) {
		nodNr = model.autoPath[posTmp].nodNr[level];
		*tssNod = 1;
	}
	else {
		*tssNod = 0;
		if (level < 0)
			nodNr = model.autoRoute[posCell].nodeNr[posTmp];
		else
			nodNr = model.autoRoute[posCell].smallerCells[level].nodeNr[posTmp];
	}
	return nodNr;
}

double getLutningFix(double x0, double y0, double x1, double y1) {
	double lutning, deltaX, deltaY;
	deltaX = x1 - x0;
	deltaY = y1 - y0;
	double vinkel = atan2(deltaY, deltaX);
	//if (abs(deltaY) > 0.0001)
	//	lutning = deltaX / deltaY;
	//else {
	//	if (deltaX > 0)
	//		lutning = 10000;
	//	else
	//		lutning = -10000;
	//}
	return vinkel;
}

double getLutningDiff(double lutn1, double lutn2) {
	double diff = abs(lutn1 - lutn2) * 180 / M_PI;
	// -pi to pi, -180 to 180
	if (diff > 180)
		diff = 360 - diff;
	return diff;
}

int writeSolutionToJson_autoRoute(std::string filename, int iter, int altRutt)
{
	FILE* filpekG, * filpekG2 = NULL, * filpekG3 = NULL;
	double distNu, distLast, lastLutning, lutningNu, xLast, yLast;
	double diffLutning, yUse;
	double costFactorAreaBas, costFactorArea, yLastUse;
	int xIndex;

	FILE* filPek2 = NULL;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	if (SKRIV_UT_NOTHING == 0) {
		sprintf(namn, "%s/autoPath_%d.txt", model.params.indataPath.c_str(), iter);
		filPek2 = fopen(namn, "w");
		fprintf(filPek2, "pos;arcNr;fromCellNr;fromSmallCell1;fromPosIcell;toCellPos2;toSmallCell2;toPosIcell2;distance;cost\n");
		if ((model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1] < model.paramsAutoRoute.x_min ||
			model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1] > model.boundingBox.xMax) && iter != 0) {
			sprintf(namn, "%s/res_autoIterLonFix.json", model.params.indataPath.c_str());
			if (altRutt == 0)
				filpekG3 = fopen(namn, "w");
			else
				filpekG3 = fopen(namn, "a+");
		}
	}
	if (iter < nMAX_ITER - 1) {
		sprintf(namn, "%s/res_autoIter%d.json", model.params.indataPath.c_str(), iter);
		if (altRutt == 0)
			filpekG = fopen(namn, "w");
		else
			filpekG = fopen(namn, "a+");
	}
	else {
		sprintf(namn, "%s/%s", model.params.indataPath.c_str(), filename.c_str());
		if (altRutt == 0 || model.paramsAutoRoute.routeAlternative != -1)
			filpekG = fopen(filename.c_str(), "w");
		else
			filpekG = fopen(filename.c_str(), "a+");

	}
	if (filpekG == NULL)
	{
		printf("Faile to open file %s for writing.\n", namn);
		errlog("Faile to open file %s for writing.\n", namn);
		postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
	}
	if (SKRIV_UT_NOTHING == 0) {
		sprintf(namn, "%s/res_autoIter_arc%d.json", model.params.indataPath.c_str(), iter);
		if (altRutt == 0)
			filpekG2 = fopen(namn, "w");
		else
			filpekG2 = fopen(namn, "a+");
		if (filpekG2 == NULL)
		{
			printf("Faile to open file %s for writing.\n", namn);
			postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
		}
	}
	if (altRutt == 0 || model.paramsAutoRoute.routeAlternative != -1){
		initGeoJsonFil(filpekG, "result_path");
	}
	else {
		fprintf(filpekG, ",\n\n");
	}

	if (filpekG3 != NULL) {
		if (altRutt == 0)
			initGeoJsonFil(filpekG3, "result_path");
		else
			fprintf(filpekG3, ",\n\n");
		fprintf(filpekG3, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
	}
	if (filpekG2 != NULL) {
		if (altRutt == 0)
			initGeoJsonFil(filpekG2, "result_arcs");
		else
			fprintf(filpekG2, ",\n\n");
	}

	model.network.last_x = -1000;
	fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");

	int iPos, arcNr, nInt, i;
	double totCost = 0, totDist = 0, y, x, y0, x0, splitDist, y1, x1, distance;
	spherical::Point p2, p3;
	for (iPos = 1; iPos < model.nBVArcs; iPos++)
	{
		arcNr = model.BVArc[iPos];
		if (arcNr == 85187)
			arcNr = arcNr;
		if (iPos == 5)
			iPos = iPos;

		totCost += model.arc[arcNr].totCost;
		totDist += model.arc[arcNr].distance;

		if (iPos == 1) {
			getCoordFromAutoArc(arcNr, 0, &y, &x);
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			y0 = y;
			x0 = x;
			xLast = x0;
			yLast = y0;
			distLast = 0;
			lastLutning = 1e10;
			if (filpekG3 != NULL) {
				if (x < model.paramsAutoRoute.x_min - 3 || x > model.boundingBox.xMax + 3)
					fprintf(filpekG3, "[%lf, %lf, 0.0]", x, y);
				else {
					if (model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1] < model.paramsAutoRoute.x_min)
						fprintf(filpekG3, "[%lf, %lf, 0.0]", x - 360, y);
					else
						fprintf(filpekG3, "[%lf, %lf, 0.0]", x + 360, y);
				}
			}
		}
		if (SKRIV_UT_NOTHING == 0) {
			if (iPos > 1)
				fprintf(filpekG2, ", ");
			fprintf(filpekG2, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"LineString\",\n\"coordinates\": [[%lf, %lf]",
				getCorrect_longitude(x0), y0);
		}

		if (iPos == 1315)
			iPos = iPos;
		getCoordFromAutoArc(arcNr, 1, &y1, &x1);
		p3 = spherical::Point(y0, fix_lonPos(x0));
		p2 = spherical::Point(y1, fix_lonPos(x1));
		distance = p3.distanceTo(p2) / 1000.0;
		// nInt = roundUp(model.arc[arcNr].distance / DIST_SPLIT);
		nInt = roundUp(distance / DIST_SPLIT);
		if (nInt > 1) {
			//splitDist = model.arc[arcNr].distance / nInt * 1000.0;
			splitDist = distance / nInt * 1000.0;
			costFactorAreaBas = getCostFactorArea(y0, x0, y1, x1, &xIndex);
		}
		else
			costFactorAreaBas = 2;
		y = y1;
		x = x1;

		for (i = 0; i < nInt; i++) {
			if (nInt > 1) {
				if (i < nInt - 1) {
					p3 = p3.destinationPoint(splitDist, p3.bearingTo(p2));
					y = p3.latitude().degrees();
					x = p3.longitude().degrees();
				}
				else {
					y = y1;
					x = x1;
				}
			}

			yUse = y;
			if (costFactorAreaBas < 1.00001) {
				costFactorArea = getCostFactorArea(y0, x0, y, x, &xIndex);
				if (costFactorArea >= 1.00001) {
					if (y < model.paramsAutoRoute.minLat_lonIndex[xIndex]) {
						yUse = model.paramsAutoRoute.minLat_lonIndex[xIndex];
					}
				}
			}


			distNu = estimateLargeCircleDistance_km(y0, x0, y, x);
			if (x < 40)
				x = x;
			if (distLast + distNu > DIST_SPLIT * 2) {
				if (distLast > 0.0001) {
					fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(xLast), yLastUse);
				}
				if (distNu > DIST_SPLIT * 0.8 * 2) {
					fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), yUse);
					distLast = 0;
					lastLutning = 1e10;
				}
				else {
					distLast = distNu;
					lastLutning = getLutningFix(xLast, yLast, x, y);
				}
			}
			else {
				distLast += distNu;
				if (lastLutning > 1e9) {
					lastLutning = getLutningFix(xLast, yLast, x, y);
				}
				else {
					lutningNu = getLutningFix(xLast, yLast, x, y);
					diffLutning = getLutningDiff(lastLutning, lutningNu);
					if (diffLutning > 3.01) {
						fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(xLast), yLast);
						distLast = distNu;
						lastLutning = lutningNu;
					}
				}
			}
			xLast = x;
			yLast = y;
			yLastUse = yUse;

			// fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			if (SKRIV_UT_NOTHING == 0) {
				fprintf(filpekG2, ", [%lf, %lf]", getCorrect_longitude(x), yUse);
				if (filpekG3 != NULL) {
					if (x < model.paramsAutoRoute.x_min - 3 || x > model.boundingBox.xMax + 3)
						fprintf(filpekG3, ", [%lf, %lf, 0.0]", x, yUse);
					else {
						if (model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1] < model.paramsAutoRoute.x_min)
							fprintf(filpekG3, ", [%lf, %lf, 0.0]", x - 360, yUse);
						else
							fprintf(filpekG3, ", [%lf, %lf, 0.0]", x + 360, yUse);
					}
				}
			}
		}
		if (SKRIV_UT_NOTHING == 0) {
			fprintf(filpekG2, "]}\n,\n\"properties\": {\"routeAlt\": %d, \"iPos\":%d, \"arcNr\":%d, \"cost\": %lf,\n\"dist\": %lf, \"accumCost\": %lf,\n\"accumDist\": %lf}}\n",
				altRutt, iPos, arcNr, model.arc[arcNr].totCost, model.arc[arcNr].distance, totCost, totDist);
			fprintf(filPek2, "%d;%d;%d;%d;%d;%d;%d;%d;%lf;%lf\n", iPos, arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromTime, model.arc[arcNr].fromPointNr,
				model.arc[arcNr].toLevel, model.arc[arcNr].toTime, model.arc[arcNr].toPointNr, model.arc[arcNr].distance, model.arc[arcNr].totCost);
		}
		y0 = y;
		x0 = x;
	}
	if (distLast > 0.00001) {
		fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(xLast), yLastUse);
	}

	fprintf(filpekG, "\n]]},\n\"properties\": {\n");
	fprintf(filpekG, "\"routeAlt\": %d, \"name\": \"%s\",\n  \"totCost\": %lf,\n\"totDistance\": %.2lf}}\n",
		altRutt, model.paramsAutoRoute.altRutt[altRutt].routeID, totCost, totDist / 1.852);
	printf("totDist %.3lf nm\n", totDist / 1.852);

	fclose(filpekG);
	if (SKRIV_UT_NOTHING == 0) {
		printf("testing\n");
		fclose(filPek2);
		if (altRutt == model.paramsAutoRoute.nAltRutter - 1)
			fprintf(filpekG2, "\n]}\n");
		fclose(filpekG2);
		if (filpekG3 != NULL) {
			fprintf(filpekG3, "\n]]},\n\"properties\": {\n");
			fprintf(filpekG3, "\"routeAlt\": %d, \"name\": \"%s\",\n  \"totCost\": %lf,\n\"totDistance\": %.5lf}}\n",
				altRutt, model.paramsAutoRoute.altRutt[altRutt].routeID, totCost, totDist / 1.852);
			if (altRutt == model.paramsAutoRoute.nAltRutter - 1 && iter == nMAX_ITER - 1)
				fprintf(filpekG3, "]}\n");
			fclose(filpekG3);
		}
	}

	return 0;
}

int writeSolutionToJson_autoRoute_alternatives(std::string filename, int callNr)
{
	FILE* filpekG, *filpekG2 = NULL, *filpekG3 = NULL;
	double distNu, distLast, lastLutning, lutningNu, xLast, yLast;
	double diffLutning, yUse;
	double costFactorAreaBas, costFactorArea, yLastUse;
	int xIndex;

	FILE* filPek2 = NULL;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), filename.c_str());
	if (callNr == 0) {
		filpekG = fopen(filename.c_str(), "w");
	}
	else {
		filpekG = fopen(filename.c_str(), "a+");
	}

	if (filpekG == NULL)
	{
		printf("Faile to open file %s for writing.\n", namn);
		errlog("Faile to open file %s for writing.\n", namn);
		postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
	}

	if (callNr == 0) {
		initGeoJsonFil(filpekG, "result_path");
	}
	//else {
	//	fprintf(filpekG, ",\n");
	//}

	fprintf(filpekG, "],\n");
	fprintf(filpekG, "\"routeAlternatives\":[\n");
	for (int i = 0; i < model.paramsAutoRoute.nAltRutter; i++) {
		if (i > 0)
			fprintf(filpekG, ",\n");
		fprintf(filpekG, "{\"routeAlt\":%d,\n\"name\":\"%s\"}", i, model.paramsAutoRoute.altRutt[i].routeID);
	}
	fprintf(filpekG, "]\n");
	fprintf(filpekG, "}\n");

	fclose(filpekG);

	return 0;
}

int addArcsAroundSolution()
{

	// from each nod, go forward to nodes along the solution path looking for feasible arcs.
	// as long as same direction as the arcs then no need to add new arc or not feasible
	// stop after 
	// stop after one or two nodes after attaching to tss
	// skip tss and only start at the last few nodes of tss

	int iPos, arcNr, nEjBattre, i1, arcNr1, nod1, nod2, tssNod1, tssNod2, nTssNodes, sameDir;
	double totCost = 0, totDist = 0, y, x, y1, x1, costNod1, costDiff, kvotBad, dist, cost;
	double dX, dY, dX2, dY2, x0, y0;

	model.seaRoute.nextSeaRouteNodePos = (int*)malloc(model.nBVArcs * sizeof(int));
	double lastDist = 1e10, distNu;
	int posSeaRoute = 0, nod, breakSoon, posUse = 0, nExtraPoints = 0, nExtraIter;
	int lastBattre;

	if (modelSea.nBVArcsUse > 0) {
		arcNr = modelSea.BVArcUse[posSeaRoute];
		nod = modelSea.arc[arcNr].toPointNr;
		y = modelSea.seaRoute.nod_y[nod];
		x = modelSea.seaRoute.nod_x[nod];
	}
	else {
		x = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
		y = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
	}
	x0 = x;
	y0 = y;

	for (iPos = 1; iPos < model.nBVArcs; iPos++)
	{
		if (posSeaRoute < modelSea.nBVArcsUse) {
			arcNr = model.BVArc[iPos];
			getCoordFromAutoArc(arcNr, 0, &y1, &x1);
			distNu = getDist_flat(y, x, y1, x1);
			if (distNu > lastDist) {
				posSeaRoute++;
				if (posSeaRoute < modelSea.nBVArcsUse) {
					arcNr = modelSea.BVArcUse[posSeaRoute];
					nod = modelSea.arc[arcNr].toPointNr;
					y = modelSea.seaRoute.nod_y[nod];
					x = modelSea.seaRoute.nod_x[nod];
				}
				else {
					x = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
					y = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
				}

				lastDist = 1e10;
				distNu = estimateLargeCircleDistance_km(y0, x0, y, x);
				if (distNu > 500) {
					y0 = y;
					x0 = x;
					posUse = posSeaRoute;
					nExtraPoints++;
				}

				
			}
			else
				lastDist = distNu;
		}
		model.seaRoute.nextSeaRouteNodePos[iPos] = posUse;
	}
	posUse = model.nBVArcs - 1;
	posSeaRoute = posSeaRoute + 1;
	for (iPos = model.nBVArcs - 1; iPos >= 0; iPos--) {
		if (model.seaRoute.nextSeaRouteNodePos[iPos] < posSeaRoute) {
			posSeaRoute = model.seaRoute.nextSeaRouteNodePos[iPos];
			posUse = iPos;
		}
		model.seaRoute.nextSeaRouteNodePos[iPos] = posUse;
	}

	for (iPos = 1; iPos < model.nBVArcs; iPos++)
	{
		arcNr = model.BVArc[iPos];
		if (iPos == 216)
			iPos = iPos;
		if (arcNr == 311448)
			arcNr = arcNr;
		if (arcNr == 292581)
			arcNr = arcNr;
		if (arcNr == 211566)
			arcNr = arcNr;

		nod1 = getNodFromArcNr(arcNr, 0, &tssNod1);
		costNod1 = (model.Dijkstra.nodes - model.Dijkstra.node_min + nod1)->dist / model.Dijkstra.FAKTOR_NATVERK;

		totCost += model.arc[arcNr].totCost;
		totDist += model.arc[arcNr].distance;

		getCoordFromAutoArc(arcNr, 0, &y, &x);
		getCoordFromAutoArc(arcNr, 1, &y1, &x1);
		dX2 = x1 - x;
		dY2 = y1 - y;
		nEjBattre = 0;
		nTssNodes = 0;
		if (iPos == 1315)
			iPos = iPos;
		sameDir = 1;
		breakSoon = 0;
		nExtraIter = 0;
		lastBattre = model.nBVArcs;
		for (i1 = iPos + 1; i1 < model.nBVArcs; i1++) {
			if (breakSoon == 1) {
				i1 = model.seaRoute.nextSeaRouteNodePos[i1];
				if (nExtraIter >= 15) {
					if (nExtraIter > 15 && nEjBattre > 0)
						break;
					i1 = lastBattre + 1;
				}
				nExtraIter++;
				if (i1 >= model.nBVArcs)
					break;
			}

			arcNr1 = model.BVArc[i1];
			if (arcNr1 == 211695)
				arcNr = arcNr;
			getCoordFromAutoArc(arcNr1, 1, &y1, &x1);
			if (sameDir == 1) {
				getCoordFromAutoArc(arcNr1, 0, &y0, &x0);
				dX = x1 - x0;
				dY = y1 - y0;
				sameDir = checkSameDir(dY, dX, dY2, dX2);
				if (sameDir == 1)
					continue;

			}

			nod2 = getNodFromArcNr(arcNr1, 1, &tssNod2);
			if (i1 == 1200)
				i1 = i1;
			if (i1 >= model.nBVArcs - 3)
				i1 = i1;
			costDiff = (model.Dijkstra.nodes - model.Dijkstra.node_min + nod2)->dist / model.Dijkstra.FAKTOR_NATVERK - costNod1;
			cost = evalCostArc(y, x, y1, x1, &dist);
			// dist = estimateLargeCircleDistance_km(y, x, y1, x1);
			if (cost < costDiff * 0.99999) {
				// add arc
				addAutoArcExtra(arcNr, arcNr1, nod1, nod2, dist, cost);
				nEjBattre = 0;
				if (breakSoon == 1)
					lastBattre = i1;
				if (tssNod2 == 1)
					nTssNodes++;
				else
					nTssNodes = 0;
			}
			else
				nEjBattre++;
			if (nEjBattre > 15 || nTssNodes >= 2 || i1 > iPos + 50)
				breakSoon = 1;

		}

		//if (iPos == 1) {
		//	getCoordFromAutoArc(arcNr, 0, &y, &x);
		//}
		//getCoordFromAutoArc(arcNr, 1, &y, &x);
	}

	return 0;
}


int addNodesAlongArc(int arcNr, int iPos, int* nod1a, int* nod1mitt, int* nod1b) {
	double dist, splitDist, x1, y1, x2, y2, costFactorArea;
	spherical::Point p1, p2, p3;

	getCoordFromAutoArc(arcNr, 0, &y1, &x1);
	getCoordFromAutoArc(arcNr, 1, &y2, &x2);
	p1 = spherical::Point(y1, fix_lonPos(x1));
	p2 = spherical::Point(y2, fix_lonPos(x2));
	int xIndex;
	double costFactorAreaBas = getCostFactorArea(y1, x1, y2, x2, &xIndex);

	dist = p1.distanceTo(p2) / 1000.0;

	int nExtraNodes = roundDown(dist / 10.0), i, nod, pathNr;
	if (nExtraNodes == 0) {
		if (dist >= 2.0)
			nExtraNodes = 1;
	}
	splitDist = dist / (nExtraNodes + 1) * 1000;

	pathNr = model.nAutoPaths + iPos;
	model.autoPath[pathNr].nAllocNoder = nExtraNodes;
	//model.autoPath[pathNr].nAllocNoder += 100;
	model.autoPath[pathNr].nodNr = (int*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(int));
	model.autoPath[pathNr].nodCoord_y = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
	model.autoPath[pathNr].nodCoord_x = (double*)malloc(model.autoPath[pathNr].nAllocNoder * sizeof(double));
	model.autoPath[pathNr].nNoder = 0;
	model.autoPath[pathNr].type = 11;
	model.autoPath[pathNr].kvotCost = 1.0;

	if (model.nNoder >= 42923)
		pathNr = pathNr;

	*nod1a = -1;
	*nod1b = -1;
	*nod1mitt = -1;
	int pos = 0;
	for (i = 0; i < nExtraNodes; i++) {
		p3 = p1.destinationPoint(splitDist * (i + 1), p1.bearingTo(p2));
		if(i < nMAX_ADD_NODES || i >= nExtraNodes - nMAX_ADD_NODES){
			if (i == nMAX_ADD_NODES - 1)
				i = i;
			if (i == nExtraNodes - 1)
				i = i;
			if (i > 600)
				i = i;
			model.autoPath[pathNr].nodCoord_y[pos] = p3.latitude().degrees();
			model.autoPath[pathNr].nodCoord_x[pos] = p3.longitude().degrees();
			if (costFactorAreaBas < 1.00001) {
				costFactorArea = getCostFactorArea(y1, x1, model.autoPath[pathNr].nodCoord_y[pos], model.autoPath[pathNr].nodCoord_x[pos], &xIndex);
				if (costFactorArea >= 1.00001) {
					if (model.autoPath[pathNr].nodCoord_y[pos] < model.paramsAutoRoute.minLat_lonIndex[xIndex]) {
						model.autoPath[pathNr].nodCoord_y[pos] = model.paramsAutoRoute.minLat_lonIndex[xIndex];
					}
				}
			}

			nod = addAutoNodePath(pathNr, model.autoPath[pathNr].nodCoord_y[pos],
				model.autoPath[pathNr].nodCoord_x[pos]);
			if (i == 0)
				*nod1a = nod;
			if (i == nExtraNodes - 1)
				*nod1b = nod;
			if (*nod1mitt == -2)
				*nod1mitt = nod;
			pos++;
		}
		else {
			*nod1mitt = -2;
		}
	}
	return 0;
}

int addUtNodToNod(int nodNr, int nodNr2, double cost) {

	if (nodNr == 779 && nodNr2 == 469)
		nodNr = nodNr;
	checkAllocNode(nodNr);
	int nodPos = model.Noder[nodNr].nUtNoder;
	model.Noder[nodNr].UtNod[nodPos] = nodNr2;
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	model.Noder[nodNr].UtNodCost[nodPos++] = cost;
	model.Noder[nodNr].nUtNoder = nodPos;
	return nodPos;

}

int fixaSameDirArcs(int iFirst, int iPos, double dist, double cost) {
	int arcNr1, arcNr2, nod1, nod2, tssNod1, tssNod2, i1;

	arcNr1 = model.BVArc[iFirst];
	arcNr2 = model.BVArc[iPos - 1];
	nod1 = getNodFromArcNr(arcNr1, 0, &tssNod1);
	nod2 = getNodFromArcNr(arcNr2, 1, &tssNod2);

	addAutoArcExtra(arcNr1, arcNr2, nod1, nod2, dist, cost);
	model.BVArc[iFirst] = model.nArcs - 1;
	for (i1 = iFirst + 1; i1 < iPos; i1++)
		model.BVArc[i1] = -1;

	return 0;
}

int checkSameDir(double dY, double dX, double dY2, double dX2) {
	double kvot;
	if (abs(dX - dX2) < 0.0001 && abs(dY - dY2) < 0.0001)
		return 1;

	if (abs(dY2) < 0.0001 && abs(dX2) < 0.0001)
		return 0;

	if (abs(dX) > 0.0001) {
		kvot = dX2 / dX;
		if (abs(kvot * dY - dY2) < 0.0001)
			return 1;
	}
	else {
		if (abs(dY) > 0.0001) {
			kvot = dY2 / dY;
			if (abs(kvot * dX - dX2) < 0.0001)
				return 1;
		}
		else
			return 0;
	}
	return 0;
}

int aggregateArcs() {
	int iPos, firstSameDir = -1, arcNr, nFixedArcs = 0, posNy, sameDir;
	double dX, dY, y1, x1, y2 = 0, x2 = 0, dXlast = 1e10, dYlast = 1e10;
	double dist, cost;

	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++) {
		arcNr = model.BVArc[iPos];
		if(iPos == 0)
			getCoordFromAutoArc(arcNr, 0, &y1, &x1);
		else {
			y1 = y2;
			x1 = x2;
		}
		getCoordFromAutoArc(arcNr, 1, &y2, &x2);
		dX = x2 - x1;
		dY = y1 - y2;
		if (y2 < -35)
			y2 = y2;
		sameDir = checkSameDir(dY, dX, dYlast, dXlast);
		if(sameDir == 1){
			// if (abs(dX - dXlast) < 0.0001 && abs(dY - dYlast) < 0.0001) {
			if (firstSameDir == -1) {
				firstSameDir = iPos - 1;
			}
			dist += model.arc[arcNr].distance;
			cost += model.arc[arcNr].totCost;
		}
		else {
			if (firstSameDir >= 0) {
				fixaSameDirArcs(firstSameDir, iPos, dist, cost);
				firstSameDir = -1;
				nFixedArcs++;
			}
			dXlast = dX;
			dYlast = dY;
			dist = model.arc[arcNr].distance;
			cost = model.arc[arcNr].totCost;
		}
	}
	if (firstSameDir >= 0) {
		fixaSameDirArcs(firstSameDir, iPos, dist, cost);
		nFixedArcs++;
	}

	if (nFixedArcs > 0) {
		posNy = 0;
		for (iPos = 0; iPos < model.nBVArcs; iPos++) {
			if (model.BVArc[iPos] >= 0)
				model.BVArc[posNy++] = model.BVArc[iPos];
		}
		model.nBVArcs = posNy;
	}

	return 0;
}

void changeNodeCost(int nod, int nodUt, double costDiff) {
	int i;
	for (i = 0; i < model.Noder[nod].nUtNoder; i++) {
		if (model.Noder[nod].UtNod[i] == nodUt)
			model.Noder[nod].UtNodCost[i] += costDiff;
	}
}

int arcIsPathNr(int arc, int* startEnd) {
	int path1 = -1, path2 = -1;
	if (model.arc[arc].fromTime >= 0 && model.arc[arc].fromLevel < 0) 
		path1 = model.arc[arc].fromPointNr;
	if (model.arc[arc].toTime >= 0 && model.arc[arc].toLevel < 0)
		path2 =  model.arc[arc].toPointNr;

	if (path1 == -1) {
		if (path2 == -1) {
			*startEnd = -1;
			return -1;
		}
		else {
			*startEnd = 2;
			return path2;
		}
	}
	else {
		if (path2 == -1)
			*startEnd = 1;
		else
			*startEnd = 3;
		return path1;
	}
	return -1;
}

int modifyCostArcs(double costDiff, double costTot, int nElement, int* arcs, int* nodes) {
	int i, i1;
	double costFix = 0, cost, costDelta, kvot;

	for (i = 0; i < nElement - 1; i++) {
		cost = model.arc[arcs[i]].totCost;
		kvot = cost / costTot;
		costDelta = kvot * costDiff;
		if (i == nElement - 2)
			costDelta = costDiff - costFix;

		if (cost + costDelta < 1.0) {
			printf("ERROR! can't decrease the arccost this much... to %.3lf from %.3lf. I skip this one\n", cost + costDelta, cost);
		}
		else {
			cost += costDelta;
			costFix += costDelta;
			model.arc[arcs[i]].totCost = cost;
			changeNodeCost(nodes[i], nodes[i + 1], costDelta);
		}
	}


	return 0;
}

int addArcsAroundSolution2()
{

	// from each nod, go forward to nodes along the solution path looking for feasible arcs.
	// add nodes along the arc before and after the node with slightly shorter distance and connect them with arcs.
	// only along first and last arc of paths
	// not longer along an arc then xx km

	//printf("aggregateArcs\n");
	aggregateArcs();


	model.autoPath = (strAutoPath*)realloc(model.autoPath, (model.nAutoPaths + model.nBVArcs) * sizeof(strAutoPath));

	int iPos, arcNr, nEjBattre, i1, nod1, nod2, tssNod1, startEnd1 = 0, startEnd2 = 0;
	double totCost = 0, totDist = 0, y2 = 0.0, x2, y1, x1, costNod1, costDiff, kvotBad, dist, cost, cost2;
	int posNu = 0;
	int addedNod2a = -1, addedNod2b, arcNrNext, i, nod3;
	int addedNod1a = -1, addedNod1b, addedNod0a = -1, addedNod0b, starti, endi1;
	int pathBas = model.nAutoPaths, addedNodMitt0, addedNodMitt1, addedNodMitt2;
	int arcMitt, nodFix, pathNr1, pathNr2;
	double costTot;
	int* modCost_arc, * modCost_nod;

	nMAX_ADD_NODES = 100;//  50;// 100;
	modCost_arc = (int*)malloc((2 * nMAX_ADD_NODES + 2) * sizeof(int));
	modCost_nod = (int*)malloc((2 * nMAX_ADD_NODES + 2) * sizeof(int));

	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
		model.autoPath[model.nAutoPaths + iPos].nNoder = 0;

	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
		{
		if (iPos == 20)
			iPos = iPos;
		if (iPos >= model.nBVArcs - 3)
			iPos = iPos;
		if (y2 >= 20.7 && iPos > 30)
			y2 = y2;
		arcNr = model.BVArc[iPos];
		arcNrNext = model.BVArc[iPos + 1];
		if (model.arc[arcNr].distance <= 2 || model.arc[arcNrNext].distance < 2) {
			addedNod2a = -1;
			continue; // arc is too short, don't add nodes along it
		}
		pathNr1 = arcIsPathNr(arcNr, &startEnd1);
		if (pathNr1 >= 0) {
			if (model.autoPath[pathNr1].type <= 10) {
				addedNod2a = -1;
				//continue; // arc is tss path or corridor, don't add arcs to the middle of it
			}
		}
		pathNr2 = arcIsPathNr(arcNrNext, &startEnd2);
		if (pathNr2 >= 0) {
			if (model.autoPath[pathNr2].type <= 10 && startEnd2 != 2) { // end in path is okay
				addedNod2a = -1;
				continue; // arc is tss path or corridor, don't add arcs to the middle of it
			}
		}
		
		nod1 = getNodFromArcNr(arcNr, 0, &tssNod1);
		nod2 = getNodFromArcNr(arcNr, 1, &tssNod1);
		if (nod2 == 9456)
			nod1 = nod1;

		if (addedNod1a >= 0) {
			addedNod0a = addedNod1a;
			addedNod0b = addedNod1b;
			addedNodMitt0 = addedNodMitt1;
		}
		else {
			addedNod0a = -1;
		}

		if (addedNod2a >= 0) {
			addedNod1a = addedNod2a;
			addedNod1b = addedNod2b;
			addedNodMitt1 = addedNodMitt2;
		}
		else {
			addedNod1a = -2;
			if (pathNr1 >= 0) {
				if (model.autoPath[pathNr1].type <= 10 && startEnd1 != 1) {
					addedNod1a = -1;
				}
			}
			if (addedNod1a == -2) {
				addNodesAlongArc(arcNr, iPos, &addedNod1a, &addedNodMitt1, &addedNod1b);
				getCoordFromAutoArc(arcNr, 0, &y1, &x1);
				y2 = model.autoPath[pathBas + iPos].nodCoord_y[0];
				x2 = model.autoPath[pathBas + iPos].nodCoord_x[0];
				cost = evalCostArc(y1, x1, y2, x2, &dist);
				addUtNodToNod(nod1, addedNod1a, cost);
				cost2 = addAutoArcSmallPath(pathBas + iPos, 0, -1, model.arc[arcNr].fromLevel, model.arc[arcNr].fromTime,
					model.arc[arcNr].fromPointNr, dist, -1, cost);
				posNu = 0;
				modCost_arc[posNu] = model.nArcs - 1;
				modCost_nod[posNu++] = nod1;
				costTot = cost2;
				for (i = addedNod1a; i < addedNod1b; i++) {
					y1 = y2;
					x1 = x2;
					y2 = model.autoPath[pathBas + iPos].nodCoord_y[i - addedNod1a + 1];
					x2 = model.autoPath[pathBas + iPos].nodCoord_x[i - addedNod1a + 1];
					cost = evalCostArc(y1, x1, y2, x2, &dist);
					addUtNodToNod(i, i + 1, cost);
					cost2 = addAutoArcSmallPath(pathBas + iPos, i - addedNod1a + 1, i - addedNod1a, -1, -1, -1, dist, -1, cost);
					modCost_arc[posNu] = model.nArcs - 1;
					modCost_nod[posNu++] = i;
					costTot += cost2;
				}
				y1 = y2;
				x1 = x2;
				getCoordFromAutoArc(arcNr, 1, &y2, &x2);
				cost = evalCostArc(y1, x1, y2, x2, &dist);
				addUtNodToNod(addedNod1b, nod2, cost);
				cost2 = addAutoArcSmallPath(pathBas + iPos, addedNod1b - addedNod1a, addedNod1b - addedNod1a - 1, 
					model.arc[arcNr].toLevel, model.arc[arcNr].toTime,
					model.arc[arcNr].toPointNr, dist, 1, cost);
				modCost_arc[posNu] = model.nArcs - 1;
				modCost_nod[posNu++] = addedNod1b;
				modCost_nod[posNu++] = nod2;
				costTot += cost2;
				if (abs(costTot - model.arc[arcNr].totCost) > 0.00001) {
					costDiff = model.arc[arcNr].totCost - costTot;
					modifyCostArcs(costDiff, costTot, posNu, modCost_arc, modCost_nod);
					//if (addedNodMitt1 < 0) {
					//	arcMitt = model.nArcs - (addedNod1b - addedNod1a) / 2 - 1;
					//	nodFix = (addedNod1b + addedNod1a) / 2;
					//}else
					//	nodFix = addedNodMitt1;

					//	model.arc[arcMitt].totCost += costMittDiff;
					//	changeNodeCost(nodFix - 1, nodFix, costMittDiff);
				}

			}
			addedNod0a = -1;
		}
		addNodesAlongArc(arcNrNext, iPos + 1, &addedNod2a, &addedNodMitt2, &addedNod2b);
		getCoordFromAutoArc(arcNrNext, 0, &y1, &x1);
		y2 = model.autoPath[pathBas + iPos + 1].nodCoord_y[0];
		x2 = model.autoPath[pathBas + iPos + 1].nodCoord_x[0];
		cost = evalCostArc(y1, x1, y2, x2, &dist);
		addUtNodToNod(nod2, addedNod2a, cost);
		cost2 = addAutoArcSmallPath(pathBas + iPos + 1, 0, -1, model.arc[arcNrNext].fromLevel, model.arc[arcNrNext].fromTime,
			model.arc[arcNrNext].fromPointNr, dist, -1, cost);
		posNu = 0;
		modCost_arc[posNu] = model.nArcs - 1;
		modCost_nod[posNu++] = nod2;
		costTot = cost2;
		// addAutoArcExtra(arcNrNext, arcNrNext, nod2, addedNod2a, dist, cost);
		for (i = addedNod2a; i < addedNod2b; i++) {
			y1 = y2;
			x1 = x2;
			y2 = model.autoPath[pathBas + iPos + 1].nodCoord_y[i - addedNod2a + 1];
			x2 = model.autoPath[pathBas + iPos + 1].nodCoord_x[i - addedNod2a + 1];
			cost = evalCostArc(y1, x1, y2, x2, &dist);
			addUtNodToNod(i, i + 1, cost);
			cost2 = addAutoArcSmallPath(pathBas + iPos + 1, i - addedNod2a + 1, i - addedNod2a, -1, -1, -1, dist, -1, cost);
			modCost_arc[posNu] = model.nArcs - 1;
			modCost_nod[posNu++] = i;
			costTot += cost2;
		}
		y1 = y2;
		x1 = x2;
		nod3 = getNodFromArcNr(arcNrNext, 1, &tssNod1);
		getCoordFromAutoArc(arcNrNext, 1, &y2, &x2);
		cost = evalCostArc(y1, x1, y2, x2, &dist);
		addUtNodToNod(addedNod2b, nod3, cost);
		if (iPos == 20)
			iPos = iPos;
		cost2 = addAutoArcSmallPath(pathBas + iPos + 1, addedNod2b - addedNod2a, addedNod2b - addedNod2a - 1, model.arc[arcNrNext].toLevel, model.arc[arcNrNext].toTime,
			model.arc[arcNrNext].toPointNr, dist, 1, cost);
		modCost_arc[posNu] = model.nArcs - 1;
		modCost_nod[posNu++] = addedNod2b;
		modCost_nod[posNu++] = nod3;
		costTot += cost2;
		if (abs(costTot - model.arc[arcNrNext].totCost) > 0.00001) {
			costDiff = model.arc[arcNrNext].totCost - costTot;// +0.001 * posNu;
			modifyCostArcs(costDiff, costTot, posNu, modCost_arc, modCost_nod);
			//if (addedNodMitt2 < 0) {
			//	arcMitt = model.nArcs - (addedNod2b - addedNod2a)/2 - 1;
			//	nodFix = (addedNod2b + addedNod2a) / 2;
			//}
			//else
			//	nodFix = addedNodMitt2;
			//model.arc[arcMitt].totCost += costMittDiff;
			//changeNodeCost(nodFix - 1, nodFix, costMittDiff);
		}

		if (addedNod1a >= 0) {
			if (addedNodMitt1 >= 0)
				starti = addedNodMitt1;
			else
				starti = addedNod1a;
			if (addedNodMitt2 >= 0)
				endi1 = addedNodMitt2 - 1;
			else
				endi1 = addedNod2b;
			getCoordFromAutoArc(arcNrNext, 1, &y2, &x2);
			for (i = starti; i <= addedNod1b; i++) {
				for (i1 = addedNod2a; i1 <= endi1; i1++) {
					addAutoArcBetweenPaths(pathBas + iPos, i - addedNod1a, pathBas + iPos + 1, i1 - addedNod2a);
					// addAutoArcExtra(arcNr, arcNrNext, i, i1, dist, cost);
				}

				y1 = model.autoPath[pathBas + iPos].nodCoord_y[i - addedNod1a];
				x1 = model.autoPath[pathBas + iPos].nodCoord_x[i - addedNod1a];
				cost = evalCostArc(y1, x1, y2, x2, &dist);
				addUtNodToNod(i, nod3, cost);
				cost2 = addAutoArcSmallPath(pathBas + iPos, i - addedNod1a, i - addedNod1a - 1, 
					model.arc[arcNrNext].toLevel, model.arc[arcNrNext].toTime,
					model.arc[arcNrNext].toPointNr, dist, 1, cost);
			}
		}
		if (addedNod0a >= 0 && addedNodMitt1 < 0) {
			if (addedNodMitt0 >= 0)
				starti = addedNodMitt0;
			else
				starti = addedNod0a;
			if (addedNodMitt2 >= 0)
				endi1 = addedNodMitt2 - 1;
			else
				endi1 = addedNod2b;
			getCoordFromAutoArc(arcNrNext, 1, &y2, &x2);
			for (i = starti; i <= addedNod0b; i++) {
				for (i1 = addedNod2a; i1 <= endi1; i1++) {
					addAutoArcBetweenPaths(pathBas + iPos - 1, i - addedNod0a, pathBas + iPos + 1, i1 - addedNod2a);
					// addAutoArcExtra(arcNr, arcNrNext, i, i1, dist, cost);
				}

				y1 = model.autoPath[pathBas + iPos - 1].nodCoord_y[i - addedNod0a];
				x1 = model.autoPath[pathBas + iPos - 1].nodCoord_x[i - addedNod0a];
				cost = evalCostArc(y1, x1, y2, x2, &dist);
				addUtNodToNod(i, nod3, cost);
				cost2 = addAutoArcSmallPath(pathBas + iPos - 1, i - addedNod0a, i - addedNod0a - 1,
					model.arc[arcNrNext].toLevel, model.arc[arcNrNext].toTime,
					model.arc[arcNrNext].toPointNr, dist, 1, cost);
			}

		}
	}

	FILE* filpek;
	char* namn;
	int toFileArcs = 0;
	if (toFileArcs == 1) {
		namn = (char*)malloc2(256 * sizeof(char));
		sprintf(namn, "%s/allOutArcs.txt", model.params.indataPath.c_str());
		filpek = fopen(namn, "w");
		for (i = 0; i < model.nNoder; i++) {
			for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++)
				fprintf(filpek, "nod %d utNr %d utNod %d utArc %d cost %.3lf\n", i, i1,
					model.Noder[i].UtNod[i1], model.Noder[i].outArcNr[i1], model.Noder[i].UtNodCost[i1]);
		}
		fclose(filpek);
	}

	return 0;
}

int sparaSeaRoutePart(int pos) {
	int iPos, posNu, arcNr, nod1, nod2;
	double dist;
	spherical::Point p1, p2;

	if (pos == 0) {
		posNu = 0;
	}
	else {
		posNu = modelSea.nBVArcsUse;
		if (posNu > 0 && modelSea.nBVArcs > 0) {
			//if (model.paramsAutoRoute.viaPositions[pos].nPoints == 1) {
				// a via point, so connect the node before with the node after...
			//}
			//else {
				// a path/corridor, add an arc representing the corridor...
			if (posNu >= 84)
				posNu = posNu;
				arcNr = modelSea.BVArcUse[posNu - 1];
				nod1 = modelSea.arc[arcNr].toPointNr;
				arcNr = modelSea.BVArc[0];
				nod2 = modelSea.arc[arcNr].fromPointNr;
				if (nod1 != nod2) {
					p1 = spherical::Point(modelSea.seaRoute.nod_y[nod1], fix_lonPos(modelSea.seaRoute.nod_x[nod1]));
					p2 = spherical::Point(modelSea.seaRoute.nod_y[nod2], fix_lonPos(modelSea.seaRoute.nod_x[nod2]));
					dist = p1.distanceTo(p2) / 1000.0;
					modelSea.BVArcUse[posNu++] = addArcBetweenNodes(nod1, nod2, dist);
					//modelSea.BVArcUse[posNu++] = modelSea.nArcs - 1;
				}
			//}
		}
	}

	for (iPos = 0; iPos < modelSea.nBVArcs; iPos++) {
		if (posNu >= 84)
			posNu = posNu;
		modelSea.BVArcUse[posNu++] = modelSea.BVArc[iPos];
	}
	modelSea.nBVArcsUse = posNu;

	return 0;
}

int writeSolutionToJson_seaRoute(std::string filename, int altRutt)
{
	FILE* filpekG = NULL, *filpekG2 = NULL;
	double minX, maxX, minY, maxY;

	minX = model.paramsAutoRoute.startPoint_lon[0];
	maxX = minX;
	minY = model.paramsAutoRoute.startPoint_lat[0];
	maxY = minY;

	FILE* filPek2 = NULL;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	std::string linePath = "";

	if (SKRIV_UT_NOTHING == 0) {
		sprintf(namn, "%s/seaRoutePath.txt", model.params.indataPath.c_str());
		filPek2 = fopen(namn, "w");
		fprintf(filPek2, "pos;arcNr;fromNod;toNod;distance;cost\n");

		sprintf(namn, "%s/%s", model.params.indataPath.c_str(), filename.c_str());
		if (altRutt == 0)
			filpekG = fopen(namn, "w");
		else
			filpekG = fopen(namn, "a+");
		if (filpekG == NULL)
		{
			printf("Faile to open file %s for writing.\n", namn);
			errlog("Faile to open file %s for writing.\n", namn);
			postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
		}
		if (altRutt == 0)
			initGeoJsonFil(filpekG, "result_path");
		else
			fprintf(filpekG, ",\n\n");

		sprintf(namn, "%s/seaRouteArcs.geojson", model.params.indataPath.c_str());
		filpekG2 = fopen(namn, "w");
		initGeoJsonFil(filpekG2, "result_ars");

		linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";
		fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
	}
	model.network.last_x = -1000;
	int iPos, arcNr, nod, corridorNr, i1, forsta = 1;
	double totCost = 0, totDist = 0, y, x, last_x = minX, x0, y0, distNu;
	for (iPos = -1; iPos <= modelSea.nBVArcsUse; iPos++)
	{
		if (iPos == 50)
			iPos = iPos;
		if (iPos < 0) {
			x = model.paramsAutoRoute.startPoint_lon[0];
			y = model.paramsAutoRoute.startPoint_lat[0];
			x = getCorrect_longitude(x);
			if (SKRIV_UT_NOTHING == 0)
				fprintf(filpekG, "[%lf, %lf, 0.0]", x, y);
			x0 = x;
			y0 = y;
		}
		else {
			if (iPos < modelSea.nBVArcsUse) {
				arcNr = modelSea.BVArcUse[iPos];
				corridorNr = modelSea.arc[arcNr].corridorNr;
				if (corridorNr >= 0) {
					if (SKRIV_UT_NOTHING == 0) {
						for (i1 = 1; i1 < model.paramsAutoRoute.corridors_noGoSoft[corridorNr].nPkter; i1++) {
							x = model.paramsAutoRoute.corridors_noGoSoft[corridorNr].xCoord[i1];
							y = model.paramsAutoRoute.corridors_noGoSoft[corridorNr].yCoord[i1];
							fprintf(filpekG, ", [%lf, %lf, 0.0]", x, y);
							fprintf(filPek2, "%d_%d;%d;%d;%d;%lf;%lf\n", iPos, i1, arcNr, modelSea.arc[arcNr].fromPointNr,
								modelSea.arc[arcNr].toPointNr, modelSea.arc[arcNr].distance, modelSea.arc[arcNr].totCost);

							if (forsta == 0)
								fprintf(filpekG2, ",");
							else
								forsta = 0;
							fprintf(filpekG2, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"LineString\",\n\"coordinates\": [\n");
							fprintf(filpekG2, "[%lf, %lf, 0.0]", x0, y0);
							fprintf(filpekG2, ", [%lf, %lf, 0.0]", x, y);
							fprintf(filpekG2, "]},\n\"properties\": {");
							distNu = estimateLargeCircleDistance_km(y0, x0, y, x);
							totCost += modelSea.arc[arcNr].totCost * distNu / modelSea.arc[arcNr].distance;
							totDist += distNu;
							fprintf(filpekG2, "\"iPos\": %d, \"noGoSoftCorridorPos\": %d, \"arcNr\": %d, \"routeAlt\": %d, \"name\": \"%s\",\n  \"totCost\": %lf,\n\"totDistance\": %lf}}\n",
								iPos, i1, arcNr, altRutt, model.paramsAutoRoute.altRutt[altRutt].routeID, totCost, totDist / 1.852);
							x0 = x;
							y0 = y;
						}
					}
				}
				nod = modelSea.arc[arcNr].toPointNr;
				y = modelSea.seaRoute.nod_y[nod];
				x = modelSea.seaRoute.nod_x[nod];
				totCost += modelSea.arc[arcNr].totCost;
				totDist += modelSea.arc[arcNr].distance;
			}
			else {
				arcNr = -1;
				x = model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1];
				y = model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1];
			}
		}
		x = getCorrect_longitude(x);
		if(iPos >= 0){
			if (SKRIV_UT_NOTHING == 0) {
				fprintf(filpekG, ", [%lf, %lf, 0.0]", x, y);
				if (iPos < modelSea.nBVArcsUse)
					fprintf(filPek2, "%d;%d;%d;%d;%lf;%lf\n", iPos, arcNr, modelSea.arc[arcNr].fromPointNr,
						modelSea.arc[arcNr].toPointNr, modelSea.arc[arcNr].distance, modelSea.arc[arcNr].totCost);
				else
					fprintf(filPek2, "%d;%d;%d;%d;%lf;%lf\n", iPos, -1, -1,
						-1, -1.0, -1.0);

				if (forsta == 0)
					fprintf(filpekG2, ",");
				else
					forsta = 0;
				fprintf(filpekG2, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"LineString\",\n\"coordinates\": [\n");
				fprintf(filpekG2, "[%lf, %lf, 0.0]", x0, y0);
				fprintf(filpekG2, ", [%lf, %lf, 0.0]", x, y);
				fprintf(filpekG2, "]},\n\"properties\": {");
				fprintf(filpekG2, "\"iPos\": %d, \"arcNr\": %d, \"routeAlt\": %d, \"name\": \"%s\",\n  \"totCost\": %lf,\n\"totDistance\": %lf}}\n", 
					iPos, arcNr, altRutt, model.paramsAutoRoute.altRutt[altRutt].routeID, totCost, totDist / 1.852);
				x0 = x;
				y0 = y;
			}
		}
		if (minY > y)
			minY = y;
		if (maxY < y)
			maxY = y;
		if (minX > x)
			minX = x;
		if (maxX < x)
			maxX = x;
		last_x = x;
	}

	if (SKRIV_UT_NOTHING == 0) {
		fprintf(filpekG, "\n]]},\n\"properties\": {\n");
		fprintf(filpekG, "\"routeAlt\": %d, \"name\": \"%s\",\n  \"totCost\": %lf,\n\"totDistance\": %lf}}\n", 
			altRutt, model.paramsAutoRoute.altRutt[altRutt].routeID, totCost, totDist / 1.852);
		if (altRutt == model.paramsAutoRoute.nAltRutter - 1)
			fprintf(filpekG, "]}\n");
		fclose(filpekG);
		fprintf(filpekG2, "]}\n");
		fclose(filpekG2);
		fclose(filPek2);
	}

	minY -= model.paramsAutoRoute.discretizationSizeLevel[0];
	minX -= model.paramsAutoRoute.discretizationSizeLevel[0];
	maxY += 2 * model.paramsAutoRoute.discretizationSizeLevel[0]; // to make room for full cells
	maxX += 2 * model.paramsAutoRoute.discretizationSizeLevel[0]; // to make room for full cells

	int heltal = roundDown((minX + 180) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	minX = -180 + heltal * model.paramsAutoRoute.discretizationSizeLevel[0];
	heltal = roundDown((minY + 90) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	minY = -90 + heltal * model.paramsAutoRoute.discretizationSizeLevel[0];

	if (minY < -90)
		minY = -90;
	if (maxY > 90)
		maxY = 90;

	model.paramsAutoRoute.x_min = minX;
	model.paramsAutoRoute.y_min = minY;
	model.boundingBox.xMin = model.paramsAutoRoute.x_min;
	model.boundingBox.yMin = model.paramsAutoRoute.y_min;

	model.boundingBox.xMax = maxX;
	model.boundingBox.yMax = maxY;


	return 0;
}

int markCellsToUse(int cellNr, int usedQuadr[4]) {
	int i, yPos, xPos, cellPos;

	model.autoRoute[cellNr].smallerCellsType = 1;
	yPos = (int)cellNr / model.paramsAutoRoute.nXbasLevel;
	xPos = cellNr - yPos * model.paramsAutoRoute.nXbasLevel;

	for (i = 0; i < 4; i++) {
		if (usedQuadr[i] == 1) {
			if (xPos > 0) {
				if (i == 0 || i == 2) { // left
					cellPos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * yPos;
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
				if (i == 0 && yPos > 0) { // below left
					cellPos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
				if (i == 2 && yPos < model.paramsAutoRoute.nYbasLevel - 1) { // above left
					cellPos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
			}
			if (yPos > 0) {
				if (i < 2) { // below
					cellPos = xPos + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
				if (i == 1 && xPos < model.paramsAutoRoute.nXbasLevel - 1) { // below right
					cellPos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
					model.autoRoute[cellPos].smallerCellsType = 1;
				}

			}
			if (xPos < model.paramsAutoRoute.nXbasLevel - 1) {
				if (i == 1 || i == 3) { // right
					cellPos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * yPos;
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
				if (i == 4 && yPos < model.paramsAutoRoute.nYbasLevel - 1) { // above right
					cellPos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
			}
			if (yPos < model.paramsAutoRoute.nYbasLevel - 1) {
				if (i > 1) { // above
					cellPos = xPos + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
					model.autoRoute[cellPos].smallerCellsType = 1;
				}
			}
		}
	}
	return 0;
}


int saveGoodCells() {
	int iPos, arcNr, posCell, posTmp, level, nX, nY, i, i1, lastCellNr;
	int usedQuadr[4];
	double kvotY, kvotX;

	for (i = 0; i < model.paramsAutoRoute.nCellsBase; i++) {
		model.autoRoute[i].smallerCellsType = 0;
	}

	lastCellNr = -1;
	for (iPos = 0; iPos < model.nBVArcs; iPos++)
	{
		arcNr = model.BVArc[iPos];
		if (iPos == model.nBVArcs - 2)
			iPos = iPos;
		posCell = model.arc[arcNr].toLevel;
		posTmp = model.arc[arcNr].toPointNr;
		level = model.arc[arcNr].toTime;

		if (posCell != lastCellNr) {
			if (lastCellNr >= 0)
				markCellsToUse(lastCellNr, usedQuadr);
			for (i1 = 0; i1 < 4; i1++)
				usedQuadr[i1] = 0;
			lastCellNr = posCell;
		}

		if (level >= 0) {
			nY = (int)(level / model.paramsAutoRoute.nDiscreteSizeLevel[1]);
			nX = level - nY * model.paramsAutoRoute.nDiscreteSizeLevel[1];
			kvotY = (double)nY / model.paramsAutoRoute.nDiscreteSizeLevel[1];
			kvotX = (double)nX / model.paramsAutoRoute.nDiscreteSizeLevel[1];
			if (kvotY < 0.4) {
				if (kvotX < 0.4)
					posTmp = 0;
				else {
					if (kvotX > 0.6)
						posTmp = 1;
					else
						posTmp = -1;
				}
			}
			else {
				if (kvotY > 0.6) {
					if (kvotX < 0.4)
						posTmp = 2;
					else {
						if (kvotX > 0.6)
							posTmp = 3;
						else
							posTmp = -1;
					}
				}
				posTmp = -1;
			}
		}
		if (posTmp >= 0)
			usedQuadr[posTmp] = 1;
	}
	if (lastCellNr >= 0)
		markCellsToUse(lastCellNr, usedQuadr);

	return 0;

}

int releaseMemoryIter() {
	int i;

	free(model.physicalMapA.valueCell);
	free(model.physicalMapB.valueCell);
	free(model.physical_lessBuffer_MapA.valueCell);
	free(model.physical_lessBuffer_MapB.valueCell);
	//free(model.fuelMapA.valueCell);
	//free(model.fuelMapB.valueCell);

	int nExtra = model.nExtraNoGoAreas + 1;
	for (i = 0; i < nExtra; i++) {
		free(model.extraNoGoArea[i].mapA.valueCell);
		free(model.extraNoGoArea[i].mapB.valueCell);
	}

	int nAlloc = (model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel + 2);
	for (i = 0; i < nAlloc; i++) {
		if (model.autoRoute[i].smallerCellsType != 0) {
			free(model.autoRoute[i].smallerCells);
		}
	}
	free(model.autoRoute);

	for (i = 0; i < model.nNoder; i++) {
		if (model.Noder[i].nAllocUtNoder > 0) {
			free(model.Noder[i].UtNod);
			free(model.Noder[i].UtNodCost);
			free(model.Noder[i].outArcNr);
		}
	}
	free(model.Noder);
	free(model.arc);

	return 0;
}

int setUpViaPartsForAltRoute(strAltRutt* altRutt, int ruttNr, json data) {
	json dataIt, dataGeom, dataCoord, dataIt0, dataIt2, prop;
	int posNu, nCoords, i2, antal;
	std::string namn, namn2;

	posNu = 0;
	antal = data.size();
	altRutt[ruttNr].sekvens = (strViaPos*)malloc(antal * sizeof(strViaPos));
	for (auto it = data.begin(); it != data.end(); ++it) {
		dataIt0 = it.value();
		dataGeom = dataIt0["geometry"];
		dataCoord = dataGeom["coordinates"];
		if (dataGeom["type"] != "LineString" && dataGeom["type"] != "Point") {
			postRequest("ERROR! in zoneConnections, the viaParts have to be either LineString or Point but is " + std::string(dataGeom["type"]) + ". I use no viaparts in this alt in OptiNav - autoRoute.", 0);
			altRutt[ruttNr].nParts = 0;
			return 0;
		}
		prop = dataIt0["properties"];
		namn = prop["resID"];
		if (posNu == 0) {
			namn2 = "Via " + namn;
			altRutt[ruttNr].routeID = str_alloc_cpyString(namn2);
		}
		else {
			namn2 = " and " + namn;
			altRutt[ruttNr].routeID = append_str_alloc_cpyString(altRutt[ruttNr].routeID, namn2);
		}
		if (dataGeom["type"] == "LineString") {
			altRutt[ruttNr].sekvens[posNu].nPoints = dataCoord.size();
			altRutt[ruttNr].sekvens[posNu].x = (double*)malloc((altRutt[ruttNr].sekvens[posNu].nPoints) * sizeof(double));
			altRutt[ruttNr].sekvens[posNu].y = (double*)malloc((altRutt[ruttNr].sekvens[posNu].nPoints) * sizeof(double));
			nCoords = 0;
			for (auto it2 = dataCoord.begin(); it2 != dataCoord.end(); ++it2) {
				dataIt2 = it2.value();
				i2 = 0;
				for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
					if (i2 == 0) {
						altRutt[ruttNr].sekvens[posNu].x[nCoords] = it3.value();
					}
					else if (i2 == 1)
						altRutt[ruttNr].sekvens[posNu].y[nCoords] = it3.value();
					i2++;
				}
				nCoords++;
			}
		}
		else {
			altRutt[ruttNr].sekvens[posNu].nPoints = 1;
			altRutt[ruttNr].sekvens[posNu].x = (double*)malloc((altRutt[ruttNr].sekvens[posNu].nPoints) * sizeof(double));
			altRutt[ruttNr].sekvens[posNu].y = (double*)malloc((altRutt[ruttNr].sekvens[posNu].nPoints) * sizeof(double));
			nCoords = 0;
			i2 = 0;
			for (auto it3 = dataCoord.begin(); it3 != dataCoord.end(); ++it3) {
				if (i2 == 0) {
					altRutt[ruttNr].sekvens[posNu].x[nCoords] = it3.value();
				}
				else if (i2 == 1)
					altRutt[ruttNr].sekvens[posNu].y[nCoords] = it3.value();
				i2++;
			}
		}
		posNu++;
	}
	altRutt[ruttNr].nParts = posNu;

	return 0;
}


int identifyAltRutter() {

	int pos;
	std::ifstream fil;
	char* namn;
	json data, dataGeo, dataGeo2, dataFeature, dataProp, dataGeo3, dataCoord;
	json dataIt, dataIt2, dataIt3, dataIt4, dataIt5;

	if (model.paramsAutoRoute.startZone <= 0 || model.paramsAutoRoute.endZone <= 0 || model.paramsAutoRoute.zoneConnectionsFileName == "") {
		model.paramsAutoRoute.nAltRutter = 1;
		model.paramsAutoRoute.altRutt = (strAltRutt*)malloc(sizeof(strAltRutt));
		model.paramsAutoRoute.altRutt[0].nParts = 0;
		model.paramsAutoRoute.altRutt[0].routeID = str_alloc_cpy("basic");
	}
	else {
		namn = (char*)malloc2(256 * sizeof(char));
		sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.zoneConnectionsFileName.c_str());
		errlog("trying to open %s\n", namn);
		if (!(check_file_exist(namn))) {
			postRequest(std::string(namn) + " does not exist but given as input data to OptiNav-autoRoute.I quit\n", 1);
		}
		printf("opens %s\n", namn);
		fil.open(namn);
		try {
			fil >> data;
		}
		catch (...) {
			postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav-autoRoute again.", 1);
		}
		fil.close();


		model.paramsAutoRoute.nAltRutter = 0;
		if (!data["zoneConnections"].is_null()) {
			dataIt = data["zoneConnections"];
			for (auto it = dataIt.begin(); it != dataIt.end(); ++it) {
				dataIt2 = it.value();
				if (dataIt2["startZone"] == model.paramsAutoRoute.startZone && dataIt2["endZone"] == model.paramsAutoRoute.endZone) {
					dataIt3 = dataIt2["alternatives"];
					model.paramsAutoRoute.nAltRutter = dataIt3.size();
					if (model.paramsAutoRoute.nAltRutter <= 0) {
						postRequest("ERROR! json file " + std::string(namn) + ", no given alt routes. I don't use any alternative routes in OptiNav-autoRoute.", 0);
						model.paramsAutoRoute.nAltRutter = 1;
						model.paramsAutoRoute.altRutt = (strAltRutt*)malloc(sizeof(strAltRutt));
						model.paramsAutoRoute.altRutt[0].nParts = 0;
						model.paramsAutoRoute.altRutt[0].routeID = str_alloc_cpy("basic");
						return 0;
					}
					model.paramsAutoRoute.altRutt = (strAltRutt*)malloc(model.paramsAutoRoute.nAltRutter * sizeof(strAltRutt));
					pos = 0;
					for (auto it2 = dataIt3.begin(); it2 != dataIt3.end(); ++it2) {
						dataIt4 = it2.value();
						setUpViaPartsForAltRoute(model.paramsAutoRoute.altRutt, pos, dataIt4["viaSections"]);
						pos++;
					}
				}
			}
		}
		if(model.paramsAutoRoute.nAltRutter <= 0){
			errlog("ERROR! json file %s is not valid or don't have the zone combination of start/end. I don't use any alternative routes in OptiNav-autoRoute.\n", 
				namn);
			model.paramsAutoRoute.nAltRutter = 1;
			model.paramsAutoRoute.altRutt = (strAltRutt*)malloc(sizeof(strAltRutt));
			model.paramsAutoRoute.altRutt[0].nParts = 0;
			model.paramsAutoRoute.altRutt[0].routeID = str_alloc_cpy("basic");
		}
		free(namn);
		
	}

	return 0;
}

int setStartSlutCoords(int ruttAlt) {
	int i, nAlloc, nPkter;

	model.paramsAutoRoute.nStartSlut = model.paramsAutoRoute.altRutt[ruttAlt].nParts + 1;
	model.paramsAutoRoute.startPoint_lon[0] = model.paramsAutoRoute.startBas_lon;
	model.paramsAutoRoute.startPoint_lat[0] = model.paramsAutoRoute.startBas_lat;
	model.paramsAutoRoute.endPoint_lon[model.paramsAutoRoute.nStartSlut - 1] = model.paramsAutoRoute.endBas_lon;
	model.paramsAutoRoute.endPoint_lat[model.paramsAutoRoute.nStartSlut - 1] = model.paramsAutoRoute.endBas_lat;

	for (i = 0; i < model.paramsAutoRoute.altRutt[ruttAlt].nParts; i++){
		nPkter = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].nPoints;
		model.paramsAutoRoute.startPoint_lon[i + 1] = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].x[nPkter - 1];
		model.paramsAutoRoute.startPoint_lat[i + 1] = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].y[nPkter - 1];
		model.paramsAutoRoute.endPoint_lon[i] = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].x[0];
		model.paramsAutoRoute.endPoint_lat[i] = model.paramsAutoRoute.altRutt[ruttAlt].sekvens[i].y[0];
	}

	return 0;
}

int identifyZonesStartEnd() {
	int nElement, *returnArr;
	double* x, * y;
	char* namn2;
	Raster raster;

	nElement = 2;
	x = (double*)malloc(nElement * sizeof(double));
	y = (double*)malloc(nElement * sizeof(double));
	returnArr = (int*)malloc(nElement * sizeof(int));

	x[0] = model.paramsAutoRoute.startBas_lon;
	y[0] = model.paramsAutoRoute.startBas_lat;
	x[1] = model.paramsAutoRoute.endBas_lon;
	y[1] = model.paramsAutoRoute.endBas_lat;

	namn2 = (char*)malloc(256 * sizeof(char));
	sprintf(namn2, "%s/%s", model.params.indataPath.c_str(), model.paramsAutoRoute.zonesFileName.c_str());
	raster.open(namn2);
	raster.GetRasterValues_intArray(1, nElement, x, y, returnArr);
	model.paramsAutoRoute.startZone = returnArr[0];
	model.paramsAutoRoute.endZone = returnArr[1];

	// model.extraNoGoArea[i].mapA.valueCell = model.extraNoGoArea[i].rasterA.GetRasterBand_intArrTest(1, &(model.extraNoGoArea[i].mapA), model.boundingBox);
	free(x);
	free(y);
	free(namn2);
	free(returnArr);

	return 0;
}

void SwapArrayD(double* Array, int a, int b)
{
	double temp = Array[a];
	Array[a] = Array[b];
	Array[b] = temp;
}

int SorteraArrayOrderToMax(double* BasArray, int nElement)
{
	int i, j;

	for (j = 0; j < nElement; j++) {
		for (i = 0; i < nElement - 1; i++) {
			if (BasArray[i] > BasArray[i + 1]) {
				SwapArrayD(BasArray, i, i + 1);
			}
		}
	}
	return 0;
}

int checkAddDistLat(double x, double y) {
	int i, i1;
	for (i = 0; i < model.seaRoute.nDistLatAlt; i++) {
		if (abs(y - model.seaRoute.latVal[i]) < 1e-5) {
			for (i1 = 0; i1 < model.seaRoute.nDistLat[i]; i1++) {
				if (abs(x - model.seaRoute.distLat[i][i1]) < 1e-5)
					break;
			}
			if (i1 >= model.seaRoute.nDistLat[i]) {
				if (model.seaRoute.nDistLat[i] >= model.seaRoute.nAllocDistLat[i]) {
					model.seaRoute.nAllocDistLat[i] += 100;
					model.seaRoute.distLat[i] = (double*)realloc(model.seaRoute.distLat[i],
						model.seaRoute.nAllocDistLat[i] * sizeof(double));
				}
				model.seaRoute.distLat[i][i1] = x;
				(model.seaRoute.nDistLat[i])++;
			}
			i = model.seaRoute.nDistLatAlt;
		}
	}
	return 0;
}


int evalSeaRoutePaths(std::string inputPath) {
	int i, antal, rad, index;
	double x0, y0, x1, y1, sog, currentDirection, currentSpeed;
	double wavePeriod = 10.0, rpm, windDirection, windSpeed, waveDirection, waveHeight;
	double calmWaterSpeed, vesselBearing, baseGroundSpeed, rel_windSpeed, rel_waveDir;
	double rel_windDir, speedDiffWind, speedDiffWave, kvot;
	double fuelConsumption_main, fuelConsumption_aux;
	double speedDiffWindWave, speedOverGround;
	double weatherFactorCurrent, weatherFactorWind, weatherFactorWave;
	double distSplit2 = DIST_SPLIT * 2;

	reset_errlog();
	initLookUpTables();
	model.params.indataPath = splitFilename(inputPath);
	model.params.knots_to_km = 1.852;
	loadFileParams_feasibilityAuto(&(model.paramsAutoRoute));

	loadStartEndPairsSeaRoutes(inputPath);

	char* namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/autoRoute/newSeaRoutes.txt", model.params.indataPath.c_str());
	FILE* filpek = fopen(namn, "w");
	sprintf(namn, "%s/autoRoute/newSeaRoutes.geojson", model.params.indataPath.c_str());
	FILE* filpekG = fopen(namn, "w");
	initGeoJsonFil(filpekG, "paths");
	
	int i1, i2, nInt, xIndex, lopNr = 11000;
	double xLast, yLast, distance, splitDist, costFactorAreaBas, y, x, costFactorArea;
	double yUse, distNu, distLast, lastLutning, yLastUse, lutningNu, diffLutning;
	spherical::Point p2, p3;

	model.seaRoute.nDistLatAlt = 2;
	model.seaRoute.nDistLat = (int*)calloc(model.seaRoute.nDistLatAlt, sizeof(int));
	model.seaRoute.nAllocDistLat = (int*)malloc(model.seaRoute.nDistLatAlt * sizeof(int));
	model.seaRoute.distLat = (double**)malloc(model.seaRoute.nDistLatAlt * sizeof(double*));
	model.seaRoute.latVal = (double*)malloc(model.seaRoute.nDistLatAlt * sizeof(double));
	model.seaRoute.latVal[0] = -45.0;
	model.seaRoute.latVal[1] = -53.0;
	for (i = 0; i < model.seaRoute.nDistLatAlt; i++) {
		model.seaRoute.nAllocDistLat[i] = 100;
		model.seaRoute.distLat[i] = (double*)malloc(model.seaRoute.nAllocDistLat[i] * sizeof(double));
	}

	int nSaved = 0;
	for (int i0 = 0; i0 < model.seaRoute.nNewPathPairs; i0++) {
		for (i1 = 0; i1 < model.seaRoute.pair[i0].nFrom; i1++) {
			for (i2 = 0; i2 < model.seaRoute.pair[i0].nTo; i2++) {
				model.network.last_x = -1000;
				y0 = model.seaRoute.pair[i0].yFrom[i1];
				x0 = model.seaRoute.pair[i0].xFrom[i1];
				checkAddDistLat(x0, y0);
				if (nSaved > 0)
					fprintf(filpekG, ",\n");
				nSaved++;
				fprintf(filpekG, "{\"type\":\"Feature\", \"properties\":{\"ID\":\"%s_%d_%d\", \"from\":\"%d_%d\", \"to\":\"%d_%d\"},\n",
					model.seaRoute.pair[i0].groupID, i1, i2, i0, i1, i0, i2);
				fprintf(filpek, "{\"type\":\"Feature\", \"properties\":{\"id\": %d },", lopNr++);
				fprintf(filpekG, "\"geometry\":{\"type\":\"LineString\", \"coordinates\":[");
				fprintf(filpek, "\"geometry\":{\"type\":\"LineString\", \"coordinates\":[");
				fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x0), y0);
				fprintf(filpek, "[%lf, %lf]", getCorrect_longitude(x0), y0);
				xLast = x0;
				yLast = y0;

				y1 = model.seaRoute.pair[i0].yTo[i2];
				x1 = model.seaRoute.pair[i0].xTo[i2];
				checkAddDistLat(x1, y1);
				p3 = spherical::Point(y0, fix_lonPos(x0));
				p2 = spherical::Point(y1, fix_lonPos(x1));
				distance = p3.distanceTo(p2) / 1000.0;//  estimateLargeCircleDistance_km(y0, x0, y1, x1);
				nInt = roundUp(distance / distSplit2);
				if (nInt > 1) {
					splitDist = distance / nInt * 1000.0;
					costFactorAreaBas = getCostFactorArea(y0, x0, y1, x1, &xIndex);
				}
				else
					costFactorAreaBas = 2;
				y = y1;
				x = x1;
				distLast = 0;
				lastLutning = 1e10;
				if (i0 == 0 && i2 == 2)
					i0 = i0;

				for (i = 0; i < nInt; i++) {
					if (nInt > 1) {
						if (i < nInt - 1) {
							p3 = p3.destinationPoint(splitDist, p3.bearingTo(p2));
							y = p3.latitude().degrees();
							x = p3.longitude().degrees();
						}
						else {
							y = y1;
							x = x1;
						}
					}

					yUse = y;
					if (costFactorAreaBas < 1.00001) {
						costFactorArea = getCostFactorArea(y0, x0, y, x, &xIndex);
						if (costFactorArea >= 1.00001) {
							if (y < model.paramsAutoRoute.minLat_lonIndex[xIndex]) {
								yUse = model.paramsAutoRoute.minLat_lonIndex[xIndex];
							}
						}
					}


					distNu = estimateLargeCircleDistance_km(y0, x0, y, x);
					if (x < 40)
						x = x;
					if (distLast + distNu > distSplit2 * 2) {
						if (distLast > 0.0001) {
							fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(xLast), yLastUse);
							fprintf(filpek, ", [%lf, %lf]", getCorrect_longitude(xLast), yLastUse);
						}
						if (distNu > distSplit2 * 0.8 * 2) {
							fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), yUse);
							fprintf(filpek, ", [%lf, %lf]", getCorrect_longitude(x), yUse);
							distLast = 0;
							lastLutning = 1e10;
						}
						else {
							distLast = distNu;
							lastLutning = getLutningFix(xLast, yLast, x, y);
						}
					}
					else {
						distLast += distNu;
						if (lastLutning > 1e9) {
							lastLutning = getLutningFix(xLast, yLast, x, y);
						}
						else {
							lutningNu = getLutningFix(xLast, yLast, x, y);
							diffLutning = getLutningDiff(lastLutning, lutningNu);
							if (diffLutning > 3.01) {
								fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(xLast), yLast);
								fprintf(filpek, ", [%lf, %lf]", getCorrect_longitude(xLast), yLast);
								distLast = distNu;
								lastLutning = lutningNu;
							}
						}
					}
					xLast = x;
					yLast = y;
					yLastUse = yUse;

				}
				fprintf(filpekG, ", [%lf, %lf, 0.0]]}}", getCorrect_longitude(x), y);
				fprintf(filpek, ", [%lf, %lf]]}},\n", getCorrect_longitude(x), y);
			}

		}
	}
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	for (i = 0; i < model.seaRoute.nDistLatAlt; i++) {
		SorteraArrayOrderToMax(model.seaRoute.distLat[i], model.seaRoute.nDistLat[i]);
		for (i1 = 0; i1 < model.seaRoute.nDistLat[i]; i1++) {
			if(i == 0 && i1 == 0)
				continue;
			if (i1 > 0 && i == 1) {
				if (model.seaRoute.distLat[i][i1 - 1] < 0 && model.seaRoute.distLat[i][i1] > 0)
					continue; // not include this part...
			}
			model.network.last_x = -1000;
			fprintf(filpek, "{\"type\":\"Feature\", \"properties\":{\"id\": %d },", lopNr++);
			fprintf(filpek, "\"geometry\":{\"type\":\"LineString\", \"coordinates\":[");
			if(i1 == 0)
				fprintf(filpek, "[%lf, %lf], [%lf, %lf]]}},\n", getCorrect_longitude(model.seaRoute.distLat[i][model.seaRoute.nDistLat[i] - 1]), model.seaRoute.latVal[i],
					getCorrect_longitude(model.seaRoute.distLat[i][i1]), model.seaRoute.latVal[i]);
			else
				fprintf(filpek, "[%lf, %lf], [%lf, %lf]]}},\n", getCorrect_longitude(model.seaRoute.distLat[i][i1 - 1]), model.seaRoute.latVal[i],
					getCorrect_longitude(model.seaRoute.distLat[i][i1]), model.seaRoute.latVal[i]);
		}
	}

	fclose(filpek);

	free(namn);

	return 0;
}

int genAutoRoute(std::string inputPath, std::string resultName) {
	int nod1, nod2, i0, ii0;
	double dist, dist1, dist2;
	long long Cost;
	bool Reached;

	reset_errlog();
	model.params.resultPath = splitFilename(resultName);
	model.params.resultName = resultName;
	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;
	model.params.failedTime = 0;

	initLookUpTables();

	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityAuto(&(model.paramsAutoRoute));
	loadParams_autoRoute(&(model.paramsAutoRoute));

	identifyZonesStartEnd();
	identifyAltRutter();

	if (model.paramsAutoRoute.routeAlternative == -3) {
		writeSolutionToJson_autoRoute_alternatives(resultName, 0);
		return 0;
	}


	modelSea.Dijkstra.nodes = NULL;
	model.paramsAutoRoute.startNod = -1;
	model.paramsAutoRoute.endNod = -1;

	checkMinnesAnvandning(__LINE__);
	if (model.paramsAutoRoute.newSeaRoutePathData == 1)
		load_searoutes();
	else
		load_saved_searoutes();
	updateCostsSeaRouteArcs();

	checkMinnesAnvandning(__LINE__);
	SattUppDijkstraNatverk3(&modelSea);

	modelSea.BVArc = (int*)malloc2(modelSea.nNoder * sizeof(int));
	modelSea.BVtempNodOrder = (int*)malloc2(modelSea.nNoder * sizeof(int));
	modelSea.BVArcUse = (int*)malloc(modelSea.nNoder * sizeof(int));
	model.paramsAutoRoute.nAllocSet[0] == 0;
	model.paramsAutoRoute.nAllocSet[1] == 0;

	int nAlloc = 0;
	for (int i = 0; i < model.paramsAutoRoute.nAltRutter; i++) {
		if (nAlloc < model.paramsAutoRoute.altRutt[i].nParts)
			nAlloc = model.paramsAutoRoute.altRutt[i].nParts;
	}
	nAlloc++;

	model.paramsAutoRoute.startPoint_lat = (double*)malloc(nAlloc * sizeof(double));
	model.paramsAutoRoute.startPoint_lon = (double*)malloc(nAlloc * sizeof(double));
	model.paramsAutoRoute.endPoint_lat = (double*)malloc(nAlloc * sizeof(double));
	model.paramsAutoRoute.endPoint_lon = (double*)malloc(nAlloc * sizeof(double));

	int nActualIter = 0;

	for (ii0 = 0; ii0 < model.paramsAutoRoute.nAltRutter; ii0++) {
		if (model.paramsAutoRoute.routeAlternative >= 0) {
			if (model.paramsAutoRoute.routeAlternative < model.paramsAutoRoute.nAltRutter)
				ii0 = model.paramsAutoRoute.routeAlternative;
			else {
				errlog("ERROR! routeAlternative %d given in input file for autoRoute but must be < %d. I ignore the given routeAlternative\n",
					model.paramsAutoRoute.routeAlternative, model.paramsAutoRoute.nAltRutter);
				postRequest("ERROR! routeAlternative " + std::to_string(model.paramsAutoRoute.routeAlternative) +
					" given in input file for autoRoute but must be < " + std::to_string(model.paramsAutoRoute.nAltRutter) + ".I ignore the given routeAlternative", 0);
				model.paramsAutoRoute.routeAlternative = -1;
			}
		}
		setStartSlutCoords(ii0);
		checkMinnesAnvandning(__LINE__);
		if (nActualIter > 0)
			releaseMemoryIter();

		for (i0 = 0; i0 < model.paramsAutoRoute.nStartSlut; i0++) {
			if (model.paramsAutoRoute.startNod == -1)
				nod1 = identify_nearestNode_toSearoutes(model.paramsAutoRoute.startPoint_lat[i0], model.paramsAutoRoute.startPoint_lon[i0], &dist1);
			else
				nod1 = model.paramsAutoRoute.startNod;
			if(model.paramsAutoRoute.endNod == -1)
				nod2 = identify_nearestNode_toSearoutes(model.paramsAutoRoute.endPoint_lat[i0], model.paramsAutoRoute.endPoint_lon[i0], &dist2);
			else
				nod2 = model.paramsAutoRoute.endNod;

			checkMinnesAnvandning(__LINE__);

			printf("\nsolving dijkstra's algorithm for searoute..");
			checkMinnesAnvandning(__LINE__);
			AnropDijkstra2(nod1, nod2, &modelSea, &Reached);
			checkMinnesAnvandning(__LINE__);
			printf("..done. Obj %I64d\n", modelSea.Dijkstra.OptCost);
			if (Reached == true) {
				dist = NystaUppBV_MassTest(&modelSea, Reached, nod1, nod2, &Cost);
				if (ii0 == 3)
					ii0 = ii0;
				sparaSeaRoutePart(i0);//  model.params.resultPath + "/resAutoRoute.json");
				checkMinnesAnvandning(__LINE__);
			}
			else {
				errlog("ERROR! Did not manage to find a route from start to finish for seaRoute...\n");
				printf("\nERROR! Did not manage to find a route from start to finish for seaRoute...\n");
			}
		}

		writeSolutionToJson_seaRoute("seaRoute.geojson", ii0);//  model.params.resultPath + "/resAutoRoute.json");

		createCells_new(); 
		checkMinnesAnvandning(__LINE__);

		createCellNetwork();
		checkMinnesAnvandning(__LINE__);
		
		if (model.paramsAutoRoute.tssName != "-")
			load_tss();
		if(model.paramsAutoRoute.corridorsName != "-")
			load_autoCorridors();
		if (SKRIV_UT_NOTHING == 0)
			save_tss_geojson(ii0);
		addArcs_tss();
		addArcs_corridors();
		checkMinnesAnvandning(__LINE__);
		addArcs_corridors_noGoSoft();
		checkMinnesAnvandning(__LINE__);
		addArcs_viaPaths(ii0);
		addArcs_betweenPaths();
		checkMinnesAnvandning(__LINE__);

		int saveNodes = 0;
		if (saveNodes == 1)
			writeAllAutoNodesToGeojson(1);

		int saveArcs = 0;
		if (saveArcs == 1)
			writeAllAutoArcsToGeojson(1);
		checkMinnesAnvandning(__LINE__);

		model.Dijkstra.nodes = NULL;
		for (int iter = 0; iter < nMAX_ITER; iter++) {


			checkMinnesAnvandning(__LINE__);
			SattUppDijkstraNatverk3(&model);
			checkMinnesAnvandning(__LINE__);

			nod1 = model.autoRoute_startNod;
			nod2 = model.autoRoute_endNod;

			if (nActualIter == 0) {
				model.BVArc = (int*)malloc2(model.nNoder * sizeof(int));
				model.BVtempNodOrder = (int*)malloc2(model.nNoder * sizeof(int));
			}

			printf("\nsolving dijkstra's algorithm ii0 %d iter %d ..", ii0, iter);

			AnropDijkstra2(nod1, nod2, &model, &Reached);

			printf("..done. Obj %lld\n", model.Dijkstra.OptCost);
			if (Reached == true) {
				dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
				if (model.nBVArcs < 2) {
					errlog("ERROR! Too few arcs %d in Dijkstra solution\n", model.nBVArcs);
				}
				else {
					if (SKRIV_UT_NOTHING == 0 || iter == nMAX_ITER - 1) {
						writeSolutionToJson_autoRoute(resultName, iter, ii0);//  model.params.resultPath + "/resAutoRoute.json");
						checkMinnesAnvandning(__LINE__);
					}
				}
			} 
			else {
				errlog("ERROR! Did not manage to find a route from start to finish...\n");
				printf("\nERROR! Did not manage to find a route from start to finish...\n");
			}
			if (iter == 0) {
				// saveGoodCells();
				// createCellNetwork2();
				addArcsAroundSolution();
			}
			if (iter == 1) {
				addArcsAroundSolution2();
				//writeAllPathNodesToGeojson(model.nAutoPaths + model.nBVArcs);
				//writeAllPathArcsToGeojson();

			}
		}
		nActualIter++;

		if (model.paramsAutoRoute.routeAlternative != -1)
			ii0 = model.paramsAutoRoute.nAltRutter;
	}

	writeSolutionToJson_autoRoute_alternatives(resultName, 1);

	return 0;
}
