#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "utils.h"
#include "primitives.h"
#include "camera.h"
#include "vect.h"

int comp_tri_z(const void* ptri_a, const void* ptri_b);
bool facing_camera(Triangle tri);


TriangleMesh* add_triangle(TriangleMesh* pmesh, Triangle tri){
    //Allocating more memory to add the new triangle
    TriangleMesh* pres = realloc(pmesh, sizeof(TriangleMesh) + (pmesh->size + 1) * sizeof(Triangle));
    check_allocation(pres, "Couldn't allocate memory to add a new triangle\n");

    pres->triangles[pres->size] = tri;
    pres->size += 1;
    return pres;
}

TriangleMesh* merge_tri_meshes(TriangleMesh* pmesh1, TriangleMesh* pmesh2){
    for (int i = 0; i < pmesh2->size; i++){
        pmesh1 = add_triangle(pmesh1, pmesh2->triangles[i]);
    }
    free(pmesh2);
    return pmesh1;
}

void flip_triangle(Triangle* ptri){
    Point3D tmp = ptri->a;
    bool tmp_vis;
    ptri->a = ptri->b;
    ptri->b = tmp;
    tmp_vis = ptri->visible[1];
    ptri->visible[1] = ptri->visible[2];
    ptri->visible[2] = tmp_vis;
}

void flip_mesh(TriangleMesh* pmesh){
    for (int i = 0; i < pmesh->size; i++){
        flip_triangle(&(pmesh->triangles[i]));
    }
}

TriangleMesh* extrude(Polygon* ppoly, float height){
    // Top
    TriangleMesh* poly1 = triangulate(ppoly);
    for (int i = 0; i < poly1->size; i++){
        poly1->triangles[i].a.z += height;
        poly1->triangles[i].b.z += height;
        poly1->triangles[i].c.z += height;
    }
    // Bottom
    TriangleMesh* poly2 = triangulate(ppoly);
    flip_mesh(poly2);

    // Sides
    TriangleMesh* psides = malloc(sizeof(TriangleMesh) + (2 * ppoly->size) * sizeof(Triangle));
    psides->size = 0;
    PolygonVertex* pcurr_vertex = ppoly->head;
    // Two triangles per sides
    Triangle tri1, tri2;
    do {
        // Triangle 1
        tri1.a.x = pcurr_vertex->next->coordinates.x;
        tri1.a.y = pcurr_vertex->next->coordinates.y;
        tri1.a.z = 0;
        tri1.b.x = pcurr_vertex->coordinates.x;
        tri1.b.y = pcurr_vertex->coordinates.y;
        tri1.b.z = 0;
        tri1.c.x = pcurr_vertex->coordinates.x;
        tri1.c.y = pcurr_vertex->coordinates.y;
        tri1.c.z = height;
        tri1.visible[0] = true;
        tri1.visible[1] = true;
        tri1.visible[2] = false;
        psides = add_triangle(psides, tri1);

        // Triangle 2
        tri2.a.x = pcurr_vertex->next->coordinates.x;
        tri2.a.y = pcurr_vertex->next->coordinates.y;
        tri2.a.z = 0;
        tri2.b.x = pcurr_vertex->coordinates.x;
        tri2.b.y = pcurr_vertex->coordinates.y;
        tri2.b.z = height;
        tri2.c.x = pcurr_vertex->next->coordinates.x;
        tri2.c.y = pcurr_vertex->next->coordinates.y;
        tri2.c.z = height;
        tri2.visible[0] = false;
        tri2.visible[1] = true;
        tri2.visible[2] = true;
        psides = add_triangle(psides, tri2);

        pcurr_vertex = pcurr_vertex->next;
    } while (pcurr_vertex != ppoly->head);

    // Merging
    poly1 = merge_tri_meshes(poly1, poly2);
    poly1 = merge_tri_meshes(poly1, psides);
    return poly1;
}

// Homogeneous coordinates transforms
Point3D transform_point(float* matrix, Point3D point){
    Point3D res;
    res.x = point.x * matrix[0] + point.y * matrix[1] + point.z * matrix[2] + matrix[3];
    res.y = point.x * matrix[4] + point.y * matrix[5] + point.z * matrix[6] + matrix[7];
    res.z = point.x * matrix[8] + point.y * matrix[9] + point.z * matrix[10] + matrix[11];
    return res;
}


Triangle transform_triangle(float* matrix, Triangle tri){
    Point3D trans_a, trans_b, trans_c;
    Triangle trans_tri;

    trans_a = transform_point(matrix, tri.a);
    trans_b = transform_point(matrix, tri.b);
    trans_c = transform_point(matrix, tri.c);

    trans_tri.a = trans_a;
    trans_tri.b = trans_b;
    trans_tri.c = trans_c;
    memcpy(trans_tri.visible, tri.visible, 3);

    return trans_tri;
}

void transform_mesh(float* matrix, TriangleMesh* pmesh){
    Triangle curr_tri;
    for (int i = 0; i < pmesh->size; i++){
        curr_tri = transform_triangle(matrix, pmesh->triangles[i]);
        pmesh->triangles[i] = curr_tri;
    }
}


void calculate_rotation_matrix(float* matrix, Point3D rotation){
    float r_x = rotation.x,
          r_y = rotation.y,
          r_z = rotation.z;

    matrix[0] = cosf(r_y) * cosf(r_z);
    matrix[1] = sinf(r_x) * sinf(r_y) * cosf(r_z) - cosf(r_x) * sinf(r_z);
    matrix[2] = cosf(r_x) * sinf(r_y) * cosf(r_z) + sinf(r_x) * sinf(r_z);
    matrix[4] = cosf(r_y) * sinf(r_z);
    matrix[5] = sinf(r_x) * sinf(r_y) * sinf(r_z) + cosf(r_x) * cosf(r_z);
    matrix[6] = cosf(r_x) * sinf(r_y) * sinf(r_z) - sinf(r_x) * cosf(r_z);
    matrix[8] = -sinf(r_y);
    matrix[9] = sinf(r_x) * cosf(r_y);
    matrix[10] = cosf(r_x) * cosf(r_y);

    matrix[3] = matrix[7]
              = matrix[11]
              = matrix[12]
              = matrix[13]
              = matrix[14] = 0;
    matrix[15] = 1;
}

void calculate_translation_matrix(float* matrix, Point3D translation){
    matrix[3] = translation.x;
    matrix[7] = translation.y;
    matrix[11] = translation.z;

    matrix[1] = matrix[2]
              = matrix[4]
              = matrix[6]
              = matrix[8]
              = matrix[9]
              = matrix[12]
              = matrix[13]
              = matrix[14] = 0;

    matrix[0] = matrix[5]
              = matrix[10]
              = matrix[15] = 1;
}

void translate_mesh(TriangleMesh* pmesh, Point3D translation){
    float matrix[16];
    calculate_translation_matrix(matrix, translation);
    transform_mesh(matrix, pmesh);
}

void rotate_mesh(TriangleMesh* pmesh, Point3D rotation){
    float matrix[16];
    calculate_rotation_matrix(matrix, rotation);
    transform_mesh(matrix, pmesh);
}

TriangleMesh* copy_mesh(TriangleMesh* pmesh){
    int size_in_memory = sizeof(TriangleMesh) + sizeof(Triangle) * pmesh->size;
    TriangleMesh* pcopy = (TriangleMesh*) malloc(size_in_memory);
    check_allocation(pcopy, "Couldn't allocate memory to copy the mesh\n");
    memcpy(pcopy, pmesh, size_in_memory);
    return pcopy;
}

void reflect_mesh(TriangleMesh* pmesh, Point3D normal){
    // Normalizing vector
    Point3D unit_norm = normalize(normal);
    float matrix[16] = {
        1 - 2*unit_norm.x*unit_norm.x, -2*unit_norm.x*unit_norm.y, -2*unit_norm.x*unit_norm.z, 0,
        -2*unit_norm.x*unit_norm.y, 1 - 2*unit_norm.y*unit_norm.y, -2*unit_norm.y*unit_norm.z, 0,
        -2*unit_norm.x*unit_norm.z, -2*unit_norm.y*unit_norm.z, 1-2*unit_norm.z*unit_norm.z, 0,
        0, 0, 0, 1,
    };
    transform_mesh(matrix, pmesh);
    flip_mesh(pmesh);
}

bool facing_camera(Triangle tri){
    Point3D vect_1 = pt_diff(tri.b, tri.a),
            vect_2 = pt_diff(tri.a, tri.c);
    Point3D normal = cross_product(vect_1, vect_2);
    Point3D center = pt_mul((float)1/3, pt_add(pt_add(tri.a, tri.b), tri.c));
    return (dot_product(center, normal) >= 0);
}

int comp_tri_z(const void* ptri_a, const void* ptri_b){
    Point3D tri_a_a = ((Triangle*) ptri_a)->a,
            tri_a_b = ((Triangle*) ptri_a)->b,
            tri_a_c = ((Triangle*) ptri_a)->c,
            tri_b_a = ((Triangle*) ptri_b)->a,
            tri_b_b = ((Triangle*) ptri_b)->b,
            tri_b_c = ((Triangle*) ptri_b)->c;

    float min_z_a = fminf(fminf(tri_a_a.z, tri_a_b.z), tri_a_c.z),
          min_z_b = fminf(fminf(tri_b_a.z, tri_b_b.z), tri_b_c.z);

    if (min_z_a < min_z_b)
        return -1;
    else if (min_z_a == min_z_b)
        return 0;
    else
        return 1;
}

TriangleMesh* bface_cull(TriangleMesh* ptri){
    TriangleMesh* pres = new_triangle_mesh(0);
    Triangle curr_tri;

    for (int i = 0; i < ptri->size; i++){
        curr_tri = ptri->triangles[i];
        if (facing_camera(curr_tri)){
            pres = add_triangle(pres, curr_tri);
        }
    }
    return pres;
}

TriangleMesh* frustum_cull(TriangleMesh* ptri, Camera* pcam){
    TriangleMesh* pres = new_triangle_mesh(0);
    Triangle curr_tri;
    Point2D a_proj, b_proj, c_proj;

    for (int i = 0; i < ptri->size; i++){
        curr_tri = ptri->triangles[i];
        // Are all three vertices behind the focal plan ?
        if (curr_tri.a.z < pcam->focal_length &&
            curr_tri.b.z < pcam->focal_length &&
            curr_tri.c.z < pcam->focal_length)
            continue;

        // Projecting the vertices
        a_proj = project_point(curr_tri.a, pcam);
        b_proj = project_point(curr_tri.b, pcam);
        c_proj = project_point(curr_tri.c, pcam);

        // Are all three vertices left of the frustum ?
        if (a_proj.x < -pcam->width/2 &&
            b_proj.x < -pcam->width/2 &&
            c_proj.x < -pcam->width/2) 
            continue;
 
        // Are all three vertices right of the frustum ?
        if (a_proj.x > pcam->width/2 &&
            b_proj.x > pcam->width/2 &&
            c_proj.x > pcam->width/2) 
            continue;
 
        // Are all three vertices above the frustum ?
        if (a_proj.y < -pcam->width/2 &&
            b_proj.y < -pcam->width/2 &&
            c_proj.y < -pcam->width/2) 
            continue;

        // Are all three vertices above the frustum ?
        if (a_proj.y > pcam->width/2 &&
            b_proj.y > pcam->width/2 &&
            c_proj.y > pcam->width/2) 
            continue;

        // The triangle is inside the frustum, add it
        pres = add_triangle(pres, curr_tri);
    }
    return pres;
}

void z_sort_triangles(TriangleMesh* pmesh){
    qsort(pmesh->triangles, pmesh->size, sizeof(Triangle), comp_tri_z);
}
