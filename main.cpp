#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "geom.h"
#include "bmp.h"

//just a cube for now
MeshSize * load_mesh(char * fname, RTCGeometry * mesh) {
	MeshSize * size = new MeshSize;
	size->num_vertices = 8;
	size->num_triangles = 12;

	Vertex * vertices  = (Vertex*) rtcSetNewGeometryBuffer(*mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), size->num_vertices);
	Triangle * triangles = (Triangle*) rtcSetNewGeometryBuffer(*mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), size->num_triangles);

	vertices[0].x = -1; vertices[0].y = -1; vertices[0].z = -1;
  vertices[1].x = -1; vertices[1].y = -1; vertices[1].z = +1;
  vertices[2].x = -1; vertices[2].y = +1; vertices[2].z = -1;
  vertices[3].x = -1; vertices[3].y = +1; vertices[3].z = +1;
  vertices[4].x = +1; vertices[4].y = -1; vertices[4].z = -1;
  vertices[5].x = +1; vertices[5].y = -1; vertices[5].z = +1;
  vertices[6].x = +1; vertices[6].y = +1; vertices[6].z = -1;
  vertices[7].x = +1; vertices[7].y = +1; vertices[7].z = +1;

	int tri = 0;
	// left side
  triangles[tri].v0 = 0; triangles[tri].v1 = 1; triangles[tri].v2 = 2; tri++;
  triangles[tri].v0 = 1; triangles[tri].v1 = 3; triangles[tri].v2 = 2; tri++;

  // right side
  triangles[tri].v0 = 4; triangles[tri].v1 = 6; triangles[tri].v2 = 5; tri++;
  triangles[tri].v0 = 5; triangles[tri].v1 = 6; triangles[tri].v2 = 7; tri++;

  // bottom side
  triangles[tri].v0 = 0; triangles[tri].v1 = 4; triangles[tri].v2 = 1; tri++;
  triangles[tri].v0 = 1; triangles[tri].v1 = 4; triangles[tri].v2 = 5; tri++;

  // top side
  triangles[tri].v0 = 2; triangles[tri].v1 = 3; triangles[tri].v2 = 6; tri++;
  triangles[tri].v0 = 3; triangles[tri].v1 = 7; triangles[tri].v2 = 6; tri++;

  // front side
  triangles[tri].v0 = 0; triangles[tri].v1 = 2; triangles[tri].v2 = 4; tri++;
  triangles[tri].v0 = 2; triangles[tri].v1 = 6; triangles[tri].v2 = 4; tri++;

  // back side
  triangles[tri].v0 = 1; triangles[tri].v1 = 5; triangles[tri].v2 = 3; tri++;
  triangles[tri].v0 = 3; triangles[tri].v1 = 5; triangles[tri].v2 = 7; tri++;

	return size;
}

int main(int argc, char** argv) {
	//create a new device
	RTCDevice device = rtcNewDevice("");
	if (rtcGetDeviceError(device) != RTC_ERROR_NONE) {
		printf("An error occured while creating device\n");
	}

	ssize_t version = rtcGetDeviceProperty(device, RTC_DEVICE_PROPERTY_VERSION);
	printf("Embree version: %ld\n", version);	
	
	RTCGeometry mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

	//load a mesh, and commit it
	MeshSize * size = load_mesh(NULL, &mesh);
	Vertex * vertices = (Vertex*) rtcGetGeometryBufferData(mesh, RTC_BUFFER_TYPE_VERTEX, 0);
	rtcCommitGeometry(mesh);

	//create a new scene
	RTCScene scene = rtcNewScene(device);

	//load the mesh into the scene and commit it
	int mesh_id = rtcAttachGeometry(scene, mesh);
	rtcCommitScene(scene);

	//default ray context
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);

	RTCRayHit rh;

	rh.ray.org_x = 3.0f; rh.ray.org_y = 0.f; rh.ray.org_z = 0.f; rh.ray.tnear = 0.f;
	rh.ray.time = 0.f;

	unsigned char *b_red, *b_green, *b_blue;
	b_red = (unsigned char*)malloc(1000*1000);
	b_green = (unsigned char*)malloc(1000*1000);
	b_blue = (unsigned char*)malloc(1000*1000);

	int ray_id = 0;
	for (int u = 0; u < 1000; u++) {
		for (int v = 0; v < 1000; v++) {
			float x = 2.0f;
			float y = ((float)u - 500.f) / 200.f;
			float z = ((float)v - 500.f) / 200.f;

			rh.ray.tfar = 9999.f; rh.ray.id = ray_id; ray_id++;
			rh.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
			rh.hit.geomID = RTC_INVALID_GEOMETRY_ID;

			rh.ray.dir_x = x - rh.ray.org_x;
			rh.ray.dir_y = y - rh.ray.org_y;
			rh.ray.dir_z = z - rh.ray.org_z;

			rtcIntersect1(scene, &context, &rh);

			if (rh.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
				b_red[u * 1000 + v] = 0xff;
				b_green[u * 1000 + v] = 0xff;
				b_blue[u * 1000 + v] = 0xff;
			} else {
				b_red[u * 1000 + v] = 0;
				b_green[u * 1000 + v] = 0;
				b_blue[u * 1000 + v] = 0;
			}
		}
	}

	write_bmp(b_red, b_green, b_blue, 1000, 1000, (char*)"out.bmp");

	rtcReleaseScene(scene);
	rtcReleaseGeometry(mesh);
	rtcReleaseDevice(device);
}
