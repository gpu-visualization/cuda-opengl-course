#include <math.h>

#define PI 3.141592653589793

// CPU処理．
#define h_U(x, y, t) (- 2.0f * (float)cos(PI * (t) / 8.0f) * (float)sin(PI * (x)) * (float)sin(PI * (x)) * (float)cos(PI * (y)) * (float)sin(PI * (y)))
#define h_V(x, y, t) (2.0f * (float)cos(PI * (t) / 8.0f) * (float)cos(PI * (x)) * (float)sin(PI * (x)) * (float)sin(PI * (y)) * (float)sin(PI * (y)))

// CPU用ルンゲ・クッタ法．
void h_RungeKutta(unsigned int index, float (*pos)[2], float time, float dt)
// unsigned int index; 粒子のインデックス．
// float (*pos)[2]; 粒子位置．
// float time; 時刻．
// float dt; 時間刻み．
{
 	float xn, yn, p1, q1, p2, q2, p3, q3, p4, q4;
	float x, y, t;
    xn = pos[index][0];
    yn = pos[index][1];

    // 1段目．
	p1 = h_U(xn, yn, time);
	q1 = h_V(xn, yn, time);

	// 2段目．
	x = xn + 0.5f * p1 * dt;
	y = yn + 0.5f * q1 * dt;
	t = time + 0.5f * dt;
	p2 = h_U(x, y, t);
	q2 = h_V(x, y, t);

	// 3段目．
	x = xn + 0.5f * p2 * dt;
	y = yn + 0.5f * q2 * dt;
	t = time + 0.5f * dt;
	p3 = h_U(x, y, t);
	q3 = h_V(x, y, t);

	// 4段目．
	x = xn + p3 * dt;
	y = yn + q3 * dt;
	t = time + dt;
	p4 = h_U(x, y, t);
	q4 = h_V(x, y, t);
   
    // 粒子位置の更新．
    pos[index][0] = xn + (p1 + 2 * p2 + 2 * p3 + p4) / 6.0f * dt;
    pos[index][1] = yn + (q1 + 2 * q2 + 2 * q3 + q4) / 6.0f * dt;
}

// CPU処理の起動．
void launchCPUKernel(unsigned int num_particles, float (*pos)[2], float time, float dt)
// unsigned int num_particles; 粒子の総数．
// float (*pos)[2]; 粒子位置．
// float time; 時刻．
// float dt; 時間刻み．
{
    unsigned int i;
    for (i = 0; i < num_particles; i++)
        h_RungeKutta(i, pos, time, dt);
}

// GPU処理．
#define d_U(x, y, t) (- 2.0f * __cosf(PI * (t) / 8.0f) * __sinf(PI * (x)) * __sinf(PI * (x)) * __cosf(PI * (y)) * __sinf(PI * (y)))
#define d_V(x, y, t) (2.0f * __cosf(PI * (t) / 8.0f) * __cosf(PI * (x)) * __sinf(PI * (x)) * __sinf(PI * (y)) * __sinf(PI * (y)))
//#define d_U(x, y, t) (__cosf(PI * (t) / 8.0f) * __sinf(4.0f * PI * (x + 0.5f)) * __sinf(4.0f * PI * (y + 0.5f)))
//#define d_V(x, y, t) (__cosf(PI * (t) / 8.0f) * __cosf(4.0f * PI * (x + 0.5f)) * __cosf(4.0f * PI * (y + 0.5f)))

// GPU用ルンゲ・クッタ法
__global__ void d_RungeKutta(unsigned int num_particles, float (*pos)[2], float time, float dt)
// unsigned int num_particles; 粒子の総数．
// float (*pos)[2]; 粒子位置．
// float time; 時刻．
// float dt; 時間刻み．
{
    unsigned int index;
	float xn, yn, p1, q1, p2, q2, p3, q3, p4, q4;
	float x, y, t;

    // 処理対象の粒子の決定．
    index = blockDim.x * blockIdx.x + threadIdx.x;
    if (index >= num_particles)
		return;
	xn = pos[index][0];
    yn = pos[index][1];

    // 1段目．
	p1 = d_U(xn, yn, time);
	q1 = d_V(xn, yn, time);

	// 2段目．
	x = xn + 0.5f * p1 * dt;
	y = yn + 0.5f * q1 * dt;
	t = time + 0.5f * dt;
	p2 = d_U(x, y, t);
	q2 = d_V(x, y, t);

	// 3段目．
	x = xn + 0.5f * p2 * dt;
	y = yn + 0.5f * q2 * dt;
	t = time + 0.5f * dt;
	p3 = d_U(x, y, t);
	q3 = d_V(x, y, t);

	// 4段目．
	x = xn + p3 * dt;
	y = yn + q3 * dt;
	t = time + dt;
	p4 = d_U(x, y, t);
	q4 = d_V(x, y, t);
   
    // 粒子位置の更新．
    pos[index][0] = xn + (p1 + 2 * p2 + 2 * p3 + p4) / 6.0f * dt;
    pos[index][1] = yn + (q1 + 2 * q2 + 2 * q3 + q4) / 6.0f * dt;
}

// GPU処理の起動．
void launchGPUKernel(unsigned int num_particles, float (*pos)[2], float time, float dt)
// unsigned int num_particles; 粒子の総数．
// float (*pos)[2]; 粒子位置．
// float time; 時刻．
// float dt; 時間刻み．
{
    dim3 grid(num_particles / 512 + 1, 1);
    dim3 block(512, 1, 1);
    d_RungeKutta <<< grid, block >>> (num_particles, pos, time, dt);
}