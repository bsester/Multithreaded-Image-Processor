//IT 388
//Created by Ryan Leone, John Carnicle, Ben Sester
//Takes in the name of an JPEG file and gives the user the options to
//sort the pixels of an image row wise, change the image to gray scale,
//change the image's contrast. or change the image's brightness.
//Compile with: mpiCC -g -Wall -o ImageEditor ImageEditor.cpp
//Run with: mpiexec -n <processors> ./ImageEditor
// OR Run with: mpiexec -n <processors> ./ImageEditor <image name> <option> <contrast/brightness>
//The image name, option, and contrast/brightness variables are optional but if an image name is provided an option MUST also be provided

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <mpi.h>
#include <regex>

using namespace std;

//Takes the RGB values for each pixel in original, averages them, and puts the average in grayScale.
//Needs the number of channels in the image to do the conversion
void convertToGrayScale(vector<unsigned char> &original, vector<unsigned char> &grayScale, int channels);

//modifies the pixels of the vector by multiplying each rgb componenet by contrast
void changeContrast(vector<unsigned char> &original, double contrast, int channels);

//modifies the pixels of the vector by adding bright to each rgb component
void changeBrightness(vector<unsigned char> &original, int bright, int channels);

//method to sort an image's pixels by their rgb values
void sortHorizontal(vector <unsigned char> &original, int start, int end);

// struct to hold pixels for sorting purposes
struct pixel
{
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

// insertion sort to sort an array of pixels
void insertionSort(vector <pixel> &pixels);

// overloading comparison operator to sort pixels
bool operator > (pixel &p1, pixel &p2)
{
  if (p1.red > p2.red)
  {
    return true;
  }
  else if ((p1.red == p2.red) &&
          (p1.green > p2.green))
  {
    return true;
  }
  else if  ((p1.red == p2.red) &&
           (p1.green == p2.green) &&
           (p1.blue > p2.blue))
  {
    return true;
  }
  else
  {
    return false;
  }
}

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

    //variables for changing the images brightness and contrast
    int bright;
    double contrast;

    if(rank == 0)
    {

        if(argc != 1 && argc != 3 && argc != 4) //If there is an incorrect number of command line arguments, abort the program
        {
            cout << "Incorrect number of command line arguments.\nExpecting no command line arguments or an image then an option and then a contrast/brightness number if option is 3 or 4.\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }   

        //Creste regular expression to make sure the input file is a JPEG
        regex nameCheck("^\\w+\\.jpg$");

        if(argc != 3 && argc != 4)//If there are no command line arguments get the input file name from the user
        {
            //Getting the name of the image file from the user
            cout << "Please input the name of a JPEG file to be edited: ";
            cin >> imgFileName;

        }
        else //Otherwise get it from the first command line argument
        {
            imgFileName = argv[1];
        }

        //If the image is a JPEG try loading it
        if(regex_match(imgFileName, nameCheck))
        {
            //Try to create the image from the input file name
            origImg = stbi_load(imgFileName.c_str(), &width, &height, &channels, 0);
        }

        //If the image could not be loaded or is not a JPEG keep trying to get a valid image name
        while(origImg == NULL || !regex_match(imgFileName, nameCheck))
        {
            cout << "\nError loading the image or file is not a JPEG (.jpg)\n";

            cout << "Please input the name of a JPEG file to be edited: ";
            cin >> imgFileName;

            origImg = stbi_load(imgFileName.c_str(), &width, &height, &channels, 0);
        }

        //Print image attributes
        cout << "Loaded image " << imgFileName << " with a width of " << width << "px, a height of " << height << "px, and " << channels << " channels\n";

        //String that holds the option that the user chose
        string option = "";

        if(argc != 3 && argc != 4) //If there are no command line arguments get the option from the user
        {
            //Printing out options
            cout << "\nPossible options:\n";
            cout << "1. Sort image's pixels horizontally\n";
            cout << "2. Convert the image to gray scale\n";
            cout << "3. Change image's contrast\n";
            cout << "4. Change image's brightness\n";
            cout << "Please enter an option (1, 2, 3, or 4): ";

            //Putting the user's input into the option string
            cin >> option;
        }
        else //Otherwise get the option from the second command line argument
        {
            option = argv[2];
        }

        //If the user's input was invalid keep propting the user for valid input
        while(option != "1" && option != "2" && option != "3" && option != "4")
        {
            cout << "\nInvalid input. Please try again. Possible options:\n";
            cout << "1. Sort image's pixels horizontally\n";
            cout << "2. Convert the image to gray scale\n";
            cout << "3. Change image's contrast\n";
            cout << "4. Change image's brightness\n";
            cout << "Please enter an option (1, 2, 3, or 4): ";

            //Putting the user's input into the option string
            cin >> option;
        }

        //Record the start time of the parallel section
        double startTime = MPI_Wtime();

        //Rank 0 needs to stores it pixels to modify
        vector<unsigned char> rankZeroPixels;
        //Need to store pixels to send to each other processor
        vector<unsigned char> pixelsToSend;

        //If sorting by rows save the work for rank 0
        int rankZeroWork;

        //Send rows to each processor
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

            if(option == "1") //If option 1 chosen need to store rank 0's work (number of rows) and send the work the the other processors
            {
              if(currentRank == 0)
              {
                rankZeroWork = localWork;
              }
            else
              {
                MPI_Send(&localWork, 1, MPI_INT, currentRank, 30, comm);
              }
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

                if(currentPixel % (width * channels) == 0) //Next row reached
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

            //Reset the vector for the next rank
            pixelsToSend.clear();

            currentRank++;
        }

        //End of sending rows to each processor


        //Send width to the other processors
        for(int i = 1; i < nproc; i++)
        {
            MPI_Send(&width, 1, MPI_INT, i, 20, comm);
        }

        //Send height to the other processors
        for(int i = 1; i < nproc; i++)
        {
            MPI_Send(&height, 1, MPI_INT, i, 21, comm);
        }

        //Send channels to the other processors
        for(int i = 1; i < nproc; i++)
        {
            MPI_Send(&channels, 1, MPI_INT, i, 22, comm);
        }

        //Need to convert option to an int to be sent
        int intOption = stoi(option);

        //Send intOption to the other processors
        for(int i = 1; i < nproc; i++)
        {
            MPI_Send(&intOption, 1, MPI_INT, i, 23, comm);
        }

        //If option 2 is chosen, a vector to help the gray scale version of rank zero's pixels is needed
        vector<unsigned char> rankZeroGrayScale;
        //If option 2 is chosen, the size of the rankZeroGrayScale vector is needed
        int rankZeroGrayScaleSize;

        //Perform editing here for rank 0
        if(option == "1") //Sort horizontally
        {
          // get the number of rows for this thread
            int rowLength = width * channels;

            for (int i = 0; i < rankZeroWork; i++)
            {
              int start = rowLength * i;
              int end = (rowLength * (i+1) - 1);
              sortHorizontal(rankZeroPixels, start, end);
            }

        }
        else if(option == "2") //Convert to gray scale
        {

            //Get rankZeroPixels' size and store it
            int numRankZeroPixels = rankZeroPixels.size();

            //Calculate the size of the rankZeroGrayScale vector
            rankZeroGrayScaleSize = numRankZeroPixels / channels;

            //Allocate space for the rankZeroGrayScale vector
            rankZeroGrayScale.assign(rankZeroGrayScaleSize, 0);

            //Convert the rankZeroPixels vector to gray scale
            convertToGrayScale(rankZeroPixels, rankZeroGrayScale, channels);

        }
        else if(option == "3") //Change contrast
        {
            if(argc != 4) //If there is no command line argument for contrast get it from the user
            {
                cout << "\nPlease enter a value greater than 0: ";
                cin >> contrast;
            }
            else
            {
                contrast = atoi(argv[3]);
            }

            while(contrast < 0)
            {
                cout << "\nInvalid input. Please enter a value greater than 0: ";
                cin >> contrast;
            }

            //broadcast the user input to the other cores
            MPI_Bcast(&contrast, 1, MPI_DOUBLE, 0, comm);

            //modify the rankZeroPixels
            changeContrast(rankZeroPixels, contrast, channels);

        }
        else //Change brightness
        {
            
            if(argc != 4) //If there is no command line argument for brightness get it from the user
            {
                cout << "\nPlease enter an integer in the interval [0, 255]: ";
                cin >> bright;
            }
            else
            {
                bright = atoi(argv[3]);
            }
            
            while(bright < 0 || bright > 255)
            {
                cout <<"\nValue not in the interval.\nPlease enter a value in the interval [0,255]: ";
                cin >> bright;
            }

            //broadcast the user input to the other cores
            MPI_Bcast(&bright, 1, MPI_INT, 0, comm);

            //modify the rankZeroPixels
            changeBrightness(rankZeroPixels, bright, channels);
        }

        //Need a edited image pointer
        unsigned char *editedImg;

        if(option != "2") //Create a resulting image with the same number of channels as the original
        {
            //Allocate memory for the edited image to receive the other processors pixels into
            editedImg = (unsigned char *) malloc(width * height * channels);
        }
        else //If the image is to be converted to gray scale, only one channel should be used
        {
            //Allocate memory for the edited image to receive the other processors pixels into
            editedImg = (unsigned char *) malloc(width * height * 1);
        }


        //Receive rows

        //Holds the index of the current pixel in the edited image
        currentPixel = 0;

        if(option != "2") //Get results from the rankZeroPixels vector
        {
            //Size of the rankZeroPixelsVector
            int rankZeroPixelsSize = rankZeroPixels.size();

            //Put all the pixels in the rankZeroPixels vector into the new image
            for(int i = 0; i < rankZeroPixelsSize; i++)
            {
                editedImg[currentPixel] = rankZeroPixels[i];
                currentPixel++;
            }
        }
        else //If option 2 was chosen, get results from the rankZeroGrayScale vector
        {
            for(int i = 0; i < rankZeroGrayScaleSize; i++)
            {
                editedImg[currentPixel] = rankZeroGrayScale[i];
                currentPixel++;
            }
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
        //End of receiving rows

        double elapsedTime = MPI_Wtime() - startTime;

        //Make the edited image's file name
        string newFileName = imgFileName.substr(0, imgFileName.rfind(".jpg"));

        //Add the corresponding ending to the new images name depending on the option
        if(option == "1")
        {
            newFileName = newFileName + "Sorted.jpg";
        }
        else if(option == "2")
        {
            newFileName = newFileName + "GrayScale.jpg";
        }
        else if(option == "3")
        {
            newFileName = newFileName + "UpdatedContrast.jpg";
        }
        else
        {
            newFileName = newFileName + "UpdatedBrightness.jpg";
        }

        if(option != "2") //Save an image with its original number of channels
        {
            //Save the edited image
            stbi_write_jpg(newFileName.c_str(), width, height, channels, editedImg, 100);
        }
        else //Save the image in gray scale
        {
            //Save the edited image
            stbi_write_jpg(newFileName.c_str(), width, height, 1, editedImg, 100);
        }

        //Print results
        cout << "\nIt took " << elapsedTime << " seconds with " << nproc << " processors to ";
        if(option == "1")
        {
            cout << "sort " << imgFileName << "'s pixels horizontally.\n";
        }
        else if(option == "2")
        {
            cout << "convert " << imgFileName << " to gray scale.\n";
        }
        else if(option == "3")
        {
            cout << "change " << imgFileName << "'s contrast.\n";
        }
        else
        {
            cout << "change " << imgFileName << "'s brightness.\n";
        }

        //Print the output images name
        cout << "\nOutput image is " << newFileName << "\n\n\n";

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

        //Receive width
        MPI_Recv(&width, 1, MPI_INT, 0, 20, comm, MPI_STATUS_IGNORE);
        //cout << "rank: " << rank << " width: " << width << "\n";

        //Receive height
        MPI_Recv(&height, 1, MPI_INT, 0, 21, comm, MPI_STATUS_IGNORE);
        //cout << "rank: " << rank << " height: " << height << "\n";

        //Receive channels
        MPI_Recv(&channels, 1, MPI_INT, 0, 22, comm, MPI_STATUS_IGNORE);
        //cout << "rank: " << rank << " channels: " << channels << "\n";

        //Receive the c string of option
        int option;
        MPI_Recv(&option, 1, MPI_INT, 0, 23, comm, MPI_STATUS_IGNORE);
        //cout << "rank: " << rank << " option: " << option << "\n";

        if(option == 1) //Sort horizontally
        {
        int rowLength = width * channels;

        int localWork;
        MPI_Recv(&localWork, 1, MPI_INT, 0, 30, comm, MPI_STATUS_IGNORE);

        for (int i = 0; i < localWork; i++)
        {
          int start = rowLength * i;
          int end = (rowLength * (i+1) - 1);
          sortHorizontal(localPixels, start, end);
        }
          //Send the number of pixels processed by this processor and send the processed pixels
          MPI_Send(&numPixels, 1, MPI_INT, 0, 14, comm);
          MPI_Send(&localPixels.front(), numPixels, MPI_UNSIGNED_CHAR, 0, 15, comm);
        }
        else if(option == 2) //Convert to gray scale
        {
            int grayScaleSize = numPixels / channels;

            vector<unsigned char> grayScale (grayScaleSize, 0);

            convertToGrayScale(localPixels, grayScale, channels);

            //Send the number of pixels processed by this processor and send the processed pixels
            MPI_Send(&grayScaleSize, 1, MPI_INT, 0, 14, comm);
            MPI_Send(&grayScale.front(), grayScaleSize, MPI_UNSIGNED_CHAR, 0, 15, comm);
        }
        else if(option == 3) //Change contrast
        {
            //receive the contrast input from the user from manager core
            MPI_Bcast(&contrast, 1, MPI_DOUBLE, 0, comm);

            changeContrast(localPixels, contrast, channels);

            //Send the number of pixels processed by this processor and send the processed pixels
            MPI_Send(&numPixels, 1, MPI_INT, 0, 14, comm);
            MPI_Send(&localPixels.front(), numPixels, MPI_UNSIGNED_CHAR, 0, 15, comm);
        }
        else //Change brightness
        {
            //recieve the brightness input from the user from the manager core
            MPI_Bcast(&bright, 1, MPI_INT, 0, comm);

            changeBrightness(localPixels, bright, channels);

            //Send the number of pixels processed by this processor and send the processed pixels
            MPI_Send(&numPixels, 1, MPI_INT, 0, 14, comm);
            MPI_Send(&localPixels.front(), numPixels, MPI_UNSIGNED_CHAR, 0, 15, comm);
        }

    } //End of else (other ranks done)


    //Clean up the MPI execution environment
    MPI_Finalize();

    return 0;

}

void convertToGrayScale(vector<unsigned char> &original, vector<unsigned char> &grayScale, int channels)
{

    //Start at the beginning of grayScale
    int curIndexGrayScale = 0;

    int originalSize = original.size();

    //Go through original and calculate the sum of the channels for the current pixels, then average
    //  the sum and store it in grayScale at the corresponding index
    for(int i = 0; i < originalSize; i += channels)
    {
        int sum = 0;
        int sumIndex = i;

        //Sum the channel values for the current pixel
        while(sumIndex < i + channels)
        {
            sum += original[sumIndex];
            sumIndex++;
        }

        //Average the sum and store it in grayScale
        grayScale[curIndexGrayScale] = (unsigned char)(sum / channels);
        curIndexGrayScale++;
    }

}

void changeContrast(vector<unsigned char> &original, double contrast, int channels)
{
    for(auto p = original.begin(); p != original.end(); p += channels)
            {
                //modifies contrast by the parameter contrast for each RBG component in a pixel
                *p = fmin(*p * contrast, 255);
                *(p + 1) = fmin(*(p + 1) * contrast, 255);
                *(p + 2) = fmin(*(p + 2) * contrast, 255);
            }
}

void changeBrightness(vector<unsigned char> &original, int bright, int channels)
{
    for(auto p = original.begin(); p != original.end(); p += channels)
            {
                //modifies brightness by the parameter bright for each RGB component in a pixel
                *p = fmin(*p + bright, 255);
                *(p + 1) = fmin(*(p + 1) + bright, 255);
                *(p + 2) = fmin(*(p + 2) + bright, 255);
            }
}
void sortHorizontal(vector <unsigned char> &original, int start, int end)
{
  // create vector to hold pixels to sort, add pixels to it from the original image
  vector<pixel> pixels;
  pixel temp;

  for (int i = start; i < end; i += 3)
  {
    temp.red = original[i];
    temp.green = original[i + 1];
    temp.blue = original[i + 2];
    pixels.push_back(temp);
  }

  // sort pixels
  insertionSort(pixels);
  // use the sorted pixels to refill the array in sorted order
  int size = pixels.size();
  for (int i = 0; i < size; i++)
  {
    original[start + (i * 3)] = pixels[i].red;
    original[start + ((i * 3) + 1)] = pixels[i].green;
    original[start + ((i * 3) + 2)] = pixels[i].blue;
  }


}
void insertionSort(vector <pixel> &pixels)
{
  int n = pixels.size();
  for (int i = 1; i < n; i++)
  {
    pixel key = pixels[i];
    int j = i - 1;
      while (j >= 0 && pixels[j] > key)
      {
        pixels[j + 1] = pixels[j];
        j = j - 1;
      }
      pixels[j + 1] = key;
  }
}
