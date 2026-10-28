#include <stdio.h>

#include <omp.h>

// xの値を[a, b]の範囲にクランプ．
float h_clamp(float x, float a, float b)
{
	if (x < a)
		x = a;
	if (x > b)
		x = b;
	return x;
}

// 0～255.0fの浮動小数点のr, g, b値から色データを生成．
int h_RGBToInt(float r, float g, float b)
{
    r = h_clamp(r, 0.0f, 255.0f);
    g = h_clamp(g, 0.0f, 255.0f);
    b = h_clamp(b, 0.0f, 255.0f);
    return (int(b) << 16) | (int(g) << 8) | int(r);
}

// CPUによる差分法計算．
void h_diffusion2D(unsigned int j, unsigned int i, float *F, float *Fn, unsigned int width, unsigned int height, float c0, float c1, float c2,
	float max_density, int *g_data)
// unsingned int j, i; 格子のX方向とY方向のインデックス．
// float *F; 現在の格子データ．
// float *Fn; 更新された格子データ．
// unsigned width, height; 格子データのX方向とY方向の解像度．
// float c0, c1, c2; 差分法計算の係数．
// float max_density; 濃度の最大値．
// int *g_data; 画像データの出力．
{
    unsigned int index;
    float fc, fr, fl, fd, ft, density;

    // F[index]の周囲の4要素(上下左右の4要素)を決める．
    index = width * i + j;
	fc = F[index];
    if (j == 0)
        fl = fc;
    else
        fl = F[index - 1];
    if (j == (width - 1)) 
        fr = fc;
    else             
        fr = F[index + 1];
	if (i == 0) 
        fd = fc;
    else        
        fd = F[index - width];
 	if (i == (height - 1))
        ft = fc;
    else             
        ft = F[index + width];
	Fn[index] = c0 * (fr + fl) + c1 * (ft + fd) + c2 * fc;
	density = 255.0f - Fn[index] / max_density * 255.5f;
	g_data[index] = h_RGBToInt(density, density, density);
}

// CPU処理の起動．
void launchCPUKernel(float *F, float *Fn, unsigned int width, unsigned int height, float dx, float dy, float kappa, float dt,
	float max_density, int *g_data)
// float *F; 現在の格子データ．
// float *Fn; 更新された格子データ．
// unsigned width, height; 格子データのX方向とY方向の解像度．
// float dx, dy; X方向とY方向の格子間隔．
// float kappa; 拡散係数．
// float dt; 時間間隔．
// float max_density; 濃度の最大値．
// int *g_data; 画像データの出力．
{
	unsigned int i, j;
    float c0, c1, c2;

    // 差分法の係数の決定．
    c0 = kappa * dt / (dx * dx);
    c1 = kappa * dt / (dy * dy);
    c2 = 1.0f - 2.0f * (c0 + c1);
	for (i = 0; i < height; i++) {
#pragma omp parallel for
		for (j = 0; j < width; j++) {
			h_diffusion2D(j, i, F, Fn, width, height, c0, c1, c2, max_density, g_data);
		}
	}
}

// ブロックサイズの定義．
#define  BLOCKDIM_X 16
#define  BLOCKDIM_Y 16

// xの値を[a, b]の範囲にクランプ．
__device__ float d_clamp(float x, float a, float b)
{
 	if (x < a)
		x = a;
	if (x > b)
		x = b;
	return x;
}

// 0～255.0fの浮動小数点のr, g, b値から色データを生成．
__device__ int d_RGBToInt(float r, float g, float b)
{
    r = d_clamp(r, 0.0f, 255.0f);
    g = d_clamp(g, 0.0f, 255.0f);
    b = d_clamp(b, 0.0f, 255.0f);
    return (int(r) << 16) | (int(g) << 8) | int(b);
}

// GPUによる差分法計算．
// グローバルメモリを直接参照する場合．
__global__ void d_diffusion2D0(float *F, float *Fn, unsigned int width, unsigned int height, float c0, float c1, float c2,
	float max_density, int *g_data)
// float *F; 現在の格子データ．
// float *Fn; 更新された格子データ．
// unsigned width, height; 格子データのX方向とY方向の解像度．
// float c0, c1, c2; 差分法計算の係数．
// float max_density; 濃度の最大値．
// int *g_data; 画像データの出力．
{
    unsigned int i, j, index;
    float fc, fr, fl, fd, ft, density;
    i = blockDim.y * blockIdx.y + threadIdx.y;
    j = blockDim.x * blockIdx.x + threadIdx.x;

	// 格子の範囲のチェック．
	if ((i >= height) || (j >= width))
		return;

    // F[index]の周囲の4要素(上下左右の4要素)を決める．
    index = width * i + j;
    fc = F[index];
    if (j == 0)
        fl = fc;
    else
        fl = F[index - 1];
    if (j == (width - 1)) 
        fr = fc;
    else             
        fr = F[index + 1];
    if (i == 0) 
        fd = fc;
    else        
        fd = F[index - width];
    if (i == (height - 1))
        ft = fc;
    else             
        ft = F[index + width];
    Fn[index] = c0 * (fr + fl) + c1 * (ft + fd) + c2 * fc;
    density = 255.0f - Fn[index] / max_density * 255.5f;
	g_data[index] = d_RGBToInt(density, density, density);
}

// GPUによる差分法計算．
// シェアードメモリを使う場合．
__global__ void  d_diffusion2D1(float* F, float* Fn, unsigned int width, unsigned int height, float c0, float c1, float c2,
    float max_density, int* g_data)
    // float *F; 現在の格子データ．
    // float *Fn; 更新された格子データ．
    // unsigned width, height; 格子データのX方向とY方向の解像度．
    // float c0, c1, c2; 差分法計算の係数．
    // float max_density; 濃度の最大値．
    // int *g_data; 画像データの出力．
{
    unsigned int gi, gj, i, j, index;
    float fc, density;
    __shared__ float fs[BLOCKDIM_Y + 2][BLOCKDIM_X + 2];
    gi = blockDim.y * blockIdx.y + threadIdx.y;
    gj = blockDim.x * blockIdx.x + threadIdx.x;

    // 格子の範囲のチェック．
    if ((gi >= height) || (gj >= width))
        return;

    // シェアードメモリに，ブロックを左右上下1ずつ広げた範囲のデータを格納．
    index = width * gi + gj;
    i = threadIdx.y + 1;
    j = threadIdx.x + 1;
    fc = fs[i][j] = F[index];
    if (threadIdx.x == 0) { // ブロックの左端．
        if (gj == 0) // そこが格子の左端の場合．
            fs[i][0] = fc;
        else
            fs[i][0] = F[index - 1];
    }
    if (threadIdx.x == (blockDim.x - 1)) { // ブロックの右端．
        if (gj == width - 1) // そこが格子の右端の場合．
            fs[i][blockDim.x + 1] = fc;
        else
            fs[i][blockDim.x + 1] = F[index + 1];
    }
    if (threadIdx.y == 0) { // ブロックの下端．
        if (gi == 0) // そこが格子の下端の場合．
            fs[0][j] = fc;
        else
            fs[0][j] = F[index - width];
    }
    if (threadIdx.y == blockDim.y - 1) { // ブロックの上端．
        if (gi == height - 1) // そこが格子の上端の場合．
            fs[blockDim.y + 1][j] = fc;
        else
            fs[blockDim.y + 1][j] = F[index + width];
    }
    __syncthreads();
    Fn[index] = c0 * (fs[i][j + 1] + fs[i][j - 1]) + c1 * (fs[i + 1][j] + fs[i - 1][j])
        + c2 * fs[i][j];
    density = 255.0f - Fn[index] / max_density * 255.5f;
    g_data[index] = d_RGBToInt(density, density, density);
}

// GPU処理の起動．
void launchGPUKernel(float *F, float *Fn, unsigned int width, unsigned int height, float dx, float dy, float kappa, float dt,
    float max_density, int *g_data)
// float *F; 現在の格子データ．
// float *Fn; 更新された格子データ．
// unsigned width, height; 格子データのX方向とY方向の解像度．
// float dx, dy; X方向とY方向の格子間隔．
// float kappa; 拡散係数．
// float dt; 時間間隔．
// float max_density; 濃度の最大値．
// bool direction; 処理の方向．trueの時F->Fn，falseの時Fn->F．
// int *g_data; 画像データの出力．
{
    float c0, c1, c2;

    // 差分法の係数の決定．
    c0 = kappa * dt / (dx * dx);
    c1 = kappa * dt / (dy * dy),
    c2 = 1.0f - 2.0f * (c0 + c1);

    // グリッドサイズとブロックサイズの定義．
    dim3 grid(width / BLOCKDIM_X + 1, height / BLOCKDIM_Y + 1);
    dim3 block(BLOCKDIM_X, BLOCKDIM_Y, 1);
	d_diffusion2D1 <<< grid, block >>> (F, Fn, width, height, c0, c1, c2, max_density, g_data);
}
