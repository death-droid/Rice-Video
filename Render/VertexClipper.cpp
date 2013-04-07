/*
Copyright (C) 2005 Rice1964

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "stdafx.h"
#include "float.h"

//OPTIMIZE ME

//////////////////////////////////////////////////////////////////////////
/*
 *	Manual vertex clipper
 */
//////////////////////////////////////////////////////////////////////////

void CopyVertexData(int oriidx, TLITVERTEX *oribuf, int dstidx, TLITVERTEX *dstbuf)
{
	memcpy(dstbuf+dstidx,oribuf+oriidx, sizeof(TLITVERTEX));
}

#define interp(a,b,r)	(((a)-(r)*(b))/	(1-(r)))
void SetVtx(TLITVERTEX &v, TLITVERTEX &v1, TLITVERTEX &v2, float r)
{
	v.x = interp(v1.x,v2.x,r);
	v.y = interp(v1.y,v2.y,r);
	//v.z = (v1.z-r*v2.z)/(1-r);
	v.dcSpecular = v2.dcSpecular; //fix me here
	v.r = (uint8)(interp((int)v1.r,(int)v2.r,r));
	v.g = (uint8)(interp((int)v1.g,(int)v2.g,r));
	v.b = (uint8)(interp((int)v1.b,(int)v2.b,r));
	v.a = (uint8)(interp((int)v1.a,(int)v2.a,r));

	for( int i=0; i<2; i++ )
	{
		v.tcord[i].u = interp(v1.tcord[i].u,v2.tcord[i].u,r);
		v.tcord[i].v = interp(v1.tcord[i].v,v2.tcord[i].v,r);
	}
}

void SwapVertexPos(int firstidx)
{
	TLITVERTEX &v1 = g_vtxBuffer[firstidx];
	TLITVERTEX &v2 = g_vtxBuffer[firstidx+1];
	TLITVERTEX &v3 = g_vtxBuffer[firstidx+2];

	if( v1.rhw >= v2.rhw && v1.rhw >= v3.rhw ) return;

	TLITVERTEX tempv;
	memcpy(&tempv,&g_vtxBuffer[firstidx], sizeof(TLITVERTEX));

	if( v2.rhw > v1.rhw && v2.rhw >= v3.rhw )
	{
		// v2 is the largest one
		memcpy(&g_vtxBuffer[firstidx],&g_vtxBuffer[firstidx+1], sizeof(TLITVERTEX));
		memcpy(&g_vtxBuffer[firstidx+1],&g_vtxBuffer[firstidx+2], sizeof(TLITVERTEX));
		memcpy(&g_vtxBuffer[firstidx+2],&tempv, sizeof(TLITVERTEX));
	}
	else
	{
		memcpy(&g_vtxBuffer[firstidx],&g_vtxBuffer[firstidx+2], sizeof(TLITVERTEX));
		memcpy(&g_vtxBuffer[firstidx+2],&g_vtxBuffer[firstidx+1], sizeof(TLITVERTEX));
		memcpy(&g_vtxBuffer[firstidx+1],&tempv, sizeof(TLITVERTEX));
	}
}

/************************************************************************/
/* Manually clipping vertexes                                           */
/* DirectX won't clip transformed vertex, unless the vertex is transfor-*/
/* med by DirectX, so we have to do vertex clipping ourself. Otherwise  */
/* this plugin works very bad on newest video cards, like ATI Radeons.  */
/************************************************************************/
bool Clip1TriangleForNegW(TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, int &dstidx);
bool Clip1TriangleForZ(TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, int &dstidx);

void ClipVertexes()
{
	// transverse the list of transformed vertex for each triangles
	// - If z values of all 3 vertexes of the triangle are greater than 1, then ignore this triangle
	// - If non z value of all 3 vertexes of the triangle is greater than 1, then this triangle is ok
	// - If one or two z values are greater than 1, then split this triangle to 2 triangles

	int dstidx = 0;

	for( uint32 i=0; i<gRSP.numVertices/3; i++)
	{
		int firstidx = i*3;
		TLITVERTEX &v1 = g_vtxBuffer[firstidx];
		TLITVERTEX &v2 = g_vtxBuffer[firstidx+1];
		TLITVERTEX &v3 = g_vtxBuffer[firstidx+2];

		if( v1.rhw < 0 && v2.rhw < 0 && v3.rhw < 0 )
			continue;	// Skip this triangle

		bool zeldaTempFix = false;
		if(options.enableHackForGames != HACK_FOR_ZELDA)
		{
			if( v1.rhw >= 0 && v2.rhw >= 0 && v3.rhw >= 0)
				zeldaTempFix = true;
		} else
		{
			if( v1.rhw >= 0 && v2.rhw >= 0 && v3.rhw >= 0 &&
				v1.x>=0 && v1.x < windowSetting.uViWidth && v1.y>=0 && v1.y<windowSetting.uViHeight &&
				v2.x>=0 && v2.x < windowSetting.uViWidth && v2.y>=0 && v2.y<windowSetting.uViHeight &&
				v3.x>=0 && v3.x < windowSetting.uViWidth && v3.y>=0 && v3.y<windowSetting.uViHeight )
					zeldaTempFix = true;
		}

		if( zeldaTempFix)	
		{
			if( v1.z < 0 || v2.z < 0 || v3.z < 0 )
			{
				if( !gRDP.otherMode.z_cmp )
				{
					// Do nothing about this triangle
					CopyVertexData(firstidx, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
					CopyVertexData(firstidx+1, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
					CopyVertexData(firstidx+2, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
					continue;
				}
				else if( v1.z < 0 && v2.z < 0 && v3.z < 0 )
					continue;
				else
				{
					Clip1TriangleForZ(v1, v2, v3, dstidx);
					continue;
				}
			}
			else if( v1.z > 1 || v2.z > 1 || v3.z > 1 )
			{
				if( v1.z > 1 && v2.z > 1 && v3.z > 1 )
					continue;
				else
				{
					Clip1TriangleForZ(v1, v2, v3, dstidx);
					continue;
				}
			}
			else
			{
				// Do nothing about this triangle
				CopyVertexData(firstidx, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
				CopyVertexData(firstidx+1, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
				CopyVertexData(firstidx+2, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
			}
			continue;
		}

		SwapVertexPos(firstidx);

		Clip1TriangleForNegW(v1, v2, v3, dstidx);
	}

	g_clippedVtxCount = dstidx;
}

typedef struct {
	double d;
	double x;
	double y;
} LineEuqationType;
LineEuqationType lines[3];	// In the format: Nx, Ny, d
double EvaLine(LineEuqationType &li, double x, double y)
{
	return li.x*x+li.y*y+li.d;
}


inline D3DXVECTOR3 Split( D3DXVECTOR3 &a, D3DXVECTOR3 &b, LineEuqationType &line  )
{
	double aDot = (a.x*line.x + a.y*line.y);
	double bDot = (b.x*line.x + b.y*line.y);

	double scale = ( - line.d - aDot) / ( bDot - aDot );

	return a + ((b - a) * (float)scale );
}

// Clipping using the Sutherland-Hodgeman algorithm
bool ClipFor1LineXY( std::vector<D3DXVECTOR3> &in, std::vector<D3DXVECTOR3> &out, LineEuqationType &line )
{
	int insize = in.size();
	int thisInd=insize-1;
	int nextInd=0;

	double thisRes = EvaLine( line, in[thisInd].x, in[thisInd].y );
	double nextRes;

	out.clear();

	for( nextInd=0; nextInd<insize; nextInd++ )
	{
		nextRes = EvaLine( line, in[nextInd].x, in[nextInd].y );

		if( thisRes >= 0 )
		{
			// Add the point
			out.push_back(in[thisInd]);
		}

		if( ( thisRes < 0 && nextRes >= 0 ) || ( thisRes >= 0 && nextRes < 0 ) )
		{
			// Add the split point
			out.push_back( Split(in[thisInd], in[nextInd], line ));
		}

		thisInd = nextInd;
		thisRes = nextRes;
	}
	if( (int)out.size() >= insize )
	{
		return true;
	}
	return false;
}
// Clipping using the Sutherland-Hodgeman algorithm
bool ClipFor1LineZ( std::vector<D3DXVECTOR3> &in, std::vector<D3DXVECTOR3> &out, bool nearplane )
{
	const float nearz = 1e-4f;
	const float farz = 1-1e-4f;

	int insize = in.size();
	int thisInd=insize-1;
	int nextInd=0;

	bool thisRes = nearplane ? (in[thisInd].z >= nearz) : (in[thisInd].z <= farz) ;
	bool nextRes;

	out.clear();

	for( nextInd=0; nextInd<insize; nextInd++ )
	{
		nextRes = nearplane ? (in[nextInd].z >= nearz) : (in[nextInd].z <= farz) ;

		if( thisRes )
		{
			// Add the point
			out.push_back(in[thisInd]);
		}

		if( ( !thisRes && nextRes ) || ( thisRes && !nextRes ) )
		{
			// Add the split point
			D3DXVECTOR3 newvtx;
			D3DXVECTOR3 &v1 = in[thisInd];
			D3DXVECTOR3 &v2 = in[nextInd];

			newvtx.z = nearplane ? nearz : farz;

			float r = (v1.z - newvtx.z )/(v1.z-v2.z);
			if( r != r )
			{
				r = (v1.z - newvtx.z )/(v1.z-v2.z);
			}
			newvtx.x = v1.x - r*(v1.x-v2.x);
			newvtx.y = v1.y - r*(v1.y-v2.y);
			out.push_back( newvtx );
		}

		thisInd = nextInd;
		thisRes = nextRes;
	}
	if( (int)out.size() >= insize )
	{
		return true;
	}
	return false;
}

void Create1LineEq(LineEuqationType &l, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3)
{
	// Line between (x1,y1) to (x2,y2)
	l.x = v2.y-v1.y;
	l.y = v1.x-v2.x;
	l.d = -(l.x*v2.x+(l.y)*v2.y);
	if( EvaLine(l,v3.x,v3.y)*v3.rhw<0 )
	{
		l.x = -l.x ;
		l.y = -l.y ;
		l.d = -l.d;
	}
}

void CreateLineEquations(float x1, float y1, float x2, float y2, float x3, float y3)
{
	// Line between (x1,y1) to (x2,y2)
	lines[0].x = y2-y1;
	lines[0].y = x1-x2;
	lines[0].d = -((y2-y1)*x2+(x1-x2)*y2);
	if( EvaLine(lines[0],x3,y3)<0 )
	{
		lines[0].x = -lines[0].x ;
		lines[0].y = -lines[0].y ;
	}

	// Line between (x1,y1) to (x3,y3)
	lines[1].x = y3-y1;
	lines[1].y = x1-x3;
	lines[1].d = -((y3-y1)*x3+(x1-x3)*y3);
	if( EvaLine(lines[1],x2,y2)<0 )
	{
		lines[1].x = -lines[1].x ;
		lines[1].y = -lines[1].y ;
	}

	// Line between (x2,y2) to (x3,y3)
	lines[2].x = y3-y2;
	lines[2].y = x2-x3;
	lines[2].d = -((y3-y2)*x3+(x2-x3)*y3);
	if( EvaLine(lines[2],x1,y1)<0 )
	{
		lines[2].x = -lines[2].x ;
		lines[2].y = -lines[2].y ;
	}
}

float inline interp3p(float a, float b, float c, double r1, double r2)
{
	return (float)((a)+(((b)+((c)-(b))*(r2))-(a))*(r1));
}

void Interp1PtZ(D3DXVECTOR3 &v, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3)
{
	if( abs(v.x-v1.x)<1 && abs(v.y-v1.y) < 1 )
	{
		v.z = v1.z;
		return;
	}
	if( abs(v.x-v2.x)<1 && abs(v.y-v2.y) < 1 )
	{
		v.z = v2.z;
		return;
	}
	if( abs(v.x-v3.x)<1 && abs(v.y-v3.y) < 1 )
	{
		v.z = v3.z;
		return;
	}

	LineEuqationType line;
	Create1LineEq(line, v2, v3, v1);

	D3DXVECTOR3 tempv1(v1.x, v1.y, 0);
	double aDot = (v.x*line.x + v.y*line.y);
	double bDot = (v1.x*line.x + v1.y*line.y);

	double scale1 = ( - line.d - aDot) / ( bDot - aDot );

	D3DXVECTOR3 tempv;
	tempv = v + ((float)scale1 * (tempv1 - v));

	double s1 = (v.x-v1.x)/(tempv.x-v1.x);
	if( !_finite(s1) )
	{
		s1 = (v.y-v1.y)/(tempv.y-v1.y);
	}
	double s2 = (tempv.x-v2.x)/(v3.x-v2.x);
	if( !_finite(s2) )
	{
		s2 = (tempv.y-v2.y)/(v3.y-v2.y);
	}

	v.z = interp3p(v1.z,v2.z,v3.z,s1,s2);

}

bool Interp1Pt(D3DXVECTOR3 &v, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, TLITVERTEX &out)
{
	LineEuqationType line;
	Create1LineEq(line, v2, v3, v1);

	D3DXVECTOR3 tempv1(v1.x, v1.y, 0);

	double aDot = (v.x*line.x + v.y*line.y);
	double bDot = (v1.x*line.x + v1.y*line.y);

	double scale1 = ( - line.d - aDot) / ( bDot - aDot );

	D3DXVECTOR3 tempv;
	tempv = v + ((float)scale1 * (tempv1 - v));

	double s1 = (v.x-v1.x)/(tempv.x-v1.x);
	if( !_finite(s1) )
	{
		s1 = (v.y-v1.y)/(tempv.y-v1.y);
	}
	double s2 = (tempv.x-v2.x)/(v3.x-v2.x);

	if( !_finite(s2) )
	{
		s2 = (tempv.y-v2.y)/(v3.y-v2.y);
	}

	if( !_finite(s1) || !_finite(s2) )
	{
		memcpy(&out, &v3, sizeof(TLITVERTEX) );
		return false;
	}

	//out.x = interp3p(v1.x,v2.x,v3.x,s1,s2);
	out.x = v.x;
	out.y = v.y;
	//out.y = interp3p(v1.y,v2.y,v3.y,s1,s2);
	out.z = interp3p(v1.z,v2.z,v3.z,s1,s2);
	out.rhw = interp3p(v1.rhw,v2.rhw,v3.rhw,s1,s2);

	out.dcSpecular = v2.dcSpecular; //fix me here
	if( gRSP.bFogEnabled )
	{
		float f1 = (v1.dcSpecular>>24)*v1.rhw;
		float f2 = (v2.dcSpecular>>24)*v2.rhw;
		float f3 = (v3.dcSpecular>>24)*v3.rhw;
		float f = interp3p(f1,f2,f3,s1,s2)/out.rhw;	
		if( f < 0 )	f = 0;
		if( f > 255 ) f = 255;
		uint32 fb = (uint8)f;
		out.dcSpecular &= 0x00FFFFFF;
		out.dcSpecular |= (fb<<24);
	}

	float r = interp3p(v1.r*v1.rhw,v2.r*v2.rhw,v3.r*v3.rhw,s1,s2)/out.rhw;	
	if( r<0 )	r=0; 
	if( r>255 )	r=255;
	out.r = (uint8)r;
	float g = interp3p(v1.g*v1.rhw,v2.g*v2.rhw,v3.g*v3.rhw,s1,s2)/out.rhw;
	if( g<0 )	g=0; 
	if( g>255 )	g=255;
	out.g = (uint8)g;
	float b = interp3p(v1.b*v1.rhw,v2.b*v2.rhw,v3.b*v3.rhw,s1,s2)/out.rhw;
	if( b<0 )	b=0; 
	if( b>255 )	b=255;
	out.b = (uint8)b;
	float a = interp3p(v1.a*v1.rhw,v2.a*v2.rhw,v3.a*v3.rhw,s1,s2)/out.rhw;
	if( a<0 )	a=0; 
	if( a>255 )	a=255;
	out.a = (uint8)a;

	for( int i=0; i<2; i++ )
	{
		out.tcord[i].u = interp3p(v1.tcord[i].u*v1.rhw,v2.tcord[i].u*v2.rhw,v3.tcord[i].u*v3.rhw,s1,s2)/out.rhw;
		out.tcord[i].v = interp3p(v1.tcord[i].v*v1.rhw,v2.tcord[i].v*v2.rhw,v3.tcord[i].v*v3.rhw,s1,s2)/out.rhw;
	}

	if( out.rhw < 0 )	
	{
		return false;
	}
	else
		return true;
}

bool Clip1TriangleForNegW(TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, int &dstidx)
{
	int lno = 0;
	if( v1.rhw >= 0 || v2.rhw >= 0 )	Create1LineEq(lines[lno++],v1,v2,v3);
	if( abs(EvaLine(lines[0],v3.x, v3.y) ) < 1 )	// v1, v2, v3 are linear
	{
		return false;
	}

	if( v1.rhw >= 0 || v3.rhw >= 0 )	Create1LineEq(lines[lno++],v1,v3,v2);
	if( v2.rhw >= 0 || v3.rhw >= 0 )	Create1LineEq(lines[lno++],v2,v3,v1);

	std::vector<D3DXVECTOR3> pts[2];

	D3DXVECTOR3 p;
	p.x = 0;
	p.y = 0;
	pts[0].push_back(p);
	p.x = windowSetting.uDisplayWidth;
	p.y = 0;
	pts[0].push_back(p);
	p.x = windowSetting.uDisplayWidth;
	p.y = windowSetting.uDisplayHeight;
	pts[0].push_back(p);
	p.x = 0;
	p.y = windowSetting.uDisplayHeight;
	pts[0].push_back(p);


	for( int j=0; j<lno; j++ )
	{
		ClipFor1LineXY(pts[j%2], pts[(j+1)%2], lines[j]);
		if( pts[(j+1)%2].size() < 3 )
			return false;
	}

	std::vector<D3DXVECTOR3> &ps = pts[lno%2];
	int size = ps.size();

	if( gRDP.otherMode.z_cmp )
	{
		for( int k=0; k<size; k++ )
		{
			Interp1PtZ(ps[k],v1,v2,v3);
		}

	//	ClipFor1LineZ(pts[lno%2], pts[(lno+1)%2], true);	// Clip for near plane
	//	if( pts[(lno+1)%2].size() < 3 )
	//		return false;
	//	ClipFor1LineZ(pts[(lno+1)%2], pts[lno%2], false);	// clip for far plane
	//	if( pts[lno%2].size() < 3 )
	//		return false;

		size = ps.size();
	}

	// now compute z and rhw values
	int i;
	TLITVERTEX  *newvertexes = new TLITVERTEX[size];

	for( i=0; i<size; i++ )
	{
		D3DXVECTOR3 &p = ps[i];
		if( abs(p.x-v1.x)<1 && abs(p.y-v1.y)<1 )
		{
			memcpy(&newvertexes[i], &v1, sizeof(TLITVERTEX) );
			continue;
		}
		else if( abs(p.x-v2.x)<1 && abs(p.y-v2.y)<1 )
		{
			memcpy(&newvertexes[i], &v2, sizeof(TLITVERTEX) );
			continue;
		}
		if( abs(p.x-v3.x)<1 && abs(p.y-v3.y)<1 )
		{
			memcpy(&newvertexes[i], &v3, sizeof(TLITVERTEX) );
			continue;
		}

		TLITVERTEX newvtx;
        if( Interp1Pt(ps[i],v1,v2,v3,newvtx) )
		{
			memcpy(&newvertexes[i], &newvtx, sizeof(TLITVERTEX) );
		}
		else
		{
			// negative w
			memcpy(&newvertexes[i], &newvtx, sizeof(TLITVERTEX) );
			//return false;	
		}
	}

	for( i=0; i<size-2; i++ )
	{
		CopyVertexData(0,   newvertexes, dstidx++, g_clippedVtxBuffer);
		CopyVertexData(i+1, newvertexes, dstidx++, g_clippedVtxBuffer);
		CopyVertexData(i+2, newvertexes, dstidx++, g_clippedVtxBuffer);
	}

	delete [] newvertexes;

	return true;
}

bool Clip1TriangleForZ(TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, int &dstidx)
{
	std::vector<D3DXVECTOR3> pts[2];

	D3DXVECTOR3 p;
	p.x = v1.x;
	p.y = v1.y;
	p.z = v1.z;
	pts[0].push_back(p);
	p.x = v2.x;
	p.y = v2.y;
	p.z = v2.z;
	pts[0].push_back(p);
	p.x = v3.x;
	p.y = v3.y;
	p.z = v3.z;
	pts[0].push_back(p);


	ClipFor1LineZ(pts[0], pts[1], true);	// Clip for near plane
	if( pts[1].size() < 3 )
	return false;
	ClipFor1LineZ(pts[1], pts[0], false);	// clip for far plane
	if( pts[0].size() < 3 )
	return false;

	std::vector<D3DXVECTOR3> &ps = pts[0];
	int size = ps.size();

	// now compute x, y and rhw values
	int i;
	TLITVERTEX  *newvertexes = new TLITVERTEX[size];

	for( i=0; i<size; i++ )
	{
		D3DXVECTOR3 &p = ps[i];
		if( abs(p.x-v1.x)<1 && abs(p.y-v1.y)<1 )
		{
			memcpy(&newvertexes[i], &v1, sizeof(TLITVERTEX) );
			continue;
		}
		else if( abs(p.x-v2.x)<1 && abs(p.y-v2.y)<1 )
		{
			memcpy(&newvertexes[i], &v2, sizeof(TLITVERTEX) );
			continue;
		}
		if( abs(p.x-v3.x)<1 && abs(p.y-v3.y)<1 )
		{
			memcpy(&newvertexes[i], &v3, sizeof(TLITVERTEX) );
			continue;
		}

		TLITVERTEX newvtx;
		Interp1Pt(ps[i],v1,v2,v3,newvtx);
		memcpy(&newvertexes[i], &newvtx, sizeof(TLITVERTEX) );
	}

	for( i=0; i<size-2; i++ )
	{
		CopyVertexData(0,   newvertexes, dstidx++, g_clippedVtxBuffer);
		CopyVertexData(i+1, newvertexes, dstidx++, g_clippedVtxBuffer);
		CopyVertexData(i+2, newvertexes, dstidx++, g_clippedVtxBuffer);
	}

	delete [] newvertexes;

	return true;
}