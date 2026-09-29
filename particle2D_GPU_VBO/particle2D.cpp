#include <stdio.h>
#include <math.h>

#include <gl/glew.h> // ←追加．
#include <gl/freeglut.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h> // ←追加．

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

// 頂点バッファオブジェクト．
GLuint vbo;
struct cudaGraphicsResource *vbo_res;

// 処理時間と時間刻み．
float anim_time = 0.0f;
float anim_dt = 0.01f;

extern void launchCPUKernel(unsigned int num_particles, float (*pos)[2], float time, float dt);
extern void launchGPUKernel(unsigned int num_particles, float (*pos)[2], float time, float dt);

// 粒子を初期位置に配置．
void setInitialPosition(void)
{
	unsigned int i;
	//srand(12131);
	for (i = 0; i < NUM_POINTS; i++) {
		h_point[i][0] = (float)rand() / RAND_MAX * 0.5f + 0.25f;
		h_point[i][1] = (float)rand() / RAND_MAX * 0.5f;
	}

	//// GPU側にデータの初期位置を転送．
	//cudaMalloc((void**)&d_point, NUM_POINTS * 2 * sizeof(float)); // ←不要なので削除．
	//cudaMemcpy(d_point, h_point,  NUM_POINTS * 2 * sizeof(float), cudaMemcpyHostToDevice); // ←不要なので削除．
}

// 頂点バッファオブジェクトの生成．追加．
void createVBO(GLuint *vbo, unsigned int size, struct cudaGraphicsResource **vbo_res, unsigned int vbo_res_flags)
// GLuint *bo; 頂点バッファオブジェクト．
// unsinged int size; バッファのサイズ．
// struct cudaGraphicsResource **vbo_res; CUDAのグラフィックスリソース．
// unsigned int vbo_res_flags; グラフィックスリソースの使い方のヒント．
{
    glGenBuffers(1, vbo);

    // 点データの頂点バッファオブジェクトへの設定．
    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, size, h_point, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // 生成した頂点バッファオブジェクトをグラフィックスリソースに登録．
	cudaGraphicsGLRegisterBuffer(vbo_res, *vbo, vbo_res_flags);
}

// 頂点バッファオブジェクトの削除．追加．
void deleteVBO(GLuint *vbo, struct cudaGraphicsResource *vbo_res)
// GLuint *vbo; 頂点バッファオブジェクト．
// struct cudaGraphicsResource *vbo_res; CUDAのグラフィックスリソース．
{

	// 頂点バッファオブジェクトを登録から外す．
	cudaGraphicsUnregisterResource(vbo_res);
    glDeleteBuffers(1, vbo);
    *vbo = 0;
}

// CPUによる解析．
void runCPUKernel(void)
{

	// CPU処理の呼び出し．
    launchCPUKernel(NUM_POINTS, h_point, anim_time, anim_dt);

	// 点データの頂点バッファオブジェクトへの再設定．追加．
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, NUM_POINTS * 2 * sizeof(float), h_point, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	anim_time += anim_dt;
}

// GPUによる解析．
void runGPUKernel(void)
{

    // 頂点バッファオブジェクトをd_pointへマップする．追加．
	cudaGraphicsMapResources(1, &vbo_res, 0);
	cudaGraphicsResourceGetMappedPointer((void**)&d_point, NULL, vbo_res);

	// GPU処理の呼び出し．
    launchGPUKernel(NUM_POINTS, d_point, anim_time, anim_dt);
	//cudaMemcpy(h_point, d_point, NUM_POINTS * 2 * sizeof(float), cudaMemcpyDeviceToHost); // ←不要なので削除．

	// アンマップ．追加．
	cudaGraphicsUnmapResources(1, &vbo_res, 0);
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

    // 粒子位置の更新．
	//runCPUKernel(); // CPU処理．
 	runGPUKernel(); // GPU処理．

    // 頂点バッファオブジェクトに基づく点群の描画．
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(1.0f, 0.0f, 0.0f);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, 0);
	glDrawArrays(GL_POINTS, 0, NUM_POINTS);
    glDisableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
 
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
//	cudaFree(d_point);

	// 頂点バッファオブジェクトの消去．追加．
    deleteVBO(&vbo, vbo_res);
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

	// glewの初期化．追加．
    glewInit();
    if (!glewIsSupported("GL_VERSION_2_0")) {
		fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing.");
        return false;
    }
  
    // 頂点バッファオブジェクトの生成．追加．
    createVBO(&vbo, NUM_POINTS * 2 * sizeof(float), &vbo_res, cudaGraphicsRegisterFlagsNone);
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
    if (!initGL())
		return 1;
 
    // アニメーション描画のループ．
    glutMainLoop();
	return 0;
}
