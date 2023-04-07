#include <iostream>
#include <fstream>
#include <cstdlib>
#include <regex>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std;

int main(int argc, char** argv){
    //Serial implementation of changing the brightness and contrast of an image
    int width, height, channels, bright; //dimensions of image, bright holds input from user for changing brightness
    double contrast;  // holds input from user for changning contrast
    string fileName, opt; //name of the input image, opt holds input of user for program function option
    unsigned char *orgImg; //pointer to input image

    cout << "Please provide the name of the JPEG image to be edited: ";
    cin >> fileName;
    regex nameCheck("^\\w+\\.jpg$"); 
    while(!regex_match(fileName, nameCheck))
    {   //checks to make sure input file ends with .jpg --- \\w+ => a string of at least one alphanumeric (a-z, A-Z, 0-9, _)
        cout << "Incorrect File: Make sure file is a JPEG! Please provide the correct file name: ";
        cin >> fileName;
    }
    
    orgImg = stbi_load(fileName.c_str(), &width, &height, &channels, 0);
    while(orgImg == NULL){
        cout << "\nError occurred loading image. Please try again.\n" << endl;
        cout << "Please provide the name of the JPEG image to be edited: ";
        cin >> fileName;
        orgImg = stbi_load(fileName.c_str(), &width, &height, &channels, 0);
    }

    cout << "\nInput Image: " << fileName << "\nHeight = " << height << "\nWidth = " << width << "\nChannels = " << channels << endl;
    unsigned char *modImg = (unsigned char *) malloc(height * width * channels); //pointer to modified image
    
    //create and store new file name
    char modFileName [sizeof(fileName) + sizeof("Edited.jpg")];
    sprintf(modFileName, "%sEdited.jpg", fileName.substr(0, fileName.length() - 4).c_str());

    cout << "\nPlease Select One of the Options: " << endl;
    cout << "1. Change the Brightness of the Image" << endl;
    cout << "2. Change the Contrast of the Image" << endl;
    cout << "3. Change both the Brightness and Contrast of the Image" << endl;
    cout << "Enter an option: ";
    cin >> opt;

    while(opt != "1" && opt != "2" && opt != "3")
    {
        cout << "\nInvalid Choice. Please enter a valid option: ";
        cin >> opt;
    }

    if(opt == "1")
    {
        cout << "\nPlease enter a value in the interval [0, 255]: ";
        cin >> bright;
        while(bright < 0 || bright > 255)
        {
            cout <<"\nValue not in the interval.\nPlease enter a value in the interval [0.255]: ";
            cin >> bright;
        }
        for(unsigned char *p = orgImg, *np = modImg; p != orgImg + (width * height * channels); p+= channels, np += channels)
        {   //Loops through each pixel, and each RGB value is increased by the value provided by the user. 
            //Goes to 255 if the sum of original and user input is greater than 255
            *np = fmin(*p + bright, 255);
            *(np + 1) = fmin(*(p + 1) + bright, 255);
            *(np + 2) = fmin(*(p + 2) + bright, 255);
        }

        stbi_write_jpg(modFileName, width, height, channels, modImg, 100); //write the new image
    }
    else if(opt == "2")
    {
        cout << "\nPlease enter a value greater than 0: ";
        cin  >> contrast;

        while(contrast < 0)
        {
            cout << "\nInvalid input. Please enter a value greater than 0: ";
            cin >> contrast;
        }

        for(unsigned char *p = orgImg, *np = modImg; p != orgImg + (width * height * channels); p+= channels, np += channels)
        {   //Loops through each pixel, and each RGB value is scaled by the value provided by the user. 
            //Goes to 255 if the product of original and user input is greater than 255
            *np = fmin(*p * contrast, 255);
            *(np + 1) = fmin(*(p + 1) * contrast, 255);
            *(np + 2) = fmin(*(p + 2) * contrast, 255);
        }

        stbi_write_jpg(modFileName, width, height, channels, modImg, 100); //write the new image
    }
    else //opt == 3
    {   //Get both the brightness and contrast
        cout << "\nPlease enter a value in the interval [0, 255]: ";
        cin >> bright;
        while(bright < 0 || bright > 255)
        {
            cout <<"\nValue not in the interval.\nPlease enter a value in the interval [0.255]: ";
            cin >> bright;
        }

        cout << "\nPlease enter a value greater than 0: ";
        cin  >> contrast;

        while(contrast < 0)
        {
            cout << "\nInvalid input. Please enter a value greater than 0: ";
            cin >> contrast;
        }

        for(unsigned char *p = orgImg, *np = modImg; p != orgImg + (width * height * channels); p+= channels, np += channels)
        {   //Loops through each pixel, and each RGB value is scaled and increased by the values provided by the user. 
            //Goes to 255 if the new value is greater than 255
            *np = fmin((*p * contrast) + bright, 255);
            *(np + 1) = fmin((*(p + 1) * contrast) + bright, 255);
            *(np + 2) = fmin((*(p + 2) * contrast) + bright, 255);
        }

        stbi_write_jpg(modFileName, width, height, channels, modImg, 100); //write the new image
    }
    
    stbi_image_free(orgImg);
    stbi_image_free(modImg);
    return 0;
}