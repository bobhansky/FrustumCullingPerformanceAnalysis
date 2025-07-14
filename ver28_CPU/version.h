#pragma once
#define VER 4
#define FRUSTUM_CULLING 1
#define SHOW_BOX 0
#define SSBO 1

// NUM == 50    try V2F1 and V1F1		and 30	
#define NUM 40
#define FAR 500.f


// VER:		        	1				        	        2                                       3
// F_C:  
//   0           no instance draw + no F-Cull           instance draw + no F-Cull
//                                                                                                Naive instance drawing
//   1           non instance draw + F-Cull             instance draw + F-Cull

// if using v2f2, set ssbo 0

// VER == 4  SSBO == 1:    SSBO stores all the model matrices,
//		each frame, we update the visible indices to shader. 

/*
try   VER2F1  and    VER2F0

where    cpu usage    VER2F1 > VER2F0   (update visable objects)

in debug mode,    FPS     24~ <  31~
in release mode           70+   > 31~

see where the bottleneck comes from?


#################################
6/25/2025
implement SSBO  VER == 4  SSBO == 1
using SSBO,
try debug/release mode,
intel iris xe,  i7 12700h

V2F1 VS V4, NUM 30,  and see diff at drawing 18000 objects~
fps 12~ vs 18~

V2F1 VS V4, NUM 10,  and see diff at drawing 1001 objects~
fps 209~ vs 290~



#################################
7/7/2025

implement gpu frustum culling with still direct draw call

CASE 1
	if bottleneck is at gpu:  RELEASE mode,			FAR plane = 300.f, NUM = 100,

	[VER == 4  SSBO == 1], where we use SSBO to store all the model matrices,
	and update the visible indices to shader each frame (n * 4 bytes of communication from cpu to gpu)
	draw size: 10267   fps 274

	>

	[project_29    VER == 2  SSBO == 2]
	draw size: 10013  fps 204
	

	and												FAR plane = 500.f, NUM = 100,	

	[VER == 4  SSBO == 1],
	draw size: 50545   fps 71

	>

	[project_29    VER == 2  SSBO == 2]
	draw size: 50319   fps 64

CASE 2 
	if bottleneck is at gpu: DEBUG mode,			FAR plane = 300.f, NUM = 40,

	[VER == 4  SSBO == 1]
	draw size: 10165   fps 91


	<

	[project_29    VER == 2  SSBO == 2]
	draw size: 10391   fps 230


	and												FAR plane = 500.f, NUM = 40,

	[VER == 4  SSBO == 1]							
	 draw size: 39076    fps 27

	 <

	[project_29    VER == 2  SSBO == 2]				
	draw size: 39912   fps 80


CASE 3 
	DEBUG mode,										FAR plane = 500.f, NUM = 100,
	 
	 [VER == 4  SSBO == 1]
	 draw size: 51659   fps = 20

	 <<<<<

	 [project_29    VER == 2  SSBO == 2]
	 draw size: 50693   fps = 64~, the same as in release mode when drawing 50k objects

*/

