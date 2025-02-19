//#define ONBOARD
//#define NAZANIN_SAFETY
//#define KAOUTAR
int USE_KVOTKOST = 1;

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
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

#endif

FILE* filSaveSpec;

int USE_ARC_TIME_EXACT = 1; // 0 if as good speed as possible from forecast to be used, 1 if the arc speed is used (discretization losses...)
int DEF_nMAX_SPLITS = 100; // default is 100

extern double cos_table[20001];
extern double sin_table[20001];
extern double atan_table[20001];
extern double LOOKUP_COS_STEP_INV;

extern strModel model;
extern strModel modelDelay;
extern strModel modelDelay_prefPath;
extern int delayVersion;
extern int globalCount2;
extern int SKRIV_UT_NOTHING;
extern int runAltForecast;
extern int globalCount1;
extern int globalCount2;
extern long long MAXVARDE_NATVERK;
extern int SEND_POST_REQUEST;
extern std::string resultPath;
extern int SPARA_RUN_DATA;

void* malloc2(size_t size) {
	return malloc(size);
}

void* calloc2(size_t count, size_t size) {
	return calloc(count, size);
}

int freeAllNodData() {
	int i;

	for (i = 0; i < model.nNoder; i++) {
		free(model.Noder[i].UtNod);
		model.Noder[i].UtNod = NULL;
		free(model.Noder[i].UtNodCost);
		free(model.Noder[i].outArcNr);
	}
	return 0;
}

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
#ifdef _WIN32
	varde = _CrtCheckMemory();
#endif
	if (varde != 1)
		errlog("Error! Minnesbugg identifierad pa rad %d\n", rad);
	return varde;
}

double calcArcTimeCost(int tidInt, int speedSettingNr, int fromLevel, int toLevel, double* calmWaterSpeed, double fuelFactorMain) {
	//, double* fuel, double* safety, double* distance, double* worstStormValue, double* worstStabilityValue)

	double tidStart = tidInt * model.params.tIndexGerH;
	double tidTot = tidStart, distNu, fuelTot_main = 0, fuelTot_aux = 0, safetyTot = 0, tmp1;
	double uWind, vWind, uCurrent, vCurrent;// , uVessel, vVessel; //  , uSpeed, vSpeed;
	double dist = 0, deltaTid;
	double windDirection, windSpeed;
	double currentDirection, currentSpeed, rel_windSpeed;
	int i, ii, badSpeed = 0, tidIntForecast;
	double waveHeight, wavePeriod, stormVarde, windSpeed2, waveDirection;
	double baseGroundSpeed, rel_windDir, rel_waveDir, speedDiffWindWave;
	double speedOverGround, timeArc, fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux, iceCover, safetyArc;
	double speedDiffWind, speedDiffWave, absWindDirDiff = 0;

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

	//model.tmpTid4[0] = std::chrono::high_resolution_clock::now();
	model.functions.valuesNow.worstStormValue = 0;
	//model.functions.valuesNow.worstStabilityValue = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0; // a / b
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;

	//if (model.nArcs == 42)
	//	model.nArcs = model.nArcs;

	//if (model.nArcs == 175518)
	//	printGlobal = 0;
	if (*calmWaterSpeed < 0)
		*calmWaterSpeed = eval_calmWaterSpeed(speedSettingNr, fromLevel, toLevel);

	if (tidTot >= 7 * 2400) {//  && model.weatherFunctions.nCheckPoints > 1){
		//double totDist = 0;
		for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
			getAllVariableValues(i, tidTot);
			if (i == 0) {
				for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
					model.functions.varValueAverage[i1] = model.functions.varValue[i1];
			}
			else {
				if (i < model.weatherFunctions.nCheckPoints - 1) {
					for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
						model.functions.varValueAverage[i1] += model.functions.varValue[i1];
				}
				else {
					for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
						model.functions.varValue[i1] = (model.functions.varValueAverage[i1] + model.functions.varValue[i1]) * model.inv_nWeatherFiles;
				}
			}
			//totDist += model.weatherFunctions.checkPoint[i].distToNextPkt;
		}
		//model.weatherFunctions.nCheckPoints = 1;
		//model.weatherFunctions.checkPoint[0].distToNextPkt = totDist;
	}

	ii = 1;
	deltaTid = 0;

	if (fromLevel == 3 && toLevel == -2)
		ii = ii;

	for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
		model.tmpTid2[5] = std::chrono::high_resolution_clock::now();

		if (printGlobal == 1)
			printf("i %d innan stormVal tidTot %.2lf\n", i, tidTot);
		stormVarde = getStormValue(tidTot + deltaTid, model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i]);// model.weatherFunctions.point[i]);
		tidIntForecast = getTidIntForecast(tidTot);


		if (stormVarde > model.functions.valuesNow.worstStormValue)
			model.functions.valuesNow.worstStormValue = stormVarde;

		if (tidTot < 0)
			printf("ERROR! Negative time %.3lf i %d ii %d\n", tidTot, i, ii);

		model.tmpTid2[6] = std::chrono::high_resolution_clock::now();
		model.durationCalcArcTimeStorm += model.tmpTid2[6] - model.tmpTid2[5];
		getAllVariableValues(i, tidTot);
		if (model.weatherFunctions.nCheckPoints > 1) {
			if (tidStart >= 3 * 2400) {
				if (ii == 1) {
					if (i != model.weatherFunctions.nCheckPoints - 1) {
						for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
							model.functions.varValueAverage[i1] = model.functions.varValue[i1];
						//if (model.weatherFunctions.modifiedPoints == 0)
						distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
						deltaTid += distNu / (*calmWaterSpeed);
						//printf("distNu %.3lf\n", distNu);
						if (printGlobal == 1)
							printf("distNu %.3lf fran forsta checkpoint\n", distNu);
						ii++;
						continue;
					}
				}
				else {
					distNu += model.weatherFunctions.checkPoint[i].distToNextPkt;
					deltaTid += model.weatherFunctions.checkPoint[i].distToNextPkt / (*calmWaterSpeed);
					if (ii == 2) {
						if (i != model.weatherFunctions.nCheckPoints - 2) {
							for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
								model.functions.varValue[i1] = (model.functions.varValueAverage[i1] + model.functions.varValue[i1]) * 0.5;
							ii = 1;
							//printf("adding %.3lf to %.3lf\n", distNu, model.weatherFunctions.checkPoint[i].distToNextPkt);
							//if(model.weatherFunctions.modifiedPoints == 0)
							//	model.weatherFunctions.checkPoint[i].distToNextPkt += distNu;
							if (printGlobal == 1)
								printf("adderade pa distNu %.3lf till distToNextPkt pos %d ii %d nu ar den %.3lf\n", distNu, i, ii,
									model.weatherFunctions.checkPoint[i].distToNextPkt);
						}
						else {
							for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
								model.functions.varValueAverage[i1] += model.functions.varValue[i1];
							ii++;
							//if (model.weatherFunctions.modifiedPoints == 0)
							if (printGlobal == 1)
								printf("adderade more to distNu %.3lf pos %d ii %d\n", distNu, i, ii);
							continue;
							//printf("adding more so distNu %.3lf\n", distNu);
						}
					}
					else {
						for (int i1 = 0; i1 < model.nWeatherFiles; i1++)
							model.functions.varValue[i1] = (model.functions.varValueAverage[i1] + model.functions.varValue[i1]) * 0.33333;
						//if (model.weatherFunctions.modifiedPoints == 0)
						//	model.weatherFunctions.checkPoint[i].distToNextPkt += distNu;
						if (printGlobal == 1)
							printf("adderade2 pa distNu %.3lf till distToNextPkt pos %d ii %d nu ar den %.3lf\n", distNu, i, ii,
								model.weatherFunctions.checkPoint[i].distToNextPkt);
						//printf("adding2 %.3lf to %.3lf\n", distNu, model.weatherFunctions.checkPoint[i].distToNextPkt);
					}
				}
				deltaTid = 0;
			}
			else
				distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
		}
		else
			distNu = model.weatherFunctions.checkPoints_totDist;
		if (printGlobal == 1)
			printf("distToNextPkt efter if loop ii %d nu ar den %.3lf tidInt %d\n\n", ii, distNu, tidInt);
		dist += distNu;

		uCurrent = model.functions.varValue[model.functions.pos_current_u]; // getVariableValue(model.functions.pos_current_u, i, tidTot);
		vCurrent = model.functions.varValue[model.functions.pos_current_v]; // getVariableValue(model.functions.pos_current_v, i, tidTot);

		if (uCurrent < 1000 && vCurrent < 1000) {
			currentDirection = ApproxAtan2(vCurrent, uCurrent);
			currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
			//if (tidTot >= model.weather[model.functions.pos_current_u].tidpHistoricalWeather)
			//	currentSpeed *= model.params.historicDataFactor_current;
		}
		else {
			currentDirection = 0;
			currentSpeed = 0;
		}

		if (uCurrent > 100000 ||
			(model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u] == 9000 &&
				model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u] == 10 &&
				(int)tidTot * model.weather_inv_timeIntervall_h == 3))
			printf("currentSpeed %.2lf uCurr %.2lf vCurr %.2lf uCurrPos %d lonPos %d latPos %d "
				"tidInt %d lon/lat %.2lf %.2lf speedSetting %d i %d av %d\n",
				currentSpeed, uCurrent, vCurrent,
				model.functions.pos_current_u,
				model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u],
				model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u],
				(int)tidTot * model.weather_inv_timeIntervall_h,
				-model.weather[model.functions.pos_current_u].minX +
				model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u] *
				model.weather[model.functions.pos_current_u].size_col,
				model.weather[model.functions.pos_current_u].maxY -
				model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u] *
				model.weather[model.functions.pos_current_u].size_row,
				speedSettingNr, i, model.weatherFunctions.nCheckPoints);

		//model.tmpTid2[5] = std::chrono::high_resolution_clock::now();
		//model.durationCalcArcTimeCurrent += model.tmpTid2[5] - model.tmpTid2[6];
		baseGroundSpeed = eval_baseGroundSpeed((*calmWaterSpeed), model.weatherFunctions.vesselBearing[i],
			currentDirection, currentSpeed);

		if (printGlobal == 1) {
			printf("checkP %d vCurrent %.3lf uCurrent %.3lf, currentDirection %.3lf currentSpeed %.3lf baseGroundSpeed %.3lf\n", i, vCurrent,
				uCurrent, currentDirection, currentSpeed, baseGroundSpeed);
		}

		uWind = model.functions.varValue[model.functions.pos_wind_u]; // getVariableValue(model.functions.pos_wind_u, i, tidTot);
		vWind = model.functions.varValue[model.functions.pos_wind_v]; // getVariableValue(model.functions.pos_wind_v, i, tidTot);

		if (uWind < 1000 && vWind < 1000) {
			windDirection = ApproxAtan2(vWind, uWind);
			windSpeed2 = uWind * uWind + vWind * vWind;
			windSpeed = sqrt(windSpeed2);

			if (windSpeed > model.functions.valuesNow.maxWindSpeed &&
				model.weather[model.functions.pos_wind_u].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_wind_u].nTimeIntervals_forecast)
				model.functions.valuesNow.maxWindSpeed = windSpeed;

			//if (windSpeed > maxWindSpeed)
			//	maxWindSpeed = windSpeed;

			//if (tidTot >= model.weather[model.functions.pos_wind_u].tidpHistoricalWeather)
			//	windSpeed *= model.params.historicDataFactor_windSpeed;

			rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
				windDirection, windSpeed, &rel_windDir);
#ifdef NAZANIN_SAFETY
			absWindDirDiff = eval_absWindDirDiff(model.weatherFunctions.vesselBearing[i], windDirection);
#endif
		}
		else {
			windSpeed = 0;
			windDirection = 0;
			rel_windDir = 0;
			rel_windSpeed = baseGroundSpeed;
		}

		waveHeight = model.functions.varValue[model.functions.pos_waveHeight]; // getVariableValue(model.functions.pos_waveHeight, i, tidTot);
		if (waveHeight > 100)
			waveHeight = 0;
		if (waveHeight > model.functions.valuesNow.maxWaveHeight &&
			model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
			model.functions.valuesNow.maxWaveHeight = waveHeight;
			if (waveHeight > 9)
				waveHeight = waveHeight;
		}


		//else {
		//	if (tidTot >= model.weather[model.functions.pos_waveHeight].tidpHistoricalWeather)
		//		waveHeight *= model.params.historicDataFactor_waveHeight;
		//}
		wavePeriod = model.functions.varValue[model.functions.pos_wavePeriod]; // getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
		if (wavePeriod > 1000)
			wavePeriod = 10;
		waveDirection = model.functions.varValue[model.functions.pos_waveDirection]; // getVariableValue(model.functions.pos_waveDirection, i, tidTot);
		if (waveDirection > 1000)
			waveDirection = 0;
		//rel_waveDir = (waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]; // / model.functions.nWaveDir;
		//rel_waveDir = M_PI - ((waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;
		rel_waveDir = M_PI + ((270 - waveDirection) * M_PI / 180 - model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;

		if (rel_waveDir < 0)
			rel_waveDir = -rel_waveDir;
		if (rel_waveDir >= 2 * M_PI)
			rel_waveDir -= 2 * M_PI;
		if (rel_waveDir > M_PI)
			rel_waveDir = 2 * M_PI - rel_waveDir;
		if (printGlobal == 1) {
			printf("checkP %d waves height %.2lf period %.2lf Direction %.3lf rel_waveDir %.3lf\n", i,
				waveHeight, wavePeriod, waveDirection, rel_waveDir);
		}

		speedDiffWind = lookup_speedDiffWindTable(baseGroundSpeed, rel_windSpeed, rel_windDir);
		speedDiffWave = lookup_speedDiffWaveTable((*calmWaterSpeed), waveHeight, wavePeriod, rel_waveDir);
		speedDiffWindWave = speedDiffWind + speedDiffWave;

		if (model.delay.nYears > 0 && delayVersion >= 4)
			speedOverGround = (*calmWaterSpeed) - speedDiffWindWave; // in km/h
		else
			speedOverGround = baseGroundSpeed - speedDiffWindWave; // in km/h

		if (speedOverGround < 0.01) {
			speedOverGround = 0.1;
			if (distNu >= 0.001)
				badSpeed = 1;
		}
		timeArc = distNu / speedOverGround; // in hours

		//if (model.nArcs == 175518)
		//	printf("arcNr1a %d kvots %.4lf %.4lf wPoint %d time %.3lf baseTime %lf timeArc %.3lf SOG %lf BGS %lf CWS %lf sDiffWW %lf bearing %lf currDir %lf currSped %lf uv %lf %lf\n", 
		//		model.nArcs, 0.0, 1.0,
		//		i, tidTot, tidTot, timeArc, speedOverGround, baseGroundSpeed, (*calmWaterSpeed), speedDiffWindWave,
		//		model.weatherFunctions.vesselBearing[i], currentDirection, currentSpeed, uCurrent, vCurrent);

		//if (model.nArcs == 36039) {
		//	fprintf(filSaveSpec, "nArcs %d i %d speedSettingNr %d calmWaterSpeed %.3lf bearing %.3lf, currDir %.3lf currSpeed %.3lf baseGroundSpeed %.3lf"
		//		" rel_windSpeed %.3lf rel_windDir %.3lf speedDiffWind %.3lf waveHeight %.3lf"
		//		" wavePeriod %.3lf rel_waveDir %.3lf speedDiffWave %.3lf speedOverGround %.3lf distNu %.3lf timeArc %.3lf\n",
		//		model.nArcs, i, speedSettingNr, *calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
		//		currentDirection, currentSpeed, baseGroundSpeed, rel_windSpeed, rel_windDir, speedDiffWind, waveHeight, wavePeriod,
		//		rel_waveDir, speedDiffWave, speedOverGround, distNu, timeArc);
		//}

		//if (model.nArcs == 36039 && i == 0) {
		//	//fprintf(filSaveSpec, "rel_windSpeed %.3lf\n", rel_windSpeed);
		//	printf("rel_windSpeed %.3lf\n", rel_windSpeed);
		//}


		//if (fuelFactorMain < 0)
		fuelConsumption_main = eval_fuelConsumption_both(speedSettingNr, &fuelConsumption_aux, fromLevel, toLevel);
		//else
		//	fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_base, &fuelConsumption_aux, -1, -100) * fuelFactorMain;

		fuelUsage_main = fuelConsumption_main * timeArc;
		fuelUsage_aux = fuelConsumption_aux * timeArc;
		if (fromLevel < 0 && toLevel < 0) {
			fuelUsage_main += model.network.channel[-fromLevel - 1].waiting_consumption_main;
			fuelUsage_aux -= fuelConsumption_aux * model.network.channel[-fromLevel - 1].waitingTime;
			fuelUsage_aux += model.network.channel[-fromLevel - 1].waiting_consumption_aux;
			timeArc += model.network.channel[-fromLevel - 1].waitingTime;
		}


		if (printGlobal == 1) {
			printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf fuelConsMain/aux %.3lf %.3lf\n",
				speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu, fuelConsumption_main, fuelConsumption_aux);
		}

		tidTot += timeArc;

		fuelTot_main += fuelUsage_main;
		fuelTot_aux += fuelUsage_aux;

		iceCover = model.functions.varValue[model.functions.pos_iceThickness]; // getVariableValue(model.functions.pos_iceThickness, i, tidTot);
		if (iceCover > 1000)
			iceCover = 0;

#ifdef NAZANIN_SAFETY
		eval_safety_nazanin(speedOverGround, (*calmWaterSpeed) - speedDiffWindWave, windSpeed, absWindDirDiff, waveHeight, wavePeriod, rel_waveDir, iceCover);
#else
		eval_safety(iceCover);
#endif
		if (speedDiffWind > 98 || speedDiffWave > 98)
			tidTot = 1e7;
	}
	model.functions.valuesNow.distance = dist;
	model.functions.valuesNow.fuel_aux = fuelTot_aux;
	model.functions.valuesNow.fuel_main = fuelTot_main;

	if (badSpeed == 1)
		tidTot = 1e7;
	return tidTot - tidStart;
}

double calcDelayedArcTimeCost(int fromLevel, int toLevel, int speedSettingNr, double calmWaterSpeed, double fuelFactorMain, double factorDelay, double dist, double speedDiff) {

	double timeArc, fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux;

	model.functions.valuesNow.worstStormValue = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0; // a / b
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;
	if (calmWaterSpeed < 0)
		calmWaterSpeed = eval_calmWaterSpeed(speedSettingNr, fromLevel, toLevel);

	double speedNu = calmWaterSpeed / factorDelay + speedDiff;
	if (speedNu < 0.1)
		speedNu = 0.1;
	timeArc = dist / speedNu; // in hours
	if (fuelFactorMain < 0)
		fuelConsumption_main = eval_fuelConsumption_both(speedSettingNr, &fuelConsumption_aux, fromLevel, toLevel);
	else
		fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_use, &fuelConsumption_aux, -1, -100) * fuelFactorMain;
	fuelUsage_main = fuelConsumption_main * timeArc;
	fuelUsage_aux = fuelConsumption_aux * timeArc;
	if (fromLevel < 0 && toLevel < 0) {
		fuelUsage_main += model.network.channel[-fromLevel - 1].waiting_consumption_main;
		//fuelUsage_aux -= fuelConsumption_aux * model.network.channel[-fromLevel - 1].waitingTime;
		fuelUsage_aux += model.network.channel[-fromLevel - 1].waiting_consumption_aux;
		timeArc += model.network.channel[-fromLevel - 1].waitingTime;
	}


	model.functions.valuesNow.distance = dist;
	model.functions.valuesNow.fuel_aux = fuelUsage_aux;
	model.functions.valuesNow.fuel_main = fuelUsage_main;

	return timeArc;
}

double calcArcTimeCostChannel(int t, int speedSettingNr, int channelNr, double* calmWaterSpeed) {
	//, double* fuel, double* safety, double* distance, double* worstStormValue, double* worstStabilityValue)

	double tidStart = t * model.params.tIndexGerH, tidTot = tidStart, distNu, fuelTot_main = 0, fuelTot_aux = 0, safetyTot = 0;
	double uWind, vWind, uCurrent, vCurrent;// , uVessel, vVessel; //  , uSpeed, vSpeed;
	double dist = 0;
	double windDirection, windSpeed;
	double currentDirection, currentSpeed, rel_windSpeed;
	int i, badSpeed = 0;
	double waveHeight, wavePeriod, stormVarde, windSpeed2, waveDirection;
	double baseGroundSpeed, rel_windDir, rel_waveDir, speedDiffWindWave;
	double speedOverGround, timeArc, fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux, iceCover, safetyArc;
	double speedDiffWind, speedDiffWave, fixTime, absWindDirDiff = 0;

	model.tmpTid4[0] = std::chrono::high_resolution_clock::now();
	model.functions.valuesNow.worstStormValue = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0; // a / b
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;

	if (model.network.channel[channelNr].intArrivalTime_h >= 0)
		tidTot = delayTimeToStartTimeDay(tidStart, model.network.channel[channelNr].intArrivalTime_h);
	tidTot += model.network.channel[channelNr].waitingTime; // .intWaitingTime;
	fixTime = model.network.channel[channelNr].timeThroughChannel;
	if (fixTime > 0) {
		*calmWaterSpeed = model.network.channel[channelNr].distance_km / fixTime;
	}
	else
		*calmWaterSpeed = eval_calmWaterSpeed(speedSettingNr, -channelNr - 1, -channelNr - 1);

	//if(model.nArcs == 6)
	//	printf(" nArcs %d calmWaterSpeed %.4lf fixTime %.4lf tidTot %.4lf nCheckp %d", 
	//		model.nArcs, *calmWaterSpeed, fixTime, tidTot, model.weatherFunctions.nCheckPoints);

	if (printGlobal == 1) {
		printf("speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", speedSettingNr, *calmWaterSpeed,
			model.weatherFunctions.nCheckPoints);
	}
	model.tmpTid2[5] = std::chrono::high_resolution_clock::now();
	model.durationCalcArcTimeCalmWater += model.tmpTid2[5] - model.tmpTid4[0];

	for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
		model.tmpTid2[5] = std::chrono::high_resolution_clock::now();
		distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
		dist += distNu;

		stormVarde = getStormValue(tidTot, model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i]);// model.weatherFunctions.point[i]);
		if (stormVarde > model.functions.valuesNow.worstStormValue)
			model.functions.valuesNow.worstStormValue = stormVarde;

		model.tmpTid2[6] = std::chrono::high_resolution_clock::now();
		model.durationCalcArcTimeStorm += model.tmpTid2[6] - model.tmpTid2[5];

		if (fixTime <= 0) {
			getAllVariableValues(i, tidTot);

			uCurrent = model.functions.varValue[model.functions.pos_current_u]; // getVariableValue(model.functions.pos_current_u, i, tidTot);
			vCurrent = model.functions.varValue[model.functions.pos_current_v]; // getVariableValue(model.functions.pos_current_v, i, tidTot);
			if (uCurrent < 1000 && vCurrent < 1000) {
				currentDirection = ApproxAtan2(vCurrent, uCurrent);
				currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
				//if (tidTot >= model.weather[model.functions.pos_current_u].tidpHistoricalWeather)
				//	currentSpeed *= model.params.historicDataFactor_current;
			}
			else {
				currentDirection = 0;
				currentSpeed = 0;
			}

			if (uCurrent > 100000 ||
				(model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u] == 9000 &&
					model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u] == 10 &&
					(int)tidTot * model.weather_inv_timeIntervall_h == 3))
				printf("currentSpeed %.2lf uCurr %.2lf vCurr %.2lf uCurrPos %d lonPos %d latPos %d "
					"tidInt %d lon/lat %.2lf %.2lf speedSetting %d i %d av %d\n",
					currentSpeed, uCurrent, vCurrent,
					model.functions.pos_current_u,
					model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u],
					model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u],
					(int)tidTot * model.weather_inv_timeIntervall_h,
					-model.weather[model.functions.pos_current_u].minX +
					model.weatherFunctions.checkPoint[i].lonPos[model.functions.pos_current_u] *
					model.weather[model.functions.pos_current_u].size_col,
					model.weather[model.functions.pos_current_u].maxY -
					model.weatherFunctions.checkPoint[i].latPos[model.functions.pos_current_u] *
					model.weather[model.functions.pos_current_u].size_row,
					speedSettingNr, i, model.weatherFunctions.nCheckPoints);

			model.tmpTid2[5] = std::chrono::high_resolution_clock::now();
			model.durationCalcArcTimeCurrent += model.tmpTid2[5] - model.tmpTid2[6];
			baseGroundSpeed = eval_baseGroundSpeed(*calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
				currentDirection, currentSpeed);
			if (printGlobal == 1) {
				printf("checkP %d vCurrent %.3lf uCurrent %.3lf, currentDirection %.3lf currentSpeed %.3lf baseGroundSpeed %.3lf\n", i, vCurrent,
					uCurrent, currentDirection, currentSpeed, baseGroundSpeed);
			}

			uWind = model.functions.varValue[model.functions.pos_wind_u]; // getVariableValue(model.functions.pos_wind_u, i, tidTot);
			vWind = model.functions.varValue[model.functions.pos_wind_v]; // getVariableValue(model.functions.pos_wind_v, i, tidTot);
			if (uWind < 1000 && vWind < 1000) {
				windDirection = ApproxAtan2(vWind, uWind);
				windSpeed2 = uWind * uWind + vWind * vWind;
				windSpeed = sqrt(windSpeed2);
				if (model.functions.valuesNow.maxWindSpeed < windSpeed)
					model.functions.valuesNow.maxWindSpeed = windSpeed;
				//if (tidTot >= model.weather[model.functions.pos_wind_u].tidpHistoricalWeather)
				//	windSpeed *= model.params.historicDataFactor_windSpeed;

				rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
					windDirection, windSpeed, &rel_windDir);
#ifdef NAZANIN_SAFETY
				absWindDirDiff = eval_absWindDirDiff(model.weatherFunctions.vesselBearing[i], windDirection);
#endif
			}
			else {
				windSpeed = 0;
				rel_windDir = 0;
			}

			waveHeight = model.functions.varValue[model.functions.pos_waveHeight]; // getVariableValue(model.functions.pos_waveHeight, i, tidTot);
			if (waveHeight > 100)
				waveHeight = 0;
			if (model.functions.valuesNow.maxWaveHeight < waveHeight)
				model.functions.valuesNow.maxWaveHeight = waveHeight;
			//else {
			//	if (tidTot >= model.weather[model.functions.pos_waveHeight].tidpHistoricalWeather)
			//		waveHeight *= model.params.historicDataFactor_waveHeight;
			//}
			wavePeriod = model.functions.varValue[model.functions.pos_wavePeriod]; // getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
			if (wavePeriod > 1000)
				wavePeriod = 0;
			waveDirection = model.functions.varValue[model.functions.pos_waveDirection]; // getVariableValue(model.functions.pos_waveDirection, i, tidTot);
			if (waveDirection > 1000)
				waveDirection = 0;
			//rel_waveDir = (waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]; // / model.functions.nWaveDir;
			//rel_waveDir = M_PI - ((waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;
			rel_waveDir = M_PI + ((270 - waveDirection) * M_PI / 180 - model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;
			if (rel_waveDir < 0)
				rel_waveDir = -rel_waveDir;
			if (rel_waveDir >= 2 * M_PI)
				rel_waveDir -= 2 * M_PI;
			if (rel_waveDir > M_PI)
				rel_waveDir = 2 * M_PI - rel_waveDir;

			if (printGlobal == 1) {
				printf("checkP %d waves height %.2lf period %.2lf Direction %.3lf rel_waveDir %.3lf\n", i,
					waveHeight, wavePeriod, waveDirection, rel_waveDir);
			}

			speedDiffWind = lookup_speedDiffWindTable(baseGroundSpeed, rel_windSpeed, rel_windDir);
			speedDiffWave = lookup_speedDiffWaveTable(*calmWaterSpeed, waveHeight, wavePeriod, rel_waveDir);
			speedDiffWindWave = speedDiffWind + speedDiffWave;

			if (model.delay.nYears > 0 && delayVersion >= 4)
				speedOverGround = *calmWaterSpeed - speedDiffWindWave; // in km/h
			else
				speedOverGround = baseGroundSpeed - speedDiffWindWave; // in km/h

			if (speedOverGround < 0.01) {
				speedOverGround = 0.1;
				if (distNu >= 0.001)
					badSpeed = 1;
			}
			timeArc = distNu / speedOverGround; // in hours

			fuelConsumption_main = eval_fuelConsumption_both(speedSettingNr, &fuelConsumption_aux, -channelNr - 1, -channelNr - 1);
			fuelUsage_main = fuelConsumption_main * timeArc;
			fuelUsage_aux = fuelConsumption_aux * timeArc;

			if (printGlobal == 1) {
				printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf fuelConsMain/aux %.3lf %.3lf\n",
					speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu, fuelConsumption_main, fuelConsumption_aux);
			}

			tidTot += timeArc;
			model.tmpTid2[5] = std::chrono::high_resolution_clock::now();
			model.durationCalcArcTimeFuel += model.tmpTid2[5] - model.tmpTid2[6];

			iceCover = getVariableValue(model.functions.pos_iceThickness, i, tidTot);
			if (iceCover > 1000)
				iceCover = 0;

#ifdef NAZANIN_SAFETY
			eval_safety_nazanin(speedOverGround, (*calmWaterSpeed) - speedDiffWindWave, windSpeed, absWindDirDiff, waveHeight, wavePeriod, rel_waveDir, iceCover);
#else
			eval_safety(iceCover);
#endif

			model.tmpTid2[6] = std::chrono::high_resolution_clock::now();
			model.durationCalcArcTimeIceSafety += model.tmpTid2[6] - model.tmpTid2[5];

			model.functions.valuesNow.current += timeArc * (baseGroundSpeed - (*calmWaterSpeed));
			model.functions.valuesNow.windSpeed += timeArc * rel_windSpeed; // windSpeed;
			model.functions.valuesNow.waveHeight += timeArc * waveHeight;
			model.functions.valuesNow.wavePeriod += timeArc * wavePeriod;
		}
		else {
			// channel without speed optimizing
			timeArc = distNu / (*calmWaterSpeed); // in hours
			// fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_use, &fuelConsumption_aux, -channelNr - 1, -channelNr - 1);
			fuelConsumption_main = eval_fuelConsumption_both(0, &fuelConsumption_aux, -channelNr - 1, -channelNr - 1);
			fuelUsage_main = fuelConsumption_main * timeArc;
			fuelUsage_aux = fuelConsumption_aux * timeArc;

			tidTot += timeArc;
		}
		if (i == 0) {
			fuelUsage_main += model.network.channel[channelNr].waiting_consumption_main;
			//fuelUsage_aux -= fuelConsumption_aux * model.network.channel[channelNr].waitingTime;
			fuelUsage_aux += model.network.channel[channelNr].waiting_consumption_aux;
		}

		fuelTot_main += fuelUsage_main;
		fuelTot_aux += fuelUsage_aux;

	}
	model.functions.valuesNow.distance = dist;
	model.functions.valuesNow.fuel_aux = fuelTot_aux;
	model.functions.valuesNow.fuel_main = fuelTot_main;
	//model.functions.valuesNow.safety = safetyTot;

	model.tmpTid4[1] = std::chrono::high_resolution_clock::now();
	model.duration3 += model.tmpTid4[1] - model.tmpTid4[0];

	// printf(" badSpeed %d tidTot %.4lf tidStart %.4lf", badSpeed, tidTot, tidStart);

	if (badSpeed == 1)
		tidTot = 1e7;
	if (tidTot - tidStart < 0)
		printf("ERROR! Negative time for corridor tid %.3lf\n", tidTot - tidStart);

	return tidTot - tidStart;
}

double calcDelayedArcTimeCostChannel(int t, int speedSettingNr, int channelNr, double delayFactor, double* calmWaterSpeed) {

	double timeArc, fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux;
	double tidStart = t * model.params.tIndexGerH, tidTot = tidStart, fixTime, dist;

	model.functions.valuesNow.worstStormValue = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0; // a / b
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;

	if (model.network.channel[channelNr].intArrivalTime_h >= 0)
		tidTot = delayTimeToStartTimeDay(tidStart, model.network.channel[channelNr].intArrivalTime_h);
	tidTot += model.network.channel[channelNr].waitingTime; // .intWaitingTime;
	fixTime = model.network.channel[channelNr].timeThroughChannel;
	dist = model.network.channel[channelNr].distance_km;
	if (fixTime > 0) {
		*calmWaterSpeed = dist / fixTime;
		delayFactor = 1.0;
	}
	else
		*calmWaterSpeed = eval_calmWaterSpeed(speedSettingNr, -channelNr - 1, -channelNr - 1);

	timeArc = dist / (*calmWaterSpeed) * delayFactor; // in hours
	tidTot += timeArc;

	//if (model.network.channel[channelNr].totalConsumption < 0) {
	fuelConsumption_main = eval_fuelConsumption_both(speedSettingNr, &fuelConsumption_aux, channelNr, channelNr);// -channelNr - 1, -channelNr - 1);
	//}
	//else {
	//	fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_use, &fuelConsumption_aux, -1, -100);
		//fuelConsumption_main = model.network.channel[channelNr].totalConsumption;
	//}
	fuelUsage_main = fuelConsumption_main * timeArc;

	fuelUsage_aux = fuelConsumption_aux * timeArc; // -model.network.channel[channelNr].waitingTime);

	fuelUsage_main += model.network.channel[channelNr].waiting_consumption_main;
	fuelUsage_aux += model.network.channel[channelNr].waiting_consumption_aux;

	model.functions.valuesNow.distance = dist;
	model.functions.valuesNow.fuel_aux = fuelUsage_aux;
	model.functions.valuesNow.fuel_main = fuelUsage_main;

	return tidTot - tidStart;
}

int addEndBage(int thisLevel, int pos1, int nextLevel, int i3, int nodNr2) {
	int arcNr, nodNr1, tidInt0, posNy;
	double totCost;

	if (model.nArcs == 61846)
		i3 = i3;
	if (thisLevel >= 0) {
		if (thisLevel < model.network.nPhysicalLevels) {
			nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][i3];
			tidInt0 = model.network.physicalLev[thisLevel].timeInterval[pos1][i3];
		}
		else {
			nodNr1 = nodNr2 - 1;
			tidInt0 = 0;
		}
	}
	else {
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][i3];
		tidInt0 = model.network.channel[-thisLevel - 1].timeInterval[pos1][i3];
	}

	totCost = 0;
	if (tidInt0 == 2795)
		tidInt0 = tidInt0;
	if (model.params.eta_h > 0.01 && thisLevel < model.network.nPhysicalLevels) {
		if (tidInt0 * model.params.tIndexGerH < model.params.eta_h)
			totCost += (model.params.eta_h - tidInt0 * model.params.tIndexGerH) * model.params.eta_cost_early;
		else
			totCost += (tidInt0 * model.params.tIndexGerH - model.params.eta_h) * model.params.eta_cost_late;
		if (model.params.tIndexGerH < 0.5 && tidInt0 < 1919)
			totCost = totCost;
	}

	posNy = adderaArc(nodNr1, nodNr2, totCost, 0);
	arcNr = model.nArcs;

	if (arcNr + 1 >= model.nAllocArcs) {
		model.nAllocArcs += 100000;
		model.arc = (strArcInfo*)realloc(model.arc,
			model.nAllocArcs * sizeof(strArcInfo));
	}
	model.arc[arcNr].fromLevel = thisLevel;
	model.arc[arcNr].toLevel = nextLevel;
	model.arc[arcNr].fromPointNr = pos1;
	model.arc[arcNr].toPointNr = 0;
	model.arc[arcNr].fromTime = tidInt0;
	model.arc[arcNr].toTime = 0;
	model.arc[arcNr].speedSetting = 0;
	model.arc[arcNr].time = 0;
	model.arc[arcNr].distance = 0;
	model.arc[arcNr].emission = 0;
	model.arc[arcNr].fuelBase = 0;
	model.arc[arcNr].fuel_aux = 0;
	model.arc[arcNr].fuel_auxEca = 0;
	model.arc[arcNr].fuel_eca = 0;
	model.arc[arcNr].fuelQualityKvot = 1.0;
	model.arc[arcNr].extraAreaCostKvot = 0;
	model.arc[arcNr].maxWindSpeed = 0;
	model.arc[arcNr].maxWaveHeight = 0;
	model.arc[arcNr].kvotCost = 1.0;
	model.arc[arcNr].fuel_noEca = 0;
	model.arc[arcNr].safetyHurricane = 0;
	model.arc[arcNr].bowSlam = 0;
	model.arc[arcNr].greenWater = 0;
	model.arc[arcNr].dynamicStability = 0;
	model.arc[arcNr].rolling = 0;
	model.arc[arcNr].surfRiding = 0;
	//model.arc[arcNr].safetyBowSlam = 0;
	//model.arc[arcNr].safetyGreenWater = 0;
	//model.arc[arcNr].safetyDynStability = 0;
	//model.arc[arcNr].feasibleSafety = 0;
	//model.arc[arcNr].iceCoverCost = 0;
	////model.arc[arcNr].safetyStability = 0;
	model.arc[arcNr].safetyBase = 0;
	//model.arc[arcNr].channelCost = 0;
	model.arc[arcNr].totCost = totCost;

	//fprintf(filSaveSpec, "arcNr %d from %d %d %d to %d %d %d cost %.4lf row %d\n",
	//	arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
	//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime, 
	//	model.arc[arcNr].totCost, __LINE__);

	model.arc[arcNr].nodNr1 = nodNr1;
	//model.arc[arcNr].nodNr2 = nodNr2;
	model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
	model.nArcs++;

	return 0;
}


int addEnBage_AB(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int i4, int* setupCheckPoints, int min_t, int max_t,
	double fuelQualityKvot, double extraAreaCostKvot, int runAlt)
{
	// i = thisLevel, i1 = pointPos, i+1 = nextLevel, i2 = outNodePos, i3 = tPos
	int tidInt, nArcsNu = 0, nodNr1, nodNr2, posNy, arcNr, prefPath = 0;
	int timeInt, nSpeedSettings, posDiff;
	double tid, calmWaterSpeed, fuelFactorMain = -1; // , safety, fuel, distance
	double totCost, channelCost, safety, emission, kvotCost;
	double fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, safetyBase; // , worstStormValue = 0;
	// double worstStabilityValue = 0;

	if (model.nArcs >= 12577)
		model.nArcs = model.nArcs;
	calmWaterSpeed = -1.0;
	//if (thisLevel == 6)
	//	printf("thisLev %d\n", thisLevel);
	if (thisLevel >= 0) {
		nSpeedSettings = model.functions.speedLevel[thisLevel].nShip_speedSettings;
		if (pos1 == model.params.preferredPathOrtoPos[thisLevel]) {
			if (nextLevel >= 0) {
				if (pos2 == model.params.preferredPathOrtoPos[nextLevel] && thisLevel == nextLevel - 1 &&
					(model.params.preferredPathStraightLineFeasibleFrom[thisLevel] == 0 || model.params.max_changeDirection == 0 || runAlt == 1))
					prefPath = 1;
			}
			else {
				if ((model.network.channel[-nextLevel - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0 || runAlt == 1) &&
					pos1 == model.params.preferredPathOrtoPos[thisLevel] && model.network.channel[-nextLevel - 1].preferredPathPoint_posConnectTo >= 0)
					prefPath = 1;
			}
			//if (model.params.preferredPathUseChannelSpeed[thisLevel] > 0) {
			//	calmWaterSpeed = model.params.preferredPathUseChannelSpeed[thisLevel];
			//	fuelFactorMain = model.network.channel[model.params.preferredPathUseChannelConsumption[thisLevel]].totalConsumption;
			//	nSpeedSettings = 1;
			//}
		}
	}
	else {
		if (nextLevel >= 0) {
			nSpeedSettings = model.functions.speedChannelOut[-thisLevel - 1].nShip_speedSettings;
			if ((model.network.channel[-thisLevel - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0 || runAlt == 1) &&
				pos2 == model.params.preferredPathOrtoPos[nextLevel] && model.network.channel[-thisLevel - 1].preferredPathPoint_posConnectFrom >= 0 &&
				model.network.channel[-thisLevel - 1].bastEndLevel == nextLevel)
				prefPath = 1;
			//if (pos2 == model.params.preferredPathOrtoPos[nextLevel]) {
				//	if (model.params.preferredPathUseChannelSpeed[nextLevel - 1] > 0) {
				//		calmWaterSpeed = model.params.preferredPathUseChannelSpeed[nextLevel - 1];
				//		fuelFactorMain = model.network.channel[model.params.preferredPathUseChannelConsumption[nextLevel - 1]].totalConsumption;
				//		nSpeedSettings = 1;
				//	}
				//}
		}
		else
			nSpeedSettings = model.functions.speedChannel[-thisLevel - 1].nShip_speedSettings;
	}


	if (*setupCheckPoints == 1) {
		if (thisLevel == 2 && pos1 == 25 && nextLevel == 3 && pos2 == 25)
			pos1 = pos1;
		if (thisLevel == 3 && nextLevel == -2)
			pos1 = pos1;
		if (printGlobal == 1)
			printf("prefPath %d\n", prefPath);
		if (thisLevel >= 0) {
			if (nextLevel >= 0) {
				if (prefPath == 1)
					calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[thisLevel].point[pos1], thisLevel);
				else {
					calcWeatherPosAlongArc(model.network.physicalLev[thisLevel].point[pos1],
						model.network.physicalLev[nextLevel].point[pos2], model.network.physicalLev[thisLevel].timeInterval[pos1][tPos]);
				}
			}
			else {
				if (prefPath == 1)
					calcWeatherPosAlongpreferredPathArc_connectChannel(model.network.physicalLev[thisLevel].point[pos1], thisLevel,
						model.network.channel[-nextLevel - 1].point[0], nextLevel);
				else
					calcWeatherPosAlongArc(model.network.physicalLev[thisLevel].point[pos1],
						model.network.channel[-nextLevel - 1].point[0], model.network.physicalLev[thisLevel].timeInterval[pos1][tPos]);
			}
		}
		else {
			if (nextLevel >= 0) {
				if (prefPath == 1)
					calcWeatherPosAlongpreferredPathArc_connectChannel(model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1],
						thisLevel, model.network.physicalLev[nextLevel].point[pos2], nextLevel);
				else
					calcWeatherPosAlongArc(model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1],
						model.network.physicalLev[nextLevel].point[pos2], model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos]);
			}
			else {
				if (thisLevel == nextLevel)
					calcWeatherPosAlongChannel(-thisLevel - 1); // , posPoly1, posPoly2);
				else
					calcWeatherPosAlongArc(model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1],
						model.network.channel[-nextLevel - 1].point[0], model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos]);
			}
		}
		*setupCheckPoints = 0;
	}

	model.functions.valuesNow.maxWaveHeight = 0;
	model.functions.valuesNow.maxWaveHeight_dir = 0;
	model.functions.valuesNow.maxWaveHeight_tp = 0;
	model.functions.valuesNow.maxWindSpeed = 0;
	model.functions.valuesNow.maxWindSpeed_dir = 0;
	model.functions.valuesNow.maxWindSpeed_tp = 0;

	//for (int i = 0; i < 2; i++) {
	//	for (int i1 = 0; i1 < 3; i1++) {
	//		model.functions.valuesNow.favorableWind[i][i1] = 0;
	//		model.functions.valuesNow.favorableWave[i][i1] = 0;
	//		model.functions.valuesNow.favorableWindWave[i][i1] = 0;
	//	}
	//}

	//if (model.nArcs == 6)
	//	model.nArcs = model.nArcs;
	if (thisLevel >= 0 || nextLevel >= 0) {
		if (thisLevel >= 0) {
			timeInt = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
			nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
		}
		else {
			timeInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
			nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
		}
		//model.tmpTid2[2] = std::chrono::high_resolution_clock::now();

		tid = calcArcTimeCost(timeInt, i4, thisLevel, nextLevel, &calmWaterSpeed, fuelFactorMain); // , & fuel, & safety, & distance, & worstStormValue, & worstStabilityValue);
		//if (model.nArcs == 36039) {
		//	fprintf(filSaveSpec, "tid i calcArcTimeCost %lf\n\n", tid);
		//}
		//if (skrivUtExtreme == 1) {
		//	if (thisLevel >= 16) {
				//printf("levels %d %d pos %d %d timeInt %d i4 %d tid %.2lf nArcs %d\n", thisLevel, nextLevel, pos1, pos2, timeInt, i4, tid, model.nArcs);
		//	}
		//}

		//model.durationCalcArcTimeCost += std::chrono::high_resolution_clock::now() - model.tmpTid2[2];
		tidInt = timeInt + (int)round(tid * model.params.nTidsperioder_perH);
	}
	else {
		if (model.network.channel[-thisLevel - 1].timeThroughChannel > -0.5) {
			// the speed through the channel should not be optimized
			if (i4 > 0)
				return -1; // set speed, so only use first speed setting, this should never happen
		}
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
		if (model.nArcs == 7)
			pos1 = pos1;
		//printf("level %d nArcs %d", -thisLevel - 1, model.nArcs);
		tid = calcArcTimeCostChannel(model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos],
			i4, -thisLevel - 1, &calmWaterSpeed);
		tidInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos] + (int)round(tid * model.params.nTidsperioder_perH);
		//printf(" .. tid %.4lf tidInt %d nArcs %d\n", tid, tidInt, model.nArcs);
	}

	globalCount2++;

	if (tidInt < 0)
		printf("tid %.3lf tidInt %d nArcs %d max/min_t %d %d\n", tid, tidInt, model.nArcs, max_t, min_t);

	//if (model.iterKaoutar.filpek != NULL) {
	//	fprintf(model.iterKaoutar.filpek, "narcs;%d;fromLev;%d;fromPos;%d;toPos;%d;speedsetting;%d;tidInt;%d;minTid;%d;maxTid;%d\n",
	//		model.nArcs, thisLevel, pos1, pos2, i4, tidInt, min_t, max_t);
	//}

	if (tidInt <= max_t && tidInt >= min_t) {
		model.tmpTid2[2] = std::chrono::high_resolution_clock::now();
		if (thisLevel < 0 && nextLevel < 0) {
			channelCost = model.network.channel[-thisLevel - 1].extraCostChannel;
			kvotCost = model.network.channel[-thisLevel - 1].kvotCost;
		}
		else {
			channelCost = 0;
			kvotCost = 1;
		}
		totCost = channelCost;

		model.tmpTid5[0] = std::chrono::high_resolution_clock::now();
		nodNr2 = addTimeTo_timeInterval(thisLevel, nextLevel, pos2, tidInt);
		model.tmpTid5[1] = std::chrono::high_resolution_clock::now();
		model.duration4 += model.tmpTid5[1] - model.tmpTid5[0];
		fuel_eca = model.functions.valuesNow.fuel_main * (1 - fuelQualityKvot);
		fuel_noEca = model.functions.valuesNow.fuel_main * fuelQualityKvot;
		fuel_aux = model.functions.valuesNow.fuel_aux * fuelQualityKvot;
		fuel_auxEca = model.functions.valuesNow.fuel_aux * (1 - fuelQualityKvot);
		fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
			fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price);

		if (model.nArcs >= 42)
			model.nArcs = model.nArcs;
		//if (fuelBase > 10000) {
			//printf("nArcs %d fuel_aux %.3lf fuel_auxEca %.3lf fuel_eca %.3lf fuel_noEca %.3lf\n", model.nArcs,
			//	fuel_aux, fuel_auxEca, fuel_eca, fuel_noEca);
		//	errlog("nArcs %d fuel_aux %.3lf fuel_auxEca %.3lf fuel_eca %.3lf fuel_noEca %.3lf fuelBase %.3lf\n", model.nArcs,
		//		fuel_aux, fuel_auxEca, fuel_eca, fuel_noEca, fuelBase);
		//}
		emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
			fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

		safety = model.functions.valuesNow.worstStormValue *
			model.params.weightSafety.hurricane +
			model.functions.valuesNow.bowSlam *
			model.params.weightSafety.bowSlam +
			model.functions.valuesNow.greenWater *
			model.params.weightSafety.greenWater +
			model.functions.valuesNow.dynamicStability *
			model.params.weightSafety.dynamicStability +
			model.functions.valuesNow.rolling *
			model.params.weightSafety.rolling +
			model.functions.valuesNow.surfRiding *
			model.params.weightSafety.surfRiding +
			(1 - model.functions.valuesNow.feasibleSafety) *
			model.params.weightSafety.feasibleSafety +
			model.functions.valuesNow.iceCoverCost;

		if (model.nArcs == 997046)
			safety = safety;
		if (model.functions.valuesNow.maxWaveHeight > model.functions.maxWaveHeight)
			safety += 1e12 * (1 + model.functions.valuesNow.maxWaveHeight - model.functions.maxWaveHeight);

		safetyBase = safety;

		if (model.params.useSimulering == 1) {
			if (model.functions.valuesNow.maxWaveHeight > model.params.user_maxWaveHeight)
				totCost += model.simulering.penOverWeatherLimit_fix +
				(model.functions.valuesNow.maxWaveHeight - model.params.user_maxWaveHeight) * model.simulering.penOverMaxWaveHeight_m;
			if (model.functions.valuesNow.maxWindSpeed > model.params.user_maxWindSpeed_kmh)
				totCost += model.simulering.penOverWeatherLimit_fix +
				(model.functions.valuesNow.maxWindSpeed - model.params.user_maxWindSpeed_kmh) * model.simulering.penOverMaxWindSpeed_kmh;
			if (thisLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[thisLevel] - pos1);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (nextLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[nextLevel] - pos2);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (model.params.simulationSpeed_kmh > 0)
				totCost += abs(calmWaterSpeed - model.params.simulationSpeed_kmh) * model.simulering.penDeviateSpeed_kmh;
		}

		//if (model.nArcs == 175518)
		//	printGlobal = 0;

		totCost += model.params.weightTime * model.params.priceTime * tid +
			model.params.weightFuel * fuelBase + model.params.weightSafety.base * safety +
			emission * model.params.weightEmission * model.params.scaleObjEmission;
		if (kvotCost < 0.99)
			kvotCost = kvotCost;

		if (totCost > 1e15)
			totCost = totCost;
		totCost *= (1 + extraAreaCostKvot); // *kvotCost;
		if (USE_KVOTKOST == 1)
			totCost *= kvotCost;

		if (totCost < 0) { // } || model.nArcs == 2619560) {
			printf("\nchannelCost %.2lf wTime %.2lf pTime %.2lf tid %.2lf wFuel %.2lf fBase %.2lf emission %.2lf wEmission %.2lf wSafety %.2lf safety %.2lf totCost %.2lf nArcs %d\n",
				channelCost, model.params.weightTime, model.params.priceTime, tid,
				1.0, fuelBase, emission, model.params.weightEmission, model.params.weightSafety.base, safety, totCost, model.nArcs);
			printf("worstStormVal %.2lf hurricaneWeight %.2lf feasibleSafety %.2lf weightFeasSafety %.2lf safety %.2lf weightSafety %.2lf\n",
				model.functions.valuesNow.worstStormValue, model.params.weightSafety.hurricane,
				(1 - model.functions.valuesNow.feasibleSafety), model.params.weightSafety.feasibleSafety, safety,
				model.params.weightSafety.base);
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

		posNy = adderaArc(nodNr1, nodNr2, totCost, i4);

			posNy = posNy;
		if (posNy == -2) {
			return -1; // do not add this arc as there is another one thats cheaper between the time nodes, this should not happen for historical data
			// or maybe if close speed settings so two different speeds get there at the same time...
		}
		if (posNy >= 0) {
			//if (model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] >= model.network.tidp_startHistoricDataOnly)
			//	model.network.arcGen_staticWeatherArcNr_outNodePosSpeed[pos2][i4] = posNy;
			//if (calmWaterSpeed < 0)
			model.arc[posNy].speedSetting = i4;
			//else
			//	model.arc[posNy].speedSetting = -1;
			model.arc[posNy].time = tid;
			model.arc[posNy].fuelBase = fuelBase;
			model.arc[posNy].emission = emission;
			if (fuelBase < 0)
				errlog("ERROR! fuelBase %lf for arcNr %d\n", fuelBase, posNy);
			model.arc[posNy].fuel_aux = fuel_aux;
			model.arc[posNy].fuel_auxEca = fuel_auxEca;
			model.arc[posNy].fuel_eca = fuel_eca;
			model.arc[posNy].fuel_noEca = fuel_noEca;
			model.arc[posNy].fuelQualityKvot = fuelQualityKvot;
			model.arc[posNy].extraAreaCostKvot = extraAreaCostKvot;
			model.arc[posNy].kvotCost = kvotCost;
			model.arc[posNy].distance = model.functions.valuesNow.distance;
			model.arc[posNy].maxWindSpeed = model.functions.valuesNow.maxWindSpeed;
			model.arc[posNy].maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
			model.arc[posNy].safetyHurricane = model.functions.valuesNow.worstStormValue;
			model.arc[posNy].bowSlam = model.functions.valuesNow.bowSlam;
			model.arc[posNy].greenWater = model.functions.valuesNow.greenWater;
			model.arc[posNy].dynamicStability = model.functions.valuesNow.dynamicStability;
			model.arc[posNy].rolling = model.functions.valuesNow.rolling;
			model.arc[posNy].surfRiding = model.functions.valuesNow.surfRiding;
			model.arc[posNy].safetyBase = safety;
			if (safety < 0)
				errlog("ERROR! safetyBase %lf for arcNr %d\n", safety, posNy);
			//model.arc[posNy].channelCost = channelCost;
			model.arc[posNy].totCost = totCost;
			arcNr = posNy;
		}
		else {
			arcNr = model.nArcs;
			if (model.nArcs >= 3939)
				arcNr = arcNr;
			if (model.nArcs == 185619)
				arcNr = arcNr;
			//if (arcNr < 500)
			//	errlog("Add arc %d levels %d %d pointNr %d %d tid %.3lf speedSetting %d dist %.3lf emission %.3lf fuelCost %.3lf\n", arcNr, thisLevel, nextLevel, pos1, pos2,
			//		tid, i4, model.functions.valuesNow.distance, emission, fuelBase);
			//if (model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] >= model.network.tidp_startHistoricDataOnly)
			//	model.network.arcGen_staticWeatherArcNr_outNodePosSpeed[pos2][i4] = arcNr;
			if (arcNr >= model.nAllocArcs) {
				model.nAllocArcs += 1000000;
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
				(model.network.channel[-nextLevel - 1].nArcsToPoint)++;
			if (thisLevel >= 0)
				model.arc[arcNr].fromTime = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
			else
				model.arc[arcNr].fromTime = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
			model.arc[arcNr].toTime = tidInt;
			//if (calmWaterSpeed < 0)
			model.arc[arcNr].speedSetting = i4;
			//else {
			//	model.arc[arcNr].speedSetting = -1;
			//}
			model.arc[arcNr].time = tid;
			model.arc[arcNr].distance = model.functions.valuesNow.distance;
			model.arc[arcNr].emission = emission;
			model.arc[arcNr].fuelBase = fuelBase;
			if (fuelBase < 0)
				errlog("ERROR! fuelBase2 %lf for arcNr %d\n", fuelBase, arcNr);
			model.arc[arcNr].fuel_aux = fuel_aux;
			model.arc[arcNr].fuel_auxEca = fuel_auxEca;
			model.arc[arcNr].fuel_eca = fuel_eca;
			model.arc[arcNr].fuel_noEca = fuel_noEca;
			model.arc[arcNr].fuelQualityKvot = fuelQualityKvot;
			model.arc[arcNr].extraAreaCostKvot = extraAreaCostKvot;
			model.arc[arcNr].maxWindSpeed = model.functions.valuesNow.maxWindSpeed;
			model.arc[arcNr].maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
			model.arc[arcNr].kvotCost = kvotCost;
			model.arc[arcNr].safetyHurricane = model.functions.valuesNow.worstStormValue;
			model.arc[arcNr].bowSlam = model.functions.valuesNow.bowSlam;
			model.arc[arcNr].greenWater = model.functions.valuesNow.greenWater;
			model.arc[arcNr].dynamicStability = model.functions.valuesNow.dynamicStability;
			model.arc[arcNr].rolling = model.functions.valuesNow.rolling;
			model.arc[arcNr].surfRiding = model.functions.valuesNow.surfRiding;
			//model.arc[arcNr].safetyBowSlam = model.functions.valuesNow.bowSlam;
			//model.arc[arcNr].safetyGreenWater = model.functions.valuesNow.greenWater;
			//model.arc[arcNr].safetyDynStability = model.functions.valuesNow.dynamicStability;
			//model.arc[arcNr].feasibleSafety = model.functions.valuesNow.feasibleSafety;
			//model.arc[arcNr].iceCoverCost = model.functions.valuesNow.iceCoverCost;
			//model.arc[arcNr].safetyStability = worstStabilityValue;
			model.arc[arcNr].safetyBase = safety;
			if (safety < 0)
				errlog("ERROR! safetyBase2 %lf for arcNr %d\n", safety, arcNr);
			//model.arc[arcNr].channelCost = channelCost;
			model.arc[arcNr].totCost = totCost;
			//fprintf(filSaveSpec, "arcNr %d from %d %d %d to %d %d %d cost %.4lf row %d\n",
			//	arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
			//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
			//	model.arc[arcNr].totCost, __LINE__);

			model.arc[arcNr].nodNr1 = nodNr1;
			//model.arc[arcNr].nodNr2 = nodNr2;
			model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
			model.nArcs++;
			nArcsNu++;
		}
		model.tmpTid2[3] = std::chrono::high_resolution_clock::now();
		model.durationSetArcValues += model.tmpTid2[3] - model.tmpTid2[2];
	}
	else {
		return -1;
		//if (model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] >= model.network.tidp_startHistoricDataOnly)
		//	model.network.arcGen_staticWeatherArcNr_outNodePosSpeed[pos2][i4] = -1;
	}

	return arcNr;
}

int addEnBage_delayAB(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int i4, int min_t, int max_t, double fuelQualityKvot,
	double extraAreaCostKvot, double dist, double delayFactor)
{
	// i = thisLevel, i1 = pointPos, i+1 = nextLevel, i2 = outNodePos, i3 = tPos
	int tidInt, nArcsNu = 0, nodNr1, nodNr2, posNy, arcNr;
	int timeInt, posDiff;
	double tid, calmWaterSpeed, fuelFactorMain = -1; // , safety, fuel, distance
	double totCost, channelCost, emission, safety = 0;
	double fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase; // , worstStormValue = 0;
	double speedDiffCurrent, kvotCost;

	if (model.nArcs == 71800)
		model.nArcs = model.nArcs;

	calmWaterSpeed = -1.0;

	if (model.nArcs == 408909)
		model.nArcs = model.nArcs;
	if (thisLevel >= 0 || nextLevel >= 0) {
		if (thisLevel >= 0) {
			timeInt = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
			nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
		}
		else {
			timeInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
			nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
		}

		calmWaterSpeed = eval_calmWaterSpeed(i4, thisLevel, nextLevel);
		speedDiffCurrent = eval_speedDiffCurrent_delayedAlongArc(thisLevel, pos1, nextLevel, pos2, tPos, calmWaterSpeed);

		tid = calcDelayedArcTimeCost(thisLevel, nextLevel, i4, calmWaterSpeed, fuelFactorMain, delayFactor, dist, speedDiffCurrent);
		tidInt = timeInt + (int)round(tid * model.params.nTidsperioder_perH);
	}
	else {
		if (model.network.channel[-thisLevel - 1].timeThroughChannel > -0.5) {
			// the speed through the channel should not be optimized
			if (i4 > 0)
				return -1; // set speed, so only use first speed setting, this should never happen
		}
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
		timeInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
		tid = calcDelayedArcTimeCostChannel(timeInt, i4, -thisLevel - 1, delayFactor, &calmWaterSpeed);
		tidInt = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos] + (int)round(tid * model.params.nTidsperioder_perH);
	}

	if (tidInt < 0)
		printf("tid %.3lf tidInt %d min/max_t %d %d nArcs %d\n", tid, tidInt, min_t, max_t, model.nArcs);

	if (tidInt <= max_t && tidInt >= min_t) {
		if (thisLevel < 0 && nextLevel < 0) {
			channelCost = model.network.channel[-thisLevel - 1].extraCostChannel;
			kvotCost = model.network.channel[-thisLevel - 1].kvotCost;
		}
		else {
			channelCost = 0;
			kvotCost = 1.0;
		}

		totCost = channelCost;

		nodNr2 = addTimeTo_timeInterval(thisLevel, nextLevel, pos2, tidInt);
		fuel_eca = model.functions.valuesNow.fuel_main * (1 - fuelQualityKvot);
		fuel_noEca = model.functions.valuesNow.fuel_main * fuelQualityKvot;
		fuel_aux = model.functions.valuesNow.fuel_aux * fuelQualityKvot;
		fuel_auxEca = model.functions.valuesNow.fuel_aux * (1 - fuelQualityKvot);
		fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
			fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price);
		emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
			fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

		totCost += model.params.weightTime * model.params.priceTime * tid +
			model.params.weightFuel * fuelBase + emission * model.params.weightEmission * model.params.scaleObjEmission;

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
			if (nextLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[nextLevel] - pos2);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (model.params.simulationSpeed_kmh > 0)
				totCost += abs(calmWaterSpeed - model.params.simulationSpeed_kmh) * model.simulering.penDeviateSpeed_kmh;
		}


		totCost *= (1 + extraAreaCostKvot);

		if (model.functions.valuesNow.prefPathArc == 0)
			totCost *= model.params.delayEjPrefPathArcFactor;

		if (USE_KVOTKOST == 1)
			totCost *= kvotCost;

		if (totCost < 0) { // } || model.nArcs == 2619560) {
			printf("\nchannelCost %.2lf wTime %.2lf pTime %.2lf tid %.2lf wFuel %.2lf fBase %.2lf emission %.2lf wEmission %.2lf wSafety %.2lf safety %.2lf totCost %.2lf nArcs %d\n",
				channelCost, model.params.weightTime, model.params.priceTime, tid,
				1.0, fuelBase, emission, model.params.weightEmission, model.params.weightSafety.base, safety, totCost, model.nArcs);
			printf("worstStormVal %.2lf hurricaneWeight %.2lf feasibleSafety %.2lf weightFeasSafety %.2lf safety %.2lf weightSafety %.2lf\n",
				model.functions.valuesNow.worstStormValue, model.params.weightSafety.hurricane,
				(1 - model.functions.valuesNow.feasibleSafety), model.params.weightSafety.feasibleSafety, safety,
				model.params.weightSafety.base);
			printf("thisLevel %d pos1 %d nextLevel %d pos2 %d tPos %d i4 %d\n",
				thisLevel, pos1, nextLevel, pos2, tPos, i4);
			printf("thisLevel %d\n", thisLevel);
			printf("tidInt %d\n",
				model.network.physicalLev[thisLevel].timeInterval[pos1][tPos]);
			printf("lon/lat %.2lf %.2lf to lon/lat %.2lf %.2lf\n",
				model.network.physicalLev[thisLevel].point[pos1].longitude().degrees(),
				model.network.physicalLev[thisLevel].point[pos1].latitude().degrees(),
				model.network.physicalLev[nextLevel].point[pos2].longitude().degrees(),
				model.network.physicalLev[nextLevel].point[pos2].latitude().degrees());
		}

		posNy = adderaArc(nodNr1, nodNr2, totCost, i4);
		if (posNy == -2) {
			return -1; // do not add this arc as there is another one thats cheaper between the time nodes, this should not happen for historical data
			// or maybe if close speed settings so two different speeds get there at the same time...
		}
		if (posNy >= 0) {
			//if (model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] >= model.network.tidp_startHistoricDataOnly)
			//	model.network.arcGen_staticWeatherArcNr_outNodePosSpeed[pos2][i4] = posNy;
			//if (calmWaterSpeed < 0)
			model.arc[posNy].speedSetting = i4;
			//else
			//	model.arc[posNy].speedSetting = -1;
			model.arc[posNy].time = tid;
			model.arc[posNy].fuelBase = fuelBase;
			model.arc[posNy].emission = emission;
			if (fuelBase < 0)
				errlog("ERROR! fuelBase %lf for arcNr %d\n", fuelBase, posNy);
			model.arc[posNy].fuel_aux = fuel_aux;
			model.arc[posNy].fuel_auxEca = fuel_auxEca;
			model.arc[posNy].fuel_eca = fuel_eca;
			model.arc[posNy].fuel_noEca = fuel_noEca;
			model.arc[posNy].fuelQualityKvot = fuelQualityKvot;
			model.arc[posNy].extraAreaCostKvot = extraAreaCostKvot;
			model.arc[posNy].maxWindSpeed = 0;
			model.arc[posNy].maxWaveHeight = 0;
			model.arc[posNy].kvotCost = kvotCost;
			model.arc[posNy].distance = model.functions.valuesNow.distance;
			model.arc[posNy].safetyHurricane = model.functions.valuesNow.worstStormValue;
			model.arc[posNy].bowSlam = model.functions.valuesNow.bowSlam;
			model.arc[posNy].greenWater = model.functions.valuesNow.greenWater;
			model.arc[posNy].dynamicStability = model.functions.valuesNow.dynamicStability;
			model.arc[posNy].rolling = model.functions.valuesNow.rolling;
			model.arc[posNy].surfRiding = model.functions.valuesNow.surfRiding;
			//model.arc[posNy].safetyBowSlam = model.functions.valuesNow.bowSlam;
			//model.arc[posNy].safetyGreenWater = model.functions.valuesNow.greenWater;
			//model.arc[posNy].safetyDynStability = model.functions.valuesNow.dynamicStability;
			//model.arc[posNy].feasibleSafety = model.functions.valuesNow.feasibleSafety;
			//model.arc[posNy].iceCoverCost = model.functions.valuesNow.iceCoverCost;
			//model.arc[posNy].safetyStability = worstStabilityValue;
			model.arc[posNy].safetyBase = safety;
			if (safety < 0 || safety > 10000)
				errlog("ERROR! safetyBase %lf for arcNr %d\n", safety, posNy);
			//model.arc[posNy].channelCost = channelCost;
			model.arc[posNy].totCost = totCost;
			arcNr = posNy;
		}
		else {
			arcNr = model.nArcs;
			if (model.nArcs >= 3939)
				model.nArcs = model.nArcs;
			//if (model.network.physicalLev[thisLevel].timeInterval[pos1][tPos] >= model.network.tidp_startHistoricDataOnly)
			//	model.network.arcGen_staticWeatherArcNr_outNodePosSpeed[pos2][i4] = arcNr;
			if (arcNr >= model.nAllocArcs) {
				model.nAllocArcs += 1000000;
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
				(model.network.channel[-nextLevel - 1].nArcsToPoint)++;
			if (thisLevel >= 0)
				model.arc[arcNr].fromTime = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
			else
				model.arc[arcNr].fromTime = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
			model.arc[arcNr].toTime = tidInt;
			//if (calmWaterSpeed < 0)
			model.arc[arcNr].speedSetting = i4;
			//else {
			//	model.arc[arcNr].speedSetting = -1;
			//}
			model.arc[arcNr].time = tid;
			model.arc[arcNr].distance = model.functions.valuesNow.distance;
			model.arc[arcNr].emission = emission;
			model.arc[arcNr].fuelBase = fuelBase;
			if (fuelBase < 0)
				errlog("ERROR! fuelBase2 %lf for arcNr %d\n", fuelBase, arcNr);
			model.arc[arcNr].fuel_aux = fuel_aux;
			model.arc[arcNr].fuel_auxEca = fuel_auxEca;
			model.arc[arcNr].fuel_eca = fuel_eca;
			model.arc[arcNr].fuel_noEca = fuel_noEca;
			model.arc[arcNr].fuelQualityKvot = fuelQualityKvot;
			model.arc[arcNr].extraAreaCostKvot = extraAreaCostKvot;
			model.arc[arcNr].maxWindSpeed = 0;
			model.arc[arcNr].maxWaveHeight = 0;
			model.arc[arcNr].kvotCost = kvotCost;
			model.arc[arcNr].safetyHurricane = model.functions.valuesNow.worstStormValue;
			model.arc[arcNr].bowSlam = model.functions.valuesNow.bowSlam;
			model.arc[arcNr].greenWater = model.functions.valuesNow.greenWater;
			model.arc[arcNr].dynamicStability = model.functions.valuesNow.dynamicStability;
			model.arc[arcNr].rolling = model.functions.valuesNow.rolling;
			model.arc[arcNr].surfRiding = model.functions.valuesNow.surfRiding;
			//model.arc[arcNr].safetyBowSlam = model.functions.valuesNow.bowSlam;
			//model.arc[arcNr].safetyGreenWater = model.functions.valuesNow.greenWater;
			//model.arc[arcNr].safetyDynStability = model.functions.valuesNow.dynamicStability;
			//model.arc[arcNr].feasibleSafety = model.functions.valuesNow.feasibleSafety;
			//model.arc[arcNr].iceCoverCost = model.functions.valuesNow.iceCoverCost;
			//model.arc[arcNr].safetyStability = worstStabilityValue;
			model.arc[arcNr].safetyBase = safety;
			if (safety < 0)
				errlog("ERROR! safetyBase2 %lf for arcNr %d\n", safety, arcNr);
			//model.arc[arcNr].channelCost = channelCost;
			model.arc[arcNr].totCost = totCost;
			//fprintf(filSaveSpec, "arcNr %d from %d %d %d to %d %d %d cost %.4lf row %d\n",
			//	arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
			//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
			//	model.arc[arcNr].totCost, __LINE__);
			model.arc[arcNr].nodNr1 = nodNr1;
			//model.arc[arcNr].nodNr2 = nodNr2;
			model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
			model.nArcs++;
			nArcsNu++;
		}
		model.tmpTid2[3] = std::chrono::high_resolution_clock::now();
		model.durationSetArcValues += model.tmpTid2[3] - model.tmpTid2[2];
	}
	else {
		return -1;
	}

	return arcNr;
}

int genArcs_withDelay(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int nSpeedSettings, double fuelQualityKvot,
	double extraAreaCostKvot, double* distArc, double delayFactor) {
	int i4, nArcs = 0, arcNr;

	if (*distArc < 0) {
		double x1, y1, x2, y2;
		if (thisLevel >= 0) {
			x1 = model.network.physicalLev[thisLevel].point_x[pos1];
			y1 = model.network.physicalLev[thisLevel].point_y[pos1];
		}
		else {
			x1 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
			y1 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
		}
		if (nextLevel >= 0) {
			x2 = model.network.physicalLev[nextLevel].point_x[pos2];
			y2 = model.network.physicalLev[nextLevel].point_y[pos2];
		}
		else {
			x2 = model.network.channel[-nextLevel - 1].point_x[pos2];
			y2 = model.network.channel[-nextLevel - 1].point_y[pos2];
		}
		*distArc = estimateLargeCircleDistance_km(y1, x1, y2, x2);
	}
	for (i4 = 0; i4 < nSpeedSettings; i4++) {
		if (model.nArcs >= 3939)
			model.nArcs = model.nArcs;

		arcNr = addEnBage_delayAB(thisLevel, pos1, nextLevel, pos2, tPos, i4, 0, 1e10, fuelQualityKvot, extraAreaCostKvot, *distArc, delayFactor);
		if (arcNr >= 0)
			nArcs++;
	}
	return nArcs;
}

int genArcsTo_delayedPreferredPath(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int nSpeedSettings, double fuelQualityKvot,
	double extraAreaCostKvot, double* delayFactor, double* distArc)
{
	// compare to addBagar_AB_speedSTid

	// thisLevel can be negative (ending of channel) and next position could be a corridor start so handle these here too

	// generate arcs back to pref path in up to nMaxLevToPrefPath
	// use max turn of already generated arcs to determine if feasible when getting back to pref path

	int posPrefP1, posPrefP2, nArcs, tidp;
	double speedDiffCurrent = 0;

	if (thisLevel >= 0) {
		posPrefP1 = model.params.preferredPathOrtoPos[thisLevel];
	}
	else {
		posPrefP1 = 1; // end point of channel
	}
	if (nextLevel >= 0) {
		posPrefP2 = model.params.preferredPathOrtoPos[nextLevel];
	}
	else {
		posPrefP2 = 0; // start point of channel
	}

	if (model.nArcs == 42)
		model.nArcs = model.nArcs;
	//if (thisLevel == 6)
	//	thisLevel = thisLevel;

	if (posPrefP1 == pos1 && delayVersion == 1) {
		if (posPrefP2 != pos2)
			return 0; // on pref path, do not turn away from it

		model.functions.valuesNow.prefPathArc = 1;
		if (thisLevel >= 12)
			thisLevel = thisLevel;
		// run delayed along preferred path
		if (thisLevel >= 0) {
			// not a channel
			if (*delayFactor < 0.0001) {
				// determine the delay factor for the pref path
				*delayFactor = eval_factorDelayedAlongPath(thisLevel, model.network.physicalLev[thisLevel].timeInterval[pos1][tPos], &speedDiffCurrent);
				if (nextLevel >= 0)
					*distArc = model.network.physicalLev[thisLevel + 1].distanceFromStartPosMid - model.network.physicalLev[thisLevel].distanceFromStartPosMid;
			}
			//nArcs = adderaArc_delayedPrefPath(thisLevel, tPos, nSpeedSettings, fuelQualityKvot);
		}
		else {
			// handle the channel separately
			if (*delayFactor < 0.0001) {
				// determine the delay factor for the pref path
				//errlog("ERROR! ADD factor delay after pref path NOT as it is now along prefPath\n");
				*delayFactor = eval_factorDelayedAlongArc(thisLevel, 1, nextLevel, pos2, model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos]);
				if (nextLevel < 0)
					*distArc = model.network.channel[-thisLevel - 1].distance_km;
			}
			//nArcs = adderaArc_channelOnPrefPath(-thisLevel - 1, model.network.channel[-thisLevel - 1].timeThroughChannel, fuelQualityKvot);
		}
		nArcs = genArcs_withDelay(thisLevel, pos1, nextLevel, pos2, tPos, nSpeedSettings, fuelQualityKvot, extraAreaCostKvot, distArc, *delayFactor);
	}
	else {
		model.functions.valuesNow.prefPathArc = 0;
		if (delayVersion == 1) {
			if (abs(posPrefP1 - pos1) > model.params.max_changeDirection * model.params.nMaxLev_posToDelayedPrefPath &&
				thisLevel < model.network.nPhysicalLevels - model.params.nMaxLev_posToDelayedPrefPath)
				return 0; // cannot get back to pref path quick enough
			if (posPrefP1 > pos1) {
				if (posPrefP2 < pos2)
					return 0; // do not add an arc to other side of pref path
				if (posPrefP1 - pos1 <= posPrefP2 - pos2)
					return 0; // do not add an arc that is not moving towards pref path
			}
			else { // posPrefP1 < pos1
				if (posPrefP2 > pos2)
					return 0; // do not add an arc to other side of pref path
				if (posPrefP1 - pos1 >= posPrefP2 - pos2)
					return 0; // do not add an arc that is not moving towards pref path
			}
		}
		if (*delayFactor < 0.0001) {
			if (thisLevel < 0 && nextLevel < 0) {
				// determine the delay factor for the arc towards pref path
				errlog("ERROR! Add a delay if on corridor with normal speed\n");
				*delayFactor = 1.0;
				*distArc = model.network.channel[-thisLevel - 1].distance_km;
			}
			else {
				if (thisLevel >= 0)
					tidp = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
				else
					tidp = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
				if (delayVersion < 4)
					*delayFactor = eval_factorDelayedAlongArc(thisLevel, pos1, nextLevel, pos2, tidp);
				else {
					if (delayVersion == 4)
						*delayFactor = eval_factorDelayedAlongArc_currSpeedDiff(thisLevel, pos1, nextLevel, pos2, tidp, &speedDiffCurrent);
					else
						*delayFactor = 1.0;
				}
			}
		}
		nArcs = genArcs_withDelay(thisLevel, pos1, nextLevel, pos2, tPos, nSpeedSettings, fuelQualityKvot, extraAreaCostKvot, distArc, *delayFactor);
		// 0 since this one is used for an earlier version, not in the more recent one
	}
	return nArcs;
}

int determineBastSpeedDelay_routeToEnd_eta(strDelayToEnd* routeToEnd, double startTidp) {
	if (abs(model.params.eta_h - startTidp) < 1.0)
		return 0; // close enough, no need to change the speed settings

	double timeDiff = model.params.eta_h - startTidp - routeToEnd->time;
	int i, arcNr, cNr, nMaxChangeIter = 10, i0, nChange;
	int deltaChange, nSpeedSettings, speedSettingNu;
	double calcSpeed, calmWaterSpeed, calmWaterSpeedNy, deltaTime, factor;

	for (i = 0; i < routeToEnd->nBVArcs; i++)
		model.delay.changedSpeed[i] = 0;

	for (i0 = 0; i0 < nMaxChangeIter; i0++) {
		nChange = 0;
		for (i = 0; i < routeToEnd->nBVArcs; i++) {
			arcNr = routeToEnd->BVArc[i];
			if (modelDelay.arc[arcNr].fromLevel < 0 && modelDelay.arc[arcNr].toLevel < 0) {
				cNr = -modelDelay.arc[arcNr].fromLevel - 1;
				if (model.network.channel[cNr].timeThroughChannel >= 0)
					continue; // cannot change this speed
			}
			speedSettingNu = getClosestSetting_fromBase(modelDelay.arc[arcNr].speedSetting, modelDelay.arc[arcNr].fromLevel, modelDelay.arc[arcNr].toLevel);
			if (timeDiff > 0) {
				deltaChange = -1; // lower the speed
				if (speedSettingNu + model.delay.changedSpeed[i] <= 0)
					continue; // cannot decrease the speed more
			}
			else {
				if (modelDelay.arc[arcNr].fromLevel >= 0)
					nSpeedSettings = model.functions.speedLevel[modelDelay.arc[arcNr].fromLevel].nShip_speedSettings;
				else {
					if (modelDelay.arc[arcNr].toLevel >= 0)
						nSpeedSettings = model.functions.speedChannelOut[-modelDelay.arc[arcNr].fromLevel - 1].nShip_speedSettings;
					else
						nSpeedSettings = model.functions.speedChannel[-modelDelay.arc[arcNr].fromLevel - 1].nShip_speedSettings;
				}

				deltaChange = 1; // increase the speed
				if (speedSettingNu + model.delay.changedSpeed[i] >= nSpeedSettings - 1)
					continue; // cannot increase the speed more
			}

			calcSpeed = modelDelay.arc[arcNr].distance / modelDelay.arc[arcNr].time;
			calmWaterSpeed = eval_calmWaterSpeed(speedSettingNu + model.delay.changedSpeed[i],
				modelDelay.arc[arcNr].fromLevel, modelDelay.arc[arcNr].toLevel);
			factor = calmWaterSpeed / calcSpeed;
			calmWaterSpeedNy = eval_calmWaterSpeed(speedSettingNu + model.delay.changedSpeed[i] + deltaChange,
				modelDelay.arc[arcNr].fromLevel, modelDelay.arc[arcNr].toLevel);
			deltaTime = modelDelay.arc[arcNr].time - modelDelay.arc[arcNr].distance / calmWaterSpeedNy * factor;
			if (abs(timeDiff) > abs(timeDiff + deltaTime)) {
				model.delay.changedSpeed[i] += deltaChange;
				timeDiff += deltaTime;
				nChange++;
			}
		}
		if (nChange == 0)
			break;
	}


	// errlog("ERROR! Fix route to end speed with eta, including if thisLevel < 0\n");
	return 0;
}

int addArcDelayToGeojson(FILE* filpek, strDelayToEnd* routeToEnd) {
	int i2, arcNr, fromLevel, toLevel, pos;

	for (i2 = 0; i2 < routeToEnd->nBVArcs; i2++) {
		arcNr = routeToEnd->BVArc[i2];
		if (i2 == 0) {
			fromLevel = modelDelay.arc[arcNr].fromLevel;
			if (fromLevel >= 0)
				fprintf(filpek, "[%.4lf,%.4lf]",
					model.network.physicalLev[fromLevel].point_x[modelDelay.arc[arcNr].fromPointNr],
					model.network.physicalLev[fromLevel].point_y[modelDelay.arc[arcNr].fromPointNr]);
			else {
				pos = modelDelay.arc[arcNr].fromPointNr;
				if (pos == 1)
					pos = model.network.channel[-fromLevel - 1].nPoints - 1;
				fprintf(filpek, "[%.4lf,%.4lf]",
					model.network.channel[-fromLevel - 1].point_x[pos],
					model.network.channel[-fromLevel - 1].point_y[pos]);
			}
		}
		toLevel = modelDelay.arc[arcNr].toLevel;
		if (toLevel >= 0)
			fprintf(filpek, ", [%.4lf,%.4lf]",
				model.network.physicalLev[toLevel].point_x[modelDelay.arc[arcNr].toPointNr],
				model.network.physicalLev[toLevel].point_y[modelDelay.arc[arcNr].toPointNr]);
		else {
			pos = modelDelay.arc[arcNr].toPointNr;
			if (pos == 1)
				pos = model.network.channel[-toLevel - 1].nPoints - 1;
			fprintf(filpek, ", [%.4lf,%.4lf]",
				model.network.channel[-toLevel - 1].point_x[pos],
				model.network.channel[-toLevel - 1].point_y[pos]);
		}
	}

	return 0;
}

int resetWaypointData() {

	model.waypointResult.arcNr = -1;

	model.waypointResult.arc_obj_cost = 0;
	model.waypointResult.diffTime = 0;
	model.waypointResult.hours = 0;
	model.waypointResult.checkHours = 0;
	model.waypointResult.distance_nm = 0;
	model.waypointResult.windF = 0;
	model.waypointResult.waveF = 0;
	model.waypointResult.currentF = 0;
	model.waypointResult.delayF = 0;

	model.waypointResult.bearingDiff = 0;

	model.waypointResult.nChangeBearingBetween = 0;
	model.waypointResult.nChange_lessXdegrees = 0;

	model.waypointResult.fuelMain_tot = 0;
	model.waypointResult.time_tot = 0;
	model.waypointResult.fuelAux_tot = 0;
	model.waypointResult.fuelEca = 0;

	model.waypointResult.maxWaveHeight = 0;
	model.waypointResult.maxWindSpeed = 0;

	model.waypointResult.windReal_lastKnown = 9999 * model.params.knots_to_km;
	sprintf(model.waypointResult.windDirReal_letters, "-");
	model.waypointResult.currentReal_lastKnown = 9999 * model.params.knots_to_km;
	model.waypointResult.waveDirReal_lastKnown = 9999;
	sprintf(model.waypointResult.waveDir_letters, "-");
	model.waypointResult.windDirReal = 0;
	model.waypointResult.currentDirReal = 0;


	model.waypointResult.worstStormValue = 0;

	model.waypointResult.fuelTransit_main = 0;
	model.waypointResult.fuelTransit_aux = 0;
	model.waypointResult.fuelWaiting_main = 0;
	model.waypointResult.fuelWaiting_aux = 0;
	model.waypointResult.transitTime = 0;
	model.waypointResult.waitingTime = 0;
	model.waypointResult.emissionWaiting_main = 0;
	model.waypointResult.emissionWaiting_aux = 0;
	model.waypointResult.fromLevel = model.waypointResult.toLevel;
	model.waypointResult.fromPointNr = model.waypointResult.toPointNr;

	return 0;
}

int sparaLastWaypoint(FILE* filpekG, int* posReport, std::string solName) {
	int lev1, lev2;

	if (model.waypointResult.arcNr == -1)
		return 0; // no waypoint to save

	if (*posReport > 0)
		fprintf(filpekG, ",\n");
	(*posReport)++;

	fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"dateUTC\":\"%s\",\n",
		model.waypointResult.dateUTC);
	fprintf(filpekG, "  \"full_date\":\"%s\",\n",
		model.waypointResult.full_Date);
	fprintf(filpekG, "\"solutionID\":\"%s\",\n", solName.c_str());

	fprintf(filpekG, "    \"arcNr\":%d,\n", model.waypointResult.arcNr);
	fprintf(filpekG, "    \"splitPos\":%d,\n", model.waypointResult.ii);
	fprintf(filpekG, "    \"arc obj cost\":%.2lf,\n", model.waypointResult.arc_obj_cost);
	fprintf(filpekG, "    \"level from\":%d,\n", model.waypointResult.fromLevel);
	fprintf(filpekG, "    \"posNodefrom\":%d,\n", model.waypointResult.fromPointNr);
	fprintf(filpekG, "    \"arcStart\":%d,\n", model.waypointResult.fromTime);
	fprintf(filpekG, "    \"accumTimeStart_h\":%.6lf,\n", model.waypointResult.accumTimeStart_h);

	if (model.waypointResult.diffTime < -99990)
		fprintf(filpekG, "    \"arrive-midTimeArriveDiff_h\":%.0lf, \n", model.waypointResult.diffTime);
	else
		fprintf(filpekG, "    \"arrive-midTimeArriveDiff_h\":%.2lf,\n", model.waypointResult.diffTime);

	fprintf(filpekG, "    \"level to\":%d,\n", model.waypointResult.toLevel);
	fprintf(filpekG, "    \"posNodeTo\":%d,\n", model.waypointResult.toPointNr);
	fprintf(filpekG, "    \"toTime\":%d,\n", model.waypointResult.toTime);

	fprintf(filpekG, "    \"hours\":%.2lf, \"checkHours\":%.3lf, \"distance_nm\":%.1lf,\n",
		model.waypointResult.hours, model.waypointResult.checkHours, model.waypointResult.distance_nm);

	fprintf(filpekG, "    \"WindF\":%.3lf, \"WaveF\":%.3lf, \"CurrentF\":%.3lf, \"DelayF\":%.3lf, \"allWeatherFactors\": %.3lf,\n",
		model.waypointResult.windF, model.waypointResult.waveF, model.waypointResult.currentF,
		model.waypointResult.delayF, model.waypointResult.windF + model.waypointResult.waveF + model.waypointResult.currentF +
		model.waypointResult.delayF);

	fprintf(filpekG, "    \"accumDistance_km\":%.2lf, \"distanceLeft_nm\":%.1lf,\n", model.waypointResult.accumDistance_km,
		model.waypointResult.distanceLeft_nm);

	fprintf(filpekG, "    \"Position_lat_lon\":\"%s\", \"bearing\":%.0lf,\n",
		model.waypointResult.fixPositionString_latlon, model.waypointResult.bearing);

	//if (model.waypointResult.bearingDiff == 1) {
		(model.functions.valuesNow.Wpt)++;
		fprintf(filpekG, "    \"Wpt\":%d,\n", model.functions.valuesNow.Wpt);
	//}

	fprintf(filpekG, "    \"nChangeBearingInbetweenNodes\":%d,\n", model.waypointResult.nChangeBearingBetween);
	fprintf(filpekG, "    \"nChangeBearing_lessThanXdegrees\":%d,\n", model.waypointResult.nChange_lessXdegrees);

	if (model.waypointResult.fromLevel >= 21)
		model.waypointResult.fromLevel = model.waypointResult.fromLevel;
	if(model.waypointResult.time_tot > 0.0001)
		fprintf(filpekG, "    \"fuelConsumptionMain_ton_day\":%.5lf, \"fuelConsumptionAux_ton_day\":%.5lf, \"fuelECA_ton\":%.3lf\n",
			model.waypointResult.fuelMain_tot * 24 / model.waypointResult.time_tot,
			model.waypointResult.fuelAux_tot * 24 / model.waypointResult.time_tot,
			model.waypointResult.fuelEca);
	else
		fprintf(filpekG, "    \"fuelConsumptionMain_ton_day\":%.5lf, \"fuelConsumptionAux_ton_day\":%.5lf, \"fuelECA_ton\":%.3lf\n",
			0.0, 0.0, model.waypointResult.fuelEca);

	lev1 = model.waypointResult.fromLevel;
	lev2 = model.waypointResult.toLevel;

	if ((lev1 >= 0 || lev2 >= 0) && model.waypointResult.speedSetting >= 0) {
		fprintf(filpekG, ", \"current_kts\":%.1lf\n", model.waypointResult.current);
		fprintf(filpekG, ", \"speedSetting\":%d, \"speedCalmWater_kts\":%.2lf, \"speedOnGround_kts\":%.2lf, \"rpm\":%.2lf,\n",
			model.waypointResult.speedSetting, model.waypointResult.calmWaterSpeed,
			model.waypointResult.speedOnGround, model.waypointResult.rpm);

		fprintf(filpekG, "    \"windSpeed_km_h\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
			model.waypointResult.windSpeed, model.waypointResult.relWindDir);
		fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
			model.waypointResult.waveHeight, model.waypointResult.wavePeriod,
			model.waypointResult.relWaveDir);
		//if (model.waypointResult.waveHeight > 3.3)
		//	model.waypointResult.waveHeight = model.waypointResult.waveHeight;
		if (model.waypointResult.waveHeight < model.functions.maxWaveHeight_warning)
			fprintf(filpekG, "    \"waveHeight_level\":\"normal\",\n");
		else
			fprintf(filpekG, "    \"waveHeight_level\":\"high\",\n");
		if (model.waypointResult.windSpeed < model.functions.maxWindSpeed_warning)
			fprintf(filpekG, "    \"windSpeed_level\":\"normal\",\n");
		else
			fprintf(filpekG, "    \"windSpeed_level\":\"high\",\n");

		if (model.nWeatherFiles > 10) {
			if (model.functions.pos_pressureSurface >= 0)
				fprintf(filpekG, "    \"pressureSurface\":%.3lf,\n", model.waypointResult.pressureSurface);
			if (model.functions.pos_pressureAir >= 0)
				fprintf(filpekG, "    \"pressureAir\":%.3lf,\n", model.waypointResult.pressureAir);
			if (model.functions.pos_precipitation >= 0)
				fprintf(filpekG, "    \"precipitation\":%.3lf,\n", model.waypointResult.precipitation);
			if (model.functions.pos_tempSea >= 0)
				fprintf(filpekG, "    \"tempSea\":%.3lf,\n", model.waypointResult.tempSea);
			if (model.functions.pos_tempAir >= 0)
				fprintf(filpekG, "    \"tempAir\":%.3lf,\n", model.waypointResult.tempAir);
			if (model.functions.pos_cloudCover >= 0)
				fprintf(filpekG, "    \"cloudCover\":%.3lf,\n", model.waypointResult.cloudCover);
			if (model.functions.pos_mdps >= 0)
				fprintf(filpekG, "    \"mdps\":%.3lf,\n", model.waypointResult.mdps);
			if (model.functions.pos_swell >= 0)
				fprintf(filpekG, "    \"swell\":%.3lf,\n", model.waypointResult.swell);

			fprintf(filpekG, "    \"maxWaveArc\":%.3lf,\n", model.waypointResult.maxWaveHeight);
			fprintf(filpekG, "    \"maxWindSpeedArc\":%.3lf,\n", model.waypointResult.maxWindSpeed);
		}

		if (model.waypointResult.windReal_lastKnown < 100000) {
			fprintf(filpekG, "    \"windSpeedReal_knots\":%.1lf, \"windDirection_degrees\":%.0lf, \"windDir_letters\":\"%s\",\n",
				model.waypointResult.windReal_lastKnown / model.params.knots_to_km, model.waypointResult.windDirReal,
				model.waypointResult.windDirReal_letters);
		}
		if (model.waypointResult.currentReal_lastKnown < 100000) {
			fprintf(filpekG, "    \"currentReal_knots\":%.3lf, \"currentDirection_degrees\":%.0lf,\n",
				model.waypointResult.currentReal_lastKnown / model.params.knots_to_km, model.waypointResult.currentDirReal);
		}
		if (model.waypointResult.waveDirReal_lastKnown < 100000) {
			fprintf(filpekG, "    \"waveDirection_degrees\":%.0lf,\"waveDir_letters\":\"%s\",\n",
				model.waypointResult.waveDirReal_lastKnown, model.waypointResult.waveDir_letters);
		}

#ifdef NAZANIN_SAFETY
		fprintf(filpekG, "    \"bow slamming p\":%.3lf, \"green water p\":%.3lf,\n    \"dynamic instability\":%.3lf,\n",
			model.waypointResult.bowSlam, model.waypointResult.greenWater,
			model.waypointResult.dynamicStability);
		fprintf(filpekG, "    \"rolling p\":%.3lf, \"surfRiding p\":%.3lf,\n",
			model.waypointResult.rolling, model.waypointResult.surfRiding);
#endif
		fprintf(filpekG, "    \"max iceCover\":%.3lf",
			model.waypointResult.iceCover_max);
		if (model.waypointResult.forecastType > 0.5) {
			if (model.params.hindCast == 0)
				fprintf(filpekG, ", \"forecastType\":\"Ext. Hist\"");
			else
				fprintf(filpekG, ", \"forecastType\":\"Past Hist %.5lf\"", model.waypointResult.forecastType);
		}
		else {
			if (model.params.hindCast == 0)
				fprintf(filpekG, ", \"forecastType\":\"Fcst\"");
			else {
				if (model.waypointResult.forecastType > 0.05)
					fprintf(filpekG, ", \"forecastType\":\"Missing Hist %.5lf\"", model.waypointResult.forecastType);
				else
					fprintf(filpekG, ", \"forecastType\":\"Hist %.5lf\"", model.waypointResult.forecastType);
			}
		}
	}
	else {
		if (lev1 < 0 && lev2 < 0) {
			if (model.network.channel[-lev1 - 1].type == 1) { // tss
				fprintf(filpekG, ", \"current_kts\":%.1lf\n",
					model.waypointResult.current);
				fprintf(filpekG, ", \"speedSetting\":%d, \"speedCalmWater_kts\":%.2lf, \"speedOnGround_kts\":%.2lf, \"rpm\":%.2lf,\n",
					model.waypointResult.speedSetting, model.waypointResult.calmWaterSpeed,
					model.waypointResult.speedOnGround, model.waypointResult.rpm);
				fprintf(filpekG, "    \"windSpeed_km_h\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
					model.waypointResult.windSpeed, model.waypointResult.relWindDir);
				fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
					model.waypointResult.waveHeight, model.waypointResult.wavePeriod,
					model.waypointResult.relWaveDir);
				if (model.waypointResult.waveHeight < model.functions.maxWaveHeight_warning)
					fprintf(filpekG, "    \"waveHeight_level\":\"normal\",\n");
				else
					fprintf(filpekG, "    \"waveHeight_level\":\"high\",\n");
				if (model.waypointResult.windSpeed < model.functions.maxWindSpeed_warning)
					fprintf(filpekG, "    \"windSpeed_level\":\"normal\",\n");
				else
					fprintf(filpekG, "    \"windSpeed_level\":\"high\",\n");

				if (model.nWeatherFiles > 10) {
					if (model.functions.pos_pressureSurface >= 0)
						fprintf(filpekG, "    \"pressureSurface\":%.3lf,\n", model.waypointResult.pressureSurface);
					if (model.functions.pos_pressureAir >= 0)
						fprintf(filpekG, "    \"pressureAir\":%.3lf,\n", model.waypointResult.pressureAir);
					if (model.functions.pos_precipitation >= 0)
						fprintf(filpekG, "    \"precipitation\":%.3lf,\n", model.waypointResult.precipitation);
					if (model.functions.pos_tempSea >= 0)
						fprintf(filpekG, "    \"tempSea\":%.3lf,\n", model.waypointResult.tempSea);
					if (model.functions.pos_tempAir >= 0)
						fprintf(filpekG, "    \"tempAir\":%.3lf,\n", model.waypointResult.tempAir);
					if (model.functions.pos_cloudCover >= 0)
						fprintf(filpekG, "    \"cloudCover\":%.3lf,\n", model.waypointResult.cloudCover);
					if (model.functions.pos_mdps >= 0)
						fprintf(filpekG, "    \"mdps\":%.3lf,\n", model.waypointResult.mdps);
					if (model.functions.pos_swell >= 0)
						fprintf(filpekG, "    \"swell\":%.3lf,\n", model.waypointResult.swell);

					fprintf(filpekG, "    \"maxWaveArc\":%.3lf,\n", model.waypointResult.maxWaveHeight);
					fprintf(filpekG, "    \"maxWindSpeedArc\":%.3lf,\n", model.waypointResult.maxWindSpeed);
				}

				if (model.waypointResult.windReal_lastKnown < 100000) {
					fprintf(filpekG, "    \"windSpeedReal_knots\":%.1lf, \"windDirection_degrees\":%.0lf, \"windDir_letters\":\"%s\",\n",
						model.waypointResult.windReal_lastKnown / model.params.knots_to_km, model.waypointResult.windDirReal,
						model.waypointResult.windDirReal_letters);
				}
				if (model.waypointResult.currentReal_lastKnown < 100000) {
					fprintf(filpekG, "    \"currentReal_knots\":%.3lf, \"currentDirection_degrees\":%.0lf,\n",
						model.waypointResult.currentReal_lastKnown / model.params.knots_to_km, model.waypointResult.currentDirReal);
				}
				if (model.waypointResult.waveDirReal_lastKnown < 100000) {
					fprintf(filpekG, "    \"waveDirection_degrees\":%.0lf,\"waveDir_letters\":\"%s\",\n",
						model.waypointResult.waveDirReal_lastKnown, model.waypointResult.waveDir_letters);
				}

#ifdef NAZANIN_SAFETY
				fprintf(filpekG, "    \"bow slamming p\":%.3lf, \"green water p\":%.3lf,\n    \"dynamic instability\":%.3lf,\n",
					model.waypointResult.bowSlam, model.waypointResult.greenWater,
					model.waypointResult.dynamicStability);
				fprintf(filpekG, "    \"rolling p\":%.3lf, \"surfRiding p\":%.3lf,\n",
					model.waypointResult.rolling, model.waypointResult.surfRiding);
#endif
				fprintf(filpekG, "    \"max iceCover\":%.3lf",
					model.waypointResult.iceCover_max);
				if (model.waypointResult.forecastType > 0.5) {
					if (model.params.hindCast == 0)
						fprintf(filpekG, ", \"forecastType\":\"Ext. Hist\"");
					else
						fprintf(filpekG, ", \"forecastType\":\"Past Hist %.5lf\"", model.waypointResult.forecastType);
				}
				else {
					if (model.params.hindCast == 0)
						fprintf(filpekG, ", \"forecastType\":\"Fcst\"");
					else {
						if (model.waypointResult.forecastType > 0.05)
							fprintf(filpekG, ", \"forecastType\":\"Missing Hist %.5lf\"", model.waypointResult.forecastType);
						else
							fprintf(filpekG, ", \"forecastType\":\"Hist %.5lf\"", model.waypointResult.forecastType);
					}
				}
			}
			else {
				fprintf(filpekG, ", \"corridorID\":\"%s\"",
					model.network.channel[-lev1 - 1].ID);
				fprintf(filpekG, ",\n\"fuelTransit_main\":%.3lf", model.waypointResult.fuelTransit_main);
				fprintf(filpekG, ", \"fuelTransit_aux\":%.3lf", model.waypointResult.fuelTransit_aux);
				fprintf(filpekG, ",\n\"fuelWaiting_main\":%.3lf", model.waypointResult.fuelWaiting_main);
				fprintf(filpekG, ", \"fuelWaiting_aux\":%.3lf", model.waypointResult.fuelWaiting_aux);

				fprintf(filpekG, ",\n\"emissionWaiting_main\":%.3lf", model.waypointResult.emissionWaiting_main);
				fprintf(filpekG, ", \"emissionWaiting_aux\":%.3lf", model.waypointResult.emissionWaiting_aux);

				fprintf(filpekG, ",\n\"transitTime\":%.3lf", model.waypointResult.transitTime);
				fprintf(filpekG, ", \"waitingTime\":%.3lf", model.waypointResult.waitingTime);
			}
		}
		else {
			fprintf(filpekG, ", \"corridorID\":\"extension of corridor since not physically feasible area around the corridor\"");
		}
	}
	fprintf(filpekG, ", \"worstStormValue\":%.3lf", model.waypointResult.worstStormValue);

	fprintf(filpekG, "},\n");

	fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n",
		getCorrect_longitude(model.waypointResult.xCoord), model.waypointResult.yCoord);

	resetWaypointData();
	return 0;
}

int genSplitsArcNew2(int arcNr, spherical::Point p1, spherical::Point p2, int prefPath) {
	double wantedTimeLength = 4.0;
	double distNu, wantedDist, kvot;
	int nSplit = 0, nWantedSplits, i3b;
	int i, cNr, level, ii, startPos, endPos;
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
		else {
			if (nWantedSplits > model.network.nMaxSplits - 1)
				nWantedSplits = model.network.nMaxSplits - 1;
		}
		if (model.arc[arcNr].speedSetting == -1)
			wantedDist = model.arc[arcNr].distance * 1000.0;
		else
			wantedDist = model.arc[arcNr].distance / nWantedSplits * 1000.0;
		//printf("--- arcNr %d nWantedSplitsPrefPath %d, wantedDist %.2lf level %d\n", arcNr, nWantedSplits, wantedDist, level);

		if (model.arc[arcNr].toLevel < 0)
			arcNr = arcNr;

		model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
		model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
		//printf("/// solPathCoord pos %d added %.3lf %.3lf\n", model.network.nCoords,
		//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
		//printf("distLastSplit %.2lf wantedDist %.2lf\n", distLastSplit, wantedDist);
		model.network.posSplitCoord[nSplit] = model.network.nCoords;
		//printf("## adderar startSplit nr %d fran pos %d i xy %.3lf %.3lf\n", nSplit, model.network.nCoords,
		//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
		model.network.startKvot[nSplit] = 0.0;
		model.network.endKvot[nSplit] = distTot;
		nSplit++;
		distLastSplit = 0;
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
			for (i = 0; i < nWantedSplits; i++) {
				if (distTmp > 1.7 * wantedDist) {
					distTot += wantedDist;
					p3 = p1.destinationPoint(distTot, bearing);
					x = p3.longitude().degrees();
					y = p3.latitude().degrees();
					model.network.xCoord[model.network.nCoords] = x;
					model.network.yCoord[model.network.nCoords] = y;
					model.network.posSplitCoord[nSplit] = model.network.nCoords;
					(model.network.nCoords)++;
					distTmp -= wantedDist;
					model.network.endKvot[nSplit] = distTot;
					nSplit++;
				}
				else {
					distTot += distTmp;
					//x = pointLast.longitude().degrees();
					//y = pointLast.latitude().degrees();
					//model.network.xCoord[model.network.nCoords] = x;
					//model.network.yCoord[model.network.nCoords] = y;
					//model.network.posSplitCoord[i] = model.network.nCoords;
					//(model.network.nCoords)++;
					//nSplit++;
					break;
				}
			}
			pointLast = model.network.physicalLev[level].preferredPathPoint[startPos];
			//startPos++;
		}
		endPos = model.network.physicalLev[level].npreferredPathPoints;

		if (model.arc[arcNr].toLevel < 0)
			endPos = model.network.channel[-model.arc[arcNr].toLevel - 1].preferredPathPoint_posConnectTo + 1;

		for (int i3 = startPos; i3 < endPos; i3++) {
			//printf("prefPath i3 %d lon/lat %.3lf %.3lf\n", i3,
			//	model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees(),
			//	model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees());
			distTmp = pointLast.distanceTo(model.network.physicalLev[level].preferredPathPoint[i3]);
			distBas = distTmp;
			distTot += distTmp;
			//printf("i3 %d distTmp %.2lf distLastSplit %.3lf\n", i3, distTmp, distLastSplit);
			for (ii = 0; ii < nWantedSplits + 1; ii++) {
				//printf("ii %d distLastSplit %.2lf distTmp %.2lf\n", ii, distLastSplit, distTmp);
				if (distLastSplit + distTmp >= 1.7 * wantedDist && (distTot < distArc - 10 || distTmp > 0.5 * wantedDist)) {
					distLastSplit = 0;
					if (distBas < wantedDist)
						pointLast = pointLast.destinationPoint(distBas, pointLast.bearingTo(model.network.physicalLev[level].preferredPathPoint[i3]));
					else
						pointLast = pointLast.destinationPoint(wantedDist, pointLast.bearingTo(model.network.physicalLev[level].preferredPathPoint[i3]));
					// if (nPkter == 0) {
					model.network.yCoord[model.network.nCoords] = pointLast.latitude().degrees();
					model.network.xCoord[model.network.nCoords] = pointLast.longitude().degrees();
					//printf("/// solPathCoord1 pos %d added %.3lf %.3lf\n", model.network.nCoords,
					//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
					//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii %d distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, ii, distTmp, x[nPkter], y[nPkter], __LINE__);
					model.network.posSplitCoord[nSplit] = model.network.nCoords;
					model.network.startKvot[nSplit] = (distTot - distTmp + wantedDist) / distArc;
					model.network.endKvot[nSplit] = distTot - distTmp + wantedDist;
					//model.network.endKvot[nSplit - 1] = (distTot - distTmp + wantedDist) / distArc;
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

			model.network.yCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees();
			model.network.xCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees();
			//printf("/// solPathCoord2 pos %d added %.3lf %.3lf\n", model.network.nCoords,
			//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords]);
			//printf("distLastSplit %.2lf distTmp %.2lf wantedDist %.2lf distTot %.2lf distArc %.2lf\n", distLastSplit, distTmp, wantedDist, distTot, distArc);
			if (distLastSplit >= wantedDist * 0.8 && distTot < distArc - wantedDist * 0.3) {
				model.network.posSplitCoord[nSplit] = model.network.nCoords;
				model.network.startKvot[nSplit] = distTot / distArc;
				model.network.endKvot[nSplit] = distTot;
				//model.network.endKvot[nSplit - 1] = distTot / distArc;
				//printf("## adderar split nr %d fran pos %d i xy %.3lf %.3lf distTot %.2lf distArc %.2lf\n", nSplit, model.network.nCoords,
				//	model.network.xCoord[model.network.nCoords], model.network.yCoord[model.network.nCoords], distTot, distArc);
				nSplit++;
				distLastSplit = 0;
			}
			(model.network.nCoords)++;
			pointLast = model.network.physicalLev[level].preferredPathPoint[i3];
		}
		//if (nSplit > 0)
		//	model.network.endKvot[nSplit - 1] = distTot;
		if (distTot < 0.0001)
			distTot = 0.0001;
		for (i3b = 0; i3b < nSplit; i3b++) {
			if (i3b > 0)
				model.network.startKvot[i3b] = model.network.endKvot[i3b] / distTot;
			if (i3b > 0)
				model.network.endKvot[i3b - 1] = model.network.endKvot[i3b] / distTot;
		}

		if (model.arc[arcNr].toLevel < 0) {
			distTmp = pointLast.distanceTo(p2);
			bearing = pointLast.bearingTo(p2);
			for (i = 0; i < nWantedSplits; i++) {
				if (distTmp > 1.7 * wantedDist) {
					distTot += wantedDist;
					p3 = p1.destinationPoint(distTot, bearing);
					x = p3.longitude().degrees();
					y = p3.latitude().degrees();
					model.network.xCoord[model.network.nCoords] = x;
					model.network.yCoord[model.network.nCoords] = y;
					model.network.posSplitCoord[nSplit] = model.network.nCoords;
					(model.network.nCoords)++;
					distTmp -= wantedDist;
					nSplit++;
				}
				else {
					//distTot += distTmp;
					//x = model.network.channel[-model.arc[arcNr].toLevel - 1].point_x[0];
					//y = model.network.channel[-model.arc[arcNr].toLevel - 1].point_y[0];
					//model.network.xCoord[model.network.nCoords] = x;
					//model.network.yCoord[model.network.nCoords] = y;
					//model.network.posSplitCoord[nSplit] = model.network.nCoords;
					//(model.network.nCoords)++;
					//nSplit++;
					break;
				}
			}
		}

		model.network.endKvot[nSplit - 1] = 1.0;
		return nSplit;
	}

	// if corridor
	if (model.arc[arcNr].fromLevel < 0 && model.arc[arcNr].toLevel < 0 && model.arc[arcNr].fromLevel == model.arc[arcNr].toLevel) {
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

int identify_nChanges_lessXdegrees(int pos1, int pos2) {
	int i, nChanges = 0;
	double bearingNu, bearing = -1000, absDiff;
	for (i = pos1; i < pos2; i++) {
		bearingNu = calcBearingFromToCoords(model.network.yCoord[i],
			model.network.xCoord[i], model.network.yCoord[i + 1],
			model.network.xCoord[i + 1]);
		if (i == pos1)
			bearing = bearingNu;
		//printf("i %d xy %.3lf %.3lf %.3lf %.3lf bearingNu %.2lf bearing %.2lf\n", i, 
		//	model.network.xCoord[i],
		//	model.network.yCoord[i], model.network.xCoord[i + 1],
		//	model.network.yCoord[i + 1], bearingNu, bearing);
		absDiff = getDiff_anglesDegrees(bearingNu, bearing);
		if (absDiff > model.params.report_minBearingDiff) {
			//printf("***** new direction ******\n");
			nChanges++;
			bearing = bearingNu;
		}
	}
	return nChanges;
}

int addExtraWeatherInfoToGeojson2() {
	if (model.functions.pos_pressureSurface >= 0)
		model.waypointResult.pressureSurface = model.functions.valuesNow.pressureSurface;
	if (model.functions.pos_pressureAir >= 0)
		model.waypointResult.pressureAir = model.functions.valuesNow.pressureAir;
	if (model.functions.pos_precipitation >= 0)
		model.waypointResult.precipitation = model.functions.valuesNow.precipitation;
	if (model.functions.pos_tempSea >= 0)
		model.waypointResult.tempSea = model.functions.valuesNow.tempSea;
	if (model.functions.pos_tempAir >= 0)
		model.waypointResult.tempAir = model.functions.valuesNow.tempAir;
	if (model.functions.pos_cloudCover >= 0)
		model.waypointResult.cloudCover = model.functions.valuesNow.cloudCover;
	if (model.functions.pos_mdps >= 0)
		model.waypointResult.mdps = model.functions.valuesNow.mdps;
	if (model.functions.pos_swell >= 0)
		model.waypointResult.swell = model.functions.valuesNow.swell;
	return 0;
}

int addExtraWeatherInfoToGeojson(FILE* filpekG) {
	if (model.functions.pos_pressureSurface >= 0)
		fprintf(filpekG, "    \"pressureSurface\":%.3lf,\n", model.functions.valuesNow.pressureSurface);
	if (model.functions.pos_pressureAir >= 0)
		fprintf(filpekG, "    \"pressureAir\":%.3lf,\n", model.functions.valuesNow.pressureAir);
	if (model.functions.pos_precipitation >= 0)
		fprintf(filpekG, "    \"precipitation\":%.3lf,\n", model.functions.valuesNow.precipitation);
	if (model.functions.pos_tempSea >= 0)
		fprintf(filpekG, "    \"tempSea\":%.3lf,\n", model.functions.valuesNow.tempSea);
	if (model.functions.pos_tempAir >= 0)
		fprintf(filpekG, "    \"tempAir\":%.3lf,\n", model.functions.valuesNow.tempAir);
	if (model.functions.pos_cloudCover >= 0)
		fprintf(filpekG, "    \"cloudCover\":%.3lf,\n", model.functions.valuesNow.cloudCover);
	if (model.functions.pos_mdps >= 0)
		fprintf(filpekG, "    \"mdps\":%.3lf,\n", model.functions.valuesNow.mdps);
	if (model.functions.pos_swell >= 0)
		fprintf(filpekG, "    \"swell\":%.3lf,\n", model.functions.valuesNow.swell);
	return 0;
}

void fixDirectionLetters(double dir, char* namn, int alt) {
	if (alt == 1) {
		if (dir <= 180)
			dir += 180;
		else
			dir -= 180;
	}
	if (dir < 0)
		dir += 360;
	if (dir > 360)
		dir -= 360;

	if (dir <= 11.25 || dir > 360 - 11.25)
		sprintf(namn, "E");
	else if (dir <= 22.5 + 11.25)
		sprintf(namn, "ENE");
	else if (dir <= 45 + 11.25)
		sprintf(namn, "NE");
	else if (dir <= 67.5 + 11.25)
		sprintf(namn, "NNE");
	else if (dir <= 90 + 11.25)
		sprintf(namn, "N");
	else if (dir <= 112.5 + 11.25)
		sprintf(namn, "NNW");
	else if (dir <= 135 + 11.25)
		sprintf(namn, "NW");
	else if (dir <= 157.5 + 11.25)
		sprintf(namn, "WNW");
	else if (dir <= 180 + 11.25)
		sprintf(namn, "W");
	else if (dir <= 202.5 + 11.25)
		sprintf(namn, "WSW");
	else if (dir <= 225 + 11.25)
		sprintf(namn, "SW");
	else if (dir <= 247.5 + 11.25)
		sprintf(namn, "SSW");
	else if (dir <= 270 + 11.25)
		sprintf(namn, "S");
	else if (dir <= 292.5 + 11.25)
		sprintf(namn, "SSE");
	else if (dir <= 315 + 11.25)
		sprintf(namn, "SE");
	else
		sprintf(namn, "ESE");

	/*
		if (dir <= 11.25 || dir > 360 - 11.25)
			sprintf(namn, "N");
		else if (dir <= 22.5 + 11.25)
			sprintf(namn, "NNE");
		else if (dir <= 45 + 11.25)
			sprintf(namn, "NE");
		else if (dir <= 77.5 + 11.25)
			sprintf(namn, "ENE");
		else if (dir <= 90 + 11.25)
			sprintf(namn, "E");
		else if (dir <= 112.5 + 11.25)
			sprintf(namn, "ESE");
		else if (dir <= 135 + 11.25)
			sprintf(namn, "SE");
		else if (dir <= 157.5 + 11.25)
			sprintf(namn, "SSE");
		else if (dir <= 180 + 11.25)
			sprintf(namn, "S");
		else if (dir <= 202.5 + 11.25)
			sprintf(namn, "SSW");
		else if (dir <= 225 + 11.25)
			sprintf(namn, "SW");
		else if (dir <= 247.5 + 11.25)
			sprintf(namn, "SSW");
		else if (dir <= 270 + 11.25)
			sprintf(namn, "W");
		else if (dir <= 292.5 + 11.25)
			sprintf(namn, "WNW");
		else if (dir <= 315 + 11.25)
			sprintf(namn, "NW");
		else
			sprintf(namn, "NNW");
	*/

}

int plotPathTimeVisuellt(int arcNr, double time1, double time2) {
	double timeDiff, distint;
	int timeInt, i, lev1, lev2, p1, p2;
	double x1, y1, x2, y2;

	lev1 = model.arc[arcNr].fromLevel;
	p1 = model.arc[arcNr].fromPointNr;
	lev2 = model.arc[arcNr].toLevel;
	p2 = model.arc[arcNr].toPointNr;
	if (lev2 < model.network.nPhysicalLevels) {
		if (lev1 >= 0) {
			x1 = model.network.physicalLev[lev1].point_x[p1];
			y1 = model.network.physicalLev[lev1].point_y[p1];
		}
		else {
			if (p1 == 1)
				p1 = model.network.channel[-lev1 - 1].nPoints - 1;
			x1 = model.network.channel[-lev1 - 1].point_x[p1];
			y1 = model.network.channel[-lev1 - 1].point_y[p1];
		}
		if (lev2 >= 0) {
			x2 = model.network.physicalLev[lev2].point_x[p2];
			y2 = model.network.physicalLev[lev2].point_y[p2];
		}
		else {
			if (p2 == 1)
				p2 = model.network.channel[-lev2 - 1].nPoints - 1;
			x2 = model.network.channel[-lev2 - 1].point_x[p2];
			y2 = model.network.channel[-lev2 - 1].point_y[p2];
		}

		double xDiff = x2 - x1;
		double yDiff = y2 - y1;

		timeDiff = time2 - time1;
		timeInt = (int)timeDiff * model.params.simulateTimeVisually_nIntHour;
		if (timeInt < 2)
			timeInt = 2;
		timeDiff /= timeInt;
		xDiff /= timeInt;
		yDiff /= timeInt;
		for (i = 1; i < timeInt; i++)
			plotNodeTimeVisuellt(time1 + i * timeDiff, x1 + i * xDiff, y1 + i * yDiff);
	}

	return 0;
}

int addPositionDataToReport(FILE* filpekG, int* posReport, int arcNr, int startSlutArc, double* timeExact, std::string solName, int useFixCalmWaterSpeed, int iter) {
	int lev1, lev2, pointNr1, pointNr2, timmar, minuter, sekunder;
	int nChangeBearingBetween, nChange_lessXdegrees, tidIntForecast;
	double fuelMan_tot, fuelAux_tot, time_tot;
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;

	double x, y, bearing, speedOnGround, fuel_day, fuel_dayCheck, diff, diffTime, fuel_dayAux;
	spherical::Point p1, p2, p3;

	//if (*posReport > 0 && filpekG != NULL && model.arc[arcNr].distance > 0.001)
	//	fprintf(filpekG, ",\n ");

	lev1 = model.arc[arcNr].fromLevel;
	lev2 = model.arc[arcNr].toLevel;
	pointNr1 = model.arc[arcNr].fromPointNr;
	pointNr2 = model.arc[arcNr].toPointNr;
	//printf("arcNr %d levels %d %d pointNr %d %d nPhysicalLevels %d\n", arcNr, lev1, lev2, pointNr1, pointNr2, model.network.nPhysicalLevels);

	if (lev2 == -2)
		lev2 = lev2;
	int prefPath = 0;
	if (arcNr == 26)
		arcNr = arcNr;
	if (lev1 >= 0 && lev2 >= 0) {
		if (pointNr1 == model.params.preferredPathOrtoPos[lev1] && pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1
			&& (model.params.preferredPathStraightLineFeasibleFrom[lev1] == 0 || model.params.max_changeDirection == 0 || iter == 1)) {
			prefPath = 1;
		}

	}
	else {
		if (lev1 >= 0) {
			if ((model.network.channel[-lev2 - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0 || iter == 1) &&
				pointNr1 == model.params.preferredPathOrtoPos[lev1] && model.network.channel[-lev2 - 1].preferredPathPoint_posConnectTo >= 0)
				prefPath = 1;
		}
		if (lev2 >= 0) {
			if ((model.network.channel[-lev1 - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0 || iter == 1) &&
				pointNr2 == model.params.preferredPathOrtoPos[lev2] && model.network.channel[-lev1 - 1].preferredPathPoint_posConnectFrom >= 0)
				prefPath = 1;
		}
	}
	if (lev1 == 60)
		arcNr = arcNr;

	if (arcNr == 0)
		arcNr = arcNr;
	double fuelQualityKvot = 0;
	if (model.arc[arcNr].distance > 0.001) {
		//fuelQualityKvot = get_fuelQualityKvot(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
		//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr);
		fuelQualityKvot = 1 - get_extraAreaKvot(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
			model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, 0, 1);
	}
	strSpeed speedSetting;
	if (lev1 >= 0) {
		speedSetting = model.functions.speedLevel[lev1];
		p1 = model.network.physicalLev[lev1].point[pointNr1];
		if (lev2 >= 0) {
			if (lev2 < model.network.nPhysicalLevels) {
				p2 = model.network.physicalLev[lev2].point[pointNr2];
				if (startSlutArc == 0) {
					if (prefPath == 1)
						calcWeatherPosAlongpreferredPathArc(p1, lev1);
					else
						calcWeatherPosAlongArc(p1, p2);
				}
			}
			else
				p2 = p1;
		}
		else {
			p2 = model.network.channel[-lev2 - 1].point[0];
			if (startSlutArc == 0) {
				//if(prefPath == 1)
				//	calcWeatherPosAlongpreferredPathArc(p1, lev1); probably add lev2 here as well;
				//else
				// model.network.physicalLev[i1].requirePrefPathFeasible
				if (prefPath == 0)
					calcWeatherPosAlongArc(p1, p2);
				else
					calcWeatherPosAlongpreferredPathArc_connectChannel(p1, lev1, p2, lev2);
			}
		}
	}
	else {
		if (lev2 >= 0) {
			speedSetting = model.functions.speedChannelOut[-lev1 - 1];
			p1 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
			if (lev2 < model.network.nPhysicalLevels)
				p2 = model.network.physicalLev[lev2].point[pointNr2];
			else
				p2 = p1;
			if (startSlutArc == 0) {
				if (prefPath == 0)
					calcWeatherPosAlongArc(p1, p2);
				else
					calcWeatherPosAlongpreferredPathArc_connectChannel(p1, lev1, p2, lev2);
			}
		}
		else {
			speedSetting = model.functions.speedChannel[-lev1 - 1];
			if (lev1 == lev2) {
				p1 = model.network.channel[-lev1 - 1].point[0];
				p2 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
				//printGlobal = 1;
				if (startSlutArc == 0)
					calcWeatherPosAlongChannel(-lev1 - 1);
			}
			else {
				// between two corridors
				p1 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
				p2 = model.network.channel[-lev2 - 1].point[0];
				//printGlobal = 1;
				if (startSlutArc == 0)
					calcWeatherPosAlongArc(p1, p2);
			}
		}
	}

	char* startTime = NULL;

	if (filpekG != NULL) {
		startTime = (char*)malloc2(256 * sizeof(char));
		time_t rawtime;
		time(&rawtime);
		tmBas = *localtime(&rawtime);
		tmBas.tm_year = model.params.startYear - 1900;
		tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
		tmBas.tm_mday = model.params.startDay_nr;
		timmar = (int)(*timeExact); // *24;
		tmBas.tm_hour = model.params.startHour + timmar;
		minuter = (int)((*timeExact - timmar) * 60.0);
		tmBas.tm_min = model.params.startMinute + minuter;
		sekunder = (int)((*timeExact - timmar - minuter / 60.0) * 60.0);
		tmBas.tm_sec = sekunder;
		time_t test = mktime(&tmBas);
		if (test == -1) {
			printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
				tmBas.tm_year,
				tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
			if (model.params.failedTime == 0)
				postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
			model.params.failedTime = 1;
		}
	}

	double timeCheck = model.arc[arcNr].time, accumTime = 0, startKvot, endKvot;
	int nSplit, ii;

	if (arcNr == 60)
		arcNr = arcNr;
	if (lev1 == 34)
		lev1 = lev1;
	if (lev2 < model.network.nPhysicalLevels) {
		// nSplit = genSplitsArc(arcNr, p1, p2, prefPath);
		if (arcNr == 3)
			arcNr = arcNr;
		nSplit = genSplitsArcNew2(arcNr, p1, p2, prefPath);
	}
	else { // last arc or a channel
		model.network.startKvot[0] = 0.0;
		model.network.endKvot[0] = 1.0;
		nSplit = 1;
		model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
		model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
		model.network.posSplitCoord[0] = model.network.nCoords;
		(model.network.nCoords)++;
	}

	FILE* filpek10 = NULL;
	char* namn = NULL;
	if (model.network.nMaxSplits == 100000) {
		namn = (char*)malloc2(256 * sizeof(char));
		sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
		filpek10 = fopen(namn, "a+");
		fprintf(filpek10, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%d\t%.3lf\t",
			arcNr, nSplit, -1, -1, -1, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
			model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime, model.arc[arcNr].totCost,
			-1, model.arc[arcNr].distance, model.arc[arcNr].emission, model.arc[arcNr].fuelBase,
			model.arc[arcNr].safetyBase, model.arc[arcNr].speedSetting, model.arc[arcNr].time);
	}

	//printf("arcNr %d nSplit %d from xy %.3lf %.3lf to %.3lf %.3lf\n", arcNr, nSplit,
	//	p1.longitude().degrees(), p1.latitude().degrees(), p2.longitude().degrees(), p2.latitude().degrees());
	//errlog("arcNr %d nSplit %d from xy %.3lf %.3lf to %.3lf %.3lf\n", arcNr, nSplit,
	//	p1.longitude().degrees(), p1.latitude().degrees(), p2.longitude().degrees(), p2.latitude().degrees());

	if (model.arc[arcNr].fromLevel == 7)
		arcNr = arcNr;
	if (arcNr == 39216)
		arcNr = arcNr;
	if (model.arc[arcNr].fromLevel == -1)
		arcNr = arcNr;

	double calmWaterSpeed, delayFactor, timeOld = *timeExact;
	if(USE_ARC_TIME_EXACT == 1) {
		if (arcNr >= 0)
			model.functions.valuesNow.deltaArcStart = model.arc[arcNr].fromTime * model.params.tIndexGerH - timeOld;
		else
			model.functions.valuesNow.deltaArcStart = 0;
	}

	if (arcNr == 17)
		arcNr = arcNr;

	//if (*timeExact >= model.network.tidp_startHistoricDataOnly) {
	if (model.arc[arcNr].fromTime * model.params.tIndexGerH >= model.network.tidp_startHistoricDataOnly) {
		//if (model.arc[arcNr].fromLevel >= 0 || model.arc[arcNr].toLevel >= 0) {
		calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
		if (model.arc[arcNr].distance > 0.01)
			delayFactor = calmWaterSpeed * model.arc[arcNr].time / model.arc[arcNr].distance;
		else
			delayFactor = 1.0;
		//}
		//else {
		//	delayFactor = -1.0;
		//	errlog("OBS! Change this when you have time to use delay if variable speed in corridor\n");
		//}
	}
	else
		delayFactor = -1;

	if (arcNr == 17)
		arcNr = arcNr;

	for (ii = 0; ii < nSplit; ii++) {

		if (model.functions.valuesNow.sparaWaypointPos == -1)
			model.functions.valuesNow.sparaWaypoint = 1; // save all waypoints
		else {
			model.functions.valuesNow.sparaWaypoint = 0;
			if (model.functions.valuesNow.sparaWaypointPos == 2) {
				if (nSplit > 1) {
					if (ii == 1)
						model.functions.valuesNow.sparaWaypoint = 1;
				}
				else
					model.functions.valuesNow.sparaWaypoint = 1;
			}
			if (model.functions.valuesNow.sparaWaypointPos == 1 && ii == 0) {
				model.functions.valuesNow.sparaWaypoint = 1;
			}
		}
		if (model.functions.valuesNow.sparaWaypoint == 1)
			sparaLastWaypoint(filpekG, posReport, solName);

		startKvot = model.network.startKvot[ii];
		endKvot = model.network.endKvot[ii];
		//printf("split %d startKvot %.3lf endKvot %.3lf\n", ii, startKvot, endKvot);

		//if (model.functions.valuesNow.Wpt >= 21)
		//	printGlobal = 1;
		//if (model.arc[arcNr].time > 0.001 || model.arc[arcNr].distance > 0.001)
		if (model.arc[arcNr].fromLevel == 87 && ii == 1)
			arcNr = arcNr;
		//if (arcNr == 175518)
		//	printGlobal = 0;
		timeCheck = evalWeatherDataAlongArcSection(arcNr, startKvot, endKvot, startSlutArc, *timeExact, delayFactor, useFixCalmWaterSpeed) - (*timeExact);
		//if (arcNr == 175518)
		//	printGlobal = 0;
		if (timeCheck < 0.001)
			continue;
		//printf("timeCheck arcNr %d arcStart %lf startNu %lf tid %lf kvoter %lf %lf tidArc %lf tidExakt %lf diff %lf\n",
		//	arcNr, model.arc[arcNr].fromTime * model.params.tIndexGerH, *timeExact, model.arc[arcNr].time,
		//	startKvot, endKvot, model.arc[arcNr].time* (endKvot - startKvot), timeCheck,
		//	model.arc[arcNr].time* (endKvot - startKvot) - timeCheck);

		//else{
		//	timeCheck = 0;


		if (model.arc[arcNr].fromTime * model.params.tIndexGerH >= model.network.tidp_startHistoricDataOnly) {
			timeCheck = model.arc[arcNr].time * (endKvot - startKvot); // we use estimated delay then, don't use the one calculated with historical weather data
			model.functions.valuesNow.totFuel_aux += (model.arc[arcNr].fuel_aux + model.arc[arcNr].fuel_auxEca) * (endKvot - startKvot);
			model.functions.valuesNow.totFuel_main += (model.arc[arcNr].fuel_eca + model.arc[arcNr].fuel_noEca) * (endKvot - startKvot);
			model.functions.valuesNow.fuel_main = (model.arc[arcNr].fuel_eca + model.arc[arcNr].fuel_noEca) * (endKvot - startKvot);
			model.functions.valuesNow.fuel_aux = (model.arc[arcNr].fuel_auxEca + model.arc[arcNr].fuel_aux) * (endKvot - startKvot);
		}
		else {
			model.functions.valuesNow.totFuel_aux += model.functions.valuesNow.fuel_aux;
			model.functions.valuesNow.totFuel_main += model.functions.valuesNow.fuel_main;
		}
		accumTime += timeCheck;
		*timeExact += timeCheck;

		if (filpekG == NULL && solName == "-")
			return 0;

		if (timeCheck > 0.01)
			speedOnGround = model.arc[arcNr].distance * (endKvot - startKvot) / timeCheck;
		else
			speedOnGround = 0;


		if (filpekG != NULL) {
			if (ii > 0) {
				time_t test = mktime(&tmBas);
				if (test == -1) {
					printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
						tmBas.tm_year,
						tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
					if (model.params.failedTime == 0)
						postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
					model.params.failedTime = 1;
				}
			}
			if (model.functions.valuesNow.sparaWaypoint == 1) {
				fixReportDate(tmBas, model.waypointResult.dateUTC);
				fixReportDate_full(tmBas, model.waypointResult.full_Date);
			}
			tmBas.tm_min += timeCheck * 60;
			if (model.functions.valuesNow.sparaWaypoint == 1) {
				if (arcNr == 4859)
					arcNr = arcNr;
				if (model.arc[arcNr].fromLevel == -3)
					arcNr = arcNr;
				model.waypointResult.arcNr = arcNr;
				model.waypointResult.ii = ii;
				model.waypointResult.fromLevel = model.arc[arcNr].fromLevel;
				model.waypointResult.fromPointNr = model.arc[arcNr].fromPointNr;
				model.waypointResult.fromTime = model.arc[arcNr].fromTime;
				model.waypointResult.accumTimeStart_h = timeOld;
			}
			model.waypointResult.arc_obj_cost += model.arc[arcNr].totCost * (endKvot - startKvot);
			if (model.arc[arcNr].fromLevel >= 0 && ii == 0) {
				diffTime = timeOld - model.network.physicalLev[model.arc[arcNr].fromLevel].midTimeArrive;
				if (model.functions.valuesNow.sparaWaypoint == 1)
					model.waypointResult.diffTime = diffTime;
				if (diffTime > model.functions.valuesNow.maxDiffTime) {
					model.functions.valuesNow.maxDiffTime = diffTime;
					model.functions.valuesNow.maxDiffTime_level = model.arc[arcNr].fromLevel;
				}
				if (diffTime < model.functions.valuesNow.minDiffTime) {
					model.functions.valuesNow.minDiffTime = diffTime;
					model.functions.valuesNow.minDiffTime_level = model.arc[arcNr].fromLevel;
				}
			}
			else {
				if (model.functions.valuesNow.sparaWaypoint == 1)
					model.waypointResult.diffTime = -99999;
				diffTime = 0;
			}
			if (model.functions.valuesNow.sparaWaypoint == 1) {
				model.waypointResult.toLevel = model.arc[arcNr].toLevel;
				model.waypointResult.toPointNr = model.arc[arcNr].toPointNr;
				if (ii == nSplit - 1)
					model.waypointResult.toTime = model.arc[arcNr].toTime;
				else
					model.waypointResult.toTime = -1;
			}
			tidIntForecast = getTidIntForecast(*timeExact);

			if (startSlutArc == 0) {
				model.functions.valuesNow.accumDistance += model.arc[arcNr].distance * (endKvot - startKvot);
				model.waypointResult.hours += model.arc[arcNr].time * (endKvot - startKvot);
				model.waypointResult.checkHours += timeCheck;
				model.waypointResult.distance_nm += model.arc[arcNr].distance * (endKvot - startKvot) / model.params.knots_to_km;

				model.waypointResult.windF += model.functions.valuesNow.WindF / model.params.knots_to_km;
				model.waypointResult.waveF += model.functions.valuesNow.WaveF / model.params.knots_to_km;
				model.waypointResult.currentF += model.functions.valuesNow.CurrentF / model.params.knots_to_km;
				model.waypointResult.delayF += model.functions.valuesNow.DelayF / model.params.knots_to_km;

				model.waypointResult.accumDistance_km = model.functions.valuesNow.accumDistance;
				model.waypointResult.distanceLeft_nm =
					(model.functions.valuesNow.totDistance - model.functions.valuesNow.accumDistance) / model.params.knots_to_km;

				if (ii < nSplit - 1) {
					bearing = calcBearingFromToCoords(model.network.yCoord[model.network.posSplitCoord[ii]],
						model.network.xCoord[model.network.posSplitCoord[ii]],
						model.network.yCoord[model.network.posSplitCoord[ii + 1]],
						model.network.xCoord[model.network.posSplitCoord[ii + 1]]);
					nChangeBearingBetween = model.network.posSplitCoord[ii + 1] - model.network.posSplitCoord[ii] - 1;
					if (nChangeBearingBetween == 0)
						nChange_lessXdegrees = 0;
					else
						nChange_lessXdegrees = identify_nChanges_lessXdegrees(model.network.posSplitCoord[ii], model.network.posSplitCoord[ii + 1]);
				}
				else {
					if (ii > 0) {
						bearing = calcBearingFromToCoords(model.network.yCoord[model.network.posSplitCoord[ii]],
							model.network.xCoord[model.network.posSplitCoord[ii]],
							p2.latitude().degrees(), p2.longitude().degrees());
						nChangeBearingBetween = model.network.nCoords - model.network.posSplitCoord[ii] - 1;
						if (nChangeBearingBetween == 0)
							nChange_lessXdegrees = 0;
						else
							nChange_lessXdegrees = identify_nChanges_lessXdegrees(model.network.posSplitCoord[ii], model.network.nCoords - 1);
					}
					else {
						bearing = p1.bearingTo(p2);
						nChangeBearingBetween = model.network.nCoords - model.network.posSplitCoord[ii] - 1;
						nChange_lessXdegrees = identify_nChanges_lessXdegrees(model.network.posSplitCoord[ii], model.network.nCoords - 1);
					}
				}

				diff = getDiff_anglesDegrees(bearing, model.functions.valuesNow.bearingOldWpt);
				if (diff >= model.params.report_minBearingDiffWpt || lev1 < 0) {
					model.waypointResult.bearingDiff = 1;
					model.functions.valuesNow.bearingOldWpt = bearing;
				}
				if (model.functions.valuesNow.sparaWaypoint == 1) {
					fixPositionString_latLon(model.network.yCoord[model.network.posSplitCoord[ii]],
						model.network.xCoord[model.network.posSplitCoord[ii]], model.waypointResult.fixPositionString_latlon);
					model.waypointResult.bearing = bearing;


				}
				model.waypointResult.nChangeBearingBetween += nChangeBearingBetween;
				model.waypointResult.nChange_lessXdegrees += nChange_lessXdegrees;
				//}
				if (arcNr == 22)
					arcNr = arcNr; // checkpfg
				if (lev1 < 0 && lev2 < 0) {

					fuel_day = (model.functions.valuesNow.fuel_main) * 24 / (timeCheck - model.network.channel[-lev1 - 1].waitingTime);
					fuel_dayAux = (model.functions.valuesNow.fuel_aux) * 24 / (timeCheck - model.network.channel[-lev1 - 1].waitingTime);
					fuelMan_tot = model.functions.valuesNow.fuel_main;
					fuelAux_tot = model.functions.valuesNow.fuel_aux;
					time_tot = timeCheck - model.network.channel[-lev1 - 1].waitingTime;
				}
				else {
					if (timeCheck > 0.001) {
						//	fuel_day = (model.arc[arcNr].fuel_aux * (endKvot - startKvot) + model.arc[arcNr].fuel_auxEca * (endKvot - startKvot)
						//		+ model.arc[arcNr].fuel_eca * (endKvot - startKvot) + model.arc[arcNr].fuel_noEca * (endKvot - startKvot)) * 24 / timeCheck;
						//fuel_day = (model.arc[arcNr].fuel_eca * (endKvot - startKvot) + model.arc[arcNr].fuel_noEca * (endKvot - startKvot)) * 24 / timeCheck;
						fuel_day = (model.functions.valuesNow.fuel_main) * 24 / timeCheck;
						fuel_dayAux = (model.functions.valuesNow.fuel_aux) * 24 / timeCheck;
						fuelMan_tot = model.functions.valuesNow.fuel_main;
						fuelAux_tot = model.functions.valuesNow.fuel_aux;
						time_tot = timeCheck;
						//fuel_dayCheck = (model.functions.valuesNow.fuel_main * (endKvot - startKvot)) * 24 / timeCheck;
					}
					else {
						fuel_day = 0;
						fuel_dayAux = 0;
						fuelMan_tot = 0;
						fuelAux_tot = 0;
						//fuel_dayCheck = 0;
					}
				}

				model.waypointResult.fuelMain_tot += fuelMan_tot;
				model.waypointResult.fuelAux_tot += fuelAux_tot;
				model.waypointResult.time_tot += time_tot;
				model.waypointResult.fuelEca += (1 - model.arc[arcNr].fuelQualityKvot) * (model.functions.valuesNow.fuel_main + model.functions.valuesNow.fuel_aux);

				if (filpek10 != NULL) {
					if (ii > 0)
						fprintf(filpek10, "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");

					fprintf(filpek10, "%.3lf\t%.3lf\t%.3lf\t%.3lf\t%s\t%.3lf\t%.3lf\t%.3lf",
						timeCheck, *timeExact, diffTime, model.functions.valuesNow.accumDistance,
						startTime, bearing, fuel_day, model.functions.valuesNow.worstStormValue);
				}



				if ((lev1 >= 0 || lev2 >= 0) && model.arc[arcNr].speedSetting >= 0) {
					if (model.functions.valuesNow.sparaWaypoint == 1) {
						model.waypointResult.current = model.functions.valuesNow.current / model.params.knots_to_km;
						model.waypointResult.speedSetting = model.arc[arcNr].speedSetting;
						model.waypointResult.calmWaterSpeed = model.functions.valuesNow.calmWaterSpeed / model.params.knots_to_km;
						model.waypointResult.speedOnGround = speedOnGround / model.params.knots_to_km;
						model.waypointResult.rpm = speedSetting.rpm[model.arc[arcNr].speedSetting];

						model.waypointResult.windSpeed = model.functions.valuesNow.windSpeed;
						model.waypointResult.relWindDir = model.functions.valuesNow.relWindDir;
						model.waypointResult.waveHeight = model.functions.valuesNow.waveHeight;
						model.waypointResult.wavePeriod = model.functions.valuesNow.wavePeriod;
						model.waypointResult.relWaveDir = model.functions.valuesNow.relWaveDir;
						//if (model.waypointResult.waveHeight > 3.3)
						//	model.waypointResult.waveHeight = model.waypointResult.waveHeight;
					}
					model.functions.valuesNow.accumRPM += speedSetting.rpm[model.arc[arcNr].speedSetting] * timeCheck;
					model.functions.valuesNow.accumRPM_time += timeCheck;

					if (model.functions.valuesNow.waveHeight > model.functions.valuesNow.maxWaveHeight &&
						model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
						model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.waveHeight;
						model.functions.valuesNow.maxWaveHeight_tp = *timeExact - timeCheck;
						model.functions.valuesNow.maxWaveHeight_dir = model.functions.valuesNow.waveDirReal; // correct?
						if (model.functions.valuesNow.maxWaveHeight > 9)
							model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
					}


					if (model.nWeatherFiles > 10) {
						if (model.waypointResult.maxWaveHeight < model.arc[arcNr].maxWaveHeight)
							model.waypointResult.maxWaveHeight = model.arc[arcNr].maxWaveHeight;
						if (model.waypointResult.maxWindSpeed < model.arc[arcNr].maxWindSpeed)
							model.waypointResult.maxWindSpeed = model.arc[arcNr].maxWindSpeed;
						if (model.functions.valuesNow.sparaWaypoint == 1) {
							addExtraWeatherInfoToGeojson2();
						}
					}

					model.functions.valuesNow.totFuel_mainMovingNoCorridors += model.functions.valuesNow.fuel_main;
					model.functions.valuesNow.totTime_movingNoCorridors += timeCheck;
					model.functions.valuesNow.sumSpeedOnWater += model.functions.valuesNow.speedOnWater * timeCheck;

					model.functions.valuesNow.totDistance_movingNoCorridors += model.arc[arcNr].distance * (endKvot - startKvot);

					if (printGlobal == 1)
						printf("arcNr %d levels %d %d wavePeriod %.3lf dist %.3lf time %.3lf speedOnGround %.3lf\n",
							arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.functions.valuesNow.wavePeriod,
							model.arc[arcNr].distance * (endKvot - startKvot), model.arc[arcNr].time * (endKvot - startKvot), speedOnGround);

					if (model.functions.valuesNow.sparaWaypoint == 1) {
						if (model.functions.valuesNow.windReal < 1000 || model.functions.valuesNow.windDirReal_lastKnown < 1000) {
							if (model.functions.valuesNow.windReal < 1000) {
								model.functions.valuesNow.windDirReal_lastKnown = model.functions.valuesNow.windDirReal;
								model.functions.valuesNow.windReal_lastKnown = model.functions.valuesNow.windReal;
							}
							model.waypointResult.windReal_lastKnown = model.functions.valuesNow.windReal_lastKnown;
							fixDirectionLetters(model.functions.valuesNow.windDirReal_lastKnown, model.waypointResult.windDirReal_letters, 1);
							model.waypointResult.windDirReal = model.functions.valuesNow.windDirReal_lastKnown;

						}
						if (model.functions.valuesNow.currentReal < 1000 || model.functions.valuesNow.currentReal_lastKnown < 1000) {
							if (model.functions.valuesNow.currentReal < 1000)
								model.functions.valuesNow.currentReal_lastKnown = model.functions.valuesNow.currentReal;
							model.waypointResult.currentReal_lastKnown = model.functions.valuesNow.currentReal_lastKnown;
							model.waypointResult.currentDirReal = model.functions.valuesNow.currentDirReal;
						}
						if (model.functions.valuesNow.waveDirReal < 1000 || model.functions.valuesNow.waveDirReal_lastKnown < 1000) {
							if (model.functions.valuesNow.waveDirReal < 1000)
								model.functions.valuesNow.waveDirReal_lastKnown = model.functions.valuesNow.waveDirReal;
							fixDirectionLetters(model.functions.valuesNow.waveDirReal_lastKnown, model.waypointResult.waveDir_letters, 1);
							model.waypointResult.waveDirReal = model.functions.valuesNow.waveDirReal;
							model.waypointResult.waveDirReal_lastKnown = model.functions.valuesNow.waveDirReal_lastKnown;
						}

						model.waypointResult.bowSlam = model.functions.valuesNow.bowSlam;
						model.waypointResult.greenWater = model.functions.valuesNow.greenWater;
						model.waypointResult.dynamicStability = model.functions.valuesNow.dynamicStability;
						model.waypointResult.rolling = model.functions.valuesNow.rolling;
						model.waypointResult.surfRiding = model.functions.valuesNow.surfRiding;

						model.waypointResult.iceCover_max = model.functions.valuesNow.iceCover_max;
						model.waypointResult.forecastType = model.functions.valuesNow.forecastType;
					}

					if (filpek10 != NULL) {
						fprintf(filpek10, "\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t"
							"%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf",
							model.functions.valuesNow.currentReal, model.functions.valuesNow.current,
							model.functions.valuesNow.calmWaterSpeed,
							model.functions.valuesNow.baseGroundSpeed, speedOnGround,
							speedSetting.rpm[model.arc[arcNr].speedSetting],
							model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir,
							model.functions.valuesNow.speedDiffWind,
							model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
							model.functions.valuesNow.relWaveDir, model.functions.valuesNow.speedDiffWave,
							model.functions.valuesNow.windReal,
							model.functions.valuesNow.windDirReal, model.functions.valuesNow.currentReal,
							model.functions.valuesNow.currentDirReal, model.functions.valuesNow.waveDirReal,
							model.functions.valuesNow.bowSlamming_max,
							model.functions.valuesNow.greenWater_max,
							model.functions.valuesNow.dynamicStability_max,
							model.functions.valuesNow.iceCover_max,
							model.functions.valuesNow.forecastType);
					}

				}
				else {
					if (lev1 < 0 && lev2 < 0) {
						if (model.network.channel[-lev1 - 1].type == 1) { // tss
							model.functions.valuesNow.accumRPM += speedSetting.rpm[model.arc[arcNr].speedSetting] * timeCheck;
							model.functions.valuesNow.accumRPM_time += timeCheck;
							if (model.functions.valuesNow.sparaWaypoint == 1) {
								model.waypointResult.current = model.functions.valuesNow.current / model.params.knots_to_km;
								model.waypointResult.speedSetting = model.arc[arcNr].speedSetting;
								model.waypointResult.calmWaterSpeed = model.functions.valuesNow.calmWaterSpeed / model.params.knots_to_km;
								model.waypointResult.speedOnGround = speedOnGround / model.params.knots_to_km;
								model.waypointResult.rpm = speedSetting.rpm[model.arc[arcNr].speedSetting];
								model.waypointResult.windSpeed = model.functions.valuesNow.windSpeed;
								model.waypointResult.relWindDir = model.functions.valuesNow.relWindDir;
								model.waypointResult.waveHeight = model.functions.valuesNow.waveHeight;
								if (model.waypointResult.waveHeight > 3.3)
									model.waypointResult.waveHeight = model.waypointResult.waveHeight;
								model.waypointResult.wavePeriod = model.functions.valuesNow.wavePeriod;
								model.waypointResult.relWaveDir = model.functions.valuesNow.relWaveDir;
							}
							if (model.functions.valuesNow.waveHeight > model.functions.valuesNow.maxWaveHeight &&
								model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
								model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.waveHeight;
								model.functions.valuesNow.maxWaveHeight_tp = *timeExact - timeCheck;
								model.functions.valuesNow.maxWaveHeight_dir = model.functions.valuesNow.waveDirReal; // correct?
								if (model.functions.valuesNow.maxWaveHeight > 9)
									model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
							}

							if (model.nWeatherFiles > 10) {
								if (model.waypointResult.maxWaveHeight < model.arc[arcNr].maxWaveHeight)
									model.waypointResult.maxWaveHeight = model.arc[arcNr].maxWaveHeight;
								if (model.waypointResult.maxWindSpeed < model.arc[arcNr].maxWindSpeed)
									model.waypointResult.maxWindSpeed = model.arc[arcNr].maxWindSpeed;
								if (model.functions.valuesNow.sparaWaypoint == 1) {
									addExtraWeatherInfoToGeojson2();
								}
							}

							model.functions.valuesNow.totFuel_mainMovingNoCorridors += model.functions.valuesNow.fuel_main;
							model.functions.valuesNow.totTime_movingNoCorridors += timeCheck;
							model.functions.valuesNow.sumSpeedOnWater += model.functions.valuesNow.speedOnWater * timeCheck;

							model.functions.valuesNow.totDistance_movingNoCorridors += model.arc[arcNr].distance * (endKvot - startKvot);

							if (printGlobal == 1)
								printf("arcNr %d levels %d %d wavePeriod %.3lf dist %.3lf time %.3lf speedOnGround %.3lf\n",
									arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.functions.valuesNow.wavePeriod,
									model.arc[arcNr].distance * (endKvot - startKvot), model.arc[arcNr].time * (endKvot - startKvot), speedOnGround);

							if (model.functions.valuesNow.sparaWaypoint == 1) {
								if (model.functions.valuesNow.windReal < 1000 || model.functions.valuesNow.windDirReal_lastKnown < 1000) {
									if (model.functions.valuesNow.windReal < 1000) {
										model.functions.valuesNow.windDirReal_lastKnown = model.functions.valuesNow.windDirReal;
										model.functions.valuesNow.windReal_lastKnown = model.functions.valuesNow.windReal;
									}
									model.waypointResult.windReal_lastKnown = model.functions.valuesNow.windReal_lastKnown;
									fixDirectionLetters(model.functions.valuesNow.windDirReal_lastKnown, model.waypointResult.windDirReal_letters, 1);
									model.waypointResult.windDirReal = model.functions.valuesNow.windDirReal;
								}
								if (model.functions.valuesNow.currentReal < 1000 || model.functions.valuesNow.currentReal_lastKnown < 1000) {
									if (model.functions.valuesNow.currentReal < 1000)
										model.functions.valuesNow.currentReal_lastKnown = model.functions.valuesNow.currentReal;
									model.waypointResult.currentReal_lastKnown = model.functions.valuesNow.currentReal_lastKnown;
									model.waypointResult.currentDirReal = model.functions.valuesNow.currentDirReal;
								}
								if (model.functions.valuesNow.waveDirReal < 1000 || model.functions.valuesNow.waveDirReal_lastKnown < 1000) {
									if (model.functions.valuesNow.waveDirReal < 1000)
										model.functions.valuesNow.waveDirReal_lastKnown = model.functions.valuesNow.waveDirReal;
									fixDirectionLetters(model.functions.valuesNow.waveDirReal_lastKnown, model.waypointResult.waveDir_letters, 1);
									model.waypointResult.waveDirReal = model.functions.valuesNow.waveDirReal;
									model.waypointResult.waveDirReal_lastKnown = model.functions.valuesNow.waveDirReal_lastKnown;
								}

								model.waypointResult.bowSlam = model.functions.valuesNow.bowSlam;
								model.waypointResult.greenWater = model.functions.valuesNow.greenWater;
								model.waypointResult.dynamicStability = model.functions.valuesNow.dynamicStability;
								model.waypointResult.rolling = model.functions.valuesNow.rolling;
								model.waypointResult.surfRiding = model.functions.valuesNow.surfRiding;

								model.waypointResult.iceCover_max = model.functions.valuesNow.iceCover_max;
								model.waypointResult.forecastType = model.functions.valuesNow.forecastType;
							}
						}
						else {
							model.waypointResult.fuelTransit_main += model.functions.valuesNow.fuel_main - model.network.channel[-lev1 - 1].waiting_consumption_main;
							model.waypointResult.fuelTransit_aux += model.functions.valuesNow.fuel_aux - model.network.channel[-lev1 - 1].waiting_consumption_aux;
							model.waypointResult.fuelWaiting_main += model.network.channel[-lev1 - 1].waiting_consumption_main;
							model.waypointResult.fuelWaiting_aux += model.network.channel[-lev1 - 1].waiting_consumption_aux;
							model.waypointResult.transitTime += timeCheck - model.network.channel[-lev1 - 1].waitingTime;
							model.waypointResult.waitingTime += model.network.channel[-lev1 - 1].waitingTime;

							if (model.arc[arcNr].fuelQualityKvot > 0.9) {
								// not eca
								model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA += model.network.channel[-lev1 - 1].waiting_consumption_main;
								model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA += model.network.channel[-lev1 - 1].waiting_consumption_aux;
								model.waypointResult.emissionWaiting_main += model.network.channel[-lev1 - 1].waiting_consumption_main * model.params.fuel.main_noEca.emissionFactor;
								model.waypointResult.emissionWaiting_aux += model.network.channel[-lev1 - 1].waiting_consumption_aux * model.params.fuel.aux_noEca.emissionFactor;
							}
							else {
								model.functions.valuesNow.totCorridorWaitingFuel_mainECA += model.network.channel[-lev1 - 1].waiting_consumption_main;
								model.functions.valuesNow.totCorridorWaitingFuel_auxECA += model.network.channel[-lev1 - 1].waiting_consumption_aux;
								model.waypointResult.emissionWaiting_main += model.network.channel[-lev1 - 1].waiting_consumption_main * model.params.fuel.main_eca.emissionFactor;
								model.waypointResult.emissionWaiting_aux += model.network.channel[-lev1 - 1].waiting_consumption_aux * model.params.fuel.aux_eca.emissionFactor;
							}
							model.functions.valuesNow.totCorridorWaitingTime += model.network.channel[-lev1 - 1].waitingTime;
						}
					}
					else {
						if (model.functions.valuesNow.sparaWaypoint == 1) {
							fprintf(filpekG, ", \"corridorID\":\"extension of corridor since not physically feasible area around the corridor\"");
						}
					}
				}

				if (model.waypointResult.worstStormValue < model.functions.valuesNow.worstStormValue)
					model.waypointResult.worstStormValue = model.functions.valuesNow.worstStormValue;

			}
			else {
				if (model.functions.valuesNow.sparaWaypoint == 1) {
					fprintf(filpekG, "    \"hours\":%.2lf, \"checkHours\":%.3lf, \"distance_nm\":%.1lf,\n", 0.0, 0.0, 0.0);
					fprintf(filpekG, "    \"accumDistance_km\":%.2lf, \"distanceLeft_nm\":%.1lf,\n", model.functions.valuesNow.accumDistance, 0.0);
					x = p2.longitude().degrees();
					y = p2.latitude().degrees();
					fprintf(filpekG, "    \"Position_lat_lon\":\"%.3lf, %.3lf\", \"bearing\":%.0lf,\n",
						y, x, bearing);
					fprintf(filpekG, "    \"nChangeBearingInbetweenNodes\":%d,\n", 0);
					fprintf(filpekG, "    \"nChangeBearing_lessThanXdegrees\":%d,\n", 0);
					fprintf(filpekG, "    \"speedSetting\":%d, \"speedCalmWater_kts\":%.2lf, \"speedOnGround_kts\":%.2lf, \"rpm\":%.2lf,\n",
						-1, 0, 0, 0);
					fprintf(filpekG, "    \"fuelConsumptionMain_ton_day\":%.5lf, \"fuelConsumptionAux_ton_day\":%.5lf, \"fuelECA_ton\":%.3lf, \"current_kts\":%.1lf,\n",
						0, 0, 0, model.functions.valuesNow.current / model.params.knots_to_km);
					fprintf(filpekG, "    \"windSpeed_km_h\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir);
					fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
						model.functions.valuesNow.relWaveDir);
				}
				if (model.functions.valuesNow.waveHeight > model.functions.valuesNow.maxWaveHeight &&
					model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
					model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.waveHeight;
					model.functions.valuesNow.maxWaveHeight_tp = *timeExact - timeCheck;
					model.functions.valuesNow.maxWaveHeight_dir = model.functions.valuesNow.waveDirReal; // correct?
					if (model.functions.valuesNow.maxWaveHeight > 9)
						model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
				}

				if (model.functions.valuesNow.sparaWaypoint == 1) {
					fixDirectionLetters(model.functions.valuesNow.windDirReal, startTime, 1);
					fprintf(filpekG, "    \"windSpeedReal_knots\":%.1lf, \"windDirection_degrees\":%.0lf, \"windDir_letters\":\"%s\",\n",
						model.functions.valuesNow.windReal / model.params.knots_to_km, model.functions.valuesNow.windDirReal, startTime);
					fprintf(filpekG, "    \"currentReal_knots\":%.3lf, \"currentDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.currentReal / model.params.knots_to_km, model.functions.valuesNow.currentDirReal);
					fixDirectionLetters(model.functions.valuesNow.waveDirReal, startTime, 1);
					fprintf(filpekG, "    \"waveDirection_degrees\":%.0lf,\"waveDir_letters\":\"%s\",\n",
						model.functions.valuesNow.waveDirReal_lastKnown, startTime);

					if (model.nWeatherFiles > 10) {
						addExtraWeatherInfoToGeojson2();
						fprintf(filpekG, "    \"maxWaveArc\":%.3lf,\n", model.arc[arcNr].maxWaveHeight);
						fprintf(filpekG, "    \"maxWindSpeedArc\":%.3lf,\n", model.arc[arcNr].maxWindSpeed);
					}

#ifdef NAZANIN_SAFETY
					fprintf(filpekG, "    \"bow slamming p\":%.3lf, \"green water p\":%.3lf,\n    \"dynamic instability\":%.3lf,\n",
						0.0, 0.0, 0.0);
					fprintf(filpekG, "    \"rolling p\":%.3lf, \"surfRiding p\":%.3lf,\n",
						0.0, 0.0);
#endif
					fprintf(filpekG, "    \"bow slamming p\":%.3lf, \"green water p\":%.3lf,\n    \"dynamic instability\":%.3lf,\n",
						0.0, 0.0, 0.0);
					fprintf(filpekG, "    \"max iceCover\":%.3lf,  \"worstStormValue\":%.3lf, \"forecastType\":\"\"",
						0.0, 0.0);
				}
			}
			if (model.functions.valuesNow.sparaWaypoint == 1) {
				model.waypointResult.xCoord = model.network.xCoord[model.network.posSplitCoord[ii]];
				model.waypointResult.yCoord = model.network.yCoord[model.network.posSplitCoord[ii]];
			}
		}
	}

	if (filpek10 != NULL) {
		free(namn);
		fprintf(filpek10, "\n");
		fclose(filpek10);
	}

	free(startTime);

	if (model.params.simuleraTidVisuellt == 1) {
		plotPathTimeVisuellt(arcNr, timeOld, *timeExact);
	}

	if (filpekG != NULL) {
		if (abs(model.arc[arcNr].time - accumTime) > 1.0) {
			(model.delay.nDiffTimeSol)++;
			errlog("ERROR! Diff between arcTime and time computed in addPositionDataToReport for arc %d, %.3lf vs %.3lf, might be okay since the check points are different?\n"
				"especially if it is a corridor with a certain starting time from midnight\n",
				arcNr, accumTime, model.arc[arcNr].time);
			printf("\n\n\n\n\n########################################################\n");
			printf("ERROR! Diff between arcTime and time computed in addPositionDataToReport for arc %d, %.3lf vs %.3lf levels %d %d, might be okay since the check points are different?\n"
				"especially if it is a corridor with a certain starting time from midnight\n",
				arcNr, accumTime, model.arc[arcNr].time, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
			//(*timeExact) += model.arc[arcNr].time - accumTime;
		}
	}
	if (SKRIV_UT_NOTHING == 0) {
		if (arcNr == 29303)
			arcNr = arcNr;
		errlog("pos %d arcNr %d arcStart %lf arcEnd %lf timeExact %lf arcTimeEst %lf exactTime %lf arcCostEst %lf exactCost %lf\n", *posReport, arcNr,
			model.arc[arcNr].fromTime / (double)model.params.nTidsperioder_perH, model.arc[arcNr].toTime / (double)model.params.nTidsperioder_perH, *timeExact,
			model.arc[arcNr].time, accumTime, model.arc[arcNr].totCost / 100.0,
			model.functions.valuesNow.totFuel_main * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.main_eca.price +
			model.functions.valuesNow.totFuel_main * model.arc[arcNr].fuelQualityKvot * model.params.fuel.main_noEca.price +
			model.functions.valuesNow.totFuel_aux * model.arc[arcNr].fuelQualityKvot * model.params.fuel.aux_noEca.price +
			model.functions.valuesNow.totFuel_aux * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.aux_eca.price +
			accumTime * model.params.priceTime);

	}



	return 0;
}

double evalWeatherDataAlongArcSection_equalTimeIntervals(int arcNr, double startKvot, double* usedKvot, int startSlutArc, double timeExact, double delayFactor, int useFixCalmWaterSpeed) {
	int i, cNr, tidInt, delayNr, tidIntForecast;
	double checkFactor, speedNu, calcDelayFactor, checkSpeedDiffCurrent, tidNu, tidCalmWater;
	double windSpeed_x, windSpeed_y, waveDir_y, waveDir_x, dist, tidTot, distNu;
	double stormVarde, uCurrent, vCurrent, deltaTid;
	double currentReal_u = 0, currentReal_v = 0, windSpeedReal_u = 0, windSpeedReal_v = 0, waveHeightReal_u = 0, waveHeightReal_v = 0;
	double timeArc_wind = 0, timeArc_current = 0, timeArc_wave = 0;
	double currentDirection, currentSpeed, baseGroundSpeed, calmWaterSpeed, uWind, vWind;
	double speedDiffWindWave, rel_windSpeed, rel_windDir, speedOverGround, timeArc;
	double fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux, windDirection, windSpeed2, windSpeed, waveHeight, wavePeriod;
	double rel_waveDir, iceCover, waveDirection, speedDiffWind, speedDiffWave;
	double fixTime = -1, distStart, distEnd, useKvotNu, kvotBort;
	double waveHeightBase, waveDirectionBase, fuelTot_main = 0, fuelTot_aux = 0;
	double timeArcSTW, kvotTimeUse, absWindDirDiff = 0, timeArcUse;
	int favorableWind, favorableWave;
	double tidTotArc, tidTotArc0;

	if (arcNr == 36)
		arcNr = arcNr;
	//if (arcNr == 412305)
	//	printGlobal = 1;
	//else
	//	printGlobal = 0;

	//if (arcNr == 96)
	//	printf("arcNr %d kvoter %.3lf %.3lf timeExact %.3lf\n", arcNr,
	//		startKvot, endKvot, timeExact);
	model.functions.valuesNow.channelCost = 0;
	if (arcNr >= 0) {
		if (model.arc[arcNr].toLevel < 0 && model.arc[arcNr].fromLevel < 0) {
			cNr = -model.arc[arcNr].toLevel - 1;
			model.functions.valuesNow.channelCost = model.network.channel[cNr].extraCostChannel;
			if (startKvot < 0.001) {
				if (model.network.channel[cNr].intArrivalTime_h >= 0) {
					//printf("corridor %d intArrivalTime_h %d\n", cNr, model.network.channel[cNr].intArrivalTime_h);
					timeExact = delayTimeToStartTimeDay(timeExact, model.network.channel[cNr].intArrivalTime_h);
				}
				timeExact += model.network.channel[cNr].waitingTime; // .intWaitingTime;
			}
			fixTime = (model.network.channel[cNr].timeThroughChannel * (1 - startKvot));
		}
		else {
			if (model.arc[arcNr].speedSetting < 0)
				fixTime = 10000;
		}
	}

	//if (arcNr == 96)
	//	printf("arcNr %d fixTime %.3lf timeExact %.3lf startSlutArc %d\n", arcNr,
	//		fixTime, timeExact, startSlutArc);

	//model.functions.valuesNow.current = 0;
	//model.functions.valuesNow.windSpeed = 0;
	windSpeed_x = 0;
	windSpeed_y = 0;
	waveDir_x = 0;
	waveDir_y = 0;

	if (model.arc[arcNr].time <= 0.001 && model.arc[arcNr].distance <= 0.001) {
		//	model.functions.valuesNow.fuel_aux = 0;
		//	model.functions.valuesNow.fuel_main = 0;
		//	model.functions.valuesNow.currentReal = 0;
		//	model.functions.valuesNow.currentDirReal = 0;
		//	model.functions.valuesNow.windReal = 9999;
		//	model.functions.valuesNow.windDirReal = 0;
		//	model.functions.valuesNow.waveDirReal = 0;
		return timeExact;
	}

	dist = 0;
	distStart = model.arc[arcNr].distance * startKvot;
	distEnd = model.arc[arcNr].distance * 1;

	if (startSlutArc == 0) {
		tidTot = timeExact;

		if (fixTime > 0) {
			if (fixTime < 9999)
				calmWaterSpeed = model.network.channel[cNr].distance_km * (1 - startKvot) / fixTime;
			else {
				if (useFixCalmWaterSpeed == 0)
					calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				else
					calmWaterSpeed = model.params.calmWaterSpeedCompareUse;
			}

			//printf("\narcNr %d levels %d %d pointPos %d %d cNr %d dist %.3lf arcDist %.3lf fixTime %.3lf\n", arcNr, 
			//	model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, 
			//	model.arc[arcNr].toPointNr, cNr,
			//	model.network.channel[cNr].distance_km, model.arc[arcNr].distance, fixTime);
		}
		else {
			if (useFixCalmWaterSpeed == 0)
				calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
			else
				calmWaterSpeed = model.params.calmWaterSpeedCompareUse;
		}
		model.functions.valuesNow.calmWaterSpeed = calmWaterSpeed;
		if (printGlobal == 1) {
			if (arcNr >= 0)
				printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, model.arc[arcNr].speedSetting, calmWaterSpeed,
					model.weatherFunctions.nCheckPoints);
			else
				printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, -1, calmWaterSpeed,
					model.weatherFunctions.nCheckPoints);
		}

		//if (arcNr == 62)
		//	arcNr = arcNr;

		if (USE_ARC_TIME_EXACT == 1)
			tidTotArc = model.arc[arcNr].fromTime * model.params.tIndexGerH;
		else
			tidTotArc = tidTot;
		tidTotArc0 = tidTotArc;

		for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
			if (dist >= distEnd)
				break; // past the end of this part of the arc

			distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
			dist += distNu;

			if (dist <= distStart)
				continue; // not far enough of the arc yet

			if (dist - distNu < distStart)
				useKvotNu = (dist - distStart) / distNu;
			else
				useKvotNu = 1.0;
			if (dist > distEnd) {
				kvotBort = (dist - distEnd) / distNu;
				useKvotNu -= kvotBort;
			}

			//printf("nCheckPoints %d i %d tidTot %.2lf distNu %.2lf dist %.2lf distStart %.2lf distEnd %.2lf useKvotNu %.3lf\n", 
			//	model.weatherFunctions.nCheckPoints, i, tidTot, distNu, dist, distStart, distEnd, useKvotNu);
			//uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
			//vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

			//if (arcNr == 2578755)
			//	arcNr = arcNr;
			stormVarde = getStormValue((int)(tidTotArc), model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i], 1);// model.weatherFunctions.point[i]);
			if (stormVarde > model.functions.valuesNow.worstStormValue) {
				if (stormVarde > model.arc[arcNr].safetyHurricane && arcNr < model.nArcs)
					stormVarde = model.arc[arcNr].safetyHurricane; // to not create a high cost compared to initial arc generation
				if (stormVarde > model.functions.valuesNow.worstStormValue)
					model.functions.valuesNow.worstStormValue = stormVarde;
			}

			if (fixTime <= 0) {
				if (model.arc[arcNr].fromLevel == 89)
					arcNr = arcNr;
				getAllVariableValues(i, tidTotArc);
				tidIntForecast = getTidIntForecast(tidTotArc);

				if (tidTotArc0 < model.network.tidp_startHistoricDataOnly) {
					uCurrent = model.functions.varValue[model.functions.pos_current_u]; // getVariableValue(model.functions.pos_current_u, i, tidTot);
					vCurrent = model.functions.varValue[model.functions.pos_current_v]; // getVariableValue(model.functions.pos_current_v, i, tidTot);
				}
				else {
					if (delayVersion != 5) {
						delayNr = getDelayPosFrom_tidp(tidTotArc);
						getCurrent_fromCurrentDelayed(delayNr, model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i], &uCurrent, &vCurrent);
					}
					else {
						uCurrent = 0;
						vCurrent = 0;
					}
				}

				if (uCurrent < 1000 && vCurrent < 1000) {
					currentDirection = ApproxAtan2(vCurrent, uCurrent);
					currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
					//if (tidTot >= model.weather[model.functions.pos_current_u].tidpHistoricalWeather)
					//	currentSpeed *= model.params.historicDataFactor_current;
				}
				else {
					currentDirection = 0;
					currentSpeed = 0;
					uCurrent = 9999;
					vCurrent = 9999;
				}

				baseGroundSpeed = eval_baseGroundSpeed(calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
					currentDirection, currentSpeed);
				if (printGlobal == 1) {
					printf("checkP %d vCurrent %.3lf uCurrent %.3lf, currentDirection %.3lf currentSpeed %.3lf baseGroundSpeed %.3lf\n", i, vCurrent,
						uCurrent, currentDirection, currentSpeed, baseGroundSpeed);
				}


				uWind = model.functions.varValue[model.functions.pos_wind_u]; // getVariableValue(model.functions.pos_wind_u, i, tidTot);
				vWind = model.functions.varValue[model.functions.pos_wind_v]; // getVariableValue(model.functions.pos_wind_v, i, tidTot);

				//errlog("arcNr %d i %d yx %.4lf %.4lf tidp %.2lf uWind %.2lf %.2lf vWind %.2lf %.2lf uCurr %.2lf %.2lf vCurr %.2lf %.2lf\n", arcNr, i,
				//	model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i], tidTot, uWind, uWind / 3.6, vWind, vWind / 3.6,
				//	uCurrent, uCurrent / 3.6, vCurrent, vCurrent / 3.6);

				if (uWind < 1000 && vWind < 1000) {
					windDirection = ApproxAtan2(vWind, uWind);
					windSpeed2 = uWind * uWind + vWind * vWind;
					windSpeed = sqrt(windSpeed2);
					//if (tidTot >= model.weather[model.functions.pos_wind_u].tidpHistoricalWeather)
					//	windSpeed *= model.params.historicDataFactor_windSpeed;

					rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
						windDirection, windSpeed, &rel_windDir);
#ifdef NAZANIN_SAFETY
					absWindDirDiff = eval_absWindDirDiff(model.weatherFunctions.vesselBearing[i], windDirection);
#endif
					if (printGlobal == 1) {
						printf("checkP %d vWind %.3lf uWind %.3lf, windDirection %.3lf windSpeed %.3lf rel_windSpeed %.3lf rel_windDir %.3lf\n", i, vWind,
							uWind, windDirection, windSpeed, rel_windSpeed, rel_windDir);
					}
					favorableWind = get_isWindFavorable(windSpeed, rel_windDir);
					//model.functions.valuesNow.worstStabilityValue += distNu * rel_windSpeed / 10000.0;
				}
				else {
					windSpeed = 0;
					windDirection = 0;
					rel_windDir = 0;
					rel_windSpeed = baseGroundSpeed;
					favorableWind = -1;
				}

				waveHeight = model.functions.varValue[model.functions.pos_waveHeight]; // getVariableValue(model.functions.pos_waveHeight, i, tidTot);
				wavePeriod = model.functions.varValue[model.functions.pos_wavePeriod]; // getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
				waveDirection = model.functions.varValue[model.functions.pos_waveDirection]; // getVariableValue(model.functions.pos_waveDirection, i, tidTot);
				waveHeightBase = waveHeight;
				waveDirectionBase = waveDirection;

				if (waveHeight > 100) {
					waveHeight = 0;
					favorableWave = -1;
				}
				else
					favorableWave = 1;
				//else {
				//	if (tidTot >= model.weather[model.functions.pos_waveHeight].tidpHistoricalWeather)
				//		waveHeight *= model.params.historicDataFactor_waveHeight;
				//}
				if (wavePeriod > 1000)
					wavePeriod = 0;
				if (waveDirection > 1000)
					waveDirection = 0;
				//rel_waveDir = (waveDirection -90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]; // / model.functions.nWaveDir;
				//rel_waveDir = M_PI - ((waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;
				rel_waveDir = M_PI + ((270 - waveDirection) * M_PI / 180 - model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;

				if (model.arc[arcNr].fromLevel == 14)
					arcNr = arcNr;
				if (rel_waveDir < 0)
					rel_waveDir = -rel_waveDir;
				if (rel_waveDir >= 2 * M_PI)
					rel_waveDir -= 2 * M_PI;
				if (rel_waveDir > M_PI)
					rel_waveDir = 2 * M_PI - rel_waveDir;
				if (printGlobal == 1) {
					printf("checkP %d waveDirection %.3lf rel_waveDir %.3lf waveHeight %.3lf wavePeriod %.3lf\n", i, waveDirection, rel_waveDir, waveHeight, wavePeriod);
				}

				//printGlobal = 1;
				// speedDiffWind = lookup_speedDiffWindTable(calmWaterSpeed, rel_windSpeed, rel_windDir);
				speedDiffWind = lookup_speedDiffWindTable(baseGroundSpeed, rel_windSpeed, rel_windDir);
				speedDiffWave = lookup_speedDiffWaveTable(calmWaterSpeed, waveHeight, wavePeriod, rel_waveDir);

				if (favorableWave >= 0)
					favorableWave = get_isSeaFavorable(waveHeight, rel_waveDir);

				//if (model.arc[arcNr].fromLevel == 76)
				//	printf("levels %d %d nodPos %d %d startKvot %.3lf wind\nshipSpeed_knots %lf\nwind_RWS_m_s %lf\nWind_RWiA_degrees %lf\nWind_WF_kts %lf\n"
				//		"wave\nshipSpeed_knots %lf\nwaveHeight_m %lf\nwavePeriod_s %lf\nwave_RWaA_degrees %lf\nwave_WF_kts %lf\n",
				//		model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].toPointNr, startKvot,
				//		baseGroundSpeed / model.params.knots_to_km, rel_windSpeed / 3.6, rel_windDir * 180 / M_PI,
				//		speedDiffWind / model.params.knots_to_km,
				//		calmWaterSpeed / model.params.knots_to_km, waveHeight, wavePeriod, rel_waveDir * 180 / M_PI,
				//		speedDiffWave / model.params.knots_to_km);
				speedDiffWindWave = speedDiffWind + speedDiffWave;
				// speedOverGround = baseGroundSpeed * model.params.knots_to_km - speedDiffWindWave; // in km/h
				if (model.delay.nYears > 0 && delayVersion >= 4)
					speedOverGround = calmWaterSpeed - speedDiffWindWave; // in km/h
				else
					speedOverGround = baseGroundSpeed - speedDiffWindWave; // in km/h

				if (speedOverGround < model.params.knots_to_km)
					speedOverGround = model.params.knots_to_km;

				if (delayFactor < 0) {
					timeArc = distNu / speedOverGround * useKvotNu; // in hours

					tidTotArc += distNu / speedOverGround;
					if (dist <= distStart) {
						continue;
					}

					if (tidTotArc + timeArc > model.functions.timeNextWayPoint)
						kvotTimeUse = (model.functions.timeNextWayPoint - (tidTotArc)) / timeArc;
					else
						kvotTimeUse = 1;
					timeArcUse = timeArc * kvotTimeUse;
					timeArcSTW = distNu / calmWaterSpeed * useKvotNu * kvotTimeUse; // in hours
					model.functions.valuesNow.WindF -= speedDiffWind * timeArcSTW;
					//printf("arcNr1b %d kvots %.4lf %.4lf wPoint %d time %.3lf timeArc %.3lf %.3lf timeArcSTW %.3lf windFs %.3lf %.3lf\n", 
					//	arcNr, startKvot, kvotTimeUse,
					//	i, tidTot, timeArc, timeArc * kvotTimeUse, timeArcSTW, model.functions.valuesNow.WindF, model.functions.valuesNow.WindF + model.functions.valuesNow.totWindF);
					model.functions.valuesNow.WaveF -= speedDiffWave * timeArcSTW;
					model.functions.valuesNow.CurrentF += (baseGroundSpeed - calmWaterSpeed) * timeArcSTW;
					model.functions.valuesNow.totTimeArcSTW += timeArcSTW;
					model.functions.valuesNow.WindFArc -= speedDiffWind * timeArcSTW;
					model.functions.valuesNow.WaveFArc -= speedDiffWave * timeArcSTW;
					model.functions.valuesNow.CurrentFArc += (baseGroundSpeed - calmWaterSpeed) * timeArcSTW;

					model.functions.valuesNow.favorableWind[0][favorableWind + 1] += timeArcUse;
					model.functions.valuesNow.favorableWave[0][favorableWave + 1] += timeArcUse;
					if (favorableWind == 1 && favorableWave == 1)
						model.functions.valuesNow.favorableWindWave[0][1 + 1] += timeArcUse;
					else {
						if (favorableWind == -1 || favorableWave == -1)
							model.functions.valuesNow.favorableWindWave[0][0] += timeArcUse;
						else
							model.functions.valuesNow.favorableWindWave[0][1] += timeArcUse;
					}

				}
				else {
					timeArc = distNu / calmWaterSpeed * delayFactor * useKvotNu;

					tidTotArc += distNu / calmWaterSpeed * delayFactor;
					if (dist <= distStart) {
						continue;
					}

					if (tidTotArc + timeArc > model.functions.timeNextWayPoint)
						kvotTimeUse = (model.functions.timeNextWayPoint - (tidTotArc)) / timeArc;
					else
						kvotTimeUse = 1;
					timeArcSTW = distNu / calmWaterSpeed * useKvotNu * kvotTimeUse; // in hours
					timeArcUse = timeArc * kvotTimeUse;
					model.functions.valuesNow.favorableWind[1][favorableWind + 1] += timeArcUse;
					model.functions.valuesNow.favorableWave[1][favorableWave + 1] += timeArcUse;
					if (favorableWind == 1 && favorableWave == 1)
						model.functions.valuesNow.favorableWindWave[1][1 + 1] += timeArcUse;
					else {
						if (favorableWind == -1 || favorableWave == -1)
							model.functions.valuesNow.favorableWindWave[1][0] += timeArcUse;
						else
							model.functions.valuesNow.favorableWindWave[1][1] += timeArcUse;
					}

					tidInt = (int)(tidTotArc) / model.params.tIndexGerH;
					checkFactor = eval_factorDelayedAlongArc_currSpeedDiff(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr,
						tidInt, &checkSpeedDiffCurrent, calmWaterSpeed);

					model.functions.valuesNow.CurrentF += checkSpeedDiffCurrent * timeArcSTW;
					model.functions.valuesNow.CurrentFArc += checkSpeedDiffCurrent * timeArcSTW;

					model.functions.valuesNow.DelayF += (calmWaterSpeed * (1 / delayFactor - 1) - checkSpeedDiffCurrent) * timeArcSTW;
					model.functions.valuesNow.totTimeArcSTW += timeArcSTW;
					model.functions.valuesNow.DelayFArc += (calmWaterSpeed * (1 / delayFactor - 1) - checkSpeedDiffCurrent) * timeArcSTW;
					//model.functions.valuesNow.DelayFArc += calmWaterSpeed * (1 / delayFactor - 1) * timeArcSTW;

					if (SKRIV_UT_NOTHING == 100) {
						errlog("bbb %d %lf %.2lf %d %lf delayFactor %lf %lf %lf"
							" %lf %lf %lf %lf %lf %lf %lf %lf %lf"
							" %lf %lf %lf %lf\n",
							arcNr, startKvot, kvotTimeUse, i, tidTot, delayFactor, calmWaterSpeed, delayFactor,
							distNu / model.params.knots_to_km * useKvotNu, timeArcSTW, timeArc, checkSpeedDiffCurrent,
							0.0, 0.0, (calmWaterSpeed * (1 / delayFactor - 1) - checkSpeedDiffCurrent) * timeArcSTW,
							(calmWaterSpeed * (1 / delayFactor - 1) * timeArcSTW) / model.params.knots_to_km,
							(calmWaterSpeed * (1 / delayFactor - 1) * timeArcSTW) / distNu, 0.0, 0.0, 0.0,
							calmWaterSpeed * (1 / delayFactor - 1) / model.params.knots_to_km);
						//glob_tmpTotDist += (model.functions.valuesNow.DelayF) / model.params.knots_to_km;
					}
				}

				if (model.functions.valuesNow.maxCurrent < currentSpeed)
					model.functions.valuesNow.maxCurrent = currentSpeed;
				if (model.params.useSimulering == 1) {
					if (model.functions.valuesNow.maxWindSpeed < windSpeed) {
						if (windSpeed > model.params.user_maxWindSpeed_kmh && windSpeed > model.arc[arcNr].maxWindSpeed)
							windSpeed = model.arc[arcNr].maxWindSpeed;
					}
					if (model.functions.valuesNow.maxWaveHeight < waveHeight) {
						if (waveHeight > model.params.user_maxWaveHeight && waveHeight > model.arc[arcNr].maxWaveHeight)
							waveHeight = model.arc[arcNr].maxWaveHeight;
					}
				}
				if (model.functions.valuesNow.maxWindSpeed < windSpeed &&
					model.weather[model.functions.pos_wind_u].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_wind_u].nTimeIntervals_forecast) {
					model.functions.valuesNow.maxWindSpeed = windSpeed;
					model.functions.valuesNow.maxWindSpeed_tp = tidTot;
					model.functions.valuesNow.maxWindSpeed_dir = windDirection * 180 / M_PI;
				}
				if (model.functions.valuesNow.maxWaveHeight < waveHeight &&
					model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
					model.functions.valuesNow.maxWaveHeight = waveHeight;
					model.functions.valuesNow.maxWaveHeight_tp = tidTot;
					model.functions.valuesNow.maxWaveHeight_dir = 270 - waveDirection;
					if (model.functions.valuesNow.maxWaveHeight > 9)
						model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
				}


				model.functions.valuesNow.speedOnWater = calmWaterSpeed;
				model.functions.valuesNow.sumWindSpeed += windSpeed * timeArcUse;
				model.functions.valuesNow.sumRelCurrent += (baseGroundSpeed - calmWaterSpeed) * timeArcUse;
				model.functions.valuesNow.sumCurrent += currentSpeed * timeArcUse;
				model.functions.valuesNow.sumWaveHeight += waveHeight * timeArcUse;

				fuelConsumption_main = eval_fuelConsumption_both(model.arc[arcNr].speedSetting, &fuelConsumption_aux,
					model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				if (arcNr == 22)
					arcNr = arcNr; // checkpfg
				fuelUsage_main = fuelConsumption_main * timeArcUse;
				fuelUsage_aux = fuelConsumption_aux * timeArcUse;
				if (model.arc[arcNr].fromLevel < 0 && model.arc[arcNr].toLevel < 0) {
					fuelUsage_main += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_main * kvotTimeUse;
					// fuelUsage_aux -= fuelConsumption_aux * model.network.channel[-model.arc[arcNr].fromLevel - 1].waitingTime;
					fuelUsage_aux += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_aux * kvotTimeUse;
				}


				fuelTot_main += fuelUsage_main;
				fuelTot_aux += fuelUsage_aux;

				if (printGlobal == 1) {
					printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf\n",
						speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu);
				}

				model.functions.valuesNow.forecastType += identifyForecastType(tidTotArc) * timeArcUse;

				tidTot += timeArcUse;

				iceCover = model.functions.varValue[model.functions.pos_iceThickness]; // getVariableValue(model.functions.pos_iceThickness, i, tidTot);
				if (iceCover > 1000)
					iceCover = 0;
				if (iceCover > model.functions.valuesNow.iceCover_max)
					model.functions.valuesNow.iceCover_max = iceCover;

#ifdef NAZANIN_SAFETY
				eval_safety_nazanin(speedOverGround, (calmWaterSpeed) - speedDiffWindWave, windSpeed, absWindDirDiff, waveHeight, wavePeriod, rel_waveDir, iceCover);
#else
				eval_safety(iceCover);
#endif
				//if (model.functions.valuesNow.bowSlam > model.functions.valuesNow.bowSlamming_max)
				//	model.functions.valuesNow.bowSlamming_max = model.functions.valuesNow.bowSlam;
				//if (model.functions.valuesNow.greenWater > model.functions.valuesNow.greenWater_max)
				//	model.functions.valuesNow.greenWater_max = model.functions.valuesNow.greenWater;
				//if (model.functions.valuesNow.dynamicStability > model.functions.valuesNow.dynamicStability_max)
				//	model.functions.valuesNow.dynamicStability_max = model.functions.valuesNow.dynamicStability;

				model.functions.valuesNow.current += timeArcUse * (baseGroundSpeed - calmWaterSpeed);
				if (printGlobal == 1)
					printf(" == arcNr %d timeArc %.2lf baseGroundSpeed %.3lf calmWaterSpeed %.3lf current %.3lf\n",
						arcNr, timeArc, baseGroundSpeed, calmWaterSpeed, model.functions.valuesNow.current);
				if (uCurrent < 1000 && vCurrent < 1000) {
					currentReal_u += timeArcUse * uCurrent;
					currentReal_v += timeArcUse * vCurrent;
					timeArc_current += timeArcUse;
				}

				model.functions.valuesNow.windSpeed += timeArcUse * rel_windSpeed; // windSpeed;

				if (uWind < 1000 && vWind < 1000) {
					windSpeedReal_u += timeArcUse * uWind;
					windSpeedReal_v += timeArcUse * vWind;
					timeArc_wind += timeArcUse;
				}
				model.functions.valuesNow.waveHeight += timeArcUse * waveHeight;
				model.functions.valuesNow.wavePeriod += timeArcUse * wavePeriod;

				model.functions.valuesNow.speedDiffWind += speedDiffWind * timeArcUse;
				model.functions.valuesNow.speedDiffWave += speedDiffWave * timeArcUse;
				model.functions.valuesNow.baseGroundSpeed += baseGroundSpeed * timeArcUse;

				if (waveHeightBase <= 100 && waveDirectionBase <= 1000) {
					//waveHeightReal_u += timeArc * waveHeight * cos((90 - waveDirection) / 180 * M_PI);
					//waveHeightReal_v += timeArc * waveHeight * sin((90 - waveDirection) / 180 * M_PI);
					waveHeightReal_u += timeArcUse * waveHeight * lookUpCos((270 - waveDirection) / 180 * M_PI);
					waveHeightReal_v += timeArcUse * waveHeight * lookUpSin((270 - waveDirection) / 180 * M_PI);
					timeArc_wave += timeArcUse;
				}
				else
					timeArc = timeArc;

				//if (printGlobal == 1)
				//	printf("wavePeriod %.3lf timeArc %.3lf tidTot %.3lf tot %.3lf\n", wavePeriod, timeArc, tidTot- timeExact, model.functions.valuesNow.wavePeriod);

				//printf("arcNr %d pos %d baseGroundSpeed %.2lf calmWaterSpeed %.2lf timeArc %.2lf currentAcc %.2lf\n", arcNr, i,
				//	baseGroundSpeed, calmWaterSpeed, timeArc, model.functions.valuesNow.current);
				//windSpeed_x += timeArc * windSpeed * cos(rel_windDir);
				//windSpeed_y += timeArc * windSpeed * sin(rel_windDir);
				windSpeed_x += timeArcUse * rel_windSpeed * lookUpCos(rel_windDir);
				windSpeed_y += timeArcUse * rel_windSpeed * lookUpSin(rel_windDir);
				waveDir_x += timeArcUse * waveHeight * lookUpCos(rel_waveDir);
				waveDir_y += timeArcUse * waveHeight * lookUpSin(rel_waveDir);

				if (model.nWeatherFiles > 8) {
					if (model.functions.pos_pressureSurface >= 0 && model.functions.varValue[model.functions.pos_pressureSurface] > 10000) {
						model.functions.valuesNow.pressureSurface += timeArcUse * model.functions.varValue[model.functions.pos_pressureSurface];
						model.functions.valuesNow.timePressureSurface += timeArcUse;
					}
					if (model.functions.pos_pressureAir >= 0 && model.functions.varValue[model.functions.pos_pressureAir] < 9998) {
						model.functions.valuesNow.pressureAir += timeArcUse * model.functions.varValue[model.functions.pos_pressureAir];
						model.functions.valuesNow.timePressureAir += timeArcUse;
					}
					if (model.functions.pos_precipitation >= 0 && model.functions.varValue[model.functions.pos_precipitation] < 9998) {
						model.functions.valuesNow.precipitation += timeArcUse * model.functions.varValue[model.functions.pos_precipitation];
						model.functions.valuesNow.timePrecipitation += timeArcUse;
					}
					if (model.functions.pos_tempSea >= 0 && model.functions.varValue[model.functions.pos_tempSea] < 9998) {
						model.functions.valuesNow.tempSea += timeArcUse * model.functions.varValue[model.functions.pos_tempSea];
						model.functions.valuesNow.timeTempSea += timeArcUse;
					}
					if (model.functions.pos_tempAir >= 0 && model.functions.varValue[model.functions.pos_tempAir] < 9998) {
						model.functions.valuesNow.tempAir += timeArcUse * model.functions.varValue[model.functions.pos_tempAir];
						model.functions.valuesNow.timeTempAir += timeArcUse;
					}
					if (model.functions.pos_cloudCover >= 0 && model.functions.varValue[model.functions.pos_cloudCover] < 9998) {
						model.functions.valuesNow.cloudCover += timeArcUse * model.functions.varValue[model.functions.pos_cloudCover];
						model.functions.valuesNow.timeCloudCover += timeArcUse;
					}
					if (model.functions.pos_mdps >= 0 && model.functions.varValue[model.functions.pos_mdps] < 9998) {
						model.functions.valuesNow.mdps += timeArcUse * model.functions.varValue[model.functions.pos_mdps];
						model.functions.valuesNow.timeMdps += timeArcUse;
					}
					if (model.functions.pos_swell >= 0 && model.functions.varValue[model.functions.pos_swell] < 9998) {
						model.functions.valuesNow.swell += timeArcUse * model.functions.varValue[model.functions.pos_swell];
						model.functions.valuesNow.timeSwell += timeArcUse;
					}
				}
			}
			else {
				// channel with fix speed....
				timeArc = distNu / calmWaterSpeed * useKvotNu; // in hours

				tidTotArc += distNu / calmWaterSpeed;
				if (dist <= distStart) {
					continue;
				}

				if (tidTotArc + timeArc > model.functions.timeNextWayPoint)
					kvotTimeUse = (model.functions.timeNextWayPoint - (tidTotArc)) / timeArc;
				else
					kvotTimeUse = 1;

				timeArcUse = timeArc * kvotTimeUse;

				model.functions.valuesNow.favorableWind[1][0] += timeArcUse;
				model.functions.valuesNow.favorableWave[1][0] += timeArcUse;
				model.functions.valuesNow.favorableWindWave[1][0] += timeArcUse;

				//if (model.arc[arcNr].fromLevel < 0) {
				//	if (model.network.channel[-model.arc[arcNr].fromLevel - 1].totalConsumption < 0) {
				//		fuelConsumption_main = eval_fuelConsumption_both(model.arc[arcNr].speedSetting, &fuelConsumption_aux,
				//			model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				//		fuelUsage_main = fuelConsumption_main * timeArc;
				//	}
				//	else {
				//		fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_use, &fuelConsumption_aux, -1, -100);
				//		fuelConsumption_main = model.network.channel[-model.arc[arcNr].fromLevel - 1].totalConsumption;
				//		fuelUsage_main = fuelConsumption_main / model.weatherFunctions.nCheckPoints; // *timeArc;
				//	}

				//}
				//else {
				fuelConsumption_main = eval_fuelConsumption_both(model.arc[arcNr].speedSetting, &fuelConsumption_aux,
					model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				fuelUsage_main = fuelConsumption_main * timeArcUse;
				//}

				fuelUsage_aux = fuelConsumption_aux * timeArcUse;

				if (model.arc[arcNr].fromLevel < 0 && i == 0) {
					fuelUsage_main += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_main * kvotTimeUse;
					//fuelUsage_aux -= fuelConsumption_aux * model.network.channel[-model.arc[arcNr].fromLevel - 1].waitingTime; already removed...
					fuelUsage_aux += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_aux * kvotTimeUse;
				}

				fuelTot_main += fuelUsage_main;
				fuelTot_aux += fuelUsage_aux;

				tidTot += timeArcUse;

			}
			if (tidTotArc > model.functions.timeNextWayPoint - 0.01) {
				//if (arcNr == 62 && tidTot > 542)
				//	arcNr = arcNr;

				*usedKvot = (dist - (1 - kvotTimeUse) * distNu * useKvotNu) / distEnd;
				break; // time for a new way point
			}
		}
		if (tidTotArc <= model.functions.timeNextWayPoint - 0.01)
			*usedKvot = 1;

		model.functions.valuesNow.fuel_aux += fuelTot_aux;
		model.functions.valuesNow.fuel_main += fuelTot_main;
		model.functions.valuesNow.arcDel_fuel_aux = fuelTot_aux;
		model.functions.valuesNow.arcDel_fuel_main = fuelTot_main;

		model.functions.valuesNow.fuel_eca += (fuelTot_aux + fuelTot_main) * (1 - model.arc[arcNr].fuelQualityKvot);

		deltaTid = tidTot - timeExact;
		if (deltaTid > 0) {
			model.functions.valuesNow.deltaTid += deltaTid;
			//model.functions.valuesNow.current /= deltaTid;
			//model.functions.valuesNow.windSpeed /= deltaTid;
			//model.functions.valuesNow.waveHeight /= deltaTid;
			//model.functions.valuesNow.wavePeriod /= deltaTid;
			//model.functions.valuesNow.speedDiffWind /= deltaTid;
			//model.functions.valuesNow.speedDiffWave /= deltaTid;
			//model.functions.valuesNow.baseGroundSpeed /= deltaTid;

			model.functions.valuesNow.currentReal_u += currentReal_u;
			model.functions.valuesNow.currentReal_v += currentReal_v;
			model.functions.valuesNow.timeArc_current += timeArc_current;
			//if (timeArc_current > 0.001) {
			//	currentReal_u /= timeArc_current;
			//	currentReal_v /= timeArc_current;
			//	model.functions.valuesNow.currentReal = sqrt(currentReal_u * currentReal_u + currentReal_v * currentReal_v);
			//	model.functions.valuesNow.currentDirReal = atan2(currentReal_v, currentReal_u) * 180.0 / M_PI;
			//}
			//else {
			//	model.functions.valuesNow.currentReal = 9999;
			//	model.functions.valuesNow.currentDirReal = 0;
			//}

			model.functions.valuesNow.windSpeedReal_u += windSpeedReal_u;
			model.functions.valuesNow.windSpeedReal_v += windSpeedReal_v;
			model.functions.valuesNow.timeArc_wind += timeArc_wind;
			//if (timeArc_wind > 0.001) {
			//	windSpeedReal_u /= timeArc_wind;
			//	windSpeedReal_v /= timeArc_wind;
			//	model.functions.valuesNow.windReal = sqrt(windSpeedReal_u * windSpeedReal_u + windSpeedReal_v * windSpeedReal_v);
			//	model.functions.valuesNow.windDirReal = atan2(windSpeedReal_v, windSpeedReal_u) * 180.0 / M_PI;
			//}
			//else {
			//	model.functions.valuesNow.windReal = 9999;
			//	model.functions.valuesNow.windDirReal = 0;
			//}

			model.functions.valuesNow.waveHeightReal_u += waveHeightReal_u;
			model.functions.valuesNow.waveHeightReal_v += waveHeightReal_v;
			model.functions.valuesNow.timeArc_wave = timeArc_wave;
			//if (timeArc_wave > 0.001)
			//	model.functions.valuesNow.waveDirReal = atan2(waveHeightReal_v, waveHeightReal_u) * 180.0 / M_PI;
			//else
			//	model.functions.valuesNow.waveDirReal = 9999;

			//if (printGlobal == 1)
			//	printf("wavePeriod %.3lf timeArc %.3lf\n", model.functions.valuesNow.wavePeriod, deltaTid);

			//model.functions.valuesNow.forecastType /= (deltaTid * 8);
		}
		//else
		//{
		//	model.functions.valuesNow.currentReal = 0;
		//	model.functions.valuesNow.currentDirReal = 0;
		//	model.functions.valuesNow.windReal = 9999;
		//	model.functions.valuesNow.windDirReal = 0;
		//	model.functions.valuesNow.waveDirReal = 0;
		//}

		if (printGlobal == 1)
			printf(" == arcNr %d tot deltaTid %.3lf current %.3lf\n",
				arcNr, deltaTid, model.functions.valuesNow.current);

		model.functions.valuesNow.windSpeed_x += windSpeed_x * deltaTid;
		model.functions.valuesNow.windSpeed_y += windSpeed_y * deltaTid;
		//model.functions.valuesNow.relWindDir = atan2(windSpeed_y, windSpeed_x) * 180.0 / M_PI;
		//if (model.functions.valuesNow.relWindDir < 0)
		//	model.functions.valuesNow.relWindDir = -model.functions.valuesNow.relWindDir;
		//printf("wind_y %.2lf wind_x %.2lf relWindDir %.2lf\n",
		//	windSpeed_y, windSpeed_x, model.functions.valuesNow.relWindDir);
		model.functions.valuesNow.waveDir_x += waveDir_x * deltaTid;
		model.functions.valuesNow.waveDir_y += waveDir_y * deltaTid;
		//model.functions.valuesNow.relWaveDir = atan2(waveDir_y, waveDir_x) * 180.0 / M_PI;
		//if (model.functions.valuesNow.relWaveDir < 0)
		//	model.functions.valuesNow.relWaveDir = -model.functions.valuesNow.relWaveDir;
	}

	//model.functions.valuesNow.totWindF += model.functions.valuesNow.WindF;
	//model.functions.valuesNow.totWaveF += model.functions.valuesNow.WaveF;
	//model.functions.valuesNow.totCurrentF += model.functions.valuesNow.CurrentF;
	//model.functions.valuesNow.totDelayF += model.functions.valuesNow.DelayF;

	//if (arcNr == 412305)
	//	errlog("arcNr %d startTime %.3lf endTime %.3lf in ..Section timeArc %.3lf\n", arcNr, timeExact, tidTot, (tidTot - timeExact) / (endKvot - startKvot));
	return tidTot;
}

void set_tmBasTime(double timeNu) {
	int timmar, minuter, sekunder;
	model.params.tmBas.tm_year = model.params.startYear - 1900;
	model.params.tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	model.params.tmBas.tm_mday = model.params.startDay_nr;
	timmar = (int)(timeNu); // *24;
	model.params.tmBas.tm_hour = model.params.startHour + timmar;
	minuter = (int)(round((timeNu - timmar) * 60.0));
	model.params.tmBas.tm_min = model.params.startMinute + minuter;
	sekunder = (int)(round((timeNu - timmar - minuter / 60.0) * 60.0));
	model.params.tmBas.tm_sec = sekunder;
	model.params.testTime = mktime(&(model.params.tmBas));
	if (model.params.tmBas.tm_min >= 55) {
		errlog("OBS! Rounding tm_min from %d to 60\n", model.params.tmBas.tm_min);
		model.params.tmBas.tm_min = 60;
		model.params.testTime = mktime(&(model.params.tmBas));
	}
	if (model.params.tmBas.tm_min <= 5) {
		errlog("OBS! Rounding tm_min from %d to 0\n", model.params.tmBas.tm_min);
		model.params.tmBas.tm_min = 0;
	}

	if (model.params.testTime == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			model.params.tmBas.tm_year,
			model.params.tmBas.tm_mon, model.params.tmBas.tm_mday, model.params.tmBas.tm_hour, model.params.tmBas.tm_min, model.params.tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}

}

int eval_xy_from_arcKvot(int arcNr, spherical::Point p1, spherical::Point p2, int prefPath, double usedKvot) {
	int i;
	int lev1 = model.arc[arcNr].fromLevel;
	int lev2 = model.arc[arcNr].toLevel;
	int pointNr1 = model.arc[arcNr].fromPointNr;
	int pointNr2 = model.arc[arcNr].toPointNr;
	double dist = model.arc[arcNr].distance, distNu, bearing, totDist, distTmp;
	spherical::Point p3;
	distNu = usedKvot * dist;

	if (prefPath == 0) {
		if (lev1 >= 0 || lev2 >= 0 || lev1 != lev2) {
			bearing = p1.bearingTo(p2);
			p3 = p1.destinationPoint(distNu * 1000, bearing);
		}
		else {
			// move along the channel till the distance is reached
			for (i = 1; i < model.network.channel[-lev1 - 1].nPoints; i++) {
				if (model.network.channel[-lev1 - 1].distanceFromStart[i] >= distNu) {
					bearing = model.network.channel[-lev1 - 1].point[i - 1].bearingTo(model.network.channel[-lev1 - 1].point[i]);
					distNu -= model.network.channel[-lev1 - 1].distanceFromStart[i - 1];
					p3 = model.network.channel[-lev1 - 1].point[i - 1].destinationPoint(distNu * 1000, bearing);
					break;
				}
			}
			if (i >= model.network.channel[-lev1 - 1].nPoints) {
				errlog("ERROR! The corridor %d is not long enough. I use the last point\n", -lev1 - 1);
				p3 = p2;
			}
		}
	}
	else {
		// prefPath
		// move along the prefPath till the distance is reached
		totDist = 0;
		for (i = 0; i < model.network.physicalLev[lev1].npreferredPathPoints; i++) {
			distTmp = p1.distanceTo(model.network.physicalLev[lev1].preferredPathPoint[i]) / 1000;
			if (totDist + distTmp >= distNu) {
				bearing = p1.bearingTo(model.network.physicalLev[lev1].preferredPathPoint[i]);
				distTmp = distNu - totDist;
				p3 = p1.destinationPoint(distTmp * 1000, bearing);
				break;
			}
			totDist += distTmp;
			p1 = model.network.physicalLev[lev1].preferredPathPoint[i];
		}
		if (i >= model.network.physicalLev[lev1].npreferredPathPoints) {
			errlog("ERROR! The prefPath from lev %d is not long enough. I use the last point\n", lev1);
			p3 = p2;
		}
	}

	model.functions.valuesNow.last_x = p3.longitude().degrees();
	model.functions.valuesNow.last_y = p3.latitude().degrees();

	return 0;
}

int addCoordsToPath_kvots(int arcNr, spherical::Point p1, spherical::Point p2, int prefPath, double kvotNu) {
	double distNu, wantedDist, kvot;
	int i3b;
	int i, cNr, level, ii, startPos, endPos;
	spherical::Point p3, pointLast;
	double x, y, bearing, distTmp, calmWaterSpeed, distBas;
	double distLastSplit, distArc = model.arc[arcNr].distance * 1000.0, distTot = 0, startKvot;
	double distStop, distStart;

	if (model.network.nCoords + 500 >= model.network.nAllocCoords) {
		model.network.nAllocCoords += 500;
		model.network.xCoord = (double*)realloc(model.network.xCoord, model.network.nAllocCoords * sizeof(double));
		model.network.yCoord = (double*)realloc(model.network.yCoord, model.network.nAllocCoords * sizeof(double));
	}

	if (model.arc[arcNr].fromLevel != model.functions.valuesNow.coords_lastFromLevel ||
		model.arc[arcNr].toLevel != model.functions.valuesNow.coords_lastToLevel) {
		startKvot = 0.0;
		model.functions.valuesNow.coords_lastFromLevel = model.arc[arcNr].fromLevel;
		model.functions.valuesNow.coords_lastToLevel = model.arc[arcNr].toLevel;
	}
	else
		startKvot = model.functions.valuesNow.coords_lastUsedKvot;
	model.functions.valuesNow.coords_lastUsedKvot = kvotNu;

	if (model.arc[arcNr].fromLevel >= 44)
		arcNr = arcNr;


	if (kvotNu > 0.9999)
		kvotNu = 2;

	if (prefPath == 1) {
		pointLast = p1;
		if (startKvot < 0.0001) {
			model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
			model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
			(model.network.nCoords)++;
		}
		level = model.arc[arcNr].fromLevel;
		startPos = 0;
		if (model.arc[arcNr].fromLevel < 0) {
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

		distStop = model.arc[arcNr].distance * kvotNu;
		if (startKvot < 0.0001)
			distStart = -100;
		else
			distStart = model.arc[arcNr].distance * startKvot;

		distTot = 0;
		for (int i3 = startPos; i3 < endPos; i3++) {
			distTot += pointLast.distanceTo(model.network.physicalLev[level].preferredPathPoint[i3]) / 1000;
			if (distTot >= distStop - 0.0001) {
				model.network.yCoord[model.network.nCoords] = model.functions.valuesNow.last_y;
				model.network.xCoord[model.network.nCoords] = model.functions.valuesNow.last_x;
				(model.network.nCoords)++;
				break;
			}
			if (distTot > distStart + 0.001) {
				model.network.yCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].latitude().degrees();
				model.network.xCoord[model.network.nCoords] = model.network.physicalLev[level].preferredPathPoint[i3].longitude().degrees();
				(model.network.nCoords)++;
			}
			pointLast = model.network.physicalLev[level].preferredPathPoint[i3];
		}
		return 0;
	}

	// if corridor
	if (model.arc[arcNr].fromLevel < 0 && model.arc[arcNr].toLevel < 0 && model.arc[arcNr].fromLevel == model.arc[arcNr].toLevel) {
		cNr = -model.arc[arcNr].fromLevel - 1;
		distStop = model.network.channel[cNr].distance_km * kvotNu;
		if (startKvot < 0.0001)
			distStart = -100;
		else
			distStart = model.network.channel[cNr].distance_km * startKvot;
		for (i = 0; i < model.network.channel[cNr].nPoints; i++) {
			if (i < model.network.channel[cNr].nPoints - 1) {
				if (model.network.channel[cNr].distanceFromStart[i] >= distStop - 0.0001) {
					model.network.yCoord[model.network.nCoords] = model.functions.valuesNow.last_y;
					model.network.xCoord[model.network.nCoords] = model.functions.valuesNow.last_x;
					(model.network.nCoords)++;
					break;
				}
				if (model.network.channel[cNr].distanceFromStart[i] > distStart + 0.001) {
					model.network.yCoord[model.network.nCoords] = model.network.channel[cNr].point[i].latitude().degrees();
					model.network.xCoord[model.network.nCoords] = model.network.channel[cNr].point[i].longitude().degrees();
					(model.network.nCoords)++;
				}
			}
			else {
				if (kvotNu < 0.9999) {
					x = model.functions.valuesNow.last_x;
					y = model.functions.valuesNow.last_y;
					model.network.xCoord[model.network.nCoords] = x;
					model.network.yCoord[model.network.nCoords] = y;
					(model.network.nCoords)++;
				}
			}
		}
		return 0;
	}

	// else straight arc
	if (startKvot < 0.0001) {
		x = p1.longitude().degrees();
		y = p1.latitude().degrees();
		model.network.xCoord[model.network.nCoords] = x;
		model.network.yCoord[model.network.nCoords] = y;
		(model.network.nCoords)++;
	}
	if (kvotNu < 0.9999) {
		x = model.functions.valuesNow.last_x;
		y = model.functions.valuesNow.last_y;
		model.network.xCoord[model.network.nCoords] = x;
		model.network.yCoord[model.network.nCoords] = y;
		(model.network.nCoords)++;
	}

	return 0;

}

int resetValuesNow() {
	model.functions.valuesNow.waveHeight = 0;
	model.functions.valuesNow.wavePeriod = 0;

	model.functions.valuesNow.forecastType = 0;
	model.functions.valuesNow.worstStormValue = 0;
	model.functions.valuesNow.iceCover_max = 0;
	model.functions.valuesNow.bowSlamming_max = 0;
	model.functions.valuesNow.greenWater_max = 0;
	model.functions.valuesNow.dynamicStability_max = 0;
	model.functions.valuesNow.rolling_max = 0;
	model.functions.valuesNow.surfRiding_max = 0;

	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0;
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;

	model.functions.valuesNow.relWindDir = 0;
	model.functions.valuesNow.relWaveDir = 0;

	model.functions.valuesNow.speedDiffWind = 0;
	model.functions.valuesNow.speedDiffWave = 0;
	model.functions.valuesNow.baseGroundSpeed = 0;

	model.functions.valuesNow.WindF = 0;
	model.functions.valuesNow.WaveF = 0;
	model.functions.valuesNow.CurrentF = 0;
	model.functions.valuesNow.DelayF = 0;

	model.functions.valuesNow.current = 0;
	model.functions.valuesNow.windSpeed = 0;

	model.functions.valuesNow.deltaTid = 0;
	model.functions.valuesNow.currentReal_u = 0;
	model.functions.valuesNow.currentReal_v = 0;
	model.functions.valuesNow.timeArc_current = 0;

	model.functions.valuesNow.windSpeedReal_u = 0;
	model.functions.valuesNow.windSpeedReal_v = 0;
	model.functions.valuesNow.timeArc_wind = 0;

	model.functions.valuesNow.waveHeightReal_u = 0;
	model.functions.valuesNow.waveHeightReal_v = 0;
	model.functions.valuesNow.timeArc_wave = 0;

	model.functions.valuesNow.windSpeed_x = 0;
	model.functions.valuesNow.windSpeed_y = 0;

	model.functions.valuesNow.waveDir_x = 0;
	model.functions.valuesNow.waveDir_y = 0;

	model.functions.valuesNow.timeSinceLast = 0;
	model.functions.valuesNow.distSinceLast = 0;
	model.functions.valuesNow.distSinceLastCalmWater = 0;

	model.functions.valuesNow.timeCheck = 0;
	model.functions.valuesNow.fuel_eca = 0;
	model.functions.valuesNow.waitingTime = 0;

	model.functions.valuesNow.fuel_aux = 0;
	model.functions.valuesNow.fuel_main = 0;

	if (model.nWeatherFiles > 10) {
		model.functions.valuesNow.pressureSurface = 0;
		model.functions.valuesNow.pressureAir = 0;
		model.functions.valuesNow.precipitation = 0;
		model.functions.valuesNow.tempSea = 0;
		model.functions.valuesNow.tempAir = 0;
		model.functions.valuesNow.cloudCover = 0;
		model.functions.valuesNow.timePressureSurface = 0;
		model.functions.valuesNow.timePressureAir = 0;
		model.functions.valuesNow.timePrecipitation = 0;
		model.functions.valuesNow.timeTempSea = 0;
		model.functions.valuesNow.timeTempAir = 0;
		model.functions.valuesNow.timeCloudCover = 0;

		model.functions.valuesNow.mdps = 0;
		model.functions.valuesNow.swell = 0;
		model.functions.valuesNow.timeMdps = 0;
		model.functions.valuesNow.timeSwell = 0;
	}

#ifdef NAZANIN_SAFETY
	model.results.bowSlam_notAllowed = 0;
	model.results.greenWater_notAllowed = 0;
	model.results.rolling_notAllowed = 0;
	model.results.dynamicStability_notAllowed = 0;
	model.results.surfRiding_notAllowed = 0;
	model.results.maxWaveHeight_notAllowed = 0;
	model.results.hurricane_insideOuterCircle = 0;
	model.results.hurricane_maxCost_insideOuterCircle = 0;
	model.results.hurricane_insideInnerCircle = 0;
	model.results.hurricane_maxCost_insideInnerCircle = 0;

	model.results.bowSlam_aver = 0;
	model.results.bowSlam_0 = 0;
	model.results.bowSlam_01 = 0;
	model.results.bowSlam_05 = 0;
	model.results.bowSlam_2 = 0;
	model.results.greenWater_aver = 0;
	model.results.greenWater_0 = 0;
	model.results.greenWater_01 = 0;
	model.results.greenWater_05 = 0;
	model.results.greenWater_2 = 0;
	model.results.dynamicStability_aver = 0;
	model.results.dynamicStability_0 = 0;
	model.results.dynamicStability_01 = 0;
	model.results.dynamicStability_05 = 0;
	model.results.dynamicStability_2 = 0;

	model.results.rolling_aver = 0;
	model.results.rolling_0 = 0;
	model.results.rolling_01 = 0;
	model.results.rolling_05 = 0;
	model.results.rolling_2 = 0;
	model.results.surfRiding_aver = 0;
	model.results.surfRiding_0 = 0;
	model.results.surfRiding_01 = 0;
	model.results.surfRiding_05 = 0;
	model.results.surfRiding_2 = 0;

	model.results.stormValue_aver = 0;
	model.results.worstStormValue_max = 0;
#endif

	return 0;
}

int addPositionDataToReport_equalTimeIntervals(FILE* filpekG, int* posReport, int arcNr, int startSlutArc, double* timeExact, std::string solName,
	int useFixCalmWaterSpeed, int iter, int nArcsLeft) {
	int lev1, lev2, pointNr1, pointNr2, timmar, minuter, sekunder;
	int nChangeBearingBetween, nChange_lessXdegrees, tidIntForecast;
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;

	double x, y, bearing, speedOnGround, fuel_day, fuel_dayCheck, diff, diffTime, fuel_dayAux;
	spherical::Point p1, p2, p3;

	//if (*posReport > 0 && filpekG != NULL && model.arc[arcNr].distance > 0.001)
	//	fprintf(filpekG, ",\n ");

	lev1 = model.arc[arcNr].fromLevel;
	lev2 = model.arc[arcNr].toLevel;
	pointNr1 = model.arc[arcNr].fromPointNr;
	pointNr2 = model.arc[arcNr].toPointNr;
	//printf("arcNr %d levels %d %d pointNr %d %d nPhysicalLevels %d\n", arcNr, lev1, lev2, pointNr1, pointNr2, model.network.nPhysicalLevels);

	int prefPath = 0;
	if (arcNr == 26)
		arcNr = arcNr;
	if (lev1 >= 0 && lev2 >= 0) {
		if (pointNr1 == model.params.preferredPathOrtoPos[lev1] && pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1
			&& (model.params.preferredPathStraightLineFeasibleFrom[lev1] == 0 || model.params.max_changeDirection == 0 || iter == 1)) {
			prefPath = 1;
		}

	}
	else {
		if (lev1 >= 0) {
			if ((model.network.channel[-lev2 - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0 || iter == 1) &&
				pointNr1 == model.params.preferredPathOrtoPos[lev1] && model.network.channel[-lev2 - 1].preferredPathPoint_posConnectTo >= 0)
				prefPath = 1;
		}
		if (lev2 >= 0) {
			if ((model.network.channel[-lev1 - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0 || iter == 1) &&
				pointNr2 == model.params.preferredPathOrtoPos[lev2] && model.network.channel[-lev1 - 1].preferredPathPoint_posConnectFrom >= 0)
				prefPath = 1;
		}
	}
	if (lev1 == 60)
		arcNr = arcNr;

	if (arcNr == 0)
		arcNr = arcNr;
	double fuelQualityKvot = 0;
	if (model.arc[arcNr].distance > 0.001) {
		//fuelQualityKvot = get_fuelQualityKvot(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
		//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr);
		fuelQualityKvot = 1 - get_extraAreaKvot(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
			model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, 0, 1);
	}
	strSpeed speedSetting;
	if (lev1 >= 0) {
		speedSetting = model.functions.speedLevel[lev1];
		p1 = model.network.physicalLev[lev1].point[pointNr1];
		if (lev2 >= 0) {
			if (lev2 < model.network.nPhysicalLevels) {
				p2 = model.network.physicalLev[lev2].point[pointNr2];
				if (startSlutArc == 0) {
					if (prefPath == 1)
						calcWeatherPosAlongpreferredPathArc(p1, lev1);
					else
						calcWeatherPosAlongArc(p1, p2);
				}
			}
			else
				p2 = p1;
		}
		else {
			p2 = model.network.channel[-lev2 - 1].point[0];
			if (startSlutArc == 0) {
				//if(prefPath == 1)
				//	calcWeatherPosAlongpreferredPathArc(p1, lev1); probably add lev2 here as well;
				//else
				// model.network.physicalLev[i1].requirePrefPathFeasible
				if (prefPath == 0)
					calcWeatherPosAlongArc(p1, p2);
				else
					calcWeatherPosAlongpreferredPathArc_connectChannel(p1, lev1, p2, lev2);
			}
		}
	}
	else {
		if (lev2 >= 0) {
			speedSetting = model.functions.speedChannelOut[-lev1 - 1];
			p1 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
			if (lev2 < model.network.nPhysicalLevels)
				p2 = model.network.physicalLev[lev2].point[pointNr2];
			else
				p2 = p1;
			if (startSlutArc == 0) {
				if (prefPath == 0)
					calcWeatherPosAlongArc(p1, p2);
				else
					calcWeatherPosAlongpreferredPathArc_connectChannel(p1, lev1, p2, lev2);
			}
		}
		else {
			speedSetting = model.functions.speedChannel[-lev1 - 1];
			if (lev1 == lev2) {
				p1 = model.network.channel[-lev1 - 1].point[0];
				p2 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
				//printGlobal = 1;
				if (startSlutArc == 0)
					calcWeatherPosAlongChannel(-lev1 - 1);
			}
			else {
				// between two corridors
				p1 = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1];
				p2 = model.network.channel[-lev2 - 1].point[0];
				//printGlobal = 1;
				if (startSlutArc == 0)
					calcWeatherPosAlongArc(p1, p2);
			}
		}
	}


	double timeCheck = model.arc[arcNr].time, accumTime = 0, startKvot, endKvot;
	int nSplit, ii;

	if (lev2 == 27)
		lev2 = lev2;
	if (arcNr == 60)
		arcNr = arcNr;
	if (model.params.wayPointHours <= 0) {
		if (lev2 < model.network.nPhysicalLevels) {
			// nSplit = genSplitsArc(arcNr, p1, p2, prefPath);
			if (arcNr == 3)
				arcNr = arcNr;
			nSplit = genSplitsArcNew2(arcNr, p1, p2, prefPath);
		}
		else { // last arc or a channel
			model.network.startKvot[0] = 0.0;
			model.network.endKvot[0] = 1.0;
			nSplit = 1;
			model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
			model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
			model.network.posSplitCoord[0] = model.network.nCoords;
			(model.network.nCoords)++;
		}
	}
	else {
		// model.functions.timeNextWayPoint
		model.network.startKvot[0] = 0.0;
		model.network.endKvot[0] = 1.0;
		nSplit = 1;
	}

	FILE* filpek10 = NULL;
	char* namn = NULL;
	if (model.network.nMaxSplits == 100000) {
		namn = (char*)malloc2(256 * sizeof(char));
		sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
		filpek10 = fopen(namn, "a+");
		fprintf(filpek10, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%d\t%.3lf\t",
			arcNr, nSplit, -1, -1, -1, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
			model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime, model.arc[arcNr].totCost,
			-1, model.arc[arcNr].distance, model.arc[arcNr].emission, model.arc[arcNr].fuelBase,
			model.arc[arcNr].safetyBase, model.arc[arcNr].speedSetting, model.arc[arcNr].time);
	}

	//printf("arcNr %d nSplit %d from xy %.3lf %.3lf to %.3lf %.3lf\n", arcNr, nSplit,
	//	p1.longitude().degrees(), p1.latitude().degrees(), p2.longitude().degrees(), p2.latitude().degrees());
	//errlog("arcNr %d nSplit %d from xy %.3lf %.3lf to %.3lf %.3lf\n", arcNr, nSplit,
	//	p1.longitude().degrees(), p1.latitude().degrees(), p2.longitude().degrees(), p2.latitude().degrees());

	if (model.arc[arcNr].fromLevel == 89)
		arcNr = arcNr;
	if (arcNr == 39216)
		arcNr = arcNr;
	if (model.arc[arcNr].fromLevel == 55)
		arcNr = arcNr;

	double calmWaterSpeed, delayFactor, timeOld = *timeExact;

	if (arcNr == 17)
		arcNr = arcNr;

	//if (*timeExact >= model.network.tidp_startHistoricDataOnly) {
	if (model.arc[arcNr].fromTime * model.params.tIndexGerH >= model.network.tidp_startHistoricDataOnly) {
		//if (model.arc[arcNr].fromLevel >= 0 || model.arc[arcNr].toLevel >= 0) {
		calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
		if (model.arc[arcNr].distance > 0.01)
			delayFactor = calmWaterSpeed * model.arc[arcNr].time / model.arc[arcNr].distance;
		else
			delayFactor = 1.0;
		//}
		//else {
		//	delayFactor = -1.0;
		//	errlog("OBS! Change this when you have time to use delay if variable speed in corridor\n");
		//}
	}
	else
		delayFactor = -1;

	if (arcNr == 17)
		arcNr = arcNr;

	double usedKvot = 0;
	if (USE_ARC_TIME_EXACT == 1) {
		if (arcNr >= 0)
			model.functions.valuesNow.deltaArcStart = model.arc[arcNr].fromTime * model.params.tIndexGerH - (*timeExact);
		else
			model.functions.valuesNow.deltaArcStart = 0;
	}
	// resetValuesNow();
	for (ii = 0; ii < nSplit; ii++) {
		//startKvot = model.network.startKvot[ii];
		//endKvot = model.network.endKvot[ii];
		//printf("split %d startKvot %.3lf endKvot %.3lf\n", ii, startKvot, endKvot);
		startKvot = usedKvot;
		tidIntForecast = getTidIntForecast(*timeExact);

		//if (model.functions.valuesNow.Wpt >= 21)
		//	printGlobal = 1;
		//if (model.arc[arcNr].time > 0.001 || model.arc[arcNr].distance > 0.001)
		if (model.arc[arcNr].fromLevel == 87 && ii == 1)
			arcNr = arcNr;
		timeCheck = evalWeatherDataAlongArcSection_equalTimeIntervals(arcNr, startKvot, &usedKvot, startSlutArc, *timeExact, delayFactor, useFixCalmWaterSpeed) - (*timeExact);
		if (timeCheck < 0.001)
			continue;

		if (model.arc[arcNr].fromTime * model.params.tIndexGerH >= model.network.tidp_startHistoricDataOnly) {
			timeCheck = model.arc[arcNr].time * (usedKvot - startKvot); // we use estimated delay then, don't use the one calculated with historical weather data
			model.functions.valuesNow.totFuel_aux += (model.arc[arcNr].fuel_aux + model.arc[arcNr].fuel_auxEca) * (usedKvot - startKvot);
			model.functions.valuesNow.totFuel_main += (model.arc[arcNr].fuel_eca + model.arc[arcNr].fuel_noEca) * (usedKvot - startKvot);
			// model.functions.valuesNow.fuel_main += (model.arc[arcNr].fuel_eca + model.arc[arcNr].fuel_noEca) * (usedKvot - startKvot);
		}
		else {
			model.functions.valuesNow.totFuel_aux += model.functions.valuesNow.arcDel_fuel_aux;
			model.functions.valuesNow.totFuel_main += model.functions.valuesNow.arcDel_fuel_main;
		}
		accumTime += timeCheck;
		*timeExact += timeCheck;

		// printf("nArcsLeft %d\n", nArcsLeft);
		//	model.params.useFixTimeToNextWayPoint == 1
		if (filpekG == NULL && solName == "-")
			return 0;

		if (timeCheck > 0.01)
			speedOnGround = model.arc[arcNr].distance * (1 - startKvot) / timeCheck;
		else
			speedOnGround = 0;

		if (model.arc[arcNr].fromLevel >= 0 && ii == 0) {
			diffTime = timeOld - model.network.physicalLev[model.arc[arcNr].fromLevel].midTimeArrive;
			if (diffTime > model.functions.valuesNow.maxDiffTime) {
				model.functions.valuesNow.maxDiffTime = diffTime;
				model.functions.valuesNow.maxDiffTime_level = model.arc[arcNr].fromLevel;
			}
			if (diffTime < model.functions.valuesNow.minDiffTime) {
				model.functions.valuesNow.minDiffTime = diffTime;
				model.functions.valuesNow.minDiffTime_level = model.arc[arcNr].fromLevel;
			}
		}
		else {
			diffTime = 0;
		}

		//model.functions.valuesNow.accumDistance += model.arc[arcNr].distance * (usedKvot - startKvot);

		if ((lev1 >= 0 || lev2 >= 0) && model.arc[arcNr].speedSetting >= 0) {
			if (model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid > model.functions.valuesNow.maxWaveHeight &&
				model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
				if (model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid > 20)
					arcNr = arcNr;
				model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid;
				model.functions.valuesNow.maxWaveHeight_tp = *timeExact - timeCheck;
				model.functions.valuesNow.maxWaveHeight_dir = model.functions.valuesNow.waveDirReal; // correct?
				if (model.functions.valuesNow.maxWaveHeight > 9)
					model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
			}

			model.functions.valuesNow.totFuel_mainMovingNoCorridors += model.functions.valuesNow.arcDel_fuel_main;
			model.functions.valuesNow.totTime_movingNoCorridors += timeCheck;
			model.functions.valuesNow.sumSpeedOnWater += model.functions.valuesNow.speedOnWater * timeCheck;

			model.functions.valuesNow.totDistance_movingNoCorridors += model.arc[arcNr].distance * (usedKvot - startKvot);

			if (model.functions.valuesNow.windReal < 1000 || model.functions.valuesNow.windDirReal_lastKnown < 1000) {
				if (model.functions.valuesNow.windReal < 1000) {
					model.functions.valuesNow.windDirReal_lastKnown = model.functions.valuesNow.windDirReal;
					model.functions.valuesNow.windReal_lastKnown = model.functions.valuesNow.windReal;
				}
				if (model.functions.valuesNow.windReal > 1000)
					model.functions.valuesNow.windReal = model.functions.valuesNow.windReal;
			}
			if (model.functions.valuesNow.currentReal < 1000 || model.functions.valuesNow.currentReal_lastKnown < 1000) {
				if (model.functions.valuesNow.currentReal < 1000)
					model.functions.valuesNow.currentReal_lastKnown = model.functions.valuesNow.currentReal;
			}
			if (model.functions.valuesNow.waveDirReal < 1000 || model.functions.valuesNow.waveDirReal_lastKnown < 1000) {
				if (model.functions.valuesNow.waveDirReal < 1000)
					model.functions.valuesNow.waveDirReal_lastKnown = model.functions.valuesNow.waveDirReal;
			}
		}
		else {
			if (lev1 < 0 && lev2 < 0) {
				if (model.network.channel[-lev1 - 1].type == 1) { // tss
					if (model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid > model.functions.valuesNow.maxWaveHeight &&
						model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
						if (model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid > 20)
							arcNr = arcNr;
						model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid;
						model.functions.valuesNow.maxWaveHeight_tp = *timeExact - timeCheck;
						model.functions.valuesNow.maxWaveHeight_dir = model.functions.valuesNow.waveDirReal; // correct?
						if (model.functions.valuesNow.maxWaveHeight > 9)
							model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;

					}
					model.functions.valuesNow.totFuel_mainMovingNoCorridors += model.functions.valuesNow.arcDel_fuel_main;
					model.functions.valuesNow.totTime_movingNoCorridors += timeCheck;
					model.functions.valuesNow.sumSpeedOnWater += model.functions.valuesNow.speedOnWater * timeCheck;
					model.functions.valuesNow.totDistance_movingNoCorridors += model.arc[arcNr].distance * (usedKvot - startKvot);

					if (model.functions.valuesNow.windReal < 1000 || model.functions.valuesNow.windDirReal_lastKnown < 1000) {
						if (model.functions.valuesNow.windReal < 1000) {
							model.functions.valuesNow.windDirReal_lastKnown = model.functions.valuesNow.windDirReal;
							model.functions.valuesNow.windReal_lastKnown = model.functions.valuesNow.windReal;
						}
						if (model.functions.valuesNow.windReal > 1000)
							model.functions.valuesNow.windReal = model.functions.valuesNow.windReal;
					}
					if (model.functions.valuesNow.currentReal < 1000 || model.functions.valuesNow.currentReal_lastKnown < 1000) {
						if (model.functions.valuesNow.currentReal < 1000)
							model.functions.valuesNow.currentReal_lastKnown = model.functions.valuesNow.currentReal;
					}
					if (model.functions.valuesNow.waveDirReal < 1000 || model.functions.valuesNow.waveDirReal_lastKnown < 1000) {
						if (model.functions.valuesNow.waveDirReal < 1000)
							model.functions.valuesNow.waveDirReal_lastKnown = model.functions.valuesNow.waveDirReal;
					}
				}
				else {
					if (model.arc[arcNr].fuelQualityKvot > 0.9) {
						// not eca
						model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA += model.network.channel[-lev1 - 1].waiting_consumption_main * (usedKvot - startKvot);
						model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA += model.network.channel[-lev1 - 1].waiting_consumption_aux * (usedKvot - startKvot);
					}
					else {
						model.functions.valuesNow.totCorridorWaitingFuel_mainECA += model.network.channel[-lev1 - 1].waiting_consumption_main * (usedKvot - startKvot);
						model.functions.valuesNow.totCorridorWaitingFuel_auxECA += model.network.channel[-lev1 - 1].waiting_consumption_aux * (usedKvot - startKvot);
					}
					model.functions.valuesNow.totCorridorWaitingTime += model.network.channel[-lev1 - 1].waitingTime * (usedKvot - startKvot);
					model.functions.valuesNow.waitingTime += model.network.channel[-lev1 - 1].waitingTime * (usedKvot - startKvot);
				}
			}
		}

		model.functions.valuesNow.timeCheck += timeCheck;
		model.functions.valuesNow.timeSinceLast += model.arc[arcNr].time * (usedKvot - startKvot);
		model.functions.valuesNow.distSinceLast += model.arc[arcNr].distance * (usedKvot - startKvot);
		model.functions.valuesNow.distSinceLastCalmWater += model.arc[arcNr].time * (usedKvot - startKvot) * model.functions.valuesNow.calmWaterSpeed;

		if (*timeExact <= model.functions.timeNextWayPoint - 0.01 && nArcsLeft > 2) {
			continue;
		}

		if (filpekG != NULL) {

			model.functions.valuesNow.current /= model.functions.valuesNow.deltaTid;
			model.functions.valuesNow.windSpeed /= model.functions.valuesNow.deltaTid;
			model.functions.valuesNow.waveHeight /= model.functions.valuesNow.deltaTid;
			model.functions.valuesNow.wavePeriod /= model.functions.valuesNow.deltaTid;
			model.functions.valuesNow.speedDiffWind /= model.functions.valuesNow.deltaTid;
			model.functions.valuesNow.speedDiffWave /= model.functions.valuesNow.deltaTid;
			model.functions.valuesNow.baseGroundSpeed /= model.functions.valuesNow.deltaTid;

			if (model.nWeatherFiles > 8) {
				if (model.functions.valuesNow.timePressureSurface > 0)
					model.functions.valuesNow.pressureSurface /= model.functions.valuesNow.timePressureSurface;
				if (model.functions.valuesNow.timePressureAir > 0)
					model.functions.valuesNow.pressureAir /= model.functions.valuesNow.timePressureAir;
				if (model.functions.valuesNow.timePrecipitation > 0)
					model.functions.valuesNow.precipitation /= model.functions.valuesNow.timePrecipitation;
				if (model.functions.valuesNow.timeTempSea > 0)
					model.functions.valuesNow.tempSea /= model.functions.valuesNow.timeTempSea;
				if (model.functions.valuesNow.timeTempAir > 0)
					model.functions.valuesNow.tempAir /= model.functions.valuesNow.timeTempAir;
				if (model.functions.valuesNow.timeCloudCover > 0)
					model.functions.valuesNow.cloudCover /= model.functions.valuesNow.timeCloudCover;
				if (model.functions.valuesNow.timeMdps > 0)
					model.functions.valuesNow.mdps /= model.functions.valuesNow.timeMdps;
				if (model.functions.valuesNow.timeSwell > 0)
					model.functions.valuesNow.swell /= model.functions.valuesNow.timeSwell;
			}


			if (model.functions.valuesNow.timeArc_current > 0.001) {
				model.functions.valuesNow.currentReal_u /= model.functions.valuesNow.timeArc_current;
				model.functions.valuesNow.currentReal_v /= model.functions.valuesNow.timeArc_current;
				model.functions.valuesNow.currentReal = sqrt(model.functions.valuesNow.currentReal_u * model.functions.valuesNow.currentReal_u +
					model.functions.valuesNow.currentReal_v * model.functions.valuesNow.currentReal_v);
				model.functions.valuesNow.currentDirReal = ApproxAtan2(model.functions.valuesNow.currentReal_v,
					model.functions.valuesNow.currentReal_u) * 180.0 / M_PI;
			}
			else {
				model.functions.valuesNow.currentReal = 9999;
				model.functions.valuesNow.currentDirReal = 0;
			}

			if (model.functions.valuesNow.timeArc_wind > 0.001) {
				model.functions.valuesNow.windSpeedReal_u /= model.functions.valuesNow.timeArc_wind;
				model.functions.valuesNow.windSpeedReal_v /= model.functions.valuesNow.timeArc_wind;
				model.functions.valuesNow.windReal = sqrt(model.functions.valuesNow.windSpeedReal_u * model.functions.valuesNow.windSpeedReal_u +
					model.functions.valuesNow.windSpeedReal_v * model.functions.valuesNow.windSpeedReal_v);
				model.functions.valuesNow.windDirReal = ApproxAtan2(model.functions.valuesNow.windSpeedReal_v, model.functions.valuesNow.windSpeedReal_u) * 180.0 / M_PI;
			}
			else {
				model.functions.valuesNow.windReal = 9999;
				model.functions.valuesNow.windDirReal = 0;
			}

			if (model.functions.valuesNow.timeArc_wave > 0.001)
				model.functions.valuesNow.waveDirReal = ApproxAtan2(model.functions.valuesNow.waveHeightReal_v, model.functions.valuesNow.waveHeightReal_u) * 180.0 / M_PI;
			else
				model.functions.valuesNow.waveDirReal = 9999;

			if (printGlobal == 1)
				printf("wavePeriod %.3lf timeArc %.3lf\n", model.functions.valuesNow.wavePeriod, model.functions.valuesNow.deltaTid);

			model.functions.valuesNow.forecastType /= (model.functions.valuesNow.deltaTid * 8);

			model.functions.valuesNow.relWindDir = ApproxAtan2(model.functions.valuesNow.windSpeed_y, model.functions.valuesNow.windSpeed_x) * 180.0 / M_PI;
			if (model.functions.valuesNow.relWindDir < 0)
				model.functions.valuesNow.relWindDir = -model.functions.valuesNow.relWindDir;

			model.functions.valuesNow.relWaveDir = ApproxAtan2(model.functions.valuesNow.waveDir_y, model.functions.valuesNow.waveDir_x) * 180.0 / M_PI;
			if (model.functions.valuesNow.relWaveDir < 0)
				model.functions.valuesNow.relWaveDir = -model.functions.valuesNow.relWaveDir;


			if (*posReport > 0)
				fprintf(filpekG, ",\n");
			(*posReport)++;
			fprintf(filpekG, "  {\"type\":\"Feature\", \"properties\":{\"dateUTC\":\"%s\",\n",
				model.params.startTime_short);
			fprintf(filpekG, "    \"full_date\":\"%s\",\n",
				model.params.startTime_full);

			set_tmBasTime(*timeExact);
			//fixReportDateNew();
			//fixReportDateNew_full();
			// tmBas.tm_min += round(timeCheck * 60);
			fixReportDate(model.params.tmBas, model.params.startTime_short);
			fixReportDate_full(model.params.tmBas, model.params.startTime_full);


			fprintf(filpekG, "\"solutionID\":\"%s\",\n",
				solName.c_str());
			if (arcNr == 4859)
				arcNr = arcNr;
			if (model.arc[arcNr].fromLevel == -3)
				arcNr = arcNr;
			fprintf(filpekG, "    \"arcNr\":%d,\n", arcNr);
			fprintf(filpekG, "    \"arc obj cost\":%.2lf,\n", model.arc[arcNr].totCost);
			fprintf(filpekG, "    \"level from\":%d,\n", model.arc[arcNr].fromLevel);
			fprintf(filpekG, "    \"posNodefrom\":%d,\n", model.arc[arcNr].fromPointNr);

			fprintf(filpekG, "    \"arcStart\":%d,\n", model.arc[arcNr].fromTime);
			fprintf(filpekG, "    \"accumTimeStart_h\":%.6lf,\n", timeOld);
			if (model.arc[arcNr].fromLevel >= 0 && ii == 0) {
				diffTime = timeOld - model.network.physicalLev[model.arc[arcNr].fromLevel].midTimeArrive;
				fprintf(filpekG, "    \"arrive-midTimeArriveDiff_h\":%.2lf,\n", diffTime);
				if (diffTime > model.functions.valuesNow.maxDiffTime) {
					model.functions.valuesNow.maxDiffTime = diffTime;
					model.functions.valuesNow.maxDiffTime_level = model.arc[arcNr].fromLevel;
				}
				if (diffTime < model.functions.valuesNow.minDiffTime) {
					model.functions.valuesNow.minDiffTime = diffTime;
					model.functions.valuesNow.minDiffTime_level = model.arc[arcNr].fromLevel;
				}
			}
			else {
				fprintf(filpekG, "    \"arrive-midTimeArriveDiff_h_end\":-99999, \n");
				diffTime = 0;
			}
			fprintf(filpekG, "    \"level to\":%d,\n", model.arc[arcNr].toLevel);
			fprintf(filpekG, "    \"posNodeTo\":%d,\n", model.arc[arcNr].toPointNr);
			if (ii == nSplit - 1)
				fprintf(filpekG, "    \"toTime\":%d,\n", model.arc[arcNr].toTime);
			else
				fprintf(filpekG, "    \"toTime\":-1,\n");

			if (startSlutArc == 0) {
				fprintf(filpekG, "    \"hours\":%.2lf, \"distance_nm\":%.1lf,\n",
					model.functions.valuesNow.timeSinceLast, model.functions.valuesNow.distSinceLast / model.params.knots_to_km);

				fprintf(filpekG, "    \"WindF\":%.3lf, \"WaveF\":%.3lf, \"CurrentF\":%.3lf, \"DelayF\":%.3lf, \"allWeatherFactors\": %.3lf,\n",
					model.functions.valuesNow.WindF / model.params.knots_to_km,
					model.functions.valuesNow.WaveF / model.params.knots_to_km,
					model.functions.valuesNow.CurrentF / model.params.knots_to_km,
					model.functions.valuesNow.DelayF / model.params.knots_to_km,
					(model.functions.valuesNow.WindF + model.functions.valuesNow.WaveF +
						model.functions.valuesNow.CurrentF + model.functions.valuesNow.DelayF) / model.params.knots_to_km);

				fprintf(filpekG, "    \"accumDistance_km\":%.2lf, \"distanceLeft_nm\":%.1lf,\n", model.functions.valuesNow.accumDistance,
					(model.functions.valuesNow.totDistance - model.functions.valuesNow.accumDistance) / model.params.knots_to_km);
				model.functions.valuesNow.accumDistance += model.functions.valuesNow.distSinceLast;
				if (model.params.wayPointHours > 0) {
					fixPositionString_latLon(model.functions.valuesNow.last_y, model.functions.valuesNow.last_x, model.params.startTime);
					bearing = calcBearingFromToCoords(model.functions.valuesNow.last_y, model.functions.valuesNow.last_x,
						p2.latitude().degrees(), p2.longitude().degrees());
				}
				else {
					if (ii < nSplit - 1) {
						bearing = calcBearingFromToCoords(model.network.yCoord[model.network.posSplitCoord[ii]],
							model.network.xCoord[model.network.posSplitCoord[ii]],
							model.network.yCoord[model.network.posSplitCoord[ii + 1]],
							model.network.xCoord[model.network.posSplitCoord[ii + 1]]);
						nChangeBearingBetween = model.network.posSplitCoord[ii + 1] - model.network.posSplitCoord[ii] - 1;
						if (nChangeBearingBetween == 0)
							nChange_lessXdegrees = 0;
						else
							nChange_lessXdegrees = identify_nChanges_lessXdegrees(model.network.posSplitCoord[ii], model.network.posSplitCoord[ii + 1]);
					}
					else {
						if (ii > 0) {
							bearing = calcBearingFromToCoords(model.network.yCoord[model.network.posSplitCoord[ii]],
								model.network.xCoord[model.network.posSplitCoord[ii]],
								p2.latitude().degrees(), p2.longitude().degrees());
							nChangeBearingBetween = model.network.nCoords - model.network.posSplitCoord[ii] - 1;
							if (nChangeBearingBetween == 0)
								nChange_lessXdegrees = 0;
							else
								nChange_lessXdegrees = identify_nChanges_lessXdegrees(model.network.posSplitCoord[ii], model.network.nCoords - 1);
						}
						else {
							bearing = p1.bearingTo(p2);
							nChangeBearingBetween = model.network.nCoords - model.network.posSplitCoord[ii] - 1;
							nChange_lessXdegrees = -1; // identify_nChanges_lessXdegrees(model.network.posSplitCoord[ii], model.network.nCoords - 1);
						}
					}

					fixPositionString_latLon(model.network.yCoord[model.network.posSplitCoord[ii]],
						model.network.xCoord[model.network.posSplitCoord[ii]], model.params.startTime);

					diff = getDiff_anglesDegrees(bearing, model.functions.valuesNow.bearingOldWpt);
					//printf("bearing %.1lf oldBearing %.1lf diff %.1lf limit %.2lf\n", bearing, model.functions.valuesNow.bearingOldWpt,
					//	diff, model.params.report_minBearingDiffWpt);
					//else {
					//	fprintf(filpekG, "    \"Wpt\":,\n");
					//}

					fprintf(filpekG, "    \"nChangeBearingInbetweenNodes\":%d,\n", nChangeBearingBetween);
					fprintf(filpekG, "    \"nChangeBearing_lessThanXdegrees\":%d,\n", nChange_lessXdegrees);
				}
				//if (diff >= model.params.report_minBearingDiffWpt || lev1 < 0) {
				(model.functions.valuesNow.Wpt)++;
				fprintf(filpekG, "    \"Wpt\":%d,\n", model.functions.valuesNow.Wpt);
				//model.functions.valuesNow.bearingOldWpt = bearing;
				//}
				fprintf(filpekG, "    \"Position_lat_lon\":\"%s\", \"bearing\":%.0lf,\n", model.params.startTime, bearing);
				//fprintf(filpekG, "    \"Position_lat_lon\":\"%.3lf, %.3lf\", \"bearing\":%.0lf,\n",
				//	model.network.yCoord[model.network.posSplitCoord[ii]],
				//	model.network.xCoord[model.network.posSplitCoord[ii]], bearing);


				if (lev1 < 0 && lev2 < 0) {

					fuel_day = (model.functions.valuesNow.fuel_main) * 24 / (model.functions.valuesNow.timeCheck - model.functions.valuesNow.waitingTime);
					fuel_dayAux = (model.functions.valuesNow.fuel_aux) * 24 / (model.functions.valuesNow.timeCheck - model.functions.valuesNow.waitingTime);
				}
				else {
					if (model.functions.valuesNow.timeCheck > 0.001) {
						//	fuel_day = (model.arc[arcNr].fuel_aux * (endKvot - startKvot) + model.arc[arcNr].fuel_auxEca * (endKvot - startKvot)
						//		+ model.arc[arcNr].fuel_eca * (endKvot - startKvot) + model.arc[arcNr].fuel_noEca * (endKvot - startKvot)) * 24 / timeCheck;
						//fuel_day = (model.arc[arcNr].fuel_eca * (endKvot - startKvot) + model.arc[arcNr].fuel_noEca * (endKvot - startKvot)) * 24 / timeCheck;
						fuel_day = (model.functions.valuesNow.fuel_main) * 24 / model.functions.valuesNow.timeCheck;
						fuel_dayAux = (model.functions.valuesNow.fuel_aux) * 24 / model.functions.valuesNow.timeCheck;
						//fuel_dayCheck = (model.functions.valuesNow.fuel_main * (endKvot - startKvot)) * 24 / timeCheck;
					}
					else {
						fuel_day = 0;
						fuel_dayAux = 0;
						//fuel_dayCheck = 0;
					}
				}
				fprintf(filpekG, "    \"fuelConsumptionMain_ton_day\":%.5lf, \"fuelConsumptionAux_ton_day\":%.5lf, \"fuelECA_ton\":%.3lf\n",
					fuel_day, fuel_dayAux, model.functions.valuesNow.fuel_eca);
				// (1 - model.arc[arcNr].fuelQualityKvot) * (model.functions.valuesNow.fuel_main + model.functions.valuesNow.fuel_aux));

			//fprintf(filpekG, "    ,\"fuelConsumptionMain_ton_dayCheck\":%.1lf, \"fuelECA_tonCheck\":%.3lf\n",
			//	fuel_dayCheck,
			//	(model.functions.valuesNow.fuel_aux + model.functions.valuesNow.fuel_main) * (endKvot - startKvot) * (1 - fuelQualityKvot));

				if (filpek10 != NULL) {
					if (ii > 0)
						fprintf(filpek10, "\n\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");

					fprintf(filpek10, "%.3lf\t%.3lf\t%.3lf\t%.3lf\t%s\t%.3lf\t%.3lf\t%.3lf",
						timeCheck, *timeExact, diffTime, model.functions.valuesNow.accumDistance,
						model.params.startTime, bearing, fuel_day, model.functions.valuesNow.worstStormValue);
				}


				if (model.functions.valuesNow.timeSinceLast < 0.0001)
					model.functions.valuesNow.timeSinceLast = 0.0001;
				if ((lev1 >= 0 || lev2 >= 0) && model.arc[arcNr].speedSetting >= 0) {
					fprintf(filpekG, ", \"current_kts\":%.1lf\n",
						model.functions.valuesNow.current / model.params.knots_to_km);
					fprintf(filpekG, ", \"speedSetting\":%d, \"speedCalmWater_kts\":%.2lf, \"speedOnGround_kts\":%.2lf, \"rpm\":%.2lf,\n",
						model.arc[arcNr].speedSetting,
						model.functions.valuesNow.distSinceLastCalmWater / model.functions.valuesNow.timeSinceLast / model.params.knots_to_km,
						model.functions.valuesNow.distSinceLast / model.functions.valuesNow.timeSinceLast / model.params.knots_to_km,
						speedSetting.rpm[model.arc[arcNr].speedSetting]);
					model.functions.valuesNow.accumRPM += speedSetting.rpm[model.arc[arcNr].speedSetting] * timeCheck;
					model.functions.valuesNow.accumRPM_time += timeCheck;
					fprintf(filpekG, "    \"windSpeed_km_h\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir);
					fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
						model.functions.valuesNow.relWaveDir);
					if (model.functions.valuesNow.waveHeight < model.functions.maxWaveHeight_warning)
						fprintf(filpekG, "    \"waveHeight_level\":\"normal\",\n");
					else
						fprintf(filpekG, "    \"waveHeight_level\":\"high\",\n");
					if (model.functions.valuesNow.windSpeed < model.functions.maxWindSpeed_warning)
						fprintf(filpekG, "    \"windSpeed_level\":\"normal\",\n");
					else
						fprintf(filpekG, "    \"windSpeed_level\":\"high\",\n");

					if (model.functions.valuesNow.windReal < 100000 || model.functions.valuesNow.windDirReal_lastKnown < 100000) {
						fixDirectionLetters(model.functions.valuesNow.windDirReal_lastKnown, model.params.startTime, 1);
						fprintf(filpekG, "    \"windSpeedReal_knots\":%.1lf, \"windDirection_degrees\":%.0lf, \"windDir_letters\":\"%s\",\n",
							model.functions.valuesNow.windReal_lastKnown / model.params.knots_to_km, model.functions.valuesNow.windDirReal, model.params.startTime);
					}
					if (model.functions.valuesNow.currentReal < 100000 || model.functions.valuesNow.currentReal_lastKnown < 100000) {
						fprintf(filpekG, "    \"currentReal_knots\":%.3lf, \"currentDirection_degrees\":%.0lf,\n",
							model.functions.valuesNow.currentReal_lastKnown / model.params.knots_to_km, model.functions.valuesNow.currentDirReal);
					}
					if (model.functions.valuesNow.waveDirReal < 100000 || model.functions.valuesNow.waveDirReal_lastKnown < 100000) {
						fixDirectionLetters(model.functions.valuesNow.waveDirReal_lastKnown, model.params.startTime, 1);
						fprintf(filpekG, "    \"waveDirection_degrees\":%.0lf,\"waveDir_letters\":\"%s\",\n",
							model.functions.valuesNow.waveDirReal_lastKnown, model.params.startTime);
					}
					if (printGlobal == 1)
						printf("arcNr %d levels %d %d wavePeriod %.3lf dist %.3lf time %.3lf speedOnGround %.3lf\n",
							arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.functions.valuesNow.wavePeriod,
							model.arc[arcNr].distance * (usedKvot - startKvot), model.arc[arcNr].time * (usedKvot - startKvot), speedOnGround);

					if (model.nWeatherFiles > 10) {
						addExtraWeatherInfoToGeojson(filpekG);
					}

#ifdef NAZANIN_SAFETY
					fprintf(filpekG, "    \"bow slamming p\":%.3lf, \"green water p\":%.3lf,\n    \"dynamic instability\":%.3lf,\n",
						model.functions.valuesNow.bowSlam, model.functions.valuesNow.greenWater,
						model.functions.valuesNow.dynamicStability); 
					fprintf(filpekG, "    \"rolling p\":%.3lf, \"surfRiding p\":%.3lf,\n",
						model.functions.valuesNow.rolling, model.functions.valuesNow.surfRiding);
#endif
					fprintf(filpekG, "    \"max iceCover\":%.3lf",
						model.functions.valuesNow.iceCover_max);
					if (model.functions.valuesNow.forecastType > 0.5) {
						if (model.params.hindCast == 0)
							fprintf(filpekG, ", \"forecastType\":\"Ext. Hist\"");
						else
							fprintf(filpekG, ", \"forecastType\":\"Past Hist %.5lf\"", model.functions.valuesNow.forecastType);
					}
					else {
						if (model.params.hindCast == 0)
							fprintf(filpekG, ", \"forecastType\":\"Fcst\"");
						else {
							if (model.functions.valuesNow.forecastType > 0.05)
								fprintf(filpekG, ", \"forecastType\":\"Missing Hist %.5lf\"", model.functions.valuesNow.forecastType);
							else
								fprintf(filpekG, ", \"forecastType\":\"Hist %.5lf\"", model.functions.valuesNow.forecastType);
						}
					}

					if (filpek10 != NULL) {
						fprintf(filpek10, "\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t"
							"%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf",
							model.functions.valuesNow.currentReal, model.functions.valuesNow.current,
							model.functions.valuesNow.calmWaterSpeed,
							model.functions.valuesNow.baseGroundSpeed, speedOnGround,
							speedSetting.rpm[model.arc[arcNr].speedSetting],
							model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir,
							model.functions.valuesNow.speedDiffWind,
							model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
							model.functions.valuesNow.relWaveDir, model.functions.valuesNow.speedDiffWave,
							model.functions.valuesNow.windReal,
							model.functions.valuesNow.windDirReal, model.functions.valuesNow.currentReal,
							model.functions.valuesNow.currentDirReal, model.functions.valuesNow.waveDirReal,
							model.functions.valuesNow.bowSlamming_max,
							model.functions.valuesNow.greenWater_max,
							model.functions.valuesNow.dynamicStability_max,
							model.functions.valuesNow.iceCover_max,
							model.functions.valuesNow.forecastType);
					}

				}
				else {
					//if (lev1 < 0 && lev2 < 0) {
						//if (model.network.channel[-lev1 - 1].type == 1) { // tss
					fprintf(filpekG, ", \"current_kts\":%.1lf\n",
						model.functions.valuesNow.current / model.params.knots_to_km);
					fprintf(filpekG, ", \"speedSetting\":%d, \"speedCalmWater_kts\":%.2lf, \"speedOnGround_kts\":%.2lf, \"rpm\":%.2lf,\n",
						model.arc[arcNr].speedSetting,
						model.functions.valuesNow.distSinceLastCalmWater / model.functions.valuesNow.timeSinceLast / model.params.knots_to_km,
						model.functions.valuesNow.distSinceLast / model.functions.valuesNow.timeSinceLast / model.params.knots_to_km,
						speedSetting.rpm[model.arc[arcNr].speedSetting]);
					model.functions.valuesNow.accumRPM += speedSetting.rpm[model.arc[arcNr].speedSetting] * timeCheck;
					model.functions.valuesNow.accumRPM_time += timeCheck;
					fprintf(filpekG, "    \"windSpeed_km_h\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir);
					fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
						model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
						model.functions.valuesNow.relWaveDir);
					if (model.functions.valuesNow.waveHeight < model.functions.maxWaveHeight_warning)
						fprintf(filpekG, "    \"waveHeight_level\":\"normal\",\n");
					else
						fprintf(filpekG, "    \"waveHeight_level\":\"high\",\n");
					if (model.functions.valuesNow.windSpeed < model.functions.maxWindSpeed_warning)
						fprintf(filpekG, "    \"windSpeed_level\":\"normal\",\n");
					else
						fprintf(filpekG, "    \"windSpeed_level\":\"high\",\n");

					if (model.functions.valuesNow.windReal < 100000 || model.functions.valuesNow.windDirReal_lastKnown < 100000) {
						fixDirectionLetters(model.functions.valuesNow.windDirReal_lastKnown, model.params.startTime, 1);
						fprintf(filpekG, "    \"windSpeedReal_knots\":%.1lf, \"windDirection_degrees\":%.0lf, \"windDir_letters\":\"%s\",\n",
							model.functions.valuesNow.windReal_lastKnown / model.params.knots_to_km, model.functions.valuesNow.windDirReal, model.params.startTime);
					}
					if (model.functions.valuesNow.currentReal < 100000 || model.functions.valuesNow.currentReal_lastKnown < 100000) {
						fprintf(filpekG, "    \"currentReal_knots\":%.3lf, \"currentDirection_degrees\":%.0lf,\n",
							model.functions.valuesNow.currentReal_lastKnown / model.params.knots_to_km, model.functions.valuesNow.currentDirReal);
					}
					if (model.functions.valuesNow.waveDirReal < 100000 || model.functions.valuesNow.waveDirReal_lastKnown < 100000) {
						fixDirectionLetters(model.functions.valuesNow.waveDirReal_lastKnown, model.params.startTime, 1);
						fprintf(filpekG, "    \"waveDirection_degrees\":%.0lf,\"waveDir_letters\":\"%s\",\n",
							model.functions.valuesNow.waveDirReal_lastKnown, model.params.startTime);
					}
					if (printGlobal == 1)
						printf("arcNr %d levels %d %d wavePeriod %.3lf dist %.3lf time %.3lf speedOnGround %.3lf\n",
							arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.functions.valuesNow.wavePeriod,
							model.arc[arcNr].distance * (usedKvot - startKvot), model.arc[arcNr].time * (usedKvot - startKvot), speedOnGround);

					if (model.nWeatherFiles > 10) {
						addExtraWeatherInfoToGeojson(filpekG);
					}

#ifdef NAZANIN_SAFETY
					fprintf(filpekG, "    \"bow slamming p\":%.3lf, \"green water p\":%.3lf,\n    \"dynamic instability\":%.3lf,\n",
						model.functions.valuesNow.bowSlam, model.functions.valuesNow.greenWater,
						model.functions.valuesNow.dynamicStability);
					fprintf(filpekG, "    \"rolling p\":%.3lf, \"surfRiding p\":%.3lf,\n",
						model.functions.valuesNow.rolling, model.functions.valuesNow.surfRiding);
#endif
					fprintf(filpekG, "    \"max iceCover\":%.3lf",
						model.functions.valuesNow.iceCover_max);
					if (model.functions.valuesNow.forecastType > 0.5) {
						if (model.params.hindCast == 0)
							fprintf(filpekG, ", \"forecastType\":\"Ext. Hist\"");
						else
							fprintf(filpekG, ", \"forecastType\":\"Past Hist %.5lf\"", model.functions.valuesNow.forecastType);
					}
					else {
						if (model.params.hindCast == 0)
							fprintf(filpekG, ", \"forecastType\":\"Fcst\"");
						else {
							if (model.functions.valuesNow.forecastType > 0.05)
								fprintf(filpekG, ", \"forecastType\":\"Missing Hist %.5lf\"", model.functions.valuesNow.forecastType);
							else
								fprintf(filpekG, ", \"forecastType\":\"Hist %.5lf\"", model.functions.valuesNow.forecastType);
						}
					}

					//}
					//else {
					//	fprintf(filpekG, ", \"corridorID\":\"%s\"",
					//		model.network.channel[-lev1 - 1].ID);
					//	fprintf(filpekG, ",\n\"fuelTransit_main\":%.3lf", model.functions.valuesNow.fuel_main - model.network.channel[-lev1 - 1].waiting_consumption_main);
					//	fprintf(filpekG, ", \"fuelTransit_aux\":%.3lf", model.functions.valuesNow.fuel_aux - model.network.channel[-lev1 - 1].waiting_consumption_aux);
					//	fprintf(filpekG, ",\n\"fuelWaiting_main\":%.3lf", model.network.channel[-lev1 - 1].waiting_consumption_main);
					//	fprintf(filpekG, ", \"fuelWaiting_aux\":%.3lf", model.network.channel[-lev1 - 1].waiting_consumption_aux);

					//	if (model.arc[arcNr].fuelQualityKvot > 0.9) {
					//		// not eca
					//		model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA += model.network.channel[-lev1 - 1].waiting_consumption_main;
					//		model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA += model.network.channel[-lev1 - 1].waiting_consumption_aux;
					//		fprintf(filpekG, ",\n\"emissionWaiting_main\":%.3lf", model.network.channel[-lev1 - 1].waiting_consumption_main * model.params.fuel.main_noEca.emissionFactor);
					//		fprintf(filpekG, ", \"emissionWaiting_aux\":%.3lf", model.network.channel[-lev1 - 1].waiting_consumption_aux * model.params.fuel.aux_noEca.emissionFactor);
					//	}
					//	else {
					//		model.functions.valuesNow.totCorridorWaitingFuel_mainECA += model.network.channel[-lev1 - 1].waiting_consumption_main;
					//		model.functions.valuesNow.totCorridorWaitingFuel_auxECA += model.network.channel[-lev1 - 1].waiting_consumption_aux;
					//		fprintf(filpekG, ",\n\"emissionWaiting_main\":%.3lf", model.network.channel[-lev1 - 1].waiting_consumption_main * model.params.fuel.main_eca.emissionFactor);
					//		fprintf(filpekG, ", \"emissionWaiting_aux\":%.3lf", model.network.channel[-lev1 - 1].waiting_consumption_aux * model.params.fuel.aux_eca.emissionFactor);
					//	}
					//	model.functions.valuesNow.totCorridorWaitingTime += model.network.channel[-lev1 - 1].waitingTime;

					//	fprintf(filpekG, ",\n\"transitTime\":%.3lf", timeCheck - model.network.channel[-lev1 - 1].waitingTime);
					//	fprintf(filpekG, ", \"waitingTime\":%.3lf", model.network.channel[-lev1 - 1].waitingTime);
					//}
				//}
				//else
				//	fprintf(filpekG, ", \"corridorID\":\"extension of corridor since not physically feasible area around the corridor\"");
				}
				fprintf(filpekG, ", \"worstStormValue\":%.3lf",
					model.functions.valuesNow.worstStormValue);
			}
			else {
				fprintf(filpekG, "    \"hours\":%.2lf, \"checkHours\":%.3lf, \"distance_nm\":%.1lf,\n", 0.0, 0.0, 0.0);
				fprintf(filpekG, "    \"accumDistance_km\":%.2lf, \"distanceLeft_nm\":%.1lf,\n", model.functions.valuesNow.accumDistance, 0.0);
				x = p2.longitude().degrees();
				y = p2.latitude().degrees();
				fprintf(filpekG, "    \"Position_lat_lon\":\"%.3lf, %.3lf\", \"bearing\":%.0lf,\n",
					y, x, bearing);
				fprintf(filpekG, "    \"nChangeBearingInbetweenNodes\":%d,\n", 0);
				fprintf(filpekG, "    \"nChangeBearing_lessThanXdegrees\":%d,\n", 0);
				fprintf(filpekG, "    \"speedSetting\":%d, \"speedCalmWater_kts\":%.2lf, \"speedOnGround_kts\":%.2lf, \"rpm\":%.2lf,\n",
					-1, 0, 0, 0);
				fprintf(filpekG, "    \"fuelConsumptionMain_ton_day\":%.5lf, \"fuelConsumptionAux_ton_day\":%.5lf, \"fuelECA_ton\":%.3lf, \"current_kts\":%.1lf,\n",
					0, 0, 0, model.functions.valuesNow.current / model.params.knots_to_km);
				fprintf(filpekG, "    \"windSpeed_km_h\":%.3lf, \"relativeWindDirection_degrees\":%.0lf,\n",
					model.functions.valuesNow.windSpeed, model.functions.valuesNow.relWindDir);
				fprintf(filpekG, "    \"waveHeight_m\":%.3lf, \"wavePeriod_s\":%.3lf,\n    \"relativeWaveDirection_degrees\":%.0lf,\n",
					model.functions.valuesNow.waveHeight, model.functions.valuesNow.wavePeriod,
					model.functions.valuesNow.relWaveDir);
				if (model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid > model.functions.valuesNow.maxWaveHeight &&
					model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
					if (model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid > 20)
						arcNr = arcNr;
					model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.waveHeight / model.functions.valuesNow.deltaTid;
					model.functions.valuesNow.maxWaveHeight_tp = *timeExact - timeCheck;
					if (model.functions.valuesNow.maxWaveHeight > 9)
						model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
				}

				fixDirectionLetters(model.functions.valuesNow.windDirReal, model.params.startTime, 1);
				fprintf(filpekG, "    \"windSpeedReal_knots\":%.1lf, \"windDirection_degrees\":%.0lf, \"windDir_letters\":\"%s\",\n",
					model.functions.valuesNow.windReal / model.params.knots_to_km, model.functions.valuesNow.windDirReal, model.params.startTime);
				fprintf(filpekG, "    \"currentReal_knots\":%.3lf, \"currentDirection_degrees\":%.0lf,\n",
					model.functions.valuesNow.currentReal / model.params.knots_to_km, model.functions.valuesNow.currentDirReal);
				fixDirectionLetters(model.functions.valuesNow.waveDirReal, model.params.startTime, 1);
				fprintf(filpekG, "    \"waveDirection_degrees\":%.0lf,\"waveDir_letters\":\"%s\",\n",
					model.functions.valuesNow.waveDirReal, model.params.startTime);

				if (model.nWeatherFiles > 10) {
					addExtraWeatherInfoToGeojson(filpekG);
				}

#ifdef NAZANIN_SAFETY
				fprintf(filpekG, "    \"max bow slamming p\":%.3lf, \"max green water p\":%.3lf,\n    \"max dynamic instability\":%.3lf,\n",
					0.0, 0.0, 0.0);
				fprintf(filpekG, "    \"max rolling p\":%.3lf, \"max surfRiding p\":%.3lf,\n",
					0.0, 0.0);
#endif
				fprintf(filpekG, "    \"max iceCover\":%.3lf,  \"worstStormValue\":%.3lf, \"forecastType\":\"\"",
					0.0, 0.0);
			}
			fprintf(filpekG, "},\n");

			if (model.params.wayPointHours > 0) {
				// eval_xy_from_arcKvot(arcNr, p1, p2, prefPath, usedKvot, &y, &x);
				fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n",
					getCorrect_longitude(model.functions.valuesNow.last_x), model.functions.valuesNow.last_y);
				model.functions.timeNextWayPoint += model.params.wayPointHours;
				eval_xy_from_arcKvot(arcNr, p1, p2, prefPath, usedKvot);

				if (lev2 < model.network.nPhysicalLevels) {
					addCoordsToPath_kvots(arcNr, p1, p2, prefPath, usedKvot);
				}
				//else { // last arc or a channel
				//	model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
				//	model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
				//	(model.network.nCoords)++;
				//}

			}
			else
				fprintf(filpekG, "    \"geometry\":{\"type\": \"Point\", \"coordinates\":[%.4lf,%.4lf]}}\n",
					getCorrect_longitude(model.network.xCoord[model.network.posSplitCoord[ii]]),
					model.network.yCoord[model.network.posSplitCoord[ii]]);

			model.functions.valuesNow.totWindF += model.functions.valuesNow.WindF;
			model.functions.valuesNow.totWaveF += model.functions.valuesNow.WaveF;
			model.functions.valuesNow.totCurrentF += model.functions.valuesNow.CurrentF;
			model.functions.valuesNow.totDelayF += model.functions.valuesNow.DelayF;

			model.functions.valuesNow.totFavorableWind_h += model.functions.valuesNow.favorableWind_h;
			model.functions.valuesNow.totFavorableWave_h += model.functions.valuesNow.favorableWave_h;
			model.functions.valuesNow.totFavorableWindWave_h += model.functions.valuesNow.favorableWindWave_h;

			resetValuesNow();

		}

		if (usedKvot < 0.999)
			ii--;
	}

	if (filpek10 != NULL) {
		free(namn);
		fprintf(filpek10, "\n");
		fclose(filpek10);
	}

	if (lev2 < model.network.nPhysicalLevels) {
		addCoordsToPath_kvots(arcNr, p1, p2, prefPath, 1.0);
	}
	else { // last arc or a channel
		model.network.yCoord[model.network.nCoords] = p1.latitude().degrees();
		model.network.xCoord[model.network.nCoords] = p1.longitude().degrees();
		(model.network.nCoords)++;
	}


	if (model.params.simuleraTidVisuellt == 1) {
		plotPathTimeVisuellt(arcNr, timeOld, *timeExact);
	}

	if (filpekG != NULL) {
		if (abs(model.arc[arcNr].time - accumTime) > 1.0) {
			(model.delay.nDiffTimeSol)++;
			errlog("ERROR! Diff between arcTime and time computed in addPositionDataToReport for arc %d, %.3lf vs %.3lf, might be okay since the check points are different?\n"
				"especially if it is a corridor with a certain starting time from midnight\n",
				arcNr, accumTime, model.arc[arcNr].time);
			printf("\n\n\n\n\n########################################################\n");
			printf("ERROR! Diff between arcTime and time computed in addPositionDataToReport for arc %d, %.3lf vs %.3lf levels %d %d, might be okay since the check points are different?\n"
				"especially if it is a corridor with a certain starting time from midnight\n",
				arcNr, accumTime, model.arc[arcNr].time, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
			//(*timeExact) += model.arc[arcNr].time - accumTime;
		}
	}
	if (SKRIV_UT_NOTHING == 0) {
		if (arcNr == 29303)
			arcNr = arcNr;
		errlog("pos %d arcNr %d arcStart %lf arcEnd %lf timeExact %lf arcTimeEst %lf exactTime %lf arcCostEst %lf exactCost %lf\n", *posReport, arcNr,
			model.arc[arcNr].fromTime / (double)model.params.nTidsperioder_perH, model.arc[arcNr].toTime / (double)model.params.nTidsperioder_perH, *timeExact,
			model.arc[arcNr].time, accumTime, model.arc[arcNr].totCost / 100.0,
			model.functions.valuesNow.totFuel_main * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.main_eca.price +
			model.functions.valuesNow.totFuel_main * model.arc[arcNr].fuelQualityKvot * model.params.fuel.main_noEca.price +
			model.functions.valuesNow.totFuel_aux * model.arc[arcNr].fuelQualityKvot * model.params.fuel.aux_noEca.price +
			model.functions.valuesNow.totFuel_aux * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.aux_eca.price +
			accumTime * model.params.priceTime);

	}



	return 0;
}

int makeSure_eta_inTime() {
	int iPos, arcNr, posIreport = 0, speedNr, fromLevel, baseSpeedNr;
	int nAlloc, newSpeedNr, iPosUse;
	double timeExact = 0, timeOld, deltaError, speedNew, speedOld;
	double* timeArc = (double*)malloc((model.nBVArcs - 2) * sizeof(double));

	model.functions.valuesNow.maxWaveHeight = 0;
	model.functions.valuesNow.maxWaveHeight_tp = 0;
	model.functions.valuesNow.maxWaveHeight_dir = 0;

	for (int i = 0; i < 2; i++) {
		for (int i1 = 0; i1 < 3; i1++) {
			model.functions.valuesNow.favorableWind[i][i1] = 0;
			model.functions.valuesNow.favorableWave[i][i1] = 0;
			model.functions.valuesNow.favorableWindWave[i][i1] = 0;
		}
	}

	model.network.nMaxSplits = 1;
	for (iPos = 0; iPos < model.nBVArcs - 2; iPos++) {
		arcNr = model.BVArc[iPos];
		timeOld = timeExact;
		if (model.arc[arcNr].speedSetting != -2)
			addPositionDataToReport(NULL, &posIreport, arcNr, 0, &timeExact, "-");
		else
			timeExact += model.arc[arcNr].time;
		timeArc[iPos] = timeExact - timeOld;
	}
	deltaError = 0;
	if (timeExact > model.params.eta_h) {
		errlog("OBS! eta %.2lf after exact calc of arcTimes but should be latest %.2lf. "
			"I try to make it earlier by increasing speed one level wherever possible, starting from the end.\n",
			timeExact, model.params.eta_h);
		deltaError = timeExact - model.params.eta_h;
		for (iPos = model.nBVArcs - 3; iPos >= -2; iPos--) {
			if (iPos < 2)
				iPos = iPos;
			if (iPos < 0) {
				if (iPos == -1)
					iPosUse = 0;
				else
					iPosUse = model.nBVArcs - 3;
			}
			else
				iPosUse = iPos;
			arcNr = model.BVArc[iPosUse];
			if (model.arc[arcNr].speedSetting == -2)
				continue;
			fromLevel = model.arc[arcNr].fromLevel;
			if (fromLevel >= 0) {
				speedNr = model.arc[arcNr].speedSetting;
				baseSpeedNr = model.functions.speedLevel[fromLevel].settingGerBaseSetting[speedNr];
				if (baseSpeedNr < model.functions.nShip_speedSettingsBase - 1) {
					speedOld = model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed[speedNr];
					if (iPos < 0) {
						speedNew = model.functions.rpmSetting_gerCalmWaterSpeedBase[model.functions.nShip_speedSettingsBase - 1];
						newSpeedNr = model.functions.speedLevel[fromLevel].nShip_speedSettings - 1;
						if (model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed[newSpeedNr] < speedNew - 0.0001)
							newSpeedNr++;
					}
					else {
						speedNew = model.functions.rpmSetting_gerCalmWaterSpeedBase[baseSpeedNr + 1];
						newSpeedNr = speedNr + 1;
					}
					deltaError -= timeArc[iPosUse] * (1 - speedOld / speedNew);
					if (newSpeedNr >= model.functions.speedLevel[fromLevel].nShip_speedSettings) {
						if (newSpeedNr >= model.functions.nAllocShipSpeedsLevel) {
							nAlloc = newSpeedNr + 1;
							model.functions.speedLevel[fromLevel].rpm = (double*)realloc(
								model.functions.speedLevel[fromLevel].rpm, nAlloc * sizeof(double));
							model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed = (double*)realloc(
								model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed, nAlloc * sizeof(double));
							model.functions.speedLevel[fromLevel].rpmSetting_gerFuelConsumption_main = (double*)realloc(
								model.functions.speedLevel[fromLevel].rpmSetting_gerFuelConsumption_main, nAlloc * sizeof(double));
							model.functions.speedLevel[fromLevel].rpmSetting_gerFuelConsumption_aux = (double*)realloc(
								model.functions.speedLevel[fromLevel].rpmSetting_gerFuelConsumption_aux, nAlloc * sizeof(double));
							model.functions.speedLevel[fromLevel].settingGerBaseSetting = (int*)realloc(
								model.functions.speedLevel[fromLevel].settingGerBaseSetting, nAlloc * sizeof(int));
						}
						set_speedSettingsFromBase(&(model.functions.speedLevel[fromLevel]), newSpeedNr, baseSpeedNr + 1);
					}
					errlog("changing arcNr %d fromLevel %d speedsetting from %d to %d  speeds (knots) %.2lf %.2lf deltaError now %.2lf\n",
						arcNr, fromLevel, speedNr, newSpeedNr,
						model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed[speedNr] / model.params.knots_to_km,
						model.functions.speedLevel[fromLevel].rpmSetting_gerCalmWaterSpeed[newSpeedNr] / model.params.knots_to_km,
						deltaError);
					model.arc[arcNr].speedSetting = newSpeedNr;
					model.arc[arcNr].time -= timeArc[iPosUse] * (1 - speedOld / speedNew);

					if (deltaError < 0)
						break;
				}
			}
		}


	}
	errlog("Estimated new end time %.2lf\n", model.params.eta_h + deltaError);

	free(timeArc);
	model.network.nMaxSplits = DEF_nMAX_SPLITS;

	return 0;
}

int init_setTimeToNextWayPoint() {
	// relates to timeExact in addPositionDataToReport_equalTimeIntervals
	errlog("Add so it's every whole timeperiods...\n");

	double dayHour = model.params.startHour + model.params.startMinute / 60.0;
	int pos = (int)(dayHour / model.params.wayPointHours);
	double timeToNext = (pos + 1) * model.params.wayPointHours - dayHour;
	if (timeToNext < 3 && timeToNext < model.params.wayPointHours / 2)
		timeToNext += model.params.wayPointHours;
	model.functions.timeNextWayPoint = timeToNext;
	return 0;
}

int isSolutionNotPrefPath(int iter) {
	int iPos, posDelay = 0, arcNr, nFelPoints = 0, level;

	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
	{
		arcNr = model.BVArc[iPos];
		if (model.arc[arcNr].speedSetting == -2) {
			copyToArcFromDelay(posDelay, arcNr, 0, iter, 0);
			posDelay++;
			if (iter != 1){// && model.results.onlyPrefPath_kaoutar != 1) {
				if (model.arc[arcNr].fromLevel >= 0) {
					if (posDelay < model.delayRouteToEnd[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
				else {
					if (posDelay < model.delayRouteToEnd_channel[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
			}
			else {
				if (model.arc[arcNr].fromLevel >= 0) {
					if (posDelay < model.delayRouteToEnd_prefPath[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
				else {
					if (posDelay < model.delayRouteToEnd_channel_prefPath[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
			}
			arcNr = model.nArcs;
		}
		level = model.arc[arcNr].fromLevel;
		if (level >= 0)
			nFelPoints += abs(model.arc[arcNr].fromPointNr - model.params.preferredPathOrtoPos[level]);
	}
	errlog("simulated path differ in total %d node positions from preferred path\n", nFelPoints);
	if (nFelPoints > 0)
		return 1;
	return 0;

}

double getArcDirection(int arcNr, spherical::Point* p1) {
	int thisLevel = model.arc[arcNr].fromLevel;
	int nextLevel = model.arc[arcNr].toLevel;
	int pos1 = model.arc[arcNr].fromPointNr;
	int pos2 = model.arc[arcNr].toPointNr;
	spherical::Point p2;
	double x0, y0, x2, y2, direction;

	if (thisLevel >= 0) {
		*p1 = model.network.physicalLev[thisLevel].point[pos1];
		if (nextLevel >= 0) {
			p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			p2 = model.network.channel[-nextLevel - 1].point[0];
		}
	}
	else {
		if (nextLevel >= 0) {
			*p1 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			p2 = model.network.physicalLev[nextLevel].point[pos2];
		}
		else {
			if (thisLevel == nextLevel) {
				*p1 = model.network.channel[-nextLevel - 1].point[0];
				p2 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
			}
			else {
				*p1 = model.network.channel[-thisLevel - 1].point[model.network.channel[-thisLevel - 1].nPoints - 1];
				p2 = model.network.channel[-nextLevel - 1].point[0];
			}
		}
	}

	y0 = (*p1).latitude().degrees();
	x0 = (*p1).longitude().degrees();
	y2 = p2.latitude().degrees();
	x2 = p2.longitude().degrees();
	direction = atan2(y2 - y0, x2 - x0) * 180 / M_PI;

	return direction;
}

int waypoint_checkSpeedDiffOK(double speedLast, double speedNu) {
	if (abs(speedLast - speedNu) / 1.852 <= 0.01)
		return 1;
	else
		return 0;
}

int waypoint_checkdirDiffOK(double dirLast, double dirNu) {
	double dirDiff = abs(dirLast - dirNu);


	if (dirDiff > 300)
		dirDiff = 360 - dirDiff; // +360...
	if (dirDiff <= 10)
		return 1;
	else
		return 0;
}



int modify_nWaypoints(int iter) {
	int iPos, posDelay, arcNr, posNu, nAlloc, i;
	double timeExact;
	spherical::Point p1;

	if (model.waypoint == NULL) {
		nAlloc = 2 * (model.network.nPhysicalLevels + 1);
		model.waypoint = (strWaypoint*)malloc(nAlloc * sizeof(strWaypoint));
	}

	posDelay = 0;
	timeExact = 0;
	posNu = 0;
	model.waypoint[posNu].type = 1;
	model.waypoint[posNu + 1].type = 0;
	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
	{
		arcNr = model.BVArc[iPos];
		if (model.arc[arcNr].speedSetting == -2) {
			if (posDelay == 0 && model.params.eta_h > 0 && iter != 1)
				determineBastSpeedDelay_routeToEnd_eta_arc(arcNr, timeExact);
			if (posDelay == 91)
				posDelay = posDelay;
			copyToArcFromDelay(posDelay, arcNr, timeExact, iter);
			posDelay++;
			if (model.arc[arcNr].fromLevel >= 0) {
				if (posDelay < model.delayRouteToEnd[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
					iPos--;
			}
			else {
				if (posDelay < model.delayRouteToEnd_channel[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
					iPos--;
			}
			arcNr = model.nArcs;
		}

		model.waypoint[posNu].tidp = timeExact;
		if (iPos != model.nBVArcs - 2) {
			model.waypoint[posNu].calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
			model.waypoint[posNu].direction = getArcDirection(arcNr, &p1);
			model.waypoint[posNu].x = p1.longitude().degrees();
			model.waypoint[posNu].y = p1.latitude().degrees();
			model.waypoint[posNu + 1].type = 0;
			if (model.arc[arcNr].fromLevel < 0) {
				model.waypoint[posNu].type = 1;
				model.waypoint[posNu + 1].type = 1;
			}
			model.waypoint[posNu].use = 0;
			posNu++;
		}
		else {
			model.waypoint[posNu].calmWaterSpeed = -1;
			model.waypoint[posNu].direction = -1;
			model.waypoint[posNu].type = -1;
			model.waypoint[posNu].x = model.network.physicalLev[model.arc[arcNr].fromLevel].point[model.arc[arcNr].fromPointNr].longitude().degrees();
			model.waypoint[posNu].y = model.network.physicalLev[model.arc[arcNr].fromLevel].point[model.arc[arcNr].fromPointNr].latitude().degrees();
			model.waypoint[posNu].use = 0;
			posNu++;

			break;
		}

		timeExact += model.arc[arcNr].time;
	}
	model.nWaypoints = posNu;

	char* namn;
	int saveWaypoints = 1;
	FILE* filpek = NULL;
	namn = (char*)malloc2(256 * sizeof(char));

	if (saveWaypoints == 1) {
		sprintf(namn, "%s/checkWaypoints.txt", model.params.indataPath.c_str());
		filpek = fopen(namn, "w");
		fprintf(filpek, "point\tx\ty\ttidp\tcalmWaterSpeed\tdirection\ttype\tuse\n");
		for (i = 0; i < posNu; i++)
			fprintf(filpek, "%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%d\t%d\n", i,
				model.waypoint[i].x, model.waypoint[i].y, model.waypoint[i].tidp,
				model.waypoint[i].calmWaterSpeed, model.waypoint[i].direction,
				model.waypoint[i].type, model.waypoint[i].use);
	}
	int speedDiffOK, dirDiffOK, tidDiffOK, nUse, lastPos, nPointsNu, i1;
	double speedLast, dirLast, tpLast, min_nPoints, tidDiff, tidpPrev, tidp;
	double diff1, diffPrev;

	speedLast = -1;
	dirLast = -360;
	tpLast = -1000;
	for (i = 0; i < model.nWaypoints - 1; i++) {
		if (model.waypoint[i].type == 0) {
			speedDiffOK = waypoint_checkSpeedDiffOK(speedLast, model.waypoint[i].calmWaterSpeed);
			dirDiffOK = waypoint_checkdirDiffOK(dirLast, model.waypoint[i].direction);
			if ((speedDiffOK == 0 || dirDiffOK == 0) && model.waypoint[i].type == 0) {
				model.waypoint[i].type = 2; // needs to be used
				speedLast = model.waypoint[i].calmWaterSpeed;
				dirLast = model.waypoint[i].direction;
			}
		}
		else {
			speedLast = model.waypoint[i].calmWaterSpeed;
			dirLast = model.waypoint[i].direction;
		}
	}

	lastPos = 0;
	tpLast = 0;
	nUse = 1;
	model.waypoint[lastPos].use = 1;
	for (i = 1; i < model.nWaypoints - 1; i++) {
		if (i >= 30)
			i = i;
		if (model.waypoint[i].type > 0 || i == model.nWaypoints - 2) {
			tidp = model.waypoint[i].tidp;
			min_nPoints = (tidp - tpLast) / model.params.aim_waypointInterval_h; // 24.0;
			if (min_nPoints <= 1.2 && i < model.nWaypoints - 2) {
				lastPos = i;
				tpLast = tidp;
				model.waypoint[lastPos].use = 1;
				nUse++;
			}
			else {
				nPointsNu = (int)min_nPoints;
				if (nPointsNu < min_nPoints)
					nPointsNu++;
				tidDiff = (tidp - tpLast) / nPointsNu;
				for (i1 = lastPos + 1; i1 <= i; i1++) {
					if (i1 == model.nWaypoints - 2)
						i1 = i1;
					tidp = model.waypoint[i1].tidp;
					if (tidp - tpLast > tidDiff || i1 == i){
						if (i1 == model.nWaypoints - 2) {
							if (model.waypoint[i1 + 1].tidp - tpLast < model.params.aim_waypointInterval_h &&
								model.waypoint[i].type == 0)
								continue;
						}
						tidpPrev = model.waypoint[i1 - 1].tidp;
						diff1 = tidp - tpLast - tidDiff;
						diffPrev = tidDiff - (tidpPrev - tpLast);
						if (diff1 < diffPrev / 3 || i1 == i) {
							lastPos = i1;
							model.waypoint[lastPos].use = 1;
						}
						else {
							lastPos = i1 - 1;
							if (diff1 / 3 > diffPrev)
								model.waypoint[lastPos].use = 1;
							else
								model.waypoint[lastPos].use = 2;
						}
						tpLast += tidDiff;
						nUse++;
					}
				}
			}
		}
	}


	if (saveWaypoints == 1) {
		fprintf(filpek, "\npoint\tx\ty\ttidp\tcalmWaterSpeed\tdirection\ttype\tuse\n");
		for (i = 0; i < model.nWaypoints; i++)
			fprintf(filpek, "%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%d\t%d\n", i,
				model.waypoint[i].x, model.waypoint[i].y, model.waypoint[i].tidp,
				model.waypoint[i].calmWaterSpeed, model.waypoint[i].direction,
				model.waypoint[i].type, model.waypoint[i].use);
		fclose(filpek);
	}
	free(namn);


	return 0;
}

int writeSolutionToJson(std::string filename, int resAlt, char* namnSol, int iter, int nFinalRoutes)
{
	int nAllocPkter, i, iPos, nPkter, nArcs, ii3, forsta, speedSetting;
	int arcNr, lev1, lev2, pointNr1, pointNr2, timeInt, * nSpeedSettingUsed, nSpeedChanges = 0;
	double* x, * y, xNu, yNu, emissionWaiting, iterStartTidp = 0, iterStartTidpArc = 0, tidTmp;
	double totSafety = 0;
	FILE* filRun;

	double totHurricane = 0, totBowSlam = 0, totGreenWater = 0;
	double totDynStab = 0, totRolling = 0, totSurfRiding = 0;
	double totFeasSafety = 0, totIce = 0, totMaxWaveHeight = 0;


	int speedSettingBase, lastSpeedSetting = -1;

	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	FILE* filpekG;
	time_t rawtime;

	if (model.nArcs >= model.nAllocArcs) {
		model.nAllocArcs += 2;
		model.arc = (strArcInfo*)realloc(model.arc,
			model.nAllocArcs * sizeof(strArcInfo));
	}


	if (model.params.eta_h > -0.01 && iter == 2) {
		makeSure_eta_inTime();
	}

	if (iter > 0) {
		for (i = 0; i < model.nStorms; i++)
			model.storms[i].closestPointToRoute = 1e10;
	}
	else {
		model.functions.valuesNow.compare_totalTime_h = -1;
	}


	if (model.network.nMaxSplits == 100000) {
		char* namn;
		namn = (char*)malloc2(256 * sizeof(char));
		sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
		FILE* filpek10 = fopen(namn, "a+");
		fprintf(filpek10, "\n%s\n", namnSol);
		fclose(filpek10);
		free(namn);
	}

	//printf("here 1\n");
	time(&rawtime);
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = model.params.startMinute;
	tmBas.tm_sec = 0;
	//timeNu = mktime(&tmBas);

	if (model.params.wayPointHours > 0) {
		init_setTimeToNextWayPoint();
		model.functions.valuesNow.coords_lastFromLevel = -10000;
		model.functions.valuesNow.coords_lastToLevel = -10000;
	}

	set_tmBasTime(0);
	model.functions.valuesNow.last_y = model.network.physicalLev[0].point_y[0];
	model.functions.valuesNow.last_x = model.network.physicalLev[0].point_x[0];
	resetValuesNow();

	int nAlloc = model.network.nMaxNodesInPath * model.nBVArcs, prefPath, saveBoth;

	if (resAlt == 0 && iter != 1) {
		for (lev1 = 0; lev1 < model.network.nPhysicalLevels; lev1++) {
			model.optPath.level[lev1].timeArrive = -1;
			model.optPath.level[lev1].pointNr = -1;
			model.optPath.level[lev1].baseSpeedSettingNr = -1;
			model.optPath.level[lev1].levelNext = -1;
		}
		for (lev1 = 0; lev1 < model.network.nChannels; lev1++) {
			model.optPath.channel[lev1].timeArriveNext = -1;
			model.optPath.channel[lev1].speedSettingNrNext = -1;
			model.optPath.channel[lev1].levelNext = -1;
			model.optPath.channel[lev1].timeArriveThrough = -1;
			model.optPath.channel[lev1].speedSettingNrThrough = -1;
		}
	}

	x = (double*)malloc2(nAlloc * sizeof(double));
	y = (double*)malloc2(nAlloc * sizeof(double));
	nSpeedSettingUsed = (int*)calloc2(model.functions.nShip_speedSettingsBase, sizeof(int));

	char* startTime, * endTime, * startTime0;
	startTime = (char*)malloc2(256 * sizeof(char));
	startTime0 = (char*)malloc2(256 * sizeof(char));
	time_t test = mktime(&tmBas);

	FILE* filPek = NULL, * filPek2 = NULL;
	//std::string solName;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	//sprintf(namn, "%s.csv", filename);
	if (SKRIV_UT_NOTHING == 0) {
		if (runAltForecast != 0 && iter == 2) {
			sprintf(namn, "%s/resSol_forecast.txt", model.params.resultPath.c_str());
			filPek = fopen(namn, "a+");
			fprintf(filPek, "\nrunAltForecast\t%d\n", runAltForecast);
			fprintf(filPek, "forecastType\t%d\n", model.results.forecastTypeOrig);
			fprintf(filPek, "optimize\t%d\n", model.iterKaoutar.evalAlt);
			fprintf(filPek, "physicalLevelNu\t%d\n", model.iterKaoutar.physLevelStart);
		}
		else {
			sprintf(namn, "%s/resSol_%s.csv", model.params.resultPath.c_str(), namnSol);
			filPek = fopen(namn, "w");
		}
		sprintf(namn, "%s/solPath_%s.txt", model.params.resultPath.c_str(), namnSol);
		filPek2 = fopen(namn, "w");
		//errlog("\n\nseaMargin\narcNr kvotStart kvotSlut checkPoint startTime delayFactor STW SOG dist timeSTW timeSOG distWind distWave distCurrent"
		//	" distDelay distWeather seaMargin vDiffWind vDiffWave vDiffCurrent vDiffDelay\n");
		//glob_tmpTotDist = 0;
	}
	std::string linePath = "";


	model.results.fileForecast = NULL;
	if (strcmp("base", namnSol) == 0 || strcmp("prefPathFixSpeed", namnSol) == 0) {
		saveBoth = 0;
		if (iter == 2 && nFinalRoutes > 0) {
			saveBoth = 1; // 0;
			//if (model.params.useSimulering == 1) {
				// check if save both sol, if different enough
			//	saveBoth = isSolutionNotPrefPath(iter);
			//}
		}

		if (saveBoth == 0) {
			filpekG = fopen(filename.c_str(), "w"); // "result_json.json", "w");
			if (filpekG == NULL)
			{
				printf("Faile to open file %s for writing.\n", namn);
				errlog("Faile to open file %s for writing.\n", namn);
				postRequest("Faile to open file " + std::string(namn) + " for writing.", 1);
			}
			initGeoJsonFil(filpekG, "result_path");
			linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";

			if (runAltForecast > 0 && iter == 2) {
				sprintf(namn, "%s/iterDataOpt_kaoutar.txt", model.params.resultPath.c_str());
				model.results.fileForecast = fopen(namn, "a+");
				//}

				test = mktime(&tmBas);
				if (test == -1) {
					printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
						tmBas.tm_year,
						tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
					if (model.params.failedTime == 0)
						postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
					model.params.failedTime = 1;
				}
				// fixReadableDate(tmBas, startTime);
				fprintf(model.results.fileForecast, "forecastType;%d\n", model.results.forecastTypeOrig);
			}
		}
		else {
			filpekG = fopen(filename.c_str(), "a+"); // "result_json.json", "w");
			linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";
			fprintf(filpekG, ",\n");
		}

	}
	else {
		if (runAltForecast < 0 && strcmp("eval", namnSol) == 0) {
			// analyses of all the runs
			sprintf(namn, "%s/resPostAnalysis_%s.txt", model.params.resultPath.c_str(),
				model.results.fileNameForecast.c_str());
			if (resAlt == 0) {
				model.results.fileForecast2 = fopen(namn, "w");
				fprintf(model.results.fileForecast2, "sim_dateTime;opt_dateTime;sim_forecastType");
				fprintf(model.results.fileForecast2, ";opt_forecastTypeOpt;weightTime;weightFuel;weightEmission;weightSafety");

				fprintf(model.results.fileForecast2, ";opt_totFuel;opt_totTime;opt_fuelCostDollar;opt_timeCostDollar;opt_totalDistance_kts;opt_safety");
				fprintf(model.results.fileForecast2, ";sim_totFuel;sim_totTime;sim_fuelCostDollar;sim_timeCostDollar;sim_totalDistance_kts;sim_safety\n");
			}
			else {
				model.results.fileForecast2 = fopen(namn, "a+");
			}

			sprintf(namn, "%s/iterData_kaoutar.txt", model.params.resultPath.c_str());
			model.results.fileForecast = fopen(namn, "a+");
			fprintf(model.results.fileForecast, "forecastType;%d\n", model.results.forecastTypeOrig);

			test = mktime(&tmBas);
			if (test == -1) {
				printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
					tmBas.tm_year,
					tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
				if (model.params.failedTime == 0)
					postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
				model.params.failedTime = 1;
			}
			fixReadableDate(tmBas, startTime);
			fprintf(model.results.fileForecast2, "%s", startTime);
			//fprintf(model.results.fileForecast2, ";%s", model.results.optRunDateTime);
			fprintf(model.results.fileForecast2, ";%d", runAltForecast);
			fprintf(model.results.fileForecast2, ";%d", model.results.forecastTypeOrig);
			fprintf(model.results.fileForecast2, ";%lf;%lf;%lf;%lf",
				model.results.weightTime, model.results.weightFuel,
				model.results.weightEmission, model.results.weightSafetyBase);

			if (resAlt == 0) {
				filpekG = fopen(filename.c_str(), "w"); // "result_json.json", "w");
				initGeoJsonFil(filpekG, "result_path");
			}
			else {
				filpekG = fopen(filename.c_str(), "a+"); // "result_json.json", "w");
				fprintf(filpekG, ",\n");
			}
		}
		else {
			filpekG = fopen(filename.c_str(), "a+"); // "result_json.json", "w");
			fprintf(filpekG, ",\n");

		}
		linePath = "{ \"type\": \"Feature\",\n\"geometry\": { \"type\": \"MultiLineString\",\n\"coordinates\": [ [\n";
	}

	if (SKRIV_UT_NOTHING == 0) {
		fprintf(filPek2, "level;nodPos;speedSetting;arcNr(for_information_only);nod1(info);nod2(info)\n");
		fprintf(filPek, "arcPos\tarcNr\tspeedSetting\tcalmWaterSpeed(k)\tsogAllInclusive(k)\tdistance(k)\taccumDist(km)\t"
			"distLeft(km)\ttime(h)\tfuelBase\temission\tsafetyBase\tchannelCost\tmaxWindSpeed\tmaxWaveHeight\tweightCost\tfromLevel\tfromPointNr\tfromTimeInterval\t"
			"toLevel\ttoPointNr\ttoTimeInterval\tlat1\tlon1\tlat2\tlon2\tWindFactor\tWaveFactor\tCurrentFactor\tDelayFactor\t"
			"WindSpeedDiff(k)\tWaveSpeedDiff(k)\tCurrentSpeedDiff(k)\tDelaySpeedDiff(k)\tdollarCostArcs\tforecastType");
#ifdef NAZANIN_SAFETY
		fprintf(filPek, "\tbowSlam\tgreenWater\tdynamicStability\trolling\tsurfRiding\tworstStormValue");
		fprintf(filPek, "\tbowSlamArc\tgreenWaterArc\tdynamicStabilityArc\trollingArc\tsurfRidingArc\tsafetyHurricane");
#endif
		fprintf(filPek, "\texactTime\tdollarCostExact\tfuelCostExact\ttimeCostExact\tchannelCost\n");
	}

	double timeNu = 0, fuel = 0, safety = 0, totCost = 0, distance = 0, channelCost = 0, emission = 0, tmpCheck = 0;
	double fuel_aux = 0, fuel_auxEca = 0, fuel_eca = 0, fuel_noEca = 0, hurricane = 0, distanceTp, distNu, distTmp;// , stability = 0;
	double bowSlamming = 0, greenWater = 0, dynStability = 0, iceCoverage = 0, feasibleSafety = 0, timeExact = 0;
	double fuelCostDollar, fuel_objCost, voyageTime_objCost, emission_objCost;
	int ii, nTp, nAdded, ii2, posIreport;
	double x1, y1, x2, y2, fuelCostDollarNu, dollarCostExactNu;
	//FILE* filtmp = fopen("tmpCheckCoords.txt", "w");
	spherical::Point pointLast, pointFinal;

	int iPosIter = 0;
	if (model.results.fileForecast != NULL) {
		fprintf(model.results.fileForecast, "nBVArcs;%d;nPhysLevels;%d;nArcs;%d\n", model.nBVArcs, model.network.nPhysicalLevels, model.nArcs);
		fprintf(model.results.fileForecast, "pos;arcNr;fromLevel;toLevel;fromPos;toPos;fromTime;toTime;speedSettingBase\n");
		if (runAltForecast > 0) {
			for (iPosIter = 0; iPosIter < model.iterKaoutar.fixedArcsEnd; iPosIter++) {
				fprintf(model.results.fileForecast, "%d;%d;%d;%d;%d;%d;%lf;%lf;%d\n", iPosIter,
					model.iterKaoutar.fixedArcs[iPosIter].arcNr, model.iterKaoutar.fixedArcs[iPosIter].fromLevel,
					model.iterKaoutar.fixedArcs[iPosIter].toLevel, model.iterKaoutar.fixedArcs[iPosIter].fromPos,
					model.iterKaoutar.fixedArcs[iPosIter].toPos, model.iterKaoutar.fixedArcs[iPosIter].fromTime,
					model.iterKaoutar.fixedArcs[iPosIter].toTime, model.iterKaoutar.fixedArcs[iPosIter].speedSettingBase);
			}
		}
	}
	if (runAltForecast != 0) {
		if (runAltForecast > 0 || strcmp("eval", namnSol) != 0)
			timeExact = model.iterKaoutar.tidpStartIter_h;
	}

	if (model.params.simuleraTidVisuellt == 1) {
		plotNodeTimeVisuellt(timeExact, model.preferredPath.point_x[0], model.preferredPath.point_y[0]);
	}


	//printf("here 3\n");
	nArcs = 0;
	arcNr = -1;
	nPkter = 0;
	posIreport = 0;
	model.functions.valuesNow.bearingOldWpt = 1000;
	model.functions.valuesNow.accumDistance = 0;
	model.functions.valuesNow.totDistance = 0;
	model.functions.valuesNow.totDistance_movingNoCorridors = 0;
	model.functions.valuesNow.totTime_movingNoCorridors = 0;
	model.functions.valuesNow.totFuel_mainMovingNoCorridors = 0;

	model.functions.valuesNow.totCorridorWaitingFuel_mainECA = 0;
	model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA = 0;
	model.functions.valuesNow.totCorridorWaitingFuel_auxECA = 0;
	model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA = 0;
	model.functions.valuesNow.totCorridorWaitingTime = 0;

	model.functions.valuesNow.accumRPM = 0;
	model.functions.valuesNow.accumRPM_time = 0;

	model.functions.valuesNow.maxDiffTime = -1e10;
	model.functions.valuesNow.minDiffTime = 1e10;
	double timePrevious = 0;

	int posDelay = 0;
	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++) {
		arcNr = model.BVArc[iPos];
		if (model.arc[arcNr].speedSetting == -2) {
			copyToArcFromDelay(posDelay, arcNr, 0, iter, 0);
			posDelay++;
			if (iter != 1) {// && model.results.onlyPrefPath_kaoutar != 1) {
				if (model.arc[arcNr].fromLevel >= 0) {
					if (posDelay < model.delayRouteToEnd[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
				else {
					if (posDelay < model.delayRouteToEnd_channel[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
			}
			else {
				if (model.arc[arcNr].fromLevel >= 0) {
					if (posDelay < model.delayRouteToEnd_prefPath[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
				else {
					if (posDelay < model.delayRouteToEnd_channel_prefPath[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
			}
			arcNr = model.nArcs;
		}
		model.functions.valuesNow.totDistance += model.arc[arcNr].distance;
		// printf("iPos %d arcNr %d accumDist %.2lf\n", iPos, arcNr, model.functions.valuesNow.totDistance / 1.852);
	}

	model.network.nCoords = 0;
	model.functions.valuesNow.Wpt = 0;
	model.network.last_x = model.preferredPath.startX;

	model.functions.valuesNow.totWindF = 0;
	model.functions.valuesNow.totWaveF = 0;
	model.functions.valuesNow.totCurrentF = 0;
	model.functions.valuesNow.totDelayF = 0;

	model.functions.valuesNow.totFavorableWind_h = 0;
	model.functions.valuesNow.totFavorableWave_h = 0;
	model.functions.valuesNow.totFavorableWindWave_h = 0;

	model.functions.valuesNow.sumWindSpeed = 0;
	model.functions.valuesNow.sumRelCurrent = 0;
	model.functions.valuesNow.sumCurrent = 0;
	model.functions.valuesNow.sumWaveHeight = 0;
	model.functions.valuesNow.maxWindSpeed = 0;
	model.functions.valuesNow.maxWindSpeed_tp = 0;
	model.functions.valuesNow.maxWindSpeed_dir = 0;
	model.functions.valuesNow.maxCurrent = 0;
	model.functions.valuesNow.sumSpeedOnWater = 0;

	model.functions.valuesNow.windDirReal_lastKnown = 9999;
	model.functions.valuesNow.currentReal_lastKnown = 9999;
	model.functions.valuesNow.waveDirReal_lastKnown = 9999;
	model.functions.valuesNow.maxWaveHeight = 0;
	model.functions.valuesNow.maxWaveHeight_tp = 0;
	model.functions.valuesNow.maxWaveHeight_dir = 0;
	model.functions.valuesNow.obj_fel_maxWindSpeed = 0;
	model.functions.valuesNow.obj_fel_maxWaveHeight = 0;


	for (int i = 0; i < 2; i++) {
		for (int i1 = 0; i1 < 3; i1++) {
			model.functions.valuesNow.favorableWind[i][i1] = 0;
			model.functions.valuesNow.favorableWave[i][i1] = 0;
			model.functions.valuesNow.favorableWindWave[i][i1] = 0;
		}
	}

	//for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)

	double iterTotDistStart = 0, iterTotFuelStart = 0, iterTotObjStart = 0, iterTotDollarCostStart = 0;
	double iterWeatherFactorsStart = 0, iterSafetyStart = 0;
	int nArcsUsed, iterSet;
	if (runAltForecast > 0) {
		iterTotDistStart = model.results.iterTotDistStart;
		iterTotFuelStart = model.results.iterTotFuelStart;
		iterTotObjStart = model.results.iterTotObjStart;
		iterTotDollarCostStart = model.results.iterTotDollarCostStart;
		iterStartTidp = model.iterKaoutar.tidpStartIter_h;
		iterStartTidpArc = model.iterKaoutar.tidpStartIterArc_h;
		iterWeatherFactorsStart = model.results.iterWeatherFactorsStart;
		iterSafetyStart = model.results.iterSafetyStart;
		iterSet = 1;
	}
	else
		iterSet = 0;

	if (model.params.aim_waypointInterval_h > 0) {
		modify_nWaypoints(iter);

	}

	resetWaypointData();

	if (model.params.wayPointHours > 0) {
		set_tmBasTime(timeExact);
		fixReportDate(model.params.tmBas, model.params.startTime_short);
		fixReportDate_full(model.params.tmBas, model.params.startTime_full);
	}


	channelCost = 0;
	posDelay = 0;
	nArcsUsed = 0;
	int posWaypoint = 0;
	for (iPos = 0; iPos < model.nBVArcs - 1; iPos++)
	{
		model.functions.valuesNow.totTimeArcSTW = 0;
		model.functions.valuesNow.WindFArc = 0;
		model.functions.valuesNow.WaveFArc = 0;
		model.functions.valuesNow.CurrentFArc = 0;
		model.functions.valuesNow.DelayFArc = 0;

		// kopiera delen av punktfoljden som anvands, dess xyz
		//if (iPos == 6)
		//	iPos = iPos;
		arcNr = model.BVArc[iPos];

		if (posDelay == 89)
			posDelay = posDelay;
		if (posDelay == 92)
			posDelay = posDelay;

		if (model.arc[arcNr].speedSetting == -2) {
			if (posDelay == 0 && model.params.eta_h > 0 && iter != 1)
				determineBastSpeedDelay_routeToEnd_eta_arc(arcNr, timeExact);
			if (posDelay == 18)
				posDelay = posDelay;
			copyToArcFromDelay(posDelay, arcNr, timeExact, iter);
			posDelay++;
			if (model.arc[model.nArcs].fromLevel == 47)
				arcNr = arcNr;
			if (iter != 1) {// && model.results.onlyPrefPath_kaoutar != 1) {
				if (model.arc[arcNr].fromLevel >= 0) {
					if (posDelay < model.delayRouteToEnd[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
				else {
					if (posDelay < model.delayRouteToEnd_channel[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
			}
			else {
				if (model.arc[arcNr].fromLevel >= 0) {
					if (posDelay < model.delayRouteToEnd_prefPath[model.arc[arcNr].fromLevel][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
				else {
					if (posDelay < model.delayRouteToEnd_channel_prefPath[-model.arc[arcNr].fromLevel - 1][model.arc[arcNr].fromPointNr].nBVArcs)
						iPos--;
				}
			}
			arcNr = model.nArcs;
		}
		else {
			if (model.arc[arcNr].maxWaveHeight > model.params.user_maxWaveHeight)
				model.functions.valuesNow.obj_fel_maxWaveHeight += model.simulering.penOverWeatherLimit_fix +
				(model.arc[arcNr].maxWaveHeight - model.params.user_maxWaveHeight) * model.simulering.penOverMaxWaveHeight_m;
			if (model.arc[arcNr].maxWindSpeed > model.params.user_maxWindSpeed_kmh)
				model.functions.valuesNow.obj_fel_maxWindSpeed += model.simulering.penOverWeatherLimit_fix +
				(model.arc[arcNr].maxWindSpeed - model.params.user_maxWindSpeed_kmh) * model.simulering.penOverMaxWindSpeed_kmh;

			if (model.arc[arcNr].bowSlam > model.functions.valuesNow.bowSlamming_max)
				model.functions.valuesNow.bowSlamming_max = model.arc[arcNr].bowSlam;
			if (model.arc[arcNr].greenWater > model.functions.valuesNow.greenWater_max)
				model.functions.valuesNow.greenWater_max = model.arc[arcNr].greenWater;
			if (model.arc[arcNr].dynamicStability > model.functions.valuesNow.dynamicStability_max)
				model.functions.valuesNow.dynamicStability_max = model.arc[arcNr].dynamicStability;
			if (model.arc[arcNr].rolling > model.functions.valuesNow.rolling_max)
				model.functions.valuesNow.rolling_max = model.arc[arcNr].rolling;
			if (model.arc[arcNr].surfRiding > model.functions.valuesNow.surfRiding_max)
				model.functions.valuesNow.surfRiding_max = model.arc[arcNr].surfRiding;
			nArcsUsed++;
		}
		if (model.arc[arcNr].fromLevel == 3)
			arcNr = arcNr;

		totCost += model.arc[arcNr].totCost;
		//model.arc[arcNr].greenWater = model.arc[arcNr].greenWater;
		//model.arc[arcNr].dynamicStability = model.arc[arcNr].dynamicStability;

		// (model.params.weightSafety.greenWater * model.arc[arcNr].greenWater +
		// model.params.weightSafety.dynamicStability* model.arc[arcNr].dynamicStability) +
		// tmpCheck += model.params.weightSafety.base * model.arc[arcNr].safetyBase +
		// 	model.arc[arcNr].time * model.params.weightTime * model.params.priceTime +
		//	model.params.weightFuel * (model.arc[arcNr].fuel_aux * model.params.fuel.aux_noEca.price + 
		//		model.arc[arcNr].fuel_auxEca * model.params.fuel.aux_eca.price + 
		//		model.arc[arcNr].fuel_eca * model.params.fuel.main_eca.price + model.arc[arcNr].fuel_noEca * model.params.fuel.main_noEca.price);
		//printf("arc %d totCost %.2lf tmpCheck %.2lf safetyBase %.2lf %.2lf greenwater %.2lf %.2lf dynStab %.2lf %.2lf time %.2lf %.2lf fuel %.2lf %.2lf\n",
		//	arcNr, totCost, tmpCheck, model.arc[arcNr].safetyBase, model.params.weightSafety.base* model.arc[arcNr].safetyBase,
		//	model.arc[arcNr].greenWater, model.params.weightSafety.greenWater* model.arc[arcNr].greenWater,
		//	model.arc[arcNr].dynamicStability, model.params.weightSafety.dynamicStability* model.arc[arcNr].dynamicStability,
		//	model.arc[arcNr].time, model.arc[arcNr].time* model.params.weightTime* model.params.priceTime,
		//	model.arc[arcNr].fuel_aux + model.arc[arcNr].fuel_auxEca + model.arc[arcNr].fuel_eca + model.arc[arcNr].fuel_noEca,
		//	model.params.weightFuel* (model.arc[arcNr].fuel_aux* model.params.fuel.aux_noEca.price +
		//		model.arc[arcNr].fuel_auxEca * model.params.fuel.aux_eca.price +
		//		model.arc[arcNr].fuel_eca * model.params.fuel.main_eca.price + model.arc[arcNr].fuel_noEca * model.params.fuel.main_noEca.price));


		//errlog("iPos;%d;arcNr;%d;totCost;%lf\n", iPos, arcNr, model.arc[arcNr].totCost);
		//xxx;

		if (iPos == model.nBVArcs - 2) {
			model.network.yCoord[model.network.nCoords] = model.network.physicalLev[model.arc[arcNr].fromLevel].point[model.arc[arcNr].fromPointNr].latitude().degrees();
			model.network.xCoord[model.network.nCoords] = model.network.physicalLev[model.arc[arcNr].fromLevel].point[model.arc[arcNr].fromPointNr].longitude().degrees();
			(model.network.nCoords)++;
			break;
		}
		model.functions.valuesNow.totFuel_aux = 0;
		model.functions.valuesNow.totFuel_main = 0;

		if (arcNr >= 2000)
			arcNr = arcNr;
		if (SKRIV_UT_NOTHING == 0) {
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
					x1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_x[0];
					y1 = model.network.channel[-model.arc[arcNr].fromLevel - 1].point_y[0];
					x2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_x[model.network.channel[-model.arc[arcNr].toLevel - 1].nPoints - 1];
					y2 = model.network.channel[-model.arc[arcNr].toLevel - 1].point_y[model.network.channel[-model.arc[arcNr].toLevel - 1].nPoints - 1];
					// channelCost += model.network.channel[-model.arc[arcNr].toLevel - 1].extraCostChannel;
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
			tidTmp = model.arc[arcNr].time;
			if (tidTmp == 0)
				tidTmp = 0.01;
			fprintf(filPek, "%d\t%d\t%d\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf\t%.6lf"
				"\t%.6lf\t%d\t%d\t%d\t%d\t%d\t%d\t%.6lf\t%.6lf\t%.6lf\t%.6lf",
				iPos, arcNr, model.arc[arcNr].speedSetting,
				eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel) / model.params.knots_to_km,
				model.arc[arcNr].distance / model.params.knots_to_km / tidTmp,
				model.arc[arcNr].distance / model.params.knots_to_km, model.functions.valuesNow.accumDistance,
				model.functions.valuesNow.totDistance - model.functions.valuesNow.accumDistance,
				model.arc[arcNr].time, model.arc[arcNr].fuelBase, model.arc[arcNr].emission, model.arc[arcNr].safetyBase,
				channelCost, model.arc[arcNr].maxWindSpeed, model.arc[arcNr].maxWaveHeight, model.arc[arcNr].totCost, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
				model.arc[arcNr].fromTime, model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
				x1, y1, x2, y2);
			if (model.arc[arcNr].fromLevel == -1)
				arcNr = arcNr;
		}

		if (arcNr == 559126)
			arcNr = arcNr;
		lev1 = model.arc[arcNr].fromLevel;
		lev2 = model.arc[arcNr].toLevel;
		pointNr1 = model.arc[arcNr].fromPointNr;
		pointNr2 = model.arc[arcNr].toPointNr;

		speedSettingBase = get_speedSettingBase(arcNr);
		if (speedSettingBase >= 0) {
			if (lastSpeedSetting != -1 && lastSpeedSetting != speedSettingBase)
				nSpeedChanges++;
			(nSpeedSettingUsed[speedSettingBase])++;
			lastSpeedSetting = speedSettingBase;
		}

		if (model.results.fileForecast != NULL) {
			fprintf(model.results.fileForecast, "%d;%d;%d;%d;%d;%d;%lf;%lf;%d\n", iPosIter++, arcNr, model.arc[arcNr].fromLevel,
				model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].toPointNr,
				model.arc[arcNr].fromTime * model.params.tIndexGerH,
				model.arc[arcNr].toTime * model.params.tIndexGerH, speedSettingBase);
		}


		//if (lev2 < model.network.nPhysicalLevels) {
		//	if(model.arc[arcNr].speedSetting >= 0)
		//		(nSpeedSettingUsed[model.arc[arcNr].speedSetting])++;
		//	//checkMinnesAnvandning(__LINE__);
		//}
		nTp = model.arc[arcNr].toTime - model.arc[arcNr].fromTime;
		if (nTp == 0)
			nTp = 1;
		distanceTp = 1000.0 * model.arc[arcNr].distance / nTp;
		//printf("arcNr %d distance %.3lf nTP %d distanceTP %.2lf fromLevPoint %d %d toLevPoint %d %d\n", 
		//	arcNr, model.arc[arcNr].distance, nTp, distanceTp,
		//	model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr,
		//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr);
		nAdded = 0;

		//if (iPos == 4)
		//	printGlobal = 1;
		//else
		//	printGlobal = 0;

		//if (resAlt == 0)
		//	addPositionDataToReport(filpekG, posIreport++, arcNr, 0, &timeExact, "base");
		//else

		if (resAlt == 0 && iter != 1) {
			// speedSetting = getClosestSetting_fromBase(model.arc[arcNr].speedSetting, lev1, lev2);
			speedSetting = getBaseSpeedSetting(model.arc[arcNr].speedSetting, lev1, lev2);
			if (lev1 >= 0) {
				model.optPath.level[lev1].timeArrive = timeExact;
				model.optPath.level[lev1].pointNr = pointNr1;
				model.optPath.level[lev1].baseSpeedSettingNr = speedSetting;
				model.optPath.level[lev1].levelNext = lev2;
			}
			else {
				if (lev2 >= 0) {
					model.optPath.channel[-lev1 - 1].timeArriveNext = timeExact;
					model.optPath.channel[-lev1 - 1].speedSettingNrNext = speedSetting;
					model.optPath.channel[-lev1 - 1].levelNext = lev2;
				}
				else {
					model.optPath.channel[-lev1 - 1].timeArriveThrough = timeExact;
					model.optPath.channel[-lev1 - 1].speedSettingNrThrough = speedSetting;
				}
			}
		}

		//if (model.arc[arcNr].fromLevel == -2)
		//	arcNr = arcNr;
		//if (arcNr == 62)
		//	arcNr = arcNr;
		if (model.params.wayPointHours > 0) {
			addPositionDataToReport_equalTimeIntervals(filpekG, &posIreport, arcNr, 0, &timeExact, namnSol, 0, iter, model.nBVArcs - 1 - iPos);
		}
		else {
			if (model.params.aim_waypointInterval_h > 0) {
				model.functions.valuesNow.sparaWaypointPos = model.waypoint[posWaypoint].use;
			}
			else
				model.functions.valuesNow.sparaWaypointPos = -1;
			addPositionDataToReport(filpekG, &posIreport, arcNr, 0, &timeExact, namnSol, 0, iter);
		}
		posWaypoint++;
		if (SKRIV_UT_NOTHING == 0) {
			if (model.functions.valuesNow.totTimeArcSTW < 0.0001)
				model.functions.valuesNow.totTimeArcSTW = 0.1;
			fprintf(filPek, "\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t",
				model.functions.valuesNow.WindFArc / model.params.knots_to_km,
				model.functions.valuesNow.WaveFArc / model.params.knots_to_km,
				model.functions.valuesNow.CurrentFArc / model.params.knots_to_km,
				model.functions.valuesNow.DelayFArc / model.params.knots_to_km,
				model.functions.valuesNow.WindFArc / model.params.knots_to_km / model.functions.valuesNow.totTimeArcSTW,
				model.functions.valuesNow.WaveFArc / model.params.knots_to_km / model.functions.valuesNow.totTimeArcSTW,
				model.functions.valuesNow.CurrentFArc / model.params.knots_to_km / model.functions.valuesNow.totTimeArcSTW,
				model.functions.valuesNow.DelayFArc / model.params.knots_to_km / model.functions.valuesNow.totTimeArcSTW);

			fprintf(filPek, "%.3lf\t", model.params.priceTime * model.arc[arcNr].time + model.arc[arcNr].fuelBase);
			//model.params.weightEmission * model.params.scaleObjEmission);
			//fprintf(filPek, "%.3lf\t", model.params.weightTime * model.params.priceTime * model.arc[arcNr].time +
			//	model.params.weightFuel * model.arc[arcNr].fuelBase +
			// model.params.weightEmission * model.params.scaleObjEmission);

			if (model.functions.valuesNow.forecastType > 0.5) {
				if (model.params.hindCast == 0)
					fprintf(filPek, "Ext. Hist");
				else
					fprintf(filPek, "Past Hist");
			}
			else {
				if (model.params.hindCast == 0)
					fprintf(filPek, "Fcst");
				else {
					if (model.functions.valuesNow.forecastType > 0.05)
						fprintf(filPek, "Missing Hist");
					else
						fprintf(filPek, "Hist");
				}
			}
#ifdef NAZANIN_SAFETY
			fprintf(filPek, "\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf%lf\t%lf\t%lf\t%lf\t%lf\t%lf",
				model.functions.valuesNow.bowSlam, model.functions.valuesNow.greenWater,
				model.functions.valuesNow.dynamicStability, model.functions.valuesNow.rolling,
				model.functions.valuesNow.surfRiding, model.functions.valuesNow.worstStormValue,
				model.arc[arcNr].bowSlam, model.arc[arcNr].greenWater, model.arc[arcNr].dynamicStability, 
				model.arc[arcNr].rolling, model.arc[arcNr].surfRiding, model.arc[arcNr].safetyHurricane);
			addStatisticsSafety(arcNr);
#endif
			fprintf(filPek, "\t%lf", timeExact);

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
			if (lev1 == lev2) {
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
								//distance += model.arc[arcNr].distance / nTp;
								//printf("arcNr1 %d totDist %.2lf\n", arcNr, distance / 1.852);
								//time += model.arc[arcNr].time / nTp;
								//fuel += model.arc[arcNr].fuelBase / nTp;
								//emission += model.arc[arcNr].emission / nTp;
								//fuel_aux += model.arc[arcNr].fuel_aux / nTp;
								//fuel_auxEca += model.arc[arcNr].fuel_auxEca / nTp;
								//fuel_eca += model.arc[arcNr].fuel_eca / nTp;
								//fuel_noEca += model.arc[arcNr].fuel_noEca / nTp;
								//printf("arcNr %d fuel_noEca %.2lf bagnoEca %.2lf nTp %d\n", arcNr, fuel_noEca, model.arc[arcNr].fuel_noEca, nTp);
								//if(ii == nTp - 1)
								//	printf("arcNr %d fuelArc_noEca %.2lf arcTime %.2lf totFuel_noEca %.2lf\n", arcNr, model.arc[arcNr].fuel_noEca, model.arc[arcNr].time, fuel_noEca);
								//safety += model.arc[arcNr].safetyBase / nTp;
								//hurricane += model.arc[arcNr].safetyHurricane / nTp;
								//bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
								//greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
								//dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
								//iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
								//feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
								//stability += model.arc[arcNr].safetyStability / nTp;
								//channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;
								//totCost += model.arc[arcNr].totCost / nTp;
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
					y[nPkter] = model.network.channel[-lev1 - 1].point_y[i];
					x[nPkter] = model.network.channel[-lev1 - 1].point_x[i];
					//fprintf(filtmp, "pkt %d lev1 %d prefPath i %d ii last distTmp %.3lf xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, i, distTmp, x[nPkter], y[nPkter], __LINE__);
					pointLast = spherical::Point(y[nPkter], x[nPkter]);
					//z[nPkter] = 0;
					nPkter++;
				}
				yNu = model.network.channel[-lev2 - 1].point_y[i - 1];
				xNu = model.network.channel[-lev2 - 1].point_x[i - 1];
			}
			else {
				// between two corridors

				if (nPkter == 0) {
					if (nPkter >= nAlloc) {
						nAlloc += 1000;
						x = (double*)realloc(x, nAlloc * sizeof(double));
						y = (double*)realloc(y, nAlloc * sizeof(double));
					}
					y[nPkter] = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1].latitude().degrees();
					x[nPkter] = model.network.channel[-lev1 - 1].point[model.network.channel[-lev1 - 1].nPoints - 1].longitude().degrees();
					nPkter++;
				}
				yNu = model.network.channel[-lev2 - 1].point[0].latitude().degrees();
				xNu = model.network.channel[-lev2 - 1].point[0].longitude().degrees();

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
					nPkter++;
				}
			}
			distance += model.arc[arcNr].distance;
			//printf("arcNr2 %d totDist %.2lf\n", arcNr, distance / 1.852);
			timeNu += model.arc[arcNr].time;
			fuel += model.arc[arcNr].fuelBase;
			safety += model.arc[arcNr].safetyBase;
			hurricane += model.arc[arcNr].safetyHurricane;
			bowSlamming += model.functions.valuesNow.bowSlam; // model.arc[arcNr].safetyBowSlam / nTp;
			greenWater += model.functions.valuesNow.greenWater; // model.arc[arcNr].safetyGreenWater / nTp;
			dynStability += model.functions.valuesNow.dynamicStability; // model.arc[arcNr].safetyDynStability / nTp;
			iceCoverage += model.functions.valuesNow.iceCoverCost; // model.arc[arcNr].iceCoverCost / nTp;
			feasibleSafety += (double)model.functions.valuesNow.feasibleSafety; // (model.arc[arcNr].feasibleSafety) / nTp;
			//stability += model.arc[arcNr].safetyStability / nTp;
			channelCost += model.functions.valuesNow.channelCost; // model.arc[arcNr].channelCost / nTp;
			//totCost += model.arc[arcNr].totCost;
			//printf("total objective cost3 after arcNr %d %.2lf arcCost %.2lf\n", arcNr, totCost, model.arc[arcNr].totCost);
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
		else {
			prefPath = 0;
			if (lev1 >= 0 && lev2 >= 0) {
				if (lev2 < model.network.nPhysicalLevels) {
					if (pointNr1 == model.params.preferredPathOrtoPos[lev1] &&
						pointNr2 == model.params.preferredPathOrtoPos[lev2] && lev1 == lev2 - 1
						&& (model.params.preferredPathStraightLineFeasibleFrom[lev1] == 0 || model.params.max_changeDirection == 0 || iter == 1)) {
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
									//printf("arcNr3 %d totDist %.2lf\n", arcNr, distance / 1.852);
									timeNu += model.arc[arcNr].time / nTp;
									fuel += model.arc[arcNr].fuelBase / nTp;
									//emission += model.arc[arcNr].emission / nTp;
									//fuel_aux += model.arc[arcNr].fuel_aux / nTp;
									//fuel_auxEca += model.arc[arcNr].fuel_auxEca / nTp;
									//fuel_eca += model.arc[arcNr].fuel_eca / nTp;
									//fuel_noEca += model.arc[arcNr].fuel_noEca / nTp;
									//if (ii == nTp - 1)
									//	printf("arcNr2 %d fuelArc_noEca %.2lf arcTime %.2lf totFuel_noEca %.2lf\n", arcNr, model.arc[arcNr].fuel_noEca, model.arc[arcNr].time, fuel_noEca);
									safety += model.arc[arcNr].safetyBase / nTp;
									if (safety > 100)
										safety = safety;
									hurricane += model.arc[arcNr].safetyHurricane / nTp;
									bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
									greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
									dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
									iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
									feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
									//stability += model.arc[arcNr].safetyStability / nTp;
									channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;
									//totCost += model.arc[arcNr].totCost / nTp;
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
			else {
				if (lev2 < 0) { // to channel
					if (pointNr1 == model.params.preferredPathOrtoPos[lev1] &&
						model.network.channel[-lev2 - 1].followExactly == 1 &&
						model.network.channel[-lev2 - 1].bastStartLevel == lev1 &&
						model.network.channel[-lev2 - 1].straightArcFeasible_toChannelFromPrefPath == 0 &&
						model.network.channel[-lev2 - 1].preferredPathPoint_posConnectTo >= 0) {
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
						for (int i3 = 0; i3 <= model.network.channel[-lev2 - 1].preferredPathPoint_posConnectTo; i3++) {
							distTmp = pointLast.distanceTo(model.network.physicalLev[lev1].preferredPathPoint[i3]);
							for (ii = 0; ii < nTp; ii++) {
								if (distNu + distTmp >= distanceTp || (i3 == model.network.physicalLev[lev1].npreferredPathPoints - 1 && distNu + distTmp >= distanceTp * 0.95)) {
									// identifiera pkten dar distTmp + distNu = distanceTp
									pointLast = pointLast.destinationPoint(distanceTp - distNu, pointLast.bearingTo(model.network.physicalLev[lev1].preferredPathPoint[i3]));
									// spara pkten dar distTmp + distNu = distanceTp
									distance += model.arc[arcNr].distance / nTp;
									timeNu += model.arc[arcNr].time / nTp;
									fuel += model.arc[arcNr].fuelBase / nTp;
									safety += model.arc[arcNr].safetyBase / nTp;
									hurricane += model.arc[arcNr].safetyHurricane / nTp;
									bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
									greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
									dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
									iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
									feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
									//stability += model.arc[arcNr].safetyStability / nTp;
									channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;

									nAdded++;
									if (nAdded >= nTp)
										break;
									distTmp -= distanceTp - distNu;
									ii3++;
									distNu = 0;
									if (nPkter >= nAlloc) {
										nAlloc += 1000;
										x = (double*)realloc(x, nAlloc * sizeof(double));
										y = (double*)realloc(y, nAlloc * sizeof(double));
									}
									y[nPkter] = pointLast.latitude().degrees();
									x[nPkter] = pointLast.longitude().degrees();
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
							nPkter++;
						}
						prefPath = 1;
					}
				}
				else { // lev1 < 0, from channel
					if (pointNr2 == model.params.preferredPathOrtoPos[lev2] &&
						model.network.channel[-lev1 - 1].followExactly == 1 &&
						model.network.channel[-lev1 - 1].bastEndLevel == lev2 &&
						model.network.channel[-lev1 - 1].straightArcFeasible_fromChannelToPrefPath == 0 &&
						model.network.channel[-lev1 - 1].preferredPathPoint_posConnectFrom >= 0) {
						if (nPkter == 0) {
							y[nPkter] = model.network.channel[-lev1 - 1].point_y[pointNr1];
							x[nPkter] = model.network.channel[-lev1 - 1].point_x[pointNr1];
							//fprintf(filtmp, "pkt %d lev1 %d NOprefPath i -- ii -- distTmp -- xy %.3lf %.3lf codeLine %d\n", nPkter, lev1, x[nPkter], y[nPkter], __LINE__);
							//z[nPkter] = 0;
							nPkter++;
						}
						pointLast = spherical::Point(y[nPkter - 1], x[nPkter - 1]);
						//nPkter = 1;
						distNu = 0;
						ii3 = 0;
						for (int i3 = model.network.channel[-lev1 - 1].preferredPathPoint_posConnectTo;
							i3 < model.network.physicalLev[lev2 - 1].npreferredPathPoints; i3++) {
							distTmp = pointLast.distanceTo(model.network.physicalLev[lev2 - 1].preferredPathPoint[i3]);
							for (ii = 0; ii < nTp; ii++) {
								if (distNu + distTmp >= distanceTp || (i3 == model.network.physicalLev[lev2 - 1].npreferredPathPoints - 1 && distNu + distTmp >= distanceTp * 0.95)) {
									// identifiera pkten dar distTmp + distNu = distanceTp
									pointLast = pointLast.destinationPoint(distanceTp - distNu, pointLast.bearingTo(model.network.physicalLev[lev2 - 1].preferredPathPoint[i3]));
									// spara pkten dar distTmp + distNu = distanceTp
									distance += model.arc[arcNr].distance / nTp;
									timeNu += model.arc[arcNr].time / nTp;
									fuel += model.arc[arcNr].fuelBase / nTp;
									safety += model.arc[arcNr].safetyBase / nTp;
									hurricane += model.arc[arcNr].safetyHurricane / nTp;
									bowSlamming += model.functions.valuesNow.bowSlam / nTp; // model.arc[arcNr].safetyBowSlam / nTp;
									greenWater += model.functions.valuesNow.greenWater / nTp; // model.arc[arcNr].safetyGreenWater / nTp;
									dynStability += model.functions.valuesNow.dynamicStability / nTp; // model.arc[arcNr].safetyDynStability / nTp;
									iceCoverage += model.functions.valuesNow.iceCoverCost / nTp; // model.arc[arcNr].iceCoverCost / nTp;
									feasibleSafety += (double)model.functions.valuesNow.feasibleSafety / nTp; // (model.arc[arcNr].feasibleSafety) / nTp;
									//stability += model.arc[arcNr].safetyStability / nTp;
									channelCost += model.functions.valuesNow.channelCost / nTp; // model.arc[arcNr].channelCost / nTp;

									nAdded++;
									if (nAdded >= nTp)
										break;
									distTmp -= distanceTp - distNu;
									ii3++;
									distNu = 0;
									if (nPkter >= nAlloc) {
										nAlloc += 1000;
										x = (double*)realloc(x, nAlloc * sizeof(double));
										y = (double*)realloc(y, nAlloc * sizeof(double));
									}
									y[nPkter] = pointLast.latitude().degrees();
									x[nPkter] = pointLast.longitude().degrees();
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
							y[nPkter] = model.network.physicalLev[lev2 - 1].preferredPathPoint[i3].latitude().degrees();
							x[nPkter] = model.network.physicalLev[lev2 - 1].preferredPathPoint[i3].longitude().degrees();
							pointLast = spherical::Point(y[nPkter], x[nPkter]);
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
				//printf("arcNr4 %d totDist %.2lf\n", arcNr, distance / 1.852);
				timeNu += model.arc[arcNr].time;
				fuel += model.arc[arcNr].fuelBase;
				//emission += model.arc[arcNr].emission;
				//fuel_aux += model.arc[arcNr].fuel_aux;
				//fuel_auxEca += model.arc[arcNr].fuel_auxEca;
				//fuel_eca += model.arc[arcNr].fuel_eca;
				//fuel_noEca += model.arc[arcNr].fuel_noEca;
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
				//totCost += model.arc[arcNr].totCost;
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
		emission += model.functions.valuesNow.totFuel_aux * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.aux_eca.emissionFactor +
			model.functions.valuesNow.totFuel_aux * model.arc[arcNr].fuelQualityKvot * model.params.fuel.aux_noEca.emissionFactor +
			model.functions.valuesNow.totFuel_main * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.main_eca.emissionFactor +
			model.functions.valuesNow.totFuel_main * model.arc[arcNr].fuelQualityKvot * model.params.fuel.main_noEca.emissionFactor;
		fuel_aux += model.functions.valuesNow.totFuel_aux * model.arc[arcNr].fuelQualityKvot;
		fuel_auxEca += model.functions.valuesNow.totFuel_aux * (1 - model.arc[arcNr].fuelQualityKvot);

		if (model.functions.valuesNow.totFuel_main * (1 - model.arc[arcNr].fuelQualityKvot) > 0.01)
			fuel_eca = fuel_eca;
		fuel_eca += model.functions.valuesNow.totFuel_main * (1 - model.arc[arcNr].fuelQualityKvot);
		if (fuel_eca > 0)
			fuel_eca = fuel_eca;

		fuel_noEca += model.functions.valuesNow.totFuel_main * model.arc[arcNr].fuelQualityKvot;

		if (SKRIV_UT_NOTHING == 0) {
			//fuelCostDollar = fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
			//	fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price;
			//dollarCost = fuelCostDollar + timeNu * model.params.priceTime + channelCost;
			fuelCostDollarNu = model.functions.valuesNow.totFuel_main * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.main_eca.price +
				model.functions.valuesNow.totFuel_main * model.arc[arcNr].fuelQualityKvot * model.params.fuel.main_noEca.price +
				model.functions.valuesNow.totFuel_aux * model.arc[arcNr].fuelQualityKvot * model.params.fuel.aux_noEca.price +
				model.functions.valuesNow.totFuel_aux * (1 - model.arc[arcNr].fuelQualityKvot) * model.params.fuel.aux_eca.price;
			dollarCostExactNu = fuelCostDollarNu + (timeExact - timePrevious) * model.params.priceTime + model.functions.valuesNow.channelCost;
			fprintf(filPek, "\t%lf\t%lf\t%lf\t%lf\n", dollarCostExactNu, fuelCostDollarNu,
				(timeExact - timePrevious) * model.params.priceTime, model.functions.valuesNow.channelCost);
			timePrevious = timeExact;
		}

		safety = model.functions.valuesNow.worstStormValue *
			model.params.weightSafety.hurricane +
			model.functions.valuesNow.bowSlam *
			model.params.weightSafety.bowSlam +
			model.functions.valuesNow.greenWater *
			model.params.weightSafety.greenWater +
			model.functions.valuesNow.dynamicStability *
			model.params.weightSafety.dynamicStability +
			model.functions.valuesNow.rolling *
			model.params.weightSafety.rolling +
			model.functions.valuesNow.surfRiding *
			model.params.weightSafety.surfRiding +
			(1 - model.functions.valuesNow.feasibleSafety) *
			model.params.weightSafety.feasibleSafety +
			model.functions.valuesNow.iceCoverCost;
		if (model.functions.valuesNow.maxWaveHeight > model.functions.maxWaveHeight)
			safety += 1e12 * (1 + model.functions.valuesNow.maxWaveHeight - model.functions.maxWaveHeight);
#ifdef NAZANIN_SAFETY
		totHurricane += model.functions.valuesNow.worstStormValue *
			model.params.weightSafety.hurricane;
		totBowSlam += model.functions.valuesNow.bowSlam *
			model.params.weightSafety.bowSlam;
		totGreenWater += model.functions.valuesNow.greenWater *
			model.params.weightSafety.greenWater;
		totDynStab += model.functions.valuesNow.dynamicStability *
			model.params.weightSafety.dynamicStability;
		totRolling += model.functions.valuesNow.rolling *
			model.params.weightSafety.rolling;
		totSurfRiding += model.functions.valuesNow.surfRiding *
			model.params.weightSafety.surfRiding;
		totFeasSafety += (1 - model.functions.valuesNow.feasibleSafety) *
			model.params.weightSafety.feasibleSafety;
		totIce += model.functions.valuesNow.iceCoverCost;
		if (model.functions.valuesNow.maxWaveHeight > model.functions.maxWaveHeight)
			totMaxWaveHeight += 1e12 * (1 + model.functions.valuesNow.maxWaveHeight - model.functions.maxWaveHeight);
		totSafety += model.arc[arcNr].safetyBase;
#else
		totSafety += safety;
#endif

		model.functions.valuesNow.feasibleSafety;
		if (runAltForecast < 0) {
			if (model.arc[arcNr].toLevel >= model.iterKaoutar.physLevelStartUse + model.iterKaoutar.nLevelsMoveForeward && iterSet == 0) {
				iterTotDistStart = distance;
				iterTotFuelStart = fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca;
				iterTotObjStart = totCost;
				iterTotDollarCostStart = fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
					fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
					timeNu * model.params.priceTime + channelCost;
				iterStartTidp = timeExact; // timeNu;
				iterStartTidpArc = timeNu;
				iterWeatherFactorsStart = (model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
					model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km;
				iterSafetyStart = totSafety;
				iterSet = 1;
			}
		}
		//printf("efter arcNr %d tot fuel_eca %.3lf\n", arcNr, fuel_eca);
	}


	if (model.params.wayPointHours <= 0)
		sparaLastWaypoint(filpekG, &posIreport, namnSol);
	//printf("here 3b\n");

	// add a waypoint at the end point 20240828
	model.waypointResult.arcNr = -2;
	model.waypointResult.xCoord = model.network.xCoord[model.network.nCoords - 1];
	model.waypointResult.yCoord = model.network.yCoord[model.network.nCoords - 1];
	fixPositionString_latLon(model.waypointResult.yCoord, model.waypointResult.xCoord,
		model.waypointResult.fixPositionString_latlon);

	tmBas.tm_min += timeExact * 60;
	test = mktime(&tmBas);
	fixReportDate(tmBas, model.waypointResult.dateUTC);
	fixReportDate_full(tmBas, model.waypointResult.full_Date);
	sparaLastWaypoint(filpekG, &posIreport, namnSol);
	tmBas.tm_min -= timeExact * 60;

	//fclose(filtmp);

	//if(arcNr >= 0)
	//	addPositionDataToReport(filpekG, posIreport++, arcNr, 1, &timeExact, solName);

	if (posIreport > 0)
		fprintf(filpekG, ", ");
	fprintf(filpekG, "%s", linePath.c_str());

	//for (ii2 = 0; ii2 < nPkter; ii2++) {

	model.network.last_x = model.preferredPath.startX;
	for (ii2 = 0; ii2 < model.network.nCoords; ii2++) {
		if (ii2 > 0) {
			fprintf(filpekG, ", ");
		}
		//fprintf(filpekG, "[ %lf, %lf, 0.0 ]\n", x[ii2], y[ii2]);

		fprintf(filpekG, "[ %lf, %lf, 0.0 ]\n", getCorrect_longitude(model.network.xCoord[ii2]), model.network.yCoord[ii2]);
	}

	//printf("here 3c\n");

	fprintf(filpekG, "\n]]},\n\"properties\": {\n");

	if (model.params.wayPointHours > 0) {
		model.functions.valuesNow.totWindF += model.functions.valuesNow.WindF;
		model.functions.valuesNow.totWaveF += model.functions.valuesNow.WaveF;
		model.functions.valuesNow.totCurrentF += model.functions.valuesNow.CurrentF;
		model.functions.valuesNow.totDelayF += model.functions.valuesNow.DelayF;

		model.functions.valuesNow.totFavorableWind_h += model.functions.valuesNow.favorableWind_h;
		model.functions.valuesNow.totFavorableWave_h += model.functions.valuesNow.favorableWave_h;
		model.functions.valuesNow.totFavorableWindWave_h += model.functions.valuesNow.favorableWindWave_h;
	}

	double averSpeed, dollarCost, lateEtaCost, earlyEtaCost;

	tmBas.tm_min += model.functions.valuesNow.maxWaveHeight_tp * 60;
	test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	fixReadableDate(tmBas, startTime);
	errlog("maxWaveHeight %.3lf at tp %d %s\n",
		model.functions.valuesNow.maxWaveHeight, model.functions.valuesNow.maxWaveHeight_tp, startTime);
	tmBas.tm_min -= model.functions.valuesNow.maxWaveHeight_tp * 60;

	tmBas.tm_min += model.functions.valuesNow.maxWindSpeed_tp * 60;
	test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	fixReadableDate(tmBas, startTime);
	errlog("maxWindSpeed %.3lf km/h %.3lf kts at tp %d %s\n",
		model.functions.valuesNow.maxWindSpeed, model.functions.valuesNow.maxWindSpeed / 1.852, model.functions.valuesNow.maxWindSpeed_tp, startTime);
	tmBas.tm_min -= model.functions.valuesNow.maxWindSpeed_tp * 60;

	test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}




	endTime = (char*)malloc2(256 * sizeof(char));
	fixReadableDate(tmBas, startTime0);
	timeNu = timeExact;
	tmBas.tm_min += timeNu * 60;
	test = mktime(&tmBas);
	if (test == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	fixReadableDate(tmBas, endTime);
	tmBas.tm_min -= timeNu * 60;

	//if(resAlt == 0)
	//	fprintf(filpekG, "\"solutionID\": \"base\", \"ID\": \"routeInfo\", \"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n", 
	//		startTime, endTime);
	//else
	fprintf(filpekG, "\"solutionID\": \"%s\", \"ID\": \"routeInfo\", \"route_startTime\": \"%s\", \"route_endTime\": \"%s\"\n",
		namnSol, startTime0, endTime);



	fprintf(filpekG, ", \"fuelConsumption_ton\": %.2lf, \"fuelEcaMain\": %.2lf, \"fuelEcaAux\": %.2lf, \"fuelNotEcaMain\": %.2lf, \"fuelNotEcaAux\": %.2lf",
		fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca
		- model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA
		- model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA
		- model.functions.valuesNow.totCorridorWaitingFuel_mainECA
		- model.functions.valuesNow.totCorridorWaitingFuel_auxECA,
		fuel_eca - model.functions.valuesNow.totCorridorWaitingFuel_mainECA,
		fuel_auxEca - model.functions.valuesNow.totCorridorWaitingFuel_auxECA,
		fuel_noEca - model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA,
		fuel_aux - model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA);

	emissionWaiting = model.functions.valuesNow.totCorridorWaitingFuel_auxECA * model.params.fuel.aux_eca.emissionFactor +
		model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA * model.params.fuel.aux_noEca.emissionFactor +
		model.functions.valuesNow.totCorridorWaitingFuel_mainECA * model.params.fuel.main_eca.emissionFactor +
		model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA * model.params.fuel.main_noEca.emissionFactor;

	fprintf(filpekG, ", \"emission\": %.2lf, \"emissionEcaMain\": %.2lf, \"emissionEcaAux\": %.2lf, \"emissionNotEcaMain\": %.2lf, \"emissionNotEcaAux\": %.2lf",
		emission - emissionWaiting, (fuel_eca - model.functions.valuesNow.totCorridorWaitingFuel_mainECA) * model.params.fuel.main_eca.emissionFactor,
		(fuel_auxEca - model.functions.valuesNow.totCorridorWaitingFuel_auxECA) * model.params.fuel.aux_eca.emissionFactor,
		(fuel_noEca - model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA) * model.params.fuel.main_noEca.emissionFactor,
		(fuel_aux - model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA) * model.params.fuel.aux_noEca.emissionFactor);

	fprintf(filpekG, ", \"corridorWaitingFuelMain\": %.2lf, \"corridorWaitingFuelAux\": %.2lf",
		model.functions.valuesNow.totCorridorWaitingFuel_mainECA + model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA,
		model.functions.valuesNow.totCorridorWaitingFuel_auxECA + model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA);
	fprintf(filpekG, ", \"corridorWaitingEmissionMain\": %.2lf, \"corridorWaitingEmissionAux\": %.2lf",
		model.functions.valuesNow.totCorridorWaitingFuel_mainECA * model.params.fuel.main_noEca.emissionFactor +
		model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA * model.params.fuel.main_eca.emissionFactor,
		model.functions.valuesNow.totCorridorWaitingFuel_auxECA * model.params.fuel.aux_eca.emissionFactor +
		model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA * model.params.fuel.aux_noEca.emissionFactor);


	fprintf(filpekG, ",    \"WindF\":%.3lf, \"WaveF\":%.3lf, \"CurrentF\":%.3lf, \"DelayF\":%.3lf, \"allWeatherFactors\": %.3lf\n",
		model.functions.valuesNow.totWindF / model.params.knots_to_km,
		model.functions.valuesNow.totWaveF / model.params.knots_to_km,
		model.functions.valuesNow.totCurrentF / model.params.knots_to_km,
		model.functions.valuesNow.totDelayF / model.params.knots_to_km,
		(model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
			model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km);

	if (timeNu - model.functions.valuesNow.totCorridorWaitingTime > 0)
		averSpeed = distance / (timeNu - model.functions.valuesNow.totCorridorWaitingTime) / model.params.knots_to_km;
	else
		averSpeed = 0;

	fprintf(filpekG, ", \"average speed\": %.1lf, \"safety\": %.3lf, \"channel cost\": %.1lf, \"total objective cost\": %.1lf,",
		averSpeed, totSafety, channelCost, totCost);

	fuelCostDollar = fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
		fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price;
	dollarCost = fuelCostDollar + timeNu * model.params.priceTime + channelCost;
	fprintf(filpekG, "    \"fuelConsumptionExtra_ton\":%.0lf,\n", model.params.fuel.extra_fuel.quantity);
	fprintf(filpekG, "    \"emissionExtra\":%.0lf,\n", model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.emissionFactor);
	fprintf(filpekG, "    \"dollar_cost\":%.0lf,\n", dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price);
	fprintf(filpekG, "    \"fuel_dollarCost\":%.0lf,\n", fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
		fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price);
	fprintf(filpekG, "    \"voyageTime_dollarCost\":%.0lf,\n", timeNu * model.params.priceTime);

	if (resAlt == 0 || runAltForecast < 0) {
		fuel_objCost = model.params.weightFuel * fuel;
		voyageTime_objCost = model.params.weightTime * timeNu * model.params.priceTime;
		emission_objCost = model.params.weightEmission * emission * model.params.scaleObjEmission;
	}
	else {
		fuel_objCost = model.params.extraOptWeights[resAlt - 1].weightFuel * (fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
			fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price);
		voyageTime_objCost = model.params.extraOptWeights[resAlt - 1].weightTime * timeNu * model.params.priceTime;
		emission_objCost = model.params.extraOptWeights[resAlt - 1].weightEmission * emission * model.params.scaleObjEmission;
	}


	fprintf(filpekG, "    \"fuel_objCost\":%.1lf,\n", fuel_objCost);
	fprintf(filpekG, "    \"voyageTime_objCost\":%.1lf,\n", voyageTime_objCost);
	fprintf(filpekG, "    \"emission_objCost\":%.1lf,\n", emission_objCost);
	lateEtaCost = 0;
	earlyEtaCost = 0;
	if (model.params.eta_h > 0.01) {
		if (timeNu < model.params.eta_h) {
			if (resAlt == 0)
				earlyEtaCost = (model.params.eta_h - timeNu) * model.params.eta_cost_early;
			else
				earlyEtaCost = (model.params.eta_h - timeNu) * model.params.extraOptWeights[resAlt - 1].eta_cost_early;
		}
		else
			lateEtaCost = (timeNu - model.params.eta_h) * model.params.eta_cost_late;
	}

	fprintf(filpekG, "    \"eta error cost\":%.1lf,\n", earlyEtaCost + lateEtaCost);

	fprintf(filpekG, "\"storms\": [\n");
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

#ifdef NAZANIN_SAFETY

	if (iter == 2) {
		fprintf(filpekG, "    \"max bow slamming p\":%.3lf, \"max green water p\":%.3lf,\n"
			"\"max dynamic instability\":%.3lf, \"max parametric rolling\":%.3lf, \"max surf-riding\":%.3lf,\n",
			model.functions.valuesNow.bowSlamming_max, model.functions.valuesNow.greenWater_max,
			model.functions.valuesNow.dynamicStability_max,
			model.functions.valuesNow.rolling_max, model.functions.valuesNow.surfRiding_max);

		sprintf(namn, "%s/resSafety_%s.csv", model.params.resultPath.c_str(), namnSol);
		FILE* filSafety = fopen(namn, "a+");
		double dist = distance;
		time_t rawtime5;
		time(&rawtime5);
		tm tmBas5 = *localtime(&rawtime5);
		fixReadableDate(tmBas5, namn);
		if (dist < 0.001)
			dist = 0.001;
		int sparaHeader = 0;
		if (sparaHeader == 1) {
			fprintf(filSafety, "caseID\tresID\thindCast\trunTime\tstartTime\tweightID\tweightTime\tweightFuel\tweightEmission\tweightsafetyBase\t"
				"weightBowSlam\tweightGreenWater\tweightDynamicStability\tweightParametricRolling\tweightSurfRiding\tweightStorm\t"
				"distTot_knots\tfuelTot_ton\ttimeTot_h\temissionTot\tvoyageDollarCost\tbowSlamming\tgreenWater\tdynStability\tparametricRolling"
				"\tsurfRiding\tobjCost\tobjAllSafety\t");
			fprintf(filSafety, "objBowSlamming\tobjGreenWater\tobjDynStability\tobjParametricRolling\tobjSurfRiding\t");
			fprintf(filSafety, "BowSlamming_notOKdist\tGreenWater_notOKdist\tDynStability_notOKdist\tParametricRolling_notOKdist\tSurfRiding_notOKdist\t"
				"highWave_notOKdist\thurricaneOuterCircle_dist\thurricaneOuterCircle_maxCost\thurricaneInnerCircle_dist\thurricaneInnerCircle_maxCost\t");
			fprintf(filSafety, "bowSlamMax\tbowSlamAver\tbowSlam_0\tbowSlam_01\tbowSlam_05\tbowSlam_2\t"
				"greenWaterMax\tgreenWaterAver\tgreenWater_0\tgreenWater_01\tgreenWater_05\tgreenWater_2\t"
				"dynamicStabilityMax\tdynamicStabilityAver\tdynamicStability_0\tdynamicStability_01\tdynamicStability_05\tdynamicStability_2\t"
				"parametricRollingMax\tparametricRollingAver\tparametricRolling_0\tparametricRolling_01\tparametricRolling_05\tparametricRolling_2\t"
				"surfRidingMax\tsurfRidingAver\tsurfRiding_0\tsurfRiding_01\tsurfRiding_05\tsurfRiding_2\t"
				"stormValueMax\tstormValueAver\n");
		}
		fprintf(filSafety, "%s\t%s\t%d\t%s\t%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t", model.params.indataPathName.c_str(),
			filename.c_str(), model.params.hindCast, namn,
			startTime0, model.params.weightID, model.params.weightTime, model.params.weightFuel,
			model.params.weightEmission, model.params.weightSafety.base,
			model.params.weightSafety.bowSlam,
			model.params.weightSafety.greenWater,
			model.params.weightSafety.dynamicStability, model.params.weightSafety.rolling, model.params.weightSafety.surfRiding,
			model.params.weightSafety.hurricane);
		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t", distance / model.params.knots_to_km,
			fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca, timeNu, emission,
			dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price,
			model.results.bowSlam_aver, model.results.greenWater_aver, model.results.dynamicStability_aver,
			model.results.rolling_aver, model.results.surfRiding_aver,
			totCost, totSafety * model.params.weightSafety.base);

		if (nArcsUsed == 0)
			nArcsUsed = 1;
		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.params.weightSafety.bowSlam * model.results.bowSlam_aver * model.params.weightSafety.base,
			model.params.weightSafety.greenWater * model.results.greenWater_aver * model.params.weightSafety.base,
			model.params.weightSafety.dynamicStability * model.results.dynamicStability_aver * model.params.weightSafety.base,
			model.params.weightSafety.rolling * model.results.rolling_aver * model.params.weightSafety.base,
			model.params.weightSafety.surfRiding * model.results.surfRiding_aver * model.params.weightSafety.base);

		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.results.bowSlam_notAllowed, model.results.greenWater_notAllowed, model.results.dynamicStability_notAllowed,
			model.results.rolling_notAllowed, model.results.surfRiding_notAllowed,
			model.results.maxWaveHeight_notAllowed,
			model.results.hurricane_insideOuterCircle, model.results.hurricane_maxCost_insideOuterCircle,
			model.results.hurricane_insideInnerCircle, model.results.hurricane_maxCost_insideInnerCircle);

		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.functions.valuesNow.bowSlamming_max, model.results.bowSlam_aver / nArcsUsed,
			model.results.bowSlam_0 / nArcsUsed, model.results.bowSlam_01 / nArcsUsed,
			model.results.bowSlam_05 / nArcsUsed, model.results.bowSlam_2 / nArcsUsed);
		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.functions.valuesNow.greenWater_max, model.results.greenWater_aver / nArcsUsed,
			model.results.greenWater_0 / nArcsUsed, model.results.greenWater_01 / nArcsUsed,
			model.results.greenWater_05 / nArcsUsed, model.results.greenWater_2 / nArcsUsed);
		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.functions.valuesNow.dynamicStability_max, model.results.dynamicStability_aver / nArcsUsed,
			model.results.dynamicStability_0 / nArcsUsed, model.results.dynamicStability_01 / nArcsUsed,
			model.results.dynamicStability_05 / nArcsUsed, model.results.dynamicStability_2 / nArcsUsed);
		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.functions.valuesNow.rolling_max, model.results.rolling_aver / nArcsUsed,
			model.results.rolling_0 / nArcsUsed, model.results.rolling_01 / nArcsUsed,
			model.results.rolling_05 / nArcsUsed, model.results.rolling_2 / nArcsUsed);
		fprintf(filSafety, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t",
			model.functions.valuesNow.surfRiding_max, model.results.surfRiding_aver / nArcsUsed,
			model.results.surfRiding_0 / nArcsUsed, model.results.surfRiding_01 / nArcsUsed,
			model.results.surfRiding_05 / nArcsUsed, model.results.surfRiding_2 / nArcsUsed);
		fprintf(filSafety, "%lf\t%lf\n",
			model.results.worstStormValue_max, model.results.stormValue_aver / nArcsUsed);
		fclose(filSafety);
	}
#endif


	if (model.functions.valuesNow.accumRPM_time > 0.01)
		fprintf(filpekG, "\"averageRPM\": %.2lf,\n", model.functions.valuesNow.accumRPM /
			model.functions.valuesNow.accumRPM_time);
	else
		fprintf(filpekG, "\"averageRPM\": %.2lf,\n", model.functions.valuesNow.accumRPM);

	fprintf(filpekG, "\"totalDistance_kts\": %.1lf, \"totalTime_h\": %.1lf,\n", distance / model.params.knots_to_km, timeNu);
	fprintf(filpekG, "\"totalSteamingTime_h\": %.1lf, \"totalAdditionalTime_h\": %.1lf,\n", timeNu - model.functions.valuesNow.totCorridorWaitingTime,
		model.functions.valuesNow.totCorridorWaitingTime); // timeNu);
	fprintf(filpekG, "\"prefPath_totalDist_nm\": %.1lf,\n", model.preferredPath.totDist / model.params.knots_to_km);

	tmBas.tm_min += (model.functions.valuesNow.maxWindSpeed_tp) * 60;
	test = mktime(&tmBas);
	fixReadableDate(tmBas, endTime);
	tmBas.tm_min -= (model.functions.valuesNow.maxWindSpeed_tp) * 60;
	fixDirectionLetters(model.functions.valuesNow.maxWindSpeed_dir, model.params.startTime, 1);
	fprintf(filpekG, "\"maxWindSpeed_kts\": %.2lf, \"maxWindSpeed_dateTime\":\"%s\", \"maxWindSpeed_dirLetters\":\"%s\",\n",
		model.functions.valuesNow.maxWindSpeed / model.params.knots_to_km, endTime, model.params.startTime);

	tmBas.tm_min += (model.functions.valuesNow.maxWaveHeight_tp) * 60;
	test = mktime(&tmBas);
	fixReadableDate(tmBas, endTime);
	tmBas.tm_min -= (model.functions.valuesNow.maxWaveHeight_tp) * 60;
	fixDirectionLetters(model.functions.valuesNow.maxWaveHeight_dir, model.params.startTime, 1);
	fprintf(filpekG, "\"maxWaveHeight\": %.2lf, \"maxWaveHeight_dateTime\":\"%s\", \"maxWaveHeight_dirLetters\":\"%s\",\n",
		model.functions.valuesNow.maxWaveHeight, endTime, model.params.startTime);

	fprintf(filpekG, "    \"favorableWindForecast_h\":%.3lf, \"notFavorableWindForecast_h\":%.3lf, \"noDataFavorableWind_h\":%.3lf\n",
		model.functions.valuesNow.favorableWind[0][2], model.functions.valuesNow.favorableWind[0][1],
		model.functions.valuesNow.favorableWind[0][0] + model.functions.valuesNow.favorableWind[1][0]);
	fprintf(filpekG, ",    \"favorableWindHistory_h\":%.3lf, \"notFavorableWindHistory_h\":%.3lf\n",
		model.functions.valuesNow.favorableWind[1][2], model.functions.valuesNow.favorableWind[1][1]);
	fprintf(filpekG, ",    \"favorableWaveForecast_h\":%.3lf, \"notFavorableWaveForecast_h\":%.3lf, \"noDataFavorableWave_h\":%.3lf\n",
		model.functions.valuesNow.favorableWave[0][2], model.functions.valuesNow.favorableWave[0][1],
		model.functions.valuesNow.favorableWave[0][0] + model.functions.valuesNow.favorableWave[1][0]);
	fprintf(filpekG, ",    \"favorableWaveHistory_h\":%.3lf, \"notFavorableWaveHistory_h\":%.3lf\n",
		model.functions.valuesNow.favorableWave[1][2], model.functions.valuesNow.favorableWave[1][1]);
	fprintf(filpekG, ",    \"favorableWindWaveForecast_h\":%.3lf, \"notFavorableWindWaveForecast_h\":%.3lf, \"noDataFavorableWindWave_h\":%.3lf\n",
		model.functions.valuesNow.favorableWindWave[0][2], model.functions.valuesNow.favorableWindWave[0][1],
		model.functions.valuesNow.favorableWindWave[0][0] + model.functions.valuesNow.favorableWindWave[1][0]);
	fprintf(filpekG, ",    \"favorableWindWaveHistory_h\":%.3lf, \"notFavorableWindWaveHistory_h\":%.3lf,\n",
		model.functions.valuesNow.favorableWindWave[1][2], model.functions.valuesNow.favorableWindWave[1][1]);

	if (model.params.useSimulering == 1) {
		fprintf(filpekG, "\"obj_fel_maxWindSpeed_kts\": %.2lf, \"obj_fel_maxWaveHeight\": %.2lf,\n",
			model.functions.valuesNow.obj_fel_maxWindSpeed, model.functions.valuesNow.obj_fel_maxWaveHeight);
	}
	if (model.functions.valuesNow.totTime_movingNoCorridors > 0.001)
		fprintf(filpekG, "\"consumptionTransit_average\": %.2lf, \"speedTransit_average\": %.2lf\n",
			model.functions.valuesNow.totFuel_mainMovingNoCorridors * 24 / model.functions.valuesNow.totTime_movingNoCorridors,
			model.functions.valuesNow.totDistance_movingNoCorridors / model.functions.valuesNow.totTime_movingNoCorridors / model.params.knots_to_km);
	else
		fprintf(filpekG, "\"consumptionTransit_average\": %.2lf, \"speedTransit_average\": %.2lf\n", 0.0, 0.0);

	if (model.results.fileForecast != NULL) {
		fprintf(model.results.fileForecast, "initTotDistStart;%lf;iterTotFuelStart;%lf;iterTotObjStart;%lf;"
			"iterTotDollarCostStart;%lf;iterStartTidp;%lf;iterStartTidpArc;%lf;iterWeatherFactorsStart;%lf;iterSafetyStart;%lf\n",
			iterTotDistStart, iterTotFuelStart, iterTotObjStart, iterTotDollarCostStart,
			iterStartTidp, iterStartTidpArc, iterWeatherFactorsStart, iterSafetyStart);

		fprintf(model.results.fileForecast, "totFuel;%lf;totTime;%lf;fuelCostDollar;%lf;timeCostDollar;%lf;"
			"totalDistance_kts;%lf;safety;%lf;objCost;%lf;totWeatherFactors;%lf\n",
			fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca
			- model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA
			- model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA
			- model.functions.valuesNow.totCorridorWaitingFuel_mainECA
			- model.functions.valuesNow.totCorridorWaitingFuel_auxECA,
			timeNu, fuelCostDollar, timeNu * model.params.priceTime,
			distance / model.params.knots_to_km, totSafety, totCost + model.results.iterTotObjStart,
			(model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
				model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km);
		fprintf(model.results.fileForecast, "weightTime;%lf;weightFuel;%lf;weightEmission;%lf;weightSafety;%lf\n",
			model.params.weightTime, model.params.weightFuel,
			model.params.weightEmission, model.params.weightSafety.base);
		fclose(model.results.fileForecast);
		model.results.fileForecast = NULL;

		sprintf(namn, "%s/iterTotalResults.txt", model.params.resultPath.c_str());
		FILE* filpekUt = fopen(namn, "a+");
		if (runAltForecast > 0 || strcmp("eval", namnSol) != 0)
			fprintf(filpekUt, "%d;%d;%d;%d;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n", model.iterKaoutar.physLevelStart, model.iterKaoutar.evalAlt,
				runAltForecast, model.results.forecastTypeOrig, (distance + model.results.iterTotDistStart) / model.params.knots_to_km, timeNu,
				fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca + model.results.iterTotFuelStart, totCost + model.results.iterTotObjStart,
				dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price + model.results.iterTotDollarCostStart -
				model.iterKaoutar.tidpStartIterArc_h * model.params.priceTime,
				(model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
					model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km + 
				model.results.iterWeatherFactorsStart, totSafety + model.results.iterSafetyStart);
		else
			fprintf(filpekUt, "%d;%d;%d;%d;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n", model.iterKaoutar.physLevelStart, model.iterKaoutar.evalAlt,
				runAltForecast, model.results.forecastTypeOrig, (distance) / model.params.knots_to_km, timeNu,
				fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca, totCost,
				dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price,
				(model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
					model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km, totSafety);
		fclose(filpekUt);
	}
	else {
		if (runAltForecast < 0 && iter == 2) {
			sprintf(namn, "%s/iterTotalResults.txt", model.params.resultPath.c_str());
			FILE* filpekUt = fopen(namn, "a+");
			fprintf(filpekUt, "%d;%d;%d;%d;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n", model.iterKaoutar.physLevelStart, model.iterKaoutar.evalAlt,
				runAltForecast, model.results.forecastTypeOrig, (distance + model.results.iterTotDistStart) / model.params.knots_to_km, timeNu,
				fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca + model.results.iterTotFuelStart, totCost + model.results.iterTotObjStart,
				dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price + model.results.iterTotDollarCostStart -
				model.iterKaoutar.tidpStartIterArc_h * model.params.priceTime,
				(model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
					model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km +
				model.results.iterWeatherFactorsStart, totSafety + model.results.iterSafetyStart);
			fclose(filpekUt);
		}
	}



	if (model.results.fileForecast2 != NULL) {
		fprintf(model.results.fileForecast2, ";%lf;%lf;%lf;%lf;%lf;%lf",
			model.results.fuel, model.results.time, model.results.fuelCost, model.results.timeCost,
			model.results.dist, model.results.safety);
		fprintf(model.results.fileForecast2, ";%lf;%lf;%lf;%lf;%lf;%lf\n",
			fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca
			- model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA
			- model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA
			- model.functions.valuesNow.totCorridorWaitingFuel_mainECA
			- model.functions.valuesNow.totCorridorWaitingFuel_auxECA,
			timeNu, fuelCostDollar, timeNu * model.params.priceTime,
			distance / model.params.knots_to_km, totSafety);
		fclose(model.results.fileForecast2);
		model.results.fileForecast2 = NULL;
	}



	if (iter == 1) {
		if (timeNu > 0) {
			model.functions.valuesNow.compare_fuelConsumption_ton = fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca
				- model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA
				- model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA
				- model.functions.valuesNow.totCorridorWaitingFuel_mainECA
				- model.functions.valuesNow.totCorridorWaitingFuel_auxECA;
			model.functions.valuesNow.compare_fuelEcaMain = fuel_eca - model.functions.valuesNow.totCorridorWaitingFuel_mainECA;
			model.functions.valuesNow.compare_fuelEcaAux = fuel_auxEca - model.functions.valuesNow.totCorridorWaitingFuel_auxECA;
			model.functions.valuesNow.compare_fuelNotEcaMain = fuel_noEca - model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA;
			model.functions.valuesNow.compare_fuelNotEcaAux = fuel_aux - model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA;
			model.functions.valuesNow.compare_emission = emission - emissionWaiting;
			model.functions.valuesNow.compare_emissionEcaMain = (fuel_eca - model.functions.valuesNow.totCorridorWaitingFuel_mainECA) * model.params.fuel.main_eca.emissionFactor;
			model.functions.valuesNow.compare_emissionEcaAux = (fuel_auxEca - model.functions.valuesNow.totCorridorWaitingFuel_auxECA) * model.params.fuel.aux_eca.emissionFactor;
			model.functions.valuesNow.compare_emissionNotEcaMain = (fuel_noEca - model.functions.valuesNow.totCorridorWaitingFuel_mainNonECA) * model.params.fuel.main_noEca.emissionFactor;
			model.functions.valuesNow.compare_emissionNotEcaAux = (fuel_aux - model.functions.valuesNow.totCorridorWaitingFuel_auxNonECA) * model.params.fuel.aux_noEca.emissionFactor;
			model.functions.valuesNow.compare_averSpeed = averSpeed;
			model.functions.valuesNow.compare_totalDistance_kts = distance / model.params.knots_to_km;
			model.functions.valuesNow.compare_totalTime_h = timeNu;
			model.functions.valuesNow.compare_dollar_cost = dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price;
		}
	}
	else {
		if (model.functions.valuesNow.compare_totalTime_h > 0) {
			fprintf(filpekG, ", \"compare_fuelConsumption_ton\": %.2lf\n", model.functions.valuesNow.compare_fuelConsumption_ton);
			fprintf(filpekG, ", \"compare_fuelEcaMain\": %.2lf\n", model.functions.valuesNow.compare_fuelEcaMain);
			fprintf(filpekG, ", \"compare_fuelEcaAux\": %.2lf\n", model.functions.valuesNow.compare_fuelEcaAux);
			fprintf(filpekG, ", \"compare_fuelNotEcaMain\": %.2lf\n", model.functions.valuesNow.compare_fuelNotEcaMain);
			fprintf(filpekG, ", \"compare_fuelNotEcaAux\": %.2lf\n", model.functions.valuesNow.compare_fuelNotEcaAux);
			fprintf(filpekG, ", \"compare_emission\": %.2lf\n", model.functions.valuesNow.compare_emission);
			fprintf(filpekG, ", \"compare_emissionEcaMain\": %.2lf\n", model.functions.valuesNow.compare_emissionEcaMain);
			fprintf(filpekG, ", \"compare_emissionEcaAux\": %.2lf\n", model.functions.valuesNow.compare_emissionEcaAux);
			fprintf(filpekG, ", \"compare_emissionNotEcaMain\": %.2lf\n", model.functions.valuesNow.compare_emissionNotEcaMain);
			fprintf(filpekG, ", \"compare_emissionNotEcaAux\": %.2lf\n", model.functions.valuesNow.compare_emissionNotEcaAux);
			fprintf(filpekG, ", \"compare_average speed\": %.2lf\n", model.functions.valuesNow.compare_averSpeed);
			fprintf(filpekG, ", \"compare_totalDistance_kts\": %.1lf\n", model.functions.valuesNow.compare_totalDistance_kts);
			fprintf(filpekG, ", \"compare_totalTime_h\": %.1lf\n", model.functions.valuesNow.compare_totalTime_h);
			fprintf(filpekG, ", \"compare_dollar_cost\": %.0lf\n", model.functions.valuesNow.compare_dollar_cost);

			tmBas.tm_min += (model.functions.valuesNow.compare_totalTime_h) * 60;
			test = mktime(&tmBas);
			if (test == -1) {
				printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
					tmBas.tm_year,
					tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
				if (model.params.failedTime == 0)
					postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
				model.params.failedTime = 1;
			}
			fixReadableDate(tmBas, endTime);

			fprintf(filpekG, ", \"compare_route_endTime\":\"%s\"\n", endTime);
		}
	}
	fprintf(filpekG, "}}\n");

	if (iter != 1)
		fprintf(filpekG, "]}\n");




	//fprintf(filpekG, "]\n}");

	free(startTime);
	free(startTime0);
	free(x);
	free(y);

	fclose(filpekG);

	if (SKRIV_UT_NOTHING == 0) {
		fprintf(filPek, "total\tcombined\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
			distance, timeNu, fuel, emission, totSafety, channelCost, totCost);
		fprintf(filPek, "\nobj_weights\ntime\tfuel\tsafety\n%lf\t%lf\t%lf\n",
			model.params.weightTime, model.params.weightFuel,
			model.params.weightSafety.base);
	}

	if (SPARA_RUN_DATA && iter == 2) {
		sprintf(namn, "%s/res_runSummary.txt", model.params.indataPath.c_str());
		filRun = fopen(namn, "a+");
		//fprintf(filRun, "dateTime\tcaseID\tdollar_cost\tfuel_dollarCost\tvoyageTime_dollarCost\t"
		//	"averageSpeed\tsafety\tchannelCost\ttotalObjective\teta error cost\t"
		//"WindF\tWaveF\tCurrentF\tDelayF\tallWeatherFactors\t"
		//"fuelConsumptionExtra_ton\temissionExtra\ttotalDistance_kts\ttotalTime_h\t"
		//"totalSteamingTime_h\ttotalAdditionalTime_h\tprefPath_totalDist_nm\t"
		//"maxWindSpeed_kts\tmaxWaveHeight\tconsumptionTransit_average\tspeedTransit_average\n");

		time_t timestamp = time(&timestamp);
		struct tm tmBas5 = *localtime(&timestamp);
		fixReadableDate(tmBas5, namn);
		fprintf(filRun, "%s\t%s\t", namn, filename.c_str());
		fprintf(filRun, "%.2lf\t%.2lf\t%.2lf\t",
			dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price,
			fuel_eca* model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
			fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price,
			timeNu* model.params.priceTime);
		fprintf(filRun, "%.2lf\t%.2lf\t%.2lf\t%.2lf\t%.2lf\t",
			averSpeed, totSafety, channelCost, totCost, earlyEtaCost + lateEtaCost);
		fprintf(filRun, "%.2lf\t%.2lf\t%.2lf\t%.2lf\t%.2lf\t",
			model.functions.valuesNow.totWindF / model.params.knots_to_km,
			model.functions.valuesNow.totWaveF / model.params.knots_to_km,
			model.functions.valuesNow.totCurrentF / model.params.knots_to_km,
			model.functions.valuesNow.totDelayF / model.params.knots_to_km,
			(model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
				model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km);

		fprintf(filRun, "%.2lf\t%.2lf\t%.2lf\t%.2lf\t", model.params.fuel.extra_fuel.quantity,
			model.params.fuel.extra_fuel.quantity* model.params.fuel.extra_fuel.emissionFactor,
			distance / model.params.knots_to_km, timeNu);
		
		fprintf(filRun, "%.2lf\t%.2lf\t%.2lf\t", timeNu - model.functions.valuesNow.totCorridorWaitingTime,
			model.functions.valuesNow.totCorridorWaitingTime,
			model.preferredPath.totDist / model.params.knots_to_km);
		fprintf(filRun, "%.2lf\t%.2lf\t", model.functions.valuesNow.maxWindSpeed / model.params.knots_to_km,
			model.functions.valuesNow.maxWaveHeight);
		if (model.functions.valuesNow.totTime_movingNoCorridors > 0.001)
			fprintf(filRun, "%.2lf\t%.2lf\n",
				model.functions.valuesNow.totFuel_mainMovingNoCorridors * 24 / model.functions.valuesNow.totTime_movingNoCorridors,
				model.functions.valuesNow.totDistance_movingNoCorridors / model.functions.valuesNow.totTime_movingNoCorridors / model.params.knots_to_km);
		else
			fprintf(filRun, "%.2lf\t%.2lf\n", 0.0, 0.0);
		fclose(filRun);
	}
	free(endTime);

	//printf("\nobj_weights\ntime\tfuel\tsafety\n%.2lf\t%.2lf\t%.2lf\n",
	//	model.params.weightTime, model.params.weightFuel.base,
	//	model.params.weightSafety.base);
	//printf("\nobj_weights\ntime\t\t%.2lf\tcost_h\t%.2lf\nfuel eca\t%.2lf\tcost_ton\t%.2lf\nfuel non eca\t%.2lf\tcost_ton\t%.2lf\n",
	//	model.params.weightTime, model.params.priceTime, 
	//	model.params.weightFuel, model.params.fuel.main_eca.price,
	//	model.params.weightFuel, model.params.fuel.main_noEca.price);
	//printf("hurricane\t%.2lf\nbowSlamming\t%.2lf\ngreenWater\t%.2lf\ndynamicStability\t\t%.2lf\n",
	//	model.params.weightSafety.hurricane, model.params.weightSafety.bowSlam,
	//	model.params.weightSafety.greenWater, model.params.weightSafety.dynamicStability);
	//printf("feasibleSafety\t%.2lf\niceCoverCost_fix\t%.2lf\niceCoverCost_thickness\t%.2lf\n",
	//	model.params.weightSafety.feasibleSafety, model.params.weightSafety.iceCoverCost_fix,
	//	model.params.weightSafety.iceCoverCost_thickness);
	printf("dist\t%.2lf\ntime\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\n"
		"fuel\t%.2lf\teca\t%.2lf\tnonEca\t%.2lf\tcost\t%.2lf\t"
		"obj\t%.2lf\n"
		"emission\t%.2lf\tcost\t%.2lf\tobj\t%.2lf\n"
		"channelCost\t%.2lf\nsafety\t%.2lf\tobj\t%.2lf\n",
		distance, timeNu, model.params.priceTime* timeNu, model.params.weightTime* model.params.priceTime* timeNu,
		fuel_aux + fuel_auxEca + fuel_eca + fuel_noEca, fuel_auxEca + fuel_eca, fuel_aux + fuel_noEca,
		fuel_aux* model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price + fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price,
		model.params.weightFuel* (fuel_aux* model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
			fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price),
		emission, emission* model.params.scaleObjEmission, emission_objCost,
		channelCost, totSafety, model.params.weightSafety.base * totSafety);
#ifdef NAZANIN_SAFETY
	printf("    hurricane %.3lf\n    feasibleSafety %.3lf\n"
		"    ice %.3lf\n    maxWaveHeightCost %.3lf\n",
		totHurricane, totFeasSafety, totIce, totMaxWaveHeight);
	printf("    bow slamming %.3lf max %.3lf\n    green water %.3lf max %.3lf\n    "
		"dynamic instability %.3lf max %.3lf\n    parametric rolling %.3lf max %.3lf\n    surf-riding %.3lf max %.3lf\n",
		totBowSlam, model.functions.valuesNow.bowSlamming_max, 
		totGreenWater, model.functions.valuesNow.greenWater_max,
		totDynStab, model.functions.valuesNow.dynamicStability_max,
		totRolling, model.functions.valuesNow.rolling_max, 
		totSurfRiding, model.functions.valuesNow.surfRiding_max);
#endif

	if (SKRIV_UT_NOTHING == 0) {
		for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
			if (nSpeedSettingUsed[i] > 0) {
				fprintf(filPek, "used rpm_setting %.2lf knots %.2lf %d times\n",
					model.functions.rpmBase[i], model.functions.rpmSetting_gerCalmWaterSpeedBase[i], nSpeedSettingUsed[i]);
				printf("used rpm_setting %.2lf knots %.2lf %d times\n", model.functions.rpmBase[i],
					model.functions.rpmSetting_gerCalmWaterSpeedBase[i], nSpeedSettingUsed[i]);
			}
		}
		printf("The speed settings are changed %d times during the trip\n", nSpeedChanges);

		fprintf(filPek, "nSpeedChange\t%d\n", nSpeedChanges);
		fprintf(filPek, "tot fuel (ton)\t%.3lf\n", fuel_eca + fuel_noEca + fuel_aux + fuel_auxEca);
		fprintf(filPek, "tot time (h)\t%.3lf\n", timeNu);
		fprintf(filPek, "tot distance (nautical miles)\t%.3lf\n", distance / model.params.knots_to_km);
		fprintf(filPek, "fuel cost (dollar)\t%.3lf\n", fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price +
			fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price);
		fprintf(filPek, "time cost (dollar)\t%.3lf\n", timeNu * model.params.priceTime);
		fprintf(filPek, "total cost (dollar)\t%.3lf\n", dollarCost + model.params.fuel.extra_fuel.quantity * model.params.fuel.extra_fuel.price);
		fprintf(filPek, "delay\t%.3lf\n", (model.functions.valuesNow.totWindF + model.functions.valuesNow.totWaveF +
			model.functions.valuesNow.totCurrentF + model.functions.valuesNow.totDelayF) / model.params.knots_to_km);
		if (model.functions.valuesNow.totTime_movingNoCorridors < 0.001)
			model.functions.valuesNow.totTime_movingNoCorridors = 0.001;
		fprintf(filPek, "averageConsumption_transit (ton/24h)\t%.3lf\n", model.functions.valuesNow.totFuel_mainMovingNoCorridors * 24 / model.functions.valuesNow.totTime_movingNoCorridors);
		fprintf(filPek, "averageSpeedSOG_transit (knots)\t%.3lf\n",
			model.functions.valuesNow.totDistance_movingNoCorridors / model.functions.valuesNow.totTime_movingNoCorridors / model.params.knots_to_km);
		fprintf(filPek, "averageSpeedOnWater_transit (knots)\t%.3lf\n",
			model.functions.valuesNow.sumSpeedOnWater / model.functions.valuesNow.totTime_movingNoCorridors / model.params.knots_to_km);
		if (timeNu < 0.001)
			timeNu = 0.001;
		fprintf(filPek, "averageWindSpeed (m/s)\t%.3lf\n", model.functions.valuesNow.sumWindSpeed / timeNu / 3.6);
		fprintf(filPek, "averageRelCurrent (knots)\t%.3lf\n", model.functions.valuesNow.sumRelCurrent / timeNu / 1.852);
		fprintf(filPek, "averageCurrent (knots)\t%.3lf\n", model.functions.valuesNow.sumCurrent / timeNu / 1.852);
		fprintf(filPek, "averageWaveHight (m)\t%.3lf\n", model.functions.valuesNow.sumWaveHeight / timeNu);
		fprintf(filPek, "maxWindSpeed (m/s)\t%.3lf\n", model.functions.valuesNow.maxWindSpeed / 3.6);
		fprintf(filPek, "maxCurrent (knots)\t%.3lf\n", model.functions.valuesNow.maxCurrent / 1.852);
		fprintf(filPek, "maxWaveHight (m)\t%.3lf\n", model.functions.valuesNow.maxWaveHeight);

		fclose(filPek);
		fclose(filPek2);
	}

	free(nSpeedSettingUsed);
	return 0;
}

void identify_hindCastMonths(tm tmEnd, int alt) {
	int endYear, endMonth, startYear, startMonth, i, i1, pos;
	int startMonthUse, endMonthUse;

	endYear = tmEnd.tm_year + 1900;
	endMonth = tmEnd.tm_mon + 1;
	startYear = model.params.startYear;
	startMonth = model.params.startMonth_nr;
	errlog("hindCastMonths start end: %d %d %d %d end day hour min %d %d %d\n", startYear, startMonth, endYear, endMonth,
		tmEnd.tm_mday, tmEnd.tm_hour, tmEnd.tm_min);

	model.params.nHindCastMonths = endMonth - startMonth + 1 + 12 * (endYear - startYear);
	model.params.hindCast_month = (int*)malloc(model.params.nHindCastMonths * sizeof(int));
	model.params.hindCast_year = (int*)malloc(model.params.nHindCastMonths * sizeof(int));
	model.params.hindCast_yearSecDiff = (int*)malloc(model.params.nHindCastMonths * sizeof(int));

	pos = 0;
	for (i = startYear; i <= endYear; i++) {
		if (i < endYear)
			endMonthUse = 12;
		else
			endMonthUse = endMonth;
		if (i == startYear)
			startMonthUse = startMonth;
		else
			startMonthUse = 1;
		for (i1 = startMonthUse; i1 <= endMonthUse && i1 <= 12; i1++) {
			model.params.hindCast_month[pos] = i1;
			if (alt == 0) {
				model.params.hindCast_year[pos] = i;
				model.params.hindCast_yearSecDiff[pos] = 0;
			}
			else {
				model.params.hindCast_year[pos] = 2022;
				model.params.hindCast_yearSecDiff[pos] = (i - 2022) * 3600 * 24 * 365;
				if (i > 2024 || (i == 2024 && i1 > 2))
					model.params.hindCast_yearSecDiff[pos] += 3600 * 24;
				if (i > 2028 || (i == 2028 && i1 > 2))
					model.params.hindCast_yearSecDiff[pos] += 3600 * 24;
				if (i > 2032 || (i == 2032 && i1 > 2))
					model.params.hindCast_yearSecDiff[pos] += 3600 * 24;
			}
			pos++;
		}
	}
	if (pos != model.params.nHindCastMonths)
		errlog("ERROR! Wrong number of hindCast months set. Should be %d but is %d\n",
			model.params.nHindCastMonths, pos);


}

long long make_gmtime_fromDateTimeStringForecast(std::string tidpkt) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	int hour, nHoursDiffTZ = 0, nMinDiffTZ = 0, nValuesHour;

	tmBas.tm_year = std::stoi(tidpkt.substr(0, 4)) - 1900;
	tmBas.tm_mon = std::stoi(tidpkt.substr(4, 2)) - 1; // sep
	tmBas.tm_mday = std::stoi(tidpkt.substr(6, 2));
	tmBas.tm_hour = std::stoi(tidpkt.substr(8, 2));
	tmBas.tm_min = std::stoi(tidpkt.substr(10, 2));
	tmBas.tm_sec = std::stoi(tidpkt.substr(12, 2));

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
	//printf("time %s", tidpkt.c_str());
	//printf("\n");
	time_t rawtime = _mkgmtime(&tmBas);
#endif
#ifndef _WIN32
	time_t rawtime = timegm(&tmBas);
#endif
	return rawtime;
}

int identifyUsableFilDirsForecast() {
	long long startTimeUse = model.params.UTC_secondsStart + model.iterKaoutar.startTimeUse * 3600;
#ifdef KAOUTAR
	int i;
	std::string path_name;
	std::set<fs::path> sorted_by_name;
	std::string namn, namn2;
	int found;
	long long UTCsec;
	double timeDiff;
	int nAlloc = 40, nData = 0, presentPos = 0;
	model.filDirsForecast.filPath = (char**)malloc(nAlloc * sizeof(char*));

	for (i = 0; i < 3; i++) {
		if (i == 0)
			path_name = model.params.weatherPath + "forecastData/202312/";
		else {
			sorted_by_name.clear();
			if (i == 1)
				path_name = model.params.weatherPath + "forecastData/202401/";
			if (i == 2)
				path_name = model.params.weatherPath + "forecastData/202402/";
		}
		//--- filenames are unique so we can use a set

		for (auto& entry : fs::directory_iterator(path_name))
			sorted_by_name.insert(entry.path());

		for (auto& filename : sorted_by_name) {
			//std::cout << filename << std::endl;
			namn = filename.u8string();
			found = namn.find_last_of("/");
			namn2 = namn.substr(found + 1);
			//printf("'%s'\t'%s'\n", namn.c_str(), namn2.c_str());

			UTCsec = make_gmtime_fromDateTimeStringForecast(namn2);
			timeDiff = (UTCsec - startTimeUse) / 3600.0;
			if (timeDiff > 0)
				break; // too late so stop using the dates from here
			if (timeDiff > -24 * 3) { // no reason to use forecast earlier than 3 days ago
				if (nData >= nAlloc) {
					nAlloc += 20;
					model.filDirsForecast.filPath = (char**)realloc(model.filDirsForecast.filPath, nAlloc * sizeof(char*));
				}
				model.filDirsForecast.filPath[nData] = (char*)malloc((namn.length() + 1) * sizeof(char));
				strcpy(model.filDirsForecast.filPath[nData], namn.c_str());
				if (timeDiff <= 0) {
					presentPos = nData;
					//printf("%s\n", namn2.c_str());
				}
				nData++;
			}
			//if (timeDiff > 0)
			//	break; // too late so stop using the dates from here
		}
		if (timeDiff > 0)
			break;
	}
	model.filDirsForecast.presentPos = presentPos; // nData - 1;
#endif

	return 0;
}

int identCorrectForecastDir(int weatherNr, char* namn) {

	for (int i = model.filDirsForecast.presentPos; i >= 0; i--) {
		sprintf(namn, "%s/%s.grb", model.filDirsForecast.filPath[i],
			model.weather[weatherNr].filePos[0].fileName); // is i1 needed?
		if (check_file_exist(namn)) {
			return i;
		}
	}

	errlog("ERROR! No forecast directory found for weather %d. I quit!\n", weatherNr);
	postRequest("ERROR! No forecast directory found", 1);

	return 0;
}


int loadForecastWeatherFile_grib_kaoutar(int ii, int* nMaxTimeInt, char* namn, long long endTime_secondsUTC) {
	int xPos0, xPos1, yPos0, yPos1, nBands;
	int pos2, pos3, i4, i5, nAlloc, i3, offset_x, offset_y;
	int nCols, nRows, nRows_inBlock, nCols_inBlock;
	int yStartBlock, yStartValue, nY_valueAdd, yEndBlock;
	int xStartBlock, xStartValue, nX_valueAdd, xEndBlock, i;
	int xBlockNr, xBlockUse, yBlockNr, pos, latPos, lonPos, tidInt;
	int nTimeIntervals_forecast_redis, nTimeIntervals_redis, forstaOverT, startT;
	int nDefault, nNoll, nTot, nBandsAlloc, nSecondsUTC, nBandsNu;
	int nAllocTmp = 0, initFile = 0;
	long long* secondsUTC_tmp = NULL;
	int startPos, endPos;

	int tidpHistoricalWeather;
	int posNu, savePosStart, i10;

	long long maxTid, sekNu, offset_x2;
	double min_lon, max_lon, min_lat, max_lat, min_lonUse, max_lonUse;
	double xPosFrac0, xPosFrac1, yPosFrac0, yPosFrac1;
	//float* arrFloat;


	double filKvot;
	int manad2, returnVal, first_i1, nBandsTooLate;
	double size_col, size_row, xPosFrac, yPosFrac, lat, lon;
	long long nSecondsUTC_next;

	model.tmpTid[0] = std::chrono::high_resolution_clock::now();

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
	nBandsTooLate = 0;
	first_i1 = 1;

	int filPathPos = identCorrectForecastDir(ii, namn);
	if (filPathPos < 0) {
		errlog("ERROR! Could not find a weather forecast for weather %d. I quit!\n", ii);
		exitKontrollerat(__LINE__);
	}
	for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
		posNu = 0;
		savePosStart = 0;
		if (i1 > 0)
			sprintf(namn, "%s/%s.grb", model.filDirsForecast.filPath[filPathPos],
				model.weather[ii].filePos[i1].fileName);
		printf("%s\n", namn);

		returnVal = model.weather[ii].rasterPos[i1].open(namn);
		if (returnVal == -1) {
			errlog("ERROR! Failed to open weather file %s. I quit!\n", namn);
			postRequest("ERROR! Failed to open weather file " + std::string(namn) + ".Fix it and run OptiNav kaoutar forecast.", 1);
		}
		model.weather[ii].filePos[i1].minX = model.weather[ii].rasterPos[i1].Get_minLongitude();
		model.weather[ii].filePos[i1].maxX = model.weather[ii].rasterPos[i1].Get_maxLongitude();
		if (check_useRaster_longitude(ii, i1) == 0)
			continue;

		if (first_i1 >= 1) {
			first_i1 = 2;
			if (size_col < -0.1) {
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
				endPos = -1;
				for (int i2 = 0; i2 < nBands; i2++) {
					if (i2 > 0)
						nSecondsUTC = nSecondsUTC_next;
					else
						nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);

					if (i2 + 1 < nBands) {
						nSecondsUTC_next = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 2);
						//if (nSecondsUTC_next < model.params.UTC_secondsStart - model.params.hindCast_yearSecDiff[i10])
						//	continue; // too early
					}

					if (nSecondsUTC >= endTime_secondsUTC) {//  - model.params.hindCast_yearSecDiff[i10])
						nBandsTooLate++;
						break;
					}
					//if (nBandsTooLate > 1) {
					//	break; // too late, do not include any more bands
					//}
					endPos = i2;

					if (posNu == 0) {
						startPos = i2;
						nBandsAlloc = (int)(nBands - i2);
						model.weather[ii].secondsUTC = (long long*)malloc2(nBandsAlloc * sizeof(long long));
						if (nBandsAlloc > nAllocTmp) {
							if (nAllocTmp == 0) {
								nAllocTmp = nBandsAlloc;
								secondsUTC_tmp = (long long*)malloc(nAllocTmp * sizeof(long long));
							}
							else {
								nAllocTmp = nBandsAlloc;
								secondsUTC_tmp = (long long*)realloc(secondsUTC_tmp, nAllocTmp * sizeof(long long));
							}
						}
						model.weather[ii].valueCell = (float**)malloc2(nBandsAlloc * sizeof(float*));
						//for (int i2 = 0; i2 < nBandsAlloc; i2++) {
						//	model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
						//	for (int i3 = 0; i3 < nAlloc; i3++)
						//		model.weather[ii].valueCell[posNu][i3] = 9999;
						//}
					}
					model.weather[ii].secondsUTC[posNu] = nSecondsUTC;
					secondsUTC_tmp[posNu] = nSecondsUTC; //  +model.params.hindCast_yearSecDiff[i10];

					posNu++;
				}
				model.weather[ii].nTimeIntervals = posNu;
				model.weather[ii].nTimeIntervals_forecast = posNu;
			}
			else {
				size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
				if (abs(model.weather[ii].size_col - size_col) > 0.0001)
					errlog("ERROR! Wrong size_col %.5lf vs %.5lf for weather data %s ii %d i1 %d\n",
						size_col, model.weather[ii].size_col, namn, ii, i1);
				xPosFrac = (model.boundingBox.xMin - model.weather[ii].rasterPos[i1].Get_minLongitude()) / size_col;
				xPos0 = roundDown(xPosFrac);
				if (abs(model.weather[ii].minX - model.weather[ii].rasterPos[i1].Get_minLongitude() -
					xPos0 * size_col) > 0.0001)
					errlog("ERROR! Wrong minX %.5lf vs %.5lf for weather data %s ii %d i1 %d\n",
						model.weather[ii].rasterPos[i1].Get_minLongitude() + xPos0 * size_col,
						model.weather[ii].minX, namn, ii, i1);
				xPosFrac = (model.boundingBox.xMax - model.weather[ii].minX) /
					size_col;

				xPos1 = roundUp(xPosFrac);
				if (abs(model.weather[ii].maxX - model.weather[ii].minX - xPos1 * size_col) > 0.0001)
					errlog("ERROR! Wrong maxX %.5lf vs %.5lf for weather data %s ii %d i1 %d\n",
						model.weather[ii].minX + xPos1 * size_col,
						model.weather[ii].maxX, namn, ii, i1);
				if (model.weather[ii].nCols != xPos1 + 1)
					errlog("ERROR! Wrong nCols %d vs %d for weather data %s ii %d i1 %d\n",
						xPos1 + 1, model.weather[ii].nCols, namn, ii, i1);
				//model.weather[ii].nCols = xPos1 + 1;

				size_row = model.weather[ii].rasterPos[i1].Get_sizeRow();
				if (abs(model.weather[ii].size_row - size_row) > 0.0001)
					errlog("ERROR! Wrong size_row %.5lf vs %.5lf for weather data %s ii %d i1 %d\n",
						size_row, model.weather[ii].size_row, namn, ii, i1);
				yPosFrac = (model.weather[ii].rasterPos[i1].Get_maxLatitude() - model.boundingBox.yMax) /
					size_row;
				yPos0 = roundDown(yPosFrac);
				if (abs(model.weather[ii].maxY - model.weather[ii].rasterPos[i1].Get_maxLatitude() +
					yPos0 * size_row) > 0.0001)
					errlog("ERROR! Wrong maxY %.5lf vs %.5lf for weather data %s ii %d i1 %d\n",
						model.weather[ii].rasterPos[i1].Get_maxLatitude() -
						yPos0 * size_row, model.weather[ii].maxY, namn, ii, i1);
				yPosFrac = (model.weather[ii].maxY - model.boundingBox.yMin) /
					size_row;
				yPos1 = roundUp(yPosFrac);
				if (yPos1 >= model.weather[ii].rasterPos[i1].Get_nRows())
					yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
				if (abs(model.weather[ii].minY - model.weather[ii].maxY +
					yPos1 * size_row) > 0.0001)
					errlog("ERROR! Wrong maxY %.5lf vs %.5lf for weather data %s ii %d i1 %d\n",
						model.weather[ii].maxY -
						yPos1 * size_row, model.weather[ii].minY, namn, ii, i1);
				//model.weather[ii].minY = model.weather[ii].maxY -
				//	yPos1 * size_row;
				if (model.weather[ii].nRows != yPos1 + 1)
					errlog("ERROR! Wrong nRows %d vs %d for weather data %s ii %d i1 %d\n",
						yPos1 + 1, model.weather[ii].nRows, namn, ii, i1);
				//model.weather[ii].nRows = yPos1 + 1;



				if (ii == 3)
					ii = ii;
				nBands = model.weather[ii].rasterPos[i1].Get_nBands();

				nBandsAlloc += nBands;
				model.weather[ii].nTimeIntervals = nBandsAlloc;
				model.weather[ii].nTimeIntervals_forecast = nBandsAlloc;
				model.weather[ii].secondsUTC = (long long*)realloc(model.weather[ii].secondsUTC, nBandsAlloc * sizeof(long long));
				nAllocTmp = nBandsAlloc;
				secondsUTC_tmp = (long long*)realloc(secondsUTC_tmp, nAllocTmp * sizeof(long long));

				model.weather[ii].valueCell = (float**)realloc(model.weather[ii].valueCell, nBandsAlloc * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;


				startPos = 0;
				endPos = -1;
				for (int i2 = 0; i2 < nBands; i2++) {
					nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
					if (nSecondsUTC >= endTime_secondsUTC) {
						nBandsTooLate++;
						break;
					}
					//if (nBandsTooLate > 1)
					//	break; // too late, do not include any more bands
					//endPos = i2;

					model.weather[ii].secondsUTC[posNu] = nSecondsUTC;
					secondsUTC_tmp[posNu] = nSecondsUTC;
					posNu++;
				}
				model.weather[ii].nTimeIntervals = posNu;
				model.weather[ii].nTimeIntervals_forecast = posNu;
			}
		}
		else {
			if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
				errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
					model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
			if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
				errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
					model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());

			errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
				ii, model.weather[ii].nTimeIntervals,
				model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);
			nBands = model.weather[ii].rasterPos[i1].Get_nBands();
		}
		if (ii == 2 && i1 == 1)
			ii = ii;

		nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
		if (initFile == 0) {
			for (posNu = savePosStart; posNu <= savePosStart + endPos - startPos; posNu++) {
				model.weather[ii].valueCell[posNu] = (float*)malloc2(nAlloc * sizeof(float));
				for (int i3 = 0; i3 < nAlloc; i3++)
					model.weather[ii].valueCell[posNu][i3] = 9999;
			}
			initFile = 1;
		}

		if (ii == 2)
			ii = ii;
		if (endPos >= nBands)
			postRequest("ERROR! To few bands in fil " + std::string(namn) + " must be the same as in the first one. I quit!", 1);

		model.weather[ii].rasterPos[i1].GetRasterValues_realHindCastBands(&(model.weather[ii]), startPos, endPos, savePosStart, filKvot);
		savePosStart += endPos - startPos + 1;
		//if (i1 < model.weather[ii].nFiles / 2.0)
		//	model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands_fixBandNr(&(model.weather[ii]), 0, nBandsAlloc);
		//else
		//	model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands_fixBandNr(&(model.weather[ii]), nBands, nBandsAlloc);

		if (first_i1 == 2)
			first_i1 = 0;



		//testCoordValue(ii, -41.43, -26.44);
		//testCoordValue(ii, -40.77, -26.76);
		// testCoordValue(ii, 93.84, 5.98);
		//testCoordValue(ii, 167.6111, 81);
		//testCoordValue(ii, 179.6111, 82);
		//testCoordValue(ii, -179.6111, 83);
		//testCoordValue(ii, -132.39, 84);


		// om olika diskretization pa oppnade raster sa stoppa
		// 



		//model.weather[ii].valueCell = model.weather[ii].rasterPos.GetRasterBand_realArrAllBands(&(model.weather[ii].raster), model.boundingBox);
		//printf("used dim %d %d tid %lf nBands %d\n", model.weather[ii].nRows,
		//	model.weather[ii].nCols, model.durationMilli[ii], model.weather[ii].nTimeIntervals);





		//errlog("iicc %d ii2 %d\n", ii, ii2);
		(model.nCallsWeatherBand[ii])++;
		//errlog("weather %d variable %s nTimeInt %d dim %d %d tid %lf\nminLon %.3lf maxLon %.3lf\nminLat %.3lf maxLat %.3lf\n", ii,
		//	model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals,
		//	model.weather[ii].nRows,
		//	model.weather[ii].nCols,
		//	model.durationMilli[ii],
		//	model.weather[ii].minX, model.weather[ii].maxX,
		//	model.weather[ii].minY, model.weather[ii].maxY);

	}
	if (ii == 2)
		ii = ii;
	// testCoordValue(ii, 25.675, -33.917);


	if (nAllocTmp > 0) {
		for (int i2 = 0; i2 < model.weather[ii].nTimeIntervals; i2++) {
			model.weather[ii].secondsUTC[i2] = secondsUTC_tmp[i2];
		}
		free(secondsUTC_tmp);
	}

	nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
	model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

	errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
		ii, model.weather[ii].nTimeIntervals,
		model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);

	tidInt = 0;
	tidpHistoricalWeather = -1;
	for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

		if (i == model.weather[ii].nTimeIntervals - 1)
			maxTid = model.weather[ii].secondsUTC[i] + 3600 - 1;
		else
			maxTid = (long long)(0.5 * (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]));

		for (; tidInt < 100000; tidInt++) {
			if (i >= model.weather[ii].nTimeIntervals_forecast && tidpHistoricalWeather == -1)
				tidpHistoricalWeather = tidInt;
			if (tidInt >= nAlloc) {
				printf("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d tidInt %d maxTid %I64d\n", tidInt, nAlloc, ii,
					model.weather[ii].weatherFileTypeName, i, tidInt, maxTid);
				errlog("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d tidInt %d maxTid %I64d\n", tidInt, nAlloc, ii,
					model.weather[ii].weatherFileTypeName, i, tidInt, maxTid);
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
		if (model.weather[ii].nTimeIntervals_maxValue > (*nMaxTimeInt))
			*nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;
	}
	if (tidpHistoricalWeather == -1)
		tidpHistoricalWeather = tidInt + 1;

	if (tidpHistoricalWeather * model.weather_timeIntervall_h > model.network.tidp_startHistoricDataOnly)
		model.network.tidp_startHistoricDataOnly = tidpHistoricalWeather * model.weather_timeIntervall_h;

	if (model.weather[ii].nTimeIntervals_maxValue < 0)
		model.weather[ii].nTimeIntervals_maxValue = 0;

	//testCoordValue(ii, 141.639, -11.240);
	//testCoordValue(ii, 149.315, -21.275);

	return 0;

}

int loadStaticWeatherFile_grib(int ii, int* nMaxTimeInt, int* initFile, char* namn, int* startPos, int* endPos, long long endTime_secondsUTC) {
	int xPos0, xPos1, yPos0, yPos1, nBands;
	int pos2, pos3, i4, i5, nAlloc, i3, offset_x, offset_y;
	int nCols, nRows, nRows_inBlock, nCols_inBlock;
	int yStartBlock, yStartValue, nY_valueAdd, yEndBlock;
	int xStartBlock, xStartValue, nX_valueAdd, xEndBlock, i;
	int xBlockNr, xBlockUse, yBlockNr, pos, latPos, lonPos, tidInt;
	int nTimeIntervals_forecast_redis, nTimeIntervals_redis, forstaOverT, startT;
	int nDefault, nNoll, nTot, nBandsAlloc, nSecondsUTC, nBandsNu;
	int nAllocTmp = 0;
	long long* secondsUTC_tmp = NULL;

	int tidpHistoricalWeather;
	int posNu, savePosStart, i10;

	long long maxTid, sekNu, offset_x2;
	double min_lon, max_lon, min_lat, max_lat, min_lonUse, max_lonUse;
	double xPosFrac0, xPosFrac1, yPosFrac0, yPosFrac1;
	//float* arrFloat;


	double filKvot;
	int manad2, returnVal, first_i1, nBandsTooLate;
	double size_col, size_row, xPosFrac, yPosFrac, lat, lon;
	long long nSecondsUTC_next;

	model.tmpTid[0] = std::chrono::high_resolution_clock::now();

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
	nBandsTooLate = 0;
	first_i1 = 1;

	for (int i10 = 0; i10 < model.params.nHindCastMonths; i10++)
		initFile[i10] = 0;

	for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
		posNu = 0;
		savePosStart = 0;
		for (int i10 = 0; i10 < model.params.nHindCastMonths; i10++) {
			if (model.params.hindCast_month[i10] < 10)
				sprintf(namn, "%sHindcast/0%d/%s_0%d_%d.grb", model.params.weatherPath.c_str(), model.params.hindCast_month[i10],
					model.weather[ii].filePos[i1].fileName, model.params.hindCast_month[i10],
					model.params.hindCast_year[i10]);
			else
				sprintf(namn, "%sHindcast/%d/%s_%d_%d.grb", model.params.weatherPath.c_str(), model.params.hindCast_month[i10],
					model.weather[ii].filePos[i1].fileName, model.params.hindCast_month[i10],
					model.params.hindCast_year[i10]);
			printf("%s\n", namn);

			returnVal = model.weather[ii].rasterPos[i1].open(namn);
			if (returnVal == -1) {
				if (i10 == 0) {
					errlog("ERROR! Failed to open weather file %s. I quit!\n", namn);
					postRequest("ERROR! Failed to open weather file " + std::string(namn) + ".Fix it and run OptiNav hindCast again.", 1);
				}
				else {
					errlog("ERROR! Failed to open weather file %s. I don't use it!\n", namn);
					continue;
				}
			}
			if (i10 == 0) {
				model.weather[ii].filePos[i1].minX = model.weather[ii].rasterPos[i1].Get_minLongitude();
				model.weather[ii].filePos[i1].maxX = model.weather[ii].rasterPos[i1].Get_maxLongitude();
				if (check_useRaster_longitude(ii, i1) == 0)
					break;
			}
			if (first_i1 >= 1) {
				first_i1 = 2;
				//if (i10 == 0) {
				if (size_col < -0.1) {
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
					endPos[i10] = -1;
					for (int i2 = 0; i2 < nBands; i2++) {
						if (i2 > 0)
							nSecondsUTC = nSecondsUTC_next;
						else
							nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);

						if (i2 + 1 < nBands) {
							nSecondsUTC_next = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 2);
							if (nSecondsUTC_next < model.params.UTC_secondsStart - model.params.hindCast_yearSecDiff[i10])
								continue; // too early
						}

						if (nSecondsUTC >= endTime_secondsUTC - model.params.hindCast_yearSecDiff[i10])
							nBandsTooLate++;
						if (nBandsTooLate > 1) {
							break; // too late, do not include any more bands
						}
						endPos[i10] = i2;

						if (posNu == 0) {
							startPos[i10] = i2;
							nBandsAlloc = (int)(nBands - i2);
							model.weather[ii].secondsUTC = (long long*)malloc2(nBandsAlloc * sizeof(long long));
							if (nBandsAlloc > nAllocTmp) {
								if (nAllocTmp == 0) {
									nAllocTmp = nBandsAlloc;
									secondsUTC_tmp = (long long*)malloc(nAllocTmp * sizeof(long long));
								}
								else {
									nAllocTmp = nBandsAlloc;
									secondsUTC_tmp = (long long*)realloc(secondsUTC_tmp, nAllocTmp * sizeof(long long));
								}
							}
							model.weather[ii].valueCell = (float**)malloc2(nBandsAlloc * sizeof(float*));
							//for (int i2 = 0; i2 < nBandsAlloc; i2++) {
							//	model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
							//	for (int i3 = 0; i3 < nAlloc; i3++)
							//		model.weather[ii].valueCell[posNu][i3] = 9999;
							//}
						}
						model.weather[ii].secondsUTC[posNu] = nSecondsUTC;
						secondsUTC_tmp[posNu] = nSecondsUTC + model.params.hindCast_yearSecDiff[i10];

						posNu++;
					}
					model.weather[ii].nTimeIntervals = posNu;
					model.weather[ii].nTimeIntervals_forecast = posNu;
				}
				else {
					size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
					if (abs(model.weather[ii].size_col - size_col) > 0.0001)
						errlog("ERROR! Wrong size_col %.5lf vs %.5lf for weather data %s ii %d i1 %d i10 %d\n",
							size_col, model.weather[ii].size_col, namn, ii, i1, i10);
					xPosFrac = (model.boundingBox.xMin - model.weather[ii].rasterPos[i1].Get_minLongitude()) / size_col;
					xPos0 = roundDown(xPosFrac);
					if (abs(model.weather[ii].minX - model.weather[ii].rasterPos[i1].Get_minLongitude() -
						xPos0 * size_col) > 0.0001)
						errlog("ERROR! Wrong minX %.5lf vs %.5lf for weather data %s ii %d i1 %d i10 %d\n",
							model.weather[ii].rasterPos[i1].Get_minLongitude() + xPos0 * size_col,
							model.weather[ii].minX, namn, ii, i1, i10);
					xPosFrac = (model.boundingBox.xMax - model.weather[ii].minX) /
						size_col;

					xPos1 = roundUp(xPosFrac);
					if (abs(model.weather[ii].maxX - model.weather[ii].minX - xPos1 * size_col) > 0.0001)
						errlog("ERROR! Wrong maxX %.5lf vs %.5lf for weather data %s ii %d i1 %d i10 %d\n",
							model.weather[ii].minX + xPos1 * size_col,
							model.weather[ii].maxX, namn, ii, i1, i10);
					if (model.weather[ii].nCols != xPos1 + 1)
						errlog("ERROR! Wrong nCols %d vs %d for weather data %s ii %d i1 %d i10 %d\n",
							xPos1 + 1, model.weather[ii].nCols, namn, ii, i1, i10);
					//model.weather[ii].nCols = xPos1 + 1;

					size_row = model.weather[ii].rasterPos[i1].Get_sizeRow();
					if (abs(model.weather[ii].size_row - size_row) > 0.0001)
						errlog("ERROR! Wrong size_row %.5lf vs %.5lf for weather data %s ii %d i1 %d i10 %d\n",
							size_row, model.weather[ii].size_row, namn, ii, i1, i10);
					yPosFrac = (model.weather[ii].rasterPos[i1].Get_maxLatitude() - model.boundingBox.yMax) /
						size_row;
					yPos0 = roundDown(yPosFrac);
					if (abs(model.weather[ii].maxY - model.weather[ii].rasterPos[i1].Get_maxLatitude() +
						yPos0 * size_row) > 0.0001)
						errlog("ERROR! Wrong maxY %.5lf vs %.5lf for weather data %s ii %d i1 %d i10 %d\n",
							model.weather[ii].rasterPos[i1].Get_maxLatitude() -
							yPos0 * size_row, model.weather[ii].maxY, namn, ii, i1, i10);
					yPosFrac = (model.weather[ii].maxY - model.boundingBox.yMin) /
						size_row;
					yPos1 = roundUp(yPosFrac);
					if (yPos1 >= model.weather[ii].rasterPos[i1].Get_nRows())
						yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
					if (abs(model.weather[ii].minY - model.weather[ii].maxY +
						yPos1 * size_row) > 0.0001)
						errlog("ERROR! Wrong maxY %.5lf vs %.5lf for weather data %s ii %d i1 %d i10 %d\n",
							model.weather[ii].maxY -
							yPos1 * size_row, model.weather[ii].minY, namn, ii, i1, i10);
					//model.weather[ii].minY = model.weather[ii].maxY -
					//	yPos1 * size_row;
					if (model.weather[ii].nRows != yPos1 + 1)
						errlog("ERROR! Wrong nRows %d vs %d for weather data %s ii %d i1 %d i10 %d\n",
							yPos1 + 1, model.weather[ii].nRows, namn, ii, i1, i10);
					//model.weather[ii].nRows = yPos1 + 1;



					if (ii == 3)
						ii = ii;
					nBands = model.weather[ii].rasterPos[i1].Get_nBands();

					nBandsAlloc += nBands;
					model.weather[ii].nTimeIntervals = nBandsAlloc;
					model.weather[ii].nTimeIntervals_forecast = nBandsAlloc;
					model.weather[ii].secondsUTC = (long long*)realloc(model.weather[ii].secondsUTC, nBandsAlloc * sizeof(long long));
					nAllocTmp = nBandsAlloc;
					secondsUTC_tmp = (long long*)realloc(secondsUTC_tmp, nAllocTmp * sizeof(long long));

					model.weather[ii].valueCell = (float**)realloc(model.weather[ii].valueCell, nBandsAlloc * sizeof(float*));
					nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;


					startPos[i10] = 0;
					endPos[i10] = -1;
					for (int i2 = 0; i2 < nBands; i2++) {
						nSecondsUTC = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1);
						if (nSecondsUTC >= endTime_secondsUTC - model.params.hindCast_yearSecDiff[i10])
							nBandsTooLate++;
						if (nBandsTooLate > 1)
							break; // too late, do not include any more bands
						endPos[i10] = i2;

						model.weather[ii].secondsUTC[posNu] = nSecondsUTC;
						secondsUTC_tmp[posNu] = nSecondsUTC + model.params.hindCast_yearSecDiff[i10];
						posNu++;
					}
					model.weather[ii].nTimeIntervals = posNu;
					model.weather[ii].nTimeIntervals_forecast = posNu;
				}
			}
			else {
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001)
					errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
				if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001)
					errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
						model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());

				errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
					ii, model.weather[ii].nTimeIntervals,
					model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);
				nBands = model.weather[ii].rasterPos[i1].Get_nBands();
			}
			if (ii == 2 && i1 == 1)
				ii = ii;

			nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
			if (initFile[i10] == 0) {
				for (posNu = savePosStart; posNu <= savePosStart + endPos[i10] - startPos[i10]; posNu++) {
					model.weather[ii].valueCell[posNu] = (float*)malloc2(nAlloc * sizeof(float));
					for (int i3 = 0; i3 < nAlloc; i3++)
						model.weather[ii].valueCell[posNu][i3] = 9999;
				}
				initFile[i10] = 1;
			}

			if (ii == 2)
				ii = ii;
			if (endPos[i10] >= nBands)
				postRequest("ERROR! To few bands in fil " + std::string(namn) + " must be the same as in the first one. I quit!", 1);

			model.weather[ii].rasterPos[i1].GetRasterValues_realHindCastBands(&(model.weather[ii]), startPos[i10], endPos[i10], savePosStart, filKvot);
			savePosStart += endPos[i10] - startPos[i10] + 1;
			//if (i1 < model.weather[ii].nFiles / 2.0)
			//	model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands_fixBandNr(&(model.weather[ii]), 0, nBandsAlloc);
			//else
			//	model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands_fixBandNr(&(model.weather[ii]), nBands, nBandsAlloc);

		}
		if (first_i1 == 2)
			first_i1 = 0;



		//testCoordValue(ii, -41.43, -26.44);
		//testCoordValue(ii, -40.77, -26.76);
		// testCoordValue(ii, 93.84, 5.98);
		//testCoordValue(ii, 167.6111, 81);
		//testCoordValue(ii, 179.6111, 82);
		//testCoordValue(ii, -179.6111, 83);
		//testCoordValue(ii, -132.39, 84);


		// om olika diskretization pa oppnade raster sa stoppa
		// 



		//model.weather[ii].valueCell = model.weather[ii].rasterPos.GetRasterBand_realArrAllBands(&(model.weather[ii].raster), model.boundingBox);
		//printf("used dim %d %d tid %lf nBands %d\n", model.weather[ii].nRows,
		//	model.weather[ii].nCols, model.durationMilli[ii], model.weather[ii].nTimeIntervals);





		//errlog("iicc %d ii2 %d\n", ii, ii2);
		(model.nCallsWeatherBand[ii])++;
		//errlog("weather %d variable %s nTimeInt %d dim %d %d tid %lf\nminLon %.3lf maxLon %.3lf\nminLat %.3lf maxLat %.3lf\n", ii,
		//	model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals,
		//	model.weather[ii].nRows,
		//	model.weather[ii].nCols,
		//	model.durationMilli[ii],
		//	model.weather[ii].minX, model.weather[ii].maxX,
		//	model.weather[ii].minY, model.weather[ii].maxY);

	}
	if (ii == 2)
		ii = ii;
	// testCoordValue(ii, 25.675, -33.917);


	if (nAllocTmp > 0) {
		for (int i2 = 0; i2 < model.weather[ii].nTimeIntervals; i2++) {
			model.weather[ii].secondsUTC[i2] = secondsUTC_tmp[i2];
		}
		free(secondsUTC_tmp);
	}

	nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
	model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

	errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
		ii, model.weather[ii].nTimeIntervals,
		model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);

	tidInt = 0;
	for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

		if (i == model.weather[ii].nTimeIntervals - 1)
			maxTid = model.weather[ii].secondsUTC[i] + 3600 - 1;
		else
			maxTid = (long long)(0.5 * (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]));

		for (; tidInt < 100000; tidInt++) {
			if (tidInt >= nAlloc) {
				printf("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d tidInt %d maxTid %I64d\n", tidInt, nAlloc, ii,
					model.weather[ii].weatherFileTypeName, i, tidInt, maxTid);
				errlog("ERROR! Too many tidInt compared to allocated (%d vs %d) for weather param %d %s. I skip the rest, i %d tidInt %d maxTid %I64d\n", tidInt, nAlloc, ii,
					model.weather[ii].weatherFileTypeName, i, tidInt, maxTid);
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
		if (model.weather[ii].nTimeIntervals_maxValue > (*nMaxTimeInt))
			*nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;
	}
	if (model.weather[ii].nTimeIntervals_maxValue < 0)
		model.weather[ii].nTimeIntervals_maxValue = 0;

	//testCoordValue(ii, 141.639, -11.240);
	//testCoordValue(ii, 149.315, -21.275);

	return 0;

}

int loadStorms_historical() {
	char* namn = (char*)malloc2(256 * sizeof(char));
	json data, dataFeature;
	std::ifstream fil;
	int manadNu, i, year, month, antal;

	model.nStorms = 0;
	model.nameTmp = (char*)malloc(256 * sizeof(char));
	model.nAllocStorms = 0;
	for (i = 0; i < model.params.nHindCastMonths; i++) {
		year = model.params.hindCast_year[i];
		manadNu = model.params.hindCast_month[i];
		if (manadNu < 10)
			sprintf(namn, "%sHindcast/storms/storms_0%d_%d.json", model.params.weatherPath.c_str(), manadNu, year);
		else {
			sprintf(namn, "%sHindcast/storms/storms_%d_%d.json", model.params.weatherPath.c_str(), manadNu, year);
			// sprintf(namn, "%s/%s_%d_%d.json", model.params.indataPath.c_str(), manadNu, year);
		}
		printf("opens storm file %s\n", namn);
		fil.open(namn);
		if (fil.is_open() == TRUE) {
			fil >> data;

			if (model.nAllocStorms == 0) {
				antal = data.size();
				if (antal < 100)
					antal = 100;
				model.nAllocStorms = antal * model.params.nHindCastMonths;
				model.nStorms = 0;
				model.storms = (strStorm*)malloc2(model.nAllocStorms * sizeof(strStorm));
			}
			for (auto it = data.begin(); it != data.end(); ++it) {
				dataFeature = it.value();
				loadStormObject(dataFeature);
			}
			fil.close();
		}
		else {
			postRequest("ERROR! There is no storm file for year " + std::to_string(year) + " month " + std::to_string(manadNu) + ". I will still continue.", 0);
		}
	}
	calc_stormsNearby_delay();

	free(namn);
	//simuleraStormsVisuellt();

	return 0;
}

void loadWeatherFiles_grib() {
	int ii, i;
	int nMaxTimeInt = 0;
	double endTime = model.network.physicalLev[model.network.nPhysicalLevels - 1].distanceFromStartPosMid / model.params.preferredSpeed_calmWater; // * delayFactor;
	double factorExtraTime = 1.3;
	int* startPos, * endPos;

	tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	setTMtime(&tmBas, endTime);
	char* timeTxt = (char*)malloc(2356 * sizeof(char));
	fixReadableDate(tmBas, timeTxt);
	errlog("Estimated endtime %.2lf: end date/time %s\n",
		endTime, timeTxt);
	endTime *= factorExtraTime;
	setTMtime(&tmBas, endTime);
	fixReadableDate(tmBas, timeTxt);
	errlog("factorExtraTime %.2lf gives Estimated endtime %.2lf: end date/time %s\n",
		factorExtraTime, endTime, timeTxt);
	identify_hindCastMonths(tmBas, 0);

	if (model.params.hindCast == 1) {
		printf("Loading storms\n");
		loadStorms_historical();
		printf("Identifying storms nearby\n");
		calc_stormsNearby();
	}


	startPos = (int*)malloc(model.params.nHindCastMonths * sizeof(int));
	endPos = (int*)malloc(model.params.nHindCastMonths * sizeof(int));

	// arrFloat = NULL;

	char* namn = (char*)malloc(2356 * sizeof(char));
	int* initFile = (int*)malloc(model.params.nHindCastMonths * sizeof(int));
	long long endTime_secondsUTC = model.params.UTC_secondsStart + endTime * 3600;

	printf("loading the grib files\n");
	//errlog("test14\n");
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		loadStaticWeatherFile_grib(ii, &nMaxTimeInt, initFile, namn, startPos, endPos, endTime_secondsUTC);
	}
	free(initFile);

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

	printf("-- Time after weather data loaded %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	printf("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	errlog("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	//exit(0);

}

void loadAllWeatherFiles_gribForecast_kaoutar() {
	int ii, i;
	int nMaxTimeInt = 0;
	//double endTime = model.network.physicalLev[model.network.nPhysicalLevels - 1].distanceFromStartPosMid / model.params.preferredSpeed_calmWater; // * delayFactor;
	//double factorExtraTime = 1.3;
	int* startPos, * endPos;

	// if (model.network.tidp_startHistoricDataOnly < 999999) {

	tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	setTMtime(&tmBas, model.iterKaoutar.tidpStartIter_h);
	char* timeTxt = (char*)malloc(2356 * sizeof(char));
	fixReadableDate(tmBas, timeTxt);
	errlog("starttime %.2lf: start date/time %s\n",
		model.iterKaoutar.tidpStartIter_h, timeTxt);

	if (model.params.hindCast == 1) {
		loadStorms_historical();
		calc_stormsNearby();
	}

	char* namn = (char*)malloc(2356 * sizeof(char));
	int nDagarFramat = model.results.forecastTypeOrig % 100;
	if (nDagarFramat < 0)
		printf("ERROR! Didn't expect negative number of days forward %d\n", nDagarFramat);
	long long endTime_secondsUTC = model.params.UTC_secondsStart + (model.iterKaoutar.startTimeUse + nDagarFramat * 24 + 3) * 3600; // allow 3 more hours...

	identifyUsableFilDirsForecast();

	//errlog("test14\n");
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		//loadStaticWeatherFile_grib(ii, &nMaxTimeInt, initFile, namn, startPos, endPos, endTime_secondsUTC);
		loadForecastWeatherFile_grib_kaoutar(ii, &nMaxTimeInt, namn, endTime_secondsUTC);
	}
	// free(initFile);

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

	printf("-- Time after weather data loaded %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	printf("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);

}

long long determineLastSecondNeeded() {
	long long lastSecond = model.params.UTC_secondsStart + model.params.longestRouteDays_history * 3600 * 24;
	long long tmpSeconds = model.params.UTC_secondsStart +
		model.network.physicalLev[model.network.nPhysicalLevels - 1].distanceFromStartPosMid / model.params.preferredSpeed_calmWater * 2 * 3600 +
		3600 * 5;
	if (tmpSeconds < lastSecond)
		return tmpSeconds;
	else
		return lastSecond;
}

int estimate_nDaysNeededHistory(int weatherNr, long long lastSecondUTC_needed, long long* nSecondsHistory_first, long long* nSecondsHistory_firstStartDay) {
	int nDaysNeeded;
	long long nSecondsUTC_last = model.weather[weatherNr].secondsUTC[model.weather[weatherNr].nTimeIntervals - 1];
	*nSecondsHistory_first = nSecondsUTC_last + model.weather_timeIntervall_h * 3600;
	*nSecondsHistory_firstStartDay = getFirstSecondOfDay(*nSecondsHistory_first);

	long long nExtraSecondsNeeded = lastSecondUTC_needed - (*nSecondsHistory_first);
	if (nExtraSecondsNeeded < 0)
		nExtraSecondsNeeded = 0;
	nDaysNeeded = (int)(nExtraSecondsNeeded / 3600 / 24) + 1;

	return nDaysNeeded;
}

void SwapArray(long long* Array, int a, int b)
{
	long long temp = Array[a];
	Array[a] = Array[b];
	Array[b] = temp;
}
void SwapArray(int* Array, int a, int b)
{
	int temp = Array[a];
	Array[a] = Array[b];
	Array[b] = temp;
}


int SorteraArrayOrderToMax2(long long* BasArray, int* Array2, int nElement)
{
	int i, j;

	for (j = 0; j < nElement; j++) {
		for (i = 0; i < nElement - 1; i++) {
			if (BasArray[i] > BasArray[i + 1]) {
				SwapArray(BasArray, i, i + 1);
				SwapArray(Array2, i, i + 1);
			}
		}
	}
	return 0;
}



void loadWeatherFiles_onboard_grib() {
	int xPos0, xPos1, yPos0, yPos1, nBands, ii;
	int pos2, pos3, i4, i5, nAlloc, i3, offset_x, offset_y;
	int nCols, nRows, nRows_inBlock, nCols_inBlock;
	int yStartBlock, yStartValue, nY_valueAdd, yEndBlock;
	int xStartBlock, xStartValue, nX_valueAdd, xEndBlock, i;
	int xBlockNr, xBlockUse, yBlockNr, pos, latPos, lonPos, tidInt;
	int nTimeIntervals_forecast_redis, nTimeIntervals_redis, forstaOverT, startT;
	int nDefault, nNoll, nTot, nBandsAlloc, nSecondsUTC, nBandsNu;
	int nMaxTimeInt = 0, tidpHistoricalWeather, nDaysUsed_history;
	int nBandsTooLate, posNu, savePosStart, * endPos, * startPos, i10;

	long long maxTid, sekNu, offset_x2;
	double min_lon, max_lon, min_lat, max_lat, min_lonUse, max_lonUse;
	double size_col, size_row, xPosFrac, yPosFrac, lat, lon;
	double xPosFrac0, xPosFrac1, yPosFrac0, yPosFrac1;
	float* arrFloat;

	double factorExtraTime = 1.3;
	double endTime = model.network.physicalLev[model.network.nPhysicalLevels - 1].distanceFromStartPosMid / model.params.preferredSpeed_calmWater; // * delayFactor;

	arrFloat = NULL;

	char* namn = (char*)malloc(2356 * sizeof(char));
	double filKvot, maxLong;
	int openOK, nDaysNeeded_history, i2, manad, dag, pos1, zNu, iY, iX;
	long long nSecondsUTC_last, nSecondsHistory_first, nSecondsHistory_firstStartDay, nExtraSecondsNeeded;
	long long nSecondsUTC_first, secondsNow;
	std::string histFileName;

	long long lastSecondUTC_needed = determineLastSecondNeeded();

	//errlog("test14\n");
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		model.tmpTid[0] = std::chrono::high_resolution_clock::now();
		if (model.params.onboard_currentStatic == 1) {
			if (model.params.weather_is_current[ii] == 1)
				continue; // do not load current from forecast but from hindcast data
		}

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
		maxLong = -9999;

		if (ii == 2)
			ii = ii;

		for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			openOK = model.weather[ii].rasterPos[i1].open(model.weather[ii].filePos[i1].fileName);
			printf("%s openOK %d\n", model.weather[ii].filePos[i1].fileName, openOK);
			if (openOK != 1) {
				errlog("ERROR! Could not open forecast file %s. This one must exist. I quit\n",
					model.weather[ii].filePos[i1].fileName);
				postRequest("ERROR! Could not open forecast file " + std::string(model.weather[ii].filePos[i1].fileName) + ". This one must exist.I quit", 1);
			}
			nBands = model.weather[ii].rasterPos[i1].Get_nBands();

			min_lon = model.weather[ii].rasterPos[i1].Get_minLongitude();
			max_lon = min_lon + 360;
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

			if (i1 == 0) {
				checkMinnesAnvandning(__LINE__);
				//printf("test1\n");
				nDaysNeeded_history = -1;

				size_col = model.weather[ii].rasterPos[i1].Get_sizeCol();
				model.weather[ii].size_col = size_col;

				xPosFrac = (model.boundingBox.xMin - min_lonUse) / size_col;
				xPos0 = roundDown(xPosFrac);
				model.weather[ii].minX = min_lonUse + xPos0 * size_col;
				xPosFrac = (model.boundingBox.xMax - model.weather[ii].minX) / size_col;
				xPos1 = roundUp(xPosFrac);
				model.weather[ii].maxX = model.weather[ii].minX + xPos1 * size_col;
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
				//yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
				model.weather[ii].minY = model.weather[ii].maxY - yPos1 * size_row;
				model.weather[ii].nRows = yPos1 + 1;
				checkMinnesAnvandning(__LINE__);

				model.weather[ii].nTimeIntervals_forecast = nBands;
				model.weather[ii].nTimeIntervals = nBands;
				model.weather[ii].secondsUTC = (long long*)malloc2((nBands) * sizeof(long long));
				model.weather[ii].timePosToBandPos = (int*)malloc2((nBands) * sizeof(int));
				// printf("\n\n### secondsUTC alloc %d ####\n\n\n", nBands + nDaysNeeded_history);
				//model.weather[ii].valueCell = (float**)malloc2((nBands) * sizeof(float*));
				//nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				checkMinnesAnvandning(__LINE__);
				for (i2 = 0; i2 < nBands; i2++) {
					//model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
					//for (int i3 = 0; i3 < nAlloc; i3++)
					//	model.weather[ii].valueCell[i2][i3] = 9999;
					model.weather[ii].secondsUTC[i2] = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1); //-1;
					model.weather[ii].timePosToBandPos[i2] = i2;
					errlog("ii %d band %d hoursUTC %.2lf\n", ii, i2, model.weather[ii].secondsUTC[i2] / 3600.0);
				}
				SorteraArrayOrderToMax2(model.weather[ii].secondsUTC, model.weather[ii].timePosToBandPos, nBands);
				for (i2 = 0; i2 < nBands; i2++) {
					errlog("ii %d band %d hoursUTC %.2lf timePosToBandPos %d\n", ii, i2,
						model.weather[ii].secondsUTC[i2] / 3600.0, model.weather[ii].timePosToBandPos[i2]);
				}

				checkMinnesAnvandning(__LINE__);
				model.weather[ii].errorCode = 0;

				nDaysNeeded_history = estimate_nDaysNeededHistory(ii, lastSecondUTC_needed, &nSecondsHistory_first,
					&nSecondsHistory_firstStartDay);

				forstaOverT = -1;
				for (i2 = 0; i2 < nBands; i2++) {
					if (model.weather[ii].secondsUTC[i2] > model.params.UTC_secondsStart && forstaOverT == -1) {
						forstaOverT = i2;
						break;
					}
				}
				if (forstaOverT <= 0) {
					if (forstaOverT == 0) {
						errlog("ERROR! %s Start planning a route at UTC second %I64d but weather data for %d only starts at %I64d, diff %.2lf hours \n",
							model.weather[ii].weatherFileTypeName, model.params.UTC_secondsStart, ii, model.weather[ii].secondsUTC[0],
							(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[0]) / 3600.0);
						startT = 0;
					}
					else {
						errlog("ERROR! Start planning a route at UTC second %I64d but last weather data for parameter %d is %I64d, diff %.2lf hours \n",
							model.params.UTC_secondsStart, ii, model.weather[ii].secondsUTC[i2 - 1],
							(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[i2 - 1]) / 3600.0);
						startT = i2 - 1;
					}
				}
				else
					startT = forstaOverT - 1;

				if (startT > 0) {
					for (i = startT; i < nBands; i++) {
						model.weather[ii].secondsUTC[i - startT] = model.weather[ii].secondsUTC[i];
						model.weather[ii].timePosToBandPos[i - startT] = model.weather[ii].timePosToBandPos[i];
						//for (i2 = 0; i2 < nAlloc; i2++)
						//	model.weather[ii].valueCell[i - startT][i2] = model.weather[ii].valueCell[i][i2];
					}
				}
				printf("nBands %d startT %d nDaysHistory %d\n", nBands, startT, nDaysNeeded_history);
				if (model.params.onboard == 1) {
					nDaysUsed_history = nDaysNeeded_history;
					if (nDaysUsed_history < 0)
						nDaysUsed_history = 0;
				}
				else
					nDaysUsed_history = 1;

				model.weather[ii].nTimeIntervals_forecast = nBands - startT;
				if (model.weather[ii].nTimeIntervals_forecast < 0)
					model.weather[ii].nTimeIntervals_forecast = 0;
				model.weather[ii].nTimeIntervals = nBands - startT + nDaysUsed_history;

				checkMinnesAnvandning(__LINE__);


				if (nBands + nDaysUsed_history - startT > nBands) {
					model.weather[ii].secondsUTC = (long long*)realloc(model.weather[ii].secondsUTC, (nBands + nDaysUsed_history - startT) * sizeof(long long));
					model.weather[ii].timePosToBandPos = (int*)realloc(model.weather[ii].timePosToBandPos, (nBands + nDaysUsed_history - startT) * sizeof(int));
				}
				model.weather[ii].valueCell = (float**)malloc((nBands + nDaysUsed_history - startT) * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (i2 = 0; i2 < nBands + nDaysUsed_history - startT; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
					for (int i3 = 0; i3 < nAlloc; i3++)
						model.weather[ii].valueCell[i2][i3] = 9999;
					if (i2 >= nBands - startT) {
						model.weather[ii].secondsUTC[i2] = -1;
						model.weather[ii].timePosToBandPos[i2] = i2;
					}
				}
				checkMinnesAnvandning(__LINE__);

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
				if (model.weather[ii].nTimeIntervals_forecast + startT != nBands) {
					errlog("ERROR! Different number of bands for weather parameter %s, %d and %d, file %s. Must be the same. I quit\n",
						model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals_forecast, nBands,
						model.weather[ii].filePos[i1].fileName);
					model.weather[ii].errorCode = 2;
					continue;
				}
			}
			//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
			//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
			//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
			checkMinnesAnvandning(__LINE__);

			if (model.weather[ii].defaultValueMore == 1)
				model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0, filKvot, startT);
			else
				model.weather[ii].rasterPos[i1].GetRasterValues_realAllBandsUp(&(model.weather[ii]), 0, filKvot, startT);
			checkMinnesAnvandning(__LINE__);

			if (model.params.onboard == 2) {
				zNu = model.weather[ii].nTimeIntervals_forecast;
				for (i2 = 0; i2 < nDaysUsed_history; i2++) {
					zNu = model.weather[ii].nTimeIntervals_forecast + i2;
					for (iY = 0; iY < model.weather[ii].nRows; iY++) {
						for (iX = 0; iX < model.weather[ii].nCols; iX++) {
							model.weather[ii].valueCell[zNu][iX + model.weather[ii].nCols * iY] =
								model.weather[ii].valueCell[zNu - 1][iX + model.weather[ii].nCols * iY];
						}
					}
					if (i2 == 0)
						secondsNow = nSecondsHistory_first + i2 * 24 * 3600;
					else
						secondsNow = nSecondsHistory_firstStartDay + (nDaysNeeded_history - 1) * 24 * 3600;
					model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
					model.weather[ii].timePosToBandPos[model.weather[ii].nTimeIntervals_forecast + i2] = model.weather[ii].nTimeIntervals_forecast + i2;
				}
			}
			else {
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
					}
					else
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
					checkMinnesAnvandning(__LINE__);
					openOK = model.weather[ii].rasterPos[i1].open(histFileName.c_str());
					if (openOK == 1) {
						errlog("Open %s okay.\n",
							histFileName.c_str());
						if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001 ||
							abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
							if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001) {
								printf("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
								errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
							}
							if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
								errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
								printf("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
							}
							model.weather[ii].errorCode = 4;
							continue;
						}
						//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
						//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
						//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
						model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), model.weather[ii].nTimeIntervals_forecast + i2, filKvot);
						//printf("history day %d changes secondsUTC from %I64d to %I64d\n", i2,
						//	model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2], secondsNow);
					}
					else {
						errlog("ERROR! Failed to open %s. I use weather data from the previous loaded file\n",
							histFileName.c_str());
						zNu = model.weather[ii].nTimeIntervals_forecast + i2;
						for (iY = 0; iY < model.weather[ii].nRows; iY++) {
							for (iX = 0; iX < model.weather[ii].nCols; iX++) {
								model.weather[ii].valueCell[zNu][iX + model.weather[ii].nCols * iY] =
									model.weather[ii].valueCell[zNu - 1][iX + model.weather[ii].nCols * iY];
							}
						}

						(model.status.weatherHistoryOpenFile_fail)++;
						printf("ERROR! Failed to open %s, nFailed %d. I use weather data from the previous loaded file. secondsNow %I64d\n",
							histFileName.c_str(), model.status.weatherHistoryOpenFile_fail, secondsNow);
					}
					//printf("test %d\n", model.weather[ii].nTimeIntervals_forecast + i2);
					model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
					model.weather[ii].timePosToBandPos[model.weather[ii].nTimeIntervals_forecast + i2] = model.weather[ii].nTimeIntervals_forecast + i2;
				}
				(model.nCallsWeatherBand[ii])++;
			}
		}
		// last dateTime plus 24 hours...
		if (model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] <= model.params.UTC_secondsStart) {
			errlog("ERROR! Last timeperiod in weather %d is earlier than the starttime for the planning (%I64d %I64d). Update the forecast planning. I set it to starttime plus 1 for it to work\n", ii,
				model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1], model.params.UTC_secondsStart);
			model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] = model.params.UTC_secondsStart + 1;
			model.weather[ii].timePosToBandPos[model.weather[ii].nTimeIntervals - 1] = model.weather[ii].nTimeIntervals - 1;
		}

		nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
		model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

		errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
			ii, model.weather[ii].nTimeIntervals,
			model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);

		tidInt = 0;
		tidpHistoricalWeather = -1;
		for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

			if (i == model.weather[ii].nTimeIntervals - 1)
				maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
			else {
				// maxTid = model.weather[ii].secondsUTC[i + 1] - 1;
				maxTid = (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]) / 2;
			}
			// maxTid = (long long)(0.5 * (model.weather[ii].secondsUTC[i] +  model.weather[ii].secondsUTC[i + 1]));

			for (; tidInt < 100000; tidInt++) {
				if (i >= model.weather[ii].nTimeIntervals_forecast && tidpHistoricalWeather == -1)
					tidpHistoricalWeather = tidInt;
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
				//errlog("ii %d tidInt %d i %d maxTid %I64d sekNu %I64d\n", ii, tidInt, i, maxTid, sekNu);
			}
		}
		if (tidpHistoricalWeather == -1)
			tidpHistoricalWeather = tidInt + 1;

		if (tidpHistoricalWeather * model.weather_timeIntervall_h > model.network.tidp_startHistoricDataOnly)
			model.network.tidp_startHistoricDataOnly = tidpHistoricalWeather * model.weather_timeIntervall_h;

		//printf("tidInt %d nAlloc %d\n", tidInt, nAlloc);

		model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;
		if (model.weather[ii].nTimeIntervals_maxValue > nMaxTimeInt)
			nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;

		//if (model.boundingBox.xMin < min_lon) {
		//	min_lonUse = min_lon - 360;
		//	max_lonUse = max_lon - 360;
		//}
		//else {
		//	if (model.boundingBox.xMin >= min_lon + 360) {
		//		min_lonUse = min_lon + 360;
		//		max_lonUse = max_lon + 360;
		//	}
		//	else {
		//		min_lonUse = min_lon;
		//		max_lonUse = max_lon;
		//	}

		//}
		(model.nCallsWeatherBand[ii])++;

		//testCoordValue(ii, 141.639, -11.240);
		//testCoordValue(ii, 149.315, -21.275);

	}


	if (model.params.onboard_currentStatic == 1) {
		double endTime = model.network.tidp_startHistoricDataOnly - model.weather_timeIntervall_h;
		if (endTime < 0)
			endTime = 0;
		tm tmBas = { 0 };
		tmBas.tm_isdst = 0;
		setTMtime(&tmBas, endTime);
		char* timeTxt = (char*)malloc(2356 * sizeof(char));
		fixReadableDate(tmBas, timeTxt);
		errlog("Estimated endtime %.2lf: end date/time %s\n",
			endTime, timeTxt);
		//endTime *= factorExtraTime;
		setTMtime(&tmBas, endTime);
		fixReadableDate(tmBas, timeTxt);
		errlog("onboard current static, using data till endtime %.2lf: end date/time %s\n",
			endTime, timeTxt);
		identify_hindCastMonths(tmBas, 1);
		int* initFile = (int*)malloc(model.params.nHindCastMonths * sizeof(int));
		long long endTime_secondsUTC = model.params.UTC_secondsStart + (int)endTime * 3600;
		startPos = (int*)malloc(model.params.nHindCastMonths * sizeof(int));
		endPos = (int*)malloc(model.params.nHindCastMonths * sizeof(int));

		for (ii = 0; ii < model.nWeatherFiles; ii++) {
			if (model.params.weather_is_current[ii] != 1)
				continue; // only load current from hindcast data

			loadStaticWeatherFile_grib(ii, &nMaxTimeInt, initFile, namn, startPos, endPos, endTime_secondsUTC);
		}
		free(initFile);
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

	printf("-- Time after weather data loaded %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	printf("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	errlog("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	//exit(0);

}

void loadWeatherFiles_onboard_grib_old() {
	int xPos0, xPos1, yPos0, yPos1, nBands, ii;
	int pos2, pos3, i4, i5, nAlloc, i3, offset_x, offset_y;
	int nCols, nRows, nRows_inBlock, nCols_inBlock;
	int yStartBlock, yStartValue, nY_valueAdd, yEndBlock;
	int xStartBlock, xStartValue, nX_valueAdd, xEndBlock, i;
	int xBlockNr, xBlockUse, yBlockNr, pos, latPos, lonPos, tidInt;
	int nTimeIntervals_forecast_redis, nTimeIntervals_redis, forstaOverT, startT;
	int nDefault, nNoll, nTot, nBandsAlloc, nSecondsUTC, nBandsNu;
	int nMaxTimeInt = 0, tidpHistoricalWeather, nDaysUsed_history;
	int nBandsTooLate, posNu, savePosStart, * endPos, * startPos, i10;

	long long maxTid, sekNu, offset_x2;
	double min_lon, max_lon, min_lat, max_lat, min_lonUse, max_lonUse;
	double size_col, size_row, xPosFrac, yPosFrac, lat, lon;
	double xPosFrac0, xPosFrac1, yPosFrac0, yPosFrac1;
	float* arrFloat;

	double factorExtraTime = 1.3;
	double endTime = model.network.physicalLev[model.network.nPhysicalLevels - 1].distanceFromStartPosMid / model.params.preferredSpeed_calmWater; // * delayFactor;

	arrFloat = NULL;

	char* namn = (char*)malloc(2356 * sizeof(char));
	double filKvot, maxLong;
	int openOK, nDaysNeeded_history, i2, manad, dag, pos1, zNu, iY, iX;
	long long nSecondsUTC_last, nSecondsHistory_first, nSecondsHistory_firstStartDay, nExtraSecondsNeeded;
	long long nSecondsUTC_first, secondsNow;
	std::string histFileName;

	long long lastSecondUTC_needed = determineLastSecondNeeded();

	//errlog("test14\n");
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		model.tmpTid[0] = std::chrono::high_resolution_clock::now();

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
		maxLong = -9999;

		if (ii == 2)
			ii = ii;

		for (int i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			openOK = model.weather[ii].rasterPos[i1].open(model.weather[ii].filePos[i1].fileName);
			printf("%s openOK %d\n", model.weather[ii].filePos[i1].fileName, openOK);
			if (openOK != 1) {
				errlog("ERROR! Could not open forecast file %s. This one must exist. I quit\n",
					model.weather[ii].filePos[i1].fileName);
				postRequest("ERROR! Could not open forecast file " + std::string(model.weather[ii].filePos[i1].fileName) + ". This one must exist.I quit", 1);
			}
			nBands = model.weather[ii].rasterPos[i1].Get_nBands();
			model.weather[ii].filePos[i1].minX = model.weather[ii].rasterPos[i1].Get_minLongitude();
			model.weather[ii].filePos[i1].maxX = model.weather[ii].rasterPos[i1].Get_maxLongitude();
			if (model.weather[ii].filePos[i1].minX >= 170) {
				model.weather[ii].filePos[i1].minX -= 360;
				model.weather[ii].filePos[i1].maxX -= 360;
			}


			if (maxLong < model.weather[ii].filePos[i1].maxX)
				maxLong = model.weather[ii].filePos[i1].maxX;
			if (i1 == 0) {
				checkMinnesAnvandning(__LINE__);
				//printf("test1\n");
				nDaysNeeded_history = -1;

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
				//yPos1 = model.weather[ii].rasterPos[i1].Get_nRows() - 1;
				model.weather[ii].minY = model.weather[ii].maxY - yPos1 * size_row;
				model.weather[ii].nRows = yPos1 + 1;
				checkMinnesAnvandning(__LINE__);

				model.weather[ii].nTimeIntervals_forecast = nBands;
				model.weather[ii].nTimeIntervals = nBands;
				model.weather[ii].secondsUTC = (long long*)malloc2((nBands) * sizeof(long long));
				// printf("\n\n### secondsUTC alloc %d ####\n\n\n", nBands + nDaysNeeded_history);
				//model.weather[ii].valueCell = (float**)malloc2((nBands) * sizeof(float*));
				//nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				checkMinnesAnvandning(__LINE__);
				for (i2 = 0; i2 < nBands; i2++) {
					//model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
					//for (int i3 = 0; i3 < nAlloc; i3++)
					//	model.weather[ii].valueCell[i2][i3] = 9999;
					model.weather[ii].secondsUTC[i2] = model.weather[ii].rasterPos[i1].GetSecondsFromUTC_metadataBand(i2 + 1); //-1;
				}
				checkMinnesAnvandning(__LINE__);
				model.weather[ii].errorCode = 0;

				nDaysNeeded_history = estimate_nDaysNeededHistory(ii, lastSecondUTC_needed, &nSecondsHistory_first,
					&nSecondsHistory_firstStartDay);

				forstaOverT = -1;
				for (i2 = 0; i2 < nBands; i2++) {
					if (model.weather[ii].secondsUTC[i2] > model.params.UTC_secondsStart && forstaOverT == -1) {
						forstaOverT = i2;
						break;
					}
				}
				if (forstaOverT <= 0) {
					if (forstaOverT == 0) {
						errlog("ERROR! %s Start planning a route at UTC second %I64d but weather data for %d only starts at %I64d, diff %.2lf hours \n",
							model.weather[ii].weatherFileTypeName, model.params.UTC_secondsStart, ii, model.weather[ii].secondsUTC[0],
							(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[0]) / 3600.0);
						startT = 0;
					}
					else {
						errlog("ERROR! Start planning a route at UTC second %I64d but last weather data for parameter %d is %I64d, diff %.2lf hours \n",
							model.params.UTC_secondsStart, ii, model.weather[ii].secondsUTC[i2 - 1],
							(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[i2 - 1]) / 3600.0);
						startT = i2 - 1;
					}
				}
				else
					startT = forstaOverT - 1;

				if (startT > 0) {
					for (i = startT; i < nBands; i++) {
						model.weather[ii].secondsUTC[i - startT] = model.weather[ii].secondsUTC[i];
						//for (i2 = 0; i2 < nAlloc; i2++)
						//	model.weather[ii].valueCell[i - startT][i2] = model.weather[ii].valueCell[i][i2];
					}
				}
				printf("nBands %d startT %d nDaysHistory %d\n", nBands, startT, nDaysNeeded_history);
				if (model.params.onboard == 1) {
					nDaysUsed_history = nDaysNeeded_history;
					if (nDaysUsed_history < 0)
						nDaysUsed_history = 0;
				}
				else
					nDaysUsed_history = 1;

				model.weather[ii].nTimeIntervals_forecast = nBands - startT;
				if (model.weather[ii].nTimeIntervals_forecast < 0)
					model.weather[ii].nTimeIntervals_forecast = 0;
				model.weather[ii].nTimeIntervals = nBands - startT + nDaysUsed_history;

				checkMinnesAnvandning(__LINE__);


				if (nBands + nDaysUsed_history - startT > nBands)
					model.weather[ii].secondsUTC = (long long*)realloc(model.weather[ii].secondsUTC, (nBands + nDaysUsed_history - startT) * sizeof(long long));
				model.weather[ii].valueCell = (float**)malloc((nBands + nDaysUsed_history - startT) * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (i2 = 0; i2 < nBands + nDaysUsed_history - startT; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
					for (int i3 = 0; i3 < nAlloc; i3++)
						model.weather[ii].valueCell[i2][i3] = 9999;
					if (i2 >= nBands - startT)
						model.weather[ii].secondsUTC[i2] = -1;
				}
				checkMinnesAnvandning(__LINE__);

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
				if (model.weather[ii].nTimeIntervals_forecast + startT != nBands) {
					errlog("ERROR! Different number of bands for weather parameter %s, %d and %d, file %s. Must be the same. I quit\n",
						model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals_forecast, nBands,
						model.weather[ii].filePos[i1].fileName);
					model.weather[ii].errorCode = 2;
					continue;
				}
			}
			//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
			//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
			//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
			checkMinnesAnvandning(__LINE__);
			model.weather[ii].timePosToBandPos = NULL;
			model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0, filKvot, startT);
			checkMinnesAnvandning(__LINE__);

			if (model.params.onboard == 2) {
				zNu = model.weather[ii].nTimeIntervals_forecast;
				for (i2 = 0; i2 < nDaysUsed_history; i2++) {
					zNu = model.weather[ii].nTimeIntervals_forecast + i2;
					for (iY = 0; iY < model.weather[ii].nRows; iY++) {
						for (iX = 0; iX < model.weather[ii].nCols; iX++) {
							model.weather[ii].valueCell[zNu][iX + model.weather[ii].nCols * iY] =
								model.weather[ii].valueCell[zNu - 1][iX + model.weather[ii].nCols * iY];
						}
					}
					if (i2 == 0)
						secondsNow = nSecondsHistory_first + i2 * 24 * 3600;
					else
						secondsNow = nSecondsHistory_firstStartDay + (nDaysNeeded_history - 1) * 24 * 3600;
					model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
				}
			}
			else {
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
					}
					else
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
					checkMinnesAnvandning(__LINE__);
					openOK = model.weather[ii].rasterPos[i1].open(histFileName.c_str());
					if (openOK == 1) {
						errlog("Open %s okay.\n",
							histFileName.c_str());
						if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001 ||
							abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
							if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001) {
								printf("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
								errlog("ERROR! raster size longitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
							}
							if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
								errlog("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
								printf("ERROR! raster size latitude differ for weather parameter %s, %lf vs %lf. Must be the same\n",
									model.weather[ii].weatherFileTypeName, size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
							}
							model.weather[ii].errorCode = 4;
							continue;
						}
						//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
						//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
						//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
						model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), model.weather[ii].nTimeIntervals_forecast + i2, filKvot);
						//printf("history day %d changes secondsUTC from %I64d to %I64d\n", i2,
						//	model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2], secondsNow);
					}
					else {
						errlog("ERROR! Failed to open %s. I use weather data from the previous loaded file\n",
							histFileName.c_str());
						zNu = model.weather[ii].nTimeIntervals_forecast + i2;
						for (iY = 0; iY < model.weather[ii].nRows; iY++) {
							for (iX = 0; iX < model.weather[ii].nCols; iX++) {
								model.weather[ii].valueCell[zNu][iX + model.weather[ii].nCols * iY] =
									model.weather[ii].valueCell[zNu - 1][iX + model.weather[ii].nCols * iY];
							}
						}

						(model.status.weatherHistoryOpenFile_fail)++;
						printf("ERROR! Failed to open %s, nFailed %d. I use weather data from the previous loaded file. secondsNow %I64d\n",
							histFileName.c_str(), model.status.weatherHistoryOpenFile_fail, secondsNow);
					}
					//printf("test %d\n", model.weather[ii].nTimeIntervals_forecast + i2);
					model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
				}
				(model.nCallsWeatherBand[ii])++;
			}
		}
		// last dateTime plus 24 hours...
		if (model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] <= model.params.UTC_secondsStart) {
			errlog("ERROR! Last timeperiod in weather %d is earlier than the starttime for the planning (%I64d %I64d). Update the forecast planning. I set it to starttime plus 1 for it to work\n", ii,
				model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1], model.params.UTC_secondsStart);
			model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] = model.params.UTC_secondsStart + 1;
		}

		nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
		model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

		errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
			ii, model.weather[ii].nTimeIntervals,
			model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);

		tidInt = 0;
		tidpHistoricalWeather = -1;
		for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

			if (i == model.weather[ii].nTimeIntervals - 1)
				maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
			else {
				// maxTid = model.weather[ii].secondsUTC[i + 1] - 1;
				maxTid = (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]) / 2;
			}
			// maxTid = (long long)(0.5 * (model.weather[ii].secondsUTC[i] +  model.weather[ii].secondsUTC[i + 1]));

			for (; tidInt < 100000; tidInt++) {
				if (i >= model.weather[ii].nTimeIntervals_forecast && tidpHistoricalWeather == -1)
					tidpHistoricalWeather = tidInt;
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
				//errlog("ii %d tidInt %d i %d maxTid %I64d sekNu %I64d\n", ii, tidInt, i, maxTid, sekNu);
			}
		}
		if (tidpHistoricalWeather == -1)
			tidpHistoricalWeather = tidInt + 1;

		if (tidpHistoricalWeather * model.weather_timeIntervall_h > model.network.tidp_startHistoricDataOnly)
			model.network.tidp_startHistoricDataOnly = tidpHistoricalWeather * model.weather_timeIntervall_h;

		//printf("tidInt %d nAlloc %d\n", tidInt, nAlloc);

		model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;
		if (model.weather[ii].nTimeIntervals_maxValue > nMaxTimeInt)
			nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;

		//if (model.boundingBox.xMin < min_lon) {
		//	min_lonUse = min_lon - 360;
		//	max_lonUse = max_lon - 360;
		//}
		//else {
		//	if (model.boundingBox.xMin >= min_lon + 360) {
		//		min_lonUse = min_lon + 360;
		//		max_lonUse = max_lon + 360;
		//	}
		//	else {
		//		min_lonUse = min_lon;
		//		max_lonUse = max_lon;
		//	}

		//}
		(model.nCallsWeatherBand[ii])++;

		//testCoordValue(ii, 141.639, -11.240);
		//testCoordValue(ii, 149.315, -21.275);

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

	printf("-- Time after weather data loaded %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	printf("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	errlog("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	//exit(0);

}

int findChannelToUse(int level, int* iNext) {
	int i1, cNr;
	for (cNr = 0; cNr < model.network.nChannels; cNr++) {
		if (model.network.channel[cNr].bastStartLevel == level) {
			*iNext = model.network.channel[cNr].bastEndLevel;
			return cNr;
		}
	}
	return -1;
}

int setNewSpeedAlt(strSpeed* speed, int baseSpeed) {
	int i, i1, posSave, indexUnder, indexOver;
	double maxSpeed, minSpeed, target, kvot;
	
	if (speed->nShip_speedSettings == model.functions.nShip_speedSettingsBase)
		return 0; // all speed settings are already used so I don't need to change them.

	if (model.params.commercialAllowedVariation > -0.001 || model.results.forecastTypeOrig > 1000)
		return 0; // commercial speed setting, I use all options from the beginning

	if (model.params.useSimulering == 1) {
		target = model.params.simulationSpeed_kmh;
		getBastSpeedPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
	}
	else
		indexUnder = -1;


	if (baseSpeed == 0) {
		if (baseSpeed + 1 < speed->nShip_speedSettings)
			maxSpeed = speed->rpmSetting_gerCalmWaterSpeed[baseSpeed + 1] + 0.01;
		else
			maxSpeed = 1e10;
		for (i = 1; i < 5 && i < model.functions.nShip_speedSettingsBase; i++) {
			if (model.functions.rpmSetting_gerCalmWaterSpeedBase[i] >= maxSpeed)
				break;
		}
		for (i1 = baseSpeed; i1 < i; i1++) {
			if (i1 - baseSpeed == indexUnder && kvot >= 0)
				set_speedSettingsFromBase(speed, i1 - baseSpeed, i1 - baseSpeed, indexOver, kvot);
			else
				set_speedSettingsFromBase(speed, i1 - baseSpeed, i1 - baseSpeed);
		}
		speed->nShip_speedSettings = i1 - baseSpeed;
	}
	else if (baseSpeed == model.functions.nShip_speedSettingsBase - 1) {
		if (speed->nShip_speedSettings - 2 >= 0)
			minSpeed = speed->rpmSetting_gerCalmWaterSpeed[speed->nShip_speedSettings - 2] - 0.01;
		else
			minSpeed = 0;
		for (i = 1; i < 5 && i < model.functions.nShip_speedSettingsBase; i++) {
			if (model.functions.rpmSetting_gerCalmWaterSpeedBase[baseSpeed - i] <= minSpeed)
				break;
		}
		for (i1 = baseSpeed - i + 1; i1 <= baseSpeed; i1++) {
			if (i1 == indexUnder && kvot >= 0)
				set_speedSettingsFromBase(speed, i1 - baseSpeed + i - 1, i1, indexOver, kvot);
			else
				set_speedSettingsFromBase(speed, i1 - baseSpeed + i - 1, i1);
		}
		speed->nShip_speedSettings = i1 - baseSpeed + i - 1;
	}
	else {
		posSave = 0;
		for (i = -2; i <= 2 && i + baseSpeed < model.functions.nShip_speedSettingsBase; i++) {
			if (i + baseSpeed < 0)
				continue;
			if (i + baseSpeed == indexUnder && kvot >= 0)
				set_speedSettingsFromBase(speed, posSave++, i + baseSpeed, indexOver, kvot);
			else
				set_speedSettingsFromBase(speed, posSave++, i + baseSpeed);
		}
		speed->nShip_speedSettings = posSave;
	}
	if (model.params.eta_naraMaxSpeed == 1) {
		if (speed->settingGerBaseSetting[speed->nShip_speedSettings - 1] < model.functions.nShip_speedSettingsBase - 1) {
			// add the higher speeds as well to make sure we can get there in time if possible
			posSave = speed->nShip_speedSettings;
			for (i = speed->settingGerBaseSetting[speed->nShip_speedSettings - 1] + 1; i < model.functions.nShip_speedSettingsBase; i++) {
				set_speedSettingsFromBase(speed, posSave++, i);
			}
			speed->nShip_speedSettings = posSave;
		}
	}


	return 0;
}

int modify_midTimeArrive(int iter) {
	int baseSpeed, indexUnder, indexOver, i1;
	double kvot, target;

	if (iter == 1) {
		if (model.params.calmWaterSpeedCompare > 0) {
			target = model.params.calmWaterSpeedCompare;
			getBastSpeedPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
		}
		else {
			if (model.params.fuelCompare > 0) {
				target = model.params.fuelCompare / 24.0;
				getBastConsumptionPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
			}
			else {
				target = model.params.calmWaterSpeedCompareUse;
				getBastSpeedPos(model.functions.nShip_speedSettingsBase, target, &indexUnder, &indexOver, &kvot);
			}
		}
		for (i1 = 0; i1 < model.network.nPhysicalLevels; i1++) {
			set_speedSettingsFromBase(&(model.functions.speedLevel[i1]), 0, indexUnder, indexOver, kvot);
			model.functions.speedLevel[i1].nShip_speedSettings = 1;
		}
		for (i1 = 0; i1 < model.network.nChannels; i1++) {
			set_speedSettingsFromBase(&(model.functions.speedChannel[i1]), 0, indexUnder, indexOver, kvot);
			model.functions.speedChannel[i1].nShip_speedSettings = 1;
			set_speedSettingsFromBase(&(model.functions.speedChannelOut[i1]), 0, indexUnder, indexOver, kvot);
			model.functions.speedChannelOut[i1].nShip_speedSettings = 1;
		}
	}
	else {
		for (int i = 0; i < model.network.nPhysicalLevels; i++) {
			if (i == 60)
				i = i;
			if (model.optPath.level[i].timeArrive >= -0.0001) {
				model.network.physicalLev[i].midTimeArrive = model.optPath.level[i].timeArrive;
				// baseSpeed = model.functions.speedLevel[i].settingGerBaseSetting[model.optPath.level[i].speedSettingNr];
				baseSpeed = model.optPath.level[i].baseSpeedSettingNr;
				setNewSpeedAlt(&(model.functions.speedLevel[i]), baseSpeed);
				//printf("%d baseSpeed %d nSpeedSettings %d nShip_speedSettingsBase %d\n", i, baseSpeed, model.functions.speedLevel[i].nShip_speedSettings,
				//	model.functions.nShip_speedSettingsBase);

			}
			else
				model.network.physicalLev[i].midTimeArrive = -1;
		}

		for (int i = 0; i < model.network.nChannels; i++) {
			if (model.network.channel[i].timeThroughChannel < 0) {
				if (model.optPath.channel[i].timeArriveThrough >= -0.0001) {
					// baseSpeed = model.functions.speedChannel[i].settingGerBaseSetting[model.optPath.channel[i].speedSettingNrThrough];
					baseSpeed = model.optPath.channel[i].speedSettingNrThrough;
					setNewSpeedAlt(&(model.functions.speedChannel[i]), baseSpeed);
				}
				if (model.optPath.channel[i].timeArriveNext >= -0.0001) {
					// baseSpeed = model.functions.speedChannelOut[i].settingGerBaseSetting[model.optPath.channel[i].speedSettingNrNext];
					baseSpeed = model.optPath.channel[i].speedSettingNrNext;
					setNewSpeedAlt(&(model.functions.speedChannelOut[i]), baseSpeed);
				}
			}
		}
	}

	return 0;
}

double estimateDistArc(int thisLevel, int pos1, int nextLevel, int pos2) {
	double x1, y1, x2, y2;
	if (thisLevel >= 0) {
		x1 = model.network.physicalLev[thisLevel].point_x[pos1];
		y1 = model.network.physicalLev[thisLevel].point_y[pos1];
	}
	else {
		x1 = model.network.channel[-thisLevel - 1].point_x[model.network.channel[-thisLevel - 1].nPoints - 1];
		y1 = model.network.channel[-thisLevel - 1].point_y[model.network.channel[-thisLevel - 1].nPoints - 1];
	}
	if (nextLevel >= 0) {
		x2 = model.network.physicalLev[nextLevel].point_x[pos2];
		y2 = model.network.physicalLev[nextLevel].point_y[pos2];
	}
	else {
		x2 = model.network.channel[-nextLevel - 1].point_x[pos2];
		y2 = model.network.channel[-nextLevel - 1].point_y[pos2];
	}
	return estimateLargeCircleDistance_km(y1, x1, y2, x2);
}

double evalEndTimeDelayAlongArc(double timeExact, int lev1, int lev2) {
	double delayFactor, distArc, speedDiff, speedNu;
	int cNr, pointNr1, pointNr2;

	if (lev1 >= 0 && lev2 >= 0) {
		delayFactor = eval_factorDelayedAlongPath(lev1, (int)timeExact, &speedDiff);
		distArc = model.network.physicalLev[lev2].distanceFromStartPosMid - model.network.physicalLev[lev1].distanceFromStartPosMid;
		speedNu = model.params.preferredSpeed_calmWater / delayFactor + speedDiff;
		if (speedNu < 0.1)
			speedNu = 0.1;
		timeExact += distArc / speedNu;
		return timeExact;
	}

	if (lev1 >= 0) {
		pointNr1 = model.params.preferredPathOrtoPos[lev1];
		if (pointNr1 < 0)
			pointNr1 = (int)(model.network.physicalLev[lev1].nPoints / 2);
		delayFactor = eval_factorDelayedAlongPath(lev1, (int)timeExact, &speedDiff);
	}
	else
		pointNr1 = 1;
	if (lev2 >= 0) {
		pointNr2 = model.params.preferredPathOrtoPos[lev2];
		if (pointNr2 < 0)
			pointNr2 = (int)(model.network.physicalLev[lev2].nPoints / 2);
		delayFactor = eval_factorDelayedAlongPath(lev2 - 1, (int)timeExact, &speedDiff);
	}
	else
		pointNr2 = 0;

	distArc = estimateDistArc(lev1, pointNr1, lev2, pointNr2);
	speedNu = model.params.preferredSpeed_calmWater / delayFactor + speedDiff;
	if (speedNu < 0.1)
		speedNu = 0.1;

	timeExact += distArc / speedNu;
	return timeExact;
}

double evalCostArc(double tid, double fuelQualityKvot, double extraAreaCostKvot) {
	double channelCost, totCost, fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, emission, safety;

	//if (thisLevel < 0 && nextLevel < 0) {
	//	channelCost = model.network.channel[-thisLevel - 1].extraCostChannel;
	//}
	//else
	channelCost = 0;
	totCost = channelCost;

	fuel_eca = model.functions.valuesNow.fuel_main * (1 - fuelQualityKvot);
	fuel_noEca = model.functions.valuesNow.fuel_main * fuelQualityKvot;
	fuel_aux = model.functions.valuesNow.fuel_aux * fuelQualityKvot;
	fuel_auxEca = model.functions.valuesNow.fuel_aux * (1 - fuelQualityKvot);
	fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
		fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price);

	emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
		fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

	safety = model.functions.valuesNow.worstStormValue *
		model.params.weightSafety.hurricane +
		model.functions.valuesNow.bowSlam *
		model.params.weightSafety.bowSlam +
		model.functions.valuesNow.greenWater *
		model.params.weightSafety.greenWater +
		model.functions.valuesNow.dynamicStability *
		model.params.weightSafety.dynamicStability +
		model.functions.valuesNow.rolling *
		model.params.weightSafety.rolling +
		model.functions.valuesNow.surfRiding *
		model.params.weightSafety.surfRiding +
		(1 - model.functions.valuesNow.feasibleSafety) *
		model.params.weightSafety.feasibleSafety +
		model.functions.valuesNow.iceCoverCost;

	if (model.functions.valuesNow.maxWaveHeight > model.functions.maxWaveHeight)
		safety += 1e12 * (1 + model.functions.valuesNow.maxWaveHeight - model.functions.maxWaveHeight);

	totCost += model.params.weightTime * model.params.priceTime * tid +
		model.params.weightFuel * fuelBase + model.params.weightSafety.base * safety +
		emission * model.params.weightEmission * model.params.scaleObjEmission;
	totCost *= (1 + extraAreaCostKvot); // obs  *kvotCost not needed here as it is multiplied in the next step;

	return totCost;
}

double get_totalExtraAreaCostKvot(int thisLevel, int pos1, int nextLevel, int pos2, double* fuelQualityKvot)
{
	double costKvot = 0, kvot;
	int i;

	//for (i = 0; i < model.nExtraNoGoAreas; i++) {
	//	kvot = get_extraAreaKvot(thisLevel, pos1, nextLevel, pos2, i, 0);
	//	costKvot += kvot * model.extraNoGoArea[i].extraCostFactor;
	//}

	if (thisLevel == -1)
		thisLevel = thisLevel;
	for (i = 0; i < model.nExtraCostAreas; i++) {
		kvot = get_extraAreaKvot(thisLevel, pos1, nextLevel, pos2, i, 1);
		if (i == 0)
			*fuelQualityKvot = 1 - kvot;
		costKvot += kvot * model.extraCostArea[i].extraCostFactor;
	}

	return costKvot;
}

double calcEndTime_withLowestCostSpeedAlongArc(int level1, int pointNr1, int level2, int pointNr2, double timeStart) {

	double tid, costNu, bastCost = 1e20, bastEndTid;
	double fuelQualityKvot, extraAreaCostKvot;
	// fuelQualityKvot = get_fuelQualityKvot(level1, pointNr1, level2, pointNr2);
	extraAreaCostKvot = get_totalExtraAreaCostKvot(level1, pointNr1, level2, pointNr2, &fuelQualityKvot);
	double distArc, delayFactor;
	double x1, y1, x2, y2, speedDiffCurrent = 0, posDiff;

	int nSpeedSettings, ii;
	double calmWaterSpeed = -1, kvotCost = 1.0;
	if (level1 >= 0)
		nSpeedSettings = model.functions.speedLevel[level1].nShip_speedSettings;
	else {
		if (level2 >= 0)
			nSpeedSettings = model.functions.speedChannelOut[-level1 - 1].nShip_speedSettings;
		else {
			nSpeedSettings = model.functions.speedChannel[-level1 - 1].nShip_speedSettings;
			kvotCost = model.network.channel[-level1 - 1].kvotCost;
		}
	}

	int timeInt = (int)round(timeStart * model.params.nTidsperioder_perH);

	for (ii = 0; ii < nSpeedSettings; ii++) {
		calmWaterSpeed = eval_calmWaterSpeed(ii, level1, level2);
		if (timeStart < model.network.tidp_startHistoricDataOnly) {
			// timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
			if (level1 >= 0 || level2 >= 0)
				tid = calcArcTimeCost(timeInt, ii, level1, level2, &calmWaterSpeed, -1);
			else
				tid = calcArcTimeCostChannel(timeInt, ii, -level1 - 1, &calmWaterSpeed);
		}
		else {
			if (ii == 0) {
				distArc = -1;
				if (level1 >= 0) {
					delayFactor = eval_factorDelayedAlongPath(level1, timeInt, &speedDiffCurrent);
					if (level2 >= 0)
						distArc = model.network.physicalLev[level1 + 1].distanceFromStartPosMid - model.network.physicalLev[level1].distanceFromStartPosMid;
					else {
						x1 = model.network.physicalLev[level1].point_x[pointNr1];
						y1 = model.network.physicalLev[level1].point_y[pointNr1];
						x2 = model.network.channel[-level2 - 1].point_x[pointNr2];
						y2 = model.network.channel[-level2 - 1].point_y[pointNr2];
					}
				}
				else {
					if (level2 < 0) {
						if (level1 == level2) {
							delayFactor = 1.0;
							distArc = model.network.channel[-level1 - 1].distance_km;
						}
						else {
							delayFactor = eval_factorDelayedAlongArc_currSpeedDiff(level1, model.network.channel[-level1 - 1].nPoints - 1,
								level2, 0, timeInt, &speedDiffCurrent, calmWaterSpeed);
							x1 = model.network.channel[-level1 - 1].point_x[model.network.channel[-level1 - 1].nPoints - 1];
							y1 = model.network.channel[-level1 - 1].point_y[model.network.channel[-level1 - 1].nPoints - 1];
							x2 = model.network.channel[-level2 - 1].point_x[0];
							y2 = model.network.channel[-level2 - 1].point_y[0];
						}
					}
					else {
						delayFactor = eval_factorDelayedAlongArc_currSpeedDiff(level1, 1, level2, pointNr2, timeInt, &speedDiffCurrent, calmWaterSpeed);
						x1 = model.network.channel[-level1 - 1].point_x[model.network.channel[-level1 - 1].nPoints - 1];
						y1 = model.network.channel[-level1 - 1].point_y[model.network.channel[-level1 - 1].nPoints - 1];
						x2 = model.network.physicalLev[level2].point_x[pointNr2];
						y2 = model.network.physicalLev[level2].point_y[pointNr2];
					}
				}
				if (distArc < 0) {
					distArc = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				}
			}
			if (level1 >= 0 || level2 >= 0)
				speedDiffCurrent = eval_speedDiffCurrent_delayedAlongArc(level1, pointNr1, level2, pointNr2, timeInt, calmWaterSpeed);
			else
				speedDiffCurrent = 0;
			tid = calcDelayedArcTimeCost(level1, level2, ii, calmWaterSpeed, -1, delayFactor, distArc, speedDiffCurrent);
			//timeExact = evalEndTimeDelayAlongArc(timeExact, i, -cNr - 1);
		}
		// costNu = evalCostArc(tid - timeStart, fuelQualityKvot, extraAreaCostKvot);
		costNu = evalCostArc(tid, fuelQualityKvot, extraAreaCostKvot); // *kvotCost;
		if (USE_KVOTKOST == 1)
			costNu *= kvotCost;

		if (model.params.useSimulering == 1) {
			if (level1 >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[level1] - pointNr1);
				costNu += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (level2 >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[level2] - pointNr2);
				costNu += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (model.params.simulationSpeed_kmh > 0)
				costNu += abs(calmWaterSpeed - model.params.simulationSpeed_kmh) * model.simulering.penDeviateSpeed_kmh;
		}


		if (costNu < bastCost) {
			bastCost = costNu;
			bastEndTid = timeStart + tid;
		}
	}

	return bastEndTid;
}

int gen_midTimeArrive_old() {
	int i, pointNr1, iNext, cNr, i1, pointNr2, pointLast, arcOK, startPos;
	double timeExact = 0, calmWaterSpeed, kvotFix, timeStart, totDist, deltaTid;
	double channelSpeed;
	double timeExactTmp;

	errlog("OBS! Calculating estimate time for the trip by following preferred path at preferred speed.\n");
	model.params.eta_naraMaxSpeed = 0;

	model.network.physicalLev[0].midTimeArrive = timeExact;
	calmWaterSpeed = model.params.preferredSpeed_calmWater;

	//model.params.preferredPathUseChannelSpeed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
	//model.params.preferredPathUseChannelConsumption = (int*)malloc2(model.network.nPhysicalLevels * sizeof(int));
	//for (i = 0; i < model.network.nPhysicalLevels; i++) {
	//	model.params.preferredPathUseChannelSpeed[i] = -1;
	//	model.params.preferredPathUseChannelConsumption[i] = -1;
	//}

	double* timeFromPreviousLevelThroughChannelFixed;

	timeFromPreviousLevelThroughChannelFixed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
	for (i = 0; i < model.network.nPhysicalLevels; i++)
		timeFromPreviousLevelThroughChannelFixed[i] = -1;

	double timeFixedChannel = 0;
	//printf("nPhysical levels %d\n", model.network.nPhysicalLevels);
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		pointNr1 = model.params.preferredPathOrtoPos[i];
		if (pointNr1 < 0)
			pointNr1 = (int)(model.network.physicalLev[i].nPoints / 2);
		//printf("lev %d pointPos %d lon/lat %.3lf %.3lf reqPrefPathFeasible %d\n", i , pointNr1, 
		//	model.network.physicalLev[i].point[pointNr1].longitude().degrees(),
		//	model.network.physicalLev[i].point[pointNr1].latitude().degrees(),
		//	model.network.physicalLev[i].requirePrefPathFeasible);


		if (model.network.physicalLev[i].requirePrefPathFeasible == 1 || model.network.physicalLev[i + 1].requirePrefPathFeasible == 1) {
			// next level can be done through a corridor instead, so try to use it
			if (model.network.physicalLev[i].requirePrefPathFeasible == 1)
				cNr = findChannelToUse(i, &iNext);
			else
				cNr = findChannelToUse(i + 1, &iNext);
			if (cNr >= 0) {
				timeStart = timeExact;
				if (timeStart < model.network.tidp_startHistoricDataOnly) {
					calcWeatherPosAlongArc(model.network.physicalLev[i].point[pointNr1], model.network.channel[cNr].point[0]);
					timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
				}
				else {
					calcWeatherPosAlongArc(model.network.physicalLev[i].point[pointNr1], model.network.channel[cNr].point[0]);
					//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
					timeExact = evalEndTimeDelayAlongArc(timeExact, i, -cNr - 1);
				}
				//printf("timeExact to get onto corridor %.3lf\n", timeExact);
				calcWeatherPosAlongChannel(cNr);
				model.network.channel[cNr].midTimeArrive = timeExact;

				timeExact = evalWeatherDataAlongArc(-cNr - 2, 0, timeExact);
				//printf("timeExact after corridor %.3lf arc from xy %.3lf %.3lf to xy %.3lf %.3lf\n", timeExact,
				//	model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].longitude().degrees(),
				//	model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].latitude().degrees(),
				//	model.network.physicalLev[iNext].point[pointNr1].longitude().degrees(), model.network.physicalLev[iNext].point[pointNr1].latitude().degrees());

				if (timeStart < model.network.tidp_startHistoricDataOnly) {
					pointNr1 = model.params.preferredPathOrtoPos[iNext];
					if (pointNr1 < 0)
						pointNr1 = -pointNr1 - 1;
					calcWeatherPosAlongArc(model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[iNext].point[pointNr1]);
					timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
				}
				else {
					pointNr1 = model.params.preferredPathOrtoPos[iNext];
					if (pointNr1 < 0)
						pointNr1 = -pointNr1 - 1;
					calcWeatherPosAlongArc(model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[iNext].point[pointNr1]);
					//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
					timeExact = evalEndTimeDelayAlongArc(timeExact, -cNr - 1, iNext);
				}
				model.network.channel[cNr].midTimeFinish = timeExact;

				deltaTid = timeExact - timeStart;
				totDist = model.network.physicalLev[iNext].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid;
				channelSpeed = model.network.channel[cNr].distance_km / model.network.channel[cNr].timeThroughChannel;
				//printf("using corridor from level %d to level %d, distance %.2lf time %.2lf. Speed %.2lf from corrDist %.2lf time %.2lf. I split the time over the levels depending on their length\n",
				//	i, iNext, totDist, deltaTid, channelSpeed, model.network.channel[cNr].distance_km, model.network.channel[cNr].timeThroughChannel);
				// splitta ut tiden map avstand fran i - 1 to iNext - 1
				for (i1 = i + 1; i1 <= iNext; i1++) {
					model.network.physicalLev[i1].midTimeArrive = timeStart + deltaTid *
						(model.network.physicalLev[i1].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid) / totDist;
					if (model.network.channel[cNr].timeThroughChannel >= 0) {
						timeFromPreviousLevelThroughChannelFixed[i1] =
							model.network.physicalLev[i1].midTimeArrive - model.network.physicalLev[i1 - 1].midTimeArrive;
						timeFixedChannel += timeFromPreviousLevelThroughChannelFixed[i1];
						//printf("level i1 %d timeFixedChannel %.2lf\n", i1, timeFixedChannel);
					}
					//printf("i1 %d seting time to %.2lf\n", i1, model.network.physicalLev[i1].midTimeArrive);
				}
				//errlog("midTimeArrive i %d timeExact %.3lf channelSpeed %.3lf\n", i, timeExact, channelSpeed);

				if (model.network.physicalLev[i].requirePrefPathFeasible == 0)
					startPos = i + 1;
				else
					startPos = i;


				i = iNext;
			}
		}

		if (i < model.network.nPhysicalLevels - 1) {
			if (timeExact < model.network.tidp_startHistoricDataOnly) {
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[i].point[pointNr1], i);
				timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
				errlog("\tforecast i %d timeExact %lf\n", i, timeExact);
			}
			else {
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[i].point[pointNr1], i);
				//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
				timeExact = evalEndTimeDelayAlongArc(timeExact, i, i + 1);
				errlog("\thistData i %d timeExact %lf\n", i, timeExact);
				//if (SKRIV_UT_NOTHING == 0) {
				//	double speedDiff, delayFactor, distArc, speedNu;
				//		delayFactor = eval_factorDelayedAlongPath(i, (int)timeExact, &speedDiff);
				//		distArc = model.network.physicalLev[i+1].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid;
				//		speedNu = model.params.preferredSpeed_calmWater / delayFactor + speedDiff;
				//		errlog("delayInfo i %d timeExact %.2lf delayFactor %.2lf speedDiff %.2lf distArc %.2lf speedNu %.2lf\n", i,
				//			timeExact, delayFactor, speedDiff, distArc, speedNu);
				//}
			}

			//errlog("midTimeArrive i %d timeExact %.3lf\n", i, timeExact);

			model.network.physicalLev[i + 1].midTimeArrive = timeExact;
			//printf("level %d midTimeArrive %.2lf\n", i + 1, model.network.physicalLev[i + 1].midTimeArrive);
			//printf("test i %d\n");
		}
	}
	//printf("test2\n");
	// model.network.physicalLev[i].midTimeArrive = timeExact;

	int pos;
	for (cNr = 0; cNr < model.network.nChannels; cNr++) {
		pos = model.network.channel[cNr].bastStartLevel;
		if (model.network.physicalLev[pos].requirePrefPathFeasible == 1) {
			if (pos >= 0) {
				arcOK = check_isPhysicalArcOK(pos, -cNr - 1, model.params.preferredPathOrtoPos[pos], 0); // a preferred path but through a channel
				if (arcOK != 1)
					arcOK = 0;
				model.network.channel[cNr].straightArcFeasible_toChannelFromPrefPath = arcOK;
			}
			pos = model.network.channel[cNr].bastEndLevel;
			if (pos >= 0) {
				arcOK = check_isPhysicalArcOK(-cNr - 1, pos, 1, model.params.preferredPathOrtoPos[pos]); // a preferred path but through a channel
				if (arcOK != 1)
					arcOK = 0;
				model.network.channel[cNr].straightArcFeasible_fromChannelToPrefPath = arcOK;
			}
		}
		//errlog("ERROR! change the below code\n");
		//model.network.channel[cNr].straightArcFeasible_toChannelFromPrefPath = 1;
		//model.network.channel[cNr].straightArcFeasible_fromChannelToPrefPath = 1;
	}

	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	time_t rawtime;
	time(&rawtime);
	char* endTime;
	endTime = (char*)malloc2(256 * sizeof(char));
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = model.params.startMinute;
	tmBas.tm_sec = 0;
	endTime = (char*)malloc2(256 * sizeof(char));
	tmBas.tm_min += timeExact * 60.0;
	time_t tidBas = mktime(&tmBas);
	if (tidBas == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	fixReadableDate(tmBas, endTime);

	double tidNu, tidPrev, varierbarTid, minKvot, maxKvot;
	//for (i = 0; i < model.network.nPhysicalLevels; i++) {
	//	errlog("midTimeArrive %d %lf\n", i, model.network.physicalLev[i].midTimeArrive);
	//}
	errlog("OLD! preferred path and speed gives ending time %.2lf: end date/time %s\n",
		timeExact, endTime);
	if (model.params.eta_h > 0) {

		varierbarTid = (timeExact - timeFixedChannel);
		if (varierbarTid < 0.1)
			varierbarTid = 0.1;
		kvotFix = (model.params.eta_h - timeFixedChannel) / varierbarTid;
		errlog("wanted ending time from eta: %.2lf kvotFix %.4lf timeFixedChannel %.3lf\n", model.params.eta_h, kvotFix, timeFixedChannel);
		if (kvotFix > 1) {
			maxKvot = model.params.preferredSpeed_calmWater / model.params.calmWaterSpeedMin;
			if (kvotFix > maxKvot) {
				errlog("ERROR! cannot decrease the speed enough to get there at eta, only allow slowest speed, giving a factor of %.4lf but we need to get up to %.4lf\n",
					maxKvot, kvotFix);
				kvotFix = maxKvot;
				model.params.etaFocus_speed = -1;
			}
			else {
				if (kvotFix > maxKvot - 0.05)
					model.params.etaFocus_speed = -1;
			}
		}
		else {
			minKvot = model.params.preferredSpeed_calmWater / model.params.calmWaterSpeedMax;
			if (kvotFix < minKvot) {
				errlog("ERROR! cannot increase the speed enough to get there at eta, only allow highest speed, giving a factor of %.4lf but we need to get down to %.4lf\n",
					minKvot, kvotFix);
				kvotFix = minKvot;
				model.params.etaFocus_speed = 1;
			}
			else {
				if (kvotFix < minKvot + 0.05)
					model.params.etaFocus_speed = 1;
			}
			if (kvotFix < minKvot + 0.05)
				model.params.eta_naraMaxSpeed = 1;
		}

		tidNu = 0;
		tidPrev = tidNu;
		for (i = 1; i < model.network.nPhysicalLevels; i++) {
			// model.network.physicalLev[i].midTimeArrive *= kvotFix;
			if (timeFromPreviousLevelThroughChannelFixed[i] > 0) {
				tidNu += timeFromPreviousLevelThroughChannelFixed[i];
			}
			else {
				tidNu += (model.network.physicalLev[i].midTimeArrive -
					model.network.physicalLev[i - 1].midTimeArrive) * kvotFix;
				//printf("tidNu %.2lf nu %.2lf prev %.2lf kvot %.2lf\n", tidNu, 
				//	model.network.physicalLev[i].midTimeArrive,
				//	model.network.physicalLev[i - 1].midTimeArrive, kvotFix);
			}
			model.network.physicalLev[i - 1].midTimeArrive = tidPrev;
			tidPrev = tidNu;
		}
		model.network.physicalLev[i - 1].midTimeArrive = tidPrev;
		// model.network.physicalLev[i].midTimeArrive *= kvotFix;
		tmBas.tm_min += (model.network.physicalLev[i - 1].midTimeArrive - timeExact) * 60.0;
		time_t tidBas = mktime(&tmBas);
		if (tidBas == -1) {
			printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
				tmBas.tm_year,
				tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
			if (model.params.failedTime == 0)
				postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
			model.params.failedTime = 1;
		}
		fixReadableDate(tmBas, endTime);
		//for (i = 0; i < model.network.nPhysicalLevels; i++) {
		//	errlog("midTimeArrive_eta %d %lf\n", i, model.network.physicalLev[i].midTimeArrive);
		//}
		errlog("using eta gives ending time %.2lf: end date/time %s\n",
			model.network.physicalLev[i - 1].midTimeArrive, endTime);
	}
	free(endTime);
	free(timeFromPreviousLevelThroughChannelFixed);


	return 0;
}

int gen_midTimeArrive() {
	int i, pointNr1, iNext, cNr, i1, pointNr2, pointLast, arcOK, startPos;
	double timeExact = 0, kvotFix, timeStart, totDist, deltaTid;
	double channelSpeed;
	double timeExactTmp;

	errlog("\nOBS! Calculating estimate time for the trip by following preferred path at preferred speed.\n");
	model.params.eta_naraMaxSpeed = 0;

	model.network.physicalLev[0].midTimeArrive = timeExact;

	//model.params.preferredPathUseChannelSpeed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
	//model.params.preferredPathUseChannelConsumption = (int*)malloc2(model.network.nPhysicalLevels * sizeof(int));
	//for (i = 0; i < model.network.nPhysicalLevels; i++) {
	//	model.params.preferredPathUseChannelSpeed[i] = -1;
	//	model.params.preferredPathUseChannelConsumption[i] = -1;
	//}

	double* timeFromPreviousLevelThroughChannelFixed;

	timeFromPreviousLevelThroughChannelFixed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
	for (i = 0; i < model.network.nPhysicalLevels; i++)
		timeFromPreviousLevelThroughChannelFixed[i] = -1;

	double timeFixedChannel = 0;
	//printf("nPhysical levels %d\n", model.network.nPhysicalLevels);
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		pointNr1 = model.params.preferredPathOrtoPos[i];
		if (pointNr1 < 0)
			pointNr1 = (int)(model.network.physicalLev[i].nPoints / 2);
		//printf("lev %d pointPos %d lon/lat %.3lf %.3lf reqPrefPathFeasible %d\n", i , pointNr1, 
		//	model.network.physicalLev[i].point[pointNr1].longitude().degrees(),
		//	model.network.physicalLev[i].point[pointNr1].latitude().degrees(),
		//	model.network.physicalLev[i].requirePrefPathFeasible);

		if (model.network.physicalLev[i].requirePrefPathFeasible == 1 || model.network.physicalLev[i + 1].requirePrefPathFeasible == 1) {
			// next level can be done through a corridor instead, so try to use it
			if (model.network.physicalLev[i].requirePrefPathFeasible == 1)
				cNr = findChannelToUse(i, &iNext);
			else
				cNr = findChannelToUse(i + 1, &iNext);
			if (cNr >= 0) {
				timeStart = timeExact;
				if (timeStart < model.network.tidp_startHistoricDataOnly) {
					calcWeatherPosAlongArc(model.network.physicalLev[i].point[pointNr1], model.network.channel[cNr].point[0]);
					// timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, -cNr - 1, 0, timeExact);
				}
				else {
					calcWeatherPosAlongArc(model.network.physicalLev[i].point[pointNr1], model.network.channel[cNr].point[0]);
					//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
					//timeExact = evalEndTimeDelayAlongArc(timeExact, i, -cNr - 1);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, -cNr - 1, 0, timeExact);
				}
				//printf("timeExact to get onto corridor %.3lf\n", timeExact);
				calcWeatherPosAlongChannel(cNr);

				//timeExact = evalWeatherDataAlongArc(-cNr - 2, 0, timeExact);
				timeExact = calcEndTime_withLowestCostSpeedAlongArc(-cNr - 1, 0, -cNr - 1, model.network.channel[cNr].nPoints - 1, timeExact);
				//printf("timeExact after corridor %.3lf arc from xy %.3lf %.3lf to xy %.3lf %.3lf\n", timeExact,
				//	model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].longitude().degrees(),
				//	model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].latitude().degrees(),
				//	model.network.physicalLev[iNext].point[pointNr1].longitude().degrees(), model.network.physicalLev[iNext].point[pointNr1].latitude().degrees());

				if (timeStart < model.network.tidp_startHistoricDataOnly) {
					pointNr1 = model.params.preferredPathOrtoPos[iNext];
					if (pointNr1 < 0)
						pointNr1 = -pointNr1 - 1;
					calcWeatherPosAlongArc(model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[iNext].point[pointNr1]);
					// timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(-cNr - 1, model.network.channel[cNr].nPoints - 1, iNext, pointNr1, timeExact);
				}
				else {
					pointNr1 = model.params.preferredPathOrtoPos[iNext];
					if (pointNr1 < 0)
						pointNr1 = -pointNr1 - 1;
					calcWeatherPosAlongArc(model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[iNext].point[pointNr1]);
					//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
					//timeExact = evalEndTimeDelayAlongArc(timeExact, -cNr - 1, iNext);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(-cNr - 1, model.network.channel[cNr].nPoints - 1, iNext, pointNr1, timeExact);

				}

				deltaTid = timeExact - timeStart;
				totDist = model.network.physicalLev[iNext].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid;
				channelSpeed = model.network.channel[cNr].distance_km / model.network.channel[cNr].timeThroughChannel;
				//printf("using corridor from level %d to level %d, distance %.2lf time %.2lf. Speed %.2lf from corrDist %.2lf time %.2lf. I split the time over the levels depending on their length\n",
				//	i, iNext, totDist, deltaTid, channelSpeed, model.network.channel[cNr].distance_km, model.network.channel[cNr].timeThroughChannel);
				// splitta ut tiden map avstand fran i - 1 to iNext - 1
				for (i1 = i + 1; i1 <= iNext; i1++) {
					model.network.physicalLev[i1].midTimeArrive = timeStart + deltaTid *
						(model.network.physicalLev[i1].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid) / totDist;
					if (model.network.channel[cNr].timeThroughChannel >= 0) {
						timeFromPreviousLevelThroughChannelFixed[i1] =
							model.network.physicalLev[i1].midTimeArrive - model.network.physicalLev[i1 - 1].midTimeArrive;
						timeFixedChannel += timeFromPreviousLevelThroughChannelFixed[i1];
						//printf("level i1 %d timeFixedChannel %.2lf\n", i1, timeFixedChannel);
					}
					//printf("i1 %d seting time to %.2lf\n", i1, model.network.physicalLev[i1].midTimeArrive);
				}
				//errlog("midTimeArrive i %d timeExact %.3lf channelSpeed %.3lf\n", i, timeExact, channelSpeed);

				if (model.network.physicalLev[i].requirePrefPathFeasible == 0)
					startPos = i + 1;
				else
					startPos = i;


				i = iNext;
			}
		}

		if (i < model.network.nPhysicalLevels - 1) {
			pointNr2 = model.params.preferredPathOrtoPos[i + 1];
			if (pointNr2 < 0)
				pointNr2 = (int)(model.network.physicalLev[i + 1].nPoints / 2);
			if (timeExact < model.network.tidp_startHistoricDataOnly) {
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[i].point[pointNr1], i);
				//timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
				timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, i + 1, pointNr2, timeExact);
			}
			else {
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[i].point[pointNr1], i);
				//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
				// timeExact = evalEndTimeDelayAlongArc(timeExact, i, i + 1);
				timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, i + 1, pointNr2, timeExact);
			}

			//errlog("midTimeArrive i %d timeExact %.3lf\n", i, timeExact);

			model.network.physicalLev[i + 1].midTimeArrive = timeExact;
			//printf("level %d midTimeArrive %.2lf\n", i + 1, model.network.physicalLev[i + 1].midTimeArrive);
			//printf("test i %d\n");
		}
		if (timeExact > 99999)
			timeExact = timeExact;
	}
	//printf("test2\n");
	// model.network.physicalLev[i].midTimeArrive = timeExact;


	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	time_t rawtime;
	time(&rawtime);
	char* endTime;
	endTime = (char*)malloc2(256 * sizeof(char));
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = model.params.startMinute;
	tmBas.tm_sec = 0;
	endTime = (char*)malloc2(256 * sizeof(char));
	tmBas.tm_min += timeExact * 60.0;
	time_t tidBas = mktime(&tmBas);
	if (tidBas == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	fixReadableDate(tmBas, endTime);

	double tidNu, tidPrev;
	//for (i = 0; i < model.network.nPhysicalLevels; i++) {
	//	errlog("midTimeArrive %d %lf\n", i, model.network.physicalLev[i].midTimeArrive);
	//}
	errlog("preferred path and speed gives ending time %.2lf: end date/time %s\n",
		timeExact, endTime);
	free(endTime);
	free(timeFromPreviousLevelThroughChannelFixed);


	return 0;
}


int gen_midTimeArrive_forecast() {
	int i, pointNr1, iNext, cNr, i1, pointNr2, pointLast, arcOK, startPos;
	double timeExact = 0, kvotFix, timeStart, totDist, deltaTid;
	double channelSpeed;
	double timeExactTmp;

	errlog("\nOBS! Calculating estimate time for the trip by following preferred path at preferred speed.\n");
	model.params.eta_naraMaxSpeed = 0;

	model.network.physicalLev[0].midTimeArrive = timeExact;

	//model.params.preferredPathUseChannelSpeed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
	//model.params.preferredPathUseChannelConsumption = (int*)malloc2(model.network.nPhysicalLevels * sizeof(int));
	//for (i = 0; i < model.network.nPhysicalLevels; i++) {
	//	model.params.preferredPathUseChannelSpeed[i] = -1;
	//	model.params.preferredPathUseChannelConsumption[i] = -1;
	//}

	double* timeFromPreviousLevelThroughChannelFixed;

	timeFromPreviousLevelThroughChannelFixed = (double*)malloc2(model.network.nPhysicalLevels * sizeof(double));
	for (i = 0; i < model.network.nPhysicalLevels; i++)
		timeFromPreviousLevelThroughChannelFixed[i] = -1;

	double timeFixedChannel = 0;

	timeExact = model.iterKaoutar.startTimeUse;
	//printf("nPhysical levels %d\n", model.network.nPhysicalLevels);
	for (i = model.iterKaoutar.physLevelStartUse; i < model.network.nPhysicalLevels - 1; i++) {
		pointNr1 = model.params.preferredPathOrtoPos[i];
		if (pointNr1 < 0)
			pointNr1 = (int)(model.network.physicalLev[i].nPoints / 2);
		//printf("lev %d pointPos %d lon/lat %.3lf %.3lf reqPrefPathFeasible %d\n", i , pointNr1, 
		//	model.network.physicalLev[i].point[pointNr1].longitude().degrees(),
		//	model.network.physicalLev[i].point[pointNr1].latitude().degrees(),
		//	model.network.physicalLev[i].requirePrefPathFeasible);

		if (model.network.physicalLev[i].requirePrefPathFeasible == 1 || model.network.physicalLev[i + 1].requirePrefPathFeasible == 1) {
			// next level can be done through a corridor instead, so try to use it
			if (model.network.physicalLev[i].requirePrefPathFeasible == 1)
				cNr = findChannelToUse(i, &iNext);
			else
				cNr = findChannelToUse(i + 1, &iNext);
			if (cNr >= 0) {
				timeStart = timeExact;
				if (timeStart < model.network.tidp_startHistoricDataOnly) {
					calcWeatherPosAlongArc(model.network.physicalLev[i].point[pointNr1], model.network.channel[cNr].point[0]);
					// timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, -cNr - 1, 0, timeExact);
				}
				else {
					calcWeatherPosAlongArc(model.network.physicalLev[i].point[pointNr1], model.network.channel[cNr].point[0]);
					//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
					//timeExact = evalEndTimeDelayAlongArc(timeExact, i, -cNr - 1);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, -cNr - 1, 0, timeExact);
				}
				//printf("timeExact to get onto corridor %.3lf\n", timeExact);
				calcWeatherPosAlongChannel(cNr);

				//timeExact = evalWeatherDataAlongArc(-cNr - 2, 0, timeExact);
				timeExact = calcEndTime_withLowestCostSpeedAlongArc(-cNr - 1, 0, -cNr - 1, model.network.channel[cNr].nPoints - 1, timeExact);
				//printf("timeExact after corridor %.3lf arc from xy %.3lf %.3lf to xy %.3lf %.3lf\n", timeExact,
				//	model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].longitude().degrees(),
				//	model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1].latitude().degrees(),
				//	model.network.physicalLev[iNext].point[pointNr1].longitude().degrees(), model.network.physicalLev[iNext].point[pointNr1].latitude().degrees());

				if (timeStart < model.network.tidp_startHistoricDataOnly) {
					pointNr1 = model.params.preferredPathOrtoPos[iNext];
					if (pointNr1 < 0)
						pointNr1 = -pointNr1 - 1;
					calcWeatherPosAlongArc(model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[iNext].point[pointNr1]);
					// timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(-cNr - 1, model.network.channel[cNr].nPoints - 1, iNext, pointNr1, timeExact);
				}
				else {
					pointNr1 = model.params.preferredPathOrtoPos[iNext];
					if (pointNr1 < 0)
						pointNr1 = -pointNr1 - 1;
					calcWeatherPosAlongArc(model.network.channel[cNr].point[model.network.channel[cNr].nPoints - 1],
						model.network.physicalLev[iNext].point[pointNr1]);
					//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
					//timeExact = evalEndTimeDelayAlongArc(timeExact, -cNr - 1, iNext);
					timeExact = calcEndTime_withLowestCostSpeedAlongArc(-cNr - 1, model.network.channel[cNr].nPoints - 1, iNext, pointNr1, timeExact);

				}

				deltaTid = timeExact - timeStart;
				totDist = model.network.physicalLev[iNext].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid;
				channelSpeed = model.network.channel[cNr].distance_km / model.network.channel[cNr].timeThroughChannel;
				//printf("using corridor from level %d to level %d, distance %.2lf time %.2lf. Speed %.2lf from corrDist %.2lf time %.2lf. I split the time over the levels depending on their length\n",
				//	i, iNext, totDist, deltaTid, channelSpeed, model.network.channel[cNr].distance_km, model.network.channel[cNr].timeThroughChannel);
				// splitta ut tiden map avstand fran i - 1 to iNext - 1
				for (i1 = i + 1; i1 <= iNext; i1++) {
					model.network.physicalLev[i1].midTimeArrive = timeStart + deltaTid *
						(model.network.physicalLev[i1].distanceFromStartPosMid - model.network.physicalLev[i].distanceFromStartPosMid) / totDist;
					if (model.network.channel[cNr].timeThroughChannel >= 0) {
						timeFromPreviousLevelThroughChannelFixed[i1] =
							model.network.physicalLev[i1].midTimeArrive - model.network.physicalLev[i1 - 1].midTimeArrive;
						timeFixedChannel += timeFromPreviousLevelThroughChannelFixed[i1];
						//printf("level i1 %d timeFixedChannel %.2lf\n", i1, timeFixedChannel);
					}
					//printf("i1 %d seting time to %.2lf\n", i1, model.network.physicalLev[i1].midTimeArrive);
				}
				//errlog("midTimeArrive i %d timeExact %.3lf channelSpeed %.3lf\n", i, timeExact, channelSpeed);

				if (model.network.physicalLev[i].requirePrefPathFeasible == 0)
					startPos = i + 1;
				else
					startPos = i;


				i = iNext;
			}
		}

		if (i < model.network.nPhysicalLevels - 1) {
			pointNr2 = model.params.preferredPathOrtoPos[i + 1];
			if (pointNr2 < 0)
				pointNr2 = (int)(model.network.physicalLev[i + 1].nPoints / 2);
			if (timeExact < model.network.tidp_startHistoricDataOnly) {
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[i].point[pointNr1], i);
				//timeExact = evalWeatherDataAlongArc(-1, 0, timeExact);
				timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, i + 1, pointNr2, timeExact);
			}
			else {
				calcWeatherPosAlongpreferredPathArc(model.network.physicalLev[i].point[pointNr1], i);
				//timeExactTmp = evalWeatherDataAlongArc(-1, 0, timeExact);
				// timeExact = evalEndTimeDelayAlongArc(timeExact, i, i + 1);
				timeExact = calcEndTime_withLowestCostSpeedAlongArc(i, pointNr1, i + 1, pointNr2, timeExact);
			}

			//errlog("midTimeArrive i %d timeExact %.3lf\n", i, timeExact);

			model.network.physicalLev[i + 1].midTimeArrive = timeExact;
			//printf("level %d midTimeArrive %.2lf\n", i + 1, model.network.physicalLev[i + 1].midTimeArrive);
			//printf("test i %d\n");
		}
		if (timeExact > 99999)
			timeExact = timeExact;
	}
	//printf("test2\n");
	// model.network.physicalLev[i].midTimeArrive = timeExact;


	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	time_t rawtime;
	time(&rawtime);
	char* endTime;
	endTime = (char*)malloc2(256 * sizeof(char));
	tmBas = *localtime(&rawtime);
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1; // sep
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour; // 0;
	tmBas.tm_min = model.params.startMinute;
	tmBas.tm_sec = 0;
	endTime = (char*)malloc2(256 * sizeof(char));
	tmBas.tm_min += timeExact * 60.0;
	time_t tidBas = mktime(&tmBas);
	if (tidBas == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}
	fixReadableDate(tmBas, endTime);

	double tidNu, tidPrev;
	//for (i = 0; i < model.network.nPhysicalLevels; i++) {
	//	errlog("midTimeArrive %d %lf\n", i, model.network.physicalLev[i].midTimeArrive);
	//}
	errlog("preferred path and speed gives ending time %.2lf: end date/time %s\n",
		timeExact, endTime);
	free(endTime);
	free(timeFromPreviousLevelThroughChannelFixed);


	return 0;
}

int get_minMax_timeFromLevel(int level, int* min, int* max) {
	if (level >= 0) {
		if (model.network.physicalLev[level].midTimeArrive > -0.5) {
			*max = (int)(model.network.physicalLev[level].midTimeArrive + model.params.maxDiffTimeFastSlow) * model.params.nTidsperioder_perH;
			*min = (int)(model.network.physicalLev[level].midTimeArrive - model.params.maxDiffTimeFastSlow) * model.params.nTidsperioder_perH;
		}
		else {
			*min = 0;
			*max = 999999;
		}
	}
	else {
		*min = 0;
		*max = 999999;
	}
	return 0;
}

void checkSP_allPhysicalLevels() {
	int nod1, nod2, i, i1;
	double dist;
	long long Cost;
	nod2 = model.network.physicalLev[model.network.nPhysicalLevels - 1].nodDelay[0];
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		nod1 = model.network.physicalLev[i].nodDelay[model.params.preferredPathOrtoPos[i]];
		if (nod1 >= 0) {
			dist = NystaUppBV_MassTest(&modelDelay, 1, nod2, nod1, &Cost);
			printf("level %d to end: dist %.2lf cost %I64d\n", i, dist, Cost);
		}
		else
			printf("level %d no nodDelay\n", i);
	}
	for (i = 0; i < model.network.nChannels; i++) {
		for (i1 = 0; i1 < 2; i1++) {
			nod1 = model.network.channel[i].nodDelay[i1];
			if (nod1 >= 0) {
				dist = NystaUppBV_MassTest(&modelDelay, 1, nod2, nod1, &Cost);
				printf("channel %d pos %d to end: dist %.2lf cost %I64d\n", i, i1, dist, Cost);
			}
			else
				printf("channel %d pos %d no nodDelay\n", i, i1);
		}
	}
	i = i;
}

void loadWeatherFiles_redis() {
	int xPos0, xPos1, yPos0, yPos1, nBands, ii;
	int pos2, pos3, i4, i5, nAlloc, i3, offset_x, offset_y;
	int nCols, nRows, nRows_inBlock, nCols_inBlock;
	int yStartBlock, yStartValue, nY_valueAdd, yEndBlock;
	int xStartBlock, xStartValue, nX_valueAdd, xEndBlock, i;
	int xBlockNr, xBlockUse, yBlockNr, pos, latPos, lonPos, tidInt;
	int nTimeIntervals_forecast_redis, nTimeIntervals_redis, forstaOverT, startT;
	int nDefault, nNoll, nTot;
	int nMaxTimeInt = 0, tidpHistoricalWeather, onBoardDef = 1;

	long long maxTid, sekNu, offset_x2;
	double min_lon, max_lon, min_lat, max_lat, min_lonUse, max_lonUse;
	double size_col, size_row, xPosFrac, yPosFrac, lat, lon;
	double xPosFrac0, xPosFrac1, yPosFrac0, yPosFrac1;
	float* arrFloat;
	std::string keyID;

	//model.network.tidp_startHistoricDataOnly = 0;

#ifndef ONBOARD
	onBoardDef = 0;
	//#ifndef _WIN32_AAAAA
	auto redis = Redis("tcp://127.0.0.1:6379/1");
	std::string redisTest;
	try {
		redisTest = redis.ping();
		if (redisTest != "PONG") {
			errlog("ERROR! Redis is not running on the server. Start it and try again\n");
			printf("ERROR! Redis is not running on the server. Start it and try again\n");
			postRequest("ERROR! Redis is not running on the server. Start it and try again", 1);
		}
	}
	catch (...) {
		errlog("ERROR! Redis is not running on the server. Start it and try again\n");
		printf("ERROR! Redis is not running on the server. Start it and try again\n");
		postRequest("ERROR! Redis is not running on the server. Start it and try again", 1);
	}

	arrFloat = NULL;

	//errlog("test14\n");
	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		model.tmpTid[0] = std::chrono::high_resolution_clock::now();
		//printf("\nweather %d variable %s\n", ii, model.weather[ii].weatherFileTypeName);
		//if (ii == 3)
		//	ii = ii;
#ifdef _WIN32_AAAAA
		// identifiera vilka raster som behover oppnas, och oppna dem
		model.weather[ii].timePosToBandPos = NULL;
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
					model.weather[ii].secondsUTC = (long long*)malloc2(nBands * sizeof(long long));
					model.weather[ii].valueCell = (float**)malloc2(nBands * sizeof(float*));
					nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
					for (int i2 = 0; i2 < nBands; i2++) {
						model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
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
			model.weather[ii].secondsUTC = (long long*)malloc2(nTimeIntervals_redis * sizeof(long long));
			forstaOverT = -1;
			for (auto it = UTC.begin(); it != UTC.end(); ++it) {
				model.weather[ii].secondsUTC[pos] = it.value();
				if (model.weather[ii].secondsUTC[pos] > model.params.UTC_secondsStart && forstaOverT == -1) {
					forstaOverT = pos;
				}
				pos++;

			}
		}
		else {
			errlog("ERROR! Failed to read metaData from redis for weather variable %s\n",
				model.weather[ii].weatherFileTypeName);
			postRequest("ERROR! Failed to read metaData from redis for weather variable " + std::string(model.weather[ii].weatherFileTypeName), 1);
		}

		if (forstaOverT <= 0) {
			if (forstaOverT == 0) {
				errlog("ERROR! %s Start planning a route at UTC second %I64d but weather data for %d only starts at %I64d, diff %.2lf hours \n",
					model.weather[ii].weatherFileTypeName, model.params.UTC_secondsStart, ii, model.weather[ii].secondsUTC[0],
					(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[0]) / 3600.0);
				startT = 0;
			}
			else {
				errlog("ERROR! Start planning a route at UTC second %I64d but last weather data for parameter %d is %I64d, diff %.2lf hours \n",
					model.params.UTC_secondsStart, ii, model.weather[ii].secondsUTC[pos - 1],
					(model.params.UTC_secondsStart - model.weather[ii].secondsUTC[pos - 1]) / 3600.0);
				startT = pos - 1;
			}
		}
		else {
			startT = forstaOverT - 1;
		}

		if (startT > 0) {
			for (i = startT; i < nTimeIntervals_redis; i++)
				model.weather[ii].secondsUTC[i - startT] = model.weather[ii].secondsUTC[i];
		}

		model.weather[ii].nTimeIntervals_forecast = nTimeIntervals_forecast_redis - startT;
		if (model.weather[ii].nTimeIntervals_forecast < 0)
			model.weather[ii].nTimeIntervals_forecast = 0;
		model.weather[ii].nTimeIntervals = nTimeIntervals_redis - startT;

		//if (model.weather[ii].nTimeIntervals_forecast * model.weather_timeIntervall_h > model.network.tidp_startHistoricDataOnly)
		//	model.network.tidp_startHistoricDataOnly = model.weather[ii].nTimeIntervals_forecast * model.weather_timeIntervall_h;


		//printf("weather %d nTredis %d startT %d nTimeIntUse %d nTimeInteUseForecast %d\n", ii, nTimeIntervals_redis, startT,
		//	model.weather[ii].nTimeIntervals, model.weather[ii].nTimeIntervals_forecast);

		// last dateTime plus 24 hours...
		if (model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] <= model.params.UTC_secondsStart) {
			errlog("ERROR! Last timeperiod in weather %d is earlier than the starttime for the planning (%I64d %I64d). Update the forecast planning. I set it to starttime plus 1 for it to work\n", ii,
				model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1], model.params.UTC_secondsStart);
			model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] = model.params.UTC_secondsStart + 1;
		}
		nAlloc = (int)((3600 * 24 + model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals - 1] - model.params.UTC_secondsStart) / 3600 / model.weather_timeIntervall_h) + 2;
		//nAlloc = (int)((model.weather[ii].nTimeIntervals - model.weather[ii].nTimeIntervals_forecast) * 24 / model.weather[ii].timeIntervall_h);
		//if (model.params.UTC_secondsStart < model.weather[ii].secondsUTC[0]) {
		//	nAlloc += (int)((model.weather[ii].secondsUTC[0] - model.params.UTC_secondsStart) / 3600 / model.weather[ii].timeIntervall_h) + 1;
		//}
		//nAlloc += model.weather[ii].nTimeIntervals_forecast + 10; // a safety buffer of 10 in case we get one or two extra...

		//printf("alloc %d for timeIntervalIndex for %s\n", nAlloc, model.weather[ii].weatherFileTypeName);
		model.weather[ii].timeIntervalIndex = (int*)malloc2(nAlloc * sizeof(int));

		errlog("weather %d nTimeIntervals %d nTimeIntForecast %d timeIntervall_h %.2lf nAlloc %d\n",
			ii, model.weather[ii].nTimeIntervals,
			model.weather[ii].nTimeIntervals_forecast, model.weather_timeIntervall_h, nAlloc);
		tidInt = 0;
		tidpHistoricalWeather = -1;
		for (i = 0; i < model.weather[ii].nTimeIntervals; i++) {

			if (i == model.weather[ii].nTimeIntervals - 1)
				maxTid = model.weather[ii].secondsUTC[i] + 3600 * 24 - 1;
			else {
				// maxTid = model.weather[ii].secondsUTC[i + 1] - 1;
				maxTid = (model.weather[ii].secondsUTC[i] + model.weather[ii].secondsUTC[i + 1]) / 2;
			}
			for (; tidInt < 100000; tidInt++) {
				if (i >= model.weather[ii].nTimeIntervals_forecast && tidpHistoricalWeather == -1)
					tidpHistoricalWeather = tidInt;
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
				//errlog("ii %d tidInt %d i %d maxTid %I64d sekNu %I64d\n", ii, tidInt, i, maxTid, sekNu);
			}
			//printf("weather %d i %d tidInt %d (over maxTid) sekNu %I64d maxSec %I64d\n", ii, i, tidInt, sekNu, maxTid);
		}
		if (tidpHistoricalWeather == -1)
			tidpHistoricalWeather = tidInt + 1;

		if (tidpHistoricalWeather * model.weather_timeIntervall_h > model.network.tidp_startHistoricDataOnly)
			model.network.tidp_startHistoricDataOnly = tidpHistoricalWeather * model.weather_timeIntervall_h;

		//printf("tidInt %d nAlloc %d\n", tidInt, nAlloc);

		model.weather[ii].nTimeIntervals_maxValue = tidInt - 1;
		if (model.weather[ii].nTimeIntervals_maxValue > nMaxTimeInt)
			nMaxTimeInt = model.weather[ii].nTimeIntervals_maxValue;


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
		xPos0 = roundDown(xPosFrac);

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
		yPos0 = roundDown(yPosFrac);
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

		model.weather[ii].valueCell = (float**)malloc2(nBands * sizeof(float*));
		nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
		//printf("ii %d nCols/nRows %d %d nAlloc %d nBands %d fromToX %.3lf %.3lf size %.3lf\n", ii, 
		//	model.weather[ii].nCols, model.weather[ii].nRows, nAlloc, nBands,
		//	model.boundingBox.xMin, model.boundingBox.xMax, size_col);
		//errlog("ERROR! Take away the code below that sets the valueCell to 99999, codeline %d\n", __LINE__);
		//printf("nAlloc %d from %d %d nBands %d nColsInBlockXY %d %d\n", nAlloc, model.weather[ii].nCols, model.weather[ii].nRows, nBands,
		//	nCols_inBlock, nRows_inBlock);
		//printf("variable %d allocs %d %d (from %d %d)\n", ii, nBands, nAlloc, model.weather[ii].nCols,  model.weather[ii].nRows);
		for (int i2 = 0; i2 < nBands; i2++) {
			//printf("bandNr %d\n", i2);
			//printf("ii %d i2 %d\n", ii, i2);
			model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
			//for (int i3 = 0; i3 < nAlloc; i3++)
			//	model.weather[ii].valueCell[i2][i3] = 99999.0;
		}

		//nAlloc = nBands * nCols_inBlock * nRows_inBlock;
		nAlloc = nTimeIntervals_redis * nCols_inBlock * nRows_inBlock;
		//printf("nAlloc %d\n", nAlloc);
		if (ii > 0)
			delete arrFloat;
		arrFloat = new float[nAlloc];
		//printf("blockAlloc nBands %d nTimeInt_redis %d ncols %d nrows %d nAlloc %d totSize %d\n",
		//	nBands, nTimeIntervals_redis, nCols_inBlock, nRows_inBlock, nAlloc, nAlloc * sizeof(float));

		//printf("nAlloc2 %d\n", nAlloc);



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
		}
		else
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
		//printf("xPosFrac0 %.3lf xPos0 %d xPosFrac1 %.3lf xPos1 %d\n",
		//	xPosFrac0, xPos0, xPosFrac1, xPos1);

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
				keyID += std::to_string(pos);
				// printf("redis call %s, xBlockNr %d yBlockNr %d\n", keyID.c_str(), xBlockNr, yBlockNr);

				auto value = redis.get(keyID);
				if (value) {
					//cout << "size " << value->size();
					memcpy(arrFloat, value->data(), value->size());
					//printf("fore exit\n");
					//exitKontrollerat(__LINE__);
					//printf("efter exit\n");
				}
				else {
					printf("ERROR! Failed to load keyID %s\n", keyID.c_str());
					errlog("ERROR! Failed to load keyID %s. Try to set redis keys again. I quit\n", keyID.c_str());
					postRequest("ERROR! Failed to load keyID " + keyID + ". Try to set redis keys again.I quit", 1);
				}

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
				//	printf("i4 %d %d min/max posCell %d %d i5 %d %d min/max posCell %d %d bands %d %d saves %d %d\n", yStartBlock, yEndBlock - 1,
				//		yStartBlock + offset_y, yEndBlock - 1 + offset_y, xStartBlock, xEndBlock - 1,
				//		xStartBlock + offset_x, xEndBlock - 1 + offset_x, startT, nTimeIntervals_redis -1, 0, nTimeIntervals_redis - startT-1);
				//printf("## blockStartX %.3lf valueCellStartX %.3lf blockStartX+360 %.3lf\n",
				//	min_lonUse + xBlockUse * nCols_inBlock * size_col,
				//	model.weather[ii].minX + offset_x * size_col,
				//	min_lonUse + xBlockUse * nCols_inBlock * size_col + 360);
				int nXLoad = xEndBlock - xStartBlock;
				for (i3 = startT; i3 < nTimeIntervals_redis; i3++) {
					for (i4 = yStartBlock; i4 < yEndBlock; i4++) {

						pos2 = xStartBlock + nCols_inBlock * (i4 + nRows_inBlock * i3);
						pos3 = xStartBlock + offset_x + model.weather[ii].nCols * (i4 + offset_y);
						memcpy(&(model.weather[ii].valueCell[i3 - startT][pos3]), &(arrFloat[pos2]), sizeof(float) * nXLoad);

						/*
						for (i5 = xStartBlock; i5 < xEndBlock; i5++) {
							pos2 = i5 + nCols_inBlock * (i4 + nRows_inBlock * i3);
							pos3 = i5 + offset_x + model.weather[ii].nCols * (i4 + offset_y);
							model.weather[ii].valueCell[i3-startT][pos3] = arrFloat[pos2];
							//if (ii == 2 && arrFloat[pos2] < 999)
							//	printf("ii %d pos1 %d pos3 %d val %.3lf\n", ii, i3 - startT, pos3, arrFloat[pos2]);
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
						*/
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

		// testCoordValue(ii, 86.14, 3.79);
		// testCoordValue(ii, 93.84, 5.98);
		//testCoordValue(ii, 167.6111, 81);
		//testCoordValue(ii, 179.6111, 82);
		//testCoordValue(ii, -179.6111, 83);
		//testCoordValue(ii, -132.39, 84);


		// om olika diskretization pa oppnade raster sa stoppa
		// 



		//model.weather[ii].valueCell = model.weather[ii].rasterPos.GetRasterBand_realArrAllBands(&(model.weather[ii].raster), model.boundingBox);
		//printf("used dim %d %d tid %lf nBands %d\n", model.weather[ii].nRows,
		//	model.weather[ii].nCols, model.durationMilli[ii], model.weather[ii].nTimeIntervals);


		/*
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
						errlog("ERROR! value of weather %d %s tidsp %d lon/lat %.3lf %.3lf i4/i5 %d %d nCols/Rows %d %d kvotxy %.3lf %.3lf is %lf\n", ii,
							model.weather[ii].weatherFileTypeName,
							ii3, lon, lat, ii4, ii5, model.weather[ii].nCols, model.weather[ii].nRows,
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
		*/


		//errlog("iicc %d ii2 %d\n", ii, ii2);
		(model.nCallsWeatherBand[ii])++;
		//errlog("weather %d variable %s nTimeInt %d dim %d %d tid %lf\nminLon %.3lf maxLon %.3lf\nminLat %.3lf maxLat %.3lf\n", ii,
		//	model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals,
		//	model.weather[ii].nRows,
		//	model.weather[ii].nCols,
		//	model.durationMilli[ii],
		//	model.weather[ii].minX, model.weather[ii].maxX,
		//	model.weather[ii].minY, model.weather[ii].maxY);

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

#endif

	if (onBoardDef == 1)
		postRequest("ERROR! onBoard is defined but OptiNav forecast is used. I quit!\n", 1);


	printf("-- Time after weather data loaded %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	printf("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	errlog("Time where only historic data is used: %d\n", model.network.tidp_startHistoricDataOnly);
	//exit(0);

}

int loadDelayFactorScale_shipSize() {

	std::ifstream fil;
	char* namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/delay/delayFactorScale_shipSize.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("ERROR! %s does not exist. I quit\n", namn);
		printf("ERROR! %s does not exist. I quit\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " does not exist.Fix it and run OptiNav again.", 1);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);
	json data, data2;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}
	fil.close();

	model.scaledDelay = -1.0;
	if (!data["waveFactorTables_scale"].is_null()) {
		data2 = data["waveFactorTables_scale"];

		for (auto it = data2.begin(); it != data2.end(); ++it) {
			json dataIt = it.value();
			std::string namn = dataIt["tableID"];
			if (namn != model.functions.waveTableID_orig)
				continue;
			model.scaledDelay = dataIt["scale"];
			break;
		}
	}
	else {
		errlog("ERROR! Field waveFactorTables_scale is missing in %s. I quit\n", namn);
		printf("ERROR! Field waveFactorTables_scale is missing in %s. I quit\n", namn);
		postRequest("ERROR! Field waveFactorTables_scale is missing in " + std::string(namn) + ".Fix it and run OptiNav again.", 1);
	}
	if (model.scaledDelay < 0.1) {
		errlog("ERROR! Scaled delay for waveFactorTable %s is not a valid scale %.3lf in file %s. I quit\n", model.functions.waveTableID.c_str(), model.scaledDelay, namn);
		printf("ERROR! Scaled delay for waveFactorTable %s is not a valid scale %.3lf in file %s. I quit\n", model.functions.waveTableID.c_str(), model.scaledDelay, namn);
		postRequest("ERROR! Scaled delay for waveFactorTable " + model.functions.waveTableID + " in file " + std::string(namn) + " is not a valid scale.Fix it and run OptiNav again.", 1);
	}

	return 0;
}

int loadTimeDelayMap() {
	int xPos0, xPos1, yPos0, yPos1, nBands;
	int nAlloc, i, returnVal;
	char* namn = (char*)malloc2(256 * sizeof(char));
	int nMaxTimeInt = 0, tidInt;
	long long maxTid, sekNu, nSecondsUTC;

	double size_col, size_row, xPosFrac, yPosFrac;

	auto tid10 = std::chrono::high_resolution_clock::now();
	size_col = -1;
	model.delay.nDelayed_months = getMonthsToUseForDelay_new(model.preferredPath.totDist);
	model.delayedGrid = (strWeather*)malloc2(model.delay.nDelayed_months * sizeof(strWeather));

	for (i = 0; i < model.delay.nDelayed_months; i++) {
		sprintf(namn, "%s/%s%d.tif", model.params.indataPath.c_str(), model.params.mapTimeDelayName.c_str(),
			model.delay.delayed_monthNr[i]);
		//printf("time delay map %s\n", namn);
		model.delayedGrid[i].rasterPos = (Raster*)malloc2(sizeof(Raster));
		returnVal = model.delayedGrid[i].rasterPos[0].open(namn);
		if (returnVal == -1) {
			errlog("\n\n\n##########################################\nERROR! Delay map does not exist for month %d, ADD IT! It must exist\n",
				model.delay.delayed_monthNr[i]);
			if (returnVal == -1) {
				postRequest("Faile to open delay map " + std::string(namn) + ". Something is very wrong!", 1);
				return -1;
			}
		}

		size_col = model.delayedGrid[i].rasterPos[0].Get_sizeCol();
		model.delayedGrid[i].size_col = size_col;
		xPosFrac = (model.boundingBox.xMin - model.delayedGrid[i].rasterPos[0].Get_minLongitude()) / size_col;
		xPos0 = roundDown(xPosFrac);
		model.delayedGrid[i].minX = model.delayedGrid[i].rasterPos[0].Get_minLongitude() +
			xPos0 * size_col;
		xPosFrac = (model.boundingBox.xMax - model.delayedGrid[i].minX) /
			size_col;
		xPos1 = roundUp(xPosFrac);
		model.delayedGrid[i].maxX = model.delayedGrid[i].minX +
			xPos1 * size_col;
		model.delayedGrid[i].nCols = xPos1 + 1;


		size_row = model.delayedGrid[i].rasterPos[0].Get_sizeRow();
		model.delayedGrid[i].size_row = size_row;
		yPosFrac = (model.delayedGrid[i].rasterPos[0].Get_maxLatitude() - model.boundingBox.yMax) /
			size_row;
		yPos0 = roundDown(yPosFrac);
		model.delayedGrid[i].maxY = model.delayedGrid[i].rasterPos[0].Get_maxLatitude() -
			yPos0 * size_row;
		yPosFrac = (model.delayedGrid[i].maxY - model.boundingBox.yMin) /
			size_row;
		yPos1 = roundUp(yPosFrac);
		if (yPos1 >= model.delayedGrid[i].rasterPos[0].Get_nRows())
			yPos1 = model.delayedGrid[i].rasterPos[i].Get_nRows() - 1;
		model.delayedGrid[i].minY = model.delayedGrid[i].maxY -
			yPos1 * size_row;
		model.delayedGrid[i].nRows = yPos1 + 1;

		nBands = model.delayedGrid[i].rasterPos[0].Get_nBands();
		model.delayedGrid[i].nTimeIntervals = nBands;
		model.delayedGrid[i].secondsUTC = NULL;
		model.delayedGrid[i].valueCell = (float**)malloc2(nBands * sizeof(float*));
		nAlloc = model.delayedGrid[i].nCols * model.delayedGrid[i].nRows;
		for (int i2 = 0; i2 < nBands; i2++) {
			model.delayedGrid[i].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
			for (int i3 = 0; i3 < nAlloc; i3++)
				model.delayedGrid[i].valueCell[i2][i3] = 9999;
		}
		model.delayedGrid[i].timePosToBandPos = NULL;

		model.delayedGrid[i].rasterPos[0].GetRasterValues_realAllBands(&(model.delayedGrid[i]), 0, 1.0);
	}

	free(namn);

	loadDelayFactorScale_shipSize();


	printf("read delay map  took %.3lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tid10));

	/*
	double lat = -40.8;
	double lon = -62.9;
	double rowDbl = (model.delayedGrid.maxY - lat) / model.delayedGrid.size_row;
	double colDbl = get_colDblFromWeatherFile(-1, lon);
	int i4 = (int)rowDbl;
	int i5 = (int)colDbl;
	for (int i3 = 0; i3 < model.delayedGrid.nTimeIntervals && i3 < 10; i3++) {
		printf("delaygrid lon/lat %.2lf %.2lf i345 %d %d %d val %.4f opposite y %.4f\n", lon, lat, i3, i4, i5,
			model.delayedGrid.valueCell[i3][i5 + model.delayedGrid.nCols * i4],
			model.delayedGrid.valueCell[i3][i5 + model.delayedGrid.nCols * (model.delayedGrid.nRows - i4 - 1)]);
	}
	*/



	return 0;
}

int loadCurrentAverageMaps() {
	int xPos0, xPos1, yPos0, yPos1, nBands;
	int nAlloc, i, returnVal;
	char* namn = (char*)malloc2(256 * sizeof(char));
	int nMaxTimeInt = 0, tidInt, i0;
	long long maxTid, sekNu, nSecondsUTC;

	double size_col, size_row, xPosFrac, yPosFrac;
	double filKvot = 3.6;

	auto tid10 = std::chrono::high_resolution_clock::now();
	size_col = -1;
	for (i = 0; i < 2; i++)
		model.delayedCurrent[i] = (strWeather*)malloc2(model.delay.nDelayed_months * sizeof(strWeather));

	for (i0 = 0; i0 < 2; i0++) {
		for (i = 0; i < model.delay.nDelayed_months; i++) {
			sprintf(namn, "%s/%s%d.grb", model.params.indataPath.c_str(), model.params.map_currentDelayName[i0].c_str(),
				model.delay.delayed_monthNr[i]);
			//printf("time delay map %s\n", namn);
			model.delayedCurrent[i0][i].rasterPos = (Raster*)malloc2(sizeof(Raster));
			returnVal = model.delayedCurrent[i0][i].rasterPos[0].open(namn);
			if (returnVal == -1) {
				errlog("\n\n\n##########################################\nERROR! Historical current map %d does not exist for month %d (%s), ADD IT! It must exist\n",
					i0, model.delay.delayed_monthNr[i], namn);
				if (returnVal == -1) {
					postRequest("Faile to open historical current " + std::string(namn) + ". Something is very wrong!", 1);
					return -1;
				}
			}

			size_col = model.delayedCurrent[i0][i].rasterPos[0].Get_sizeCol();
			model.delayedCurrent[i0][i].size_col = size_col;
			xPosFrac = (model.boundingBox.xMin - model.delayedCurrent[i0][i].rasterPos[0].Get_minLongitude()) / size_col;
			xPos0 = roundDown(xPosFrac);
			model.delayedCurrent[i0][i].minX = model.delayedCurrent[i0][i].rasterPos[0].Get_minLongitude() +
				xPos0 * size_col;
			xPosFrac = (model.boundingBox.xMax - model.delayedCurrent[i0][i].minX) /
				size_col;
			xPos1 = roundUp(xPosFrac);
			model.delayedCurrent[i0][i].maxX = model.delayedCurrent[i0][i].minX +
				xPos1 * size_col;
			model.delayedCurrent[i0][i].nCols = xPos1 + 1;


			size_row = model.delayedCurrent[i0][i].rasterPos[0].Get_sizeRow();
			model.delayedCurrent[i0][i].size_row = size_row;
			yPosFrac = (model.delayedCurrent[i0][i].rasterPos[0].Get_maxLatitude() - model.boundingBox.yMax) /
				size_row;
			yPos0 = roundDown(yPosFrac);
			model.delayedCurrent[i0][i].maxY = model.delayedCurrent[i0][i].rasterPos[0].Get_maxLatitude() -
				yPos0 * size_row;
			yPosFrac = (model.delayedCurrent[i0][i].maxY - model.boundingBox.yMin) /
				size_row;
			yPos1 = roundUp(yPosFrac);
			if (yPos1 >= model.delayedCurrent[i0][i].rasterPos[0].Get_nRows())
				yPos1 = model.delayedCurrent[i0][i].rasterPos[i].Get_nRows() - 1;
			model.delayedCurrent[i0][i].minY = model.delayedCurrent[i0][i].maxY -
				yPos1 * size_row;
			model.delayedCurrent[i0][i].nRows = yPos1 + 1;

			nBands = model.delayedCurrent[i0][i].rasterPos[0].Get_nBands();
			model.delayedCurrent[i0][i].nTimeIntervals = nBands;
			model.delayedCurrent[i0][i].secondsUTC = NULL;
			model.delayedCurrent[i0][i].valueCell = (float**)malloc2(nBands * sizeof(float*));
			nAlloc = model.delayedCurrent[i0][i].nCols * model.delayedCurrent[i0][i].nRows;
			for (int i2 = 0; i2 < nBands; i2++) {
				model.delayedCurrent[i0][i].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
				for (int i3 = 0; i3 < nAlloc; i3++)
					model.delayedCurrent[i0][i].valueCell[i2][i3] = 9999;
			}

			model.delayedCurrent[i0][i].timePosToBandPos = NULL;
			model.delayedCurrent[i0][i].rasterPos[0].GetRasterValues_realAllBands(&(model.delayedCurrent[i0][i]), 0, filKvot);
		}
	}
	free(namn);

	printf("read delay map  took %.3lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tid10));

	/*
	double lat = -40.8;
	double lon = -62.9;
	double rowDbl = (model.delayedGrid.maxY - lat) / model.delayedGrid.size_row;
	double colDbl = get_colDblFromWeatherFile(-1, lon);
	int i4 = (int)rowDbl;
	int i5 = (int)colDbl;
	for (int i3 = 0; i3 < model.delayedGrid.nTimeIntervals && i3 < 10; i3++) {
		printf("delaygrid lon/lat %.2lf %.2lf i345 %d %d %d val %.4f opposite y %.4f\n", lon, lat, i3, i4, i5,
			model.delayedGrid.valueCell[i3][i5 + model.delayedGrid.nCols * i4],
			model.delayedGrid.valueCell[i3][i5 + model.delayedGrid.nCols * (model.delayedGrid.nRows - i4 - 1)]);
	}
	*/



	return 0;
}

int getHoursAfterStart_monthEnd(int month) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	tmBas.tm_year = model.params.startYear - 1900;
	tmBas.tm_mon = model.params.startMonth_nr - 1;
	tmBas.tm_mday = model.params.startDay_nr;
	tmBas.tm_hour = model.params.startHour;
	tmBas.tm_min = model.params.startMinute;
	tmBas.tm_sec = 0;
	time_t tidBas = mktime(&tmBas);
	if (tidBas == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmBas.tm_year,
			tmBas.tm_mon, tmBas.tm_mday, tmBas.tm_hour, tmBas.tm_min, tmBas.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}

	struct tm tmNy = { 0 };
	tmNy.tm_isdst = 0;
	if (month + 1 < model.params.startMonth_nr)
		tmNy.tm_year = model.params.startYear - 1900 + 1;
	else
		tmNy.tm_year = model.params.startYear - 1900;
	tmNy.tm_mon = month;
	tmNy.tm_mday = 1;
	tmNy.tm_hour = 0;
	tmNy.tm_min = 0;
	tmNy.tm_sec = 0;
	time_t tidNy = mktime(&tmNy);
	if (tidNy == -1) {
		printf("failed mktime on row %d time %d %d %d: %d %d %d\n", __LINE__,
			tmNy.tm_year,
			tmNy.tm_mon, tmNy.tm_mday, tmNy.tm_hour, tmNy.tm_min, tmNy.tm_sec);
		if (model.params.failedTime == 0)
			postRequest("Failed mktime on row " + std::to_string(__LINE__), 0);
		model.params.failedTime = 1;
	}

	double difference = difftime(tidNy, tidBas) / 3600.0;
	int diffInt = (int)difference;
	return diffInt - 1;
}

int set_tidp_ger_delayPos() {
	int i, i1, lastHour, startPos = model.network.tidp_startHistoricDataOnly;
	int nAlloc = model.delay.nDelayed_months * 31 * 24;

	model.delay.tidpHistorical_ger_delayMapNr = (int*)malloc(nAlloc * sizeof(int));
	for (i = 0; i < model.delay.nDelayed_months; i++) {
		lastHour = getHoursAfterStart_monthEnd(model.delay.delayed_monthNr[i]);
		if (lastHour - model.network.tidp_startHistoricDataOnly >= nAlloc) {
			// errlog("OBS! lastHour too big, is %d but cannot be more than %d, monthPos %d\n", lastHour, nAlloc - 1 + model.network.tidp_startHistoricDataOnly, i);
			// okay ty only one delay month to use, the one in the middle
			lastHour = nAlloc - 1 + model.network.tidp_startHistoricDataOnly;
		}
		for (i1 = startPos; i1 <= lastHour; i1++)
			model.delay.tidpHistorical_ger_delayMapNr[i1 - model.network.tidp_startHistoricDataOnly] = i;
		startPos = i1;
	}
	model.network.tidp_lastDelayTidp = lastHour;

	return 0;
}

int createTimeArcs(int runAlt)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0;
	double fuel, safety, tid, totCost, fuelQualityKvot, extraAreaCostKvot;

	model.nErrorCoordBB = 0;
	checkMinnesAnvandning(__LINE__);

	if (runAlt == 0) {
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

		for (i = 0; i < model.network.nPhysicalLevels; i++) {
			model.network.physicalLev[i].nodNr_from_pt = (int**)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int*));
			model.network.physicalLev[i].nTimeIntervals = (int*)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int));
			model.network.physicalLev[i].nAllocTimeIntervals = (int*)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int));
			model.network.physicalLev[i].timeInterval = (int**)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int*));
			if (i == 0) {
				model.network.physicalLev[i].timeInterval[0] = (int*)malloc2(sizeof(int));
				model.network.physicalLev[i].timeInterval[0][0] = model.params.startDelay_h;
				model.network.physicalLev[i].nodNr_from_pt[0] = (int*)malloc2(sizeof(int));
				model.network.physicalLev[i].nTimeIntervals[0] = 1;
				model.nAllocNoder = 50000;
				model.Noder = (strNoder*)malloc2(model.nAllocNoder * sizeof(strNoder));
				for (int i0 = 0; i0 < model.nAllocNoder; i0++)
					model.Noder[i0].UtNod = NULL;
				model.nArcs = 0;
				model.nNoder = 0;
				model.network.physicalLev[i].nodNr_from_pt[0][0] = model.nNoder;
				adderaNod(i, 0, 0);
			}
			else {
				for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
					model.network.physicalLev[i].nTimeIntervals[i1] = 0;
					model.network.physicalLev[i].nAllocTimeIntervals[i1] = 100;
					model.network.physicalLev[i].timeInterval[i1] = (int*)malloc2(
						model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));
					model.network.physicalLev[i].nodNr_from_pt[i1] = (int*)malloc2(
						model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));

				}
			}
		}
		model.nCallsWeatherBand = (int*)calloc2(model.nWeatherFiles, sizeof(int));

		if (model.params.hindCast == 0) {
			if (model.params.onboard == 0)
				loadWeatherFiles_redis();
			else
				loadWeatherFiles_onboard_grib();
			if (model.network.tidp_startHistoricDataOnly < 999999) {
				loadTimeDelayMap();
				if (delayVersion == 4)
					loadCurrentAverageMaps();
				set_tidp_ger_delayPos();
			}
		}
		else {
			printf("Loading weather files from grib\n");
			loadWeatherFiles_grib();
		}

		if (SKRIV_UT_NOTHING == 0)
			printf("-- Time after loading all data %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

		//gen_infoWeatherAroundStorms();
		if (model.params.eta_h > 0)
			gen_midTimeArrive_old();
		else {
			gen_midTimeArrive_old();
			gen_midTimeArrive();
		}

		if (SKRIV_UT_NOTHING == 0)
			printf("-- Time after creating basic solution %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

		if (delayVersion >= 3 && model.network.tidp_startHistoricDataOnly < 999999) {
			solve_SP_delay();
			if (SKRIV_UT_NOTHING == 0)
				printf("-- Time after solving SP_delay %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

			solve_SP_delayPrefPath();
			if (SKRIV_UT_NOTHING == 0) {
				printf("-- Time after solving SP_delayPrefPath %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
				// checkSP_allPhysicalLevels();
			}
		}
	}
	else {
		for (i = 0; i < model.network.nPhysicalLevels; i++) {
			if (i == 0) {
				model.nArcs = 0;
				model.nNoder = 0;
				model.network.physicalLev[i].nTimeIntervals[0] = 1;
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

	//modelDelay.filpek = fopen("checkDijkstra.txt", "w");
	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		if (i == 47)
			i = i;

		n_added_t = 0;
		nArcsNu = 0;
		model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
		tid1 = std::chrono::high_resolution_clock::now();

		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (i == 17 && i1 == 23)
				i = i;
			if (runAlt == 1) {
				if (i1 != model.params.preferredPathOrtoPos[i])
					continue; // only along preferred path in this opt
			}
			else {
				if (model.optPath.level[i].pointNr >= 0 && abs(i1 - model.optPath.level[i].pointNr) > model.params.maxDiff_pointNrFas3)
					continue;
			}

			if (model.network.physicalLev[i].nTimeIntervals[i1] == 0)
				continue; // inga tidsbagar till denna punkt
			if (i1 == 24)
				i1 = i1;
			if (i == 34)
				i = i;

			if (i == model.network.nPhysicalLevels - 2 && i1 == model.params.preferredPathOrtoPos[i])
				i = i;
			if (model.network.physicalLev[i].nArcsToPoint[i1] == 0 && i > 0)
				continue; // no arc to this point so no use to add arcs out


			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];
				if (nextLevel < 0)
					nextLevel = nextLevel;
				else {
					if (runAlt == 1 && i2 != model.params.preferredPathOrtoPos[nextLevel])
						continue; // only along preferred path in this opt
				}
				setupCheckPoints = 1;
				// fuelQualityKvot = get_fuelQualityKvot(i, i1, nextLevel, i2);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(i, i1, nextLevel, i2, &fuelQualityKvot);
				if (runAlt != 1)
					get_minMax_timeFromLevel(nextLevel, &min_t, &max_t);
				else
					get_minMax_timeFromLevel(-1, &min_t, &max_t);
				if (runAlt == 1)
					i = i;
				addBagar_AB_speedSTid(i, i1, nextLevel, i2, &setupCheckPoints, min_t, max_t, fuelQualityKvot, extraAreaCostKvot, runAlt);
			}
		}
		model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
		model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
		tid2 = std::chrono::high_resolution_clock::now();

		if (i + 1 == 7)
			i = i;
		for (i1 = 0; i1 < model.network.nChannels; i1++) {
			cNr = i1;
			for (i2b = 0; i2b < model.network.channel[cNr].nOutNodes; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[i2b];
				if (nextLevel != i + 1 && nextLevel >= 0)
					continue;
				if (nextLevel < 0 && i > 0)
					continue;
				if (nextLevel >= 0) {
					if (runAlt == 1 && model.network.channel[cNr].outNode[i2b] != model.params.preferredPathOrtoPos[nextLevel])
						continue; // only along preferred path in this opt
				}
				else {
					if (runAlt == 1)
						continue; // only along preferred path in this opt
					if (model.network.channel[cNr].earliestStartLevel != i)
						continue;
				}
				setupCheckPoints = 1;
				// fuelQualityKvot = get_fuelQualityKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b]);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], &fuelQualityKvot);
				if (runAlt != 1)
					get_minMax_timeFromLevel(nextLevel, &min_t, &max_t);
				else
					get_minMax_timeFromLevel(-1, &min_t, &max_t);
				addBagar_AB_speedSTid(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], &setupCheckPoints,
					min_t, max_t, fuelQualityKvot, extraAreaCostKvot, runAlt);
			}
		}

		nArcsTot += nArcsNu;
		if (model.nArcs > 1000)
			printf("\tLevel %d done (of %d). I have %d arcs now.\n",
				i + 1, model.network.nPhysicalLevels - 1, model.nArcs);
	}
	//fclose(modelDelay.filpek);
	//modelDelay.filpek = NULL;

	errlog("\n");

	// add arcs from last node and time to a super sink
	nArcsNu = 0;
	nodNr2 = adderaNod(i + 1, 0, 0);
	//printf("\n\n#### globalCount1 %d\n", globalCount1);
	//printf("#### globalCount2 %d\n\n\n", globalCount2);
	i1 = 0;
	for (i3 = 0; i3 < model.network.physicalLev[i].nTimeIntervals[i1]; i3++) {
		addEndBage(i, i1, i + 1, i3, nodNr2);
		nArcsNu++;
	}

	nodNr1 = nodNr2;
	nodNr2 = adderaNod(i + 1, 0, 0);
	addEndBage(i + 1, 0, i + 2, 0, nodNr2);

	nArcsTot += nArcsNu;

	return 0;
}

int genArcsToEnd_delayed(int thisLevel, int pos1, int nextLevel, int pos2, int tPos, int nSpeedSettings)
{
	// thisLevel can be negative (ending of channel) and next position could be a corridor start so handle these here too
	// from this point to end of route
	int tidInt, nodNr1, nodNr2, posNy, arcNr, endLevel, posEnd;
	int nArcsNu = 0, i, nod1, nod2, Reached, tidIntStart;
	double time = 0, fuel_main_noEca = 0, fuel_main_eca = 0, fuel_aux_noEca = 0, fuel_aux_eca = 0;
	double tid, fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, emission;
	double totCost = 0, dist;
	long long Cost;
	strDelayToEnd* routeToEnd;

	if (model.nArcs >= 584959)
		model.nArcs = model.nArcs;

	posEnd = 0;
	endLevel = model.network.nPhysicalLevels - 1;

	if (model.nArcs == 42)
		model.nArcs = model.nArcs;
	//if (thisLevel == 6)
	//	thisLevel = thisLevel;
	if (model.params.preferredPathOrtoPos[thisLevel] == pos1)
		pos1 = pos1;

	if (thisLevel >= 0) {
		if (model.delayRouteToEnd[thisLevel] == NULL) {
			model.delayRouteToEnd[thisLevel] = (strDelayToEnd*)malloc(model.network.physicalLev[thisLevel].nPoints * sizeof(strDelayToEnd));
			for (i = 0; i < model.network.physicalLev[thisLevel].nPoints; i++)
				model.delayRouteToEnd[thisLevel][i].time = -1;
		}
		routeToEnd = &(model.delayRouteToEnd[thisLevel][pos1]);
		nod1 = model.network.physicalLev[thisLevel].nodDelay[pos1];
		nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
	}
	else {
		if (model.delayRouteToEnd_channel[-thisLevel - 1] == NULL) {
			model.delayRouteToEnd_channel[-thisLevel - 1] = (strDelayToEnd*)malloc(2 * sizeof(strDelayToEnd));
			for (i = 0; i < 2; i++)
				model.delayRouteToEnd_channel[-thisLevel - 1][i].time = -1;
		}
		routeToEnd = &(model.delayRouteToEnd_channel[-thisLevel - 1][pos1]);
		nod1 = model.network.channel[-thisLevel - 1].nodDelay[pos1];
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
	}

	//if (model.delayRouteToEnd[thisLevel][pos1].time < 0) {
	if (routeToEnd->time < 0) {
		if (routeToEnd->time < -1.1)
			return 0; // no feasible path exists

		nod2 = model.network.physicalLev[model.network.nPhysicalLevels - 1].nodDelay[0];
		//if (thisLevel == 30 && pos1 == 40)
		//	modelDelay.filpek = fopen("checkDijkstra.txt", "w");
		dist = NystaUppBV_MassTest(&modelDelay, 1, nod2, nod1, &Cost);
		//if (thisLevel == 30 && pos1 == 40){
		//	fclose(modelDelay.filpek);
		//	modelDelay.filpek = NULL;
		//}
		if (modelDelay.nBVArcs < 1) {
			routeToEnd->time = -2; // no feasible path exists
			return 0;
		}

		routeToEnd->BVArc = (int*)malloc(modelDelay.nBVArcs * sizeof(int));
		routeToEnd->time = 0;
		dist = 0;
		for (i = 0; i < modelDelay.nBVArcs; i++) {
			arcNr = modelDelay.BVArc[i];
			routeToEnd->BVArc[modelDelay.nBVArcs - i - 1] = arcNr;
			time += modelDelay.arc[arcNr].time;
			fuel_main_noEca += modelDelay.arc[arcNr].fuel_noEca;
			fuel_main_eca += modelDelay.arc[arcNr].fuel_eca;
			fuel_aux_noEca += modelDelay.arc[arcNr].fuel_aux;
			fuel_aux_eca += modelDelay.arc[arcNr].fuel_auxEca;
			dist += modelDelay.arc[arcNr].distance;
			//if (thisLevel == 38 && pos1 == 29)
			//	printf("i %d arcNr %d tid %.2lf %.2lf fuelBase %.2lf fuel_aux %.4lf fuel_noEca %.4lf arcDist %.3lf\n", i, arcNr, modelDelay.arc[arcNr].time, time,
			//		modelDelay.arc[arcNr].fuel_aux * model.params.fuel.aux_noEca.price + modelDelay.arc[arcNr].fuel_auxEca * model.params.fuel.aux_eca.price +
			//		modelDelay.arc[arcNr].fuel_eca * model.params.fuel.main_eca.price + modelDelay.arc[arcNr].fuel_noEca * model.params.fuel.main_noEca.price,
			//		modelDelay.arc[arcNr].fuel_aux, modelDelay.arc[arcNr].fuel_noEca, modelDelay.arc[arcNr].distance / 1.852);
		}
		routeToEnd->nBVArcs = i;
		routeToEnd->time = time;
		routeToEnd->fuel_main_noEca = fuel_main_noEca;
		routeToEnd->fuel_main_eca = fuel_main_eca;
		routeToEnd->fuel_aux_noEca = fuel_aux_noEca;
		routeToEnd->fuel_aux_eca = fuel_aux_eca;
		routeToEnd->distance = dist;
	}

	if (thisLevel >= 0)
		tidIntStart = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
	else
		tidIntStart = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];

	if (model.params.eta_h > 0) {
		determineBastSpeedDelay_routeToEnd_eta(routeToEnd, tidIntStart * model.params.tIndexGerH);
	}

	tid = routeToEnd->time;
	tidInt = tidIntStart + (int)round(tid * model.params.nTidsperioder_perH);

	nodNr2 = addTimeTo_timeInterval(thisLevel, endLevel, posEnd, tidInt);

	fuel_noEca = routeToEnd->fuel_main_noEca;
	fuel_eca = routeToEnd->fuel_main_eca;
	fuel_aux = routeToEnd->fuel_aux_noEca;
	fuel_auxEca = routeToEnd->fuel_aux_eca;
	fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
		fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price);
	emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
		fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

	totCost += model.params.weightTime * model.params.priceTime * tid +
		model.params.weightFuel * fuelBase + emission * model.params.weightEmission * model.params.scaleObjEmission;

	if (model.params.useSimulering == 1) {
		if (thisLevel >= 0) {
			totCost += abs(model.params.preferredPathOrtoPos[thisLevel] - pos1) * model.simulering.penDeviatePrefPath_nodes;
		}
	}

	//if (thisLevel == 38 && pos1 == 29)
	//	printf("\ttotCost %.3lf tid %.3lf fuelBase %.3lf emission %.3lf\n", totCost,
	//		tid, fuelBase, emission);

	posNy = adderaArc(nodNr1, nodNr2, totCost, -1);

	arcNr = model.nArcs;
	if (model.nArcs == 272303)
		model.nArcs = model.nArcs;
	if (arcNr >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc,
			model.nAllocArcs * sizeof(strArcInfo));
	}
	model.arc[arcNr].fromLevel = thisLevel;
	model.arc[arcNr].toLevel = endLevel;
	model.arc[arcNr].fromPointNr = pos1;
	model.arc[arcNr].toPointNr = posEnd;
	(model.network.physicalLev[endLevel].nArcsToPoint[posEnd])++;
	if (thisLevel >= 0)
		model.arc[arcNr].fromTime = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
	else
		model.arc[arcNr].fromTime = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
	model.arc[arcNr].toTime = tidInt;
	model.arc[arcNr].speedSetting = -2;
	model.arc[arcNr].time = tid;
	model.arc[arcNr].distance = routeToEnd->distance; // model.functions.valuesNow.distance;
	model.arc[arcNr].emission = emission;
	model.arc[arcNr].fuelBase = fuelBase;
	model.arc[arcNr].fuel_aux = fuel_aux;
	model.arc[arcNr].fuel_auxEca = fuel_auxEca;
	model.arc[arcNr].fuel_eca = fuel_eca;
	model.arc[arcNr].fuel_noEca = fuel_noEca;
	model.arc[arcNr].fuelQualityKvot = -1;
	model.arc[arcNr].extraAreaCostKvot = 0;
	model.arc[arcNr].maxWindSpeed = 0;
	model.arc[arcNr].maxWaveHeight = 0;
	model.arc[arcNr].kvotCost = 1.0;
	model.arc[arcNr].safetyHurricane = 0;
	model.arc[arcNr].bowSlam = 0;
	model.arc[arcNr].greenWater = 0;
	model.arc[arcNr].dynamicStability = 0;
	model.arc[arcNr].rolling = 0;
	model.arc[arcNr].surfRiding = 0;
	model.arc[arcNr].safetyBase = 0;
	model.arc[arcNr].totCost = totCost;
	//fprintf(filSaveSpec, "arcNr %d from %d %d %d to %d %d %d cost %.4lf row %d\n",
	//	arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
	//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
	//	model.arc[arcNr].totCost, __LINE__);
	model.arc[arcNr].nodNr1 = nodNr1;
	model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
	model.nArcs++;
	nArcsNu++;

	return nArcsNu;
}

int genArcsToEnd_delayed_prefPath(int thisLevel, int pos1, int nextLevel, int pos2, int tPos)
{
	// thisLevel can be negative (ending of channel) and next position could be a corridor start so handle these here too
	// from this point to end of route
	int tidInt, nodNr1, nodNr2, posNy, arcNr, endLevel, posEnd;
	int nArcsNu = 0, i, nod1, nod2, Reached, tidIntStart;
	double time = 0, fuel_main_noEca = 0, fuel_main_eca = 0, fuel_aux_noEca = 0, fuel_aux_eca = 0;
	double tid, fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, emission;
	double totCost = 0, dist;
	long long Cost;
	strDelayToEnd* routeToEnd;

	if (model.nArcs >= 61846)
		model.nArcs = model.nArcs;

	posEnd = 0;
	endLevel = model.network.nPhysicalLevels - 1;

	if (model.nArcs == 42)
		model.nArcs = model.nArcs;
	//if (thisLevel == 6)
	//	thisLevel = thisLevel;

	int posUse = model.params.preferredPathOrtoPos[thisLevel];
	if (thisLevel >= 0) {
		if (model.delayRouteToEnd_prefPath[thisLevel] == NULL) {
			model.delayRouteToEnd_prefPath[thisLevel] = (strDelayToEnd*)malloc(model.network.physicalLev[thisLevel].nPoints * sizeof(strDelayToEnd));
			model.delayRouteToEnd_prefPath[thisLevel][posUse].time = -1;
		}
		routeToEnd = &(model.delayRouteToEnd_prefPath[thisLevel][pos1]);
		nod1 = model.network.physicalLev[thisLevel].nodDelay_prefPath[pos1];
		nodNr1 = model.network.physicalLev[thisLevel].nodNr_from_pt[pos1][tPos];
	}
	else {
		if (model.delayRouteToEnd_channel_prefPath[-thisLevel - 1] == NULL) {
			model.delayRouteToEnd_channel_prefPath[-thisLevel - 1] = (strDelayToEnd*)malloc(2 * sizeof(strDelayToEnd));
			for (i = 0; i < 2; i++)
				model.delayRouteToEnd_channel_prefPath[-thisLevel - 1][i].time = -1;
		}
		routeToEnd = &(model.delayRouteToEnd_channel_prefPath[-thisLevel - 1][pos1]);
		nod1 = model.network.channel[-thisLevel - 1].nodDelay_prefPath[pos1];
		nodNr1 = model.network.channel[-thisLevel - 1].nodNr_from_pt[pos1][tPos];
	}


	//if (model.delayRouteToEnd[thisLevel][pos1].time < 0) {
	if (routeToEnd->time < 0) {
		if (routeToEnd->time < -1.1)
			return 0; // no feasible path exists

		nod2 = model.network.physicalLev[model.network.nPhysicalLevels - 1].nodDelay_prefPath[0];
		dist = NystaUppBV_MassTest(&modelDelay_prefPath, 1, nod2, nod1, &Cost);
		if (modelDelay_prefPath.nBVArcs < 1) {
			routeToEnd->time = -2; // no feasible path exists
			return 0;
		}

		routeToEnd->BVArc = (int*)malloc(modelDelay_prefPath.nBVArcs * sizeof(int));
		routeToEnd->time = 0;
		dist = 0;
		for (i = 0; i < modelDelay_prefPath.nBVArcs; i++) {
			arcNr = modelDelay_prefPath.BVArc[i];
			routeToEnd->BVArc[modelDelay_prefPath.nBVArcs - i - 1] = arcNr;
			time += modelDelay_prefPath.arc[arcNr].time;
			fuel_main_noEca += modelDelay_prefPath.arc[arcNr].fuel_noEca;
			fuel_main_eca += modelDelay_prefPath.arc[arcNr].fuel_eca;
			fuel_aux_noEca += modelDelay_prefPath.arc[arcNr].fuel_aux;
			fuel_aux_eca += modelDelay_prefPath.arc[arcNr].fuel_auxEca;
			dist += modelDelay_prefPath.arc[arcNr].distance;
			//if (thisLevel == 38 && pos1 == 29)
			//	printf("i %d arcNr %d tid %.2lf %.2lf fuelBase %.2lf fuel_aux %.4lf fuel_noEca %.4lf arcDist %.3lf\n", i, arcNr, modelDelay.arc[arcNr].time, time,
			//		modelDelay.arc[arcNr].fuel_aux * model.params.fuel.aux_noEca.price + modelDelay.arc[arcNr].fuel_auxEca * model.params.fuel.aux_eca.price +
			//		modelDelay.arc[arcNr].fuel_eca * model.params.fuel.main_eca.price + modelDelay.arc[arcNr].fuel_noEca * model.params.fuel.main_noEca.price,
			//		modelDelay.arc[arcNr].fuel_aux, modelDelay.arc[arcNr].fuel_noEca, modelDelay.arc[arcNr].distance / 1.852);
		}
		routeToEnd->nBVArcs = i;
		routeToEnd->time = time;
		routeToEnd->fuel_main_noEca = fuel_main_noEca;
		routeToEnd->fuel_main_eca = fuel_main_eca;
		routeToEnd->fuel_aux_noEca = fuel_aux_noEca;
		routeToEnd->fuel_aux_eca = fuel_aux_eca;
		routeToEnd->distance = dist;
	}

	if (thisLevel >= 0)
		tidIntStart = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
	else
		tidIntStart = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];

	tid = routeToEnd->time;
	tidInt = tidIntStart + (int)round(tid * model.params.nTidsperioder_perH);

	nodNr2 = addTimeTo_timeInterval(thisLevel, endLevel, posEnd, tidInt);

	fuel_noEca = routeToEnd->fuel_main_noEca;
	fuel_eca = routeToEnd->fuel_main_eca;
	fuel_aux = routeToEnd->fuel_aux_noEca;
	fuel_auxEca = routeToEnd->fuel_aux_eca;
	fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
		fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price);
	emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
		fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

	totCost += model.params.weightTime * model.params.priceTime * tid +
		model.params.weightFuel * fuelBase + emission * model.params.weightEmission * model.params.scaleObjEmission;
	//if (thisLevel == 38 && pos1 == 29)
	//	printf("\ttotCost %.3lf tid %.3lf fuelBase %.3lf emission %.3lf\n", totCost,
	//		tid, fuelBase, emission);

	posNy = adderaArc(nodNr1, nodNr2, totCost, -1);

	arcNr = model.nArcs;
	if (model.nArcs == 272303)
		model.nArcs = model.nArcs;
	if (arcNr >= model.nAllocArcs) {
		model.nAllocArcs += 1000000;
		model.arc = (strArcInfo*)realloc(model.arc,
			model.nAllocArcs * sizeof(strArcInfo));
	}
	model.arc[arcNr].fromLevel = thisLevel;
	model.arc[arcNr].toLevel = endLevel;
	model.arc[arcNr].fromPointNr = pos1;
	model.arc[arcNr].toPointNr = posEnd;
	(model.network.physicalLev[endLevel].nArcsToPoint[posEnd])++;
	if (thisLevel >= 0)
		model.arc[arcNr].fromTime = model.network.physicalLev[thisLevel].timeInterval[pos1][tPos];
	else
		model.arc[arcNr].fromTime = model.network.channel[-thisLevel - 1].timeInterval[pos1][tPos];
	model.arc[arcNr].toTime = tidInt;
	model.arc[arcNr].speedSetting = -2;
	model.arc[arcNr].time = tid;
	model.arc[arcNr].distance = routeToEnd->distance; // model.functions.valuesNow.distance;
	model.arc[arcNr].emission = emission;
	model.arc[arcNr].fuelBase = fuelBase;
	model.arc[arcNr].fuel_aux = fuel_aux;
	model.arc[arcNr].fuel_auxEca = fuel_auxEca;
	model.arc[arcNr].fuel_eca = fuel_eca;
	model.arc[arcNr].fuel_noEca = fuel_noEca;
	model.arc[arcNr].fuelQualityKvot = -1;
	model.arc[arcNr].extraAreaCostKvot = 0;
	model.arc[arcNr].maxWindSpeed = 0;
	model.arc[arcNr].maxWaveHeight = 0;
	model.arc[arcNr].kvotCost = 1.0;
	model.arc[arcNr].safetyHurricane = 0;
	model.arc[arcNr].bowSlam = 0;
	model.arc[arcNr].greenWater = 0;
	model.arc[arcNr].dynamicStability = 0;
	model.arc[arcNr].rolling = 0;
	model.arc[arcNr].surfRiding = 0;
	model.arc[arcNr].safetyBase = 0;
	model.arc[arcNr].totCost = totCost;
	//fprintf(filSaveSpec, "arcNr %d from %d %d %d to %d %d %d cost %.4lf row %d\n",
	//	arcNr, model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].fromTime,
	//	model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr, model.arc[arcNr].toTime,
	//	model.arc[arcNr].totCost, __LINE__);
	model.arc[arcNr].nodNr1 = nodNr1;
	model.arc[arcNr].nodNr1_utNodPos = model.Noder[nodNr1].nUtNoder - 1;
	model.nArcs++;
	nArcsNu++;

	return nArcsNu;
}


int addBagar_AB_speedSTid(int thisLevel, int pos1, int nextLevel, int pos2, int* setupCheckPoints, int min_t, int max_t,
	double fuelQualityKvot, double extraAreaCostKvot, int runAlt) {
	int nSpeedSettings, prefPath, i4, i3, arcNr;
	int nArcsNu = 0, firstArcNu;
	double kvotCost, minCost, newCost, diffCost;
	double calmWaterSpeed = -1.0, distArc = -1, delayFactor = 0;
	double fuelFactorMain, nAddedTotArcs = 0, speedDiffCurrent = 0;
	int nodNr1, utNodPos;
	//if (thisLevel == 6)
	//	printf("thisLev %d\n", thisLevel);
	if (thisLevel >= 0) {
		nSpeedSettings = model.functions.speedLevel[thisLevel].nShip_speedSettings;
		//if (model.nArcs == 36039 && thisLevel == 17) {
		//	printf("nSpeedSettings %d\n", model.functions.speedLevel[thisLevel].nShip_speedSettings);
		//}
		if (pos1 == model.params.preferredPathOrtoPos[thisLevel]) {
			if (nextLevel >= 0) {
				if (pos2 == model.params.preferredPathOrtoPos[nextLevel] && thisLevel == nextLevel - 1 &&
					(model.params.preferredPathStraightLineFeasibleFrom[thisLevel] == 0 || model.params.max_changeDirection == 0 || runAlt == 1))
					prefPath = 1;
			}
			else {
				if ((model.network.channel[-nextLevel - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0 || runAlt == 1) &&
					pos1 == model.params.preferredPathOrtoPos[thisLevel] && model.network.channel[-nextLevel - 1].preferredPathPoint_posConnectTo >= 0)
					prefPath = 1;
			}
			//if (model.params.preferredPathUseChannelSpeed[thisLevel] > 0) {
			//	calmWaterSpeed = model.params.preferredPathUseChannelSpeed[thisLevel];
			//	fuelFactorMain = model.network.channel[model.params.preferredPathUseChannelConsumption[thisLevel]].totalConsumption;
			//	nSpeedSettings = 1;
				//if (model.nArcs == 20)
				//	errlog("calmWaterSpeed %.2lf fuelFactorMain %.2lf thisLevel %d\n", calmWaterSpeed, fuelFactorMain, thisLevel);
			//}
		}
	}
	else {
		if (nextLevel >= 0) {
			nSpeedSettings = model.functions.speedChannelOut[-thisLevel - 1].nShip_speedSettings;
			if ((model.network.channel[-thisLevel - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0 || runAlt == 1) &&
				pos2 == model.params.preferredPathOrtoPos[nextLevel] && model.network.channel[-thisLevel - 1].preferredPathPoint_posConnectFrom >= 0 &&
				model.network.channel[-thisLevel - 1].bastEndLevel == nextLevel)
				prefPath = 1;
			//if (pos2 == model.params.preferredPathOrtoPos[nextLevel]) {
			//	if (model.params.preferredPathUseChannelSpeed[nextLevel - 1] > 0) {
			//		calmWaterSpeed = model.params.preferredPathUseChannelSpeed[nextLevel - 1];
			//		fuelFactorMain = model.network.channel[model.params.preferredPathUseChannelConsumption[nextLevel - 1]].totalConsumption;
			//		nSpeedSettings = 1;
					//if (model.nArcs == 20)
					//	errlog("calmWaterSpeed2 %.2lf fuelFactorMain %.2lf thisLevel %d\n", calmWaterSpeed, fuelFactorMain, thisLevel);
			//	}
			//}
		}
		else {
			nSpeedSettings = model.functions.speedChannel[-thisLevel - 1].nShip_speedSettings;
			if (model.network.channel[-thisLevel - 1].timeThroughChannel > -0.5)
				nSpeedSettings = 1; // only one speed option if fix speed through channel
		}
	}

	if (model.nArcs >= 6)
		model.nArcs = model.nArcs;
	//if (model.nArcs == 36039 && thisLevel == 17) {
	//	printf("nSpeedSettings %d\n", nSpeedSettings);
	//}
	if (thisLevel >= 0) {
		if (thisLevel == 22 && pos1 == 23)
			pos1 = pos1;
		for (i3 = 0; i3 < model.network.physicalLev[thisLevel].nTimeIntervals[pos1]; i3++) {
			if (model.network.physicalLev[thisLevel].timeInterval[pos1][i3] * model.params.tIndexGerH < model.network.tidp_startHistoricDataOnly) {
				for (i4 = 0; i4 < nSpeedSettings; i4++)
					arcNr = addEnBage_AB(thisLevel, pos1, nextLevel, pos2, i3, i4, setupCheckPoints, min_t, max_t, fuelQualityKvot, extraAreaCostKvot, runAlt);
			}
			else {
				if (delayVersion < 3)
					arcNr = genArcsTo_delayedPreferredPath(thisLevel, pos1, nextLevel, pos2, i3, nSpeedSettings, fuelQualityKvot, extraAreaCostKvot, &delayFactor, &distArc);
				else {
					if (runAlt != 1) {// && model.results.onlyPrefPath_kaoutar != 1) {
						//if(thisLevel ==22 && pos1==23&&nextLevel==23&&pos2==23)
						//	arcNr = genArcsToEnd_delayed_prefPath(thisLevel, pos1, nextLevel, pos2, i3);

						arcNr = genArcsToEnd_delayed(thisLevel, pos1, nextLevel, pos2, i3, nSpeedSettings);
					}
					else
						arcNr = genArcsToEnd_delayed_prefPath(thisLevel, pos1, nextLevel, pos2, i3);
				}
			}
		}
	}
	else {
		for (i3 = 0; i3 < model.network.channel[-thisLevel - 1].nTimeIntervals[1]; i3++) {
			firstArcNu = model.nArcs;
			if (model.network.channel[-thisLevel - 1].timeInterval[pos1][i3] * model.params.tIndexGerH < model.network.tidp_startHistoricDataOnly) {
				for (i4 = 0; i4 < nSpeedSettings; i4++)
					arcNr = addEnBage_AB(thisLevel, pos1, nextLevel, pos2, i3, i4, setupCheckPoints, min_t, max_t, fuelQualityKvot, extraAreaCostKvot, runAlt);
				kvotCost = model.network.channel[-thisLevel - 1].kvotCost;
				if (kvotCost < 1 && thisLevel == nextLevel) {
					minCost = 1e20;
					for (arcNr = firstArcNu; arcNr < model.nArcs; arcNr++) {
						if (minCost > model.arc[arcNr].totCost)
							minCost = model.arc[arcNr].totCost;
					}
					newCost = minCost;
					if (USE_KVOTKOST == 1)
						newCost *= kvotCost;
					diffCost = minCost - newCost;
					for (arcNr = firstArcNu; arcNr < model.nArcs; arcNr++) {
						model.arc[arcNr].totCost -= diffCost;
						nodNr1 = model.arc[arcNr].nodNr1;
						utNodPos = model.arc[arcNr].nodNr1_utNodPos;
						model.Noder[nodNr1].UtNodCost[utNodPos] -= diffCost;
					}
				}
			}
			else {
				if (nextLevel == 102)
					nextLevel = nextLevel;
				if (delayVersion < 3)
					arcNr = genArcsTo_delayedPreferredPath(thisLevel, pos1, nextLevel, pos2, i3, nSpeedSettings, fuelQualityKvot, extraAreaCostKvot, &delayFactor, &distArc);
				else {
					if (runAlt != 1) {// && model.results.onlyPrefPath_kaoutar != 1)
						arcNr = genArcsToEnd_delayed(thisLevel, pos1, nextLevel, pos2, i3, nSpeedSettings);
					}
					else
						arcNr = genArcsToEnd_delayed_prefPath(thisLevel, pos1, nextLevel, pos2, i3);
				}
			}
		}
	}
	if (nextLevel < 0) { // add arcs for the channel path
		model.functions.valuesNow.prefPathArc = 2;
		delayFactor = 0;
		if (model.network.channel[-nextLevel - 1].timeThroughChannel > -0.5)
			nSpeedSettings = 1; // only one speed option if fix speed through channel
		*setupCheckPoints = 1;
		// fuelQualityKvot = get_fuelQualityKvot(nextLevel, 0, nextLevel, 1);
		extraAreaCostKvot = get_totalExtraAreaCostKvot(nextLevel, 0, nextLevel, 1, &fuelQualityKvot);
		// 		totCost *= (1 + extraAreaCostKvot);

		if (model.network.channel[-nextLevel - 1].timeThroughChannel > -0.5)
			nSpeedSettings = 1;		// obs only one speed setting if speed at corridor
		if (thisLevel < 0 && nextLevel != thisLevel) {
			for (i3 = 0; i3 < model.network.channel[-thisLevel - 1].nTimeIntervals[1]; i3++) {
				firstArcNu = model.nArcs;
				if (model.network.channel[-thisLevel - 1].timeInterval[1][i3] * model.params.tIndexGerH < model.network.tidp_startHistoricDataOnly) {
					for (i4 = 0; i4 < nSpeedSettings; i4++)
						arcNr = addEnBage_AB(thisLevel, 1, nextLevel, 0, i3, i4, setupCheckPoints, 0, 99999, fuelQualityKvot, extraAreaCostKvot, runAlt);
					kvotCost = model.network.channel[-thisLevel - 1].kvotCost;
					if (kvotCost < 1 && thisLevel == nextLevel) {
						minCost = 1e20;
						for (arcNr = firstArcNu; arcNr < model.nArcs; arcNr++) {
							if (minCost > model.arc[arcNr].totCost)
								minCost = model.arc[arcNr].totCost;
						}
						newCost = minCost;
						if (USE_KVOTKOST == 1)
							newCost *= kvotCost;
						diffCost = minCost - newCost;
						for (arcNr = firstArcNu; arcNr < model.nArcs; arcNr++) {
							model.arc[arcNr].totCost -= diffCost;
							nodNr1 = model.arc[arcNr].nodNr1;
							utNodPos = model.arc[arcNr].nodNr1_utNodPos;
							model.Noder[nodNr1].UtNodCost[utNodPos] -= diffCost;
						}
					}
				}
				else {
					if (runAlt != 1) {// && model.results.onlyPrefPath_kaoutar != 1)
						arcNr = genArcsToEnd_delayed(thisLevel, 1, nextLevel, 0, i3, nSpeedSettings);
					}
					else
						arcNr = genArcsToEnd_delayed_prefPath(thisLevel, 1, nextLevel, 0, i3);
				}
			}
		}
		else {
			nSpeedSettings = model.functions.speedChannel[-nextLevel - 1].nShip_speedSettings;
			if (model.network.channel[-nextLevel - 1].timeThroughChannel > -0.5)
				nSpeedSettings = 1; // only one speed option if fix speed through channel
			for (i3 = 0; i3 < model.network.channel[-nextLevel - 1].nTimeIntervals[0]; i3++) {
				firstArcNu = model.nArcs;
				if (model.network.channel[-nextLevel - 1].timeInterval[0][i3] * model.params.tIndexGerH < model.network.tidp_startHistoricDataOnly) {
					for (i4 = 0; i4 < nSpeedSettings; i4++)
						arcNr = addEnBage_AB(nextLevel, 0, nextLevel, 1, i3, i4, setupCheckPoints, 0, 99999, fuelQualityKvot, extraAreaCostKvot, runAlt);
					kvotCost = model.network.channel[-nextLevel - 1].kvotCost;
					if (kvotCost < 1) {
						minCost = 1e20;
						for (arcNr = firstArcNu; arcNr < model.nArcs; arcNr++) {
							if (minCost > model.arc[arcNr].totCost)
								minCost = model.arc[arcNr].totCost;
						}
						newCost = minCost;
						if (USE_KVOTKOST == 1)
							newCost *= kvotCost;
						diffCost = minCost - newCost;
						for (arcNr = firstArcNu; arcNr < model.nArcs; arcNr++) {
							model.arc[arcNr].totCost -= diffCost;
							nodNr1 = model.arc[arcNr].nodNr1;
							utNodPos = model.arc[arcNr].nodNr1_utNodPos;
							model.Noder[nodNr1].UtNodCost[utNodPos] -= diffCost;
							if (model.Noder[nodNr1].UtNodCost[utNodPos] < 0)
								printf("ERROR code row %d!\n", __LINE__);
						}
					}
				}
				else {
					if (delayVersion < 3) {
						if (model.network.channel[-nextLevel - 1].timeThroughChannel <= -0.5) {
							delayFactor = eval_factorDelayedAlongPath(nextLevel, model.network.channel[-nextLevel - 1].timeInterval[0][i3], &speedDiffCurrent);
						}
						else
							delayFactor = 1;
						distArc = model.network.channel[-nextLevel - 1].distance_km;
						for (i4 = 0; i4 < nSpeedSettings; i4++) {
							nArcsNu += addEnBage_delayAB(nextLevel, 0, nextLevel, 1, i3, i4, 0, 1e10, fuelQualityKvot, extraAreaCostKvot, distArc, delayFactor);
						}
					}
					else {
						if (runAlt != 1) {// && model.results.onlyPrefPath_kaoutar != 1)
							arcNr = genArcsToEnd_delayed(nextLevel, 0, nextLevel, 1, i3, nSpeedSettings);
						}
						else
							arcNr = genArcsToEnd_delayed_prefPath(nextLevel, 0, nextLevel, 1, i3);
					}
				}
			}
		}
	}
	return nArcsNu;
}


int createTimeArcs_kaoutar(int runAlt, int iter0)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0, nDagarFramat;
	double fuel, safety, tid, totCost, fuelQualityKvot, extraAreaCostKvot;

	model.nErrorCoordBB = 0;
	checkMinnesAnvandning(__LINE__);

	//printf("nSpeedSettings %d\n", model.functions.speedLevel[17].nShip_speedSettings);
	//fprintf(filSaveSpec, "nSpeedSettings %d\n", model.functions.speedLevel[17].nShip_speedSettings);


	if (model.nAllocArcs == 0){ // iter0 == 0 && runAlt == 0) {
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

		for (i = 0; i < model.network.nPhysicalLevels; i++) {
			model.network.physicalLev[i].nodNr_from_pt = (int**)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int*));
			model.network.physicalLev[i].nTimeIntervals = (int*)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int));
			model.network.physicalLev[i].nAllocTimeIntervals = (int*)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int));
			model.network.physicalLev[i].timeInterval = (int**)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int*));

			model.nAllocNoder = 50000;
			model.Noder = (strNoder*)malloc2(model.nAllocNoder * sizeof(strNoder));
			for (int i0 = 0; i0 < model.nAllocNoder; i0++)
				model.Noder[i0].UtNod = NULL;

			for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
				model.network.physicalLev[i].nTimeIntervals[i1] = 0;
				model.network.physicalLev[i].nAllocTimeIntervals[i1] = 100;
				model.network.physicalLev[i].timeInterval[i1] = (int*)malloc2(
					model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));
				model.network.physicalLev[i].nodNr_from_pt[i1] = (int*)malloc2(
					model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));

			}
		}
		model.nCallsWeatherBand = (int*)calloc2(model.nWeatherFiles, sizeof(int));

	}

	if (runAlt == 0) {
		if (runAltForecast >= 1) {
			// loadWeatherFiles_redis();
			loadAllWeatherFiles_gribForecast_kaoutar(); // load forecast from model.iterKaoutar.tidpStartIter_h
			if (model.network.tidp_startHistoricDataOnly < 999999) {
				nDagarFramat = model.results.forecastTypeOrig % 100;
				if (model.results.forecastType < 100) {
					if (model.iterKaoutar.tidpStartIter_h + 24 * nDagarFramat < model.network.tidp_startHistoricDataOnly)
						model.network.tidp_startHistoricDataOnly = model.iterKaoutar.tidpStartIter_h + 24 * nDagarFramat;
					// delay map
					loadTimeDelayMap();
					loadCurrentAverageMaps();
					set_tidp_ger_delayPos();
				}
				else {
					if (model.results.forecastType < 200) {
						// last day forecast
						model.network.tidp_startHistoricDataOnly = 999999;
					}
					else {
						// calm water, no weather...
						model.network.tidp_startHistoricDataOnly = 999999;
					}

				}
			}
		}
		else if (runAltForecast == -2) {
			if (iter0 == 0)
				loadWeatherFiles_grib(); // hindcast data
		}
		else {
			errlog("ERROR! No alternative for runAltForecast %d. I quit!\n", runAltForecast);
			exit(0);
		}

		if (SKRIV_UT_NOTHING == 0)
			printf("-- Time after loading all data %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

		if (model.params.eta_h > 0)
			gen_midTimeArrive_old();
		else {
			if (runAltForecast == 0) {
				gen_midTimeArrive_old();
				gen_midTimeArrive();
			}
			else {
				gen_midTimeArrive_forecast();
			}
		}
	}

	if (SKRIV_UT_NOTHING == 0)
		printf("-- Time after creating basic solution %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

	//}

	for (i = model.iterKaoutar.physLevelStartUse; i < model.network.nPhysicalLevels; i++) {
		if (i == model.iterKaoutar.physLevelStartUse) {
			model.nArcs = 0;
			model.nNoder = 0;

			// model.network.physicalLev[i].timeInterval[model.iterKaoutar.physPointStart][0] = model.iterKaoutar.tidpStartIterArc_h * model.params.nTidsperioder_perH;
			model.network.physicalLev[i].timeInterval[model.iterKaoutar.physPointStart][0] = model.iterKaoutar.startTimeUse * model.params.nTidsperioder_perH;
			model.network.physicalLev[i].nTimeIntervals[model.iterKaoutar.physPointStart] = 1;
			model.network.physicalLev[i].nodNr_from_pt[model.iterKaoutar.physPointStart][0] = model.nNoder;
			//adderaNod(i, 0, 0);
			adderaNod(i, model.iterKaoutar.physPointStart, 0);
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

	if (runAlt == 0) {
		if (delayVersion >= 3 && model.network.tidp_startHistoricDataOnly < 999999) {
			//if(model.results.onlyPrefPath_kaoutar == 0)
				solve_SP_delay();
			if (SKRIV_UT_NOTHING == 0)
				printf("-- Time after solving SP_delay %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

			solve_SP_delayPrefPath();
			if (SKRIV_UT_NOTHING == 0) {
				printf("-- Time after solving SP_delay %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
				// checkSP_allPhysicalLevels();
			}
		}
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

	//modelDelay.filpek = fopen("checkDijkstra.txt", "w");
	for (i = model.iterKaoutar.physLevelStartUse; i < model.network.nPhysicalLevels - 1; i++) {

		n_added_t = 0;
		nArcsNu = 0;
		model.tmpTid2[0] = std::chrono::high_resolution_clock::now();
		tid1 = std::chrono::high_resolution_clock::now();
		if (i >= 59)
			i = i;
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (i1 == 22)
				i1 = i1;
			if (i == model.iterKaoutar.physLevelStartUse && i1 != model.iterKaoutar.physPointStart)
				continue; // only out from the point the ship is when it starts...

			if (i == 17 && i1 == 23)
				i = i;
			if (runAlt == 1) {
				if (i1 != model.params.preferredPathOrtoPos[i])
					continue; // only along preferred path in this opt
			}
			else {
				if (model.optPath.level[i].pointNr >= 0 && abs(i1 - model.optPath.level[i].pointNr) > model.params.maxDiff_pointNrFas3)
					continue;
			}

			if (model.network.physicalLev[i].nTimeIntervals[i1] == 0)
				continue; // inga tidsbagar till denna punkt

			if (i == model.network.nPhysicalLevels - 2 && i1 == model.params.preferredPathOrtoPos[i])
				i = i;
			if (model.network.physicalLev[i].nArcsToPoint[i1] == 0 && i > model.iterKaoutar.physLevelStartUse)
				continue; // no arc to this point so no use to add arcs out


			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];
				if (nextLevel < 0)
					nextLevel = nextLevel;
				else {
					if ((runAlt == 1 || model.results.onlyPrefPath_kaoutar == 1) && i2 != model.params.preferredPathOrtoPos[nextLevel])
						continue; // only along preferred path in this opt
				}
				setupCheckPoints = 1;
				// fuelQualityKvot = get_fuelQualityKvot(i, i1, nextLevel, i2);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(i, i1, nextLevel, i2, &fuelQualityKvot);
				if (runAlt != 1)
					get_minMax_timeFromLevel(nextLevel, &min_t, &max_t);
				else
					get_minMax_timeFromLevel(-1, &min_t, &max_t);
				if (runAlt == 1)
					i = i;
				//if(i==5)
				//	modelDelay.filpek = fopen("checkOptArcCosts.txt", "w");

				addBagar_AB_speedSTid(i, i1, nextLevel, i2, &setupCheckPoints, min_t, max_t, fuelQualityKvot, extraAreaCostKvot, runAlt);
				//if (i == 5) {
				//	fclose(modelDelay.filpek);
				//	modelDelay.filpek = NULL;
				//}
			}
		}
		model.tmpTid2[1] = std::chrono::high_resolution_clock::now();
		model.duration1 += model.tmpTid2[1] - model.tmpTid2[0];
		tid2 = std::chrono::high_resolution_clock::now();

		if (i + 1 == 21)
			i = i;
		for (i1 = 0; i1 < model.network.nChannels; i1++) {
			cNr = i1;
			for (i2b = 0; i2b < model.network.channel[cNr].nOutNodes; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[i2b];
				if (nextLevel != i + 1 && nextLevel >= 0)
					continue;
				if (nextLevel < 0 && i > 0)
					continue;
				if (nextLevel >= 0) {
					if (runAlt == 1 && model.network.channel[cNr].outNode[i2b] != model.params.preferredPathOrtoPos[nextLevel])
						continue; // only along preferred path in this opt
				}
				else {
					if (runAlt == 1)
						continue; // only along preferred path in this opt
					if (model.network.channel[cNr].earliestStartLevel != i)
						continue;
				}
				setupCheckPoints = 1;
				// fuelQualityKvot = get_fuelQualityKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b]);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], &fuelQualityKvot);
				if (runAlt != 1)
					get_minMax_timeFromLevel(nextLevel, &min_t, &max_t);
				else
					get_minMax_timeFromLevel(-1, &min_t, &max_t);
				addBagar_AB_speedSTid(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], &setupCheckPoints,
					min_t, max_t, fuelQualityKvot, extraAreaCostKvot, runAlt);
			}
		}

		nArcsTot += nArcsNu;
		//if (model.nArcs > 1000)
		printf("\tLevel %d done (of %d). I have %d arcs now.\n",
			i + 1, model.network.nPhysicalLevels - 1, model.nArcs);
	}
	checkMinnesAnvandning(__LINE__);
	//fclose(modelDelay.filpek);
	//modelDelay.filpek = NULL;

	errlog("\n");

	// add arcs from last node and time to a super sink
	nArcsNu = 0;
	nodNr2 = adderaNod(i + 1, 0, 0);
	//printf("\n\n#### globalCount1 %d\n", globalCount1);
	//printf("#### globalCount2 %d\n\n\n", globalCount2);
	i1 = 0;
	for (i3 = 0; i3 < model.network.physicalLev[i].nTimeIntervals[i1]; i3++) {
		addEndBage(i, i1, i + 1, i3, nodNr2);
		nArcsNu++;
	}

	nodNr1 = nodNr2;
	nodNr2 = adderaNod(i + 1, 0, 0);
	addEndBage(i + 1, 0, i + 2, 0, nodNr2);

	nArcsTot += nArcsNu;
	printf("\tLevel %d done (final). I have %d arcs now.\n",
		i + 1, model.nArcs);

	return 0;
}

// i3 = getTimeIntervalFromTidp(fromLevel, fromPos, toLevel, toPos, tidP);
int getTimeIntervalFromTidp(int thisLevel, int pos1, int nextLevel, int pos2, int tidP) {
	int i3;
	if (thisLevel >= 0) {
		for (i3 = 0; i3 < model.network.physicalLev[thisLevel].nTimeIntervals[pos1]; i3++) {
			if (tidP == model.network.physicalLev[thisLevel].timeInterval[pos1][i3])
				break;
		}
		if (i3 >= model.network.physicalLev[thisLevel].nTimeIntervals[pos1])
			i3 = -1;
	}
	else {
		for (i3 = 0; i3 < model.network.channel[-thisLevel - 1].nTimeIntervals[1]; i3++) {
			if (tidP == model.network.channel[-thisLevel - 1].timeInterval[pos1][i3])
				break;
		}
		if (i3 >= model.network.channel[-thisLevel - 1].nTimeIntervals[pos1])
			i3 = -1;
	}
	return i3;
}


int createArcsFromFixSol(FILE* fileFixSol, int runAlt)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0;
	double fuel, safety, tid, totCost, fuelQualityKvot, extraAreaCostKvot;

	model.nErrorCoordBB = 0;
	checkMinnesAnvandning(__LINE__);

	if (runAlt == 0) {
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

		for (i = 0; i < model.network.nPhysicalLevels; i++) {
			model.network.physicalLev[i].nodNr_from_pt = (int**)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int*));
			model.network.physicalLev[i].nTimeIntervals = (int*)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int));
			model.network.physicalLev[i].nAllocTimeIntervals = (int*)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int));
			model.network.physicalLev[i].timeInterval = (int**)malloc2(
				model.network.physicalLev[i].nPoints * sizeof(int*));
			if (i == 0) {
				model.network.physicalLev[i].timeInterval[0] = (int*)malloc2(sizeof(int));
				model.network.physicalLev[i].timeInterval[0][0] = model.params.startDelay_h;
				model.network.physicalLev[i].nodNr_from_pt[0] = (int*)malloc2(sizeof(int));
				model.network.physicalLev[i].nTimeIntervals[0] = 1;
				model.nAllocNoder = 50000;
				model.Noder = (strNoder*)malloc2(model.nAllocNoder * sizeof(strNoder));
				for (int i0 = 0; i0 < model.nAllocNoder; i0++)
					model.Noder[i0].UtNod = NULL;
				model.nArcs = 0;
				model.nNoder = 0;
				model.network.physicalLev[i].nodNr_from_pt[0][0] = model.nNoder;
				adderaNod(i, 0, 0);
			}
			else {
				for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
					model.network.physicalLev[i].nTimeIntervals[i1] = 0;
					model.network.physicalLev[i].nAllocTimeIntervals[i1] = 100;
					model.network.physicalLev[i].timeInterval[i1] = (int*)malloc2(
						model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));
					model.network.physicalLev[i].nodNr_from_pt[i1] = (int*)malloc2(
						model.network.physicalLev[i].nAllocTimeIntervals[i1] * sizeof(int));

				}
			}
		}
		model.nCallsWeatherBand = (int*)calloc2(model.nWeatherFiles, sizeof(int));


		if (runAltForecast == -1) {
			//if (model.params.hindCast == 0) {
			//	if(model.params.onboard == 0)
			loadWeatherFiles_redis();
			//	else
			//		loadWeatherFiles_onboard_grib();
			//	if (model.network.tidp_startHistoricDataOnly < 999999) {
			//		loadTimeDelayMap();
			//		if (delayVersion == 4)
			//			loadCurrentAverageMaps();
			//		set_tidp_ger_delayPos();
			//	}
		}
		else
			loadWeatherFiles_grib();

		if (SKRIV_UT_NOTHING == 0)
			printf("-- Time after loading all data %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

	}
	else {
		for (i = 0; i < model.network.nPhysicalLevels; i++) {
			if (i == 0) {
				model.nArcs = 0;
				model.nNoder = 0;
				model.network.physicalLev[i].nTimeIntervals[0] = 1;
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
	dataStr* data = (dataStr*)malloc(sizeof(dataStr));
	char objects[100][CHAR_ALLOC];
	int antal, fromLevel, toLevel, fromPos, toPos, fromTime, toTime, speedSetting, tidP;

	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // forecastType
	model.results.forecastTypeOrig = char_to_int(objects[1]);
	model.results.forecastType = model.results.forecastTypeOrig % 1000;
	if (model.results.forecastType > 300)
		model.results.onlyPrefPath_kaoutar = 1;
	else
		model.results.onlyPrefPath_kaoutar = 0;
	model.results.forecastType = model.results.forecastType % 300;

	if (model.results.onlyPrefPath_kaoutar == 1) {
		model.params.max_changeDirection = 0;
		model.params.max_changeDirection_factorStartEnd = 0;
	}
	else {
		model.params.max_changeDirection = model.params.max_changeDirection_base;
		model.params.max_changeDirection_factorStartEnd = model.params.max_changeDirection_factorStartEnd_base;
	}


	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // nBVArcs
	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // fieldNames

	tidP = 0;
	for (i = 0; i < 10000; i++) {
		antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol);
		if (antal != 9)
			break; // done with the arcs

		fromLevel = char_to_int(objects[2]);
		toLevel = char_to_int(objects[3]);
		fromPos = char_to_int(objects[4]);
		toPos = char_to_int(objects[5]);
		//fromTime = (int)(char_to_double(objects[6]) * model.params.nTidsperioder_perH);
		toTime = (int)(char_to_double(objects[7]) * model.params.nTidsperioder_perH);
		speedSetting = char_to_int(objects[8]);

		extraAreaCostKvot = get_totalExtraAreaCostKvot(fromLevel, fromPos, toLevel, toPos, &fuelQualityKvot);

		setupCheckPoints = 1;
		i3 = getTimeIntervalFromTidp(fromLevel, fromPos, toLevel, toPos, tidP);
		arcNr = addEnBage_AB(fromLevel, fromPos, toLevel, toPos, i3, speedSetting, &setupCheckPoints, 0, toTime + 10000, fuelQualityKvot, extraAreaCostKvot, runAlt);
		tidP = model.arc[arcNr].toTime;

		nArcsTot++;
	}

	// add arcs from last node and time to a super sink
	nArcsNu = 0;
	i = model.network.nPhysicalLevels - 1;
	nodNr2 = adderaNod(i + 1, 0, 0);
	i1 = 0;
	i3 = getTimeIntervalFromTidp(i, i1, i + 1, i1, tidP);
	addEndBage(i, i1, i + 1, i3, nodNr2);
	nArcsNu++;

	nodNr1 = nodNr2;
	nodNr2 = adderaNod(i + 1, 0, 0);
	addEndBage(i + 1, 0, i + 2, 0, nodNr2);

	nArcsTot += nArcsNu;

	// initValues
	model.results.iterTotDistStart = char_to_double(objects[1]);
	model.results.iterTotFuelStart = char_to_double(objects[3]);
	model.results.iterTotObjStart = char_to_double(objects[5]);
	model.results.iterTotDollarCostStart = char_to_double(objects[7]);
	model.iterKaoutar.tidpStartIter_h = char_to_double(objects[9]);
	model.iterKaoutar.tidpStartIterArc_h = char_to_double(objects[11]);
	model.results.iterWeatherFactorsStart = char_to_double(objects[13]);
	model.results.iterSafetyStart = char_to_double(objects[15]);


	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol);	// totFuel
	model.results.fuel = char_to_double(objects[1]);
	model.results.time = char_to_double(objects[3]);
	model.results.fuelCost = char_to_double(objects[5]);
	model.results.timeCost = char_to_double(objects[7]);
	model.results.dist = char_to_double(objects[9]);
	model.results.safety = char_to_double(objects[11]);
	model.results.objCost = char_to_double(objects[13]);
	model.results.totWeatherFactors = char_to_double(objects[15]);
	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // weightTime
	model.results.weightTime = char_to_double(objects[1]);
	model.results.weightFuel = char_to_double(objects[3]);
	model.results.weightEmission = char_to_double(objects[5]);
	model.results.weightSafetyBase = char_to_double(objects[7]);

	return nArcsTot;
}

int createArcsFromFixSol2(int runAlt)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0;
	double fuel, safety, tid, totCost, fuelQualityKvot, extraAreaCostKvot;

	model.nErrorCoordBB = 0;

	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		if (i == 0) {
			model.nArcs = 0;
			model.nNoder = 0;
			model.network.physicalLev[i].nTimeIntervals[0] = 1;
			model.network.physicalLev[i].timeInterval[0][0] = 0; // model.params.startDelay_h;
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

	int antal, fromLevel, toLevel, fromPos, toPos, fromTime, toTime, speedSetting, tidP;
	int speedSettingBase;
	tidP = 0;
	nArcsTot = 0;
	for (i = 0; i < model.iterKaoutar.nFixedArcs; i++) {
		fromLevel = model.iterKaoutar.fixedArcs[i].fromLevel;
		toLevel = model.iterKaoutar.fixedArcs[i].toLevel;
		fromPos = model.iterKaoutar.fixedArcs[i].fromPos;
		toPos = model.iterKaoutar.fixedArcs[i].toPos;
		//fromTime = (int)(char_to_double(objects[6]) * model.params.nTidsperioder_perH);
		toTime = (int)(model.iterKaoutar.fixedArcs[i].toTime * model.params.nTidsperioder_perH);
		speedSettingBase = model.iterKaoutar.fixedArcs[i].speedSettingBase;
		speedSetting = getClosestSetting_fromBase(speedSettingBase, fromLevel, toLevel);

		extraAreaCostKvot = get_totalExtraAreaCostKvot(fromLevel, fromPos, toLevel, toPos, &fuelQualityKvot);

		setupCheckPoints = 1;
		i3 = getTimeIntervalFromTidp(fromLevel, fromPos, toLevel, toPos, tidP);
		if (fromLevel == -4 && toLevel == -4)
			fromLevel = fromLevel;
		arcNr = addEnBage_AB(fromLevel, fromPos, toLevel, toPos, i3, speedSetting, &setupCheckPoints, 0, toTime + 10000, fuelQualityKvot, extraAreaCostKvot, runAlt);
		tidP = model.arc[arcNr].toTime;

		nArcsTot++;
	}

	// add arcs from last node and time to a super sink
	nArcsNu = 0;
	i = model.network.nPhysicalLevels - 1;
	nodNr2 = adderaNod(i + 1, 0, 0);
	i1 = 0;
	i3 = getTimeIntervalFromTidp(i, i1, i + 1, i1, tidP);
	addEndBage(i, i1, i + 1, i3, nodNr2);
	nArcsNu++;

	nodNr1 = nodNr2;
	nodNr2 = adderaNod(i + 1, 0, 0);
	addEndBage(i + 1, 0, i + 2, 0, nodNr2);

	nArcsTot += nArcsNu;
	return nArcsTot;
}

int loadIterData_kaoutar(FILE* fileFixSol, int runAlt)
{
	int i, i1, i2, i3, setupCheckPoints;
	int tidInt, nArcsTot, min_t, max_t, n_added_t, nArcsNu;
	int i2b, nodNr1, nodNr2, posNy, arcNr, nextLevel;
	int cNr, tidInt0;
	double fuel, safety, tid, totCost, fuelQualityKvot, extraAreaCostKvot;
	FILE* filpek;
	char* namn;

	model.nErrorCoordBB = 0;
	checkMinnesAnvandning(__LINE__);



	dataStr* data = (dataStr*)malloc(sizeof(dataStr));
	char objects[100][CHAR_ALLOC];
	int antal, fromLevel, toLevel, fromPos, toPos, fromTime, toTime, speedSetting, tidP;
	double tidp_h;

	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // forecastType
	model.results.forecastTypeOrig = char_to_int(objects[1]);
	model.results.forecastType = model.results.forecastTypeOrig % 1000;
	if (model.results.forecastType > 300)
		model.results.onlyPrefPath_kaoutar = 1;
	else
		model.results.onlyPrefPath_kaoutar = 0;
	model.results.forecastType = model.results.forecastType % 300;

	if (model.results.onlyPrefPath_kaoutar == 1) {
		model.params.max_changeDirection = 0;
		model.params.max_changeDirection_factorStartEnd = 0;
	}
	else {
		model.params.max_changeDirection = model.params.max_changeDirection_base;
		model.params.max_changeDirection_factorStartEnd = model.params.max_changeDirection_factorStartEnd_base;
	}

	if (model.iterKaoutar.physLevelStart == 0 && runAltForecast > 0) {
		model.iterKaoutar.physPointStart = 0;
		model.iterKaoutar.tidpStartIter_h = 0;
		model.iterKaoutar.tidpStartIterArc_h = 0;
		model.iterKaoutar.fixedArcsEnd = 0;
		return 0;

	}

	if (model.iterKaoutar.physLevelStartUse >= model.network.nPhysicalLevels) {
		namn = (char*)malloc(256 * sizeof(char));

		sprintf(namn, "%s/iterTotalResults.txt", model.params.resultPath.c_str());
		filpek = fopen(namn, "a+");
		fprintf(filpek, "%d;%d;%d;%d;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n", -1, -1,
				-1, -1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
		fclose(filpek);

		sprintf(namn, "%s/breakFile.txt", model.params.resultPath.c_str());
		filpek = fopen(namn, "w");
		fprintf(filpek, "1\n");
		fclose(filpek);

		sprintf(namn, "%s/OptSnurraStatus.txt", model.params.resultPath.c_str());
		filpek = fopen(namn, "w");
		fprintf(filpek, "2\n");
		fclose(filpek);

		return -1; // the runs are done...
	}

	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // nBVArcs
	if (model.iterKaoutar.fixedArcs == NULL)
		model.iterKaoutar.fixedArcs = (strFixedArc*)malloc((2 * model.network.nChannels + model.network.nPhysicalLevels) * sizeof(strFixedArc));

	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // fieldNames

	if (model.iterKaoutar.physLevelStart == 0) {
		model.iterKaoutar.physPointStart = 0;
		model.iterKaoutar.physLevelStartUse = 0;
		model.iterKaoutar.fixedArcsEnd = 0;
		model.iterKaoutar.startTimeUse = 0;
	}
	else
		model.iterKaoutar.physPointStart = -1;

	tidP = 0;
	for (i = 0; i < 10000; i++) {
		antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol);
		if (antal != 9)
			break; // done with the arcs
		if (i >= 20)
			i = i;
		fromLevel = char_to_int(objects[2]);
		toLevel = char_to_int(objects[3]);
		fromPos = char_to_int(objects[4]);
		toPos = char_to_int(objects[5]);
		// fromTime = (int)(char_to_double(objects[6]) * model.params.nTidsperioder_perH);
		// toTime = (int)(char_to_double(objects[7]) * model.params.nTidsperioder_perH);
		tidp_h = char_to_double(objects[7]);
		speedSetting = char_to_int(objects[8]);
		if (toLevel >= model.iterKaoutar.physLevelStart && model.iterKaoutar.physPointStart == -1) {
			model.iterKaoutar.physPointStart = toPos;
			model.iterKaoutar.physLevelStartUse = toLevel;
			model.iterKaoutar.startTimeUse = tidp_h;
			model.iterKaoutar.fixedArcsEnd = i + 1;

			// model.iterKaoutar.tidpStartIter_h = tidp_h;
		}
		//if (i < model.iterKaoutar.physLevelStart) {
		model.iterKaoutar.fixedArcs[i].arcNr = char_to_int(objects[1]);
		model.iterKaoutar.fixedArcs[i].fromLevel = fromLevel;
		model.iterKaoutar.fixedArcs[i].toLevel = toLevel;
		model.iterKaoutar.fixedArcs[i].fromPos = fromPos;
		model.iterKaoutar.fixedArcs[i].toPos = toPos;
		model.iterKaoutar.fixedArcs[i].fromTime = char_to_double(objects[6]);
		model.iterKaoutar.fixedArcs[i].toTime = tidp_h;
		model.iterKaoutar.fixedArcs[i].speedSettingBase = speedSetting;
		//}
	}
	model.iterKaoutar.nFixedArcs = i;

	// initValues
	model.results.iterTotDistStart = char_to_double(objects[1]);
	model.results.iterTotFuelStart = char_to_double(objects[3]);
	model.results.iterTotObjStart = char_to_double(objects[5]);
	model.results.iterTotDollarCostStart = char_to_double(objects[7]);
	model.iterKaoutar.tidpStartIter_h = char_to_double(objects[9]);
	model.iterKaoutar.tidpStartIterArc_h = char_to_double(objects[11]);
	model.results.iterWeatherFactorsStart = char_to_double(objects[13]);
	model.results.iterSafetyStart = char_to_double(objects[15]);

	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol);	// totFuel
	model.results.fuel = char_to_double(objects[1]);
	model.results.time = char_to_double(objects[3]);
	model.results.fuelCost = char_to_double(objects[5]);
	model.results.timeCost = char_to_double(objects[7]);
	model.results.dist = char_to_double(objects[9]);
	model.results.safety = char_to_double(objects[11]);
	model.results.objCost = char_to_double(objects[13]);
	model.results.totWeatherFactors = char_to_double(objects[15]);
	antal = get_data_objects_till_EOL_semkol(objects, data, fileFixSol); // weightTime
	model.results.weightTime = char_to_double(objects[1]);
	model.results.weightFuel = char_to_double(objects[3]);
	model.results.weightEmission = char_to_double(objects[5]);
	model.results.weightSafetyBase = char_to_double(objects[7]);

	return 0;
}

int genExtraOpts() {
	int nExtraOpt = 0;
	int nAlloc = 5;

	//errlog("ERROR! remove the below row to do multiple opts\n");
	//return 0;


	model.params.extraOptWeights = (strExtraWeights*)malloc2(nAlloc * sizeof(strExtraWeights));

	printf("weightEmission %.2lf\nweightTime %.2lf\nweightFuel %.2lf\nweightSafetyBase %.2lf\neta_h %.2lf\n",
		model.params.weightEmission, model.params.weightTime, model.params.weightFuel,
		model.params.weightSafety.base, model.params.eta_h);

	// least$Cost
	if (model.params.weightEmission > 0.15 || abs(model.params.weightTime - 1) > 0.01 ||
		abs(model.params.weightFuel - 1) > 0.01 || abs(model.params.weightSafety.base - 0.1) > 0.1) {
		printf("add extra opt leastDCost\n");
		model.params.extraOptWeights[nExtraOpt].weightEmission = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightTime = 1;
		model.params.extraOptWeights[nExtraOpt].weightFuel = 1;
		model.params.extraOptWeights[nExtraOpt].weightSafetyBase = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightDistance = 0.0;
		model.params.extraOptWeights[nExtraOpt].eta_cost_early = model.params.eta_cost_early;
		model.params.extraOptWeights[nExtraOpt].identifierOpt = str_alloc_cpy("leastDCost");
		nExtraOpt++;
	}

	// lowestEmission
	if (abs(model.params.weightEmission - 1) > 0.01 || abs(model.params.weightTime - 0.1) > 0.01 ||
		abs(model.params.weightFuel - 0.1) > 0.01 || abs(model.params.weightSafety.base - 0.1) > 0.1) {
		printf("add extra opt lowestEmission\n");
		model.params.extraOptWeights[nExtraOpt].weightEmission = 1;
		model.params.extraOptWeights[nExtraOpt].weightTime = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightFuel = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightSafetyBase = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightDistance = 0.0;
		model.params.extraOptWeights[nExtraOpt].eta_cost_early = model.params.eta_cost_early;
		model.params.extraOptWeights[nExtraOpt].identifierOpt = str_alloc_cpy("lowestEmission");
		nExtraOpt++;
	}

	// shortTime
	if (model.params.weightEmission > 0.15 || abs(model.params.weightTime - 1) > 0.01 ||
		abs(model.params.weightFuel - 0.1) > 0.01 || abs(model.params.weightSafety.base - 0.1) > 0.1) {
		printf("add extra opt shortestTime\n");
		model.params.extraOptWeights[nExtraOpt].weightEmission = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightTime = 1;
		model.params.extraOptWeights[nExtraOpt].weightFuel = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightSafetyBase = 0.01;
		model.params.extraOptWeights[nExtraOpt].weightDistance = 0.0;
		model.params.extraOptWeights[nExtraOpt].eta_cost_early = model.params.eta_cost_early;
		model.params.extraOptWeights[nExtraOpt].identifierOpt = str_alloc_cpy("shortestTime");
		nExtraOpt++;
	}

	// shortestPath
	if (model.params.weightEmission > 0.01 || abs(model.params.weightTime - 0.01) > 0.01 ||
		abs(model.params.weightFuel - 0.01) > 0.01 || abs(model.params.weightSafety.base - 0.01) > 0.1) {
		printf("add extra opt shortestPath\n");
		model.params.extraOptWeights[nExtraOpt].weightEmission = 0.00001;
		model.params.extraOptWeights[nExtraOpt].weightTime = 0.000001;
		model.params.extraOptWeights[nExtraOpt].weightFuel = 0.00001;
		model.params.extraOptWeights[nExtraOpt].weightSafetyBase = 0.00001;
		model.params.extraOptWeights[nExtraOpt].weightDistance = 1;
		model.params.extraOptWeights[nExtraOpt].eta_cost_early = model.params.eta_cost_early;
		model.params.extraOptWeights[nExtraOpt].identifierOpt = str_alloc_cpy("shortestPath");
		nExtraOpt++;
	}

	// eta with same weights but without too early cost
	if (model.params.eta_h > 0) {
		printf("add extra opt eta without too early cost\n");
		model.params.extraOptWeights[nExtraOpt].weightEmission = model.params.weightEmission;
		model.params.extraOptWeights[nExtraOpt].weightTime = model.params.weightTime;
		model.params.extraOptWeights[nExtraOpt].weightFuel = model.params.weightFuel;
		model.params.extraOptWeights[nExtraOpt].weightSafetyBase = model.params.weightSafety.base;
		model.params.extraOptWeights[nExtraOpt].weightDistance = 0;
		model.params.extraOptWeights[nExtraOpt].eta_cost_early = 0;
		model.params.extraOptWeights[nExtraOpt].identifierOpt = str_alloc_cpy("eta_onlyLateCost");
		nExtraOpt++;
	}

	return nExtraOpt;
}


int modify_utNodCost(int alt) {
	int i, i1, arcNr;
	double cost, weightTime, weightFuel, weightSafety, weightDistance, weightEmission;

	if (alt > 0) {
		weightTime = model.params.extraOptWeights[alt - 1].weightTime;
		weightFuel = model.params.extraOptWeights[alt - 1].weightFuel;
		weightSafety = model.params.extraOptWeights[alt - 1].weightSafetyBase;
		weightDistance = model.params.extraOptWeights[alt - 1].weightDistance;
		weightEmission = model.params.extraOptWeights[alt - 1].weightEmission;
	}
	else {
		weightTime = model.params.weightTime;
		weightFuel = model.params.weightFuel;
		weightSafety = model.params.weightSafety.base;
		weightDistance = 0;
		weightEmission = model.params.weightEmission;
	}

	double maxCost = 0;
	for (i = 0; i < model.nNoder; i++) {
		for (i1 = 0; i1 < model.Noder[i].nUtNoder; i1++) {
			arcNr = model.Noder[i].outArcNr[i1];
			if (arcNr == 74)
				arcNr = arcNr;
			if (model.arc[arcNr].fromLevel == 7 && model.arc[arcNr].fromPointNr == 26 && model.arc[arcNr].toPointNr == 25 ||
				model.arc[arcNr].fromLevel == 8 && model.arc[arcNr].fromPointNr == 25 && model.arc[arcNr].toPointNr == 22 ||
				model.arc[arcNr].fromLevel == 9 && model.arc[arcNr].fromPointNr == 22 && model.arc[arcNr].toPointNr == 23)
				arcNr = arcNr;
			if (model.arc[arcNr].fromLevel == 7 && model.arc[arcNr].fromPointNr == 26 && model.arc[arcNr].toPointNr == 24 ||
				model.arc[arcNr].fromLevel == 8 && model.arc[arcNr].fromPointNr == 24 && model.arc[arcNr].toPointNr == 23 ||
				model.arc[arcNr].fromLevel == 9 && model.arc[arcNr].fromPointNr == 23 && model.arc[arcNr].toPointNr == 23)
				arcNr = arcNr;

			cost = weightTime * model.params.priceTime * model.arc[arcNr].time +
				weightFuel * model.arc[arcNr].fuelBase + weightSafety * model.arc[arcNr].safetyBase +
				model.arc[arcNr].distance * weightDistance + weightEmission * model.arc[arcNr].emission * model.params.scaleObjEmission;

			if (model.arc[arcNr].toLevel == model.network.nPhysicalLevels && model.params.eta_h > 0.01) {
				if (model.arc[arcNr].fromTime * model.params.tIndexGerH < model.params.eta_h)
					cost += (model.params.eta_h - model.arc[arcNr].fromTime * model.params.tIndexGerH) * model.params.extraOptWeights[alt - 1].eta_cost_early;
				else
					cost += (model.arc[arcNr].fromTime * model.params.tIndexGerH - model.params.eta_h) * model.params.eta_cost_late;
			}

			model.Noder[i].UtNodCost[i1] = cost;
			model.arc[arcNr].totCost = cost;
			if (cost > maxCost)
				maxCost = cost;
			if (cost < 0) {
				printf("nod %d pos %d arcNr %d wTime %.2lf price %.2lf time %.2lf wFuel %.2lf fBase %.2lf wEmission %.2lf emission %.2lf wSafe %.2lf sBase %.2lf dist %.2lf wDist %.2lf\n",
					i, i1, arcNr, weightTime, model.params.priceTime, model.arc[arcNr].time,
					weightFuel, model.arc[arcNr].fuelBase, weightEmission, model.arc[arcNr].emission, weightSafety, model.arc[arcNr].safetyBase,
					model.arc[arcNr].distance, weightDistance);
			}
			//if (model.arc[arcNr].fromLevel >= 3 && model.arc[arcNr].toLevel <= 5 && model.arc[arcNr].fromPointNr >= 22
			//	&& model.arc[arcNr].fromPointNr <= 24 && model.arc[arcNr].toPointNr >= 22 && model.arc[arcNr].toPointNr <= 24)
			//	errlog("arcNr %d noder %d %d levels %d %d pointNrs %d %d timeInts %d %d dist %.3lf speedSetting %d cost %.3lf\n",
			//		arcNr, model.arc[arcNr].nodNr1, model.arc[arcNr].nodNr2, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel,
			//		model.arc[arcNr].fromPointNr, model.arc[arcNr].toPointNr, model.arc[arcNr].fromTime,
			//		model.arc[arcNr].toTime, model.arc[arcNr].distance, model.arc[arcNr].speedSetting, cost);
		}
	}

	if (maxCost > 0)
		model.Dijkstra.FAKTOR_NATVERK = (MAXVARDE_NATVERK / maxCost);

	return 0;
}

int savePathToSolutionCheck(int pos) {
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/solutionCheck_%d.txt", model.params.indataPath.c_str(), pos);
	FILE* filpek = fopen(namn, "w");

	for (int i = 0; i < model.nBVArcs; i++)
		fprintf(filpek, "%d %d %d\n",
			model.arc[model.BVArc[i]].toLevel, model.arc[model.BVArc[i]].toPointNr,
			model.arc[model.BVArc[i]].speedSetting);

	fclose(filpek);
	return 0;
}

double genBV_franFixLsning(strModel* model, long long* Cost) {
	int i, i1, nNoder = 0, nodNr, nArcsOpt;
	int arcNr, lev1, lev2, speedSetting, pointNr2, antal;
	long long TotCost = 0;
	double dist = 0;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/solutionCheck.txt", model->params.indataPath.c_str());
	FILE* filpek = fopen(namn, "r");

	nArcsOpt = model->nBVArcs;
	nodNr = 0;
	for (i = 0; i < 100000; i++) {
		antal = fscanf(filpek, "%d %d %d\n", &lev2, &pointNr2, &speedSetting);
		if (antal < 3)
			break;

		for (i1 = 0; i1 < model->Noder[nodNr].nUtNoder; i1++) {
			arcNr = model->Noder[nodNr].outArcNr[i1];
			if (model->arc[arcNr].toLevel == lev2 && model->arc[arcNr].toPointNr == pointNr2 &&
				model->arc[arcNr].speedSetting == speedSetting)
				break;
		}
		if (i1 >= model->Noder[nodNr].nUtNoder) {
			errlog("ERROR! No solution found for fix solution, outnod %d. No arc levels %d %d pointPos %d %d speedSetting %d. I don't save the fix solution\n",
				nodNr, model->Noder[nodNr].physicalLevel, lev2, model->Noder[nodNr].pointNr,
				pointNr2, speedSetting);
			fclose(filpek);
			return -1.0;
		}
		model->BVArc[i] = arcNr;
		dist += model->arc[model->BVArc[i]].distance;
		TotCost += model->arc[model->BVArc[i]].totCost;
		nodNr = model->Noder[nodNr].UtNod[i1];
	}
	fclose(filpek);
	if (i > nArcsOpt + 4 || i < nArcsOpt - 4) {
		errlog("ERROR! The fix solution route from solutionCheck.txt has wrong number of arcs, is %d should be about %d. I skip this\n",
			i, nArcsOpt);
		printf("ERROR! The fix solution route from solutionCheck.txt has wrong number of arcs, is %d should be about %d. I skip this\n",
			i, nArcsOpt);
		return -1.0;
	}
	model->nBVArcs = i;
	*Cost = TotCost;

	return dist;
}

long long evalCostFixLsning() {
	int i;
	double TotCost = 0;
	double dist = 0;

	for (i = 0; i < model.nBVArcs; i++) {
		TotCost += model.arc[model.BVArc[i]].totCost;
		dist += model.arc[model.BVArc[i]].distance;
		//errlog("pos %d arcNr %d dist %.2lf totDist %.2lf cost %.2lf totCost %I64d\n", i, model.BVArc[i], 
		//	model.arc[model.BVArc[i]].distance, dist,
		//	model.arc[model.BVArc[i]].totCost, TotCost);
	}
	return (long long)TotCost;
}

int initLookUpTables() {
	for (int index = 0; index < 20001; index++)
	{
		cos_table[index] = std::cos(M_PI * (index) / 10000.0);
		sin_table[index] = std::sin(M_PI * (index) / 10000.0);
		atan_table[index] = std::atan(M_PI * (index) / 10000.0);
	}
	LOOKUP_COS_STEP_INV = 10000.0 / M_PI;
	return 0;
}

int readParameterInfoForTable(json data, strTableParam* param, std::string namn, int useFactor, double skalning = 1.0) {
	int nError = 0;
	if (useFactor == 0)
		skalning = 1.0;

	if (!data[namn].is_null()) {
		json dataNu = data[namn];
		if (!dataNu["minValue"].is_null()) {
			param->minValue = (double)(dataNu["minValue"]) * skalning;
		}
		else {
			errlog("ERROR! minValue is missing for parameter %s\n", namn.c_str());
			nError = 1;
		}
		if (!dataNu["maxValue"].is_null()) {
			param->maxValue = (double)(dataNu["maxValue"]) * skalning;
		}
		else {
			errlog("ERROR! maxValue is missing for parameter %s\n", namn.c_str());
			nError = 1;
		}
		if (!dataNu["intervalSize"].is_null()) {
			param->intervalSize = (double)(dataNu["intervalSize"]) * skalning;
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
	return nError;
}

int loadTablesInfo(int useFactor)
{
	double maxWaveHeight, maxWaveHeight_warning;
	double maxWindSpeed, maxWindSpeed_warning;
	std::ifstream fil;
	char* namn;
	std::string nameTable;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/table_parameters.json", model.params.indataPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		postRequest(std::string(namn) + " does not exist. I quit", 1);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);

	json data;

	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}

	int typeNr, pos, paramsError, i, nTypes = 3;
#ifdef NAZANIN_SAFETY
	nTypes = 7;
#endif

	model.tables.nBasAlloc = 20;
	for (i = 0; i < nTypes; i++) {
		model.tables.nTableTyp[i] = 0;
		model.tables.nAllocTableTyp[i] = model.tables.nBasAlloc;
		model.tables.tableTyp[i] = (strTableTyp*)malloc2(model.tables.nAllocTableTyp[i] * sizeof(strTableTyp));
	}

	std::string namnID, tableID;

	for (auto it = data.begin(); it != data.end(); ++it) {
		json dataTable = it.value();

		if (!dataTable["tableType"].is_null()) {
			if (!dataTable["tableID"].is_null())
				tableID = str_alloc_cpyString(cleanString(dataTable["tableID"]));
			else {
				errlog("ERROR! tableID missing for a table in %s. It must be there, I skip this one.\n", namn);
				continue;
			}

			std::string tableType = dataTable["tableType"];
			if (tableType == "wind")
				typeNr = 0;
			else if (tableType == "wave")
				typeNr = 1;
			else if (tableType == "stability")
				typeNr = 2;
			else if (tableType == "bowSlamming")
				typeNr = 3;
			else if (tableType == "greenWater")
				typeNr = 4;
			else if (tableType == "rolling")
				typeNr = 5;
			else if (tableType == "surfRiding")
				typeNr = 6;

			if (useFactor == 1) {
				if (typeNr == 0)
					namnID = model.functions.windTableID;
				else if (typeNr == 1)
					namnID = model.functions.waveTableID;
				else if (typeNr == 2)
					namnID = model.functions.stabilityTableID;
				else if (typeNr == 3)
					namnID = model.functions.bowSlammingTableID;
				else if (typeNr == 4)
					namnID = model.functions.greenWaterTableID;
				else if (typeNr == 5)
					namnID = model.functions.rollingTableID;
				else if (typeNr == 6)
					namnID = model.functions.surfRidingTableID;
				if (namnID != tableID)
					continue;
			}

			pos = model.tables.nTableTyp[typeNr];
			if (pos >= model.tables.nAllocTableTyp[typeNr]) {
				model.tables.nAllocTableTyp[typeNr] += model.tables.nBasAlloc;
				model.tables.tableTyp[typeNr] = (strTableTyp*)realloc(model.tables.tableTyp[typeNr], model.tables.nAllocTableTyp[typeNr] * sizeof(strTableTyp));
			}

			if (!dataTable["tableID"].is_null())
				model.tables.tableTyp[typeNr][pos].tableID = str_alloc_cpyString(cleanString(dataTable["tableID"]));
			else {
				errlog("ERROR! tableID missing for a table in %s. It must be there, I skip this one.\n", namn);
				continue;
			}

			if (!dataTable["fileName"].is_null())
				model.tables.tableTyp[typeNr][pos].fileName = str_alloc_cpyString(dataTable["fileName"]);
			else {
				errlog("ERROR! fileName is missing for tableID %s in %s. It must be there, I skip this one.\n", model.tables.tableTyp[typeNr][pos].tableID, namn);
				continue;
			}
			if (!dataTable["parameters"].is_null()) {
				paramsError = 0;
				json dataParam = dataTable["parameters"];
				if (typeNr == 0) { // wind
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].shipSpeedCalmWater), "shipSpeed_calmWater_knots", useFactor, model.params.knots_to_km);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].windSpeed), "relativeWindSpeed_m_s", useFactor, 3.6);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].windDirection), "relativeWindDirection", useFactor, M_PI / 180.0);
					if (!dataTable["maxWindSpeed_warning"].is_null()) {
						model.tables.tableTyp[typeNr][pos].maxWindSpeed_warning = (double)(dataTable["maxWindSpeed_warning"]) * model.params.knots_to_km;
					}
					else {
						maxWindSpeed_warning = 28 * model.params.knots_to_km;
						errlog("OBS! No maxWindSpeed_warning is set for parameter %s, I set it to %.2lf knots\n", model.tables.tableTyp[typeNr][pos].fileName,
							maxWindSpeed_warning / model.params.knots_to_km);
						model.tables.tableTyp[typeNr][pos].maxWindSpeed_warning = maxWindSpeed_warning;
					}
				}
				else if (typeNr == 1) { // wave
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].shipSpeedCalmWater), "shipSpeed_calmWater_knots", useFactor, model.params.knots_to_km);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveHeight), "significantWaveHeight_m", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].wavePeriod), "meanWavePeriod_s", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveDirection), "relativeWaveDirection", useFactor, M_PI / 180.0);
					if (!dataTable["maxWaveHeight"].is_null()) {
						model.tables.tableTyp[typeNr][pos].maxWaveHeight = (double)(dataTable["maxWaveHeight"]);
					}
					else {
						maxWaveHeight = 8.5;
						errlog("ERROR! maxWaveHeight is missing for parameter %s, I set it to %.1lf meters\n", model.tables.tableTyp[typeNr][pos].fileName, maxWaveHeight);
						model.tables.tableTyp[typeNr][pos].maxWaveHeight = maxWaveHeight;
					}
					if (!dataTable["maxWaveHeight_warning"].is_null()) {
						model.tables.tableTyp[typeNr][pos].maxWaveHeight_warning = (double)(dataTable["maxWaveHeight_warning"]);
					}
					else {
						maxWaveHeight_warning = 4.99; //  7.0;
						errlog("OBS! No maxWaveHeight_warning is set for parameter %s, I set it to %.2lf meters\n", model.tables.tableTyp[typeNr][pos].fileName,
							maxWaveHeight_warning);
						model.tables.tableTyp[typeNr][pos].maxWaveHeight_warning = maxWaveHeight_warning;
					}
				}
				else if (typeNr == 2) { // stability
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].windSpeed), "absoluteWindSpeed_knots", useFactor, model.params.knots_to_km);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].windDirection), "windDirectionDiff", useFactor, 1.0);// M_PI / 180.0);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].shipSpeedOverGround), "shipSpeedOverGround_knots", useFactor, model.params.knots_to_km);
				}
				else if (typeNr == 3) { // bowSlamming
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveHeight), "significantWaveHeight_m", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].wavePeriod), "meanWavePeriod_s", useFactor);
				}
				else if (typeNr == 4) { // greenWater
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveHeight), "significantWaveHeight_m", useFactor);
				}
				else if (typeNr == 5) { // rolling
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].relShipSpeed), "shipSpeed_relative_knots", useFactor, model.params.knots_to_km);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveHeight), "significantWaveHeight_m", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].wavePeriod), "meanWavePeriod_s", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveDirection), "relativeWaveDirection", useFactor, M_PI / 180.0);
				}
				else if (typeNr == 6) { // surfRiding
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].relShipSpeed), "shipSpeed_relative_knots", useFactor, model.params.knots_to_km);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveHeight), "significantWaveHeight_m", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].wavePeriod), "meanWavePeriod_s", useFactor);
					paramsError += readParameterInfoForTable(dataTable["parameters"], &(model.tables.tableTyp[typeNr][pos].waveDirection), "relativeWaveDirection", useFactor, M_PI / 180.0);
				}
			}
			else {
				errlog("ERROR! parameters is missing for tableID %s in %s. It must be there, I skip this one.\n", model.tables.tableTyp[typeNr][pos].tableID, namn);
				continue;
			}
			if (paramsError > 0) {
				errlog("ERROR! tableID %s in file %s not read properly, error in parameters\n", model.tables.tableTyp[typeNr][pos].tableID, namn);
				continue;
			}

			(model.tables.nTableTyp[typeNr])++;
		}


	}

	for (i = 0; i < 3; i++) {
		if (model.tables.nTableTyp[i] < 1) {
			if (useFactor == 0) {
				errlog("ERROR! No tables in %s of type %d (0 wind, 1 wave, 2 stability). There must be at least one. I quit\n", namn, i);
				postRequest("ERROR! No tables in " + std::string(namn) + " of type " + std::to_string(i) + " (0 wind, 1 wave, 2 stability).There must be at least one.I quit", 1);
			}
			else {
				if (i == 0)
					namnID = model.functions.windTableID;
				else if (i == 1)
					namnID = model.functions.waveTableID;
				else if (i == 2)
					namnID = model.functions.stabilityTableID;
				errlog("ERROR! Table %s typ %d is not defined in table_parameters.json. Fix this, generate new redis values and then try again. I quit\n", namnID.c_str(), i);
				postRequest("ERROR! Table " + namnID + " is not defined in table_parameters.json. Fix this, generate new redis values and then try again. I quit", 1);
			}
		}
		for (int i1 = 0; i1 < model.tables.nTableTyp[i]; i1++) {
			for (int i2 = i1 + 1; i2 < model.tables.nTableTyp[i]; i2++) {
				if (strcmp(model.tables.tableTyp[i][i1].tableID, model.tables.tableTyp[i][i2].tableID) == 0) {
					errlog("ERROR! Two tables in %s with the same tableID (%S) type %d (0 wind, 1 wave, 2 stability). The first one will be used.\n",
						namn, model.tables.tableTyp[i][i2].tableID, i);
					break;
				}
			}
		}
	}

	if (useFactor == 1) {
		model.functions.windTableNr = 0;
		model.functions.waveTableNr = 0;
		model.functions.stabilityTableNr = 0;
	}


	fil.close();




	return 0;
}

int loadStartDatum_nazanin() {

	std::ifstream fil;
	char* namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/startTidpkt_nazanin.json", model.params.resultPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		exit(0);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);

	json data, dataSpeed, dataVar, dataIt;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}
	fil.close();

	model.params.UTC_secondsStart = make_gmtime_fromDateTimeString(data["startDateTime"], &(model.params));

	free(namn);

	return 0;
}

int loadVesselTableIDs_nazanin() {

	std::ifstream fil;
	char* namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/vesselTableIDs_nazanin.json", model.params.resultPath.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		exit(0);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);

	json dataShip;
	try {
		fil >> dataShip;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}
	fil.close();


	if (!dataShip["tableID_wind"].is_null()) {
		model.functions.windTableID = cleanString(dataShip["tableID_wind"]);
	}
	else {
		errlog("OBS No windTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}
	if (!dataShip["tableID_wave"].is_null()) {
		model.functions.waveTableID_orig = dataShip["tableID_wave"];
		model.functions.waveTableID = cleanString(dataShip["tableID_wave"]);
	}
	else {
		errlog("OBS No waveTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}
	if (!dataShip["tableID_stability"].is_null()) {
		model.functions.stabilityTableID = cleanString(dataShip["tableID_stability"]);
	}
	else {
		errlog("OBS No stabilityTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}
	if (!dataShip["tableID_bowSlamming"].is_null()) {
		model.functions.bowSlammingTableID = cleanString(dataShip["tableID_bowSlamming"]);
	}
	else {
		errlog("OBS No bowSlammingTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}
	if (!dataShip["tableID_greenWater"].is_null()) {
		model.functions.greenWaterTableID = cleanString(dataShip["tableID_greenWater"]);
	}
	else {
		errlog("OBS No greenWaterTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}

	if (!dataShip["tableID_rolling"].is_null()) {
		model.functions.rollingTableID = cleanString(dataShip["tableID_rolling"]);
	}
	else {
		errlog("OBS No rollingTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}
	if (!dataShip["tableID_surfRiding"].is_null()) {
		model.functions.surfRidingTableID = cleanString(dataShip["tableID_surfRiding"]);
	}
	else {
		errlog("OBS No surfRidingTableID given in vesselTableIDs_nazanin.json in tag 'ship_specification', I use the one from the input data file.\n");
	}
	free(namn);

	return 0;
}

int voyageOpt(std::string inputPath, std::string resultName)
{

	double dist, cost;
	long long Cost;
	reset_errlog();
	//callRaster();

	FILE* filPek3;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	model.timeStart = std::chrono::high_resolution_clock::now();
	init_tmBas();

	initLookUpTables();


	initModelStatusValues();

	model.params.resultPath = splitFilename(resultName);
	model.params.resultName = resultName;

	filPek3 = fopen(resultName.c_str(), "w"); // "result_json.json", "w");
	fprintf(filPek3, "{\n\t\"errorMessage\": \"unknown error\"\n}\n");
	fclose(filPek3);

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;
	model.params.hindCast = 0;
	model.params.failedTime = 0;
	model.params.etaFocus_speed = 0;
	model.params.UTC_secondsStart = 0;
	model.results.fileForecast = NULL;
	model.results.fileForecast2 = NULL;

	if (runAltForecast > 0)
		model.results.fileNameForecast = splitFilename(inputPath, 1);

	//errlog("#######\nERROR! Change the below code rows as it is for analysis only\n");
	model.params.nSpeedSettingDivideIter1 = 2; // 2 ger 3 speed settings, 4 ger 5 speed settings
	model.params.nTidsperioder_perH_iter1 = 1; // 1 is default, 4 ger var 15:e minut
	model.params.tidp_startHistoricDataOnly_iter1 = 0; // 999999; // 0 is default
	model.results.forecastTypeOrig = -1;
	model.results.onlyPrefPath_kaoutar = 0;


	//model.params.resultPath = resultPath;

	//printf("Reading data for the problem\n");

	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityOptiNav(&(model.params));

	//loadTablesInfo();

	loadParams_new(&(model.params));

#ifdef NAZANIN_SAFETY
	loadStartDatum_nazanin();
	loadVesselTableIDs_nazanin();
#endif

	if (delayVersion == 5)
		errlog("ERROR! OBS delayVersion %d\n", 5);

	// testSaveMapToSQLite();
	// testSaveMapToBinaryFile();

	//printf("obj weight dynamicStability %.3lf\n", model.params.weightSafety.dynamicStability);

	loadAllNeededTablesFromSQLite();

	loadVariables();

	model.corridorPath.nLines = 0;

	printf("-- Time before creating physical network %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	createPhysicalNetwork(0, 0);


	setupUsableSpeedSettings();

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
	int evalExtraSol = 0, nFinalRoutes = 0;
	int nExtraOpt = 0; // genExtraOpts();
	int nod1, nod2, nSolSaved = 0, endTidsp, i;
	int* solSavedEndTP = NULL;
	int* sol_nSavedArcs = NULL;
	int** solSavedArcs = NULL;
	double* solSavedDist = NULL;
	char* baseName;
	baseName = (char*)malloc2(100 * sizeof(char));
	sprintf(baseName, "base");

	model.BVArc = NULL;

	//model.functions.waveFactor.waveHeight.maxValue = 7.0;
	//errlog("ERROR! Hard coded max wave height now of %.2lf. Fix this\n",
	//	model.functions.waveFactor.waveHeight.maxValue);

	for (int iter = 0; iter < 3; iter++) {
		if (iter > 0) {
			if (iter == 1 && model.params.calmWaterSpeedCompare <= 0 && model.params.fuelCompare <= 0)
				continue; // no reason to do this iteration as there are no speed or fuel to compare with
			freeAllNodData();
			modify_midTimeArrive(iter);
			model.params.maxDiffTimeFastSlow = model.params.maxDiffTimeFastSlow_fas3;
			model.params.nTidsperioder_perH = 4;
			// errlog("ERROR! Change nTidsperioder_perH to 4 above\n");
			model.params.tIndexGerH = 1.0 / model.params.nTidsperioder_perH;
			if (iter == 1)
				sprintf(baseName, "prefPathFixSpeed");
			else
				sprintf(baseName, "base");
		}
		printf("-- Time before creating the time dimension %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
		createTimeArcs(iter);
		printf("-- Time after creating the time dimension %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

		//if(iter == 0)
		//	saveSP_delay(iter);

		// freeAllMemory();

		if (model.network.physicalLev[model.network.nPhysicalLevels - 1].nTimeIntervals[0] == 0) {
			errlog("ERROR! Number of time intervals to the last level is 0. Is the preferred path outside of the extent of the feasibility map? I quit.\n");
			printf("ERROR! Number of time intervals to the last level is 0. Is the preferred path outside of the extent of the feasibility map? I quit.\n");
			postRequest("ERROR! Number of time intervals to the last level (" + std::to_string(model.network.nPhysicalLevels - 1) + ") is 0. Is the preferred path outside of the extent of the feasibility map ? I quit.", 1);
		}

		printf("setting up data for dijkstra's algorithm\n");
		auto tid0 = std::chrono::high_resolution_clock::now();
		SattUppDijkstraNatverk3(&model);
		auto tid1c = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::milli> fp_ms = tid1c - tid0;
		errlog("sattUppDijkstra took %lf\n", fp_ms);
		nod1 = 0;
		nod2 = model.nNoder - 1;
		bool Reached;
		checkMinnesAnvandning(__LINE__);

		if (model.params.simuleraTidVisuellt == 1) {
			sprintf(namn, "%s/solVisuellt.geojson", model.params.indataPath.c_str());
			model.timeVisual.filVisuell = fopen(namn, "w");
			initGeoJsonFil(model.timeVisual.filVisuell, "sol");
			model.timeVisual.pos = 0;
			model.timeVisual.startTime = (char*)malloc(256 * sizeof(char));
			model.timeVisual.tmBas = { 0 };
		}

		model.network.nMaxSplits = DEF_nMAX_SPLITS;

#ifdef _WIN32
		if (model.network.nMaxSplits == 10000) {
			errlog("ERROR! Only one split per arc\n");
			model.network.nMaxSplits = 1;
			evalExtraSol = 0;
			if (model.network.nMaxSplits != 1000 && iter == 0) {
				namn = (char*)malloc2(256 * sizeof(char));
				sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
				FILE* filpek = fopen(namn, "w");
				fprintf(filpek, "arcNr\tnSplit\tnodNr1\tnod1UtPos\tnodNr2\tfromLevel\tfromPointNr\tfromTimeInterval\ttoLevel\ttoPointNr\ttoTimeInterval\ttotCost\t"
					"channelCost\tdistance\temission\tfuelBase\tsafetyBase\tspeedSetting\ttime\ttimeCheck\ttimeElapsed\tdiffTimeToMid\t"
					"accumDist\tlatLon\tbearing\tfuel_day\tworstStormValue\t"
					"currentReal\trelCurrent\tcalmWaterSpeed\tbaseGroundSpeed\tspeedOnGround\trpm\trelWindSpeed\trelWindDir\t"
					"deltaSpeedWind\twaveheight\twavePeriod\trelWaveDir\tdeltaSpeedWave\twindSpeedReal\twindDirReal\t"
					"currentReal\tcurrentDirReal\twaveDirReal\tbowSlamming_max\tgreenWater_max\tdynamiStability_max\ticeCover_max\tforecastType\n");
				fclose(filpek);
			}
		}
#endif 

		if (model.BVArc == NULL) {
			model.nAllocBVArcs = model.nNoder + 10;
			model.BVArc = (int*)malloc(model.nAllocBVArcs * sizeof(int));
			model.BVtempNodOrder = (int*)malloc(model.nAllocBVArcs * sizeof(int));

			model.network.startKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
			model.network.endKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
			model.network.posSplitCoord = (int*)malloc2((model.network.nMaxSplits + 1) * sizeof(int));
			solSavedDist = (double*)malloc2(3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(double));
			solSavedEndTP = (int*)malloc2(3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int));
			sol_nSavedArcs = (int*)malloc2(3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int));
			solSavedArcs = (int**)malloc2(3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int*));

			model.waypointResult.dateUTC = (char*)malloc(256 * sizeof(char));
			model.waypointResult.full_Date = (char*)malloc(256 * sizeof(char));
			model.waypointResult.fixPositionString_latlon = (char*)malloc(256 * sizeof(char));
			model.waypointResult.windDirReal_letters = (char*)malloc(256 * sizeof(char));
			model.waypointResult.waveDir_letters = (char*)malloc(256 * sizeof(char));

		}
		else {
			if (model.nAllocBVArcs < model.nNoder) {
				model.nAllocBVArcs = model.nNoder + 10;
				model.BVArc = (int*)realloc(model.BVArc, model.nAllocBVArcs * sizeof(int));
				model.BVtempNodOrder = (int*)realloc(model.BVtempNodOrder, model.nAllocBVArcs * sizeof(int));
			}
		}

#ifdef NAZANIN_SAFETY
		iter = 2; // only do one optimization for this option
#endif

		std::string resAltName;

		for (int ii = 0; ii < 1 + nExtraOpt; ii++) {
			if (ii > 0) {
				modify_utNodCost(ii);
				ChangeArcCosts3(&model);
			}
			checkMinnesAnvandning(__LINE__);
			printf("\nsolving dijkstra's algorithm..");
			AnropDijkstra2(nod1, nod2, &model, &Reached);
			checkMinnesAnvandning(__LINE__);
			auto tid1c2 = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms2 = tid1c2 - tid0;
			errlog("after Dijkstra %lf\n", fp_ms2);
			printf("..done. Obj %I64d\n", model.Dijkstra.OptCost);
			checkMinnesAnvandning(__LINE__);
			if (Reached == true) {
				//printf("har1\n");
				//printf("har12\n");
				// model.filpek = fopen("checkOptArcCosts.txt", "w");
				dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
				checkMinnesAnvandning(__LINE__);
				// fclose(model.filpek);
				//model.filpek = NULL;
				if (model.nBVArcs < 2) {
					endTidsp = 0;
					errlog("ERROR! Too few arcs %d in Dijkstra solution\n", model.nBVArcs);
				}
				else
					endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
				for (i = 0; i < nSolSaved; i++) {
					if (abs(dist - solSavedDist[i]) < 0.000001 && endTidsp == solSavedEndTP[i])
						break;
				}
				if (i < nSolSaved && ii != 0) {
					errlog("Solution (dist %.3lf) same as earlier saved solution no %d so do not save this one, cost %I64d\n",
						dist, i, Cost);
					continue;
				}
				if (ii == 0)
					errlog("Solution %s (dist %.3lf) will be saved as no %d, cost %I64d\n", baseName,
						dist, i, Cost);
				else
					errlog("Solution %s (dist %.3lf) will be saved as no %d, cost %I64d\n",
						model.params.extraOptWeights[ii - 1].identifierOpt, dist, i, Cost);
				checkMinnesAnvandning(__LINE__);
				solSavedDist[nSolSaved] = dist;
				solSavedEndTP[nSolSaved] = endTidsp;
				//solSavedArcs[nSolSaved] = (int*)malloc2(model.nBVArcs * sizeof(int));
				//for(int ii = 0; ii < model.nBVArcs; ii++)
				//	solSavedArcs[nSolSaved][ii] = model.BVArc[ii];
				//sol_nSavedArcs[nSolSaved] = model.nBVArcs;
				nSolSaved++;

				//printf("dist %.4lf endTidsp %d nSolSaved %d\n", dist, endTidsp, nSolSaved);

				//printf("har13\n");
				//if (ii == 0)
				//	sprintf(namn, "%s/%s", resultPath.c_str(), model.params.solutionFileName.c_str());
				//else
				//	sprintf(namn, "%s/resObj_%d", resultPath.c_str(), ii);

				//printf("har14\n");
				//writeSolutionPathToGeoJson(namn, 0);

				//printf("Saving solution path1..");
				checkMinnesAnvandning(__LINE__);
				if (ii == 0) {
					printf("solution to %s\n", baseName);
					if (iter < 1){// 2 && SKRIV_UT_NOTHING == 0 && (model.params.useSimulering == 0 || iter != 1)) {
						writeSolutionToJson(model.params.resultPath + "/resStep" + std::to_string(iter) + ".json", ii, baseName, iter);
					}
					else {
						// evalCompareSolution();
						writeSolutionToJson(resultName, ii, baseName, iter, nFinalRoutes);
						nFinalRoutes++;
					}
				}
				else {
					printf("solution to %s\n", model.params.extraOptWeights[ii - 1].identifierOpt);
					writeSolutionToJson(resultName, ii, model.params.extraOptWeights[ii - 1].identifierOpt, iter, nFinalRoutes);
					nFinalRoutes++;
				}
				checkMinnesAnvandning(__LINE__);
				//for (int ii = 0; ii < nSolSaved; ii++) {
				//	cost = 0;
				//	for (int ii1 = 0; ii1 < sol_nSavedArcs[ii]; ii1++) {
				//		cost += model.arc[solSavedArcs[ii][ii1]].totCost;
				//		if (ii1 >= sol_nSavedArcs[ii] - 3)
				//			printf("\tarcNr %d cost %.3lf totCost %.3lf TP from/to %d %d levels %d %d endLev %d\n", solSavedArcs[ii][ii1],
				//				model.arc[solSavedArcs[ii][ii1]].totCost, cost, model.arc[solSavedArcs[ii][ii1]].fromTime, model.arc[solSavedArcs[ii][ii1]].toTime,
				//				model.arc[solSavedArcs[ii][ii1]].fromLevel, model.arc[solSavedArcs[ii][ii1]].toLevel, model.network.nPhysicalLevels);
				//	}
				//	printf("savedSol %d totCost %.2lf\n", ii, cost);
				//}

				//printf(".done\n");
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

#ifdef _WIN32
				if (iter == 2 && SKRIV_UT_NOTHING == 0) {
					if (ii == 0)
						errlog("saving solution path for base to solutionCheck_%d.txt, solution no %d\n", ii, nSolSaved - 1);
					else
						errlog("saving solution path for %s to solutionCheck_%d.txt, solution no %d\n",
							model.params.extraOptWeights[ii - 1].identifierOpt, ii, nSolSaved - 1);
					savePathToSolutionCheck(ii);
				}
#endif // _WIN32
			}
			else {
				errlog("ERROR! Did not manage to find a route from start to finish...\n");
				printf("\nERROR! Did not manage to find a route from start to finish...\n");
			}
		}

		if (model.params.simuleraTidVisuellt == 1) {
			fprintf(model.timeVisual.filVisuell, "]}\n");
			fclose(model.timeVisual.filVisuell);
			simuleraStormsVisuellt();
		}
		//if (evalExtraSol == 0) {
		//	FILE* filpekG;
		//	filpekG = fopen(resultName.c_str(), "a+"); // "result_json.json", "w");
		//	fprintf(filpekG, "]}\n");
		//	fclose(filpekG);
		//}

	}
	checkMinnesAnvandning(__LINE__);



	if (evalExtraSol == 1) {
		dist = genBV_franFixLsning(&model, &Cost);
		checkMinnesAnvandning(__LINE__);
		if (dist > 0) {
			for (int ii = 0; ii < 1 + nExtraOpt; ii++) {
				modify_utNodCost(ii);


				if (model.nBVArcs < 2) {
					endTidsp = 0;
					errlog("ERROR! Too few arcs %d in fix solution\n", model.nBVArcs);
				}
				else
					endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
				for (i = 0; i < nSolSaved; i++) {
					if (abs(dist - solSavedDist[i]) < 0.000001 && endTidsp == solSavedEndTP[i])
						break;
				}
				Cost = evalCostFixLsning();

				if (ii == 0) {
					sprintf(namn, "%s_eval", baseName);
				}
				else {
					sprintf(namn, "%s_eval", model.params.extraOptWeights[ii - 1].identifierOpt);
				}

				if (i < nSolSaved && ii < nExtraOpt) {
					errlog("Solution %s (dist %.3lf) same as earlier saved solution no %d so do not save this one, cost %I64d\n",
						namn, dist, i, Cost);
					continue;
				}
				errlog("Solution %s (dist %.3lf) will be saved as no %d, cost %I64d\n",
					namn, dist, i, Cost);
				solSavedDist[nSolSaved] = dist;
				solSavedEndTP[nSolSaved] = endTidsp;
				nSolSaved++;

				writeSolutionToJson(resultName, ii, namn, 0, nFinalRoutes);
				nFinalRoutes++;
			}
		}
	}

	//printf("MaxBearingDiff %lf between %.3lf %.3lf to %.3lf %.3lf\n", maxBearingDiff, bearingErrorXY[0],
	//	bearingErrorXY[1], bearingErrorXY[2], bearingErrorXY[3]);
	//printf("nApprox %d snittFel %.3lf snittAbsFel %.3lf\n", nBearingDiff, sumBearingDiff/nBearingDiff, sumAbsBearingDiff/ nBearingDiff);


	printf("all done\n");
	if (evalExtraSol == 1) {
		FILE* filpekG = fopen(resultName.c_str(), "a+"); // "result_json.json", "w");
		fprintf(filpekG, "]}\n");
		fclose(filpekG);
	}

	//printf("All done. I quit.\n");
	//auto tid1c3 = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double, std::milli> fp_ms3 = tid1c3 - tid0;
	//errlog("all done %lf\n", fp_ms3);

	//callJsonTest();
	return 0;
}

int loadInitIterData_kaoutar(FILE* filpek, int runAlt) {

	char* namn = (char*)malloc2(256 * sizeof(char));
	dataStr* data = (dataStr*)malloc(sizeof(dataStr));
	char objects[100][CHAR_ALLOC];
	int antal;
	std::string tempString;
	FILE* filpekUt;

	if (runAlt == 0)
		sprintf(namn, "%s/iterDataOpt_kaoutar.txt", model.params.resultPath.c_str());
	else
		sprintf(namn, "%s/iterData_kaoutar.txt", model.params.resultPath.c_str());
	model.results.fileForecast = fopen(namn, "w");


	antal = get_data_objects_till_EOL_semkol(objects, data, filpek); // nLevelsMoveForeward
	model.iterKaoutar.nLevelsMoveForeward = char_to_int(objects[1]);
	fprintf(model.results.fileForecast, "nLevelsMoveForeward;%d\n", model.iterKaoutar.nLevelsMoveForeward);
	antal = get_data_objects_till_EOL_semkol(objects, data, filpek); // startDateTime
	tempString = objects[1];
	fprintf(model.results.fileForecast, "startDateTime;%s\n", objects[1]);
	model.params.UTC_secondsStart = make_gmtime_fromDateTimeString(tempString, &(model.params));
	antal = get_data_objects_till_EOL_semkol(objects, data, filpek);
	fprintf(model.results.fileForecast, "startDateTimeNuFirst;%s\n", objects[1]);
	tempString = objects[1];
	model.iterKaoutar.UTC_secondsFirstStart = make_gmtime_fromDateTimeString(tempString, NULL);
	antal = get_data_objects_till_EOL_semkol(objects, data, filpek);
	model.iterKaoutar.physLevelStart = char_to_int(objects[1]);
	model.iterKaoutar.physLevelStartUse = model.iterKaoutar.physLevelStart;

	antal = get_data_objects_till_EOL_semkol(objects, data, filpek);
	model.iterKaoutar.nForecastRuns = char_to_int(objects[1]);
	free(namn);

	if (runAlt == 0) {
		fprintf(model.results.fileForecast, "physicalLevelNu;%d\n", model.iterKaoutar.physLevelStart);
		if (model.iterKaoutar.physLevelStart == 0) {
			sprintf(namn, "%s/iterTotalResults.txt", model.params.resultPath.c_str());
			filpekUt = fopen(namn, "w");
			fprintf(filpekUt, "physLevelStart;optimize;runAlt;forecastType;totDistance_kts;totTime_h;totFuel;totObjCost;totDollarCost;weatherFactors;safety\n");
			fclose(filpekUt);
			sprintf(namn, "%s/resSol_forecast.txt", model.params.resultPath.c_str());
			filpekUt = fopen(namn, "w");
			fclose(filpekUt);
		}
		model.iterKaoutar.fixedArcs = NULL;
	}
	else
		fprintf(model.results.fileForecast, "physicalLevelNu;%d\n",
			model.iterKaoutar.physLevelStart + model.iterKaoutar.nLevelsMoveForeward);
	fprintf(model.results.fileForecast, "nForecastRuns;%d\n", model.iterKaoutar.nForecastRuns);
	fclose(model.results.fileForecast);

	return 0;
}

int saveFixForecastSolutionToNextIter() {
	int iPosIter;
	char* namn = (char*)malloc2(256 * sizeof(char));

	if (runAltForecast > 0)
		sprintf(namn, "%s/iterDataOpt_kaoutar.txt", model.params.resultPath.c_str());
	else
		sprintf(namn, "%s/iterData_kaoutar.txt", model.params.resultPath.c_str());

	model.results.fileForecast = fopen(namn, "a+");
	fprintf(model.results.fileForecast, "forecastType;%d\n", model.results.forecastTypeOrig);

	fprintf(model.results.fileForecast, "nBVArcs;%d;nPhysLevels;%d;nArcs;%d\n", model.nBVArcs, model.network.nPhysicalLevels, model.nArcs);
	fprintf(model.results.fileForecast, "pos;arcNr;fromLevel;toLevel;fromPos;toPos;fromTime;toTime;speedSettingBase\n");
	for (iPosIter = 0; iPosIter < model.iterKaoutar.nFixedArcs; iPosIter++) {
		fprintf(model.results.fileForecast, "%d;%d;%d;%d;%d;%d;%lf;%lf;%d\n", iPosIter,
			model.iterKaoutar.fixedArcs[iPosIter].arcNr, model.iterKaoutar.fixedArcs[iPosIter].fromLevel,
			model.iterKaoutar.fixedArcs[iPosIter].toLevel, model.iterKaoutar.fixedArcs[iPosIter].fromPos,
			model.iterKaoutar.fixedArcs[iPosIter].toPos, model.iterKaoutar.fixedArcs[iPosIter].fromTime,
			model.iterKaoutar.fixedArcs[iPosIter].toTime, model.iterKaoutar.fixedArcs[iPosIter].speedSettingBase);
	}

	fprintf(model.results.fileForecast, "initTotDistStart;%lf;iterTotFuelStart;%lf;iterTotObjStart;%lf;"
		"iterTotDollarCostStart;%lf;iterStartTidp;%lf;iterStartTidpArc;%lf;iterWeatherFactorsStart;%lf;iterSafetyStart;%lf\n",
		model.results.iterTotDistStart, model.results.iterTotFuelStart, 
		model.results.iterTotObjStart, model.results.iterTotDollarCostStart,
		model.iterKaoutar.tidpStartIter_h, model.iterKaoutar.tidpStartIterArc_h,
		model.results.iterWeatherFactorsStart, model.results.iterSafetyStart);
	fprintf(model.results.fileForecast, "totFuel;%lf;totTime;%lf;fuelCostDollar;%lf;timeCostDollar;%lf;"
		"totalDistance_kts;%lf;safety;%lf;objCost;%lf;totWeatherFactors;%lf\n",
		model.results.fuel, model.results.time, model.results.fuelCost, model.results.timeCost,
		model.results.dist, model.results.safety, model.results.objCost, model.results.totWeatherFactors);
	fprintf(model.results.fileForecast, "weightTime;%lf;weightFuel;%lf;weightEmission;%lf;weightSafety;%lf\n",
		model.params.weightTime, model.params.weightFuel,
		model.params.weightEmission, model.params.weightSafety.base);
	fclose(model.results.fileForecast);

	sprintf(namn, "%s/iterTotalResults.txt", model.params.resultPath.c_str());
	FILE* filpekUt = fopen(namn, "a+");
	if(runAltForecast < 0)
		fprintf(filpekUt, "%d;%d;%d;%d;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n", model.iterKaoutar.physLevelStart, 1,
			runAltForecast, model.results.forecastTypeOrig, model.results.dist, model.results.time,
			model.results.fuel,
			model.results.objCost, model.results.fuelCost + model.results.timeCost,
			model.results.totWeatherFactors, model.results.safety);
	fprintf(filpekUt, "%d;%d;%d;%d;%lf;%lf;%lf;%lf;%lf;%lf;%lf\n", model.iterKaoutar.physLevelStart, 0,
		runAltForecast, model.results.forecastTypeOrig, model.results.dist, model.results.time,
		model.results.fuel,
		model.results.objCost, model.results.fuelCost + model.results.timeCost,
		model.results.totWeatherFactors, model.results.safety);


	return 0;
}

int voyageOpt_fixPartSol(std::string inputPath, std::string resultName)
{

	double dist, cost;
	long long Cost;
	std::string resNamnNu;
	reset_errlog();
	//callRaster();

	FILE* filPek3;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	model.timeStart = std::chrono::high_resolution_clock::now();
	init_tmBas();

	initLookUpTables();


	initModelStatusValues();

	model.params.resultPath = splitFilename(resultName);
	model.params.resultName = resultName;

	filPek3 = fopen(resultName.c_str(), "w"); // "result_json.json", "w");
	fprintf(filPek3, "{\n\t\"errorMessage\": \"unknown error\"\n}\n");
	fclose(filPek3);

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;
	model.params.hindCast = 0;
	model.params.failedTime = 0;
	model.params.etaFocus_speed = 0;
	model.params.UTC_secondsStart = 0;
	model.results.fileForecast = NULL;
	model.results.fileForecast2 = NULL;

	if (runAltForecast > 0)
		model.results.fileNameForecast = splitFilename(inputPath, 1);

	//errlog("#######\nERROR! Change the below code rows as it is for analysis only\n");
	model.params.nSpeedSettingDivideIter1 = 2; // 2 ger 3 speed settings, 4 ger 5 speed settings
	model.params.nTidsperioder_perH_iter1 = 1; // 1 is default, 4 ger var 15:e minut
	model.params.tidp_startHistoricDataOnly_iter1 = 0; // 999999; // 0 is default

	//model.params.resultPath = resultPath;

	//printf("Reading data for the problem\n");

	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityOptiNav(&(model.params));

	//loadTablesInfo();

	loadParams_new(&(model.params));

	model.params.max_changeDirection_base = model.params.max_changeDirection;
	model.params.max_changeDirection_factorStartEnd_base = model.params.max_changeDirection_factorStartEnd;


	if (model.params.calmWaterSpeedCompare > 0 || model.params.fuelCompare > 0) {
		errlog("ERROR! calmWaterSpeedCompare is %lf and fuelCompare is %lf. I set them to 0 as I don't do the comparison\n",
			model.params.fuelCompare);
		model.params.calmWaterSpeedCompare = 0;
		model.params.fuelCompare = 0;
	}

#ifdef NAZANIN_SAFETY
	loadStartDatum_nazanin();
	loadVesselTableIDs_nazanin();
#endif

	if (delayVersion == 5)
		errlog("ERROR! OBS delayVersion %d\n", 5);

	// testSaveMapToSQLite();
	// testSaveMapToBinaryFile();

	//printf("obj weight dynamicStability %.3lf\n", model.params.weightSafety.dynamicStability);

	loadAllNeededTablesFromSQLite();

	loadVariables();

	model.corridorPath.nLines = 0;

	printf("-- Time before creating physical network %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	createPhysicalNetwork(0, 0);


	// setupUsableSpeedSettings();

	model.optPath.level = (strOptPathLevel*)malloc2(model.network.nPhysicalLevels * sizeof(strOptPathLevel));
	model.optPath.channel = (strOptPathChannel*)malloc2(model.network.nChannels * sizeof(strOptPathChannel));

	model.Dijkstra.nodes = NULL;
	int evalExtraSol = 0;
	int nExtraOpt = 0; // genExtraOpts();
	int nod1, nod2, nSolSaved = 0, endTidsp, i;
	int* solSavedEndTP = NULL;
	int* sol_nSavedArcs = NULL;
	int** solSavedArcs = NULL;
	double* solSavedDist = NULL;
	char* baseName;
	baseName = (char*)malloc2(100 * sizeof(char));

	//filSaveSpec = fopen("data/forecastTest/checkArcs.txt", "w");

	FILE* fileFixSol;
	int korAlt;
	if (runAltForecast > 0) {
		sprintf(namn, "%s/iterData_kaoutar.txt", model.params.resultPath.c_str());
		korAlt = 0;
	}
	else {
		sprintf(namn, "%s/iterDataOpt_kaoutar.txt", model.params.resultPath.c_str());
		korAlt = 1;
	}
	fileFixSol = fopen(namn, "r");
	if (fileFixSol == NULL) {
		errlog("ERROR! The file %s does not exist. Nothing to evaluate. I quit!\n", namn);
		exit(0);
	}
	loadInitIterData_kaoutar(fileFixSol, korAlt);

	model.BVArc = NULL;
	int nArcsNu, nSpeedSettingsDivideIter1 = model.params.nSpeedSettingDivideIter1;
	int maxDiffTimeFastSlow = model.params.maxDiffTimeFastSlow;

	model.nAllocArcs = 0;


	for (int iter0 = 0; iter0 < model.iterKaoutar.nForecastRuns; iter0++) {

		checkMinnesAnvandning(__LINE__);

		for (int lev1 = 0; lev1 < model.network.nPhysicalLevels; lev1++) {
			model.optPath.level[lev1].timeArrive = -1;
			model.optPath.level[lev1].pointNr = -1;
			model.optPath.level[lev1].baseSpeedSettingNr = -1;
			model.optPath.level[lev1].levelNext = -1;
		}
		for (int lev1 = 0; lev1 < model.network.nChannels; lev1++) {
			model.optPath.channel[lev1].timeArriveNext = -1;
			model.optPath.channel[lev1].speedSettingNrNext = -1;
			model.optPath.channel[lev1].levelNext = -1;
			model.optPath.channel[lev1].timeArriveThrough = -1;
			model.optPath.channel[lev1].speedSettingNrThrough = -1;
		}

		model.results.iterTotDistStart = 0;
		model.results.iterTotFuelStart = 0;
		model.results.iterTotObjStart = 0;
		model.results.iterTotDollarCostStart = 0;
		model.results.iterWeatherFactorsStart = 0;
		model.results.iterSafetyStart = 0;

		model.params.nTidsperioder_perH = model.params.nTidsperioder_perH_iter1;
		model.params.tIndexGerH = 1.0 / model.params.nTidsperioder_perH;
		model.params.maxDiffTimeFastSlow = maxDiffTimeFastSlow;

		model.params.commercialAllowedVariation = -1.0;
		nArcsNu = loadIterData_kaoutar(fileFixSol, iter0);
		if (nArcsNu < 0) {
			return -1; // have stepped through the path, nothing more to do
		}

		if (model.results.forecastTypeOrig > 1000 && model.results.onlyPrefPath_kaoutar == 1 && 
			model.iterKaoutar.physLevelStart > 0) {
			// save to iterData but do no opt
			saveFixForecastSolutionToNextIter();
			continue;
		}


		model.params.nSpeedSettingDivideIter1 = nSpeedSettingsDivideIter1;
		if (model.results.forecastTypeOrig > 300)
			model.params.nSpeedSettingDivideIter1 = 4;

		setupUsableSpeedSettings();

		if (iter0 > 0)
			freeAllNodData();

		sprintf(baseName, "base");
		model.iterKaoutar.evalAlt = 1;
		checkMinnesAnvandning(__LINE__);
		for (int iter = 0; iter < 3; iter++) {
			//fprintf(filSaveSpec, "\n\niter0 %d iter %d\n", iter0, iter);

			if (iter > 0) {
				if (iter == 1 && model.params.calmWaterSpeedCompare <= 0 && model.params.fuelCompare <= 0)
					continue; // no reason to do this iteration as there are no speed or fuel to compare with
				freeAllNodData();
				//printf("nSpeedSettings %d\n", model.functions.speedLevel[17].nShip_speedSettings);
				modify_midTimeArrive(iter);
				//printf("nSpeedSettings %d\n", model.functions.speedLevel[17].nShip_speedSettings);
				model.params.maxDiffTimeFastSlow = model.params.maxDiffTimeFastSlow_fas3;
				model.params.nTidsperioder_perH = 4;
				// errlog("ERROR! Change nTidsperioder_perH to 4 above\n");
				model.params.tIndexGerH = 1.0 / model.params.nTidsperioder_perH;
				if (iter == 1)
					sprintf(baseName, "prefPathFixSpeed");
				else
					sprintf(baseName, "base");
			}
			printf("-- Time before creating the time dimension %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

			//char* namnTmp = (char*)malloc(256 * sizeof(char));
			//sprintf(namnTmp, "checkArcs%d_%d.txt", iter0, iter);
			//model.iterKaoutar.filpek = fopen(namnTmp, "w");
			createTimeArcs_kaoutar(iter, iter0);
			//fclose(model.iterKaoutar.filpek);
			printf("-- Time after creating the time dimension %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));

			//if(iter == 0)
			//	saveSP_delay(iter);

			// freeAllMemory();

			if (model.network.physicalLev[model.network.nPhysicalLevels - 1].nTimeIntervals[0] == 0) {
				errlog("ERROR! Number of time intervals to the last level is 0. Is the preferred path outside of the extent of the feasibility map? I quit.\n");
				printf("ERROR! Number of time intervals to the last level is 0. Is the preferred path outside of the extent of the feasibility map? I quit.\n");
				postRequest("ERROR! Number of time intervals to the last level (" + std::to_string(model.network.nPhysicalLevels - 1) + ") is 0. Is the preferred path outside of the extent of the feasibility map ? I quit.", 1);
			}

			printf("setting up data for dijkstra's algorithm\n");
			auto tid0 = std::chrono::high_resolution_clock::now();
			SattUppDijkstraNatverk3(&model);
			auto tid1c = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> fp_ms = tid1c - tid0;
			errlog("sattUppDijkstra took %lf\n", fp_ms);
			nod1 = 0;
			nod2 = model.nNoder - 1;
			bool Reached;
			checkMinnesAnvandning(__LINE__);

			if (model.params.simuleraTidVisuellt == 1) {
				sprintf(namn, "%s/solVisuellt.geojson", model.params.indataPath.c_str());
				model.timeVisual.filVisuell = fopen(namn, "w");
				initGeoJsonFil(model.timeVisual.filVisuell, "sol");
				model.timeVisual.pos = 0;
				model.timeVisual.startTime = (char*)malloc(256 * sizeof(char));
				model.timeVisual.tmBas = { 0 };
			}

			model.network.nMaxSplits = DEF_nMAX_SPLITS;

#ifdef _WIN32
			if (model.network.nMaxSplits == 10000) {
				errlog("ERROR! Only one split per arc\n");
				model.network.nMaxSplits = 1;
				evalExtraSol = 0;
				if (model.network.nMaxSplits != 1000 && iter == 0) {
					namn = (char*)malloc2(256 * sizeof(char));
					sprintf(namn, "%s/checkArcsInSolution.txt", model.params.indataPath.c_str());
					FILE* filpek = fopen(namn, "w");
					fprintf(filpek, "arcNr\tnSplit\tnodNr1\tnod1UtPos\tnodNr2\tfromLevel\tfromPointNr\tfromTimeInterval\ttoLevel\ttoPointNr\ttoTimeInterval\ttotCost\t"
						"channelCost\tdistance\temission\tfuelBase\tsafetyBase\tspeedSetting\ttime\ttimeCheck\ttimeElapsed\tdiffTimeToMid\t"
						"accumDist\tlatLon\tbearing\tfuel_day\tworstStormValue\t"
						"currentReal\trelCurrent\tcalmWaterSpeed\tbaseGroundSpeed\tspeedOnGround\trpm\trelWindSpeed\trelWindDir\t"
						"deltaSpeedWind\twaveheight\twavePeriod\trelWaveDir\tdeltaSpeedWave\twindSpeedReal\twindDirReal\t"
						"currentReal\tcurrentDirReal\twaveDirReal\tbowSlamming_max\tgreenWater_max\tdynamiStability_max\ticeCover_max\tforecastType\n");
					fclose(filpek);
				}
			}
#endif 

			if (model.BVArc == NULL) {
				model.nAllocBVArcs = model.nNoder + 10;
				model.BVArc = (int*)malloc(model.nAllocBVArcs * sizeof(int));
				model.BVtempNodOrder = (int*)malloc(model.nAllocBVArcs * sizeof(int));

				model.network.startKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
				model.network.endKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
				model.network.posSplitCoord = (int*)malloc2((model.network.nMaxSplits + 1) * sizeof(int));
				solSavedDist = (double*)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(double));
				solSavedEndTP = (int*)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int));
				sol_nSavedArcs = (int*)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int));
				solSavedArcs = (int**)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int*));

				model.waypointResult.dateUTC = (char*)malloc(256 * sizeof(char));
				model.waypointResult.full_Date = (char*)malloc(256 * sizeof(char));
				model.waypointResult.fixPositionString_latlon = (char*)malloc(256 * sizeof(char));
				model.waypointResult.windDirReal_letters = (char*)malloc(256 * sizeof(char));
				model.waypointResult.waveDir_letters = (char*)malloc(256 * sizeof(char));
			}
			else {
				if (model.nAllocBVArcs < model.nNoder) {
					model.nAllocBVArcs = model.nNoder + 10;
					model.BVArc = (int*)realloc(model.BVArc, model.nAllocBVArcs * sizeof(int));
					model.BVtempNodOrder = (int*)realloc(model.BVtempNodOrder, model.nAllocBVArcs * sizeof(int));
				}
			}

			std::string resAltName;

			if (iter0 == 1)
				iter0 = iter0;
			for (int ii = 0; ii < 1 + nExtraOpt; ii++) {
				if (ii > 0) {
					modify_utNodCost(ii);
					ChangeArcCosts3(&model);
				}
				printf("\nsolving dijkstra's algorithm..");
				AnropDijkstra2(nod1, nod2, &model, &Reached);
				auto tid1c2 = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double, std::milli> fp_ms2 = tid1c2 - tid0;
				errlog("after Dijkstra %lf\n", fp_ms2);
				printf("..done. Obj %I64d\n", model.Dijkstra.OptCost);
				if (Reached == true) {
					//printf("har1\n");
					//printf("har12\n");
					//model.filpek = fopen("checkOptArcCosts.txt", "w");
					dist = NystaUppBV_MassTest(&model, Reached, nod1, nod2, &Cost);
					//fclose(model.filpek);
					//model.filpek = NULL;
					if (model.nBVArcs < 2) {
						endTidsp = 0;
						errlog("ERROR! Too few arcs %d in Dijkstra solution\n", model.nBVArcs);
					}
					else
						endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
					for (i = 0; i < nSolSaved; i++) {
						if (abs(dist - solSavedDist[i]) < 0.000001 && endTidsp == solSavedEndTP[i])
							break;
					}
					if (i < nSolSaved && ii != 0) {
						errlog("Solution (dist %.3lf) same as earlier saved solution no %d so do not save this one, cost %I64d\n",
							dist, i, Cost);
						continue;
					}
					if (ii == 0)
						errlog("Solution %s (dist %.3lf) will be saved as no %d, cost %I64d\n", baseName,
							dist, i, Cost);
					else
						errlog("Solution %s (dist %.3lf) will be saved as no %d, cost %I64d\n",
							model.params.extraOptWeights[ii - 1].identifierOpt, dist, i, Cost);
					solSavedDist[nSolSaved] = dist;
					solSavedEndTP[nSolSaved] = endTidsp;
					//solSavedArcs[nSolSaved] = (int*)malloc2(model.nBVArcs * sizeof(int));
					//for(int ii = 0; ii < model.nBVArcs; ii++)
					//	solSavedArcs[nSolSaved][ii] = model.BVArc[ii];
					//sol_nSavedArcs[nSolSaved] = model.nBVArcs;
					nSolSaved++;

					//printf("dist %.4lf endTidsp %d nSolSaved %d\n", dist, endTidsp, nSolSaved);

					//printf("har13\n");
					//if (ii == 0)
					//	sprintf(namn, "%s/%s", resultPath.c_str(), model.params.solutionFileName.c_str());
					//else
					//	sprintf(namn, "%s/resObj_%d", resultPath.c_str(), ii);

					//printf("har14\n");
					//writeSolutionPathToGeoJson(namn, 0);

					//printf("Saving solution path1..");
					if (ii == 0) {
						printf("solution to %s\n", baseName);
						if (iter < 2 && SKRIV_UT_NOTHING == 0 && (model.params.useSimulering == 0 || iter != 1)) {
							writeSolutionToJson(model.params.resultPath + "/resStep" + std::to_string(iter) + ".json", ii, baseName, iter);
						}
						else {
							// evalCompareSolution();
							if (iter == 2)
								resNamnNu = resultName.substr(0, resultName.length() - 5) + "_" + std::to_string(model.iterKaoutar.physLevelStart) +
								"_" + std::to_string(runAltForecast) +
								"_" + std::to_string(model.results.forecastTypeOrig) +
								"_" + std::to_string(model.iterKaoutar.evalAlt) + ".json";
							else
								resNamnNu = resultName;
							writeSolutionToJson(resNamnNu, ii, baseName, iter);
						}
					}
					else {
						printf("solution to %s\n", model.params.extraOptWeights[ii - 1].identifierOpt);
						writeSolutionToJson(resultName, ii, model.params.extraOptWeights[ii - 1].identifierOpt, iter);
					}
					checkMinnesAnvandning(__LINE__);
					//for (int ii = 0; ii < nSolSaved; ii++) {
					//	cost = 0;
					//	for (int ii1 = 0; ii1 < sol_nSavedArcs[ii]; ii1++) {
					//		cost += model.arc[solSavedArcs[ii][ii1]].totCost;
					//		if (ii1 >= sol_nSavedArcs[ii] - 3)
					//			printf("\tarcNr %d cost %.3lf totCost %.3lf TP from/to %d %d levels %d %d endLev %d\n", solSavedArcs[ii][ii1],
					//				model.arc[solSavedArcs[ii][ii1]].totCost, cost, model.arc[solSavedArcs[ii][ii1]].fromTime, model.arc[solSavedArcs[ii][ii1]].toTime,
					//				model.arc[solSavedArcs[ii][ii1]].fromLevel, model.arc[solSavedArcs[ii][ii1]].toLevel, model.network.nPhysicalLevels);
					//	}
					//	printf("savedSol %d totCost %.2lf\n", ii, cost);
					//}

					//printf(".done\n");
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

#ifdef _WIN32
					if (iter == 2 && SKRIV_UT_NOTHING == 0) {
						if (ii == 0)
							errlog("saving solution path for base to solutionCheck_%d.txt, solution no %d\n", ii, nSolSaved - 1);
						else
							errlog("saving solution path for %s to solutionCheck_%d.txt, solution no %d\n",
								model.params.extraOptWeights[ii - 1].identifierOpt, ii, nSolSaved - 1);
						savePathToSolutionCheck(ii);
					}
#endif // _WIN32
				}
				else {
					errlog("ERROR! Did not manage to find a route from start to finish...\n");
					printf("\nERROR! Did not manage to find a route from start to finish...\n");
				}
			}

			if (model.params.simuleraTidVisuellt == 1) {
				fprintf(model.timeVisual.filVisuell, "]}\n");
				fclose(model.timeVisual.filVisuell);
				simuleraStormsVisuellt();
			}
			//if (evalExtraSol == 0) {
			//	FILE* filpekG;
			//	filpekG = fopen(resultName.c_str(), "a+"); // "result_json.json", "w");
			//	fprintf(filpekG, "]}\n");
			//	fclose(filpekG);
			//}

		}

		if (runAltForecast < 0) {
			// eval sol of fixed path
			model.params.nSpeedSettingDivideIter1 = model.functions.nShip_speedSettingsBase;
			setupUsableSpeedSettings();

			nArcsNu = createArcsFromFixSol2(iter0);

			if (model.nAllocBVArcs < nArcsNu) {
				model.nAllocBVArcs = nArcsNu + 10;
				model.BVArc = (int*)realloc(model.BVArc, model.nAllocBVArcs * sizeof(int));
				model.BVtempNodOrder = (int*)realloc(model.BVtempNodOrder, model.nAllocBVArcs * sizeof(int));
			}
			for (i = 0; i < nArcsNu; i++)
				model.BVArc[i] = i;
			model.BVArc[i] = -1;
			model.nBVArcs = nArcsNu + 1;

			std::string resAltName;

			model.iterKaoutar.evalAlt = 0;
			endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
			sprintf(baseName, "eval"); // , iter0); // "base");
			resNamnNu = resultName.substr(0, resultName.length() - 5) + "_" + std::to_string(model.iterKaoutar.physLevelStart) +
				"_" + std::to_string(runAltForecast) +
				"_" + std::to_string(model.results.forecastTypeOrig) +
				"_" + std::to_string(model.iterKaoutar.evalAlt) + ".json";
			writeSolutionToJson(resNamnNu, 0, baseName, 2);
			checkMinnesAnvandning(__LINE__);
		}
	}
	checkMinnesAnvandning(__LINE__);

	fclose(fileFixSol);
	//fclose(filSaveSpec);

	if (evalExtraSol == 1) {
		dist = genBV_franFixLsning(&model, &Cost);
		checkMinnesAnvandning(__LINE__);
		if (dist > 0) {
			for (int ii = 0; ii < 1 + nExtraOpt; ii++) {
				modify_utNodCost(ii);


				if (model.nBVArcs < 2) {
					endTidsp = 0;
					errlog("ERROR! Too few arcs %d in fix solution\n", model.nBVArcs);
				}
				else
					endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
				for (i = 0; i < nSolSaved; i++) {
					if (abs(dist - solSavedDist[i]) < 0.000001 && endTidsp == solSavedEndTP[i])
						break;
				}
				Cost = evalCostFixLsning();

				if (ii == 0) {
					sprintf(namn, "%s_eval", baseName);
				}
				else {
					sprintf(namn, "%s_eval", model.params.extraOptWeights[ii - 1].identifierOpt);
				}

				if (i < nSolSaved && ii < nExtraOpt) {
					errlog("Solution %s (dist %.3lf) same as earlier saved solution no %d so do not save this one, cost %I64d\n",
						namn, dist, i, Cost);
					continue;
				}
				errlog("Solution %s (dist %.3lf) will be saved as no %d, cost %I64d\n",
					namn, dist, i, Cost);
				solSavedDist[nSolSaved] = dist;
				solSavedEndTP[nSolSaved] = endTidsp;
				nSolSaved++;

				writeSolutionToJson(resultName, ii, namn, 0);
			}
		}
	}

	//printf("MaxBearingDiff %lf between %.3lf %.3lf to %.3lf %.3lf\n", maxBearingDiff, bearingErrorXY[0],
	//	bearingErrorXY[1], bearingErrorXY[2], bearingErrorXY[3]);
	//printf("nApprox %d snittFel %.3lf snittAbsFel %.3lf\n", nBearingDiff, sumBearingDiff/nBearingDiff, sumAbsBearingDiff/ nBearingDiff);


	char* namn2 = (char*)malloc(256 * sizeof(char));
	if (korAlt == 0) {
		sprintf(namn, "%s/iterDataOpt_kaoutar.txt", model.params.resultPath.c_str());
		sprintf(namn2, "%s/iterDataOpt_kaoutar_%d_%d.txt", model.params.resultPath.c_str(),
			model.iterKaoutar.physLevelStart, runAltForecast);
	}
	else {
		sprintf(namn, "%s/iterData_kaoutar.txt", model.params.resultPath.c_str());
		sprintf(namn2, "%s/iterData_kaoutar_%d_%d.txt", model.params.resultPath.c_str(),
			model.iterKaoutar.physLevelStart, runAltForecast);
	}
	write_copyAtoB(namn2, (char*)"-", namn, (char*)"w");


	printf("all done\n");
	checkMinnesAnvandning(__LINE__);
	if (evalExtraSol == 1) {
		FILE* filpekG = fopen(resultName.c_str(), "a+"); // "result_json.json", "w");
		fprintf(filpekG, "]}\n");
		fclose(filpekG);
	}

	sprintf(namn, "%s/OptSnurraStatus.txt", model.params.resultPath.c_str());
	FILE* filtmp = fopen(namn, "w");
	fprintf(filtmp, "2\n");
	fclose(filtmp);


	//printf("All done. I quit.\n");
	//auto tid1c3 = std::chrono::high_resolution_clock::now();
	//std::chrono::duration<double, std::milli> fp_ms3 = tid1c3 - tid0;
	//errlog("all done %lf\n", fp_ms3);

	//callJsonTest();
	return 0;
}

int voyageEval_fixSol(std::string inputPath, std::string resultName)
{

	double dist, cost;
	long long Cost;
	int i;
	reset_errlog();
	//callRaster();

	FILE* filPek3;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	model.timeStart = std::chrono::high_resolution_clock::now();
	init_tmBas();

	initLookUpTables();


	initModelStatusValues();

	model.params.resultPath = splitFilename(resultName);
	model.params.resultName = resultName;

	filPek3 = fopen(resultName.c_str(), "w"); // "result_json.json", "w");
	fprintf(filPek3, "{\n\t\"errorMessage\": \"unknown error\"\n}\n");
	fclose(filPek3);

	model.params.indataPathName = inputPath;
	model.params.indataPath = splitFilename(inputPath);
	model.params.errorCode = 0;
	model.params.hindCast = 0;
	model.params.failedTime = 0;
	model.params.etaFocus_speed = 0;
	model.params.UTC_secondsStart = 0;

	model.results.fileNameForecast = splitFilename(inputPath, 1);

	//errlog("#######\nERROR! Change the below code rows as it is for analysis only\n");
	model.params.nSpeedSettingDivideIter1 = 2; // 2 ger 3 speed settings, 4 ger 5 speed settings
	model.params.nTidsperioder_perH_iter1 = 1; // 1 is default, 4 ger var 15:e minut
	model.params.tidp_startHistoricDataOnly_iter1 = 0; // 999999; // 0 is default

	//model.params.resultPath = resultPath;

	//printf("Reading data for the problem\n");

	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityOptiNav(&(model.params));

	//loadTablesInfo();

	loadParams_new(&(model.params));

	model.params.max_changeDirection_base = model.params.max_changeDirection;
	model.params.max_changeDirection_factorStartEnd_base = model.params.max_changeDirection_factorStartEnd;


	if (delayVersion == 5)
		errlog("ERROR! OBS delayVersion %d\n", 5);

	// testSaveMapToSQLite();
	// testSaveMapToBinaryFile();

	//printf("obj weight dynamicStability %.3lf\n", model.params.weightSafety.dynamicStability);

	loadAllNeededTablesFromSQLite();

	loadVariables();

	model.corridorPath.nLines = 0;

	printf("-- Time before creating physical network %lf\n", std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - model.timeStart));
	createPhysicalNetwork(0, 0);
	for (i = 0; i < model.network.nPhysicalLevels; i++)
		model.network.physicalLev[i].midTimeArrive = 0;

	model.params.nSpeedSettingDivideIter1 = model.functions.nShip_speedSettingsBase;
	setupUsableSpeedSettings();

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
	int evalExtraSol = 0;
	int nExtraOpt = 0; // genExtraOpts();
	int nod1, nod2, nSolSaved = 0, endTidsp, nArcsNu;
	int* solSavedEndTP = NULL;
	int* sol_nSavedArcs = NULL;
	int** solSavedArcs = NULL;
	double* solSavedDist = NULL;
	char* baseName;
	baseName = (char*)malloc2(100 * sizeof(char));
	//sprintf(baseName, "eval"); // "base");

	FILE* fileFixSol;
	sprintf(namn, "%s/iterDataOpt_kaoutar.txt", model.params.resultPath.c_str());
	fileFixSol = fopen(namn, "r");
	if (fileFixSol == NULL) {
		errlog("ERROR! The file %s does not exist. Nothing to evaluate. I quit!\n", namn);
		exit(0);
	}
	loadInitIterData_kaoutar(fileFixSol, 1);

	model.BVArc = NULL;

	model.params.nTidsperioder_perH = 4;
	model.params.tIndexGerH = 1.0 / model.params.nTidsperioder_perH;
	model.network.nMaxSplits = DEF_nMAX_SPLITS;

	for (int iter = 0; iter < model.iterKaoutar.nForecastRuns; iter++) {

		model.results.iterTotDistStart = 0;
		model.results.iterTotFuelStart = 0;
		model.results.iterTotObjStart = 0;
		model.results.iterTotDollarCostStart = 0;
		model.results.iterWeatherFactorsStart = 0;
		model.results.iterSafetyStart = 0;

		nArcsNu = createArcsFromFixSol(fileFixSol, iter);
		if (nArcsNu < 0)
			break; // no more paths to evaluate

		if (model.BVArc == NULL) {
			model.BVArc = (int*)malloc2(model.nNoder * sizeof(int));

			model.BVtempNodOrder = (int*)malloc2(model.nNoder * sizeof(int));

			model.network.startKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
			model.network.endKvot = (double*)malloc2((model.network.nMaxSplits + 1) * sizeof(double));
			model.network.posSplitCoord = (int*)malloc2((model.network.nMaxSplits + 1) * sizeof(int));
			solSavedDist = (double*)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(double));
			solSavedEndTP = (int*)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int));
			sol_nSavedArcs = (int*)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int));
			solSavedArcs = (int**)malloc2(model.iterKaoutar.nForecastRuns * 3 * (nExtraOpt + 1) * (evalExtraSol + 1) * sizeof(int*));

			model.waypointResult.dateUTC = (char*)malloc(256 * sizeof(char));
			model.waypointResult.full_Date = (char*)malloc(256 * sizeof(char));
			model.waypointResult.fixPositionString_latlon = (char*)malloc(256 * sizeof(char));
			model.waypointResult.windDirReal_letters = (char*)malloc(256 * sizeof(char));
			model.waypointResult.waveDir_letters = (char*)malloc(256 * sizeof(char));
		}

		for (i = 0; i < nArcsNu; i++)
			model.BVArc[i] = i;
		model.BVArc[i] = -1;
		model.nBVArcs = nArcsNu + 1;

		std::string resAltName;

		endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
		sprintf(baseName, "eval_%d", iter); // "base");
		writeSolutionToJson(resultName, iter, baseName, 2);
		// writeSolutionToJson(std::string filename, int resAlt, char* namnSol, int iter)
	}
	checkMinnesAnvandning(__LINE__);
	fclose(fileFixSol);

	//dist = genBV_franFixLsning(&model, &Cost);
	//		modify_utNodCost(ii);
//					endTidsp = model.arc[model.BVArc[model.nBVArcs - 2]].fromTime * model.params.tIndexGerH;
	//			Cost = evalCostFixLsning();

	FILE* filpekG = fopen(resultName.c_str(), "a+"); // "result_json.json", "w");
	fprintf(filpekG, "]}\n");
	fclose(filpekG);

	return 0;
}

int generate_solutionPathTest() {
	int i;
	double yNext, yNu, yUse, xNext, xNu, xUse;

	model.solutionPath.point = (spherical::Point*)malloc2(model.preferredPath.nPoints * sizeof(spherical::Point));

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

int check_isChannelNodePosAllowed(int nr, int pos) {
	int i;
	double distNu, dist1, dist1b, dist3, minDist, kvot, distTmp;
	double distLimit, dist2;
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


int fix_minMaxFromFrac(double indexFrac, int minVal, int maxVal) {
	int index = (int)indexFrac;
	if (index < indexFrac - 0.99999)
		index++;

	if (index < minVal)
		index = minVal;
	if (index > maxVal)
		index = maxVal;
	return index;
}


unsigned short* openBinaryMap(int ii, Raster::strPhysRaster* physRaster, strBoundBox boundingBox) {

	unsigned short* arrShortInt;
	char* namn2, * namn;
	namn = (char*)malloc2(256 * sizeof(char));
	Raster rasterPhysicalMapA, rasterFuelMapA;
	json mDataMap;
	int nBlockRows, nBlockCols, nAlloc;

	auto tid1 = std::chrono::high_resolution_clock::now();

	if (ii == 1) {
		sprintf(namn, "%s/mapPhysicalA.bin", model.params.indataPath.c_str());
	}
	else {
		sprintf(namn, "%s/mapFuelA.bin", model.params.indataPath.c_str());
	}


	long long nXBlocks = (model.sqliteMap[ii].nCols + model.sqliteMap[ii].nBlockCols - 1) / model.sqliteMap[ii].nBlockCols;
	long long nYBlocks = (model.sqliteMap[ii].nRows + model.sqliteMap[ii].nBlockRows - 1) / model.sqliteMap[ii].nBlockRows;
	//long long nXBlocks = (poBand->GetXSize() + pnXSize - 1) / pnXSize;
	//long long nYBlocks = (poBand->GetYSize() + pnYSize - 1) / pnYSize;

	//n_xBlocks = (int)((double)NCOLS / pnXSize);
	//if (n_xBlocks * pnXSize < NCOLS)
	//	n_xBlocks++;

	if (model.sqliteMap[ii].nBlockCols == model.sqliteMap[ii].nCols) {
		boundingBox.xMin = model.sqliteMap[ii].minX; // min_lon;
		boundingBox.xMax = model.sqliteMap[ii].maxX; // max_lon;
	}

	//xPosFrac1 = (boundingBox.xMin - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
	//xPosFrac2 = (boundingBox.xMax - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
	//yPosFrac1 = (max_lat - boundingBox.yMax) * NROWS / pnYSize / (max_lat - min_lat);
	//yPosFrac2 = (max_lat - boundingBox.yMin) * NROWS / pnYSize / (max_lat - min_lat);
	double xPosFrac1, xPosFrac2, yPosFrac1, yPosFrac2;
	int xMin, yMin, xMax, yMax;
	xPosFrac1 = (boundingBox.xMin - model.sqliteMap[ii].minX) * model.sqliteMap[ii].nCols / model.sqliteMap[ii].nBlockCols / (model.sqliteMap[ii].maxX - model.sqliteMap[ii].minX);
	xPosFrac2 = (boundingBox.xMax - model.sqliteMap[ii].minX) * model.sqliteMap[ii].nCols / model.sqliteMap[ii].nBlockCols / (model.sqliteMap[ii].maxX - model.sqliteMap[ii].minX);
	yPosFrac1 = (model.sqliteMap[ii].maxY - boundingBox.yMax) * model.sqliteMap[ii].nRows / model.sqliteMap[ii].nBlockRows / (model.sqliteMap[ii].maxY - model.sqliteMap[ii].minY);
	yPosFrac2 = (model.sqliteMap[ii].maxY - boundingBox.yMin) * model.sqliteMap[ii].nRows / model.sqliteMap[ii].nBlockRows / (model.sqliteMap[ii].maxY - model.sqliteMap[ii].minY);

	if (model.sqliteMap[ii].nBlockCols == model.sqliteMap[ii].nCols) {
		xMin = fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
		xMax = fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
	}
	else {
		xMin = (int)xPosFrac1; // fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
		xMax = (int)xPosFrac2; // fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
	}
	yMin = fix_minMaxFromFrac(yPosFrac1, 0, nYBlocks - 1);
	yMax = fix_minMaxFromFrac(yPosFrac2, 0, nYBlocks - 1);

	//printf("blocks to open for physical map x %d %d y %d %d\n", xMin, xMax, yMin, yMax);
	physRaster->minLongitude = model.sqliteMap[ii].minX + (double)xMin * model.sqliteMap[ii].nBlockCols * model.sqliteMap[ii].size_col;
	physRaster->minLatitude = model.sqliteMap[ii].maxY - (double)(yMax + 1) * model.sqliteMap[ii].nBlockRows * model.sqliteMap[ii].size_row;
	physRaster->maxLongitude = model.sqliteMap[ii].minX + (double)(xMax + 1) * model.sqliteMap[ii].nBlockCols * model.sqliteMap[ii].size_col;
	physRaster->maxLatitude = model.sqliteMap[ii].maxY - (double)yMin * model.sqliteMap[ii].nBlockRows * model.sqliteMap[ii].size_row;

	GByte* pabyData = (GByte*)CPLMalloc(model.sqliteMap[ii].nBlockCols * model.sqliteMap[ii].nBlockRows);

	physRaster->nCols = (xMax - xMin + 1) * model.sqliteMap[ii].nBlockCols;
	physRaster->nRows = (yMax - yMin + 1) * model.sqliteMap[ii].nBlockRows;
	physRaster->size_col = model.sqliteMap[ii].size_col;
	physRaster->size_row = model.sqliteMap[ii].size_row;
	physRaster->nBlock_x = nXBlocks;
	physRaster->nBlock_y = nYBlocks;

	unsigned short* valueCell = (unsigned short*)calloc2((long long)physRaster->nCols * (long long)physRaster->nRows, sizeof(unsigned short));

	int pos, iYBlock, nNotValid, yPosNu, nYValid, iXBlock, xUse, xPosNu, nXValid, iY, iX, posTmp, posTmp2;
	pos = 0;


	nAlloc = model.sqliteMap[ii].nBlockCols * model.sqliteMap[ii].nBlockRows;
	arrShortInt = (unsigned short*)malloc2(nAlloc * sizeof(unsigned short));
	std::ifstream rf(namn, std::ios::in | std::ios::binary);
	if (!rf) {
		std::cout << "Cannot open file!" << std::endl;
		return NULL;
	}
	int nCols = physRaster->nCols;
	nBlockCols = model.sqliteMap[ii].nBlockCols;

	int nLoops1 = 0, nLoops2 = 0;
	for (iYBlock = yMin; iYBlock <= yMax; iYBlock++) {
		nNotValid = 0;
		yPosNu = (iYBlock - yMin) * model.sqliteMap[ii].nBlockRows;
		if (iYBlock == physRaster->nBlock_y - 1)
			nYValid = model.sqliteMap[ii].nRows - iYBlock * model.sqliteMap[ii].nBlockRows;
		else
			nYValid = model.sqliteMap[ii].nBlockRows;

		for (iXBlock = xMin; iXBlock <= xMax; iXBlock++) {
			if (iXBlock < 0)
				xUse = iXBlock + physRaster->nBlock_x;
			else {
				if (iXBlock >= physRaster->nBlock_x)
					xUse = iXBlock - physRaster->nBlock_x;
				else
					xUse = iXBlock;
			}
			rf.seekg(nAlloc * sizeof(unsigned short) * (iYBlock * physRaster->nBlock_x + xUse));
			rf.read((char*)(arrShortInt), nAlloc * sizeof(unsigned short));
			nLoops1++;

			xPosNu = (iXBlock - xMin) * nBlockCols - nNotValid;
			if (iXBlock == physRaster->nBlock_x - 1) {
				nXValid = model.sqliteMap[ii].nCols - iXBlock * nBlockCols;
				nNotValid += nBlockCols - nXValid;
			}
			else
				nXValid = nBlockCols;


			for (iY = 0; iY < nYValid; iY++) {
				posTmp = xPosNu + nCols * (iY + yPosNu);
				posTmp2 = iY * nBlockCols;
				memcpy(&(valueCell[posTmp]), &(arrShortInt[posTmp2]), sizeof(unsigned short) * nXValid);
				/*
				for (iX = 0; iX < nXValid; iX++) {
					nLoops2++;
					valueCell[iX + xPosNu + nCols * (iY + yPosNu)] =
						arrShortInt[iX + iY * nBlockCols];
				}
			*/
			}
		}
	}
	rf.close();
	if (!rf.good()) {
		std::cout << "Error occurred at reading time!" << std::endl;
		return NULL;
	}
	printf("nLoops1 %d nLoops2 %d\n", nLoops1, nLoops2);
	auto tid2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms2 = tid2 - tid1;
	printf("read whole binary %s took %.3lf\n", namn, fp_ms2);

	free(namn);

	return valueCell;
}

int adderaNodDelay(int physicalLevel, int pointNr, int timeInterval)
{
	int nAlloc;

	if (modelDelay.Noder[modelDelay.nNoder].UtNod == NULL) {
		if (modelDelay.nNoder == 86)
			modelDelay.nNoder = modelDelay.nNoder;
		if (physicalLevel >= 0)
			nAlloc = model.network.physicalLev[physicalLevel].nInNodes[pointNr] + 1; // .nOutNodes[pointNr] + 1;
		else
			nAlloc = model.params.nPkterOrto + 1;
		modelDelay.Noder[modelDelay.nNoder].UtNod = (int*)malloc2(nAlloc * sizeof(int));
		modelDelay.Noder[modelDelay.nNoder].UtNodCost = (double*)malloc2(nAlloc * sizeof(double));
		modelDelay.Noder[modelDelay.nNoder].outArcNr = (int*)malloc2(nAlloc * sizeof(int));
	}
	modelDelay.Noder[modelDelay.nNoder].nUtNoder = 0;
	modelDelay.Noder[modelDelay.nNoder].physicalLevel = physicalLevel;
	modelDelay.Noder[modelDelay.nNoder].pointNr = pointNr;
	modelDelay.Noder[modelDelay.nNoder].timeInterval = timeInterval;
	if (physicalLevel >= 0)
		model.network.physicalLev[physicalLevel].nodDelay[pointNr] = modelDelay.nNoder;
	else
		model.network.channel[-physicalLevel - 1].nodDelay[pointNr] = modelDelay.nNoder;

	(modelDelay.nNoder)++;
	return modelDelay.nNoder - 1;
}

int adderaNodDelay_prefPath(int physicalLevel, int pointNr, int timeInterval)
{
	int nAlloc;
	if (modelDelay_prefPath.nNoder == 66)
		modelDelay_prefPath.nNoder = modelDelay_prefPath.nNoder;
	if (modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].UtNod == NULL) {
		if (physicalLevel >= 0)
			nAlloc = model.network.physicalLev[physicalLevel].nInNodes[pointNr] + 1; // .nOutNodes[pointNr] + 1;
		else
			nAlloc = model.params.nPkterOrto + 1;
		modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].UtNod = (int*)malloc2(nAlloc * sizeof(int));
		modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].UtNodCost = (double*)malloc2(nAlloc * sizeof(double));
		modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].outArcNr = (int*)malloc2(nAlloc * sizeof(int));
	}
	modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].nUtNoder = 0;
	modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].physicalLevel = physicalLevel;
	modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].pointNr = pointNr;
	modelDelay_prefPath.Noder[modelDelay_prefPath.nNoder].timeInterval = timeInterval;
	if (physicalLevel >= 0)
		model.network.physicalLev[physicalLevel].nodDelay_prefPath[pointNr] = modelDelay_prefPath.nNoder;
	else
		model.network.channel[-physicalLevel - 1].nodDelay_prefPath[pointNr] = modelDelay_prefPath.nNoder;

	(modelDelay_prefPath.nNoder)++;
	return modelDelay_prefPath.nNoder - 1;
}

int adderaArcDelay(int nodNr1, int nodNr2, double cost, int speedSetting)
{
	int i;
	if (nodNr2 == 86)
		nodNr2 = nodNr2;
	if (nodNr1 > 4000)
		nodNr1 = nodNr1;
	/*
	for (i = modelDelay.Noder[nodNr2].nUtNoder - 1; i >= 0; i--) {
		if (modelDelay.Noder[nodNr2].UtNod[i] == nodNr1) {
			if (cost < modelDelay.Noder[nodNr2].UtNodCost[i]) {
				// this speed setting is cheaper than the old one...
				modelDelay.Noder[nodNr2].UtNodCost[i] = cost;
				if (cost < 0)
					printf("ERROR cost negative %.2lf\n", cost);
				return modelDelay.Noder[nodNr2].outArcNr[i];
			}
			else {
				return -2;
			}
		}
		//if (modelDelay.Noder[modelDelay.Noder[nodNr2].UtNod[i]].pointNr != modelDelay.Noder[nodNr1].pointNr)
		//	break;
	}
	*/

	modelDelay.Noder[nodNr2].UtNod[modelDelay.Noder[nodNr2].nUtNoder] = nodNr1;
	modelDelay.Noder[nodNr2].UtNodCost[modelDelay.Noder[nodNr2].nUtNoder] = cost;
	if (cost < 0)
		printf("ERROR negative cost %.2lf\n", cost);
	if (modelDelay.nArcs == 41034)
		modelDelay.nArcs = modelDelay.nArcs;
	modelDelay.Noder[nodNr2].outArcNr[modelDelay.Noder[nodNr2].nUtNoder] = modelDelay.nArcs;
	(modelDelay.Noder[nodNr2].nUtNoder)++;
	return -1;
}

int addBastSpeed_arcDelayed(int thisLevel, int pos1, int nextLevel, int pos2, int tidInt, int nSpeedSettings, 
	double fuelQualityKvot, double extraAreaCostKvot, int addArc) {
	// loop over all speedsettings, find lowest cost with delay factor
	int i4, minPos, posTmp;
	double minCost = 1e20, totCost, channelCost, calmWaterSpeed, timeArc;
	double fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux;
	double fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, emission, factorDelay, dist;
	double x1, y1, x2, y2, speedNow;
	int arcNr, nodNr1, nodNr2, posDiff;
	double minTime, minBase, minAux, minAuxEca, minEca, minNoEca, speedDiffCurrent = 0;
	double fixTime = -1, waitingTime = 0, fuelWaitCorridor = 0;
	double fuelWaitCorridor_main = 0, fuelWaitCorridor_aux = 0, totCostCompare, kvotCost = 1.0;

	if (modelDelay.nArcs == 23964)
		modelDelay.nArcs = modelDelay.nArcs;
	// factorDelay = eval_factorDelayedAlongPath(thisLevel, tidInt);
	if (nextLevel >= 0) {
		if (thisLevel >= 0) {
			if (model.params.preferredPathOrtoPos[thisLevel] == pos1 && model.params.preferredPathOrtoPos[nextLevel] == pos2)
				dist = model.network.physicalLev[thisLevel + 1].distanceFromStartPosMid - model.network.physicalLev[thisLevel].distanceFromStartPosMid;
			else {
				x1 = model.network.physicalLev[thisLevel].point_x[pos1];
				y1 = model.network.physicalLev[thisLevel].point_y[pos1];
				x2 = model.network.physicalLev[nextLevel].point_x[pos2];
				y2 = model.network.physicalLev[nextLevel].point_y[pos2];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				dist = model.network.physicalLev[thisLevel].point[pos1].distanceTo(model.network.physicalLev[nextLevel].point[pos2]) / 1000;

			}
		}
		else {
			if ((model.network.channel[-thisLevel - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0) &&
				pos2 == model.params.preferredPathOrtoPos[nextLevel] && model.network.channel[-thisLevel - 1].preferredPathPoint_posConnectFrom >= 0 &&
				model.network.channel[-thisLevel - 1].bastEndLevel == nextLevel)
				dist = evalDistanceBetweenPrefPathAndChannel(thisLevel, nextLevel);
			else {
				if (pos1 == 1)
					posTmp = model.network.channel[-thisLevel - 1].nPoints - 1;
				else
					posTmp = 0;
				x1 = model.network.channel[-thisLevel - 1].point_x[posTmp];
				y1 = model.network.channel[-thisLevel - 1].point_y[posTmp];
				x2 = model.network.physicalLev[nextLevel].point_x[pos2];
				y2 = model.network.physicalLev[nextLevel].point_y[pos2];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				//dist = model.network.physicalLev[-thisLevel - 1].point[posTmp].distanceTo(model.network.physicalLev[nextLevel].point[pos2]) / 1000;
				dist = model.network.channel[-thisLevel - 1].point[posTmp].distanceTo(model.network.physicalLev[nextLevel].point[pos2]) / 1000;
			}
		}
	}
	else {
		if (thisLevel >= 0) {
			if ((model.network.channel[-nextLevel - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0) &&
				pos1 == model.params.preferredPathOrtoPos[thisLevel] && model.network.channel[-nextLevel - 1].preferredPathPoint_posConnectTo >= 0)
				dist = evalDistanceBetweenPrefPathAndChannel(thisLevel, nextLevel);
			else {
				x1 = model.network.physicalLev[thisLevel].point_x[pos1];
				y1 = model.network.physicalLev[thisLevel].point_y[pos1];
				if (pos2 == 1)
					posTmp = model.network.channel[-nextLevel - 1].nPoints - 1;
				else
					posTmp = 0;
				x2 = model.network.channel[-nextLevel - 1].point_x[posTmp];
				y2 = model.network.channel[-nextLevel - 1].point_y[posTmp];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				//dist = model.network.physicalLev[thisLevel].point[pos1].distanceTo(model.network.physicalLev[-nextLevel - 1].point[posTmp]) / 1000;
				dist = model.network.physicalLev[thisLevel].point[pos1].distanceTo(model.network.channel[-nextLevel - 1].point[posTmp]) / 1000;
			}
		}
		else {
			if (thisLevel == nextLevel)
				dist = model.network.channel[-thisLevel - 1].distance_km;
			else {
				posTmp = model.network.channel[-thisLevel - 1].nPoints - 1;
				x1 = model.network.channel[-thisLevel - 1].point_x[posTmp];
				y1 = model.network.channel[-thisLevel - 1].point_y[posTmp];
				x2 = model.network.channel[-nextLevel - 1].point_x[0];
				y2 = model.network.channel[-nextLevel - 1].point_y[0];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				//dist = model.network.physicalLev[-thisLevel - 1].point[posTmp].distanceTo(model.network.physicalLev[-nextLevel - 1].point[0]) / 1000;
				dist = model.network.channel[-thisLevel - 1].point[posTmp].distanceTo(model.network.channel[-nextLevel - 1].point[0]) / 1000;
			}
			fixTime = model.network.channel[-thisLevel - 1].timeThroughChannel;
			waitingTime = model.network.channel[-thisLevel - 1].waitingTime;
			kvotCost = model.network.channel[-thisLevel - 1].kvotCost;
			fuelWaitCorridor_main = model.network.channel[-thisLevel - 1].waiting_consumption_main;
			fuelWaitCorridor_aux = model.network.channel[-thisLevel - 1].waiting_consumption_aux;
		}
	}

	channelCost = 0;
	if (modelDelay.nArcs == 6085)
		modelDelay.nArcs = modelDelay.nArcs;

	if (thisLevel == 23 && pos1 == 44 && pos2 == 43)
		pos1 = pos1;
	if (thisLevel == 24 && pos1 == 43 && pos2 == 42)
		pos1 = pos1;
	if (thisLevel == 26 && pos1 == 43 && pos2 == 44)
		pos1 = pos1;
	if (thisLevel < 0)
		pos1 = pos1;
	for (i4 = 0; i4 < model.functions.nShip_speedSettingsDelay; i4++) {
		// if (model.params.commercialAllowedVariation >= 0 && i4 >= model.functions.speedLevel[0].nShip_speedSettings)
		//	break;


		if (modelDelay.nArcs == 14)
			modelDelay.nArcs = modelDelay.nArcs;
		totCost = channelCost;
		if (fixTime < -0.5) {
			// if (model.params.commercialAllowedVariation < 0)
			calmWaterSpeed = eval_calmWaterSpeed(i4, -1, -1000);
			//else
			//	calmWaterSpeed = eval_calmWaterSpeed(i4, -1, -1);

			if (delayVersion < 4)
				factorDelay = eval_factorDelayedAlongArc(thisLevel, pos1, nextLevel, pos2, tidInt); // model.network.physicalLev[thisLevel].timeInterval[pos1][tidInt]);
			else {
				if (delayVersion == 4)
					factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(thisLevel, pos1, nextLevel, pos2, tidInt, &speedDiffCurrent, calmWaterSpeed);
				else
					factorDelay = 1.0;
			}

			speedNow = calmWaterSpeed / factorDelay + speedDiffCurrent;
			if (speedNow < 0.1)
				speedNow = 0.1;
			timeArc = dist / speedNow; // in hours

			//if (model.params.commercialAllowedVariation < 0)
			fuelConsumption_main = eval_fuelConsumption_both(i4, &fuelConsumption_aux, -1, -1000);
			//else
			//	fuelConsumption_main = eval_fuelConsumption_both(i4, &fuelConsumption_aux, -1, -1);
		}
		else {
			if (i4 > 0)
				break; // only one fix time
			//if (delayVersion < 4)
			//	factorDelay = eval_factorDelayedAlongArc(thisLevel, pos1, nextLevel, pos2, tidInt); // model.network.physicalLev[thisLevel].timeInterval[pos1][tidInt]);
			//else {
			//	if (delayVersion == 4)
			//		factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(thisLevel, pos1, nextLevel, pos2, tidInt, &speedDiffCurrent);
			//	else
			//		factorDelay = 1.0;
			//}

			timeArc = fixTime; // in hours
			calmWaterSpeed = dist / fixTime;

			fuelConsumption_main = eval_fuelConsumption_both(i4, &fuelConsumption_aux, thisLevel, -1);

		}

		fuelUsage_main = fuelConsumption_main * timeArc;
		fuelUsage_aux = fuelConsumption_aux * timeArc;
		timeArc += waitingTime;

		fuelUsage_main += fuelWaitCorridor_main;
		fuelUsage_aux += fuelWaitCorridor_aux;

		fuel_eca = fuelUsage_main * (1 - fuelQualityKvot);
		fuel_noEca = fuelUsage_main * fuelQualityKvot;
		fuel_aux = fuelUsage_aux * fuelQualityKvot;
		fuel_auxEca = fuelUsage_aux * (1 - fuelQualityKvot);
		fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
			fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price) + fuelWaitCorridor;
		emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
			fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

		totCost += model.params.weightTime * model.params.priceTime * timeArc +
			model.params.weightFuel * fuelBase + emission * model.params.weightEmission * model.params.scaleObjEmission;

		if (model.params.useSimulering == 1) {
			if (thisLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[thisLevel] - pos1);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (nextLevel >= 0) {
				posDiff = abs(model.params.preferredPathOrtoPos[nextLevel] - pos2);
				totCost += posDiff * model.simulering.penDeviatePrefPath_nodes;
			}
			if (model.params.simulationSpeed_kmh > 0)
				totCost += abs(calmWaterSpeed - model.params.simulationSpeed_kmh) * model.simulering.penDeviateSpeed_kmh;
		}



		totCost *= (1 + extraAreaCostKvot);
		if (USE_KVOTKOST == 1)
			totCost *= kvotCost;

		if (thisLevel == 0)
			totCost /= 2; // this to make sure there is a bigger difference between the cheapest and most expensive arc or there might be problems when solving Dijkstra

		if (model.params.etaFocus_speed == 0)
			totCostCompare = totCost;
		else {
			if (model.params.etaFocus_speed == -1)
				totCostCompare = totCost * (i4 + 1) / 2;
			else
				totCostCompare = totCost * (nSpeedSettings - i4) / 2;
		}
		if (minCost > totCostCompare) {
			minCost = totCost;
			minPos = i4;
			minTime = timeArc;
			minBase = fuelBase;
			minAux = fuel_aux;
			minAuxEca = fuel_auxEca;
			minEca = fuel_eca;
			minNoEca = fuel_noEca;
		}
	}

	if (addArc == 1) {
		if (thisLevel >= 0)
			nodNr1 = model.network.physicalLev[thisLevel].nodDelay[pos1];
		else
			nodNr1 = model.network.channel[-thisLevel - 1].nodDelay[pos1];
		if (nextLevel >= 0)
			nodNr2 = model.network.physicalLev[nextLevel].nodDelay[pos2];
		else
			nodNr2 = model.network.channel[-nextLevel - 1].nodDelay[pos2];

		adderaArcDelay(nodNr1, nodNr2, minCost, minPos);

		arcNr = modelDelay.nArcs;
		if (arcNr == 4810)
			arcNr = arcNr;
		modelDelay.arc[arcNr].fromLevel = thisLevel;
		modelDelay.arc[arcNr].toLevel = nextLevel;
		modelDelay.arc[arcNr].fromPointNr = pos1;
		modelDelay.arc[arcNr].toPointNr = pos2;

		if (model.params.commercialAllowedVariation < 0)
			modelDelay.arc[arcNr].speedSetting = minPos;
		else
			modelDelay.arc[arcNr].speedSetting = model.functions.speedLevel[0].settingGerBaseSetting[minPos];

		modelDelay.arc[arcNr].time = minTime;
		modelDelay.arc[arcNr].distance = dist;
		modelDelay.arc[arcNr].fuelBase = minBase;
		modelDelay.arc[arcNr].fuel_aux = minAux;
		modelDelay.arc[arcNr].fuel_auxEca = minAuxEca;
		modelDelay.arc[arcNr].fuel_eca = minEca;
		modelDelay.arc[arcNr].fuel_noEca = minNoEca;
		modelDelay.arc[arcNr].fuelQualityKvot = fuelQualityKvot;
		modelDelay.arc[arcNr].extraAreaCostKvot = extraAreaCostKvot;
		modelDelay.arc[arcNr].kvotCost = kvotCost;
		modelDelay.arc[arcNr].totCost = minCost;
		modelDelay.nArcs++;
	}

	return minPos;
}

int addBage_AB_delayFysiskt(int thisLevel, int pos1, int nextLevel, int pos2, int tidInt, double fuelQualityKvot, double extraAreaCostKvot) {
	int nSpeedSettings, prefPath, i4, i3, arcNr;
	int nArcsNu = 0;
	double calmWaterSpeed = -1.0, distArc = -1, delayFactor = 0;
	double fuelFactorMain, nAddedTotArcs = 0, speedDiffCurrent = 0;

	if (thisLevel >= 0) {
		nSpeedSettings = model.functions.speedLevel[thisLevel].nShip_speedSettings;
		if (pos1 == model.params.preferredPathOrtoPos[thisLevel]) {
			if (nextLevel >= 0) {
				if (pos2 == model.params.preferredPathOrtoPos[nextLevel] && thisLevel == nextLevel - 1 &&
					(model.params.preferredPathStraightLineFeasibleFrom[thisLevel] == 0 || model.params.max_changeDirection == 0))
					prefPath = 1;
			}
			else {
				if ((model.network.channel[-nextLevel - 1].straightArcFeasible_toChannelFromPrefPath == 0 || model.params.max_changeDirection == 0) &&
					pos1 == model.params.preferredPathOrtoPos[thisLevel] && model.network.channel[-nextLevel - 1].preferredPathPoint_posConnectTo >= 0)
					prefPath = 1;
			}
		}
	}
	else {
		if (nextLevel >= 0) {
			nSpeedSettings = model.functions.speedChannelOut[-thisLevel - 1].nShip_speedSettings;
			if ((model.network.channel[-thisLevel - 1].straightArcFeasible_fromChannelToPrefPath == 0 || model.params.max_changeDirection == 0) &&
				pos2 == model.params.preferredPathOrtoPos[nextLevel] && model.network.channel[-thisLevel - 1].preferredPathPoint_posConnectFrom >= 0 &&
				model.network.channel[-thisLevel - 1].bastEndLevel == nextLevel)
				prefPath = 1;
		}
		else {
			nSpeedSettings = model.functions.speedChannel[-thisLevel - 1].nShip_speedSettings;
			if (model.network.channel[-thisLevel - 1].timeThroughChannel > -0.5)
				nSpeedSettings = 1; // only one speed option if fix speed through channel
		}
	}


	arcNr = addBastSpeed_arcDelayed(thisLevel, pos1, nextLevel, pos2, tidInt, nSpeedSettings, fuelQualityKvot, extraAreaCostKvot);
	// loop over all speedsettings, find lowest cost with delay factor

	//if (nextLevel < 0) { // add arcs for the channel path
	//	model.functions.valuesNow.prefPathArc = 2;
	//	if (model.network.channel[-nextLevel - 1].timeThroughChannel > -0.5)
	//		nSpeedSettings = 1; // only one speed option if fix speed through channel
	//	fuelQualityKvot = get_fuelQualityKvot(nextLevel, 0, nextLevel, 1);

	//	arcNr = addBastSpeed_arcDelayed(nextLevel, 0, nextLevel, 1, tidInt, nSpeedSettings, fuelQualityKvot);
	//}
	return nArcsNu;
}

int checkCosts_SP_delay_PP(int pos) {
	FILE* filpek;
	int nod2, nod1, i, arcNr, i0;
	double dist1, dist, time, fuel;
	long long costL;
	strModel* modelTmp;

	char* namn = (char*)malloc(256 * sizeof(char));
	sprintf(namn, "%s/tmp_checkSP_delay.txt", model.params.resultPath.c_str());
	if (pos == 0) {
		filpek = fopen(namn, "w");
		fprintf(filpek, "SP open opt\n");
	}
	else {
		filpek = fopen(namn, "a+");
		fprintf(filpek, "\nSP pref Path opt\n");
	}

	for (i0 = 0; i0 < model.network.nPhysicalLevels - 1; i0++) {
		if (pos == 0) {
			nod2 = model.network.physicalLev[model.network.nPhysicalLevels - 1].nodDelay[0];
			nod1 = model.network.physicalLev[i0].nodDelay[model.params.preferredPathOrtoPos[i0]];
			modelTmp = &modelDelay;
		}
		else {
			nod2 = model.network.physicalLev[model.network.nPhysicalLevels - 1].nodDelay_prefPath[0];
			nod1 = model.network.physicalLev[i0].nodDelay_prefPath[model.params.preferredPathOrtoPos[i0]];
			modelTmp = &modelDelay_prefPath;
		}
		dist1 = NystaUppBV_MassTest(modelTmp, 1, nod2, nod1, &costL);
		time = 0;
		dist = 0;
		fuel = 0;
		for (i = 0; i < modelTmp->nBVArcs; i++) {
			arcNr = modelTmp->BVArc[i];
			time += modelTmp->arc[arcNr].time;
			fuel += modelTmp->arc[arcNr].fuel_noEca + modelTmp->arc[arcNr].fuel_eca + modelTmp->arc[arcNr].fuel_aux + modelTmp->arc[arcNr].fuel_auxEca;
			dist += modelTmp->arc[arcNr].distance;
			//if (thisLevel == 38 && pos1 == 29)
			//	printf("i %d arcNr %d tid %.2lf %.2lf fuelBase %.2lf fuel_aux %.4lf fuel_noEca %.4lf arcDist %.3lf\n", i, arcNr, modelDelay.arc[arcNr].time, time,
			//		modelDelay.arc[arcNr].fuel_aux * model.params.fuel.aux_noEca.price + modelDelay.arc[arcNr].fuel_auxEca * model.params.fuel.aux_eca.price +
			//		modelDelay.arc[arcNr].fuel_eca * model.params.fuel.main_eca.price + modelDelay.arc[arcNr].fuel_noEca * model.params.fuel.main_noEca.price,
			//		modelDelay.arc[arcNr].fuel_aux, modelDelay.arc[arcNr].fuel_noEca, modelDelay.arc[arcNr].distance / 1.852);
		}
		fprintf(filpek, "pos %d dist %.3lf dist1 %.3lf time %.3lf fuel %.3lf costL %I64d\n", i0, dist, dist1, time, fuel, costL);
	}
	fclose(filpek);



	return 0;
}


int solve_SP_delay() {
	int i, i1, tidInt, nod1, i2b, i2, nextLevel, setupCheckPoints;
	int nAllocNoder, nAllocArcs, cNr, nAlloc;
	bool Reached;
	double fuelQualityKvot, extraAreaCostKvot;

	model.delayRouteToEnd = (strDelayToEnd**)calloc(model.network.nPhysicalLevels, sizeof(strDelayToEnd*));
	model.delayRouteToEnd_channel = (strDelayToEnd**)calloc(model.network.nChannels, sizeof(strDelayToEnd*));
	nAlloc = model.network.nPhysicalLevels + 2 * model.network.nChannels + 2;
	model.delay.changedSpeed = (int*)calloc(nAlloc, sizeof(int));

	nAllocNoder = 1;
	nAllocArcs = 1;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].nodDelay = (int*)malloc(model.network.physicalLev[i].nPoints * sizeof(int));
		nAllocNoder += model.network.physicalLev[i].nPoints;
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			model.network.physicalLev[i].nodDelay[i1] = -1;
			nAllocArcs += model.network.physicalLev[i].nOutNodes[i1];
		}
	}
	for (i1 = 0; i1 < model.network.nChannels; i1++) {
		nAllocArcs += 1 + model.network.channel[i1].nOutNodes;
		nAllocNoder += 2;
	}

	modelDelay.Noder = (strNoder*)malloc2(nAllocNoder * sizeof(strNoder));
	for (int i0 = 0; i0 < nAllocNoder; i0++)
		modelDelay.Noder[i0].UtNod = NULL;

	modelDelay.arc = (strArcInfo*)malloc2(nAllocArcs * sizeof(strArcInfo));
	modelDelay.BVArc = (int*)malloc2((model.network.nPhysicalLevels + model.network.nChannels + 2) * sizeof(int));
	modelDelay.BVtempNodOrder = (int*)malloc2((model.network.nPhysicalLevels + model.network.nChannels + 2) * sizeof(int));


	modelDelay.nArcs = 0;
	modelDelay.nNoder = 0;
	for (i = 0; i < model.network.nChannels; i++) {
		tidInt = (int)(round(model.network.channel[i].midTimeArrive / model.params.tIndexGerH));
		adderaNodDelay(-i - 1, 0, tidInt);
		tidInt = (int)(round(model.network.channel[i].midTimeFinish / model.params.tIndexGerH));
		adderaNodDelay(-i - 1, 1, tidInt);

	}
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		//for (i = 0; i < 2; i++) {
		tidInt = (int)(round(model.network.physicalLev[i].midTimeArrive / model.params.tIndexGerH));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.network.physicalLev[i].nInNodes[i1] > 0 ||
				model.network.physicalLev[i].nOutNodes[i1] > 0)
				adderaNodDelay(i, i1, tidInt);
		}
	}

	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		//for (i = 0; i < 1; i++) {
		tidInt = (int)(round(model.network.physicalLev[i].midTimeArrive / model.params.tIndexGerH));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.results.onlyPrefPath_kaoutar == 1 && model.params.preferredPathOrtoPos[i] != i1)
				continue;
			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];
				if (nextLevel < 0)
					nextLevel = nextLevel;
				else {
					if (model.results.onlyPrefPath_kaoutar == 1 && model.params.preferredPathOrtoPos[nextLevel] != i2)
						continue;
				}
				setupCheckPoints = 1;
				// fuelQualityKvot = get_fuelQualityKvot(i, i1, nextLevel, i2);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(i, i1, nextLevel, i2, &fuelQualityKvot);
				addBage_AB_delayFysiskt(i, i1, nextLevel, i2, tidInt, fuelQualityKvot, extraAreaCostKvot);
			}
		}

		for (i1 = 0; i1 < model.network.nChannels; i1++) {
			cNr = i1;
			tidInt = (int)(round(model.network.channel[i1].midTimeFinish / model.params.tIndexGerH));
			for (i2b = 0; i2b < model.network.channel[cNr].nOutNodes; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[i2b];
				if (nextLevel != i + 1 && nextLevel >= 0)
					continue;
				if (nextLevel < 0 && i > 0)
					continue;
				if (nextLevel >= 0) {
					if (model.results.onlyPrefPath_kaoutar == 1 && model.params.preferredPathOrtoPos[nextLevel] != model.network.channel[cNr].outNode[i2b])
						continue;
				}

				// fuelQualityKvot = get_fuelQualityKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b]);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], &fuelQualityKvot);
				if (i == 2 && i1 == 3 && i2b == 8)
					i = i;
				addBage_AB_delayFysiskt(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], tidInt, fuelQualityKvot, extraAreaCostKvot);
			}
		}

	}
	for (i1 = 0; i1 < model.network.nChannels; i1++) {
		cNr = i1;
		model.functions.valuesNow.prefPathArc = 2;
		// fuelQualityKvot = get_fuelQualityKvot(-i1 - 1, 0, -i1 - 1, 1);
		extraAreaCostKvot = get_totalExtraAreaCostKvot(-i1 - 1, 0, -i1 - 1, 1, &fuelQualityKvot);
		tidInt = (int)(round(model.network.channel[i1].midTimeArrive / model.params.tIndexGerH));
		addBage_AB_delayFysiskt(-cNr - 1, 0, -cNr - 1, 1, tidInt, fuelQualityKvot, extraAreaCostKvot);
	}

	checkMinnesAnvandning(__LINE__);
	FILE* filpek;

	int sparaNetworkSPdelay = 0;
	if (sparaNetworkSPdelay == 1) {
		filpek = fopen("checkNetworkSPdelay.txt", "w");
		for (int i = 0; i < modelDelay.nArcs; i++) {
			fprintf(filpek, "arc %d from %d %d to %d %d cost %.3lf dist %lf fuelBase %lf time %lf speedSetting %d\n", i,
				modelDelay.arc[i].fromLevel, modelDelay.arc[i].fromPointNr,
				modelDelay.arc[i].toLevel, modelDelay.arc[i].toPointNr, modelDelay.arc[i].totCost,
				modelDelay.arc[i].distance, modelDelay.arc[i].fuelBase, modelDelay.arc[i].time,
				modelDelay.arc[i].speedSetting);
		}
		fprintf(filpek, "\n");
		for (int i = 0; i < modelDelay.nNoder; i++) {
			for (int i1 = 0; i1 < modelDelay.Noder[i].nUtNoder; i1++) {
				fprintf(filpek, "arc from nod %d till nod %d cost %.3lf\n", i,
					modelDelay.Noder[i].UtNod[i1], modelDelay.Noder[i].UtNodCost[i1]);
			}
		}
		fclose(filpek);
	}


	SattUppDijkstraNatverk3(&modelDelay);
	nod1 = modelDelay.nNoder - 1;
	AnropDijkstra2(nod1, nod1, &modelDelay, &Reached);
	checkMinnesAnvandning(__LINE__);

	//checkCosts_SP_delay_PP(0);

	//testAnrop(&modelDelay, nod1);

	return 0;
}

int evalDistanceBetweenPrefPathAndChannel(int level1, int level2)
{
	int i, posLast, posEnd, level;
	double totDist;
	spherical::Point pMid;

	if (level1 >= 0) {
		level = level1;
		posLast = -1;
		posEnd = model.network.channel[-level2 - 1].preferredPathPoint_posConnectTo;
		pMid = model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]];
		if (posEnd < 0)
			totDist = model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]].distanceTo(model.network.channel[-level2 - 1].point[0]) / 1000;
		else
			totDist = model.network.physicalLev[level].preferredPathPoint[posEnd].distanceTo(model.network.channel[-level2 - 1].point[0]) / 1000;
	}
	else {
		level = level2 - 1;
		posLast = model.network.channel[-level1 - 1].preferredPathPoint_posConnectFrom;
		posEnd = model.network.physicalLev[level].npreferredPathPoints;
		if (posLast < 0 || posLast >= posEnd)
			pMid = model.network.physicalLev[level].point[model.params.preferredPathOrtoPos[level]];
		else
			pMid = model.network.physicalLev[level].preferredPathPoint[posLast];
		totDist = pMid.distanceTo(model.network.channel[-level1 - 1].point[model.network.channel[-level1 - 1].nPoints - 1]) / 1000;
	}

	for (i = posLast + 1; i < posEnd; i++) {
		if (printGlobal == 1)
			printf("i %d innan totDist %.3lf\n", i, totDist);
		totDist += pMid.distanceTo(model.network.physicalLev[level].preferredPathPoint[i]) / 1000.0;
		//if (i < model.network.physicalLev[level].npreferredPathPoints - 1)
		pMid = model.network.physicalLev[level].preferredPathPoint[i];
	}

	return totDist;
}

int adderaArcDelay_prefPath(int nodNr1, int nodNr2, double cost, int speedSetting)
{
	int i;
	if (nodNr2 == 3461)
		nodNr2 = nodNr2;
	if (nodNr1 > 4000)
		nodNr1 = nodNr1;
	/*
	for (i = modelDelay.Noder[nodNr2].nUtNoder - 1; i >= 0; i--) {
		if (modelDelay.Noder[nodNr2].UtNod[i] == nodNr1) {
			if (cost < modelDelay.Noder[nodNr2].UtNodCost[i]) {
				// this speed setting is cheaper than the old one...
				modelDelay.Noder[nodNr2].UtNodCost[i] = cost;
				if (cost < 0)
					printf("ERROR cost negative %.2lf\n", cost);
				return modelDelay.Noder[nodNr2].outArcNr[i];
			}
			else {
				return -2;
			}
		}
		//if (modelDelay.Noder[modelDelay.Noder[nodNr2].UtNod[i]].pointNr != modelDelay.Noder[nodNr1].pointNr)
		//	break;
	}
	*/

	modelDelay_prefPath.Noder[nodNr2].UtNod[modelDelay_prefPath.Noder[nodNr2].nUtNoder] = nodNr1;
	modelDelay_prefPath.Noder[nodNr2].UtNodCost[modelDelay_prefPath.Noder[nodNr2].nUtNoder] = cost;
	if (cost < 0)
		printf("ERROR negative cost %.2lf\n", cost);
	if (modelDelay_prefPath.nArcs == 41034)
		modelDelay_prefPath.nArcs = modelDelay_prefPath.nArcs;
	modelDelay_prefPath.Noder[nodNr2].outArcNr[modelDelay_prefPath.Noder[nodNr2].nUtNoder] = modelDelay_prefPath.nArcs;
	(modelDelay_prefPath.Noder[nodNr2].nUtNoder)++;
	return -1;
}

int addBastSpeed_arcDelayed_prefPath(int thisLevel, int pos1, int nextLevel, int pos2, int tidInt, double fuelQualityKvot, double extraAreaCostKvot) {
	// loop over all speedsettings, find lowest cost with delay factor
	int i4, minPos, posTmp;
	double minCost = 1e20, totCost, channelCost, calmWaterSpeed, timeArc;
	double fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux;
	double fuel_eca, fuel_noEca, fuel_aux, fuel_auxEca, fuelBase, emission, factorDelay, dist;
	double x1, y1, x2, y2, speedNow;
	int arcNr, nodNr1, nodNr2;
	double minTime, minBase, minAux, minAuxEca, minEca, minNoEca, speedDiffCurrent = 0;
	double fixTime = -1, waitingTime = 0, fuelWaitCorridor = 0;
	double fuelWaitCorridor_main = 0, fuelWaitCorridor_aux = 0, totCostCompare, kvotCost = 1.0;

	if (modelDelay_prefPath.nArcs == 6111)
		modelDelay_prefPath.nArcs = modelDelay_prefPath.nArcs;
	// factorDelay = eval_factorDelayedAlongPath(thisLevel, tidInt);
	factorDelay = eval_factorDelayedAlongArc_currSpeedDiff(thisLevel, pos1, nextLevel, pos2, tidInt, &speedDiffCurrent,
		model.params.calmWaterSpeedCompareUse);
	if (nextLevel >= 0) {
		if (thisLevel >= 0) {
			if (model.params.preferredPathOrtoPos[thisLevel] == pos1 && model.params.preferredPathOrtoPos[nextLevel] == pos2)
				dist = model.network.physicalLev[thisLevel + 1].distanceFromStartPosMid - model.network.physicalLev[thisLevel].distanceFromStartPosMid;
			else {
				x1 = model.network.physicalLev[thisLevel].point_x[pos1];
				y1 = model.network.physicalLev[thisLevel].point_y[pos1];
				x2 = model.network.physicalLev[nextLevel].point_x[pos2];
				y2 = model.network.physicalLev[nextLevel].point_y[pos2];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				dist = model.network.physicalLev[thisLevel].point[pos1].distanceTo(model.network.physicalLev[nextLevel].point[pos2]) / 1000;
			}
		}
		else {
			if (pos2 == model.params.preferredPathOrtoPos[nextLevel])
				dist = evalDistanceBetweenPrefPathAndChannel(thisLevel, nextLevel);
			else {
				if (pos1 == 1)
					posTmp = model.network.channel[-thisLevel - 1].nPoints - 1;
				else
					posTmp = 0;
				x1 = model.network.channel[-thisLevel - 1].point_x[posTmp];
				y1 = model.network.channel[-thisLevel - 1].point_y[posTmp];
				x2 = model.network.physicalLev[nextLevel].point_x[pos2];
				y2 = model.network.physicalLev[nextLevel].point_y[pos2];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				dist = model.network.channel[-thisLevel - 1].point[posTmp].distanceTo(model.network.physicalLev[nextLevel].point[pos2]) / 1000;
			}
		}
	}
	else {
		if (thisLevel >= 0) {
			if (pos1 == model.params.preferredPathOrtoPos[thisLevel])
				dist = evalDistanceBetweenPrefPathAndChannel(thisLevel, nextLevel);
			else {
				x1 = model.network.physicalLev[thisLevel].point_x[pos1];
				y1 = model.network.physicalLev[thisLevel].point_y[pos1];
				if (pos2 == 1)
					posTmp = model.network.channel[-nextLevel - 1].nPoints - 1;
				else
					posTmp = 0;
				x2 = model.network.channel[-nextLevel - 1].point_x[posTmp];
				y2 = model.network.channel[-nextLevel - 1].point_y[posTmp];
				// dist = estimateLargeCircleDistance_km(y1, x1, y2, x2);
				dist = model.network.physicalLev[thisLevel].point[pos1].distanceTo(model.network.channel[-nextLevel - 1].point[posTmp]) / 1000;
			}
		}
		else {
			dist = model.network.channel[-thisLevel - 1].distance_km;
			fixTime = model.network.channel[-thisLevel - 1].timeThroughChannel;
			waitingTime = model.network.channel[-thisLevel - 1].waitingTime;
			kvotCost = model.network.channel[-thisLevel - 1].kvotCost;
			fuelWaitCorridor_main = model.network.channel[-thisLevel - 1].waiting_consumption_main;
			fuelWaitCorridor_aux = model.network.channel[-thisLevel - 1].waiting_consumption_aux;
		}
	}

	channelCost = 0;
	i4 = 0;
	totCost = channelCost;
	fuelConsumption_main = eval_fuelConsumption_both(i4, &fuelConsumption_aux, -1, -100);
	if (fixTime < -0.5) {
		calmWaterSpeed = model.params.calmWaterSpeedCompareUse; // eval_calmWaterSpeed(i4, -1, -100);
		speedNow = calmWaterSpeed / factorDelay + speedDiffCurrent;
		if (speedNow < 0.1)
			speedNow = 0.1;
		timeArc = dist / speedNow; // +waitingTime; // in hours
	}
	else {
		timeArc = fixTime; // +waitingTime; // in hours
	}

	fuelUsage_main = fuelConsumption_main * timeArc + fuelWaitCorridor_main;
	fuelUsage_aux = fuelConsumption_aux * timeArc + fuelWaitCorridor_aux;

	timeArc += waitingTime;

	fuel_eca = fuelUsage_main * (1 - fuelQualityKvot);
	fuel_noEca = fuelUsage_main * fuelQualityKvot;
	fuel_aux = fuelUsage_aux * fuelQualityKvot;
	fuel_auxEca = fuelUsage_aux * (1 - fuelQualityKvot);
	fuelBase = (fuel_aux * model.params.fuel.aux_noEca.price + fuel_auxEca * model.params.fuel.aux_eca.price +
		fuel_eca * model.params.fuel.main_eca.price + fuel_noEca * model.params.fuel.main_noEca.price) + fuelWaitCorridor;
	emission = fuel_aux * model.params.fuel.aux_noEca.emissionFactor + fuel_auxEca * model.params.fuel.aux_eca.emissionFactor +
		fuel_eca * model.params.fuel.main_eca.emissionFactor + fuel_noEca * model.params.fuel.main_noEca.emissionFactor;

	totCost += model.params.weightTime * model.params.priceTime * timeArc +
		model.params.weightFuel * fuelBase + emission * model.params.weightEmission * model.params.scaleObjEmission;
	totCost *= (1 + extraAreaCostKvot);
	if (USE_KVOTKOST == 1)
		totCost *= kvotCost;
	
	if (thisLevel == 0)
		totCost /= 2; // this to make sure there is a bigger difference between the cheapest and most expensive arc or there might be problems when solving Dijkstra

	totCostCompare = totCost;
	minCost = totCost;
	minPos = i4;
	minTime = timeArc;
	minBase = fuelBase;
	minAux = fuel_aux;
	minAuxEca = fuel_auxEca;
	minEca = fuel_eca;
	minNoEca = fuel_noEca;

	if (thisLevel >= 0)
		nodNr1 = model.network.physicalLev[thisLevel].nodDelay_prefPath[pos1];
	else
		nodNr1 = model.network.channel[-thisLevel - 1].nodDelay_prefPath[pos1];
	if (nextLevel >= 0)
		nodNr2 = model.network.physicalLev[nextLevel].nodDelay_prefPath[pos2];
	else
		nodNr2 = model.network.channel[-nextLevel - 1].nodDelay_prefPath[pos2];
	adderaArcDelay_prefPath(nodNr1, nodNr2, minCost, minPos);

	arcNr = modelDelay_prefPath.nArcs;
	modelDelay_prefPath.arc[arcNr].fromLevel = thisLevel;
	modelDelay_prefPath.arc[arcNr].toLevel = nextLevel;
	modelDelay_prefPath.arc[arcNr].fromPointNr = pos1;
	modelDelay_prefPath.arc[arcNr].toPointNr = pos2;
	modelDelay_prefPath.arc[arcNr].speedSetting = minPos;
	modelDelay_prefPath.arc[arcNr].time = minTime;
	modelDelay_prefPath.arc[arcNr].distance = dist;
	modelDelay_prefPath.arc[arcNr].fuelBase = minBase;
	modelDelay_prefPath.arc[arcNr].fuel_aux = minAux;
	modelDelay_prefPath.arc[arcNr].fuel_auxEca = minAuxEca;
	modelDelay_prefPath.arc[arcNr].fuel_eca = minEca;
	modelDelay_prefPath.arc[arcNr].fuel_noEca = minNoEca;
	modelDelay_prefPath.arc[arcNr].fuelQualityKvot = fuelQualityKvot;
	modelDelay_prefPath.arc[arcNr].extraAreaCostKvot = extraAreaCostKvot;
	modelDelay_prefPath.arc[arcNr].kvotCost = kvotCost;
	modelDelay_prefPath.arc[arcNr].totCost = minCost;
	modelDelay_prefPath.nArcs++;


	return 0;
}


int solve_SP_delayPrefPath() {
	int i, i1, tidInt, nod1, i2b, i2, nextLevel, setupCheckPoints;
	int nAllocNoder, nAllocArcs, cNr, nAlloc;
	bool Reached;
	double fuelQualityKvot, extraAreaCostKvot;

	model.delayRouteToEnd_prefPath = (strDelayToEnd**)calloc(model.network.nPhysicalLevels, sizeof(strDelayToEnd*));
	model.delayRouteToEnd_channel_prefPath = (strDelayToEnd**)calloc(model.network.nChannels, sizeof(strDelayToEnd*));
	nAlloc = model.network.nPhysicalLevels + 2 * model.network.nChannels + 2;

	nAllocNoder = 1;
	nAllocArcs = 1;
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		model.network.physicalLev[i].nodDelay_prefPath = (int*)malloc(model.network.physicalLev[i].nPoints * sizeof(int));
		nAllocNoder++;// += model.network.physicalLev[i].nPoints;
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			model.network.physicalLev[i].nodDelay_prefPath[i1] = -1;
			if (model.params.preferredPathOrtoPos[i] == i1)
				nAllocArcs += model.network.physicalLev[i].nOutNodes[i1];
		}
	}
	for (i1 = 0; i1 < model.network.nChannels; i1++) {
		nAllocArcs += 1 + model.network.channel[i1].nOutNodes;
		nAllocNoder += 2;// += model.network.physicalLev[i].nPoints;
	}

	modelDelay_prefPath.Noder = (strNoder*)malloc2(nAllocNoder * sizeof(strNoder));
	for (int i0 = 0; i0 < nAllocNoder; i0++)
		modelDelay_prefPath.Noder[i0].UtNod = NULL;

	modelDelay_prefPath.arc = (strArcInfo*)malloc2(nAllocArcs * sizeof(strArcInfo));
	modelDelay_prefPath.BVArc = (int*)malloc2((model.network.nPhysicalLevels + model.network.nChannels + 2) * sizeof(int));
	modelDelay_prefPath.BVtempNodOrder = (int*)malloc2((model.network.nPhysicalLevels + model.network.nChannels + 2) * sizeof(int));


	modelDelay_prefPath.nArcs = 0;
	modelDelay_prefPath.nNoder = 0;
	for (i = 0; i < model.network.nChannels; i++) {
		tidInt = (int)(round(model.network.channel[i].midTimeArrive / model.params.tIndexGerH));
		adderaNodDelay_prefPath(-i - 1, 0, tidInt);
		tidInt = (int)(round(model.network.channel[i].midTimeFinish / model.params.tIndexGerH));
		adderaNodDelay_prefPath(-i - 1, 1, tidInt);

	}
	checkMinnesAnvandning(__LINE__);
	for (i = 0; i < model.network.nPhysicalLevels; i++) {
		//for (i = 0; i < 2; i++) {
		tidInt = (int)(round(model.network.physicalLev[i].midTimeArrive / model.params.tIndexGerH));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.params.preferredPathOrtoPos[i] != i1)
				continue;
			if (model.network.physicalLev[i].nInNodes[i1] > 0 ||
				model.network.physicalLev[i].nOutNodes[i1] > 0)
				adderaNodDelay_prefPath(i, i1, tidInt);
		}
	}

	for (i = 0; i < model.network.nPhysicalLevels - 1; i++) {
		//for (i = 0; i < 1; i++) {
		tidInt = (int)(round(model.network.physicalLev[i].midTimeArrive / model.params.tIndexGerH));
		for (i1 = 0; i1 < model.network.physicalLev[i].nPoints; i1++) {
			if (model.params.preferredPathOrtoPos[i] != i1)
				continue;
			for (i2b = 0; i2b < model.network.physicalLev[i].nOutNodes[i1]; i2b++) {
				i2 = model.network.physicalLev[i].outNode[i1][i2b];
				nextLevel = model.network.physicalLev[i].outLevel[i1][i2b];
				if (nextLevel < 0)
					nextLevel = nextLevel;
				else {
					if (model.params.preferredPathOrtoPos[nextLevel] != i2)
						continue;
				}
				setupCheckPoints = 1;
				// fuelQualityKvot = get_fuelQualityKvot(i, i1, nextLevel, i2);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(i, i1, nextLevel, i2, &fuelQualityKvot);
				addBastSpeed_arcDelayed_prefPath(i, i1, nextLevel, i2, tidInt, fuelQualityKvot, extraAreaCostKvot);
			}
		}

		for (i1 = 0; i1 < model.network.nChannels; i1++) {
			cNr = i1;
			tidInt = (int)(round(model.network.channel[i1].midTimeFinish / model.params.tIndexGerH));
			for (i2b = 0; i2b < model.network.channel[cNr].nOutNodes; i2b++) {
				nextLevel = model.network.channel[cNr].outLevel[i2b];
				if (nextLevel != i + 1)
					continue;
				if (model.params.preferredPathOrtoPos[nextLevel] != model.network.channel[cNr].outNode[i2b])
					continue;

				// fuelQualityKvot = get_fuelQualityKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b]);
				extraAreaCostKvot = get_totalExtraAreaCostKvot(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], &fuelQualityKvot);
				addBastSpeed_arcDelayed_prefPath(-cNr - 1, 1, nextLevel, model.network.channel[cNr].outNode[i2b], tidInt, fuelQualityKvot, extraAreaCostKvot);
			}
		}

	}
	for (i1 = 0; i1 < model.network.nChannels; i1++) {
		cNr = i1;
		model.functions.valuesNow.prefPathArc = 2;
		// fuelQualityKvot = get_fuelQualityKvot(-i1 - 1, 0, -i1 - 1, 1);
		extraAreaCostKvot = get_totalExtraAreaCostKvot(-i1 - 1, 0, -i1 - 1, 1, &fuelQualityKvot);
		tidInt = (int)(round(model.network.channel[i1].midTimeArrive / model.params.tIndexGerH));
		addBastSpeed_arcDelayed_prefPath(-cNr - 1, 0, -cNr - 1, 1, tidInt, fuelQualityKvot, extraAreaCostKvot);
	}

	checkMinnesAnvandning(__LINE__);
	FILE* filpek;

	int sparaNetworkSPdelay2 = 0;
	if (sparaNetworkSPdelay2 == 1) {
		filpek = fopen("checkNetworkSPdelay_prefPath.txt", "w");
		for (int i = 0; i < modelDelay_prefPath.nArcs; i++) {
			fprintf(filpek, "arc %d from %d %d to %d %d cost %.3lf dist %lf fuelBase %lf time %lf speedSetting %d\n", i,
				modelDelay_prefPath.arc[i].fromLevel, modelDelay_prefPath.arc[i].fromPointNr,
				modelDelay_prefPath.arc[i].toLevel, modelDelay_prefPath.arc[i].toPointNr, modelDelay_prefPath.arc[i].totCost,
				modelDelay_prefPath.arc[i].distance, modelDelay_prefPath.arc[i].fuelBase, modelDelay_prefPath.arc[i].time,
				modelDelay_prefPath.arc[i].speedSetting);
		}
		fprintf(filpek, "\n");
		for (int i = 0; i < modelDelay_prefPath.nNoder; i++) {
			for (int i1 = 0; i1 < modelDelay_prefPath.Noder[i].nUtNoder; i1++) {
				fprintf(filpek, "arc from nod %d till nod %d cost %.3lf\n", i,
					modelDelay_prefPath.Noder[i].UtNod[i1], modelDelay_prefPath.Noder[i].UtNodCost[i1]);
			}
		}
		fclose(filpek);
	}

	SattUppDijkstraNatverk3(&modelDelay_prefPath);
	nod1 = modelDelay_prefPath.nNoder - 1;
	AnropDijkstra2(nod1, nod1, &modelDelay_prefPath, &Reached);
	checkMinnesAnvandning(__LINE__);

	//checkCosts_SP_delay_PP(1);

	//testAnrop(&modelDelay, nod1);

	return 0;
}


double eval_vesselBearing(double y1, double x1, double y2, double x2) {
	double bearing;

	spherical::Point p1, p2;

	p1 = spherical::Point(y1, fix_lonPos(x1));
	p2 = spherical::Point(y2, fix_lonPos(x2));

	//bearing = (90 - p1.bearingTo(p2)) * M_PI / 180;
	bearing = p1.bearingTo(p2);
	double bearingRadians = (90 - bearing) * M_PI / 180;
	if (bearingRadians < -M_PI)
		bearingRadians += 2 * M_PI;

	return bearingRadians;
}


double eval_calmWaterSpeed_fromRPM_base(double rpm, int* index, double* kvotRet) {
	int i;
	double speed, kvot;

	for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
		if (rpm <= model.functions.rpmBase[i])
			break;
	}
	if (i < model.functions.nShip_speedSettingsBase) {
		if (i == 0) {
			if (rpm < model.functions.rpmBase[i])
				errlog("ERROR! Too low rpm %.3lf. Lowest one given in speed setting is %.3lf which I use\n", rpm, model.functions.rpmBase[i]);
			speed = model.functions.rpmSetting_gerCalmWaterSpeedBase[i];
			*kvotRet = 1;
			*index = 0;
		}
		else {
			kvot = (rpm - model.functions.rpmBase[i - 1]) / (model.functions.rpmBase[i] - model.functions.rpmBase[i - 1]);
			speed = (1 - kvot) * model.functions.rpmSetting_gerCalmWaterSpeedBase[i - 1] + kvot * model.functions.rpmSetting_gerCalmWaterSpeedBase[i];
			*kvotRet = 1 - kvot;
			*index = i - 1;
		}
	}
	else {
		errlog("ERROR! Too high rpm %.3lf. Highest one given in speed setting is %.3lf which I use\n", rpm, model.functions.rpmBase[i - 1]);
		speed = model.functions.rpmSetting_gerCalmWaterSpeedBase[i - 1];
		*index = i - 2;
		*kvotRet = 0;
	}
	return speed;
}

double getIndexKvot_fromCalmWaterSpeed(double calmWaterSpeed, int* index, double* kvotRet) {
	int i;
	double rpm, kvot;

	for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
		if (calmWaterSpeed <= model.functions.rpmSetting_gerCalmWaterSpeedBase[i])
			break;
	}
	if (i < model.functions.nShip_speedSettingsBase) {
		if (i == 0) {
			if (calmWaterSpeed < model.functions.rpmSetting_gerCalmWaterSpeedBase[i])
				errlog("ERROR! Too low calmWaterSpeed %.3lf. Lowest one given in speed setting is %.3lf which data I use for fuel consumption\n",
					calmWaterSpeed, model.functions.rpmSetting_gerCalmWaterSpeedBase[i]);
			rpm = model.functions.rpmBase[i];
			*kvotRet = 1;
			*index = 0;
		}
		else {
			kvot = (calmWaterSpeed - model.functions.rpmSetting_gerCalmWaterSpeedBase[i - 1]) /
				(model.functions.rpmSetting_gerCalmWaterSpeedBase[i] - model.functions.rpmSetting_gerCalmWaterSpeedBase[i - 1]);
			rpm = (1 - kvot) * model.functions.rpmBase[i - 1] + kvot * model.functions.rpmBase[i];
			*kvotRet = 1 - kvot;
			*index = i - 1;
		}
	}
	else {
		errlog("ERROR! Too high calmWaterSpeed %.3lf. Highest one given in speed setting is %.3lf which data I use for fuel consumption\n",
			calmWaterSpeed, model.functions.rpmSetting_gerCalmWaterSpeedBase[i - 1]);
		rpm = model.functions.rpmBase[i - 1];
		*index = i - 2;
		*kvotRet = 0;
	}
	return rpm;
}

int loadKaoutarShipSpeeds(std::string inputPath) {

	json dataJson, dataJson2, dataJson3;
	int i1;
	std::ifstream fil;
	std::string namnID;
	char* namn;

	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%skaoutarShipSpeed.json", inputPath.c_str());
	//printf("opens %s\n", namn);
	fil.open(namn);
	fil >> dataJson;

	dataJson2 = dataJson["ship_specification"];

	int nShipTypes = dataJson2.size();
	model.kaoutar = (strKaoutar*)malloc(nShipTypes * sizeof(strKaoutar));

	int i = 0;
	for (auto it = dataJson2.begin(); it != dataJson2.end(); ++it) {
		json dataSpec = it.value();
		namnID = dataSpec["tableID_wind"];
		model.kaoutar[i].nShipTypes = 0;
		model.kaoutar[i].windTableID = str_alloc_cpyString(cleanString(namnID));
		namnID = dataSpec["tableID_wave"];
		model.kaoutar[i].waveTableID = str_alloc_cpyString(cleanString(namnID));
		namnID = dataSpec["tableID_stability"];
		model.kaoutar[i].stabilityTableID = str_alloc_cpyString(cleanString(namnID));

		json dataSpeed = dataSpec["shipSpeedSettings"];
		model.kaoutar[i].nShip_speedSettingsBase = dataSpeed.size();
		model.kaoutar[i].rpmBase = (double*)malloc2(model.kaoutar[i].nShip_speedSettingsBase * sizeof(double));
		model.kaoutar[i].rpmSetting_gerCalmWaterSpeedBase = (double*)malloc2(model.kaoutar[i].nShip_speedSettingsBase * sizeof(double));
		model.kaoutar[i].rpmSetting_gerFuelConsumption_mainBase = (double*)malloc2(model.kaoutar[i].nShip_speedSettingsBase * sizeof(double));
		model.kaoutar[i].rpmSetting_gerFuelConsumption_auxBase = (double*)malloc2(model.kaoutar[i].nShip_speedSettingsBase * sizeof(double));
		i1 = 0;
		for (auto it = dataSpeed.begin(); it != dataSpeed.end(); ++it) {
			json dataSpeed2 = it.value();
			if (!dataSpeed2["rpm"].is_null())
				model.kaoutar[i].rpmBase[i1] = dataSpeed2["rpm"];
			else {
				errlog("ERROR! No rpm in shipSpeedSetting nr %d in input data. I set it to 70 so I can keep loading data\n", i1);
				model.kaoutar[i].rpmBase[i1] = 70;
			}
			if (!dataSpeed2["calmWaterSpeed_kts"].is_null())
				model.kaoutar[i].rpmSetting_gerCalmWaterSpeedBase[i1] = (double)(dataSpeed2["calmWaterSpeed_kts"]) * model.params.knots_to_km;
			else {
				errlog("ERROR! No calmWaterSpeed_kts in a shipSpeedSetting in input data. I set it to 11 so I can keep loading data\n");
				model.kaoutar[i].rpmSetting_gerCalmWaterSpeedBase[i1] = 11 * model.params.knots_to_km;
			}
			if (!dataSpeed2["fuelConsumption_main_mpd"].is_null())
				model.kaoutar[i].rpmSetting_gerFuelConsumption_mainBase[i1] = (double)(dataSpeed2["fuelConsumption_main_mpd"]) / 24.0;
			else {
				errlog("ERROR! No fuelConsumption_main_mpd in a shipSpeedSetting in input data. I set it to 11 so I can keep loading data\n");
				model.kaoutar[i].rpmSetting_gerFuelConsumption_mainBase[i1] = 11 / 24.0;
			}

			if (!dataSpeed2["fuelConsumption_aux_mpd"].is_null())
				model.kaoutar[i].rpmSetting_gerFuelConsumption_auxBase[i1] = (double)(dataSpeed2["fuelConsumption_aux_mpd"]) / 24.0;
			else {
				errlog("ERROR! No fuelConsumption_aux_mpd in a shipSpeedSetting in input data. I set it to 2 so I can keep loading data\n");
				model.kaoutar[i].rpmSetting_gerFuelConsumption_auxBase[i1] = 2 / 24.0;
			}
			i1++;
		}
		i++;
	}
	return nShipTypes;
}

int evalKaoutarData(std::string inputPath) {
	int i, antal, rad, index;
	double x0, y0, x1, y1, sog, currentDirection, currentSpeed;
	double wavePeriod = 10.0, rpm, windDirection, windSpeed, waveDirection, waveHeight;
	double calmWaterSpeed, vesselBearing, baseGroundSpeed, rel_windSpeed, rel_waveDir;
	double rel_windDir, speedDiffWind, speedDiffWave, kvot;
	double fuelConsumption_main, fuelConsumption_aux;
	double speedDiffWindWave, speedOverGround;
	double weatherFactorCurrent, weatherFactorWind, weatherFactorWave;

	char* namn = (char*)malloc(256 * sizeof(char));
	model.params.indataPath = inputPath;
	model.params.knots_to_km = 1.852;

	strKaoutarData* data;
	int nAlloc = 10000, nData = 0, i1;

	data = (strKaoutarData*)malloc(nAlloc * sizeof(strKaoutarData));

	int nShipTypes = loadKaoutarShipSpeeds(inputPath);

	sprintf(namn, "%skaoutarData.txt", inputPath.c_str());
	FILE* filpek = fopen(namn, "r");
	sprintf(namn, "%sres_kaoutarData.txt", inputPath.c_str());
	FILE* filut = fopen(namn, "w");
	FILE* filIter = fopen("data/resIter.txt", "w");
	fprintf(filIter, "shipType;dataNr;iter;updatKvot;cws;sog;calcSOG;diff\n");
	int nMaxIter, i2;
	double speedDiff, rpmSetting, updatKvot;

	for (i = 0; i < 10000; i++) {
		if (nData >= nAlloc) {
			nAlloc += 10000;
			data = (strKaoutarData*)realloc(data, nAlloc * sizeof(strKaoutarData));
		}
		antal = fscanf(filpek, "%d\t%lf\t%lf\t%lf\t%lf\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
			&(data[nData].radNr), &(data[nData].x0), &(data[nData].y0), &(data[nData].x1), &(data[nData].y1),
			&(data[nData].shipType), &(data[nData].rpm), &(data[nData].sog), &(data[nData].fuelCons),
			&(data[nData].windDir), &(data[nData].windSpeed), &(data[nData].waveDir), &(data[nData].waveHeight),
			&(data[nData].currDir), &(data[nData].currSpeed));
		data[nData].sog *= 1.852;

		if (antal <= 0)
			break;

		if (data[nData].shipType >= nShipTypes) {
			printf("ERROR! shipeType %d given in kaoutarData.txt row %d but only up to %d exists in kaoutarShipSpeed.json. I skip this one\n",
				data[nData].shipType, i + 1, nShipTypes - 1);
			continue;
		}

		nData++;
	}

	fprintf(filut, "shipAlt\tdataNo\trowNr\trpm\ty0\tx0\ty1\tx1\tvesselBearing\trpmSettingFuel\tcalmWaterSpeed_kts\t"
		"currDir\tcurrSpeed_kts\tbaseGroundSpeed_kts\tspeedOverGround_kts\tspeedDiffCurrent_kts\trel_windSpeed_kts\t"
		"rel_windDir\tspeedDiffWind_kts\twaveHeight\twavePeriod\trel_waveDir\tspeedDiffWave_kts\t"
		"weatherFactorCurrent\tweatherFactorWind\tweatherFactorWave\tfuelConsumption_main_ton_24h\tfuelConsumption_aux_ton_24h\n");

	wavePeriod = 10.0; 

	nMaxIter = 15;
	for (i = 0; i < nShipTypes; i++) {
		model.functions.windTableID = model.kaoutar[i].windTableID;
		model.functions.waveTableID = model.kaoutar[i].waveTableID;
		model.functions.stabilityTableID = model.kaoutar[i].stabilityTableID;
		model.functions.nShip_speedSettingsBase = model.kaoutar[i].nShip_speedSettingsBase;
		model.functions.rpmBase = model.kaoutar[i].rpmBase;
		model.functions.rpmSetting_gerCalmWaterSpeedBase = model.kaoutar[i].rpmSetting_gerCalmWaterSpeedBase;


		loadAllNeededTablesFromSQLite();
		for (i1 = 0; i1 < nData; i1++) {
			if (data[i1].shipType != i)
				continue;

			vesselBearing = eval_vesselBearing(data[i1].y0, data[i1].x0, data[i1].y1, data[i1].x1);

			updatKvot = 1.0;
			for (i2 = 0; i2 < nMaxIter; i2++) {
				if (i2 == 0) {
					if (nMaxIter == 1)
						calmWaterSpeed = eval_calmWaterSpeed_fromRPM_base(data[i1].rpm, &index, &kvot);
					else
						calmWaterSpeed = data[i1].sog;
				}
				else {
					speedDiff = speedOverGround - data[i1].sog;
					if (abs(speedDiff) < 0.01)
						break;
					if (i2 == 3)
						updatKvot = 0.5;
					if (i2 == 6)
						updatKvot = 0.25;
					if (i2 == 8)
						updatKvot = 0.15;
					if (i2 == 11)
						updatKvot = 0.05;
					calmWaterSpeed -= updatKvot * speedDiff;
				}

				baseGroundSpeed = eval_baseGroundSpeed(calmWaterSpeed, vesselBearing,
					data[i1].currDir, data[i1].currSpeed);

				rel_windSpeed = eval_relWindSpeedExact(baseGroundSpeed, vesselBearing,
					data[i1].windDir, data[i1].windSpeed, &rel_windDir);
				//rel_waveDir = M_PI + ((270 - data[i1].waveDir) * M_PI / 180 - vesselBearing); // / model.functions.nWaveDir;
				rel_waveDir = data[i1].waveDir - vesselBearing; // / model.functions.nWaveDir;
				if (rel_waveDir < 0)
					rel_waveDir = -rel_waveDir;
				if (rel_waveDir >= 2 * M_PI)
					rel_waveDir -= 2 * M_PI;
				if (rel_waveDir > M_PI)
					rel_waveDir = 2 * M_PI - rel_waveDir;


				speedDiffWind = lookup_speedDiffWindTable(baseGroundSpeed, rel_windSpeed, rel_windDir);
				speedDiffWave = lookup_speedDiffWaveTable(calmWaterSpeed, data[i1].waveHeight, wavePeriod, rel_waveDir);
				speedDiffWindWave = speedDiffWind + speedDiffWave;
				speedOverGround = baseGroundSpeed - speedDiffWindWave; // in km/h
				fprintf(filIter, "%d;%d;%d;%lf;%lf;%lf;%lf;%lf\n",
					i, i1, i2, updatKvot, calmWaterSpeed, data[i1].sog, speedOverGround, speedOverGround - data[i1].sog);
			}

			weatherFactorCurrent = 2 - baseGroundSpeed / calmWaterSpeed;
			weatherFactorWind = 1 + speedDiffWind / baseGroundSpeed;
			weatherFactorWave = 1 + speedDiffWave / baseGroundSpeed;

			if (nMaxIter > 1)
				rpmSetting = getIndexKvot_fromCalmWaterSpeed(calmWaterSpeed, &index, &kvot);
			else
				rpmSetting = data[i1].rpm;

			fuelConsumption_main = kvot * model.kaoutar[i].rpmSetting_gerFuelConsumption_mainBase[index] +
				(1 - kvot) * model.kaoutar[i].rpmSetting_gerFuelConsumption_mainBase[index + 1];
			fuelConsumption_aux = kvot * model.kaoutar[i].rpmSetting_gerFuelConsumption_auxBase[index] +
				(1 - kvot) * model.kaoutar[i].rpmSetting_gerFuelConsumption_auxBase[index + 1];
			// ton/h?

			fprintf(filut, "%d\t%d\t%d\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t"
				"%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\t%.3lf\n",
				i, i1, data[i1].radNr, data[i1].rpm, data[i1].y0,
				data[i1].x0, data[i1].y1, data[i1].x1, vesselBearing, rpmSetting, calmWaterSpeed / 1.852,
				data[i1].currDir, data[i1].currSpeed / 1.852, baseGroundSpeed / 1.852, speedOverGround / 1.852, (baseGroundSpeed - calmWaterSpeed) / 1.852,
				rel_windSpeed / 1.852, rel_windDir, -speedDiffWind / 1.852,
				data[i1].waveHeight, wavePeriod, rel_waveDir, -speedDiffWave / 1.852,
				weatherFactorCurrent, weatherFactorWind, weatherFactorWave,
				fuelConsumption_main * 24, fuelConsumption_aux * 24);
		}
	}

	fclose(filut);
	fclose(filpek);
	free(namn);
	fclose(filIter);

	return 0;
}

void redisTestRead() {
	std::string keyID;
	json mData;
	int nBlockRows, nBlockCols, ii, xBlockStart, xBlockEnd, yBlockStart, yBlockEnd;
	int i1, i2, forsta, nBands, pos, pos2, i3, i4, i5;
	size_t nAlloc;
	float* arrFloat, lat, lon, rowDbl, colDbl;
	FILE* filtmp;

#ifndef ONBOARD
	auto redis = Redis("tcp://127.0.0.1:6379/1");
	auto tid0 = std::chrono::high_resolution_clock::now();

	//printf("testA\n");
	arrFloat = NULL;

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
		model.weather[ii].valueCell = (float**)malloc2(nBands * sizeof(float*));
		nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
		for (int i2 = 0; i2 < nBands; i2++) {
			model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
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
				keyID += std::to_string(pos);
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






#endif


}

time_t make_gmtime_now() {
	struct tm tmBas = { std::time(0) };
	tmBas.tm_isdst = 0;

	//#ifdef _WIN32
	//	time_t rawtime = _mkgmtime(&tmBas);
	//#endif
	//#ifndef _WIN32
	//	time_t rawtime = timegm(&tmBas);
	//#endif
		//cout << "nSeconds " << tmBas.tm_sec << "\n";
	return tmBas.tm_sec;
	//return rawtime;
}


int redisSetKeys(std::string inputPath) {
	int ii, i1, i2, i3, i4, i5, xPos1, yPos0, yPos1, nBands;
	size_t nAlloc;
	int nBlockRows, nBlockCols, pos, pos2, manad, dag;
	double maxLong, xPosFrac, size_col, size_row, lat, lon, rowDbl, colDbl;
	float* arrFloat;
	float filKvot;
	std::string keyID, histFileName;
	json mData;
	long long nSecondsUTC_last, nSecondsHistory_first, nSecondsHistory_firstStartDay;
	long long lastSecondUTC_needed, nExtraSecondsNeeded, nSecondsUTC_first, secondsNow;
	int nDaysNeeded_history, openOK, zNu, iY, iX, pos1;

#ifndef ONBOARD
	resultPath = inputPath;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));

	sprintf(namn, "%s/missingFiles.txt", resultPath.c_str());
	FILE* filNu = fopen(namn, "w");
	fclose(filNu);

	sprintf(namn, "%s/checkWeatherData_tmp.txt", resultPath.c_str());

	//printf("opens %s\n", namn);
	FILE* filcheck = fopen(namn, "w");
	if (filcheck == NULL) {
		errlog("ERROR! Could not open %s\n", namn);
		postRequest("ERROR! Could not open " + std::string(namn), 1);
	}
	printf("done\n");
	fprintf(filcheck, "testing\n");
	printf("done2\n");

	sprintf(namn, "%s/checkWeather2.txt", resultPath.c_str());
	FILE* filCheck2 = fopen(namn, "w");

	std::list<int> listOfInts;

	std::stringstream stream, stream2;
	stream.precision(3);
	stream << std::fixed;
	//stream2.precision(3);
	//stream2 << fixed;

	//printf("test1\n");

	//model.params.variableFileName = inputPath;

	initModelStatusValues();


	//model.params.indataPath = inputPath;
	//model.params.errorCode = 0;
	//loadParams_theRestOld(&(model.params));

	GDALAllRegister();

	// Raster test;
	// printf("skips everything...\n");
	// return 0;


	//testAnropRedisMap();

	printf("opening redis\n");
	auto redis = Redis("tcp://127.0.0.1:6379/1");
	std::string redisTest;
	try {
		redisTest = redis.ping();
		if (redisTest != "PONG") {
			errlog("ERROR! Redis is not running on the server. Start it and try again\n");
			printf("ERROR! Redis is not running on the server. Start it and try again\n");
			postRequest("ERROR! Redis is not running on the server. Start it and try again", 1);
		}
	}
	catch (...) {
		errlog("ERROR! Redis is not running on the server. Start it and try again\n");
		printf("ERROR! Redis is not running on the server. Start it and try again\n");
		postRequest("ERROR! Redis is not running on the server. Start it and try again", 1);
	}

	loadVariables(1);

	json mDataParam;

	lastSecondUTC_needed = make_gmtime_now() + model.params.longestRouteDays_history * 3600 * 24;


	//char* namnTest = (char*)malloc(256 * sizeof(char));
	//sprintf(namnTest, "F:/TNM/runDir/data/weather/u0_01_2018.grb");
	//openOK = model.weather[0].rasterPos[0].open(namnTest);
	//sprintf(namnTest, "F:/TNM/runDir/data/weather/u0_03_2018.grb");
	//openOK = model.weather[0].rasterPos[0].open(namnTest);

	for (ii = 0; ii < model.nWeatherFiles; ii++) {
		printf("redisSetKeys weather file %d of %d\n", ii, model.nWeatherFiles);
		printf("\tweatherFileTypeName %s\n", model.weather[ii].weatherFileTypeName);
		size_col = -1;
		maxLong = -9999;
		//for (i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
		//	if (maxLong < model.weather[ii].filePos[i1].maxX)
		//		maxLong = model.weather[ii].filePos[i1].maxX;
		//}
		model.weather[ii].timePosToBandPos = NULL;

		filKvot = 1.0; // to change from m/s to km/h
		if (strcmp(model.weather[ii].weatherFileTypeName, "wind_uComponent") == 0)
			filKvot = 3.6;
		if (strcmp(model.weather[ii].weatherFileTypeName, "wind_vComponent") == 0)
			filKvot = 3.6;
		if (strcmp(model.weather[ii].weatherFileTypeName, "current_uComponent") == 0)
			filKvot = 3.6;
		if (strcmp(model.weather[ii].weatherFileTypeName, "current_vComponent") == 0)
			filKvot = 3.6;

		for (i1 = 0; i1 < model.weather[ii].nFiles; i1++) {
			//printf("test1 ii %d %d\n", ii, i1);
			openOK = model.weather[ii].rasterPos[i1].open(model.weather[ii].filePos[i1].fileName);
			printf("%s openOK %d\n", model.weather[ii].filePos[i1].fileName, openOK);
			if (openOK != 1) {
				errlog("ERROR! Could not open forecast file %s. This one must exist. I quit\n",
					model.weather[ii].filePos[i1].fileName);
				postRequest("ERROR! Could not open forecast file " + std::string(model.weather[ii].filePos[i1].fileName) + ". This one must exist.I quit", 1);
			}
			nBands = model.weather[ii].rasterPos[i1].Get_nBands();
			model.weather[ii].filePos[i1].minX = model.weather[ii].rasterPos[i1].Get_minLongitude();
			model.weather[ii].filePos[i1].maxX = model.weather[ii].rasterPos[i1].Get_maxLongitude();
			if (maxLong < model.weather[ii].filePos[i1].maxX)
				maxLong = model.weather[ii].filePos[i1].maxX;
			if (i1 == 0) {
				nSecondsUTC_last = model.weather[ii].rasterPos[i1].GetMetaData_nSecondsUTC_last(&nSecondsUTC_first);
				nSecondsHistory_first = nSecondsUTC_last + model.weather_timeIntervall_h * 3600;
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
				model.weather[ii].secondsUTC = (long long*)malloc2((nBands + nDaysNeeded_history) * sizeof(long long));
				// printf("\n\n### secondsUTC alloc %d ####\n\n\n", nBands + nDaysNeeded_history);
				model.weather[ii].valueCell = (float**)malloc2((nBands + nDaysNeeded_history) * sizeof(float*));
				nAlloc = model.weather[ii].nCols * model.weather[ii].nRows;
				for (i2 = 0; i2 < nBands + nDaysNeeded_history; i2++) {
					model.weather[ii].valueCell[i2] = (float*)malloc2(nAlloc * sizeof(float));
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
					errlog("ERROR! Different number of bands for weather parameter %s, %d and %d, file %s. Must be the same. I quit\n",
						model.weather[ii].weatherFileTypeName, model.weather[ii].nTimeIntervals_forecast, nBands,
						model.weather[ii].filePos[i1].fileName);
					model.weather[ii].errorCode = 2;
					continue;
				}
			}
			//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
			//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
			//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
			model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), 0, filKvot);

			for (i2 = 0; i2 < nDaysNeeded_history; i2++) {
				// printf("i2 %d\n", i2);
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
				}
				else
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
					errlog("Open %s okay.\n",
						histFileName.c_str());
					if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001 ||
						abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
						if (abs(model.weather[ii].rasterPos[i1].Get_sizeCol() - size_col) > 0.0001) {
							printf("ERROR! raster size longitude differ for weather parameter %s for file %s, %lf vs %lf. Must be the same\n",
								model.weather[ii].weatherFileTypeName, histFileName.c_str(), size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
							errlog("ERROR! raster size longitude differ for weather parameter %s for file %s, %lf vs %lf. Must be the same\n",
								model.weather[ii].weatherFileTypeName, histFileName.c_str(), size_col, model.weather[ii].rasterPos[i1].Get_sizeCol());
						}
						if (abs(model.weather[ii].rasterPos[i1].Get_sizeRow() - size_row) > 0.0001) {
							errlog("ERROR! raster size latitude differ for weather parameter %s for file %s, %lf vs %lf. Must be the same\n",
								model.weather[ii].weatherFileTypeName, histFileName.c_str(), size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
							printf("ERROR! raster size latitude differ for weather parameter %s for file %s, %lf vs %lf. Must be the same\n",
								model.weather[ii].weatherFileTypeName, histFileName.c_str(), size_row, model.weather[ii].rasterPos[i1].Get_sizeRow());
						}
						model.weather[ii].errorCode = 4;
						model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
						continue;
					}
					//printf("test1c %d %s min/maxX %.2lf %.2lf nBand %d\n", i1, model.weather[ii].filePos[i1].fileName, 
					//	model.weather[ii].rasterPos[i1].Get_minLongitude(), 
					//	model.weather[ii].rasterPos[i1].Get_maxLongitude(), model.weather[ii].rasterPos[i1].Get_nBands());
					model.weather[ii].rasterPos[i1].GetRasterValues_realAllBands(&(model.weather[ii]), model.weather[ii].nTimeIntervals_forecast + i2, filKvot);
					//printf("history day %d changes secondsUTC from %I64d to %I64d\n", i2,
					//	model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2], secondsNow);
				}
				else {
					errlog("ERROR! Failed to open %s. I use weather data from the previous loaded file\n",
						histFileName.c_str());
					zNu = model.weather[ii].nTimeIntervals_forecast + i2;
					for (iY = 0; iY < model.weather[ii].nRows; iY++) {
						for (iX = 0; iX < model.weather[ii].nCols; iX++) {
							model.weather[ii].valueCell[zNu][iX + model.weather[ii].nCols * iY] =
								model.weather[ii].valueCell[zNu - 1][iX + model.weather[ii].nCols * iY];
						}
					}

					(model.status.weatherHistoryOpenFile_fail)++;
					printf("ERROR! Failed to open %s, nFailed %d. I use weather data from the previous loaded file. secondsNow %I64d\n",
						histFileName.c_str(), model.status.weatherHistoryOpenFile_fail, secondsNow);
				}
				//printf("test %d\n", model.weather[ii].nTimeIntervals_forecast + i2);
				model.weather[ii].secondsUTC[model.weather[ii].nTimeIntervals_forecast + i2] = secondsNow;
				//printf("test igen\n");
			}
			//printf("test igen2\n");


		}
		// printf("test igen3\n");

		fprintf(filcheck, "\nvar %d %s tidsperioder\n", ii, model.weather[ii].weatherFileTypeName);
		// printf("\nvar %d %s tidsperioder\n", ii, model.weather[ii].weatherFileTypeName);
		for (i1 = 0; i1 < model.weather[ii].nTimeIntervals; i1++) {
			stringDateFromUTCSeconds(model.weather[ii].secondsUTC[i1]);
			fprintf(filcheck, "%d %I64d %s\n", i1, model.weather[ii].secondsUTC[i1],
				stringDateFromUTCSeconds(model.weather[ii].secondsUTC[i1]).c_str());
		}

		// printf("test2\n");
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
		//printf("nAlloc = %d nTimePeriods %d nBlockCols/Rows %d %d\n",
		//	nAlloc, model.weather[ii].nTimeIntervals, nBlockCols, nBlockRows);
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
		arrFloat = (float*)malloc2(nAlloc * sizeof(float));
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
							if (i4 < model.weather[ii].nRows && i5 < model.weather[ii].nCols) {
								//if (model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4] < 9998)
								//	arrFloat[pos2] = model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4] * kvot;
								//else
								arrFloat[pos2] = model.weather[ii].valueCell[i3][i5 + model.weather[ii].nCols * i4];
							}
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
				keyID += std::to_string(pos);
				//printf("Setting key %s for i3 0 %d i4 %d %d i5 %d %d pos %d\n", keyID.c_str(),
				//	nBands, nBlockRows * i1, i4-1, nBlockCols * i2, i5-1, pos);
				//if (pos == 10) {
				//	redis.set(keyID, stream.str());
				//	printf("pos %d arrFloat[471839] = %.3lf\n", pos, arrFloat[471839]);
				//}
				//else
				//if (ii >= 2)
				//	printf("setting key2\n");
				redis.set(keyID, std::string_view(reinterpret_cast<const char*>(arrFloat), nAlloc * sizeof(float)));
				//if (ii >= 2)
				//	printf("done\n");
				pos++;
			}
		}
		free(arrFloat);

		//printf("ii %d nBands %d nTimePeriods %d\n", ii, nBands, model.weather[ii].nTimeIntervals);
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
		errlog("%s nCols/Rows %d %d sizes %.3lf %.3lf minXY %.3lf %.3lf nTime/forecast %d %d\n",
			model.weather[ii].weatherFileTypeName, model.weather[ii].nCols, model.weather[ii].nRows,
			model.weather[ii].size_col, model.weather[ii].size_row,
			model.weather[ii].minX, model.weather[ii].minY, model.weather[ii].nTimeIntervals,
			model.weather[ii].nTimeIntervals_forecast);

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
			printf("here ii == 2\n");
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
		if (model.weather[ii].errorCode != 0) {
			printf("weather %d errorCode sparad\n", ii);
			model.params.errorCode = model.weather[ii].errorCode;
			printf("\tthe error code is %d\n", model.weather[ii].errorCode);
		}

	}
	printf("redisSetKeys done, just some cleaning up left\n");

	fclose(filcheck);
	printf("cleaning1\n");
	fclose(filCheck2);
	printf("cleaning2\n");

#endif

	if (model.params.errorCode != 0)
		errlog("ERROR! Generation of redis keys failed, errorCode %d\n", model.params.errorCode);
	else
		errlog("Generation of redis keys successful\n");
	printf("cleaning3\n");

	return 0;
}

double evalWeatherDataAlongArc(int arcNr, int startSlutArc, double timeExact, int speedSettingGiven) {
	int i, cNr;
	double windSpeed_x, windSpeed_y, waveDir_y, waveDir_x, dist, tidTot, distNu;
	double stormVarde, uCurrent, vCurrent, deltaTid;
	double currentDirection, currentSpeed, baseGroundSpeed, calmWaterSpeed, uWind, vWind;
	double speedDiffWindWave, rel_windSpeed, rel_windDir, speedOverGround, timeArc;
	double fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux, windDirection, windSpeed2, windSpeed, waveHeight, wavePeriod;
	double rel_waveDir, iceCover, waveDirection, speedDiffWind, speedDiffWave;
	double fixTime = -1, absWindDirDiff = 0;


	cNr = -1;
	if (arcNr >= 0) {
		if (model.arc[arcNr].toLevel < 0 && model.arc[arcNr].fromLevel < 0) {
			cNr = -model.arc[arcNr].toLevel - 1;
		}
	}
	else {
		if (arcNr < -1) {
			cNr = -arcNr - 2;
		}
	}
	if (cNr >= 0) {
		//printf("corridor %d timeStart %.2lf\n", cNr, timeExact);
		if (model.network.channel[cNr].intArrivalTime_h >= 0) {
			//printf("corridor %d intArrivalTime_h %d\n", cNr, model.network.channel[cNr].intArrivalTime_h);
			timeExact = delayTimeToStartTimeDay(timeExact, model.network.channel[cNr].intArrivalTime_h);
		}
		timeExact += model.network.channel[cNr].waitingTime; // .intWaitingTime;
		//printf("corridor %d time after intWaitingTime %.2lf timeThroughChannel %.3lf\n", cNr, timeExact, model.network.channel[cNr].timeThroughChannel);
		fixTime = model.network.channel[cNr].timeThroughChannel;
	}

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
	//model.functions.valuesNow.bowSlamming_max = 0;
	//model.functions.valuesNow.greenWater_max = 0;
	//model.functions.valuesNow.dynamicStability_max = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0;
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;

	model.functions.valuesNow.relWindDir = 0;
	model.functions.valuesNow.relWaveDir = 0;


	dist = 0;

	if (startSlutArc == 0) {
		tidTot = timeExact;
		if (USE_ARC_TIME_EXACT == 1) {
			if (arcNr >= 0)
				model.functions.valuesNow.deltaArcStart = model.arc[arcNr].fromTime * model.params.tIndexGerH - tidTot;
			else
				model.functions.valuesNow.deltaArcStart = 0;
		}

		if (fixTime > 0) {
			calmWaterSpeed = model.network.channel[cNr].distance_km / fixTime;
			//printf("\narcNr %d levels %d %d pointPos %d %d cNr %d dist %.3lf arcDist %.3lf fixTime %.3lf\n", arcNr, 
			//	model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, 
			//	model.arc[arcNr].toPointNr, cNr,
			//	model.network.channel[cNr].distance_km, model.arc[arcNr].distance, fixTime);
		}
		else {
			if (arcNr >= 0)
				calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
			else {
				if (speedSettingGiven >= 0)
					calmWaterSpeed = eval_calmWaterSpeed(speedSettingGiven, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				else
					calmWaterSpeed = model.params.preferredSpeed_calmWater;
			}
		}
		if (printGlobal == 1) {
			if (arcNr >= 0)
				printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, model.arc[arcNr].speedSetting, calmWaterSpeed,
					model.weatherFunctions.nCheckPoints);
			else
				printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, -1, calmWaterSpeed,
					model.weatherFunctions.nCheckPoints);
		}

		for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
			distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
			dist += distNu;

			//uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
			//vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

			stormVarde = getStormValue((int)(tidTot + model.functions.valuesNow.deltaArcStart), model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i]);// model.weatherFunctions.point[i]);
			if (stormVarde > model.functions.valuesNow.worstStormValue) {
				if (stormVarde > model.arc[arcNr].safetyHurricane)
					stormVarde = model.arc[arcNr].safetyHurricane; // to not create a high cost compared to initial arc generation
				model.functions.valuesNow.worstStormValue = stormVarde;
			}

			if (fixTime <= 0) {
				getAllVariableValues(i, tidTot + model.functions.valuesNow.deltaArcStart);

				uCurrent = model.functions.varValue[model.functions.pos_current_u]; // getVariableValue(model.functions.pos_current_u, i, tidTot);
				vCurrent = model.functions.varValue[model.functions.pos_current_v]; // getVariableValue(model.functions.pos_current_v, i, tidTot);

				if (uCurrent < 1000 && vCurrent < 1000) {
					currentDirection = ApproxAtan2(vCurrent, uCurrent);
					currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
					//if (tidTot >= model.weather[model.functions.pos_current_u].tidpHistoricalWeather)
					//	currentSpeed *= model.params.historicDataFactor_current;
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


				uWind = model.functions.varValue[model.functions.pos_wind_u]; // getVariableValue(model.functions.pos_wind_u, i, tidTot);
				vWind = model.functions.varValue[model.functions.pos_wind_v]; // getVariableValue(model.functions.pos_wind_v, i, tidTot);
				if (uWind < 1000 && vWind < 1000) {
					windDirection = ApproxAtan2(vWind, uWind);
					windSpeed2 = uWind * uWind + vWind * vWind;
					windSpeed = sqrt(windSpeed2);
					//if (tidTot >= model.weather[model.functions.pos_wind_u].tidpHistoricalWeather)
					//	windSpeed *= model.params.historicDataFactor_windSpeed;

					rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
						windDirection, windSpeed, &rel_windDir);
#ifdef NAZANIN_SAFETY
					absWindDirDiff = eval_absWindDirDiff(model.weatherFunctions.vesselBearing[i], windDirection);
#endif
					if (printGlobal == 1) {
						printf("checkP %d vWind %.3lf uWind %.3lf, windDirection %.3lf windSpeed %.3lf rel_windSpeed %.3lf rel_windDir %.3lf\n", i, vWind,
							uWind, windDirection, windSpeed, rel_windSpeed, rel_windDir);
					}
					//model.functions.valuesNow.worstStabilityValue += distNu * rel_windSpeed / 10000.0;
				}
				else {
					windSpeed = 0;
					windDirection = 0;
					rel_windDir = 0;
					rel_windSpeed = baseGroundSpeed;
				}

				waveHeight = model.functions.varValue[model.functions.pos_waveHeight]; // getVariableValue(model.functions.pos_waveHeight, i, tidTot);
				if (waveHeight > 100)
					waveHeight = 0;
				else {
					//if (tidTot >= model.weather[model.functions.pos_waveHeight].tidpHistoricalWeather)
					//	waveHeight *= model.params.historicDataFactor_waveHeight;
				}
				wavePeriod = model.functions.varValue[model.functions.pos_wavePeriod]; // getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
				if (wavePeriod > 1000)
					wavePeriod = 0;
				waveDirection = model.functions.varValue[model.functions.pos_waveDirection]; // getVariableValue(model.functions.pos_waveDirection, i, tidTot);
				if (waveDirection > 1000)
					waveDirection = 0;
				//rel_waveDir = (waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]; // / model.functions.nWaveDir;
				//rel_waveDir = M_PI - ((waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;
				rel_waveDir = M_PI + ((270 - waveDirection) * M_PI / 180 - model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;

				if (rel_waveDir < 0)
					rel_waveDir = -rel_waveDir;
				if (rel_waveDir >= 2 * M_PI)
					rel_waveDir -= 2 * M_PI;
				if (rel_waveDir > M_PI)
					rel_waveDir = 2 * M_PI - rel_waveDir;
				if (printGlobal == 1) {
					printf("checkP %d waveDirection %.3lf rel_waveDir %.3lf waveHeight %.3lf wavePeriod %.3lf\n", i, waveDirection, rel_waveDir, waveHeight, wavePeriod);
				}

				//printGlobal = 1;
				// speedDiffWind = lookup_speedDiffWindTable(calmWaterSpeed, rel_windSpeed, rel_windDir);
				speedDiffWind = lookup_speedDiffWindTable(baseGroundSpeed, rel_windSpeed, rel_windDir);
				speedDiffWave = lookup_speedDiffWaveTable(calmWaterSpeed, waveHeight, wavePeriod, rel_waveDir);
				speedDiffWindWave = speedDiffWind + speedDiffWave;
				// speedOverGround = baseGroundSpeed * model.params.knots_to_km - speedDiffWindWave; // in km/h
				if (model.delay.nYears > 0 && delayVersion >= 4)
					speedOverGround = calmWaterSpeed - speedDiffWindWave; // in km/h
				else
					speedOverGround = baseGroundSpeed - speedDiffWindWave; // in km/h

				if (speedOverGround > calmWaterSpeed * 2)
					speedOverGround = calmWaterSpeed * 2;
				if (speedOverGround < calmWaterSpeed / 2)
					speedOverGround = calmWaterSpeed / 2;
				timeArc = distNu / speedOverGround; // in hours
				//errlog("uWind %.3lf vWind %.3lf\n", uWind, vWind);

				//int tidInt = (int)(tidTot * model.weather_inv_timeIntervall_h);
				//if (tidInt > model.weather_nTimeIntervals_maxValue)
				//	tidInt = model.weather_nTimeIntervals_maxValue;
				//errlog("i %d tidTot %.3lf tidInt %d tidPos %d pos_latLon %d u0 %.3lf pos2 %d v0 %.3lf\n", i,
				//	tidTot, tidInt, model.weather[0].timeIntervalIndex[tidInt], model.weatherFunctions.checkPoint[i].pos_latLon[0],
				//	model.weather[0].valueCell[model.weather[0].timeIntervalIndex[tidInt]][model.weatherFunctions.checkPoint[i].pos_latLon[0]],
				//	model.weatherFunctions.checkPoint[i].pos_latLon[1],
				//	model.weather[1].valueCell[model.weather[1].timeIntervalIndex[tidInt]][model.weatherFunctions.checkPoint[i].pos_latLon[1]]);
				//errlog("arcNr -1 %d calmWaterSpeed %.3lf bearing %.3lf, currDir %.3lf currSpeed %.3lf baseGroundSpeed %.3lf"
				//		" rel_windSpeed %.3lf rel_windDir %.3lf speedDiffWind %.3lf waveHeight %.3lf"
				//	" wavePeriod %.3lf rel_waveDir %.3lf speedDiffWave %.3lf speedOverGround %.3lf distNu %.3lf timeArc %.3lf\n",
				//	i, calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
				//	currentDirection, currentSpeed, baseGroundSpeed, rel_windSpeed, rel_windDir, speedDiffWind, waveHeight, wavePeriod,
				//	rel_waveDir, speedDiffWave, speedOverGround, distNu, timeArc);

				//errlog("evalWeatherDataAlongArc arcNr %d i %d calmWaterSpeed %.3lf baseGroundSpeed %.3lf speedDiffWind %.3lf wave %.3lf speedOverGround %.3lf distNu %.3lf timeArc %.3lf\n",
				//	arcNr, i, calmWaterSpeed, baseGroundSpeed, speedDiffWind, speedDiffWave, speedOverGround, distNu, timeArc);

				//printf("arcNr %d i %d dist %.2lf speedSetting %d bearing %.3lf calmWaterSpeed %.2lf worstStormValue %.2lf vesselBearing %.4lf currDir %.2lf\n"
				//	"currSpeed %.2lf uCurr %.2lf vCurr %.2lf baseGroundSpeed %.4lf uWind %.4lf vWind %.4lf rel_windSpeed %.4lf rel_windDir %.4lf\n"
				//	"waveHeight %.4lf wavePeriod %.4lf waveDir %.4lf rel_waveDir %.4lf speedDiffWind %.4lf speedDiffWave %.4lf speedOverGround %.4lf timeArc %.2lf\n", arcNr, i,
				//	dist, model.arc[arcNr].speedSetting, model.weatherFunctions.vesselBearing[i], calmWaterSpeed, model.functions.valuesNow.worstStormValue, model.weatherFunctions.vesselBearing[i],
				//	currentDirection, currentSpeed, uCurrent, vCurrent, baseGroundSpeed, uWind, vWind, rel_windSpeed, rel_windDir, waveHeight, wavePeriod,
				//	waveDirection, rel_waveDir, speedDiffWind, speedDiffWave, speedOverGround, timeArc);

				//fuelConsumption_main = eval_fuelConsumption_main(model.arc[arcNr].speedSetting);
				//fuelConsumption_aux = eval_fuelConsumption_aux(model.arc[arcNr].speedSetting);
				//fuelUsage_main += fuelConsumption_main * timeArc;
				//fuelUsage_aux += fuelConsumption_aux * timeArc;

				if (printGlobal == 1) {
					printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf\n",
						speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu);
				}

				//model.functions.valuesNow.forecastType += identifyForecastType(tidTot) * timeArc;

				tidTot += timeArc;

				iceCover = model.functions.varValue[model.functions.pos_iceThickness]; // getVariableValue(model.functions.pos_iceThickness, i, tidTot);
				if (iceCover > 1000)
					iceCover = 0;
				if (iceCover > model.functions.valuesNow.iceCover_max)
					model.functions.valuesNow.iceCover_max = iceCover;

#ifdef NAZANIN_SAFETY
				eval_safety_nazanin(speedOverGround, (calmWaterSpeed) - speedDiffWindWave, windSpeed, absWindDirDiff, waveHeight, wavePeriod, rel_waveDir, iceCover);
#else
				eval_safety(iceCover);
#endif
				//if (model.functions.valuesNow.bowSlam > model.functions.valuesNow.bowSlamming_max)
				//	model.functions.valuesNow.bowSlamming_max = model.functions.valuesNow.bowSlam;
				//if (model.functions.valuesNow.greenWater > model.functions.valuesNow.greenWater_max)
				//	model.functions.valuesNow.greenWater_max = model.functions.valuesNow.greenWater;
				//if (model.functions.valuesNow.dynamicStability > model.functions.valuesNow.dynamicStability_max)
				//	model.functions.valuesNow.dynamicStability_max = model.functions.valuesNow.dynamicStability;

				model.functions.valuesNow.current += timeArc * (baseGroundSpeed - calmWaterSpeed);
				model.functions.valuesNow.windSpeed += timeArc * rel_windSpeed; // windSpeed;
				model.functions.valuesNow.waveHeight += timeArc * waveHeight;
				model.functions.valuesNow.wavePeriod += timeArc * wavePeriod;
				//if (printGlobal == 1)
				//	printf("wavePeriod %.3lf timeArc %.3lf tidTot %.3lf tot %.3lf\n", wavePeriod, timeArc, tidTot- timeExact, model.functions.valuesNow.wavePeriod);

				//printf("arcNr %d pos %d baseGroundSpeed %.2lf calmWaterSpeed %.2lf timeArc %.2lf currentAcc %.2lf\n", arcNr, i,
				//	baseGroundSpeed, calmWaterSpeed, timeArc, model.functions.valuesNow.current);
				//windSpeed_x += timeArc * windSpeed * cos(rel_windDir);
				//windSpeed_y += timeArc * windSpeed * sin(rel_windDir);
				windSpeed_x += timeArc * rel_windSpeed * lookUpCos(rel_windDir);
				windSpeed_y += timeArc * rel_windSpeed * lookUpSin(rel_windDir);

				waveDir_x += timeArc * waveHeight * lookUpCos(rel_waveDir);
				waveDir_y += timeArc * waveHeight * lookUpSin(rel_waveDir);
			}
			else {
				// channel with fix speed....
				timeArc = distNu / calmWaterSpeed; // in hours
				tidTot += timeArc;

			}
		}

		deltaTid = tidTot - timeExact;
		if (deltaTid > 0) {
			model.functions.valuesNow.current /= deltaTid;
			model.functions.valuesNow.windSpeed /= deltaTid;
			model.functions.valuesNow.waveHeight /= deltaTid;
			//if (model.functions.valuesNow.waveHeight > 3.3)
			//	model.functions.valuesNow.waveHeight = model.functions.valuesNow.waveHeight;
			model.functions.valuesNow.wavePeriod /= deltaTid;
			//if (printGlobal == 1)
			//	printf("wavePeriod %.3lf timeArc %.3lf\n", model.functions.valuesNow.wavePeriod, deltaTid);
			model.functions.valuesNow.forecastType /= (deltaTid * 8);
		}
		model.functions.valuesNow.relWindDir = ApproxAtan2(windSpeed_y, windSpeed_x) * 180.0 / M_PI;
		if (model.functions.valuesNow.relWindDir < 0)
			model.functions.valuesNow.relWindDir = -model.functions.valuesNow.relWindDir;
		//printf("wind_y %.2lf wind_x %.2lf relWindDir %.2lf\n",
		//	windSpeed_y, windSpeed_x, model.functions.valuesNow.relWindDir);
		model.functions.valuesNow.relWaveDir = ApproxAtan2(waveDir_y, waveDir_x) * 180.0 / M_PI;
		if (model.functions.valuesNow.relWaveDir < 0)
			model.functions.valuesNow.relWaveDir = -model.functions.valuesNow.relWaveDir;
	}
	if (arcNr == 3249)
		errlog("arcNr %d startTime %.3lf endTime %.3lf\n", arcNr, timeExact, tidTot);
	return tidTot;
}

double load_eta(json data) {
	std::string tidpkt = data;
	if (tidpkt == "null")
		return -1; // no eta given
	long long UTCsec;
	UTCsec = make_gmtime_fromDateTimeString(tidpkt);
	double eta_h = (UTCsec - model.params.UTC_secondsStart) / 3600.0;
	if (eta_h < 0) {
		errlog("ERROR! eta %s is earlier than the start of the route, I set it to the same.\n", tidpkt.c_str());
		eta_h = 0.0;
	}
	else
		errlog("ETA is %.2lf hours after starting time\n", eta_h);
	return eta_h;
}

int readCommercialData(json data, strParams* params) {

	if (!data["assigned_speed"].is_null())
		params->commercialSpeed = data["assigned_speed"];
	else
		params->commercialSpeed = -1;

	if (!data["assigned_consumption"].is_null())
		params->commercialFuel = data["assigned_consumption"];
	else
		params->commercialFuel = -1;

	if (!data["allowable_variation"].is_null())
		params->commercialAllowedVariation = data["allowable_variation"];
	else
		params->commercialAllowedVariation = 0;

	if (params->commercialSpeed <= 0 && params->commercialFuel <= 0) {
		if (params->commercialAllowedVariation > 0.001)
			errlog("ERROR! commercial allowable_variation is %.3lf but both assigned_speed and assigned_consumption is defined as not active\n",
				params->commercialAllowedVariation);
		printf("commercial opt not used\n");
		params->commercialAllowedVariation = -1;
	}
	else {
		printf("commercial opt used\n");
		if (params->commercialSpeed > 0 && params->commercialFuel > 0) {
			errlog("ERROR! both commercial assigned_speed %.2lf and assigned_consumption %.2lf. Only one of them can be used, I use speed\n",
				params->commercialSpeed, params->commercialFuel);
			params->commercialFuel = -1;
		}
		if (params->commercialAllowedVariation < 0.001) {
			if (params->commercialAllowedVariation < 0) {
				errlog("ERROR! commercial assigned_speed %.2lf and assigned_consumption %.2lf but allowable_variation is %lf. I set it to 0\n",
					params->commercialSpeed, params->commercialFuel, params->commercialAllowedVariation);
				params->commercialAllowedVariation = 0.0;
			}
			else
				errlog("OBS! commercial assigned_speed %.2lf and assigned_consumption %.2lf but allowable_variation is %lf.\n",
					params->commercialSpeed, params->commercialFuel, params->commercialAllowedVariation);
		}
	}


	return 0;
}

int analyzePreferredPath_longitude() {
	int i, dist;

	//model.preferredPath.threeSixty = 0;
	//errlog("ERROR! Add code here\n");
	//for (i = 1; i < model.preferredPath.nPoints; i++) {
	//	dist = model.preferredPath.point_x[i] - model.preferredPath.point_x[i - 1];
	//	if (dist > 179 || dist < -179)
	//		model.preferredPath.threeSixty = 1;
	//}

	if (model.preferredPath.minX < -180) {
		model.preferredPath.minX += 360;
		model.preferredPath.maxX += 360;
	}

	return 0;
}

int getCheckGeometry(json data, int cNr, int doGeomtryCheck = 1) {
	json dataIt, dataIt2;
	int nCoords = 0, i2, pos0, pos1, i;
	//errlog("ERROR! OBS! Change channel xCoords to same as prefered path in getCheckGeometry\n");

	if (data["coordinates"].is_null()) {
		errlog("ERROR! Corridor does not have tag coordinates in its geometry. I skip it\n");
		return 0;
	}
	if (data["type"].is_null()) {
		errlog("ERROR! Corridor does not have tag type in its geometry. I skip it\n");
		return 0;
	}
	std::string geoType = data["type"];
	if (geoType != "LineString" && geoType != "MultiLineString") {
		errlog("ERROR! Geometry type of corridor path must be LineString or MultiLineString (but it is %s). I skip this one!\n", geoType.c_str());
		printf("ERROR! Geometry type of corridor path must be LineString or MultiLineString (but it is %s). I skip this one!\n", geoType.c_str());
		return 0;
	}

	// read the coordinates
	json dataCoord = data["coordinates"];

	if (geoType == "MultiLineString") {
		for (auto it = dataCoord.begin(); it != dataCoord.end(); ++it) {
			dataIt = it.value();
			if (dataIt.size() > model.network.nAllocCoords) {
				model.network.nAllocCoords = dataIt.size() + 10;
				model.network.xCoord = (double*)realloc(model.network.xCoord, model.network.nAllocCoords * sizeof(double));
				model.network.yCoord = (double*)realloc(model.network.yCoord, model.network.nAllocCoords * sizeof(double));
			}
			for (auto it2 = dataIt.begin(); it2 != dataIt.end(); ++it2) {
				dataIt2 = it2.value();
				i2 = 0;
				for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
					if (i2 == 0) {
						model.network.xCoord[nCoords] = it3.value();
					}
					else if (i2 == 1)
						model.network.yCoord[nCoords] = it3.value();
					i2++;
				}
				nCoords++;
			}
		}
	}
	else {
		if (dataCoord.size() > model.network.nAllocCoords) {
			model.network.nAllocCoords = dataCoord.size() + 10;
			model.network.xCoord = (double*)realloc(model.network.xCoord, model.network.nAllocCoords * sizeof(double));
			model.network.yCoord = (double*)realloc(model.network.yCoord, model.network.nAllocCoords * sizeof(double));
		}
		for (auto it = dataCoord.begin(); it != dataCoord.end(); ++it) {
			dataIt2 = it.value();
			i2 = 0;
			for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
				if (i2 == 0) {
					model.network.xCoord[nCoords] = it3.value();
					if (model.network.xCoord[nCoords] < -180)
						model.network.xCoord[nCoords] += 360;
					if (model.network.xCoord[nCoords] > 180)
						model.network.xCoord[nCoords] -= 360;
				}
				else if (i2 == 1) {
					model.network.yCoord[nCoords] = it3.value();
				}
				i2++;
			}
			nCoords++;
		}
	}

	if (doGeomtryCheck == 1) {
		//printf("corridor from %.3lf %.3lf to %.3lf %.3lf\n", model.network.xCoord[0], model.network.yCoord[0],
		//	model.network.xCoord[nCoords - 1], model.network.yCoord[nCoords - 1]);
		// check if start and end point of the channel is in preferred path rectangle, no => skip
		if (eval_coordWithinBoundingBox(model.network.xCoord[0], model.network.yCoord[0]) == 0)
			return 0;
		if (eval_coordWithinBoundingBox(model.network.xCoord[nCoords - 1], model.network.yCoord[nCoords - 1]) == 0)
			return 0;

		// check if start and end point of the channel is in preferred path's allowed area, no => skip
		pos0 = getBastPhysLevelToConnectToChannel(0, cNr);
		if (pos0 < 0)
			return 0;
		pos1 = getBastPhysLevelToConnectToChannel(1, cNr);
		if (pos1 < 0)
			return 0;

		// validate start and end node as valid in feasible network
		if (check_isCoordFeasiblePhysicalMap(model.network.yCoord[0],
			model.network.xCoord[0]) == 0) {
			errlog("ERROR! channel starts at a position lat/lon %.3lf %.3lf that is not allowed in the feasible map so we can never use it\n",
				model.network.yCoord[0], model.network.xCoord[0]);
			return 0;
		}
		if (check_isCoordFeasiblePhysicalMap(model.network.yCoord[nCoords - 1],
			model.network.xCoord[nCoords - 1]) == 0) {
			errlog("ERROR! channel ends at a position lat/lon %.3lf %.3lf that is not allowed in the feasible map so we can never use it\n",
				model.network.yCoord[nCoords - 1], model.network.xCoord[nCoords - 1]);
			return 0;
		}
	}
	else {
		pos0 = 0;
		pos1 = model.network.nPhysicalLevels - 1;
	}

	// errlog("ERRRO! Add use of the below params when using channels everywhere\n");
	if (pos0 - 2 < 0)
		model.network.channel[cNr].earliestStartLevel = 0;
	else
		model.network.channel[cNr].earliestStartLevel = pos0 - 2;
	if (pos1 + 2 >= model.network.nPhysicalLevels)
		model.network.channel[cNr].latestEndLevel = model.network.nPhysicalLevels - 1;
	else
		model.network.channel[cNr].latestEndLevel = pos1 + 2;

	// save the geometry to the channel including distances...
	model.network.channel[cNr].point = (spherical::Point*)malloc2(nCoords * sizeof(spherical::Point));
	//model.network.channel[cNr].allowedPoint = (int*)malloc2(nCoords * sizeof(int));
	model.network.channel[cNr].point_y = (double*)malloc2(nCoords * sizeof(double));
	model.network.channel[cNr].point_x = (double*)malloc2(nCoords * sizeof(double));
	model.network.channel[cNr].distanceFromStart = (double*)malloc2(nCoords * sizeof(double));
	initBoundingBox(&(model.network.channel[cNr].boundingBox));
	for (i = 0; i < nCoords; i++) {
		//printf("i %d xy %.3lf %.3lf\n", i, model.network.xCoord[i], model.network.yCoord[i]);
		model.network.channel[cNr].point[i] = spherical::Point(model.network.yCoord[i], model.network.xCoord[i]);
		model.network.channel[cNr].point_y[i] = model.network.yCoord[i];
		model.network.channel[cNr].point_x[i] = model.network.xCoord[i];
		updateBoundingBoxWithCoord(&(model.network.channel[cNr].boundingBox), model.network.channel[cNr].point_y[i],
			model.network.channel[cNr].point_x[i]);
		//model.network.channel[cNr].allowedPoint[i] = 1;
		if (i == 0)
			model.network.channel[cNr].distanceFromStart[i] = 0;
		else
			model.network.channel[cNr].distanceFromStart[i] = model.network.channel[cNr].distanceFromStart[i - 1] +
			model.network.channel[cNr].point[i - 1].distanceTo(model.network.channel[cNr].point[i]) / 1000.0;
	}
	model.network.channel[cNr].nPoints = i;
	model.network.channel[cNr].nOutNodes = 0;
	if (model.network.nMaxNodesInPath < i)
		model.network.nMaxNodesInPath = i;
	model.network.channel[cNr].distance_km = model.network.channel[cNr].distanceFromStart[model.network.channel[cNr].nPoints - 1];

	return 1;
}

double timeToHoursSinceMidnight(std::string namn) {
	int valNu, typ = 0, hour = 0, minut = 0;
	double tid;
	for (int i = 0; i < namn.size(); i++) {
		if (namn[i] == ':')
			typ = 1;
		else {
			valNu = namn[i] - '0';
			if (typ == 0)
				hour = hour * 10 + valNu;
			else
				minut = minut * 10 + valNu;
		}
	}
	tid = hour + minut / 60.0;

	return tid;
}

int loadChannelsFromInfile(json data)
{ // not used
	double dist;
	int useChannel, pos, pos1;
	model.network.nChannels = 0;
	model.network.nMaxNodesInPath = 2;

	if (data["features"].is_null()) {
		errlog("ERROR! No features in corridors in input file. I use no corridors\n");
		return 0;
	}



	model.network.nAllocChannels = 10;
	model.network.channel = (strChannel*)malloc2(model.network.nAllocChannels * sizeof(strChannel));

	model.network.nAllocCoords = 500;
	model.network.xCoord = (double*)malloc2(model.network.nAllocCoords * sizeof(double));
	model.network.yCoord = (double*)malloc2(model.network.nAllocCoords * sizeof(double));

	std::ifstream fil;
	char* namn;
	std::string nameTable;
	namn = (char*)malloc2(256 * sizeof(char));
	errlog("Loading corridors\n");

	json data2 = data["features"];
	pos = 0;
	for (auto it = data2.begin(); it != data2.end(); ++it) {
		json dataTable = it.value();

		if (model.network.nChannels >= model.network.nAllocChannels) {
			model.network.nAllocChannels += 10;
			model.network.channel = (strChannel*)realloc(model.network.channel, model.network.nAllocChannels * sizeof(strChannel));
		}

		if (!dataTable["geometry"].is_null()) {
			useChannel = getCheckGeometry(dataTable["geometry"], pos, 0);
			if (useChannel == 0) {
				errlog("ERROR! I skip a corridor since it is not within right geometry area\n");
				continue;
			}
		}
		else {
			errlog("ERROR! Skips a feature in corridors_predefined.json since it has no tag 'geometry'\n");
			continue;
		}
		if (!dataTable["properties"].is_null()) {
			json dataT = dataTable["properties"];
			//if (!dataT["polygon_start"].is_null())
			//	model.network.channel[pos].nPolygonPoints[0] = loadPolygonToChannel(dataT["polygon_start"], pos, 0);
			//else {
			//	model.network.channel[pos].nPolygonPoints[0] = 0;
			//}
			//if (!dataT["polygon_end"].is_null())
			//	model.network.channel[pos].nPolygonPoints[1] = loadPolygonToChannel(dataT["polygon_end"], pos, 1);
			//else {
			//	model.network.channel[pos].nPolygonPoints[1] = 0;
			//}
			if (!dataT["pilot_cost"].is_null())
				model.network.channel[pos].extraCostChannel = dataT["pilot_cost"];
			else {
				errlog("OBS! No 'pilot_cost' given for a corridor. I set it to 0\n");
				model.network.channel[pos].extraCostChannel = 0.0;
			}
			model.network.channel[pos].kvotCost = 1.0;

			if (!dataT["ID"].is_null()) {
				std::string ID = dataT["ID"];
				model.network.channel[pos].ID = str_alloc_cpy(ID.c_str());
			}
			else {
				errlog("OBS! No 'ID' given for a corridor. I set it to corridorID\n");
				model.network.channel[pos].ID = str_alloc_cpy("corridorID");
			}
			if (!dataT["corridor_time_h"].is_null())
				model.network.channel[pos].timeThroughChannel = dataT["corridor_time_h"];
			else {
				errlog("OBS! No 'corridor_time_h' given for a corridor. I set it to -1, speed to be determined by the optimizer\n");
				model.network.channel[pos].timeThroughChannel = -1.0;
			}
			if (!dataT["waiting_time_h"].is_null())
				model.network.channel[pos].waitingTime = dataT["waiting_time_h"];
			else {
				errlog("OBS! No 'waiting_time_h' given for a corridor. I set it to 0\n");
				model.network.channel[pos].waitingTime = 0.0;
			}
			if (!dataT["waiting_consumption_main"].is_null())
				model.network.channel[pos].waiting_consumption_main = dataT["waiting_consumption_main"];
			else {
				errlog("OBS! No 'waiting_consumption_main' given for a corridor. I set it to 0\n");
				model.network.channel[pos].waiting_consumption_main = 0.0;
			}
			if (!dataT["waiting_consumption_aux"].is_null())
				model.network.channel[pos].waiting_consumption_aux = dataT["waiting_consumption_aux"];
			else {
				errlog("OBS! No 'waiting_consumption_aux' given for a corridor. I set it to 0\n");
				model.network.channel[pos].waiting_consumption_aux = 0.0;
			}
			if (!dataT["distance_km"].is_null())
				model.network.channel[pos].distance_km = dataT["distance_km"];
			else {
				errlog("OBS! No 'distance_km' given for a corridor. I set it to -1, to be calculated by the optimizer\n");
				model.network.channel[pos].distance_km = -1.0;
			}
			if (!dataT["total_consumption"].is_null())
				model.network.channel[pos].totalConsumption = dataT["total_consumption"];
			else {
				errlog("OBS! No 'total_consumption' given for a corridor. I set it to -1, to be calculated by the optimizer\n");
				model.network.channel[pos].totalConsumption = -1.0;
			}
			if (!dataT["Eca_area"].is_null())
				model.network.channel[pos].ECA_type = dataT["Eca_area"];
			else {
				errlog("OBS! No 'Eca_area' given for a corridor. I set it to -1, to be determined by the ECA map\n");
				model.network.channel[pos].ECA_type = -1;
			}
			if (!dataT["followExactly"].is_null())
				model.network.channel[pos].followExactly = dataT["followExactly"];
			else
				model.network.channel[pos].followExactly = 0;

			if (model.network.channel[pos].timeThroughChannel >= 0 || model.network.channel[pos].totalConsumption >= 0) {
				if (model.network.channel[pos].timeThroughChannel < 0) {
					errlog("ERROR? total_consumption for a corridor is given but not corridor_time_h.\n");
					// model.network.channel[pos].totalConsumption = -1;
				}
				if (model.network.channel[pos].totalConsumption < 0) {
					errlog("ERROR! corridor_time_h for a corridor is given but not total_consumption. I set it to be calculated by OptiNav\n");
					// model.network.channel[pos].totalConsumption = 0.8;
				}

			}

			if (!dataT["transit_arrival_time"].is_null()) {
				if (dataT["transit arrival_time"] == "-1:00")
					model.network.channel[pos].arrivalTime_h = -1;
				else
					model.network.channel[pos].arrivalTime_h = timeToHoursSinceMidnight(dataT["transit_arrival_time"]);
			}
			else {
				errlog("OBS! No 'transit_arrival_time' given for a corridor. I set it to -1, no required arrival time\n");
				model.network.channel[pos].arrivalTime_h = -1.0;
			}
			if (model.network.channel[pos].arrivalTime_h > -0.01) {
				model.network.channel[pos].intArrivalTime_h = (int)model.network.channel[pos].arrivalTime_h;
				if (model.network.channel[pos].intArrivalTime_h < model.network.channel[pos].arrivalTime_h - 0.5)
					(model.network.channel[pos].intArrivalTime_h)++;
			}
			else
				model.network.channel[pos].intArrivalTime_h = -1;
			//model.network.channel[pos].intWaitingTime = (int)model.network.channel[pos].waitingTime;
			//if (model.network.channel[pos].intWaitingTime < model.network.channel[pos].waitingTime - 0.5)
			//	(model.network.channel[pos].intWaitingTime)++;
			model.network.channel[pos].factorDelayedPrefPathAfter = 0;
			model.network.channel[pos].factorDelayedPrefPathDuring = 0;

			model.network.channel[pos].midTimeArrive = 0;
			model.network.channel[pos].midTimeFinish = 0;
		}
		else {
			errlog("ERROR! Skips a feature in corridors_predefined.json since it has no tag 'properties'\n");
			continue;
		}

		dist = 0;
		for (int i = 0; i < model.network.channel[pos].nPoints - 1; i++) {
			dist += model.network.channel[pos].point[i].distanceTo(model.network.channel[pos].point[i + 1]);
			//printf("channel %d pos %d coords %.2lf %.2lf distNu %.2lf\n", pos, i + 1, 
			//	model.network.channel[pos].point[i + 1].longitude().degrees(), model.network.channel[pos].point[i + 1].latitude().degrees(), dist / 1000.0);
		}
		dist /= 1000.0;
		if (abs(dist - model.network.channel[pos].distance_km) > 0.01)
			errlog("OBS! Distance of used channel %d is wrong, given %.3lf but is %.3lf. I use the later one xy %.3lf %.3lf to %.3lf %.3lf.\n",
				pos, model.network.channel[pos].distance_km, dist,
				model.network.channel[pos].point_x[0], model.network.channel[pos].point_y[0],
				model.network.channel[pos].point_x[model.network.channel[pos].nPoints - 1],
				model.network.channel[pos].point_y[model.network.channel[pos].nPoints - 1]);
		model.network.channel[pos].distance_km = dist;


		//model.network.channel[pos].nOutNodes = (int*)calloc2(1, sizeof(int));
		//model.network.channel[pos].nArcsToPoint = (int*)calloc2(1, sizeof(int));
		//model.network.channel[pos].outNode = (int**)malloc2(sizeof(int*));
		//model.network.channel[pos].outPolyPoint = (int**)malloc2(sizeof(int*));
		model.network.channel[pos].outLevel = (int*)malloc2(sizeof(int));
		//model.network.channel[pos].outPolyPoint = (int*)malloc2(sizeof(int));
		//model.network.channel[pos].outNode = (int*)malloc2(model.params.nPkterOrto * 2 * sizeof(int));
		//model.network.channel[pos].outLevel = (int*)malloc2(model.params.nPkterOrto * 2 * sizeof(int));

		model.network.channel[pos].nodNr_from_pt = (int**)malloc2(2 * sizeof(int*));
		model.network.channel[pos].nTimeIntervals = (int*)malloc2(2 * sizeof(int));
		model.network.channel[pos].nAllocTimeIntervals = (int*)malloc2(2 * sizeof(int));
		model.network.channel[pos].timeInterval = (int**)malloc2(2 * sizeof(int*));

		for (int i3 = 0; i3 < 2; i3++) { // start och endnod i channel
			model.network.channel[pos].nTimeIntervals[i3] = 0;
			model.network.channel[pos].nAllocTimeIntervals[i3] = 100;
			model.network.channel[pos].timeInterval[i3] = (int*)malloc2(
				model.network.channel[pos].nAllocTimeIntervals[i3] * sizeof(int));
			model.network.channel[pos].nodNr_from_pt[i3] = (int*)malloc2(
				model.network.channel[pos].nAllocTimeIntervals[i3] * sizeof(int));
		}

		model.network.channel[pos].type = 0;
		pos++;
		(model.network.nChannels)++;
	}


	return 0;
}

int loadWeightsFromSeparateFile(strParams* params) {

	std::ifstream fil;
	json data;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s/%s", params->resultPath.c_str(), params->loadWeightsFile);
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		postRequest(std::string(namn) + " does not exist but given as input data to OptiNav.I quit\n", 1);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}
	fil.close();

	loadWeights(params, data);


	free(namn);
	return 0;

}

time_t make_gmtime(strParams* params) {
	struct tm tmBas = { 0 };
	tmBas.tm_isdst = 0;
	tmBas.tm_year = params->startYear - 1900;
	tmBas.tm_mon = params->startMonth_nr - 1; // sep
	tmBas.tm_mday = params->startDay_nr;
	tmBas.tm_hour = params->startHour; // 0;
	tmBas.tm_min = params->startMinute;
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

int loadParams_new(strParams* params)
{
	int i, closestI, nSplit;
	double xValOld, yValOld, last_x = -999, worstDegree, maxWind, diffI, diffNu;
	double fuelMain, fuelAux, xDiff, yDiff, xNu, yNu, dist;
	spherical::Point pNu;

	std::ifstream fil;
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	//sprintf(namn, "%s/input.json", model.params.indataPath.c_str());
	sprintf(namn, "%s", model.params.indataPathName.c_str());
	errlog("trying to open %s\n", namn);
	if (!(check_file_exist(namn))) {
		errlog("%s does not exist. I quit\n", namn);
		printf("%s does not exist. I quit\n", namn);
		postRequest(std::string(namn) + " does not exist but given as input data to OptiNav.I quit\n", 1);
	}
	//printf("opens %s\n", namn);
	fil.open(namn);

	json data, dataGeo, dataGeo2, dataFeature, dataProp, dataGeo3, dataCoord;
	json dataIt, dataIt2;
	int i2, nAlloc = 0, nPointsTot = 0, nPointsNu, pos, posBase;
	double xVal, yVal;
	try {
		fil >> data;
	}
	catch (...) {
		errlog("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		printf("ERROR! json file %s is not valid. Fix it and run OptiNav again.\n", namn);
		postRequest("ERROR! json file " + std::string(namn) + " is not valid.Fix it and run OptiNav again.", 1);
	}

	if (!data["maxDeviationPreferred_km"].is_null())
		params->maxDeviationPreferred_km = data["maxDeviationPreferred_km"];
	params->maxDeviationPreferred2_km = params->maxDeviationPreferred_km * params->maxDeviationPreferred_km;
	if (!data["maxDistStartToCorridorConnect_km"].is_null())
		params->maxDistStartToCorridorConnect = data["maxDistStartToCorridorConnect_km"];
	else
		params->maxDistStartToCorridorConnect = 20.0;

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

	if (!data["SKRIV_UT_NOTHING"].is_null()) {
		SKRIV_UT_NOTHING = data["SKRIV_UT_NOTHING"];
		if (SKRIV_UT_NOTHING < 2)
			reset_errlog();
	}

	params->onboard_currentStatic = 0;
	if (!data["onboard"].is_null()) {
		// loadSet_startDateTime(data["startDateTime", params]);
		params->onboard = data["onboard"];
		if (params->onboard == 2) {
			if (!data["onboard_currentStatic"].is_null()) {
				params->onboard_currentStatic = data["onboard_currentStatic"];
			}
		}
	}
	else
		params->onboard = 0;

	if (!data["wayPointHours"].is_null()) {
		params->wayPointHours = data["wayPointHours"];
	}
	else
		params->wayPointHours = -1;

	if (!data["doSimulation"].is_null()) {
		params->useSimulering = data["doSimulation"];
	}
	else
		params->useSimulering = 0;

	if (params->useSimulering == 1) {
		if (!data["simulation_calmWaterSpeed_kts"].is_null()) {
			params->simulationSpeed_kmh = data["simulation_calmWaterSpeed_kts"];
			params->simulationSpeed_kmh *= 1.852;
		}
		else
			params->simulationSpeed_kmh = -1;
		if (!data["simulation_maxWaveHeight"].is_null())
			params->user_maxWaveHeight = data["simulation_maxWaveHeight"];
		else
			params->user_maxWaveHeight = 1e10;
		if (!data["simulation_maxWindSpeed_kts"].is_null()) {
			params->user_maxWindSpeed_kmh = data["simulation_maxWindSpeed_kts"];
			params->user_maxWindSpeed_kmh *= 1.852;
		}
		else
			params->user_maxWindSpeed_kmh = 1e10;
		//if (params->wayPointHours < 0)
		//	params->wayPointHours = 6; // want fix time way points when doing simulations
	}


	if (model.params.UTC_secondsStart == 0) {
		if (!data["startDateTime"].is_null()) {
			// loadSet_startDateTime(data["startDateTime", params]);
			params->UTC_secondsStart = make_gmtime_fromDateTimeString(data["startDateTime"], params);
		}
		else {
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
			params->UTC_secondsStart = make_gmtime(params);
		}
	}
	else {
		errlog("ERROR! OBS! Starting time taken from file_params.json, not from input file\n");
		//params->UTC_secondsStart = model.params.UTC_secondsStart;
	}
	if (params->startMinute >= 30) {
		params->startHoursSinceMidnight = params->startHour + 1;
		if (params->startHoursSinceMidnight == 24)
			params->startHoursSinceMidnight = 0;
	}
	else
		params->startHoursSinceMidnight = params->startHour;
	//printf("%jd seconds since the epoch began\n", (intmax_t)(params->UTC_secondsStart));
	errlog("Do the planning for the dateTime %s\n", asctime(gmtime(&(params->UTC_secondsStart))));
	printf("Do the planning for the dateTime %s\n", asctime(gmtime(&(params->UTC_secondsStart))));

	if (!data["SEND_POST_REQUEST"].is_null())
		SEND_POST_REQUEST = data["SEND_POST_REQUEST"];

	if (!data["hindCast"].is_null())
		params->hindCast = data["hindCast"];

	if (!data["eta"].is_null())
		params->eta_h = load_eta(data["eta"]);
	else
		params->eta_h = -1;

	if (!data["aim_waypointInterval_h"].is_null())
		params->aim_waypointInterval_h = data["aim_waypointInterval_h"];
	else
		params->aim_waypointInterval_h = -1;

	if (!data["calmWaterSpeedCompare"].is_null()) {
		params->calmWaterSpeedCompare = data["calmWaterSpeedCompare"];
		params->calmWaterSpeedCompare *= params->knots_to_km;
	}
	else
		params->calmWaterSpeedCompare = -1;
	if (!data["fuelCompare"].is_null()) {
		params->fuelCompare = data["fuelCompare"];
	}
	else
		params->fuelCompare = -1;

	if (params->useSimulering == 1) {
		if (params->simulationSpeed_kmh < 0 && params->calmWaterSpeedCompare > 0)
			params->simulationSpeed_kmh = params->calmWaterSpeedCompare;
		if (params->simulationSpeed_kmh > 0 &&
			(params->calmWaterSpeedCompare < 0 && params->fuelCompare < 0))
			params->calmWaterSpeedCompare = params->simulationSpeed_kmh;
	}


	if (!data["max_changeDirection"].is_null()) {
		params->max_changeDirection = data["max_changeDirection"];
		if (params->max_changeDirection == 0) {

			// any use to change nPkterOrto???
		}
	}


	if (params->hindCast != 1 && runAltForecast != -2) {
		if (!data["startHistoricalDataOnly"].is_null())
			model.network.tidp_startHistoricDataOnly = data["startHistoricalDataOnly"];
		else
			model.network.tidp_startHistoricDataOnly = model.params.tidp_startHistoricDataOnly_iter1; // 0;
	}
	else
		model.network.tidp_startHistoricDataOnly = 999999;

	if (!data["commercial"].is_null())
		readCommercialData(data["commercial"], params);
	else {
		params->commercialSpeed = -1;
		params->commercialFuel = -1;
		params->commercialAllowedVariation = -1;
	}

	if (!data["useOptionalExtraNoGoAreas"].is_null()) {
		json dataExtra = data["useOptionalExtraNoGoAreas"];
		model.nExtraNoGoAreas = dataExtra.size();
		model.extraNoGoArea = (strExtraNoGo*)malloc(model.nExtraNoGoAreas * sizeof(strExtraNoGo));
		pos = 0;
		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			std::string namnID = dataNu["extraAreaID"];
			if (!dataNu["active"].is_null()) {
				posBase = dataNu["active"];
				if (posBase != 1)
					continue; // not using this areaID
			}
			model.extraNoGoArea[pos].areaID = str_alloc_cpy(namnID.c_str());
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
		model.nExtraNoGoAreas = 1;
		model.extraNoGoArea = (strExtraNoGo*)malloc(model.nExtraNoGoAreas * sizeof(strExtraNoGo));
		pos = 0;

		for (int ii = 0; ii < 1; ii++) {
			model.extraNoGoArea[pos].areaID = str_alloc_cpy("HRA");
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

	pos = 0;
	if (!data["useOptionalExtraCostAreas"].is_null()) {
		json dataExtra = data["useOptionalExtraCostAreas"];
		model.nExtraCostAreas = dataExtra.size();
		model.extraCostArea = (strExtraNoGo*)malloc((model.nExtraCostAreas + 1) * sizeof(strExtraNoGo));

		model.extraCostArea[pos].areaID = str_alloc_cpy("ECA");
		posBase = findAreaIDpos_inBase(model.extraCostArea[pos].areaID);
		if (posBase < 0) {
			errlog("ERROR! extra cost AreaID %s is not defined in file_paramsFeasibility.json. Add this area as it has to be there. I quit.\n",
				model.extraCostArea[pos].areaID);
			postRequest("ERROR! extra cost AreaID " + std::string(model.extraCostArea[pos].areaID) + " is not defined in file_params.json. Add this area. I quit.", 1);
		}
		model.extraCostArea[pos].posBase = posBase;
		model.extraCostArea[pos].extraCostFactor = 0.0;
		pos++;


		for (auto it = dataExtra.begin(); it != dataExtra.end(); ++it) {
			json dataNu = it.value();
			std::string namnID = dataNu["extraAreaID"];
			model.extraCostArea[pos].areaID = str_alloc_cpy(namnID.c_str());
			posBase = findAreaIDpos_inBase(model.extraCostArea[pos].areaID);
			if (posBase == model.extraCostArea[0].posBase) {
				if (!dataNu["extraCostFactor"].is_null()) {
					model.extraCostArea[0].extraCostFactor = dataNu["extraCostFactor"];
				}
				continue; // since this is ECA and it has already been added, only the cost factor can be changed here
			}
			if (posBase < 0) {
				errlog("ERROR! extra noGoAreaID %s is not defined in file_paramsFeasibility.json. Add this area. I ignore it for now.\n",
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
		model.nExtraCostAreas = 1;
		model.extraCostArea = (strExtraNoGo*)malloc((model.nExtraCostAreas) * sizeof(strExtraNoGo));
		model.extraCostArea[pos].areaID = str_alloc_cpy("ECA");
		posBase = findAreaIDpos_inBase(model.extraCostArea[pos].areaID);
		if (posBase < 0) {
			errlog("ERROR! extra cost AreaID %s is not defined in file_paramsFeasibility.json. Add this area as it has to be there. I quit.\n",
				model.extraCostArea[pos].areaID);
			postRequest("ERROR! extra cost AreaID " + std::string(model.extraCostArea[pos].areaID) + " is not defined in file_params.json. Add this area. I quit.", 1);
		}
		model.extraCostArea[pos].posBase = posBase;
		model.extraCostArea[pos].extraCostFactor = 0.0;
	}


	if (!data["ship_specification"].is_null()) {
		json dataShip = data["ship_specification"];

		if (!dataShip["shipDraft"].is_null()) {
			params->shipDraft = dataShip["shipDraft"];
			errlog("Loaded shipDraft %.3lf\n", params->shipDraft);
		}
		else {
			params->shipDraft = 10.03;
			errlog("ERROR! Load shipDraft, default now %.3lf\n", params->shipDraft);
		}
		if (!dataShip["freeBoard"].is_null()) {
			params->freeBoard2 = dataShip["freeBoard"];
			params->freeBoard2 *= params->freeBoard2;
			errlog("Loaded freeBoard %.3lf\n", sqrt(params->freeBoard2));
		}
		else {
			params->freeBoard2 = 4.39 * 4.39;
			errlog("ERROR! Load freeBoard, default now %.3lf\n", sqrt(params->freeBoard2));
		}
		if (!dataShip["shipLength"].is_null()) {
			params->shipLength = dataShip["shipLength"];
			errlog("Loaded shipLength %.3lf\n", params->shipLength);
		}
		else {
			params->shipLength = 177;
			errlog("ERROR! Load shipLength, default now %.3lf\n", params->shipLength);
		}
		if (!dataShip["iceCoverMaxFree_m"].is_null()) {
			model.functions.iceCoverMaxFree = dataShip["iceCoverMaxFree_m"];
			errlog("Max free ice cover %.3lf\n", model.functions.iceCoverMaxFree);
		}
		else {
			model.functions.iceCoverMaxFree = 0;
			errlog("ERROR! No iceCoverMaxFree_m, I use default %.3lf\n", model.functions.iceCoverMaxFree);
		}

		if (!dataShip["shipSpeedSettings"].is_null()) {
			json dataSpeed = dataShip["shipSpeedSettings"];
			model.functions.nShip_speedSettingsBase = dataSpeed.size();
			model.functions.rpmBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			model.functions.rpmSetting_gerCalmWaterSpeedBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			model.functions.rpmSetting_gerFuelConsumption_mainBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			model.functions.rpmSetting_gerFuelConsumption_auxBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			i = 0;
			for (auto it = dataSpeed.begin(); it != dataSpeed.end(); ++it) {
				json dataSpeed2 = it.value();
				if (!dataSpeed2["rpm"].is_null())
					model.functions.rpmBase[i] = dataSpeed2["rpm"];
				else {
					errlog("ERROR! No rpm in shipSpeedSetting nr %d in input data. I set it to 70 so I can keep loading data\n", i);
					model.functions.rpmBase[i] = 70;
				}
				if (!dataSpeed2["calmWaterSpeed_kts"].is_null())
					model.functions.rpmSetting_gerCalmWaterSpeedBase[i] = (double)(dataSpeed2["calmWaterSpeed_kts"]) * model.params.knots_to_km;
				else {
					errlog("ERROR! No calmWaterSpeed_kts in a shipSpeedSetting in input data. I set it to 11 so I can keep loading data\n");
					model.functions.rpmSetting_gerCalmWaterSpeedBase[i] = 11 * model.params.knots_to_km;
				}
				if (!dataSpeed2["fuelConsumption_main_mpd"].is_null())
					model.functions.rpmSetting_gerFuelConsumption_mainBase[i] = (double)(dataSpeed2["fuelConsumption_main_mpd"]) / 24.0;
				else {
					errlog("ERROR! No fuelConsumption_main_mpd in a shipSpeedSetting in input data. I set it to 11 so I can keep loading data\n");
					model.functions.rpmSetting_gerFuelConsumption_mainBase[i] = 11 / 24.0;
				}

				if (!dataSpeed2["fuelConsumption_aux_mpd"].is_null())
					model.functions.rpmSetting_gerFuelConsumption_auxBase[i] = (double)(dataSpeed2["fuelConsumption_aux_mpd"]) / 24.0;
				else {
					errlog("ERROR! No fuelConsumption_aux_mpd in a shipSpeedSetting in input data. I set it to 2 so I can keep loading data\n");
					model.functions.rpmSetting_gerFuelConsumption_auxBase[i] = 2 / 24.0;
				}
				i++;
			}
		}
		else {
			model.functions.nShip_speedSettingsBase = 1; // 15, 20, 25
			model.functions.rpmSetting_gerCalmWaterSpeedBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			model.functions.rpmSetting_gerFuelConsumption_mainBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			model.functions.rpmSetting_gerFuelConsumption_auxBase = (double*)malloc2(model.functions.nShip_speedSettingsBase * sizeof(double));
			model.functions.rpmBase[0] = 80.0;
			model.functions.rpmSetting_gerCalmWaterSpeedBase[0] = 11 * model.params.knots_to_km;
			model.functions.rpmSetting_gerFuelConsumption_mainBase[0] = 15 / 24.0;
			model.functions.rpmSetting_gerFuelConsumption_auxBase[0] = 2 / 24.0;
		}
		closestI = 0;
		diffI = 0;
		for (i = 0; i < model.functions.nShip_speedSettingsBase; i++) {
			//diffNu = abs(model.functions.rpmBase[i] - 95);
			//if (diffNu < diffI) {
			//	diffI = diffNu;
			//	closestI = i;
			//}
			diffNu = model.functions.rpmBase[i];
			if (diffNu > diffI) {
				diffI = diffNu;
				closestI = i;
			}
		}
		model.functions.speedSetting95MCR_base = closestI;
		if (abs(diffI - 95) > 0.5)
			errlog("OBS! max rpm is taken from %.2lf rpm setting\n", model.functions.rpmBase[closestI]);

		fuelMain = eval_fuelConsumption_both(model.functions.speedSetting95MCR_base, &fuelAux, -1, -100);
		errlog("max rpm gives a calmWaterSpeed of %.2lf knots and a fuel consumption of (main/aux) %.2lf %.2lf ton/hour\n",
			eval_calmWaterSpeed(model.functions.speedSetting95MCR_base, -1, -100) / params->knots_to_km, fuelMain, fuelAux);

		if (!dataShip["tableID_wind"].is_null()) {
			model.functions.windTableID = cleanString(dataShip["tableID_wind"]);
		}
		else {
			errlog("ERROR No windTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No windTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
		if (!dataShip["tableID_wave"].is_null()) {
			model.functions.waveTableID_orig = dataShip["tableID_wave"];
			model.functions.waveTableID = cleanString(dataShip["tableID_wave"]);
		}
		else {
			errlog("ERROR No waveTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No waveTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
		if (!dataShip["tableID_stability"].is_null()) {
			model.functions.stabilityTableID = cleanString(dataShip["tableID_stability"]);
		}
		else {
			errlog("ERROR No stabilityTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No stabilityTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
		/*
#ifdef NAZANIN_SAFETY
		if (!dataShip["tableID_bowSlamming"].is_null()) {
			model.functions.bowSlammingTableID = cleanString(dataShip["tableID_bowSlamming"]);
		}
		else {
			errlog("ERROR No bowSlammingTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No bowSlammingTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
		if (!dataShip["tableID_greenWater"].is_null()) {
			model.functions.greenWaterTableID = cleanString(dataShip["tableID_greenWater"]);
		}
		else {
			errlog("ERROR No greenWaterTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No greenWaterTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
		if (!dataShip["tableID_rolling"].is_null()) {
			model.functions.rollingTableID = cleanString(dataShip["tableID_rolling"]);
		}
		else {
			errlog("ERROR No rollingTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No rollingTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
		if (!dataShip["tableID_surfRiding"].is_null()) {
			model.functions.surfRidingTableID = cleanString(dataShip["tableID_surfRiding"]);
		}
		else {
			errlog("ERROR No surfRidingTableID given in input data in tag 'ship_specification'. It must exist. I quit.\n");
			postRequest("ERROR No surfRidingTableID given in input data in tag 'ship_specification'. It must exist. I quit.", 1);
		}
#endif
*/
	}
	else {
		errlog("ERROR! no ship_specification in input file. It must exist. I quit.\n");
		postRequest("ERROR! no ship_specification in input file. It must exist. I quit.", 1);
	}


	if (!data["preferredPath_followExactOK"].is_null())
		params->preferredPath_followExactOK = data["preferredPath_followExactOK"];
	if (!data["delay_onlySolveSP"].is_null())
		params->delay_onlySolveSP = data["delay_onlySolveSP"];
	else
		params->delay_onlySolveSP = 0;

	if (!data["shipSpeed"].is_null())
		params->shipSpeed_average = data["shipSpeed"];

	if (data["geoData"].is_null()) {
		errlog("ERROR! No geoData tag in input.json. I quit\n");
		postRequest("ERROR! No geoData tag in input data. I quit", 1);
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
				postRequest("ERROR! No properties for a feature in geoData. I skip this one", 0);
				continue; // no properties exists for this one, cannot be a preferred path
			}
			dataProp = dataFeature["properties"];
			std::string namnNu = dataProp["type"];
			if (namnNu != "preferredPath" && namnNu != "optimizedPath") {
				printf("ERROR! Not the name preferredPath of type for a property in geoData. I skip this one\n");
				errlog("ERROR! Not the name preferredPath of type for a property in geoData. I skip this one\n");
				postRequest("ERROR! Not the type 'preferredPath' or 'optimizedPath' for property in geoData. It is '" + namnNu + "'. ", 0);
				continue; //not a preferred path
			}
			dataGeo3 = dataFeature["geometry"];
			if (dataGeo3["type"].is_null()) {
				errlog("ERROR! No type given for the geometry of prefered path. I quit!\n");
				printf("ERROR! No type given for the geometry of prefered path. I quit!\n");
				postRequest("ERROR! No type given for the geometry of prefered path. I quit!", 1);
			}
			std::string geoType = dataGeo3["type"];
			if (geoType != "LineString" && geoType != "MultiLineString") {
				errlog("ERROR! Geometry type of prefered path must be LineString or MultiLineString (but it is %s). I quit!\n", geoType.c_str());
				printf("ERROR! Geometry type of prefered path must be LineString or MultiLineString (but it is %s). I quit!\n", geoType.c_str());
				postRequest("ERROR! Geometry type of prefered path must be LineString or MultiLineString (but it is " + geoType + ").I quit!", 1);
			}

			if (dataGeo3["coordinates"].is_null()) {
				errlog("ERROR! No coordinates given for the prefered path. I quit!\n");
				printf("ERROR! No coordinates given for the prefered path. I quit!\n");
				postRequest("ERROR! No coordinates given for the prefered path. I quit!", 1);
			}
			dataCoord = dataGeo3["coordinates"];

			i = 0;
			xValOld = -999999;
			yValOld = -999999;
			model.preferredPath.startX = -1000;
			if (geoType == "MultiLineString") {
				for (auto it = dataCoord.begin(); it != dataCoord.end(); ++it) {
					dataIt = it.value();
					nPointsNu = (int)dataIt.size();
					if (nAlloc == 0) {
						nAlloc = nPointsNu;
						model.preferredPath.point = (spherical::Point*)malloc2(nAlloc * sizeof(spherical::Point));
						model.preferredPath.point_y = (double*)malloc2(nAlloc * sizeof(double));
						model.preferredPath.point_x = (double*)malloc2(nAlloc * sizeof(double));
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
								if (model.preferredPath.startX < -998)
									model.preferredPath.startX = xVal;
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

							pNu = spherical::Point(yVal, xVal);
							if (nPointsTot > 0) {
								dist = model.preferredPath.point[nPointsTot - 1].distanceTo(pNu) / 1000;
								if (dist > model.params.maxDistBetweenPrefPathPoints) {
									nSplit = (int)(dist / model.params.maxDistBetweenPrefPathPoints);
									if (nPointsTot + nSplit >= nAlloc) {
										nAlloc += 10 + nSplit;
										model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point, nAlloc * sizeof(spherical::Point));
										model.preferredPath.point_y = (double*)realloc(model.preferredPath.point_y, nAlloc * sizeof(double));
										model.preferredPath.point_x = (double*)realloc(model.preferredPath.point_x, nAlloc * sizeof(double));
									}

									xDiff = (xVal - model.preferredPath.point_x[nPointsTot - 1]);
									if (xDiff > 180)
										xDiff = 360 - xDiff;
									if (xDiff < -180)
										xDiff = -(360 + xDiff);
									xDiff /= (nSplit + 1);
									yDiff = (yVal - model.preferredPath.point_y[nPointsTot - 1]) / (nSplit + 1);
									for (int i3 = 0; i3 < nSplit; i3++) {
										xNu = xValOld + (i3 + 1) * xDiff;
										yNu = yValOld + (i3 + 1) * yDiff;
										if (xNu > 180)
											xNu -= 360;
										if (xNu < -180)
											xNu += 360;
										model.preferredPath.point[nPointsTot] = spherical::Point(yNu, xNu);
										model.preferredPath.point_y[nPointsTot] = yNu;
										model.preferredPath.point_x[nPointsTot] = xNu;
										nPointsTot++;
									}
								}
							}
							if (nPointsTot >= nAlloc) {
								nAlloc += 10;
								model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point, nAlloc * sizeof(spherical::Point));
								model.preferredPath.point_y = (double*)realloc(model.preferredPath.point_y, nAlloc * sizeof(double));
								model.preferredPath.point_x = (double*)realloc(model.preferredPath.point_x, nAlloc * sizeof(double));
							}

							model.preferredPath.point[nPointsTot] = pNu;
							model.preferredPath.point_y[nPointsTot] = yVal;
							model.preferredPath.point_x[nPointsTot] = xVal;
							xValOld = xVal;
							yValOld = yVal;
							nPointsTot++;
						}
					}
				}
			}
			else {
				// linestring
				nPointsNu = (int)dataCoord.size();
				if (nAlloc == 0) {
					nAlloc = nPointsNu;
					model.preferredPath.point = (spherical::Point*)malloc2(nAlloc * sizeof(spherical::Point));
					model.preferredPath.point_y = (double*)malloc2(nAlloc * sizeof(double));
					model.preferredPath.point_x = (double*)malloc2(nAlloc * sizeof(double));
				}
				else {
					if (nPointsTot + nPointsNu >= nAlloc) {
						nAlloc += nPointsNu;
						model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point,
							nAlloc * sizeof(spherical::Point));
					}
				}
				for (auto it2 = dataCoord.begin(); it2 != dataCoord.end(); ++it2) {
					dataIt2 = it2.value();
					i2 = 0;
					for (auto it3 = dataIt2.begin(); it3 != dataIt2.end(); ++it3) {
						if (i2 == 0) {
							xVal = it3.value();
							if (model.preferredPath.startX < -998)
								model.preferredPath.startX = xVal;
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
						pNu = spherical::Point(yVal, xVal);
						if (nPointsTot > 0) {
							dist = model.preferredPath.point[nPointsTot - 1].distanceTo(pNu) / 1000;
							if (dist > model.params.maxDistBetweenPrefPathPoints) {
								nSplit = (int)(dist / model.params.maxDistBetweenPrefPathPoints);
								if (nPointsTot + nSplit >= nAlloc) {
									nAlloc += 10 + nSplit;
									model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point, nAlloc * sizeof(spherical::Point));
									model.preferredPath.point_y = (double*)realloc(model.preferredPath.point_y, nAlloc * sizeof(double));
									model.preferredPath.point_x = (double*)realloc(model.preferredPath.point_x, nAlloc * sizeof(double));
								}

								xDiff = (xVal - model.preferredPath.point_x[nPointsTot - 1]);
								if (xDiff > 180)
									xDiff = xDiff - 360; // 360 - xDiff;
								if (xDiff < -180)
									xDiff = 360 + xDiff; //  -(360 + xDiff);
								xDiff /= (nSplit + 1);

								yDiff = (yVal - model.preferredPath.point_y[nPointsTot - 1]) / (nSplit + 1);
								for (int i3 = 0; i3 < nSplit; i3++) {
									xNu = xValOld + (i3 + 1) * xDiff;
									yNu = yValOld + (i3 + 1) * yDiff;
									if (xNu > 180)
										xNu -= 360;
									if (xNu < -180)
										xNu += 360;
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
									model.preferredPath.point[nPointsTot] = spherical::Point(yNu, xNu);
									model.preferredPath.point_y[nPointsTot] = yNu;
									model.preferredPath.point_x[nPointsTot] = xNu;
									nPointsTot++;
								}
							}
						}
						if (nPointsTot >= nAlloc) {
							nAlloc += 10;
							model.preferredPath.point = (spherical::Point*)realloc(model.preferredPath.point, nAlloc * sizeof(spherical::Point));
							model.preferredPath.point_y = (double*)realloc(model.preferredPath.point_y, nAlloc * sizeof(double));
							model.preferredPath.point_x = (double*)realloc(model.preferredPath.point_x, nAlloc * sizeof(double));
						}

						model.preferredPath.point[nPointsTot] = pNu;
						model.preferredPath.point_y[nPointsTot] = yVal;
						model.preferredPath.point_x[nPointsTot] = xVal;
						xValOld = xVal;
						yValOld = yVal;
						nPointsTot++;
					}
				}
			}
			i++;
			model.preferredPath.nPoints = nPointsTot;
			//for(int ii = 0; ii < model.preferredPath.nPoints; ii++)
			//	printf("prefPath ii %d xy %.3lf %.3lf\n", ii, model.preferredPath.point_x[ii], model.preferredPath.point_y[ii]);
		}
	}

	//printf("nPoints in preferredPath %d\n", model.preferredPath.nPoints);
	errlog("nPoints in preferredPath %d\n", model.preferredPath.nPoints);
	if (model.preferredPath.nPoints == 0) {
		printf("ERROR! There must be points in the preferred path. I have nothing to do so I quit!\n");
		errlog("ERROR! There must be points in the preferred path. I have nothing to do so I quit!\n");
		postRequest("ERROR! There must be points in the preferred path. I have nothing to do so I quit!", 1);
	}

	analyzePreferredPath_longitude();

	json dataStorm, dataStorm2, dataGeom, dataIt3;
	int i1, stormNr;

	if (!data["storms"].is_null() && model.params.hindCast == 0) {
		dataStorm = data["storms"];
		if (!dataStorm["features"].is_null()) {
			dataStorm2 = dataStorm["features"];
			model.nStorms = 0;
			model.nameTmp = (char*)malloc(256 * sizeof(char));
			model.nAllocStorms = (int)dataStorm2.size();
			model.storms = (strStorm*)malloc2(model.nAllocStorms * sizeof(strStorm));
			for (auto it = dataStorm2.begin(); it != dataStorm2.end(); ++it) {
				dataFeature = it.value();
				loadStormObject(dataFeature);
			}
		}

		// errlog("ERROR! Add extended storm information when decided what to use. nStorms %d\n", model.nStorms);
	}
	else {
		if (model.params.hindCast == 0)
			errlog("OBS! No storms given in input data\n");
		model.nStorms = 0;
	}

	if (!data["corridors"].is_null()) {
		loadChannelsFromInfile(data["corridors"]);
	}
	else {
		errlog("OBS! No corridors given in input data\n");
		model.network.nChannels = 0;
		model.network.nMaxNodesInPath = 2;
	}

	//params->weightFuel.base = 1;
	params->weightFuel = 1;
	params->weightEmission = 1;
	model.params.scaleObjEmission = 250;

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
	params->weightSafety.rolling = 0;
	params->weightSafety.surfRiding = 0;
	params->weightSafety.feasibleSafety = 100000;
	params->weightSafety.iceCoverCost_fix = 10000;
	params->weightSafety.iceCoverCost_thickness = 0;

	params->priceTime = 500;

	params->penalties.storm_costInsideInner = 100000000;
	params->penalties.storm_costInsideOuter_kvot = 100;

	if (!data["fuel"].is_null()) {
		json dataFuel = data["fuel"];
		if (!dataFuel["fuel_aux_eca"].is_null()) {
			json fuelType = dataFuel["fuel_aux_eca"];
			if (!fuelType["price"].is_null())
				params->fuel.aux_eca.price = fuelType["price"];
			else {
				errlog("ERROR! No fuel price for aux engine in eca zones given in input data, I use default\n");
				params->fuel.aux_eca.price = 800;
			}
			if (!fuelType["emissionFactor"].is_null())
				params->fuel.aux_eca.emissionFactor = fuelType["emissionFactor"];
			else {
				errlog("ERROR! No emissionFactor for aux engine in eca zones given in input data, I use default\n");
				params->fuel.aux_eca.emissionFactor = 1;
			}
		}
		else {
			errlog("ERROR! No fuel information for aux engine in eca zones given in input data, I use default\n");
			params->fuel.aux_eca.price = 800;
			params->fuel.aux_eca.emissionFactor = 1;
		}
		if (!dataFuel["fuel_aux_noEca"].is_null()) {
			json fuelType = dataFuel["fuel_aux_noEca"];
			if (!fuelType["price"].is_null())
				params->fuel.aux_noEca.price = fuelType["price"];
			else {
				errlog("ERROR! No fuel price for aux engine outside eca zones given in input data, I use default\n");
				params->fuel.aux_noEca.price = 800;
			}
			if (!fuelType["emissionFactor"].is_null())
				params->fuel.aux_noEca.emissionFactor = fuelType["emissionFactor"];
			else {
				errlog("ERROR! No emissionFactor for aux engine outside eca zones given in input data, I use default\n");
				params->fuel.aux_noEca.emissionFactor = 1;
			}
		}
		else {
			errlog("ERROR! No fuel information for aux engine outside eca zones given in input data, I use default\n");
			params->fuel.aux_noEca.price = 800;
			params->fuel.aux_noEca.emissionFactor = 1;
		}
		if (!dataFuel["fuel_main_eca"].is_null()) {
			json fuelType = dataFuel["fuel_main_eca"];
			if (!fuelType["price"].is_null())
				params->fuel.main_eca.price = fuelType["price"];
			else {
				errlog("ERROR! No fuel price for main engine in eca zones given in input data, I use default\n");
				params->fuel.main_eca.price = 800;
			}
			if (!fuelType["emissionFactor"].is_null())
				params->fuel.main_eca.emissionFactor = fuelType["emissionFactor"];
			else {
				errlog("ERROR! No emissionFactor for main engine in eca zones given in input data, I use default\n");
				params->fuel.main_eca.emissionFactor = 1;
			}
		}
		else {
			errlog("ERROR! No fuel information for main engine in eca zones given in input data, I use default\n");
			params->fuel.main_eca.price = 800;
			params->fuel.main_eca.emissionFactor = 1;
		}
		if (!dataFuel["fuel_main_noEca"].is_null()) {
			json fuelType = dataFuel["fuel_main_noEca"];
			if (!fuelType["price"].is_null())
				params->fuel.main_noEca.price = fuelType["price"];
			else {
				errlog("ERROR! No fuel price for main engine outside eca zones given in input data, I use default\n");
				params->fuel.main_noEca.price = 700;
			}
			if (!fuelType["emissionFactor"].is_null())
				params->fuel.main_noEca.emissionFactor = fuelType["emissionFactor"];
			else {
				errlog("ERROR! No emissionFactor for main engine outside eca zones given in input data, I use default\n");
				params->fuel.main_noEca.emissionFactor = 1;
			}
		}
		else {
			errlog("ERROR! No fuel information for main engine outside eca zones given in input data, I use default\n");
			params->fuel.main_noEca.price = 700;
			params->fuel.main_noEca.emissionFactor = 1;
		}

		if (!dataFuel["extra_fuel_consumption"].is_null()) {
			json fuelType = dataFuel["extra_fuel_consumption"];
			if (!fuelType["price"].is_null())
				params->fuel.extra_fuel.price = fuelType["price"];
			else {
				errlog("ERROR! No fuel price for extra fuel, I use default\n");
				params->fuel.extra_fuel.price = 700;
			}
			if (!fuelType["emissionFactor"].is_null())
				params->fuel.extra_fuel.emissionFactor = fuelType["emissionFactor"];
			else {
				errlog("ERROR! No emissionFactor for extra fuel given in input data, I use default 1\n");
				params->fuel.extra_fuel.emissionFactor = 1;
			}
			if (!fuelType["quantity_mts"].is_null())
				params->fuel.extra_fuel.quantity = fuelType["quantity_mts"];
			else {
				errlog("ERROR! No quantity_mts for extra fuel given in input data, I set it to 0\n");
				params->fuel.extra_fuel.quantity = 0;
			}
		}
		else {
			errlog("No extra fuel given in input data\n");
			params->fuel.extra_fuel.quantity = 0;
			params->fuel.extra_fuel.emissionFactor = 1;
			params->fuel.extra_fuel.price = 700;
		}

	}
	else {
		errlog("ERROR! No fuel information given in input data, I use default\n");
	}
	if (!data["vessel_price"].is_null()) {
		params->priceTime = data["vessel_price"];
		params->priceTime /= 24.0;
	}

	if (!data["loadWeightsFile"].is_null()) {
		params->loadWeightsFile = str_alloc_cpy(cleanString(data["loadWeightsFile"]).c_str());
	}
	else
		params->loadWeightsFile = NULL;

	if (params->loadWeightsFile == NULL) {
		if (!data["objective"].is_null()) {
			json dataObj = data["objective"];
			loadWeights(params, dataObj);
		}
	}
	else {
		loadWeightsFromSeparateFile(params);
	}

	if (params->useSimulering == 1) {
		model.params.weightEmission = 0;
		model.params.weightFuel = 0;
		model.params.weightTime = 0;
	}

	fil.close();




	return 0;
}

int loadTableSQLite(int type, int nAlloc, char* namn, char* tableID, float* tableValue) {
	sqlite3* db;
	char* zErrMsg = 0;
	int rc;
	sqlite3_stmt* query;
	const char* data = "Callback function called";
	FILE* filpek;
	//filpek = fopen("tmpTable.txt", "w");
	int retval, pos;
	int count;

	rc = sqlite3_open(namn, &db);
	if (rc) {
		fprintf(stderr, "Can't open database %s: %s\n", namn, sqlite3_errmsg(db));
		return(0);
	}
	else {
		//fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	std::string sql = "SELECT * from " + std::string(tableID) + ";";
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &query, NULL) != SQLITE_OK) {
		printf("error executing query: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	count = 0;
	while (1) {
		retval = sqlite3_step(query);

		if (retval == SQLITE_ROW) {
			pos = (uint32_t)sqlite3_column_int(query, 0);
			if (pos >= nAlloc) {
				errlog("ERROR! Too many values in db table waveTable for type %d. Is more than %d.\n", type,
					nAlloc);
				printf("ERROR! Too many values in db table waveTable for type %d. Is more than %d.\n", type,
					nAlloc);
				postRequest("ERROR! Too many values in db table waveTable for type " + std::to_string(type) +
					" is more than " + std::to_string(nAlloc), 1);
				sqlite3_finalize(query);
				return 0;
			}
			tableValue[pos] = (float)sqlite3_column_double(query, 1);
			//fprintf(filpek, "%f\n", model.functions.waveFactor.tableValue[pos]);
			//printf("type %d speed %d waveHeight %.3lf waveperiod %d waveAngle %.3lf delta %.5lf\n",
			//	(uint32_t)sqlite3_column_int(query, 0),
			//	(uint32_t)sqlite3_column_int(query, 1),
			//	(double)sqlite3_column_double(query, 2),
			//	(uint32_t)sqlite3_column_int(query, 3),
			//	(double)sqlite3_column_double(query, 4),
			//	(double)sqlite3_column_double(query, 5));
		}
		else if (retval == SQLITE_DONE) {
			/* all done */
			//printf("search: row processing done, %u rows processed\n", count);
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

	if (count != nAlloc) {
		errlog("ERROR! Too few values in db table %s for type %d. Is %d, should be %d.\n",
			tableID, type,
			count, nAlloc);
		postRequest("ERROR! Too few values in db table " + std::string(tableID) + " for type " + std::to_string(type) +
			". Is " + std::to_string(count) + ", should be " + std::to_string(nAlloc), 1);
	}
	return 0;
}

int loadAllNeededTablesFromSQLite() {
	loadTablesInfo(1);

	auto tid0 = std::chrono::high_resolution_clock::now();
	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	float* tableValue = NULL;
	int tableNr, nAlloc, nTables = 3;

#ifdef NAZANIN_SAFETY
	nTables = 7; // 5;
#endif


	for (int ii = 0; ii < nTables; ii++) {
		if (ii == 0) { // wind
			tableNr = model.functions.windTableNr;
			copyAddTableInfo(model.tables.tableTyp[0][tableNr].shipSpeedCalmWater, &(model.functions.windFactor.shipSpeedCalmWater));
			copyAddTableInfo(model.tables.tableTyp[0][tableNr].windSpeed, &(model.functions.windFactor.windSpeed));
			copyAddTableInfo(model.tables.tableTyp[0][tableNr].windDirection, &(model.functions.windFactor.windDirection));
			model.functions.maxWindSpeed_warning = model.tables.tableTyp[0][tableNr].maxWindSpeed_warning;
			nAlloc = model.functions.windFactor.shipSpeedCalmWater.nIndex * model.functions.windFactor.windSpeed.nIndex *
				model.functions.windFactor.windDirection.nIndex;
			model.functions.windFactor.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.windFactor.tableValue;
		}
		else if (ii == 1) {
			tableNr = model.functions.waveTableNr;
			copyAddTableInfo(model.tables.tableTyp[1][tableNr].shipSpeedCalmWater, &(model.functions.waveFactor.shipSpeedCalmWater));
			copyAddTableInfo(model.tables.tableTyp[1][tableNr].waveHeight, &(model.functions.waveFactor.waveHeight));
			copyAddTableInfo(model.tables.tableTyp[1][tableNr].wavePeriod, &(model.functions.waveFactor.wavePeriod));
			copyAddTableInfo(model.tables.tableTyp[1][tableNr].waveDirection, &(model.functions.waveFactor.waveDirection));
			model.functions.maxWaveHeight = model.tables.tableTyp[1][tableNr].maxWaveHeight;
			model.functions.maxWaveHeight_warning = model.tables.tableTyp[1][tableNr].maxWaveHeight_warning;
			nAlloc = model.functions.waveFactor.shipSpeedCalmWater.nIndex * model.functions.waveFactor.waveHeight.nIndex *
				model.functions.waveFactor.wavePeriod.nIndex * model.functions.waveFactor.waveDirection.nIndex;
			model.functions.waveFactor.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.waveFactor.tableValue;
		}
		else if (ii == 2) {
			tableNr = model.functions.stabilityTableNr;
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].windSpeed, &(model.functions.dynStability.windSpeed));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].windDirection, &(model.functions.dynStability.windDirection));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].shipSpeedOverGround, &(model.functions.dynStability.shipSpeedOverGround));

			nAlloc = model.functions.dynStability.windSpeed.nIndex * model.functions.dynStability.windDirection.nIndex * model.functions.dynStability.shipSpeedOverGround.nIndex;
			model.functions.dynStability.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.dynStability.tableValue;
		}
		else if (ii == 3) {
			tableNr = model.functions.stabilityTableNr;
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].waveHeight, &(model.functions.bowSlamming.waveHeight));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].wavePeriod, &(model.functions.bowSlamming.wavePeriod));

			nAlloc = model.functions.bowSlamming.waveHeight.nIndex * model.functions.bowSlamming.wavePeriod.nIndex;
			model.functions.bowSlamming.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.bowSlamming.tableValue;
		}
		else if (ii == 4) {
			tableNr = model.functions.stabilityTableNr;
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].waveHeight, &(model.functions.greenWater.waveHeight));

			nAlloc = model.functions.greenWater.waveHeight.nIndex;
			model.functions.greenWater.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.greenWater.tableValue;
		}
		else if (ii == 5) {
			tableNr = model.functions.stabilityTableNr;
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].waveHeight, &(model.functions.rolling.waveHeight));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].wavePeriod, &(model.functions.rolling.wavePeriod));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].waveDirection, &(model.functions.rolling.waveDirection));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].relShipSpeed, &(model.functions.rolling.relShipSpeed));

			nAlloc = model.functions.rolling.relShipSpeed.nIndex * model.functions.rolling.waveDirection.nIndex *
				model.functions.rolling.wavePeriod.nIndex * model.functions.rolling.waveHeight.nIndex;
			model.functions.rolling.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.rolling.tableValue;
		}
		else if (ii == 6) {
			tableNr = model.functions.stabilityTableNr;
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].relShipSpeed, &(model.functions.surfRiding.relShipSpeed));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].waveDirection, &(model.functions.surfRiding.waveDirection));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].waveHeight, &(model.functions.surfRiding.waveHeight));
			copyAddTableInfo(model.tables.tableTyp[ii][tableNr].wavePeriod, &(model.functions.surfRiding.wavePeriod));

			nAlloc = model.functions.surfRiding.relShipSpeed.nIndex * model.functions.surfRiding.waveDirection.nIndex *
				model.functions.surfRiding.waveHeight.nIndex * model.functions.surfRiding.wavePeriod.nIndex;
			model.functions.surfRiding.tableValue = (float*)malloc2(nAlloc * sizeof(float));
			tableValue = model.functions.surfRiding.tableValue;
		}

		sprintf(namn, "%s/shipTables/%s_%d.db", model.params.indataPath.c_str(),
			model.tables.tableTyp[ii][tableNr].tableID, ii);

		loadTableSQLite(ii, nAlloc, namn, model.tables.tableTyp[ii][tableNr].tableID, tableValue);
	}

	auto tid1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
	//printf("read all weather factor tables %.3lf\n", fp_ms);

	return 0;
}

static int callbackDB(void* NotUsed, int argc, char** argv, char** azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int readSQLiteAllTablesInfo() {
	int nTypes = 7;

	model.sqliteTables = (strSQLiteTables*)malloc2(nTypes * sizeof(strSQLiteTables));
	for (int type = 0; type < nTypes; type++) {
		model.sqliteTables[type].nTables = 0;
		model.sqliteTables[type].nAlloc = 100;
		model.sqliteTables[type].table = (strDBTableInfo*)malloc2(
			model.sqliteTables[type].nAlloc * sizeof(strDBTableInfo));
	}
	model.sqliteMap = (strSQLiteMap*)malloc2(2 * sizeof(strSQLiteMap));
	for (int type = 0; type < 2; type++) {
		model.sqliteMap[type].textFileName = NULL;
	}

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

	std::string sql;// = "CREATE TABLE all_tables("  \
		//	"tableID TEXT NOT NULL, " \
		//	"tableType INT, textFileName TEXT, epochCount REAL);";

		///* Execute SQL statement */
		//rc = sqlite3_exec(db, sql.c_str(), callbackDB, 0, &zErrMsg);
		//if (rc) {
		//	errlog("ERROR. Failed to add table all_tables to database: %s\n", sqlite3_errmsg(db));
		//	//return(0);
		//}

	sql = "SELECT * from all_tables;";
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &query, NULL) != SQLITE_OK) {
		printf("error executing query: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	int retval, pos;
	int count = 0, type;
	while (1) {
		retval = sqlite3_step(query);

		if (retval == SQLITE_ROW) {
			type = (uint32_t)sqlite3_column_int(query, 1);
			pos = model.sqliteTables[type].nTables;
			if (pos >= model.sqliteTables[type].nAlloc) {
				model.sqliteTables[type].nAlloc += 100;
				model.sqliteTables[type].table = (strDBTableInfo*)realloc(model.sqliteTables[type].table,
					model.sqliteTables[type].nAlloc * sizeof(strDBTableInfo));
			}
			model.sqliteTables[type].table[pos].tableID = str_alloc_cpy((char*)sqlite3_column_text(query, 0));
			model.sqliteTables[type].table[pos].textFileName = str_alloc_cpy((char*)sqlite3_column_text(query, 2));
			model.sqliteTables[type].table[pos].epochCount = (double)sqlite3_column_double(query, 3);
			(model.sqliteTables[type].nTables)++;
		}
		else if (retval == SQLITE_DONE) {
			/* all done */
			//printf("search: row processing done, %u rows processed\n", count);
			break;
		}
		else {
			/* error of some sort */
			printf("Error search: error during row processing: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(query);
			errlog("Error search: error during row processing: %s\n", sqlite3_errmsg(db));
			postRequest("Error search in table all_tables: error during row processing: " + std::string(sqlite3_errmsg(db)), 1);
			return 0;
		}
		count++;
	}
	sqlite3_finalize(query);

	count = 0;
	for (int count2 = 0; count2 < 2; count2++) {
		sql = "SELECT * from maps;";
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &query, NULL) != SQLITE_OK) {
			if (count2 == 0) {
				/* error of some sort */
				sql = "CREATE TABLE maps("  \
					"mapNr INT PRIMARY KEY     NOT NULL," \
					"textFileName TEXT NOT NULL," \
					"epochCount REAL NOT NULL," \
					"nCols INT NOT NULL," \
					"nRows INT NOT NULL," \
					"size_col REAL NOT NULL," \
					"size_row REAL NOT NULL," \
					"minX REAL NOT NULL," \
					"maxX REAL NOT NULL," \
					"minY REAL NOT NULL," \
					"maxY REAL NOT NULL," \
					"nBlockRows INT NOT NULL," \
					"nBlockCols INT NOT NULL); ";

				/* Execute SQL statement */
				rc = sqlite3_exec(db, sql.c_str(), callbackDB, 0, &zErrMsg);
				if (rc) {
					postRequest("Failed to add table maps to database: " + std::string(sqlite3_errmsg(db)), 1);
				}
				sqlite3_stmt* query, * query2;
				sql = "INSERT INTO maps (mapNr, textFileName, epochCount, nCols, nRows, size_col, size_row, minX, maxX, minY, maxY, nBlockRows, nBlockCols) VALUES "
					"(0, 'text', 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0);";
				rc = sqlite3_exec(db, sql.c_str(), callbackDB, 0, &zErrMsg);
				sql = "INSERT INTO maps (mapNr, textFileName, epochCount, nCols, nRows, size_col, size_row, minX, maxX, minY, maxY, nBlockRows, nBlockCols) VALUES "
					"(1, 'text', 0, 0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0);";
				rc = sqlite3_exec(db, sql.c_str(), callbackDB, 0, &zErrMsg);
				continue;
			}
			else {
				postRequest("Error executing query: " + std::string(sqlite3_errmsg(db)), 1);
			}
		}

		while (1) {
			retval = sqlite3_step(query);

			if (retval == SQLITE_ROW) {
				type = (uint32_t)sqlite3_column_int(query, 0);
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
				//printf("search: row processing done, %u rows processed\n", count);
				break;
			}
			else {
				printf("Error search: error during row processing: %s\n", sqlite3_errmsg(db));
				sqlite3_finalize(query);
				errlog("Error search: error during row processing: %s\n", sqlite3_errmsg(db));
				postRequest("Error search in table maps: error during row processing: " + std::string(sqlite3_errmsg(db)), 1);
				return 0;
			}
			count++;
		}
		if (count > 0)
			break;
	}
	sqlite3_finalize(query);

	sqlite3_close(db);


	return 0;
}

int findTableNr_inDatabase(int type, int tablePos) {
	int i;

	for (i = 0; i < model.sqliteTables[type].nTables; i++) {
		if (strcmp(model.tables.tableTyp[type][tablePos].tableID, model.sqliteTables[type].table[i].tableID) == 0) {
			break;
		}
	}
	if (i < model.sqliteTables[type].nTables)
		return i;
	else
		return -1;
}

int checkIfModifiedFile(int type, int tablePos) {

	int tableNr = findTableNr_inDatabase(type, tablePos);

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/%s", model.params.indataPath.c_str(), model.tables.tableTyp[type][tablePos].fileName);

#ifdef _WIN32
	try {
		const auto fileTime = std::filesystem::last_write_time(namn);
		//std::filesystem::file_time_type ftime = std::filesystem::last_write_time("..//data//shipTables//tmp.txt");
		const auto ticks = fileTime.time_since_epoch().count() - 1.33e17;
		model.tmpEpochCount = ticks;
	}
	catch (...) {
		printf("ERROR! Last modified date of %s given in table_parameters.json could not be read. Does it exist\n", namn);
		postRequest("ERROR! Last modified date of " + std::string(namn) + " given in table_parameters.json could not be read. Does it exist?", 1);
	}
#else
	struct stat result;
	if (stat(namn, &result) != 0)
	{
		printf("ERROR! Failed to get stats from file %s\n", namn);
		postRequest("ERROR! Last modified date of " + std::string(namn) + " given in table_parameters.json could not be read. Does it exist?", 1);
	}
	auto ticks = result.st_mtime;
	model.tmpEpochCount = ticks;
#endif // 


	printf("model.tmpEpochCount %lf\n", model.tmpEpochCount);
	if (tableNr == -1)
		return 2; // this tables doesn't exist in the database, so add it

	//printf("%I64d\n", ticks);
	//time_t test = to_time_t(fileTime);
	//printf("to_time_t: %I64d\n", test);

	if (model.sqliteTables[type].table[tableNr].epochCount < model.tmpEpochCount - 10)
		return 1;
	else
		return 0;
}

int sub_deleteFile(std::string namn)
{
	int res;
	res = remove(namn.c_str());
	if (res == -1) {
		if (strcmp(strerror(errno), "Permission denied") == 0) {
			res = -2;
			errlog("Deleting file %s\n\tError message: '%s' return value %d\n", namn.c_str(), strerror(errno), res);
		}
	}
	return res;
}

int saveFactorTableToSQLite(int type, int tablePos, int modified, int nAlloc) {

	sqlite3* db;
	char* zErrMsg = 0;
	int rc;

	char* namn;


	auto tid0 = std::chrono::high_resolution_clock::now();
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/shipTables/%s_%d.db", model.params.indataPath.c_str(),
		model.tables.tableTyp[type][tablePos].tableID, type);
	//if (modified == 1) {
	int res = sub_deleteFile(namn);
	if (res == -2) {
		errlog("ERROR! Could not delete the database %s. It must be open in another application. Close it and run the redis update again\n",
			namn);
		postRequest("ERROR! Could not delete the database " + std::string(namn) + ". Is it possibly locked by another application. Close it and run the redis update again", 1);
	}
	//}

	rc = sqlite3_open(namn, &db);

	if (rc) {
		fprintf(stderr, "Can't open database %s: %s\n", namn, sqlite3_errmsg(db));
		return(0);
	}
	else {
		//fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	std::string sql = "CREATE TABLE " + std::string(model.tables.tableTyp[type][tablePos].tableID) + "("  \
		"ID INT PRIMARY KEY NOT NULL, " \
		"value REAL );";

	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql.c_str(), callbackDB, 0, &zErrMsg);
	if (rc) {
		fprintf(stderr, "Failed to add table to database: %s\n", sqlite3_errmsg(db));
		//return(0);
	}

	sqlite3_stmt* query, * query2;
	sql = "INSERT INTO " + std::string(model.tables.tableTyp[type][tablePos].tableID) +
		" (ID, value) VALUES (?1, ?2)";
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &query, NULL) != SQLITE_OK) {
		printf("error executing insert query: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	rc = sqlite3_exec(db, "BEGIN", callbackDB, 0, &zErrMsg);
	float* tableValue = NULL;
	if (type == 0)
		tableValue = model.functions.windFactor.tableValue;
	else if (type == 1)
		tableValue = model.functions.waveFactor.tableValue;
	else if (type == 2)
		tableValue = model.functions.dynStability.tableValue;
	else if (type == 3)
		tableValue = model.functions.bowSlamming.tableValue;
	else if (type == 4)
		tableValue = model.functions.greenWater.tableValue;
	else if (type == 5)
		tableValue = model.functions.rolling.tableValue;
	else if (type == 6)
		tableValue = model.functions.surfRiding.tableValue;

	int pos = 0, retval;
	for (int i = 0; i < nAlloc; i++) {
		retval = sqlite3_bind_int(query, 1, pos);
		retval = sqlite3_bind_double(query, 2, tableValue[pos]);
		retval = sqlite3_step(query);
		if (retval != SQLITE_DONE)
			printf("error executing insert query: %s\n", sqlite3_errmsg(db));
		sqlite3_reset(query);
		pos++;
	}
	sqlite3_finalize(query);
	rc = sqlite3_exec(db, "COMMIT", callbackDB, 0, &zErrMsg);
	sqlite3_close(db);

	auto tid1 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> fp_ms = tid1 - tid0;
	printf("write table %s took %.3lf\n", model.tables.tableTyp[type][tablePos].tableID, fp_ms);

	updateSQLiteAllTablesInfo(type, tablePos, modified);

	return 0;
}

int saveTablesToSQLite(std::string inputPath) {
	int ii;

	resultPath = inputPath;

	model.params.indataPath = inputPath;
	model.params.errorCode = 0;
	loadParams_theRestOld(&(model.params));
	loadFileParams_feasibilityOptiNav(&(model.params));

	loadTablesInfo(0);

	loadVariables(1);

	readSQLiteAllTablesInfo();

	model.functions.windFactor.tableValue = NULL;
	model.functions.windFactor.tableValue = NULL;
	model.functions.windFactor.tableValue = NULL;

	int i, modified, nAlloc = 0, nTables = 3;
	json mDataParam;

#ifdef NAZANIN_SAFETY
	nTables = 7; // 5;
#endif

	for (ii = 0; ii < nTables; ii++) {
		for (i = 0; i < model.tables.nTableTyp[ii]; i++) {
			modified = checkIfModifiedFile(ii, i);
			if (modified == 0)
				continue; // this file is not modified so no need to update it

			if (ii == 0) {
				nAlloc = loadWeatherFactorTableWind(i);
			}
			else if (ii == 1) {
				nAlloc = loadWeatherFactorTableWave(i);
			}
			else if (ii == 2) {
				nAlloc = loadDynamicStabilityTable(i);
			}
			else if (ii == 3) {
				nAlloc = loadBowSlammingTable(i);
			}
			else if (ii == 4) {
				nAlloc = loadGreenWaterTable(i);
			}
			else if (ii == 5) {
				nAlloc = loadRollingTable(i);
			}
			else if (ii == 6) {
				nAlloc = loadSurfRidingTable(i);
			}
			saveFactorTableToSQLite(ii, i, modified, nAlloc);
		}
	}

	if (model.params.errorCode != 0) {
		errlog("ERROR! Saving tables to SQLite databases failed\n");
	}
	else
		errlog("Saving tables to SQLite database successful\n");

	return 0;
}

double lookup_dynamicStabilityTable(double windSpeed, double relWindDirection, double shipSpeed) {
	int wSpeedIndex, wDirIndex, shipSpeedIndex, pos;

	wSpeedIndex = get_tableIndex(windSpeed, model.functions.dynStability.windSpeed); // getWindSpeedIndex(relWindSpeed, model.functions.dynStability);
	wDirIndex = get_tableIndexDirection(relWindDirection, model.functions.dynStability.windDirection); // getWindDirectionIndex(relWindDirection, model.functions.dynStability);
	shipSpeedIndex = get_tableIndexDirection(shipSpeed, model.functions.dynStability.shipSpeedOverGround); // getWindDirectionIndex(relWindDirection, model.functions.dynStability);
	if (wSpeedIndex < 0 || wDirIndex < 0 || shipSpeedIndex < 0)
		return 9999.9;
	else {
		pos = shipSpeedIndex + model.functions.dynStability.shipSpeedOverGround.nIndex *
			(wDirIndex + model.functions.dynStability.windDirection.nIndex * wSpeedIndex);
		return model.functions.dynStability.tableValue[pos];
	}
}

double lookup_bowSlammingTable(double waveHeight, double wavePeriod) {
	int heightIndex, periodIndex, pos;

	heightIndex = get_tableIndex(waveHeight, model.functions.bowSlamming.waveHeight);
	periodIndex = get_tableIndex(wavePeriod, model.functions.bowSlamming.wavePeriod);
	if (heightIndex < 0 || periodIndex < 0)
		return 9999.9;
	else {
		pos = periodIndex + model.functions.bowSlamming.wavePeriod.nIndex * heightIndex;
		return model.functions.bowSlamming.tableValue[pos];
	}
}

double lookup_surfRidingTable(double waveHeight, double wavePeriod, double relWaveDirection, double shipSpeed) {
	int speedIndex, dirIndex, pos, heightIndex, periodIndex;

	heightIndex = get_tableIndex(waveHeight, model.functions.surfRiding.waveHeight);
	periodIndex = get_tableIndex(wavePeriod, model.functions.surfRiding.wavePeriod);
	dirIndex = get_tableIndex(relWaveDirection, model.functions.surfRiding.waveDirection);
	speedIndex = get_tableIndex(shipSpeed, model.functions.surfRiding.relShipSpeed);
	if (speedIndex < 0 || dirIndex < 0)
		return 9999.9;
	else {
		pos = speedIndex + model.functions.surfRiding.relShipSpeed.nIndex * (
			dirIndex + model.functions.surfRiding.waveDirection.nIndex * 
			(periodIndex + model.functions.surfRiding.wavePeriod.nIndex * heightIndex));
		return model.functions.surfRiding.tableValue[pos];
	}
}

double lookup_rollingTable(double waveHeight, double wavePeriod, double waveDirection, double shipSpeed) {
	int heightIndex, dirIndex, periodIndex, shipSpeedIndex, pos;

	heightIndex = get_tableIndex(waveHeight, model.functions.rolling.waveHeight);
	periodIndex = get_tableIndex(wavePeriod, model.functions.rolling.wavePeriod);
	dirIndex = get_tableIndex(waveDirection, model.functions.rolling.waveDirection);
	shipSpeedIndex = get_tableIndexDirection(shipSpeed, model.functions.rolling.relShipSpeed);
	if (heightIndex < 0 || periodIndex < 0 || dirIndex < 0 || shipSpeedIndex < 0)
		return 9999.9;
	else {
		pos = shipSpeedIndex + model.functions.rolling.relShipSpeed.nIndex *
			(dirIndex + model.functions.rolling.waveDirection.nIndex *
			(periodIndex + model.functions.rolling.wavePeriod.nIndex * heightIndex));
		return model.functions.rolling.tableValue[pos];
	}
}


double lookup_greenWaterTable(double waveHeight) {
	int heightIndex, pos;

	heightIndex = get_tableIndex(waveHeight, model.functions.greenWater.waveHeight);
	if (heightIndex < 0)
		return 9999.9;
	else {
		pos = heightIndex;
		return model.functions.greenWater.tableValue[pos];
	}
}


void eval_safety_nazanin(double shipSpeedOverLand, double shipSpeedRelWater, double windspeed, double absWindDirDiff, double waveHeight,
	double wavePeriod, double relWaveDirection, double iceCover) {
	int feasibleSafety = 1;
	double dynStab = 0, iceCost, bowSlam, greenWater, rolling, surfRiding;

	dynStab = lookup_dynamicStabilityTable(windspeed, absWindDirDiff, shipSpeedOverLand);
	bowSlam = lookup_bowSlammingTable(waveHeight, wavePeriod);
	greenWater = lookup_greenWaterTable(waveHeight);
	if (waveHeight >= 6)
		waveHeight = waveHeight;
	rolling = lookup_rollingTable(waveHeight, wavePeriod, relWaveDirection, shipSpeedRelWater);

	if (waveHeight >= 3)
		waveHeight = waveHeight;
	surfRiding = lookup_surfRidingTable(waveHeight, wavePeriod, relWaveDirection, shipSpeedRelWater);

	if (model.functions.valuesNow.bowSlam < bowSlam)
		model.functions.valuesNow.bowSlam = bowSlam;
	if (bowSlam > 0.999 && model.params.weightSafety.bowSlam > 0.0001) {
		model.functions.valuesNow.feasibleSafety = 0;
	}

	if (model.functions.valuesNow.greenWater < greenWater)
		model.functions.valuesNow.greenWater = greenWater;
	if (greenWater > 0.999 && model.params.weightSafety.greenWater > 0.0001)
		model.functions.valuesNow.feasibleSafety = 0;

	if (model.functions.valuesNow.rolling < rolling)
		model.functions.valuesNow.rolling = rolling;
	if (rolling > 0.999 && model.params.weightSafety.rolling > 0.0001)
		model.functions.valuesNow.feasibleSafety = 0;

	if (surfRiding > 0.001)
		surfRiding = surfRiding;
	if (model.functions.valuesNow.surfRiding < surfRiding)
		model.functions.valuesNow.surfRiding = surfRiding;
	if (surfRiding > 0.999 && model.params.weightSafety.surfRiding > 0.0001)
		model.functions.valuesNow.feasibleSafety = 0;

	if (model.functions.valuesNow.dynamicStability < dynStab)
		model.functions.valuesNow.dynamicStability = dynStab;
	if (dynStab >= 0.999 && model.params.weightSafety.dynamicStability > 0.0001)
		model.functions.valuesNow.feasibleSafety = 0;


	if (iceCover > model.functions.iceCoverMaxFree) {
		iceCost = model.params.weightSafety.iceCoverCost_fix + model.params.weightSafety.iceCoverCost_thickness * (
			iceCover - model.functions.iceCoverMaxFree);
		if (model.functions.valuesNow.iceCoverCost < iceCost)
			model.functions.valuesNow.iceCoverCost = iceCost;
		model.functions.valuesNow.feasibleSafety = 0;
	}

}


void eval_safety(double iceCover) {
	double dynStab = 0, iceCost, bowSlam, greenWater, rolling,surfRiding;

	if (model.functions.valuesNow.dynamicStability < dynStab)
		model.functions.valuesNow.dynamicStability = dynStab;
	if (dynStab >= 0.999)
		model.functions.valuesNow.feasibleSafety = 0;


	if (iceCover > model.functions.iceCoverMaxFree) {
		iceCost = model.params.weightSafety.iceCoverCost_fix + model.params.weightSafety.iceCoverCost_thickness * (
			iceCover - model.functions.iceCoverMaxFree);
		if (model.functions.valuesNow.iceCoverCost < iceCost)
			model.functions.valuesNow.iceCoverCost = iceCost;
		model.functions.valuesNow.feasibleSafety = 0;
	}

}

int saveWindTableToSQLite_old() {

	sqlite3* db;
	char* zErrMsg = 0;
	int rc;

	char* namn;
	namn = (char*)malloc2(256 * sizeof(char));
	sprintf(namn, "%s/shipTables/windTable.db", model.params.indataPath.c_str());
	rc = sqlite3_open(namn, &db);

	if (rc) {
		fprintf(stderr, "Can't open database %s: %s\n", namn, sqlite3_errmsg(db));
		return(0);
	}
	else {
		//fprintf(stderr, "Opened database successfully\n");
	}

	/* Create SQL statement */
	std::string sql = "CREATE TABLE windTable("  \
		"ID INT PRIMARY KEY     NOT NULL," \
		"shipSpeed           REAL NOT NULL," \
		"windSpeed REAL NOT NULL," \
		"windDir REAL NOT NULL," \
		"dSpeed REAL );";

	/* Execute SQL statement */
	rc = sqlite3_exec(db, sql.c_str(), callbackDB, 0, &zErrMsg);
	if (rc) {
		fprintf(stderr, "Failed to add table to database: %s\n", sqlite3_errmsg(db));
		return(0);
	}

	std::stringstream stream;
	double shipSpeed, windSpeed, windDir;
	int pos = 0;
	for (int i = 0; i < model.functions.windFactor.shipSpeedCalmWater.nIndex; i++) {
		shipSpeed = model.functions.windFactor.shipSpeedCalmWater.minValue +
			model.functions.windFactor.shipSpeedCalmWater.intervalSize * i;
		for (int i1 = 0; i1 < model.functions.windFactor.windSpeed.nIndex; i1++) {
			windSpeed = model.functions.windFactor.windSpeed.minValue +
				model.functions.windFactor.windSpeed.intervalSize * i1;
			for (int i2 = 0; i2 < model.functions.windFactor.windDirection.nIndex; i2++) {
				windDir = model.functions.windFactor.windDirection.minValue +
					model.functions.windFactor.windDirection.intervalSize * i2;
				//stream << "INSERT INTO windTable VALUES (" << pos + 1 << ", " << std::fixed << std::setprecision(2) << shipSpeed <<
				//	", " << std::fixed << std::setprecision(2) << windSpeed <<
				//	", " << std::fixed << std::setprecision(2) << windDir <<
				//	", " << std::fixed << std::setprecision(5) << model.functions.windFactor.tableValue[pos] << ");\n";
				pos++;
			}
		}
	}
	rc = sqlite3_exec(db, stream.str().c_str(), callbackDB, 0, &zErrMsg);
	if (rc) {
		fprintf(stderr, "Failed to add rows to database: %s\n", sqlite3_errmsg(db));
		return(0);
	}



	sqlite3_close(db);
	return 0;
}

int getAllVariableValues(int checkPointNr, double tidpkt)
 {
	int varNr;
	int tidInt, tidIndex;

	tidInt = (int)(tidpkt * model.weather_inv_timeIntervall_h);
#ifdef KAOUTAR
	if (tidInt > model.weather_nTimeIntervals_maxValue){
		if (model.results.forecastType < 200) {
			tidInt = model.weather_nTimeIntervals_maxValue;
		}
		else {
			// calm water, no weather...
			for (varNr = 0; varNr < model.nWeatherFiles; varNr++) {
				model.functions.varValue[varNr] = 0;
			}
			return 0;
		}
	}
#else
	if (tidInt > model.weather_nTimeIntervals_maxValue)
		tidInt = model.weather_nTimeIntervals_maxValue;
#endif
	globalCount1++;

	for (varNr = 0; varNr < model.nWeatherFiles; varNr++) {
		tidIndex = model.weather[varNr].timeIntervalIndex[tidInt];
		if (printGlobal == 1) {
			errlog("checkP %d t %.3lf %d %d var %d pos %d from %d %d %d val %.3lf\n", checkPointNr, tidpkt, tidInt, tidIndex, varNr,
				model.weatherFunctions.checkPoint[checkPointNr].pos_latLon[varNr],
				model.weatherFunctions.checkPoint[checkPointNr].latPos[varNr],
				model.weatherFunctions.checkPoint[checkPointNr].lonPos[varNr], model.weather[varNr].nCols,
				model.weather[varNr].valueCell[tidIndex][model.weatherFunctions.checkPoint[checkPointNr].pos_latLon[varNr]]);
		}
		model.functions.varValue[varNr] = model.weather[varNr].valueCell[tidIndex][model.weatherFunctions.checkPoint[checkPointNr].pos_latLon[varNr]];
	}

	return 0;
}


double evalWeatherDataAlongArcSection(int arcNr, double startKvot, double endKvot, int startSlutArc, double timeExact, double delayFactor, int useFixCalmWaterSpeed) {
	int i, cNr, tidInt, delayNr, tidIntForecast;
	double checkFactor, speedNu, calcDelayFactor, checkSpeedDiffCurrent, tidNu, tidCalmWater;
	double windSpeed_x, windSpeed_y, waveDir_y, waveDir_x, dist, tidTot, distNu;
	double stormVarde, uCurrent, vCurrent, deltaTid;
	double currentReal_u = 0, currentReal_v = 0, windSpeedReal_u = 0, windSpeedReal_v = 0, waveHeightReal_u = 0, waveHeightReal_v = 0;
	double timeArc_wind = 0, timeArc_current = 0, timeArc_wave = 0;
	double currentDirection, currentSpeed, baseGroundSpeed, calmWaterSpeed, uWind, vWind;
	double speedDiffWindWave, rel_windSpeed, rel_windDir, speedOverGround, timeArc;
	double fuelConsumption_main, fuelConsumption_aux, fuelUsage_main, fuelUsage_aux, windDirection, windSpeed2, windSpeed, waveHeight, wavePeriod;
	double rel_waveDir, iceCover, waveDirection, speedDiffWind, speedDiffWave;
	double fixTime = -1, distStart, distEnd, useKvotNu, kvotBort;
	double waveHeightBase, waveDirectionBase, fuelTot_main = 0, fuelTot_aux = 0;
	double timeArcSTW, absWindDirDiff = 0;
	int favorableWind, favorableWave;
	double tidTotArc, tidTotArc0;

	if (arcNr == 36)
		arcNr = arcNr;
	//if (arcNr == 412305)
	//	printGlobal = 1;
	//else
	//	printGlobal = 0;

	//if (arcNr == 96)
	//	printf("arcNr %d kvoter %.3lf %.3lf timeExact %.3lf\n", arcNr,
	//		startKvot, endKvot, timeExact);
	model.functions.valuesNow.channelCost = 0;
	if (arcNr >= 0) {
		if (model.arc[arcNr].toLevel < 0 && model.arc[arcNr].fromLevel < 0) {
			cNr = -model.arc[arcNr].toLevel - 1;
			model.functions.valuesNow.channelCost = model.network.channel[cNr].extraCostChannel;
			if (startKvot < 0.001) {
				if (model.network.channel[cNr].intArrivalTime_h >= 0) {
					//printf("corridor %d intArrivalTime_h %d\n", cNr, model.network.channel[cNr].intArrivalTime_h);
					timeExact = delayTimeToStartTimeDay(timeExact, model.network.channel[cNr].intArrivalTime_h);
				}
				timeExact += model.network.channel[cNr].waitingTime; // .intWaitingTime;
			}
			fixTime = (model.network.channel[cNr].timeThroughChannel * (endKvot - startKvot));
		}
		else {
			if (model.arc[arcNr].speedSetting < 0)
				fixTime = 10000;
		}
	}

	//if (arcNr == 96)
	//	printf("arcNr %d fixTime %.3lf timeExact %.3lf startSlutArc %d\n", arcNr,
	//		fixTime, timeExact, startSlutArc);

	model.functions.valuesNow.current = 0;
	model.functions.valuesNow.windSpeed = 0;
	windSpeed_x = 0;
	windSpeed_y = 0;
	model.functions.valuesNow.waveHeight = 0;
	model.functions.valuesNow.wavePeriod = 0;

	if (model.nWeatherFiles > 10) {
		model.functions.valuesNow.pressureSurface = 0;
		model.functions.valuesNow.pressureAir = 0;
		model.functions.valuesNow.precipitation = 0;
		model.functions.valuesNow.tempSea = 0;
		model.functions.valuesNow.tempAir = 0;
		model.functions.valuesNow.cloudCover = 0;
		model.functions.valuesNow.timePressureSurface = 0;
		model.functions.valuesNow.timePressureAir = 0;
		model.functions.valuesNow.timePrecipitation = 0;
		model.functions.valuesNow.timeTempSea = 0;
		model.functions.valuesNow.timeTempAir = 0;
		model.functions.valuesNow.timeCloudCover = 0;

		model.functions.valuesNow.mdps = 0;
		model.functions.valuesNow.swell = 0;
		model.functions.valuesNow.timeMdps = 0;
		model.functions.valuesNow.timeSwell = 0;
	}

	waveDir_x = 0;
	waveDir_y = 0;
	model.functions.valuesNow.forecastType = 0;
	model.functions.valuesNow.worstStormValue = 0;
	model.functions.valuesNow.iceCover_max = 0;
	//model.functions.valuesNow.bowSlamming_max = 0;
	//model.functions.valuesNow.greenWater_max = 0;
	//model.functions.valuesNow.dynamicStability_max = 0;
	model.functions.valuesNow.bowSlam = 0;
	model.functions.valuesNow.greenWater = 0;
	model.functions.valuesNow.dynamicStability = 0;
	model.functions.valuesNow.feasibleSafety = 1;
	model.functions.valuesNow.iceCoverCost = 0;
	model.functions.valuesNow.rolling = 0;
	model.functions.valuesNow.surfRiding = 0;

	model.functions.valuesNow.relWindDir = 0;
	model.functions.valuesNow.relWaveDir = 0;

	model.functions.valuesNow.speedDiffWind = 0;
	model.functions.valuesNow.speedDiffWave = 0;
	model.functions.valuesNow.baseGroundSpeed = 0;

	model.functions.valuesNow.WindF = 0;
	model.functions.valuesNow.WaveF = 0;
	model.functions.valuesNow.CurrentF = 0;
	model.functions.valuesNow.DelayF = 0;

	model.functions.valuesNow.favorableWind_h = 0;
	model.functions.valuesNow.favorableWave_h = 0;
	model.functions.valuesNow.favorableWindWave_h = 0;

	if (model.arc[arcNr].time <= 0.001 && model.arc[arcNr].distance <= 0.001) {
		model.functions.valuesNow.fuel_aux = 0;
		model.functions.valuesNow.fuel_main = 0;
		model.functions.valuesNow.currentReal = 0;
		model.functions.valuesNow.currentDirReal = 0;
		model.functions.valuesNow.windReal = 9999;
		model.functions.valuesNow.windDirReal = 0;
		model.functions.valuesNow.waveDirReal = 0;
		return timeExact;
	}

	dist = 0;
	distStart = model.arc[arcNr].distance * startKvot;
	distEnd = model.arc[arcNr].distance * endKvot;

	if (startSlutArc == 0) {
		tidTot = timeExact;

		//if (arcNr == 175518)
		//	arcNr = arcNr;
		if (fixTime > 0) {
			if (fixTime < 9999)
				calmWaterSpeed = model.network.channel[cNr].distance_km * (endKvot - startKvot) / fixTime;
			else {
				if (useFixCalmWaterSpeed == 0)
					calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				else
					calmWaterSpeed = model.params.calmWaterSpeedCompareUse;
			}

			//printf("\narcNr %d levels %d %d pointPos %d %d cNr %d dist %.3lf arcDist %.3lf fixTime %.3lf\n", arcNr, 
			//	model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, 
			//	model.arc[arcNr].toPointNr, cNr,
			//	model.network.channel[cNr].distance_km, model.arc[arcNr].distance, fixTime);
		}
		else {
			if (useFixCalmWaterSpeed == 0)
				calmWaterSpeed = eval_calmWaterSpeed(model.arc[arcNr].speedSetting, model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
			else
				calmWaterSpeed = model.params.calmWaterSpeedCompareUse;
		}
		model.functions.valuesNow.calmWaterSpeed = calmWaterSpeed;
		if (printGlobal == 1) {
			if (arcNr >= 0)
				printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, model.arc[arcNr].speedSetting, calmWaterSpeed,
					model.weatherFunctions.nCheckPoints);
			else
				printf("arcNr %d speedSet %d calmWaterSpeed %.3lf nCheckPoints %d\n", arcNr, -1, calmWaterSpeed,
					model.weatherFunctions.nCheckPoints);
		}

		//if (arcNr == 62)
		//	arcNr = arcNr;

		if (USE_ARC_TIME_EXACT == 1)
			tidTotArc = model.arc[arcNr].fromTime * model.params.tIndexGerH;
		else
			tidTotArc = tidTot;
		tidTotArc0 = tidTotArc;

		for (i = 0; i < model.weatherFunctions.nCheckPoints; i++) {
			if (dist >= distEnd)
				break; // past the end of this part of the arc

			distNu = model.weatherFunctions.checkPoint[i].distToNextPkt;
			dist += distNu;

			if (dist <= distStart && USE_ARC_TIME_EXACT == 0)
				continue; // not far enough of the arc yet

			if (dist - distNu < distStart)
				useKvotNu = (dist - distStart) / distNu;
			else
				useKvotNu = 1.0;
			if (dist > distEnd) {
				kvotBort = (dist - distEnd) / distNu;
				useKvotNu -= kvotBort;
			}

			//printf("nCheckPoints %d i %d tidTot %.2lf distNu %.2lf dist %.2lf distStart %.2lf distEnd %.2lf useKvotNu %.3lf\n", 
			//	model.weatherFunctions.nCheckPoints, i, tidTot, distNu, dist, distStart, distEnd, useKvotNu);
			//uVessel = sin(model.weatherFunctions.vesselBearing[i] * M_PI / 180);
			//vVessel = cos(model.weatherFunctions.vesselBearing[i] * M_PI / 180);

			//if (arcNr == 2578755)
			//	arcNr = arcNr;
			stormVarde = getStormValue((int)(tidTotArc), model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i], 1);// model.weatherFunctions.point[i]);
			if (stormVarde > model.functions.valuesNow.worstStormValue) {
				if (stormVarde > model.arc[arcNr].safetyHurricane && arcNr < model.nArcs)
					stormVarde = model.arc[arcNr].safetyHurricane; // to not create a high cost compared to initial arc generation
				if (stormVarde > model.functions.valuesNow.worstStormValue)
					model.functions.valuesNow.worstStormValue = stormVarde;
			}

			if (fixTime <= 0) {
				if (model.arc[arcNr].fromLevel == 89)
					arcNr = arcNr;
				if (arcNr == 277 && i == 2)
					i = i;
				getAllVariableValues(i, tidTotArc);

				tidIntForecast = getTidIntForecast(tidTotArc);

				if (tidTotArc0 < model.network.tidp_startHistoricDataOnly) {
					uCurrent = model.functions.varValue[model.functions.pos_current_u]; // getVariableValue(model.functions.pos_current_u, i, tidTot);
					vCurrent = model.functions.varValue[model.functions.pos_current_v]; // getVariableValue(model.functions.pos_current_v, i, tidTot);
				}
				else {
					if (delayVersion != 5) {
						delayNr = getDelayPosFrom_tidp(tidTotArc);
						getCurrent_fromCurrentDelayed(delayNr, model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i], &uCurrent, &vCurrent);
					}
					else {
						uCurrent = 0;
						vCurrent = 0;
					}
				}

				if (uCurrent < 1000 && vCurrent < 1000) {
					currentDirection = ApproxAtan2(vCurrent, uCurrent); // atan2(vCurrent, uCurrent);
					currentSpeed = sqrt(uCurrent * uCurrent + vCurrent * vCurrent);
					//if (tidTot >= model.weather[model.functions.pos_current_u].tidpHistoricalWeather)
					//	currentSpeed *= model.params.historicDataFactor_current;
				}
				else {
					currentDirection = 0;
					currentSpeed = 0;
					uCurrent = 9999;
					vCurrent = 9999;
				}

				baseGroundSpeed = eval_baseGroundSpeed(calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
					currentDirection, currentSpeed);
				if (printGlobal == 1) {
					printf("checkP %d vCurrent %.3lf uCurrent %.3lf, currentDirection %.3lf currentSpeed %.3lf baseGroundSpeed %.3lf\n", i, vCurrent,
						uCurrent, currentDirection, currentSpeed, baseGroundSpeed);
				}


				uWind = model.functions.varValue[model.functions.pos_wind_u]; // getVariableValue(model.functions.pos_wind_u, i, tidTot);
				vWind = model.functions.varValue[model.functions.pos_wind_v]; // getVariableValue(model.functions.pos_wind_v, i, tidTot);

				//errlog("arcNr %d i %d yx %.4lf %.4lf tidp %.2lf uWind %.2lf %.2lf vWind %.2lf %.2lf uCurr %.2lf %.2lf vCurr %.2lf %.2lf\n", arcNr, i,
				//	model.weatherFunctions.point_lat[i], model.weatherFunctions.point_lon[i], tidTot, uWind, uWind / 3.6, vWind, vWind / 3.6,
				//	uCurrent, uCurrent / 3.6, vCurrent, vCurrent / 3.6);

				if (uWind < 1000 && vWind < 1000) {
					windDirection = ApproxAtan2(vWind, uWind); // atan2(vWind, uWind);
					windSpeed2 = uWind * uWind + vWind * vWind;
					windSpeed = sqrt(windSpeed2);
					//if (tidTot >= model.weather[model.functions.pos_wind_u].tidpHistoricalWeather)
					//	windSpeed *= model.params.historicDataFactor_windSpeed;

					rel_windSpeed = eval_relWindSpeed(baseGroundSpeed, model.weatherFunctions.vesselBearing[i],
						windDirection, windSpeed, &rel_windDir);
#ifdef NAZANIN_SAFETY
					absWindDirDiff = eval_absWindDirDiff(model.weatherFunctions.vesselBearing[i], windDirection);
#endif

					if (printGlobal == 1) {
						printf("checkP %d vWind %.3lf uWind %.3lf, windDirection %.3lf windSpeed %.3lf rel_windSpeed %.3lf rel_windDir %.3lf\n", i, vWind,
							uWind, windDirection, windSpeed, rel_windSpeed, rel_windDir);
					}
					favorableWind = get_isWindFavorable(windSpeed, rel_windDir);
					//model.functions.valuesNow.worstStabilityValue += distNu * rel_windSpeed / 10000.0;
				}
				else {
					windSpeed = 0;
					windDirection = 0;
					rel_windDir = 0;
					rel_windSpeed = baseGroundSpeed;
					favorableWind = -1;
				}

				waveHeight = model.functions.varValue[model.functions.pos_waveHeight]; // getVariableValue(model.functions.pos_waveHeight, i, tidTot);
				wavePeriod = model.functions.varValue[model.functions.pos_wavePeriod]; // getVariableValue(model.functions.pos_wavePeriod, i, tidTot);
				waveDirection = model.functions.varValue[model.functions.pos_waveDirection]; // getVariableValue(model.functions.pos_waveDirection, i, tidTot);
				waveHeightBase = waveHeight;
				waveDirectionBase = waveDirection;

				if (waveHeight > 100) {
					waveHeight = 0;
					favorableWave = -1;
				}
				else
					favorableWave = 1;
				//else {
				//	if (tidTot >= model.weather[model.functions.pos_waveHeight].tidpHistoricalWeather)
				//		waveHeight *= model.params.historicDataFactor_waveHeight;
				//}
				if (wavePeriod > 1000)
					wavePeriod = 0;
				if (waveDirection > 1000) {
					waveDirection = 0;
					favorableWave = -1;
				}
				//rel_waveDir = (waveDirection -90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]; // / model.functions.nWaveDir;
				//rel_waveDir = M_PI - ((waveDirection - 90) * M_PI / 180 + model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;
				rel_waveDir = M_PI + ((270 - waveDirection) * M_PI / 180 - model.weatherFunctions.vesselBearing[i]); // / model.functions.nWaveDir;

				if (model.arc[arcNr].fromLevel == 14)
					arcNr = arcNr;
				if (rel_waveDir < 0)
					rel_waveDir = -rel_waveDir;
				if (rel_waveDir >= 2 * M_PI)
					rel_waveDir -= 2 * M_PI;
				if (rel_waveDir > M_PI)
					rel_waveDir = 2 * M_PI - rel_waveDir;
				if (printGlobal == 1) {
					printf("checkP %d waveDirection %.3lf rel_waveDir %.3lf waveHeight %.3lf wavePeriod %.3lf\n", i, waveDirection, rel_waveDir, waveHeight, wavePeriod);
				}

				//printGlobal = 1;
				// speedDiffWind = lookup_speedDiffWindTable(calmWaterSpeed, rel_windSpeed, rel_windDir);
				speedDiffWind = lookup_speedDiffWindTable(baseGroundSpeed, rel_windSpeed, rel_windDir);
				speedDiffWave = lookup_speedDiffWaveTable(calmWaterSpeed, waveHeight, wavePeriod, rel_waveDir);

				if (favorableWave >= 0)
					favorableWave = get_isSeaFavorable(waveHeight, rel_waveDir);

				//if (model.arc[arcNr].fromLevel == 76)
				//	printf("levels %d %d nodPos %d %d startKvot %.3lf wind\nshipSpeed_knots %lf\nwind_RWS_m_s %lf\nWind_RWiA_degrees %lf\nWind_WF_kts %lf\n"
				//		"wave\nshipSpeed_knots %lf\nwaveHeight_m %lf\nwavePeriod_s %lf\nwave_RWaA_degrees %lf\nwave_WF_kts %lf\n",
				//		model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].toPointNr, startKvot,
				//		baseGroundSpeed / model.params.knots_to_km, rel_windSpeed / 3.6, rel_windDir * 180 / M_PI,
				//		speedDiffWind / model.params.knots_to_km,
				//		calmWaterSpeed / model.params.knots_to_km, waveHeight, wavePeriod, rel_waveDir * 180 / M_PI,
				//		speedDiffWave / model.params.knots_to_km);
				speedDiffWindWave = speedDiffWind + speedDiffWave;
				
				// speedOverGround = baseGroundSpeed * model.params.knots_to_km - speedDiffWindWave; // in km/h
				if (model.delay.nYears > 0 && delayVersion >= 4)
					speedOverGround = calmWaterSpeed - speedDiffWindWave; // in km/h
				else {
					//if (USE_TIME_EXACT == 1)
					speedOverGround = baseGroundSpeed - speedDiffWindWave; // in km/h
					//else
					//	speedOverGround = model.arc[arcNr].distance / model.arc[arcNr].time;
				}
				if (speedOverGround < model.params.knots_to_km)
					speedOverGround = model.params.knots_to_km;

				if (delayFactor < 0) {
					timeArc = distNu / speedOverGround * useKvotNu; // in hours

					tidTotArc += distNu / speedOverGround;
					if (dist <= distStart) {
						continue;
					}

					timeArcSTW = distNu / calmWaterSpeed * useKvotNu; // in hours
					model.functions.valuesNow.WindF -= speedDiffWind * timeArcSTW;
					//if (arcNr == 175518) {
					//	printf("arcNr1a %d kvots %.4lf %.4lf wPoint %d time %.3lf baseTime %lf timeArc %.3lf SOG %lf BGS %lf CWS %lf sDiffWW %lf bearing %lf currDir %lf currSped %lf uv %lf %lf\n", arcNr, startKvot, endKvot,
					//		i, tidTot, tidTotArc, timeArc, speedOverGround, baseGroundSpeed, calmWaterSpeed, speedDiffWindWave,
					//		model.weatherFunctions.vesselBearing[i], currentDirection, currentSpeed, uCurrent, vCurrent);
					//}
					model.functions.valuesNow.WaveF -= speedDiffWave * timeArcSTW;
					model.functions.valuesNow.CurrentF += (baseGroundSpeed - calmWaterSpeed) * timeArcSTW;
					model.functions.valuesNow.totTimeArcSTW += timeArcSTW;
					model.functions.valuesNow.WindFArc -= speedDiffWind * timeArcSTW;
					model.functions.valuesNow.WaveFArc -= speedDiffWave * timeArcSTW;
					model.functions.valuesNow.CurrentFArc += (baseGroundSpeed - calmWaterSpeed) * timeArcSTW;

					model.functions.valuesNow.favorableWind[0][favorableWind + 1] += timeArc;
					model.functions.valuesNow.favorableWave[0][favorableWave + 1] += timeArc;
					if (favorableWind == 1 && favorableWave == 1)
						model.functions.valuesNow.favorableWindWave[0][1 + 1] += timeArc;
					else {
						if (favorableWind == -1 || favorableWave == -1)
							model.functions.valuesNow.favorableWindWave[0][0] += timeArc;
						else
							model.functions.valuesNow.favorableWindWave[0][1] += timeArc;
					}

					//if (SKRIV_UT_NOTHING == 0) {
					//	errlog("aaa %d %lf %.2lf %d %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf"
					//		" %lf %lf %lf %lf\n",
					//		arcNr, startKvot, endKvot, i, tidTot, delayFactor, calmWaterSpeed / model.params.knots_to_km, speedOverGround / model.params.knots_to_km,
					//		distNu / model.params.knots_to_km * useKvotNu, timeArcSTW, timeArc, -speedDiffWind* timeArcSTW / model.params.knots_to_km,
					//		-speedDiffWave * timeArcSTW / model.params.knots_to_km,
					//		model.functions.valuesNow.CurrentF / model.params.knots_to_km, 0.0,
					//		(-speedDiffWind * timeArcSTW - speedDiffWave * timeArcSTW + (baseGroundSpeed - calmWaterSpeed) * timeArcSTW) / model.params.knots_to_km,
					//		(-speedDiffWind * timeArcSTW - speedDiffWave * timeArcSTW + (baseGroundSpeed - calmWaterSpeed) * timeArcSTW) / distNu,
					//		-speedDiffWind / model.params.knots_to_km, -speedDiffWave / model.params.knots_to_km,
					//		(baseGroundSpeed - calmWaterSpeed) / model.params.knots_to_km, 0.0);
					//	glob_tmpTotDist += model.functions.valuesNow.WaveF;
					//}
				}
				else {
					timeArc = distNu / calmWaterSpeed * delayFactor * useKvotNu;

					tidTotArc += distNu / calmWaterSpeed * delayFactor;
					if (dist <= distStart) {
						continue;
					}

					timeArcSTW = distNu / calmWaterSpeed * useKvotNu; // in hours

					model.functions.valuesNow.favorableWind[1][favorableWind + 1] += timeArc;
					model.functions.valuesNow.favorableWave[1][favorableWave + 1] += timeArc;
					if (favorableWind == 1 && favorableWave == 1)
						model.functions.valuesNow.favorableWindWave[1][1 + 1] += timeArc;
					else {
						if (favorableWind == -1 || favorableWave == -1)
							model.functions.valuesNow.favorableWindWave[1][0] += timeArc;
						else
							model.functions.valuesNow.favorableWindWave[1][1] += timeArc;
					}

					tidInt = (int)(tidTotArc) / model.params.tIndexGerH;
					checkFactor = eval_factorDelayedAlongArc_currSpeedDiff(model.arc[arcNr].fromLevel, model.arc[arcNr].fromPointNr, model.arc[arcNr].toLevel, model.arc[arcNr].toPointNr,
						tidInt, &checkSpeedDiffCurrent, calmWaterSpeed);

					//speedNu = calmWaterSpeed / checkFactor + checkSpeedDiffCurrent;
					//tidNu = distNu / speedNu * useKvotNu;
					//tidCalmWater = distNu / calmWaterSpeed * useKvotNu;
					//calcDelayFactor = tidNu / tidCalmWater;

					//printf("check av delayFactor. used %.3lf eval factor %.3lf speedDiff %.3lf result %.3lf diff %.3lf arcNr %d endKvot %.3lf\n",
					//	delayFactor, checkFactor, checkSpeedDiffCurrent, calcDelayFactor, calcDelayFactor - delayFactor, arcNr, endKvot);

					model.functions.valuesNow.CurrentF += checkSpeedDiffCurrent * timeArcSTW;
					model.functions.valuesNow.CurrentFArc += checkSpeedDiffCurrent * timeArcSTW;

					model.functions.valuesNow.DelayF += (calmWaterSpeed * (1 / delayFactor - 1) - checkSpeedDiffCurrent) * timeArcSTW;
					model.functions.valuesNow.totTimeArcSTW += timeArcSTW;
					model.functions.valuesNow.DelayFArc += (calmWaterSpeed * (1 / delayFactor - 1) - checkSpeedDiffCurrent) * timeArcSTW;
					//model.functions.valuesNow.DelayFArc += calmWaterSpeed * (1 / delayFactor - 1) * timeArcSTW;

					if (SKRIV_UT_NOTHING == 100) {
						errlog("bbb %d %lf %.2lf %d %lf delayFactor %lf %lf %lf"
							" %lf %lf %lf %lf %lf %lf %lf %lf %lf"
							" %lf %lf %lf %lf\n",
							arcNr, startKvot, endKvot, i, tidTot, delayFactor, calmWaterSpeed, delayFactor,
							distNu / model.params.knots_to_km * useKvotNu, timeArcSTW, timeArc, checkSpeedDiffCurrent,
							0.0, 0.0, (calmWaterSpeed * (1 / delayFactor - 1) - checkSpeedDiffCurrent) * timeArcSTW,
							(calmWaterSpeed * (1 / delayFactor - 1) * timeArcSTW) / model.params.knots_to_km,
							(calmWaterSpeed * (1 / delayFactor - 1) * timeArcSTW) / distNu, 0.0, 0.0, 0.0,
							calmWaterSpeed * (1 / delayFactor - 1) / model.params.knots_to_km);
						//glob_tmpTotDist += (model.functions.valuesNow.DelayF) / model.params.knots_to_km;
					}
				}

				if (model.functions.valuesNow.maxCurrent < currentSpeed)
					model.functions.valuesNow.maxCurrent = currentSpeed;
				if (model.params.useSimulering == 1) {
					if (model.functions.valuesNow.maxWindSpeed < windSpeed) {
						if (windSpeed > model.params.user_maxWindSpeed_kmh && windSpeed > model.arc[arcNr].maxWindSpeed)
							windSpeed = model.arc[arcNr].maxWindSpeed;
					}
					if (model.functions.valuesNow.maxWaveHeight < waveHeight) {
						if (waveHeight > model.params.user_maxWaveHeight && waveHeight > model.arc[arcNr].maxWaveHeight)
							waveHeight = model.arc[arcNr].maxWaveHeight;
					}
				}
				if (model.functions.valuesNow.maxWindSpeed < windSpeed &&
					model.weather[model.functions.pos_wind_u].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_wind_u].nTimeIntervals_forecast) {
					model.functions.valuesNow.maxWindSpeed = windSpeed;
					model.functions.valuesNow.maxWindSpeed_tp = tidTot;
					model.functions.valuesNow.maxWindSpeed_dir = windDirection * 180 / M_PI;
				}
				if (model.functions.valuesNow.maxWaveHeight < waveHeight &&
					model.weather[model.functions.pos_waveHeight].timeIntervalIndex[tidIntForecast] < model.weather[model.functions.pos_waveHeight].nTimeIntervals_forecast) {
					model.functions.valuesNow.maxWaveHeight = waveHeight;
					model.functions.valuesNow.maxWaveHeight_tp = tidTot;
					model.functions.valuesNow.maxWaveHeight_dir = 270 - waveDirection;
					if (model.functions.valuesNow.maxWaveHeight > 9)
						model.functions.valuesNow.maxWaveHeight = model.functions.valuesNow.maxWaveHeight;
				}

				model.functions.valuesNow.speedOnWater = calmWaterSpeed;
				model.functions.valuesNow.sumWindSpeed += windSpeed * timeArc;
				model.functions.valuesNow.sumRelCurrent += (baseGroundSpeed - calmWaterSpeed) * timeArc;
				model.functions.valuesNow.sumCurrent += currentSpeed * timeArc;
				model.functions.valuesNow.sumWaveHeight += waveHeight * timeArc;

				//errlog("arcNr %d i %d calmWaterSpeed %.3lf bearing %.3lf, currDir %.3lf currSpeed %.3lf baseGroundSpeed %.3lf"
				//	" rel_windSpeed %.3lf rel_windDir %.3lf speedDiffWind %.3lf waveHeight %.3lf"
				//	" wavePeriod %.3lf rel_waveDir %.3lf speedDiffWave %.3lf speedOverGround %.3lf distNu %.3lf timeArc %.3lf\n",
				//	arcNr, i, calmWaterSpeed, model.weatherFunctions.vesselBearing[i],
				//	currentDirection, currentSpeed, baseGroundSpeed, rel_windSpeed, rel_windDir, speedDiffWind, waveHeight, wavePeriod,
				//	rel_waveDir, speedDiffWave, speedOverGround, distNu, timeArc);

				//printf("timeArc %.2lf distNu %.2lf speedOverGround %.2lf useKvotNu %.2lf\n", timeArc, distNu, speedOverGround, useKvotNu);

				//if (arcNr == 96)
				//	errlog("error: nArcs %d tidTot %.2lf UTCsec %.0lf dist %.2lf calmWaterSpeed %.2lf worstStormValue %.2lf vesselBearing %.2lf currDir %.2lf "
				//		"currSpeed %.2lf uCurr %.2lf vCurr %.2lf baseGroundSpeed %.2lf rel_windSpeed %.2lf rel_windDir %.2lf uWind %.2lf vWind %.2lf waveHeight %.2lf "
				//		"wavePeriod %.2lf rel_waveDir %.2lf speedDiffWave %.2lf speedDiffWindWave %.2lf speedOverGround %.2lf timeArc %.2lf\n",
				//		arcNr, tidTot, model.params.UTC_secondsStart + tidTot * 3600, dist, calmWaterSpeed, model.functions.valuesNow.worstStormValue, 
				//		model.weatherFunctions.vesselBearing[i],
				//		currentDirection, currentSpeed, uCurrent, vCurrent, baseGroundSpeed, rel_windSpeed, rel_windDir, uWind, vWind, waveHeight, wavePeriod,
				//		rel_waveDir, speedDiffWave, speedDiffWindWave, speedOverGround, timeArc);

				//printf("arcNr %d i %d dist %.2lf speedSetting %d bearing %.3lf calmWaterSpeed %.2lf worstStormValue %.2lf vesselBearing %.4lf currDir %.2lf\n"
				//	"currSpeed %.2lf uCurr %.2lf vCurr %.2lf baseGroundSpeed %.4lf uWind %.4lf vWind %.4lf rel_windSpeed %.4lf rel_windDir %.4lf\n"
				//	"waveHeight %.4lf wavePeriod %.4lf waveDir %.4lf rel_waveDir %.4lf speedDiffWind %.4lf speedDiffWave %.4lf speedOverGround %.4lf timeArc %.2lf\n", arcNr, i,
				//	dist, model.arc[arcNr].speedSetting, model.weatherFunctions.vesselBearing[i], calmWaterSpeed, model.functions.valuesNow.worstStormValue, model.weatherFunctions.vesselBearing[i],
				//	currentDirection, currentSpeed, uCurrent, vCurrent, baseGroundSpeed, uWind, vWind, rel_windSpeed, rel_windDir, waveHeight, wavePeriod,
				//	waveDirection, rel_waveDir, speedDiffWind, speedDiffWave, speedOverGround, timeArc);

				fuelConsumption_main = eval_fuelConsumption_both(model.arc[arcNr].speedSetting, &fuelConsumption_aux,
					model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				//if (model.arc[arcNr].fromLevel < 0) {
				//	if (model.network.channel[-model.arc[arcNr].fromLevel - 1].totalConsumption >= 0)
				//		fuelConsumption_main = model.network.channel[-model.arc[arcNr].fromLevel - 1].totalConsumption;
				//}
				if (arcNr == 22)
					arcNr = arcNr; // checkpfg
				fuelUsage_main = fuelConsumption_main * timeArc;
				fuelUsage_aux = fuelConsumption_aux * timeArc;
				if (model.arc[arcNr].fromLevel < 0 && model.arc[arcNr].toLevel < 0) {
					fuelUsage_main += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_main;
					// fuelUsage_aux -= fuelConsumption_aux * model.network.channel[-model.arc[arcNr].fromLevel - 1].waitingTime;
					fuelUsage_aux += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_aux;
				}


				fuelTot_main += fuelUsage_main;
				fuelTot_aux += fuelUsage_aux;

				if (printGlobal == 1) {
					printf("speedDiffWindWave %.2lf %.2lf speedOverGround %.2lf timeArc %.2lf distArc %.2lf\n",
						speedDiffWind, speedDiffWave, speedOverGround, timeArc, distNu);
				}

				model.functions.valuesNow.forecastType += identifyForecastType(tidTotArc) * timeArc;

				tidTot += timeArc;
				
				iceCover = model.functions.varValue[model.functions.pos_iceThickness]; // getVariableValue(model.functions.pos_iceThickness, i, tidTot);
				if (iceCover > 1000)
					iceCover = 0;
				if (iceCover > model.functions.valuesNow.iceCover_max)
					model.functions.valuesNow.iceCover_max = iceCover;

#ifdef NAZANIN_SAFETY
				eval_safety_nazanin(speedOverGround, (calmWaterSpeed) - speedDiffWindWave, windSpeed, absWindDirDiff, waveHeight, wavePeriod, rel_waveDir, iceCover);
#else
				eval_safety(iceCover);
#endif
				//if (model.functions.valuesNow.bowSlam > model.functions.valuesNow.bowSlamming_max)
				//	model.functions.valuesNow.bowSlamming_max = model.functions.valuesNow.bowSlam;
				//if (model.functions.valuesNow.greenWater > model.functions.valuesNow.greenWater_max)
				//	model.functions.valuesNow.greenWater_max = model.functions.valuesNow.greenWater;
				//if (model.functions.valuesNow.dynamicStability > model.functions.valuesNow.dynamicStability_max)
				//	model.functions.valuesNow.dynamicStability_max = model.functions.valuesNow.dynamicStability;

				model.functions.valuesNow.current += timeArc * (baseGroundSpeed - calmWaterSpeed);
				if (printGlobal == 1)
					printf(" == arcNr %d timeArc %.2lf baseGroundSpeed %.3lf calmWaterSpeed %.3lf current %.3lf\n",
						arcNr, timeArc, baseGroundSpeed, calmWaterSpeed, model.functions.valuesNow.current);
				if (uCurrent < 1000 && vCurrent < 1000) {
					currentReal_u += timeArc * uCurrent;
					currentReal_v += timeArc * vCurrent;
					timeArc_current += timeArc;
				}

				model.functions.valuesNow.windSpeed += timeArc * rel_windSpeed; // windSpeed;

				if (uWind < 1000 && vWind < 1000) {
					windSpeedReal_u += timeArc * uWind;
					windSpeedReal_v += timeArc * vWind;
					timeArc_wind += timeArc;
				}
				model.functions.valuesNow.waveHeight += timeArc * waveHeight;
				model.functions.valuesNow.wavePeriod += timeArc * wavePeriod;

				model.functions.valuesNow.speedDiffWind += speedDiffWind * timeArc;
				model.functions.valuesNow.speedDiffWave += speedDiffWave * timeArc;
				model.functions.valuesNow.baseGroundSpeed += baseGroundSpeed * timeArc;

				if (waveHeightBase <= 100 && waveDirectionBase <= 1000) {
					//waveHeightReal_u += timeArc * waveHeight * cos((90 - waveDirection) / 180 * M_PI);
					//waveHeightReal_v += timeArc * waveHeight * sin((90 - waveDirection) / 180 * M_PI);
					waveHeightReal_u += timeArc * waveHeight * lookUpCos((270 - waveDirection) / 180 * M_PI);
					waveHeightReal_v += timeArc * waveHeight * lookUpSin((270 - waveDirection) / 180 * M_PI);
					timeArc_wave += timeArc;
				}
				else
					timeArc = timeArc;

				//if (printGlobal == 1)
				//	printf("wavePeriod %.3lf timeArc %.3lf tidTot %.3lf tot %.3lf\n", wavePeriod, timeArc, tidTot- timeExact, model.functions.valuesNow.wavePeriod);

				//printf("arcNr %d pos %d baseGroundSpeed %.2lf calmWaterSpeed %.2lf timeArc %.2lf currentAcc %.2lf\n", arcNr, i,
				//	baseGroundSpeed, calmWaterSpeed, timeArc, model.functions.valuesNow.current);
				//windSpeed_x += timeArc * windSpeed * cos(rel_windDir);
				//windSpeed_y += timeArc * windSpeed * sin(rel_windDir);
				windSpeed_x += timeArc * rel_windSpeed * lookUpCos(rel_windDir);
				windSpeed_y += timeArc * rel_windSpeed * lookUpSin(rel_windDir);
				waveDir_x += timeArc * waveHeight * lookUpCos(rel_waveDir);
				waveDir_y += timeArc * waveHeight * lookUpSin(rel_waveDir);

				if (model.nWeatherFiles > 8) {
					if (model.functions.pos_pressureSurface >= 0 && model.functions.varValue[model.functions.pos_pressureSurface] > 10000) {
						model.functions.valuesNow.pressureSurface += timeArc * model.functions.varValue[model.functions.pos_pressureSurface];
						model.functions.valuesNow.timePressureSurface += timeArc;
					}
					if (model.functions.pos_pressureAir >= 0 && model.functions.varValue[model.functions.pos_pressureAir] < 9998) {
						model.functions.valuesNow.pressureAir += timeArc * model.functions.varValue[model.functions.pos_pressureAir];
						model.functions.valuesNow.timePressureAir += timeArc;
					}
					if (model.functions.pos_precipitation >= 0 && model.functions.varValue[model.functions.pos_precipitation] < 9998) {
						model.functions.valuesNow.precipitation += timeArc * model.functions.varValue[model.functions.pos_precipitation];
						model.functions.valuesNow.timePrecipitation += timeArc;
					}
					if (model.functions.pos_tempSea >= 0 && model.functions.varValue[model.functions.pos_tempSea] < 9998) {
						model.functions.valuesNow.tempSea += timeArc * model.functions.varValue[model.functions.pos_tempSea];
						model.functions.valuesNow.timeTempSea += timeArc;
					}
					if (model.functions.pos_tempAir >= 0 && model.functions.varValue[model.functions.pos_tempAir] < 9998) {
						model.functions.valuesNow.tempAir += timeArc * model.functions.varValue[model.functions.pos_tempAir];
						model.functions.valuesNow.timeTempAir += timeArc;
					}
					if (model.functions.pos_cloudCover >= 0 && model.functions.varValue[model.functions.pos_cloudCover] < 9998) {
						model.functions.valuesNow.cloudCover += timeArc * model.functions.varValue[model.functions.pos_cloudCover];
						model.functions.valuesNow.timeCloudCover += timeArc;
					}
					if (model.functions.pos_mdps >= 0 && model.functions.varValue[model.functions.pos_mdps] < 9998) {
						model.functions.valuesNow.mdps += timeArc * model.functions.varValue[model.functions.pos_mdps];
						model.functions.valuesNow.timeMdps += timeArc;
					}
					if (model.functions.pos_swell >= 0 && model.functions.varValue[model.functions.pos_swell] < 9998) {
						model.functions.valuesNow.swell += timeArc * model.functions.varValue[model.functions.pos_swell];
						model.functions.valuesNow.timeSwell += timeArc;
					}
				}

			}
			else {
				// channel with fix speed....
				timeArc = distNu / calmWaterSpeed * useKvotNu; // in hours

				tidTotArc += distNu / calmWaterSpeed;
				if (dist <= distStart) {
					continue;
				}

				model.functions.valuesNow.favorableWind[1][0] += timeArc;
				model.functions.valuesNow.favorableWave[1][0] += timeArc;
				model.functions.valuesNow.favorableWindWave[1][0] += timeArc;

				//if (model.arc[arcNr].fromLevel < 0) {
				//	if (model.network.channel[-model.arc[arcNr].fromLevel - 1].totalConsumption < 0) {
				//		fuelConsumption_main = eval_fuelConsumption_both(model.arc[arcNr].speedSetting, &fuelConsumption_aux,
				//			model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				//		fuelUsage_main = fuelConsumption_main * timeArc;
				//	}
				//	else {
				//		fuelConsumption_main = eval_fuelConsumption_both(model.functions.speedSetting95MCR_use, &fuelConsumption_aux, -1, -100);
				//		fuelConsumption_main = model.network.channel[-model.arc[arcNr].fromLevel - 1].totalConsumption;
				//		fuelUsage_main = fuelConsumption_main / model.weatherFunctions.nCheckPoints; // *timeArc;
				//	}

				//}
				//else {
				fuelConsumption_main = eval_fuelConsumption_both(model.arc[arcNr].speedSetting, &fuelConsumption_aux,
					model.arc[arcNr].fromLevel, model.arc[arcNr].toLevel);
				fuelUsage_main = fuelConsumption_main * timeArc;
				//}

				fuelUsage_aux = fuelConsumption_aux * timeArc;

				if (model.arc[arcNr].fromLevel < 0 && i == 0) {
					fuelUsage_main += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_main;
					//fuelUsage_aux -= fuelConsumption_aux * model.network.channel[-model.arc[arcNr].fromLevel - 1].waitingTime; already removed...
					fuelUsage_aux += model.network.channel[-model.arc[arcNr].fromLevel - 1].waiting_consumption_aux;
				}

				fuelTot_main += fuelUsage_main;
				fuelTot_aux += fuelUsage_aux;
				model.functions.valuesNow.forecastType += identifyForecastType(tidTotArc) * timeArc;

				tidTot += timeArc;

			}
		}

		model.functions.valuesNow.fuel_aux = fuelTot_aux;
		model.functions.valuesNow.fuel_main = fuelTot_main;
		deltaTid = tidTot - timeExact;
		if (deltaTid > 0) {
			model.functions.valuesNow.current /= deltaTid;
			model.functions.valuesNow.windSpeed /= deltaTid;
			model.functions.valuesNow.waveHeight /= deltaTid;
			//if (model.functions.valuesNow.waveHeight > 3.3)
			//	model.functions.valuesNow.waveHeight = model.functions.valuesNow.waveHeight;
			model.functions.valuesNow.wavePeriod /= deltaTid;
			model.functions.valuesNow.speedDiffWind /= deltaTid;
			model.functions.valuesNow.speedDiffWave /= deltaTid;
			model.functions.valuesNow.baseGroundSpeed /= deltaTid;

			if (timeArc_current > 0.001) {
				currentReal_u /= timeArc_current;
				currentReal_v /= timeArc_current;
				model.functions.valuesNow.currentReal = sqrt(currentReal_u * currentReal_u + currentReal_v * currentReal_v);
				model.functions.valuesNow.currentDirReal = ApproxAtan2(currentReal_v, currentReal_u) * 180.0 / M_PI;
			}
			else {
				model.functions.valuesNow.currentReal = 9999;
				model.functions.valuesNow.currentDirReal = 0;
			}

			if (timeArc_wind > 0.001) {
				windSpeedReal_u /= timeArc_wind;
				windSpeedReal_v /= timeArc_wind;
				model.functions.valuesNow.windReal = sqrt(windSpeedReal_u * windSpeedReal_u + windSpeedReal_v * windSpeedReal_v);
				model.functions.valuesNow.windDirReal = ApproxAtan2(windSpeedReal_v, windSpeedReal_u) * 180.0 / M_PI;
			}
			else {
				model.functions.valuesNow.windReal = 9999;
				model.functions.valuesNow.windDirReal = 0;
			}

			if (timeArc_wave > 0.001)
				model.functions.valuesNow.waveDirReal = ApproxAtan2(waveHeightReal_v, waveHeightReal_u) * 180.0 / M_PI;
			else
				model.functions.valuesNow.waveDirReal = 9999;

			//if (printGlobal == 1)
			//	printf("wavePeriod %.3lf timeArc %.3lf\n", model.functions.valuesNow.wavePeriod, deltaTid);
			model.functions.valuesNow.forecastType /= (deltaTid * 8);

			if (model.nWeatherFiles > 8) {
				if (model.functions.valuesNow.timePressureSurface > 0)
					model.functions.valuesNow.pressureSurface /= model.functions.valuesNow.timePressureSurface;
				if (model.functions.valuesNow.timePressureAir > 0)
					model.functions.valuesNow.pressureAir /= model.functions.valuesNow.timePressureAir;
				if (model.functions.valuesNow.timePrecipitation > 0)
					model.functions.valuesNow.precipitation /= model.functions.valuesNow.timePrecipitation;
				if (model.functions.valuesNow.timeTempSea > 0)
					model.functions.valuesNow.tempSea /= model.functions.valuesNow.timeTempSea;
				if (model.functions.valuesNow.timeTempAir > 0)
					model.functions.valuesNow.tempAir /= model.functions.valuesNow.timeTempAir;
				if (model.functions.valuesNow.timeCloudCover > 0)
					model.functions.valuesNow.cloudCover /= model.functions.valuesNow.timeCloudCover;
				if (model.functions.valuesNow.timeMdps > 0)
					model.functions.valuesNow.mdps /= model.functions.valuesNow.timeMdps;
				if (model.functions.valuesNow.timeSwell > 0)
					model.functions.valuesNow.swell /= model.functions.valuesNow.timeSwell;
			}

		}
		else
		{
			model.functions.valuesNow.currentReal = 0;
			model.functions.valuesNow.currentDirReal = 0;
			model.functions.valuesNow.windReal = 9999;
			model.functions.valuesNow.windDirReal = 0;
			model.functions.valuesNow.waveDirReal = 0;
		}

		if (printGlobal == 1)
			printf(" == arcNr %d tot deltaTid %.3lf current %.3lf\n",
				arcNr, deltaTid, model.functions.valuesNow.current);
		if (arcNr == 7174)
			arcNr = arcNr;
		model.functions.valuesNow.relWindDir = ApproxAtan2(windSpeed_y, windSpeed_x) * 180.0 / M_PI;
		if (model.functions.valuesNow.relWindDir < 0)
			model.functions.valuesNow.relWindDir = -model.functions.valuesNow.relWindDir;
		//printf("wind_y %.2lf wind_x %.2lf relWindDir %.2lf\n",
		//	windSpeed_y, windSpeed_x, model.functions.valuesNow.relWindDir);
		model.functions.valuesNow.relWaveDir = ApproxAtan2(waveDir_y, waveDir_x) * 180.0 / M_PI;
		if (model.functions.valuesNow.relWaveDir < 0)
			model.functions.valuesNow.relWaveDir = -model.functions.valuesNow.relWaveDir;
	}

	model.functions.valuesNow.totWindF += model.functions.valuesNow.WindF;
	model.functions.valuesNow.totWaveF += model.functions.valuesNow.WaveF;
	model.functions.valuesNow.totCurrentF += model.functions.valuesNow.CurrentF;
	model.functions.valuesNow.totDelayF += model.functions.valuesNow.DelayF;

	model.functions.valuesNow.totFavorableWind_h += model.functions.valuesNow.favorableWind_h;
	model.functions.valuesNow.totFavorableWave_h += model.functions.valuesNow.favorableWave_h;
	model.functions.valuesNow.totFavorableWindWave_h += model.functions.valuesNow.favorableWindWave_h;


	//if (arcNr == 412305)
	//	errlog("arcNr %d startTime %.3lf endTime %.3lf in ..Section timeArc %.3lf\n", arcNr, timeExact, tidTot, (tidTot - timeExact) / (endKvot - startKvot));
	return tidTot;
}
