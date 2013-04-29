vs_1_1
#include "shaderdx9_include.h"

dcl_position v0
dcl_normal v1
dcl_color v2
dcl_texcoord v3

// Transform position to clip space and output it
dp4 R_POS.x, iPos, c[CV_WORLDVIEWPROJ_0]
dp4 R_POS.y, iPos, c[CV_WORLDVIEWPROJ_1]
dp4 R_POS.z, iPos, c[CV_WORLDVIEWPROJ_2]
dp4 R_POS.w, iPos, c[CV_WORLDVIEWPROJ_3]

// Lighting
// Transform normal to eye space
dp3 R_NORMAL.x, iNormal, c[CV_WORLDVIEW_0]
dp3 R_NORMAL.y, iNormal, c[CV_WORLDVIEW_1]
dp3 R_NORMAL.z, iNormal, c[CV_WORLDVIEW_2]

// Normalize transformed normal
dp3 R_NORMAL.w, R_NORMAL, R_NORMAL
rsq R_NORMAL.w, R_NORMAL.w
mul R_NORMAL, R_NORMAL, R_NORMAL.w

// ** Light 0, the ambient light
mov R_DIFFUSE, c[CV_LIGHT0_AMBIENT]

// ** Light 1
// Dot normal with light vector
// This is the intensity of the diffuse component
mov r3, c[CV_ZERO]
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT1_DIRECTION]

// Calculate the diffuse & specular factors
lit R_TEMP2, R_TEMP1

// add the (diffuse color * diffuse light color * diffuse intensity(R4.y))
mad R_DIFFUSE, R_TEMP2.y, c[CV_LIGHT1_DIFFUSE], R_DIFFUSE

// ** Light 2
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT2_DIRECTION]
lit R_TEMP2, R_TEMP1
mad R_DIFFUSE, R_TEMP2.y, c[CV_LIGHT2_DIFFUSE], R_DIFFUSE

// ** Light 3
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT3_DIRECTION]
lit R_TEMP2, R_TEMP1
mad R_DIFFUSE, R_TEMP2.y, c[CV_LIGHT3_DIFFUSE], R_DIFFUSE

// ** Light 4
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT4_DIRECTION]
lit R_TEMP2, R_TEMP1
mad R_DIFFUSE, R_TEMP2.y, c[CV_LIGHT4_DIFFUSE], R_DIFFUSE

// ** Light 5
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT5_DIRECTION]
lit R_TEMP2, R_TEMP1
mad R_DIFFUSE, R_TEMP2.y, c[CV_LIGHT5_DIFFUSE], R_DIFFUSE

// ** Light 6
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT6_DIRECTION]
lit R_TEMP2, R_TEMP1
mad R_DIFFUSE, R_TEMP2.y, c[CV_LIGHT6_DIFFUSE], R_DIFFUSE

// ** Light 7
dp3 R_TEMP1.x, R_NORMAL, c[CV_LIGHT7_DIRECTION]
lit R_TEMP2, R_TEMP1
mad R_TEMP3, R_TEMP2.y, c[CV_LIGHT7_DIFFUSE], R_DIFFUSE
// Light result is in R_TEMP3
//Still use the vertex alpha
mov R_TEMP3.w, iColor.w	

// If Lighting is not enabled, then use color from either primitive color or from vertex color
add R_TEMP1, c[PRIMARY_COLOR], -iColor
mad R_TEMP1, R_TEMP1, c[USE_PRIMARY_COLOR], iColor
add R_DIFFUSE, R_TEMP1, -R_TEMP3
mad R_DIFFUSE, R_DIFFUSE, c[LIGHTING_ENABLED], R_TEMP3

// Force vertex alpha
add R_TEMP1, c[VTX_ALPHA], -R_DIFFUSE
mad oD0.w, R_TEMP1, c[FORCE_VTX_ALPHA], R_DIFFUSE
mov oD0.xyz, R_DIFFUSE


// Fog factor
// Fog_factor = vertex.z, if w<0 or z<0, then factor=0
slt R_TEMP1, R_POS, c[CV_ZERO]
mul R_TEMP2, R_TEMP1.z, R_POS
mul R_TEMP2, R_TEMP1.w, R_TEMP2
mul R_TEMP2, c[FOG_MUL], R_TEMP2
add R_TEMP2, R_TEMP2, c[FOG_ADD]
mov oD1.w, R_TEMP2.z
mov oFog, R_TEMP2.z



// Texture coordinates
mul R_TEMP2, iT0.xy, c[T0_SCALE_X_Y]
add oT0.xy, R_TEMP2.xy, c[T0_OFFSET_X_Y]
mul R_TEMP2, iT0.xy, c[T1_SCALE_X_Y]
add oT1.xy, R_TEMP2.xy, c[T1_OFFSET_X_Y]



// Optional transforms

// viewport transform
// z = (z+1)*0.5 = z*0.5+0.5
mul R_TEMP2.z, c[CV_HALF], R_POS.z
add R_POS.z, R_TEMP2.z, c[CV_HALF]
mov oPos, R_POS


// Hacks
// z = z/10+0.9
// mad R_TEMP1, oPos, c[CV_TENTH], CV_ONE

// To do
// 1. Replace alpha with fog factor if fog is enabled
