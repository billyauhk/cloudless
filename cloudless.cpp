/*
Cloudless Atlas v0.2.0

Description:
The program is currently able to read all JPEG (".jpg") from a folder and generate the output in the same name, but in PNG

Limitations (To be implemented?):
1. No exception handlings.
2. GUI is disabled for Bill marking.
3. Will not work on greyscale images (known problem due to the heuristic).
4. Too much RAM is wasted, this is the next target we are working on.

Usage: cloudless FolderName(without space) [x y width height namePrefix] [Year (optional)]
        (For cropping, (x,y) is the left-top corner. namePrefix is to be attached to the generated filename)
Example: ./cloudless r12c32

Modification log:
Forte: 2013-06-24 00:37    Version 0.1.0 is out.
Bill:  2013-07-27 22:18    Version 0.2.0 is out.

*/

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <cstring>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <cstdio>
#include <glob.h>
#include <time.h>

#define PARALLEL
#ifdef PARALLEL
  #include <omp.h>
#endif

#define MAX_IMG (366*8)

using namespace cv;
using namespace std;

int from_day, to_day, total_days;
int total_image;
Mat img[MAX_IMG];
Mat mark[MAX_IMG];
Mat final_image; // Where we put the average image
Mat final_aux; // Auxiliary image for diagnosis

#define numChannel 3
typedef struct data_s{
  uint8_t mark;
  uint8_t img_data[numChannel];
} data_t;
data_t* data;

int pixelComparator(const void* a, const void* b){
  return (((data_t*)b)->mark) - (((data_t*)a)->mark);
}

void trackbar_Index(int value, void*){
  int rows = img[0].rows, cols = img[0].cols;

  Mat imgOut = cvCreateMat(rows, cols*2, img[0].type());
  // RHS: normalized
  Mat tmp = imgOut(Rect(cols,0,cols,rows));
  /* Trying to mimick the behavior of ImageMagick's normalize here
  Not very successful, need some extra time on understand the contrast stretch function there.
  The current equalizeHist of channel 2 comes from trial-n-error
  */
  Mat hsv;
  cvtColor(img[value],hsv,CV_BGR2HSV);
  vector<Mat> channels;
  split(hsv,channels);
  equalizeHist(channels[2], channels[2]);
  merge(channels,hsv);
  cvtColor(hsv,tmp,CV_HSV2BGR);
  // LHS: Original
  tmp = imgOut(Rect(0,0,cols,rows));
  img[value].copyTo(tmp);

  imshow("Thumbnail",imgOut);
}

/* Commented out as it is unreachable
void combineImages(Mat &dst){
  int rows = img[0].rows, cols = img[0].cols;
  int out_img_col = (unsigned int) sqrt(total_image); // How many pics we will pack on a row
  Mat tmp;

  // Create destination image
  dst = cvCreateMat((total_image/out_img_col + 1) * rows, out_img_col * cols, img[0].type());

  for(int i=0;i< total_image/out_img_col + 1;i++){
    tmp = dst(Rect(0, i * rows, cols, rows));
    for(int j=0;j<out_img_col;j++){
      if(img[out_img_col*i + j%out_img_col].data) {
        img[out_img_col*i + j%out_img_col].copyTo(tmp);
      }
      if(j < out_img_col-1){
        tmp = dst(Rect( (j+1)*cols, i*rows, cols, rows));
      }
    }
  }
}
*/

int main(int argc, char** argv){
  //STAGE: Read arguments
  char pattern[256];
  Mat output;
  int boundX, boundY, boundW, boundH; // Region of Interest, in image coordinate

  if(argc>2){
    while(argv[1][strlen(argv[1])-1]=='/'){
      printf("Extra slash is seen and ignored.\n");
      argv[1][strlen(argv[1])-1]='\0';
    }
  }

  switch(argc){
    case 2:
      sprintf(pattern, "%s/*a.250m.jpg", argv[1]);
      break;
    case 3:
      sprintf(pattern, "%s/*.%s*a.250m.jpg", argv[1], argv[2]);
      break;
    case 7:
      sprintf(pattern, "%s/*a.250m.jpg", argv[1]);
      boundX = atoi(argv[2]); boundY = atoi(argv[3]);
      boundW = atoi(argv[4]); boundH = atoi(argv[5]);
      printf("Suffix: %s\n", argv[6]);
      break;
    case 8:
      sprintf(pattern, "%s/*.%s*a.250m.jpg", argv[1], argv[7]);
      boundX = atoi(argv[2]); boundY = atoi(argv[3]);
      boundW = atoi(argv[4]); boundH = atoi(argv[5]);
      printf("Suffix: %s\n", argv[6]);
      break;
    default:
      printf("Usage: %s FolderName [x y width height nameSuffix] [Year]\n", argv[0]);
      return -1;
      break;
  }

  //STAGE: Read images
  printf("Reading images...\n");
  int i,j,row,col;
  total_image = 0;

  printf("Pattern to match: %s\n",pattern);
  glob_t globbuf;
  globbuf.gl_offs = 0;

  Mat lookUpTable(1, 256, CV_8U);
  uint8_t* p = lookUpTable.data;
  for(int i=0;i<256;i++){
    p[i] = (i < 5 || i >= 250 ? 0 : 255);
  }

  // The correct way is checking return value, not globbuf.gl_pathc
  if((glob(pattern, GLOB_DOOFFS, NULL, &globbuf)) == 0){
    Rect region;
    // Reading our first image
    img[0] = imread(string(globbuf.gl_pathv[0]));
    // This portion of the boundary-checking code is being factored out
    // With assumption that all images are of the same size
    if(argc==7 || argc==8){
      // Prevent array out of bound
      if(boundX + boundW >= img[0].cols){
        boundW = img[0].cols - boundX;
      }
      if(boundY + boundH >= img[0].rows){
        boundH = img[0].rows - boundY;
      }
      region = Rect(boundX, boundY, boundW, boundH);

      Mat temp_image; // Temporary buffer for clipping
      Mat(img[0], region).copyTo(temp_image);
      img[0] = temp_image;
    }

    row = img[0].rows, col = img[0].cols;
    int flag = 1;
    // First image will be loaded twice...but that's just a very little overhead
    int emptySlot = 0; // Can be set to other value later on (esp. when it is load after sort.)
    int emptyGlob = 0;
    int currentSlot, currentGlob;
    char currentName[500]; // A scratch space to generate the filename of the opaque mask

#ifdef PARALLEL
    #pragma omp parallel private(currentSlot,currentGlob,currentName) shared(flag,total_image)
#endif
    {
      // Give everybody an empty slot and an empty glob
      #pragma omp critical(getNewSlot)
      {
        currentSlot = emptySlot;
        emptySlot++;
        img[currentSlot].data=NULL; // Just to prevent some error
        //fprintf(stderr,"Thread ID: %d gets the slot %d\n", omp_get_thread_num(), currentSlot);
      }

      while(flag){
        if(img[currentSlot].data!=NULL){
          // Slot have been used up, let's get a new one
          #pragma omp critical(getNewSlot)
          {
            currentSlot = emptySlot;
            emptySlot++;
            img[currentSlot].data=NULL; // Just to prevent some error
          }//end critical
          //fprintf(stderr,"Thread ID: %d gets the slot %d\n", omp_get_thread_num(), currentSlot);
          if(currentSlot>=MAX_IMG){
            // There will be no more new slot, stop other threads coming here
            #pragma omp atomic
              flag *= 0;
          }
        }else{
          // Slot is unused. Let us get some data.
          //fprintf(stderr,"Thread ID: %d have slot %d\n", omp_get_thread_num(), currentSlot);
          // Let us take a glob
          #pragma omp critical(getNewGlob)
          {
            currentGlob = emptyGlob;
            emptyGlob++;
          }//end critical
          //fprintf(stderr,"Thread ID: %d preparing to load %d-th image\n", omp_get_thread_num(), currentGlob);
          if(currentGlob>=globbuf.gl_pathc){
            // There will be no more new files, stop other threads coming here
            #pragma omp atomic
              flag *= 0;
          }
          // Normal load and cropping
          if((currentSlot<MAX_IMG)&&(currentGlob<globbuf.gl_pathc)&&(globbuf.gl_pathv[currentGlob]!=NULL)){
            img[currentSlot]=imread(string(globbuf.gl_pathv[currentGlob]));
            // Check out the data
            if(img[currentSlot].data!=NULL){
              // Load the opaque mask
              strncpy(currentName,globbuf.gl_pathv[currentGlob],500);
// Filename are in the form of "RRGlobal_r10c20.2012001.aqua.250m.jpg" -> "RRGlobal_r10c20.2012001.aqua.opaque.250m.png"
              int p, comma;
              for(p = strlen(currentName),comma=0;comma!=2; p--){
                if(currentName[p]=='.')comma++;
              } // Make currentName[p] points to the second comma
              currentName[p+2]='\0';
              snprintf(currentName+(p+2),500-(p-3),"opaque.250m.png");
              //printf("NAME: %s\n",currentName);
              mark[currentSlot]=imread(string(currentName),CV_LOAD_IMAGE_GRAYSCALE);
              // Clip the data here
              if(argc==7 || argc==8){
                Mat temp_image; // Temporary buffer for clipping
                Mat(img[currentSlot], region).copyTo(temp_image);
                img[currentSlot] = temp_image;
                Mat(mark[currentSlot], region).copyTo(temp_image);
                mark[currentSlot] = temp_image;
              }
              #pragma omp atomic
                total_image++;
              /*if(total_image%20==0){
                fprintf(stderr,"The %d-th image is loaded.\n",total_image);
              }*/

// STAGE: Giving Mark (put inside in attempt to enhance CPU utilization) =================================
  #ifdef ORIGINAL
    Mat gray = Mat(img[currentSlot]);
    cvtColor( img[currentSlot], gray, CV_RGB2GRAY );
    LUT(gray, lookUpTable, mark[currentSlot]);
    // The original marking method by Charlie
    // Not upgraded when we are moving to accurate masking
    //printf("Charlie marking\n");
      Mat channel[3];
      for(int j=0;j<3;j++){
        channel[j] = Mat(row, col, CV_8UC1, 0);
      }
      split( img[currentSlot], channel );
      for(int j=0;j<3;j++){
        channel[j].convertTo(channel[j], CV_16UC1);
      }
      Mat sum = Mat(row, col, CV_16UC1, 0);
      sum = channel[0] + channel[1] + channel[2];
      mark[currentSlot] = (sum < 10 | sum > (3 * 255) - 3);

      // Let us use the great "matrix operators" in OpenCV to encode the parallelism
      Mat max_channel = max(channel[0],max(channel[1],channel[2]));
      Mat min_channel = min(channel[0],min(channel[1],channel[2]));
      Mat saturation = max_channel - min_channel;
      Mat darkness = (255*3 - sum)/10;
      saturation.convertTo(saturation, CV_8UC1);
      darkness.convertTo(darkness, CV_8UC1);
      mark[currentSlot] = (~mark[currentSlot]/255).mul((saturation + darkness)*0.5f, 1/(4.0f/3.0f)); // S channel
  #else
    // Our way...seems better?
    // Then we give marks to remaining pixels...
    // Depending on the S value in the HSV colorspace.
    //printf("Bill marking\n");
      Mat hsv = Mat(img[currentSlot].rows, img[currentSlot].cols, img[currentSlot].type(), 0);
      Mat channel[3];

      for(int j = 0;j<3;j++){
        channel[j] = Mat(hsv.rows, hsv.cols, img[currentSlot].type(), 0);
      }
      cvtColor( img[currentSlot], hsv, CV_RGB2HSV);
      split( hsv, channel );
      // If they are 0 already, then they are really masked out and not used
      mark[currentSlot] = mark[currentSlot].mul(channel[1], 1/255.0f); // S channel
  #endif
//=======================================================================================================

              //fprintf(stderr,"[%ld] Thread ID: %d loaded an image...\n", time(NULL), omp_get_thread_num());
            }
          }else{
            /*if(!flag){fprintf(stderr,"Thread ID: %d have no image to read as told by FLAG.\n", omp_get_thread_num());}
            else{fprintf(stderr,"Thread ID: %d got an invalid glob entry, it was the %d-th.\n", omp_get_thread_num(), currentGlob);}*/
          }
        }
      }
    }// end parallel

    fprintf(stderr,"End of parallel loading.\n");
    for(int i=0;i<total_image;i++){
      if(img[i].data==NULL){
        fprintf(stderr,"Discovered empty slot at %d-th...shoot\n",i);
      }else{
        //fprintf(stderr,"Image %d: %d x %d\n",i,img[i].rows,img[i].cols);
      }
    }

  }else{
    fprintf(stderr,"Error in return value of glob().\n");
    exit(-1);
  }

  // Free the glob buffers
  globfree(&globbuf);
  printf("Got %d images.\n", total_image);
  if(total_image==MAX_IMG){
    printf("Too many images...I only handle the first %d images.\n",total_image);
  }
  if(total_image==0){
    fprintf(stderr,"No image to proceed!\n");
    return -1;
  }

  //STAGE: Sort pixels
  printf("Sorting pixels...\n");
  // Testing parallelization of the loop
  #ifdef PARALLEL
  #pragma omp parallel for private(i,j,data) collapse(2)
  #endif
  for(int i=0;i<row;i++){
    for(int j=0;j<col;j++){
      // Initialize the matrix and extract data from arrays img and data
      data = (data_t*) malloc(sizeof(data_t)*total_image);
      for(int k=0;k<total_image;k++){
        if(img[k].data){
          data[k].mark = ((uint8_t*)(mark[k].data))[i*col+j];
          // Unroll it might be slightly better for performance
          data[k].img_data[0] = ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+0];
          data[k].img_data[1] = ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+1];
          data[k].img_data[2] = ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+2];
        }
      } // end for(k)
      // Sorting procedure
      qsort(data,total_image,sizeof(data_t),pixelComparator);
      //Putting data back
      for(int k=0;k<total_image;k++){
        if(img[k].data){
          ((uint8_t*)(mark[k].data))[i*col+j] = data[k].mark;
          // Unroll it might be slightly better for performance
          ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+0] = data[k].img_data[0];
          ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+1] = data[k].img_data[1];
          ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+2] = data[k].img_data[2];
        }else{
          // If this is invoked, that means we will have to add code for skipping the null img[].data
          fprintf(stderr,"Containminated Image data at %d-th image, pixel (%d,%d).\n",k,i,j);
          assert(1);
        }
      } // end for(k)
      free(data);
    } // end for(j)
  } // end for(i)

  //STAGE: Preview. Code no longer active.
/*
  printf("View the thumbnail...\n");
  // Or showing everybody a window
  namedWindow("Thumbnail",CV_GUI_EXPANDED);
  createTrackbar("Index","Thumbnail",NULL,total_image-1,trackbar_Index,NULL);
  trackbar_Index(0,NULL);
  waitKey(0);
  destroyWindow("Thumbnail");

  // Generating "Thumbnail" for reference
  printf("Do you want to save the image (y/n)?\n");
  char key;
  while((key=getchar())!=EOF){
    switch(key){
      case 'y': case 'Y':
      printf("Generating output...\n");
      combineImages(output);
      imwrite("Output.jpg", output);
      break;
      case 'n': case 'N':
      printf("Not generating output, byebye!\n");
      break;
      default:
      goto breakReadKeyLoop; // Bad way to do, but we do not have break(2)
    }
  }
  breakReadKeyLoop:
*/

  //STAGE: Average and Output
  printf("Averaging and Output.\n");
  final_image = cvCreateMat(row, col, img[0].type());
  int average_count = total_image>>2 +2; // Hard code, but seems working good...

  // Average
  #ifdef PARALLEL
  #pragma omp parallal private(sumR,sumG,sumB) collapse(2)
  #endif
  for(int i=0;i<row;i++){
    for(int j=0;j<col;j++){
      float sumR, sumG, sumB;
      sumR = sumG = sumB = 0;
      for(int k=0;(k<average_count)&&(img[k].data!=NULL);k++){
        sumB += ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+0];
        sumG += ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+1];
        sumR += ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+2];
      }
      ((uint8_t*)(final_image.data))[i*col*numChannel+j*numChannel+0] = round(sumB/((float) average_count));
      ((uint8_t*)(final_image.data))[i*col*numChannel+j*numChannel+1] = round(sumG/((float) average_count));
      ((uint8_t*)(final_image.data))[i*col*numChannel+j*numChannel+2] = round(sumR/((float) average_count));
    }
  }

  // Colour Adjustment: Copy the saturation value from img[0]
/*  Mat hsv = Mat(img[0].rows, img[0].cols, img[0].type(), 0);
  cvtColor( final_image, final_aux, CV_RGB2HSV);
  cvtColor( img[0], hsv, CV_RGB2HSV);
  for(int i=0;i<row;i++){
    for(int j=0;j<col;j++){
      // Use saturation (channel 1) only
      ((uint8_t*)(final_aux.data))[i*col*numChannel+j*numChannel+1] = ((uint8_t*)(hsv.data))[i*col*numChannel+j*numChannel+1];
    }
  }
  cvtColor( final_aux, final_image, CV_HSV2RGB);*/

  vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  compression_params.push_back(3); // Larger: smaller size, longer time

  try{
    if(argc==7 || argc==8){
      imwrite(argv[1]+string("_")+argv[6]+string(".png"), final_image, compression_params);    
    }else{
      imwrite(argv[1]+string(".png"), final_image, compression_params);
    }
  }catch(runtime_error& ex){
    fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
    return 1;
  }

  printf("Program ends.\n");
  return 0;
}
