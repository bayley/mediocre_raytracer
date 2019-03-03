#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "geom.h"
#include "bmp.h"

//load an OBJ
MeshSize * load_mesh(char * fname, RTCGeometry * mesh) {
	FILE * in = fopen(fname, "r");
	MeshSize * size = new MeshSize;
	size->num_vertices = 0;
	size->num_triangles = 0;

	char * line = NULL; size_t len = 0; ssize_t nread;	
	while ((nread = getline(&line, &len, in)) != -1) {
		if (line[0] == 'v') size->num_vertices++;
		if (line[0] == 'f') size->num_triangles++;
	}
	free(line);

	Vertex * vertices  = (Vertex*) rtcSetNewGeometryBuffer(*mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), size->num_vertices);
	Triangle * triangles = (Triangle*) rtcSetNewGeometryBuffer(*mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), size->num_triangles);

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
	return size;
}

inline void resetRayHit(RTCRayHit * rh) {
	rh->ray.tnear = 0.01f;
	rh->ray.tfar = FLT_MAX; rh->ray.id = 0;
	rh->hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
	rh->hit.geomID = RTC_INVALID_GEOMETRY_ID;
}

inline void setRayDir(RTCRayHit * rh, vec3f * dir) {
	rh->ray.dir_x = dir->x;
	rh->ray.dir_y = dir->y;
	rh->ray.dir_z = dir->z;
}

inline vec3f * eval_ray(RTCRay ray, float t) {
	return new vec3f(ray.org_x + t * ray.dir_x, ray.org_y + t * ray.dir_y, ray.org_z + t * ray.dir_z);
}

inline void set_px(unsigned char * red, unsigned char * green, unsigned char * blue,
									 unsigned char r, unsigned char g, unsigned char b, int width, int height, int u, int v) {
	red[u * height + v] = r;
	green[u * height + v] = g;
	blue[u * height + v] = b;
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
	if (argc < 2) {
		printf("Please give a file name\n");
		return -1;
	}
	MeshSize * size = load_mesh(argv[1], &mesh);
	printf("Loaded %s with %d vertices and %d faces\n", argv[1], size->num_vertices, size->num_triangles);
	rtcCommitGeometry(mesh);

	//create a new scene
	RTCScene scene = rtcNewScene(device);

	//load the mesh into the scene and commit it
	int mesh_id = rtcAttachGeometry(scene, mesh);
	rtcCommitScene(scene);

	//default ray context
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);

	//hit object
	RTCRayHit rh;
	
	//time not used, for now
	rh.ray.time = 0.f;

	//output buffers
	int img_width = 1000, img_height = 1000;
	unsigned char *b_red, *b_green, *b_blue;
	b_red = (unsigned char*)malloc(img_width * img_height);
	b_green = (unsigned char*)malloc(img_width * img_height);
	b_blue = (unsigned char*)malloc(img_width * img_height);

	//static camera at <6, 6, 6> looking at the origin
	vec3f * eye = new vec3f(6.f, 6.f, 6.f);
	vec3f * cam_dir = new vec3f(-1.f, -1.f, -1.f);
	cam_dir->normalize();

	Camera * cam = new Camera(eye->x, eye->y, eye->z,
														cam_dir->x, cam_dir->y, cam_dir->z,
														1.0f, img_width, img_height);

	//initialize ray origin to camera origin	
	rh.ray.org_x = eye->x; rh.ray.org_y = eye->y; rh.ray.org_z = eye->z;

	//test point light
	vec3f * lamp = new vec3f(4.f, 6.f, 8.f);

	for (int u = 0; u < img_width; u++) {
		for (int v = 0; v < img_height; v++) {
			resetRayHit(&rh);

			setRayDir(&rh, cam->lookat(u, v));

			rtcIntersect1(scene, &context, &rh);

			//fill with background color
			if (rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
				set_px(b_red, b_green, b_blue, 0, 0, 0, img_width, img_height, u, v);
				continue;
			}

			//hit point, normal vector
			vec3f * hit_p = eval_ray(rh.ray, rh.ray.tfar);
			vec3f * hit_n = new vec3f(rh.hit.Ng_x, rh.hit.Ng_y, rh.hit.Ng_z);
			hit_n->normalize();

			//simple cosine shading
			vec3f * to_lamp = sub(lamp, hit_p);
			to_lamp->normalize();
			float lambert = hit_n->dot(to_lamp);
			if (lambert < 0.f) lambert = 0.f; //backside
			unsigned char shade = (unsigned char)(lambert * 255.f);
			
			//write output color
			set_px(b_red, b_green, b_blue, shade, shade, shade, img_width, img_height, u, v);
		}
	}

	write_bmp(b_red, b_green, b_blue, img_width, img_height, (char*)"out.bmp");

	rtcReleaseScene(scene);
	rtcReleaseGeometry(mesh);
	rtcReleaseDevice(device);
}
