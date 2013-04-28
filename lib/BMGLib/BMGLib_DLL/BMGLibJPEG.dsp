# Microsoft Developer Studio Project File - Name="BMGLibJPEG" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=BMGLibJPEG - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "BMGLibJPEG.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "BMGLibJPEG.mak" CFG="BMGLibJPEG - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "BMGLibJPEG - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "BMGLibJPEG - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "BMGLibJPEG - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBJPEG_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gz /MT /W3 /GX /O2 /I "..\JPEG" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBJPEG_EXPORTS" /D "BUILD_BMG_DLL" /D "_BMG_LIBJPEG_STANDALONE" /YX /FD /c
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

!ELSEIF  "$(CFG)" == "BMGLibJPEG - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBJPEG_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\JPEG" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "BMGLIBJPEG_EXPORTS" /D "BUILD_BMG_DLL" /D "_BMG_LIBJPEG_STANDALONE" /YX /FD /GZ /c
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

# Name "BMGLibJPEG - Win32 Release"
# Name "BMGLibJPEG - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "JPEG src"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\JPEG\jcapimin.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcapistd.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jccoefct.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jccolor.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcdctmgr.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jchuff.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcinit.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcmainct.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcmarker.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcmaster.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcomapi.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcparam.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcphuff.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcprepct.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jcsample.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jctrans.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdapimin.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdapistd.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdatadst.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdatasrc.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdcoefct.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdcolor.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jddctmgr.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdhuff.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdinput.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdmainct.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdmarker.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdmaster.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdmerge.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdphuff.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdpostct.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdsample.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdtrans.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jerror.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jfdctflt.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jfdctfst.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jfdctint.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jidctflt.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jidctfst.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jidctint.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jidctred.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jmemmgr.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jmemnobs.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jquant1.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jquant2.c
# End Source File
# Begin Source File

SOURCE=..\JPEG\jutils.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\bcbLibJPEG.c
# End Source File
# Begin Source File

SOURCE=.\BMGImage.c
# End Source File
# Begin Source File

SOURCE=.\BMGUtils.c
# End Source File
# Begin Source File

SOURCE=.\jpegrw.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "JPEG hdr"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\JPEG\jchuff.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jconfig.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdct.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jdhuff.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jerror.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jinclude.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jmemsys.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jmorecfg.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jpegint.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jpeglib.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\jversion.h
# End Source File
# Begin Source File

SOURCE=..\JPEG\transupp.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\BMGImage.h
# End Source File
# Begin Source File

SOURCE=.\BMGLibJPEG.h
# End Source File
# Begin Source File

SOURCE=.\BMGUtils.h
# End Source File
# Begin Source File

SOURCE=.\jpegrw.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
