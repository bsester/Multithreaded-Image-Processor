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
    
        /*(for(unsigned int i = 0; i < width * height; i++)
        {
            cout << "i = " << i << "origImg[i] = " <<  origImg[i] << "\n";
        }*/

        //char newFileName1 [sizeof(imgFileName) + sizeof("2.jpg")];
        //sprintf(newFileName1, "%s2.jpg", imgFileName.c_str());
        //stbi_write_jpg(newFileName1, width, height, channels, origImg, 100);
        //cout << "origImg[width * height - 1]: " << origImg[width * height - 1] <<"\n";

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

        //Rank 0 needs to stores it pixels to modify
        vector<unsigned char> rankZeroPixels;
        //Need to store pixels to send to each other processor
        vector<unsigned char> pixelsToSend;

        if(option == "1" || option == "3") //Send rows to each processor
        {

            //Need to know if extra rows must be assigned
            int remainder = height % nproc;

            //Number of rows each processor must work on
            int defaultWork = height / nproc;

            //Holds the current rank of who's pixels are being stored
            int currentRank = 0;
            //Holds the index current pixel of the image
            int currentPixel = 0;

            //Get the vector of pixels for each rank
            while(currentRank < nproc)
            {
                
                //Holds how muany rows the current rank needs to process
                int localWork = defaultWork;

                if(currentRank < remainder) //Need to assign extra rows to workers with ranks lower than the remainder to evenly distribute work
                {
                    localWork += 1;
                }

                //Holds how many rows have been put into the processors array to be sent
                int numRowsProcessed = 0;

                //Store the pixels for the current rank from the reqiured rows
                while(numRowsProcessed < localWork)
                {
                    if(currentRank == 0) //Store rank 0's pixels in a special vector for later use
                    {
                        rankZeroPixels.push_back(origImg[currentPixel]);
                    }
                    else //Store the other processors' pixels temporarily to be sent
                    {
                        pixelsToSend.push_back(origImg[currentPixel]);
                    }

                    currentPixel++;

                    if(currentPixel % width == 0) //Next row reached
                    {
                        numRowsProcessed++;
                    }
                }


                //Send the size of the pixelsToSend vector and the pixelsToSend vector to each other processor
                if(currentRank != 0)
                {
                    int numPixels = pixelsToSend.size();
                    MPI_Send(&numPixels, 1, MPI_INT, currentRank, 12, comm);
                    MPI_Send(&pixelsToSend.front(), pixelsToSend.size(), MPI_UNSIGNED_CHAR, currentRank, 13, comm);
                }

                //Reset the vecotr for the next rank
                pixelsToSend.clear();

                currentRank++;
            }

        }
        else //Send columns to each processor
        {

            int remainder = width % nproc;

        }

        //Allocate memory for the edited image to receive the other processors pixels into
        unsigned char *editedImg = (unsigned char *) malloc(width * height * channels);
        //unsigned char *editedImg = stbi_load(imgFileName.c_str(), &width, &height, &channels, 0);

        if(option == "1" || option == "3") //Receive rows
        {
            //Holds the index of the current pixel in the edited image
            int currentPixel = 0;
            
            //Size of the rankZeroPixelsVector
            int rankZeroPixelsSize = rankZeroPixels.size();

            //Put all the pixels in the rankZeroPixels vector into the new image
            for(int i = 0; i < rankZeroPixelsSize; i++)
            {
                editedImg[currentPixel] = rankZeroPixels[i];
                currentPixel++;
            }

            //Get all the pixels from the other processor and put then into the edited image
            for(int i = 1; i < nproc; i++)
            {
                //Number of pixels to receive from the current processor
                int numPixels;
                MPI_Recv(&numPixels, 1, MPI_INT, i, 14, comm, MPI_STATUS_IGNORE);

                //Vector to receive the current processor'ss pixels into
                vector<unsigned char> newPixels (numPixels, '0');
                MPI_Recv(&newPixels.front(), numPixels, MPI_UNSIGNED_CHAR, i, 15, comm, MPI_STATUS_IGNORE);

                //Put the pixels from the current processor into the edited image
                for(int j = 0; j < numPixels; j++)
                {
                    editedImg[currentPixel] = newPixels[j];
                    currentPixel++;
                }
            }

            /*for(int i = 0; i < height * width; i++)
            {
                cout << "i = " << i << "editedImg[i] = " << editedImg[i] << "\n";
            }*/

            //cout << "current pixel: " << currentPixel << "\n";

            //Make the edited image's file name
            char newFileName [sizeof(imgFileName) + sizeof("Edited.png")];
            sprintf(newFileName, "%sEdited.jpg", imgFileName.c_str());

            //editedImg[0] = origImg[0];
            //editedImg[1] = origImg[0];
            //editedImg[2] = origImg[0];
            //editedImg[3] = origImg[0];
            //editedImg[4] = origImg[0];

            //Save the edited image
            stbi_write_jpg(newFileName, width, height, channels, editedImg, 100);

        }
        else //Receive columns
        {

        }

        //Free the memory used by the origianl image and edited image
        stbi_image_free(origImg); 
        stbi_image_free(editedImg); 

    } //End of if(rank == 0)
    else
    {
        //Recieve the number of pixels for this processor
        int numPixels;
        MPI_Recv(&numPixels, 1, MPI_INT, 0, 12, comm, MPI_STATUS_IGNORE);

        //Receive the pixels for this processor
        vector<unsigned char> localPixels (numPixels, 0);
        MPI_Recv(&localPixels.front(), numPixels, MPI_UNSIGNED_CHAR, 0, 13, comm, MPI_STATUS_IGNORE);

        //cout<<rank<<" numPixels: " << numPixels << "\n";
        //cout<<rank<<" size: " << localPixels.size() << "\n";

        //Send the number of pixels processed by this processor and send the processed pixels
        MPI_Send(&numPixels, 1, MPI_INT, 0, 14, comm);
        MPI_Send(&localPixels.front(), numPixels, MPI_UNSIGNED_CHAR, 0, 15, comm);

    } //End of else (other ranks done)


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
