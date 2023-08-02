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

strModel modelSea;

int loadParams_autoRoute(strParamsAutoRoute* params)
{
	int i, closestI;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux;


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
	json dataIt, dataIt2;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu, pos, posBase;
	double xVal, yVal;
	try {
		fil >> data;
	}
	catch (...) {
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav-autoRoute again.", 1);
	}

	params->startPoint_lon = -9999;
	params->startPoint_lat = -9999;
	params->endPoint_lon = -9999;
	params->endPoint_lat = -9999;
	params->nCellLevels = 2;
	params->discretizationSizeLevel = (double*)malloc(params->nCellLevels * sizeof(double));
	params->nDiscreteSizeLevel = (int*)malloc(params->nCellLevels * sizeof(int));
	params->discretizationSizeLevel[0] = 1.5; // 3.0; // degrees
	params->nDiscreteSizeLevel[1] = 12; // km
	params->discretizationSizeLevel[1] = params->discretizationSizeLevel[0] / params->nDiscreteSizeLevel[1]; // degrees
	params->factorExtraCover = 6;// 10; //  2;
	params->maxBaseFeasibleCost = 1000;
	params->mapAutoRoutePhysicalAFileName = std::string();
	params->mapAutoRoutePhysicalBFileName = std::string();
	params->tssName = std::string();


	if (!data["startPoint_lon"].is_null())
		params->startPoint_lon = data["startPoint_lon"];
	if (!data["startPoint_lat"].is_null())
		params->startPoint_lat = data["startPoint_lat"];
	if (!data["endPoint_lon"].is_null())
		params->endPoint_lon = data["endPoint_lon"];
	if (!data["endPoint_lat"].is_null())
		params->endPoint_lat = data["endPoint_lat"];
	if (!data["searoutePathFile"].is_null())
		params->searoutePathsName = data["searoutePathFile"];
	else {
		postRequest("ERROR! No searoutePaths, they must exist when running autoRoute. I quit!", 1);
	}
	if (!data["newSeaRoutePathData"].is_null())
		params->newSeaRoutePathData = data["newSeaRoutePathData"];
	else
		params->newSeaRoutePathData = 0;

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


	if (!data["optionalExtraNoGoAreas"].is_null()) {
		json dataExtra = data["optionalExtraNoGoAreas"];
		model.nExtraNoGoAreas = dataExtra.size();
		model.extraNoGoArea = (strExtraNoGo*)malloc((model.nExtraNoGoAreas + 1) * sizeof(strExtraNoGo));
		pos = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			model.extraNoGoArea[pos].areaID = dataNu["noGoAreaID"];
			posBase = findAreaIDpos_inBase(model.extraNoGoArea[pos].areaID);
			if (posBase < 0) {
				errlog("ERROR! extra noGoAreaID %s is not defined in file_params.json. Add this area. I ignore it for now.\n",
					model.extraNoGoArea[pos].areaID);
				postRequest("ERROR! extra noGoAreaID " + std::string(model.extraNoGoArea[pos].areaID) + " is not defined in file_params.json. Add this area. I ignore it for now and keep running.", 0);
				continue;
			}
			model.extraNoGoArea[pos].posBase = posBase;
			pos++;
		}
		model.nExtraNoGoAreas = pos;
	}
	else {
		model.nExtraNoGoAreas = 0;
		model.extraNoGoArea = (strExtraNoGo*)malloc((model.nExtraNoGoAreas + 1) * sizeof(strExtraNoGo));
	}

	fil.close();

	if (params->startPoint_lon < -9998 || params->startPoint_lat < -9998) {
		postRequest("ERROR! startCoord not given correctly in " + std::string(namn) + ". Fix it and run OptiNav-autoRoute again.\n", 1);
	}
	if (params->endPoint_lon < -9998 || params->endPoint_lat < -9998) {
		postRequest("ERROR! endCoord not given correctly in " + std::string(namn) + ". Fix it and run OptiNav-autoRoute again.\n", 1);
	}
	return 0;
}

void getCoordOnTss(int nTss, int pos, double kvot, double* y, double* x) {
	*y = model.tss[nTss].yCoord[pos] * kvot + model.tss[nTss].yCoord[pos - 1] * (1 - kvot);
	*x = model.tss[nTss].xCoord[pos] * kvot + model.tss[nTss].xCoord[pos - 1] * (1 - kvot);
}

void get_xy_fromModelSeaBVArcsPos(int i, double* y1, double* x1) {
	int arcNr, nod;
	if (i <= 0) {
		if (i == -1) {
			*x1 = model.paramsAutoRoute.startPoint_lon;
			*y1 = model.paramsAutoRoute.startPoint_lat;
		}
		else {
		arcNr = modelSea.BVArc[i];
		nod = modelSea.arc[arcNr].fromPointNr;
		*y1 = modelSea.seaRoute.nod_y[nod];
		*x1 = modelSea.seaRoute.nod_x[nod];
		}
	}
	else {
	if (i < modelSea.nBVArcs) {
		arcNr = modelSea.BVArc[i];
		nod = modelSea.arc[arcNr].toPointNr;
		*y1 = modelSea.seaRoute.nod_y[nod];
		*x1 = modelSea.seaRoute.nod_x[nod];
	}
	else {
		*x1 = model.paramsAutoRoute.endPoint_lon;
		*y1 = model.paramsAutoRoute.endPoint_lat;
	}
	}

}

double getDist_flat(double x1, double y1, double x2, double y2) {
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

int getClosestPathPos(int pos1, int pos2, double y, double x, double* kvot) {
	int i, min_i, arcNr, nod;
	double minDist = 1e10, dist, x1, y1;
	double segmentLength, dist1, dist2, factor1, factor2, xTmp, yTmp, xTmp2, yTmp2, y0, x0, x2, y2;

	for (i = pos1 - 1; i <= pos2; i++) {
		get_xy_fromModelSeaBVArcsPos(i, &y1, &x1);
		dist = estimateLargeCircleDistance_km(y, x, y1, x1);
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
			//dist2 = estimateLargeCircleDistance_km(x, y, xTmp2, yTmp2);
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

int checkIfTssInUsedCells(int nTss) {
	int i, traff, xPos, yPos, pos, i0, firstTraff = -1, lastTraff = -1, firstTraffPos, lastTraffPos;
	double yPosDbl, xPosDbl, xNu, yNu, dx, dy, kvot, next_xKvot, next_yKvot, yPosDbl_prev, xPosDbl_prev;
	double firstTraffKvot, lastTraffKvot, firstTraffPos_endKvot, lastTraffPos_startKvot;
	double tss_y1, tss_x1, tss_y2, tss_x2, pathKvotStart, pathKvotEnd;
	int firstPathPos, lastPathPos, closestPathPosStart, closestPathPosEnd, lastPos;

	traff = 0;
	lastPos = -1;

	for (i0 = 0; i0 < model.tss[nTss].nCoords; i0++) {
		if (i0 == 0 && (abs(model.tss[nTss].xCoord[i0] + 9.966667) < 0.0001))
			i0 = i0;
		yPosDbl = (model.tss[nTss].yCoord[i0] - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		xPosDbl = (model.tss[nTss].xCoord[i0] - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		if (i0 > 0) {
			if ((yPosDbl < 0 && yPosDbl_prev < 0) || (xPosDbl < 0 && xPosDbl_prev < 0) ||
				(yPosDbl > model.paramsAutoRoute.nYbasLevel) && (yPosDbl_prev > model.paramsAutoRoute.nYbasLevel) ||
				(xPosDbl > model.paramsAutoRoute.nXbasLevel) && (xPosDbl_prev > model.paramsAutoRoute.nXbasLevel)) {
				// go to next point as both these are outside the interesting area
			}
			else {
				// kolla om en neagtiv o en i ok omrade sa fortsatt for den kommer in i tillatet omrade.Kolla kvotBerakningen;
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
						if (nTss == 5)
							i = i;
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
			getCoordOnTss(nTss, firstTraff, firstTraffKvot, &tss_y1, &tss_x1);
			getCoordOnTss(nTss, lastTraff, lastTraffKvot, &tss_y2, &tss_x2);
		//}
		firstPathPos = model.autoRoute[firstTraffPos].firstUsePathPos;
		lastPathPos = model.autoRoute[firstTraffPos].lastUsePathPos;
		closestPathPosStart = getClosestPathPos(firstPathPos, lastPathPos, tss_y1, tss_x1, &pathKvotStart);
		firstPathPos = model.autoRoute[lastTraffPos].firstUsePathPos;
		lastPathPos = model.autoRoute[lastTraffPos].lastUsePathPos;
		closestPathPosEnd = getClosestPathPos(firstPathPos, lastPathPos, tss_y2, tss_x2, &pathKvotEnd);
		if (closestPathPosStart < closestPathPosEnd || (closestPathPosStart == closestPathPosEnd && pathKvotStart < pathKvotEnd)) {
			model.tss[nTss].firstTraffCoord = firstTraff;
			model.tss[nTss].lastTraffCoord = lastTraff;
			model.tss[nTss].firstTraffCoordKvot = firstTraffKvot;
			model.tss[nTss].lastTraffCoordKvot = lastTraffKvot;
			model.tss[nTss].firstTraffPos = firstTraffPos;
			model.tss[nTss].lastTraffPos = lastTraffPos;
			return 1;
		}
		else
			return 0;
	}
	else
		return 0;
}

int load_tss()
{
	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
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
	json data, geom, coords, dataIt2;
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

		if (pos == 6)
			pos = pos;
		useTss = checkIfTssInUsedCells(nTss);
		if(useTss == 1)
			nTss++;
		else {
			free(model.tss[nTss].xCoord);
			free(model.tss[nTss].yCoord);
		}
	}
	model.nTss = nTss;
	free(namn);

	return 0;
}

int addArcBetweenNodes(int nod1, int nod2, double dist) {
	int i, i0;
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
		modelSea.arc[modelSea.nArcs].totCost = dist;
		modelSea.arc[modelSea.nArcs].distance = dist;
		(modelSea.nArcs)++;
	}
	return 0;
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
		if (i >= modelSea.nAllocNoder) {
			modelSea.nAllocNoder += 10000;
			modelSea.Noder = (strNoder*)realloc(modelSea.Noder, modelSea.nAllocNoder * sizeof(strNoder));
			modelSea.seaRoute.nod_y = (double*)realloc(modelSea.seaRoute.nod_y, modelSea.nAllocNoder * sizeof(double));
			modelSea.seaRoute.nod_x = (double*)realloc(modelSea.seaRoute.nod_x, modelSea.nAllocNoder * sizeof(double));
		}
		modelSea.seaRoute.nod_y[i] = y;
		modelSea.seaRoute.nod_x[i] = x;
		modelSea.Noder[i].nUtNoder = 0;
		modelSea.Noder[i].nAllocUtNoder = 10;
		modelSea.Noder[i].UtNod = (int*)malloc(modelSea.Noder[i].nAllocUtNoder * sizeof(int));
		modelSea.Noder[i].UtNodCost = (double*)malloc(modelSea.Noder[i].nAllocUtNoder * sizeof(double));
		modelSea.Noder[i].outArcNr = (int*)malloc(modelSea.Noder[i].nAllocUtNoder * sizeof(int));

		(modelSea.nNoder)++;
	}
	return i;
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
				p2 = spherical::Point(model.seaRoute.seaRoutePath[posUse].yCoord[nCoords], xNu);
				nCoords++;
				if (nCoords >= 2) {
					//dist = estimateLargeCircleDistance_km(modelSea.seaRoute.nod_y[nodPrev], modelSea.seaRoute.nod_x[nodPrev],
					// 	modelSea.seaRoute.nod_y[nodNu], modelSea.seaRoute.nod_x[nodNu]);
					dist = p1.distanceTo(p2) / 1000.0;
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
	int i;
	filpek = fopen(namn, "w");
	fprintf(filpek, "%d\n", modelSea.nNoder);
	for (i = 0; i < modelSea.nNoder; i++) {
		fprintf(filpek, "%d %d %.4lf %.4lf\n", i, modelSea.Noder[i].nUtNoder, modelSea.seaRoute.nod_y[i], modelSea.seaRoute.nod_x[i]);
	}
	fclose(filpek);

	sprintf(namn, "%s/autoRoute/seaRoute_arcs.txt", model.params.indataPath.c_str());
	filpek = fopen(namn, "w");
	fprintf(filpek, "%d\n", modelSea.nArcs);
	for (i = 0; i < modelSea.nArcs; i++) {
		fprintf(filpek, "%d %d %d %.4lf %.4lf\n", i, modelSea.arc[i].fromPointNr, 
			modelSea.arc[i].toPointNr, modelSea.arc[i].totCost, modelSea.arc[i].distance);
	}
	fclose(filpek);

	return 0;
}

int load_saved_searoutes() {
	FILE* filpek;
	int i, iTmp, antal, nod1, nod2;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

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
		antal = fscanf(filpek, "%d %d %lf %lf\n", &iTmp, &(modelSea.Noder[i].nUtNoder), 
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
	for (i = 0; i < modelSea.nArcs; i++) {
		antal = fscanf(filpek, "%d %d %d %lf %lf\n", &iTmp, &(nod1),
			&(nod2), &(modelSea.arc[i].totCost), &(modelSea.arc[i].distance));
		if (antal != 5) {
			postRequest("ERROR! failed to read from seaRoute_arcs.txt. i " + std::to_string(i) + " antal " + std::to_string(antal), 1);
		}
		modelSea.arc[i].fromPointNr = nod1;
		modelSea.arc[i].toPointNr = nod2;
		if (modelSea.Noder[nod1].nUtNoder >= modelSea.Noder[nod1].nAllocUtNoder) {
			errlog("ERROR! Too many outnodes for nod %d compared to given in seRoute_arcs.txt, %d\n", nod1, modelSea.Noder[nod1].nUtNoder);
		}
		modelSea.Noder[nod1].UtNod[modelSea.Noder[nod1].nUtNoder] = nod2;
		modelSea.Noder[nod1].UtNodCost[modelSea.Noder[nod1].nUtNoder] = modelSea.arc[i].totCost;
		modelSea.Noder[nod1].outArcNr[modelSea.Noder[nod1].nUtNoder] = iTmp;
		(modelSea.Noder[nod1].nUtNoder)++;
	}
	fclose(filpek);
	printf("nNodes %d nArcs %d\n", modelSea.nNoder, modelSea.nArcs);
	return 0;
}


int calc_boundingBoxAutoRoute() { // not needed anymore
	double minX, minY, maxX, maxY, kvot;

	if (model.paramsAutoRoute.startPoint_lon < model.paramsAutoRoute.endPoint_lon) {
		minX = model.paramsAutoRoute.startPoint_lon;
		maxX = model.paramsAutoRoute.endPoint_lon;
	}
	else {
		maxX = model.paramsAutoRoute.startPoint_lon;
		minX = model.paramsAutoRoute.endPoint_lon;
	}
	if (model.paramsAutoRoute.startPoint_lat < model.paramsAutoRoute.endPoint_lat) {
		minY = model.paramsAutoRoute.startPoint_lat;
		maxY = model.paramsAutoRoute.endPoint_lat;
	}
	else {
		maxY = model.paramsAutoRoute.startPoint_lat;
		minY = model.paramsAutoRoute.endPoint_lat;
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
	int intTmp = (int)(model.paramsAutoRoute.x_min * 10);
	double floatTmp = (double)intTmp / 10;
	if (model.paramsAutoRoute.x_min < floatTmp)
		model.paramsAutoRoute.x_min = floatTmp - 0.1;
	else
		model.paramsAutoRoute.x_min = floatTmp;

	intTmp = (int)(model.paramsAutoRoute.y_min * 10);
	floatTmp = (double)intTmp / 10;
	if (model.paramsAutoRoute.y_min < floatTmp)
		model.paramsAutoRoute.y_min = floatTmp - 0.1;
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

	getRowColDblFromPhysicalMap(model.physicalMapB, lat1, lon1, &row1Dbl, &col1Dbl);
	getRowColDblFromPhysicalMap(model.physicalMapB, lat2, lon2, &row2Dbl, &col2Dbl);

	delta_row = row2Dbl - row1Dbl;
	if (delta_row > model.physicalMapB.nRows / 2) {
		delta_row = model.physicalMapB.nRows - delta_row;
	}
	else {
		if (delta_row < -model.physicalMapB.nRows / 2) {
			delta_row = -model.physicalMapB.nRows - delta_row;
		}
	}
	delta_col = col2Dbl - col1Dbl;
	if (delta_col > model.physicalMapB.nCols / 2) {
		delta_col = delta_col - model.physicalMapB.nCols;
	}
	else {
		if (delta_col < -model.physicalMapB.nCols / 2) {
			delta_col = model.physicalMapB.nCols + delta_col;
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
		if (col >= model.physicalMapB.nCols)
			colUse = col - model.physicalMapB.nCols;
		else {
			if (col < 0)
				colUse = col + model.physicalMapB.nCols;
			else
				colUse = col;
		}
		if (model.physicalMapB.valueCell[row * model.physicalMapB.nCols + colUse] != 0)
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

double check_map_badKvot_auto(double lat1, double lon1, double lat2, double lon2, int mapAlt, int pos_noGoMap) {
	Raster::strPhysRaster physicalMap;
	double row1Dbl, col1Dbl, row2Dbl, col2Dbl, delta_row, delta_col;
	double kvot, a0, a1, ac, ar, colDbl, rowDbl;
	double x1, y1, x2, y2, colForeg, kvotOK = 0, kvotOKTmp;
	int row, col, i, isOK, colUse, posOK;
	int nVardenOnMap = 0;

	if (pos_noGoMap >= 0) {
		if (mapAlt == 0)
			physicalMap = model.extraNoGoArea[pos_noGoMap].mapB;
		else
			physicalMap = model.extraNoGoArea[pos_noGoMap].mapA;
	}
	else {
		if (mapAlt == 0)
			physicalMap = model.physicalMapB;
		else
			physicalMap = model.physicalMapA;
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
				kvotOKTmp = check_map_badKvot_auto(y1, x1, y2, x2, 1, pos_noGoMap);
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

double evalCostArc(double y1b, double x1b, double y2b, double x2b, double* distRes) {
	double cost, dist, kvotBad2, totCost = 0, totDist;
	double kvotBad, x1, y1, x2, y2, splitDist;
	totDist = estimateLargeCircleDistance_km(y1b, x1b, y2b, x2b);
	int nInt, i;
	spherical::Point p1, p2, p3;

	x1 = x1b;
	y1 = y1b;
	x2 = x2b;
	y2 = y2b;
	// split the arc as it is too long
	nInt = roundUp(totDist / DIST_SPLIT);
	if (nInt > 1) {
		p3 = spherical::Point(y1, x1);
		p2 = spherical::Point(y2, x2);
		splitDist = totDist / nInt * 1000.0;
	}


	for (i = 0; i < nInt; i++) {
		if (nInt > 1) {
			if (i < nInt - 1) {
				p3 = p3.destinationPoint(splitDist, p3.bearingTo(p2));
				y2 = p3.latitude().degrees();
				x2 = p3.longitude().degrees();
			}
			else {
				y2 = y2b;
				x2 = x2b;
			}
		}
		dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
		kvotBad = check_map_badKvot_auto(y1, x1, y2, x2, 0, -1);
		if (kvotBad < 1) {
			for (int i = 0; i < model.nExtraNoGoAreas; i++) {
				kvotBad += check_map_badKvot_auto(y1, x1, y2, x2, 0, i);
				if (kvotBad >= 1)
					break;
			}
		}

		if (kvotBad > 0) {
			kvotBad2 = check_map_badKvot_auto(y1, x1, y2, x2, 0, model.nExtraNoGoAreas); // the last extra map is for land!!!
			cost = dist * (1 + 1000 * kvotBad + 100000 * kvotBad2);
		}
		else
			cost = dist;
		totCost += cost;
		// totDist += dist;
		x1 = x2;
		y1 = y2;
	}

	*distRes = totDist;
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
	double kvotBad2;

	double kvotBad = check_map_badKvot_auto(y1, x1, y2, x2, 0, -1);

	if (kvotBad < 1) {
		for (int i = 0; i < model.nExtraNoGoAreas; i++) {
			kvotBad += check_map_badKvot_auto(y1, x1, y2, x2, 0, i);
			if (kvotBad >= 1)
				break;
		}
	}
	*dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);

	if (kvotBad > 0) {
		kvotBad2 = check_map_badKvot_auto(y1, x1, y2, x2, 0, model.nExtraNoGoAreas); // the last extra map is for land!!!
		*cost = *dist * (1 + 1000 * kvotBad + 100000 * kvotBad2);
	}
	else
		*cost = *dist;
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
	cells = (strAutoCells*)malloc(nAlloc * sizeof(strAutoCells));
	for (i = 0; i < nAlloc; i++) {
		cells[i].smallerCells = NULL;
		cells[i].smallerCellsType = 0;
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

int identifyCellFromPoint(double y, double x, int* yPos, int* xPos, int* level) {

	*yPos = (y - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
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

	int i, i1, pos = 0, level;
	double x, y, xNy, yNy;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (forsta != 1)
				fprintf(filpekG, ", ");
			else
				forsta = 0;
			fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"level\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"xPos\":%d, \"yPos\":%d},\n",
				0, model.autoRoute[pos].nodeNr[0], pos, i1, i);
			x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
			y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
			fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", x, y);
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
						fprintf(filpekG, "{\"type\":\"Feature\", \"properties\":{\"level\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"cellPos2\":%d, \"xPos\":%d, \"yPos\":%d},\n",
							level, model.autoRoute[pos].smallerCells[i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3].nodeNr[0],
							pos, i4 + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3, i4, i3);
						xNy = x + i4 * model.paramsAutoRoute.discretizationSizeLevel[level];
						yNy = y + i3 * model.paramsAutoRoute.discretizationSizeLevel[level];
						fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", xNy, yNy);
					}
				}

			}
			pos++;
		}
	}

	for (i = 0; i < model.nTss; i++) {
		for (i1 = 0; i1 < model.tss[i].nNoder; i1++) {
			fprintf(filpekG, ", ");
			fprintf(filpekG, "{\"type\":\"Feature\", \"properties\":{\"level\":%d, \"nodeNr\":%d, \"cellPos\":%d, \"cellPos2\":%d, \"xPos\":%d, \"yPos\":%d},\n",
				2, model.tss[i].nodNr[i1], -1, i, i1, -1, -1);
			xNy = model.tss[i].nodCoord_x[i1];
			yNy = model.tss[i].nodCoord_y[i1];
			fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n", xNy, yNy);

		}
	}

	fprintf(filpekG, "]}\n");
	fclose(filpekG);


	return 0;
}

void getCoordFromTss(int nr, int pos, double kvot, double* y, double* x) {
	double y1, y2, x1, x2;
	y1 = model.tss[nr].yCoord[pos];
	x1 = model.tss[nr].xCoord[pos];
	if (kvot > 0.0001) {
		y2 = model.tss[nr].yCoord[pos + 1];
		x2 = model.tss[nr].xCoord[pos + 1];
		*y = y1 * (1 - kvot) + y2 * kvot;
		*x = x1 * (1 - kvot) + x2 * kvot;
	}
	else {
		*y = y1;
		*x = x1;
	}
}

int generate_nodes_along_tssSegment(int tssNr, int coordPos, double y1, double x1, double y2, double x2) {
	double dist, distRef, nIntDbl, dx, dy, x, y, prevX, prevY;
	int nInt, i, startI, prevNodNr, nodNr;

	dist = estimateLargeCircleDistance_km(x1, y1, x2, y2);
	distRef = estimateLargeCircleDistance_km(x1, y1, x1 + model.paramsAutoRoute.discretizationSizeLevel[1], y1);
	nIntDbl = dist / distRef;
	if (nIntDbl < 0.001)
		return 0; // too close points, don't add any nodes
	nInt = roundUp(nIntDbl);
	dx = x2 - x1;
	dy = y2 - y1;

	if (coordPos == model.tss[tssNr].firstTraffCoord) {
		startI = 0;
		prevNodNr = -1;
		dist = -1;
	}
	else {
		startI = 1;
		prevNodNr = model.tss[tssNr].nodNr[model.tss[tssNr].nNoder];
	}
	for (i = 0; i <= nInt; i++) {
		x = x1 + dx * i / nInt;
		y = y1 + dy * i / nInt;
		if (i < startI) {
			nodNr = model.tss[tssNr].nodNr[model.tss[tssNr].nNoder - 1];
		}
		else {
			nodNr = addAutoNodeTss(tssNr, y, x); // pos, 3, posSmall);
			if (prevNodNr >= 0)
				dist = estimateLargeCircleDistance_km(prevX, prevY, x, y);
			else
				dist = 0;
			addArcsInOutFromTssNode(nodNr, tssNr, prevNodNr, dist); // nodNr, pos, i, i1, i3, i4, posSmall);
		}
		prevNodNr = nodNr;
		prevX = x;
		prevY = y;
	}


	return 0;
}

int addArcs_tss() {
	int i, i1;
	double x1 = -1, y1 = -1, x2, y2;

	for (i = 0; i < model.nTss; i++) {
		model.tss[i].nAllocNoder = 100;
		model.tss[i].nodNr = (int*)malloc(model.tss[i].nAllocNoder * sizeof(int));
		model.tss[i].nodCoord_y = (double*)malloc(model.tss[i].nAllocNoder * sizeof(double));
		model.tss[i].nodCoord_x = (double*)malloc(model.tss[i].nAllocNoder * sizeof(double));
		model.tss[i].nNoder = 0;

		if (i == 11)
			i = i;
		for (i1 = model.tss[i].firstTraffCoord - 1; i1 <= model.tss[i].lastTraffCoord; i1++) {
			if (i1 == model.tss[i].firstTraffCoord - 1)
				getCoordFromTss(i, i1, model.tss[i].firstTraffCoordKvot, &y2, &x2);
			else {
				if (i1 == model.tss[i].lastTraffCoord)
					getCoordFromTss(i, i1 - 1, model.tss[i].lastTraffCoordKvot, &y2, &x2);
				else
					getCoordFromTss(i, i1, 0.0, &y2, &x2);
			}
			if (i1 >= model.tss[i].firstTraffCoord) {
				generate_nodes_along_tssSegment(i, i1, y1, x1, y2, x2);
			}
			x1 = x2;
			y1 = y2;

		}
	}

	return 0;
}

int save_tss_geojson() {
	int i, i1;
	FILE* filpekG;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/autoArcs_tssActive.geojson", model.params.indataPath.c_str());
	filpekG = fopen(namn, "w");
	free(namn);
	initGeoJsonFil(filpekG, "tss_active");

	double x, y, kvot;

	for (i = 0; i < model.nTss; i++) {
		if(i > 0)
			fprintf(filpekG, ", ");
		fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
		for (i1 = model.tss[i].firstTraffCoord - 1; i1 <= model.tss[i].lastTraffCoord; i1++) {
			if (i1 == model.tss[i].firstTraffCoord - 1)
				getCoordFromTss(i, i1, model.tss[i].firstTraffCoordKvot, &y, &x);
			else {
				if (i1 == model.tss[i].lastTraffCoord)
					getCoordFromTss(i, i1 - 1, model.tss[i].lastTraffCoordKvot, &y, &x);
				else
					getCoordFromTss(i, i1, 0.0, &y, &x);
				fprintf(filpekG, ", ");
			}
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
		}
		fprintf(filpekG, "\n]]},\n\"properties\": {\n");
		fprintf(filpekG, "\"tssNr\": %d, \"pos1\": %d,\"kvot1\": %.3lf,\n\"pos2\": %d, \"kvot2\": %.3lf,\"posCell1\": %d,\n\"posCell2\": %d}}\n",
			i, model.tss[i].firstTraffCoord, model.tss[i].firstTraffCoordKvot, model.tss[i].lastTraffCoord,
			model.tss[i].lastTraffCoordKvot, model.tss[i].firstTraffPos, model.tss[i].lastTraffPos);
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


int createCells() {
	openNeededRasterFilesNew(-10);

	model.paramsAutoRoute.x_min = model.boundingBox.xMin;
	model.paramsAutoRoute.nXbasLevel = roundUp((model.boundingBox.xMax - model.paramsAutoRoute.x_min + model.paramsAutoRoute.discretizationSizeLevel[0] * 0.5) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	model.paramsAutoRoute.y_min = model.boundingBox.yMin;
	model.paramsAutoRoute.nYbasLevel = roundUp((model.boundingBox.yMax - model.paramsAutoRoute.y_min + model.paramsAutoRoute.discretizationSizeLevel[0] * 0.5) / model.paramsAutoRoute.discretizationSizeLevel[0]);

	int i, i1, pos, i2, posTmp, posCellStart, posCellEnd, yPos, xPos, level, isLand;
	double x, y, xTmp, yTmp;

	identifyCellFromPoint(model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon, &yPos, &xPos, &level);
	posCellStart = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	identifyCellFromPoint(model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon, &yPos, &xPos, &level);
	posCellEnd = xPos + model.paramsAutoRoute.nXbasLevel * yPos;

	model.autoRoute = (strAutoCells*)malloc((model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel + 2) * sizeof(strAutoCells)); // +2 since start and end node as well
	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			model.autoRoute[pos].smallerCells = NULL;
			model.autoRoute[pos].smallerCellsType = 0;
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

			if (pos == 21)
				pos = pos;
			if (isLand == 0) {
				if (0 < model.paramsAutoRoute.nCellLevels - 1) {
					for (i2 = 0; i2 < 4; i2++) {
						if (model.autoRoute[pos].arcCost[i2] > model.paramsAutoRoute.maxBaseFeasibleCost) {
							break;
						}
					}
					if (i2 < 4 || posCellStart == pos || posCellEnd == pos) {
						model.autoRoute[pos].smallerCellsType = 1;
						set_nXY_smallCells(&(model.autoRoute[pos]));
						model.autoRoute[pos].smallerCells = addSmallerCells(&(model.autoRoute[pos]), x, y, 1);
						//if (model.autoRoute[pos].arcCost[0] > model.paramsAutoRoute.maxBaseFeasibleCost) {
						//	yTmp = y - model.paramsAutoRoute.discretizationSizeLevel[0];
						//	posTmp = i1 + model.paramsAutoRoute.nXbasLevel * (i - 1);
						//	if (posTmp == 11)
						//		pos = pos;
						//	if (yTmp > model.paramsAutoRoute.y_min - model.paramsAutoRoute.discretizationSizeLevel[0] * 0.1) {
						//		if(model.autoRoute[posTmp].isLand == 0)
						//			model.autoRoute[posTmp].smallerCells = addSmallerCells(model.autoRoute[posTmp].smallerCells, x, yTmp, 1);
						//	}
						//}
						//if (model.autoRoute[pos].arcCost[2] > model.paramsAutoRoute.maxBaseFeasibleCost) {
						//	xTmp = x - model.paramsAutoRoute.discretizationSizeLevel[0];
						//	if (xTmp > model.paramsAutoRoute.x_min - model.paramsAutoRoute.discretizationSizeLevel[0] * 0.1) {
						//		if (model.autoRoute[pos - 1].isLand == 0)
						//			model.autoRoute[pos - 1].smallerCells = addSmallerCells(model.autoRoute[pos - 1].smallerCells, xTmp, y, 1);
						//	}
						//}
					}
				}
			}
			pos++;
		}
	}

	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (pos == 9)
				pos = pos;
			if (model.autoRoute[pos].smallerCellsType == 0) {
				if (i1 > 0) {
					posTmp = pos - 1;
					if (model.autoRoute[posTmp].smallerCellsType == 1)
						model.autoRoute[pos].smallerCellsType = 3;
				}
				if (i > 0) {
					posTmp = pos - model.paramsAutoRoute.nXbasLevel;
					if (model.autoRoute[posTmp].smallerCellsType == 1) {
						if (model.autoRoute[pos].smallerCellsType == 3)
							model.autoRoute[pos].smallerCellsType = 4;
						else
							model.autoRoute[pos].smallerCellsType = 2;
					}
				}
				if (i1 > 0 && i > 0 && model.autoRoute[pos].smallerCellsType == 0) {
					posTmp = pos - 1 - model.paramsAutoRoute.nXbasLevel;
					if (model.autoRoute[posTmp].smallerCellsType == 1)
						model.autoRoute[pos].smallerCellsType = 5;
				}
				if (model.autoRoute[pos].smallerCellsType > 1) {
					set_nXY_smallCells(&(model.autoRoute[pos]));
					x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
					y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
					model.autoRoute[pos].smallerCells = addSmallerCells(&(model.autoRoute[pos]), x, y, 1);
				}
			}
			pos++;
		}
	}


	return 0;
}

int get_nextHeltal(double x, double dx) {
	int heltal;
	heltal = (int)x;
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

int createCells_new() {
	openNeededRasterFilesNew(-11);

	// model.paramsAutoRoute.x_min = model.boundingBox.xMin;
	model.paramsAutoRoute.nXbasLevel = roundUp((model.boundingBox.xMax - model.paramsAutoRoute.x_min + model.paramsAutoRoute.discretizationSizeLevel[0] * 0.5) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	// model.paramsAutoRoute.y_min = model.boundingBox.yMin;
	model.paramsAutoRoute.nYbasLevel = roundUp((model.boundingBox.yMax - model.paramsAutoRoute.y_min + model.paramsAutoRoute.discretizationSizeLevel[0] * 0.5) / model.paramsAutoRoute.discretizationSizeLevel[0]);

	int iPos, arcNr, nod, i, i1, pos, i2, posTmp, posCellStart, posCellEnd, yPos, xPos, level, isLand;
	double x, y, xTmp, yTmp, yPosDbl, xPosDbl, dx, dy, xNu, yNu, kvot, next_xKvot, next_yKvot, yPosDbl_prev, xPosDbl_prev;
	int nAlloc = (model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel + 2);
	model.autoRoute = (strAutoCells*)malloc(nAlloc * sizeof(strAutoCells)); // +2 since start and end node as well
	for (i = 0; i < nAlloc; i++)
		model.autoRoute[i].use = 0;

	for (iPos = -1; iPos <= modelSea.nBVArcs; iPos++)
	{
		if (iPos <= 0) {
			if (iPos == -1) {
				x = model.paramsAutoRoute.startPoint_lon;
				y = model.paramsAutoRoute.startPoint_lat;
			}
			else {
				arcNr = modelSea.BVArc[iPos];
				nod = modelSea.arc[arcNr].fromPointNr;
				y = modelSea.seaRoute.nod_y[nod];
				x = modelSea.seaRoute.nod_x[nod];
			}
		}
		else {
			if (iPos < modelSea.nBVArcs) {
				arcNr = modelSea.BVArc[iPos];
				nod = modelSea.arc[arcNr].toPointNr;
				y = modelSea.seaRoute.nod_y[nod];
				x = modelSea.seaRoute.nod_x[nod];
			}
			else {
				x = model.paramsAutoRoute.endPoint_lon;
				y = model.paramsAutoRoute.endPoint_lat;
			}
		}

		yPosDbl = (y - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		if (x < model.paramsAutoRoute.x_min)
			x += 360;
		if (x > model.paramsAutoRoute.x_min + 360)
			x -= 360;
		xPosDbl = (x - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0];
		if (iPos >= 0) {
			dx = xPosDbl - xPosDbl_prev;
			dy = yPosDbl - yPosDbl_prev;

			xNu = xPosDbl_prev;
			yNu = yPosDbl_prev;
			kvot = 0;
			if (iPos == 18)
				y = y;
			for (i = 0;; i++) {
				xPos = (int)xNu;
				yPos = (int)yNu;
				pos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
				if (model.autoRoute[pos].use == 0)
					model.autoRoute[pos].firstUsePathPos = iPos;
				model.autoRoute[pos].lastUsePathPos = iPos;
				model.autoRoute[pos].use = 1;
				if (xNu - xPos < 0.5 && xPos > 0) {
					pos = xPos - 1 + model.paramsAutoRoute.nXbasLevel * yPos;
					if (model.autoRoute[pos].use == 0)
						model.autoRoute[pos].firstUsePathPos = iPos;
					model.autoRoute[pos].lastUsePathPos = iPos;
					model.autoRoute[pos].use = 1;
				}
				else {
					if (xNu - xPos > 0.5 && xPos < model.paramsAutoRoute.nXbasLevel - 1) {
						pos = xPos + 1 + model.paramsAutoRoute.nXbasLevel * yPos;
						if (model.autoRoute[pos].use == 0)
							model.autoRoute[pos].firstUsePathPos = iPos;
						model.autoRoute[pos].lastUsePathPos = iPos;
						model.autoRoute[pos].use = 1;
					}
				}
				if (yNu - yPos < 0.5 && yPos > 0) {
					pos = xPos + model.paramsAutoRoute.nXbasLevel * (yPos - 1);
					if (model.autoRoute[pos].use == 0)
						model.autoRoute[pos].firstUsePathPos = iPos;
					model.autoRoute[pos].lastUsePathPos = iPos;
					model.autoRoute[pos].use = 1;
				}
				else {
					if (yNu - yPos > 0.5 && yPos < model.paramsAutoRoute.nYbasLevel - 1) {
						pos = xPos + model.paramsAutoRoute.nXbasLevel * (yPos + 1);
						if (model.autoRoute[pos].use == 0)
							model.autoRoute[pos].firstUsePathPos = iPos;
						model.autoRoute[pos].lastUsePathPos = iPos;
						model.autoRoute[pos].use = 1;
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

	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
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

	/*
	pos = 0;
	for (i = 0; i < model.paramsAutoRoute.nYbasLevel; i++) {
		for (i1 = 0; i1 < model.paramsAutoRoute.nXbasLevel; i1++) {
			if (pos == 9)
				pos = pos;
			if (model.autoRoute[pos].smallerCellsType == 0) {
				if (i1 > 0) {
					posTmp = pos - 1;
					if (model.autoRoute[posTmp].smallerCellsType == 1)
						model.autoRoute[pos].smallerCellsType = 3;
				}
				if (i > 0) {
					posTmp = pos - model.paramsAutoRoute.nXbasLevel;
					if (model.autoRoute[posTmp].smallerCellsType == 1) {
						if (model.autoRoute[pos].smallerCellsType == 3)
							model.autoRoute[pos].smallerCellsType = 4;
						else
							model.autoRoute[pos].smallerCellsType = 2;
					}
				}
				if (i1 > 0 && i > 0 && model.autoRoute[pos].smallerCellsType == 0) {
					posTmp = pos - 1 - model.paramsAutoRoute.nXbasLevel;
					if (model.autoRoute[posTmp].smallerCellsType == 1)
						model.autoRoute[pos].smallerCellsType = 5;
				}
				if (model.autoRoute[pos].smallerCellsType > 1) {
					set_nXY_smallCells(&(model.autoRoute[pos]));
					x = model.paramsAutoRoute.x_min + i1 * model.paramsAutoRoute.discretizationSizeLevel[0];
					y = model.paramsAutoRoute.y_min + i * model.paramsAutoRoute.discretizationSizeLevel[0];
					model.autoRoute[pos].smallerCells = addSmallerCells(&(model.autoRoute[pos]), x, y, 1);
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

int addAutoNodeTss(int tssNr, double y, double x) {
	int posItss;
	if (model.nNoder >= model.nAllocNoder) {
		model.nAllocNoder += 100000;
		model.Noder = (strNoder*)realloc(model.Noder, model.nAllocNoder * sizeof(strNoder));
	}

	posItss = model.tss[tssNr].nNoder;
	if (posItss >= model.tss[tssNr].nAllocNoder) {
		model.tss[tssNr].nAllocNoder += 100;
		model.tss[tssNr].nodNr = (int*)realloc(model.tss[tssNr].nodNr, model.tss[tssNr].nAllocNoder * sizeof(int));
		model.tss[tssNr].nodCoord_y = (double*)realloc(model.tss[tssNr].nodCoord_y, model.tss[tssNr].nAllocNoder * sizeof(double));
		model.tss[tssNr].nodCoord_x = (double*)realloc(model.tss[tssNr].nodCoord_x, model.tss[tssNr].nAllocNoder * sizeof(double));
	}
	model.Noder[model.nNoder].physicalLevel = -1;
	model.Noder[model.nNoder].pointNr = tssNr;
	model.Noder[model.nNoder].timeInterval = posItss;
	model.Noder[model.nNoder].nAllocUtNoder = 0;
	model.Noder[model.nNoder].nUtNoder = 0;
	model.tss[tssNr].nodNr[posItss] = model.nNoder;

	model.tss[tssNr].nodCoord_y[posItss] = y;
	model.tss[tssNr].nodCoord_x[posItss] = x;

	(model.tss[tssNr].nNoder)++;

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
	if (model.nArcs == 373)
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
	if (model.nArcs == 293345)
		model.nArcs = model.nArcs;
	return cost;
}

double addAutoArcSmallTss(int tssNr, int posItss, int prev_posItss, int cellPos, int cellSmall, int posTmp, double dist, int direction) {
	double cost, kvotCost = 1.0;
	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc, model.nAllocArcs * sizeof(strArcInfo));
	}

	if (model.nArcs == 15472)
		model.nArcs = model.nArcs;

	if (direction == -1) { // in to tss
		if (prev_posItss >= 0) {
			model.arc[model.nArcs].fromLevel = -1;
			model.arc[model.nArcs].fromPointNr = tssNr;
			model.arc[model.nArcs].fromTime = prev_posItss;
			kvotCost = 0.01;
		}
		else {
			model.arc[model.nArcs].fromLevel = cellPos;
			model.arc[model.nArcs].fromPointNr = posTmp;
			model.arc[model.nArcs].fromTime = cellSmall;
		}
		model.arc[model.nArcs].toLevel = -1;
		model.arc[model.nArcs].toPointNr = tssNr;
		model.arc[model.nArcs].toTime = posItss;
	}
	else {
		model.arc[model.nArcs].fromLevel = -1;
		model.arc[model.nArcs].fromPointNr = tssNr;
		model.arc[model.nArcs].fromTime = posItss;
		model.arc[model.nArcs].toLevel = cellPos;
		model.arc[model.nArcs].toTime = cellSmall;
		model.arc[model.nArcs].toPointNr = posTmp;
	}
	model.arc[model.nArcs].distance = dist;
	cost = kvotCost * dist;
	model.arc[model.nArcs].totCost = cost;

	(model.nArcs)++;
	if (model.nArcs == 373)
		model.nArcs = model.nArcs;
	return cost;
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

	if (model.nArcs == 15472)
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

	(model.nArcs)++;
	if (model.nArcs == 373)
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
	if (model.nArcs == 373)
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

int addArcsInOutFromTssNode(int nodNr, int tssNr, int prevNodNr, double distPrev) {
	int nodPos, posItss, yPos, xPos, yPos2, xPos2, cellPos, cellSmall, i, nodNr2, nodPos2;
	double cost, y, x, y0, x0, y1, x1, yLocal, xLocal, kvotBad, dist;

	if (nodNr == 1929)
		nodNr = nodNr;

	posItss = model.tss[tssNr].nNoder - 1;

	if (prevNodNr >= 0) {
		nodPos = model.Noder[prevNodNr].nUtNoder;
		model.Noder[prevNodNr].UtNod[nodPos] = nodNr;
		model.Noder[prevNodNr].outArcNr[nodPos] = model.nArcs;
		cost = addAutoArcSmallTss(tssNr, posItss, posItss - 1, -1, -1, -1, distPrev, -1);
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
	y = model.tss[tssNr].nodCoord_y[posItss];
	x = model.tss[tssNr].nodCoord_x[posItss];
	yPos = (int)((y - model.paramsAutoRoute.y_min) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	xPos = (int)((x - model.paramsAutoRoute.x_min) / model.paramsAutoRoute.discretizationSizeLevel[0]);
	yLocal = y - yPos * model.paramsAutoRoute.discretizationSizeLevel[0] - model.paramsAutoRoute.y_min;
	xLocal = x - xPos * model.paramsAutoRoute.discretizationSizeLevel[0] - model.paramsAutoRoute.x_min;
	yPos2 = (int)(yLocal / model.paramsAutoRoute.discretizationSizeLevel[1]);
	xPos2 = (int)(xLocal / model.paramsAutoRoute.discretizationSizeLevel[1]);

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
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

		kvotBad = check_map_badKvot_auto(y1, x1, y, x, 0, -1);
		if (kvotBad < 0.001) { // only add allowed arcs
			dist = estimateLargeCircleDistance_km(y1, x1, y, x);

			nodNr2 = model.autoRoute[cellPos].smallerCells[cellSmall].nodeNr[i];
			nodPos2 = model.Noder[nodNr2].nUtNoder;
			checkAllocNode(nodNr2);
			model.Noder[nodNr2].UtNod[nodPos2] = nodNr;
			model.Noder[nodNr2].outArcNr[nodPos2] = model.nArcs;
			cost = addAutoArcSmallTss(tssNr, posItss, -1, cellPos, cellSmall, i, dist, -1);
			model.Noder[nodNr2].UtNodCost[nodPos2++] = cost;
			model.Noder[nodNr2].nUtNoder = nodPos2;

			model.Noder[nodNr].UtNod[nodPos] = nodNr2;
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
			cost = addAutoArcSmallTss(tssNr, posItss, -1, cellPos, cellSmall, i, dist, 1);
			model.Noder[nodNr].UtNodCost[nodPos++] = cost;
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
	model.autoRoute[posEnd].y = model.paramsAutoRoute.startPoint_lat;
	model.autoRoute[posEnd].x = model.paramsAutoRoute.startPoint_lon;
	posTmp = 0;// 0, down left
	y = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	x = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y, x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 0, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 1, down right
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y, x + model.paramsAutoRoute.discretizationSizeLevel[0]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 1, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 2, up left
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArc(posEnd, 0, cellPos, 2, posTmp);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 3, up right
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
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
	yPos2 = (model.paramsAutoRoute.startPoint_lat - model.paramsAutoRoute.y_min - yPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	xPos2 = (model.paramsAutoRoute.startPoint_lon - model.paramsAutoRoute.x_min - xPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	//cellPos2 = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;

	nodNr = model.nNoder - 2;

	model.Noder[nodNr].nAllocUtNoder = 16;
	model.Noder[nodNr].UtNod = (int*)malloc(16 * sizeof(int));
	model.Noder[nodNr].outArcNr = (int*)malloc(16 * sizeof(int));
	model.Noder[nodNr].UtNodCost = (double*)malloc(16 * sizeof(double));

	nodPos = 0;


	posEnd = model.paramsAutoRoute.nCellsBase;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.startPoint_lat;
	model.autoRoute[posEnd].x = model.paramsAutoRoute.startPoint_lon;
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
			evalCostArc2(model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
				y1, x1, &cost, &dist);
			model.Noder[nodNr].outArcNr[pos] = model.nArcs;
			cost = addAutoArcSmall2(posEnd, -1, 0, cellPos, cellPos2, posNod, cost, dist);
			model.Noder[nodNr].UtNodCost[pos] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
			pos++;
		}
	}
	model.Noder[nodNr].nUtNoder = pos;
	model.autoRoute[posEnd].nodeNr[0] = nodNr;

	/*
	posTmp = 0;// 0, down left
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y, x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(posEnd, -1, 0, cellPos, cellPos2, 0, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 1, down right
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y, x + model.paramsAutoRoute.discretizationSizeLevel[1]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(posEnd, -1, 0, cellPos, cellPos2, 1, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 2, up left
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y + model.paramsAutoRoute.discretizationSizeLevel[1], x);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(posEnd, -1, 0, cellPos, cellPos2, 2, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];

	// 3, up right
	model.Noder[nodNr].UtNod[nodPos] = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon,
		y + model.paramsAutoRoute.discretizationSizeLevel[1], x + model.paramsAutoRoute.discretizationSizeLevel[1]);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(posEnd, -1, 0, cellPos, cellPos2, 3, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;
	*/

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


	return 0;
}

int addArcsToEndNode(int yPos, int xPos, int level) {
	int nodPos, posEnd, posTmp, cellPos, nodNr, nodEnd;
	double y, x, cost;

	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	nodEnd = model.nNoder - 1;
	model.Noder[nodEnd].nUtNoder = 0;

	posEnd = model.paramsAutoRoute.nYbasLevel * model.paramsAutoRoute.nXbasLevel + 1;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.endPoint_lat;
	model.autoRoute[posEnd].x = model.paramsAutoRoute.endPoint_lon;

	nodPos = 0;
	// connect to all four corners of the cell

	posTmp = 0;// 0, down left
	y = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	x = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0];
	nodNr = model.autoRoute[cellPos].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc_cells(&(model.autoRoute[posEnd]), posTmp, y, x, model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
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
		y, x + model.paramsAutoRoute.discretizationSizeLevel[0], model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
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
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x, model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
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
		y + model.paramsAutoRoute.discretizationSizeLevel[0], x, model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
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
	yPos2 = (model.paramsAutoRoute.endPoint_lat - model.paramsAutoRoute.y_min - yPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	xPos2 = (model.paramsAutoRoute.endPoint_lon - model.paramsAutoRoute.x_min - xPos * model.paramsAutoRoute.discretizationSizeLevel[0]) / model.paramsAutoRoute.discretizationSizeLevel[1];
	//cellPos2 = xPos2 + model.paramsAutoRoute.nDiscreteSizeLevel[1] * yPos2;

	nodEnd = model.nNoder - 1;
	model.Noder[nodEnd].nUtNoder = 0;

	posEnd = model.paramsAutoRoute.nCellsBase + 1;
	model.autoRoute[posEnd].y = model.paramsAutoRoute.endPoint_lat;
	model.autoRoute[posEnd].x = model.paramsAutoRoute.endPoint_lon;
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
			evalCostArc2(y1, x1, model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon, &cost, &dist);
			model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
			cost = addAutoArcSmall2(cellPos, cellPos2, posNod, posEnd, -1, 0, cost, dist);
			model.Noder[nodNr].UtNodCost[nodPos] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
			model.Noder[nodNr].nUtNoder = nodPos + 1;
			pos++;
		}
	}

	/*
	nodPos = 0;
	// connect to all four corners of the cell

	posTmp = 0;// 0, down left
	y = model.paramsAutoRoute.y_min + yPos * model.paramsAutoRoute.discretizationSizeLevel[0] + yPos2 * model.paramsAutoRoute.discretizationSizeLevel[1];
	x = model.paramsAutoRoute.x_min + xPos * model.paramsAutoRoute.discretizationSizeLevel[0] + xPos2 * model.paramsAutoRoute.discretizationSizeLevel[1];
	nodNr = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc(&(model.autoRoute[posEnd]), posTmp, y, x, model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(cellPos, cellPos2, 0, posEnd, -1, 0, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	// 1, down right
	nodNr = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc(&(model.autoRoute[posEnd]), posTmp,
		y, x + model.paramsAutoRoute.discretizationSizeLevel[1], model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(cellPos, cellPos2, 1, posEnd, -1, 0, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	// 2, up left
	nodNr = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc(&(model.autoRoute[posEnd]), posTmp,
		y + model.paramsAutoRoute.discretizationSizeLevel[1], x, model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(cellPos, cellPos2, 2, posEnd, -1, 0, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;

	// 3, up right
	nodNr = model.autoRoute[cellPos].smallerCells[cellPos2].nodeNr[posTmp];
	nodPos = model.Noder[nodNr].nUtNoder;
	checkAllocNode(nodNr);
	model.Noder[nodNr].UtNod[nodPos] = nodEnd;
	evalCostArc(&(model.autoRoute[posEnd]), posTmp,
		y + model.paramsAutoRoute.discretizationSizeLevel[1], x + model.paramsAutoRoute.discretizationSizeLevel[1], model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon);
	model.Noder[nodNr].outArcNr[nodPos] = model.nArcs;
	cost = addAutoArcSmall(cellPos, cellPos2, 3, posEnd, -1, 0, posTmp++);
	model.Noder[nodNr].UtNodCost[nodPos++] = cost; // model.autoRoute[posEnd].arcCost[posTmp++];
	model.Noder[nodNr].nUtNoder = nodPos;
	*/
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
			if (pos == 55)
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
							if (i4 > 0) {
								model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = model.autoRoute[pos].smallerCells[posSmall - 1].nodeNr[3];
								// addAutoNode(pos, 2, posSmall);
							}
							else { // i4 == 0
								if (i3 < model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1) {
									if (pos > 0) {
										if (model.autoRoute[pos - 1].smallerCellsType == 1) {
											posTmpNu = (model.paramsAutoRoute.nDiscreteSizeLevel[level] - 1) + model.paramsAutoRoute.nDiscreteSizeLevel[level] * i3;
											model.autoRoute[pos].smallerCells[posSmall].nodeNr[2] = model.autoRoute[pos - 1].smallerCells[posTmpNu].nodeNr[3];
										}
										else
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
	identifyCellFromPoint(model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon, &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);
	addAutoNode(cellPos, -level - 1, -1);
	checkMinnesAnvandning(__LINE__);

	// addArcsFromStartNode(yPos, xPos, level);
	addArcsSmallFromStartNode(yPos, xPos, level);

	// connect to all four corners of the cell

	// add nod and arcs for the end of the route
	identifyCellFromPoint(model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon, &yPos, &xPos, &level);
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
	identifyCellFromPoint(model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon, &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);
	addAutoNode(cellPos, -level - 1, -1);
	checkMinnesAnvandning(__LINE__);

	// addArcsFromStartNode(yPos, xPos, level);
	addArcsSmallFromStartNode(yPos, xPos, level);

	// connect to all four corners of the cell

	// add nod and arcs for the end of the route
	identifyCellFromPoint(model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon, &yPos, &xPos, &level);
	cellPos = xPos + model.paramsAutoRoute.nXbasLevel * yPos;
	addAutoNode(cellPos, -level - 1, -1);

	// addArcsToEndNode(yPos, xPos, level);
	addArcsSmallToEndNode(yPos, xPos, level);

	int saveNodes = 1;
	if (saveNodes == 1)
		writeAllAutoNodesToGeojson(2);

	int saveArcs = 1;
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
			*y = model.tss[posTmp].nodCoord_y[level];
			*x = model.tss[posTmp].nodCoord_x[level];
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
		nodNr = model.tss[posTmp].nodNr[level];
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

int writeSolutionToJson_autoRoute(std::string filename, int iter)
{
	FILE* filpekG, *filpekG2;

	FILE* filPek2 = NULL;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	if (SKRIV_UT_NOTHING == 0) {
		sprintf(namn, "%s/autoPath_%d.txt", model.params.indataPath.c_str(), iter);
		filPek2 = fopen(namn, "w");
		fprintf(filPek2, "pos;arcNr;fromCellNr;fromSmallCell1;fromPosIcell;toCellPos2;toSmallCell2;toPosIcell2;distance;cost\n");
	}

	if (iter == 0) {
		sprintf(namn, "%s/res_autoIter0.json", model.params.indataPath.c_str());
		filpekG = fopen(namn, "w");
	}
	else {
		sprintf(namn, "%s/%s", model.params.indataPath.c_str(), filename.c_str());
		filpekG = fopen(filename.c_str(), "w");
	}
	if (filpekG == NULL)
	{
		printf("Faile to open file %s for writing.\n", namn);
		errlog("Faile to open file %s for writing.\n", namn);
		postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
	}
	sprintf(namn, "%s/res_autoIter_arc%d.json", model.params.indataPath.c_str(), iter);
	filpekG2 = fopen(namn, "w");
	if (filpekG2 == NULL)
	{
		printf("Faile to open file %s for writing.\n", namn);
		postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
	}


	initGeoJsonFil(filpekG, "result_path");
	initGeoJsonFil(filpekG2, "result_arcs");

	fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
	int iPos, arcNr, nInt, i;
	double totCost = 0, totDist = 0, y, x, y0, x0, splitDist, y1, x1;
	spherical::Point p2, p3;
	for (iPos = 1; iPos < model.nBVArcs; iPos++)
	{
		arcNr = model.BVArc[iPos];

		totCost += model.arc[arcNr].totCost;
		totDist += model.arc[arcNr].distance;

		if (iPos == 1) {
			getCoordFromAutoArc(arcNr, 0, &y, &x);
			fprintf(filpekG, "[%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			y0 = y;
			x0 = x;
		}
		if (iPos > 1)
			fprintf(filpekG2, ", ");
		fprintf(filpekG2, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"LineString\",\n\"coordinates\": [[%lf, %lf]",
			getCorrect_longitude(x0), y0);

		if (iPos == 1315)
			iPos = iPos;
		getCoordFromAutoArc(arcNr, 1, &y1, &x1);
		nInt = roundUp(model.arc[arcNr].distance / DIST_SPLIT);
		if (nInt > 1) {
			p3 = spherical::Point(y0, x0);
			p2 = spherical::Point(y1, x1);
			splitDist = model.arc[arcNr].distance / nInt * 1000.0;
		}
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
			fprintf(filpekG, ", [%lf, %lf, 0.0]", getCorrect_longitude(x), y);
			fprintf(filpekG2, ", [%lf, %lf]", getCorrect_longitude(x), y);
		}
		fprintf(filpekG2, "]}\n,\n\"properties\": {\"iPos\":%d, \"arcNr\":%d, \"cost\": %lf,\n\"dist\": %lf, \"accumCost\": %lf,\n\"accumDist\": %lf}}\n",
			iPos, arcNr, model.arc[arcNr].totCost, model.arc[arcNr].distance, totCost, totDist);

		fprintf(filPek2, "%d;%d;%d;%d;%d;%d;%d;%d;%lf;%lf\n", iPos, arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromTime, model.arc[arcNr].fromPointNr,
			model.arc[arcNr].toLevel, model.arc[arcNr].toTime, model.arc[arcNr].toPointNr, model.arc[arcNr].distance, model.arc[arcNr].totCost);
		y0 = y;
		x0 = x;
	}

	fprintf(filpekG, "\n]]},\n\"properties\": {\n");
	fprintf(filpekG, "\"totCost\": %lf,\n\"totDistance\": %lf}}\n]}\n", totCost, totDist);
	fclose(filpekG);
	fclose(filPek2);
	fprintf(filpekG2, "\n]}\n");
	fclose(filpekG2);

	return 0;
}

int addArcsAroundSolution()
{

	// from each nod, go forward to nodes along the solution path looking for feasible arcs.
	// as long as same direction as the arcs then no need to add new arc or not feasible
	// stop after 
	// stop after one or two nodes after attaching to tss
	// skip tss and only start at the last few nodes of tss

	int iPos, arcNr, nEjBattre, i1, arcNr1, nod1, nod2, tssNod1, tssNod2, nTssNodes;
	double totCost = 0, totDist = 0, y, x, y1, x1, costNod1, costDiff, kvotBad, dist, cost;
	for (iPos = 1; iPos < model.nBVArcs; iPos++)
	{
		arcNr = model.BVArc[iPos];
		if (iPos == 11)
			iPos = iPos;
		nod1 = getNodFromArcNr(arcNr, 0, &tssNod1);
		costNod1 = (model.Dijkstra.nodes - model.Dijkstra.node_min + nod1)->dist / model.Dijkstra.FAKTOR_NATVERK;

		totCost += model.arc[arcNr].totCost;
		totDist += model.arc[arcNr].distance;

		getCoordFromAutoArc(arcNr, 0, &y, &x);
		nEjBattre = 0;
		nTssNodes = 0;
		if (iPos == 1315)
			iPos = iPos;
		for (i1 = iPos + 1; i1 < model.nBVArcs; i1++) {
			arcNr1 = model.BVArc[i1];
			getCoordFromAutoArc(arcNr1, 1, &y1, &x1);

			nod2 = getNodFromArcNr(arcNr1, 1, &tssNod2);
			costDiff = (model.Dijkstra.nodes - model.Dijkstra.node_min + nod2)->dist / model.Dijkstra.FAKTOR_NATVERK - costNod1;
			cost = evalCostArc(y, x, y1, x1, &dist);
			// dist = estimateLargeCircleDistance_km(y, x, y1, x1);
			if (cost < costDiff * 0.99999) {
				// add arc
				addAutoArcExtra(arcNr, arcNr1, nod1, nod2, dist, cost);
				nEjBattre = 0;
				if (tssNod2 == 1)
					nTssNodes++;
				else
					nTssNodes = 0;
			}else
				nEjBattre++;
			if (nEjBattre > 10 || nTssNodes >= 2)
				break;
		}

		if (iPos == 1) {
			getCoordFromAutoArc(arcNr, 0, &y, &x);
		}
		getCoordFromAutoArc(arcNr, 1, &y, &x);
	}

	return 0;
}

int writeSolutionToJson_seaRoute(std::string filename)
{
	FILE* filpekG;
	double minX, maxX, minY, maxY;

	minX = model.paramsAutoRoute.startPoint_lon;
	maxX = minX;
	minY = model.paramsAutoRoute.startPoint_lat;
	maxY = minY;

	FILE* filPek2 = NULL;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	if (SKRIV_UT_NOTHING == 0) {
		sprintf(namn, "%s/seaRoutePath.txt", model.params.indataPath.c_str());
		filPek2 = fopen(namn, "w");
		fprintf(filPek2, "pos;arcNr;fromNod;toNod;distance;cost\n");
	}
	std::string linePath = "";

	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), filename.c_str());
	filpekG = fopen(namn, "w");
	if (filpekG == NULL)
	{
		printf("Faile to open file %s for writing.\n", namn);
		errlog("Faile to open file %s for writing.\n", namn);
		postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
	}
	initGeoJsonFil(filpekG, "result_path");
	linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";
	model.network.last_x = -1000;
	fprintf(filpekG, "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n");
	int iPos, arcNr, nod;
	double totCost = 0, totDist = 0, y, x, last_x = minX;
	for (iPos = -1; iPos <= modelSea.nBVArcs; iPos++)
	{
		if (iPos <= 0) {
			if (iPos == -1) {
				x = model.paramsAutoRoute.startPoint_lon;
				y = model.paramsAutoRoute.startPoint_lat;
				x = getCorrect_longitude(x);
				fprintf(filpekG, "[%lf, %lf, 0.0]", x, y);
			}
			else {
				arcNr = modelSea.BVArc[iPos];
				nod = modelSea.arc[arcNr].fromPointNr;
				y = modelSea.seaRoute.nod_y[nod];
				x = modelSea.seaRoute.nod_x[nod];
				totCost += modelSea.arc[arcNr].totCost;
				totDist += modelSea.arc[arcNr].distance;
			}
		}
		else {
			if (iPos < modelSea.nBVArcs) {
				arcNr = modelSea.BVArc[iPos];
				nod = modelSea.arc[arcNr].toPointNr;
				y = modelSea.seaRoute.nod_y[nod];
				x = modelSea.seaRoute.nod_x[nod];
				totCost += modelSea.arc[arcNr].totCost;
				totDist += modelSea.arc[arcNr].distance;
			}
			else {
				x = model.paramsAutoRoute.endPoint_lon;
				y = model.paramsAutoRoute.endPoint_lat;
			}
		}
		x = getCorrect_longitude(x);
		if(iPos >= 0){
			fprintf(filpekG, ", [%lf, %lf, 0.0]", x, y);
			if(iPos < modelSea.nBVArcs)
				fprintf(filPek2, "%d;%d;%d;%d;%lf;%lf\n", iPos, arcNr, modelSea.arc[arcNr].fromPointNr,
					modelSea.arc[arcNr].toPointNr, modelSea.arc[arcNr].distance, modelSea.arc[arcNr].totCost);
			else
				fprintf(filPek2, "%d;%d;%d;%d;%lf;%lf\n", iPos, -1, -1,
					-1, -1, -1);
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

	fprintf(filpekG, "\n]]},\n\"properties\": {\n");
	fprintf(filpekG, "\"totCost\": %lf,\n\"totDistance\": %lf}}\n]}\n", totCost, totDist);
	fclose(filpekG);
	fclose(filPek2);

	minY -= model.paramsAutoRoute.discretizationSizeLevel[0];
	minX -= model.paramsAutoRoute.discretizationSizeLevel[0];
	maxY += 2 * model.paramsAutoRoute.discretizationSizeLevel[0]; // to make room for full cells
	maxX += 2 * model.paramsAutoRoute.discretizationSizeLevel[0]; // to make room for full cells

	if (minY < -90)
		minY = 90;
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

int genAutoRoute(std::string inputPath, std::string resultName) {
	int nod1, nod2;
	double dist, dist1, dist2;
	long long Cost;
	bool Reached;

	reset_errlog();
	model.params.resultPath = splitFilename(resultName);
	model.params.resultName = resultName;
	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;

	initLookUpTables();

	loadParams_theRestOld(&(model.params));
	loadParams_autoRoute(&(model.paramsAutoRoute));


	int solveHere = 1;
	if (solveHere == 1) {
		modelSea.Dijkstra.nodes = NULL;

		checkMinnesAnvandning(__LINE__);
		if (model.paramsAutoRoute.newSeaRoutePathData == 1)
			load_searoutes();
		else
			load_saved_searoutes();

		checkMinnesAnvandning(__LINE__);
		SattUppDijkstraNatverk3(&modelSea);
		checkMinnesAnvandning(__LINE__);
		nod1 = identify_nearestNode_toSearoutes(model.paramsAutoRoute.startPoint_lat, model.paramsAutoRoute.startPoint_lon, &dist1);
		nod2 = identify_nearestNode_toSearoutes(model.paramsAutoRoute.endPoint_lat, model.paramsAutoRoute.endPoint_lon, &dist2);

		checkMinnesAnvandning(__LINE__);
		modelSea.BVArc = (int*)malloc2(modelSea.nNoder * sizeof(int));
		modelSea.BVtempNodOrder = (int*)malloc2(modelSea.nNoder * sizeof(int));

		printf("\nsolving dijkstra's algorithm for searoute..");
		checkMinnesAnvandning(__LINE__);
		AnropDijkstra2(nod1, nod2, &modelSea, &Reached);
		checkMinnesAnvandning(__LINE__);
		printf("..done. Obj %I64d\n", modelSea.Dijkstra.OptCost);
		if (Reached == true) {
			dist = NystaUppBV_MassTest(&modelSea, Reached, nod1, nod2, &Cost);
			if (modelSea.nBVArcs < 2) {
				errlog("ERROR! Too few arcs %d in Dijkstra solution for seaRoute\n", modelSea.nBVArcs);
			}
			else {
				writeSolutionToJson_seaRoute("seaRoute.geojson");//  model.params.resultPath + "/resAutoRoute.json");
			}
			checkMinnesAnvandning(__LINE__);
		}
		else {
			errlog("ERROR! Did not manage to find a route from start to finish for seaRoute...\n");
			printf("\nERROR! Did not manage to find a route from start to finish for seaRoute...\n");
		}
	}

	//createCells();
	createCells_new();
	checkMinnesAnvandning(__LINE__);

	createCellNetwork();
	checkMinnesAnvandning(__LINE__);
	load_tss();
	save_tss_geojson();
	addArcs_tss();

	int saveNodes = 0;
	if (saveNodes == 1)
		writeAllAutoNodesToGeojson(1);

	int saveArcs = 0;
	if (saveArcs == 1)
		writeAllAutoArcsToGeojson(1);
	checkMinnesAnvandning(__LINE__);

	model.Dijkstra.nodes = NULL;
	for (int iter = 0; iter < 2; iter++) {
		

		SattUppDijkstraNatverk3(&model);
		checkMinnesAnvandning(__LINE__);

		nod1 = model.autoRoute_startNod;
		nod2 = model.autoRoute_endNod; 

		model.BVArc = (int*)malloc2(model.nNoder * sizeof(int));
		model.BVtempNodOrder = (int*)malloc2(model.nNoder * sizeof(int));

		printf("\nsolving dijkstra's algorithm..");
		AnropDijkstra2(nod1, nod2, &model, &Reached);
		printf("..done. Obj %I64d\n", model.Dijkstra.OptCost);
		if (Reached == true) {
			dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
			if (model.nBVArcs < 2) {
				errlog("ERROR! Too few arcs %d in Dijkstra solution\n", model.nBVArcs);
			}
			else
				writeSolutionToJson_autoRoute(resultName, iter);//  model.params.resultPath + "/resAutoRoute.json");
			checkMinnesAnvandning(__LINE__);
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
	}



	return 0;
}