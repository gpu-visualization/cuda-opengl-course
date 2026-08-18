#include <stdio.h>

#include <cuda_runtime.h>
#include <time.h>

#define BLOCK 16 // 各ブロックは16×16個のスレッドから定義されるものとする．
#define WIDTH 1024 // 処理対象の行列のサイズはWIDTH×WIDTH．

// ホスト（CPU）側の行列定義．
float h_A[WIDTH * WIDTH];
float h_B[WIDTH * WIDTH];
float h_C[WIDTH * WIDTH];

// デバイス（GPU）側の行列へのポインタ．
float *d_A, *d_B, *d_C;

void h_multiply(float *A, float *B, float *C);
__global__ void d_multiply0(float *A, float *B, float *C);
__global__ void d_multiply1(float *A, float *B, float *C);

// メイン関数．
int main()
{
	unsigned int i;
	long start, end;

	// デバイス側に行列用のメモリを確保．
    cudaMalloc((void**)&d_A, WIDTH * WIDTH * sizeof(float));
	cudaMalloc((void**)&d_B, WIDTH * WIDTH * sizeof(float));
	cudaMalloc((void**)&d_C, WIDTH * WIDTH * sizeof(float));

	// ホスト側の行列に値をセット．
    for (i = 0; i < (WIDTH * WIDTH); i++) {
		h_A[i] = (float)i;
		h_B[i] = (float)i;
	}

    // 計算時間計測用のタイマーのセット．
	start = clock();

	// ホスト側の行列のデータをデバイス側の行列へ転送．
	cudaMemcpy(d_A, h_A, WIDTH * WIDTH * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, WIDTH * WIDTH * sizeof(float), cudaMemcpyHostToDevice);

	// グリッドとブロックの定義．
    dim3 grid(WIDTH / BLOCK+1, WIDTH / BLOCK+1);
    dim3 block(BLOCK, BLOCK);

	// GPU処理の起動．
	d_multiply0 <<< grid, block >>> (d_A, d_B, d_C);
	//d_multiply1 <<< grid, block >>> (d_A, d_B, d_C);

	// 計算結果はd_Cに格納されているので，それをホスト側のh_Cへ転送．
	cudaMemcpy(h_C, d_C, WIDTH * WIDTH * sizeof(float), cudaMemcpyDeviceToHost);

    // 計算結果の表示．
	end = clock();
	printf("デバイス計算時間:   %f(s)　", (double)(end - start) / CLOCKS_PER_SEC);
	printf("デバイス計算結果:   %f\n", h_C[WIDTH * WIDTH - 1]);

    // デバイス側のメモリを解放．
	cudaFree(d_A);
	cudaFree(d_B);
	cudaFree(d_C);

	// 比較用にホスト側でも計算してみる．
	start = clock();
    h_multiply(h_A, h_B, h_C);
	end = clock();
	printf("　ホスト計算時間:   %f(s)　", (double)(end - start) / CLOCKS_PER_SEC);
	printf("　ホスト計算結果:   %f\n", h_C[WIDTH * WIDTH - 1]);			
}

// CPUによる行列の積の計算．
void h_multiply(float *A, float *B, float *C)
{
	unsigned int r, c, i;
    float tmp;
	for (r = 0; r < WIDTH; r++) {
		for (c = 0; c < WIDTH; c++) {
	        tmp = 0.0;
	        for (i = 0; i < WIDTH; i++)
				tmp += A[WIDTH * r + i] * B[WIDTH * i + c];
	        C[WIDTH * r + c] = tmp;
		}
	}
}

// GPUによる行列の積の計算．
__global__ void d_multiply0(float *A, float *B, float *C)
{
	unsigned int r = blockDim.y * blockIdx.y + threadIdx.y;
	unsigned int c = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int i;
	float tmp;
	tmp = 0.0f;
	if ((r < WIDTH) && (c < WIDTH)) {
	for (i = 0; i < WIDTH; i++)
		tmp += A[WIDTH * r + i] * B[WIDTH * i + c];
	C[WIDTH * r + c] = tmp;
	}
}

// GPUによる行列の積の計算．
// シェアードメモリを使う場合．
__global__ void d_multiply1(float *A, float *B, float *C)
{
	unsigned int r = blockDim.y * blockIdx.y + threadIdx.y;
	unsigned int c = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int i, j;
	float tmp;
	__shared__ float s_A[BLOCK][BLOCK];
	__shared__ float s_B[BLOCK][BLOCK];
    tmp = 0.0f;
	if ((r < WIDTH) && (c < WIDTH)) {
		for (i = 0; i < WIDTH; i += BLOCK) {

			// 行列の一部をシェアードメモリに確保．
			s_A[threadIdx.y][threadIdx.x] = A[WIDTH * r + i + threadIdx.x];
			s_B[threadIdx.y][threadIdx.x] = B[WIDTH * (i + threadIdx.y) + c];
			__syncthreads();

			// シェアードメモリで積を計算．
			for (j = 0; j < BLOCK; j++)
				tmp += s_A[threadIdx.y][j] * s_B[j][threadIdx.x];
			__syncthreads();
		}
		C[WIDTH * r + c] = tmp;
	}
}
