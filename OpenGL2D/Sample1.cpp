// Sample.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <GL/freeglut.h>

#define X 0
#define Y 1
#define Z 2

unsigned int num_points = 5;
double point[][3] = { {0.5, 0.5, 0.0},
 {-0.5, 0.5, 0.0}, {-0.5, -0.5, 0.0},
 {0.5, -0.5, 0.0}, {0.0, 0.0, 0.0} };

void display1(void)
{
	unsigned int i;
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glPointSize(2.0f);
	glBegin(GL_POINTS);
	for (i = 0; i < num_points; i++)
		glVertex3d(point[i][X], point[i][Y],
			point[i][Z]);
	glEnd();
	glFlush();
}

void display2(void)
{
	unsigned int i;
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(2.0f);
	glBegin(GL_LINE_STRIP);
	for (i = 0; i < num_points; i++)
		glVertex3d(point[i][X], point[i][Y],
			point[i][Z]);
	glEnd();
	glFlush();
}

void display3(void)
{
	unsigned int i;
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glLineWidth(2.0f);
	glBegin(GL_LINE_LOOP);
	for (i = 0; i < num_points; i++)
		glVertex3d(point[i][X], point[i][Y],
			point[i][Z]);
	glEnd();
	glFlush();
}

void display4(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_POLYGON);
	glVertex3d(point[0][X], point[0][Y],
		point[0][Z]);
	glVertex3d(point[1][X], point[1][Y],
		point[1][Z]);
	glVertex3d(point[2][X], point[2][Y],
		point[2][Z]);
	glVertex3d(point[3][X], point[3][Y],
		point[3][Z]);
	glEnd();
	glFlush();
}

void display5(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_POLYGON);
	glVertex3d(point[0][X], point[0][Y], point[0][Z]);
	glVertex3d(point[1][X], point[1][Y], point[1][Z]);
	glVertex3d(point[2][X], point[2][Y], point[2][Z]);
	glVertex3d(point[3][X], point[3][Y], point[3][Z]);
	glVertex3d(point[4][X], point[4][Y], point[4][Z]);
	glEnd();
	glFlush();
}

void display6(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_TRIANGLES);
	glVertex3d(point[0][X], point[0][Y], point[0][Z]);
	glVertex3d(point[1][X], point[1][Y], point[1][Z]);
	glVertex3d(point[4][X], point[4][Y], point[4][Z]);

	glVertex3d(point[1][X], point[1][Y], point[1][Z]);
	glVertex3d(point[2][X], point[2][Y], point[2][Z]);
	glVertex3d(point[4][X], point[4][Y], point[4][Z]);

	glVertex3d(point[2][X], point[2][Y], point[2][Z]);
	glVertex3d(point[3][X], point[3][Y], point[3][Z]);
	glVertex3d(point[4][X], point[4][Y], point[4][Z]);
	glEnd();
	glFlush();
}

void display7(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_TRIANGLES);
	glVertex3dv(point[0]);
	glVertex3dv(point[1]);
	glVertex3dv(point[4]);

	glVertex3dv(point[1]);
	glVertex3dv(point[2]);
	glVertex3dv(point[4]);

	glVertex3dv(point[2]);
	glVertex3dv(point[3]);
	glVertex3dv(point[4]);
	glEnd();
	glFlush();
}

void display8(void)
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

void display9(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_POLYGON);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3dv(point[0]);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3dv(point[1]);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3dv(point[2]);
	glEnd();
	glFlush();
}

void initGL(void)
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
}

int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA);
	glutCreateWindow(argv[0]);
	glutDisplayFunc(display9);
	initGL();
	glutMainLoop();
	return 0;
}
