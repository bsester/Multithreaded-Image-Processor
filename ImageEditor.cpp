//IT 388
//Created by Ryan Leone, John Carnicle, Ben Sester
//Takes in the name of an image file and gives the user the options to
//sort the pixels of an image row or column wise or to change the contrast
//and brightness of an image

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <mpi.h>

using namespace std;

// sort the vector using bottom up mergesort
void mergesort(vector<long>& arr);

// merges the logical array in arr from indecies curLeft to curRight - 1 with the logical array in arr from indecies 
//      curRight to endRight and updates the arr, requires a temoprary vector temp
void merge(vector<long> &arr, vector<long> &temp, int curLeft, int curRight, int endRight);

int main(int argc, char** argv)
{

    //Getting the name of the image file from the user
    cout << "Please input the name of the image file to be edited: ";
    string imageFileName;
    cin >> imageFileName;

    cout << "The name of the image file is " << imageFileName << "\n";
    
    //String that holds the option that the user chose
    string option = "";

    //Keep performing operations on the image until the user decides to exit
    while(option != "3")
    {
        
        //Printing out options
        cout << "\nPossible options:\n";
        cout << "1. Sort image's pixels\n";
        cout << "2. Change image's contrast and brightness\n";
        cout << "3. Exit\n";
        cout << "Please enter an option (1, 2, or 3): ";

        //Putting the user's input into the option string
        cin >> option;

        //If the user's input was invalid keep propting the user for valid input
        while(option != "1" && option != "2" && option != "3")
        {
            cout << "\nInvalid input. Please try again. Possible options:\n";
            cout << "1. Sort image's pixels\n";
            cout << "2. Change image's contrast and brightness\n";
            cout << "3. Exit\n";
            cout << "Please enter an option (1, 2, or 3): ";

            //Putting the user's input into the option string
            cin >> option;
        }

    }

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
