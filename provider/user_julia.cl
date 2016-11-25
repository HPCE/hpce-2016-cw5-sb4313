inline float cmod(float2 a){
	return (sqrt(a.x*a.x + a.y*a.y));
}

inline float2  cmult(float2 a, float2 b){
    return (float2)( a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}

__kernel void julia_kernel (int height, int width, int maxIter, float2 c, __global uint *pDest) {

	float dx=3.0f/width, dy=3.0f/height;

	int x = get_global_id(0);
	int y = get_global_id(1);
    //int loop3 = get_global_id(2);

	// typedef float2 cfloat;
	float2 z;

	z.x = -1.5f+x*dx;
	z.y = -1.5f+y*dy;

	int iter=0;
	while(iter<maxIter){
		if(cmod(z) > 2){
			break;
		}
        // Anybody want to refine/tighten this?
		z = cmult(z,z)+ c;
		++iter;
	}
	pDest[y*width+x] = iter;
};


