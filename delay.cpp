#include "pch.h"
#include <time.h>
#include <cmath>
#include<fstream>

extern double cos_table[20001];
extern double sin_table[20001];
extern double atan_table[20001];
extern double LOOKUP_COS_STEP_INV;
extern strModel model; 
extern std::string resultPath;
extern int SKRIV_UT_NOTHING;


using json = nlohmann::json;
int skrivMycket = 0;

int getDirection(double y1, double x1, double y2, double x2) {
	double xDiff = x2 - x1;
	if (xDiff > 180)
		xDiff -= 360;
	if (xDiff < -180)
		xDiff += 360;

	double angle = atan2(y2 - y1, xDiff) * 180 / M_PI;
	if (angle < 0)
		angle += 360;
	int angleInt = round(angle / 45.0);

	return angleInt * 45;
}

/*
int loadNodeAndNeighbours_delayedFactors() {
	int nAlloc, direction, antal, pos, nNu, i;
	double x, y;

	model.delay.nYears = 4;
	model.delay.year = (int*)malloc2(model.delay.nYears * sizeof(int));
	for (int i = 0; i < model.delay.nYears; i++)
		model.delay.year[i] = 2018 + i;

	model.delay.nNodes = 0;
	nAlloc = 100;
	model.delay.baseNode = (strPointxy*)malloc2(nAlloc * sizeof(strPointxy));
	model.delay.neighbour = (strPointxy**)malloc2(nAlloc * sizeof(strPointxy*));
	model.delay.nNodeNeighbours = (int*)malloc2(nAlloc * sizeof(int));

	FILE* filpek = fopen("data/neighbours.txt", "r");
	for (i = 0; i < 10000; i++) {
		antal = fscanf(filpek, "%d\t%lf\t%lf\n", &pos, &x, &y);
		if (antal <= 0)
			break;
		if (pos == 0) {
			if (model.delay.nNodes >= nAlloc) {
				nAlloc += 100;

			}
			model.delay.baseNode[model.delay.nNodes].point_x = x;
			model.delay.baseNode[model.delay.nNodes].point_y = y;
			model.delay.nNodeNeighbours[model.delay.nNodes] = 0;
			model.delay.neighbour[model.delay.nNodes] = (strPointxy*)malloc2(8 * sizeof(strPointxy));
			(model.delay.nNodes)++;
		}
		else {
			nNu = model.delay.nNodeNeighbours[model.delay.nNodes - 1];
			model.delay.neighbour[model.delay.nNodes - 1][nNu].point_x = x;
			model.delay.neighbour[model.delay.nNodes - 1][nNu].point_y = y;
			model.delay.neighbour[model.delay.nNodes - 1][nNu].rowNr = i + 1;
			direction = getDirection(model.delay.baseNode[model.delay.nNodes - 1].point_y,
				model.delay.baseNode[model.delay.nNodes - 1].point_x,
				model.delay.neighbour[model.delay.nNodes - 1][nNu].point_y,
				model.delay.neighbour[model.delay.nNodes - 1][nNu].point_x);
			model.delay.neighbour[model.delay.nNodes-1][nNu].direction = direction;
			(model.delay.nNodeNeighbours[model.delay.nNodes - 1])++;
		}
	}
	fclose(filpek);

	double maxDist, dist;
	int maxPos, rowNr;
	for (i = 0; i < model.delay.nNodes; i++) {
		maxDist = 0;
		maxPos = 0;
		for (int i1 = 0; i1 < model.delay.nNodeNeighbours[i]; i1++) {
			dist = estimateLargeCircleDistance_km(model.delay.baseNode[i].point_y, model.delay.baseNode[i].point_x,
				model.delay.neighbour[i][i1].point_y, model.delay.neighbour[i][i1].point_x);
			if (maxDist < dist) {
				maxDist = dist;
				maxPos = i1;
			}
		}
		if (maxPos > 0) {
			y = model.delay.neighbour[i][0].point_y;
			model.delay.neighbour[i][0].point_y = model.delay.neighbour[i][maxPos].point_y;
			model.delay.neighbour[i][maxPos].point_y = y;
			x = model.delay.neighbour[i][0].point_x;
			model.delay.neighbour[i][0].point_x = model.delay.neighbour[i][maxPos].point_x;
			model.delay.neighbour[i][maxPos].point_x = x;
			direction = model.delay.neighbour[i][0].direction;
			model.delay.neighbour[i][0].direction = model.delay.neighbour[i][maxPos].direction;
			model.delay.neighbour[i][maxPos].direction = direction;
			rowNr = model.delay.neighbour[i][0].rowNr;
			model.delay.neighbour[i][0].rowNr = model.delay.neighbour[i][maxPos].rowNr;
			model.delay.neighbour[i][maxPos].rowNr = rowNr;
		}
	}


	nAlloc = 2;

	model.preferredPath.point = (spherical::Point*)malloc2(nAlloc * sizeof(spherical::Point));
	model.preferredPath.point_y = (double*)malloc2(nAlloc * sizeof(double));
	model.preferredPath.point_x = (double*)malloc2(nAlloc * sizeof(double));

	return 0;
}
*/

int loadGroups_delayedFactors() {
	int nAlloc, direction, antal, groupNr, arcNr, nNu, i, nAllocGrupp, nDir = 2, startDir = 0;
	double x1, y1, x2, y2, distTmp;

	if (nDir == 1)
		errlog("ERROR! Only using 1 direction for delay paths, for testing. CHANGE!!!!\n");
	if (startDir == 1)
		errlog("ERROR! Only using 1 direction for delay paths, the opposite!!!, for testing. CHANGE!!!!\n");
	// model.delay.nYears = 4;
	model.delay.year = (int*)malloc2(model.delay.nYears * sizeof(int));
	for (int i = 0; i < model.delay.nYears; i++)
		model.delay.year[i] = 2018 + i;

	model.delay.nGroups = 0;
	nAlloc = 100;
	model.delay.group = (strPointxy**)malloc2(nAlloc * sizeof(strPointxy*));
	model.delay.nGroupArcs = (int*)malloc2(nAlloc * sizeof(int));

	FILE* filpek = fopen("data/neighboursNew.txt", "r");
	char* namn = (char*)malloc(256 * sizeof(char));
	antal = fscanf(filpek, "%s\n", namn);

	int oldGroup = -1, pos = -1;
	for (i = 0; i < 10000; i++) {
		antal = fscanf(filpek, "%d\t%lf\t%d\t%lf\t%lf\t%lf\t%lf\n", &groupNr, &distTmp, &arcNr, &x1, &y1, &x2, &y2);
		if (antal < 7 || distTmp < 1000)
			break;

		if (groupNr != oldGroup) {
			if (model.delay.nGroups >= nAlloc) {
				nAlloc += 100;
				model.delay.group = (strPointxy**)realloc(model.delay.group, nAlloc * sizeof(strPointxy*));
				model.delay.nGroupArcs = (int*)realloc(model.delay.nGroupArcs, nAlloc * sizeof(int));
			}
			pos++;
			nAllocGrupp = 10;
			model.delay.group[pos] = (strPointxy*)malloc2(nAllocGrupp * sizeof(strPointxy));
			model.delay.nGroupArcs[pos] = 0;
			oldGroup = groupNr;
		}
		if (groupNr == 43)
			pos = pos;
		for (int i1 = startDir; i1 < nDir; i1++) {
			nNu = model.delay.nGroupArcs[pos];
			if (nNu >= nAllocGrupp) {
				nAllocGrupp += 10;
				model.delay.group[pos] = (strPointxy*)realloc(model.delay.group[pos], nAllocGrupp * sizeof(strPointxy));
			}
			if (i1 == 0) {
				model.delay.group[pos][nNu].point_x1 = x1;
				model.delay.group[pos][nNu].point_y1 = y1;
				model.delay.group[pos][nNu].point_x2 = x2;
				model.delay.group[pos][nNu].point_y2 = y2;
				direction = getDirection(model.delay.group[pos][nNu].point_y1,
					model.delay.group[pos][nNu].point_x1,
					model.delay.group[pos][nNu].point_y2,
					model.delay.group[pos][nNu].point_x2);
				model.delay.group[pos][nNu].arcNr = arcNr;
			}
			else {
				model.delay.group[pos][nNu].point_x1 = x2;
				model.delay.group[pos][nNu].point_y1 = y2;
				model.delay.group[pos][nNu].point_x2 = x1;
				model.delay.group[pos][nNu].point_y2 = y1;
				direction = getDirection(model.delay.group[pos][nNu].point_y1,
					model.delay.group[pos][nNu].point_x1,
					model.delay.group[pos][nNu].point_y2,
					model.delay.group[pos][nNu].point_x2);
				model.delay.group[pos][nNu].arcNr = -arcNr;
			}
			model.delay.group[pos][nNu].direction = direction;
			(model.delay.nGroupArcs[pos])++;
		}
	}
	fclose(filpek);
	model.delay.nGroups = pos + 1;

	nAlloc = 2;

	model.preferredPath.point = (spherical::Point*)malloc2(nAlloc * sizeof(spherical::Point));
	model.preferredPath.point_y = (double*)malloc2(nAlloc * sizeof(double));
	model.preferredPath.point_x = (double*)malloc2(nAlloc * sizeof(double));

	return 0;
}

int setUpBoundingBox_delayNew(int node) {

	model.delay.year = (int*)malloc2(model.delay.nYears * sizeof(int));
	for (int i = 0; i < model.delay.nYears; i++)
		model.delay.year[i] = 2018 + i;

	int nAlloc = 2;

	model.preferredPath.point = (spherical::Point*)malloc2(nAlloc * sizeof(spherical::Point));
	model.preferredPath.point_y = (double*)malloc2(nAlloc * sizeof(double));
	model.preferredPath.point_x = (double*)malloc2(nAlloc * sizeof(double));

	if (node == 10000) {
		model.boundingBox.xMin = -30;
		model.boundingBox.yMin = 34;
		model.boundingBox.xMax = -26;
		model.boundingBox.yMax = 36;
	}
	else {
		int yInt = (int)(node / 2);
		int xInt = node - yInt * 2;
		model.boundingBox.xMin = -180 + xInt * 180;
		model.boundingBox.yMin = -80 + yInt * 80;
		model.boundingBox.xMax = xInt * 180;
		model.boundingBox.yMax = yInt * 80;
	}
	model.delay.nXinterval = model.boundingBox.xMax - model.boundingBox.xMin;
	model.delay.nYinterval = model.boundingBox.yMax - model.boundingBox.yMin;

	model.preferredPath.minX = model.boundingBox.xMin;
	model.preferredPath.maxX = model.boundingBox.xMax;
	errlog("boundingBox in setUpBoundingBox node %d after groupArcs min/max %.3lf %.3lf\n", node,
		model.boundingBox.xMin, model.boundingBox.xMax);



	return 0;
}

int setUpBoundingBox_delay(int node){

	double maxDist, dist, y1, y2, x1, x2;
	int maxPos, direction, arcNr;
	spherical::Point p1, p2;

	maxDist = 0;
	maxPos = 0;
	for (int i1 = 0; i1 < model.delay.nGroupArcs[node]; i1++) {
		p1 = spherical::Point(model.delay.group[node][i1].point_y1, fix_lonPos(model.delay.group[node][i1].point_x1));
		p2 = spherical::Point(model.delay.group[node][i1].point_y2, fix_lonPos(model.delay.group[node][i1].point_x2));
		dist = p1.distanceTo(p2) / 1000.0;
		if (maxDist < dist) {
			maxDist = dist;
			maxPos = i1;
		}
	}
	if (maxPos > 0) {
		y1 = model.delay.group[node][0].point_y1;
		model.delay.group[node][0].point_y1 = model.delay.group[node][maxPos].point_y1;
		model.delay.group[node][maxPos].point_y1 = y1;
		y2 = model.delay.group[node][0].point_y2;
		model.delay.group[node][0].point_y2 = model.delay.group[node][maxPos].point_y2;
		model.delay.group[node][maxPos].point_y2 = y2;
		x1 = model.delay.group[node][0].point_x1;
		model.delay.group[node][0].point_x1 = model.delay.group[node][maxPos].point_x1;
		model.delay.group[node][maxPos].point_x1 = x1;
		x2 = model.delay.group[node][0].point_x2;
		model.delay.group[node][0].point_x2 = model.delay.group[node][maxPos].point_x2;
		model.delay.group[node][maxPos].point_x2 = x2;
		direction = model.delay.group[node][0].direction;
		model.delay.group[node][0].direction = model.delay.group[node][maxPos].direction;
		model.delay.group[node][maxPos].direction = direction;
		arcNr = model.delay.group[node][0].arcNr;
		model.delay.group[node][0].arcNr = model.delay.group[node][maxPos].arcNr;
		model.delay.group[node][maxPos].arcNr = arcNr;
	}

	model.boundingBox.xMin = 360;
	model.boundingBox.yMin = 360;
	model.boundingBox.xMax = -360;
	model.boundingBox.yMax = -360;

	double x, y;
	for(int i = 0; i < model.delay.nGroupArcs[node]; i++) {
		for (int i1 = 0; i1 < 2; i1++) {
			if (i1 == 0) {
				x = model.delay.group[node][i].point_x1;
				y = model.delay.group[node][i].point_y1;
			}
			else {
				x = model.delay.group[node][i].point_x2;
				y = model.delay.group[node][i].point_y2;
			}
			if (model.boundingBox.xMin > x)
				model.boundingBox.xMin = x;
			if (model.boundingBox.yMin > y)
				model.boundingBox.yMin = y;
			if (model.boundingBox.xMax < x)
				model.boundingBox.xMax = x;
			if (model.boundingBox.yMax < y)
				model.boundingBox.yMax = y;
		}
	}
	model.preferredPath.minX = model.boundingBox.xMin;
	model.preferredPath.maxX = model.boundingBox.xMax;
	errlog("boundingBox in setUpBoundingBox node %d after groupArcs min/max %.3lf %.3lf\n", node,
		model.boundingBox.xMin, model.boundingBox.xMax);


	double absYmax = abs(model.boundingBox.yMax);
	double absYmin = abs(model.boundingBox.yMin);
	double maxY = absYmax;
	if (absYmin > maxY)
		maxY = absYmin;
	double diffAddx = 0;

	if (maxY > 80)
		diffAddx += 27;
	else if (maxY > 70)
		diffAddx += 10 + (maxY - 70) * 1.7;
	else if (maxY > 60)
		diffAddx += 5 + (maxY - 60) * 0.5;
	else if (maxY > 50)
		diffAddx += 2 + (maxY - 60) * 0.3;
	else if (maxY > 40)
		diffAddx += 1 + (maxY - 60) * 0.1;

	model.boundingBox.yMax += 6 + absYmax / 15.0;
	model.boundingBox.xMax += 6 + diffAddx;
	model.boundingBox.yMin -= 6 + absYmin / 15.0;
	model.boundingBox.xMin -= 6 + diffAddx;
	errlog("boundingBox in setUpBoundingBox node %d after diffAddx min/max %.3lf %.3lf\n", node,
		model.boundingBox.xMin, model.boundingBox.xMax);

	return 0;
}

double fix_lonPos(double lon) {
	if (lon > 180)
		lon -= 360;
	if (lon < -180)
		lon += 360;
	return lon;
}

int createPrefPath_delay(int node, int pos) {

	model.preferredPath.point_y[0] = model.delay.group[node][pos].point_y1;
	model.preferredPath.point_x[0] = fix_lonPos(model.delay.group[node][pos].point_x1);
	model.preferredPath.point[0] = spherical::Point(model.preferredPath.point_y[0], model.preferredPath.point_x[0]);
	model.preferredPath.startX = model.delay.group[node][pos].point_x1;
	model.preferredPath.minX = model.preferredPath.startX;
	model.preferredPath.maxX = model.preferredPath.startX;

	double x = fix_lonPos(model.delay.group[node][pos].point_x2), xNu;
	double last_x = model.preferredPath.point_x[0];
	model.preferredPath.nPoints = 2;
	model.preferredPath.point_y[model.preferredPath.nPoints-1] = model.delay.group[node][pos].point_y2;
	model.preferredPath.point_x[model.preferredPath.nPoints - 1] = x;
	model.preferredPath.point[model.preferredPath.nPoints-1] = spherical::Point(model.delay.group[node][pos].point_y2, x);
	double dist = model.preferredPath.point[0].distanceTo(model.preferredPath.point[model.preferredPath.nPoints - 1]);
	double bearing = model.preferredPath.point[0].bearingTo(model.preferredPath.point[model.preferredPath.nPoints - 1]);
	spherical::Point p2;
	for (int i = 1; i < model.preferredPath.nPoints; i++) {
		//p2 = model.preferredPath.point[0].destinationPoint(dist * i / (double)model.preferredPath.nPoints, bearing);
		xNu = model.preferredPath.point_x[i]; // p2.longitude().degrees();
		if (abs(last_x - xNu) > 180) {
			if (last_x > xNu)
				last_x = xNu + 360;
			else
				last_x = xNu - 360;
		}
		else
			last_x = xNu;
		if (model.preferredPath.minX > last_x)
			model.preferredPath.minX = last_x;
		if (model.preferredPath.maxX < last_x)
			model.preferredPath.maxX = last_x;

		//model.preferredPath.point_y[i] = p2.latitude().degrees();
		//model.preferredPath.point_x[i] = xNu;
		//model.preferredPath.point[i] = spherical::Point(model.preferredPath.point_y[i], xNu);
	}


	return 0;
}

int setupNodesArcsNoTime_delay() {
	int i, i2, i1, nodNu, nextLev, pos2, nodNext, posNy;
	int** usedLevPos, arcNr;
	double x1, y1, x2, y2, dist, maxSpeed, timeNu;
	usedLevPos = (int**)malloc2(model.network.nPhysicalLevels * sizeof(int*));
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		usedLevPos[i] = (int*)calloc2(model.network.physicalLev[i].nPoints, sizeof(int));
	}

	double maxSpeedCalmWater = model.functions.speedLevel[0].rpmSetting_gerCalmWaterSpeed[model.functions.speedLevel[0].nShip_speedSettings - 1];
	double speedDiffWind = lookup_speedDiffWindTable(maxSpeedCalmWater, maxSpeedCalmWater, 0.0);
	maxSpeed = maxSpeedCalmWater - speedDiffWind;

	usedLevPos[0][0] = 1;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].nodNr_from_pt = (int**)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		model.network.physicalLev[i].nTimeIntervals = (int*)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].nAllocTimeIntervals = (int*)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int));
		model.network.physicalLev[i].timeInterval = (int**)malloc2(
			model.network.physicalLev[i].nPoints * sizeof(int*));
		for (i2 = 0; i2 < model.network.physicalLev[i].nPoints; i2++) {
			if (model.network.physicalLev[i].allowedPoint[i2] == 0) {
				if (i == 0 || i == model.network.nPhysicalLevels - 1) {
					errlog("ERROR! start/end point of path not feasible, move it physLev %d lon/lat %.3lf %.3lf\n",
						i, model.network.physicalLev[i].point_x[0],
						model.network.physicalLev[i].point_y[0]);
					FILE* filTmp = fopen("data/resDelay_opt.txt", "a+");
					fprintf(filTmp, "ERROR! start/end point of path not feasible, move it physLev %d lon/lat %.3lf %.3lf\n",
						i, model.network.physicalLev[i].point_x[0],
						model.network.physicalLev[i].point_y[0]);
					fclose(filTmp);
					return -1;
					continue; // not allowed node
				}
			}
			if (i == 0) {
				model.network.physicalLev[i].timeInterval[i2] = (int*)malloc2(sizeof(int));
				model.network.physicalLev[i].timeInterval[i2][0] = 0; // model.params.startDelay_h;
				model.network.physicalLev[i].nodNr_from_pt[i2] = (int*)malloc2(sizeof(int));
				model.network.physicalLev[i].nTimeIntervals[i2] = 1;
				model.nAllocNoder = 50000;
				model.Noder = (strNoder*)malloc2(model.nAllocNoder * sizeof(strNoder));
				for (int i0 = 0; i0 < model.nAllocNoder; i0++)
					model.Noder[i0].UtNod = NULL;
				model.nArcs = 0;
				model.nNoder = 0;
				model.network.physicalLev[i].nodNr_from_pt[i2][0] = model.nNoder;
				adderaNod(i, 0, 0);
			}
			else {
				model.network.physicalLev[i].nAllocTimeIntervals[i2] = 100;
				model.network.physicalLev[i].timeInterval[i2] = (int*)malloc2(
					model.network.physicalLev[i].nAllocTimeIntervals[i2] * sizeof(int));
				model.network.physicalLev[i].nodNr_from_pt[i2] = (int*)malloc2(
					model.network.physicalLev[i].nAllocTimeIntervals[i2] * sizeof(int));
				model.network.physicalLev[i].nTimeIntervals[i2] = 0;
			}
		}
	}

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i == 19)
			i = i;
		for (i2 = 0; i2 < model.network.physicalLev[i].nPoints; i2++) {
			if (model.network.physicalLev[i].allowedPoint[i2] == 0)
				continue; // not allowed node
			nodNu = usedLevPos[i][i2] - 1;
			for (i1 = 0; i1 < model.network.physicalLev[i].nOutNodes[i2]; i1++) {
				nextLev = model.network.physicalLev[i].outLevel[i2][i1];
				pos2 = model.network.physicalLev[i].outNode[i2][i1];
				if (usedLevPos[nextLev][pos2] == 0) {
					nodNext = adderaNod(nextLev, pos2, 0);
					usedLevPos[nextLev][pos2] = nodNext + 1;
				}
				else
					nodNext = usedLevPos[nextLev][pos2] - 1;

				x1 = model.network.physicalLev[i].point_x[i2];
				y1 = model.network.physicalLev[i].point_y[i2];
				x2 = model.network.physicalLev[nextLev].point_x[pos2];
				y2 = model.network.physicalLev[nextLev].point_y[pos2];
				//dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				dist = model.network.physicalLev[i].point[i2].distanceTo(model.network.physicalLev[nextLev].point[pos2]) / 1000.0;
				timeNu = dist / maxSpeed;
				posNy = adderaArc(nodNu, nodNext, timeNu, 1);
				arcNr = model.nArcs;
				if (arcNr + 1 >= model.nAllocArcs) {
					model.nAllocArcs += 100000;
					model.arc = (strArcInfo*)realloc(model.arc,
						model.nAllocArcs * sizeof(strArcInfo));
				}
				model.arc[arcNr].fromLevel = i;
				model.arc[arcNr].toLevel = nextLev;
				model.arc[arcNr].fromPointNr = i2;
				model.arc[arcNr].outNodePos = i1;
				model.arc[arcNr].toPointNr = pos2;
				model.arc[arcNr].fromTime = 0;
				model.arc[arcNr].toTime = 0;
				model.arc[arcNr].speedSetting = 1;
				model.arc[arcNr].time = timeNu;
				model.arc[arcNr].distance = dist;
				model.arc[arcNr].emission = 0;
				model.arc[arcNr].fuelBase = 0;
				model.arc[arcNr].fuel_aux = 0;
				model.arc[arcNr].fuel_auxEca = 0;
				model.arc[arcNr].fuel_eca = 0;
				model.arc[arcNr].fuel_noEca = 0;
				model.arc[arcNr].safetyHurricane = 0;
				model.arc[arcNr].dynamicStability = 0;
				model.arc[arcNr].bowSlam = 0;
				model.arc[arcNr].greenWater = 0;
				model.arc[arcNr].rolling= 0;
				model.arc[arcNr].surfRiding = 0;
				//model.arc[arcNr].safetyBowSlam = 0;
				//model.arc[arcNr].safetyGreenWater = 0;
				//model.arc[arcNr].safetyDynStability = 0;
				//model.arc[arcNr].feasibleSafety = 0;
				//model.arc[arcNr].iceCoverCost = 0;
				//model.arc[arcNr].safetyStability = 0;
				model.arc[arcNr].safetyBase = 0;
				//model.arc[arcNr].channelCost = 0;
				model.arc[arcNr].totCost = timeNu;
				//model.arc[arcNr].nodNr1 = nodNu;
				//model.arc[arcNr].nodNr2 = nodNext;
				model.nArcs++;
			}
		}
	}
	i = i - 1;
	nodNu = usedLevPos[i][model.network.physicalLev[i].nPoints - 1] - 1;
	adderaNod(i + 1, 0, 0);
	nodNext = model.nNoder - 1;
	addEndBage(i, 0, i + 1, 0, nodNext, 0);



	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		free(usedLevPos[i]);
	}
	free(usedLevPos);

	return 0;
}

int loadStorms_delayed(int year) {
	char* namn = (char*)malloc2(256 * sizeof(char));
	json data, dataFeature;
	std::ifstream fil;
	int manadNu;

	model.nameTmp = (char*)malloc(256 * sizeof(char));

	if (model.delay.stormsYear[year] == NULL) {
		manadNu = model.delay.delayed_monthNr[0];
		if(manadNu < 10)
			sprintf(namn, "%s/%s_0%d_%d.json", model.params.indataPath.c_str(), model.delay.delayed_stormFileName, manadNu, model.delay.year[year]);
		else
			sprintf(namn, "%s/%s_%d_%d.json", model.params.indataPath.c_str(), model.delay.delayed_stormFileName, manadNu, model.delay.year[year]);

		fil.open(namn);
		model.nStorms = 0;
		if (fil.is_open() == TRUE) {
			fil >> data;

			model.nAllocStorms = data.size();
			model.storms = (strStorm*)malloc2(model.nAllocStorms * sizeof(strStorm));

			for (auto it = data.begin(); it != data.end(); ++it) {
				dataFeature = it.value();
				loadStormObject(dataFeature);
			}

			calc_stormsNearby_delay();
			model.delay.stormsYear[year] = model.storms;
		}
		model.delay.nStormsYear[year] = model.nStorms;
		free(namn);

	}
	else {
		model.storms = model.delay.stormsYear[year];
		model.nStorms = model.delay.nStormsYear[year];
	}

	//simuleraStormsVisuellt();

	return 0;
}

int loadWeatherFiles_delayed_new(int year) {
	int xPos0, xPos1, yPos0, yPos1, nBands, ii;
	int nAlloc, i, returnVal, manad2, manadNu, nBandsNu, nBandsAlloc;
	char* namn = (char*)malloc2(256 * sizeof(char));
	int nMaxTimeInt = 0, tidInt;
	long long maxTid, sekNu, nSecondsUTC;
	int yearNu;

	Raster test;

	loadStorms_delayed(year);

	double size_col, size_row, xPosFrac, yPosFrac, filKvot;
	for (ii = 0; ii < model.nWeatherFiles; ii++) {

		filKvot = 1.0; // to change from m/s to km/h
		if (strcmp(model.weather[ii].weatherFileTypeName, "wind_uComponent") == 0)
			filKvot = 3.6;
		if (strcmp(model.weather[ii].weatherFileTypeName, "wind_vComponent") == 0)
			filKvot = 3.6;
		if (strcmp(model.weather[ii].weatherFileTypeName, "current_uComponent") == 0)
			filKvot = 3.6;
		if (strcmp(model.weather[ii].weatherFileTypeName, "current_vComponent") == 0)
			filKvot = 3.6;

		size_col = -1;
		manad2 = 0;
		for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			yearNu = model.delay.year[year];
			if (i1 < model.weather[ii].nFiles / 2.0)
				manadNu = model.delay.delayed_monthNr[0];
			else {
				manadNu = model.delay.delayed_monthNr[1];
				if (manadNu == 1)
					yearNu = model.delay.year[year] + 1;
			}
			if (manadNu < 10)
				sprintf(namn, "%s_0%d_%d.grb", model.weather[ii].filePos[i1].fileName, manadNu,
					yearNu);
			else
				sprintf(namn, "%s_%d_%d.grb", model.weather[ii].filePos[i1].fileName, manadNu,
					yearNu);
			printf("%s\n", namn);

			returnVal = model.weather[ii].rasterPos[i1].open(namn);
			if (returnVal == -1)
				return -1;
			if (i1 == 0 && i1 < model.weather[ii].nFiles / 2.0) { // if(size_col < 0){
				size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
				model.weather[ii].size_col = size_col;
				xPosFrac = (model.boundingBox.xMin - model.weather[ii].rasterPos[i1].Get_minLongitude()) / size_col;
				xPos0 = roundDown(xPosFrac);
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
				yPos0 = roundDown(yPosFrac);
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
				nBandsAlloc = (int)(1.33 * nBands);
				model.weather[ii].nTimeIntervals = nBandsAlloc;
				model.weather[ii].nTimeIntervals_forecast = nBandsAlloc;
				model.weather[ii].secondsUTC = (long long*)malloc2(nBandsAlloc * sizeof(long long));
				model.weather[ii].valueCell = (float**)malloc2(nBandsAlloc * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (int i2 = 0; i2 < nBandsAlloc; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
					for (int i3 = 0; i3 < nAlloc; i3++)
						model.weather[ii].valueCell[i2][i3] = 9999;

					if (i2 < nBands){
						//if (ii != 6) {
						nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
						model.weather[ii].secondsUTC[i2] = nSecondsUTC;
					}
					//else {
					//	model.weather[ii].secondsUTC[i2] = model.weather[ii - 2].secondsUTC[i2];
					//}
				}
				if (ii == 0)
					model.params.UTC_secondsStart = model.weather[ii].secondsUTC[0];

			}
			else {
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
					errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
					errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());

				if (manad2 == 0 && i1 >= model.weather[ii].nFiles / 2.0) {
					manad2 = 1;
					nBandsNu = model.weather[ii].rasterPos[i1].Get_nBands();
					if (nBands + nBandsNu < nBandsAlloc) {
						errlog("ERROR! Too few bands in the second months, should be %d + %d = %d but is only %d + %d = %d file %s\n",
							nBands, nBandsAlloc - nBands, nBandsAlloc, nBands + nBandsNu, nBands + nBandsNu, namn);
						model.weather[ii].nTimeIntervals = nBands + nBandsNu;
						model.weather[ii].nTimeIntervals_forecast = nBands + nBandsNu;
						nBandsAlloc = nBands + nBandsNu;
					}

					for (int i2 = nBands; i2 < nBandsAlloc; i2++) {
						nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1 - nBands);
						model.weather[ii].secondsUTC[i2] = nSecondsUTC;
					}
					nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
					model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));
					if (nAlloc < 0) {
						errlog("ERROR! nAlloc = %d, is it because if icetk fix, otherwise it's wrong. I set it to 2\n", nAlloc);
						printf("ERROR! nAlloc = %d, is it because if icetk fix, otherwise it's wrong. I set it to 2\n", nAlloc);
						nAlloc = 2;
					}
					errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
						ii, model.weather[ii].nTimeIntervals,
						model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);
					tidInt = 0;
					for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

						if (i == model.weather[ii].nTimeIntervals - 1)
							maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
						else {
							// maxTid = model.weather[ii].secondsUTC[i + 1] - 1;
							maxTid = (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]) / 2;
						}
					
						for (; tidInt < 100000; tidInt++) {
							if (tidInt >= nAlloc) {
								printf("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", tidInt, nAlloc, ii,
									model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
								errlog("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", tidInt, nAlloc, ii,
									model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
								break;
							}
							sekNu = (long long)(tidInt * model.weather_timeIntervall_h * 3600 + model.params.UTC_secondsStart);
							if (sekNu <= maxTid)
								model.weather[ii].timeIntervalIndex[tidInt] = i;
							else
								break;
						}
						//printf("weather %d i %d tidInt %d (over maxTid) sekNu %I64d maxSec %I64d\n", ii, i, tidInt, sekNu, maxTid);
						model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;
						if (model.weather[ii].nTimeIntervals_maxValue > nMaxTimeInt)
							nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;
					}
				}
			}
			if (ii == 2 && i1 == 1)
				ii = ii;
			if (i1 < model.weather[ii].nFiles / 2.0)
				model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands_fixBandNr(&(model.weather[ii]), 0, nBandsAlloc, filKvot);
			else
				model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands_fixBandNr(&(model.weather[ii]), nBands, nBandsAlloc, filKvot);

			//printf("valueCell 1111 val %.3lf\n", model.weather[ii].valueCell[0][1111]);
			//extractGribInfo("data/gribFilesToCheck.json");


		}

	}

	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		//printf("var %d maxVal %d\n", ii, model.weather[ii].nTimeIntervals_maxValue);
		if (model.weather[ii].nTimeIntervals_maxValue < nMaxTimeInt) {
			errlog("extends weather %s to nMaxTimeInt %d from %d endValue %d\n",
				model.weather[ii].weatherFileTypeName, nMaxTimeInt, model.weather[ii].nTimeIntervals_maxValue,
				model.weather[ii].timeIntervalIndex[model.weather[ii].nTimeIntervals_maxValue]);
			model.weather[ii].timeIntervalIndex = (int*)realloc(model.weather[ii].timeIntervalIndex, (nMaxTimeInt + 1) * sizeof(int));
			for (i = model.weather[ii].nTimeIntervals_maxValue; i <= nMaxTimeInt; i++) {
				model.weather[ii].timeIntervalIndex[i] = model.weather[ii].timeIntervalIndex[model.weather[ii].nTimeIntervals_maxValue];
			}
		}
	}
	model.weather_nTimeIntervals_maxValue = nMaxTimeInt;


	free(namn);
	return 0;
}

int loadWeatherFiles_delayed_notUsed(int year) {
	int xPos0, xPos1, yPos0, yPos1, nBands, ii;
	int nAlloc, i, returnVal;
	char* namn = (char*)malloc2(256 * sizeof(char));
	int nMaxTimeInt = 0, tidInt;
	long long maxTid, sekNu, nSecondsUTC;

	loadStorms_delayed(year);

	double size_col, size_row, xPosFrac, yPosFrac;
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		model.weather[ii].timePosToBandPos = NULL;
		size_col = -1;
		for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			sprintf(namn, "%s%d.grb", model.weather[ii].filePos[i1].fileName, model.delay.year[year]);
			sprintf(namn, "%smwp0_02_%d.grb", model.params.weatherPath.c_str(), model.delay.year[year]);
			printf("%s\n", namn);

			returnVal = model.weather[ii].rasterPos[i1].open(namn);
			if (returnVal == -1)
				return -1;
			if (size_col < 0) {
				size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
				model.weather[ii].size_col = size_col;
				xPosFrac = (model.boundingBox.xMin - model.weather[ii].rasterPos[i1].Get_minLongitude()) / size_col;
				xPos0 = roundDown(xPosFrac);
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
				yPos0 = roundDown(yPosFrac);
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
				model.weather[ii].nTimeIntervals_forecast = nBands;
				model.weather[ii].secondsUTC = (long long*)malloc2(nBands * sizeof(long long));
				model.weather[ii].valueCell = (float**)malloc2(nBands * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (int i2 = 0; i2 < nBands; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
					for (int i3 = 0; i3 < nAlloc; i3++)
						model.weather[ii].valueCell[i2][i3] = 9999;

					if (ii != 6) {
						nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
						model.weather[ii].secondsUTC[i2] = nSecondsUTC;
					}
					else {
						model.weather[ii].secondsUTC[i2] = model.weather[ii - 2].secondsUTC[i2];
					}
				}
				if (ii == 0)
					model.params.UTC_secondsStart = model.weather[ii].secondsUTC[0];


				nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
				model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

				errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
					ii, model.weather[ii].nTimeIntervals,
					model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);
				tidInt = 0;
				for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

					if (i == model.weather[ii].nTimeIntervals - 1)
						maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
					else {
						// maxTid = model.weather[ii].secondsUTC[i + 1] - 1;
						maxTid = (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]) / 2;
					}
				
					for (; tidInt < 100000; tidInt++) {
						if (tidInt >= nAlloc) {
							printf("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", tidInt, nAlloc, ii,
								model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
							errlog("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", tidInt, nAlloc, ii,
								model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
							break;
						}
						sekNu = (long long)(tidInt * model.weather_timeIntervall_h * 3600 + model.params.UTC_secondsStart);
						if (sekNu <= maxTid)
							model.weather[ii].timeIntervalIndex[tidInt] = i;
						else
							break;
					}
					//printf("weather %d i %d tidInt %d (over maxTid) sekNu %I64d maxSec %I64d\n", ii, i, tidInt, sekNu, maxTid);
				}
				model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;
				if (model.weather[ii].nTimeIntervals_maxValue > nMaxTimeInt)
					nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;


			}
			else {
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
					errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
					errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
			}
			if (ii == 2 && i1 == 1)
				ii = ii;
			model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0, 1.0);

			//extractGribInfo("data/gribFilesToCheck.json");


		}

	}

	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		//printf("var %d maxVal %d\n", ii, model.weather[ii].nTimeIntervals_maxValue);
		if (model.weather[ii].nTimeIntervals_maxValue < nMaxTimeInt) {
			errlog("extends weather %s to nMaxTimeInt %d from %d endValue %d\n",
				model.weather[ii].weatherFileTypeName, nMaxTimeInt, model.weather[ii].nTimeIntervals_maxValue,
				model.weather[ii].timeIntervalIndex[model.weather[ii].nTimeIntervals_maxValue]);
			model.weather[ii].timeIntervalIndex = (int*)realloc(model.weather[ii].timeIntervalIndex, (nMaxTimeInt + 1) * sizeof(int));
			for (i = model.weather[ii].nTimeIntervals_maxValue; i <= nMaxTimeInt; i++) {
				model.weather[ii].timeIntervalIndex[i] = model.weather[ii].timeIntervalIndex[model.weather[ii].nTimeIntervals_maxValue];
			}
		}
	}
	model.weather_nTimeIntervals_maxValue = nMaxTimeInt;


	free(namn);
	return 0;
}

int loadWeatherFiles_checkData(int year) {
	int xPos0, xPos1, yPos0, yPos1, nBands, ii, pos2;
	int nAlloc, i, returnVal;
	char* namn = (char*)malloc2(256 * sizeof(char));
	int nMaxTimeInt = 0, tidInt, row, col;
	long long maxTid, sekNu, nSecondsUTC;

	Raster test;

	model.delay.delayed_monthNr[0] = 2;

	loadStorms_delayed(year);

	double size_col, size_row, xPosFrac, yPosFrac, rowDbl, colDbl;
	double x1 = -38.46, y1 = 16.02; // i Atlanten

	model.boundingBox.xMin = x1;
	model.boundingBox.xMax = x1;
	model.boundingBox.yMin = y1;
	model.boundingBox.yMax = y1;

	std::ifstream fil;

	resultPath = "data";
	reset_errlog();

	std::string inputPath = "data/gribFilesToCheck.json";
	printf("opens %s\n", inputPath.c_str());
	fil.open(inputPath.c_str());

	json data, dataSpeed, dataVar, dataIt;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", inputPath.c_str());
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", inputPath.c_str());
		exitKontrollerat(__LINE__);
	}

	ii = 0;
	int pos = 0;
	double filKvot;

	FILE* filpek = fopen("checkTimeDateGribfiles.txt", "w");
	std::string namn2;
	errlog("fileName\tpos\tnBands\tsizeCol\tsizeRow\tminLongitude\tmaxLatitude\tnCols\tnRows\n");
	int i1 = 0;
	for (auto it = data.begin(); it != data.end(); ++it) {
		dataIt = it.value();
		if (!dataIt["fileName"].is_null()) {
			namn2 = dataIt["fileName"];
		}
		else
			continue;

		printf("%s\n", namn2.c_str());

		filKvot = 1.0; // to change from m/s to km/h

		size_col = -1;
		model.weather[ii].timePosToBandPos = NULL;
		returnVal = model.weather[ii].rasterPos[i1].open(namn2.c_str());
		if (returnVal == -1)
			return -1;
		if (size_col < 0) {
			size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
			model.weather[ii].size_col = size_col;
			xPosFrac = (model.boundingBox.xMin - model.weather[ii].rasterPos[i1].Get_minLongitude()) / size_col;
			xPos0 = roundDown(xPosFrac);
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
			yPos0 = roundDown(yPosFrac);
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

			if (pos > 0) {
				free(model.weather[ii].secondsUTC);
				free(model.weather[ii].timeIntervalIndex);
				for (int i2 = 0; i2 < nBands; i2++) {
					free(model.weather[ii].valueCell[i2]);
				}
				free(model.weather[ii].valueCell);
			}

			nBands = model.weather[ii].rasterPos[i1].Get_nBands();
			model.weather[ii].nTimeIntervals = nBands;
			model.weather[ii].nTimeIntervals_forecast = nBands;


			model.weather[ii].secondsUTC = (long long*)malloc2(nBands * sizeof(long long));
			model.weather[ii].valueCell = (float**)malloc2(nBands * sizeof(float*));
			nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
			for (int i2 = 0; i2 < nBands; i2++) {
				model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
				for (int i3 = 0; i3 < nAlloc; i3++) 
					model.weather[ii].valueCell[i2][i3] = 9999;

				if (ii != 6) {
					nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
					model.weather[ii].secondsUTC[i2] = nSecondsUTC;
				}
				else {
					model.weather[ii].secondsUTC[i2] = model.weather[ii-2].secondsUTC[i2];
				}
			}
			if (ii == 0)
				model.params.UTC_secondsStart = model.weather[ii].secondsUTC[0];
					

			nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
			model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

			errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
				ii, model.weather[ii].nTimeIntervals,
				model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);
			tidInt = 0;
			for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

				if (i == model.weather[ii].nTimeIntervals - 1)
					maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
				else {
					// maxTid = model.weather[ii].secondsUTC[i + 1] - 1;
					maxTid = (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]) / 2;
				}
			
				for (; tidInt < 100000; tidInt++) {
					if (tidInt >= nAlloc) {
						printf("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", tidInt, nAlloc, ii,
							model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
						errlog("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d sekNr %I64d maxTid %I64d\n", tidInt, nAlloc, ii,
							model.weather[ii].weatherFileTypeName, i, sekNu, maxTid);
						break;
					}
					sekNu = (long long)(tidInt * model.weather_timeIntervall_h * 3600 + model.params.UTC_secondsStart);
					if (sekNu <= maxTid)
						model.weather[ii].timeIntervalIndex[tidInt] = i;
					else
						break;
				}
				//printf("weather %d i %d tidInt %d (over maxTid) sekNu %I64d maxSec %I64d\n", ii, i, tidInt, sekNu, maxTid);
			}
			model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;
			if (model.weather[ii].nTimeIntervals_maxValue > nMaxTimeInt)
				nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;


		}
		else {
			if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
				errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
					model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
			if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
				errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
					model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
		}
		if (ii == 2&&i1==1)
			ii = ii;

		errlog("%s\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%d\t%d\n", namn2.c_str(), pos, nBands,
			size_col, model.weather[ii].rasterPos[i1].Get_sizeRow(), model.weather[ii].rasterPos[i1].Get_minLongitude(),
			model.weather[ii].rasterPos[i1].Get_maxLatitude(),
			model.weather[ii].rasterPos[i1].Get_nCols(), model.weather[ii].rasterPos[i1].Get_nRows());

		model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0, filKvot);

		rowDbl = (model.weather[ii].maxY - y1) / model.weather[ii].size_row;
		colDbl = get_colDblFromWeatherFile(i1, x1);
		row = (int)rowDbl;
		col = (int)colDbl;
		pos2 = col + model.weather[ii].nCols * row;
		for (int i2 = 0; i2 < nBands; i2++) {
			nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
			fprintf(filpek, "%s\t%d\t%d\t%lf\t%I64d\t%s\n", namn2.c_str(), pos, i2,
				model.weather[ii].valueCell[i2][pos2],
				nSecondsUTC, stringDateFromUTCSeconds(nSecondsUTC).c_str());
		}
		pos++;
	}
	fclose(filpek);

	free(namn);
	exit(0);
	return 0;
}

long long make_gmtime_fromGivenDate_delay(int yearPos, strParams* params) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;

	int hour, nHoursDiffTZ = 0, nMinDiffTZ = 0, nValuesHour;

	tmBas.tm_year = model.delay.year[yearPos] - 1900;
	tmBas.tm_mon = model.delay.delayed_monthNr[0] - 1; // sep
	tmBas.tm_mday = 1;
	hour = 0;
	tmBas.tm_hour = hour;
	tmBas.tm_min = 0;
	tmBas.tm_sec = 0;

	time_t test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
	}

	params->startYear = tmBas.tm_year + 1900;
	params->startMonth_nr = tmBas.tm_mon + 1;
	params->startDay_nr = tmBas.tm_mday;
	params->startHour = tmBas.tm_hour;
	params->startMinute = tmBas.tm_min;

#ifdef _WIN32
	time_t rawtime = _mkgmtime(&tmBas);
#endif
#ifndef _WIN32
	time_t rawtime = timegm(&tmBas);
#endif
	//cout << "rawtime " << rawtime << "\n";
	return rawtime;
}

double get_lonFromKvotAvTva_delay(double x1, double x2, double factor) {
	double x, diff = x1 - x2;
	if (diff > 180)
		x = x1 * (1 - factor) + (x2 + 360) * factor;
	else if(diff < - 180)
		x = (x1 + 360) * (1 - factor) + x2 * factor;
	else
		x = x1 * (1 - factor) + x2 * factor;
	if (x > 180)
		x -= 360;
	if (x < -180)
		x += 360;
	return x;
}

int determine_nPointsDelayVisuellt(double dist, double* faktorer) {
	int nPkter;
	if (dist < 1500) {
		nPkter = 1;
		faktorer[0] = 0.5;
	}
	else if (dist < 2800) {
		nPkter = 2;
		faktorer[0] = 0.25;
		faktorer[1] = 0.75;
	}
	else{
		nPkter = 3;
		faktorer[0] = 1 / 6.0;
		faktorer[1] = 0.5;
		faktorer[2] = 5 / 6.0;
	}
	return nPkter;
}

int writeErrorMsgToFiles(int node, int year, int pos, int i1) {
	
	FILE* filTmp = fopen("data/resDelay_opt.txt", "a+");
	fprintf(filTmp, "delay ERROR node %d neighbourNr %d arcID %d yearPos %d i1 %d optCost -1 "
		"nBagar %d fagelDist %.2lf nPhysLev %d nDiffTimeSol -1\n",
		node, pos, model.delay.group[pos][i1].arcNr, year, i1, model.nArcs,
		model.preferredPath.point[0].distanceTo(model.preferredPath.point[1]) / 1000.0, model.network.nPhysicalLevels);
	fclose(filTmp);
	FILE* filDelay = fopen("data/res_delay.txt", "a+");
	double distance = model.preferredPath.point[0].distanceTo(model.preferredPath.point[1]) / 1000.0;
	fprintf(filDelay, "%d\t%d\t%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%d\t%d\t%.4lf\t%.4lf\t%.4lf",
		node, pos, model.delay.year[year], i1 + 1,
		model.delay.group[node][pos].direction,
		-1, -1, distance, -1, -1,
		2.0, 1.5, 1.5);

	for (int i = 0; i < 2; i++) {
		fprintf(filDelay, "\t%.3lf\t%.3lf", model.preferredPath.point_x[i], model.preferredPath.point_y[i]);
	}
	double* faktorer = (double*)malloc(5 * sizeof(double));
	int nPkter = determine_nPointsDelayVisuellt(distance, faktorer);
	int i;
	fprintf(filDelay, "\t%d", nPkter);
	for (i = 0; i < nPkter; i++) {
		double x = get_lonFromKvotAvTva_delay(model.preferredPath.point_x[0], model.preferredPath.point_x[1], faktorer[i]);
		fprintf(filDelay, "\t%.3lf\t%.3lf", x,
			model.preferredPath.point_y[0] * (1 - faktorer[i]) + model.preferredPath.point_y[1] * faktorer[i]);
	}
	fprintf(filDelay, "\t%d\n", node, model.delay.group[node][pos].arcNr);
	fclose(filDelay);
	free(faktorer);

	return 0;
}

int writeSolutionToJson_delay(int node, int alt, int yearPos, int startPos)
{
	int nAllocPkter, i, iPos, nPkter, nArcs, ii3, forsta;
	int arcNr, lev1, lev2, pointNr1, pointNr2, timeInt, * nSpeedSettingUsed, nSpeedChanges = 0;
	double* x, * y, xNu, yNu;
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	FILE* filpekG;
	time_t rawtime;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	if (skrivMycket == 1) {
		sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
		FILE* filpek10 = fopen(namn, "a+");
		fprintf(filpek10, "\nneighbourPos %d year %d startPos %d\n", alt, model.delay.year[yearPos], startPos);
		fclose(filpek10);
	}
	int nAlloc = model.network.nMaxNodesInPath * model.nBVArcs, prefPath;

	x = (double*)malloc2(nAlloc * sizeof(double));
	y = (double*)malloc2(nAlloc * sizeof(double));
	nSpeedSettingUsed = (int*)calloc2(model.functions.nShip_speedSettingsBase, sizeof(int));

	FILE* filPek = NULL, * filPek2 = NULL, * filPek3 = NULL;
	//std::string solName;
	//sprintf(namn, "%s.csv", filename);
	if (skrivMycket == 1) {
		sprintf(namn, "%s/resSol_%d_%d_%d.csv", model.params.indataPath.c_str(), alt, model.delay.year[yearPos], startPos);
		filPek = fopen(namn, "w");
		sprintf(namn, "%s/solPath_%d_%d_%d.csv", model.params.indataPath.c_str(), alt, model.delay.year[yearPos], startPos);
		filPek2 = fopen(namn, "w");
		fprintf(filPek2, "level;nodPos;speedSetting;arcNr(for_information_only);nod1(info);nod2(info)\n");
		fprintf(filPek, "arcPos\tspeedSetting\tdistance\ttime\tfuelBase\temission\tsafetyBase\tchannelCost\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
			"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\tnodNr1\tnodNr2\n");
		sprintf(namn, "%s/resDetailsDelayRoutes.txt", model.params.indataPath.c_str());
		filPek3 = fopen(namn, "a+");
	}

	std::string linePath = "";
	if (skrivMycket == 1) {
		sprintf(namn, "%s/res_delay.json", model.params.indataPath.c_str());
		filpekG = fopen(namn, "a+");
		if (alt > 0 || startPos >= 0)
			fprintf(filpekG, ",\n");

		linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";
	}
	else
		filpekG = NULL;

	double timeNu = 0, fuel = 0, safety = 0, totCost = 0, distance = 0, channelCost = 0, emission = 0;
	double fuel_aux = 0, fuel_auxEca = 0, fuel_eca = 0, fuel_noEca = 0, hurricane = 0, distanceTp, distNu, distTmp;// , stability = 0;
	double bowSlamming = 0, greenWater = 0, dynStability = 0, iceCoverage = 0, feasibleSafety = 0, timeExact = 0;
	double fuelCostDollar, fuel_objCost, voyageTime_objCost, emission_objCost, safety_objCost;
	int ii, nTp, nAdded, ii2, posIreport, legNr;
	double x1, y1, x2, y2;
	spherical::Point pointLast, pointFinal;

	if (startPos > 0)
		timeExact = startPos * 24;

	if (model.params.simuleraTidVisuellt == 1 && startPos >= 0) {
		plotNodeTimeVisuellt(timeExact, model.preferredPath.point_x[0], model.preferredPath.point_y[0]);
	}


	nArcs = 0;
	arcNr = -1;
	nPkter = 0;
	posIreport = 0;
	model.functions.valuesNow.bearingOldWpt = 1000;
	model.functions.valuesNow.accumDistance = 0;
	model.functions.valuesNow.totDistance = 0;

	model.functions.valuesNow.maxDiffTime = -1e10;
	model.functions.valuesNow.minDiffTime = 1e10;

	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++) {
		arcNr = model.BVArc[iPos];
		model.functions.valuesNow.totDistance += model.arc[arcNr].distance;
	}

	model.network.nCoords = 0;
	model.functions.valuesNow.Wpt = 0;
	model.network.last_x = model.preferredPath.startX;
	model.functions.valuesNow.maxWaveHeight = 0;
	model.functions.valuesNow.maxWaveHeight_tp = 0;

	fuel_objCost = 0;
	voyageTime_objCost = 0;
	emission_objCost = 0;
	safety_objCost = 0;
	model.waypointResult.accumDistance_km = 0;

	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
	{
		// kopiera delen av punktfoljden som anvands, dess xyz
		arcNr = model.BVArc[iPos];
		if (startPos == -1) {
			if (iPos == 0) {
				x1 = model.network.physicalLev[model.arc[arcNr].fromLevel].point_x[model.arc[arcNr].fromPointNr];
				y1 = model.network.physicalLev[model.arc[arcNr].fromLevel].point_y[model.arc[arcNr].fromPointNr];
				model.network.xCoord[model.network.nCoords] = x1;
				model.network.yCoord[model.network.nCoords] = y1;
				model.results.leg[legNr].y[model.results.leg[legNr].nCoords] = model.network.yCoord[model.network.nCoords];
				model.results.leg[legNr].x[model.results.leg[legNr].nCoords] = model.network.xCoord[model.network.nCoords];
				(model.results.leg[legNr].nCoords)++;
				(model.network.nCoords)++;
			}
			x1 = model.network.physicalLev[model.arc[arcNr].toLevel].point_x[model.arc[arcNr].toPointNr];
			y1 = model.network.physicalLev[model.arc[arcNr].toLevel].point_y[model.arc[arcNr].toPointNr];
			model.network.xCoord[model.network.nCoords] = x1;
			model.network.yCoord[model.network.nCoords] = y1;
			model.results.leg[legNr].y[model.results.leg[legNr].nCoords] = model.network.yCoord[model.network.nCoords];
			model.results.leg[legNr].x[model.results.leg[legNr].nCoords] = model.network.xCoord[model.network.nCoords];
			(model.results.leg[legNr].nCoords)++;
			(model.network.nCoords)++;
		}

		if (model.arc[arcNr].fromLevel >= 0) {
			x1 = model.network.physicalLev[model.arc[arcNr].fromLevel].point_x[model.arc[arcNr].fromPointNr];
			y1 = model.network.physicalLev[model.arc[arcNr].fromLevel].point_y[model.arc[arcNr].fromPointNr];
			if (model.arc[arcNr].toLevel >= 0) {
				if (model.arc[arcNr].toLevel < model.network.nPhysicalLevels) {
					x2 = model.network.physicalLev[model.arc[arcNr].toLevel].point_x[model.arc[arcNr].toPointNr];
					y2 = model.network.physicalLev[model.arc[arcNr].toLevel].point_y[model.arc[arcNr].toPointNr];
				}
				else {
					x2 = x1;
					y2 = y1;
				}
			}
			else {
				x2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_x[0];
				y2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_y[0];
			}
		}
		else {
			if (model.arc[arcNr].toLevel < 0) {
				if (model.arc[arcNr].fromLevel == model.arc[arcNr].toLevel) {
					x1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_x[0];
					y1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_y[0];
					x2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_x[model.network.channel[-model.arc[arcNr].toLevel - 1].nPoints - 1];
					y2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_y[model.network.channel[-model.arc[arcNr].toLevel - 1].nPoints - 1];
				}
				else {
					x1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_x[model.network.channel[-model.arc[arcNr].fromLevel - 1].nPoints - 1];
					y1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_y[model.network.channel[-model.arc[arcNr].fromLevel - 1].nPoints - 1];
					x2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_x[0];
					y2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_y[0];
				}
			}
			else {
				x1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_x[model.network.channel[-model.arc[arcNr].fromLevel - 1].nPoints - 1];
				y1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_y[model.network.channel[-model.arc[arcNr].fromLevel - 1].nPoints - 1];
				if (model.arc[arcNr].toLevel < model.network.nPhysicalLevels) {
					x2 = model.network.physicalLev[model.arc[arcNr].toLevel].point_x[model.arc[arcNr].toPointNr];
					y2 = model.network.physicalLev[model.arc[arcNr].toLevel].point_y[model.arc[arcNr].toPointNr];
				}
				else {
					x2 = x1;
					y2 = y1;
				}
			}
		}

		if (arcNr == 559126)
			arcNr = arcNr;
		lev1 = model.arc[arcNr].fromLevel;
		lev2 = model.arc[arcNr].toLevel;
		pointNr1 = model.arc[arcNr].fromPointNr;
		pointNr2 = model.arc[arcNr].toPointNr;
		if (lev2 < model.network.nPhysicalLevels) {
			if (iPos > 0) {
				if (model.arc[arcNr].speedSetting != model.arc[model.BVArc[iPos - 1]].speedSetting)
					nSpeedChanges++;
			}
			(nSpeedSettingUsed[model.arc[arcNr].speedSetting])++;
		}
		nTp = model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
		if (nTp == 0)
			nTp = 1;
		distanceTp = 1000.0 * model.arc[arcNr].distance / nTp;
		nAdded = 0;

		sprintf(namn, "res_%d_%d_%d", alt, model.delay.year[yearPos], startPos);
		if (startPos >= 0) {
			addPositionDataToReport(filpekG, &posIreport, arcNr, 0, &timeExact, namn);

			if (skrivMycket == 1) {
				fprintf(filPek, "%d\t%d\t%.2lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%d\t%d\t%d\t%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%.3lf\n", iPos, model.arc[arcNr].speedSetting, model.arc[arcNr].distance,
					model.arc[arcNr].time, model.arc[arcNr].fuelBase, model.arc[arcNr].emission, model.arc[arcNr].safetyBase,
					model.functions.valuesNow.channelCost, model.arc[arcNr].totCost, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
					model.arc[arcNr].fromTime, model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
					x1, y1, x2, y2);

				if (iPos < model.nBVArcs - 2)
					fprintf(filPek3, "%d\t%d\t%d\t%d\t%d\t%d\t%.2lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%d\t%d\t%d\t%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t"
						"%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\n",
						node, alt, yearPos, startPos, iPos, model.arc[arcNr].speedSetting, model.arc[arcNr].distance,
						model.arc[arcNr].time, model.arc[arcNr].fuelBase, model.arc[arcNr].emission, model.arc[arcNr].safetyBase,
						model.functions.valuesNow.channelCost, model.arc[arcNr].totCost, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
						model.arc[arcNr].fromTime, model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
						x1, y1, x2, y2, model.functions.valuesNow.currentReal, model.functions.valuesNow.currentDirReal,
						model.functions.valuesNow.windReal, model.functions.valuesNow.windDirReal,
						model.functions.valuesNow.current, model.functions.valuesNow.windSpeed,
						model.functions.valuesNow.relWindDir, model.functions.valuesNow.waveHeight, model.functions.valuesNow.relWaveDir,
						model.functions.valuesNow.wavePeriod, model.functions.valuesNow.speedDiffWind,
						model.functions.valuesNow.speedDiffWave, model.functions.valuesNow.baseGroundSpeed,
						model.functions.valuesNow.WindF, model.functions.valuesNow.WaveF, model.functions.valuesNow.CurrentF, model.functions.valuesNow.DelayF);
				else
					fprintf(filPek3, "\n\n");
			}
		}

		//if (lev1 >= 0)
		//	printf("BV iPos %d lev1 %d coord %.3lf %.3lf\n", iPos, lev1,
		//		model.network.physicalLev[lev1].point_x[pointNr1], model.network.physicalLev[lev1].point_y[pointNr1]);
		//if (lev2 >= 0 && lev2 < model.network.nPhysicalLevels)
		//	printf("BV iPos %d lev2 %d coord %.3lf %.3lf\n", iPos, lev2,
		//		model.network.physicalLev[lev2].point_x[pointNr2], model.network.physicalLev[lev2].point_y[pointNr2]);

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
								//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii %d distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, ii, distTmp, x[nPkter], y[nPkter], __LINE__);
								nPkter++;
							}
							// spara pkten dar distTmp + distNu = distanceTp
							distance += model.arc[arcNr].distance / nTp;
							timeNu += model.arc[arcNr].time / nTp;
							fuel += model.arc[arcNr].fuelBase / nTp;
							emission += model.arc[arcNr].emission / nTp;

							legNr = getLegNrFromLevels(lev1, lev2);
							fuel_objCost += model.params.legWeights[legNr].weightFuel * model.arc[arcNr].fuelBase / nTp;
							voyageTime_objCost += model.params.legWeights[legNr].weightTime * model.params.priceTime * model.arc[arcNr].time / nTp;
							emission_objCost += model.params.legWeights[legNr].weightEmission * model.params.scaleObjEmission * model.arc[arcNr].emission / nTp;
							safety_objCost += model.params.legWeights[legNr].weightSafety.base * model.arc[arcNr].safetyBase / nTp;

							safety += model.arc[arcNr].safetyBase / nTp;
							fuel_aux += model.arc[arcNr].fuel_aux / nTp;
							fuel_auxEca += model.arc[arcNr].fuel_auxEca / nTp;
							fuel_eca += model.arc[arcNr].fuel_eca / nTp;
							fuel_noEca += model.arc[arcNr].fuel_noEca / nTp;
							//if(ii == nTp - 1)
							//	printf("arcNr %d fuelArc_noEca %.2lf arcTime %.2lf totFuel_noEca %.2lf\n", arcNr, model.arc[arcNr].fuel_noEca, model.arc[arcNr].time, fuel_noEca);
							hurricane += model.arc[arcNr].safetyHurricane / nTp;
							bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
							greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
							dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
							iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
							feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
							//stability += model.arc[arcNr].safetyStability / nTp;
							channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;
							totCost += model.arc[arcNr].totCost / nTp;
							//printf("total objective cost1 after arcNr (part) %d %.2lf arcCost %.2lf\n", arcNr, totCost, model.arc[arcNr].totCost / nTp);

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
							//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii %d distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, ii, distTmp, x[nPkter], y[nPkter], __LINE__);
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
				//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii last distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, distTmp, x[nPkter], y[nPkter], __LINE__);
				pointLast = spherical::Point(y[nPkter], x[nPkter]);
				//z[nPkter] = 0;
				nPkter++;
			}
		}
		else {
			prefPath = 0;
			if (lev1 >= 0 && lev2 >= 0) {
				if (lev2 < model.network.nPhysicalLevels) {
					if (pointNr1 == model.params.preferredPathOrtoPos[lev1] &&
						pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1
						&& model.params.preferredPathStraightLineFeasibleFrom[lev1] == 0) {
						if (nPkter == 0) {
							y[nPkter] = model.network.physicalLev[lev1].point[pointNr1].latitude().degrees();
							x[nPkter] = model.network.physicalLev[lev1].point[pointNr1].longitude().degrees();
							//fprintf(filtmp, "pkt %d lev1 %d NOprefPath i -- ii -- distTmp -- xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, x[nPkter], y[nPkter], __LINE__);
							//z[nPkter] = 0;
							nPkter++;
						}
						pointLast = spherical::Point(y[nPkter - 1], x[nPkter - 1]);
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
									timeNu += model.arc[arcNr].time / nTp;
									fuel += model.arc[arcNr].fuelBase / nTp;
									emission += model.arc[arcNr].emission / nTp;

									legNr = getLegNrFromLevels(lev1, lev2);
									fuel_objCost += model.params.legWeights[legNr].weightFuel * model.arc[arcNr].fuelBase / nTp;
									voyageTime_objCost += model.params.legWeights[legNr].weightTime * model.params.priceTime * model.arc[arcNr].time / nTp;
									emission_objCost += model.params.legWeights[legNr].weightEmission * model.params.scaleObjEmission * model.arc[arcNr].emission / nTp;
									safety_objCost += model.params.legWeights[legNr].weightSafety.base * model.arc[arcNr].safetyBase / nTp;

									fuel_aux += model.arc[arcNr].fuel_aux / nTp;
									fuel_auxEca += model.arc[arcNr].fuel_auxEca / nTp;
									fuel_eca += model.arc[arcNr].fuel_eca / nTp;
									fuel_noEca += model.arc[arcNr].fuel_noEca / nTp;
									//if (ii == nTp - 1)
									//	printf("arcNr2 %d fuelArc_noEca %.2lf arcTime %.2lf totFuel_noEca %.2lf\n", arcNr, model.arc[arcNr].fuel_noEca, model.arc[arcNr].time, fuel_noEca);
									safety += model.arc[arcNr].safetyBase / nTp;
									hurricane += model.arc[arcNr].safetyHurricane / nTp;
									bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
									greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
									dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
									iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
									feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
									//stability += model.arc[arcNr].safetyStability / nTp;
									channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;
									totCost += model.arc[arcNr].totCost / nTp;
									//printf("total objective cost2 after arcNr (part) %d %.2lf arcCost %.2lf\n", arcNr, totCost, model.arc[arcNr].totCost / nTp);

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
									//fprintf(filtmp, "pkt %d lev1 %d prefPath i3 %d ii %d distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i3, ii, distTmp, x[nPkter], y[nPkter], __LINE__);
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
							//fprintf(filtmp, "pkt %d lev1 %d prefPath i3 %d ii -- distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i3, distTmp, x[nPkter], y[nPkter], __LINE__);
							pointLast = spherical::Point(y[nPkter], x[nPkter]);
							//z[nPkter] = 0;
							nPkter++;
						}
						prefPath = 1;
					}
				}
			}
			if (prefPath == 0) {
				if (lev2 < model.network.nPhysicalLevels) {
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
						//fprintf(filtmp, "pkt %d lev1 %d NOprefPath i3 -- ii -- distTmp -- xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, x[nPkter], y[nPkter], __LINE__);
						//z[nPkter] = 0;
						nPkter++;
					}

					if (lev2 >= 0) {
						yNu = model.network.physicalLev[lev2].point[pointNr2].latitude().degrees();
						xNu = model.network.physicalLev[lev2].point[pointNr2].longitude().degrees();
					}
					else {
						yNu = model.network.channel[-lev2 - 1].point[0].latitude().degrees();
						xNu = model.network.channel[-lev2 - 1].point[0].longitude().degrees();
					}
					pointLast = spherical::Point(y[nPkter - 1], x[nPkter - 1]);
					pointFinal = spherical::Point(yNu, xNu);
					for (ii = 0; ii < nTp - 1; ii++) {
						pointLast = pointLast.destinationPoint(distanceTp, pointLast.bearingTo(pointFinal));
						if (nPkter >= nAlloc) {
							nAlloc += 1000;
							x = (double*)realloc(x, nAlloc * sizeof(double));
							y = (double*)realloc(y, nAlloc * sizeof(double));
						}
						y[nPkter] = pointLast.latitude().degrees();
						x[nPkter] = pointLast.longitude().degrees();
						//fprintf(filtmp, "pkt %d lev1 %d NOprefPath i3 -- ii %d distance -- xy %.3lf %.3lf codeLine %d\n",
						//	nPkter, lev1, ii, x[nPkter], y[nPkter], __LINE__);
						nPkter++;
					}
				}
				distance += model.arc[arcNr].distance;
				timeNu += model.arc[arcNr].time;
				fuel += model.arc[arcNr].fuelBase;
				emission += model.arc[arcNr].emission;
				legNr = getLegNrFromLevels(lev1, lev2);
				fuel_objCost += model.params.legWeights[legNr].weightFuel * model.arc[arcNr].fuelBase;
				voyageTime_objCost += model.params.legWeights[legNr].weightTime * model.params.priceTime * model.arc[arcNr].time;
				emission_objCost += model.params.legWeights[legNr].weightEmission * model.params.scaleObjEmission * model.arc[arcNr].emission;
				safety_objCost += model.params.legWeights[legNr].weightSafety.base * model.arc[arcNr].safetyBase;


				fuel_aux += model.arc[arcNr].fuel_aux;
				fuel_auxEca += model.arc[arcNr].fuel_auxEca;
				fuel_eca += model.arc[arcNr].fuel_eca;
				fuel_noEca += model.arc[arcNr].fuel_noEca;
				//printf("arcNr3 %d fuelArc_noEca %.2lf arcTime %.2lf totFuel_noEca %.2lf\n", arcNr, model.arc[arcNr].fuel_noEca, model.arc[arcNr].time, fuel_noEca);
				safety += model.arc[arcNr].safetyBase;
				hurricane += model.arc[arcNr].safetyHurricane;
				bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
				greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
				dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
				iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
				feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
				//stability += model.arc[arcNr].safetyStability / nTp;
				channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;
				totCost += model.arc[arcNr].totCost;
				//printf("total objective cost3 after arcNr %d %.2lf arcCost %.2lf\n", arcNr, totCost, model.arc[arcNr].totCost);
				if (lev2 < model.network.nPhysicalLevels) {
					if (nPkter >= nAlloc) {
						nAlloc += 1000;
						x = (double*)realloc(x, nAlloc * sizeof(double));
						y = (double*)realloc(y, nAlloc * sizeof(double));
					}
					y[nPkter] = yNu;
					x[nPkter] = xNu;
					//fprintf(filtmp, "pkt %d lev1 %d NOprefPath i3 -- ii -- distTmp -- xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, x[nPkter], y[nPkter], __LINE__);
					//z[nPkter] = 0;
					nPkter++;
				}

			}
		}
	}
	//printf("here 3b\n");

	//fclose(filtmp);

	//if(arcNr >= 0)
	//	addPositionDataToReport(filpekG, posIreport++, arcNr, 1, &timeExact, solName);

	if (skrivMycket == 1) {
		fclose(filPek3);
		if (posIreport > 0)
			fprintf(filpekG, ", ");
		fprintf(filpekG, "%s", linePath.c_str());
	}
	//for (ii2 = 0; ii2 < nPkter; ii2++) {

	model.network.last_x = model.preferredPath.startX;

	if (skrivMycket == 1) {
		for (ii2 = 0; ii2 < model.network.nCoords; ii2++) {
			if (ii2 > 0) {
				fprintf(filpekG, ", ");
			}
			//fprintf(filpekG, "[ %lf, %lf, 0.0 ]\n", x[ii2], y[ii2]);

			fprintf(filpekG, "[ %lf, %lf, 0.0 ]\n", getCorrect_longitude(model.network.xCoord[ii2]), model.network.yCoord[ii2]);
		}

		//printf("here 3c\n");

		fprintf(filpekG, "\n]]},\n\"properties\": {\n");
	}
	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour + startPos * 24; // 0;
	tmBas.tm_min = model.params.startMinute;
	tmBas.tm_sec = 0;

	double averSpeed, dollarCost, lateEtaCost, earlyEtaCost, * faktorer;
	faktorer = (double*)malloc(5 * sizeof(double));
	char* startTime, * endTime;
	startTime = (char*)malloc2(256 * sizeof(char));
	endTime = (char*)malloc2(256 * sizeof(char));

	if (skrivMycket == 1) {
		time_t test = mktime(&tmBas);
		if (test == -1) {
			printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
				tmBas.tm_year,
				tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		}
		fixReadableDate(tmBas, startTime);
		tmBas.tm_min += timeNu * 60;
		test = mktime(&tmBas);
		if (test == -1) {
			printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
				tmBas.tm_year,
				tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		}
		fixReadableDate(tmBas, endTime);

		//if(resAlt == 0)
		//	fprintf(filpekG, "\"solutionID\": \"base\", \"ID\": \"routeInfo\", \"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n", 
		//		startTime, endTime);
		//else
		fprintf(filpekG, "\"solutionID\": \"%s\", \"ID\": \"routeInfo\", \"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n",
			namn, startTime, endTime);
		fprintf(filpekG, ", \"fuelConsumption_ton\": %.2lf, \"fuelEcaMain\": %.2lf, \"fuelEcaAux\": %.2lf, \"fuelNotEcaMain\": %.2lf, \"fuelNotEcaAux\": %.2lf",
			fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca, fuel_eca, fuel_auxEca, fuel_noEca, fuel_aux);
		fprintf(filpekG, ", \"emission\": %.2lf, \"emissionEcaMain\": %.2lf, \"emissionEcaAux\": %.2lf, \"emissionNotEcaMain\": %.2lf, \"emissionNotEcaAux\": %.2lf",
			emission, fuel_eca * model.params.fuel.main_eca.emissionFactor,
			fuel_auxEca * model.params.fuel.aux_eca.emissionFactor, fuel_noEca * model.params.fuel.main_noEca.emissionFactor,
			fuel_aux * model.params.fuel.aux_noEca.emissionFactor);
	}
	if (timeNu > 0)
		averSpeed = distance / timeNu / model.params.knots_to_km;
	else
		averSpeed = 0;

	fuelCostDollar = fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
		fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price;
	dollarCost = fuelCostDollar + timeNu * model.params.priceTime + channelCost;

	//fuel_objCost = model.params.legWeights[legNr].weightFuel * fuel;
	//voyageTime_objCost = model.params.legWeights[legNr].weightTime * timeNu * model.params.priceTime;
	//emission_objCost = model.params.legWeights[legNr].weightEmission * emission * model.params.scaleObjEmission;

	lateEtaCost = 0;
	earlyEtaCost = 0;

	int nStormsPath;
	double timeDelay;

	if (skrivMycket == 1) {
		fprintf(filpekG, ", \"average speed\": %.1lf, \"safety\": %.3lf, \"channel cost\": %.1lf, \"total objective cost\": %.1lf,",
			averSpeed, safety, channelCost, totCost);
		fprintf(filpekG, "    \"dollar_cost\":%.0lf,\n", dollarCost);
		fprintf(filpekG, "    \"fuel_dollarCost\":%.0lf,\n", fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
			fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price);
		fprintf(filpekG, "    \"voyageTime_dollarCost\":%.0lf,\n", timeNu * model.params.priceTime);

		fprintf(filpekG, "    \"fuel_objCost\":%.1lf,\n", fuel_objCost);
		fprintf(filpekG, "    \"voyageTime_objCost\":%.1lf,\n", voyageTime_objCost);
		fprintf(filpekG, "    \"emission_objCost\":%.1lf,\n", emission_objCost);

		fprintf(filpekG, "\"storms\": [\n");
		nStormsPath = 0;
		for (i = 0; i < model.nStorms; i++) {
			forsta = 1;
			if (model.storms[i].closestPointToRoute < 1e6) {
				if (forsta == 0)
					fprintf(filpekG, ", ");
				forsta = 0;
				fixPositionString_latLon(model.storms[i].feature[model.storms[i].nFeatures - 1].lat,
					model.storms[i].feature[model.storms[i].nFeatures - 1].lon, startTime);
				fprintf(filpekG, "  {\"stormID\":\"%s\", \"STORMNAME\":\"%s\", \"CPA_nm\": %.1lf, \"latestPositionKnown\":\"%s\"}\n",
					model.storms[i].stormID, model.storms[i].stormName, model.storms[i].closestPointToRoute / model.params.knots_to_km,
					startTime);
				nStormsPath++;
			}
		}
		fprintf(filpekG, "], ");

		if (model.functions.valuesNow.maxDiffTime > 0) {
			fprintf(filpekG, "\"maxLateMidTimeArrive_h\": %.1lf, \"level_maxLateMidTimeArrive\": %d,\n",
				model.functions.valuesNow.maxDiffTime, model.functions.valuesNow.maxDiffTime_level);
			errlog("maxLateMidTimeArrive_h %.1lf level %d av %d\n", model.functions.valuesNow.maxDiffTime,
				model.functions.valuesNow.maxDiffTime_level, model.network.nPhysicalLevels);
		}
		if (model.functions.valuesNow.minDiffTime < 0) {
			fprintf(filpekG, "\"maxEarlyMidTimeArrive_h\": %.1lf, \"level_maxEarlyMidTimeArrive\": %d,\n",
				model.functions.valuesNow.minDiffTime, model.functions.valuesNow.minDiffTime_level);
			errlog("minLateMidTimeArrive_h %.1lf level %d av %d\n", model.functions.valuesNow.minDiffTime,
				model.functions.valuesNow.minDiffTime_level, model.network.nPhysicalLevels);
		}
		fprintf(filpekG, "\"totalDistance_kts\": %.1lf, \"totalTime_h\": %.1lf}}\n", distance / model.params.knots_to_km, timeNu);

		//fprintf(filpekG, "]\n}");

		fclose(filpekG);

		fprintf(filPek, "total\tcombined\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
			distance, timeNu, fuel, emission, safety, channelCost, totCost);
		fprintf(filPek, "\nobj_weights\nobj_nr\tlev1\tlev2\time\tfuel\tsafety\n");
		for(int i11 = 0; i11 < model.params.nLegs; i11++)
			fprintf(filPek, "%d\t%d\t%d\t%lf\t%lf\t%lf\n", i,
				model.params.legWeights[i11].level1, model.params.legWeights[i11].level2,
				model.params.legWeights[i11].weightTime, 1.0,
				model.params.legWeights[i11].weightSafety.base);

		printf("dist\t%.2lf\ntime\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\n"
			"fuel\t%.2lf\teca\t%.2lf\tnonEca\t%.2lf\tcost\t%.2lf\t"
			"obj\t%.2lf\n"
			"emission\t%.2lf\t%.2lf\tobj\t%.2lf\n"
			"safety\t%.2lf\tobj\t%.2lf\nchannelCost\t%.2lf\n",
			distance, timeNu, model.params.priceTime * timeNu, voyageTime_objCost,
			fuel_aux + fuel_auxEca + fuel_eca + fuel_noEca, fuel_aux + fuel_eca, fuel_noEca,
			fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price + 
			fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price,
			fuel_objCost, emission, emission_objCost,safety, safety_objCost, channelCost);

		for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
			if (nSpeedSettingUsed[i] > 0) {
				fprintf(filPek, "used rpm_setting %.2lf %d times\n", model.functions.rpmBase[i], nSpeedSettingUsed[i]);
				printf("used rpm_setting %.2lf %d times\n", model.functions.rpmBase[i], nSpeedSettingUsed[i]);
			}
		}
		fprintf(filPek, "The speed settings are changed %d times during the trip\n", nSpeedChanges);
		printf("The speed settings are changed %d times during the trip\n", nSpeedChanges);


	}
	else {
		nStormsPath = 0;
		for (i = 0; i < model.nStorms; i++) {
			if (model.storms[i].closestPointToRoute < 1e6) {
				nStormsPath++;
			}
		}
		printf("dist\t%.2lf\ttime\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\tnStorms\t%d\n",
			distance, timeNu, model.params.priceTime * timeNu, voyageTime_objCost, nStormsPath);
	}


	if (startPos == -1) {
		model.delay.SPsol.costs = dollarCost;
		model.delay.SPsol.time = timeNu;
		model.delay.SPsol.distance = distance;
		model.delay.SPsol.nSlowSpeed = nSpeedSettingUsed[0];
		model.delay.SPsol.nHighSpeed = nSpeedSettingUsed[1];
		if (yearPos == 0) {
			FILE* filDelay = fopen("data/res_delaySP.txt", "a+");
			fprintf(filDelay, "%d\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%d\t%.3lf\t%.3lf\t%.3lf\t%d\t%d\t%d\n", node,
				model.preferredPath.point_x[0], model.preferredPath.point_y[0], model.preferredPath.point_x[1],
				model.preferredPath.point_y[1],
				model.delay.group[node][alt].direction,
				dollarCost, timeNu, distance, nSpeedSettingUsed[0], nSpeedSettingUsed[1],
				model.delay.group[node][alt].arcNr);
			fclose(filDelay);
		}
	}
	else {
		FILE* filDelay = fopen("data/res_delay.txt", "a+");
		timeDelay = timeNu / model.delay.SPsol.time;
		if (nStormsPath > 0) {
			if (timeDelay < 1.6)
				timeDelay = 1.6;
		}
		fprintf(filDelay, "%d\t%d\t%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%d\t%d\t%.4lf\t%.4lf\t%.4lf",
			node, alt, model.delay.year[yearPos], startPos + 1,
			model.delay.group[node][alt].direction,
			dollarCost, timeNu, distance, nSpeedSettingUsed[0], nSpeedSettingUsed[1],
			dollarCost / model.delay.SPsol.costs, timeDelay,
			distance / model.delay.SPsol.distance);

		for (i = 0; i < 2; i++) {
			fprintf(filDelay, "\t%.3lf\t%.3lf", model.preferredPath.point_x[i], model.preferredPath.point_y[i]);
		}
		nPkter = determine_nPointsDelayVisuellt(model.preferredPath.point[0].distanceTo(model.preferredPath.point[1]) / 1000.0,
			faktorer);
		fprintf(filDelay, "\t%d", nPkter);
		for (i = 0; i < nPkter; i++) {
			double x = get_lonFromKvotAvTva_delay(model.preferredPath.point_x[0], model.preferredPath.point_x[1], faktorer[i]);
			fprintf(filDelay, "\t%.3lf\t%.3lf", x,
				model.preferredPath.point_y[0] * (1 - faktorer[i]) + model.preferredPath.point_y[1] * faktorer[i]);
		}
		fprintf(filDelay, "\t%d\n", model.delay.group[node][alt].arcNr);
		fclose(filDelay);
	}

	if (skrivMycket == 1) {
		fclose(filPek);
		fclose(filPek2);
	}

	free(faktorer);
	free(namn);
	free(x);
	free(y);
	free(nSpeedSettingUsed);

	return 0;
}

int solveOnlyShortestPathWithoutTime_delay(int node, int yearPos, int alt) {
	int nod1, nod2;
	bool Reached;
	double dist;
	long long Cost;

	checkMinnesAnvandning(__LINE__);
	int retVal = setupNodesArcsNoTime_delay();
	if (retVal < 0)
		return retVal;

	if (model.BVArc == NULL) {
		model.BVArc = (int*)malloc2(10000 * sizeof(int));
		model.BVtempNodOrder = (int*)malloc2(10000 * sizeof(int));

		model.network.startKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
		model.network.endKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
		model.network.posSplitCoord = (int*)malloc2((model.network.nMaxSplits + 1) * sizeof(int));

		model.optPath.level = (strOptPathLevel*)malloc2(model.network.nPhysicalLevels * sizeof(strOptPathLevel));
		for (int lev1 = 0; lev1 < model.network.nPhysicalLevels; lev1++) {
			model.optPath.level[lev1].timeArrive = -1;
			model.optPath.level[lev1].pointNr = -1;
			model.optPath.level[lev1].baseSpeedSettingNr = -1;
			model.optPath.level[lev1].levelNext = -1;
		}
		model.optPath.channel = (strOptPathChannel*)malloc2(model.network.nChannels * sizeof(strOptPathChannel));
		for (int lev1 = 0; lev1 < model.network.nChannels; lev1++) {
			model.optPath.channel[lev1].timeArriveNext = -1;
			model.optPath.channel[lev1].speedSettingNrNext = -1;
			model.optPath.channel[lev1].levelNext = -1;
			model.optPath.channel[lev1].timeArriveThrough = -1;
			model.optPath.channel[lev1].speedSettingNrThrough = -1;
		}
		model.Dijkstra.nodes = NULL;

		model.waypointResult.dateUTC = (char*)malloc(256 * sizeof(char));
		model.waypointResult.full_Date = (char*)malloc(256 * sizeof(char));
		model.waypointResult.fixPositionString_latlon = (char*)malloc(256 * sizeof(char));
		model.waypointResult.windDirReal_letters = (char*)malloc(256 * sizeof(char));
		model.waypointResult.waveDir_letters = (char*)malloc(256 * sizeof(char));

		//model.params.preferredPathUseChannelSpeed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
		//model.params.preferredPathUseChannelConsumption = (int*)malloc2(model.network.nPhysicalLevels * sizeof(int));
		//for (int i = 0; i < model.network.nPhysicalLevels; i++) {
			//model.params.preferredPathUseChannelSpeed[i] = -1;
		//	model.params.preferredPathUseChannelConsumption[i] = -1;
		//}
	}

	SattUppDijkstraNatverk3(&model);
	nod1 = 0;
	nod2 = model.nNoder - 1;
	AnropDijkstra2(nod1, nod2, &model, &Reached);
	if (Reached == true) {
		dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
		if (model.nBVArcs < 2) {
			errlog("ERROR! Too few arcs %d in Dijkstra solution\n", model.nBVArcs);
			return -1;
		}
		else {
			writeSolutionToJson_delay(node, alt, yearPos, -1);
		}
	}
	else {
		errlog("ERROR! Did not manage to find a route from start to finish...\n");
		printf("\nERROR! Did not manage to find a route from start to finish...\n");
		return -1;
	}
	
	if(skrivMycket == 1)
		printf("xy %.2lf %.2lf %.2lf %.2lf dir %d dist %.1lf time %.1lf speed %.3lf nPhysLev %d\n", 
			model.delay.group[node][alt].point_x1, model.delay.group[node][alt].point_y1,
			model.delay.group[node][alt].point_x2, model.delay.group[node][alt].point_y2,
			model.delay.group[node][alt].direction,
			model.delay.SPsol.distance, model.delay.SPsol.time, model.delay.SPsol.distance / model.delay.SPsol.time,
			model.network.nPhysicalLevels);
	return 0;
}


int createTimeArcs_delay(int year, int startDay, int neighbourPos)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0, returnVal;
	double fuel, safety, tid, totCost, fuelQualityKvot;

	model.nErrorCoordBB = 0;

	if (model.weatherFunctions.nAllocPoints == 0) {
		model.weatherFunctions.nAllocPoints = 50;
		model.weatherFunctions.vesselBearing = (double*)malloc2(
			model.weatherFunctions.nAllocPoints * sizeof(double));
		model.weatherFunctions.point_lat = (double*)malloc2(
			model.weatherFunctions.nAllocPoints * sizeof(double));
		model.weatherFunctions.point_lon = (double*)malloc2(
			model.weatherFunctions.nAllocPoints * sizeof(double));
		model.weatherFunctions.checkPoint = (strCheckPkt*)malloc2(model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
		for (i = 0; i < model.weatherFunctions.nAllocPoints; i++) {
			model.weatherFunctions.checkPoint[i].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			model.weatherFunctions.checkPoint[i].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			model.weatherFunctions.checkPoint[i].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
		}
		model.nAllocArcs = 500000;
		model.arc = (strArcInfo*)malloc2(model.nAllocArcs * sizeof(strArcInfo));

		model.delay.stormsYear = (strStorm**)calloc2(model.delay.nYears, sizeof(strStorm));
		model.delay.nStormsYear = (int*)malloc2(model.delay.nYears * sizeof(int));

	}
	if (startDay == 0 && neighbourPos == 0 || model.weather[0].valueCell == NULL) {
		if (model.params.checkGribFilesSpecial == 1)
			returnVal = loadWeatherFiles_checkData(year);
		else
			returnVal = loadWeatherFiles_delayed_new(year);
		if (returnVal == -1)
			return -1;
	}

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].midTimeArrive = 0;
		if (i == 0) {
			model.nArcs = 0;
			model.nNoder = 0;
			model.network.physicalLev[i].nTimeIntervals[0] = 1;
			model.network.physicalLev[i].timeInterval[0][0] = 24 * startDay;
			model.network.physicalLev[i].nodNr_from_pt[0][0] = model.nNoder;
			adderaNod(i, 0, 0);
		}
		else {
			for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
				model.network.physicalLev[i].nTimeIntervals[i1] = 0;
			}
		}
	}
	for (i = 0; i < model.network.nChannels; i++) {
		model.network.channel[i].nTimeIntervals[0] = 0;
		model.network.channel[i].nTimeIntervals[1] = 0;
	}

#ifdef _WIN32
	std::chrono::steady_clock::time_point tid1, tid2, tid3, tid4, tid3b, tid3c, tid3d, tt;
#else
	std::chrono::system_clock::time_point tid1, tid2, tid3, tid4, tid3b, tid3c, tid3d, tt;
#endif
	std::chrono::duration<double, std::milli> dur2, dur3, dur4, dur3b, dur3c, dur3d;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	errlog("\n");

	nArcsTot = 0;
	min_t = 0;
	max_t = 999999;

	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		if (i >= 16)
			i = i;

		n_added_t = 0;
		nArcsNu = 0;
		model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
		tid1 = std::chrono::high_resolution_clock::now();

		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			//if (model.optPath.level[i].pointNr >= 0 && abs(i1 - model.optPath.level[i].pointNr) > model.params.maxDiff_pointNrFas3)
			//	continue;

			if (model.network.physicalLev[i].nTimeIntervals[i1] == 0)
				continue; // inga tidsbagar till denna punkt
			if (i1 == 24)
				i1 = i1;

			if (i == model.network.nPhysicalLevels - 2 && i1 == model.params.preferredPathOrtoPos[i])
				i = i;
			if (model.network.physicalLev[i].nArcsToPoint[i1] == 0 && i > 0)
				continue; // no arc to this point so no use to add arcs out


			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];
				if (nextLevel < 0)
					nextLevel = nextLevel;
				setupCheckPoints = 1;
				fuelQualityKvot = 1;
				// get_minMax_timeFromLevel(nextLevel, &min_t, &max_t);
				addBagar_AB_speedSTid(i, i1, nextLevel, i2, i2b, &setupCheckPoints, min_t, max_t, fuelQualityKvot, 0.0);
			}
		}
		model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
		model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
		tid2 = std::chrono::high_resolution_clock::now();

		for (i1 = 0; i1 < model.network.nChannels; i1++) {
			if (i == 0 && model.network.channel[i1].earliestStartLevel == i)
				addChannelArcs(i1, 0);
			cNr = i1;
			for (i2b = 0; i2b < model.network.channel[cNr].nOutNodes; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[i2b];
				if (nextLevel != i + 1 && nextLevel >= 0)
					continue;
				if (nextLevel < 0 && i > 0)
					continue; // i > 0 since onle add these arcs once...
				if (model.network.channel[cNr].outRestrictedAreaNr[i2b] == -2) {
					continue; // do not include this arc as a tss should be used instead.
				}
				setupCheckPoints = 1;
				fuelQualityKvot = 1;
				// get_minMax_timeFromLevel(nextLevel, &min_t, &max_t);
				addBagar_AB_speedSTid(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], i2b, &setupCheckPoints, min_t, max_t, fuelQualityKvot, 0.0);
			}
		}

		nArcsTot += nArcsNu;
		if (skrivMycket == 1)
			printf("\tLevel %d done (of %d). I have %d arcs now.\n",
				i + 1, model.network.nPhysicalLevels - 1, model.nArcs);
	}

	errlog("\n");

	// add arcs from last node and time to a super sink
	nArcsNu = 0;
	nodNr2 = adderaNod(i + 1, 0, 0);
	i1 = 0;
	for (i3 = 0; i3 < model.network.physicalLev[i].nTimeIntervals[i1]; i3++) {
		addEndBage(i, i1, i + 1, i3, nodNr2, i3);
		nArcsNu++;
	}

	nodNr1 = nodNr2;
	nodNr2 = adderaNod(i + 1, 0, 0);
	addEndBage(i + 1, 0, i + 2, 0, nodNr2, 0);

	nArcsTot += nArcsNu;
	free(namn);
	return 0;
}

int openNeededWeatherFiles(int year)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0, returnVal;
	double fuel, safety, tid, totCost, fuelQualityKvot;

	model.nErrorCoordBB = 0;

	if (model.weatherFunctions.nAllocPoints == 0) {
		model.weatherFunctions.nAllocPoints = 50;
		model.weatherFunctions.vesselBearing = (double*)malloc2(
			model.weatherFunctions.nAllocPoints * sizeof(double));
		model.weatherFunctions.point_lat = (double*)malloc2(
			model.weatherFunctions.nAllocPoints * sizeof(double));
		model.weatherFunctions.point_lon = (double*)malloc2(
			model.weatherFunctions.nAllocPoints * sizeof(double));
		model.weatherFunctions.checkPoint = (strCheckPkt*)malloc2(model.weatherFunctions.nAllocPoints * sizeof(strCheckPkt));
		for (i = 0; i < model.weatherFunctions.nAllocPoints; i++) {
			model.weatherFunctions.checkPoint[i].latPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			model.weatherFunctions.checkPoint[i].lonPos = (int*)malloc2(model.nWeatherFiles * sizeof(int));
			model.weatherFunctions.checkPoint[i].pos_latLon = (int*)malloc2(model.nWeatherFiles * sizeof(int));
		}
		model.nAllocArcs = 50;
		model.arc = (strArcInfo*)malloc2(model.nAllocArcs * sizeof(strArcInfo));

		model.delay.stormsYear = (strStorm**)calloc2(model.delay.nYears, sizeof(strStorm));
		model.delay.nStormsYear = (int*)malloc2(model.delay.nYears * sizeof(int));
	}

	if(model.params.checkGribFilesSpecial == 1)
		returnVal = loadWeatherFiles_checkData(year);
	else
		returnVal = loadWeatherFiles_delayed_new(year);
	if (returnVal == -1)
		return -1;

	return 0;
}

void setupUsableSpeedSettings_delay() {
	int nAlloc, minPos = 0, maxPos = 0, midPos, add95, iUse, i, indexUnder, indexOver, i1;
	double min_rpm, max_rpm, midVal, diff, min_diff, delta, target, kvot;

	model.network.nPhysicalLevels = 1;
	model.functions.speedLevel = (strSpeed*)malloc2(model.network.nPhysicalLevels * sizeof(strSpeed));
	int nShip_speedSettings;
	double maxSpeed = 0, minSpeed = 1e10;

	nShip_speedSettings = model.functions.nShip_speedSettingsBase;
	nAlloc = model.functions.nShip_speedSettingsBase;

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.functions.speedLevel[i].rpm = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerCalmWaterSpeed = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerFuelConsumption_main = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerFuelConsumption_aux = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].settingGerBaseSetting = (int*)malloc2(nAlloc * sizeof(int));
		model.functions.speedLevel[i].nShip_speedSettings = nShip_speedSettings;
	}

	i = 0;
	for (iUse = 0; iUse < model.functions.nShip_speedSettingsBase; iUse++) {
		for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
			set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, iUse);
			if (i1 == 0)
				errlog(" %d %.2lf", i, model.functions.speedLevel[i1].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
		}
		i++;
	}
	errlog(" knots\n");
	model.functions.speedSetting95MCR_base = model.functions.nShip_speedSettingsBase - 1;

	if (model.params.nLegs != 1) {
		errlog("ERROR! Only one leg can be used (now %d) when setting up speed settings for delay - creating delay maps\n", 
			model.params.nLegs);
		postRequest("ERROR! Only one leg can be used (now " + std::to_string(model.params.nLegs) + ") when setting up speed settings for delay - creating delay maps", 1);
	}
	model.functions.speedSetting95MCR_use = (int*)malloc(model.params.nLegs * sizeof(int));
	model.functions.speedSetting95MCR_use[0] = model.functions.speedSetting95MCR_base;

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

	model.params.preferredSpeed_calmWater = averSpeed / model.functions.speedLevel[0].nShip_speedSettings;
	model.params.calmWaterSpeedMin = minSpeed;
	model.params.calmWaterSpeedMax = maxSpeed;
	errlog("OBS! Setting preferred speed to average of all speed settings right now: %.2lf knots (it is modified if eta is given)\n",
		model.params.preferredSpeed_calmWater / model.params.knots_to_km);


}
void setupUsableSpeedSettings_delayOld() {
	int nAlloc, minPos = 0, maxPos = 0, midPos, add95, iUse, i, indexUnder, indexOver, i1;
	double min_rpm, max_rpm, midVal, diff, min_diff, delta, target, kvot;

	model.functions.speedLevel = (strSpeed*)malloc2(model.network.nPhysicalLevels * sizeof(strSpeed));
	model.functions.speedChannel = (strSpeed*)malloc2(model.network.nChannels * sizeof(strSpeed));
	model.functions.speedChannelOut = (strSpeed*)malloc2(model.network.nChannels * sizeof(strSpeed));
	int nShip_speedSettings;
	double maxSpeed = 0, minSpeed = 1e10;

	nShip_speedSettings = model.functions.nShip_speedSettingsBase;
	nAlloc = model.functions.nShip_speedSettingsBase;

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.functions.speedLevel[i].rpm = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerCalmWaterSpeed = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerFuelConsumption_main = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].rpmSetting_gerFuelConsumption_aux = (double*)malloc2(nAlloc * sizeof(double));
		model.functions.speedLevel[i].settingGerBaseSetting = (int*)malloc2(nAlloc * sizeof(int));
		model.functions.speedLevel[i].nShip_speedSettings = nShip_speedSettings;
	}

	i = 0;
	for (iUse = 0; iUse < model.functions.nShip_speedSettingsBase; iUse++) {
		for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
			set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), i, iUse);
			if (i1 == 0)
				errlog(" %d %.2lf", i, model.functions.speedLevel[i1].rpmSetting_gerCalmWaterSpeed[i] / model.params.knots_to_km);
		}
		i++;
	}
	errlog(" knots\n");
	model.functions.speedSetting95MCR_base = model.functions.nShip_speedSettingsBase - 1;
	model.functions.speedSetting95MCR_use[0] = model.functions.speedSetting95MCR_base;

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

	model.params.preferredSpeed_calmWater = averSpeed / model.functions.speedLevel[0].nShip_speedSettings;
	model.params.calmWaterSpeedMin = minSpeed;
	model.params.calmWaterSpeedMax = maxSpeed;
	errlog("OBS! Setting preferred speed to average of all speed settings right now: %.2lf knots (it is modified if eta is given)\n",
		model.params.preferredSpeed_calmWater / model.params.knots_to_km);


}

int testCallWeatherFile() {

	int xPos0, xPos1, yPos0, yPos1, nBands;
	int nAlloc;
	char* namn = (char*)malloc2(256 * sizeof(char));
	int nMaxTimeInt = 0;
	long long nSecondsUTC;	
	
	strWeather weather;
	Raster Test;
	weather.rasterPos = (Raster*)malloc2(4 * sizeof(Raster));
	int i1 = 1;
	checkMinnesAnvandning(__LINE__);
	sprintf(namn, "data/weather/ucurr1_nov_2018.grb");
	//sprintf(namn, "data/weather/out.tif");
	int returnVal = weather.rasterPos[i1].open(namn);
	checkMinnesAnvandning(__LINE__);

	model.boundingBox.yMin=28.5745;
	model.boundingBox.yMax=52.5118;
	model.boundingBox.xMin=-73.4723;
	model.boundingBox.xMax=-47.9467;

	double size_col, size_row, xPosFrac, yPosFrac;
	size_col = weather.rasterPos[i1].Get_sizeCol();
	weather.size_col = size_col;
	xPosFrac = (model.boundingBox.xMin - weather.rasterPos[i1].Get_minLongitude()) / size_col;
	xPos0 = roundDown(xPosFrac);/////////////////////////////////////////////// roundDown!!!
	weather.minX = weather.rasterPos[i1].Get_minLongitude() +
		xPos0 * size_col;
	xPosFrac = (model.boundingBox.xMax - weather.minX) /
		size_col;
	xPos1 = roundUp(xPosFrac);
	weather.maxX = weather.minX +
		xPos1 * size_col;
	weather.nCols = xPos1 + 1;

	size_row = weather.rasterPos[i1].Get_sizeRow();
	weather.size_row = size_row;
	yPosFrac = (weather.rasterPos[i1].Get_maxLatitude() - model.boundingBox.yMax) /
		size_row;
	yPos0 = roundDown(yPosFrac);
	weather.maxY = weather.rasterPos[i1].Get_maxLatitude() -
		yPos0 * size_row;
	yPosFrac = (weather.maxY - model.boundingBox.yMin) /
		size_row;
	yPos1 = roundUp(yPosFrac);
	if (yPos1 >= weather.rasterPos[i1].Get_nRows())
		yPos1 = weather.rasterPos[i1].Get_nRows() - 1;
	weather.minY = weather.maxY -
		yPos1 * size_row;
	weather.nRows = yPos1 + 1;

	nBands = weather.rasterPos[i1].Get_nBands();
	weather.nTimeIntervals = nBands;
	weather.nTimeIntervals_forecast = nBands;
	weather.secondsUTC = (long long*)malloc2(nBands * sizeof(long long));
	weather.valueCell = (float**)malloc2(nBands * sizeof(float*));
	nAlloc = weather.nCols * weather.nRows;
	for (int i2 = 0; i2 < nBands; i2++) {
		weather.valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));

		nSecondsUTC = weather.rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
		weather.secondsUTC[i2] = nSecondsUTC;
		
	}
	model.params.UTC_secondsStart = weather.secondsUTC[0];

	checkMinnesAnvandning(__LINE__);
	//weather.valueCell = weather.rasterPos[i1].GetRasterValues_realAllBandsTest(&(weather), 0);
	weather.timePosToBandPos = NULL;
	weather.rasterPos[i1].GetRasterValues_realAllBands(&(weather), 0, 1.0);
	checkMinnesAnvandning(__LINE__);
	return 0;
}


double get_colDblFromRaster(Raster raster, double lon)
{
	double colDbl, tmpLon = lon;

	if (lon < raster.Get_minLongitude()) {
		if (raster.Get_maxLongitude() - raster.Get_minLongitude() < 355) {
			if (lon < raster.Get_minLongitude() - 20)
				lon += 360;
		}
		else
			lon += 360;
	}
	if (lon > raster.Get_maxLongitude()) {
		if (raster.Get_maxLongitude() - raster.Get_minLongitude() < 355) {
			if (lon > raster.Get_maxLongitude() + 20)
				lon -= 360;
		}
		else
			lon -= 360;
	}

	colDbl = (lon - raster.Get_minLongitude()) / raster.Get_sizeCol();
	if (colDbl < 0)
		colDbl += raster.Get_nCols();
	if (colDbl < 0 || colDbl >= raster.Get_nCols()) {
		if (colDbl < -0.1 || colDbl > raster.Get_nCols() + 0.1) {
			errlog("ERROR! This should not happen. Fix it!! colDbl %.3lf nCols %d lon %.3lf minLon maxLon %.3lf\n",
				colDbl, raster.Get_nCols(), lon,
				raster.Get_minLongitude(), raster.Get_maxLongitude());
			exit(0);
		}
		else {
			if (colDbl < 0)
				colDbl = 0;
			else
				colDbl = raster.Get_nCols() - 0.1;
		}
	}


	return colDbl;
}

int extractGribInfo(std::string inputPath)
{
	std::ifstream fil;

	resultPath = "data";
	reset_errlog();

	printf("opens %s\n", inputPath.c_str());
	fil.open(inputPath.c_str());

	json data, dataSpeed, dataVar, dataIt;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", inputPath.c_str());
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", inputPath.c_str());
		exitKontrollerat(__LINE__);
	}

	std::string namn;
	Raster raster;
	int pos = 0, nBands, nAlloc, i2, i3, nXblock, nYblock;
	long long nSecondsUTC;
	FILE* filpek;
	double x1 = -38.46, y1 = 16.02, x2, y2; // i Atlanten
	double sizeCol, minLon, rowDbl, colDbl;
	double xPosFrac, yPosFrac, size_row;
	int xPos0, yPos0, xPos1, yPos1;
	int nCols, nRows, row, col, rowBlock, colBlock, blockY, blockX;
	strWeather weatherData;


	//x1 = model.boundingBox.xMin;
	x2 = x1 + 10;// model.boundingBox.xMax;
	//y1 = model.boundingBox.yMin;
	y2 = y1 + 10;// model.boundingBox.yMax;

	weatherData.secondsUTC = NULL;

	filpek = fopen("checkTimeDateGribfiles.txt", "w");
	errlog("fileName\tpos\tnBands\tsizeCol\tsizeRow\tminLongitude\tmaxLatitude\tnCols\tnRows\n");
	for (auto it = data.begin(); it != data.end(); ++it) {
		dataIt = it.value();
		if (!dataIt["fileName"].is_null()) {
			namn = dataIt["fileName"];
		}
		else
			continue;

		printf("opens %s\n", namn.c_str());
		raster.open(namn.c_str());
		printf("done\n");

		nBands = raster.Get_nBands();

		sizeCol = raster.Get_sizeCol();
		minLon = raster.Get_minLongitude();
		nCols = raster.Get_nCols();
		nRows = raster.Get_nRows();
		errlog("%s\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%d\t%d\n", namn.c_str(), pos, nBands,
			sizeCol, raster.Get_sizeRow(), minLon, raster.Get_maxLatitude(),
			nCols, nRows);


		for (int i2 = 0; i2 < nBands; i2++) {
			printf("get time %d\n", i2);
			nSecondsUTC = raster.GetSecondsFromUTC_metadataBand(i2 + 1);
			fprintf(filpek, "%s\t%d\t%d\t%lf\t%I64d\t%s\n", namn.c_str(), pos, i2,
				0,
				nSecondsUTC, stringDateFromUTCSeconds(nSecondsUTC).c_str());
		}


		pos++;

	}
	fclose(filpek);


	return 0;
}

int extractGribInfo2(std::string inputPath)
{
	std::ifstream fil;

	resultPath = "data";
	reset_errlog();

	printf("opens %s\n", inputPath.c_str());
	fil.open(inputPath.c_str());

	json data, dataSpeed, dataVar, dataIt;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", inputPath.c_str());
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", inputPath.c_str());
		exitKontrollerat(__LINE__);
	}

	std::string namn;
	Raster raster;
	int pos = 0, nBands, nAlloc, i2, i3, nXblock, nYblock;
	long long nSecondsUTC;
	FILE* filpek;
	double x1 = -38.46, y1 = 16.02, x2, y2; // i Atlanten
	double sizeCol, minLon, rowDbl, colDbl;
	double xPosFrac, yPosFrac, size_row;
	int xPos0, yPos0, xPos1, yPos1;
	int nCols, nRows, row, col, rowBlock, colBlock, blockY, blockX;
	strWeather weatherData;


	//x1 = model.boundingBox.xMin;
	x2 = x1 + 10;// model.boundingBox.xMax;
	//y1 = model.boundingBox.yMin;
	y2 = y1 + 10;// model.boundingBox.yMax;

	weatherData.secondsUTC = NULL;

	filpek = fopen("checkTimeDateGribfiles.txt", "w");
	errlog("fileName\tpos\tnBands\tsizeCol\tsizeRow\tminLongitude\tmaxLatitude\tnCols\tnRows\n");
	for (auto it = data.begin(); it != data.end(); ++it) {
		dataIt = it.value();
		if (!dataIt["fileName"].is_null()) {
			namn = dataIt["fileName"];
		}
		else
			continue;

		raster.open(namn.c_str());

		nBands = raster.Get_nBands();

		sizeCol = raster.Get_sizeCol();
		minLon = raster.Get_minLongitude();
		nCols = raster.Get_nCols();
		nRows = raster.Get_nRows();
		errlog("%s\t%d\t%d\t%lf\t%lf\t%lf\t%lf\t%d\t%d\n", namn.c_str(), pos, nBands,
			sizeCol, raster.Get_sizeRow(), minLon, raster.Get_maxLatitude(),
			nCols, nRows);


		weatherData.size_col = sizeCol;
		xPosFrac = (x1 - raster.Get_minLongitude()) / sizeCol;
		xPos0 = roundDown(xPosFrac);
		weatherData.minX = raster.Get_minLongitude() +
			xPos0 * sizeCol;
		xPosFrac = (x2 - weatherData.minX) /
			sizeCol;
		xPos1 = roundUp(xPosFrac);
		weatherData.maxX = weatherData.minX +
			xPos1 * sizeCol;
		weatherData.nCols = xPos1 + 1;
		size_row = raster.Get_sizeRow();
		weatherData.size_row = size_row;
		yPosFrac = (raster.Get_maxLatitude() - y2) /
			size_row;
		yPos0 = roundDown(yPosFrac);
		weatherData.maxY = raster.Get_maxLatitude() -
			yPos0 * size_row;
		yPosFrac = (weatherData.maxY - y1) /
			size_row;
		yPos1 = roundUp(yPosFrac);
		if (yPos1 >= raster.Get_nRows())
			yPos1 = raster.Get_nRows() - 1;
		weatherData.minY = weatherData.maxY -
			yPos1 * size_row;
		weatherData.nRows = yPos1 + 1;
		weatherData.valueCell = (float**)malloc2(nBands * sizeof(float*));
		
		raster.Get_BlockSize(1, &nXblock, &nYblock);
		//nAlloc = nXblock * nYblock;
		nAlloc = weatherData.nCols * weatherData.nRows;
		for (int i2 = 0; i2 < nBands; i2++) {
			weatherData.valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
			for (int i3 = 0; i3 < nAlloc; i3++)
				weatherData.valueCell[i2][i3] = 9999;
		}


		rowDbl = (raster.Get_maxLatitude() - y1) / raster.Get_sizeRow();
		blockY = (int)(rowDbl / nYblock);
		colDbl = 287.8;// get_colDblFromRaster(raster, x1);
		colDbl = get_colDblFromRaster(raster, x1);
		blockX = (int)(colDbl / nXblock);
		row = (int)rowDbl;
		col = (int)colDbl;

		rowBlock = row - blockY * nYblock;
		colBlock = col - blockX * nXblock;

		pos = colBlock + nXblock * rowBlock;

		weatherData.timePosToBandPos = NULL;
		raster.GetRasterValues_realAllBands(&weatherData, 0, 1.0);
		for (int i2 = 0; i2 < nBands; i2++) {
			nSecondsUTC = raster.GetSecondsFromUTC_metadataBand(i2 + 1);
			fprintf(filpek, "%s\t%d\t%d\t%lf\t%I64d\t%s\n", namn.c_str(), pos, i2,
				weatherData.valueCell[i2][pos],
				nSecondsUTC, stringDateFromUTCSeconds(nSecondsUTC).c_str());
		}


		pos++;

	}
	fclose(filpek);


	return 0;
}

int generateDelayedFactors_old(std::string inputPath, int node, int manad)
{

	double dist;
	long long Cost;
	reset_errlog();
	errlog("delay for manad %d node %d\n", manad, node);
	FILE* filTmp;

	if (node == 0) {
		filTmp = fopen("data/resDelay_opt.txt", "w");
		fclose(filTmp);
	}

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;
	model.delay.nDelayed_months = 2;
	model.delay.delayed_monthNr = (int*)malloc(model.delay.nDelayed_months * sizeof(int));
	model.params.failedTime = 0;


	model.delay.delayed_monthNr[0] = manad;
	if (manad == 12)
		model.delay.delayed_monthNr[1] = 1;
	else
		model.delay.delayed_monthNr[1] = manad + 1;

	//testCallWeatherFile();

	char* namn;
	FILE* filPek3;
	namn = (char*)malloc2(256 * sizeof(char));
	if (node == 0) {
		FILE* filDelay = fopen("data/res_delay.txt", "w");
		fprintf(filDelay, "node\tgroup\tyear\tday\tdirection\tcost\ttime\tdistance\tnSlowSpeed\tnHighSpeed\tcost_diffKvot\ttime_diffKvot\t"
			"dist_diffKvot\txStart\tyStart\txEnd\tyEnd\tnPoints");
		for (int i = 0; i < 2; i++)
			fprintf(filDelay, "\tx_%d\ty_%d", i, i);
		fprintf(filDelay, "\tarcNr\n");
		fclose(filDelay);
		filDelay = fopen("data/res_delaySP.txt", "w");
		fprintf(filDelay, "nodeNr\tpoint_x1\tpoint_y1\tpoint_x2\tpoint_y2\tdirection\tcost\ttime\tdistance\tnSlowSpeed\tnHighSpeed\trowIndata\n");
		fclose(filDelay);

		sprintf(namn, "%s/res_delay.json", model.params.indataPath.c_str());
		FILE* filpekG = fopen(namn, "w");
		initGeoJsonFil(filpekG, "result_path");
		fclose(filpekG);

		if (skrivMycket == 1) {
			sprintf(namn, "%s/resDetailsDelayRoutes.txt", model.params.indataPath.c_str());
			filPek3 = fopen(namn, "w");
			fprintf(filPek3, "node\talt\tyearPos\tstartPos\tarcPos\tspeedSetting\tdistance\ttime\tfuelBase\temission\tsafetyBase\tchannelCost\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
				"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\t"
				"currentReal\tcurrentDirReal\twindReal\twindDirReal\t"
				"current\twindSpeed\trelWindDir\twaveHeight\trelWaveDir\twavePeriod\t"
				"speedDiffWind\tspeedDiffWave\tbaseGroundSpeed\twindF\twaveF\tcurrentF\tdelayF\n");
			fclose(filPek3);
		}


	}

	model.timeStart = std::chrono::high_resolution_clock::now();

	for (int index = 0; index < 20001; index++)
	{
		cos_table[index] = std::cos(M_PI * (index) / 10000.0);
		sin_table[index] = std::sin(M_PI * (index) / 10000.0);
		atan_table[index] = std::atan(M_PI * (index) / 10000.0);
	}
	LOOKUP_COS_STEP_INV = 10000.0 / M_PI;


	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityOptiNav(&(model.params));

	loadParams_new(&(model.params));
	if (SKRIV_UT_NOTHING == 0 || skrivMycket == 1) {
		skrivMycket = 1;
		SKRIV_UT_NOTHING = 0;
	}

	model.network.tidp_startHistoricDataOnly = 999999;


	// load the node to analyze and its neighbours
	// generate a bounding box around all neighbours
	// start with the neighbour furtherst away

	//loadNodeAndNeighbours_delayedFactors();

	model.delay.nYears = 5; // 6; // 2018 - 2023

	loadGroups_delayedFactors();

	if (node < 0 || node >= model.delay.nGroups) {
		printf("ERROR! Wrong node number %d, must be between 0 and %d. I quit\n", node,
			model.delay.nGroups - 1);
		exit(0);
	}

	loadAllNeededTablesFromSQLite();
	loadVariables();
	model.corridorPath.nLines = 0;

	model.network.nMaxSplits = 1;

	int i, i1, resultVal, i0;
	int evalExtraSol = 0;
	int nExtraOpt = 0; // genExtraOpts();
	int nod1, nod2, nSolSaved = 0, endTidsp, returnVal;
	char* baseName;
	baseName = (char*)malloc2(10 * sizeof(char));
	sprintf(baseName, "base");

	errlog("\n\n\n\n################################\nsolving for group %d %.3lf %.3lf month %d\n#################################\n\n",
		node, model.delay.group[node][0].point_x1, model.delay.group[node][0].point_y1, model.delay.delayed_monthNr[0]);
	printf("\n\n\n\n################################\nsolving for group %d %.3lf %.3lf month %d\n#################################\n\n",
		node, model.delay.group[node][0].point_x1, model.delay.group[node][0].point_y1, model.delay.delayed_monthNr[0]);
	printf("nNoder %d month %d\n", model.delay.nGroups, model.delay.delayed_monthNr[0]);
	model.weatherFunctions.nAllocPoints = 0;

	setUpBoundingBox_delay(node);

	model.BVArc = NULL;

	if (model.params.simuleraTidVisuellt == 1) {
		sprintf(namn, "%s/solVisuellt.geojson", model.params.indataPath.c_str());
		model.timeVisual.filVisuell = fopen(namn, "w");
		initGeoJsonFil(model.timeVisual.filVisuell, "sol");
		model.timeVisual.pos = 0;
		model.timeVisual.startTime = (char*)malloc(256 * sizeof(char));
		model.timeVisual.tmBas = { 0 };
	}

	errlog("ERROR! Change model.delay.nYears if years after %d are included\n", 2018 + model.delay.nYears - 1);
	if (model.params.delay_onlySolveSP == 1)
		model.delay.nYears = 1;
	//nMaxYear = 1;
	//errlog("ERROR! nMaxYear hard coded to 1\n");

	int nDaysMonths = 31;
	if (model.delay.delayed_monthNr[0] == 2) {
		nDaysMonths = 28;
	}
	if (model.delay.delayed_monthNr[0] == 4 || model.delay.delayed_monthNr[0] == 4 || model.delay.delayed_monthNr[0] == 6 ||
		model.delay.delayed_monthNr[0] == 9 || model.delay.delayed_monthNr[0] == 11)
		nDaysMonths = 30;

	for (int year = 0; year < model.delay.nYears; year++) {
		if (year == 3 && model.delay.delayed_monthNr[0] == 2)
			nDaysMonths = 29;
		//if (year != 3)
		//	continue;
		model.params.UTC_secondsStart = make_gmtime_fromGivenDate_delay(year, &(model.params));

		for (i = 0; i < model.delay.nGroupArcs[node]; i++) {
			//if (i != 12)
			//	continue;
		//for (i = 2; i < model.delay.nNodeNeighbours[node]; i++) { // !!!!!!!!!!!!!!!!!!!!!!!!!
		//	errlog("ERROR!!!!!!!!!!\n");
			checkMinnesAnvandning(__LINE__);
			createPrefPath_delay(node, i);
			printf("node %d year %d i %d boundingBox x %.3lf %.3lf y %.3lf %.3lf\n", node, year, i, model.boundingBox.xMin,
				model.boundingBox.xMax, model.boundingBox.yMin, model.boundingBox.yMax);
			errlog("node %d year %d i %d boundingBox x %.3lf %.3lf y %.3lf %.3lf\n", node, year, i, model.boundingBox.xMin,
				model.boundingBox.xMax, model.boundingBox.yMin, model.boundingBox.yMax);
			checkMinnesAnvandning(__LINE__);
			createPhysicalNetwork(0, 2);
			checkMinnesAnvandning(__LINE__);
			setupUsableSpeedSettings_delay();

			resultVal = solveOnlyShortestPathWithoutTime_delay(node, year, i);
			if (model.params.delay_onlySolveSP == 1)
				continue;

			if (resultVal == -1) {
				errlog("ERROR! No solution found for node %d pos %d xy %.3lf %.3lf %.3lf %.3lf\n", node, i,
					model.delay.group[node][i].point_x1, model.delay.group[node][i].point_y1,
					model.delay.group[node][i].point_x2, model.delay.group[node][i].point_y2);
				continue;
			}
			for (i1 = 0; i1 < nDaysMonths; i1++) {
				//if (i1 != 0)
				//	continue;
				//if (i != 1 || (i1 != 2 && i1 != 27))
				//	continue;
				//for (i1 = 0; i1 < 2; i1++) {
				//errlog("ERRROR2!!!!!!!!!!!!!!\n");
				errlog("-- node %d year %d i %d i1 %d\n", node, year, i, i1);
				if (skrivMycket == 1)
					printf("-- node %d year %d i %d i1 %d\n", node, year, i, i1);
				else
					printf("-- node %d year %d i %d i1 %d ", node, year, i, i1);
				if (i1 == 2)
					i1 = i1;
				returnVal = createTimeArcs_delay(year, i1, i);
				if (returnVal == -1) {
					errlog("ERROR! At least one weather file is missing for year %d. I skip this month\n", model.delay.year[year]);
					i = model.delay.nGroupArcs[node];
					break;
				}
				if (model.nArcs < model.network.nPhysicalLevels + 1) {
					printf("\n\n\n\n\n\n*******************************************************************\nNot path in network, skip this one\n");
					writeErrorMsgToFiles(node, year, i, i1);
					continue;
				}

				for (i0 = 0; i0 < model.nStorms; i0++)
					model.storms[i0].closestPointToRoute = 1e10;

				SattUppDijkstraNatverk3(&model);
				checkMinnesAnvandning(__LINE__);
				nod1 = 0;
				nod2 = model.nNoder - 1;
				bool Reached;

#ifdef _WIN32
				//sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
				//FILE* filpek = fopen(namn, "w");
				//fprintf(filpek, "arcNr\tsSplit\tnodNr1\tnod1UtPos\tnodNr2\tfromLevel\tfromPointNr\tfromTimeInterval\ttoLevel\ttoPointNr\ttoTimeInterval\ttotCost\t"
				//	"channelCost\tdistance\temission\tfuelBase\tsafetyBase\tspeedSetting\ttime\ttimeCheck\ttimeElapsed\tdiffTimeToMid\t"
				//	"accumDist\tlatLon\tbearing\tfuel_day\tworstStormValue\t"
				//	"currentReal\trelCurrent\tcalmWaterSpeed\tbaseGroundSpeed\tspeedOnGround\trpm\trelWindSpeed\trelWindDir\t"
				//	"deltaSpeedWind\twaveheight\twavePeriod\trelWaveDir\tdeltaSpeedWave\twindSpeedReal\twindDirReal\t"
				//	"currentReal\tcurrentDirReal\twaveDirReal\tbowSlamming_max\tgreenWater_max\tdynamiStability_max\ticeCover_max\tforecastType\n");
				//fclose(filpek);
#endif 
				std::string resAltName;

				//printf("\nsolving dijkstra's algorithm..");
				AnropDijkstra2(nod1, nod2, &model, &Reached);
				checkMinnesAnvandning(__LINE__);
				model.delay.nDiffTimeSol = 0;
				//printf("..done. Obj %I64d\n", model.Dijkstra.OptCost);
				filTmp = fopen("data/resDelay_opt.txt", "a+");
				if (Reached == true) {
					checkMinnesAnvandning(__LINE__);
					dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
					checkMinnesAnvandning(__LINE__);
					if (model.nBVArcs < 2) {
						endTidsp = 0;
						errlog("ERROR! Too few arcs %d in Dijkstra solution\n", model.nBVArcs);
						fprintf(filTmp, "delay ERROR2 node %d neighbourNr %d arcID %d yearPos %d i1 %d optCost %I64d "
							"nBagar %d fagelDist %.2lf nPhysLev %d nDiffTimeSol %d\n",
							node, i, model.delay.group[i][i1].arcNr, year, i1, model.Dijkstra.OptCost, model.nArcs,
							model.preferredPath.point[0].distanceTo(model.preferredPath.point[1]) / 1000.0, model.network.nPhysicalLevels,
							model.delay.nDiffTimeSol);
						fclose(filTmp);
						continue;
					}
					else
						endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime;
					//printf("solution to base\n");
					if (model.params.simuleraTidVisuellt == 1) {
						if (model.timeVisual.pos > 0)
							fprintf(model.timeVisual.filVisuell, ",\n");
						model.timeVisual.pos = 0;
					}
					checkMinnesAnvandning(__LINE__);
					writeSolutionToJson_delay(node, i, year, i1);
					checkMinnesAnvandning(__LINE__);

					fprintf(filTmp, "delay opt node %d neighbourNr %d arcID %d yearPos %d i1 %d optCost %I64d "
						"nBagar %d fagelDist %.2lf nPhysLev %d nDiffTimeSol %d\n",
						node, i, model.delay.group[node][i].arcNr, year, i1, model.Dijkstra.OptCost, model.nArcs,
						model.preferredPath.point[0].distanceTo(model.preferredPath.point[1]) / 1000.0, model.network.nPhysicalLevels,
						model.delay.nDiffTimeSol);
					fclose(filTmp);
				}
				else {
					errlog("ERROR! Did not manage to find a route from start to finish...\n");
					printf("\nERROR! Did not manage to find a route from start to finish...\n");
					fprintf(filTmp, "delay ERROR3 node %d neighbourNr %d arcID %d yearPos %d i1 %d optCost %I64d "
						"nBagar %d fagelDist %.2lf nPhysLev %d nDiffTimeSol %d\n",
						node, i, model.delay.group[node][i].arcNr, year, i1, model.Dijkstra.OptCost, model.nArcs,
						model.preferredPath.point[0].distanceTo(model.preferredPath.point[1]) / 1000.0, model.network.nPhysicalLevels,
						model.delay.nDiffTimeSol);
					fclose(filTmp);
				}

				checkMinnesAnvandning(__LINE__);
			}
		}
	}
	//callJsonTest();

	sprintf(namn, "%s/res_delay.json", model.params.indataPath.c_str());
	filTmp = fopen(namn, "a+");
	fprintf(filTmp, "]}\n");
	fclose(filTmp);

	if (model.params.simuleraTidVisuellt == 1) {
		fprintf(model.timeVisual.filVisuell, "]}\n");
		fclose(model.timeVisual.filVisuell);
		simuleraStormsVisuellt();
	}

	return 0;
}


int checkWeatherCoverOK(int xPos, int yPos)
{
	int i, i1, i0, latPos, lonPos, pos_latLon;
	double fy, fx, x, y, rowDbl, colDbl;

	for (i = 0; i < 4; i++) {
		if (i == 0) {
			fy = 0.9;
			fx = 0.9;
		}
		if (i == 1) {
			fy = 0.9;
			fx = 0.1;
		}
		if (i == 2) {
			fy = 0.1;
			fx = 0.9;
		}
		if (i == 3) {
			fy = 0.1;
			fx = 0.1;
		}
		y = fy * (yPos + model.boundingBox.yMin) + (1 - fy) * (yPos + 1 + model.boundingBox.yMin);
		x = fx * (xPos + model.boundingBox.xMin) + (1 - fx) * (xPos + 1 + model.boundingBox.xMin);

		for (i0 = 0; i0 < 4; i0++) {
			if (i0 == 0) i1 = model.functions.pos_wind_u;
			if (i0 == 1) i1 = model.functions.pos_wind_v;
			if (i0 == 2) i1 = model.functions.pos_waveHeight;
			if (i0 == 3) i1 = model.functions.pos_waveDirection;

			rowDbl = (model.weather[i1].maxY - y) / model.weather[i1].size_row;
			colDbl = get_colDblFromWeatherFile(i1, x);
			latPos = (int)rowDbl;
			if (latPos < 0) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! latPos %d\n", latPos);
					printf("\n\n\n\n\n\n\n############################################################\nERROR! latPos %d\n", latPos);
					model.nErrorCoordBB = 1;
				}
				latPos = 0;
			}
			if (latPos >= model.weather[i1].nRows) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! latPos too high %d (max %d) lat lon %.4lf %.4lf maxY %.4lf minX %.4lf nArcs %d\n",
						latPos,
						model.weather[i1].nRows - 1, y, x,
						model.weather[i1].maxY, model.weather[i1].minX, model.nArcs);
					model.nErrorCoordBB = 1;
				}
				latPos = model.weather[i1].nRows - 1;
			}
			lonPos = (int)colDbl;
			if (lonPos < 0) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! lonPos %d\n", lonPos);
					model.nErrorCoordBB = 1;
				}
				lonPos = 0;
			}
			if (lonPos >= model.weather[i1].nCols) {
				if (model.nErrorCoordBB == 0) {
					errlog("ERROR! lonPos too high %d (max %d)\n", lonPos,
						model.weather[i1].nCols - 1);
					model.nErrorCoordBB = 1;
				}
				lonPos = model.weather[i1].nCols - 1;
			}
			pos_latLon = latPos * model.weather[i1].nCols + lonPos;

			if (model.weather[i1].valueCell[0][pos_latLon] > 1000)
				return 0; // no value given
		}
	}
	return 1;
}

int checkTidpOK(int tidp)
{
	int i, i1, i0, lastPos;

	long long nSecondsUTC = model.params.UTC_secondsStart + (long long)(tidp * 3600);

	for (i0 = 0; i0 < 4; i0++) {
		if (i0 == 0) i1 = model.functions.pos_wind_u;
		if (i0 == 1) i1 = model.functions.pos_wind_v;
		if (i0 == 2) i1 = model.functions.pos_waveHeight;
		if (i0 == 3) i1 = model.functions.pos_waveDirection;

		lastPos = model.weather[i1].nTimeIntervals - 1;
		if (2 * model.weather[i1].secondsUTC[lastPos] - model.weather[i1].secondsUTC[lastPos - 1] <= nSecondsUTC)
			return 0;
	}
	return 1;
}


int generateDelayedFactors(std::string inputPath, int node, int manad)
{

	double dist;
	long long Cost;
	reset_errlog();
	errlog("delay for manad %d node %d\n", manad, node);
	FILE* filTmp;

	if (node == 0) {
		filTmp = fopen("data/resDelay_opt.txt", "w");
		fclose(filTmp);
	}

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;
	model.delay.nDelayed_months = 2;
	model.delay.delayed_monthNr = (int*)malloc(model.delay.nDelayed_months * sizeof(int));
	model.params.failedTime = 0;


	model.delay.delayed_monthNr[0] = manad;
	if (manad == 12)
		model.delay.delayed_monthNr[1] = 1;
	else
		model.delay.delayed_monthNr[1] = manad + 1;

	//testCallWeatherFile();

	char* namn;
	FILE* filPek3;
	namn = (char*)malloc2(256 * sizeof(char));
	if (node == 0) {
		FILE* filDelay = fopen("data/res_delay.txt", "w");
		fprintf(filDelay, "node\tgroup\tyear\tday\tdirection\tcost\ttime\tdistance\tnSlowSpeed\tnHighSpeed\tcost_diffKvot\ttime_diffKvot\t"
			"dist_diffKvot\txStart\tyStart\txEnd\tyEnd\tnPoints");
		for (int i = 0; i < 2; i++)
			fprintf(filDelay, "\tx_%d\ty_%d", i, i);
		fprintf(filDelay, "\tarcNr\n");
		fclose(filDelay);
		filDelay = fopen("data/res_delaySP.txt", "w");
		fprintf(filDelay, "nodeNr\tpoint_x1\tpoint_y1\tpoint_x2\tpoint_y2\tdirection\tcost\ttime\tdistance\tnSlowSpeed\tnHighSpeed\trowIndata\n");
		fclose(filDelay);

		sprintf(namn, "%s/res_delay.json", model.params.indataPath.c_str());
		FILE* filpekG = fopen(namn, "w");
		initGeoJsonFil(filpekG, "result_path");
		fclose(filpekG);

		if (skrivMycket == 1) {
			sprintf(namn, "%s/resDetailsDelayRoutes.txt", model.params.indataPath.c_str());
			filPek3 = fopen(namn, "w");
			fprintf(filPek3, "node\talt\tyearPos\tstartPos\tarcPos\tspeedSetting\tdistance\ttime\tfuelBase\temission\tsafetyBase\tchannelCost\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
				"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\t"
				"currentReal\tcurrentDirReal\twindReal\twindDirReal\t"
				"current\twindSpeed\trelWindDir\twaveHeight\trelWaveDir\twavePeriod\t"
				"speedDiffWind\tspeedDiffWave\tbaseGroundSpeed\twindF\twaveF\tcurrentF\tdelayF\n");
			fclose(filPek3);
		}


	}

	model.timeStart = std::chrono::high_resolution_clock::now();

	for (int index = 0; index < 20001; index++)
	{
		cos_table[index] = std::cos(M_PI * (index) / 10000.0);
		sin_table[index] = std::sin(M_PI * (index) / 10000.0);
		atan_table[index] = std::atan(M_PI * (index) / 10000.0);
	}
	LOOKUP_COS_STEP_INV = 10000.0 / M_PI;


	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityOptiNav(&(model.params));

	loadParams_new(&(model.params));
	if (SKRIV_UT_NOTHING == 0 || skrivMycket == 1) {
		skrivMycket = 1;
		SKRIV_UT_NOTHING = 0;
	}

	model.network.tidp_startHistoricDataOnly = 999999;


	// load the node to analyze and its neighbours
	// generate a bounding box around all neighbours
	// start with the neighbour furtherst away
	
	//loadNodeAndNeighbours_delayedFactors();

	model.delay.nYears = 5; // 6; // 2018 - 2023

	//setUpAreaToCalculateFor_delayedFactors();
	setUpBoundingBox_delayNew(node);

	if (node < 0 || (node >= 4 && node != 10000)) {
		printf("ERROR! Wrong node number %d, must be between 0 and %d. I quit\n", node,
			3);
		exit(0);
	}

	loadAllNeededTablesFromSQLite();
	loadVariables();
	model.corridorPath.nLines = 0;

	model.network.nMaxSplits = 1;

	int i, i1, resultVal, i0;
	int evalExtraSol = 0;
	int nExtraOpt = 0; // genExtraOpts();
	int nod1, nod2, nSolSaved = 0, endTidsp, returnVal;
	double maxAllowedFactor = 2.0;
	char* baseName;
	baseName = (char*)malloc2(10 * sizeof(char));
	sprintf(baseName, "base");

	errlog("\n\n\n\n################################\nsolving for node %d month %d\n#################################\n\n",
		node, model.delay.delayed_monthNr[0]);
	model.weatherFunctions.nAllocPoints = 0;

	model.BVArc = NULL;

	if (model.params.simuleraTidVisuellt == 1) {
		sprintf(namn, "%s/solVisuellt.geojson", model.params.indataPath.c_str());
		model.timeVisual.filVisuell = fopen(namn, "w");
		initGeoJsonFil(model.timeVisual.filVisuell, "sol");
		model.timeVisual.pos = 0;
		model.timeVisual.startTime = (char*)malloc(256 * sizeof(char));
		model.timeVisual.tmBas = { 0 };
	}

	errlog("ERROR! Change model.delay.nYears if years after %d are included\n", 2018 + model.delay.nYears - 1);
	if (model.params.delay_onlySolveSP == 1)
		model.delay.nYears = 1;
	//nMaxYear = 1;
	//errlog("ERROR! nMaxYear hard coded to 1\n");

	int nDaysMonths = 31;
	if (model.delay.delayed_monthNr[0] == 2)
		nDaysMonths = 28;
	if (model.delay.delayed_monthNr[0] == 4 || model.delay.delayed_monthNr[0] == 4 || model.delay.delayed_monthNr[0] == 6 ||
		model.delay.delayed_monthNr[0] == 9 || model.delay.delayed_monthNr[0] == 11)
		nDaysMonths = 30;

	setupUsableSpeedSettings_delay();


	spherical::Point p1, p2;

	double calmWaterSpeed = model.params.preferredSpeed_calmWater;
	int direction, x, y, grader, timep, okCover, nFeasible, nVal, okTidp;
	double x1, x2, y1, y2, tid, tidBas, factor;
	double sumVal[8], minVal[8], maxVal[8], coord[2][8], distArr[8];
	FILE* filpek, *filpek2;
	
	double* factor_posXYr;
	int* antal_posXYr, pos_XYr;

	factor_posXYr = (double*)calloc(8 * model.delay.nXinterval * model.delay.nXinterval, sizeof(double));
	antal_posXYr = (int*)calloc(8 * model.delay.nXinterval * model.delay.nXinterval, sizeof(int));


	if (node == 0 || node == 10000) {
		filpek = fopen("data\\res_delayNew.txt", "w");
		fprintf(filpek, "node\tyear\tday\tdirection\tdistance\ttime_factor\tmin_factor\tmax_factor\txMid\tyMid\n");
	}
	else {
		filpek = fopen("data\\res_delayNew.txt", "a+");
	}

	//for (int year = 0; year < 2; year++) {
	for (int year = 0; year < model.delay.nYears; year++) {
			//if (year != 3)
		//	continue;
		model.params.UTC_secondsStart = make_gmtime_fromGivenDate_delay(year, &(model.params));

		openNeededWeatherFiles(year);

		printf("node %d year %d boundingBox x %.3lf %.3lf y %.3lf %.3lf\n", node, year, model.boundingBox.xMin,
			model.boundingBox.xMax, model.boundingBox.yMin, model.boundingBox.yMax);
		//for (x = 142; x < 146; x++) {
		for (x = 0; x < model.delay.nXinterval; x++) {
			nFeasible = 0;
			//for (y = 63; y < 65; y++) {
			for (y = 0; y < model.delay.nYinterval; y++) {
				checkMinnesAnvandning(__LINE__);

				okCover = checkWeatherCoverOK(x, y);
				if (okCover == 0)
					continue; // not enought weather cover

				for (direction = 0; direction < 8; direction++) {
					minVal[direction] = 100;
					maxVal[direction] = 0;
					sumVal[direction] = 0;
					nVal = 0;
				}
				for (i1 = 0; i1 < nDaysMonths; i1++) {
					timep = i1 - 8 * (int)(i1 / 8) + i1 * 24;
					okTidp = checkTidpOK(timep);
					if (okTidp == 0)
						break; // this day is past the last date

					if (i1 == 2)
						i1 = i1;

					for (direction = 0; direction < 8; direction++) {
						if (direction < 2 || direction == 7) {
							x1 = x + model.boundingBox.xMin;
							x2 = x + model.boundingBox.xMin + 1;
						}
						if (direction == 2 || direction == 6) {
							x1 = x + model.boundingBox.xMin + 0.5;
							x2 = x1;
						}
						if (direction >= 3 && direction <= 5) {
							x1 = x + model.boundingBox.xMin + 1;
							x2 = x + model.boundingBox.xMin;
						}
						if (direction >= 1 && direction <= 3) {
							y1 = y + model.boundingBox.yMin;
							y2 = y + model.boundingBox.yMin + 1;
						}
						if (direction == 0 || direction == 4) {
							y1 = y + model.boundingBox.yMin + 0.5;
							y2 = y1;
						}
						if (direction >= 5) {
							y1 = y + model.boundingBox.yMin + 1;
							y2 = y + model.boundingBox.yMin;
						}
						grader = getDirection(y1, x1, y2, x2);
						p1 = spherical::Point(y1, x1);
						p2 = spherical::Point(y2, x2);
						for (i0 = 0; i0 < model.nStorms; i0++)
							model.storms[i0].closestPointToRoute = 1e10;
						pos_XYr = direction + 8 * (x + model.delay.nXinterval * y);
						if (pos_XYr == 93296)
							i1 = i1;
						calcWeatherPosAlongArc(p1, p2, timep);
						tid = calcArcTimeCost(timep, -1, 0, 1, -1, &calmWaterSpeed);
						tidBas = model.functions.valuesNow.distance / calmWaterSpeed;
						factor = tid / tidBas;
						if (factor > maxAllowedFactor)
							factor = maxAllowedFactor;
						if (minVal[direction] > factor)
							minVal[direction] = factor;
						if (maxVal[direction] < factor)
							maxVal[direction] = factor;
						sumVal[direction] += factor;
						factor_posXYr[pos_XYr] += factor;
						(antal_posXYr[pos_XYr])++;
						if (i1 == 0) {
							distArr[direction] = model.functions.valuesNow.distance;
							coord[0][direction] = (x1 + x2) / 2;
							coord[1][direction] = (y1 + y2) / 2;
						}
					}
					nVal++;
				}
				for (direction = 0; direction < 8; direction++) {
					fprintf(filpek, "%d\t%d\t%d\t%d\t%.3lf\t%.4lf\t%.4lf\t%.4lf\t%.3lf\t%.3lf\n", node, year, i1, direction * 45, distArr[direction],
						sumVal[direction] / nVal, minVal[direction], maxVal[direction], coord[0][direction], coord[1][direction]);
					if (abs(coord[0][direction] + 37.5) < 0.1 && abs(coord[0][direction] - 64.5) < 0.1)
						direction = direction;
				}
				checkMinnesAnvandning(__LINE__);
				nFeasible++;
			}
			printf("x %d nFeasible %d av %d\n", x, nFeasible, model.delay.nYinterval);
		}
	}
	fclose(filpek);

	for (i = 0; i < 8; i++) {
		sprintf(namn, "data\\res_nodeDir_%d_%d.txt", i * 45, manad);
		if (node == 0 || node == 10000) {
			filpek2 = fopen(namn, "w");
			fprintf(filpek2, "node\tdirection\ttime_factor\tnValues\txMid\tyMid\n");
		}
		else {
			filpek2 = fopen(namn, "a+");
		}
		for (x = 0; x < model.delay.nXinterval; x++) {
			for (y = 0; y < model.delay.nYinterval; y++) {
				pos_XYr = i + 8 * (x + model.delay.nXinterval * y);
				if (antal_posXYr[pos_XYr] > 0)
					fprintf(filpek2, "%d\t%d\t%.4lf\t%d\t%.3lf\t%.3lf\n",
						node, i * 45, factor_posXYr[pos_XYr] / antal_posXYr[pos_XYr], antal_posXYr[pos_XYr],
						x + model.boundingBox.xMin + 0.5, y + model.boundingBox.yMin + 0.5);
				if (abs(x + model.boundingBox.xMin + 0.5 + 37.5) < 0.1 && abs(y + model.boundingBox.yMin + 0.5 - 64.5) < 0.1)
					direction = direction;
			}
		}
		fclose(filpek2);
	}

	//callJsonTest();


	if (model.params.simuleraTidVisuellt == 1) {
		fprintf(model.timeVisual.filVisuell, "]}\n");
		fclose(model.timeVisual.filVisuell);
		simuleraStormsVisuellt();
	}

	return 0;
}
