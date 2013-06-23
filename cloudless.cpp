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

#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>

#define MAX_IMG 366
#define OUT_IMG_COL 10

using namespace cv;
using namespace std;

int from_day, to_day, total_days;
Mat img[MAX_IMG];

void combineImages(Mat &dst)
{
    int rows = img[0].rows, cols = img[0].cols;
    Mat tmp;
    
    dst = cvCreateMat((total_days / OUT_IMG_COL + 1) * rows, OUT_IMG_COL * cols, img[0].type());
    
    for (int i = 0; i < total_days / OUT_IMG_COL + 1; i++) {
        tmp = dst(Rect(0, i * rows, cols, rows));
        for (int j = 0; j < OUT_IMG_COL; j++) {
            if (img[OUT_IMG_COL * i + j % OUT_IMG_COL].data) {
                img[OUT_IMG_COL * i + j % OUT_IMG_COL].copyTo(tmp);
            }
            if (j < OUT_IMG_COL - 1) {
                tmp = dst(Rect((j + 1) * cols, i * rows, cols, rows));
            }
        }
    }
}

int main( int argc, char** argv )
{
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
    
    // Read images
    for (int i = 0; i < total_days; i++) {
        sprintf(day, "%03d", from_day + i);
        filename = string("Images/RRGlobal_r12c32.") + argv[1] + day + string(".aqua.2km.jpg");
        img[i] = imread(filename, 1);
        if (!img[i].data) {
            cout << "No image data: " << filename << endl;
            return -1;
        }
    }
    
    combineImages(output);
    imwrite("Output.jpg", output);
    
    // waitKey(0);
    
    return 0;
}