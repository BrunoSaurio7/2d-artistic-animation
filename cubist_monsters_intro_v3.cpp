// cubist_monsters_intro_v4_fix.cpp
// Monsters Inc–style montage -> cubist portrait of Dave Brubeck.
// v4 FIXED:
// - Removed duplicate declaration of std::vector<Actor> G
// - Clearer highlight logic for glasses halos
// Timeline ~38 s (≈ 106 beats @168 BPM)
//
// Build (Windows / MSYS2):
//   g++ cubist_monsters_intro_v4_fix.cpp -lfreeglut -lopengl32 -lglu32 -lwinmm -O2 -o cubist_monsters_intro_v4.exe
// Linux:
//   g++ cubist_monsters_intro_v4_fix.cpp -lglut -lGLU -lGL -O2 -o cubist_monsters_intro_v4
//
// Controls: P=play/resync audio (Windows), Space=pause, , and . = nudge sync ±20ms, Esc=exit

#ifdef _WIN32
  #include <GL/freeglut.h>
#else
  #include <GL/glut.h>
#endif
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
  #include <mmsystem.h>
  #pragma comment(lib, "winmm.lib")
#endif

static const double PI=3.14159265358979323846;

// -------- Timing / Music --------
static const double BPM=168.0; // 5/4
static const int SIG_BEATS=5;
static double OFFSET_SEC=0.0;
static bool gPaused=false;
static double gStart=0.0;
inline double nowSec(){ return glutGet(GLUT_ELAPSED_TIME)/1000.0; }
inline double animSec(){ return gPaused?0.0:(nowSec()-gStart); }
inline double beatsFromSec(double t){ return (t - OFFSET_SEC) * (BPM/60.0); }
inline double clamp01(double x){ return x<0?0:x>1?1:x; }
inline double mixd(double a,double b,double t){ return a + (b-a)*t; }
inline double smooth(double x){ x=clamp01(x); return x*x*(3-2*x); }
inline double ph5(double bt){ double b=fmod(bt,(double)SIG_BEATS); if(b<0)b+=SIG_BEATS; return b; }
inline double pulse1(double bt){ double d=fabs(ph5(bt)-0.0); d=std::min(d,(double)SIG_BEATS-d); double v=std::max(0.0,1.0-d/0.20); return smooth(v); }
inline double pulse4(double bt){ double d=fabs(ph5(bt)-3.0); d=std::min(d,(double)SIG_BEATS-d); double v=std::max(0.0,1.0-d/0.18); return smooth(v); }
inline double easeOvershoot(double t){ t=clamp01(t); double e=smooth(t); return e + 0.12*sin(2*PI*e)*(1.0-e); }
inline double easeExpoOut(double t){ t=clamp01(t); return 1.0 - pow(2.0, -8.0*t); }
inline double easeBackIn(double t){ t=clamp01(t); return t*t*(2.7*t-1.7); }

// -------- Palette --------
struct Col{ float r,g,b; };
static Col C[] = {
  {0.02f,0.02f,0.03f}, // 0 near black (ONLY backdrop falloff)
  {0.20f,0.90f,0.95f}, // 1 aqua
  {1.00f,0.84f,0.10f}, // 2 vivid yellow
  {1.00f,0.28f,0.55f}, // 3 hot pink
  {0.10f,0.92f,0.35f}, // 4 lime
  {0.98f,0.52f,0.16f}, // 5 orange
  {0.73f,0.52f,1.00f}, // 6 purple
  {0.20f,0.65f,1.00f}, // 7 sky
  {0.98f,0.98f,1.00f}, // 8 off-white (eyes/shine)
  {0.93f,0.80f,0.70f}, // 9 skin light
  {0.80f,0.68f,0.58f}, // 10 skin shadow
  {1.00f,0.10f,0.10f}, // 11 red accent
  {0.10f,0.10f,0.12f}, // 12 dark gray (panels)
  {0.15f,0.18f,0.25f}, // 13 ink (pupils/frames) — dark, not black
  {0.25f,0.33f,0.55f}, // 14 charcoal-blue (frames)
};

static void setRGBA(float r,float g,float b,float a=1.0f){ glColor4f(r,g,b,a); }

// -------- Primitives --------
static void rectWH(float w,float h){
  glBegin(GL_TRIANGLE_FAN);
  glVertex2f(-w*0.5f,-h*0.5f); glVertex2f(+w*0.5f,-h*0.5f);
  glVertex2f(+w*0.5f,+h*0.5f); glVertex2f(-w*0.5f,+h*0.5f);
  glEnd();
}
static void circle(float r,int seg=96){
  glBegin(GL_TRIANGLE_FAN); glVertex2f(0,0);
  for(int i=0;i<=seg;i++){ float a=2*PI*i/seg; glVertex2f(r*cosf(a), r*sinf(a)); } glEnd();
}
static void ring(float r0,float r1,int seg=128){
  glBegin(GL_TRIANGLE_STRIP);
  for(int i=0;i<=seg;i++){ float a=2*PI*i/seg, c=cosf(a), s=sinf(a); glVertex2f(r1*c,r1*s); glVertex2f(r0*c,r0*s); } glEnd();
}
static void triUp(float s){
  glBegin(GL_TRIANGLES);
  glVertex2f(0,s); glVertex2f(-s,-s); glVertex2f(+s,-s); glEnd();
}
static void triDown(float s){
  glBegin(GL_TRIANGLES);
  glVertex2f(0,-s); glVertex2f(-s,s); glVertex2f(+s,s); glEnd();
}
static void oval(float rx,float ry,int seg=96){
  glBegin(GL_TRIANGLE_FAN); glVertex2f(0,0);
  for(int i=0;i<=seg;i++){ float a=2*PI*i/seg; glVertex2f(rx*cosf(a), ry*sinf(a)); } glEnd();
}
static void arcBand(float r,float a0,float a1,float th=0.06f,int seg=96){
  glBegin(GL_TRIANGLE_STRIP);
  for(int i=0;i<=seg;i++){ float t=(float)i/seg; float a=a0+(a1-a0)*t; float c=cosf(a), s=sinf(a);
    glVertex2f((r+th)*c,(r+th)*s); glVertex2f((r-th)*c,(r-th)*s); } glEnd();
}

// -------- Actors / personalities --------
enum Shape{ S_RECT, S_CIRC, S_RING, S_TRI_UP, S_TRI_DOWN, S_OVAL, S_ARC };
enum Persona{ PR_LEADER, PR_WOBBLE, PR_BOUNCER, PR_SPINNER, PR_SNEAK, PR_SASSY, PR_NERVOUS, PR_STOIC, PR_PENDULUM, PR_SQUISH };

struct Actor{
  Shape shape; Persona pr; int col; float alpha;
  float x,y,sx,sy,ang;         // current
  float tx,ty,tsx,tsy,tang;    // target
  bool on;
  float a0,a1;                 // arc params
  float gainA, gainB;          // personality gains
};

static std::vector<Actor> G; // single, global

static Actor make(Shape s, Persona p,int col,
                  float tx,float ty,float tsx,float tsy,float tang,
                  float alpha=0.0f,bool on=false,float a0=0,float a1=0,
                  float gA=1.0f,float gB=1.0f){
  Actor a; a.shape=s; a.pr=p; a.col=col; a.alpha=alpha; a.on=on;
  a.tx=tx; a.ty=ty; a.tsx=tsx; a.tsy=tsy; a.tang=tang;
  a.x=tx; a.y=ty; a.sx=tsx; a.sy=tsy; a.ang=tang; a.a0=a0; a.a1=a1;
  a.gainA=gA; a.gainB=gB; return a;
}

static void drawActor(const Actor& a){
  if(!a.on || a.alpha<=0.001f) return;
  glPushMatrix();
  glTranslatef(a.x,a.y,0); glRotatef(a.ang,0,0,1); glScalef(a.sx,a.sy,1);
  setRGBA(C[a.col].r, C[a.col].g, C[a.col].b, a.alpha);
  switch(a.shape){
    case S_RECT: rectWH(1,1); break;
    case S_CIRC: circle(0.5f); break;
    case S_RING: ring(0.38f,0.50f); break;
    case S_TRI_UP: triUp(0.5f); break;
    case S_TRI_DOWN: triDown(0.5f); break;
    case S_OVAL: oval(0.5f,0.33f); break;
    case S_ARC: arcBand(0.45f,a.a0,a.a1,0.07f); break;
  }
  glPopMatrix();
}

static void applyPersona(Actor& a, double bt){
  double p1=pulse1(bt), p4=pulse4(bt);
  switch(a.pr){
    case PR_LEADER:    a.sy = a.tsy*(1.0f + 0.05f*(float)p1*a.gainA); break;
    case PR_WOBBLE:    a.ang= a.tang + (12.0f*a.gainA)*(float)sin(2*PI*(bt*0.7)); break;
    case PR_BOUNCER:   a.y  = a.ty   + (0.12f*a.gainA)*(float)fabs(sin(2*PI*(bt*0.9))); break;
    case PR_SPINNER:   a.ang= a.tang + (120.0f*a.gainA)*(float)(p1*0.5 + p4*0.5); break;
    case PR_SNEAK:     a.x  = a.tx   + (0.10f*a.gainA)*(float)sin(2*PI*(bt*0.33)); break;
    case PR_SASSY:     a.ang= a.tang + (18.0f*a.gainA)*(float)sin(2*PI*(bt*0.5));
                       a.y  = a.ty   + (0.05f*a.gainB)*(float)sin(2*PI*(bt*1.0)); break;
    case PR_NERVOUS:   a.x  = a.tx   + 0.02f*(float)sin(2*PI*(bt*15.0));
                       a.y  = a.ty   + 0.02f*(float)cos(2*PI*(bt*13.0)); break;
    case PR_PENDULUM:  a.ang= a.tang + (22.0f*a.gainA)*(float)sin(2*PI*(bt*0.25)); break;
    case PR_SQUISH:    a.sx = a.tsx*(1.0f + 0.08f*(float)p1*a.gainA);
                       a.sy = a.tsy*(1.0f - 0.06f*(float)p1*a.gainA); break;
    case PR_STOIC: default: break;
  }
}

// -------- Portrait IDs --------
enum FigID{
  F_FACE, F_MOUTH, F_EYE_L, F_EYE_R, F_PUPIL_L, F_PUPIL_R, F_NOSE,
  F_HAIR_TOP, F_HAIR_L, F_HAIR_R,
  F_BROW_L, F_BROW_R, F_EAR_L, F_EAR_R, F_CHIN,
  F_TIE_TOP, F_TIE_BOTTOM,
  F_ARM_L, F_ARM_R, F_GLAS_L, F_GLAS_R, F_GLAS_BR,
  F_CHEEK_L, F_CHEEK_R, F_FOREHEAD, F_JAW, F_LIP_HL,
  F_NOSTRIL_L, F_NOSTRIL_R, F_GLAS_HL_L, F_GLAS_HL_R,
  F_TEMP_L, F_TEMP_R, F_NOSE_HL,
  F_COUNT
};

static Actor& A(int id){ return G[id]; }

static void initFigures(){
  G.clear();
  // Geometry (eyes & glasses separated, non-touching)
  G.push_back(make(S_RECT,    PR_LEADER, 9,    0,-0.05f, 0.80f,0.95f, 0));                                  // F_FACE
  G.push_back(make(S_ARC,     PR_SASSY,  3,    0,-0.32f, 1,1,0, 0,false,-0.25f*PI,0.25f*PI, 1.0f,1.2f));    // F_MOUTH
  G.push_back(make(S_RING,    PR_WOBBLE, 8,   -0.34f,0.22f, 0.60f,0.60f,0));                                // F_EYE_L
  G.push_back(make(S_RING,    PR_WOBBLE, 8,    0.34f,0.22f, 0.60f,0.60f,0));                                // F_EYE_R
  G.push_back(make(S_CIRC,    PR_NERVOUS,13,  -0.34f,0.22f, 0.16f,0.16f,0));                                // F_PUPIL_L
  G.push_back(make(S_CIRC,    PR_NERVOUS,13,   0.34f,0.22f, 0.16f,0.16f,0));                                // F_PUPIL_R
  G.push_back(make(S_TRI_UP,  PR_BOUNCER,5,    0,0.06f, 0.44f,0.44f,0));                                    // F_NOSE
  G.push_back(make(S_RECT,    PR_SPINNER,7,    0,0.60f, 0.90f,0.28f,0));                                    // F_HAIR_TOP
  G.push_back(make(S_RECT,    PR_SNEAK,  7,   -0.46f,0.30f, 0.14f,0.70f,0));                                // F_HAIR_L
  G.push_back(make(S_RECT,    PR_SNEAK,  7,    0.46f,0.30f, 0.14f,0.70f,0));                                // F_HAIR_R
  G.push_back(make(S_RECT,    PR_SASSY,  6,   -0.35f,0.35f, 0.50f,0.09f,-6));                               // F_BROW_L
  G.push_back(make(S_RECT,    PR_SASSY,  6,    0.35f,0.33f, 0.50f,0.09f, 6));                               // F_BROW_R
  G.push_back(make(S_TRI_UP,  PR_STOIC,10,   -0.66f,0.00f, 0.60f,0.60f,0));                                 // F_EAR_L
  G.push_back(make(S_TRI_UP,  PR_STOIC,10,    0.66f,0.00f, 0.60f,0.60f,180));                               // F_EAR_R
  G.push_back(make(S_OVAL,    PR_STOIC, 9,     0,-0.62f, 0.60f,0.36f,0));                                   // F_CHIN
  G.push_back(make(S_TRI_DOWN,PR_LEADER,1,     0,-0.95f, 0.36f,0.36f,0));                                   // F_TIE_TOP
  G.push_back(make(S_TRI_DOWN,PR_LEADER,1,     0,-1.22f, 0.60f,0.60f,0));                                   // F_TIE_BOTTOM
  G.push_back(make(S_RECT,    PR_STOIC,  14,  -0.90f,0.28f, 0.90f,0.06f,0));                                // F_ARM_L
  G.push_back(make(S_RECT,    PR_STOIC,  14,   0.90f,0.28f, 0.90f,0.06f,0));                                // F_ARM_R
  G.push_back(make(S_RING,    PR_STOIC,  14,  -0.44f,0.22f, 0.72f,0.72f,0));                                // F_GLAS_L
  G.push_back(make(S_RING,    PR_STOIC,  14,   0.44f,0.22f, 0.72f,0.72f,0));                                // F_GLAS_R
  G.push_back(make(S_RECT,    PR_STOIC,  14,   0.00f,0.22f, 0.26f,0.06f,0));                                // F_GLAS_BR

  // Extras
  G.push_back(make(S_RECT,    PR_SQUISH, 4,   -0.22f,-0.10f, 0.40f,0.28f, 10));                              // F_CHEEK_L
  G.push_back(make(S_RECT,    PR_SQUISH, 2,    0.22f,-0.12f, 0.42f,0.26f,-10));                              // F_CHEEK_R
  G.push_back(make(S_RECT,    PR_PENDULUM,6,    0.00f,0.46f, 0.42f,0.14f, 0));                               // F_FOREHEAD
  G.push_back(make(S_RECT,    PR_WOBBLE, 5,     0.00f,-0.48f,0.56f,0.18f, 4));                               // F_JAW
  G.push_back(make(S_ARC,     PR_SASSY,  11,    0.00f,-0.29f,1,1,0, 0,false, -0.08f*PI, 0.08f*PI, 1.0f,1.4f)); // F_LIP_HL
  G.push_back(make(S_ARC,     PR_STOIC,  14,   -0.04f,0.06f,1,1,0, 0,false, 0.65f*PI,0.85f*PI));             // F_NOSTRIL_L
  G.push_back(make(S_ARC,     PR_STOIC,  14,    0.04f,0.06f,1,1,0, 0,false, 0.15f*PI,0.35f*PI));             // F_NOSTRIL_R
  G.push_back(make(S_RING,    PR_SQUISH, 8,   -0.44f,0.22f,0.90f,0.90f,0));                                  // F_GLAS_HL_L
  G.push_back(make(S_RING,    PR_SQUISH, 8,    0.44f,0.22f,0.90f,0.90f,0));                                  // F_GLAS_HL_R
  G.push_back(make(S_RECT,    PR_WOBBLE, 7,   -0.58f,0.18f, 0.16f,0.32f, 8));                                // F_TEMP_L
  G.push_back(make(S_RECT,    PR_WOBBLE, 7,    0.58f,0.18f, 0.16f,0.32f,-8));                                // F_TEMP_R
  G.push_back(make(S_ARC,     PR_SASSY,  2,     0.00f,0.18f,1,1,0, 0,false, 0.48f*PI,0.52f*PI, 1.0f,1.0f));   // F_NOSE_HL

  for(auto& a:G){ a.on=false; a.alpha=0.0f; }
}

// -------- Panels & spotlight --------
struct Panel{ float x,y,w,h; float ox,oy; };
static std::vector<Panel> PAN;
static void buildPanels(){
  PAN.clear();
  int cols=4, rows=3; float W=2.0f/cols, H=2.0f/rows;
  for(int j=0;j<rows;j++) for(int i=0;i<cols;i++){
    float cx=-1.0f + (i+0.5f)*W, cy=-1.0f + (j+0.5f)*H;
    Panel p{cx,cy,W*1.02f,H*1.02f, (i%2?+1.6f:-1.6f), (j%2?+1.6f:-1.6f)};
    PAN.push_back(p);
  }
}
static void drawPanels(float k){
  setRGBA(C[12].r,C[12].g,C[12].b, 0.92f);
  for(auto p:PAN){
    glPushMatrix();
    glTranslatef(p.x + (1.0f-k)*p.ox, p.y + (1.0f-k)*p.oy, 0);
    rectWH(p.w,p.h);
    glPopMatrix();
  }
}
static void spotlight(float cx,float cy,float r){
  int seg=96;
  glBegin(GL_TRIANGLE_FAN);
  setRGBA(0,0,0,0); glVertex2f(cx,cy);
  for(int i=0;i<=seg;i++){
    float a=2*PI*i/seg, X=cx+r*cosf(a), Y=cy+r*sinf(a);
    setRGBA(0,0,0,0.86f); glVertex2f(X,Y);
  }
  glEnd();
}

// -------- Timeline (~106 beats ≈ 37.9 s) --------
struct Scene{ double b0,b1; };
static std::vector<Scene> TL;
static void addScene(double beats){ double b=TL.empty()?0.0:TL.back().b1; TL.push_back({b,b+beats}); }
static void buildTimeline(){
  TL.clear();
  // sum = 106 beats
  addScene(5);  // 0 Panels in
  addScene(2);  // 1 Tease: face
  addScene(2);  // 2 Tease: mouth
  addScene(2);  // 3 Tease: eye
  addScene(4);  // 4 Panels out

  addScene(10); // 5 eye rings
  addScene(6);  // 6 pupils

  addScene(9);  // 7 glasses assemble (separate lenses)

  addScene(5);  // 8 eyes+glasses+MOUTH (NEW)
  addScene(6);  // 9 eyes+glasses+mouth+NOSE with personality (NEW)

  addScene(6);  // 10 cheeks (semi-transparent)
  addScene(6);  // 11 forehead & jaw & temples

  addScene(4);  // 12 top hair
  addScene(4);  // 13 side hair

  addScene(6);  // 14 brows + show eyes

  addScene(5);  // 15 ears & chin

  addScene(5);  // 16 tie & face lift

  addScene(7);  // 17 hop-in assembly
  addScene(4);  // 18 reveal wobble (emphasize key features)
  addScene(2);  // 19 freeze
}
static int sceneOf(double bt){
  for(size_t i=0;i<TL.size();++i) if(bt>=TL[i].b0 && bt<TL[i].b1) return (int)i;
  return (int)TL.size()-1;
}

// -------- Utilities --------
static void drawBG(){ glBegin(GL_QUADS); setRGBA(0,0,0,1); glVertex2f(-1,-1); glVertex2f(+1,-1); glVertex2f(+1,+1); glVertex2f(-1,+1); glEnd(); }
static void hideAll(){ for(auto& a:G){ a.on=false; a.alpha=0.0f; a.x=a.tx; a.y=a.ty; a.sx=a.tsx; a.sy=a.tsy; a.ang=a.tang; } }
static void placeOn(int id,double bt,float alpha=1.0f){ auto& a=A(id); a.on=true; a.alpha=alpha; a.x=a.tx; a.y=a.ty; a.sx=a.tsx; a.sy=a.tsy; a.ang=a.tang; applyPersona(a,bt); }
static float hopWave(double bt, double phase){
  double b=ph5(bt+phase);
  auto gauss=[&](double x,double mu,double sigma){ double d=(x-mu)/sigma; return exp(-0.5*d*d); };
  float v = (float)(1.0*gauss(b,0.0,0.45) + 0.6*gauss(b,3.0,0.40));
  return v;
}

// -------- Update per scene --------
static void updateAndDraw(double bt){
  int si=sceneOf(bt);
  double b0=TL[si].b0, b1=TL[si].b1, u=clamp01((bt-b0)/(b1-b0)), e=smooth(u);

  if(si==0) buildPanels();
  if(si<=4) drawPanels(si==0? easeOvershoot(e) : (si==4? (1.0 - easeOvershoot(e)) : 1.0));

  switch(si){
    case 0: hideAll(); break;
    case 1: hideAll(); placeOn(F_FACE,bt,0.6f); break;
    case 2: hideAll(); placeOn(F_MOUTH,bt,0.8f); break;
    case 3: hideAll(); placeOn(F_EYE_L,bt,1.0f); break;
    case 4: hideAll(); break;

    // Eye rings
    case 5:{
      hideAll();
      auto &EL=A(F_EYE_L), &ER=A(F_EYE_R);
      EL.on=ER.on=true; EL.alpha=ER.alpha=1.0f;
      float k=(float)easeBackIn(e);
      EL.x = (float)mixd(-1.2, EL.tx, k); EL.y=(float)mixd(+0.9, EL.ty, k);
      ER.x = (float)mixd(+1.2, ER.tx, k); ER.y=(float)mixd(-0.9, ER.ty, k);
      EL.sx=EL.sy= ER.sx=ER.sy= EL.tsx * (1.0f + 0.05f*(float)pulse1(bt));
      applyPersona(EL,bt); applyPersona(ER,bt);
    } break;

    // Pupils
    case 6:{
      hideAll(); placeOn(F_EYE_L,bt,1.0f); placeOn(F_EYE_R,bt,1.0f);
      auto &PL=A(F_PUPIL_L), &PR=A(F_PUPIL_R);
      PL.on=PR.on=true; PL.alpha=PR.alpha=1.0f;
      float r=0.06f;
      PL.x = (float)mixd(-0.9, PL.tx, easeExpoOut(e)) + r*(float)sin(2*PI*(bt*0.8));
      PL.y = PL.ty + r*(float)cos(2*PI*(bt*0.6));
      PR.x = (float)mixd(+0.9, PR.tx, easeExpoOut(e)) + r*(float)cos(2*PI*(bt*0.7));
      PR.y = PR.ty + r*(float)sin(2*PI*(bt*0.9));
    } break;

    // Glasses (separate lenses)
    case 7:{
      hideAll();
      auto &GL=A(F_GLAS_L), &GR=A(F_GLAS_R), &GB=A(F_GLAS_BR), &AL=A(F_ARM_L), &AR=A(F_ARM_R);
      auto &HL=A(F_GLAS_HL_L), &HR=A(F_GLAS_HL_R);
      GL.on=GR.on=GB.on=AL.on=AR.on=HL.on=HR.on=true;
      GL.alpha=GR.alpha=GB.alpha=AL.alpha=AR.alpha=1.0f;
      HL.alpha=HR.alpha=0.70f;
      float k=(float)easeExpoOut(e);
      GL.x=(float)mixd(-1.5, GL.tx, k); GL.y=GL.ty;
      GR.x=(float)mixd(+1.5, GR.tx, k); GR.y=GR.ty;
      GB.x=(float)mixd( 0.0, GB.tx, k); GB.y=GB.ty;
      AL.x=(float)mixd(-1.7, AL.tx, k); AL.y=AL.ty;
      AR.x=(float)mixd(+1.7, AR.tx, k); AR.y=AR.ty;
      HL.x=GL.x; HL.y=GL.y; HR.x=GR.x; HR.y=GR.y;
      applyPersona(HL,bt); applyPersona(HR,bt);
    } break;

    // Eyes + Glasses + Mouth
    case 8:{
      hideAll();
      placeOn(F_EYE_L,bt,1.0f); placeOn(F_EYE_R,bt,1.0f);
      placeOn(F_PUPIL_L,bt,1.0f); placeOn(F_PUPIL_R,bt,1.0f);
      placeOn(F_GLAS_L,bt,1.0f); placeOn(F_GLAS_R,bt,1.0f); placeOn(F_GLAS_BR,bt,1.0f);
      placeOn(F_MOUTH,bt,1.0f); auto &LH=A(F_LIP_HL); LH.on=true; LH.alpha=0.85f; LH.x=LH.tx; LH.y=LH.ty;
      applyPersona(A(F_MOUTH),bt); applyPersona(LH,bt);
    } break;

    // Nose + previous (personality)
    case 9:{
      hideAll();
      int prev[] = {F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R,F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_MOUTH};
      for(int id:prev){ placeOn(id,bt,1.0f); }
      auto &N=A(F_NOSE); N.on=true; N.alpha=1.0f;
      auto &NL=A(F_NOSTRIL_L); auto &NR=A(F_NOSTRIL_R); auto &NH=A(F_NOSE_HL);
      NL.on=NR.on=NH.on=true; NL.alpha=NR.alpha=1.0f; NH.alpha=0.75f;
      float k=(float)easeBackIn(e);
      N.x=(float)mixd(0.0, N.tx, k); N.y=(float)mixd(+1.2, N.ty, k);
      applyPersona(N,bt); applyPersona(NL,bt); applyPersona(NR,bt); applyPersona(NH,bt);
    } break;

    // Cheeks
    case 10:{
      hideAll(); auto &CL=A(F_CHEEK_L), &CR=A(F_CHEEK_R);
      CL.on=CR.on=true; CL.alpha=CR.alpha=0.85f;
      float k=(float)easeExpoOut(e);
      CL.x=(float)mixd(-1.2, CL.tx, k); CR.x=(float)mixd(+1.2, CR.tx, k);
      CL.y=CL.ty; CR.y=CR.ty;
      applyPersona(CL,bt); applyPersona(CR,bt);
    } break;

    // Forehead & jaw & temples
    case 11:{
      hideAll(); auto &Fh=A(F_FOREHEAD), &J=A(F_JAW), &TLs=A(F_TEMP_L), &TRs=A(F_TEMP_R);
      Fh.on=J.on=TLs.on=TRs.on=true; Fh.alpha=J.alpha=0.85f; TLs.alpha=TRs.alpha=0.80f;
      float k=(float)easeOvershoot(e);
      Fh.y=(float)mixd(+1.2, Fh.ty, k); Fh.x=Fh.tx;
      J.y =(float)mixd(-1.2, J.ty,  k); J.x=J.tx;
      TLs.x=(float)mixd(-1.4, TLs.tx, k); TRs.x=(float)mixd(+1.4, TRs.tx, k);
      TLs.y=TLs.ty; TRs.y=TRs.ty;
      applyPersona(Fh,bt); applyPersona(J,bt); applyPersona(TLs,bt); applyPersona(TRs,bt);
    } break;

    // Hair top
    case 12: hideAll(); placeOn(F_HAIR_TOP,bt,1.0f); break;
    // Hair sides
    case 13: hideAll(); placeOn(F_HAIR_L,bt,1.0f); placeOn(F_HAIR_R,bt,1.0f); break;

    // Brows + eyes
    case 14:{
      hideAll();
      placeOn(F_BROW_L,bt,1.0f); placeOn(F_BROW_R,bt,1.0f);
      placeOn(F_EYE_L,bt,1.0f); placeOn(F_EYE_R,bt,1.0f);
      placeOn(F_PUPIL_L,bt,1.0f); placeOn(F_PUPIL_R,bt,1.0f);
    } break;

    // Ears & Chin
    case 15: hideAll(); placeOn(F_EAR_L,bt,1.0f); placeOn(F_EAR_R,bt,1.0f); placeOn(F_CHIN,bt,1.0f); break;

    // Tie & face lift
    case 16:{
      hideAll(); placeOn(F_TIE_TOP,bt,1.0f); placeOn(F_TIE_BOTTOM,bt,1.0f);
      auto &FACE=A(F_FACE); FACE.on=true; FACE.alpha=1.0f;
      FACE.x=FACE.tx; FACE.y=(float)mixd(FACE.ty-0.06, FACE.ty, e);
    } break;

    // Hop-in assembly
    case 17:{
      for(int i=0;i<F_COUNT;++i){ auto& a=G[i]; a.on=true; if(a.alpha<=0.0f) a.alpha=1.0f; }
      for(int i=0;i<F_COUNT;++i){
        auto& a=G[i];
        double ph = (i%SIG_BEATS)*0.15;
        float hop = 0.12f * hopWave(bt, ph) * (1.0f - (float)e);
        a.x = (float)mixd(a.x, a.tx, easeOvershoot(e));
        a.y = (float)mixd(a.y, a.ty + hop, easeOvershoot(e));
        a.sx= (float)mixd(a.sx,a.tsx, easeOvershoot(e));
        a.sy= (float)mixd(a.sy,a.tsy, easeOvershoot(e));
        a.ang=(float)mixd(a.ang,a.tang, easeOvershoot(e));
      }
    } break;

    // Reveal wobble (emphasize eyes, glasses, nose, mouth)
    case 18:{
      for(int i=0;i<F_COUNT;++i){ auto& a=G[i]; a.on=true; a.alpha = (a.alpha>0? a.alpha : 1.0f); }
      int keys[] = {F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R,F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_MOUTH,F_NOSE};
      for(int id:keys){
        auto& k=G[id]; float br = 1.0f + 0.05f*(float)pulse1(bt);
        k.sx = k.tsx * br; k.sy = k.tsy * (1.0f/br);
        k.alpha = std::min(1.0f, k.alpha + 0.10f);
        if(id==F_GLAS_L){ G[F_GLAS_HL_L].on=true; G[F_GLAS_HL_L].alpha=0.80f; }
        if(id==F_GLAS_R){ G[F_GLAS_HL_R].on=true; G[F_GLAS_HL_R].alpha=0.80f; }
      }
      for(int i=0;i<F_COUNT;++i){ auto& a=G[i];
        a.x  = a.tx + 0.008f*(float)sin(2*PI*(bt*0.20 + i*0.03));
        a.y  = a.ty + 0.008f*(float)cos(2*PI*(bt*0.17 + i*0.04));
        a.ang= a.tang + 2.0f*(float)sin(2*PI*(bt*0.08 + i*0.02));
      }
    } break;

    // Freeze
    case 19:{
      for(int i=0;i<F_COUNT;++i){ auto& a=G[i]; a.on=true;
        a.x=a.tx; a.y=a.ty; a.sx=a.tsx; a.sy=a.tsy; a.ang=a.tang; a.alpha = (a.alpha>0? a.alpha : 1.0f);
      }
    } break;
  }

  // Draw actors
  for(const auto& a:G) drawActor(a);

  // Spotlights in 1..3
  if(si==1||si==2||si==3){
    int id=(si==1?F_FACE:(si==2?F_MOUTH:F_EYE_L));
    spotlight(A(id).tx, A(id).ty, (float)mixd(1.8,0.65,e));
  }
}

// -------- GLUT plumbing --------
static void reshape(int w,int h){
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  double ar=(double)w/(double)h;
  if(ar>=16.0/9.0) glOrtho(-ar/(16.0/9.0), ar/(16.0/9.0), -1,1,-1,1);
  else             glOrtho(-1,1, -(16.0/9.0)/ar, (16.0/9.0)/ar, -1,1);
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
}
static void display(){
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS); setRGBA(0,0,0,1); glVertex2f(-1,-1); glVertex2f(+1,-1); glVertex2f(+1,+1); glVertex2f(-1,+1); glEnd();
  double t=animSec(), bt=beatsFromSec(t);
  updateAndDraw(bt);
  glutSwapBuffers();
}
static void idle(){ if(!gPaused) glutPostRedisplay(); }
static void keyb(unsigned char k,int,int){
  if(k==27) std::exit(0);
  if(k==' ') gPaused=!gPaused;
  if(k==',') OFFSET_SEC-=0.020;
  if(k=='.') OFFSET_SEC+=0.020;
  if(k=='p'||k=='P'){
    gStart=nowSec();
#ifdef _WIN32
    mciSendStringA("close tf", NULL, 0, NULL);
    mciSendStringA("open \"Take Five.mp3\" type mpegvideo alias tf", NULL, 0, NULL);
    mciSendStringA("play tf from 0", NULL, 0, NULL);
#endif
  }
}

int main(int argc,char** argv){
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
  glutInitWindowSize(1920,1080);
  glutCreateWindow("Monsters-style Cubist Intro — Take Five v4 (~38s) FIX");
  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyb);
  gStart=nowSec();
  initFigures();
  buildPanels();
  buildTimeline();
  glutMainLoop();
  return 0;
}
