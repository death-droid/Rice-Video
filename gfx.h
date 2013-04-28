/**********************************************************************************
Common gfx plugin spec, version #1.3 maintained by zilmar (zilmar@emulation64.com)

All questions or suggestions should go through the mailing list.
http://www.egroups.com/group/Plugin64-Dev
***********************************************************************************

Notes:
------

Setting the approprate bits in the MI_INTR_REG and calling CheckInterrupts which 
are both passed to the DLL in InitiateGFX will generate an Interrupt from with in 
the plugin.

The Setting of the RSP flags and generating an SP interrupt  should not be done in
the plugin

**********************************************************************************/
#ifndef _GFX_H_INCLUDED__
#define _GFX_H_INCLUDED__

#include "typedefs.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* Plugin types */
#define PLUGIN_TYPE_GFX				2

#ifndef EXPORT
#define EXPORT						__declspec(dllexport)
#endif

#define CALL						_cdecl

/***** Structures *****/
typedef struct {
	uint16 Version;        /* Set to 0x0103 */
	uint16 Type;           /* Set to PLUGIN_TYPE_GFX */
	char Name[100];      /* Name of the DLL */

	/* If DLL supports memory these memory options then set them to TRUE or FALSE
	   if it does not support it */
	BOOL NormalMemory;    /* a normal uint8 array */ 
	BOOL MemoryBswaped;  /* a normal uint8 array where the memory has been pre
	                          bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

typedef struct {
	HWND hWnd;			/* Render window */
	HWND hStatusBar;    /* if render window does not have a status bar then this is NULL */

	BOOL MemoryBswaped;    // If this is set to TRUE, then the memory has been pre
	                       //   bswap on a dword (32 bits) boundry 
						   //	eg. the first 8 bytes are stored like this:
	                       //        4 3 2 1   8 7 6 5

	uint8 * HEADER;	// This is the rom header (first 40h bytes of the rom
					// This will be in the same memory format as the rest of the memory.
	uint8 * RDRAM;
	uint8 * DMEM;
	uint8 * IMEM;

	uint32 * MI_INTR_REG;

	uint32 * DPC_START_REG;
	uint32 * DPC_END_REG;
	uint32 * DPC_CURRENT_REG;
	uint32 * DPC_STATUS_REG;
	uint32 * DPC_CLOCK_REG;
	uint32 * DPC_BUFBUSY_REG;
	uint32 * DPC_PIPEBUSY_REG;
	uint32 * DPC_TMEM_REG;

	uint32 * VI_STATUS_REG;
	uint32 * VI_ORIGIN_REG;
	uint32 * VI_WIDTH_REG;
	uint32 * VI_INTR_REG;
	uint32 * VI_V_CURRENT_LINE_REG;
	uint32 * VI_TIMING_REG;
	uint32 * VI_V_SYNC_REG;
	uint32 * VI_H_SYNC_REG;
	uint32 * VI_LEAP_REG;
	uint32 * VI_H_START_REG;
	uint32 * VI_V_START_REG;
	uint32 * VI_V_BURST_REG;
	uint32 * VI_X_SCALE_REG;
	uint32 * VI_Y_SCALE_REG;

	void (*CheckInterrupts)( void );
} GFX_INFO;


typedef struct
{
	uint32 addr;
	uint32 val;
	uint32 size;				// 1 = uint8, 2 = uint16, 4=uint32
} FrameBufferModifyEntry;

#define NAME_DEFINE(name)  CALL name
#define FUNC_TYPE(type) EXPORT type
/******************************************************************
  Function: CaptureScreen
  Purpose:  This function dumps the current frame to a file
  input:    pointer to the directory to save the file to
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(CaptureScreen) ( char * Directory );

/******************************************************************
  Function: ChangeWindow
  Purpose:  to change the window between fullscreen and window 
            mode. If the window was in fullscreen this should 
			change the screen to window mode and vice vesa.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(ChangeWindow) (void);

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(CloseDLL) (void);

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(DllAbout) ( HWND hParent );

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(DllConfig) ( HWND hParent );

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(DllTest) ( HWND hParent );

/******************************************************************
  Function: DrawScreen
  Purpose:  This function is called when the emulator receives a
            WM_PAINT message. This allows the gfx to fit in when
			it is being used in the desktop.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(DrawScreen) (void);

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(GetDllInfo) ( PLUGIN_INFO * PluginInfo );

/******************************************************************
  Function: InitiateGFX
  Purpose:  This function is called when the DLL is started to give
            information from the emulator that the n64 graphics
			uses. This is not called from the emulation thread.
  Input:    Gfx_Info is passed to this function which is defined
            above.
  Output:   TRUE on success
            FALSE on failure to initialise
             
  ** note on interrupts **:
  To generate an interrupt set the appropriate bit in MI_INTR_REG
  and then call the function CheckInterrupts to tell the emulator
  that there is a waiting interrupt.
*******************************************************************/ 
FUNC_TYPE(BOOL) NAME_DEFINE(InitiateGFX) (GFX_INFO Gfx_Info);

/******************************************************************
  Function: MoveScreen
  Purpose:  This function is called in response to the emulator
            receiving a WM_MOVE passing the xpos and ypos passed
			from that message.
  input:    xpos - the x-coordinate of the upper-left corner of the
            client area of the window.
			ypos - y-coordinate of the upper-left corner of the
			client area of the window. 
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(MoveScreen) (int xpos, int ypos);

/******************************************************************
  Function: ProcessDList
  Purpose:  This function is called when there is a Dlist to be
            processed. (High level GFX list)
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(ProcessDList)(void);
FUNC_TYPE(uint32) NAME_DEFINE(ProcessDListCountCycles)(void);

/******************************************************************
  Function: ProcessRDPList
  Purpose:  This function is called when there is a Dlist to be
            processed. (Low level GFX list)
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(ProcessRDPList)(void);

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(RomClosed) (void);

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the 
            emulation thread)
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(bool) NAME_DEFINE(RomOpen) (void);

/******************************************************************
  Function: ShowCFB
  Purpose:  Useally once Dlists are started being displayed, cfb is
            ignored. This function tells the dll to start displaying
			them again.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(ShowCFB) (void);

/******************************************************************
  Function: UpdateScreen
  Purpose:  This function is called in response to a vsync of the
            screen were the VI bit in MI_INTR_REG has already been
			set
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(UpdateScreen) (void);

/******************************************************************
  Function: ViStatusChanged
  Purpose:  This function is called to notify the dll that the
            ViStatus registers value has been changed.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(ViStatusChanged) (void);

/******************************************************************
  Function: ViWidthChanged
  Purpose:  This function is called to notify the dll that the
            ViWidth registers value has been changed.
  input:    none
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(ViWidthChanged) (void);



/******************************************************************
  Function: FrameBufferWrite
  Purpose:  This function is called to notify the dll that the
            frame buffer has been modified by CPU at the given address.
  input:    addr		rdram address
			val			val
			size		1 = uint8, 2 = uint16, 4 = uint32
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(FBWrite)(uint32, uint32);

/******************************************************************
  Function: FrameBufferWriteList
  Purpose:  This function is called to notify the dll that the
            frame buffer has been modified by CPU at the given address.
  input:    FrameBufferModifyEntry *plist
			size = size of the plist, max = 1024
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(FBWList)(FrameBufferModifyEntry *plist, uint32 size);

/******************************************************************
  Function: FrameBufferRead
  Purpose:  This function is called to notify the dll that the
            frame buffer memory is beening read at the given address.
			DLL should copy content from its render buffer to the frame buffer
			in N64 RDRAM
			DLL is responsible to maintain its own frame buffer memory addr list
			DLL should copy 4KB block content back to RDRAM frame buffer.
			Emulator should not call this function again if other memory
			is read within the same 4KB range
  input:    addr		rdram address
			val			val
			size		1 = uint8, 2 = uint16, 4 = uint32
  output:   none
*******************************************************************/ 
FUNC_TYPE(void) NAME_DEFINE(FBRead)(uint32 addr);




/************************************************************************
Function: FBGetFrameBufferInfo
Purpose:  This function is called by the emulator core to retrieve depth
buffer information from the video plugin in order to be able
to notify the video plugin about CPU depth buffer read/write
operations

size:
= 1		byte
= 2		word (16 bit) <-- this is N64 default depth buffer format
= 4		dword (32 bit)

when depth buffer information is not available yet, set all values
in the FrameBufferInfo structure to 0

input:    FrameBufferInfo *pinfo
pinfo is pointed to a FrameBufferInfo structure which to be
filled in by this function
output:   Values are return in the FrameBufferInfo structure
/************************************************************************/
FUNC_TYPE(void) NAME_DEFINE(FBGetFrameBufferInfo)(void *pinfo);


/******************************************************************
  Function: GetFullScreenStatus
  Purpose:  
  Input:    
  Output:   TRUE if current display is in full screen
            FALSE if current display is in windowed mode

  Attention: After the CPU core call the ChangeWindow function to request
			 the video plugin to switch between full screen and window mode,
			 the plugin may not carry out the request at the function call.
			 The video plugin may want to delay and do the screen switching later.
             
*******************************************************************/ 
EXPORT BOOL CALL GetFullScreenStatus(void);

/******************************************************************
  Function: SetOnScreenText
  Purpose:  
  Input:    char *msg
  Output:   
             
*******************************************************************/ 
EXPORT void CALL SetOnScreenText(char *msg);


FUNC_TYPE(void) NAME_DEFINE(SetMaxTextureMem)(DWORD mem);

#if defined(__cplusplus)
}
#endif
#endif
