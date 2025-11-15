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
#include <string>
#if defined(__APPLE__)
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#endif

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

const char *SOUND_TRACK = "assets/audio/Crab Rave Noisestorm.mp3";
const char *SOUND_SERVO = "assets/audio/Mechanical Servo Tremolo by Patrick Lieberkind.wav";
const char *SOUND_GOAL = "assets/audio/Underwater Bubbles by Robinhood76.wav";
const char *SOUND_BUZZER = "assets/audio/Time Running Out Buzzer.wav";

#if defined(__APPLE__)
pid_t backgroundMusicPid = -1;
#endif

bool loseSoundPlayed = false;
bool winSoundPlayed = false;

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
void drawWallPanel(float width, float height, float colorPhase);
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
void startBackgroundMusic();
void stopBackgroundMusic();
void playEffect(const char *path);

void playEffect(const char *path) {
#if defined(__APPLE__)
	if (!path) {
		return;
	}
	std::string command = "afplay -q 1 \"";
	command += path;
	command += "\" >/dev/null 2>&1 &";
	system(command.c_str());
#else
	(void)path;
#endif
}

void stopBackgroundMusic() {
#if defined(__APPLE__)
	if (backgroundMusicPid > 0) {
		kill(backgroundMusicPid, SIGTERM);
		backgroundMusicPid = -1;
	}
#endif
}

void startBackgroundMusic() {
#if defined(__APPLE__)
	stopBackgroundMusic();
	std::string command = "afplay -t 110 -q 1 \"";
	command += SOUND_TRACK;
	command += "\" >/dev/null 2>&1 & echo $!";
	FILE *pipe = popen(command.c_str(), "r");
	if (pipe) {
		char buffer[32];
		if (fgets(buffer, sizeof(buffer), pipe)) {
			backgroundMusicPid = static_cast<pid_t>(atoi(buffer));
		}
		pclose(pipe);
	}
#endif
}

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
	loseSoundPlayed = false;
	winSoundPlayed = false;
	startBackgroundMusic();
	lastTick = glutGet(GLUT_ELAPSED_TIME);
}

void setupLights() {
	// Enhanced material properties for underwater metallic surfaces
	GLfloat ambient[] = { 0.15f, 0.22f, 0.3f, 1.0f };
	GLfloat diffuse[] = { 0.5f, 0.65f, 0.75f, 1.0f };
	GLfloat specular[] = { 0.9f, 0.95f, 1.0f, 1.0f };
	GLfloat shininess[] = { 80.0f };
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, shininess);

	// Main overhead light (cool blue-white)
	GLfloat position0[] = { 0.0f, 1.5f, 0.0f, 1.0f };
	GLfloat lightDiffuse0[] = { 0.6f, 0.75f, 0.95f, 1.0f };
	GLfloat lightSpecular0[] = { 0.8f, 0.9f, 1.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular0);
	glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1.0f);
	glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.3f);

	// Secondary accent light (warm orange from equipment)
	GLfloat position1[] = { -0.7f, 0.4f, -0.6f, 1.0f };
	GLfloat lightDiffuse1[] = { 0.8f, 0.5f, 0.3f, 1.0f };
	glLightfv(GL_LIGHT1, GL_POSITION, position1);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, lightDiffuse1);
	glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.0f);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 1.2f);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.5f);
	glEnable(GL_LIGHT1);

	// Underwater fog effect
	GLfloat fogColor[] = { 0.05f, 0.15f, 0.22f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_START, 1.5f);
	glFogf(GL_FOG_END, 4.0f);
	glFogf(GL_FOG_DENSITY, 0.3f);
	glEnable(GL_FOG);
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
	// Base plate (1)
	glColor3f(0.18f, 0.2f, 0.22f);
	glPushMatrix();
	glScalef(0.18f, 0.04f, 0.18f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Base corners (4)
	glColor3f(0.15f, 0.17f, 0.19f);
	for (int i = 0; i < 4; ++i) {
		glPushMatrix();
		float angle = i * 90.0f;
		float offsetX = 0.07f * cosf(DEG2RAD(angle));
		float offsetZ = 0.07f * sinf(DEG2RAD(angle));
		glTranslatef(offsetX, 0.025f, offsetZ);
		glScalef(0.03f, 0.05f, 0.03f);
		glutSolidCube(1.0);
		glPopMatrix();
	}
	// Main stand (5)
	glColor3f(0.18f, 0.2f, 0.22f);
	glPushMatrix();
	glTranslatef(0.0f, 0.12f, 0.0f);
	glScalef(0.08f, 0.24f, 0.08f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Stand ring detail (6)
	glColor3f(0.3f, 0.35f, 0.4f);
	glPushMatrix();
	glTranslatef(0.0f, 0.15f, 0.0f);
	glutSolidTorus(0.015, 0.055, 12, 16);
	glPopMatrix();
	// Top mounting plate (7)
	glColor3f(0.18f, 0.2f, 0.22f);
	glPushMatrix();
	glTranslatef(0.0f, 0.25f, 0.0f);
	glScalef(0.14f, 0.04f, 0.14f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Rotating mechanism
	glTranslatef(0.0f, 0.27f, 0.0f);
	glRotatef(rotation, 0.0f, 1.0f, 0.0f);
	// Light housing (8)
	glColor3f(0.24f, 0.3f, 0.35f);
	glPushMatrix();
	glScalef(0.12f, 0.06f, 0.2f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Housing side vents (9-10)
	glColor3f(0.15f, 0.2f, 0.25f);
	glPushMatrix();
	glTranslatef(0.065f, 0.0f, 0.05f);
	glScalef(0.015f, 0.05f, 0.06f);
	glutSolidCube(1.0);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.065f, 0.0f, 0.05f);
	glScalef(0.015f, 0.05f, 0.06f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Main lens (11)
	glColor3f(0.65f, 0.85f, 0.9f);
	glPushMatrix();
	glTranslatef(0.0f, 0.01f, 0.08f);
	glScalef(0.08f, 0.06f, 0.08f);
	glutSolidSphere(0.8f, 20, 20);
	glPopMatrix();
	// Lens rim (12)
	glColor3f(0.2f, 0.25f, 0.3f);
	glPushMatrix();
	glTranslatef(0.0f, 0.01f, 0.11f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glutSolidTorus(0.008, 0.045, 10, 16);
	glPopMatrix();
	glPopMatrix();
}

void drawAirlock(float doorPhase) {
	float openOffset = 0.16f * (0.5f + 0.5f * sinf(doorPhase));
	glPushMatrix();
	// Left frame pillar (1)
	glColor3f(0.25f, 0.3f, 0.35f);
	glPushMatrix();
	glTranslatef(-0.22f, 0.3f, 0.0f);
	glScalef(0.08f, 0.6f, 0.4f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Right frame pillar (2)
	glPushMatrix();
	glTranslatef(0.22f, 0.3f, 0.0f);
	glScalef(0.08f, 0.6f, 0.4f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Top frame (3)
	glPushMatrix();
	glTranslatef(0.0f, 0.6f, 0.0f);
	glScalef(0.44f, 0.06f, 0.4f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Frame reinforcement bolts (4-7)
	glColor3f(0.4f, 0.45f, 0.5f);
	float boltPositions[4][2] = {{-0.22f, 0.55f}, {0.22f, 0.55f}, {-0.22f, 0.05f}, {0.22f, 0.05f}};
	for (int i = 0; i < 4; ++i) {
		glPushMatrix();
		glTranslatef(boltPositions[i][0], boltPositions[i][1], 0.21f);
		glScalef(0.025f, 0.025f, 0.02f);
		glutSolidCube(1.0);
		glPopMatrix();
	}
	// Left door panel (8)
	glColor3f(0.35f, 0.52f, 0.6f);
	glPushMatrix();
	glTranslatef(-openOffset, 0.3f, 0.0f);
	glScalef(0.16f, 0.5f, 0.32f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Left door window (9)
	glColor3f(0.5f, 0.75f, 0.85f);
	glPushMatrix();
	glTranslatef(-openOffset, 0.35f, 0.165f);
	glScalef(0.1f, 0.2f, 0.02f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Right door panel (10)
	glColor3f(0.35f, 0.52f, 0.6f);
	glPushMatrix();
	glTranslatef(openOffset, 0.3f, 0.0f);
	glScalef(0.16f, 0.5f, 0.32f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Right door window (11)
	glColor3f(0.5f, 0.75f, 0.85f);
	glPushMatrix();
	glTranslatef(openOffset, 0.35f, 0.165f);
	glScalef(0.1f, 0.2f, 0.02f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Bottom seal (12)
	glColor3f(0.18f, 0.22f, 0.26f);
	glPushMatrix();
	glTranslatef(0.0f, 0.05f, 0.0f);
	glScalef(0.42f, 0.1f, 0.08f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Control panel (13)
	glColor3f(0.2f, 0.25f, 0.3f);
	glPushMatrix();
	glTranslatef(-0.3f, 0.25f, 0.18f);
	glScalef(0.06f, 0.12f, 0.06f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Status lights (14-15)
	glPushMatrix();
	glTranslatef(-0.3f, 0.3f, 0.22f);
	glColor3f(0.2f, 0.8f, 0.3f);
	glScalef(0.02f, 0.02f, 0.02f);
	glutSolidSphere(1.0f, 12, 12);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(-0.3f, 0.27f, 0.22f);
	glColor3f(0.9f, 0.3f, 0.2f);
	glScalef(0.02f, 0.02f, 0.02f);
	glutSolidSphere(1.0f, 12, 12);
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
	float spinPhase = bobPhase * 8.0f;
	glPushMatrix();
	glTranslatef(0.0f, 0.16f + bob, 0.0f);
	// Main body (1)
	glColor3f(0.65f, 0.2f, 0.3f);
	glPushMatrix();
	glScalef(0.16f, 0.08f, 0.16f);
	glutSolidSphere(1.0f, 22, 22);
	glPopMatrix();
	// Body band detail (2)
	glColor3f(0.5f, 0.15f, 0.25f);
	glPushMatrix();
	glutSolidTorus(0.012, 0.09, 12, 20);
	glPopMatrix();
	// Rotor arms (3-6)
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
	// Rotor propellers (7-10)
	glColor3f(0.3f, 0.35f, 0.4f);
	float rotorPos[4][2] = {{0.2f, 0.0f}, {-0.2f, 0.0f}, {0.0f, 0.2f}, {0.0f, -0.2f}};
	for (int i = 0; i < 4; ++i) {
		glPushMatrix();
		glTranslatef(rotorPos[i][0], 0.02f, rotorPos[i][1]);
		glRotatef(spinPhase * (i % 2 == 0 ? 1.0f : -1.0f), 0.0f, 1.0f, 0.0f);
		glScalef(0.08f, 0.01f, 0.08f);
		glutSolidCube(1.0);
		glPopMatrix();
	}
	// Top sensor dome (11)
	glColor3f(0.9f, 0.5f, 0.6f);
	glPushMatrix();
	glTranslatef(0.0f, 0.05f, 0.0f);
	glScalef(0.08f, 0.02f, 0.08f);
	glutSolidSphere(1.0f, 18, 18);
	glPopMatrix();
	// Front sensor (12)
	glColor3f(0.15f, 0.7f, 0.8f);
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, 0.09f);
	glScalef(0.04f, 0.04f, 0.04f);
	glutSolidSphere(1.0f, 16, 16);
	glPopMatrix();
	// Antenna mast (13)
	glColor3f(0.25f, 0.28f, 0.32f);
	glPushMatrix();
	glTranslatef(0.0f, 0.08f, 0.0f);
	glScalef(0.015f, 0.06f, 0.015f);
	glutSolidCube(1.0);
	glPopMatrix();
	// Antenna tip (14)
	glColor3f(0.9f, 0.7f, 0.2f);
	glPushMatrix();
	glTranslatef(0.0f, 0.12f, 0.0f);
	glScalef(0.02f, 0.02f, 0.02f);
	glutSolidSphere(1.0f, 12, 12);
	glPopMatrix();
	// Bottom light (15)
	glColor3f(0.9f, 0.95f, 0.3f);
	glPushMatrix();
	glTranslatef(0.0f, -0.05f, 0.0f);
	glScalef(0.025f, 0.015f, 0.025f);
	glutSolidSphere(1.0f, 14, 14);
	glPopMatrix();
	glPopMatrix();
}

void drawGround() {
	glPushMatrix();
	glTranslatef(0.0f, GROUND_Y - 0.01f, 0.0f);
	
	// Main seabed floor with grid pattern
	int gridSize = 20;
	float tileSize = (SCENE_HALF * 2.2f) / gridSize;
	for (int i = 0; i < gridSize; ++i) {
		for (int j = 0; j < gridSize; ++j) {
			float x = -SCENE_HALF * 1.1f + i * tileSize;
			float z = -SCENE_HALF * 1.1f + j * tileSize;
			float noise = sinf(i * 0.5f) * cosf(j * 0.4f) * 0.005f;
			
			// Varying tile colors for depth
			float colorVar = 0.9f + 0.1f * sinf((i + j) * 0.3f);
			glColor3f(0.06f * colorVar, 0.14f * colorVar, 0.18f * colorVar);
			
			glPushMatrix();
			glTranslatef(x + tileSize * 0.5f, noise, z + tileSize * 0.5f);
			glBegin(GL_QUADS);
			glNormal3f(0.0f, 1.0f, 0.0f);
			glVertex3f(-tileSize * 0.48f, 0.0f, -tileSize * 0.48f);
			glVertex3f(tileSize * 0.48f, 0.0f, -tileSize * 0.48f);
			glVertex3f(tileSize * 0.48f, 0.0f, tileSize * 0.48f);
			glVertex3f(-tileSize * 0.48f, 0.0f, tileSize * 0.48f);
			glEnd();
			glPopMatrix();
		}
	}
	
	// Grid lines for detail
	glDisable(GL_LIGHTING);
	glLineWidth(1.0f);
	glColor3f(0.12f, 0.25f, 0.3f);
	glBegin(GL_LINES);
	for (int i = 0; i <= gridSize; ++i) {
		float pos = -SCENE_HALF * 1.1f + i * tileSize;
		glVertex3f(pos, 0.002f, -SCENE_HALF * 1.1f);
		glVertex3f(pos, 0.002f, SCENE_HALF * 1.1f);
		glVertex3f(-SCENE_HALF * 1.1f, 0.002f, pos);
		glVertex3f(SCENE_HALF * 1.1f, 0.002f, pos);
	}
	glEnd();
	glEnable(GL_LIGHTING);
	
	glPopMatrix();
}

void drawWallPanel(float width, float height, float colorPhase) {
	float r = 0.18f + 0.12f * sinf(colorPhase);
	float g = 0.38f + 0.18f * sinf(colorPhase + 2.094f);
	float b = 0.52f + 0.18f * sinf(colorPhase + 4.188f);
	
	int panels = 5;
	float panelWidth = width / panels;
	float panelHeight = height / 3.0f;
	
	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < panels; ++col) {
			float px = -width * 0.5f + col * panelWidth + panelWidth * 0.5f;
			float py = row * panelHeight + panelHeight * 0.5f;
			
			// Panel plate with slight color variation
			float variation = 0.95f + 0.05f * sinf((row + col) * 1.2f);
			glColor3f(r * variation, g * variation, b * variation);
			glPushMatrix();
			glTranslatef(px, py, 0.015f);
			glScalef(panelWidth * 0.92f, panelHeight * 0.9f, 0.025f);
			glutSolidCube(1.0f);
			glPopMatrix();
			
			// Panel frame
			glColor3f(r * 0.6f, g * 0.6f, b * 0.6f);
			glPushMatrix();
			glTranslatef(px, py, 0.005f);
			glScalef(panelWidth * 0.96f, panelHeight * 0.94f, 0.015f);
			glutSolidCube(1.0f);
			glPopMatrix();
			
			// Rivets at corners
			glColor3f(0.4f, 0.45f, 0.5f);
			float rivetPos[4][2] = {
				{-panelWidth * 0.42f, -panelHeight * 0.4f},
				{panelWidth * 0.42f, -panelHeight * 0.4f},
				{-panelWidth * 0.42f, panelHeight * 0.4f},
				{panelWidth * 0.42f, panelHeight * 0.4f}
			};
			for (int i = 0; i < 4; ++i) {
				glPushMatrix();
				glTranslatef(px + rivetPos[i][0], py + rivetPos[i][1], 0.025f);
				glutSolidSphere(0.008f, 8, 8);
				glPopMatrix();
			}
		}
	}
}

void drawWalls() {
	float height = 0.7f;
	float thickness = 0.05f;
	float width = SCENE_HALF * 2.0f;
	
	// Back wall (-Z)
	glPushMatrix();
	glTranslatef(0.0f, height * 0.5f, -SCENE_HALF);
	drawWallPanel(width, height, wallColorPhase);
	glPopMatrix();
	
	// Front wall (+Z)
	glPushMatrix();
	glTranslatef(0.0f, height * 0.5f, SCENE_HALF);
	glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
	drawWallPanel(width, height, wallColorPhase + 1.5f);
	glPopMatrix();
	
	// Left wall (-X)
	glPushMatrix();
	glTranslatef(-SCENE_HALF, height * 0.5f, 0.0f);
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
	drawWallPanel(width, height, wallColorPhase + 3.0f);
	glPopMatrix();
	
	// Right wall (+X)
	glPushMatrix();
	glTranslatef(SCENE_HALF, height * 0.5f, 0.0f);
	glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
	drawWallPanel(width, height, wallColorPhase + 4.5f);
	glPopMatrix();
}

void drawPlayer() {
	glPushMatrix();
	glTranslatef(player.position.x, player.position.y, player.position.z);
	glRotatef(player.yaw, 0.0f, 1.0f, 0.0f);
	glRotatef(player.tilt, 1.0f, 0.0f, 0.0f);
	
	// Torso (wetsuit body)
	glColor3f(0.12f, 0.3f, 0.5f);
	glPushMatrix();
	glTranslatef(0.0f, 0.13f, 0.0f);
	glScalef(0.1f, 0.18f, 0.07f);
	glutSolidSphere(1.0f, 20, 20);
	glPopMatrix();
	
	// Torso equipment harness
	glColor3f(0.15f, 0.15f, 0.18f);
	glPushMatrix();
	glTranslatef(0.0f, 0.15f, 0.055f);
	glScalef(0.08f, 0.14f, 0.02f);
	glutSolidCube(1.0f);
	glPopMatrix();
	
	// Legs (upper)
	glColor3f(0.1f, 0.25f, 0.42f);
	glPushMatrix();
	glTranslatef(-0.035f, 0.05f, 0.0f);
	glRotatef(-5.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.03f, 0.1f, 0.03f);
	glutSolidSphere(1.0f, 16, 16);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.035f, 0.05f, 0.0f);
	glRotatef(5.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.03f, 0.1f, 0.03f);
	glutSolidSphere(1.0f, 16, 16);
	glPopMatrix();
	
	// Arms (shoulders to elbows)
	glColor3f(0.1f, 0.25f, 0.42f);
	glPushMatrix();
	glTranslatef(-0.08f, 0.18f, 0.0f);
	glRotatef(-15.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.025f, 0.08f, 0.025f);
	glutSolidSphere(1.0f, 16, 16);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.08f, 0.18f, 0.0f);
	glRotatef(15.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.025f, 0.08f, 0.025f);
	glutSolidSphere(1.0f, 16, 16);
	glPopMatrix();
	
	// Arms (elbows to hands)
	glPushMatrix();
	glTranslatef(-0.09f, 0.1f, 0.0f);
	glRotatef(-10.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.022f, 0.07f, 0.022f);
	glutSolidSphere(1.0f, 14, 14);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.09f, 0.1f, 0.0f);
	glRotatef(10.0f, 0.0f, 0.0f, 1.0f);
	glScalef(0.022f, 0.07f, 0.022f);
	glutSolidSphere(1.0f, 14, 14);
	glPopMatrix();
	
	// Helmet (glass dome)
	glColor3f(0.55f, 0.75f, 0.85f);
	glPushMatrix();
	glTranslatef(0.0f, 0.28f, 0.01f);
	glutSolidSphere(0.065f, 24, 24);
	glPopMatrix();
	
	// Helmet ring collar
	glColor3f(0.3f, 0.32f, 0.35f);
	glPushMatrix();
	glTranslatef(0.0f, 0.23f, 0.0f);
	glutSolidTorus(0.015f, 0.07f, 12, 20);
	glPopMatrix();
	
	// Backpack/air tank
	glColor3f(0.25f, 0.27f, 0.3f);
	glPushMatrix();
	glTranslatef(0.0f, 0.16f, -0.06f);
	glScalef(0.06f, 0.12f, 0.04f);
	glutSolidSphere(1.0f, 16, 16);
	glPopMatrix();
	
	// Face behind visor (darker)
	glDisable(GL_LIGHTING);
	glColor4f(0.15f, 0.12f, 0.1f, 0.6f);
	glPushMatrix();
	glTranslatef(0.0f, 0.28f, 0.035f);
	glScalef(0.04f, 0.05f, 0.03f);
	glutSolidSphere(1.0f, 12, 12);
	glPopMatrix();
	glEnable(GL_LIGHTING);
	
	glPopMatrix();
}

void drawGoalAt(const Goal &goal) {
	glPushMatrix();
	glTranslatef(goal.position.x, goal.position.y, goal.position.z);
	glRotatef(goalRotation, 0.0f, 1.0f, 0.0f);
	
	float pulse = 1.0f + 0.15f * sinf(goalRotation * 0.1f);
	
	// Outer containment cylinder
	glColor3f(0.3f, 0.35f, 0.4f);
	glPushMatrix();
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	GLUquadric* quad1 = gluNewQuadric();
	gluCylinder(quad1, 0.06f, 0.06f, 0.18f, 20, 4);
	gluDeleteQuadric(quad1);
	glPopMatrix();
	
	// Top and bottom caps
	glPushMatrix();
	glTranslatef(0.0f, 0.09f, 0.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	glutSolidCone(0.062f, 0.02f, 20, 1);
	glPopMatrix();
	glPushMatrix();
	glTranslatef(0.0f, -0.09f, 0.0f);
	glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
	glutSolidCone(0.062f, 0.02f, 20, 1);
	glPopMatrix();
	
	// Glowing energy core (pulsing)
	glDisable(GL_LIGHTING);
	glColor4f(0.2f, 0.7f, 0.95f, 0.8f);
	glPushMatrix();
	glScalef(pulse, pulse, pulse);
	glutSolidSphere(0.045f, 24, 24);
	glPopMatrix();
	
	// Inner energy glow
	glColor4f(0.4f, 0.85f, 1.0f, 0.5f);
	glPushMatrix();
	glScalef(pulse * 1.2f, pulse * 1.2f, pulse * 1.2f);
	glutSolidSphere(0.055f, 20, 20);
	glPopMatrix();
	glEnable(GL_LIGHTING);
	
	// Support stand
	glColor3f(0.25f, 0.28f, 0.32f);
	glPushMatrix();
	glTranslatef(0.0f, -0.12f, 0.0f);
	glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
	GLUquadric* quad2 = gluNewQuadric();
	gluCylinder(quad2, 0.025f, 0.025f, 0.04f, 12, 2);
	gluDeleteQuadric(quad2);
	glPopMatrix();
	
	// Base platform
	glPushMatrix();
	glTranslatef(0.0f, -0.14f, 0.0f);
	glScalef(0.08f, 0.015f, 0.08f);
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
	float wallThickness = 0.03f;  // Account for wall panel thickness
	player.position.x = clampf(player.position.x, -SCENE_HALF + PLAYER_RADIUS + wallThickness, SCENE_HALF - PLAYER_RADIUS - wallThickness);
	player.position.z = clampf(player.position.z, -SCENE_HALF + PLAYER_RADIUS + wallThickness, SCENE_HALF - PLAYER_RADIUS - wallThickness);
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
				playEffect(SOUND_GOAL);
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
	
	// At 10 seconds remaining, play the buzzer
	if (remainingTime <= 10.0f && remainingTime > 9.9f && !loseSoundPlayed) {
		playEffect(SOUND_BUZZER);
		loseSoundPlayed = true;
	}
	
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
	playEffect(SOUND_SERVO);
}

void toggleAllAnimations() {
	for (int i = 0; i < 5; ++i) {
		objectControllers[i].active = true;
	}
	playEffect(SOUND_SERVO);
}

void stopAllAnimations() {
	for (int i = 0; i < 5; ++i) {
		objectControllers[i].active = false;
	}
	playEffect(SOUND_SERVO);
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
		toggleAllAnimations();
		break;
	case '6':
		stopAllAnimations();
		break;
	case 'p':
	case 'P':
		resetGame();
		break;
	case GLUT_KEY_ESCAPE:
		stopBackgroundMusic();
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
	glClearColor(0.03f, 0.12f, 0.18f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#if defined(__APPLE__)
	atexit(stopBackgroundMusic);
#endif
	resetGame();
	glutTimerFunc(16, UpdateTimer, 0);
	glutMainLoop();
	return 0;
}
