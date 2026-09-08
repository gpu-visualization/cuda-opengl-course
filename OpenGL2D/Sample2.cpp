// Sample.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <stdio.h>
#include <GL/freeglut.h>

#define X 0
#define Y 1
#define Z 2

unsigned int num_points = 5;
//double point[][3] = { {0.5, 0.5, 0.0},
// {-0.5, 0.5, 0.0}, {-0.5, -0.5, 0.0},
// {0.5, -0.5, 0.0}, {0.0, 0.0, 0.0} };
double point[][3] = { {1.3, 1.3, 0.0}, {0.3, 1.3, 0.0}, {0.3, 0.3, 0.0}, {1.3, 0.3, 0.0}, {0.8, 0.8, 0.0} };

unsigned int window_width, window_height;
double left, right, bottom, top;

double init_left = -2.0;
double init_right = 2.0;
double init_bottom = -2.0;
double init_top = 2.0;

void display1(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3dv(point[0]);
	glVertex3dv(point[1]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3dv(point[1]);
	glVertex3dv(point[2]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3dv(point[2]);
	glVertex3dv(point[3]);
	glVertex3dv(point[4]);
	glEnd();
	glFlush();
}

void display2(void)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-2.0, 2.0, -2.0, 2.0, -100.0, 100.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3dv(point[0]);
	glVertex3dv(point[1]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3dv(point[1]);
	glVertex3dv(point[2]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3dv(point[2]);
	glVertex3dv(point[3]);
	glVertex3dv(point[4]);
	glEnd();
	glFlush();
}

void display3(void)
{
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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, -100.0, 100.0);
	glViewport(0, 0, window_width, window_height);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3dv(point[0]);
	glVertex3dv(point[1]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3dv(point[1]);
	glVertex3dv(point[2]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3dv(point[2]);
	glVertex3dv(point[3]);
	glVertex3dv(point[4]);
	glEnd();
	glFlush();
}

void display4(void)
{
	double dx, dy, d_aspect, w_aspect, d;
	double margin;
	unsigned int i;

	// データの範囲の読み取り．
	init_left = init_bottom = 10000.0;
	init_right = init_top = -10000.0;
	for (i = 0; i < num_points; i++) {
		if (point[i][X] < init_left)
			init_left = point[i][X];
		if (point[i][X] > init_right)
			init_right = point[i][X];
		if (point[i][Y] < init_bottom)
			init_bottom = point[i][Y];
		if (point[i][Y] > init_top)
			init_top = point[i][Y];
	}

	// 周囲を5%だけ広げる．
	margin = (init_right - init_left) * 0.05;
	init_left -= margin;
	init_right += margin;
	margin = (init_top - init_bottom) * 0.05;
	init_bottom -= margin;
	init_top += margin;

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

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(left, right, bottom, top, -100.0, 100.0);
	glViewport(0, 0, window_width, window_height);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3dv(point[0]);
	glVertex3dv(point[1]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3dv(point[1]);
	glVertex3dv(point[2]);
	glVertex3dv(point[4]);

	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3dv(point[2]);
	glVertex3dv(point[3]);
	glVertex3dv(point[4]);
	glEnd();
	glFlush();
}

void resize(int w, int h)
{
	printf("Size %d x %d\n", w, h);
	window_width = w;
	window_height = h;
}

void initGL(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

int main(int argc, char* argv[])
{
	glutInitWindowPosition(128, 128);
	glutInitWindowSize(768, 768);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutCreateWindow(argv[0]);
	glutDisplayFunc(display4);
	glutReshapeFunc(resize);
	initGL();
	glutMainLoop();
	return 0;
}
