#include <stdio.h>

#include <gl/glew.h>
#include <gl/freeglut.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#define INIT_X_POS 128
#define INIT_Y_POS 128
#define INIT_WIDTH 512
#define INIT_HEIGHT 512

unsigned int window_width, window_height;
double left, right, bottom, top;

// 格子空間の範囲．
float min_x = -0.5f;
float max_x = 0.5f;
float min_y = -0.5f;
float max_y = 0.5f;

double init_left = min_x;
double init_right = max_x;
double init_bottom = min_y;
double init_top = max_y;

#define GRID_WIDTH 1024
#define GRID_HEIGHT 1024

// 格子（模様データ）の解像度．
unsigned int grid_width = GRID_WIDTH;
unsigned int grid_height = GRID_HEIGHT;

// 濃度値を格納する2次元格子．
float h_F[GRID_HEIGHT * GRID_WIDTH];
float h_Fn[GRID_HEIGHT * GRID_WIDTH];
float *d_F, *d_Fn;
float dx, dy;

// テクスチャ．
GLuint tex;
int h_g_data[GRID_HEIGHT * GRID_WIDTH];
int *d_g_data;

// ピクセルバッファオブジェクト．
GLuint pbo;
struct cudaGraphicsResource* pbo_res;

// 繰り返し回数のカウンタ．
unsigned int incrt = 1;

// 解析時間と時間刻み．
float anim_time = 0.0f;
float anim_dt;

// 解析で用いるパラメータ．
float kappa = 0.1f;
float max_density;

extern void launchCPUKernel(float *F, float *Fn, unsigned int width, unsigned int height, float dx, float dy, float kappa, float dt,
	float max_density, int *g_data);
extern void launchGPUKernel(float* F, float* Fn, unsigned int width, unsigned int height, float dx, float dy, float kappa, float dt,
	float max_density, int* g_data);

// 初期濃度の設定．
void setInitialDensity(unsigned int width, unsigned int height, float *max_density)
// unsigned int width, height; 格子のX方向とY方向の解像度．
// float *max_density; 濃度の最大値．
{
    unsigned int i, j, index;
    float x, y;
	dx = (max_x - min_x) / width;
	dy = (max_y - min_y) / height;

	// 矩形の濃度場を与える．
	*max_density = 1.0f;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            index = width * i + j;
            x = dx * (j + 0.5f) + min_x;
            y = dy * (i + 0.5f) + min_y;
			if ((x > (0.75f * min_x + 0.25f * max_x)) && (x < (0.25f * min_x + 0.75f * max_x)) 
				&& (y > (0.75f * min_y + 0.25f * max_y)) && (y < (0.25f * min_y + 0.75f * max_y)))
				h_F[index] = *max_density;
			else
				h_F[index] = 0.0f;
        }
    }

	// GPU処理用のメモリの確保．
	cudaMalloc((void**)&d_F, width * height * sizeof(float));
	cudaMalloc((void**)&d_Fn, width * height * sizeof(float));

	// h_Fのデータをデバイスメモリの配列d_Fへ転送．
	cudaMemcpy(d_F, h_F, width * height * sizeof(float), cudaMemcpyHostToDevice);
}

// テクスチャの生成．
void createTexture(GLuint *tex, unsigned int width, unsigned int height)
// GLuint *tex; テクスチャ．
// unsinged int width, height; テクスチャに格納するイメージデータの解像度．
{
    glGenTextures(1, tex);

    // 基本的なパラメータの設定．
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // 格納するイメージデータの定義．
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

// テクスチャの削除．
void deleteTexture(GLuint* tex)
// GLuint *tex; テクスチャ．
{
    glDeleteTextures(1, tex);
    *tex = 0;
}

// ピクセルバッファオブジェクトの生成．
void createPBO(GLuint* pbo, unsigned int size, struct cudaGraphicsResource** pbo_res, unsigned int pbo_res_flags)
// GLuint *pbo; ピクセルバッファオブジェクト．
// unsigned int size; バッファのサイズ．
// struct cudaGraphicsResource **pbo_res; CUDAのグラフィックスリソース．
// unsigned int pbo_res_flags; グラフィックスリソースの使い方のヒント．
{
	glGenBuffers(1, pbo);

	// 格子データのピクセルバッファオブジェクトへの設定．
	glBindBuffer(GL_ARRAY_BUFFER, *pbo);
	glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// 生成したピクセルバッファオブジェクトをグラフィックスリソースに登録．
	cudaGraphicsGLRegisterBuffer(pbo_res, *pbo, pbo_res_flags);
}

// ピクセルバッファオブジェクトの削除．
void deletePBO(GLuint* pbo, struct cudaGraphicsResource* pbo_res)
// GLuint *pbo; ピクセルバッファオブジェクト．
// struct cudaGraphicsResource *pbo_res; CUDAのグラフィックスリソース．
{

	// ピクセルバッファオブジェクトを登録から外す．
	cudaGraphicsUnregisterResource(pbo_res);
	glDeleteBuffers(1, pbo);
	*pbo = 0;
}

bool direction_p = true;

// CPUによる解析．
void runCPUKernel(void) 
{

	// CPU処理の呼び出し．
	if (direction_p)
		launchCPUKernel(h_F, h_Fn, grid_width, grid_height, dx, dy, kappa, anim_dt, max_density, h_g_data);
	else
		launchCPUKernel(h_Fn, h_F, grid_width, grid_height, dx, dy, kappa, anim_dt, max_density, h_g_data);
	direction_p = !direction_p;
	anim_time += anim_dt;
	
	// 模様データをテクスチャへ転送．	
	glBindTexture(GL_TEXTURE_2D, tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, grid_width, grid_height, GL_RGBA, GL_UNSIGNED_BYTE, h_g_data);
}

// GPUによる解析．
void runGPUKernel(void)
{

	// ピクセルバッファオブジェクトをd_g_dataへマップする．
	cudaGraphicsMapResources(1, &pbo_res, 0);
	cudaGraphicsResourceGetMappedPointer((void**)&d_g_data, NULL, pbo_res);

	// GPU処理の呼び出し．
	if (direction_p)
		launchGPUKernel(d_F, d_Fn, grid_width, grid_height, dx, dy, kappa, anim_dt, max_density, d_g_data);
	else
		launchGPUKernel(d_Fn, d_F, grid_width, grid_height, dx, dy, kappa, anim_dt, max_density, d_g_data);
	direction_p = !direction_p;
	anim_time += anim_dt;

	// アンマップ．
	cudaGraphicsUnmapResources(1, &pbo_res, 0);

	// ピクセルバッファオブジェクトの内容をテクスチャにバインド．
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, grid_width, grid_height, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
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

	// 拡散現象の解析と画像の生成．
	//runCPUKernel(); // CPU処理．
	runGPUKernel(); // GPU処理．

    // テクスチャが貼り付けられた四角形の描画．
	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, tex);
 	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(min_x, min_y); 
    glTexCoord2f(1.0f, 0.0f); glVertex2f(max_x, min_y); 
    glTexCoord2f(1.0f, 1.0f); glVertex2f(max_x, max_y); 
    glTexCoord2f(0.0f, 1.0f); glVertex2f(min_x, max_y);
    glEnd();	

    // 処理の進捗を表示．
	incrt++;
    if (incrt % 100 == 0) 
		printf("time(%4d): %7.5f\n", incrt, anim_time);	

	// 画像の更新．
	glutPostRedisplay();
}

// ウィンドウのリサイズ．
void resize(int width, int height)
{
    window_width = width;
    window_height = height;
}

// 後処理．
void cleanUp(void)
{
	cudaFree(d_F);
	cudaFree(d_Fn);

	// ピクセルバッファオブジェクトとテクスチャの消去．
	deletePBO(&pbo, pbo_res);

	// テクスチャの消去．
	//cudaFree(d_g_data);
	deleteTexture(&tex);
	cudaDeviceReset();
}

// キー処理．
void keyboard(unsigned char key, int /*x*/, int /*y*/)
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
    glEnable(GL_TEXTURE_2D);

	// glewの初期化．
	glewInit();
	if (!glewIsSupported("GL_VERSION_2_0")) {
		fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing.");
		return false;
	}

	// ピクセルバッファオブジェクトの生成．
	createPBO(&pbo, grid_width * grid_height * 4 * sizeof(GLubyte), &pbo_res, cudaGraphicsRegisterFlagsWriteDiscard);

	// テクスチャの生成．
	//cudaMalloc((void**)&d_g_data, grid_width * grid_height * sizeof(int));
    createTexture(&tex, grid_width, grid_height);
    return true;
}

int main(int argc, char** argv)
{
 
	// 初期濃度の設定と最大濃度の取得．
    setInitialDensity(grid_width, grid_height, &max_density);
	printf("Max: %f\n", max_density);

    // 時間刻みの設定．
	anim_dt = 0.2f * min(dx * dx, dy * dy) / kappa;

    // GLUT関係の設定．
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA);
	glutInitWindowPosition(INIT_X_POS, INIT_Y_POS);
	glutInitWindowSize(INIT_WIDTH, INIT_HEIGHT);
    glutCreateWindow("2D Diffusion Simulation");
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(resize);
	atexit(cleanUp);

	// OpenGLの設定．
	initGL();

    // アニメーション描画のループ．
    glutMainLoop();
    return 0;
}
