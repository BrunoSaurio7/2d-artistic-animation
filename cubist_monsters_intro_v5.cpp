// cubist_monsters_intro_v5d.cpp
// Monsters Inc–style montage -> cubist portrait of Dave Brubeck.
// v5d: Fix build error (initializer_list vs vector) and keep v5c behavior.
//
// Build (Windows / MSYS2):
//   g++ cubist_monsters_intro_v5d.cpp -lfreeglut -lopengl32 -lglu32 -lwinmm -O2 -o cubist_monsters_intro_v5d.exe
// Linux:
//   g++ cubist_monsters_intro_v5d.cpp -lglut -lGLU -lGL -O2 -o cubist_monsters_intro_v5d
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
#include <initializer_list>
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
inline double pulse1(double bt){ double d=fabs(ph5(bt)-0.0); d=std::min(d,(double)SIG_BEATS-d); double v=std::max(0.0,1.0-d/0.23); return smooth(v); }
inline double pulse4(double bt){ double d=fabs(ph5(bt)-3.0); d=std::min(d,(double)SIG_BEATS-d); double v=std::max(0.0,1.0-d/0.20); return smooth(v); }
inline double easeOvershoot(double t){ t=clamp01(t); double e=smooth(t); return e + 0.12*sin(2*PI*e)*(1.0-e); }
inline double easeExpoOut(double t){ t=clamp01(t); return 1.0 - pow(2.0, -8.0*t); }
inline double easeBackIn(double t){ t=clamp01(t); return t*t*(2.7*t-1.7); }
inline double easeCubic(double t){ t=clamp01(t); return t*t*(3-2*t); }

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
static std::vector<float> VIS; // target alpha [0..1] per actor

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
  if(a.alpha<=0.001f) return;
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
    case PR_LEADER:    a.sy = a.tsy*(1.0f + 0.06f*(float)p1*a.gainA); break;
    case PR_WOBBLE:    a.ang= a.tang + (18.0f*a.gainA)*(float)sin(2*PI*(bt*0.55)); break;
    case PR_BOUNCER:   a.y  = a.ty   + (0.15f*a.gainA)*(float)fabs(sin(2*PI*(bt*0.9))); break;
    case PR_SPINNER:   a.ang= a.tang + (140.0f*a.gainA)*(float)(p1*0.5 + p4*0.5); break;
    case PR_SNEAK:     a.x  = a.tx   + (0.12f*a.gainA)*(float)sin(2*PI*(bt*0.33)); break;
    case PR_SASSY:     a.ang= a.tang + (22.0f*a.gainA)*(float)sin(2*PI*(bt*0.45));
                       a.y  = a.ty   + (0.06f*a.gainB)*(float)sin(2*PI*(bt*1.0)); break;
    case PR_NERVOUS:   a.x  = a.tx   + 0.025f*(float)sin(2*PI*(bt*15.0));
                       a.y  = a.ty   + 0.025f*(float)cos(2*PI*(bt*13.0)); break;
    case PR_PENDULUM:  a.ang= a.tang + (26.0f*a.gainA)*(float)sin(2*PI*(bt*0.22)); break;
    case PR_SQUISH:    a.sx = a.tsx*(1.0f + 0.10f*(float)p1*a.gainA);
                       a.sy = a.tsy*(1.0f - 0.08f*(float)p1*a.gainA); break;
    case PR_STOIC:     a.sx = a.tsx*(1.0f + 0.025f*(float)(p1*0.6+p4*0.4));
                       a.sy = a.tsy*(1.0f - 0.020f*(float)(p1*0.6+p4*0.4)); break;
    default: break;
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
  // Geometry (align glasses to eyes centers; eyebrows above eyes)
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
  // Glasses centered on eyes
  G.push_back(make(S_RING,    PR_STOIC,  14,  -0.34f,0.22f, 0.78f,0.78f,0));                                // F_GLAS_L
  G.push_back(make(S_RING,    PR_STOIC,  14,   0.34f,0.22f, 0.78f,0.78f,0));                                // F_GLAS_R
  G.push_back(make(S_RECT,    PR_STOIC,  14,   0.00f,0.22f, 0.28f,0.06f,0));                                // F_GLAS_BR

  // Extras
  G.push_back(make(S_RECT,    PR_SQUISH, 4,   -0.22f,-0.10f, 0.40f,0.28f, 10));                              // F_CHEEK_L
  G.push_back(make(S_RECT,    PR_SQUISH, 2,    0.22f,-0.12f, 0.42f,0.26f,-10));                              // F_CHEEK_R
  G.push_back(make(S_RECT,    PR_PENDULUM,6,    0.00f,0.46f, 0.42f,0.14f, 0));                               // F_FOREHEAD
  G.push_back(make(S_RECT,    PR_WOBBLE, 5,     0.00f,-0.48f,0.56f,0.18f, 4));                               // F_JAW
  G.push_back(make(S_ARC,     PR_SASSY,  11,    0.00f,-0.29f,1,1,0, 0,false, -0.08f*PI, 0.08f*PI, 1.0f,1.4f)); // F_LIP_HL
  G.push_back(make(S_ARC,     PR_STOIC,  14,   -0.04f,0.06f,1,1,0, 0,false, 0.65f*PI,0.85f*PI));             // F_NOSTRIL_L
  G.push_back(make(S_ARC,     PR_STOIC,  14,    0.04f,0.06f,1,1,0, 0,false, 0.15f*PI,0.35f*PI));             // F_NOSTRIL_R
  G.push_back(make(S_RING,    PR_SQUISH, 8,   -0.34f,0.22f,0.92f,0.92f,0));                                  // F_GLAS_HL_L
  G.push_back(make(S_RING,    PR_SQUISH, 8,    0.34f,0.22f,0.92f,0.92f,0));                                  // F_GLAS_HL_R
  G.push_back(make(S_RECT,    PR_WOBBLE, 7,   -0.58f,0.18f, 0.16f,0.32f, 8));                                // F_TEMP_L
  G.push_back(make(S_RECT,    PR_WOBBLE, 7,    0.58f,0.18f, 0.16f,0.32f,-8));                                // F_TEMP_R
  G.push_back(make(S_ARC,     PR_SASSY,  2,     0.00f,0.18f,1,1,0, 0,false, 0.48f*PI,0.52f*PI, 1.0f,1.0f));   // F_NOSE_HL

  for(auto& a:G){ a.on=true; a.alpha=0.0f; } // keep all "on", we control by alpha
  VIS.assign(F_COUNT, 0.0f);
}

// -------- Panels --------
struct Panel{ float x,y,w,h; float ox,oy; };
static std::vector<Panel> PAN;
static void buildPanels(){
  PAN.clear();
  int cols=6, rows=3; float W=2.0f/cols, H=2.0f/rows;
  for(int j=0;j<rows;j++) for(int i=0;i<cols;i++){
    float cx=-1.0f + (i+0.5f)*W, cy=-1.0f + (j+0.5f)*H;
    Panel p{cx,cy,W*1.02f,H*1.02f, (i%2?+1.4f:-1.4f), (j%2?+1.2f:-1.2f)};
    PAN.push_back(p);
  }
}
static void drawPanels(float k, float alpha=0.92f){
  setRGBA(C[12].r,C[12].g,C[12].b, alpha);
  for(auto p:PAN){
    glPushMatrix();
    glTranslatef(p.x + (1.0f-k)*p.ox, p.y + (1.0f-k)*p.oy, 0);
    rectWH(p.w,p.h);
    glPopMatrix();
  }
}

// -------- Timeline --------
struct Scene{ double b0,b1; };
static std::vector<Scene> TL;
static void addScene(double beats){ double b=TL.empty()?0.0:TL.back().b1; TL.push_back({b,b+beats}); }
static void buildTimeline(){
  TL.clear();
  addScene(12); // 0 Panels in (longer)
  addScene(6);  // 1 Tease: face
  addScene(6);  // 2 Tease: mouth
  addScene(6);  // 3 Tease: eye
  addScene(8);  // 4 Panels out

  addScene(10); // 5 eye rings
  addScene(6);  // 6 pupils
  addScene(10); // 7 glasses assemble

  addScene(6);  // 8 eyes+glasses+MOUTH
  addScene(8);  // 9 + NOSE (hop-in)

  addScene(6);  // 10 cheeks
  addScene(6);  // 11 forehead & jaw & temples

  addScene(4);  // 12 top hair
  addScene(4);  // 13 side hair

  addScene(6);  // 14 brows + show eyes
  addScene(5);  // 15 ears & chin
  addScene(5);  // 16 tie & face lift

  addScene(7);  // 17 hop-in assembly
  addScene(4);  // 18 reveal wobble
  addScene(2);  // 19 freeze
}
static int sceneOf(double bt){
  for(size_t i=0;i<TL.size();++i) if(bt>=TL[i].b0 && bt<TL[i].b1) return (int)i;
  return (int)TL.size()-1;
}

// -------- Utilities --------
static void drawBG(){ glBegin(GL_QUADS); setRGBA(0,0,0,1); glVertex2f(-1,-1); glVertex2f(+1,-1); glVertex2f(+1,+1); glVertex2f(-1,+1); glEnd(); }
// Overload 1: initializer_list
static void wantOnly(std::initializer_list<int> ids){
  std::fill(VIS.begin(), VIS.end(), 0.0f);
  for(int id:ids) if(id>=0 && id<F_COUNT) VIS[id]=1.0f;
}
// Overload 2: vector<int>
static void wantOnlyVec(const std::vector<int>& ids){
  std::fill(VIS.begin(), VIS.end(), 0.0f);
  for(int id:ids) if(id>=0 && id<F_COUNT) VIS[id]=1.0f;
}
// helper for spotlight
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
static float hopWave(double bt, double phase){
  double b=ph5(bt+phase);
  auto gauss=[&](double x,double mu,double sigma){ double d=(x-mu)/sigma; return exp(-0.5*d*d); };
  float v = (float)(1.0*gauss(b,0.0,0.40) + 0.7*gauss(b,3.0,0.36));
  return v;
}

// Custom draw order to ensure top layering of key features
static void drawAllWithTop(double bt){
  std::vector<int> tops = {F_BROW_L,F_BROW_R,F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R,F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_NOSE,F_MOUTH};
  auto isTop=[&](int id){ return std::find(tops.begin(),tops.end(),id)!=tops.end(); };
  // Base pass
  for(int i=0;i<F_COUNT;++i){
    if(isTop(i)) continue;
    drawActor(G[i]);
  }
  // Top pass
  for(int id:tops){
    auto& k=G[id]; float br = 1.0f + 0.05f*(float)pulse1(bt);
    if(id==F_MOUTH || id==F_NOSE){ k.sx = k.tsx * br; k.sy = k.tsy * (1.0f/br); }
    drawActor(G[id]);
  }
}

// Fade overlay between scenes
static void fadeOverlay(double bt){
  int si=sceneOf(bt);
  double b0=TL[si].b0, b1=TL[si].b1;
  double inDur = std::min(2.0, (b1-b0)*0.25);
  double outDur= inDur;
  double uIn = clamp01((bt-b0)/std::max(0.0001,inDur));
  double uOut= clamp01((b1-bt)/std::max(0.0001,outDur));
  float a = 0.0f;
  a = (float)(0.14 * (1.0 - smooth(uIn)));
  if((b1-bt) < outDur){
    float a2 = (float)(0.14 * (1.0 - smooth(uOut)));
    a = std::max(a, a2);
  }
  if(a>0.001f){
    glBegin(GL_QUADS);
    setRGBA(0,0,0,a);
    glVertex2f(-1,-1); glVertex2f(+1,-1); glVertex2f(+1,+1); glVertex2f(-1,+1);
    glEnd();
  }
}

// -------- Update per scene --------
static void updateAndDraw(double bt){
  int si=sceneOf(bt);
  double b0=TL[si].b0, b1=TL[si].b1, u=clamp01((bt-b0)/(b1-b0)), e=smooth(u);

  // Panels
  if(si==0) buildPanels();
  if(si<=4) drawPanels(si==0? (float)easeOvershoot(e) : (si==4? (float)(1.0 - easeOvershoot(e)) : 1.0f), 0.92f);
  if(si==5 || si==7 || si==14 || si==17) drawPanels(1.0f, 0.80f);

  // Set scene targets
  switch(si){
    case 0: wantOnly({}); break;
    case 1: wantOnly({F_FACE}); break;
    case 2: wantOnly({F_MOUTH}); break;
    case 3: wantOnly({F_EYE_L}); break;
    case 4: wantOnly({}); break;
    case 5: wantOnly({F_EYE_L,F_EYE_R}); break;
    case 6: wantOnly({F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R}); break;
    case 7: wantOnly({F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_ARM_L,F_ARM_R,F_GLAS_HL_L,F_GLAS_HL_R}); break;
    case 8: wantOnly({F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R,F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_MOUTH,F_GLAS_HL_L,F_GLAS_HL_R}); break;
    case 9: wantOnly({F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R,F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_MOUTH,F_NOSE,F_NOSTRIL_L,F_NOSTRIL_R,F_NOSE_HL}); break;
    case 10: wantOnly({F_CHEEK_L,F_CHEEK_R}); break;
    case 11: wantOnly({F_FOREHEAD,F_JAW,F_TEMP_L,F_TEMP_R}); break;
    case 12: wantOnly({F_HAIR_TOP}); break;
    case 13: wantOnly({F_HAIR_L,F_HAIR_R}); break;
    case 14: wantOnly({F_BROW_L,F_BROW_R,F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R}); break;
    case 15: wantOnly({F_EAR_L,F_EAR_R,F_CHIN}); break;
    case 16: wantOnly({F_FACE,F_TIE_TOP,F_TIE_BOTTOM}); break;
    case 17: {
      std::vector<int> ids; for(int i=0;i<F_COUNT;++i) ids.push_back(i);
      wantOnlyVec(ids);
    } break;
    case 18: {
      std::vector<int> ids; for(int i=0;i<F_COUNT;++i) ids.push_back(i);
      wantOnlyVec(ids);
    } break;
    case 19: {
      std::vector<int> ids; for(int i=0;i<F_COUNT;++i) ids.push_back(i);
      wantOnlyVec(ids);
    } break;
  }

  // Animate special motions
  switch(si){
    case 5: {
      auto &EL=A(F_EYE_L), &ER=A(F_EYE_R);
      float k=(float)easeBackIn(e);
      EL.x = (float)mixd(-1.2, EL.tx, k); EL.y=(float)mixd(+0.9, EL.ty, k);
      ER.x = (float)mixd(+1.2, ER.tx, k); ER.y=(float)mixd(-0.9, ER.ty, k);
    } break;
    case 6: {
      auto &PL=A(F_PUPIL_L), &PR=A(F_PUPIL_R);
      float r=0.07f;
      PL.x = (float)mixd(-0.9, PL.tx, easeExpoOut(e)) + r*(float)sin(2*PI*(bt*0.8));
      PL.y = PL.ty + r*(float)cos(2*PI*(bt*0.6));
      PR.x = (float)mixd(+0.9, PR.tx, easeExpoOut(e)) + r*(float)cos(2*PI*(bt*0.7));
      PR.y = PR.ty + r*(float)sin(2*PI*(bt*0.9));
    } break;
    case 7: {
      auto &GL=A(F_GLAS_L), &GR=A(F_GLAS_R), &GB=A(F_GLAS_BR), &AL=A(F_ARM_L), &AR=A(F_ARM_R);
      auto &HL=A(F_GLAS_HL_L), &HR=A(F_GLAS_HL_R);
      float k=(float)easeExpoOut(e);
      GL.x=(float)mixd(-1.5, GL.tx, k); GL.y=GL.ty;
      GR.x=(float)mixd(+1.5, GR.tx, k); GR.y=GR.ty;
      GB.x=(float)mixd( 0.0, GB.tx, k); GB.y=GB.ty;
      AL.x=(float)mixd(-1.7, AL.tx, k); AL.y=AL.ty;
      AR.x=(float)mixd(+1.7, AR.tx, k); AR.y=AR.ty;
      HL.x=GL.x; HL.y=GL.y; HR.x=GR.x; HR.y=GR.y;
    } break;
    case 9: {
      auto &N=A(F_NOSE);
      float k = (float)easeCubic(e);
      float hops = 0.18f * hopWave(bt, 0.0);
      N.x = (float)mixd(-1.25, N.tx, k);
      N.y = (float)mixd(N.ty+0.30, N.ty, k) + hops;
      N.ang = (float)mixd(-10.0, 0.0, k);
      N.sx = N.tsx*(1.0f + 0.05f*(float)pulse1(bt));
      N.sy = N.tsy*(1.0f - 0.04f*(float)pulse1(bt));
    } break;
    case 17: {
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
    case 18: {
      int keys[] = {F_BROW_L,F_BROW_R,F_EYE_L,F_EYE_R,F_PUPIL_L,F_PUPIL_R,F_GLAS_L,F_GLAS_R,F_GLAS_BR,F_MOUTH,F_NOSE};
      for(int id:keys){
        auto& kf=G[id]; float br = 1.0f + 0.05f*(float)pulse1(bt);
        kf.sx = kf.tsx * br; kf.sy = kf.tsy * (1.0f/br);
      }
      for(int i=0;i<F_COUNT;++i){ auto& a=G[i];
        a.x  = a.tx + 0.008f*(float)sin(2*PI*(bt*0.20 + i*0.03));
        a.y  = a.ty + 0.008f*(float)cos(2*PI*(bt*0.17 + i*0.04));
        a.ang= a.tang + 1.8f*(float)sin(2*PI*(bt*0.08 + i*0.02));
      }
    } break;
    default: break;
  }

  // Update alpha & personas
  for(int i=0;i<F_COUNT;++i){
    auto& a=G[i];
    a.alpha += (VIS[i] - a.alpha) * 0.14f;
    if(a.alpha<0.002f) a.alpha=0.0f;
    if(a.alpha>0.998f) a.alpha=1.0f;
    applyPersona(a, bt);
  }

  // Draw actors
  drawAllWithTop(bt);

  // Spotlights
  if(si==1) spotlight(A(F_FACE).tx, A(F_FACE).ty, (float)mixd(1.8,0.65,e));
  if(si==2) spotlight(A(F_MOUTH).tx, A(F_MOUTH).ty, (float)mixd(1.6,0.60,e));
  if(si==3) spotlight(A(F_EYE_L).tx, A(F_EYE_L).ty, (float)mixd(1.4,0.60,e));

  // Scene overlay
  fadeOverlay(bt);
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
  drawBG();
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
  glutCreateWindow("Monsters-style Cubist Intro — Take Five v5d");
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
