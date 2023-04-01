//IT 388
//Created by Ryan Leone, John Carnicle, Ben Sester
//Takes in the name of an image file and gives the user the options to
//sort the pixels of an image row or column wise or to change the contrast
//and brightness of an image
//Compile with: mpiCC -g -Wall -o ImageEditor ImageEditor.cpp
//Run with: mpiexec -n <processors> ./ImageEditor

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <mpi.h>

using namespace std;

//Sort the vector using bottom up mergesort
void mergesort(vector<long>& arr);

//Merges the logical array in arr from indecies curLeft to curRight - 1 with the logical array in arr from indecies 
//      curRight to endRight and updates the arr, requires a temoprary vector temp
void merge(vector<long> &arr, vector<long> &temp, int curLeft, int curRight, int endRight);

int main(int argc, char** argv)
{

    //MPI variables
    int rank;
    int nproc;
    MPI_Comm comm;

    //Initialize the MPI execution environment
    MPI_Init(&argc, &argv);

    //Initialize the MPI variables
    comm = MPI_COMM_WORLD;
    MPI_Comm_size(comm, &nproc);
    MPI_Comm_rank(comm, &rank);

    //Variables to hold the images attributes got from the stbi_load function
    int width;
    int height;
    int channels;

    //Will hold the image's file name got from the keyboard
    string imgFileName;

    //Will hold a pointer to the original image
    unsigned char *origImg;
    //Will hold a pointer to the updated image
    unsigned char *editedImg;

    if(rank == 0)
    {
        //Getting the name of the image file from the user
        cout << "Please input the name of the image file to be edited: ";
        cin >> imgFileName;

        //Try to create the image from the input file name
        origImg = stbi_load(imgFileName.c_str(), &width, &height, &channels, 0);

        //If the image could not be loaded keep trying to get a valid image name
        while(origImg == NULL) 
        {
            cout << "\nError in loading the image\n";

            cout << "Please input the name of the image file to be edited: ";
            cin >> imgFileName;
        
            origImg = stbi_load(imgFileName.c_str(), &width, &height, &channels, 0);
        }

        //Print image attributes
        cout << "Loaded image " << imgFileName << " with a width of " << width << "px, a height of " << height << "px, and " << channels << " channels\n";
    
        //String that holds the option that the user chose
        string option = "";
        
        //Printing out options
        cout << "\nPossible options:\n";
        cout << "1. Sort image's pixels horizontally\n";
        cout << "2. Sort image's pixels vertically\n";
        cout << "3. Change image's contrast and brightness\n";
        cout << "Please enter an option (1, 2, or 3): ";

        //Putting the user's input into the option string
        cin >> option;

        //If the user's input was invalid keep propting the user for valid input
        while(option != "1" && option != "2" && option != "3")
        {
            cout << "\nInvalid input. Please try again. Possible options:\n";
            cout << "1. Sort image's pixels horizontally\n";
            cout << "2. Sort image's pixels vertically\n";
            cout << "3. Change image's contrast and brightness\n";
            cout << "Please enter an option (1, 2, or 3): ";

            //Putting the user's input into the option string
            cin >> option;
        }
        
    } //End of if(rank == 0)

    //Send the width and height of the image to each processor
    MPI_Bcast(&width, 1, MPI_INT, 0, comm);
    MPI_Bcast(&height, 1, MPI_INT, 0, comm);

    //Free the memory used by the origianl image
    if(rank == 0)
    {
        stbi_image_free(origImg); 
    }

    //Clean up the MPI execution environment
    MPI_Finalize();

    return 0;

}

void merge(vector<long> &arr, vector<long> &temp, int curLeft, int curRight, int endRight)
{
    
    int endLeft = curRight - 1;
    int curTemp = curLeft;
    int numElements = endRight - curLeft + 1;
    
    // merging the elements while both logical arrays still have values
    while(curLeft <= endLeft && curRight <= endRight)
    {
        
        // handling if the current item in the left logical array is smaller than the current item in the right 
        if(arr[curLeft] <= arr[curRight])
        {
            temp[curTemp] = arr[curLeft];

            curLeft++;
            curTemp ++;
        }
        // handling if the current item in the right logical array is smaller than the current item in the left 
        else
        {
            temp[curTemp] = arr[curRight];
           
            curRight++;
            curTemp++;
        }
        
    }
  
    // if the right logical array ran out of items then all the items left in the left logical array 
    //      are larger so them must be merged
    while(curLeft <= endLeft)
    {
        temp[curTemp] = arr[curLeft];
        
        curLeft++;
        curTemp++;
    }

    // if the left logical array ran out of items then all the items left in the right logical array 
    //      are larger so them must be merged
    while(curRight <= endRight)
    {
        temp[curTemp] = arr[curRight];

        curRight++;
        curTemp++;
    }
    
    // copying values back to the arr vector
    for(int i = 0; i < numElements; i++, endRight--)
    {
        arr[endRight] = temp[endRight]; 
    }

}
