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
#define INIT_H        450
#define MAX_PIPES     10
#define MAX_PARTICLES 150
#define MAX_CLOUDS    10
#define PIPE_GAP      3.5f
#define PIPE_SPEED    5.5f
#define GRAVITY       18.0f
#define JUMP_VEL      7.5f
#define PIPE_SPACING  7.0f
#define GROUND_Y      (-3.5f)
#define CEILING_Y     8.0f
#define BIRD_RADIUS   0.35f
#define PIPE_RADIUS   0.55f
#define CAP_RADIUS    0.72f

typedef enum { ST_MENU, ST_PLAY, ST_DEAD, ST_WIN } GameState;
#define WIN_SCORE 6

typedef struct {
    float y, velocity, rotation, wingAngle;
} Bird;

typedef struct {
    float x, gapY, hue;
    int active, scored;
} Pipe;

typedef struct {
    Vector3 pos, vel;
    float life, size;
    Color color;
    int active;
} Particle;

typedef struct {
    float x, y, z, speed, scale;
} Cloud;

static GameState state = ST_MENU;
static Bird bird;
static Pipe pipes[MAX_PIPES];
static Particle particles[MAX_PARTICLES];
static Cloud clouds[MAX_CLOUDS];
static Camera3D cam;
static int score = 0;
static float gTime = 0.0f, nextPipeX = 12.0f;
static float shakeTimer = 0.0f, flashTimer = 0.0f, deadTimer = 0.0f;
static int partIdx = 0;
static float scoreScale = 1.0f;
static int jsCalled = 0;

static Color HsvColor(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, g, b;
    if      (h < 60)  { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else               { r = c; g = 0; b = x; }
    return (Color){
        (unsigned char)((r + m) * 255.0f),
        (unsigned char)((g + m) * 255.0f),
        (unsigned char)((b + m) * 255.0f), 255
    };
}

static float RandF(float lo, float hi) {
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}

static void InitClouds(void) {
    for (int i = 0; i < MAX_CLOUDS; i++) {
        clouds[i] = (Cloud){
            RandF(-25, 35), RandF(4, 10), RandF(-12, -4),
            RandF(0.2f, 0.5f), RandF(0.6f, 1.4f)
        };
    }
}

static void ResetGame(void) {
    bird = (Bird){ 1.0f, 0, 0, 0 };
    jsCalled = 0;
    for (int i = 0; i < MAX_PIPES; i++) pipes[i].active = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = 0;
    score = 0;
    nextPipeX = 12.0f;
    gTime = 0;
    shakeTimer = flashTimer = deadTimer = 0;
    partIdx = 0;
    scoreScale = 1.0f;
}

static void InitGame(void) {
    cam.position = (Vector3){ -6.0f, 5.0f, 12.0f };
    cam.target   = (Vector3){  4.0f, 1.0f,  0.0f };
    cam.up       = (Vector3){  0.0f, 1.0f,  0.0f };
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    InitClouds();
    ResetGame();
}

static void EmitParticle(float x, float y, float z, Color c) {
    Particle *p = &particles[partIdx % MAX_PARTICLES];
    p->active = 1;
    p->pos = (Vector3){ x, y, z };
    p->vel = (Vector3){ RandF(-1.5f, 0.5f), RandF(-0.5f, 1.5f), RandF(-1.0f, 1.0f) };
    p->life = RandF(0.3f, 0.8f);
    p->size = RandF(0.04f, 0.12f);
    p->color = c;
    partIdx++;
}

static void EmitScoreBurst(float x, float y) {
    for (int i = 0; i < 25; i++) {
        Color c = HsvColor(RandF(0, 360), 0.9f, 1.0f);
        Particle *p = &particles[partIdx % MAX_PARTICLES];
        p->active = 1;
        p->pos = (Vector3){ x, y, 0 };
        p->vel = (Vector3){ RandF(-3.0f, 3.0f), RandF(-3.0f, 3.0f), RandF(-2.0f, 2.0f) };
        p->life = RandF(0.5f, 1.2f);
        p->size = RandF(0.08f, 0.2f);
        p->color = c;
        partIdx++;
    }
}

static void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        particles[i].life -= dt;
        if (particles[i].life <= 0) { particles[i].active = 0; continue; }
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].pos.z += particles[i].vel.z * dt;
        particles[i].vel.y -= 5.0f * dt;
        particles[i].size *= 0.985f;
    }
}

static void SpawnPipe(void) {
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!pipes[i].active) {
            pipes[i].active = 1;
            pipes[i].scored = 0;
            pipes[i].x = nextPipeX;
            pipes[i].gapY = RandF(-0.5f, 3.0f);
            pipes[i].hue = fmodf(nextPipeX * 25.0f, 360.0f);
            nextPipeX += PIPE_SPACING;
            return;
        }
    }
}

static void EnsurePipesAhead(void) {
    int safety = 0;
    while (safety < 5) {
        float maxX = -999.0f;
        for (int i = 0; i < MAX_PIPES; i++)
            if (pipes[i].active && pipes[i].x > maxX) maxX = pipes[i].x;
        if (maxX >= 20.0f) break;
        SpawnPipe();
        safety++;
    }
}

static void UpdateClouds(float dt) {
    for (int i = 0; i < MAX_CLOUDS; i++) {
        clouds[i].x -= clouds[i].speed * dt;
        if (clouds[i].x < -28.0f) {
            clouds[i].x = 35.0f;
            clouds[i].y = RandF(4, 10);
            clouds[i].scale = RandF(0.6f, 1.4f);
        }
    }
}

static void DrawBird3D(void) {
    rlPushMatrix();
    rlTranslatef(0.0f, bird.y, 0.0f);
    rlRotatef(-bird.rotation, 0.0f, 0.0f, 1.0f);

    DrawSphere((Vector3){0, 0, 0}, 0.4f, GOLD);

    DrawSphere((Vector3){0.05f, -0.1f, 0.0f}, 0.28f, (Color){255, 240, 180, 255});

    DrawSphere((Vector3){0.22f, 0.15f,  0.22f}, 0.12f, WHITE);
    DrawSphere((Vector3){0.27f, 0.17f,  0.24f}, 0.06f, (Color){30, 30, 30, 255});
    DrawSphere((Vector3){0.22f, 0.15f, -0.22f}, 0.12f, WHITE);
    DrawSphere((Vector3){0.27f, 0.17f, -0.24f}, 0.06f, (Color){30, 30, 30, 255});

    DrawCube((Vector3){0.42f,  0.0f,  0.0f}, 0.2f, 0.1f, 0.18f, (Color){255, 120, 30, 255});
    DrawCube((Vector3){0.42f, -0.06f, 0.0f}, 0.18f, 0.06f, 0.16f, (Color){220, 80, 20, 255});

    rlPushMatrix();
    rlTranslatef(-0.1f, 0.05f, 0.35f);
    rlRotatef(bird.wingAngle, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0, 0, 0.18f}, 0.28f, 0.06f, 0.35f, (Color){255, 200, 50, 255});

    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(-0.1f, 0.05f, -0.35f);
    rlRotatef(-bird.wingAngle, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){0, 0, -0.18f}, 0.28f, 0.06f, 0.35f, (Color){255, 200, 50, 255});

    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(-0.38f, 0.1f, 0.0f);
    DrawCube((Vector3){0, 0, 0}, 0.14f, 0.2f, 0.08f, (Color){255, 160, 30, 255});
    DrawCube((Vector3){-0.05f, 0.05f,  0.06f}, 0.1f, 0.16f, 0.05f, (Color){255, 180, 50, 255});
    DrawCube((Vector3){-0.05f, 0.05f, -0.06f}, 0.1f, 0.16f, 0.05f, (Color){255, 180, 50, 255});
    rlPopMatrix();

    rlPopMatrix();
}

static void DrawPipe3D(Pipe *p) {
    if (!p->active) return;

    Color base  = HsvColor(p->hue, 0.75f, 0.9f);
    Color dark  = HsvColor(p->hue, 0.85f, 0.55f);
    Color light = HsvColor(p->hue, 0.5f, 1.0f);
    Color shine = HsvColor(fmodf(p->hue + 40.0f, 360.0f), 0.35f, 1.0f);

    float topE = p->gapY + PIPE_GAP * 0.5f;
    float botE = p->gapY - PIPE_GAP * 0.5f;

    float botH = botE - GROUND_Y;
    if (botH > 0.01f) {
        DrawCylinder((Vector3){p->x, GROUND_Y, 0}, PIPE_RADIUS, PIPE_RADIUS, botH, 14, base);

        DrawCylinder((Vector3){p->x, botE-0.2f, 0}, CAP_RADIUS, CAP_RADIUS, 0.4f, 14, light);

        DrawCylinder((Vector3){p->x+0.15f, GROUND_Y, 0.15f}, 0.06f, 0.06f, botH, 6, shine);

        int rings = (int)(botH / 1.5f);
        for (int r = 1; r <= rings; r++) {
            float ry = GROUND_Y + r * 1.5f;
            if (ry > botE - 0.3f) break;
            Color rc = HsvColor(fmodf(p->hue + r * 40.0f, 360.0f), 0.6f, 1.0f);
            DrawCylinder((Vector3){p->x, ry, 0}, PIPE_RADIUS+0.04f, PIPE_RADIUS+0.04f, 0.08f, 10, rc);
        }
    }

    float topH = CEILING_Y - topE;
    if (topH > 0.01f) {
        DrawCylinder((Vector3){p->x, topE, 0}, PIPE_RADIUS, PIPE_RADIUS, topH, 14, base);

        DrawCylinder((Vector3){p->x, topE-0.2f, 0}, CAP_RADIUS, CAP_RADIUS, 0.4f, 14, light);

        DrawCylinder((Vector3){p->x+0.15f, topE, 0.15f}, 0.06f, 0.06f, topH, 6, shine);

        int rings = (int)(topH / 1.5f);
        for (int r = 1; r <= rings; r++) {
            float ry = topE + r * 1.5f;
            if (ry > CEILING_Y - 0.3f) break;
            Color rc = HsvColor(fmodf(p->hue + r * 40.0f + 180.0f, 360.0f), 0.6f, 1.0f);
            DrawCylinder((Vector3){p->x, ry, 0}, PIPE_RADIUS+0.04f, PIPE_RADIUS+0.04f, 0.08f, 10, rc);
        }
    }
}

static void DrawGround3D(void) {
    DrawCube((Vector3){0, GROUND_Y - 0.25f, 0}, 60, 0.5f, 14, (Color){50, 180, 50, 255});
    DrawCube((Vector3){0, GROUND_Y - 0.50f, 0}, 60, 0.02f, 14, (Color){35, 145, 35, 255});
    DrawCube((Vector3){0, GROUND_Y - 1.0f,  0}, 60, 1.0f, 14, (Color){139, 90, 43, 255});
    DrawCube((Vector3){0, GROUND_Y - 2.0f,  0}, 60, 1.0f, 14, (Color){100, 65, 30, 255});

    float off = fmodf(gTime * PIPE_SPEED, 3.0f);
    for (int i = -10; i < 10; i++) {
        float sx = i * 3.0f - off;
        DrawCube((Vector3){sx, GROUND_Y + 0.01f, 0}, 0.06f, 0.01f, 14, (Color){40, 160, 40, 120});
    }
}

static void DrawClouds3D(void) {
    for (int i = 0; i < MAX_CLOUDS; i++) {
        Cloud *c = &clouds[i];
        Color w = (Color){255, 255, 255, 230};
        float s = c->scale;
        DrawSphere((Vector3){c->x, c->y, c->z}, 0.5f*s, w);
        DrawSphere((Vector3){c->x+0.5f*s, c->y-0.1f, c->z}, 0.4f*s, w);
        DrawSphere((Vector3){c->x-0.5f*s, c->y-0.08f, c->z}, 0.45f*s, w);
        DrawSphere((Vector3){c->x+0.15f*s, c->y+0.22f*s, c->z}, 0.32f*s, w);
    }
}

static void DrawBackground3D(void) {
    for (int i = 0; i < 7; i++) {
        float hx = -18.0f + i * 7.0f;
        float hz = -9.0f - (float)(i % 3);
        float hr = 2.5f + (float)(i % 3) * 0.8f;
        Color hc = {(unsigned char)(40+i*10), (unsigned char)(100+i*12), (unsigned char)(40+i*6), 255};
        DrawSphere((Vector3){hx, GROUND_Y, hz}, hr, hc);
    }

    for (int i = 0; i < 8; i++) {
        float tx = -14.0f + i * 4.2f;
        float tz = -5.5f - (float)(i % 3) * 1.2f;
        float th = 1.0f + (float)(i % 3) * 0.3f;
        DrawCylinder((Vector3){tx, GROUND_Y, tz}, 0.06f, 0.1f, th, 6, (Color){120, 80, 40, 255});
        float lr = 0.4f + (float)(i % 2) * 0.15f;
        Color lc = {(unsigned char)(25+i*7), (unsigned char)(130+i*10), (unsigned char)(25+i*4), 255};
        DrawSphere((Vector3){tx, GROUND_Y+th+lr*0.4f, tz}, lr, lc);
    }
}

static void DrawSun3D(void) {
    DrawSphere((Vector3){22, 13, -16}, 2.0f, (Color){255, 245, 160, 220});
    DrawSphere((Vector3){22, 13, -16}, 2.8f, (Color){255, 225, 110, 80});
}

static void DrawParticles3D(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        float a = particles[i].life * 2.5f;
        if (a > 1.0f) a = 1.0f;
        Color c = particles[i].color;
        c.a = (unsigned char)(a * 255);
        DrawCube(particles[i].pos, particles[i].size, particles[i].size, particles[i].size, c);
    }
}

static void UpdateDrawFrame(void) {
#if defined(PLATFORM_WEB)
    int bw = GetBrowserWidth(), bh = GetBrowserHeight();
    if (bw != GetScreenWidth() || bh != GetScreenHeight()) {
        SetWindowSize(bw, bh);
    }
#endif

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;
    gTime += dt;

    int sw = GetScreenWidth(), sh = GetScreenHeight();

    int jumped = IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
              || IsGestureDetected(GESTURE_TAP);

    if (scoreScale > 1.0f) {
        scoreScale -= dt * 4.0f;
        if (scoreScale < 1.0f) scoreScale = 1.0f;
    }

    switch (state) {
    case ST_MENU:
        bird.y = 1.0f + sinf(gTime * 2.0f) * 0.5f;
        bird.wingAngle = sinf(gTime * 8.0f) * 30.0f;
        bird.rotation = sinf(gTime * 1.5f) * 5.0f;
        UpdateClouds(dt);
        if (jumped) { state = ST_PLAY; ResetGame(); bird.velocity = JUMP_VEL * 0.5f; }
        break;

    case ST_PLAY:
        bird.velocity -= GRAVITY * dt;
        bird.y += bird.velocity * dt;

        {
            float wspd = bird.velocity > 0 ? 14.0f : 7.0f;
            bird.wingAngle = sinf(gTime * wspd) * 35.0f;
        }
        bird.rotation = Clamp(bird.velocity * 6.0f, -60.0f, 45.0f);

        if (jumped) bird.velocity = JUMP_VEL;

        for (int i = 0; i < MAX_PIPES; i++) {
            if (!pipes[i].active) continue;
            pipes[i].x -= PIPE_SPEED * dt;

            if (!pipes[i].scored && pipes[i].x < 0.0f) {
                pipes[i].scored = 1;
                score++;
                scoreScale = 1.6f;
                EmitScoreBurst(0.0f, bird.y);
                if (score >= WIN_SCORE) {
                    state = ST_WIN;
                    deadTimer = 0.0f;
                    for (int j = 0; j < 40; j++) {
                        Color wc = HsvColor(RandF(0,360), 0.9f, 1.0f);
                        Particle *wp = &particles[partIdx % MAX_PARTICLES];
                        wp->active = 1;
                        wp->pos = (Vector3){ RandF(-3,3), RandF(-1,5), RandF(-2,2) };
                        wp->vel = (Vector3){ RandF(-4,4), RandF(2,8), RandF(-3,3) };
                        wp->life = RandF(1.0f, 2.5f);
                        wp->size = RandF(0.1f, 0.25f);
                        wp->color = wc;
                        partIdx++;
                    }
                }
            }

            if (pipes[i].x < -15.0f) pipes[i].active = 0;

            float dx = fabsf(pipes[i].x);
            float topE = pipes[i].gapY + PIPE_GAP * 0.5f;
            float botE = pipes[i].gapY - PIPE_GAP * 0.5f;
            float effR = (fabsf(bird.y - topE) < 0.3f || fabsf(bird.y - botE) < 0.3f)
                         ? CAP_RADIUS : PIPE_RADIUS;

            if (dx < (effR + BIRD_RADIUS)) {
                if (bird.y + BIRD_RADIUS > topE || bird.y - BIRD_RADIUS < botE) {
                    state = ST_DEAD;
                    shakeTimer = 0.4f;
                    flashTimer = 0.25f;
                    deadTimer = 0.0f;
                }
            }
        }

        EnsurePipesAhead();

        if (bird.y - BIRD_RADIUS < GROUND_Y) {
            bird.y = GROUND_Y + BIRD_RADIUS;
            state = ST_DEAD;
            shakeTimer = 0.4f;
            flashTimer = 0.25f;
            deadTimer = 0.0f;
        }
        if (bird.y + BIRD_RADIUS > CEILING_Y) {
            bird.y = CEILING_Y - BIRD_RADIUS;
            bird.velocity = 0;
        }

        if (((int)(gTime * 30)) % 2 == 0) {
            Color tc = HsvColor(fmodf(gTime * 80.0f, 360.0f), 0.85f, 1.0f);
            EmitParticle(-0.35f, bird.y, 0, tc);
        }

        UpdateParticles(dt);
        UpdateClouds(dt);
        break;

    case ST_DEAD:
        deadTimer += dt;
        if (shakeTimer > 0) shakeTimer -= dt;
        if (flashTimer > 0) flashTimer -= dt;

        bird.velocity -= GRAVITY * dt;
        bird.y += bird.velocity * dt;
        if (bird.y < GROUND_Y + BIRD_RADIUS) {
            bird.y = GROUND_Y + BIRD_RADIUS;
            bird.velocity = 0;
        }
        bird.rotation = Clamp(bird.velocity * 6.0f, -90.0f, 45.0f);
        bird.wingAngle *= 0.94f;

        UpdateParticles(dt);
        UpdateClouds(dt);

#if defined(PLATFORM_WEB)
        if (!jsCalled && deadTimer > 0.8f) {
            jsCalled = 1;
            JSonGameOver(score, 0);
        }
#endif
        break;

    case ST_WIN:
        deadTimer += dt;
        bird.y = 1.0f + sinf(gTime * 2.5f) * 0.4f;
        bird.wingAngle = sinf(gTime * 10.0f) * 35.0f;
        bird.rotation = sinf(gTime * 2.0f) * 8.0f;

        if (((int)(gTime * 15)) % 3 == 0) {
            Color fc = HsvColor(fmodf(gTime * 100.0f, 360.0f), 0.9f, 1.0f);
            Particle *fp = &particles[partIdx % MAX_PARTICLES];
            fp->active = 1;
            fp->pos = (Vector3){ RandF(-6, 10), RandF(-2, 6), RandF(-3, 3) };
            fp->vel = (Vector3){ RandF(-1, 1), RandF(2, 5), RandF(-1, 1) };
            fp->life = RandF(0.8f, 1.8f);
            fp->size = RandF(0.08f, 0.2f);
            fp->color = fc;
            partIdx++;
        }

        UpdateParticles(dt);
        UpdateClouds(dt);

#if defined(PLATFORM_WEB)
        if (!jsCalled && deadTimer > 1.2f) {
            jsCalled = 1;
            JSonGameWin(score, 0);
        }
#endif
        break;
    }

    BeginDrawing();

    DrawRectangleGradientV(0, 0, sw, sh,
        (Color){65, 125, 225, 255}, (Color){175, 215, 255, 255});

    Camera3D dc = cam;
    if (shakeTimer > 0) {
        float s = shakeTimer / 0.4f;
        dc.position.x += RandF(-0.3f, 0.3f) * s;
        dc.position.y += RandF(-0.3f, 0.3f) * s;
    }

    BeginMode3D(dc);

    DrawSun3D();
    DrawBackground3D();
    DrawClouds3D();
    DrawGround3D();

    for (int i = 0; i < MAX_PIPES; i++) DrawPipe3D(&pipes[i]);
    DrawBird3D();
    DrawParticles3D();

    EndMode3D();

    if (flashTimer > 0) {
        unsigned char a = (unsigned char)(flashTimer / 0.25f * 180.0f);
        DrawRectangle(0, 0, sw, sh, (Color){255, 255, 255, a});
    }

    if (state == ST_PLAY || state == ST_DEAD || state == ST_WIN) {
        const char *st_txt = TextFormat("%d", score);
        int fs = (int)(60 * scoreScale);
        int tw = MeasureText(st_txt, fs);
        DrawText(st_txt, sw/2 - tw/2 + 3, 22, fs, (Color){0,0,0,150});
        DrawText(st_txt, sw/2 - tw/2, 20, fs, WHITE);
    }

    if (state == ST_MENU) {
        const char *title = "FLAPPY BIRD 3D";
        int ts = 50;
        int ttw = MeasureText(title, ts);
        Color tc = HsvColor(fmodf(gTime * 60.0f, 360.0f), 0.7f, 1.0f);
        DrawText(title, sw/2 - ttw/2 + 3, 53, ts, (Color){0,0,0,150});
        DrawText(title, sw/2 - ttw/2, 50, ts, tc);

        const char *sub = "Press SPACE or Click to Play";
        int subw = MeasureText(sub, 24);
        float al = (sinf(gTime * 3.0f) + 1.0f) * 0.5f;
        DrawText(sub, sw/2 - subw/2, sh - 55, 24,
            (Color){255, 255, 255, (unsigned char)(al * 255)});

    }

    if (state == ST_DEAD) {
        DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 100});

        const char *go = "GAME OVER";
        int gos = 55;
        int gow = MeasureText(go, gos);
        DrawText(go, sw/2 - gow/2 + 3, sh/2 - 63, gos, (Color){0,0,0,180});
        DrawText(go, sw/2 - gow/2, sh/2 - 65, gos, (Color){255, 80, 80, 255});

        const char *sf = TextFormat("Score: %d", score);
        int sfw = MeasureText(sf, 30);
        DrawText(sf, sw/2 - sfw/2, sh/2, 30, WHITE);
    }

    if (state == ST_WIN) {
        DrawRectangle(0, 0, sw, sh, (Color){0, 0, 0, 80});

        const char *wt = "YOU WIN!";
        int wts = 65;
        int wtw = MeasureText(wt, wts);
        Color wc = HsvColor(fmodf(gTime * 90.0f, 360.0f), 0.8f, 1.0f);
        DrawText(wt, sw/2 - wtw/2 + 3, sh/2 - 80 + 3, wts, (Color){0,0,0,180});
        DrawText(wt, sw/2 - wtw/2, sh/2 - 80, wts, wc);

        const char *ws = TextFormat("Score: %d", score);
        int wsw = MeasureText(ws, 30);
        DrawText(ws, sw/2 - wsw/2, sh/2 - 5, 30, WHITE);
    }

    EndDrawing();
}

int main(void) {
    int initW = INIT_W, initH = INIT_H;
#if defined(PLATFORM_WEB)
    initW = GetBrowserWidth();
    initH = GetBrowserHeight();
#endif
    InitWindow(initW, initH, "Flappy Bird 3D");
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