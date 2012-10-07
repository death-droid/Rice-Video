/*

  Copyright (C) 2003-2009 Rice1964

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

#ifndef __UNIQUE_H__
#define __UNIQUE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// CUnique is an abstract base class for classes where only one
// instance exists throughout the execution of the program
// Typical usuage is:
//
// CMyUniqueClass::Create();
// ..
// CMyUniqueClass::Get()->DoSomething();
// etc
// ..
// CMyUniqueClass::Destroy();
//
//

#ifdef _MSC_VER
#include <crtdbg.h>
#endif

template < class T > class CUnique  
{
	protected:
		static T * m_pInstance;
	public:
		//CUnique();
		virtual ~CUnique() {}

		static T * Get();
		static void Create();
		static void Destroy();
		static bool IsAvailable();
		
		static void	Attach( T * );
};

template < class T > T * CUnique< T >::m_pInstance = NULL;


template < class T > bool CUnique< T >::IsAvailable()
{
	return (m_pInstance != NULL);
}

template < class T > void CUnique< T >::Destroy()
{
	_ASSERT(m_pInstance != NULL);
	
	delete m_pInstance;
	m_pInstance = NULL;
}

template < class T > T * CUnique< T >::Get()
{
	_ASSERT(m_pInstance != NULL);
	
	return m_pInstance;
}

template < class T > void CUnique< T >::Attach( T * p )
{
	_ASSERT(m_pInstance == NULL);
	
	m_pInstance = p;
}
#endif // __UNIQUE_H__
