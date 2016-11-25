__kernel void random_walk_kernel (__global uint* edge, uint* count, uint length ,__global uint *seed, __global uint *start, __global uint edge_size) {
    uint n  = get_global_id(0);
    uint rng=seed[n];
    uint current=start[n];
    for(uint i=0; i<n; i++){
      count[current] = count[current] + 1;
      unsigned edgeIndex = rng % edge_size;
      rng=rng*1664525+1013904223; 
      current=edges[n*edge_size+edgeIndex];
    }
};


