#version 430
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// this is not used in compute shader
layout(std430, binding = 0) buffer ModelMatrices {
    mat4 allModeMatrix[];
};

// input: AABBs 
struct AABB {
    // vec3 occupies 16 bytes: 12 for data + 4 bytes of padding
    vec3 minP;
    vec3 maxP;
};
layout(std430, binding = 1) buffer AABBs {
    AABB aabbs[]; // 0 or 1 for each instance
};


// output
layout(std430, binding = 2) buffer VisibleIndices {
    uint count;
    uint visibleIndices[];
};

    
uniform vec4 frustumPlanes[6]; // each is vec4(a, b, c, d)

bool isOnOrAbovePlane(vec4 plane, AABB aabb){
    vec3 center = (aabb.minP + aabb.maxP) * 0.5;
    vec3 extent = vec3(aabb.maxP.x - center.x, aabb.maxP.y - center.y, aabb.maxP.z - center.z);

    vec3 normal = plane.xyz;
    // we only care about the magnitude of the maximum reach
    float r = extent.x * abs(normal.x) + extent.y * abs(normal.y) + extent.z * abs(normal.z);

    return -r <= dot(normal, center) - plane.w;
}

bool isOnFrustum(AABB aabb){
    return isOnOrAbovePlane(frustumPlanes[0], aabb) && 
           isOnOrAbovePlane(frustumPlanes[1], aabb) && 
           isOnOrAbovePlane(frustumPlanes[2], aabb) && 
           isOnOrAbovePlane(frustumPlanes[3], aabb) && 
           isOnOrAbovePlane(frustumPlanes[4], aabb) && 
           isOnOrAbovePlane(frustumPlanes[5], aabb);   
}


void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= aabbs.length()) return;

    AABB aabb = aabbs[id];
    
    if(isOnFrustum(aabb)) {
        // return the count before the addition
        uint outIdx = atomicAdd(count, 1);
        visibleIndices[outIdx] = id;
    }
    
    // uint outIdx = atomicAdd(count, 1);
    // visibleIndices[outIdx] = id;

}