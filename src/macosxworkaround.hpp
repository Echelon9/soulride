/*
    Copyright 2003 Bjorn Leffler

    This file is part of The Soul Ride Engine, see http://soulride.com

    The Soul Ride Engine is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    The Soul Ride Engine is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

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

  int GetMusicCount(const char* dir);
  std::vector<std::string> GetMusicFiles(const char* dir);
  void GenerateMusicIndex();

}

#endif // MACOSX
