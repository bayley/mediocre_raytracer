#include <embree3/rtcore.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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

int main(int argc, char** argv) {
	//print a message and quit if no args
	if (argc < 2) {
		printf("Please give a file name\n");
		return -1;
	}

	//random seed
	srand(time(0));

	//create a new scene
	RTScene scene;

	//load a skybox
	RTSkyBox * sky = new RTSkyBox(&scene, 30.f, new vec3f(0.f, 0.f, 0.f));
	sky->loadFile((char*)"bliss_bad.bmp", (char*)"pave.bmp", 1440, 1440);

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

	//test point light
	vec3f * lamp = new vec3f(4.f, 6.f, 8.f);
	float lamp_p = 0.3f;

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

			//hit point, normal vector, local axes
			vec3f * hit_p = scene.hitP();
			vec3f * hit_n = scene.hitN();
			vec3f * hit_u = local_u(hit_n);
			vec3f * hit_v = hit_n->cross(hit_u);

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
			float cos_l = hit_n->dot(to_lamp);
			if (cos_l < 0.f) cos_l = 0.f; //backside
			if (isinf(scene.rh.ray.tfar)) cos_l = 0.f;

			//store last hit
			float last_id = scene.rh.hit.geomID;
			float last_prim = scene.rh.hit.primID;

			//emission + direct
			vec3f * last_color = scene.color(last_id, last_prim, scene.rh.hit.u, scene.rh.hit.v);
			float last_refl = scene.reflect(last_id, 0.f, 0.f, 0.f, 0.f);
			//float d_illum = scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v) +
			//							  cos_l * last_color * last_refl;
			vec3f * d_illum = add(scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v), mul(last_color, last_refl * cos_l * lamp_p));

			//do GI
			float hit_theta, hit_phi;
			vec3f * g_illum = new vec3f(0.f, 0.f, 0.f);	
			int n_samples = 1000;

			for (int sample = 0; sample < n_samples; sample++) {
				hit_theta = rangle(); hit_phi = rangle();
				float c_u = sinf(hit_theta) * cosf(hit_phi);
				float c_v = sinf(hit_theta) * sinf(hit_phi);
				float c_n = cosf(hit_theta);
				vec3f * out_dir = add(add(mul(hit_u, c_u), mul(hit_v, c_v)), mul(hit_n, c_n));
				out_dir->normalize();
				float cos_g = hit_n->dot(out_dir);

				//one GI bounce
				scene.resetRH();
				setRayOrg(&scene.rh, hit_p);
				setRayDir(&scene.rh, out_dir);

				rtcIntersect1(scene.scene, &scene.context, &scene.rh);

				if (scene.rh.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
					continue;
				}
				
				//cheating - we ignore direct illumination
				//g_illum += cos_g * scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v) * 
				//									 last_color * scene.reflect(last_id, 0.f, 0.f, 0.f, 0.f);
				g_illum = add(g_illum, mul(mul(last_color, cos_g * scene.reflect(last_id, 0.f, 0.f, 0.f, 0.f)),
																	 scene.emit(scene.rh.hit.geomID, scene.rh.hit.primID, scene.rh.hit.u, scene.rh.hit.v)));
			}
			//g_illum /= (float)(n_samples);
			g_illum = mul(g_illum, 1.f / (float)n_samples);

			//float illum = d_illum + g_illum;
			//if (illum > 1.f) illum = 1.f;
			vec3f * illum = add(d_illum, g_illum);
			if (illum->x > 1.f) illum->x = 1.f;
			if (illum->y > 1.f) illum->y = 1.f;
			if (illum->z > 1.f) illum->z = 1.f;
			unsigned char c_r, c_g, c_b;
			c_r = (unsigned char)(illum->x * 255.f);
			c_g = (unsigned char)(illum->y * 255.f);
			c_b = (unsigned char)(illum->z * 255.f);
			
			//write output color
			output.set_px(u, v, c_r, c_g, c_b);
		}
	}

	output.write((char*)"out.bmp");

	scene.cleanup();
}
