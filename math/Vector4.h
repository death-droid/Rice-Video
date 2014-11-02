#ifndef MATH_VECTOR4_H_
#define MATH_VECTOR4_H_
__declspec(align(16)) class v4
{
public:
	v4() {}
	v4( float _x, float _y, float _z, float _w ) : x( _x ), y( _y ), z( _z ), w( _w ) {}

	float Normalise()
	{
		float	len_sq( LengthSq() );
		if(len_sq > 0.0001f)
		{
			float r( 1.0f / sqrtf( len_sq ) );

			x *= r;
			y *= r;
			z *= r;
			w *= r;
		}
		return len_sq;
	}

	v4 operator+( const v4 & v ) const
	{
		return v4( x + v.x, y + v.y, z + v.z, w + v.w );
	}
	v4 operator-( const v4 & v ) const
	{
		return v4( x - v.x, y - v.y, z - v.z, w - v.w );
	}

	v4 operator*( float s ) const
	{
		return v4( x * s, y * s, z * s, w * s );
	}

	float Length() const
	{
		return sqrtf( (x*x)+(y*y)+(z*z)+(w*w) );
	}

	float LengthSq() const
	{
		return (x*x)+(y*y)+(z*z)+(w*w);
	}

	float Dot( const v4 & rhs ) const
	{
		return (x*rhs.x) + (y*rhs.y) + (z*rhs.z) + (w*rhs.w);
	}

	float x, y, z, w;
};


#endif // MATH_VECTOR4_H_
