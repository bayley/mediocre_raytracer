#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "geom.h"
#include "bmpc.h"
#include "brdf.h"
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
	//print a message and quit if no args
	if (argc < 2) {
		printf("Please give a file name\n");
		return -1;
	}

	//create a new scene
	RTScene scene;

	//load a skybox
	RTSkyBox * sky = new RTSkyBox(&scene, 40.f, new vec3f(0.f, 0.f, 0.f));
	sky->loadFile((char*)"");

	//load a mesh into the scene
	RTTriangleMesh * teapot = new RTTriangleMesh(&scene, brdf_lambert, emit_black);
	teapot->loadFile(argv[1]);
	printf("Loaded %s with %d vertices and %d faces\n", argv[1], teapot->num_vertices, teapot->num_triangles);

	//commit scene and build BVH
	scene.commit();

	//output file
	BMPC output(1000, 1000);

	//camera location
	scene.move(6.f, 6.f, 6.f);
	scene.point(-1.f, -1.f, -1.f);
	scene.zoom(1.f);
	scene.resize(output.width, output.height);

	//test point light
	vec3f * lamp = new vec3f(4.f, 6.f, 8.f);

	for (int u = 0; u < output.width; u++) {
		for (int v = 0; v < output.height; v++) {
			scene.resetRH();

			setRayOrg(&scene.rh, scene.cam->eye);
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

			//ray to light, don't normalize yet!
			vec3f * to_lamp = sub(lamp, hit_p);

			//check occlusion
			scene.resetR();
			scene.rh.ray.tfar = 1.f; //only check occlusion between light and hit
			setRayOrg(&scene.rh, hit_p);
			setRayDir(&scene.rh, to_lamp);
			rtcOccluded1(scene.scene, &scene.context, &scene.rh.ray);

			//cosine term
			to_lamp->normalize();
			float cos_n = hit_n->dot(to_lamp);
			if (cos_n < 0.f) cos_n = 0.f; //backside
			if (isinf(scene.rh.ray.tfar)) cos_n = 0.f;

			//materials are stored in the scene
			//works OK for one-mesh one-material, not so much for one-face one-material
			float shade = scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v) +
										cos_n * scene.brdf(scene.rh.hit.geomID, 0.f, 0.f, 0.f, 0.f);

			unsigned char color = (unsigned char)(shade * 255.f);
			
			//write output color
			output.set_px(u, v, color, color, color);
		}
	}

	output.write((char*)"out.bmp");

	scene.cleanup();
}
