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

// macOS workarounds


#ifdef MACOSX

#include <math.h>

#include "macosxworkaround.hpp"
#include <string>
#include <fstream>
#include <iostream>

// includes for finding files
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <vector>

using namespace std;

/*

These are no longer needed with gcc 4.0

//float cosf(float a){ return cos(a); }
float acosf(float a){ return acos(a); }
//float sinf(float a){ return sin(a); }
float asinf(float a){ return asin(a); }
float tanf(float a){ return tan(a); }
float atanf(float a){ return atan(a); }
float atan2f(float a, float b){ return atan2(a, b); }
//float sqrtf(float a){ return sqrt(a); }
float powf(float a, float b){ return pow(a, b); }

// using gcc 3.3, the following functions are implemented in 
// /usr/lib/gcc/darwin/3.3/libstdc++.a

//float expf(float a){ return exp(a); }
//float logf(float a){ return log(a); }

*/

static char username[100];

namespace MacOSX {

const char* PlayerData_directory(){
  std::string dirname("");
  dirname += getenv("HOME");
  dirname += "/Documents/SoulRide/PlayerData";
  return dirname.c_str();
}

const char* whoami(){

  // add process id to filename to get a unique filename
  system("whoami > /tmp/.username.txt");
  
  FILE *in = fopen("/tmp/.username.txt","r");

  if (in == NULL ){ 
    printf("Cannot open /tmp/.username.txt\n");
    exit(1);
  }
  if (!feof(in)) {
    fscanf(in,"%s",username);
  }      

  fclose(in);
}

  int number_of_music_files = -1;
  vector<string> music_files;

  int GetMusicCount(const char* dir){

    // this avoids searching for files all the time
    if (music_files.size() == 0 && number_of_music_files < 0)
      GetMusicFiles(dir);
    
    number_of_music_files = music_files.size();
    
    return music_files.size();
  }

vector<string> GetMusicFiles(const char* dir){
  if (music_files.size() > 0)
    return music_files;
  
  DIR *directory;
  struct dirent *dirEntry;
  struct stat buff;
  int mp3_counter = 0, ogg_counter = 0;
  
  directory = opendir(dir);
  while (dirEntry = readdir(directory)) {

    stat (dirEntry->d_name,&buff);
    
    std::string path(dirEntry->d_name);
    int mp3_position = path.rfind(".mp3", path.length());
    int ogg_position = path.rfind(".ogg", path.length());
    if (mp3_position > 0 && buff.st_size > 0){
      music_files.push_back(path);
      mp3_counter++;
      cout << "Found the following mp3 file: " << dirEntry->d_name << endl;
    }
    if (ogg_position > 0 && buff.st_size > 0){
      music_files.push_back(path);
      ogg_counter++;
      cout << "Found the following ogg file: " << dirEntry->d_name << endl;
    }
  }
  
  closedir (directory); 
  
  cout << "There are " << mp3_counter << " mp3 and " 
       << ogg_counter  << " ogg files" << endl;

  GenerateMusicIndex();
  
  return music_files;
}

  void GenerateMusicIndex(){
    
    const char* filename = "/tmp/cdaindex.txt";

    ofstream outfile(filename);

    if (!outfile){
      cerr << "Could not open " << filename << " for writing" << endl; 
    }

    for (int i=0; i<music_files.size(); i++){
      outfile << (i+1) << ". " << music_files[i] << endl;
      outfile << "Artist Name" << endl;
      outfile << "Record Label" << endl;
      outfile << "Web site" << endl << endl;
    }

    outfile.close();
  }

} // namespace MacOSX

#endif // MACOSX
