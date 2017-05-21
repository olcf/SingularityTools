#include <stdio.h>
#include <cuda.h>

// CUDA kernel. Each thread takes care of one element of c
__global__ void hello_cuda() {
  printf("hello from the GPU\n");
}
 
int main( int argc, char* argv[] )
{
  // Execute the kernel
  hello_cuda<<<1, 1>>>();

  cudaDeviceSynchronize(); 

  return 0;
}
