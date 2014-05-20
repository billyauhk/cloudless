/*
This code downloads images from GIBS to generate LANCE-compatible images for the old cloudless, as a temporary fix between the LANCE-MODIS HDD crash and the upcoming recovery.
This program is not complete, as the input parameters and outputs still have to change to make it compatible with the old download.sh
BTW, the corrected reflectance from GIBS seems to be more reliable to me.

http://map1.vis.earthdata.nasa.gov/wmts-geo/MODIS_Terra_Data_No_Data/default/2014-01-01/EPSG4326_250m/0/0/0.png
How about 250m water mask? It could also be used.
*/

// Headers for GDAL
#include <gdal_priv.h>
#include "cpl_conv.h"
// Headers for OpenCV
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
// Other general headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
// For calling wait()
#include <sys/types.h>
#include <sys/wait.h>

using namespace cv;
using namespace std;

  char normal_bounds[] = "<UpperLeftX>-180.0</UpperLeftX><UpperLeftY>90</UpperLeftY><LowerRightX>396.0</LowerRightX><LowerRightY>-198</LowerRightY><TileLevel>8</TileLevel><TileCountX>2</TileCountX><TileCountY>1</TileCountY>";
  char wms_format[] = "<GDAL_WMS>"
    "<Service name=\"TMS\">"
      // arctic/geo/antarctic, Terra/Aqua, CorrectedReflectance_TrueColor/Data_No_Data, then yyyy-mm-dd, EPSG , jpg/png
      "<ServerUrl>http://map1.vis.earthdata.nasa.gov/wmts-%s/MODIS_%s_%s/default/%04d-%02d-%02d/%s_250m/${z}/${y}/${x}.%s</ServerUrl>"
    "</Service>"
    "<DataWindow>"
      "<UpperLeftX>-180.0</UpperLeftX><UpperLeftY>90</UpperLeftY><LowerRightX>396.0</LowerRightX><LowerRightY>-198</LowerRightY><TileLevel>8</TileLevel><TileCountX>2</TileCountX><TileCountY>1</TileCountY>"
      "<YOrigin>top</YOrigin>"
    "</DataWindow>"
    // North - EPSG:3413, Rectangular - EPSG:4326, South - EPSG:3031
    "<Projection>%s</Projection>"
    "<BlockSizeX>512</BlockSizeX>"
    "<BlockSizeY>512</BlockSizeY>"
    // JPG - 3, PNG - 4
    "<BandsCount>%u</BandsCount>"
  "</GDAL_WMS>";

// Data common to all threads
int year, month, day;
int row, col;
int daynum;

typedef enum {NORTH, NORMAL, SOUTH} region;
typedef enum {TERRA, AQUA} satellite;
typedef enum {REFLECTANCE, MASK} data_layer;
typedef enum {JPG, PNG} data_format;

int download(region regionFlag, satellite satFlag, data_layer dataFlag, data_format typeFlag){
  GDALDataset *poDataset = NULL;
  Mat imageBuffer, outputBuffer;
  double pixel2geo[6];
  double geo2pixel[6];
  uint64_t xmin, ymin;
  uint64_t xsize, ysize;
  char GIBS_XML[1000];
  char outFileName[100];
  int numChannel;

  // Get the data
    sprintf(GIBS_XML, wms_format, /* Region name    */ (regionFlag==NORTH)?"arctic":((regionFlag==NORMAL)?"geo":"antarctic"),
                                  /*Satellite  name */ (satFlag==TERRA)?"Terra":"Aqua",
                                  /*Data layer name */ (dataFlag==REFLECTANCE)?"CorrectedReflectance_TrueColor":"Data_No_Data",
                                                       year, month, day,
                                  /*Region EPSG code*/ (regionFlag==NORTH)?"EPSG3413":((regionFlag==NORMAL)?"EPSG4326":"EPSG3031"),
                                  /*Data Format     */ (typeFlag==JPG)?"jpg":"png",
                                  /*Region EPSG code*/ (regionFlag==NORTH)?"EPSG:3413":((regionFlag==NORMAL)?"EPSG:4326":"EPSG:3031"),
                                                       (typeFlag==JPG)?3:4);
    printf("%s", GIBS_XML);
  // Open file
    poDataset = (GDALDataset*) GDALOpen(GIBS_XML, GA_ReadOnly);
    if(!poDataset){
      fprintf(stderr,"File cannot be opened!\n");return 1;
    }

  // Print some metadata
    xsize = poDataset->GetRasterXSize();
    ysize = poDataset->GetRasterYSize();
    numChannel = poDataset->GetRasterCount();

  // Calculate coordinates
    poDataset->GetGeoTransform(pixel2geo);
    GDALInvGeoTransform(pixel2geo, geo2pixel);

    double lat_offset, lon_offset;
    switch(regionFlag){
      case NORTH:return 1;break;
      case NORMAL:
        GDALApplyGeoTransform(geo2pixel, (col-20)*9.0, (row-9)*9.0, &lon_offset, &lat_offset);
      break;
      case SOUTH:return 1;break;
    }
    printf("Offset=(%lf,%lf)\n",lon_offset,lat_offset);

  // Read Image data
    xmin = (int) lon_offset;
    ymin = (int) lat_offset;
    ysize = 4096;
    xsize = 4096;

    imageBuffer.create(ysize, xsize, CV_8UC1);
    outputBuffer.create(ysize, xsize, (typeFlag==JPG)?CV_8UC3:CV_8UC4);
    outputBuffer.zeros(ysize, xsize, (typeFlag==JPG)?CV_8UC3:CV_8UC4);
    int bandArray[1];
    for(int i=0;i<((typeFlag==JPG)?3:4);i++){
      bandArray[0] = i+1;
      imageBuffer.zeros(ysize, xsize, CV_8U);
      poDataset->RasterIO(GF_Read, xmin, ymin, xsize, ysize,
                          (void*) imageBuffer.ptr(0), xsize, ysize,
                          GDT_Byte, 1, bandArray, 0, 0, 0);
      for(int x=0;x<xsize;x++){
        for(int y=0;y<ysize;y++){
            ((uint8_t*)(outputBuffer.data))[(y*xsize+x)*numChannel+(numChannel-i-1)] = ((uint8_t*)(imageBuffer.data))[y*xsize+x];
        }
      }
    }

    if(typeFlag==PNG){
      outputBuffer = (outputBuffer==0);
      outputBuffer.convertTo(outputBuffer, CV_8UC1, 1, 0);
    }

    sprintf(outFileName, "%s_r%dc%d.%04d%03d.%s%s250m.%s", (regionFlag==NORTH)?"RRArctic":((regionFlag==NORMAL)?"RRGlobal":"RRAntarctic"), row, col, year, daynum,
                                                          (satFlag==TERRA)?"terra":"aqua", (dataFlag==REFLECTANCE)?".":".opaque", (typeFlag==JPG)?"jpg":"png");
    imwrite(outFileName, outputBuffer);
    printf("File %s is written\n", outFileName);
    return 0;
}

int main(int argc, char* argv[]){
  // New main function

  // Read arguments
  year = 2014; month = 1; day = 1; daynum = 1;
  row = 12; col = 32;

  // Prepare the GDAL driver
  GDALAllRegister();
  //download(NORMAL, TERRA, REFLECTANCE, JPG);
  //download(NORMAL, AQUA, REFLECTANCE, JPG);
  for(day=1;day<=16;day++){
    daynum=day;
    if(fork()==0){download(NORMAL, TERRA, MASK, PNG);exit(0);}
    if(fork()==0){download(NORMAL, AQUA, MASK, PNG);exit(0);}
    if(fork()==0){download(NORMAL, TERRA, REFLECTANCE, JPG);exit(0);}
    if(fork()==0){download(NORMAL, AQUA, REFLECTANCE, JPG);exit(0);}
    for(int j=0;j<4;j++)wait(NULL);
  }
  return 0;
}
