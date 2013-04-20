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
typedef struct {
    double d;
	double x;
	double y;
} LineEuqationType;
LineEuqationType alllines[3];	// In the format: Nx, Ny, d
double EvaLine(LineEuqationType &li, double x, double y)
{
	return li.x*x+li.y*y+li.d;
}

struct PointInfo
{
	float x;
	float y;
	float z;
	int onVtx;
	int onLine;
};

PointInfo Split( PointInfo &a, PointInfo &b, TLITVERTEX &v1, TLITVERTEX &v2, int lineno )
{
	LineEuqationType l;
	l.x = b.y-a.y;
	l.y = a.x-b.x;
	l.d = -(l.x*b.x+(l.y)*b.y);
	double cDot = (v1.x*l.x + v1.y*l.y);
	double dDot = (v2.x*l.x + v2.y*l.y);
	double scale = ( - l.d - cDot) / ( dDot - cDot );

	D3DXVECTOR3 va(v1.x, v1.y, v1.z);
	D3DXVECTOR3 vb(v2.x, v2.y, v2.z);
	D3DXVECTOR3 vc = va + ((vb - va) * (float)scale );

	PointInfo d = {vc.x, vc.y, vc.z, -1, lineno};

	if( a.onLine == b.onLine && a.onLine >= 0 )
	{
		if( a.onLine == 0 && lineno == 1 )
			d.onVtx = 0;
		else if ( a.onLine == 0 && lineno == 2 )
			d.onVtx = 1;
		else if( a.onLine == 1 && lineno == 0 )
			d.onVtx = 0;
		else if( a.onLine == 1 && lineno == 2 )
			d.onVtx = 2;
		else if( a.onLine == 2 && lineno == 0 )
			d.onVtx = 1;
		else if( a.onLine == 2 && lineno == 1 )
			d.onVtx = 2;
	}

	return d;
}

// Clipping using the Sutherland-Hodgeman algorithm
bool ClipFor1LineXY( std::vector<PointInfo> &in, int lineno, TLITVERTEX &v1, TLITVERTEX &v2 )
{
	std::vector<PointInfo> out;

	LineEuqationType &line = alllines[lineno];

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
			out.push_back( Split(in[thisInd], in[nextInd], v1, v2, lineno ));
		}

		thisInd = nextInd;
		thisRes = nextRes;
	}

	in = out;	// Copy element from one vector to another vector

	return( (int)in.size() >= insize );
}

float interp2p(float a, float b, double r)
{
	// r = (zb-0)/(zb-za)
	return (float)((b)*(r)+(1-(r))*(a));
}

// To clip to the near clipping for N64, don't clip the vertex, but adding vertex at line interception at z = 0
// and set all z < 0 to z = 0
bool InsertPointsAtNearPlane(std::vector<PointInfo> &in)
{
	std::vector<PointInfo> out;
	int insize = in.size();
	out.clear();

	int i;
	int needclip = 0;
	for( i=0; i<insize; i++)
	{
		if( in[i].z < 0 )
		{
			needclip++;
		}
	}

	if( needclip == 0 || needclip == insize )
	{
		return false;	// No need to clip
	}

	int start_z_idx=0;
	for( start_z_idx=0; start_z_idx<insize; start_z_idx++)
	{
		PointInfo &vtx = in[start_z_idx];
		if( vtx.z > 0 )
			break;
	}

	out.push_back(in[start_z_idx]);
	for( i=0; i<insize; i++ )
	{
		int a = (start_z_idx+i)%insize;
		int b = (start_z_idx+i+1)%insize;

		if( in[a].z * in[b].z < 0 )
		{
			// these two points cross z=0
			PointInfo newvtx;
			float r = (0-in[a].z)/(in[b].z-in[a].z);
			newvtx.x = interp2p(in[a].x, in[b].x, r);
			newvtx.y = interp2p(in[a].y, in[b].y, r);
			newvtx.z = 0;

			out.push_back(newvtx);
			if( start_z_idx != b )	out.push_back(in[b]);
		}
		else
		{
			if( start_z_idx != b )	out.push_back(in[b]);
		}
	}

	in = out;
	return true;
}

void SplitPointsToTwoPolygons(std::vector<PointInfo> &points, std::vector<PointInfo> &polygon1, std::vector<PointInfo> &polygon2)
{
	// The points has vertex with z < 0, we need to split the array to two arrays
	polygon1.clear();
	polygon2.clear();

	int size = points.size();
	for( int i=0; i<size; i++ )
	{
		if( points[i].z <= 0 )
			polygon1.push_back(points[i]);
		if( points[i].z >= 0 )
			polygon2.push_back(points[i]);
	}
}


// Clipping using the Sutherland-Hodgeman algorithm
bool ClipFarPlane( std::vector<PointInfo> &in )
{
	std::vector<PointInfo> out;

	const float farz = 1-1e-4f;

	int insize = in.size();
	int thisInd=insize-1;
	int nextInd=0;

	bool thisRes = (in[thisInd].z <= farz) ;
	bool nextRes;

	out.clear();

	for( nextInd=0; nextInd<insize; nextInd++ )
	{
		nextRes = (in[nextInd].z <= farz) ;

		if( thisRes )
		{
			// Add the point
#ifdef _DEBUG
			PointInfo &vtx = in[thisInd];
			out.push_back(vtx);
#else
			out.push_back(in[thisInd]);
#endif
		}

		if( ( !thisRes && nextRes ) || ( thisRes && !nextRes ) )
		{
			// Add the split point
			PointInfo newvtx;
			PointInfo &v1 = in[thisInd];
			PointInfo &v2 = in[nextInd];

			newvtx.z = farz;

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

	in = out;
	return( (int)out.size() >= insize );
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

void CreateLineEquations(TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3)
{
	Create1LineEq(alllines[0],v1,v2,v3);
	Create1LineEq(alllines[1],v1,v3,v2);
	Create1LineEq(alllines[2],v2,v3,v1);
}

float interp3p(float a, float b, float c, double r1, double r2)
{
	return (float)((a)+(((b)+((c)-(b))*(r2))-(a))*(r1));
}

bool Interp1Point3pZ(PointInfo &v, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3)
{
	bool res = true;

	LineEuqationType line;
	Create1LineEq(line, v2, v3, v1);

	D3DXVECTOR3 tempv1(v1.x, v1.y, 0);

	double aDot = (v.x*line.x + v.y*line.y);
	double bDot = (v1.x*line.x + v1.y*line.y);

	double scale1 = ( - line.d - aDot) / ( bDot - aDot );

	D3DXVECTOR3 tempv;
	D3DXVECTOR3 va(v.x, v.y, v.z);
	tempv = va + ((float)scale1 * (tempv1 - va));

	double s1 = (v.x-v1.x)/(tempv.x-v1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-v1.y)/(tempv.y-v1.y);
		if( !_finite(s1) )	return false;
	}

	double s2 = (tempv.x-v2.x)/(v3.x-v2.x);
	if( !_finite(s2) || abs(s2) > 100 )
	{
		s2 = (tempv.y-v2.y)/(v3.y-v2.y);
		if( !_finite(s2) )	return false;
	}

	if( !_finite(s1) || !_finite(s2) )
	{
		return false;
	}

	v.z = interp3p(v1.z,v2.z,v3.z,s1,s2);

	return true;
}

bool Interp1Point3p(PointInfo &v, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, TLITVERTEX &out)
{
	bool res = true;

	LineEuqationType line;
	Create1LineEq(line, v2, v3, v1);

	D3DXVECTOR3 tempv1(v1.x, v1.y, 0);

	double aDot = (v.x*line.x + v.y*line.y);
	double bDot = (v1.x*line.x + v1.y*line.y);

	double scale1 = ( - line.d - aDot) / ( bDot - aDot );

	D3DXVECTOR3 tempv;
	D3DXVECTOR3 va(v.x, v.y, v.z);
	tempv = va + ((float)scale1 * (tempv1 - va));

	double s1 = (v.x-v1.x)/(tempv.x-v1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-v1.y)/(tempv.y-v1.y);
		if( !_finite(s1) )	return false;
	}

	double s2 = (tempv.x-v2.x)/(v3.x-v2.x);
	if( !_finite(s2) || abs(s2) > 100 )
	{
		s2 = (tempv.y-v2.y)/(v3.y-v2.y);
		if( !_finite(s2) )	return false;
	}

	if( !_finite(s1) || !_finite(s2) )
	{
		memcpy(&out, &v3, sizeof(TLITVERTEX) );
		RDP_NOIMPL_WARN("s1 or s2 is still infinite, it is a coplaner triangle");
		return false;
	}

	out.x = v.x;
	out.y = v.y;
	out.z = v.z;
	//out.z = interp3p(v1.z,v2.z,v3.z,s1,s2);
	out.rhw = interp3p(v1.rhw,v2.rhw,v3.rhw,s1,s2);

	if( !_finite(out.z) || !_finite(out.rhw) || out.rhw < 0 )
	{
#ifdef _DEBUG
		if( out.rhw < 0 )	
			DebuggerAppendMsg("Warning: rhw<0");
		else
			RDP_NOIMPL_WARN("Z or RHW value is #INF, check me as below");
		DebuggerAppendMsg("v1=[%f %f %f %f];\n",v1.x, v1.y, v1.z, v1.rhw);
		DebuggerAppendMsg("v2=[%f %f %f %f];\n",v2.x, v2.y, v2.z, v2.rhw);
		DebuggerAppendMsg("v3=[%f %f %f %f];\n",v3.x, v3.y, v3.z, v3.rhw);
		DebuggerAppendMsg("v=[%f %f];\n",v.x, v.y);
		DebuggerAppendMsg("v-out=[z=%f rhw=%f];\n",out.z, out.rhw);
		DebuggerAppendMsg("tempv=[%f %f %f];\n",tempv.x, tempv.y, tempv.z);
		DebuggerAppendMsg("s1=%f, s2=%f;\n",s1,s2);
#endif
		return false;
	}


	if( out.z > 1 )	
		RDP_NOIMPL_WARN("Warning: z>1");

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

bool Interp1Point2pZ(PointInfo &v, TLITVERTEX &v1, TLITVERTEX &v2)
{
	double s1 = (v.x-v1.x)/(v2.x-v1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-v1.y)/(v2.y-v1.y);
		if( !_finite(s1) )	return false;
	}

	v.z = interp2p(v1.z,v2.z,s1);
	return true;
}

bool Interp1Point2p(PointInfo &v, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &out)
{
	double s1 = (v.x-v1.x)/(v2.x-v1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-v1.y)/(v2.y-v1.y);
		if( !_finite(s1) )	return false;
	}

	out.x = v.x;
	out.y = v.y;
	out.z = v.z;
	//out.z = interp2p(v1.z,v2.z,s1);
	out.rhw = interp2p(v1.rhw,v2.rhw,s1);

	if( !_finite(out.z) || !_finite(out.rhw) || out.rhw < 0 )
	{
#ifdef _DEBUG
		if( out.rhw < 0 )	
			DebuggerAppendMsg("Warning: rhw<0");
		else
			RDP_NOIMPL_WARN("Z or RHW value is #INF, check me as below");
		DebuggerAppendMsg("v1=[%f %f %f %f];\n",v1.x, v1.y, v1.z, v1.rhw);
		DebuggerAppendMsg("v2=[%f %f %f %f];\n",v2.x, v2.y, v2.z, v2.rhw);
		DebuggerAppendMsg("v=[%f %f];\n",v.x, v.y);
		DebuggerAppendMsg("v-out=[z=%f rhw=%f];\n",out.z, out.rhw);
		DebuggerAppendMsg("s1=%f;\n",s1);
#endif
		return false;
	}

	if( out.z > 1 )	
		RDP_NOIMPL_WARN("Warning: z>1");

	out.dcSpecular = v2.dcSpecular; //fix me here
	if( gRSP.bFogEnabled )
	{
		float f1 = (v1.dcSpecular>>24)*v1.rhw;
		float f2 = (v2.dcSpecular>>24)*v2.rhw;
		float f = interp2p(f1,f2,s1)/out.rhw;	
		if( f < 0 )	f = 0;
		if( f > 255 ) f = 255;
		uint32 fb = (uint8)f;
		out.dcSpecular &= 0x00FFFFFF;
		out.dcSpecular |= (fb<<24);
	}

	float r = interp2p(v1.r*v1.rhw,v2.r*v2.rhw,s1)/out.rhw;	
	if( r<0 )	r=0; 
	if( r>255 )	r=255;
	out.r = (uint8)r;
	float g = interp2p(v1.g*v1.rhw,v2.g*v2.rhw,s1)/out.rhw;
	if( g<0 )	g=0; 
	if( g>255 )	g=255;
	out.g = (uint8)g;
	float b = interp2p(v1.b*v1.rhw,v2.b*v2.rhw,s1)/out.rhw;
	if( b<0 )	b=0; 
	if( b>255 )	b=255;
	out.b = (uint8)b;
	float a = interp2p(v1.a*v1.rhw,v2.a*v2.rhw,s1)/out.rhw;
	if( a<0 )	a=0; 
	if( a>255 )	a=255;
	out.a = (uint8)a;

	for( int i=0; i<2; i++ )
	{
		out.tcord[i].u = interp2p(v1.tcord[i].u*v1.rhw,v2.tcord[i].u*v2.rhw,s1)/out.rhw;
		out.tcord[i].v = interp2p(v1.tcord[i].v*v1.rhw,v2.tcord[i].v*v2.rhw,s1)/out.rhw;
	}

	if( out.rhw < 0 )	
	{
		return false;
	}
	else
		return true;
}

bool ClipXYBy1Triangle( std::vector<PointInfo> &points, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3 )
{
	if( abs(EvaLine(alllines[0],v3.x, v3.y) ) < 1 )	// v1, v2, v3 are linear
	{
		points.clear();
		return false;
	}

	if( v1.rhw >= 0 || v2.rhw >= 0 )	ClipFor1LineXY(points, 0, v1, v2);
	if( points.size() < 3 )	return false;
	if( v1.rhw >= 0 || v3.rhw >= 0 )	ClipFor1LineXY(points, 1, v1, v3);
	if( points.size() < 3 )	return false;
	if( v2.rhw >= 0 || v3.rhw >= 0 )	ClipFor1LineXY(points, 2, v2, v3);
	if( points.size() < 3 )	return false;

	return true;
}

void InitScreenPoints(std::vector<PointInfo> &points)
{
	points.clear();
	PointInfo p;
	p.x = (float)windowSetting.clipping.left-2;
	p.y = (float)windowSetting.clipping.top-2;
	p.onLine = p.onVtx = -1;
	points.push_back(p);
	p.x = (float)windowSetting.clipping.right+2;
	p.y = (float)windowSetting.clipping.top-2;
	points.push_back(p);
	p.x = (float)windowSetting.clipping.right+2;
	p.y = (float)windowSetting.clipping.bottom+2;
	points.push_back(p);
	p.x = (float)windowSetting.clipping.left-2;
	p.y = (float)windowSetting.clipping.bottom+2;
	points.push_back(p);

}

void InterpolatePoints(std::vector<PointInfo> &points, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3, int &dstidx)
{
	// now compute z and rhw values
	int i;
	int size = points.size();
	TLITVERTEX  *newvertexes = new TLITVERTEX[size];

	for( i=0; i<size; i++ )
	{
		PointInfo &p = points[i];
		TLITVERTEX newvtx;

		if( p.onVtx == 0 )
		{
			memcpy(&newvertexes[i], &v1, sizeof(TLITVERTEX) );
			continue;
		}
		else if( p.onVtx == 1 )
		{
			memcpy(&newvertexes[i], &v2, sizeof(TLITVERTEX) );
			continue;
		}
		else if( p.onVtx == 2 )
		{
			memcpy(&newvertexes[i], &v3, sizeof(TLITVERTEX) );
			continue;
		}
		else if( p.onLine == 0 )
		{
			if( !Interp1Point2p(p,v1,v2,newvtx) )
			{
				delete [] newvertexes;
				return; 
			}
		}
		else if( p.onLine == 1 )
		{
			if( !Interp1Point2p(p,v1,v3,newvtx) )
			{
				delete [] newvertexes;
				return; 
			}
		}
		else if( p.onLine == 2 )
		{
			if( !Interp1Point2p(p,v2,v3,newvtx) )
			{
				delete [] newvertexes;
				return; 
			}
		}
		//else if( abs(p.x-v1.x)<1 && abs(p.y-v1.y)<1 )
		//{
		//	memcpy(&newvertexes[i], &v1, sizeof(TLITVERTEX) );
		//	continue;
		//}
		//else if( abs(p.x-v2.x)<1 && abs(p.y-v2.y)<1 )
		//{
		//	memcpy(&newvertexes[i], &v2, sizeof(TLITVERTEX) );
		//	continue;
		//}
		//else if( abs(p.x-v3.x)<1 && abs(p.y-v3.y)<1 )
		//{
		//	memcpy(&newvertexes[i], &v3, sizeof(TLITVERTEX) );
		//	continue;
		//}
		//else if( abs(EvaLine(alllines[0],p.x, p.y) ) < 1 )	// v1, v2, p are linear
		//{
		//	if( !Interp1Point2p(p,v1,v2,newvtx) )
		//	{
		//		delete [] newvertexes;
		//		return; 
		//	}
		//}
		//else if( abs(EvaLine(alllines[1],p.x, p.y) ) < 1 )	// v1, v3, p are linear
		//{
		//	if( !Interp1Point2p(p,v1,v3,newvtx) )
		//	{
		//		delete [] newvertexes;
		//		return; 
		//	}
		//}
		//else if( abs(EvaLine(alllines[2],p.x, p.y) ) < 1 )	// v2, v3, p are linear
		//{
		//	if( !Interp1Point2p(p,v2,v3,newvtx) )
		//	{
		//		delete [] newvertexes;
		//		return; 
		//	}
		//}
		else if( !Interp1Point3p(p,v1,v2,v3,newvtx) )
		{
			delete [] newvertexes;
			return; 
		}

		memcpy(&newvertexes[i], &newvtx, sizeof(TLITVERTEX) );
	}

	TRI_DUMP( 
	{
		TRACE0("Generated new vertexes after clipping:");
		for( i=0; i<size; i++ )
		{
			DebuggerAppendMsg("Vtx %d: %f, %f, %f, %f", i, newvertexes[i].x, newvertexes[i].y, newvertexes[i].z, 1/newvertexes[i].rhw);
		}
	});

	for( i=0; i<size-2; i++ )
	{
		CopyVertexData(0,   newvertexes, dstidx++, g_clippedVtxBuffer);
		CopyVertexData(i+1, newvertexes, dstidx++, g_clippedVtxBuffer);
		CopyVertexData(i+2, newvertexes, dstidx++, g_clippedVtxBuffer);
	}

	delete [] newvertexes;
}

void InterpolatePointsZ(std::vector<PointInfo> &points, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3)
{
	// now compute z values
	int i;
	int size = points.size();

	for( i=0; i<size; i++ )
	{
		PointInfo &p = points[i];

		//if( abs(p.x-v1.x)<1 && abs(p.y-v1.y)<1 )
		//{
		//	p.z = v1.z;
		//}
		//else if( abs(p.x-v2.x)<1 && abs(p.y-v2.y)<1 )
		//{
		//	p.z = v2.z;
		//}
		//else if( abs(p.x-v3.x)<1 && abs(p.y-v3.y)<1 )
		//{
		//	p.z = v3.z;
		//}
		//else if( abs(EvaLine(alllines[0],p.x, p.y) ) < 1 )	// v1, v2, p are linear
		//{
		//	Interp1Point2pZ(p,v1,v2);
		//}
		//else if( abs(EvaLine(alllines[1],p.x, p.y) ) < 1 )	// v1, v3, p are linear
		//{
		//	Interp1Point2pZ(p,v1,v3);
		//}
		//else if( abs(EvaLine(alllines[2],p.x, p.y) ) < 1 )	// v2, v3, p are linear
		//{
		//	Interp1Point2pZ(p,v2,v3);
		//}
		//else
		{
			Interp1Point3pZ(p,v1,v2,v3);
		}
	}
}

void InitTrianglePoints( std::vector<PointInfo> &points, TLITVERTEX &v1, TLITVERTEX &v2, TLITVERTEX &v3 )
{
	points.clear();

	PointInfo p;
	p.x = v1.x;
	p.y = v1.y;
	p.z = v1.z;
	p.onVtx = 0;
	p.onLine = -1;
	points.push_back(p);
	p.x = v2.x;
	p.y = v2.y;
	p.z = v2.z;
	p.onVtx = 1;
	points.push_back(p);
	p.x = v3.x;
	p.y = v3.y;
	p.z = v3.z;
	p.onVtx = 2;
	points.push_back(p);
}

bool OffScreen(TLITVERTEX &v)
{
	if( !windowSetting.clipping.needToClip ) return false;

	return		v.x	<	windowSetting.clipping.left
		||	v.x >	windowSetting.clipping.right
		||	v.y	<	windowSetting.clipping.top
		||	v.y	>	windowSetting.clipping.bottom;
}

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
		int size;
		TLITVERTEX &v1 = g_vtxBuffer[firstidx];
		TLITVERTEX &v2 = g_vtxBuffer[firstidx+1];
		TLITVERTEX &v3 = g_vtxBuffer[firstidx+2];

		if( v1.rhw < 0 && v2.rhw < 0 && v3.rhw < 0 )
			continue;	// Skip this triangle

		std::vector<PointInfo> points;

		bool vtxoffscreen = false;
		bool vtxOffFar = false;
		bool vtxOffNear = false;

		TRI_DUMP( 
		{
			TRACE1("Clip triangle %d for negative w", i);
			TRACE4("V1: x=%f, y=%f, z=%f, rhw=%f", v1.x, v1.y, v1.z, v1.rhw);
			TRACE4("V2: x=%f, y=%f, z=%f, rhw=%f", v2.x, v2.y, v2.z, v2.rhw);
			TRACE4("V3: x=%f, y=%f, z=%f, rhw=%f", v3.x, v3.y, v3.z, v3.rhw);
		});

		SwapVertexPos(firstidx);
		CreateLineEquations(v1,v2,v3);
		InitScreenPoints(points);
		InterpolatePointsZ(points,v1,v2,v3);
		ClipXYBy1Triangle(points, v1, v2, v3);

		TRI_DUMP( 
		{
			TRACE1("Got %d vertex after clipping", points.size());
			for( unsigned int k=0; k<points.size(); k++)
			{
				TRACE4("V(%d): x=%f, y=%f, z=%f", k, points[k].x, points[k].y, points[k].z);
			}
		});

		/*if( v1.rhw >= 0 && v2.rhw >= 0 && v3.rhw >= 0 )	
		{
			if( options.bForceSoftwareTnL && (windowSetting.clipping.left > 0 || windowSetting.clipping.top > 0) )
			{
				vtxoffscreen = OffScreen(v1) || OffScreen(v2) || OffScreen(v3);
			}
			//vtxoffscreen = false;	// Disable clipping for large triangles

			if( !vtxoffscreen && gRDP.otherMode.z_cmp )
			{
				vtxOffFar = v1.z > 1 || v2.z > 1 || v3.z > 1;
				vtxOffNear = v1.z < 0 || v2.z < 0 || v3.z < 0;
			}

			//vtxOffFar = false;	// Disable Far plane clipping, don't really to do so

			if( vtxoffscreen )
			{
				TRI_DUMP( 
				{
					TRACE1("Clip large triangle %d", i);
					TRACE3("V1: x=%f, y=%f, z=%f", v1.x, v1.y, v1.z);
					TRACE3("V2: x=%f, y=%f, z=%f", v2.x, v2.y, v2.z);
					TRACE3("V3: x=%f, y=%f, z=%f", v3.x, v3.y, v3.z);
				});

				CreateLineEquations(v1,v2,v3);
				InitScreenPoints(points);
				InterpolatePointsZ(points,v1,v2,v3);
				ClipXYBy1Triangle(points, v1, v2, v3);

				TRI_DUMP( 
				{
					TRACE1("Got %d vertex after clipping", points.size());
					for( unsigned int k=0; k<points.size(); k++)
					{
						TRACE4("V(%d): x=%f, y=%f, z=%f", k, points[k].x, points[k].y, points[k].z);
					}
				});
			}
			else if( vtxOffNear )
			{
				CreateLineEquations(v1,v2,v3);
				InitTrianglePoints(points,v1,v2,v3);
			}
			else if( vtxOffFar )
			{
				CreateLineEquations(v1,v2,v3);
				InitTrianglePoints(points,v1,v2,v3);
			}
			else
			{
				CopyVertexData(firstidx, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
				CopyVertexData(firstidx+1, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
				CopyVertexData(firstidx+2, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);

				TRI_DUMP( 
				{
					TRACE1("Good triangle %d, not clipped", i);
					TRACE3("V1: x=%f, y=%f, z=%f", v1.x, v1.y, v1.z);
					TRACE3("V2: x=%f, y=%f, z=%f", v2.x, v2.y, v2.z);
					TRACE3("V3: x=%f, y=%f, z=%f", v3.x, v3.y, v3.z);
				});
				continue;
			}

		}
		else
		{
			TRI_DUMP( 
			{
				TRACE1("Clip triangle %d for negative w", i);
				TRACE4("V1: x=%f, y=%f, z=%f, rhw=%f", v1.x, v1.y, v1.z, v1.rhw);
				TRACE4("V2: x=%f, y=%f, z=%f, rhw=%f", v2.x, v2.y, v2.z, v2.rhw);
				TRACE4("V3: x=%f, y=%f, z=%f, rhw=%f", v3.x, v3.y, v3.z, v3.rhw);
			});

			SwapVertexPos(firstidx);
			CreateLineEquations(v1,v2,v3);
			InitScreenPoints(points);
			InterpolatePointsZ(points,v1,v2,v3);
			ClipXYBy1Triangle(points, v1, v2, v3);

			TRI_DUMP( 
			{
				TRACE1("Got %d vertex after clipping", points.size());
				for( unsigned int k=0; k<points.size(); k++)
				{
					TRACE4("V(%d): x=%f, y=%f, z=%f", k, points[k].x, points[k].y, points[k].z);
				}
			});
		}*/

		size = points.size();
		if( size < 3 ) continue;

		if( gRDP.otherMode.z_cmp )
		{
			if( vtxOffFar )
			{
				ClipFarPlane(points);	// Clip for far plane
				if( points.size() < 3 )	continue;
			}

			if( InsertPointsAtNearPlane(points) )
			{
				std::vector<PointInfo> polygon1;
				std::vector<PointInfo> polygon2;

				SplitPointsToTwoPolygons(points,polygon1,polygon2);

				InterpolatePoints(polygon1,v1,v2,v3,dstidx);
				InterpolatePoints(polygon2,v1,v2,v3,dstidx);
				polygon1.clear();
				polygon2.clear();
			}
			else
			{
				InterpolatePoints(points,v1,v2,v3,dstidx);
			}
		}
		else
		{
			InterpolatePoints(points,v1,v2,v3,dstidx);
		}

		points.clear();
	}

	g_clippedVtxCount = dstidx;
	for( int m=0; m<dstidx; m++ )
	{
		if( g_clippedVtxBuffer[m].z < 0 ) g_clippedVtxBuffer[m].z = 0;
	}
}

void ClipVertexesForRect()
{
	if( options.bForceSoftwareTnL && (windowSetting.clipping.left > 0 || windowSetting.clipping.top > 0) )
	{
		bool offleft = g_texRectTVtx[0].x < gRDP.scissor.left*windowSetting.fMultX && gRDP.scissor.left > 0;
		bool offright = g_texRectTVtx[3].x > gRDP.scissor.right*windowSetting.fMultX && g_texRectTVtx[3].x < windowSetting.uWindowDisplayWidth;
		bool offtop = g_texRectTVtx[0].y < gRDP.scissor.top*windowSetting.fMultY && gRDP.scissor.top > 0 ;
		bool offbottom = g_texRectTVtx[3].y > gRDP.scissor.bottom*windowSetting.fMultY && g_texRectTVtx[3].y < windowSetting.uWindowDisplayWidth;

		if( offleft || offright || offtop || offbottom )
		{
			TLITVERTEX newv;// = g_texRectTVtx;
			PointInfo v;

			if( offleft || offtop )
			{
				// Clipping for v0
				v.x = offleft ? gRDP.scissor.left*windowSetting.fMultX : g_texRectTVtx[0].x;
				v.y = offtop ? gRDP.scissor.top*windowSetting.fMultY : g_texRectTVtx[0].y;
				v.z = g_texRectTVtx[0].z;
				Interp1Point3p(v, g_texRectTVtx[0], g_texRectTVtx[1], g_texRectTVtx[2], newv);
				memcpy(&g_texRectTVtx[0], &newv, sizeof(TLITVERTEX));
			}

			if( offright || offtop )
			{
				// Clipping for v1
				v.x = offright ? gRDP.scissor.right*windowSetting.fMultX : g_texRectTVtx[1].x;
				v.y = offtop ? gRDP.scissor.top*windowSetting.fMultY : g_texRectTVtx[1].y;
				v.z = g_texRectTVtx[1].z;
				Interp1Point3p(v, g_texRectTVtx[0], g_texRectTVtx[1], g_texRectTVtx[2], newv);
				memcpy(&g_texRectTVtx[1], &newv, sizeof(TLITVERTEX));
			}

			if( offleft || offbottom )
			{
				// Clipping for v2
				v.x = offleft ? gRDP.scissor.left*windowSetting.fMultX : g_texRectTVtx[2].x;
				v.y = offbottom ? gRDP.scissor.bottom*windowSetting.fMultY : g_texRectTVtx[2].y;
				v.z = g_texRectTVtx[1].z;
				Interp1Point3p(v, g_texRectTVtx[0], g_texRectTVtx[1], g_texRectTVtx[2], newv);
				memcpy(&g_texRectTVtx[1], &newv, sizeof(TLITVERTEX));
			}

			if( offright || offright )
			{
				// Clipping for v1
				v.x = offright ? gRDP.scissor.right*windowSetting.fMultX : g_texRectTVtx[3].x;
				v.y = offbottom ? gRDP.scissor.bottom*windowSetting.fMultY : g_texRectTVtx[3].y;
				v.z = g_texRectTVtx[3].z;
				Interp1Point3p(v, g_texRectTVtx[0], g_texRectTVtx[1], g_texRectTVtx[2], newv);
				memcpy(&g_texRectTVtx[3], &newv, sizeof(TLITVERTEX));
			}
		}
	}
}