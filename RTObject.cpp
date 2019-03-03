#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "brdf.h"
#include "bmp.h"
#include "geom.h"
#include "RTObject.h"

inline vec3f * eval_ray(RTCRay ray, float t) {
  return new vec3f(ray.org_x + t * ray.dir_x, ray.org_y + t * ray.dir_y, ray.org_z + t * ray.dir_z);
}

RTScene::RTScene() {
	device = rtcNewDevice("");
	scene = rtcNewScene(device);
	cam = new Camera(0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.0f, 100, 100);
	rtcInitIntersectContext(&context);
	rh.ray.time = 0.f;

	obs = (RTObject **)malloc(64 * sizeof(RTObject *));
	obj_count = 0;
}

int RTScene::record_obj(RTObject * obj) {
	if (obj_count >= 64) return -1;
	obs[obj_count] = obj;
	obj_count++;
	return obj_count - 1;
}

void RTScene::commit() {
	rtcCommitScene(scene);
}

void RTScene::resetRH() {
  rh.ray.tnear = 0.01f;
  rh.ray.tfar = FLT_MAX; rh.ray.id = 0;
  rh.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
  rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;
}

void RTScene::resetR() {
  rh.ray.tnear = 0.01f;
  rh.ray.tfar = FLT_MAX; rh.ray.id = 0;
}

void RTScene::cleanup() {
	for (int i = 0; i < obj_count; i++) {
		rtcReleaseGeometry(obs[i]->geom);
	}
	rtcReleaseScene(scene);
	rtcReleaseDevice(device);
}

void RTScene::move(float x, float y, float z) {
	cam->move(new vec3f(x, y, z));
}

void RTScene::point(float x, float y, float z) {
	cam->point(new vec3f(x, y, z));
}

void RTScene::zoom(float theta) {
	cam->zoom(theta);
}

void RTScene::resize(int w, int h) {
	cam->resize(w, h);
}

vec3f * RTScene::hitP() {
	return eval_ray(rh.ray, rh.ray.tfar);
}

vec3f * RTScene::hitN() {
	vec3f * result = new vec3f(rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z);
	result->normalize();
	return result;
}

float RTScene::reflect(int id, float theta_i, float phi_i, float theta_o, float phi_o) {
	return obs[id]->reflect(theta_i, phi_i, theta_o, phi_o);
}

float RTScene::emit(int id, int prim, float u, float v) {
	return obs[id]->emit(prim, u, v);
}

RTTriangleMesh::RTTriangleMesh(RTScene * s, brdf_t m, emit_t b) {
	device = &(s->device);
	scene = &(s->scene);
	geom = rtcNewGeometry(*device, RTC_GEOMETRY_TYPE_TRIANGLE);
	num_vertices = 0;
	num_triangles = 0;
	material = m;
	emission = b;
	id = s->record_obj(this);
}

void RTTriangleMesh::loadFile(char * fname) {
  FILE * in = fopen(fname, "r");
  char * line = NULL; size_t len = 0; ssize_t nread;
  while ((nread = getline(&line, &len, in)) != -1) {
    if (line[0] == 'v') num_vertices++;
    if (line[0] == 'f') num_triangles++;
  }
  free(line);

  Vertex * vertices  = (Vertex*) rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), num_vertices);
  Triangle * triangles = (Triangle*) rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), num_triangles);

  line = NULL; len = 0; fseek(in, 0L, SEEK_SET);
  char fn[100]; int v_index = 0, f_index = 0;
  float x, y, z; int u, v, w;
  while ((nread = getline(&line, &len, in)) != -1) {
    if (line[0] == 'v') {
      sscanf(line, "%s %f %f %f", fn, &x, &y, &z);
      vertices[v_index].x = x; vertices[v_index].y = y; vertices[v_index].z = z;
      v_index++;
    } else if (line[0] == 'f') {
      sscanf(line, "%s %d %d %d", fn, &u, &v, &w);
      triangles[f_index].v0 = u - 1; triangles[f_index].v1 = v - 1; triangles[f_index].v2 = w - 1;
      f_index++;
    }
  }

  rtcCommitGeometry(geom);
  rtcAttachGeometryByID(*scene, geom, id);
  fclose(in);
}

float RTTriangleMesh::reflect(float theta_i, float phi_i, float theta_o, float phi_o) {
	return material(theta_i, phi_i, theta_o, phi_o);
}

float RTTriangleMesh::emit(int id, float u, float v) {
	return emission(id, u, v);
}

RTSkyBox::RTSkyBox(RTScene * s, float l, vec3f *p) {
	device = &(s->device);
	scene = &(s->scene);
	geom = rtcNewGeometry(*device, RTC_GEOMETRY_TYPE_TRIANGLE);
	len = l; pos = p;
	id = s->record_obj(this);
}

void RTSkyBox::loadFile(char * fname, int w, int h) {
	texw = w; texh = h;

	red = (unsigned char*)malloc(w*h);
	green = (unsigned char*)malloc(w*h);
	blue = (unsigned char*)malloc(w*h);
	
	read_bmp(red, green, blue, w, h, fname);

  Vertex * vertices  = (Vertex*) rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), 8);
  Triangle * triangles = (Triangle*) rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), 12);
	
	float l2 = len / 2.f;

	vertices[0].x = -l2 + pos->x; vertices[0].y = -l2 + pos->y; vertices[0].z = -l2 + pos->z;
  vertices[1].x = -l2 + pos->x; vertices[1].y = -l2 + pos->y; vertices[1].z = l2 + pos->z;
  vertices[2].x = -l2 + pos->x; vertices[2].y = l2 + pos->y; vertices[2].z = -l2 + pos->z;
  vertices[3].x = -l2 + pos->x; vertices[3].y = l2 + pos->y; vertices[3].z = l2 + pos->z;
  vertices[4].x = l2 + pos->x; vertices[4].y = -l2 + pos->y; vertices[4].z = -l2 + pos->z;
  vertices[5].x = l2 + pos->x; vertices[5].y = -l2 + pos->y; vertices[5].z = l2 + pos->z;
  vertices[6].x = l2 + pos->x; vertices[6].y = l2 + pos->y; vertices[6].z = -l2 + pos->z;
  vertices[7].x = l2 + pos->x; vertices[7].y = l2 + pos->y; vertices[7].z = l2 + pos->z;

	int tri = 0;

	// left side
	triangles[tri].v0 = 2; triangles[tri].v1 = 1; triangles[tri].v2 = 0; tri++;
  triangles[tri].v0 = 2; triangles[tri].v1 = 3; triangles[tri].v2 = 1; tri++;

  // right side
  triangles[tri].v0 = 5; triangles[tri].v1 = 6; triangles[tri].v2 = 4; tri++;
  triangles[tri].v0 = 7; triangles[tri].v1 = 6; triangles[tri].v2 = 5; tri++;

  // bottom side
  triangles[tri].v0 = 1; triangles[tri].v1 = 4; triangles[tri].v2 = 0; tri++;
  triangles[tri].v0 = 5; triangles[tri].v1 = 4; triangles[tri].v2 = 1; tri++;

  // top side
  triangles[tri].v0 = 6; triangles[tri].v1 = 3; triangles[tri].v2 = 2; tri++;
  triangles[tri].v0 = 6; triangles[tri].v1 = 7; triangles[tri].v2 = 3; tri++;

  // front side
  triangles[tri].v0 = 4; triangles[tri].v1 = 2; triangles[tri].v2 = 0; tri++;
  triangles[tri].v0 = 4; triangles[tri].v1 = 6; triangles[tri].v2 = 2; tri++;

  // back side
  triangles[tri].v0 = 3; triangles[tri].v1 = 5; triangles[tri].v2 = 1; tri++;
  triangles[tri].v0 = 7; triangles[tri].v1 = 5; triangles[tri].v2 = 3; tri++;

	rtcCommitGeometry(geom);
	rtcAttachGeometryByID(*scene, geom, id);
}

float RTSkyBox::reflect(float theta_i, float phi_i, float theta_o, float phi_o) {
	return 0.0f;
}

float RTSkyBox::emit(int id, float u, float v) {
	int p = (int)(u*texw);
	int q = (int)(v*texh);
	float raw = 0.2 * red[q * texw + p] + 0.7 * green[q * texw + p] + 0.1 * blue[q * texw + p];
	return 0.5f * raw / 255.f;
}
