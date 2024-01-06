#ifndef TRANSFORMS_H
#define TRANSFORMS_H

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "primitives.h"
#include "camera.h"

TriangleMesh* transform_and_cull(TriangleMesh* pmesh, Camera* pcam, bool do_bface_cull);

TriangleMesh* add_triangle(TriangleMesh* pmesh, Triangle tri);
TriangleMesh* merge_tri_meshes(TriangleMesh* pmesh1, TriangleMesh* pmesh2);
void flip_triangle(Triangle* ptri);
void flip_mesh(TriangleMesh* pmesh);
TriangleMesh* extrude(Polygon* ppoly, float height);
void calculate_rotation_matrix(float* matrix, Point3D rotation);
void calculate_translation_matrix(float* matrix, Point3D translation);
void translate_mesh(TriangleMesh* pmesh, Point3D translation);
void rotate_mesh(TriangleMesh* pmesh, Point3D rotation);
void reflect_mesh(TriangleMesh* pmesh, Point3D normal);
void transform_mesh(float* matrix, TriangleMesh* pmesh);
TriangleMesh* copy_mesh(TriangleMesh* pmesh);

#endif
