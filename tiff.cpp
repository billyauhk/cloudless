#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>

// tiff.cpp -- trying to read a TIFF and see whether that works

using namespace cv;
using namespace std;

int main(int argc, char* argv[]){
  string filename;
  Mat img;

  printf("Trying to read %s\n",argv[1]);
  filename = string(argv[1]);
  img = imread(filename, CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_COLOR);

  printf("%d x %d in size\n", img.rows, img.cols);
  
  switch(img.depth()){
    case CV_8U:printf("uint8_t");break;
    case CV_8S:printf("int8_t");break;
    case CV_16U:printf("uint16_t");break;
    case CV_16S:printf("int16_t");break;
    case CV_32S:printf("int32_t");break;
    case CV_32F:printf("float (s23e8)");break;
    case CV_64F:printf("double (s52e11)");break;
    default:printf("Unknown depth!?");exit(-1);break;
  }
  printf(" x %d channels\n", img.channels());

  namedWindow("Viewer",CV_GUI_EXPANDED);
  imshow("Viewer",img);
  waitKey(0);
  destroyWindow("Viewer");
  return 0;
}
