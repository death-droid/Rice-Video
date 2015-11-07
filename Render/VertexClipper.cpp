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

#include "..\stdafx.h"
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
void SetVtx(TLITVERTEX &v, TLITVERTEX &vec1, TLITVERTEX &vec2, float r)
{
	v.x = interp(vec1.x,vec2.x,r);
	v.y = interp(vec1.y,vec2.y,r);
	//v.z = (vec1.z-r*vec2.z)/(1-r);
	v.dcSpecular = vec2.dcSpecular; //fix me here
	v.r = (uint8)(interp((int)vec1.r,(int)vec2.r,r));
	v.g = (uint8)(interp((int)vec1.g,(int)vec2.g,r));
	v.b = (uint8)(interp((int)vec1.b,(int)vec2.b,r));
	v.a = (uint8)(interp((int)vec1.a,(int)vec2.a,r));

	for( int i=0; i<2; i++ )
	{
		v.tcord[i].u = interp(vec1.tcord[i].u,vec2.tcord[i].u,r);
		v.tcord[i].v = interp(vec1.tcord[i].v,vec2.tcord[i].v,r);
	}
}

void SwapVertexPos(int firstidx)
{
	TLITVERTEX &vec1 = g_vtxBuffer[firstidx];
	TLITVERTEX &vec2 = g_vtxBuffer[firstidx+1];
	TLITVERTEX &vec3 = g_vtxBuffer[firstidx+2];

	if( vec1.rhw >= vec2.rhw && vec1.rhw >= vec3.rhw ) return;

	TLITVERTEX tempv;
	memcpy(&tempv,&g_vtxBuffer[firstidx], sizeof(TLITVERTEX));

	if( vec2.rhw > vec1.rhw && vec2.rhw >= vec3.rhw )
	{
		// vec2 is the largest one
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

PointInfo Split( PointInfo &a, PointInfo &b, TLITVERTEX &vec1, TLITVERTEX &vec2, int lineno )
{
	LineEuqationType l;
	l.x = b.y-a.y;
	l.y = a.x-b.x;
	l.d = -(l.x*b.x+(l.y)*b.y);
	double cDot = (vec1.x*l.x + vec1.y*l.y);
	double dDot = (vec2.x*l.x + vec2.y*l.y);
	double scale = ( - l.d - cDot) / ( dDot - cDot );

	v3 va(vec1.x, vec1.y, vec1.z);
	v3 vb(vec2.x, vec2.y, vec2.z);
	v3 vc = va + ((vb - va) * (float)scale );

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
bool ClipFor1LineXY( std::vector<PointInfo> &in, int lineno, TLITVERTEX &vec1, TLITVERTEX &vec2 )
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
			out.push_back( Split(in[thisInd], in[nextInd], vec1, vec2, lineno ));
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

void Create1LineEq(LineEuqationType &l, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3)
{
	// Line between (x1,y1) to (x2,y2)
	l.x = vec2.y-vec1.y;
	l.y = vec1.x-vec2.x;
	l.d = -(l.x*vec2.x+(l.y)*vec2.y);
	if( EvaLine(l,vec3.x,vec3.y)*vec3.rhw<0 )
	{
		l.x = -l.x ;
		l.y = -l.y ;
		l.d = -l.d;
	}
}

void CreateLineEquations(TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3)
{
	Create1LineEq(alllines[0],vec1,vec2,vec3);
	Create1LineEq(alllines[1],vec1,vec3,vec2);
	Create1LineEq(alllines[2],vec2,vec3,vec1);
}

float interp3p(float a, float b, float c, double r1, double r2)
{
	return (float)((a)+(((b)+((c)-(b))*(r2))-(a))*(r1));
}

bool Interp1Point3pZ(PointInfo &v, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3)
{
	bool res = true;

	LineEuqationType line;
	Create1LineEq(line, vec2, vec3, vec1);

	v3 tempvec1(vec1.x, vec1.y, 0);

	double aDot = (v.x*line.x + v.y*line.y);
	double bDot = (vec1.x*line.x + vec1.y*line.y);

	double scale1 = ( - line.d - aDot) / ( bDot - aDot );

	v3 tempv;
	v3 va(v.x, v.y, v.z);
	tempv = va + ((float)scale1 * (tempvec1 - va));

	double s1 = (v.x-vec1.x)/(tempv.x-vec1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-vec1.y)/(tempv.y-vec1.y);
		if( !_finite(s1) )	return false;
	}

	double s2 = (tempv.x-vec2.x)/(vec3.x-vec2.x);
	if( !_finite(s2) || abs(s2) > 100 )
	{
		s2 = (tempv.y-vec2.y)/(vec3.y-vec2.y);
		if( !_finite(s2) )	return false;
	}

	if( !_finite(s1) || !_finite(s2) )
	{
		return false;
	}

	v.z = interp3p(vec1.z,vec2.z,vec3.z,s1,s2);

	return true;
}

bool Interp1Point3p(PointInfo &v, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3, TLITVERTEX &out)
{
	bool res = true;

	LineEuqationType line;
	Create1LineEq(line, vec2, vec3, vec1);

	v3 tempvec1(vec1.x, vec1.y, 0);

	double aDot = (v.x*line.x + v.y*line.y);
	double bDot = (vec1.x*line.x + vec1.y*line.y);

	double scale1 = ( - line.d - aDot) / ( bDot - aDot );

	v3 tempv;
	v3 va(v.x, v.y, v.z);
	tempv = va + ((float)scale1 * (tempvec1 - va));

	double s1 = (v.x-vec1.x)/(tempv.x-vec1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-vec1.y)/(tempv.y-vec1.y);
		if( !_finite(s1) )	return false;
	}

	double s2 = (tempv.x-vec2.x)/(vec3.x-vec2.x);
	if( !_finite(s2) || abs(s2) > 100 )
	{
		s2 = (tempv.y-vec2.y)/(vec3.y-vec2.y);
		if( !_finite(s2) )	return false;
	}

	if( !_finite(s1) || !_finite(s2) )
	{
		memcpy(&out, &vec3, sizeof(TLITVERTEX) );
		RDP_NOIMPL_WARN("s1 or s2 is still infinite, it is a coplaner triangle");
		return false;
	}

	out.x = v.x;
	out.y = v.y;
	out.z = v.z;
	//out.z = interp3p(vec1.z,vec2.z,vec3.z,s1,s2);
	out.rhw = interp3p(vec1.rhw,vec2.rhw,vec3.rhw,s1,s2);

	if( !_finite(out.z) || !_finite(out.rhw) || out.rhw < 0 )
	{
#ifdef _DEBUG
		if( out.rhw < 0 )	
			DebuggerAppendMsg("Warning: rhw<0");
		else
			RDP_NOIMPL_WARN("Z or RHW value is #INF, check me as below");
		DebuggerAppendMsg("vec1=[%f %f %f %f];\n",vec1.x, vec1.y, vec1.z, vec1.rhw);
		DebuggerAppendMsg("vec2=[%f %f %f %f];\n",vec2.x, vec2.y, vec2.z, vec2.rhw);
		DebuggerAppendMsg("vec3=[%f %f %f %f];\n",vec3.x, vec3.y, vec3.z, vec3.rhw);
		DebuggerAppendMsg("v=[%f %f];\n",v.x, v.y);
		DebuggerAppendMsg("v-out=[z=%f rhw=%f];\n",out.z, out.rhw);
		DebuggerAppendMsg("tempv=[%f %f %f];\n",tempv.x, tempv.y, tempv.z);
		DebuggerAppendMsg("s1=%f, s2=%f;\n",s1,s2);
#endif
		return false;
	}


	if( out.z > 1 )
	{
		RDP_NOIMPL_WARN("Warning: z>1");
		return false;
	}

	out.dcSpecular = vec2.dcSpecular; //fix me here
	if( gRDP.tnl.Fog )
	{
		float f1 = (vec1.dcSpecular >> 24)*vec1.rhw;
		float f2 = (vec2.dcSpecular >> 24)*vec2.rhw;
		float f3 = (vec3.dcSpecular >> 24)*vec3.rhw;
		float f = interp3p(f1,f2,f3,s1,s2)/out.rhw;	
		if( f < 0 )	f = 0;
		if( f > 255 ) f = 255;
		uint32 fb = (uint8)f;
		out.dcSpecular &= 0x00FFFFFF;
		out.dcSpecular |= (fb<<24);
	}

	float r = interp3p(vec1.r*vec1.rhw,vec2.r*vec2.rhw,vec3.r*vec3.rhw,s1,s2)/out.rhw;	
	if( r<0 )	r=0; 
	if( r>255 )	r=255;
	out.r = (uint8)r;
	float g = interp3p(vec1.g*vec1.rhw,vec2.g*vec2.rhw,vec3.g*vec3.rhw,s1,s2)/out.rhw;
	if( g<0 )	g=0; 
	if( g>255 )	g=255;
	out.g = (uint8)g;
	float b = interp3p(vec1.b*vec1.rhw,vec2.b*vec2.rhw,vec3.b*vec3.rhw,s1,s2)/out.rhw;
	if( b<0 )	b=0; 
	if( b>255 )	b=255;
	out.b = (uint8)b;
	float a = interp3p(vec1.a*vec1.rhw,vec2.a*vec2.rhw,vec3.a*vec3.rhw,s1,s2)/out.rhw;
	if( a<0 )	a=0; 
	if( a>255 )	a=255;
	out.a = (uint8)a;

	for( int i=0; i<2; i++ )
	{
		out.tcord[i].u = interp3p(vec1.tcord[i].u*vec1.rhw,vec2.tcord[i].u*vec2.rhw,vec3.tcord[i].u*vec3.rhw,s1,s2)/out.rhw;
		out.tcord[i].v = interp3p(vec1.tcord[i].v*vec1.rhw,vec2.tcord[i].v*vec2.rhw,vec3.tcord[i].v*vec3.rhw,s1,s2)/out.rhw;
	}

	if( out.rhw < 0 )	
	{
		return false;
	}
	else
		return true;
}

bool Interp1Point2pZ(PointInfo &v, TLITVERTEX &vec1, TLITVERTEX &vec2)
{
	double s1 = (v.x-vec1.x)/(vec2.x-vec1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-vec1.y)/(vec2.y-vec1.y);
		if( !_finite(s1) )	return false;
	}

	v.z = interp2p(vec1.z,vec2.z,s1);
	return true;
}

bool Interp1Point2p(PointInfo &v, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &out)
{
	double s1 = (v.x-vec1.x)/(vec2.x-vec1.x);
	if( !_finite(s1) || abs(s1) > 100 )
	{
		s1 = (v.y-vec1.y)/(vec2.y-vec1.y);
		if( !_finite(s1) )	return false;
	}

	out.x = v.x;
	out.y = v.y;
	out.z = v.z;
	//out.z = interp2p(vec1.z,vec2.z,s1);
	out.rhw = interp2p(vec1.rhw,vec2.rhw,s1);

	if( !_finite(out.z) || !_finite(out.rhw) || out.rhw < 0 )
	{
#ifdef _DEBUG
		if( out.rhw < 0 )	
			DebuggerAppendMsg("Warning: rhw<0");
		else
			RDP_NOIMPL_WARN("Z or RHW value is #INF, check me as below");
		DebuggerAppendMsg("vec1=[%f %f %f %f];\n",vec1.x, vec1.y, vec1.z, vec1.rhw);
		DebuggerAppendMsg("vec2=[%f %f %f %f];\n",vec2.x, vec2.y, vec2.z, vec2.rhw);
		DebuggerAppendMsg("v=[%f %f];\n",v.x, v.y);
		DebuggerAppendMsg("v-out=[z=%f rhw=%f];\n",out.z, out.rhw);
		DebuggerAppendMsg("s1=%f;\n",s1);
#endif
		return false;
	}

	if( out.z > 1 )	
		RDP_NOIMPL_WARN("Warning: z>1");

	out.dcSpecular = vec2.dcSpecular; //fix me here
	if( gRDP.tnl.Fog )
	{
		float f1 = (vec1.dcSpecular>>24)*vec1.rhw;
		float f2 = (vec2.dcSpecular>>24)*vec2.rhw;
		float f = interp2p(f1,f2,s1)/out.rhw;	
		if( f < 0 )	f = 0;
		if( f > 255 ) f = 255;
		uint32 fb = (uint8)f;
		out.dcSpecular &= 0x00FFFFFF;
		out.dcSpecular |= (fb<<24);
	}

	float r = interp2p(vec1.r*vec1.rhw,vec2.r*vec2.rhw,s1)/out.rhw;	
	if( r<0 )	r=0; 
	if( r>255 )	r=255;
	out.r = (uint8)r;
	float g = interp2p(vec1.g*vec1.rhw,vec2.g*vec2.rhw,s1)/out.rhw;
	if( g<0 )	g=0; 
	if( g>255 )	g=255;
	out.g = (uint8)g;
	float b = interp2p(vec1.b*vec1.rhw,vec2.b*vec2.rhw,s1)/out.rhw;
	if( b<0 )	b=0; 
	if( b>255 )	b=255;
	out.b = (uint8)b;
	float a = interp2p(vec1.a*vec1.rhw,vec2.a*vec2.rhw,s1)/out.rhw;
	if( a<0 )	a=0; 
	if( a>255 )	a=255;
	out.a = (uint8)a;

	for( int i=0; i<2; i++ )
	{
		out.tcord[i].u = interp2p(vec1.tcord[i].u*vec1.rhw,vec2.tcord[i].u*vec2.rhw,s1)/out.rhw;
		out.tcord[i].v = interp2p(vec1.tcord[i].v*vec1.rhw,vec2.tcord[i].v*vec2.rhw,s1)/out.rhw;
	}

	if( out.rhw < 0 )	
	{
		return false;
	}
	else
		return true;
}

bool ClipXYBy1Triangle( std::vector<PointInfo> &points, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3 )
{
	if( abs(EvaLine(alllines[0],vec3.x, vec3.y) ) < 1 )	// vec1, vec2, vec3 are linear
	{
		points.clear();
		return false;
	}

	if( vec1.rhw >= 0 || vec2.rhw >= 0 )	ClipFor1LineXY(points, 0, vec1, vec2);
	if( points.size() < 3 )	return false;
	if( vec1.rhw >= 0 || vec3.rhw >= 0 )	ClipFor1LineXY(points, 1, vec1, vec3);
	if( points.size() < 3 )	return false;
	if( vec2.rhw >= 0 || vec3.rhw >= 0 )	ClipFor1LineXY(points, 2, vec2, vec3);
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

void InterpolatePoints(std::vector<PointInfo> &points, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3, int &dstidx)
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
			memcpy(&newvertexes[i], &vec1, sizeof(TLITVERTEX) );
			continue;
		}
		else if( p.onVtx == 1 )
		{
			memcpy(&newvertexes[i], &vec2, sizeof(TLITVERTEX) );
			continue;
		}
		else if( p.onVtx == 2 )
		{
			memcpy(&newvertexes[i], &vec3, sizeof(TLITVERTEX) );
			continue;
		}
		else if( p.onLine == 0 )
		{
			if( !Interp1Point2p(p,vec1,vec2,newvtx) )
			{
				delete [] newvertexes;
				return; 
			}
		}
		else if( p.onLine == 1 )
		{
			if( !Interp1Point2p(p,vec1,vec3,newvtx) )
			{
				delete [] newvertexes;
				return; 
			}
		}
		else if( p.onLine == 2 )
		{
			if( !Interp1Point2p(p,vec2,vec3,newvtx) )
			{
				delete [] newvertexes;
				return; 
			}
		}
		//else if( abs(p.x-vec1.x)<1 && abs(p.y-vec1.y)<1 )
		//{
		//	memcpy(&newvertexes[i], &vec1, sizeof(TLITVERTEX) );
		//	continue;
		//}
		//else if( abs(p.x-vec2.x)<1 && abs(p.y-vec2.y)<1 )
		//{
		//	memcpy(&newvertexes[i], &vec2, sizeof(TLITVERTEX) );
		//	continue;
		//}
		//else if( abs(p.x-vec3.x)<1 && abs(p.y-vec3.y)<1 )
		//{
		//	memcpy(&newvertexes[i], &vec3, sizeof(TLITVERTEX) );
		//	continue;
		//}
		//else if( abs(EvaLine(alllines[0],p.x, p.y) ) < 1 )	// vec1, vec2, p are linear
		//{
		//	if( !Interp1Point2p(p,vec1,vec2,newvtx) )
		//	{
		//		delete [] newvertexes;
		//		return; 
		//	}
		//}
		//else if( abs(EvaLine(alllines[1],p.x, p.y) ) < 1 )	// vec1, vec3, p are linear
		//{
		//	if( !Interp1Point2p(p,vec1,vec3,newvtx) )
		//	{
		//		delete [] newvertexes;
		//		return; 
		//	}
		//}
		//else if( abs(EvaLine(alllines[2],p.x, p.y) ) < 1 )	// vec2, vec3, p are linear
		//{
		//	if( !Interp1Point2p(p,vec2,vec3,newvtx) )
		//	{
		//		delete [] newvertexes;
		//		return; 
		//	}
		//}
		else if( !Interp1Point3p(p,vec1,vec2,vec3,newvtx) )
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

void InterpolatePointsZ(std::vector<PointInfo> &points, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3)
{
	// now compute z values
	int i;
	int size = points.size();

	for( i=0; i<size; i++ )
	{
		PointInfo &p = points[i];

		//if( abs(p.x-vec1.x)<1 && abs(p.y-vec1.y)<1 )
		//{
		//	p.z = vec1.z;
		//}
		//else if( abs(p.x-vec2.x)<1 && abs(p.y-vec2.y)<1 )
		//{
		//	p.z = vec2.z;
		//}
		//else if( abs(p.x-vec3.x)<1 && abs(p.y-vec3.y)<1 )
		//{
		//	p.z = vec3.z;
		//}
		//else if( abs(EvaLine(alllines[0],p.x, p.y) ) < 1 )	// vec1, vec2, p are linear
		//{
		//	Interp1Point2pZ(p,vec1,vec2);
		//}
		//else if( abs(EvaLine(alllines[1],p.x, p.y) ) < 1 )	// vec1, vec3, p are linear
		//{
		//	Interp1Point2pZ(p,vec1,vec3);
		//}
		//else if( abs(EvaLine(alllines[2],p.x, p.y) ) < 1 )	// vec2, vec3, p are linear
		//{
		//	Interp1Point2pZ(p,vec2,vec3);
		//}
		//else
		{
			Interp1Point3pZ(p,vec1,vec2,vec3);
		}
	}
}

void InitTrianglePoints( std::vector<PointInfo> &points, TLITVERTEX &vec1, TLITVERTEX &vec2, TLITVERTEX &vec3 )
{
	points.clear();

	PointInfo p;
	p.x = vec1.x;
	p.y = vec1.y;
	p.z = vec1.z;
	p.onVtx = 0;
	p.onLine = -1;
	points.push_back(p);
	p.x = vec2.x;
	p.y = vec2.y;
	p.z = vec2.z;
	p.onVtx = 1;
	points.push_back(p);
	p.x = vec3.x;
	p.y = vec3.y;
	p.z = vec3.z;
	p.onVtx = 2;
	points.push_back(p);
}

void ClipVertexes()
{
	// transverse the list of transformed vertex for each triangles
	// - If z values of all 3 vertexes of the triangle are greater than 1, then ignore this triangle
	// - If non z value of all 3 vertexes of the triangle is greater than 1, then this triangle is ok
	// - If one or two z values are greater than 1, then split this triangle to 2 triangles

	int dstidx = 0;

	for (uint32 i = 0; i<gRSP.numVertices / 3; i++)
	{
		int firstidx = i * 3;
		int size;
		TLITVERTEX &v1 = g_vtxBuffer[firstidx];
		TLITVERTEX &v2 = g_vtxBuffer[firstidx + 1];
		TLITVERTEX &v3 = g_vtxBuffer[firstidx + 2];

		if (v1.rhw < 0 && v2.rhw < 0 && v3.rhw < 0)
			continue;	// Skip this triangle

		std::vector<PointInfo> points;

		TRI_DUMP(
		{
			TRACE1("Clip triangle %d for negative w", i);
		TRACE4("V1: x=%f, y=%f, z=%f, rhw=%f", v1.x, v1.y, v1.z, v1.rhw);
		TRACE4("V2: x=%f, y=%f, z=%f, rhw=%f", v2.x, v2.y, v2.z, v2.rhw);
		TRACE4("V3: x=%f, y=%f, z=%f, rhw=%f", v3.x, v3.y, v3.z, v3.rhw);
		});

		SwapVertexPos(firstidx);
		CreateLineEquations(v1, v2, v3);
		InitScreenPoints(points);
		InterpolatePointsZ(points, v1, v2, v3);
		ClipXYBy1Triangle(points, v1, v2, v3);

		TRI_DUMP(
		{
			TRACE1("Got %d vertex after clipping", points.size());
		for (unsigned int k = 0; k<points.size(); k++)
		{
			TRACE4("V(%d): x=%f, y=%f, z=%f", k, points[k].x, points[k].y, points[k].z);
		}
		});

		if( v1.rhw >= 0 && v2.rhw >= 0 && v3.rhw >= 0 )
		{
			CopyVertexData(firstidx, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
			CopyVertexData(firstidx+1, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
			CopyVertexData(firstidx+2, g_vtxBuffer, dstidx++, g_clippedVtxBuffer);
			continue;
		}
		else
		{
			SwapVertexPos(firstidx);
			CreateLineEquations(v1,v2,v3);
			InitScreenPoints(points);
			InterpolatePointsZ(points,v1,v2,v3);
			ClipXYBy1Triangle(points, v1, v2, v3);
		}

		size = points.size();
		if (size < 3) continue;

		if (gRDP.otherMode.z_cmp)
		{
			if (InsertPointsAtNearPlane(points))
			{
				std::vector<PointInfo> polygon1;
				std::vector<PointInfo> polygon2;

				SplitPointsToTwoPolygons(points, polygon1, polygon2);

				InterpolatePoints(polygon1, v1, v2, v3, dstidx);
				InterpolatePoints(polygon2, v1, v2, v3, dstidx);
				polygon1.clear();
				polygon2.clear();
			}
			else
			{
				InterpolatePoints(points, v1, v2, v3, dstidx);
			}
		}
		else
		{
			InterpolatePoints(points, v1, v2, v3, dstidx);
		}

		points.clear();
	}

	g_clippedVtxCount = dstidx;
	for (int m = 0; m<dstidx; m++)
	{
		if (g_clippedVtxBuffer[m].z < 0) g_clippedVtxBuffer[m].z = 0;
	}
}