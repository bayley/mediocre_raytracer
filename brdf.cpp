#include <math.h>

#include "brdf.h"

float brdf_lambert(float theta_i, float phi_i, float theta_o, float phi_o) {
	return 1.f;
}

float brdf_black(float theta_i, float phi_i, float theta_o, float phi_o) {
	return 0.f;
}

float emit_black(int id, float u, float v) {
	return 0.f;
}

float emit_white(int id, float u, float v) {
	return 0.3f;
}
