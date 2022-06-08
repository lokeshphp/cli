
#ifndef RASTER_CPP
#define RASTER_CPP

#include "iostream"
#include "string"
#include "gdal_priv.h"
#include "cpl_conv.h"
#include "gdalwarper.h"
#include "stdlib.h"
#include "gdal.h"



int errlog(const char* format, ...);

using namespace std;
typedef std::string String;

struct strBoundBox {
	double xMin;
	double yMin;
	double xMax;
	double yMax;
};


class Raster { 

private: // NOTE: "private" keyword is redundant here.  
		 // we place it here for emphasis. Because these
		 // variables are declared outside of "public", 
		 // they are private. 

	const char* filename;        // name of Geotiff
	GDALDataset *rasterDataset; // Geotiff GDAL datset object. 
	double geotransform[6];      // 6-element geotranform array.
	int dimensions[3];           // X,Y, and Z dimensions. 
	int NROWS, NCOLS, NLEVELS;     // dimensions of data in Geotiff. 
	double max_lat, min_lat, min_lon, max_lon, size_row, size_col;

public:
	struct strPhysRaster {
		unsigned short* valueCell;
		long long nCols;
		long long nRows;
		double size_col;
		double size_row;
		double minLongitude; // xMinUse;
		double minLatitude;// yMinUse;
		double maxLongitude; // xMaxUse;
		double maxLatitude;// yMaxUse;
		//double xMaxUse;
		//double yMaxUse;
	};

	struct strWeatherRaster {
		float* valueCell;
		long long nCols;
		long long nRows;
		double size_col;
		double size_row;
		double minLongitude; // xMinUse;
		double minLatitude;// yMinUse;
		double maxLongitude; // xMaxUse;
		double maxLatitude;// yMaxUse;
		int nTimeIntervals;
		long long* secondsUTC;
		//double xMaxUse;
		//double yMaxUse;
	};

	int checkMinnesAnvandning(int rad)
	{
		int varde = 1;
		//varde = _CrtCheckMemory();
		if (varde != 1)
			printf("Error! Minnesbugg identifierad pa rad %d\n", rad);
		return varde;
	}



	// define constructor function to instantiate object
	// of this Raster class. 
	Raster() {
		GDALAllRegister();
	}

	void open(const char* tiffname) {
		filename = tiffname;

		// set pointer to Geotiff dataset as class member.  
		rasterDataset = (GDALDataset*)GDALOpen(filename, GA_ReadOnly);
		if (rasterDataset == NULL) {
			errlog("ERROR! Raster %s cannot be open. Fix it and run again\n", tiffname);
			exit(0);
		}

		// set the dimensions of the Geotiff 
		NROWS = GDALGetRasterYSize(rasterDataset);
		NCOLS = GDALGetRasterXSize(rasterDataset);
		NLEVELS = GDALGetRasterCount(rasterDataset);
		rasterDataset->GetGeoTransform(geotransform);
		max_lat = geotransform[3]; // +geotransform[5] * NROWS;
		min_lat = geotransform[3] + geotransform[5] * NROWS;
		min_lon = geotransform[0];
		max_lon = geotransform[0] + geotransform[1] * NCOLS;
		size_row = -geotransform[5];
		size_col = geotransform[1];
	}

	/*
	void openReadClose(const char* tiffname, strPhysRaster* rasterData) {
		filename = tiffname;
		
		// set pointer to Geotiff dataset as class member.  
		rasterDataset = (GDALDataset*)GDALOpen(filename, GA_ReadOnly);
		if (rasterDataset == NULL) {
			errlog("ERROR! Raster %s cannot be open. Fix it and run again\n", tiffname);
			exit(0);
		}

		// set the dimensions of the Geotiff 
		NROWS = GDALGetRasterYSize(rasterDataset);
		NCOLS = GDALGetRasterXSize(rasterDataset);
		NLEVELS = GDALGetRasterCount(rasterDataset);
		rasterDataset->GetGeoTransform(geotransform);
		max_lat = geotransform[3]; // +geotransform[5] * NROWS;
		min_lon = geotransform[0];
		max_lon = geotransform[0] + geotransform[1] * NCOLS;
		size_row = -geotransform[5];
		size_col = geotransform[1];
	}
	*/

	// define destructor function to close dataset, 
	// for when object goes out of scope or is removed
	// from memory. 
	~Raster() {
		// close the Geotiff dataset, free memory for array.  
		GDALClose(rasterDataset);
		// GDALDestroyDriverManager();
	}

	const char *GetFileName() {
		/*
		 * function GetFileName()
		 * This function returns the filename of the Geotiff.
		 */
		return filename;
	}

	const char *GetProjection() {
		/* function const char* GetProjection():
		 *  This function returns a character array (string)
		 *  for the projection of the geotiff file. Note that
		 *  the "->" notation is used. This is because the
		 *  "geotiffDataset" class variable is a pointer
		 *  to an object or structure, and not the object
		 *  itself, so the "." dot notation is not used.
		 */
		return rasterDataset->GetProjectionRef();
	}

	double *GetGeoTransform() {
		/*
		 * function double *GetGeoTransform()
		 *  This function returns a pointer to a double that
		 *  is the first element of a 6 element array that holds
		 *  the geotransform of the geotiff.
		 */
		rasterDataset->GetGeoTransform(geotransform);
		return geotransform;
	}

	double GetNoDataValue() {
		/*
		 * function GetNoDataValue():
		 *  This function returns the NoDataValue for the Geotiff dataset.
		 *  Returns the NoData as a double.
		 */
		return (double)rasterDataset->GetRasterBand(1)->GetNoDataValue();
	}

	int Get_nRows() {
		return NROWS;
	}
	int Get_nCols() {
		return NCOLS;
	}
	int Get_nBands() {
		return NLEVELS;
	}
	double Get_maxLatitude() {
		return max_lat;
	}
	double Get_minLongitude() {
		return min_lon;
	}
	double Get_maxLongitude() {
		return max_lon;
	}
	double Get_sizeRow() {
		return size_row;
	}
	double Get_sizeCol() {
		return size_col;
	}

	int *GetDimensions() {
		/*
		 * int *GetDimensions():
		 *
		 *  This function returns a pointer to an array of 3 integers
		 *  holding the dimensions of the Geotiff. The array holds the
		 *  dimensions in the following order:
		 *   (1) number of columns (x size)
		 *   (2) number of rows (y size)
		 *   (3) number of bands (number of bands, z dimension)
		 */
		dimensions[0] = NROWS;
		dimensions[1] = NCOLS;
		dimensions[2] = NLEVELS;
		return dimensions;
	}

	float** GetRasterBand(int z) {

		/*
		 * function float** GetRasterBand(int z):
		 * This function reads a band from a geotiff at a
		 * specified vertical level (z value, 1 ...
		 * n bands). To this end, the Geotiff's GDAL
		 * data type is passed to a switch statement,
		 * and the template function GetArray2D (see below)
		 * is called with the appropriate C++ data type.
		 * The GetArray2D function uses the passed-in C++
		 * data type to properly read the band data from
		 * the Geotiff, cast the data to float**, and return
		 * it to this function. This function returns that
		 * float** pointer.
		 */

		float** bandLayer = new float* [NROWS];
		//printf("alloced %d to bandlayer nrows\n", NROWS);
		switch (GDALGetRasterDataType(rasterDataset->GetRasterBand(z))) {
		case 0:
			return NULL; // GDT_Unknown, or unknown data type.
		case 1:
			// GDAL GDT_Byte (-128 to 127) - unsigned  char
			return GetArray2D<unsigned char>(z, bandLayer);
		case 2:
			// GDAL GDT_UInt16 - short
			return GetArray2D<unsigned short>(z, bandLayer);
		case 3:
			// GDT_Int16
			return GetArray2D<short>(z, bandLayer);
		case 4:
			// GDT_UInt32
			return GetArray2D<unsigned int>(z, bandLayer);
		case 5:
			// GDT_Int32
			return GetArray2D<int>(z, bandLayer);
		case 6:
			// GDT_Float32
			return GetArray2D<float>(z, bandLayer);
		case 7:
			// GDT_Float64
			return GetArray2D<double>(z, bandLayer);
		default:
			break;
		}
		return NULL;
	}

	int GetRasterBand_noAlloc(int z, float** bandLayer, float* rowBuff) {

		/*
		 * function float** GetRasterBand(int z):
		 * This function reads a band from a geotiff at a
		 * specified vertical level (z value, 1 ...
		 * n bands). To this end, the Geotiff's GDAL
		 * data type is passed to a switch statement,
		 * and the template function GetArray2D (see below)
		 * is called with the appropriate C++ data type.
		 * The GetArray2D function uses the passed-in C++
		 * data type to properly read the band data from
		 * the Geotiff, cast the data to float**, and return
		 * it to this function. This function returns that
		 * float** pointer.
		 */

		//float** bandLayer = new float*[NROWS];
		GetArray2D_noAlloc(z, bandLayer, rowBuff);
		return 0;
	}

	unsigned short* GetArray1D(int layerIndex, unsigned short* bandLayer, int xSize, int ySize) {

		 // get the raster data type (ENUM integer 1-12, 
		 // see GDAL C/C++ documentation for more details)        
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		// get number of bytes per pixel in Geotiff
		int nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
		//int nbytes = GDALGetDataTypeSizeBytes(bandType);

		//printf("nbytes %d\n", nbytes);
		// allocate pointer to memory block for one row (scanline) 
		// in 2D Geotiff array.  
		unsigned short* rowBuff = (unsigned short*)CPLMalloc(nbytes * NCOLS);


		for (long long row = 0; row < NROWS; row++) {     // iterate through rows
			//printf("row %d assign\n", row);

		  // read the scanline into the dynamically allocated row-buffer       
			CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
				GF_Read, 0, row, NCOLS, 1, rowBuff, NCOLS, 1, bandType, 0, 0);
			if (!(e == 0)) {
				cout << "Warning: Unable to read scanline in Raster!" << endl;
				exit(1);
			}

			//printf("loop\n");
			// bandLayer[row] = new float[NCOLS];
			for (long long col = 0; col < NCOLS; col++) { // iterate through columns
				bandLayer[col + (long long)NCOLS * row] = rowBuff[col];
			}
		}
		//printf("done\n");
		CPLFree(rowBuff);
		return bandLayer;
	}

	template<typename T>
	float** GetArray2D(int layerIndex, float** bandLayer) {

		/*
		 * function float** GetArray2D(int layerIndex):
		 * This function returns a pointer (to a pointer)
		 * for a float array that holds the band (array)
		 * data from the geotiff, for a specified layer
		 * index layerIndex (1,2,3... for GDAL, for Geotiffs
		 * with more than one band or data layer, 3D that is).
		 *
		 * Note this is a template function that is meant
		 * to take in a valid C++ data type (i.e. char,
		 * short, int, float), for the Geotiff in question
		 * such that the Geotiff band data may be properly
		 * read-in as numbers. Then, this function casts
		 * the data to a float data type automatically.
		 */

		 // get the raster data type (ENUM integer 1-12, 
		 // see GDAL C/C++ documentation for more details)        
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		// get number of bytes per pixel in Geotiff
		int nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
		//int nbytes = GDALGetDataTypeSizeBytes(bandType);

		//printf("nbytes %d\n", nbytes);
		// allocate pointer to memory block for one row (scanline) 
		// in 2D Geotiff array.  
		T *rowBuff = (T*)CPLMalloc(nbytes*NCOLS);

		for (int row = 0; row < NROWS; row++) {
			//printf("row %d nAlloc %d\n", row, NCOLS);
			bandLayer[row] = new float[NCOLS];
		}
		//printf("alloked\n");

		for (int row = 0; row < NROWS; row++) {     // iterate through rows
			//printf("row %d assign\n", row);

		  // read the scanline into the dynamically allocated row-buffer       
			CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
				GF_Read, 0, row, NCOLS, 1, rowBuff, NCOLS, 1, bandType, 0, 0);
			if (!(e == 0)) {
				cout << "Warning: Unable to read scanline in Raster!" << endl;
				exit(1);
			}

			//printf("loop\n");
			// bandLayer[row] = new float[NCOLS];
			for (int col = 0; col < NCOLS; col++) { // iterate through columns
				bandLayer[row][col] = (float)rowBuff[col];
			}
		}
		//printf("done\n");
		CPLFree(rowBuff);
		return bandLayer;
	}

	int GetArray2D_noAlloc(int layerIndex, float** bandLayer, float* rowBuff) {

		/*
		 * function float** GetArray2D(int layerIndex):
		 * This function returns a pointer (to a pointer)
		 * for a float array that holds the band (array)
		 * data from the geotiff, for a specified layer
		 * index layerIndex (1,2,3... for GDAL, for Geotiffs
		 * with more than one band or data layer, 3D that is).
		 *
		 * Note this is a template function that is meant
		 * to take in a valid C++ data type (i.e. char,
		 * short, int, float), for the Geotiff in question
		 * such that the Geotiff band data may be properly
		 * read-in as numbers. Then, this function casts
		 * the data to a float data type automatically.
		 */

		 // get the raster data type (ENUM integer 1-12, 
		 // see GDAL C/C++ documentation for more details)        
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		// get number of bytes per pixel in Geotiff
		//int nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
		//int nbytes = GDALGetDataTypeSizeBytes(bandType);

		// allocate pointer to memory block for one row (scanline) 
		// in 2D Geotiff array.  
		//float* rowBuff = (float*)CPLMalloc(nbytes * NCOLS);

		for (int row = 0; row < NROWS; row++) {     // iterate through rows

		  // read the scanline into the dynamically allocated row-buffer       
			CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
				GF_Read, 0, row, NCOLS, 1, rowBuff, NCOLS, 1, bandType, 0, 0);
			if (!(e == 0)) {
				cout << "Warning: Unable to read scanline in Raster!" << endl;
				exit(1);
			}

			//bandLayer[row] = new float[NCOLS];
			for (int col = 0; col < NCOLS; col++) { // iterate through columns
				bandLayer[row][col] = (float)rowBuff[col];
			}
		}
		//CPLFree(rowBuff);
		return 0;
	}

	int GetArray2D_noAlloc2(int layerIndex, float* rowBuff) {

		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		for (int row = 0; row < NROWS; row++) {     // iterate through rows

		  // read the scanline into the dynamically allocated row-buffer       
			CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
				GF_Read, 0, row, NCOLS, 1, rowBuff, NCOLS, 1, bandType, 0, 0);
			if (!(e == 0)) {
				cout << "Warning: Unable to read scanline in Raster!" << endl;
				exit(1);
			}

		}
		return 0;
	}

	int getAllocSize(int layerIndex) {
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));
		int nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
		return nbytes * NROWS * NCOLS;
	}

	int fix_minMax(int index, int minVal, int maxVal) {
		if (index < minVal)
			index = minVal;
		if (index > maxVal)
			index = maxVal;
		return index;
	}

	int fix_minMaxFromFrac(double indexFrac, int minVal, int maxVal) {
		int index = (int)indexFrac;

		if (index < minVal)
			index = minVal;
		if (index > maxVal)
			index = maxVal;
		return index;
	}

	unsigned short* GetRasterBand_intArr(int z, strPhysRaster* rasterData, strBoundBox boundingBox) {

		int pnXSize, pnYSize, nXValid, nYValid, xMin, yMin, xMax, yMax, xUse;
		double xPosFrac1, yPosFrac1, xPosFrac2, yPosFrac2;
		unsigned short* valueCell;
		GDALRasterBand* poBand = rasterDataset->GetRasterBand(z);

		//char** test2 = rasterDataset->GetMetadata("AREA_OR_PONT");
		//char** test3 = rasterDataset->GetMetadata();

		poBand->GetBlockSize(&pnXSize, &pnYSize);
		//printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
		//	pnXSize, pnYSize,
		//	GDALGetDataTypeName(poBand->GetRasterDataType()),
		//	GDALGetColorInterpretationName(
		//		poBand->GetColorInterpretation()));

		long long nXBlocks = (poBand->GetXSize() + pnXSize - 1) / pnXSize;
		long long nYBlocks = (poBand->GetYSize() + pnYSize - 1) / pnYSize;
		int n_yBlocks, n_xBlocks;

		printf("nCols/nRows %d %d nBlocks xy %d %d type %s\n", NCOLS, NROWS, nXBlocks, nYBlocks,
			GDALGetDataTypeName(poBand->GetRasterDataType()));

		//boundingBox.yMin = -39.0553;
		//boundingBox.yMax = -38.553;

		n_xBlocks = (double)NCOLS / pnXSize;
		if (n_xBlocks * pnXSize < NCOLS)
			n_xBlocks++;

		if (pnXSize == NCOLS) {
			boundingBox.xMin = min_lon;
			boundingBox.xMax = max_lon;
		}

		xPosFrac1 = (boundingBox.xMin - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
		xPosFrac2 = (boundingBox.xMax - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
		yPosFrac1 = (max_lat - boundingBox.yMax) * NROWS / pnYSize / (max_lat - min_lat);
		yPosFrac2 = (max_lat - boundingBox.yMin) * NROWS / pnYSize / (max_lat - min_lat);

		if (pnXSize == NCOLS) {
			xMin = fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
			xMax = fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
		}
		else {
			xMin = (int)xPosFrac1; // fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
			xMax = (int)xPosFrac2; // fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
		}
		yMin = fix_minMaxFromFrac(yPosFrac1, 0, nYBlocks - 1);
		yMax = fix_minMaxFromFrac(yPosFrac2, 0, nYBlocks - 1);
		//printf("bounding box %.3lf %.3lf %.3lf %.3lf\n",
		//	boundingBox.xMin, boundingBox.xMax, boundingBox.yMin, boundingBox.yMax);


		//printf("blocks to open for physical map x %d %d y %d %d\n", xMin, xMax, yMin, yMax);
		rasterData->minLongitude = min_lon + (double)xMin * pnXSize * size_col; // pnXSize / NCOLS * (max_lon - min_lon);
		rasterData->minLatitude = max_lat - (double)(yMax + 1) * pnYSize * size_row; // pnYSize / NROWS * (max_lat - min_lat);
		rasterData->maxLongitude = min_lon + (double)(xMax + 1) * pnXSize * size_col; // pnXSize / NCOLS * (max_lon - min_lon);
		rasterData->maxLatitude = max_lat - (double)yMin * pnYSize * size_row; // pnYSize / NROWS * (max_lat - min_lat);

		//printf("rasterData limits %.3lf %.3lf %.3lf %.3lf\n",
		//	rasterData->minLongitude, rasterData->maxLongitude,
		//	rasterData->minLatitude, rasterData->maxLatitude);

		GByte* pabyData = (GByte*)CPLMalloc(pnXSize * pnYSize);

		rasterData->nCols = (xMax - xMin + 1) * pnXSize;
		rasterData->nRows = (yMax - yMin + 1) * pnYSize;
		rasterData->size_col = size_col;
		rasterData->size_row = size_row;
		valueCell = (unsigned short*)calloc((long long)rasterData->nCols * (long long)rasterData->nRows, sizeof(unsigned short));

		FILE* filpek;
		//filpek = fopen("testRasterData.txt", "w");

		long long xPosNu, yPosNu, iY, iX, iYBlock, iXBlock, pos, pos2, pos_x, pos_y, first_y;
		for (iYBlock = yMin; iYBlock <= yMax; iYBlock++)
		{
			yPosNu = (iYBlock - yMin) * pnYSize;
			for (iXBlock = xMin; iXBlock <= xMax; iXBlock++)
			{
				if (iXBlock < 0)
					xUse = iXBlock + n_xBlocks;
				else {
					if (iXBlock >= n_xBlocks)
						xUse = iXBlock - n_xBlocks;
					else
						xUse = iXBlock;
				}
				poBand->ReadBlock(xUse, iYBlock, pabyData);

				xPosNu = (iXBlock - xMin) * pnXSize;

				// Compute the portion of the block that is valid
				// for partial edge blocks.
				poBand->GetActualBlockSize(iXBlock, iYBlock, &nXValid, &nYValid);
				//printf("block xy %d %d nValid xy %d %d\n", iXBlock, iYBlock, nXValid, nYValid);
				for (iY = 0; iY < pnYSize; iY++) {
					first_y = 0;
					for (iX = 0; iX < pnXSize; iX++) {
						if (iY < nYValid && iX < nXValid)
							valueCell[iX + xPosNu + rasterData->nCols * (iY + yPosNu)] = pabyData[iX + iY * pnXSize];
						else
							valueCell[iX + xPosNu + rasterData->nCols * (iY + yPosNu)] = 3;
						/*
						if (pabyData[iX + iY * nXBlocks] == 100 || first_y == 1) {
							pos_y = (int)(iX + xPosNu + rasterData->nCols * (iY + yPosNu)) / rasterData->nCols;
							pos_x = (iX + xPosNu + rasterData->nCols * (iY + yPosNu)) - pos_y * rasterData->nCols;
							fprintf(filpek, "pos %d, is %d at xy %.4lf %.4lf\n",
								iX + xPosNu + rasterData->nCols * (iY + yPosNu), pabyData[iX + iY * nXBlocks],
								rasterData->minLongitude + (double)(pos_x + 0.5) * rasterData->size_col,
							rasterData->maxLatitude - (double)(pos_y + 0.5) * rasterData->size_row); // / rasterData->nRows * (max_lat - min_lat));
							first_y = 0;
						}
						*/
					}
				}
			}
		}

		/*
		for (int x = 0; x < rasterData->nCols && x < 2; x++) {
			for (int y = 0; y < rasterData->nRows; y++) {
				fprintf(filpek, "pos %d xy val %.4lf %.4lf %d yx val %.4lf %.4lf %d\n",
				x + rasterData->nCols * (y),
				rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
				rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
				valueCell[x + rasterData->nCols * (y)],
				rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
				rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
				valueCell[x + rasterData->nCols * (y)]); // / rasterData->nRows * (max_lat - min_lat));
			}
			fprintf(filpek, "\n");
		}
		for (int y = 0; y < rasterData->nRows; y++) {
			for (int x = 0; x < rasterData->nCols; x++) {
				fprintf(filpek, "pos %d xy val %.4lf %.4lf %d yx val %.4lf %.4lf %d\n",
					x + rasterData->nCols * (y),
					rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
					rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
					valueCell[x + rasterData->nCols * (y)],
					rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
					rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
					valueCell[x + rasterData->nCols * (y)]); // / rasterData->nRows * (max_lat - min_lat));
			}
			fprintf(filpek, "\n");
		}
		fclose(filpek);


		printf("value at xMin %.3lf yMin %.3lf is %d\n", rasterData->minLongitude, rasterData->maxLatitude, valueCell[0]);
		*/

		/*
		// boundingBox.yMin = 1.117;
		double x = -178.49811; // boundingBox.xMin;
		double y = 51.39961;// boundingBox.yMin;
		//double x = -179.5; // boundingBox.xMin;
		//double y = 47.5;// boundingBox.yMin;
		//double x = 179.28479; // boundingBox.xMin;
		//double y = 51.93273;// boundingBox.yMin;
		printf("base xy %.3lf %.3lf\n", x, y);
		if (x < rasterData->minLongitude)
			x += 360;
		else {
			if (x > rasterData->maxLongitude)
				x -= 360;
		}
		pos_x = (int)((x - rasterData->minLongitude) / rasterData->size_col);
		pos_y = (int)((rasterData->maxLatitude - y) / rasterData->size_row);
		printf("value at xMinBound %.3lf yMinBound %.3lf mittpkt cell is %d\n", x, y,
			valueCell[pos_x + rasterData->nCols * pos_y]);
		printf("xValue 3prev 2prev prev this next 2next 3next %d %d %d %d %d %d %d\n",
			valueCell[pos_x - 3 + rasterData->nCols * pos_y],
			valueCell[pos_x - 2 + rasterData->nCols * pos_y],
			valueCell[pos_x - 1 + rasterData->nCols * pos_y],
			valueCell[pos_x + rasterData->nCols * pos_y],
			valueCell[pos_x + 1 + rasterData->nCols * pos_y],
			valueCell[pos_x + 2 + rasterData->nCols * pos_y],
			valueCell[pos_x + 3 + rasterData->nCols * pos_y]);
		printf("prev rowxValue 3prev 2prev prev this next 2next 3next %d %d %d %d %d %d %d\n",
			valueCell[pos_x - 3 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x - 2 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x - 1 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + 1 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + 2 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + 3 + rasterData->nCols * (pos_y - 1)]);
		printf("next rowxValue 3prev 2prev prev this next 2next 3next %d %d %d %d %d %d %d\n",
			valueCell[pos_x - 3 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x - 2 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x - 1 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + 1 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + 2 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + 3 + rasterData->nCols * (pos_y + 1)]);
		pos = pos_x + rasterData->nCols * pos_y;
		for (pos2 = pos + 1; pos2 < rasterData->nCols * rasterData->nRows; pos2++) {
			if (valueCell[pos2] == 0) {
				pos_y = (int)pos2 / rasterData->nCols;
				pos_x = pos2 - pos_y * rasterData->nCols;
				printf("started at pos %d, all 1:s until pos2 %d xy %.3lf %.3lf\n",
					pos, pos2,
					rasterData->minLongitude + (double)(pos_x + 0.5) * rasterData->size_col,
					rasterData->maxLatitude - (double)(pos_y + 0.5) * rasterData->size_row);
				break;
			}
		}
		*/

		return valueCell;
	}

	float* GetRasterBand_realArr(int z, strWeatherRaster* rasterData, strBoundBox boundingBox) {
		
		int pnXSize, pnYSize, nXValid, nYValid, xMin, yMin, xMax, yMax, xUse;
		double xPosFrac1, yPosFrac1, xPosFrac2, yPosFrac2;
		float* valueCell;
		GDALRasterBand* poBand = rasterDataset->GetRasterBand(z);

		poBand->GetBlockSize(&pnXSize, &pnYSize);
		//printf("Block=%dx%d Type=%s, ColorInterp=%s\n",
		//	pnXSize, pnYSize,
		//	GDALGetDataTypeName(poBand->GetRasterDataType()),
		//	GDALGetColorInterpretationName(
		//		poBand->GetColorInterpretation()));

		long long nXBlocks = (poBand->GetXSize() + pnXSize - 1) / pnXSize;
		long long nYBlocks = (poBand->GetYSize() + pnYSize - 1) / pnYSize;
		int n_yBlocks, n_xBlocks;

		//boundingBox.yMin = -39.0553;
		//boundingBox.yMax = -38.553;

		n_xBlocks = (double)NCOLS / pnXSize;
		if (n_xBlocks * pnXSize < NCOLS)
			n_xBlocks++;

		if (pnXSize == NCOLS) {
			boundingBox.xMin = min_lon;
			boundingBox.xMax = max_lon;
		}
		
		xPosFrac1 = (boundingBox.xMin - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
		xPosFrac2 = (boundingBox.xMax - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
		yPosFrac1 = (max_lat - boundingBox.yMax) * NROWS / pnYSize / (max_lat - min_lat);
		yPosFrac2 = (max_lat - boundingBox.yMin) * NROWS / pnYSize / (max_lat - min_lat);

		if (pnXSize == NCOLS) { 
			xMin = fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
			xMax = fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
		}
		else {
			xMin = (int)xPosFrac1; // fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
			xMax = (int)xPosFrac2; // fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
		}
		yMin = fix_minMaxFromFrac(yPosFrac1, 0, nYBlocks - 1);
		yMax = fix_minMaxFromFrac(yPosFrac2, 0, nYBlocks - 1);
		//printf("bounding box %.3lf %.3lf %.3lf %.3lf\n", 
		//	boundingBox.xMin, boundingBox.xMax, boundingBox.yMin, boundingBox.yMax);


		//printf("blocks to open for physical map x %d %d y %d %d\n", xMin, xMax, yMin, yMax);
		rasterData->minLongitude = min_lon + (double)xMin * pnXSize * size_col; // pnXSize / NCOLS * (max_lon - min_lon);
		rasterData->minLatitude = max_lat - (double)(yMax + 1) * pnYSize * size_row; // pnYSize / NROWS * (max_lat - min_lat);
		rasterData->maxLongitude = min_lon + (double)(xMax + 1) * pnXSize * size_col; // pnXSize / NCOLS * (max_lon - min_lon);
		rasterData->maxLatitude = max_lat - (double)yMin * pnYSize * size_row; // pnYSize / NROWS * (max_lat - min_lat);

		//printf("rasterData limits %.3lf %.3lf %.3lf %.3lf\n", 
		//	rasterData->minLongitude, rasterData->maxLongitude,
		//	rasterData->minLatitude, rasterData->maxLatitude);

		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(z));
		int nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
				//int nbytes = GDALGetDataTypeSizeBytes(bandType);
		//float* pabyData = (float*)CPLMalloc(pnXSize * pnYSize * nbytes);
		double* pabyData = (double*)CPLMalloc(pnXSize * pnYSize * nbytes);

		rasterData->nCols = (xMax - xMin + 1) * pnXSize;
		rasterData->nRows = (yMax - yMin + 1) * pnYSize;
		rasterData->size_col = size_col;
		rasterData->size_row = size_row;
		valueCell = (float*)calloc((long long)rasterData->nCols * (long long)rasterData->nRows, sizeof(float));

		FILE* filpek;
		//filpek = fopen("testRasterData.txt", "w");

		long long xPosNu, yPosNu, iY, iX, iYBlock, iXBlock, pos, pos2, pos_x, pos_y, first_y;
		for (iYBlock = yMin; iYBlock <= yMax; iYBlock++)
		{
			yPosNu = (iYBlock - yMin) * pnYSize;
			for (iXBlock = xMin; iXBlock <= xMax; iXBlock++)
			{
				if (iXBlock < 0)
					xUse = iXBlock + n_xBlocks;
				else {
					if (iXBlock >= n_xBlocks)
						xUse = iXBlock - n_xBlocks;
					else
						xUse = iXBlock;
				}
				poBand->ReadBlock(xUse, iYBlock, pabyData);

				xPosNu = (iXBlock - xMin) * pnXSize;

				// Compute the portion of the block that is valid
				// for partial edge blocks.
				poBand->GetActualBlockSize(iXBlock, iYBlock, &nXValid, &nYValid);
				//printf("block xy %d %d nValid xy %d %d\n", iXBlock, iYBlock, nXValid, nYValid);
				for (iY = 0; iY < pnYSize; iY++){
					first_y = 0;
					for (iX = 0; iX < pnXSize; iX++){
						if (iY < nYValid && iX < nXValid)
							valueCell[iX + xPosNu + rasterData->nCols * (iY + yPosNu)] = pabyData[iX + iY * pnXSize];
						else
							valueCell[iX + xPosNu + rasterData->nCols * (iY + yPosNu)] = 0;
						if (pabyData[iX + iY * pnXSize] < 9998)
							iX = iX;
						/*
						if (pabyData[iX + iY * nXBlocks] == 100 || first_y == 1) {
							pos_y = (int)(iX + xPosNu + rasterData->nCols * (iY + yPosNu)) / rasterData->nCols;
							pos_x = (iX + xPosNu + rasterData->nCols * (iY + yPosNu)) - pos_y * rasterData->nCols;
							fprintf(filpek, "pos %d, is %d at xy %.4lf %.4lf\n",
								iX + xPosNu + rasterData->nCols * (iY + yPosNu), pabyData[iX + iY * nXBlocks],
								rasterData->minLongitude + (double)(pos_x + 0.5) * rasterData->size_col,
							rasterData->maxLatitude - (double)(pos_y + 0.5) * rasterData->size_row); // / rasterData->nRows * (max_lat - min_lat));
							first_y = 0;
						}
						*/
					}
				}
			}
		}

		/*
		for (int x = 0; x < rasterData->nCols && x < 2; x++) {
			for (int y = 0; y < rasterData->nRows; y++) {
				fprintf(filpek, "pos %d xy val %.4lf %.4lf %d yx val %.4lf %.4lf %d\n",
				x + rasterData->nCols * (y),
				rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
				rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
				valueCell[x + rasterData->nCols * (y)],
				rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
				rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col, 
				valueCell[x + rasterData->nCols * (y)]); // / rasterData->nRows * (max_lat - min_lat));
			}
			fprintf(filpek, "\n");
		}
		for (int y = 0; y < rasterData->nRows; y++) {
			for (int x = 0; x < rasterData->nCols; x++) {
				fprintf(filpek, "pos %d xy val %.4lf %.4lf %d yx val %.4lf %.4lf %d\n",
					x + rasterData->nCols * (y),
					rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
					rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
					valueCell[x + rasterData->nCols * (y)],
					rasterData->maxLatitude - (double)(y + 0.5) * rasterData->size_row,
					rasterData->minLongitude + (double)(x + 0.5) * rasterData->size_col,
					valueCell[x + rasterData->nCols * (y)]); // / rasterData->nRows * (max_lat - min_lat));
			}
			fprintf(filpek, "\n");
		}
		fclose(filpek);
		

		printf("value at xMin %.3lf yMin %.3lf is %d\n", rasterData->minLongitude, rasterData->maxLatitude, valueCell[0]);
		*/
		/*
		// boundingBox.yMin = 1.117;
		double x = -178.49811; // boundingBox.xMin;
		double y = 51.39961;// boundingBox.yMin;
		//double x = -179.5; // boundingBox.xMin;
		//double y = 47.5;// boundingBox.yMin;
		//double x = 179.28479; // boundingBox.xMin;
		//double y = 51.93273;// boundingBox.yMin;
		//printf("base xy %.3lf %.3lf\n", x, y);
		if (x < rasterData->minLongitude)
			x += 360;
		else {
			if (x > rasterData->maxLongitude)
				x -= 360;
		}
		pos_x = (int)((x - rasterData->minLongitude) / rasterData->size_col);
		pos_y = (int)((rasterData->maxLatitude - y) / rasterData->size_row);
		printf("value at xMinBound %.3lf yMinBound %.3lf mittpkt cell is %.2lf\n", x, y,
			valueCell[pos_x + rasterData->nCols * pos_y]);
		printf("xValue 3prev 2prev prev this next 2next 3next %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf\n", 
			valueCell[pos_x - 3 + rasterData->nCols * pos_y],
			valueCell[pos_x - 2 + rasterData->nCols * pos_y],
			valueCell[pos_x - 1 + rasterData->nCols * pos_y],
			valueCell[pos_x + rasterData->nCols * pos_y],
			valueCell[pos_x + 1 + rasterData->nCols * pos_y],
			valueCell[pos_x + 2 + rasterData->nCols * pos_y],
			valueCell[pos_x + 3 + rasterData->nCols * pos_y]);
		printf("prev rowxValue 3prev 2prev prev this next 2next 3next %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf\n",
			valueCell[pos_x - 3 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x - 2 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x - 1 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + 1 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + 2 + rasterData->nCols * (pos_y - 1)],
			valueCell[pos_x + 3 + rasterData->nCols * (pos_y - 1)]);
		printf("next rowxValue 3prev 2prev prev this next 2next 3next %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf %.2lf\n",
			valueCell[pos_x - 3 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x - 2 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x - 1 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + 1 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + 2 + rasterData->nCols * (pos_y + 1)],
			valueCell[pos_x + 3 + rasterData->nCols * (pos_y + 1)]);
		*/

		return valueCell;
	}

	long long getSecondsFromUTC(const char* time) {
		int i;
		long long varde = 0, faktor = 10;
		if (time == NULL)
			return -1;

		for (i = 0; i < 256; i++) {
			if(time[i]=='\0')
				break;
			if (time[i] == ' ') {
				if (varde > 0)
					break;
				else
					continue;
			}
			varde = faktor * varde + (long long)(time[i] - '0');
		}

		return varde;
	}

	
	float** GetRasterBand_realArrAllBands(strWeatherRaster* rasterData, strBoundBox boundingBox) {

		int pnXSize, pnYSize, nXValid, nYValid, xMin, yMin, xMax, yMax, xUse;
		double xPosFrac1, yPosFrac1, xPosFrac2, yPosFrac2;
		float** valueCell;
		int z, bas_pnXSize, bas_pnYSize, bas_nXBlocks, bas_nYBlocks, bas_nbytes;
		long long nXBlocks;
		long long nYBlocks;
		int n_yBlocks, n_xBlocks;
		int nbytes, nBands;
		double* pabyData;
		long long xPosNu, yPosNu, iY, iX, iYBlock, iXBlock, pos, pos2, pos_x, pos_y, first_y;
		long long nSecondsUTC;
		GDALRasterBand* poBand;
		GDALDataType bandType;

		nBands = rasterDataset->GetRasterCount();
		for (z = 1; z <= nBands; z++) {
			poBand = rasterDataset->GetRasterBand(z);
			poBand->GetBlockSize(&pnXSize, &pnYSize);
			//printf("GRIB_FORECAST_SECONDS %s\n", poBand->GetMetadataItem("GRIB_FORECAST_SECONDS"));
			//const char* refTime = poBand->GetMetadataItem("GRIB_REF_TIME");
			//nSecondsUTC = getSecondsFromUTC(refTime);
			//tm* localTime = localtime(&nSecondsUTC);
			//printf("local DateTime: %d/%d/%d_%d:%d:%d\n", 1900 + localTime->tm_year,
			//	1 + localTime->tm_mon, localTime->tm_mday, localTime->tm_hour,
			//	localTime->tm_min, localTime->tm_sec);
			//tm* gmTime = gmtime(&nSecondsUTC);
			//printf("UTC DateTime: %d/%d/%d_%d:%d:%d ascii %s\n", 1900 + gmTime->tm_year,
			//	1 + gmTime->tm_mon, gmTime->tm_mday, gmTime->tm_hour,
			//	gmTime->tm_min, gmTime->tm_sec, asctime(gmTime));
			//printf("GRIB_REF_TIME %s\n", refTime);
			nSecondsUTC = getSecondsFromUTC(poBand->GetMetadataItem("GRIB_VALID_TIME"));
			//const char* validTime = poBand->GetMetadataItem("GRIB_VALID_TIME");
			//nSecondsUTC = getSecondsFromUTC(validTime);
			//tm* gmTime = gmtime(&nSecondsUTC);
			//printf("UTC valid DateTime: %d/%d/%d_%d:%d:%d ascii %s\n", 1900 + gmTime->tm_year,
			//	1 + gmTime->tm_mon, gmTime->tm_mday, gmTime->tm_hour,
			//	gmTime->tm_min, gmTime->tm_sec, asctime(gmTime));
			//printf("GRIB_VALID_TIME %s\n", poBand->GetMetadataItem("GRIB_VALID_TIME"));
			//printf("REF_TIME %s\n", poBand->GetMetadataItem("REF_TIME"));
			//printf("GRIB_IDS %s\n", poBand->GetMetadataItem("GRIB_IDS"));
			nXBlocks = (poBand->GetXSize() + pnXSize - 1) / pnXSize;
			nYBlocks = (poBand->GetYSize() + pnYSize - 1) / pnYSize;
			if (z == 1) {

				printf("nCols/nRows %d %d nBlocks xy %d %d type %s\n", NCOLS, NROWS, nXBlocks, nYBlocks,
					GDALGetDataTypeName(poBand->GetRasterDataType()));

				n_xBlocks = (double)NCOLS / pnXSize;
				if (n_xBlocks * pnXSize < NCOLS)
					n_xBlocks++;

				if (pnXSize == NCOLS) {
					boundingBox.xMin = min_lon;
					boundingBox.xMax = max_lon;
				}

				xPosFrac1 = (boundingBox.xMin - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
				xPosFrac2 = (boundingBox.xMax - min_lon) * NCOLS / pnXSize / (max_lon - min_lon);
				yPosFrac1 = (max_lat - boundingBox.yMax) * NROWS / pnYSize / (max_lat - min_lat);
				yPosFrac2 = (max_lat - boundingBox.yMin) * NROWS / pnYSize / (max_lat - min_lat);

				if (pnXSize == NCOLS) {
					xMin = fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
					xMax = fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
				}
				else {
					xMin = (int)xPosFrac1; // fix_minMaxFromFrac(xPosFrac1, 0, nXBlocks - 1);
					xMax = (int)xPosFrac2; // fix_minMaxFromFrac(xPosFrac2, 0, nXBlocks - 1);
				}
				yMin = fix_minMaxFromFrac(yPosFrac1, 0, nYBlocks - 1);
				yMax = fix_minMaxFromFrac(yPosFrac2, 0, nYBlocks - 1);

				rasterData->minLongitude = min_lon + (double)xMin * pnXSize * size_col; // pnXSize / NCOLS * (max_lon - min_lon);
				rasterData->minLatitude = max_lat - (double)(yMax + 1) * pnYSize * size_row; // pnYSize / NROWS * (max_lat - min_lat);
				rasterData->maxLongitude = min_lon + (double)(xMax + 1) * pnXSize * size_col; // pnXSize / NCOLS * (max_lon - min_lon);
				rasterData->maxLatitude = max_lat - (double)yMin * pnYSize * size_row; // pnYSize / NROWS * (max_lat - min_lat);
				rasterData->nTimeIntervals = nBands;
				rasterData->secondsUTC = (long long*)malloc(nBands * sizeof(long long));

				bandType = GDALGetRasterDataType(
					rasterDataset->GetRasterBand(z));
				nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
				pabyData = (double*)CPLMalloc(pnXSize * pnYSize * nbytes);

				rasterData->nCols = (xMax - xMin + 1) * pnXSize;
				rasterData->nRows = (yMax - yMin + 1) * pnYSize;
				rasterData->size_col = size_col;
				rasterData->size_row = size_row;
				valueCell = (float**)malloc(nBands * sizeof(float*));
				bas_pnXSize = pnXSize;
				bas_pnYSize = pnYSize;
				bas_nXBlocks = nXBlocks;
				bas_nYBlocks = nYBlocks;
				bas_nbytes = nbytes;
			}
			else {
				bandType = GDALGetRasterDataType(
					rasterDataset->GetRasterBand(z));
				nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
				if (bas_pnXSize != pnXSize) {
					errlog("ERROR! Different pnXSize of bands in weather raster, %d and %d\n", bas_pnXSize, pnXSize);
					return NULL;
				}
				if (bas_pnYSize != pnYSize) {
					errlog("ERROR! Different pnYSize of bands in weather raster, %d and %d\n", bas_pnYSize, pnYSize);
					return NULL;
				}
				if (bas_nXBlocks != nXBlocks) {
					errlog("ERROR! Different nXBlocks of bands in weather raster, %d and %d\n", bas_nXBlocks, nXBlocks);
					return NULL;
				}
				if (bas_nYBlocks != nYBlocks) {
					errlog("ERROR! Different nYBlocks of bands in weather raster, %d and %d\n", bas_nYBlocks, nYBlocks);
					return NULL;
				}
				if (bas_nbytes != nbytes) {
					errlog("ERROR! Different nbytes of bands in weather raster, %d and %d\n", bas_nbytes, nbytes);
					return NULL;
				}
			}
			rasterData->secondsUTC[z-1] = nSecondsUTC;
			valueCell[z-1] = (float*)calloc((long long)rasterData->nCols * (long long)rasterData->nRows, sizeof(float));
			for (iYBlock = yMin; iYBlock <= yMax; iYBlock++)
			{
				yPosNu = (iYBlock - yMin) * pnYSize;
				for (iXBlock = xMin; iXBlock <= xMax; iXBlock++)
				{
					if (iXBlock < 0)
						xUse = iXBlock + n_xBlocks;
					else {
						if (iXBlock >= n_xBlocks)
							xUse = iXBlock - n_xBlocks;
						else
							xUse = iXBlock;
					}
					poBand->ReadBlock(xUse, iYBlock, pabyData);

					xPosNu = (iXBlock - xMin) * pnXSize;

					// Compute the portion of the block that is valid
					// for partial edge blocks.
					poBand->GetActualBlockSize(iXBlock, iYBlock, &nXValid, &nYValid);
					for (iY = 0; iY < pnYSize; iY++) {
						first_y = 0;
						for (iX = 0; iX < pnXSize; iX++) {
							if (iY < nYValid && iX < nXValid)
								valueCell[z-1][iX + xPosNu + rasterData->nCols * (iY + yPosNu)] = pabyData[iX + iY * pnXSize];
							else
								valueCell[z-1][iX + xPosNu + rasterData->nCols * (iY + yPosNu)] = 0;
							if (pabyData[iX + iY * pnXSize] < 9998)
								iX = iX;
						}
					}
				}
			}
		}

		return valueCell;
	}

	int GetRasterBand_ny(int layerIndex, float* rowBuff) {

		//checkMinnesAnvandning(__LINE__);
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		// read the scanline into the dynamically allocated row-buffer       
		CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
			GF_Read, 0, 0, NCOLS, NROWS, rowBuff, NCOLS, NROWS, bandType, 0, 0);
		//checkMinnesAnvandning(__LINE__);
		if (!(e == 0)) {
			cout << "Warning: Unable to read scanline in Raster!" << endl;
			exit(1);
		}
		return 0;
	}

	int GetRasterBand_ny2(int layerIndex, double* rowBuff) {

		//checkMinnesAnvandning(__LINE__);
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		// read the scanline into the dynamically allocated row-buffer       
		CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
			GF_Read, 0, 0, NCOLS, NROWS, rowBuff, NCOLS, NROWS, bandType, 0, 0);
		//checkMinnesAnvandning(__LINE__);
		if (!(e == 0)) {
			cout << "Warning: Unable to read scanline in Raster!" << endl;
			exit(1);
		}
		return 0;
	}

	int GetRasterBand_nyLess(int layerIndex, float* rowBuff) {

		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		int startPos = 100, nElementX = 400, nElementY = 200;
		
		// read the scanline into the dynamically allocated row-buffer       
		CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
			GF_Read, startPos, startPos, nElementX, nElementY, rowBuff, nElementX, nElementY, bandType, 0, 0);
		if (!(e == 0)) {
			cout << "Warning: Unable to read scanline in Raster!" << endl;
			exit(1);
		}

		return 0;
	}

	int GetRasterBand_matrixNoAlloc(int layerIndex, float* bandLayer) {
		printf("test3\n");
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		//CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
		//	GF_Read, 0, 0, NCOLS, NROWS, bandLayer, NCOLS, NROWS, bandType, 0, 0);
		CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
			GF_Read, 0, 0, NCOLS, 1, bandLayer, NCOLS, 1, bandType, 0, 0);
		printf("e %d\n", e);
		int e1 = e;
		printf("e1 %d\n", e1);
		if (e1 == 0)
			printf("call worked\n");
		else{
			cout << "Warning: Unable to read scanline in Raster!" << endl;
			printf("error msg %d, '%s'\n", e, (char*)(CPLGetLastErrorMsg()));
			exit(1);
		}
		printf("pos 5 %f NCOLS %d NROWS %d\n", bandLayer[5], NCOLS, NROWS);
		printf("test5\n");
		printf("pos 50 %f\n", bandLayer[50]);
	}

	int tmpTest(int layerIndex) {

		float* testArray;
		int nAlloc;
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));
		int nbytes = GDALGetDataTypeSize(bandType); // pfg ty nasta rad fungerade ej
		nAlloc = NCOLS;// *(NROWS + 1);
		testArray = (float*)CPLMalloc(nbytes * nAlloc);
		printf("alloced %d x %d bytes\n", nbytes, nAlloc);
		GetRasterBand_matrixNoAlloc(layerIndex, testArray);
		printf("anrop klart\n");
		return 0;

	}



	int GetRasterBand_matrixNoAllocTmp(int layerIndex, float* bandLayer) {
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));

		int startPos = 100, nElementX = 400, nElementY = 200;

		CPLErr e = rasterDataset->GetRasterBand(layerIndex)->RasterIO(
			GF_Read, startPos, startPos, nElementX, nElementY, bandLayer, nElementX, nElementY, bandType, 0, 0);
		if (!(e == 0)) {
			cout << "Warning: Unable to read scanline in Raster!" << endl;
			exit(1);
		}
	}
	int GetRasterBand_matrixNoAlloc2(int layerIndex, float* bandLayer) {
		GDALDataType bandType = GDALGetRasterDataType(
			rasterDataset->GetRasterBand(layerIndex));
		int nBandCount = 30;
		int* panBandMap;
		panBandMap = (int*)malloc(nBandCount * sizeof(int));
		for (int i = 0; i < 30; i++) {
			panBandMap[i] = i + 1;
		}

		CPLErr e = GDALDatasetRasterIOEx(rasterDataset,
			GF_Read, 0, 0, NCOLS, NROWS, bandLayer, NCOLS, NROWS, bandType, nBandCount, panBandMap, 0, 0, 0, NULL);
		if (!(e == 0)) {
			cout << "Warning: Unable to read scanline in Raster!" << endl;
			exit(1);
		}
	}
};




class Raster2 {

private: // NOTE: "private" keyword is redundant here.  
		 // we place it here for emphasis. Because these
		 // variables are declared outside of "public", 
		 // they are private. 

	const char* filename;        // name of Geotiff
	GDALDataset *rasterDataset; // Geotiff GDAL datset object. 
	double geotransform[6];      // 6-element geotranform array.
	int dimensions[3];           // X,Y, and Z dimensions. 
	int NROWS, NCOLS, NLEVELS;     // dimensions of data in Geotiff. 
	double min_lat, min_lon, size_row, size_col;

public:

	// define constructor function to instantiate object
	// of this Raster class. 
	Raster2() {
		GDALAllRegister();
	}

	Raster2 open(const char* tiffname) {
		filename = tiffname;

		// set pointer to Geotiff dataset as class member.  
		rasterDataset = (GDALDataset*)GDALOpen(filename, GA_ReadOnly);

		// set the dimensions of the Geotiff 
		NROWS = GDALGetRasterYSize(rasterDataset);
		NCOLS = GDALGetRasterXSize(rasterDataset);
		NLEVELS = GDALGetRasterCount(rasterDataset);
		rasterDataset->GetGeoTransform(geotransform);
		min_lat = geotransform[3] + geotransform[5] * NROWS;
		min_lon = geotransform[0];
		size_row = -geotransform[5];
		size_col = geotransform[1];
	}

	// define destructor function to close dataset, 
	// for when object goes out of scope or is removed
	// from memory. 
	~Raster2() {
		// close the Geotiff dataset, free memory for array.  
		GDALClose(rasterDataset);
		GDALDestroyDriverManager();
	}
};

#endif //RASTER_CPP
