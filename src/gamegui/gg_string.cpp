/*
    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/////////////////////////////////////////////////
//
//  File: gg_string.cpp
//
//  Basic string parsing
//
//  Author: Mike Linkovich
//
//  Start date: Feb 9 2000
//
////////////////////////////////


#include "gg_string.h"


//
//  GG_String funcs
//
void GG_String::resize( unsigned int n )
{
  if( n < 4 )
    n = 4;

  //  Same as previous capacity?
  if( n == m_capacity )
    return;

  //  Alloc space
  char  *newBuf = new char[n];

  //  Trunc string if we have to
  if( length() >= n )
    m_str[n-1] = '\0';

  strcpy( newBuf, m_str );
  delete[] m_str;
  m_str = newBuf;
  m_capacity = n;
}


void GG_String::trimWhiteSpace( char *str )
{
  int   i;
  char  *p = skipWhiteSpace( str );

  //  Do any leading whitespace..

  if( p > str )
    memmove( str, p, strlen(str) - (p - str) + 1 );

  //  Do trailing whitespace..

  for( i = strlen(str) - 1; i >= 0; i-- )
  {
    if( (str[i] == ' ') || (str[i] == '\t') ||
        (str[i] == '\n') || (str[i] == '\r') )
      str[i] = '\0';
    else
      return;   //  Hit a non-w.s. char, done
  }
}


void GG_String::capitalize( char *str )
{
  char  *p = str;
  while( *p != '\0' )
  {
    if( *p >= 'a' && *p <= 'z' )
      *p = 'A' + (*p - 'a');
    p++;
  }
}


void GG_String::lowerCase( char *str )
{
  char  *p = str;
  while( *p != '\0' )
  {
    if( *p >= 'A' && *p <= 'Z' )
      *p = 'a' + (*p - 'A');
    p++;
  }
}


char* GG_String::skipWhiteSpace( char *str )
{
  if( str == null )
    return null;
  char *p = str;
  while( (*p == ' ') || (*p == '\t') || (*p == '\n') || (*p == '\r') )
    p++;
  return p;
}


//  Return a pointer to the first whitespace character (or null) encountered.
//
char* GG_String::skipToWhiteSpace( char *str )
{
  if( str == null )
    return null;
  char *p = str;
  while( (*p != '\0') && (*p != ' ') && (*p != '\t') && (*p != '\n') && (*p != '\r') )
    p++;
  return p;
}


//  Finds the next word to skip, returns word AFTER. Eg:
//  "  skip word" returns a ptr to "word"
//  Stops on terminator (null)
//
char* GG_String::skipNextWord(char *str)
{
  if( str == null )
    return null;
  char *p = skipWhiteSpace(str);
  p = skipToWhiteSpace(p);
  p = skipWhiteSpace(p);

  return p;
}


char* GG_String::findQuote( char *str, int *len )
{
  int  start = 0,
       quoteLen = 0,
       sLen = strlen(str);

  while( start < sLen )
  {
    if( str[start] == '"' )
    {
      start++;    //  Move past " to 1st char in quote str

      //  Read characters until 2nd quote found or limit is reached..
      //
      while( (start + quoteLen) < sLen )
      {
        if( str[start + quoteLen] == '"' )
        {
          //  Found second quote, send back info..
          *len = quoteLen;
          return &str[start];
        }
        //  Keep looking for endquote
        quoteLen++;
      }
    }
    start++;
  }
  return null;    //  less than 2 quotes found
}


bool GG_String::getQuote( char *str, char *quote )
{
  int  len = 0;
  char *p = findQuote(str, &len);
  if( p == null )
    return false;

  memcpy( quote, p, len );
  quote[len] = '\0';
  return true;
}


/**
 * Skip whitespace in str, compare the following characters with
 * null-terminated wrd
 *
 * @param str String to skip whitespace
 * @param wrd String to compare against
 *
 * @return True on exact match
 */
bool
GG_String::cmpFirstWord( char *str, char *wrd )
{
  char *p = skipWhiteSpace(str);
  size_t  wrdLen = strlen(wrd);

  if( wrdLen <= strlen(p) )
    if( memcmp( p, wrd, wrdLen ) == 0 )
      return true;

  return false;
}


/**
 * Skip wrdId number of words in str, then compare the following characters
 * with null-terminated wrd
 *
 * @param str String to skip wrdId number of words
 * @param wrd String to compare against
 * @param wrdId Number of words in str to skip
 *
 * @return True on exact match
 */
bool
GG_String::cmpWord( char *str, char *wrd, uint32 wrdId )
{
  char    *p = str;
  uint32  wrdCount = 1;

  while( (wrdCount < wrdId) && ((p = skipNextWord(p)) != null) )
    wrdCount++;

  if( p == null )
    return false;

  size_t wrdLen = strlen(wrd);

  if( wrdLen <= strlen(p) )
    if( memcmp(wrd, p, wrdLen) == 0 )
      return true;

  return false;
}


bool GG_String::stripComment( char *str )
{
  int i, len = strlen(str);

  for( i = 0; i < len && str[i] != '\0'; i++ )
  {
    //  Note it is safe to do [i+1] because we haven't reached null term yet
    if( (str[i] == '/' && str[i+1] == '/') || str[i] == '\n' )
    {
      str[i] = '\0';
      break;
    }
  }

  trimWhiteSpace(str);
  return isComment(str);    //  Return true if this line is ONLY a comment
}


//  Verifies if the line contains no useful data
//  (i.e., only comments, newlines, spaces or tabs)
//
bool GG_String::isComment(char *str)
{
  char *p = str;

  if( *p == '\0' || *p == '\n' )
    return true;                    // 1st character is terminator or newline -- definitely Comment

  // Check line until a terminator or newline pops up

  while( (*p != '\0') && (*p != '\n') )
  {
    if( (*p == '/') && (*(p+1) == '/') )
      return true;                   // Found a '//' indicating comment
    if( *p != ' ' && *p != '\t' )
      return false;                  // A character other than newline, terminator, space, etc.
    p++;                             // space or '/' was found -- NOT a comment.
  }
  return true;
}


//  Returns a pointer to the first character of the actual file name..
//  all characters before that are part of the path.  If no path
//  is found, it returns str
//
char* GG_String::skipDOSPath( char *str )
{
  int   len = strlen(str);
  int   i;

  if( len < 3 )
    return str;

  //  Trace from end of string to front, until '\' found, or beginning of
  //  string reached.
  //
  for( i = len - 1; i > 0; i-- )
    if( str[i] == '\\' )
      return &str[i+1];

  return str;
}


//  Returns ptr to fist character match found, otherwise null
//
char* GG_String::findChar( char *str, char c )
{
  char *p = str;
  while( *p != c && *p != '\0' )
    p++;

  if( *p != c )
    return null;

  return p;
}


char* GG_String::getWord( char *str, char *wrd )    //  Word needs sufficient space allocated!
{
  char *p = skipWhiteSpace(str);
  char *pEnd = skipToWhiteSpace(p);

  if( pEnd <= p )
  {
    wrd[0] = '\0';
    return null;     //  No word found
  }

  memcpy( wrd, p, (pEnd-p) );
  wrd[pEnd-p] = '\0';

  return pEnd;
}



/***************************

    bool getFirstWord( char *wrd, Enumerator &en )
      {
        char *p = m_str;
        en.cPos = 0;

        if( p == null )
          return false;

        while( (*p != '\0') && (*p == ' ' || *p == '\t') && en.cPos < m_capacity )
          en.cPos++;

        if( *p == '\0' || en.cPos >= m_capacity )
          return false;

        uint32 cStart = en.cPos;

        while( (en.cPos < m_capacity) && (*p != '\0') &&
               (*p != ' ') && (*p != '\t') &&
               (*p != '
') && (*p != '\r') )
        {
          wrd[en.cPos - cStart] = *p;
          en.cPos++;
          p++;
        }

        return true;
      }

    bool getNextWord( char *wrd, Enumerator &en )
      {
        uint32 l = strlen(m_str);
        if( en.cPos >= l )
          return false;

        char *p = &m_str[en.cPos];

        while( (*p != '\0') && (*p == ' ' || *p == '\t') && en.cPos < l )
          en.cPos++;

        if( *p == '\0' || en.cPos >= l )
          return false;

        uint32 cStart = en.cPos;

        while( (en.cPos < l) && (*p != '\0') &&
               (*p != ' ') && (*p != '\t') &&
               (*p != '
') && (*p != '\r') )
        {
          wrd[en.cPos - cStart] = *p;
          en.cPos++;
          p++;
        }

        return true;
      }

    bool getWord( char *wrd, uint32 wrdId )
    {
      uint32  wrdCount = 0;
      char    *p = m_str;

      if( wrdId < 1 )
        return false;

      while( wrdCount < wrdId-1 )
      {
        if( (p = skipToNextWord(p)) == null )
          return false;
        wrdCount++;
      }

      if( sscanf( p, "%s", wrd ) < 1 )
        return false;

      return true;
    }

**************************/

