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
#include <iostream>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>

#define MAX_IMG (366*2)

using namespace cv;
using namespace std;

int from_day, to_day, total_days;
int total_image;
Mat img[MAX_IMG];

void combineImages(Mat &dst)
{
    int rows = img[0].rows, cols = img[0].cols;
    int out_img_col = (unsigned int) sqrt(total_image);
    Mat tmp;
    

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
    int i,j;
    total_image = 0;
    i=0;j=from_day;
    while (j<=to_day) {
        sprintf(day, "%03d", j);
        
        // Loading Aqua file
        filename = string("Images/RRGlobal_r12c32.") + argv[1] + day + string(".aqua.2km.jpg");
        img[i] = imread(filename, 1);
        if (!img[i].data) {
            cout << "No image data: " << filename << endl;
            return -1;
        }else{
            i++;
            total_image++;
        }
        
        // Loading Terra file
        filename = string("Images/RRGlobal_r12c32.") + argv[1] + day + string(".terra.2km.jpg");
        img[i] = imread(filename, 1);
        if (!img[i].data) {
            cout << "No image data: " << filename << endl;
            return -1;
        }else{
            i++;
            total_image++;
        }
        j++; // Advance to the next day
    }
    
    combineImages(output);
    imwrite("Output.jpg", output);
    
    // waitKey(0);
    
    return 0;
}
