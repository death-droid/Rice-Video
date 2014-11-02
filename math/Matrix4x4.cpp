#include "../stdafx.h"
#include "./Matrix4x4.h"
#include "./Vector3.h"
#include "./Vector4.h"
#include "./Math.h"

void MatrixMultiplyUnaligned(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b)
{
	*m_out = *mat_a * *mat_b;
}

void MatrixMultiplyAligned(Matrix4x4 * m_out, const Matrix4x4 *mat_a, const Matrix4x4 *mat_b)
{
	*m_out = *mat_a * *mat_b;
}

Matrix4x4 & Matrix4x4::SetIdentity()
{
	*this = gMatrixIdentity;
	return *this;
}

Matrix4x4 & Matrix4x4::SetTranslate( const v3 & vec )
{
	m11 = 1;		m12 = 0;		m13 = 0;		m14 = 0;
	m21 = 0;		m22 = 1;		m23 = 0;		m24 = 0;
	m31 = 0;		m32 = 0;		m33 = 1;		m34 = 0;
	m41 = vec.x;	m42 = vec.y;	m43 = vec.z;	m44 = 1;
	return *this;
}

Matrix4x4 & Matrix4x4::SetScaling( float scale )
{
	for ( u32 r = 0; r < 4; ++r )
	{
		for ( u32 c = 0; c < 4; ++c )
		{
			m[ r ][ c ] = ( r == c ) ? scale : 0;
		}
	}
	return *this;
}

Matrix4x4 & Matrix4x4::SetRotateX( float angle )
{
	float	s = sinf(angle);
	float	c = cosf(angle);

	m11 = 1;	m12 = 0;	m13 = 0;	m14 = 0;
	m21 = 0;	m22 = c;	m23 = -s;	m24 = 0;
	m31 = 0;	m32 = s;	m33 = c;	m34 = 0;
	m41 = 0;	m42 = 0;	m43 = 0;	m44 = 1;
	return *this;
}

Matrix4x4 & Matrix4x4::SetRotateY( float angle )
{
	float	s = sinf(angle);
	float	c = cosf(angle);

	m11 = c;	m12 = 0;	m13 = s;	m14 = 0;
	m21 = 0;	m22 = 1;	m23 = 0;	m24 = 0;
	m31 = -s;	m32 = 0;	m33 = c;	m34 = 0;
	m41 = 0;	m42 = 0;	m43 = 0;	m44 = 1;
	return *this;
}

Matrix4x4 & Matrix4x4::SetRotateZ( float angle )
{
	float	s = sinf(angle);
	float	c = cosf(angle);

	m11 = c;	m12 = -s;	m13 = 0;	m14 = 0;
	m21 = s;	m22 = c;	m23 = 0;	m24 = 0;
	m31 = 0;	m32 = 0;	m33 = 1;	m34 = 0;
	m41 = 0;	m42 = 0;	m43 = 0;	m44 = 1;
	return *this;
}

v3 Matrix4x4::TransformCoord( const v3 & vec ) const
{
	return v3( vec.x * m11 + vec.y * m21 + vec.z * m31 + m41,
			   vec.x * m12 + vec.y * m22 + vec.z * m32 + m42,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 + m43 );
}

v3 Matrix4x4::TransformNormal( const v3 & vec ) const
{
	return v3( vec.x * m11 + vec.y * m21 + vec.z * m31,
			   vec.x * m12 + vec.y * m22 + vec.z * m32,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 );
}

v4 Matrix4x4::Transform( const v4 & vec ) const
{
	return v4( vec.x * m11 + vec.y * m21 + vec.z * m31 + vec.w * m41,
			   vec.x * m12 + vec.y * m22 + vec.z * m32 + vec.w * m42,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 + vec.w * m43,
			   vec.x * m14 + vec.y * m24 + vec.z * m34 + vec.w * m44 );
}

v3 Matrix4x4::Transform( const v3 & vec ) const
{
	v4	trans( vec.x * m11 + vec.y * m21 + vec.z * m31 + m41,
			   vec.x * m12 + vec.y * m22 + vec.z * m32 + m42,
			   vec.x * m13 + vec.y * m23 + vec.z * m33 + m43,
			   vec.x * m14 + vec.y * m24 + vec.z * m34 + m44 );

	if(abs(trans.w) > 0.0f)
	{
		return v3( trans.x / trans.w, trans.y / trans.w, trans.z / trans.w );
	}

	return v3(trans.x, trans.y, trans.z);
}

Matrix4x4 Matrix4x4::operator*( const Matrix4x4 & rhs ) const
{
	Matrix4x4 r;

	for ( u32 i = 0; i < 4; ++i )
	{
		for ( u32 j = 0; j < 4; ++j )
		{
			r.m[ i ][ j ] = m[ i ][ 0 ] * rhs.m[ 0 ][ j ] +
							m[ i ][ 1 ] * rhs.m[ 1 ][ j ] +
							m[ i ][ 2 ] * rhs.m[ 2 ][ j ] +
							m[ i ][ 3 ] * rhs.m[ 3 ][ j ];
		}
	}

	return r;
}

const Matrix4x4	gMatrixIdentity( 1.0f, 0.0f, 0.0f, 0.0f,
								 0.0f, 1.0f, 0.0f, 0.0f,
								 0.0f, 0.0f, 1.0f, 0.0f,
								 0.0f, 0.0f, 0.0f, 1.0f );