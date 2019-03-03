#ifndef __RTOBJECT_H
#define __RTOBJECT_H

#include <embree3/rtcore.h>

#include "geom.h"

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
};

class RTTriangleMesh : public RTObject {
public:
	RTTriangleMesh(RTScene * s);
public:
	void loadFile(char * fname);
public:
	int num_vertices, num_triangles;
};

class RTSkyBox : public RTObject {
public:
	RTSkyBox(RTScene * s, float l, vec3f * p);
public:
	void loadFile(char * fname);
public:
	float len;
	vec3f * pos;
private:
	int texw, texh;
	unsigned char *red, *green, *blue;
};

#endif
