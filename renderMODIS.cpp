/*
 Cloudless Atlas
 
 Usage: renderMODIS [folder path] [pattern] [resource file]
 Example: ./renderMODIS Images/ *.jpg resource.txt
 
 */

#include <stdio.h>
#include <cstdlib>
#include <iostream>
#include <cstring>
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

// Image infomation
#define WIDTH 512
#define HEIGHT 512
#define CHANNEL 3
#define SCALE_CONSTANT 1.4

using namespace std;

typedef struct
{
    char hostname[HOST_NAME_MAX];
    int cpu_core; // Number of CPU core
    int memory; // Total memory size in GB
} machine_t;

off_t min_mem; // The minimum memory size available per CPU core among all nodes in MB
int total_cpu_core = 0; // The total number of CPU core

void matchFilePattern(char * pattern, long int * total_image)
{
    // ---------------------------------------------------------------
    // This function does the followings:
    // (1)parse files according to specified pattern
    // (2)set file_size, total_image
    // ---------------------------------------------------------------
    
    glob_t globbuf;
    
    globbuf.gl_offs = 0;
    glob(pattern, GLOB_DOOFFS, NULL, &globbuf);
    if (globbuf.gl_pathc == 0) {
        *total_image = 0;
        printf("No files match the pattern under this path.\n");
        exit(EXIT_FAILURE);
    } else {
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
    for (i = 0; (fscanf(fd, "%s %d %d", machine[i].hostname, &machine[i].cpu_core, &machine[i].memory)) != EOF; i++) {
        total_cpu_core += machine[i].cpu_core;
        if (i == 0) {
            min_mem = (machine[i].memory << 10) / machine[i].cpu_core;
        } else if (min_mem > (machine[i].memory << 10) / machine[i].cpu_core) {
            min_mem = (machine[i].memory << 10) / machine[i].cpu_core;
        }
    }
    *total_machine = i;
    fclose(fd);
}

void jobsAlloc(machine_t machine[], int total_machine, long int total_image)
{
    // ---------------------------------------------------------------
    // This function does the followings:
    // (1)run the jobs allocation algorithm according to CPU and RAM
    // (2)output a mapping of jobs and handling machines
    // ---------------------------------------------------------------
    
    int row, col;
    off_t alloc_pointer = 1; // The pointer to the starting row number of next allocation
    off_t alloc_row; // The number of row being allocated to a CPU core
    off_t req_mem_size = (off_t)WIDTH * HEIGHT * CHANNEL * SCALE_CONSTANT / (1024 * 1024); // Required memory size for one tile in MB
    
    row = col = (unsigned int)sqrt(total_image);
    alloc_row = min_mem / (req_mem_size * row);
    if (alloc_row == 0) { // A weakness of the algorithm: When it has not enough memory to handle even one row
        printf("Too many images that the program cannot handle!");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < total_machine && alloc_pointer != row; i++) {
        for (int j = 0; j < machine[i].cpu_core && alloc_pointer != row; j++) {
            printf("%s CPU Core %d: Row %lld - ", machine[i].hostname, j+1, (long long int)alloc_pointer);
            if ((alloc_pointer += alloc_row) > row) {
                alloc_pointer = row;
            }
            printf("%lld\n", (long long int)alloc_pointer);
        }
        if (i == total_machine - 1) { // If it is the last machine and
            if (alloc_pointer < row) { // it leaves some unallocated rows,
                i = -1; // reset "i" to let the algorithm run again starting from the first machine.
            }
        }
    }
}

int main(int argc, const char * argv[])
{
    char folder_path[PATH_MAX], pattern[PATH_MAX], resource_file[PATH_MAX];
    int total_machine;
    long int total_image;
    machine_t machine[MACHINE_MAX];
    
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
    
    matchFilePattern(pattern, &total_image);
    
    jobsAlloc(machine, total_machine, total_image);
    
    return 0;
}

