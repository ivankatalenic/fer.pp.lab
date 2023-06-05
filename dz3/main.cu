#include <iostream>

#include <cuda_runtime.h>

// Complex pointwise multiplication and scale
static __global__ void ComplexPointwiseMulAndScale(
	cufftComplex* arr,
	int len,
	float scale,
	int offset
) {
	const unsigned i{blockIdx.x * blockDim.x + threadIdx.x};
	if (i >= len) {
		return;
	}
	arr[i + offset] = ComplexScale(
		ComplexMul(arr[i + offset], arr[i % offset]),
		scale
	);
}

int main(int argc, char* argv[]) {
	return 0;
}
