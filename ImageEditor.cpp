//IT 388
//Created by Ryan Leone, John Carnicle, Ben Sester
//Takes 

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <mpi.h>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"

using namespace std;

// sort the vector using bottom up mergesort
void mergesort(vector<long>& arr);

// add any function prototypes for any additional functions here
// merges the logical array in arr from indecies curLeft to curRight - 1 with the logical array in arr from indecies 
//      curRight to endRight and updates the arr, requires a temoprary vector temp
void merge(vector<long> &arr, vector<long> &temp, int curLeft, int curRight, int endRight);

int main(int argc, char** argv)
{

}