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
//  File: veceditor.cpp
//
//  Vector Editing class
//
//  Author: Mike Linkovich
//
//  Start Date: Feb 29 2000
//
///////////////////////////////


#include "veceditor.h"
#include "gg_actor.h"
#include "gg_string.h"
#include "gg_log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/gl.h>

#include <math.h>

#ifdef MACOSX
#include "../macosxworkaround.hpp"
#endif




static char logBuf[256] = "";


///////////////////////////////////////////////////////
//
//  Private functions (used internally only)
//
///////////////////////////////


void VecEditor::screenToVec( int ix, int iy, GG_Vector2D *v )
{
  int sw = m_rcScreen.x2 - m_rcScreen.x1,
      sh = m_rcScreen.y2 - m_rcScreen.y1;

  v->x = m_aspect.x * ((float)(ix - sw/2) / (float)(sw/2));
  v->y = m_aspect.y * ((float)((sh-iy) - sh/2) / (float)(sh/2));
}


//  Returns -1 if no point in range
//
int VecEditor::findPoint( float x, float y, GG_Vector2D *pos )
{
  if( m_poly == null )
    return -1;

  int i;

  vector<GG_Act_Poly::Vertex>::iterator v;

  for( i = 0, v = m_vtx->begin(); v < m_vtx->end(); v++, i++ )
  {
    if( fabs(x - v->p.x) < m_nodeSizef.x &&
        fabs(y - v->p.y) < m_nodeSizef.y )
    {
      if( pos != null )
        *pos = v->p;
      return i;
    }
  }

  return -1;
}


int VecEditor::findPoint( int ix, int iy, GG_Vector2D *pos )
{
  if( m_poly == null )
    return -1;

  GG_Vector2D vec;
  screenToVec( ix, iy, &vec );

  return findPoint( vec.x, vec.y, pos );
}


//  Returns index of point intersecting after,
//  or -1 if no intersection found
//
int VecEditor::findIntersect( const GG_Vector2D &pos )
{
  if( m_poly == null )
    return -1;

  int           i;
  GG_LineSeg2D  t1, t2,   //  Test lines -- use an 'x' to check against each poly edge
                l;
  t1.set( pos.x - m_nodeSizef.x, pos.y - m_nodeSizef.y,
          pos.x + m_nodeSizef.x, pos.y + m_nodeSizef.y );
  t2.set( pos.x + m_nodeSizef.x, pos.y - m_nodeSizef.y,
          pos.x - m_nodeSizef.x, pos.y + m_nodeSizef.y );

  for( i = 0; i < m_vtx->size(); i++ )
  {
    l.p1 = (*m_vtx)[i].p;
    l.p2 = (*m_vtx)[(i+1) % m_vtx->size()].p;

    if( l.intersect(t1, null) || l.intersect(t2, null) )
    {
      return i;
    }
  }

  return -1;
}


///////////////////////////////////////////////////////
//
//  Public functions
//
///////////////////////////////


VE_Rval VecEditor::read( GG_File *f )
{
  int         i = 0;
  char        buf[256] = "";
  GG_Act_Poly *poly = new GG_Act_Poly(m_aspect);

  //  Find POLYGON identifier, otherwise file is bogus
  while( f->readLine( buf, 250 ) )
  {
    if( GG_String::stripComment(buf) )
      continue;
    GG_String::capitalize(buf);
    if( strcmp(buf, "POLYGON") == 0 )
      break;

    return VE_ERR;
  }

  if( poly->read(f) != GG_OK )
  {
    delete poly;
    return VE_ERR;
  }

  if( m_poly != null )
    delete m_poly;
  m_poly = poly;

  m_vtx = m_poly->getVertices();
  m_vtxStates.erase( m_vtxStates.begin(), m_vtxStates.end() );
  for( i = 0; i < m_vtx->size(); i++ )
    m_vtxStates.push_back(0);

  return VE_OK;
}


VE_Rval VecEditor::read( FILE *fp )
{
  GG_File *f = GG_FileAliasFILE( fp );
  VE_Rval rval = read( f );
  f->close();
  return rval;
}


VE_Rval VecEditor::read( char *filename )
{
  VE_Rval rval;
  GG_File *f = GG_FileOpen( filename, GGFILE_OPENTXT_READ );
  if( f == null )
  {
    GG_LOG( "error: Failed to open file:" );
    GG_LOG( filename );
    return VE_ERR;
  }

  rval = read(f);

  f->close();
  return rval;
}


VE_Rval VecEditor::write( GG_File *f )
{
  if( m_poly != null )
    if( m_poly->write(f) != GG_OK )
      return VE_ERR;

  return VE_OK;
}


VE_Rval VecEditor::write( FILE *fp )
{
  GG_File *f = GG_FileAliasFILE( fp );
  VE_Rval rval = write( f );
  f->close();
  return rval;
}


VE_Rval VecEditor::write( char *filename )
{
  VE_Rval rval;
  GG_File *f = GG_FileOpen( filename, GGFILE_OPENTXT_WRITENEW );
  if( f == null )
  {
    return VE_ERR;
  }

  rval = write(f);

  f->close();
  return rval;
}


VE_Rval VecEditor::paint(void)
{
  float         x, y;
  int           w, h;
  GG_ColorARGBf col;
  vector<int>::iterator   vs;
  vector<GG_Act_Poly::Vertex>::iterator v;


  //  Clear background color..
  //
  glClearColor( m_clrBg.r, m_clrBg.g, m_clrBg.b, m_clrBg.a );
  glClear( GL_COLOR_BUFFER_BIT );

  glPushMatrix();

  w = m_rcScreen.x2 - m_rcScreen.x1;    //  Figure out the aspect ratio
  h = m_rcScreen.y2 - m_rcScreen.y1;    //   then scale appropriately

  if( w > h )
  {
    x = (float)h / w;
    y = 1.0f;
  }
  else
  {
    x = 1.0f;
    y = (float)w / h;
  }
  glScalef( x, y, 1.0f );

  //  Draw the poly...
  //
  if( m_poly != null )
  {
    m_poly->doFrame( m_actorState, 0, 0 );

    //  Display the nodes...
    //
    for( v = m_vtx->begin(), vs = m_vtxStates.begin();
         v < m_vtx->end(); v++, vs++ )
    {
      glBegin(GL_LINE_LOOP);

      if( *vs == 1 )          //  Node selected?  Pick color
        col = m_clrNodeSel;
      else
        col = m_clrNode;

      //  Draw square around node
      glColor3f( col.r, col.g, col.b );
      glVertex2f( v->p.x - m_nodeSizef.x, v->p.y + m_nodeSizef.y );
      glColor3f( col.r, col.g, col.b );
      glVertex2f( v->p.x + m_nodeSizef.x, v->p.y + m_nodeSizef.y );
      glColor3f( col.r, col.g, col.b );
      glVertex2f( v->p.x + m_nodeSizef.x, v->p.y - m_nodeSizef.y );
      glColor3f( col.r, col.g, col.b );
      glVertex2f( v->p.x - m_nodeSizef.x, v->p.y - m_nodeSizef.y );

      glEnd();
    }
  }

  glPopMatrix();

  return VE_OK;
}


VE_Rval VecEditor::handleMessage( const VE_Message &msg )
{
  VE_Rval rval = VE_OK;

  switch( msg.id )
  {
    case VE_MSG_MOUSE1DOWN:
      if( m_poly != null )
      {
        int i, j;
        m_mouse1State = VE_MOUSESTATE_DOWN;
        m_mouse1DragStart = msg.p;
        m_mouse1DragPrev = msg.p;
        i = findPoint( msg.p.x, msg.p.y, &m_statPos );    // Supplies offset from picked point
        if( i >= 0 )
        {
          m_clrPen = (*m_vtx)[i].c;
          if( (msg.p2.x & (VE_MSGMOD_SHIFT | VE_MSGMOD_CTRL)) != 0 )
            m_vtxStates[i] = !m_vtxStates[i]; // ctrl or shift down, toggle point state
          else
          {
            if( m_vtxStates[i] == 0 )         //  Otherwise, is it already selected?
            {                                 //  no, desel all, sel this one
              if( (msg.p2.x & (VE_MSGMOD_SHIFT | VE_MSGMOD_CTRL)) == 0 )
                for( j = 0; j < m_vtxStates.size(); j++ )
                  m_vtxStates[j] = 0;
              m_vtxStates[i] = 1;
            }
          }
        }
        else    //  no point in range of mouse.  If no ctrl or shift down,
        {       //  desel. all points...
          if( (msg.p2.x & (VE_MSGMOD_SHIFT | VE_MSGMOD_CTRL)) == 0 )
            for( i = 0; i < m_vtxStates.size(); i++ )
              m_vtxStates[i] = 0;
          screenToVec( msg.p.x, msg.p.y, &m_statPos );
          m_statPos.set(0,0);
        }

        return VE_DIRTY;
      }
      return VE_OK;

    case VE_MSG_MOUSE2DOWN:
      if( m_poly != null )
      {
        int i;
        screenToVec( msg.p.x, msg.p.y, &m_statPos );

        if( (i = findPoint(m_statPos.x, m_statPos.y, null)) >= 0 )
        {
          m_vtx->erase( m_vtx->begin() + i );
          m_vtxStates.erase( m_vtxStates.begin() + i );
          return VE_DIRTY;   //  Hit a vertex -- delete it
        }

        if( (i = findIntersect(m_statPos)) >= 0 )
        {
          //  De-select all other points
          for( vector<int>::iterator j = m_vtxStates.begin(); j < m_vtxStates.end(); j++ )
            *j = 0;
          GG_Act_Poly::Vertex newVtx;
          newVtx.p = m_statPos;
          newVtx.c = m_clrPen;
          newVtx.t = newVtx.p;
          m_vtx->insert( m_vtx->begin() + i+1, newVtx );
          m_vtxStates.insert( m_vtxStates.begin() + i+1, 1 ); //  Add a new (selected) point
          return VE_DIRTY;
        }
      }
      return VE_OK;

    case VE_MSG_MOUSEMOVE:
      if( m_poly != null && m_mouse1State == VE_MOUSESTATE_DOWN )
      {
        bool bChanged = false;
        int i;
        int dx = msg.p.x - m_mouse1DragPrev.x;
        int dy = msg.p.y - m_mouse1DragPrev.y;
        float fx =  m_aspect.x * (dx / ((m_rcScreen.x2 - m_rcScreen.x1) * 0.5f));
        float fy = -m_aspect.y * (dy / ((m_rcScreen.y2 - m_rcScreen.y1) * 0.5f));

        m_statPos.x += fx;
        m_statPos.y += fy;

        for( i = 0; i < m_vtx->size(); i++ )
        {
          if( m_vtxStates[i] != 0 )
          {
            (*m_vtx)[i].p.x += fx;
            (*m_vtx)[i].p.y += fy;
            bChanged = true;
          }
        }

        m_mouse1DragPrev = msg.p;
        if( bChanged )
          return VE_DIRTY;
      }
      else
      {
        screenToVec( msg.p.x, msg.p.y, &m_statPos );
      }
      return VE_OK;

    case VE_MSG_MOUSE1UP:
      m_mouse1State = VE_MOUSESTATE_UP;
      screenToVec( msg.p.x, msg.p.y, &m_statPos );
      return VE_OK;

    case VE_MSG_SETCOLOR:     //  Color selected
      m_clrPen = *(GG_ColorARGBf*)&msg.p;
      m_clrPen.clamp();
      if( m_poly != null )
      {
        for( int i = 0; i < m_vtx->size(); i++ )
        {
          if( m_vtxStates[i] == 1 )   //  Color any selected nodes
          {
            (*m_vtx)[i].c = m_clrPen;
            rval = VE_DIRTY;
          }
        }
      }
      return rval;

    case VE_MSG_DELETESEL:
      if( m_poly != null )
      {
        int i;
        for( i = (int)m_vtx->size() - 1; i >= 0; i-- )
        {
          if( m_vtxStates[i] == 1 )
          {
            m_vtxStates.erase( m_vtxStates.begin() + i );
            m_vtx->erase( m_vtx->begin() + i );
            rval = VE_DIRTY;
          }
        }
      }
      return rval;
  }

  return VE_ERR;
}
