# FrustumCullingPerformanceAnalysis

### Features of this project
- Bounding Volume Hierarchy
- CPU frustum culling
- GPU frustum culling
- Compute shader
- Instance Drawing

**The Main purpose is to compare the peformance boost with some features, and also analyze the performnance bottleneck.**

## Frustum Culling:
![Demo](https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/fc_show.gif)


## 6 Versions:

 -  Ver1: **No** instance drawing (draw call for every identical object) + **No** Frustum Culling. 
 -  Ver2: **No** instance drawing + **With** CPU Frustum Culling
 -  Ver3: **With** Instance drawing + **No** Frustum Culling
 -  Ver4: **With** Instance drawing + **With** CPU Frustum Culling. Update and send the ModelMatrix(4x16 bytes * N instance) of visible objects to shader by VBO every frame
 -  Ver5: **With** Instance drawing + **With** CPU Frustum Culling. Send all model matrix of an object to shader in SSBO once, and then update and send the indices(4 bytes * N instance) of visible objects to shader every frame
 -  Ver6: **With** Instance drawing + **With** GPU Frustum Culling.

# Note
    1. BVH only contain 1 object on leaf node. It is partitioned by longeset extention and by midpoint.
    2. CPU Frustum Culling test visibility by traversing through BVH and test against bounding box of the node that's been traversing.
    3. GPU Frustum Culling test visibility by sending the bounding box of each objects to compute shader to tell.  (brute force)
    4. Applications is ran under release mode, unless there is specific setting to debug mode.


# Test Environment
   Test scene description: The cup objects (666 trirangles) forming an 3d matrix, with dimension NUM = 40 and in total 64001 cups.
   Each setting is at the same position, looking at the same angle, rendering the same content to the frame.
   Test is performed on Windows 11, i7 12700h, Nvidia RTX3060 laptop

# Result

## Ver1: **No** instance drawing (draw call for every identical object) + **No** Frustum Culling. 
   <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v1f0/frame.png" width=900 height=506/>
   <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v1f0/renderdoc.png" width=900 height=506/>

   Naive drwaing called 64001 (instances) * 2 (meshes) times of draw calls, resulting in an unsuprisingly low **7 fps**

-------------------------------------------------------------------------------------
## Ver2: No instance drawing + With CPU Frustum Culling
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v1f1/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v1f1/renderdoc.png" width=900 height=506/>

  Naive drwaing + CPU frustum culling only drew 29450 objects. in total of 29450 * 2 times of draw calls, resulting a doubled **15 fps** compared to Ver1
  
-----------------------------------------------------------------------------------
## Ver3: **With** Instance drawing + **No** Frustum Culling
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f0/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f0/renderdoc.png" width=900 height=506/>
  
  Instance Drwaing + No Frustum Culling reduces the draw call times to 2 but rendered all 64001 objects. FPS increased to **68 fps**

-----------------------------------------------------------------------------------

## Ver4: **With** Instance drawing + **With** CPU Frustum Culling. Update and send the ModelMatrix(4x16 bytes * N instance) of visible objects to shader by VBO every frame
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f1/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f1/renderdoc.png" width=900 height=506/>

  Instance Drawing + CPU Side Frustum Culling:
  In Release Mode, where CPU performance is not capped, the bottleneck is at gpu. Fps **Increased** to 137 compared to 68 with no frustum culling.
  In Debug Mode, 
  
