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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
///////////////////////////////////////////////////////
//
//  Vector Editing class
//
//  Author: Mike Linkovich
//
//  Start Date: Feb 29 2000
//
///////////////////////////////


#ifndef _VECEDITOR_INCLUDED
#define _VECEDITOR_INCLUDED


#include "gamegui.h"
#include "gg_actor.h"
// #include <vector>


typedef int VE_Rval;

#define VE_DIRTY    1                 //  Means VE requires a repaint
#define VE_OK       0                 //  Return values
#define VE_ERR     -1


struct VE_Message
{
  int       id;
  GG_Point  p;
  GG_Point  p2;
};

#define VE_MSG_NONE         0         //  Possible message IDs
#define VE_MSG_MOUSE1DOWN   1         //  Position of cursor in p, p2.x is modifier flags
#define VE_MSG_MOUSE2DOWN   2         //     .
#define VE_MSG_MOUSE1UP     3         //     .
#define VE_MSG_MOUSE2UP     4         //     .
#define VE_MSG_MOUSEMOVE    5         //     .
#define VE_MSG_KEYDOWN      6         //  ID of key in p2.x
#define VE_MSG_KEYUP        7         //     .
#define VE_MSG_SETCOLOR     8         //  p, p2 are a 4-byte fp color (GG_ColorARGBf) cast &p
#define VE_MSG_DELETESEL    9         //  Delete selected

#define VE_MSGMOD_SHIFT     0x01      //  Flag indicates shift key down during msg
#define VE_MSGMOD_CTRL      0x02      //  ctrl key down during msg


#define VE_MOUSESTATE_UP    0         //  Mouse button states
#define VE_MOUSESTATE_DOWN  1

#define VE_MODE_EDITVERTICES  0       //  Editor mode


class VecEditor
{
  protected:
    vector<GG_Act_Poly::Vertex> *m_vtx;
    vector<int>   m_vtxStates;            //  Selected/deselected state of each vertex
    GG_Rect       m_rcScreen;
    GG_Vector2D   m_aspect;
    GG_Act_Poly   *m_poly;
    GG_Point      m_nodeSize;             //  Size of nodes, in pixels
    GG_Vector2D   m_nodeSizef;            //  World coords size
    GG_ColorARGBf m_clrPen,               //  Pen color
                  m_clrNode,              //  Node color
                  m_clrNodeSel,           //  Node selected color
                  m_clrBg;                //  Background color
    int           m_mouse1State,
                  m_mouse2State;
    GG_Point      m_mouse1DragStart,
                  m_mouse1DragPrev;
    GG_ActorState m_actorState;
    GG_Vector2D   m_statPos;              //  Coordinate to display in status string

    void screenToVec( int ix, int iy, GG_Vector2D *v );
    int findPoint( float x, float y, GG_Vector2D *pos );    //  Scene coordinates in, scene coords out
    int findPoint( int x, int y, GG_Vector2D *pos );        //  Screen coordinates in, scene coords out
    int findIntersect( const GG_Vector2D &pos );

  public:
    VecEditor( const GG_Rect &rc )
    {
      m_vtx = null;
      m_rcScreen = rc;
      m_rcScreen.getAspect( &m_aspect );
      m_poly = null;
      m_clrPen.set( 1.0, 1.0, 1.0, 1.0 );
      m_clrNode.set( 1.0, 0.75, 0.75, 0.75 );
      m_clrNodeSel.set( 1.0, 1.0, 0.95, 0.75 );
      m_clrBg.set( 0,0,0,0 );
      m_nodeSize.x = 6;
      m_nodeSize.y = 6;
      m_nodeSizef.x = (float)m_nodeSize.y / (m_rcScreen.y2 - m_rcScreen.y1);
      m_nodeSizef.y = m_nodeSizef.x;
      m_mouse1State = VE_MOUSESTATE_UP;
      m_mouse2State = VE_MOUSESTATE_UP;
      m_mouse1DragStart.set( 0,0 );
      m_mouse1DragPrev.set( 0,0 );
      m_statPos.set( 0,0 );
    }

    ~VecEditor()
    {
      clear();
    }

    void getStatusStr( char *buf )    //  Supply at least 64 chars
    {
      sprintf( buf, "%1.2f , %1.2f", m_statPos.x, m_statPos.y );
    }

    void getPenColor( GG_ColorARGBf *c )
    {
      *c = m_clrPen;
    }

    void clear(void)
    {
      if( m_poly != null )
      {
        delete m_poly;
        m_poly = null;
      }
      m_vtx = null;
      m_vtxStates.erase( m_vtxStates.begin(), m_vtxStates.end() );
    }

    VE_Rval read( char *filename );
    VE_Rval read( FILE *fp );
    VE_Rval read( GG_File *f );
    VE_Rval write( char *filename );
    VE_Rval write( FILE *fp );
    VE_Rval write( GG_File *f );
    VE_Rval handleMessage( const VE_Message &msg );
    VE_Rval paint(void);
};


#endif  //  _VECEDITOR_INCLUDED
