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

#if !defined(AFX_CRITSECT_H__E073C8E6_58B7_4DBE_88A3_D52539BC6C60__INCLUDED_)
#define AFX_CRITSECT_H__E073C8E6_58B7_4DBE_88A3_D52539BC6C60__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CCritSect
{
public:
	CCritSect()
	{
		InitializeCriticalSection(&cs);
	}
	~CCritSect()
	{
		DeleteCriticalSection(&cs);
	}

	void Lock()
	{
		EnterCriticalSection(&cs);
	}
	void Unlock()
	{
		LeaveCriticalSection(&cs);
	}

	bool IsLocked()
	{
		return cs.LockCount > 0;
	}

protected:
	CRITICAL_SECTION cs;
};

#endif // !defined(AFX_CRITSECT_H__E073C8E6_58B7_4DBE_88A3_D52539BC6C60__INCLUDED_)
