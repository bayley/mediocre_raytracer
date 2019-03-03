#ifndef __RTOBJECT_H
#define __RTOBJECT_H

#include <embree3/rtcore.h>

#include "geom.h"
#include "brdf.h"

class RTObject;

//encapsulates scene, device, rayhit, context
//not to be confused with RTCScene!
class RTScene {
public:
	RTScene();
public:
	int record_obj(RTObject * obj);
	void commit();
public:
	void resetRH();
	void resetR();
	void cleanup();
public:
	void move(float x, float y, float z);
	void point(float x, float y, float z);
	void zoom(float theta);
	void resize(int w, int h);
public:
	vec3f * hitP();
	vec3f * hitN();
public:
	vec3f * color(int id, int prim, float u, float v);
	float reflect(int id, float theta_i, float phi_i, float theta_o, float phi_o);
	vec3f * emit(int id, int prim, float u, float v);
public:
	RTCDevice device;
	RTCScene scene;
	RTCRayHit rh;
	RTCIntersectContext context;
	Camera * cam;
private:
	RTObject ** obs;
	int obj_count;
};

class RTObject {
public:
	RTCDevice * device;
	RTCScene * scene;
	RTCGeometry geom;
	int id;
public:
	virtual vec3f * color(int id, float u, float v) {return new vec3f(0.f, 0.f, 0.f);}
	virtual float reflect(float theta_i, float phi_i, float theta_o, float phi_o) {return 1.f;}
	virtual vec3f * emit(int id, float u, float v) {return new vec3f(0.f, 0.f, 0.f);}
};

class RTTriangleMesh : public RTObject {
public:
	RTTriangleMesh(RTScene * s, brdf_t m, emit_t b);
public:
	void loadFile(char * fname);
public:
	virtual vec3f * color(int id, float u, float v);
	virtual float reflect(float theta_i, float phi_i, float theta_o, float phi_o);
	virtual vec3f * emit(int id, float u, float v);
public:
	brdf_t material;
	emit_t emission;
	int num_vertices, num_triangles;
};

class RTSkyBox : public RTObject {
public:
	RTSkyBox(RTScene * s, float l, vec3f * p);
public:
	void loadFile(char * sname, char * bname, int w, int h);
public:
	virtual vec3f * color(int id, float u, float v);
	virtual vec3f * emit(int id, float u, float v);
public:
	float len;
	vec3f * pos;
private:
	int texw, texh;
	unsigned char *s_red, *s_green, *s_blue;
	unsigned char *b_red, *b_green, *b_blue;
	unsigned char *t_red, *t_green, *t_blue;
};

#endif
