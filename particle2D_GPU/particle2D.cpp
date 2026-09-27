#include <stdio.h>
#include <math.h>

#include <gl/freeglut.h>

#include <cuda_runtime.h>

#define INIT_X_POS 128
#define INIT_Y_POS 128
#define INIT_WIDTH 512
#define INIT_HEIGHT 512

unsigned int window_width, window_height;
double left, right, bottom, top;

double init_left = -0.25;
double init_right = 1.25;
double init_bottom = -0.25;
double init_top = 1.25;

// 粒子数とその位置情報．
#define X 0
#define Y 1
#define NUM_POINTS (1024 * 1024)
float h_point[NUM_POINTS][2];
float (*d_point)[2];

// 処理時間と時間刻み．
float anim_time = 0.0f;
float anim_dt = 0.01f;

extern void launchCPUKernel(unsigned int num_particles, float (*pos)[2], float time, float dt);
extern void launchGPUKernel(unsigned int num_particles, float (*pos)[2], float time, float dt);

// 粒子を初期位置に配置．
void setInitialPosition(void)
{
	unsigned int i;
	for (i = 0; i < NUM_POINTS; i++) {
		h_point[i][0] = (float)rand() / RAND_MAX * 0.5f + 0.25f;
		h_point[i][1] = (float)rand() / RAND_MAX * 0.5f;
	}

	// GPU側にデータの初期位置を転送．
	cudaMalloc((void**)&d_point, NUM_POINTS * 2 * sizeof(float));
	cudaMemcpy(d_point, h_point,  NUM_POINTS * 2 * sizeof(float), cudaMemcpyHostToDevice);
}

// CPUによる解析．
void runCPUKernel(void)
{

	// CPU処理の呼び出し．
    launchCPUKernel(NUM_POINTS, h_point, anim_time, anim_dt);
	anim_time += anim_dt;
}

// GPUによる解析．
void runGPUKernel(void)
{

	// GPU処理の呼び出し．
    launchGPUKernel(NUM_POINTS, d_point, anim_time, anim_dt);
	cudaMemcpy(h_point, d_point, NUM_POINTS * 2 * sizeof(float), cudaMemcpyDeviceToHost);
	anim_time += anim_dt;
}

// 表示．
void display(void)
{
	unsigned int i;
	double dx, dy, d_aspect, w_aspect, d;

	// 表示範囲のアスペクト比とウィンドウのアスペクト比の比較．
	dx = init_right - init_left;
	dy = init_top - init_bottom;
	d_aspect = dy / dx;
	w_aspect = (double)window_height / (double)window_width;

	// ウィンドウが表示範囲よりも縦長．表示範囲を縦に広げる．
	if (w_aspect > d_aspect) {
		d = (dy * (w_aspect / d_aspect - 1.0)) * 0.5;
		left = init_left;
		right = init_right;
		bottom = init_bottom - d;
		top = init_top + d;

		// ウィンドウが表示範囲よりも横長．表示範囲を横に広げる．
	}
	else {
		d = (dx * (d_aspect / w_aspect - 1.0)) * 0.5;
		left = init_left - d;
		right = init_right + d;
		bottom = init_bottom;
		top = init_top;
	}

	// 正投影の設定．
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	glOrtho(left, right, bottom, top, -100.0, 100.0);
	glViewport(0, 0, window_width, window_height);

    // 粒子位置の更新．以下で希望しない側の処理をコメントアウトする．
	//runCPUKernel(); // CPU処理．
 	runGPUKernel(); // GPU処理．

	// 点群の描画．
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	for (i = 0; i < NUM_POINTS; i++)
		glVertex2f(h_point[i][X], h_point[i][Y]);
	glEnd();
 
	// 画像の更新．
    glutPostRedisplay();
}

// リサイズ．
void resize(int width, int height)
{

	// ウィンドウサイズの取得．
	window_width = width;
	window_height = height;
}

// 後処理．
void cleanUp(void)
{
	cudaFree(d_point);
	cudaDeviceReset();
}

// キー処理．
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
		case 'q':
		case 'Q':
		case '\033':
			exit(0);
    }
}

// OpenGL関係の初期設定．
bool initGL(void)
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  	return true;
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowPosition(INIT_X_POS, INIT_Y_POS);
	glutInitWindowSize(INIT_WIDTH, INIT_HEIGHT);
    glutCreateWindow("2D Particle Simulation");
    glutDisplayFunc(display);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyboard);
    atexit(cleanUp);

    // 粒子を初期位置に配置．
    setInitialPosition();
 
	// OpenGLの設定．
    initGL();
 
    // アニメーション描画のループ．
    glutMainLoop();
	return 0;
}
