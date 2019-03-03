#ifndef __BRDF_H
#define __BRDF_H

#include "geom.h"

typedef float (*brdf_t)(float, float, float, float);
typedef float (*emit_t)(int, float, float);

//uniform BRDF
float brdf_lambert(float theta_i, float phi_i, float theta_o, float phi_o);

//all zeroes, for HDRI's
float brdf_black(float theta_i, float phi_i, float theta_o, float phi_o);

//no emission
float emit_black(int id, float u, float v);

//constant emission
float emit_white(int id, float u, float v);

#endif
