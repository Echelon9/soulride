// Add header here (GPL, etc...)

#ifdef MACOSX

#include <string>
#include <vector>

//#include <iostream>
//using namespace std;

/*
float cosf(float a);
float acosf(float a);
float sinf(float a);
float asinf(float a);
float tanf(float a);
float atanf(float a);
float atan2f(float a, float b);
float sqrtf(float a);
float powf(float a, float b);
float expf(float a);
float logf(float a);
*/

namespace MacOSX {

  const char* PlayerData_directory();
  const char* whoami();

  int GetMusicCount(char* dir);
  std::vector<std::string> GetMusicFiles(char* dir);
  void GenerateMusicIndex();

}

#endif // MACOSX
