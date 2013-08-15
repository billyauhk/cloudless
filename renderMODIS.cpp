/*
 Cloudless Atlas
 
 Usage: renderMODIS [folder path] [pattern] [resource file]
 Example: ./renderMODIS Images/ *.jpg resource.txt
 
 */

#include <iostream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <cmath>
#include <cctype>
#include <cerrno>
#include <dirent.h>
#include <climits>
#include <glob.h>

#define MACHINE_MAX 100
#ifndef HOST_NAME_MAX
    #define HOST_NAME_MAX 255
#endif

using namespace std;

typedef struct stat stat_t;
typedef struct
{
    char hostname[HOST_NAME_MAX];
    int cpu_core; // Number of CPU core
    int memory; // Total memory size in MB
} machine_t;

void matchFilePattern(char * pattern, off_t * file_size, long int * total_image)
{
    // ---------------------------------------------------------------
    // This function does the followings:
    // (1)parse files according to specified pattern
    // (2)set file_size, total_image
    // ---------------------------------------------------------------
    
    glob_t globbuf;
    stat_t statbuf;
    
    globbuf.gl_offs = 0;
    glob(pattern, GLOB_DOOFFS, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
        *total_image = 0;
        printf("No files match the pattern under this path.\n");
        exit(EXIT_FAILURE);
    } else {
        if (stat(globbuf.gl_pathv[0], &statbuf) == 0) {
            *file_size = statbuf.st_size;
        }
        *total_image = globbuf.gl_pathc;
    }
}

void parseResourceFile(char resource_file[], machine_t * machine, int * total_machine)
{
    // ---------------------------------------------------------------
    // This function does the followings:
    // (1)reads the specified resource file
    // ---------------------------------------------------------------
    
    FILE * fd;
    int i;

    if ((fd = fopen(resource_file, "r")) == NULL) {
        perror(resource_file);
        exit(EXIT_FAILURE);
    }
    for (i = 0; (fscanf(fd, "%s %d %d", machine[i].hostname, &machine[i].cpu_core, &machine[i].memory)) != EOF; i++);
    *total_machine = i;
    fclose(fd);
}

void jobsAlloc(machine_t machine[], off_t file_size, int total_machine, long int total_image)
{
    // ---------------------------------------------------------------
    // This function does the followings:
    // (1)run the jobs allocation algorithm according to CPU and RAM
    // (2)output a mapping of jobs and handling machines
    // ---------------------------------------------------------------
}

int main(int argc, const char * argv[])
{
    char folder_path[PATH_MAX], pattern[PATH_MAX], resource_file[PATH_MAX];
    int total_machine;
    long int total_image;
    machine_t machine[MACHINE_MAX];
    off_t file_size; // File size in unit of Byte(B)
    
    if (argc != 4) {
        printf("Usage: %s [folder path] [pattern] [resource file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    strcpy(folder_path, argv[1]);
    strcpy(pattern, argv[2]);
    strcpy(resource_file, argv[3]);
    
    parseResourceFile(resource_file, machine, &total_machine);
    
    // Move to the image folder
    if (chdir(folder_path)) {
        perror(folder_path);
        exit(EXIT_FAILURE);
    }
    
    matchFilePattern(pattern, &file_size, &total_image);
    
    jobsAlloc(machine, file_size, total_machine, total_image);
    
    return 0;
}


