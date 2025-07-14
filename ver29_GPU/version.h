#pragma once
#define VER 2
#define FRUSTUM_CULLING 1


// draw BVH AABB box
#define SHOW_BOX 0

// SSBO 0: send visible model matrix to shader with vbo:  NOT WORKING NOW
// SSBO 1: send all model matrix to shader as ssbo, send visible indices matrix to shader with vbo :  NO LONGER WORKING NOW
// SSBO 2: gpu frustum culling, send all model matreix as ssbo, calculate visible indices in compute shader 
//		   and store it in ssbo.
#define SSBO 2

// NUM == 50    try V2F1 and V1F1		and 30	
#define NUM 40
#define FAR 500.f
