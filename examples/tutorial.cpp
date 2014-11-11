// A simple program that computes the square root of a number
#include <cmath>
#include <iostream>
#include <string>
#include "TutorialConfig.h"
#ifdef USE_MYMATH
#include "MathFunctions.h"
#endif

int main (int argc, char *argv[])
{
  if (argc < 2)
    {

      std::cout<<Tutorial_VERSION_MAJOR<<" Version "<<Tutorial_VERSION_MINOR<<std::endl;

      //fprintf(stdout,"%s Version %d.%d\n",
       //     argv[0],
       //     Tutorial_VERSION_MAJOR,
       //     Tutorial_VERSION_MINOR);
    //fprintf(stdout,"Usage: %s number\n",argv[0]);
      std::cout<<"Usage: "<<argv[0]<<std::endl;
    return 1;
    }
  double inputValue = std::stod(argv[1]);
#ifdef USE_MYMATH
  double outputValue = mysqrt(inputValue);
#else
  double outputValue = sqrt(inputValue);
#endif
  //fprintf(stdout,"The square root of %g is %g\n",
  //        inputValue, outputValue);
  std::cout<<"The square root of "<<inputValue<<" is "<<outputValue<<std::endl;
  return 0;
}
