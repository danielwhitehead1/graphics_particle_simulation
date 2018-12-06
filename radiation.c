/*
==========================================================================
File:        ex2.c (skeleton)
Authors:     Toby Howard
==========================================================================
*/

/* The following ratios are not to scale: */
/* Moon orbit : planet orbit */
/* Orbit radius : body radius */
/* Sun radius : planet radius */

#include <GL/glut.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define RUN_SPEED  1
#define TURN_ANGLE 4.0
#define DEG_TO_RAD 0.017453293

#define MAX_PARTICLES 1100000
#define MAX_EMITTERS 3
#define ONE_MILLION_SPEED 9804

#define ALPHA 0
#define BETA 1
#define GAMMA 2

typedef int bool;
#define true 1
#define false 0

GLdouble lat,     lon;              /* View angles (degrees)    */
GLdouble mlat,    mlon;             /* Mouse look offset angles */   
GLfloat  eyex,    eyey,    eyez;    /* Eye point                */
GLfloat  centerx, centery, centerz; /* Look point               */
GLfloat  upx,     upy,     upz;     /* View up vector           */

struct Emitter {
  float xFrom, xTo, yFrom, yTo, zFrom, zTo;
  float xCentre, yCentre, zCentre;
  float size;
  float charge;
  float vX, vY, vZ;
};

struct Particle {
  float x, y, z; //Position
  float vX, vY, vZ; //Velocity
  unsigned char r,g,b,a; // Color
  float life; // Remaining life of the particle. if < 0 : dead and unused.
  int source;
  float m; // Particle mass
};

int showAxis;

/* Particle variables */
struct Particle particle;
struct Particle particles[MAX_EMITTERS][MAX_PARTICLES];
struct Particle alphaParticles[MAX_PARTICLES];
struct Emitter emitters[MAX_EMITTERS];
int totalParticles[MAX_EMITTERS];
int lastUsedParticle[MAX_EMITTERS];
int emitSpeed;

/* Variables for calculating change in velocity */
int signX;
int signY;
float theeta;
float lambda;
float row;
float v;
char *resultSign;
float F;

/* Variables to keep track of timechanges */
int startTime[MAX_EMITTERS];
float timeElapsed[MAX_EMITTERS];
float oldTimeElapsed[MAX_EMITTERS];
float deltaTime[MAX_EMITTERS];

/* Emitter variables */
float emitterCentre[MAX_EMITTERS][3];
float emitterSize[MAX_EMITTERS];
float emitterCharge[MAX_EMITTERS];
float particleMass[MAX_EMITTERS];
float emitterXSpeed[MAX_EMITTERS];
float emitterYSpeed[MAX_EMITTERS];
float emitterZSpeed[MAX_EMITTERS];
char sign[2][2][2];

float B; /* Magnetic field charge */
bool FIELD_ACTIVE;
float xAcceleration;
float zAcceleration;
float xAbsAcceleration;
float zAbsAcceleration;

bool RAND_X;
float randomX;

float myRand();
void init();
void menu(int menuentry);
void animate();
void drawAxis();
void display();
void reshape(int w, int h);
void renderParticles();
void createMenu();
void keyboard(unsigned char key, int x, int y);
void changeInTime();
void animateParticles();
void emitParticles();
void initEmitters();
void renderEmitters();
void resetParticles();
void accelerateParticle(float vX, float vZ, int emitter, int i, float q);
void calculateForce(float vX, float vZ);
void initParticle(int i, int emitter);
void initParticles(int emitter);
int findUnusedParticle(int emitter);

float myRand (void)
{
  return ((float) rand() / RAND_MAX);
}

void initParticle(int i, int emitter) {
  particles[emitter][i].x = emitters[emitter].xTo;
  particles[emitter][i].y =  myRand() * ( emitters[emitter].yTo - emitters[emitter].yFrom ) + emitters[emitter].yFrom;
  particles[emitter][i].z = myRand() * ( emitters[emitter].zTo - emitters[emitter].zFrom ) + emitters[emitter].zFrom;
  if(RAND_X) { randomX = myRand(); }
  else { randomX = 1.0f; }
  particles[emitter][i].vX = randomX * emitters[emitter].vX;
  particles[emitter][i].vY = emitters[emitter].vY;
  particles[emitter][i].vZ = emitters[emitter].vZ;
  particles[emitter][i].source = emitter;
  particles[emitter][i].m = particleMass[emitter];
  particles[emitter][i].life = -1.0f;
}

void initParticles(int emitter) {
  lastUsedParticle[emitter] = 0;
  totalParticles[emitter] = 0;

  int i;
  srand(time(NULL));
  for(i = 0; i < MAX_PARTICLES; i++) {
   initParticle(i, emitter);
  }
}

void createMenu() {
  glutCreateMenu(menu);
  glutAddMenuEntry("Toggle Alpha", 1);
  glutAddMenuEntry("Toggle Beta", 2);
  glutAddMenuEntry("Toggle Gamma", 3);
  glutAddMenuEntry("Quit", 4);
  glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void initEmitters() {
  int emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    emitters[emitter].xFrom = emitterCentre[emitter][0] - emitterSize[emitter] / 2; /* These should be controlled by variables so that can be changed */
    emitters[emitter].xTo = emitterCentre[emitter][0] + emitterSize[emitter] / 2;
    emitters[emitter].yFrom = emitterCentre[emitter][1] - emitterSize[emitter] / 2;
    emitters[emitter].yTo = emitterCentre[emitter][1] + emitterSize[emitter] / 2;
    emitters[emitter].zFrom = emitterCentre[emitter][2] - emitterSize[emitter] / 2;
    emitters[emitter].zTo = emitterCentre[emitter][2] + emitterSize[emitter] / 2;
    emitters[emitter].xCentre = emitterCentre[emitter][0];
    emitters[emitter].yCentre = emitterCentre[emitter][1];
    emitters[emitter].zCentre = emitterCentre[emitter][2];
    emitters[emitter].size = emitterSize[emitter];
    emitters[emitter].charge = emitterCharge[emitter];
    emitters[emitter].vX = emitterXSpeed[emitter];
    emitters[emitter].vY = emitterYSpeed[emitter];
    emitters[emitter].vZ = emitterZSpeed[emitter];
  }
}

void init(void)
{
  /* Define background colour */
  glClearColor(204.0/255.0, 229.0/255.0, 255.0/255.0, 0.0);


  /* Set initial view parameters */
  eyex = 60.0; /* Set eyepoint at eye height within the scene */
  eyey = 30.0;
  eyez = 60.0;

  upx = 0.0;   /* Set up direction to the +Y axis  */
  upy = 1.0;
  upz = 0.0;

  lat = 114.0;   /* Look at 0,0,0 from start point */
  lon = -135.0;
  strcpy(sign[0][0], "01");
  strcpy(sign[0][1], "11");
  strcpy(sign[1][0], "00");
  strcpy(sign[1][1], "10");
  
  emitterSize[ALPHA] = 0.5f;
  emitterCharge[ALPHA] = 1.0f;
  emitterCentre[ALPHA][0] = -1.0f;
  emitterCentre[ALPHA][1] = 0.0f;
  emitterCentre[ALPHA][2] = 3.0f;
  particleMass[ALPHA] = 1.0f;
  emitterXSpeed[ALPHA] = 10.0f;
  emitterYSpeed[ALPHA] = 0.0f;
  emitterZSpeed[ALPHA] = 0.0f;

  emitterSize[BETA] = 0.5f;
  emitterCharge[BETA] = -1.0f;
  emitterCentre[BETA][0] = -1.0f;
  emitterCentre[BETA][1] = 0.0f;
  emitterCentre[BETA][2] = 15.0f;
  particleMass[BETA] = 0.1f;
  emitterXSpeed[BETA] = 10.0f;
  emitterYSpeed[BETA] = 0.0f;
  emitterZSpeed[BETA] = 0.0f;

  emitterSize[GAMMA] = 0.5f;
  emitterCharge[GAMMA] = 0.0f;
  emitterCentre[GAMMA][0] = -1.0f;
  emitterCentre[GAMMA][1] = 0.0f;
  emitterCentre[GAMMA][2] = 9.0f;
  particleMass[GAMMA] = 0.0001f;
  emitterXSpeed[GAMMA] = 10.0f;
  emitterYSpeed[GAMMA] = 0.0f;
  emitterZSpeed[GAMMA] = 0.0f;

  B = 1.0f;
  FIELD_ACTIVE = false;
  
  initEmitters();
  int emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    initParticles(emitter);
    oldTimeElapsed[emitter] = 0;
    startTime[emitter] = 0;
  }

  showAxis = 1;
  emitSpeed = 1;
  createMenu();
}


void menu(int menuentry) {
  switch (menuentry) { 
  case 1: 
    if(emitters[ALPHA].size > 0) { 
      emitters[ALPHA].size = 0;
    }
    else { 
      emitters[ALPHA].size = emitterSize[ALPHA];
      startTime[ALPHA] = glutGet(GLUT_ELAPSED_TIME);
      oldTimeElapsed[ALPHA] = 0;
      initParticles(ALPHA);
    };
    break;
  case 2: 
    if(emitters[BETA].size > 0) { emitters[BETA].size = 0; }
    else { 
      emitters[BETA].size = emitterSize[BETA];
      startTime[BETA] = glutGet(GLUT_ELAPSED_TIME);
      oldTimeElapsed[BETA] = 0;
      initParticles(BETA);
    }; break; 
  case 3: 
    if(emitters[GAMMA].size > 0) { emitters[GAMMA].size = 0; }
    else {
      emitters[GAMMA].size = emitterSize[GAMMA];
      startTime[GAMMA] = glutGet(GLUT_ELAPSED_TIME);
      oldTimeElapsed[GAMMA] = 0;
      initParticles(GAMMA);
    }; break;              
  case 4: exit(0);
  }
}

void changeInTime(int emitter) {
  timeElapsed[emitter] = glutGet(GLUT_ELAPSED_TIME) - startTime[emitter];
  deltaTime[emitter] = (timeElapsed[emitter] - oldTimeElapsed[emitter]) / 1000;
  oldTimeElapsed[emitter] = timeElapsed[emitter];
}

void calculateForce(float vX, float vZ) {
  v = sqrt(pow(vX, 2) + pow(vZ, 2));
  theeta = atan(vZ/vX);
  lambda = (M_PI / 2.0f) - fabs(theeta);
  signX = 1;
  if(vX < 0.0f) { signX = 0; };
  signY = 1;
  if(vZ < 0.0f) { signY = 0; };
  resultSign = sign[signX][signY];
  F = v * B;
}

void accelerateParticle(float vX, float vZ, int emitter, int i, float q) {
  calculateForce(vX, vZ);

  xAbsAcceleration = fabs(cos(lambda) * (F / particles[emitter][i].m));
  zAbsAcceleration = fabs(sin(lambda) * (F / particles[emitter][i].m));
  if(resultSign[0] == '1') { xAcceleration = xAbsAcceleration; }
  else { xAcceleration = -1 * xAbsAcceleration; }
  if(resultSign[1] == '1') { zAcceleration = zAbsAcceleration; }
  else { zAcceleration = -1 * zAbsAcceleration; }
  vX += q * xAcceleration * deltaTime[emitter];
  vZ += q * zAcceleration * deltaTime[emitter];
  signX = 1;
  if(vX < 0.0f) { signX = -1; };
  signY = 1;
  if(vZ < 0.0f) { signY = -1; };
  row = atan(vZ/vX);
  float resultingvX = signX * fabs(cos(row) * v); 
  float resultingvZ = signY * fabs(sin(row) * v);
  particles[emitter][i].vX = resultingvX;
  particles[emitter][i].vZ = resultingvZ;
}

void animateParticles() {
  int i, emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    for(i = 0; i < totalParticles[emitter] + 50000 || i < MAX_PARTICLES; i++) {
      if(particles[emitter][i].life > 0) {
        if(FIELD_ACTIVE) { accelerateParticle(particles[emitter][i].vX, particles[emitter][i].vZ, emitter, i, emitters[emitter].charge); }
        particles[emitter][i].x += particles[emitter][i].vX * deltaTime[emitter];
        particles[emitter][i].y += particles[emitter][i].vY * deltaTime[emitter];
        particles[emitter][i].z += particles[emitter][i].vZ * deltaTime[emitter];
        particles[emitter][i].life -= deltaTime[emitter] / 3;
        if(particles[emitter][i].life <= 0) {
          totalParticles[emitter]--;
          initParticle(i, emitter);
        }
      }
    }
  }
}

void animate(int i)
{
  int emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    changeInTime(emitter);
  }
  animateParticles();

  glutPostRedisplay();
  glutTimerFunc(1000.0/60.0, animate, 0);
}

void drawAxis() {
  float LEN = 100.0;

   glLineWidth(0.01);

   glBegin(GL_LINES);
   glColor3f(1.0,0.0,0.0); // red
       glVertex3f(0.0, 0.0, 0.0);
       glVertex3f(LEN, 0.0, 0.0);

   glColor3f(0.0,1.0,0.0); // green
       glVertex3f(0.0, 0.0, 0.0);
       glVertex3f(0.0, LEN, 0.0);

   glColor3f(0.0,0.0,1.0); // blue
       glVertex3f(0.0, 0.0, 0.0);
       glVertex3f(0.0, 0.0, LEN);
   glEnd();

   glLineWidth(1.0);
}

  int j = 0;

int findUnusedParticle(int emitter) {
  int i;
  for(i = lastUsedParticle[emitter]; i < totalParticles[emitter]; i++) {
    if (particles[emitter][i].life < 0) {
      lastUsedParticle[emitter] = i;
      return i;
    }
  }

  for(i = 0; i < lastUsedParticle[emitter]; i++) {
    if (particles[emitter][i].life < 0) {
      lastUsedParticle[emitter] = i;
      return i;
    }
  }
  lastUsedParticle[emitter] = 0;
  return 0; // All particles are taken, override the first one
}

void emitParticles() {
  int i, emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    for(i = 0; i < emitSpeed; i++) {
      if(emitters[emitter].size > 0) {
        int unusedIndex = findUnusedParticle(emitter);
        initParticle(unusedIndex, emitter);
        particles[emitter][unusedIndex].life = 3.0f;
        totalParticles[emitter]++;
      }
    }
  }
}

void renderParticles() {
  glPointSize(3.0f);
  glBegin(GL_POINTS);
    int i;
    for(i = 0; i < totalParticles[ALPHA] + 50000 || i < MAX_PARTICLES; i++) {
      if(particles[ALPHA][i].life > 0) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        glVertex3f(particles[ALPHA][i].x, particles[ALPHA][i].y, particles[ALPHA][i].z);
      }
    }
  glEnd();

  glLineWidth(2.5); 
  glColor3f(1.0, 0.0, 0.0);
  glBegin(GL_LINES);
  for(i = 0; i < totalParticles[GAMMA] + 50000 || i < MAX_PARTICLES; i++) {
      if(particles[GAMMA][i].life > 0) {
        glVertex3f(particles[GAMMA][i].x, particles[GAMMA][i].y, particles[GAMMA][i].z);
        glVertex3f(particles[GAMMA][i].x + 0.2, particles[GAMMA][i].y, particles[GAMMA][i].z);
      }
    }
  glEnd();

  glBegin(GL_QUADS);
  float x,y,z;
  float s = 0.05;
  for(i = 0; i < totalParticles[BETA] + 50000 || i < MAX_PARTICLES; i++) {
      if(particles[BETA][i].life > 0) {
        x = particles[BETA][i].x;
        y = particles[BETA][i].y;
        z = particles[BETA][i].z;
        // top
        glColor3f(169/250.0f, 169/250.0f, 169/250.0f);
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(x, y - s, z - s);
        glVertex3f(x, y - s, z);
        glVertex3f(x - s, y - s, z);
        glVertex3f(x - s, y - s, z - s);
       
        // front
        glColor3f(128/250.0f, 128/250.0f, 128/250.0f);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(x - s, y, z);
        glVertex3f(x - s, y, z - s);
        glVertex3f(x - s, y - s, z - s);
        glVertex3f(x - s, y - s, z);

       
        // right
        glColor3f(192/250.0f, 192/250.0f, 192/250.0f);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(x - s, y, z - s);
        glVertex3f(x, y, z - s);
        glVertex3f(x, y - s, z - s);
        glVertex3f(x - s, y - s, z - s);

        // left
        glColor3f(192/250.0f, 192/250.0f, 192/250.0f);
        glNormal3f(0.0f, 0.0f, 1.0f);
        glVertex3f(x, y, z);
        glVertex3f(x - s, y, z);
        glVertex3f(x - s, y - s, z);
        glVertex3f(x, y - s, z);

        // bottom
        glColor3f(169/250.0f, 169/250.0f, 169/250.0f);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(x, y, z);
        glVertex3f(x, y, z - s);
        glVertex3f(x - s, y, z - s);
        glVertex3f(x - s, y, z);

        // back
        glColor3f(128/250.0f, 128/250.0f, 128/250.0f);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(x, y, z - s);
        glVertex3f(x, y , z);
        glVertex3f(x, y - s, z);
        glVertex3f(x, y - s, z - s);
      }
    }
  glEnd();

  glFlush();
}

void renderEmitters() {
  int emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    if(emitters[emitter].size > 0) {
      glColor3f(emitter / 3.0, emitter / 4.0, emitter / 5.0);
      glPushMatrix();
      glTranslatef(emitters[emitter].xCentre,emitters[emitter].yCentre,emitters[emitter].zCentre);
      glutSolidCube(emitters[emitter].size);
      glPopMatrix();
    }
  }
}

void calculateLookpoint(void) { 
  centerx = eyex + sin(DEG_TO_RAD*(lon));
  centery = eyey + cos(DEG_TO_RAD*(lat));
  centerz = eyez + cos(DEG_TO_RAD*(lon));
}

void display(void)
{
  glClear(GL_COLOR_BUFFER_BIT);

  calculateLookpoint(); /* Compute the centre of interest   */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);

  if(showAxis == 1) drawAxis();

  renderEmitters();

  renderParticles();
  emitParticles();

  glutSwapBuffers();
}

void reshape(int w, int h)
{
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective (48.0, (GLfloat) w/(GLfloat) h, 0.0, 10000.0);
}

void resetParticles() {
  int emitter;
  for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
    initParticles(emitter);
  }
}

void keyboard(unsigned char key, int x, int y)
{
  int emitter;
  switch (key)
  {
    case 27:  /* Escape key */
      exit(0);
    case 49: /* 1 Key */
      emitSpeed = 1;
      resetParticles();
      break;
    case 50: /* 2 Key */
      emitSpeed = 50;
      resetParticles();
      break;
    case 51: /* 3 Key */
      emitSpeed = 300;
      resetParticles();
      break;
    case 52: /* 4 Key */
      emitSpeed = 3100;
      resetParticles();
      break;
    case 53: /* 5 Key */
      emitSpeed = 5000;
      resetParticles();
      break;
    case 115: /* s key to increase particle emittion */
      if(emitSpeed + 10 > 3300) {break;}
      emitSpeed += 10;
      break;  
    case 120: /* x key to decrease particle emittion */
    if(emitSpeed - 10 < 0) {break;}
      emitSpeed -= 10;
      break;  
    case 97: /* a key to move forward */
      eyex += RUN_SPEED*sin(DEG_TO_RAD*(lon));
      eyez += RUN_SPEED*cos(DEG_TO_RAD*(lon));
      eyey += RUN_SPEED*cos(DEG_TO_RAD*(lat));
      break;  
    case 122: /* z key to move backwards */
      eyex -= RUN_SPEED*sin(DEG_TO_RAD*(lon));
      eyez -= RUN_SPEED*cos(DEG_TO_RAD*(lon));
      eyey -= RUN_SPEED*cos(DEG_TO_RAD*(lat));
      break;
    case 112:
      if(FIELD_ACTIVE) {FIELD_ACTIVE = false; }
      else { FIELD_ACTIVE = true; }
      break;
    case 108: /* l key to decrease field strength */
      if(B > 0.1f) { B -= 0.1; }
      break;  
    case 111: /* o key to increase field strength */
      B += 0.1;
      break;
    case 105: /* i key to increase particle mass */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        particleMass[emitter] += 0.1;
      }
      break;
    case 107: /* k key to decrease particle mass */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        if(particleMass[emitter] > 0.2f) { particleMass[emitter] -= 0.1; }
      }
      break;
    case 101: /* e key to increase emitter size */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        emitterSize[emitter] += 0.1f;
      }
      initEmitters();
      break;
    case 100: /* e key to increase emitter size */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        if(emitterSize[emitter] > 0.2f) { emitterSize[emitter] -= 0.1f; }
      }
      initEmitters();
      break;
    case 114: /* r key to increase y speed */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        emitterYSpeed[emitter] += 0.1f;
      }
      initEmitters();
      break;
    case 102: /* f key to decrease y speed */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        emitterYSpeed[emitter] -= 0.1f;
      }
      initEmitters();
      break;
    case 116: /* t key to increase x speed */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        emitterXSpeed[emitter] += 0.1f;
      }
      initEmitters();
      break;
    case 103: /* g key to increase x speed */
      for(emitter = 0; emitter < MAX_EMITTERS; emitter++) {
        emitterXSpeed[emitter] -= 0.1f;
      }
      initEmitters();
      break; /* . key to toggle random x velocity */
    case 46:
      if(RAND_X) { RAND_X = false; }
      else { RAND_X = true; }
    case 32:
      for(emitter = 0; emitter < MAX_EMITTERS ; emitter++) {
        startTime[emitter] = glutGet(GLUT_ELAPSED_TIME);
        oldTimeElapsed[emitter] = 0;
        initParticles(emitter);
      }
      break;
  }
}

void cursor_keys(int key, int x, int y) {
  switch (key) {
    case GLUT_KEY_LEFT: lon += TURN_ANGLE; break;
    case GLUT_KEY_RIGHT: lon -= TURN_ANGLE; break;
    case GLUT_KEY_UP: lat -= TURN_ANGLE; break;
    case GLUT_KEY_DOWN: lat += TURN_ANGLE; break;
  }
}

void stats(int i) {
  int currentParticles = totalParticles[ALPHA] + totalParticles[BETA] + totalParticles[GAMMA];
  printf("FPS: %f, Total Particles: %i\n", 1.0 / deltaTime[ALPHA], currentParticles);  
  glutTimerFunc(1000.0, stats, 0);
}

int main(int argc, char** argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
  glutInitWindowSize( 800, 600 );
  glutCreateWindow("Radiation");
  init();
  glutDisplayFunc(display); 
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutSpecialFunc(cursor_keys);
  glutTimerFunc(1000.0/60.0, animate, 0);
  glutTimerFunc(1000.0, stats, 0);
  glutMainLoop();
  return 0;
}