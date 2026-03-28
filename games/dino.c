#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <math.h>

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>
    EM_JS(int, GetBrowserWidth, (), { return window.innerWidth; });
    EM_JS(int, GetBrowserHeight, (), { return window.innerHeight; });
    EM_JS(void, JSonGameOver, (int score, int best), {
        var token = localStorage.getItem("urStore_token");
        fetch("https://urstore.dev/api/game", {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": token || ""
            },
            body: JSON.stringify({ win: false })
        }).finally(function() {
            setTimeout(function() { window.parent.postMessage("close", "*"); }, 3000);
        });
    });
    EM_JS(void, JSonGameWin, (int score, int best), {
        var token = localStorage.getItem("urStore_token");
        fetch("https://urstore.dev/api/game", {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                "Authorization": token || ""
            },
            body: JSON.stringify({ win: true })
        }).finally(function() {
            setTimeout(function() { window.parent.postMessage("close", "*"); }, 3000);
        });
    });
#endif

#define INIT_W        800
#define INIT_H        500
#define MAX_OBS       12
#define MAX_PART      200
#define MAX_CLOUDS    10
#define MAX_DECOR     20

#define GROUND_Y      0.0f
#define GRAVITY       30.0f
#define JUMP_VEL      12.5f
#define DINO_X        0.0f
#define BASE_SPEED    8.0f
#define MAX_SPEED     17.0f
#define SPEED_INC     0.12f

typedef enum { ST_MENU, ST_PLAY, ST_DEAD, ST_WIN } GameState;
#define WIN_SCORE 500
typedef enum { OBS_CACT_S, OBS_CACT_T, OBS_CACT_G, OBS_BIRD } ObsType;

typedef struct {
    float y, vel, legAnim, blinkT;
    int jumping, ducking, blinking;
} Dino;

typedef struct {
    float x, birdY, hue, wingA;
    ObsType type;
    int active;
} Obstacle;

typedef struct {
    Vector3 pos, vel;
    float life, size;
    Color col;
    int active;
} Particle;

typedef struct { float x, y, z, spd, scl; } Cloud;
typedef struct { float x, z, scl; Color col; int type; } Decor;

static GameState state = ST_MENU;
static Dino dino;
static Obstacle obs[MAX_OBS];
static Particle parts[MAX_PART];
static Cloud clouds[MAX_CLOUDS];
static Decor decors[MAX_DECOR];
static Camera3D cam;

static int score;
static int jsCalled;
static float gTime, gameSpd, dist;
static float shakeT, flashT, deadT;
static int pIdx;
static float scoreScl;
static int lastMile;

static Color Hsv(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r=c; g=x; b=0; }
    else if (h < 120) { r=x; g=c; b=0; }
    else if (h < 180) { r=0; g=c; b=x; }
    else if (h < 240) { r=0; g=x; b=c; }
    else if (h < 300) { r=x; g=0; b=c; }
    else              { r=c; g=0; b=x; }
    return (Color){ (unsigned char)((r+m)*255), (unsigned char)((g+m)*255),
                    (unsigned char)((b+m)*255), 255 };
}

static float Rf(float lo, float hi) {
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}

static void EmitP(float x, float y, float z, Vector3 vBase, Color c, float lifeMax, float sizeMax) {
    Particle *p = &parts[pIdx % MAX_PART];
    p->active = 1;
    p->pos = (Vector3){x, y, z};
    p->vel = (Vector3){ vBase.x + Rf(-1.0f, 1.0f), vBase.y + Rf(-0.5f, 1.5f), vBase.z + Rf(-0.8f, 0.8f) };
    p->life = Rf(lifeMax * 0.4f, lifeMax);
    p->size = Rf(sizeMax * 0.3f, sizeMax);
    p->col = c;
    pIdx++;
}

static void EmitBurst(float x, float y, int n, float hueBase) {
    for (int i = 0; i < n; i++) {
        Color c = Hsv(fmodf(hueBase + i * 25.0f, 360.0f), 0.9f, 1.0f);
        Particle *p = &parts[pIdx % MAX_PART];
        p->active = 1;
        p->pos = (Vector3){x, y, 0};
        p->vel = (Vector3){Rf(-4, 4), Rf(-1, 6), Rf(-2.5f, 2.5f)};
        p->life = Rf(0.5f, 1.3f);
        p->size = Rf(0.06f, 0.2f);
        p->col = c;
        pIdx++;
    }
}

static void EmitDust(float x, float y, float z) {
    for (int i = 0; i < 6; i++) {
        Particle *p = &parts[pIdx % MAX_PART];
        p->active = 1;
        p->pos = (Vector3){x + Rf(-0.2f, 0.2f), y + Rf(0, 0.1f), z + Rf(-0.3f, 0.3f)};
        p->vel = (Vector3){Rf(-2.0f, -0.3f), Rf(0.5f, 2.5f), Rf(-0.5f, 0.5f)};
        p->life = Rf(0.3f, 0.7f);
        p->size = Rf(0.04f, 0.1f);
        p->col = (Color){210, 190, 150, 200};
        pIdx++;
    }
}

static void UpdateParts(float dt) {
    for (int i = 0; i < MAX_PART; i++) {
        if (!parts[i].active) continue;
        parts[i].life -= dt;
        if (parts[i].life <= 0) { parts[i].active = 0; continue; }
        parts[i].pos.x += parts[i].vel.x * dt;
        parts[i].pos.y += parts[i].vel.y * dt;
        parts[i].pos.z += parts[i].vel.z * dt;
        parts[i].vel.y -= 7.0f * dt;
        parts[i].size *= 0.99f;
    }
}

static void DrawParts3D(void) {
    for (int i = 0; i < MAX_PART; i++) {
        if (!parts[i].active) continue;
        float a = parts[i].life * 2.5f;
        if (a > 1.0f) a = 1.0f;
        Color c = parts[i].col;
        c.a = (unsigned char)(a * 255);
        DrawCube(parts[i].pos, parts[i].size, parts[i].size, parts[i].size, c);
    }
}

static void InitClouds(void) {
    for (int i = 0; i < MAX_CLOUDS; i++)
        clouds[i] = (Cloud){ Rf(-25, 35), Rf(6, 15), Rf(-14, -5), Rf(0.3f, 0.7f), Rf(0.6f, 1.6f) };
}

static void UpdateClouds(float dt) {
    for (int i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].x -= clouds[i].spd * dt;
        if (clouds[i].x < -30) {
            clouds[i].x = 36;
            clouds[i].y = Rf(6, 15);
            clouds[i].scl = Rf(0.6f, 1.6f);
        }
    }
}

static void DrawClouds3D(void) {
    Color w = {255, 255, 255, 225};
    for (int i = 0; i < MAX_CLOUDS; i++) {
        Cloud *c = &clouds[i];
        float s = c->scl;
        DrawSphere((Vector3){c->x, c->y, c->z}, 0.6f*s, w);
        DrawSphere((Vector3){c->x + 0.55f*s, c->y - 0.1f, c->z}, 0.45f*s, w);
        DrawSphere((Vector3){c->x - 0.55f*s, c->y - 0.08f, c->z}, 0.5f*s, w);
        DrawSphere((Vector3){c->x + 0.12f*s, c->y + 0.3f*s, c->z}, 0.35f*s, w);
    }
}

static void InitDecors(void) {
    for (int i = 0; i < MAX_DECOR; i++) {
        decors[i].x = Rf(-5, 30);
        decors[i].z = Rf(1.5f, 5.5f) * (GetRandomValue(0, 1) ? 1.0f : -1.0f);
        decors[i].scl = Rf(0.08f, 0.2f);
        decors[i].col = Hsv(Rf(0, 360), 0.85f, 1.0f);
        decors[i].type = GetRandomValue(0, 2);
    }
}

static void UpdateDecors(float dt) {
    for (int i = 0; i < MAX_DECOR; i++) {
        decors[i].x -= gameSpd * dt;
        if (decors[i].x < -12) {
            decors[i].x = Rf(22, 38);
            decors[i].z = Rf(1.5f, 5.5f) * (GetRandomValue(0, 1) ? 1.0f : -1.0f);
            decors[i].scl = Rf(0.08f, 0.2f);
            decors[i].col = Hsv(Rf(0, 360), 0.85f, 1.0f);
            decors[i].type = GetRandomValue(0, 2);
        }
    }
}

static void DrawDecors3D(void) {
    for (int i = 0; i < MAX_DECOR; i++) {
        Decor *d = &decors[i];
        float gx = d->x, gz = d->z, s = d->scl * 8.0f;
        switch (d->type) {
        case 0:
            DrawCylinder((Vector3){gx, GROUND_Y, gz}, 0.012f*s, 0.012f*s, 0.14f*s, 5, (Color){80, 180, 45, 255});
            DrawSphere((Vector3){gx, GROUND_Y + 0.14f*s, gz}, 0.06f*s, d->col);
            DrawSphere((Vector3){gx, GROUND_Y + 0.14f*s + 0.025f*s, gz}, 0.03f*s, YELLOW);
            break;
        case 1:
            DrawSphere((Vector3){gx, GROUND_Y + s*0.035f, gz}, s*0.05f, (Color){165, 155, 140, 255});
            break;
        case 2:
            DrawCylinder((Vector3){gx, GROUND_Y, gz}, 0.015f*s, 0.02f*s, 0.1f*s, 5, (Color){225, 205, 175, 255});
            DrawSphere((Vector3){gx, GROUND_Y + 0.11f*s, gz}, 0.055f*s, d->col);
            DrawSphere((Vector3){gx + 0.025f*s, GROUND_Y + 0.12f*s, gz + 0.01f*s}, 0.018f*s, WHITE);
            break;
        }
    }
}

static void ResetGame(void) {
    dino = (Dino){ GROUND_Y, 0, 0, Rf(2, 4), 0, 0, 0 };
    for (int i = 0; i < MAX_OBS; i++) obs[i].active = 0;
    for (int i = 0; i < MAX_PART; i++) parts[i].active = 0;
    score = 0;
    jsCalled = 0;
    gameSpd = BASE_SPEED;
    dist = 0;
    gTime = 0;
    shakeT = flashT = deadT = 0;
    pIdx = 0;
    scoreScl = 1.0f;
    lastMile = 0;
}

static void InitGame(void) {
    cam.position   = (Vector3){ -3.5f, 5.0f, 10.0f };
    cam.target     = (Vector3){  4.0f, 1.5f,  0.0f };
    cam.up         = (Vector3){  0.0f, 1.0f,  0.0f };
    cam.fovy       = 50.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    InitClouds();
    InitDecors();
    ResetGame();
}

static void DrawDino3D(void) {
    float y = dino.y;
    int dead = (state == ST_DEAD);

    Color body  = {80,  200, 80,  255};
    Color belly = {160, 235, 145, 255};
    Color dark  = {50,  160, 50,  255};
    Color blush = {255, 140, 140, 200};

    rlPushMatrix();
    rlTranslatef(DINO_X, y, 0.0f);

    if (dead) {
        float fall = fminf(deadT * 3.0f, 1.0f);
        rlRotatef(-90.0f * fall, 0, 0, 1);
    } else if (dino.jumping) {
        rlRotatef(Clamp(dino.vel * 2.5f, -25, 30), 0, 0, 1);
    }

    if (!dead && state == ST_PLAY && !dino.jumping)
        rlRotatef(sinf(dino.legAnim) * 2.5f, 0, 0, 1);

    if (dino.ducking) rlScalef(1.2f, 0.6f, 1.2f);

    DrawSphere((Vector3){0, 0.85f, 0}, 0.48f, body);
    DrawSphere((Vector3){0.08f, 0.68f, 0}, 0.35f, belly);

    float hb = (!dino.jumping && !dead && state == ST_PLAY) ? sinf(dino.legAnim * 2.0f) * 0.035f : 0;
    float hx = 0.28f, hy = 1.4f + hb;
    DrawSphere((Vector3){hx, hy, 0}, 0.40f, body);
    DrawSphere((Vector3){hx + 0.32f, hy - 0.08f, 0}, 0.20f, (Color){105, 215, 105, 255});
    DrawSphere((Vector3){hx + 0.48f, hy - 0.01f, 0.06f}, 0.028f, dark);
    DrawSphere((Vector3){hx + 0.48f, hy - 0.01f, -0.06f}, 0.028f, dark);

    if (!dead) {
        DrawSphere((Vector3){hx + 0.18f, hy + 0.18f, 0.22f}, 0.15f, WHITE);
        DrawSphere((Vector3){hx + 0.18f, hy + 0.18f, -0.22f}, 0.15f, WHITE);
        if (!dino.blinking) {
            DrawSphere((Vector3){hx + 0.24f, hy + 0.20f, 0.28f}, 0.075f, (Color){30,30,30,255});
            DrawSphere((Vector3){hx + 0.26f, hy + 0.23f, 0.30f}, 0.028f, WHITE);
            DrawSphere((Vector3){hx + 0.24f, hy + 0.20f, -0.28f}, 0.075f, (Color){30,30,30,255});
            DrawSphere((Vector3){hx + 0.26f, hy + 0.23f, -0.30f}, 0.028f, WHITE);
        } else {
            DrawCube((Vector3){hx + 0.20f, hy + 0.17f, 0.23f}, 0.18f, 0.03f, 0.05f, dark);
            DrawCube((Vector3){hx + 0.20f, hy + 0.17f, -0.23f}, 0.18f, 0.03f, 0.05f, dark);
        }
    } else {
        Color xc = {200, 50, 50, 255};
        DrawCube((Vector3){hx+0.22f, hy+0.18f, 0.24f}, 0.16f, 0.035f, 0.035f, xc);
        DrawCube((Vector3){hx+0.22f, hy+0.18f, 0.24f}, 0.035f, 0.16f, 0.035f, xc);
        DrawCube((Vector3){hx+0.22f, hy+0.18f, -0.24f}, 0.16f, 0.035f, 0.035f, xc);
        DrawCube((Vector3){hx+0.22f, hy+0.18f, -0.24f}, 0.035f, 0.16f, 0.035f, xc);
    }

    if (!dead) {
        DrawSphere((Vector3){hx + 0.26f, hy - 0.06f, 0.33f}, 0.075f, blush);
        DrawSphere((Vector3){hx + 0.26f, hy - 0.06f, -0.33f}, 0.075f, blush);
    }

    if (!dead) {
        DrawCube((Vector3){hx + 0.44f, hy - 0.18f, 0}, 0.08f, 0.022f, 0.1f, (Color){40,40,40,255});
        DrawCube((Vector3){hx + 0.41f, hy - 0.16f, 0.055f}, 0.035f, 0.022f, 0.018f, (Color){40,40,40,255});
        DrawCube((Vector3){hx + 0.41f, hy - 0.16f, -0.055f}, 0.035f, 0.022f, 0.018f, (Color){40,40,40,255});
    } else {
        DrawCube((Vector3){hx + 0.44f, hy - 0.2f, 0}, 0.08f, 0.022f, 0.08f, (Color){40,40,40,255});
    }

    float as = sinf(gTime * 5.0f) * 12.0f;
    rlPushMatrix();
    rlTranslatef(0.22f, 0.78f, 0.36f);
    rlRotatef(20.0f + as, 0, 0, 1);
    DrawCube((Vector3){0.06f, -0.05f, 0}, 0.11f, 0.065f, 0.065f, body);
    DrawSphere((Vector3){0.11f, -0.06f, 0}, 0.038f, belly);
    rlPopMatrix();
    rlPushMatrix();
    rlTranslatef(0.22f, 0.78f, -0.36f);
    rlRotatef(20.0f - as, 0, 0, 1);
    DrawCube((Vector3){0.06f, -0.05f, 0}, 0.11f, 0.065f, 0.065f, body);
    DrawSphere((Vector3){0.11f, -0.06f, 0}, 0.038f, belly);
    rlPopMatrix();

    float la = dino.jumping ? 0 : dino.legAnim;
    float lL = sinf(la) * 30.0f;
    float rL = sinf(la + PI) * 30.0f;
    if (dead) { lL = 0; rL = 0; }

    rlPushMatrix();
    rlTranslatef(-0.05f, 0.44f, 0.22f);
    rlRotatef(lL, 0, 0, 1);
    DrawCube((Vector3){0, -0.14f, 0}, 0.15f, 0.28f, 0.14f, dark);
    DrawCube((Vector3){0.04f, -0.30f, 0}, 0.19f, 0.09f, 0.17f, body);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(-0.05f, 0.44f, -0.22f);
    rlRotatef(rL, 0, 0, 1);
    DrawCube((Vector3){0, -0.14f, 0}, 0.15f, 0.28f, 0.14f, dark);
    DrawCube((Vector3){0.04f, -0.30f, 0}, 0.19f, 0.09f, 0.17f, body);
    rlPopMatrix();

    float tw = sinf(gTime * 4.5f) * 0.15f;
    DrawSphere((Vector3){-0.42f, 0.9f, tw}, 0.22f, body);
    DrawSphere((Vector3){-0.62f, 1.0f, tw * 1.6f}, 0.16f, body);
    DrawSphere((Vector3){-0.78f, 1.08f, tw * 2.2f}, 0.11f, dark);

    float plateHues[] = {0, 35, 55, 180, 280};
    for (int i = 0; i < 5; i++) {
        float px = -0.15f + i * 0.14f;
        float py = 1.22f + sinf(i * 1.0f) * 0.05f;
        float ps = 0.065f + (i == 2 ? 0.025f : 0);
        Color pc = Hsv(plateHues[i], 0.85f, 1.0f);
        DrawCube((Vector3){px, py + ps, 0}, ps * 0.55f, ps * 2.0f, ps * 0.4f, pc);
    }

    rlPopMatrix();
}

static void DrawCactus(float x, float h, float r, float hue) {
    Color base  = Hsv(hue, 0.7f, 0.88f);
    Color light = Hsv(hue, 0.45f, 1.0f);
    Color dark  = Hsv(hue, 0.85f, 0.55f);

    DrawCylinder((Vector3){x, GROUND_Y, 0}, r, r * 0.82f, h, 10, base);
    DrawCylinder((Vector3){x, GROUND_Y + h, 0}, r * 0.82f, 0, 0.14f, 8, light);
    DrawCylinder((Vector3){x + r * 0.55f, GROUND_Y, r * 0.18f}, 0.018f, 0.018f, h * 0.75f, 4, light);

    int rings = (int)(h / 0.55f);
    for (int i = 0; i < rings; i++) {
        float ry = GROUND_Y + 0.25f + i * 0.55f;
        if (ry > GROUND_Y + h - 0.15f) break;
        DrawCylinder((Vector3){x, ry, 0}, r + 0.015f, r + 0.015f, 0.035f, 8, dark);
    }

    for (int i = 0; i < 4; i++) {
        float a = i * 1.7f + hue * 0.01f;
        float sy = GROUND_Y + h * 0.25f + i * h * 0.18f;
        DrawSphere((Vector3){x + cosf(a)*r*0.8f, sy, sinf(a)*r*0.8f}, 0.035f, light);
    }

    DrawSphere((Vector3){x, GROUND_Y + h + 0.08f, 0}, 0.04f, (Color){255, 230, 100, 255});
}

static void DrawObs3D(Obstacle *o) {
    if (!o->active) return;
    switch (o->type) {
    case OBS_CACT_S:
        DrawCactus(o->x, 1.0f, 0.22f, o->hue);
        break;
    case OBS_CACT_T:
        DrawCactus(o->x, 1.85f, 0.2f, o->hue);
        rlPushMatrix();
        rlTranslatef(o->x, GROUND_Y + 1.0f, 0);
        rlRotatef(-30, 0, 0, 1);
        DrawCylinder((Vector3){0.18f, 0, 0}, 0.07f, 0.05f, 0.5f, 6, Hsv(o->hue, 0.7f, 0.88f));
        DrawSphere((Vector3){0.18f, 0.5f, 0}, 0.08f, Hsv(o->hue, 0.45f, 1.0f));
        rlPopMatrix();
        break;
    case OBS_CACT_G:
        DrawCactus(o->x - 0.38f, 1.1f, 0.19f, o->hue);
        DrawCactus(o->x + 0.0f, 1.35f, 0.21f, fmodf(o->hue + 45, 360));
        DrawCactus(o->x + 0.38f, 0.9f, 0.17f, fmodf(o->hue + 90, 360));
        break;
    case OBS_BIRD: {
        float bx = o->x, by = o->birdY;
        Color bc = Hsv(o->hue, 0.75f, 0.95f);
        Color bw = Hsv(fmodf(o->hue + 30, 360), 0.6f, 1.0f);

        DrawSphere((Vector3){bx, by, 0}, 0.24f, bc);
        DrawSphere((Vector3){bx + 0.22f, by + 0.12f, 0}, 0.16f, bc);
        DrawCube((Vector3){bx + 0.38f, by + 0.1f, 0}, 0.12f, 0.045f, 0.06f, ORANGE);

        DrawSphere((Vector3){bx + 0.28f, by + 0.2f, 0.1f}, 0.058f, WHITE);
        DrawSphere((Vector3){bx + 0.30f, by + 0.21f, 0.12f}, 0.03f, (Color){30,30,30,255});
        DrawSphere((Vector3){bx + 0.28f, by + 0.2f, -0.1f}, 0.058f, WHITE);
        DrawSphere((Vector3){bx + 0.30f, by + 0.21f, -0.12f}, 0.03f, (Color){30,30,30,255});

        float wa = o->wingA;
        rlPushMatrix();
        rlTranslatef(bx - 0.04f, by + 0.04f, 0.2f);
        rlRotatef(wa, 1, 0, 0);
        DrawCube((Vector3){0, 0, 0.2f}, 0.28f, 0.045f, 0.38f, bw);
        rlPopMatrix();
        rlPushMatrix();
        rlTranslatef(bx - 0.04f, by + 0.04f, -0.2f);
        rlRotatef(-wa, 1, 0, 0);
        DrawCube((Vector3){0, 0, -0.2f}, 0.28f, 0.045f, 0.38f, bw);
        rlPopMatrix();

        DrawCube((Vector3){bx - 0.26f, by + 0.04f, 0}, 0.14f, 0.06f, 0.04f, bw);
        break;
    }
    }
}

static void DrawGround3D(void) {
    DrawCube((Vector3){5, GROUND_Y - 0.15f, 0}, 65, 0.3f, 18, (Color){95, 200, 65, 255});
    DrawCube((Vector3){5, GROUND_Y - 0.005f, 0}, 65, 0.01f, 18, (Color){78, 178, 52, 255});
    DrawCube((Vector3){5, GROUND_Y - 0.55f, 0}, 65, 0.5f, 18, (Color){165, 118, 62, 255});
    DrawCube((Vector3){5, GROUND_Y - 1.3f, 0}, 65, 1.0f, 18, (Color){125, 98, 58, 255});

    float off = fmodf(dist, 3.0f);
    for (int i = -10; i < 16; i++) {
        float sx = i * 3.0f - off;
        DrawCube((Vector3){sx, GROUND_Y + 0.005f, 0}, 0.07f, 0.008f, 10, (Color){82, 185, 58, 100});
    }

    float off2 = fmodf(dist, 2.0f);
    for (int i = -8; i < 22; i++) {
        float dx = i * 2.0f - off2;
        DrawCube((Vector3){dx, GROUND_Y + 0.008f, 0}, 0.7f, 0.004f, 0.055f, (Color){255,255,255,50});
    }
}

static void DrawBackground3D(void) {
    Color hc[] = {{60,160,80,255},{50,145,72,255},{72,178,92,255},{48,135,68,255},{82,188,98,255}};
    for (int i = 0; i < 5; i++) {
        float hxx = -16.0f + i * 9.5f;
        float hz  = -8.0f - (float)(i % 3) * 2.0f;
        float hr  = 3.2f + (float)(i % 3) * 1.2f;
        DrawSphere((Vector3){hxx, GROUND_Y, hz}, hr, hc[i]);
    }
    for (int i = 0; i < 6; i++) {
        float tx = -13.0f + i * 5.8f;
        float tz = -5.5f - (float)(i % 3) * 1.1f;
        float th = 1.3f + (float)(i % 3) * 0.4f;
        DrawCylinder((Vector3){tx, GROUND_Y, tz}, 0.08f, 0.12f, th, 6, (Color){135,92,48,255});
        float lr = 0.52f + (float)(i % 2) * 0.2f;
        Color lc = {(unsigned char)(38+i*8), (unsigned char)(145+i*12), (unsigned char)(38+i*5), 255};
        DrawSphere((Vector3){tx, GROUND_Y + th + lr * 0.3f, tz}, lr, lc);
    }
    for (int i = 0; i < 3; i++) {
        float mx = -10.0f + i * 16.0f;
        Color mc = {(unsigned char)(85+i*18), (unsigned char)(125+i*14), (unsigned char)(165+i*10), 160};
        DrawCylinder((Vector3){mx, GROUND_Y, -16.0f}, 0.1f, 4.5f + i, 5.5f + i * 1.5f, 6, mc);
    }
}

static void DrawSun3D(void) {
    DrawSphere((Vector3){22, 15, -15}, 2.4f, (Color){255, 248, 165, 235});
    DrawSphere((Vector3){22, 15, -15}, 3.2f, (Color){255, 228, 115, 65});
    DrawSphere((Vector3){22, 15, -15}, 4.2f, (Color){255, 205, 85, 25});
}

static void EnsureObstacles(void) {
    int safety = 0;
    while (safety < 4) {
        float maxX = -999;
        for (int i = 0; i < MAX_OBS; i++)
            if (obs[i].active && obs[i].x > maxX) maxX = obs[i].x;
        if (maxX >= 22.0f) break;

        float baseGap = 5.0f + gameSpd * 0.18f;
        float spawnX = (maxX < 0) ? 14.0f : maxX + Rf(baseGap, baseGap + 3.5f);
        if (spawnX < 12.0f) spawnX = 12.0f;

        int found = 0;
        for (int i = 0; i < MAX_OBS; i++) {
            if (!obs[i].active) {
                obs[i].active = 1;
                obs[i].x = spawnX;
                obs[i].hue = Rf(0, 360);
                obs[i].wingA = 0;
                int roll = GetRandomValue(0, 100);
                if (score > 150 && roll < 38) {
                    obs[i].type = OBS_CACT_G;
                } else if (roll < 65) {
                    obs[i].type = OBS_CACT_T;
                } else {
                    obs[i].type = OBS_CACT_S;
                }
                found = 1;
                break;
            }
        }
        if (!found) break;
        safety++;
    }
}

static int CheckCol(void) {
    float dx0, dx1, dy0, dy1;
    if (dino.ducking) {
        dx0 = DINO_X - 0.28f; dx1 = DINO_X + 0.32f;
        dy0 = dino.y;          dy1 = dino.y + 0.7f;
    } else {
        dx0 = DINO_X - 0.22f; dx1 = DINO_X + 0.28f;
        dy0 = dino.y;          dy1 = dino.y + 1.4f;
    }

    for (int i = 0; i < MAX_OBS; i++) {
        if (!obs[i].active) continue;
        float ox0, ox1, oy0, oy1;
        switch (obs[i].type) {
        case OBS_CACT_S:
            ox0 = obs[i].x - 0.24f; ox1 = obs[i].x + 0.24f;
            oy0 = GROUND_Y;          oy1 = GROUND_Y + 1.0f;
            break;
        case OBS_CACT_T:
            ox0 = obs[i].x - 0.22f; ox1 = obs[i].x + 0.22f;
            oy0 = GROUND_Y;          oy1 = GROUND_Y + 1.85f;
            break;
        case OBS_CACT_G:
            ox0 = obs[i].x - 0.58f; ox1 = obs[i].x + 0.58f;
            oy0 = GROUND_Y;          oy1 = GROUND_Y + 1.35f;
            break;
        case OBS_BIRD:
            ox0 = obs[i].x - 0.3f;  ox1 = obs[i].x + 0.3f;
            oy0 = obs[i].birdY - 0.24f; oy1 = obs[i].birdY + 0.24f;
            break;
        default: continue;
        }
        if (dx0 < ox1 && dx1 > ox0 && dy0 < oy1 && dy1 > oy0) return 1;
    }
    return 0;
}

static void UpdateDrawFrame(void) {
#if defined(PLATFORM_WEB)
    int bw = GetBrowserWidth(), bh = GetBrowserHeight();
    if (bw != GetScreenWidth() || bh != GetScreenHeight()) SetWindowSize(bw, bh);
#endif

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    gTime += dt;

    int sw = GetScreenWidth(), sh = GetScreenHeight();
    int jumped = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)
               || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsGestureDetected(GESTURE_TAP);
    (void)0;

    if (scoreScl > 1.0f) { scoreScl -= dt * 3.0f; if (scoreScl < 1.0f) scoreScl = 1.0f; }

    switch (state) {
    case ST_MENU:
        dino.y = GROUND_Y + sinf(gTime * 2.0f) * 0.3f;
        dino.legAnim += dt * 8.0f;
        dino.blinkT -= dt;
        if (dino.blinkT <= 0) { dino.blinking = !dino.blinking; dino.blinkT = dino.blinking ? 0.15f : Rf(2, 5); }
        UpdateClouds(dt);
        if (jumped) { state = ST_PLAY; ResetGame(); dino.vel = JUMP_VEL * 0.4f; dino.jumping = 1; }
        break;

    case ST_PLAY:
        dino.vel -= GRAVITY * dt;
        dino.y += dino.vel * dt;

        if (dino.y <= GROUND_Y) {
            if (dino.jumping) { dino.jumping = 0; EmitDust(DINO_X, GROUND_Y, 0); }
            dino.y = GROUND_Y;
            dino.vel = 0;
        }

        if (jumped && !dino.jumping && !dino.ducking) {
            dino.vel = JUMP_VEL;
            dino.jumping = 1;
            EmitDust(DINO_X, GROUND_Y, 0);
        }

        dino.ducking = 0;
        if (!dino.jumping) dino.legAnim += dt * gameSpd * 1.8f;

        dino.blinkT -= dt;
        if (dino.blinkT <= 0) { dino.blinking = !dino.blinking; dino.blinkT = dino.blinking ? 0.12f : Rf(2.5f, 5.0f); }

        dist += gameSpd * dt;
        score = (int)(dist * 3.0f);

        if (score >= WIN_SCORE) {
            state = ST_WIN; deadT = 0;
            for (int j = 0; j < 45; j++) {
                Color wc = Hsv(Rf(0,360), 0.9f, 1.0f);
                Particle *wp = &parts[pIdx % MAX_PART];
                wp->active = 1;
                wp->pos = (Vector3){ Rf(-3,3), Rf(0,5), Rf(-2,2) };
                wp->vel = (Vector3){ Rf(-4,4), Rf(2,8), Rf(-3,3) };
                wp->life = Rf(1.0f, 2.5f);
                wp->size = Rf(0.1f, 0.25f);
                wp->col = wc;
                pIdx++;
            }
            break;
        }

        int mile = score / 100;
        if (mile > lastMile) { lastMile = mile; scoreScl = 1.8f; EmitBurst(DINO_X, dino.y + 1.6f, 28, Rf(0, 360)); }

        if (gameSpd < MAX_SPEED) { gameSpd += SPEED_INC * dt; if (gameSpd > MAX_SPEED) gameSpd = MAX_SPEED; }

        for (int i = 0; i < MAX_OBS; i++) {
            if (!obs[i].active) continue;
            obs[i].x -= gameSpd * dt;
            if (obs[i].type == OBS_BIRD) obs[i].wingA = sinf(gTime * 12.0f) * 40.0f;
            if (obs[i].x < -12) obs[i].active = 0;
        }
        EnsureObstacles();

        if (CheckCol()) {
            state = ST_DEAD; shakeT = 0.4f; flashT = 0.25f; deadT = 0;
            EmitBurst(DINO_X, dino.y + 0.8f, 35, 0);
        }

        if (!dino.jumping && ((int)(gTime * 22)) % 3 == 0) {
            EmitP(DINO_X - 0.15f, GROUND_Y + 0.04f, Rf(-0.2f, 0.2f),
                  (Vector3){-1.5f, 0.8f, 0}, (Color){195, 175, 135, 180}, 0.5f, 0.08f);
        }

        UpdateParts(dt);
        UpdateClouds(dt);
        UpdateDecors(dt);
        break;

    case ST_DEAD:
        deadT += dt;
        if (shakeT > 0) shakeT -= dt;
        if (flashT > 0) flashT -= dt;
        dino.vel -= GRAVITY * dt;
        dino.y += dino.vel * dt;
        if (dino.y <= GROUND_Y) { dino.y = GROUND_Y; dino.vel = 0; }
        UpdateParts(dt);
        UpdateClouds(dt);
#if defined(PLATFORM_WEB)
        if (!jsCalled && deadT > 0.8f) { jsCalled = 1; JSonGameOver(score, 0); }
#endif
        break;

    case ST_WIN:
        deadT += dt;
        dino.y = GROUND_Y + sinf(gTime * 2.5f) * 0.4f;
        dino.legAnim += dt * 6.0f;
        dino.blinkT -= dt;
        if (dino.blinkT <= 0) { dino.blinking = !dino.blinking; dino.blinkT = dino.blinking ? 0.12f : Rf(2, 4); }

        if (((int)(gTime * 15)) % 3 == 0) {
            Color fc = Hsv(fmodf(gTime * 100.0f, 360.0f), 0.9f, 1.0f);
            Particle *fp = &parts[pIdx % MAX_PART];
            fp->active = 1;
            fp->pos = (Vector3){ Rf(-6, 10), Rf(-2, 6), Rf(-3, 3) };
            fp->vel = (Vector3){ Rf(-1, 1), Rf(2, 5), Rf(-1, 1) };
            fp->life = Rf(0.8f, 1.8f);
            fp->size = Rf(0.08f, 0.2f);
            fp->col = fc;
            pIdx++;
        }

        UpdateParts(dt);
        UpdateClouds(dt);
#if defined(PLATFORM_WEB)
        if (!jsCalled && deadT > 1.2f) { jsCalled = 1; JSonGameWin(score, 0); }
#endif
        break;
    }

    BeginDrawing();
    DrawRectangleGradientV(0, 0, sw, sh, (Color){85, 165, 245, 255}, (Color){195, 228, 255, 255});

    Camera3D dc = cam;
    if (shakeT > 0) {
        float s = shakeT / 0.4f;
        dc.position.x += Rf(-0.3f, 0.3f) * s;
        dc.position.y += Rf(-0.3f, 0.3f) * s;
    }

    BeginMode3D(dc);
    DrawSun3D();
    DrawBackground3D();
    DrawClouds3D();
    DrawGround3D();
    DrawDecors3D();
    for (int i = 0; i < MAX_OBS; i++) DrawObs3D(&obs[i]);
    DrawDino3D();
    DrawParts3D();
    EndMode3D();

    if (flashT > 0) {
        unsigned char a = (unsigned char)(flashT / 0.25f * 180);
        DrawRectangle(0, 0, sw, sh, (Color){255, 255, 255, a});
    }

    if (state == ST_PLAY || state == ST_DEAD || state == ST_WIN) {
        const char *st = TextFormat("%d", score);
        int fs = (int)(55 * scoreScl);
        int tw = MeasureText(st, fs);
        DrawText(st, sw/2 - tw/2 + 2, 22, fs, (Color){0,0,0,110});
        DrawText(st, sw/2 - tw/2, 20, fs, WHITE);
    }

    if (state == ST_MENU) {
        const char *title = "DINO RUN 3D";
        int ts = 58;
        int ttw = MeasureText(title, ts);
        Color tc = Hsv(fmodf(gTime * 55, 360), 0.7f, 1.0f);
        DrawText(title, sw/2 - ttw/2 + 3, sh/4 - 28 + 3, ts, (Color){0,0,0,140});
        DrawText(title, sw/2 - ttw/2, sh/4 - 28, ts, tc);

        const char *sub1 = "SPACE / Click to Jump";
        int sw1 = MeasureText(sub1, 22);
        DrawText(sub1, sw/2 - sw1/2, sh/2 + 20, 22, (Color){255,255,255,200});

        const char *start = "Press SPACE or Click to Start!";
        int startw = MeasureText(start, 26);
        float al = (sinf(gTime * 3.0f) + 1.0f) * 0.5f;
        DrawText(start, sw/2 - startw/2, sh - 68, 26,
            (Color){255, 255, 255, (unsigned char)(al * 255)});
    }

    if (state == ST_DEAD) {
        DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 100});
        const char *go = "GAME OVER";
        int gos = 62;
        int gow = MeasureText(go, gos);
        DrawText(go, sw/2 - gow/2 + 3, sh/2 - 78 + 3, gos, (Color){0,0,0,180});
        DrawText(go, sw/2 - gow/2, sh/2 - 78, gos, (Color){255, 88, 88, 255});

        const char *sf = TextFormat("Score: %d", score);
        int sfw = MeasureText(sf, 32);
        DrawText(sf, sw/2 - sfw/2, sh/2 - 2, 32, WHITE);
    }

    if (state == ST_WIN) {
        DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 80});

        const char *wt = "YOU WIN!";
        int wts = 68;
        int wtw = MeasureText(wt, wts);
        Color wc = Hsv(fmodf(gTime * 90.0f, 360.0f), 0.8f, 1.0f);
        DrawText(wt, sw/2 - wtw/2 + 3, sh/2 - 85 + 3, wts, (Color){0,0,0,180});
        DrawText(wt, sw/2 - wtw/2, sh/2 - 85, wts, wc);

        const char *ws = TextFormat("Score: %d", score);
        int wsw = MeasureText(ws, 32);
        DrawText(ws, sw/2 - wsw/2, sh/2 - 5, 32, WHITE);
    }

    EndDrawing();
}

int main(void) {
    int w = INIT_W, h = INIT_H;
#if defined(PLATFORM_WEB)
    w = GetBrowserWidth();
    h = GetBrowserHeight();
#endif
    InitWindow(w, h, "Dino Run 3D");
    InitGame();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) UpdateDrawFrame();
#endif

    CloseWindow();
    return 0;
}