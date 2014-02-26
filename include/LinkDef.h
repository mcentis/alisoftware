// header file to steer the cint dictionary generation
// i feel a lot of magic here...

#include "vector"
#include "clusterDef.h"

#ifdef __CINT__
#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;
#pragma link C++ nestedclasses;

#pragma link C++ class cluster+;
#pragma link C++ class vector<cluster>+;

#endif // #ifdef __CINT__
