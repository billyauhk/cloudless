#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<cstring>

/*
  Expected invocation:
  Specifying year and day: -start 2012001 -stop 2012366
  Specifying location: -row 12 -col 32 (default r12c32)
  Specifying resolution: -res 2km (default 2km)
  NO need to specify satellite name (use wildcard try both "aqua" and "terra")

  Specifying interactiveness: +i (need GUI), -i (no GUI)
  
*/

using namespace std;

int main(int argc, char* argv[]){
  string filePath;
  
  // Pattern for reference: Images/RRGlobal_r12c32.2012001.aqua.2km.jpg
  string folderPath="Images/";
  string filePrefix="RRGlobal_";
  string fileSuffix=".jpg";

  filename = string("Images/RRGlobal_r12c32.2012001.aqua.2km.jpg");
  cout << filename << endl;


  return 0;
}
