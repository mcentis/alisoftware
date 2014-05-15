#ifndef CONSTANTS_H
#define CONSTANTS_H

// modify the construction of the branches when these cont are changed
const int nChannels = 256; // number of chennels
const int totHeadBits = 32; // number of header bits for all the chips 
const int nParameters = 2; // paramters to fit the calibration
const char* const fitCal = "pol1"; // fit the calibration
const char* const convertToe = "(x - [0]) / [1]"; // convert to alibava e-

// first const: the pointed value is constant, second const: the pointer is constant
// this syntax is needed to compile the code

#endif // #ifndef CONSTANTS_H
