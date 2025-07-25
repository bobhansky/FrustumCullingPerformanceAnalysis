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
 -  Ver6: **With** Instance drawing + **With** GPU Frustum Culling (Compute Shader).

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

   Naive drawing called 64001 (instances) * 2 (meshes) times of draw calls, resulting in an unsuprisingly low **7 fps**

-------------------------------------------------------------------------------------
## Ver2: No instance drawing + With CPU Frustum Culling
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v1f1/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v1f1/renderdoc.png" width=900 height=506/>

  Naive drawing + CPU frustum culling only drew 29450 objects. in total of 29450 * 2 times of draw calls, resulting a **doubled** **15 fps** compared to Ver1
  
-----------------------------------------------------------------------------------
## Ver3: **With** Instance drawing + **No** Frustum Culling
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f0/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f0/renderdoc.png" width=900 height=506/>
  
  Instance Drawing + No Frustum Culling reduces the draw call times to 2 but rendered all 64001 objects. FPS increased to **68 fps**. Increased **353%** compared to V2

-----------------------------------------------------------------------------------

## Ver4: **With** Instance drawing + **With** CPU Frustum Culling. Update and send the ModelMatrix(4x16 bytes * N instance) of visible objects to shader by VBO every frame
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f1/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f1/renderdoc.png" width=900 height=506/>

  Instance Drawing + CPU Side Frustum Culling:
  
  In Release Mode, where CPU performance is not capped, the bottleneck is at gpu. Fps **Increased** to **137** compared to 68 of Ver3. Increased **101%** compared to V3 
  
  In Debug Mode, CPU performance is significantly slower (to mimic cpu bottleneck). With Frustum Culling, the extra computation to check if bounding box is inside the 
  view frustum comsumes nearly half of the cpu cycles. Thus in this case, with frustum culling, the fps is **37**, significantly lower than the fps of Ver3 in **Debug mode**: still 68 fps.

  CPU profile in Visual Studio 2022 shows ~45% of time is on updateVisibleObject(), inside which program performs BVH traversal and test bounding volume against view frustum.
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v2f1/vs_shots.png" />

-----------------------------------------------------------------------------------

## Ver5: **With** Instance drawing + **With** CPU Frustum Culling. Send all model matrix of an object to shader in SSBO once, and then update and send the indices(4 bytes * N instance) of visible objects to shader every frame
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v4f1s1/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/28_v4f1s1/renderdoc.png" width=900 height=506/>

  CPU-GPU communication data size is reduced to 1/16 compared to the method of sending matrices to gpu every frame. Increase to **153 fps**. Increased **11.678%** compared to V4 release mode. 


## Ver6: **With** Instance drawing + **With** GPU Frustum Culling (Compute Shader).
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/29/frame.png" width=900 height=506/>
  <img src="https://github.com/bobhansky/FrustumCullingPerformanceAnalysis/blob/main/resources/29/renderdoc.png" width=900 height=506/>

  In release mode, FPS reduced to **140** because of the bottleneck is at gpu in release mode, and at the same time gpu need to do frustum culling, and program needs to synchronize with the execution of compute shader.
  In Debug mode, where bottleneck is at cpu, transfering the frustum culling job to gpu makes the fps goes from **37** to **138**. (Debug mode VER4 **VS** Debug mode VER6)
