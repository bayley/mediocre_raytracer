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
	float reflect(int id, float theta_i, float phi_i, float theta_o, float phi_o);
	float emit(int id, int prim, float u, float v);
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
	virtual float reflect(float theta_i, float phi_i, float theta_o, float phi_o) {return 1.f;}
	virtual float emit(int id, float u, float v) {return 0.f;}
};

class RTTriangleMesh : public RTObject {
public:
	RTTriangleMesh(RTScene * s, brdf_t m, emit_t b);
public:
	void loadFile(char * fname);
public:
	virtual float reflect(float theta_i, float phi_i, float theta_o, float phi_o);
	virtual float emit(int id, float u, float v);
public:
	brdf_t material;
	emit_t emission;
	int num_vertices, num_triangles;
};

class RTSkyBox : public RTObject {
public:
	RTSkyBox(RTScene * s, float l, vec3f * p);
public:
	void loadFile(char * fname, int w, int h);
public:
	virtual float reflect(float theta_i, float phi_i, float theta_o, float phi_o);
	virtual float emit(int id, float u, float v);
public:
	float len;
	vec3f * pos;
private:
	int texw, texh;
	unsigned char *red, *green, *blue;
};

#endif
