#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
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

inline vec3f * local_u(vec3f * hit_n) {
	vec3f * hit_u;
	hit_u = new vec3f(-hit_n->y, hit_n->x, 0.f);
	if (hit_u->abs() == 0.f) {
		hit_u = new vec3f(0.f, -hit_n->z, hit_n->y);
	}
	hit_u->normalize();
	return hit_u;
}

inline float rangle() {
	return (float)(M_PI * (double)(rand())/(double)(RAND_MAX));
}

inline vec3f * random_dir(vec3f * n, float backside) {
	vec3f * u = local_u(n);
	vec3f * v = n->cross(u);

	float hit_theta = rangle() - M_PI / 2.f, hit_phi = rangle() * 2.f;

	float c_u = sinf(hit_theta) * cosf(hit_phi);
	float c_v = sinf(hit_theta) * sinf(hit_phi);
	float c_n = cosf(hit_theta) * backside;
	vec3f * out_dir = add(add(mul(u, c_u), mul(v, c_v)), mul(n, c_n));
	out_dir->normalize();

	return out_dir;
}

int main(int argc, char** argv) {
	int n_samples = 16;
	//print a message and quit if no args
	if (argc < 2) {
		printf("Please give a file name\n");
		return -1;
	}
	if (argc > 2) {
		n_samples = atoi(argv[2]);
	}

	//random seed
	srand(time(0));

	//create a new scene
	RTScene scene;

	//load a skybox
	RTSkyBox * sky = new RTSkyBox(&scene, 30.f, new vec3f(0.f, 0.f, 0.f));
	sky->loadFile((char*)"bliss_bad.bmp", (char*)"pave.bmp", (char*)"cloud.bmp", 1440, 1440);

	//load a mesh into the scene
	RTTriangleMesh * teapot = new RTTriangleMesh(&scene, brdf_lambert, emit_black);
	teapot->loadFile(argv[1]);
	printf("Loaded %s with %d vertices and %d faces\n", argv[1], teapot->num_vertices, teapot->num_triangles);

	//commit scene and build BVH
	scene.commit();

	//output file
	BMPC output(1000, 1000);

	//camera location
	scene.move(5.f, 8.f, 0.f);
	scene.point(-1.f, -1.5f, 0.f);
	scene.zoom(1.2f);
	scene.resize(output.width, output.height);

	vec3f *last_dir, *last_color;
	vec3f * emission;
	int last_id, last_prim;
	float refl;

	for (int u = 0; u < output.width; u++) {
		for (int v = 0; v < output.height; v++) {
			scene.resetRH();

			setRayOrg(&scene.rh, scene.cam->eye);
			setRayDir(&scene.rh, scene.cam->lookat(u, v));

			rtcIntersect1(scene.scene, &scene.context, &scene.rh);

			//fill with background color
			if (scene.rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
				output.set_px(u, v, 0.f, 0.f, 0.f);	
				continue;
			}

			//hit point, normal vector
			vec3f * hit_p = scene.hitP();
			vec3f * hit_n = scene.hitN();

			//store last hit
			last_id = scene.rh.hit.geomID;
			last_prim = scene.rh.hit.primID;
			last_dir = new vec3f(scene.rh.ray.dir_x, scene.rh.ray.dir_y, scene.rh.ray.dir_z);
			last_color = scene.color(last_id, last_prim, scene.rh.hit.u, scene.rh.hit.v);

			//direct (just emission for now)
			vec3f * d_illum = scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v);

			//do GI
			float hit_theta, hit_phi;
			vec3f * g_illum = new vec3f(0.f, 0.f, 0.f);	

			for (int sample = 0; sample < n_samples; sample++) {
				float backside = last_dir->dot(hit_n) > 0.f ? -1.f : 1.f;
				vec3f * out_dir = random_dir(hit_n, backside);
				float cos_g = backside * hit_n->dot(out_dir);
				refl = scene.reflect(last_id, last_prim, 0.f, 0.f, 0.f, 0.f); //TODO: use real angles!

				if (refl == 0.f) continue; //trick to speed up the skybox

				//one GI bounce
				scene.resetRH();
				setRayOrg(&scene.rh, hit_p);
				setRayDir(&scene.rh, out_dir);

				rtcIntersect1(scene.scene, &scene.context, &scene.rh);

				if (scene.rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
					continue;
				}
				
				//add the emission from the new hit
				emission = scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v);
				g_illum = add(g_illum, mul(mul(last_color, cos_g * refl), emission));
			}
			g_illum = mul(g_illum, 1.f / (float)n_samples);

			//direct + global
			vec3f * illum = add(d_illum, g_illum);
			
			//write output color
			output.set_px(u, v, illum->x, illum->y, illum->z);
		}
	}

	output.write((char*)"out.bmp");

	scene.cleanup();
}
