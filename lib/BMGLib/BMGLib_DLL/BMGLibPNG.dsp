# Microsoft Developer Studio Project File - Name="BMGLibPNG" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=BMGLibPNG - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "BMGLibPNG.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BMGLibPNG.mak" CFG="BMGLibPNG - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BMGLibPNG - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BMGLibPNG - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BMGLibPNG - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBPNG_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\lpng125" /I "..\zlib114" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBPNG_EXPORTS" /D "_BMG_LIBPNG_STANDALONE" /D "BUILD_BMG_DLL" /D "PNG_NO_CONSOLE_IO" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "BMGLibPNG - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ""
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBPNG_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\lpng125" /I "..\zlib114" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBPNG_EXPORTS" /D "_BMG_LIBPNG_STANDALONE" /D "BUILD_BMG_DLL" /D "PNG_NO_CONSOLE_IO" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "BMGLibPNG - Win32 Release"
# Name "BMGLibPNG - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "LibPNG src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lpng125\png.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngerror.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngget.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngmem.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngpread.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngread.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngrio.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngrtran.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngrutil.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngset.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngtrans.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngwio.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngwrite.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngwtran.c
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngwutil.c
# End Source File
# End Group
# Begin Group "zLib src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\zlib114\adler32.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\compress.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\crc32.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\deflate.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\gzio.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\infblock.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\infcodes.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\inffast.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\inflate.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\inftrees.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\infutil.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\trees.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\uncompr.c
# End Source File
# Begin Source File

SOURCE=..\zlib114\zutil.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\bcbLibPNG.c
# End Source File
# Begin Source File

SOURCE=.\BMGImage.c
# End Source File
# Begin Source File

SOURCE=.\BMGUtils.c
# End Source File
# Begin Source File

SOURCE=.\pngrw.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "LibPNG hdr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lpng125\png.h
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngasmrd.h
# End Source File
# Begin Source File

SOURCE=..\lpng125\pngconf.h
# End Source File
# End Group
# Begin Group "zLib hdr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\zlib114\deflate.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\infblock.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\infcodes.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\inffast.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\inffixed.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\inftrees.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\infutil.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\trees.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\zconf.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\zlib.h
# End Source File
# Begin Source File

SOURCE=..\zlib114\zutil.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\BMGImage.h
# End Source File
# Begin Source File

SOURCE=.\BMGLibPNG.h
# End Source File
# Begin Source File

SOURCE=.\BMGUtils.h
# End Source File
# Begin Source File

SOURCE=.\pngrw.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
