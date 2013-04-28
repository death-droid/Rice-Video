#include "BMGUtils.h"

#ifdef __BORLANDC__
#pragma argsused
#endif
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason,
                         void* lpReserved)
{
    switch( reason )
    {
    case DLL_PROCESS_ATTACH:
        InitBackground();
        break;
    case DLL_PROCESS_DETACH:
        FreeBMGImage( GetBackgroundImage() );
        break;
    }

    return 1;
}
//---------------------------------------------------------------------------
 