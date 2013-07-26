/*
 Cloudless Atlas v0.1.0

 Description:
 The program is currently able to take in images located under the folder "Images" in current directory (e.g. "./Images/RRGlobal_r12c32.2012001.aqua.2km.jpg"), and then produce an image file "Output.jpg" which combined the input images into a large image.

 Limitations (To be implemented?):
 1. The output image is not sorted yet.
 2. The program read in Aqua images only.
 3. No exception handlings.



 Usage: cloudless [Year] [From Day] [To Day]
 Example: ./cloudless 2012 5 19



 Modification log:
 Forte: 2013-6-24 00:37    Version 0.1.0 is out.

*/

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>
#include <omp.h>

#define MAX_IMG (366*2)

using namespace cv;
using namespace std;

int from_day, to_day, total_days;
int total_image;
Mat img[MAX_IMG];
Mat mark[MAX_IMG];
Mat final_image; // Where we put the average image

#define numChannel 3
typedef struct data_s{
  uint8_t mark;
  uint8_t img_data[numChannel];
} data_t;
data_t* data;

int pixelComparator(const void* a, const void* b)
{
  return (((data_t*)b)->mark) - (((data_t*)a)->mark);
}

void trackbar_Index(int value, void*)
{
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

void combineImages(Mat &dst)
{
    int rows = img[0].rows, cols = img[0].cols;
    int out_img_col = (unsigned int) sqrt(total_image); // How many pics we will pack on a row
    Mat tmp;

    // Create destination image
    dst = cvCreateMat((total_image / out_img_col + 1) * rows, out_img_col * cols, img[0].type());

    for (int i = 0; i < total_image / out_img_col + 1; i++) {
        tmp = dst(Rect(0, i * rows, cols, rows));
        for (int j = 0; j < out_img_col; j++) {
            if (img[out_img_col * i + j % out_img_col].data) {
                img[out_img_col * i + j % out_img_col].copyTo(tmp);
            }
            if (j < out_img_col - 1) {
                tmp = dst(Rect((j + 1) * cols, i * rows, cols, rows));
            }
        }
    }
}

int main( int argc, char** argv )
{
//STAGE: Read arguments
    char day[3];
    string filename;
    Mat output;

    if (argc != 4) {
        printf("Usage: %s [Year] [From Day] [To Day]\n", argv[0]);
        return -1;
    }

    from_day = atoi(argv[2]);
    to_day = atoi(argv[3]);
    total_days = to_day - from_day + 1;

//STAGE: Read images
printf("Reading images...\n");
    int i,j;
    total_image = 0;
    for (int i=from_day;i<=to_day;i++) {
        sprintf(day, "%03d", i);

        // Loading Aqua file
        filename = string("Images/RRGlobal_r12c32.") + argv[1] + day + string(".aqua.2km.jpg");
        img[total_image] = imread(filename, 1);
        if (!img[total_image].data) {
            cout << "No image data: " << filename << endl;
            return -1;
        }else{
            total_image++;
        }

        // Loading Terra file
        filename = string("Images/RRGlobal_r12c32.") + argv[1] + day + string(".terra.2km.jpg");
        img[total_image] = imread(filename, 1);
        if (!img[total_image].data) {
            cout << "No image data: " << filename << endl;
            return -1;
        }else{
            total_image++;
        }
    }

//STAGE: Give marks
printf("Giving marks to pixels...\n");
    // First we identify NULL/Data-invalid pixels.
    // These pixels are given zero marks directly.
    Mat lookUpTable(1, 256, CV_8U); // CV_8U is just hard-coded here.
    uchar* p = lookUpTable.data;
    for( int i = 0; i < 256; ++i) {
#ifdef ORIGINAL
        p[i] = 1;
#else
        p[i] = 255;
#endif
    }
    for( int i = 0; i < 5; ++i) {
#ifdef ORIGINAL
      p[i] = 0;
#else
      p[i] = p[255-i] = 0;
#endif
    }
    
    // This has to be kept as we still have to drop out the invalid pixels
    #pragma omp parallel for private(i)
    for (int i=0;i<total_image;i++) {
        Mat gray = Mat(img[i]);
        cvtColor( img[i], gray, CV_RGB2GRAY );
        LUT(gray, lookUpTable, mark[i]);
    }

    int row = img[0].rows, col = img[0].cols;
#ifdef ORIGINAL
// The original marking method by Charlie...seems not working!?
printf("Charlie marking\n");
    #pragma omp parallel for private(i)
    for (int i=0;i<total_image;i++) {
        Mat channel[3];
        for (int j = 0;j<3;j++) {
          channel[j] = Mat(row, col, CV_8UC1, 0); // CV_8UC1 is just hard-coded here.
        }
        split( img[i], channel );
        // Let us use the great "matrix operators" in OpenCV to emcode the parallelism
        Mat max_channel = max(channel[0],max(channel[1],channel[2]));
        Mat min_channel = min(channel[0],min(channel[1],channel[2]));
        Mat saturation = max_channel - min_channel;
        Mat darkness = max_channel*3 - (channel[0] + channel[1] + channel[2]);
        mark[i] = mark[i].mul((saturation + darkness*0.1f)*0.5f, 1/(4.0f/3.0f)); // S channel
        mark[i] = 255.0-mark[i]; // Invert them as we are sorting in reverse
    }
#else
// Our way...seems better?
    // Then we give marks to remaining pixels...
    // Depending on the S value in the HSV colorspace.
printf("Bill marking\n");
    #pragma omp parallel for private(i)
    for (int i=0;i<total_image;i++) {
        Mat hsv = Mat(img[i].rows, img[i].cols, img[i].type(), 0);
        Mat channel[3];
        for (int j = 0;j<3;j++) {
          channel[j] = Mat(hsv.rows, hsv.cols, img[i].type(), 0);
        }
        cvtColor( img[i], hsv, CV_RGB2HSV);
        split( hsv, channel );
        mark[i] = mark[i].mul(channel[1], 1/255.0f); // S channel
    } // end for each image i
#endif


//STAGE: Sort pixels
printf("Sorting pixels...\n");
    // Testing parallelization of the loop   
    #pragma omp parallel for private(i,j,data)
    for (int i=0;i<row;i++) {
        for (int j=0;j<col;j++) {
            // Initialize the matrix and extract data from arrays img and data
            data = (data_t*) malloc(sizeof(data_t)*total_image);
            for (int k=0;k<total_image;k++) {
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
            for (int k=0;k<total_image;k++) {
                if(img[k].data){
                  ((uint8_t*)(mark[k].data))[i*col+j] = data[k].mark;
                  // Unroll it might be slightly better for performance
                  ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+0] = data[k].img_data[0];
                  ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+1] = data[k].img_data[1];
                  ((uint8_t*)(img[k].data))[i*col*numChannel+j*numChannel+2] = data[k].img_data[2];
                }else{
                  // If this is invoked, that means we will have to add code for skipping the null img[].data
                  assert(1);
                }
            } // end for(k)            
            free(data);
        } // end for(j)
    } // end for(i)

//STAGE: Output
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
    while ((key=getchar())!=EOF) {
        switch (key) {
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

//STAGE:Averaging
// Seems it is better to get 30/732 image for r12c32 instead of the n/4+2...

    final_image = cvCreateMat(row, col, img[0].type());
    int average_count = total_image>>2 +2; // Hard code
    
    #pragma omp parallal private(sumR,sumG,sumB)
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

    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);

    try {
        imwrite("average.png", final_image, compression_params);
    }
    catch (runtime_error& ex) {
        fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
        return 1;
    }
    
    return 0;
}
