#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "geom.h"
#include "bmpc.h"
#include "RTObject.h"

inline void setRayDir(RTCRayHit * rh, vec3f * dir) {
	rh->ray.dir_x = dir->x;
	rh->ray.dir_y = dir->y;
	rh->ray.dir_z = dir->z;
}

inline void setRayOrg(RTCRayHit * rh, vec3f * org) {
	rh->ray.org_x = org->x;
	rh->ray.org_y = org->y;
	rh->ray.org_z = org->z;
}

int main(int argc, char** argv) {
	//create a new scene
	RTScene scene;

	//load a mesh into the scene
	if (argc < 2) {
		printf("Please give a file name\n");
		return -1;
	}
	RTTriangleMesh * teapot_m = new RTTriangleMesh(&scene);
	teapot_m->loadFile(argv[1]);
	printf("Loaded %s with %d vertices and %d faces\n", argv[1], teapot_m->num_vertices, teapot_m->num_triangles);

	//commit scene and build BVH
	scene.commit();

	//output file
	BMPC output(1000, 1000);

	scene.move(6.f, 6.f, 6.f);
	scene.point(-1.f, -1.f, -1.f);
	scene.zoom(1.f);
	scene.resize(output.width, output.height);

	//initialize ray origin to camera origin	
	setRayOrg(&(scene.rh), scene.cam->eye);

	//test point light
	vec3f * lamp = new vec3f(4.f, 6.f, 8.f);

	for (int u = 0; u < output.width; u++) {
		for (int v = 0; v < output.height; v++) {
			scene.resetRH();

			setRayDir(&scene.rh, scene.cam->lookat(u, v));

			rtcIntersect1(scene.scene, &scene.context, &scene.rh);

			//fill with background color
			if (scene.rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
				output.set_px(u, v, 0, 0, 0);	
				continue;
			}

			//hit point, normal vector
			vec3f * hit_p = scene.hitP();
			vec3f * hit_n = scene.hitN();

			//simple cosine shading
			vec3f * to_lamp = sub(lamp, hit_p);
			to_lamp->normalize();
			float lambert = hit_n->dot(to_lamp);
			if (lambert < 0.f) lambert = 0.f; //backside
			unsigned char shade = (unsigned char)(lambert * 255.f);
			
			//write output color
			output.set_px(u, v, shade, shade, shade);
		}
	}

	output.write((char*)"out.bmp");

	scene.cleanup();
}
