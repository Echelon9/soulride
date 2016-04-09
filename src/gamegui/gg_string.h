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
//  File: gg_string.h
//
//  Basic string parsing
//
//  Author: Mike Linkovich
//
//  Start date: Feb 9 2000
//
////////////////////////////////


#ifndef _GG_STRING_INCLUDED
#define _GG_STRING_INCLUDED


#include "gg_types.h"
#include <string.h>


#define GGSTRING_DEFAULT_SIZE 256


class GG_String
{
  private:
    char    *m_str;       //  Ptr to string data
    uint32  m_capacity;   //  Space allocated

  public:
    //
    //  Constructors
    //
    GG_String()
    {
      m_capacity = 0;
      m_str = null;
    }

    GG_String( uint32 capacity )
    {
      if( capacity < 4 )
        m_capacity = 4;
      else
        m_capacity = capacity;

      m_str = new char[m_capacity];
      m_str[0] = '\0';
    }

    GG_String( char *s )
    {
      m_capacity = strlen(s);
      if( m_capacity < 4 )
        m_capacity = 4;
      m_str = new char[m_capacity];

      strcpy( m_str, s );
    }

    ~GG_String()
    {
      if( m_str != null )
        delete[] m_str;
    }

    //
    //  management funcs
    //
    void resize( unsigned int n );

    //
    //  Get a regular char pointer to the string
    //
    inline char *c_str(void)
    {
      return m_str;
    }

    inline int capacity(void)
    {
      return m_capacity;
    }

    inline size_t length(void)
    {
      return strlen(m_str);
    }

    //
    //  Override =
    //
    void operator=( const GG_String &s )
    {
      uint32  l = strlen(s.m_str);
      if( l != m_capacity )
        resize(l);
      strcpy( m_str, s.m_str );
    }

    void operator=( char *s )
    {
      uint32  l = strlen(s);
      if( l >= m_capacity )
        resize(l);
      strcpy( m_str, s );
    }

    //
    //  String Functions
    //  Static, and member
    //

    //  Trim all leading spaces, tabs, newlines, as well as trailing ones

    static void trimWhiteSpace( char *str );

    inline void trimWhiteSpace(void)
    {
      trimWhiteSpace(m_str);
    }


    static void capitalize( char *str );

    inline void capitalize(void)
    {
      capitalize(m_str);
    }


    static void lowerCase( char *str );

    inline void lowerCase(void)
    {
      lowerCase(m_str);
    }

    //  Return a pointer to the first non-whitespace
    //  character encountered (or terminator)
    //
    static char *skipWhiteSpace( char *str );

    inline char *skipWhiteSpace()
    {
      return skipWhiteSpace( m_str );
    }


    //  Return a pointer to the first whitespace character (or null) encountered.
    //
    static char *skipToWhiteSpace( char *str );

    inline char *skipToWhiteSpace()
    {
      return skipToWhiteSpace(m_str);
    }


    //  Finds the next word to skip, returns word AFTER. Eg:
    //  "  skip word" returns a ptr to "word"
    //  Stops on terminator (null)
    //
    static char *skipNextWord(char *str);

    inline char *skipNextWord(void)
    {
      return skipNextWord(m_str);
    }

    //  Returns pointer to 1st character within first quote " " pair found.
    //  len is filled with length of quote (not incl. " chars)
    //
    //  Null returned if " " pair not found
    //  Additional " " pairs ignored
    //
    static char *findQuote( char *str, int *len );

    inline char *findQuote( int *len )
    {
      return findQuote( m_str, len );
    }


    static bool getQuote( char *str, char *quote );

    inline bool getQuote( char *quote )
    {
      return getQuote( m_str, quote );
    }


    //  Skip whitespace in str, compare the following characters with
    //  null-terminated wrd, return true on exact match
    //
    static bool cmpFirstWord( char *str, char *wrd );

    inline bool cmpFirstWord( char *wrd )
    {
      return cmpFirstWord( m_str, wrd );
    }


    static bool cmpWord( char *str, char *wrd, uint32 wrdId );

    inline bool cmpWord( char *wrd, uint32 wrdId )
    {
      return cmpWord( m_str, wrd, wrdId );
    }


    //  Strips comment from line, AS WELL AS
    //  any leading/trailing whitespace!  This leaves
    //  us with a string that contains only the useful stuff.
    //  Returns true if the result string is a comment, false
    //  if it contains characters
    //
    static bool stripComment( char *str );

    inline void stripComment(void)
    {
      stripComment(m_str);
    }


    //  Verifies if the line contains no useful data
    //  (i.e., only comments, newlines, spaces or tabs)
    //
    static bool isComment(char *str);

    inline bool isComment(void)
    {
      return isComment( m_str );
    }


    //  Returns a pointer to the first character of the actual file name..
    //  all characters before that are part of the path.  If no path
    //  is found, it returns str
    //
    static char *skipDOSPath( char *str );

    inline char *skipDOSPath(void)
    {
      return skipDOSPath( m_str );
    }


    //  Returns ptr to fist character match found, otherwise null
    //
    static char *findChar( char *str, char c );

    inline char *findChar( char c )
    {
      return findChar(m_str, c);
    }


    static char *getWord( char *str, char *wrd );

    inline char *getWord( char *wrd )
    {
      return getWord( m_str, wrd );
    }

};


#endif  // _GG_STRING_INCLUDED
