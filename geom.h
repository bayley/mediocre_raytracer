#ifndef __GEOM_H
#define __GEOM_H

#include <math.h>

class matrix3f;
class vec3f;
class Camera;

class matrix3f {
public:
	matrix3f(float a11, float a12, float a13,
					 float a21, float a22, float a23,
					 float a31, float a32, float a33);
	matrix3f() {}
public:
	vec3f * mul(vec3f * x);
	matrix3f * mul(float c);
	float at(int i, int j);
	void set(int i, int j, float x);
private:
	float data[3][3];
};

class vec3f {
public:
	vec3f() {}
	vec3f(float u, float v, float w) {x = u; y = v; z = w;}
public:
	float abs() {return sqrtf(x * x + y * y + z * z);}
	vec3f * cross(vec3f * b);
	float dot(vec3f * b) {return x * b->x + y * b->y + z * b->z;}
public:
	void normalize(); //modifies this vector!
public:
	float x; float y; float z;
};

class Camera {
public:
	Camera(float ex, float ey, float ez, float dx, float dy, float dz, float theta, int w, int h);
public:
	vec3f * lookat(int x, int y);
private:
	void update(float ex, float ey, float ez, float dx, float dy, float dz, float theta, int w, int h);
private:
	vec3f *eye, *dir;
	vec3f *u, *v;
	int width, height;
};

//utility functions
matrix3f * rotation(float theta, int axis);
vec3f * mul(vec3f * v, float c);
vec3f * add(vec3f * u, vec3f * v);
vec3f * sub(vec3f * u, vec3f * v);

//these don't do much, so they are just structs
typedef struct {
	int v0; int v1; int v2;
} Triangle;

typedef struct {
	int num_vertices;
	int num_triangles;
} MeshSize;

enum {
	AXIS_X,
	AXIS_Y,
	AXIS_Z,
};

typedef vec3f Vertex;

#endif
