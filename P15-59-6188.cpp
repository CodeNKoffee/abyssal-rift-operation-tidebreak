#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <vector>

#define GLUT_KEY_ESCAPE 27
#define DEG2RAD(a) (a * 0.0174532925f)

class Vector3f {
public:
	float x, y, z;

	Vector3f(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f) {
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3f operator+(const Vector3f &v) const {
		return Vector3f(x + v.x, y + v.y, z + v.z);
	}

	Vector3f operator-(const Vector3f &v) const {
		return Vector3f(x - v.x, y - v.y, z - v.z);
	}

	Vector3f operator*(float n) const {
		return Vector3f(x * n, y * n, z * n);
	}

	Vector3f operator/(float n) const {
		return Vector3f(x / n, y / n, z / n);
	}

	Vector3f &operator+=(const Vector3f &v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	float length() const {
		return sqrtf(x * x + y * y + z * z);
	}

	Vector3f unit() const {
		float len = length();
		if (len == 0.0f) {
			return Vector3f();
		}
		return *this / len;
	}

	Vector3f cross(const Vector3f &v) const {
		return Vector3f(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
	}
};

class Camera {
public:
	Vector3f eye, center, up;

	Camera(float eyeX = 1.0f, float eyeY = 1.0f, float eyeZ = 1.0f, float centerX = 0.0f, float centerY = 0.0f, float centerZ = 0.0f, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f) {
		eye = Vector3f(eyeX, eyeY, eyeZ);
		center = Vector3f(centerX, centerY, centerZ);
		up = Vector3f(upX, upY, upZ);
	}

	void moveX(float d) {
		Vector3f right = up.cross(center - eye).unit();
		eye = eye + right * d;
		center = center + right * d;
	}

	void moveY(float d) {
		Vector3f u = up.unit();
		eye = eye + u * d;
		center = center + u * d;
	}

	void moveZ(float d) {
		Vector3f view = (center - eye).unit();
		eye = eye + view * d;
		center = center + view * d;
	}

	void rotateX(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		float rad = DEG2RAD(a);
		Vector3f rotated = view * cosf(rad) + up * sinf(rad);
		up = rotated.cross(right);
		center = eye + rotated;
	}

	void rotateY(float a) {
		Vector3f view = (center - eye).unit();
		Vector3f right = up.cross(view).unit();
		float rad = DEG2RAD(a);
		Vector3f rotated = view * cosf(rad) + right * sinf(rad);
		right = rotated.cross(up);
		center = eye + rotated;
	}

	void look() {
		gluLookAt(
			eye.x, eye.y, eye.z,
			center.x, center.y, center.z,
			up.x, up.y, up.z
		);
	}
};

enum GameState {
	STATE_PLAYING,
	STATE_WIN,
	STATE_LOSE
};

struct Player {
	Vector3f position;
	Vector3f velocity;
	float yaw;
	float tilt;
	bool airborne;
};

struct Goal {
	Vector3f position;
	bool collected;
};

struct AnimationController {
	bool active;
	float phase;
};

Camera camera(1.8f, 0.9f, 1.8f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f, 0.0f);
Player player;
std::vector<Goal> goals;
AnimationController objectControllers[5];

GameState gameState = STATE_PLAYING;

bool moveForward = false;
bool moveBackward = false;
bool moveLeft = false;
bool moveRight = false;
bool moveUp = false;
bool moveDown = false;

float goalRotation = 0.0f;
float wallColorPhase = 0.0f;
float remainingTime = 120.0f;
int lastTick = 0;

const float SCENE_HALF = 1.0f;
const float GROUND_Y = 0.0f;
const float MAX_HEIGHT = 0.85f;
const float PLAYER_RADIUS = 0.05f;
const float PLAYER_SPEED = 0.65f;
const float PLAYER_ASCEND_SPEED = 0.5f;
const float GOAL_RADIUS = 0.12f;

void setupLights();
void setupCamera();
void resetGame();
void updateGame(float dt);
void drawScene();
void drawGround();
void drawWalls();
void drawPlayer();
void drawGoals();
void drawFloodlight(float rotation);
void drawAirlock(float doorPhase);
void drawCoralCluster(float swayPhase);
void drawConsole(float pulsePhase);
void drawDrone(float bobPhase);
void drawHud();
void drawGameResult();
void setFrontView();
void setSideView();
void setTopView();
void setFreeView();

int goalsRemaining() {
	int count = 0;
	for (size_t i = 0; i < goals.size(); ++i) {
		if (!goals[i].collected) {
			++count;
		}
	}
	return count;
}

void initGoals() {
	goals.clear();
	goals.push_back({ Vector3f(-0.55f, 0.12f, -0.45f), false });
	goals.push_back({ Vector3f(0.58f, 0.18f, 0.32f), false });
	goals.push_back({ Vector3f(0.1f, 0.14f, -0.05f), false });
}

void resetPlayer() {
	player.position = Vector3f(0.0f, PLAYER_RADIUS, 0.0f);
	player.velocity = Vector3f();
	player.yaw = 0.0f;
	player.tilt = 0.0f;
	player.airborne = false;
}

void resetAnimations() {
	for (int i = 0; i < 5; ++i) {
		objectControllers[i].active = false;
		objectControllers[i].phase = 0.0f;
	}
}

void resetGame() {
	gameState = STATE_PLAYING;
	remainingTime = 120.0f;
	goalRotation = 0.0f;
	wallColorPhase = 0.0f;
	resetPlayer();
	resetAnimations();
	initGoals();
	moveForward = moveBackward = moveLeft = moveRight = false;
	moveUp = moveDown = false;
	lastTick = glutGet(GLUT_ELAPSED_TIME);
}

void setupLights() {
	GLfloat ambient[] = { 0.2f, 0.25f, 0.35f, 1.0f };
	GLfloat diffuse[] = { 0.4f, 0.6f, 0.7f, 1.0f };
	GLfloat specular[] = { 0.6f, 0.7f, 0.8f, 1.0f };
	GLfloat shininess[] = { 40.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

	GLfloat position[] = { -0.6f, 1.2f, 0.6f, 1.0f };
	GLfloat lightDiffuse[] = { 0.5f, 0.7f, 0.9f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, position);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightDiffuse);
}

void setupCamera() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, 640.0f / 480.0f, 0.01f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	camera.look();
}

void drawFloodlight(float rotation) {
	glPushMatrix();
	glColor3f(0.18f, 0.2f, 0.22f);
	glPushMatrix();
	glScalef(0.18f, 0.04f, 0.18f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.12f, 0.0f);
	glScalef(0.08f, 0.24f, 0.08f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.25f, 0.0f);
	glScalef(0.14f, 0.04f, 0.14f);
	glutSolidCube(1.0);
	glPopMatrix();
	glTranslatef(0.0f, 0.27f, 0.0f);
	glRotatef(rotation, 0.0f, 1.0f, 0.0f);
	glColor3f(0.24f, 0.3f, 0.35f);
	glPushMatrix();
	glScalef(0.12f, 0.06f, 0.2f);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.65f, 0.85f, 0.9f);
	glTranslatef(0.0f, 0.01f, 0.08f);
	glScalef(0.08f, 0.06f, 0.08f);
	glutSolidSphere(0.8f, 20, 20);
	glPopMatrix();
}

void drawAirlock(float doorPhase) {
	float openOffset = 0.16f * (0.5f + 0.5f * sinf(doorPhase));
	glPushMatrix();
	glColor3f(0.25f, 0.3f, 0.35f);
	glPushMatrix();
	glTranslatef(-0.22f, 0.3f, 0.0f);
	glScalef(0.08f, 0.6f, 0.4f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.22f, 0.3f, 0.0f);
	glScalef(0.08f, 0.6f, 0.4f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.6f, 0.0f);
	glScalef(0.44f, 0.06f, 0.4f);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.35f, 0.52f, 0.6f);
	glPushMatrix();
	glTranslatef(-openOffset, 0.3f, 0.0f);
	glScalef(0.16f, 0.5f, 0.32f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(openOffset, 0.3f, 0.0f);
	glScalef(0.16f, 0.5f, 0.32f);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.18f, 0.22f, 0.26f);
	glPushMatrix();
	glTranslatef(0.0f, 0.05f, 0.0f);
	glScalef(0.42f, 0.1f, 0.08f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPopMatrix();
}

void drawCoralCluster(float swayPhase) {
	float sway = 8.0f * sinf(swayPhase);
	glPushMatrix();
	glColor3f(0.25f, 0.18f, 0.35f);
	glPushMatrix();
	glTranslatef(0.0f, 0.08f, 0.0f);
	glScalef(0.22f, 0.04f, 0.22f);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.58f, 0.25f, 0.6f);
	glPushMatrix();
	glTranslatef(-0.05f, 0.18f, 0.02f);
	glRotatef(sway, 0.0f, 0.0f, 1.0f);
	glScalef(0.08f, 0.18f, 0.08f);
	glutSolidSphere(1.0f, 18, 18);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.06f, 0.2f, -0.04f);
	glRotatef(-sway * 0.6f, 0.0f, 0.0f, 1.0f);
	glScalef(0.06f, 0.16f, 0.06f);
	glutSolidSphere(1.0f, 18, 18);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.02f, 0.12f, 0.06f);
	glScalef(0.05f, 0.14f, 0.05f);
	glutSolidSphere(1.0f, 18, 18);
	glPopMatrix();
	glPopMatrix();
}

void drawConsole(float pulsePhase) {
	float pulse = 1.0f + 0.1f * sinf(pulsePhase);
	glPushMatrix();
	glColor3f(0.26f, 0.32f, 0.38f);
	glPushMatrix();
	glScalef(0.28f, 0.12f, 0.36f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.1f, -0.12f);
	glScalef(0.24f, 0.14f, 0.14f);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.15f, 0.7f, 0.75f);
	glPushMatrix();
	glTranslatef(0.0f, 0.18f, -0.15f);
	glScalef(0.28f * pulse, 0.02f, 0.14f * pulse);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.3f, 0.5f, 0.6f);
	glPushMatrix();
	glTranslatef(-0.08f, 0.07f, 0.15f);
	glScalef(0.08f, 0.16f, 0.08f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.08f, 0.07f, 0.15f);
	glScalef(0.08f, 0.16f, 0.08f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPopMatrix();
}

void drawDrone(float bobPhase) {
	float bob = 0.07f * sinf(bobPhase);
	glPushMatrix();
	glTranslatef(0.0f, 0.16f + bob, 0.0f);
	glColor3f(0.65f, 0.2f, 0.3f);
	glPushMatrix();
	glScalef(0.16f, 0.08f, 0.16f);
	glutSolidSphere(1.0f, 22, 22);
	glPopMatrix();
	glColor3f(0.2f, 0.22f, 0.25f);
	glPushMatrix();
	glTranslatef(0.14f, 0.0f, 0.0f);
	glScalef(0.12f, 0.04f, 0.04f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.14f, 0.0f, 0.0f);
	glScalef(0.12f, 0.04f, 0.04f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.14f);
	glScalef(0.04f, 0.04f, 0.12f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -0.14f);
	glScalef(0.04f, 0.04f, 0.12f);
	glutSolidCube(1.0);
	glPopMatrix();
	glColor3f(0.9f, 0.5f, 0.6f);
	glPushMatrix();
	glTranslatef(0.0f, 0.05f, 0.0f);
	glScalef(0.08f, 0.02f, 0.08f);
	glutSolidSphere(1.0f, 18, 18);
	glPopMatrix();
	glPopMatrix();
}

void drawGround() {
	glPushMatrix();
	glColor3f(0.08f, 0.18f, 0.22f);
	glTranslatef(0.0f, GROUND_Y - 0.015f, 0.0f);
	glScalef(SCENE_HALF * 2.2f, 0.03f, SCENE_HALF * 2.2f);
	glutSolidCube(1.0);
	glPopMatrix();
}

void drawWalls() {
	float r = 0.15f + 0.15f * sinf(wallColorPhase);
	float g = 0.35f + 0.2f * sinf(wallColorPhase + 2.094f);
	float b = 0.5f + 0.2f * sinf(wallColorPhase + 4.188f);
	glColor3f(r, g, b);
	float height = 0.7f;
	float thickness = 0.04f;
	float width = SCENE_HALF * 2.0f;
	glPushMatrix();
	glTranslatef(0.0f, height * 0.5f, -SCENE_HALF);
	glScalef(width, height, thickness);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, height * 0.5f, SCENE_HALF);
	glScalef(width, height, thickness);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-SCENE_HALF, height * 0.5f, 0.0f);
	glScalef(thickness, height, width);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(SCENE_HALF, height * 0.5f, 0.0f);
	glScalef(thickness, height, width);
	glutSolidCube(1.0f);
	glPopMatrix();
}

void drawPlayer() {
	glPushMatrix();
	glTranslatef(player.position.x, player.position.y, player.position.z);
	glRotatef(player.yaw, 0.0f, 1.0f, 0.0f);
	glRotatef(player.tilt, 1.0f, 0.0f, 0.0f);
	glColor3f(0.1f, 0.27f, 0.45f);
	glPushMatrix();
	glTranslatef(0.0f, 0.12f, 0.0f);
	glScalef(0.12f, 0.22f, 0.08f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glColor3f(0.08f, 0.18f, 0.3f);
	glPushMatrix();
	glTranslatef(-0.07f, 0.12f, 0.0f);
	glScalef(0.06f, 0.18f, 0.06f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.07f, 0.12f, 0.0f);
	glScalef(0.06f, 0.18f, 0.06f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.05f, 0.22f, 0.0f);
	glScalef(0.04f, 0.16f, 0.04f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.05f, 0.22f, 0.0f);
	glScalef(0.04f, 0.16f, 0.04f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glColor3f(0.6f, 0.8f, 0.9f);
	glPushMatrix();
	glTranslatef(0.0f, 0.32f, 0.02f);
	glScalef(0.12f, 0.12f, 0.12f);
	glutSolidSphere(0.8f, 20, 20);
	glPopMatrix();
	glColor3f(0.2f, 0.22f, 0.24f);
	glPushMatrix();
	glTranslatef(0.0f, 0.32f, -0.03f);
	glScalef(0.1f, 0.1f, 0.04f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPopMatrix();
}

void drawGoalAt(const Goal &goal) {
	glPushMatrix();
	glTranslatef(goal.position.x, goal.position.y, goal.position.z);
	glRotatef(goalRotation, 0.0f, 1.0f, 0.0f);
	glColor3f(0.9f, 0.85f, 0.2f);
	glPushMatrix();
	glScalef(0.09f, 0.16f, 0.09f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glColor3f(0.2f, 0.45f, 0.9f);
	glPushMatrix();
	glTranslatef(0.0f, 0.08f, 0.0f);
	glScalef(0.07f, 0.07f, 0.07f);
	glutSolidSphere(1.0f, 20, 20);
	glPopMatrix();
	glColor3f(0.75f, 0.3f, 0.2f);
	glPushMatrix();
	glTranslatef(0.0f, 0.16f, 0.0f);
	glScalef(0.02f, 0.14f, 0.02f);
	glutSolidCube(1.0f);
	glPopMatrix();
	glPopMatrix();
}

void drawGoals() {
	for (size_t i = 0; i < goals.size(); ++i) {
		if (!goals[i].collected) {
			drawGoalAt(goals[i]);
		}
	}
}

void drawHudText(float x, float y, const char *text) {
	glRasterPos2f(x, y);
	for (const char *c = text; *c; ++c) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}
}

void drawHud() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	glColor3f(0.9f, 0.95f, 0.98f);
	char info[64];
	snprintf(info, sizeof(info), "Goals: %d", goalsRemaining());
	drawHudText(0.03f, 0.95f, info);
	snprintf(info, sizeof(info), "Time: %02d", (int)ceilf(remainingTime));
	drawHudText(0.03f, 0.9f, info);
	glEnable(GL_LIGHTING);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void drawGameResult() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, 1.0, 0.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	const char *headline = gameState == STATE_WIN ? "GAME WIN" : "GAME LOSE";
	glColor3f(1.0f, 0.95f, 0.6f);
	glRasterPos2f(0.4f, 0.55f);
	for (const char *c = headline; *c; ++c) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}
	glColor3f(0.85f, 0.9f, 0.95f);
	glRasterPos2f(0.25f, 0.45f);
	const char *hint = "Press P to restart";
	for (const char *c = hint; *c; ++c) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}
	glEnable(GL_LIGHTING);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

void drawScene() {
	drawGround();
	drawWalls();
	glPushMatrix();
	glTranslatef(-0.75f, 0.0f, -0.65f);
	drawFloodlight(objectControllers[0].phase * 60.0f);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -0.95f);
	drawAirlock(objectControllers[1].phase);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.68f, 0.0f, -0.35f);
	drawCoralCluster(objectControllers[2].phase);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.55f, 0.0f, 0.55f);
	drawConsole(objectControllers[3].phase);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.45f, 0.0f, 0.75f);
	drawDrone(objectControllers[4].phase);
	glPopMatrix();
	drawGoals();
	drawPlayer();
}

float clampf(float v, float minVal, float maxVal) {
	if (v < minVal) {
		return minVal;
	}
	if (v > maxVal) {
		return maxVal;
	}
	return v;
}

void handlePlayerMovement(float dt) {
	Vector3f direction;
	if (moveForward) {
		direction.z -= 1.0f;
	}
	if (moveBackward) {
		direction.z += 1.0f;
	}
	if (moveLeft) {
		direction.x -= 1.0f;
	}
	if (moveRight) {
		direction.x += 1.0f;
	}
	if (direction.length() > 0.0f) {
		Vector3f dirUnit = direction.unit();
		player.position += dirUnit * (PLAYER_SPEED * dt);
		player.yaw = atan2f(dirUnit.x, -dirUnit.z) * 180.0f / 3.14159265f;
	}
	if (moveUp) {
		player.position.y += PLAYER_ASCEND_SPEED * dt;
	}
	if (moveDown) {
		player.position.y -= PLAYER_ASCEND_SPEED * dt;
	}
	float minY = PLAYER_RADIUS;
	player.position.x = clampf(player.position.x, -SCENE_HALF + PLAYER_RADIUS, SCENE_HALF - PLAYER_RADIUS);
	player.position.z = clampf(player.position.z, -SCENE_HALF + PLAYER_RADIUS, SCENE_HALF - PLAYER_RADIUS);
	player.position.y = clampf(player.position.y, minY, MAX_HEIGHT);
	bool onGround = fabsf(player.position.y - minY) < 0.002f;
	player.airborne = !onGround;
	player.tilt = player.airborne ? -20.0f : 0.0f;
}

void handleGoalCollection() {
	for (size_t i = 0; i < goals.size(); ++i) {
		if (!goals[i].collected) {
			Vector3f diff = player.position - goals[i].position;
			if (diff.length() < GOAL_RADIUS) {
				goals[i].collected = true;
			}
		}
	}
	if (goalsRemaining() == 0 && gameState == STATE_PLAYING) {
		gameState = STATE_WIN;
	}
}

void updateAnimations(float dt) {
	float speed[5] = { 1.2f, 0.9f, 1.6f, 2.0f, 1.4f };
	for (int i = 0; i < 5; ++i) {
		if (objectControllers[i].active) {
			objectControllers[i].phase += dt * speed[i];
		}
	}
}

void updateGame(float dt) {
	if (gameState != STATE_PLAYING) {
		return;
	}
	remainingTime -= dt;
	if (remainingTime <= 0.0f) {
		remainingTime = 0.0f;
		gameState = goalsRemaining() == 0 ? STATE_WIN : STATE_LOSE;
	}
	goalRotation += dt * 50.0f;
	wallColorPhase += dt * 0.7f;
	handlePlayerMovement(dt);
	updateAnimations(dt);
	handleGoalCollection();
}

void Display() {
	setupCamera();
	setupLights();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (gameState == STATE_PLAYING) {
		drawScene();
		drawHud();
	} else {
		drawScene();
		drawGameResult();
	}

	glutSwapBuffers();
}

void toggleAnimation(int index) {
	if (index < 0 || index >= 5) {
		return;
	}
	objectControllers[index].active = !objectControllers[index].active;
}

void Keyboard(unsigned char key, int, int) {
	float d = 0.05f;
	switch (key) {
	case 'w':
		camera.moveY(d);
		break;
	case 's':
		camera.moveY(-d);
		break;
	case 'a':
		camera.moveX(d);
		break;
	case 'd':
		camera.moveX(-d);
		break;
	case 'q':
		camera.moveZ(d);
		break;
	case 'e':
		camera.moveZ(-d);
		break;
	case 'i':
		moveForward = true;
		break;
	case 'k':
		moveBackward = true;
		break;
	case 'j':
		moveLeft = true;
		break;
	case 'l':
		moveRight = true;
		break;
	case 'r':
		moveUp = true;
		break;
	case 'f':
		moveDown = true;
		break;
	case '1':
		setFrontView();
		break;
	case '2':
		setSideView();
		break;
	case '3':
		setTopView();
		break;
	case '0':
		setFreeView();
		break;
	case '5':
		toggleAnimation(0);
		break;
	case '6':
		toggleAnimation(1);
		break;
	case '7':
		toggleAnimation(2);
		break;
	case '8':
		toggleAnimation(3);
		break;
	case '9':
		toggleAnimation(4);
		break;
	case 'p':
	case 'P':
		resetGame();
		break;
	case GLUT_KEY_ESCAPE:
		exit(EXIT_SUCCESS);
	}
}

void KeyboardUp(unsigned char key, int, int) {
	switch (key) {
	case 'i':
		moveForward = false;
		break;
	case 'k':
		moveBackward = false;
		break;
	case 'j':
		moveLeft = false;
		break;
	case 'l':
		moveRight = false;
		break;
	case 'r':
		moveUp = false;
		break;
	case 'f':
		moveDown = false;
		break;
	}
}

void Special(int key, int, int) {
	float a = 1.5f;
	switch (key) {
	case GLUT_KEY_UP:
		camera.rotateX(a);
		break;
	case GLUT_KEY_DOWN:
		camera.rotateX(-a);
		break;
	case GLUT_KEY_LEFT:
		camera.rotateY(a);
		break;
	case GLUT_KEY_RIGHT:
		camera.rotateY(-a);
		break;
	}
}

void setFrontView() {
	camera.eye = Vector3f(0.0f, 0.8f, 2.0f);
	camera.center = Vector3f(0.0f, 0.3f, 0.0f);
	camera.up = Vector3f(0.0f, 1.0f, 0.0f);
}

void setSideView() {
	camera.eye = Vector3f(2.0f, 0.7f, 0.0f);
	camera.center = Vector3f(0.0f, 0.3f, 0.0f);
	camera.up = Vector3f(0.0f, 1.0f, 0.0f);
}

void setTopView() {
	camera.eye = Vector3f(0.0f, 2.2f, 0.0f);
	camera.center = Vector3f(0.0f, 0.0f, 0.0f);
	camera.up = Vector3f(0.0f, 0.0f, -1.0f);
}

void setFreeView() {
	camera.eye = Vector3f(1.8f, 0.9f, 1.8f);
	camera.center = Vector3f(0.0f, 0.3f, 0.0f);
	camera.up = Vector3f(0.0f, 1.0f, 0.0f);
}

void UpdateTimer(int) {
	int now = glutGet(GLUT_ELAPSED_TIME);
	float dt = (now - lastTick) / 1000.0f;
	lastTick = now;
	updateGame(dt);
	glutPostRedisplay();
	glutTimerFunc(16, UpdateTimer, 0);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(50, 50);
	glutCreateWindow("Underwater Base");
	glutDisplayFunc(Display);
	glutKeyboardFunc(Keyboard);
	glutKeyboardUpFunc(KeyboardUp);
	glutSpecialFunc(Special);
	glClearColor(0.02f, 0.08f, 0.12f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	resetGame();
	glutTimerFunc(16, UpdateTimer, 0);
	glutMainLoop();
	return 0;
}
