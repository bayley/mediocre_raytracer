#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

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
	return obj_count++;
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
	for(int i = 0; i < obj_count; i++) {
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

RTTriangleMesh::RTTriangleMesh(RTScene * s) {
	device = &(s->device);
	scene = &(s->scene);
	geom = rtcNewGeometry(*device, RTC_GEOMETRY_TYPE_TRIANGLE);
	num_vertices = 0;
	num_triangles = 0;
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
