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

mov oD0, iColor


// Texture coordinates
mul R_TEMP2, iT0.xy, c[T0_SCALE_X_Y]
add oT0.xy, R_TEMP2.xy, c[T0_OFFSET_X_Y]


// Optional transforms

// viewport transform
mov oPos, R_POS

