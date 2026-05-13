// RTS step 10 — Barracks (second building) + Fog of War.
//
// Builds on rts_step9:
//   * Two buildings now. The Town Hall (existing, brown) produces
//     Workers; the Barracks (new, darker red-brown, placed east of
//     the hall) produces Soldiers. Each building has its own queue
//     and its own progress. Clicking a building selects it and opens
//     its specific build menu.
//   * Keyboard shortcuts: Space queues a Worker at the hall, B queues
//     a Soldier at the barracks. Same costs, same upfront pay.
//   * Fog of war on a 64x64 tile grid. Three states: UNEXPLORED
//     (opaque black), EXPLORED (dim — you remember the terrain) and
//     VISIBLE (clear — friendly units can currently see it).
//   * Sight radii per entity (in tile-cells): Worker 5, Soldier 6,
//     Hall 8, Barracks 7. Fog updates every frame.
//   * Enemies only render in currently-VISIBLE cells. Nodes render
//     in EXPLORED or VISIBLE (you remember them after seeing them
//     once). The same rule applies to the minimap.
//   * Save/load (F5/F9) now also captures both queues, both progress
//     timers, and the explored-mask layer of the fog.

import * from "../lib/Sfml.mt";
import * from "../lib/Graphics.mt";
import * from "../lib/System.mt";

__plugin_load("mt/mtype_sfml.dll");

string HUD_FONT_PATH = "C:/Windows/Fonts/arial.ttf";

// ---- world / window ----
int   WIN_W      = 1280;
int   WIN_H      = 800;
int   TILE_PX    = 32;
int   MAP_TILES  = 64;
float WORLD_W    = (float)(MAP_TILES * TILE_PX);
float WORLD_H    = (float)(MAP_TILES * TILE_PX);
float EDGE_PAD   = 12.0;
float PAN_SPEED  = 700.0;
float ZOOM_MIN   = 0.5;
float ZOOM_MAX   = 3.0;
float ZOOM_RATE  = 1.2;
float ZOOM_STEP  = 0.9;

// ---- unit constants ----
int   MAX_UNITS    = 512;
float ARRIVE_EPS   = 2.0;
float SEP_RADIUS   = 24.0;
float SEP_STRENGTH = 380.0;
float FORM_SPACING = 28.0;

float SEP_GRID_CELL = 32.0;
int   SEP_GRID_W    = (int)(WORLD_W / SEP_GRID_CELL);
int   SEP_GRID_H    = (int)(WORLD_H / SEP_GRID_CELL);

int   UT_WORKER  = 0;
int   UT_SOLDIER = 1;

float WORKER_RADIUS  = 10.0;
float WORKER_SPEED   = 140.0;
int   WORKER_HP_INIT = 25;
int   WORKER_COST_CRYSTAL = 10;
int   WORKER_COST_WOOD    = 5;
float WORKER_BUILD_TIME = 2.5;
float WORKER_SIGHT  = 5.0;       // in fog cells

float SOLDIER_RADIUS = 13.0;
float SOLDIER_SPEED  = 110.0;
int   SOLDIER_HP_INIT = 60;
int   SOLDIER_COST_CRYSTAL = 20;
int   SOLDIER_COST_WOOD    = 15;
float SOLDIER_BUILD_TIME = 5.0;
int   SOLDIER_DAMAGE  = 8;
float SOLDIER_ATTACK_RANGE    = 30.0;
float SOLDIER_ATTACK_COOLDOWN = 0.7;
float SOLDIER_SIGHT = 6.0;

int QUEUE_MAX = 5;

// ---- economy constants ----
float HALL_HALF        = 36.0;
float HALL_SIGHT       = 8.0;
float BARRACKS_HALF    = 30.0;
float BARRACKS_SIGHT   = 7.0;

float NODE_RADIUS      = 18.0;
int   NODE_AMOUNT_INIT = 200;
float HARVEST_RANGE    = 38.0;
float DEPOSIT_RANGE    = 56.0;
float HARVEST_TIME     = 0.8;
int   HARVEST_AMOUNT   = 5;

int RES_CRYSTAL = 0;
int RES_WOOD    = 1;

// ---- enemy constants ----
int   ENEMY_HP_INIT = 60;
float ENEMY_RADIUS  = 16.0;

// ---- fog of war ----
int   FOG_W = 64;
int   FOG_H = 64;
float FOG_CELL = 32.0;
int   FOG_UNEXPLORED = 0;
int   FOG_EXPLORED   = 1;
int   FOG_VISIBLE    = 2;

// ---- FSM ----
int WS_IDLE       = 0;
int WS_TO_RES     = 1;
int WS_HARVEST    = 2;
int WS_TO_HALL    = 3;
int SOL_TO_ENEMY  = 10;
int SOL_ATTACKING = 11;

// ---- data model ----
class Unit {
    public float x;
    public float y;
    public float tx;
    public float ty;
    public int   hasTarget;
    public int   selected;
    public int   type;
    public int   state;
    public float speed;
    public float radius;
    public int   hp;
    public int   assignedNode;
    public int   cargo;
    public float harvestTimer;
    public int   targetEnemy;
    public float attackTimer;

    public constructor(float x, float y, int type,
                        float speed, float radius, int hp) {
        this.x = x;  this.y = y;
        this.tx = x; this.ty = y;
        this.hasTarget = 0;
        this.selected = 0;
        this.type = type;
        this.state = 0;
        this.speed = speed;
        this.radius = radius;
        this.hp = hp;
        this.assignedNode = -1;
        this.cargo = 0;
        this.harvestTimer = 0.0;
        this.targetEnemy = -1;
        this.attackTimer = 0.0;
    }
}

class Node {
    public float x;
    public float y;
    public int   amount;
    public int   resType;
    public constructor(float x, float y, int amt, int rt) {
        this.x = x; this.y = y;
        this.amount = amt;
        this.resType = rt;
    }
}

class Enemy {
    public float x;
    public float y;
    public int   hp;
    public int   maxHp;
    public constructor(float x, float y, int hp) {
        this.x = x; this.y = y;
        this.hp = hp;
        this.maxHp = hp;
    }
}

function newWorker(float x, float y): Unit {
    return new Unit(x, y, 0, 140.0, 10.0, 25);
}
function newSoldier(float x, float y): Unit {
    return new Unit(x, y, 1, 110.0, 13.0, 60);
}

// ---- helpers ----
function sqrt(float v): float {
    if (v <= 0.0) { return 0.0; }
    float x = v;
    if (x > 1.0) { x = x * 0.5; }
    int i = 0;
    while (i < 6) {
        x = 0.5 * (x + v / x);
        i = i + 1;
    }
    return x;
}

function isqrtCeil(int n): int {
    if (n <= 1) { return 1; }
    int s = 1;
    while (s * s < n) { s = s + 1; }
    return s;
}

class Coords {
    public static function screenToWorld(float px, float py,
                                          float cx, float cy,
                                          float sw, float sh,
                                          float winW, float winH): float[] {
        float wx = cx - sw * 0.5 + (px / winW) * sw;
        float wy = cy - sh * 0.5 + (py / winH) * sh;
        float[] out = new float[2];
        out[0] = wx; out[1] = wy;
        return out;
    }
}

function inRect(float mx, float my, float bx, float by, float bw, float bh): int {
    if (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh) { return 1; }
    return 0;
}

function costCrystalOf(int type): int {
    if (type == UT_SOLDIER) { return SOLDIER_COST_CRYSTAL; }
    return WORKER_COST_CRYSTAL;
}
function costWoodOf(int type): int {
    if (type == UT_SOLDIER) { return SOLDIER_COST_WOOD; }
    return WORKER_COST_WOOD;
}
function buildTimeOf(int type): float {
    if (type == UT_SOLDIER) { return SOLDIER_BUILD_TIME; }
    return WORKER_BUILD_TIME;
}

// Reveal a circular area around (sx, sy) by setting matching fog cells
// to VISIBLE. Mutates the int[] in place — the array reference is
// passed in, the writes go through `fogArr[i] = ...`.
function revealAround(int[] fogArr, float sx, float sy, float radius): void {
    float invCell = 1.0 / FOG_CELL;
    float sxC = sx * invCell;
    float syC = sy * invCell;
    int cx = (int)sxC;
    int cy = (int)syC;
    int rT = (int)radius + 1;
    int xMin = cx - rT; if (xMin < 0) { xMin = 0; }
    int xMax = cx + rT; if (xMax >= FOG_W) { xMax = FOG_W - 1; }
    int yMin = cy - rT; if (yMin < 0) { yMin = 0; }
    int yMax = cy + rT; if (yMax >= FOG_H) { yMax = FOG_H - 1; }
    float r2 = radius * radius;
    for (int gy = yMin; gy <= yMax; gy++) {
        for (int gx = xMin; gx <= xMax; gx++) {
            float dx = (float)gx + 0.5 - sxC;
            float dy = (float)gy + 0.5 - syC;
            if (dx * dx + dy * dy <= r2) {
                fogArr[gy * FOG_W + gx] = FOG_VISIBLE;
            }
        }
    }
}

// ---- window + tilemap ----
RenderWindow window = Sfml::createWindow("RTS — step 10", WIN_W, WIN_H);
window.setFramerateLimit(144);

int tileCount   = MAP_TILES * MAP_TILES;
int vertexCount = tileCount * 6;
VertexArray ground = VertexArrays::create(Primitive::triangles(), vertexCount);
for (int ty = 0; ty < MAP_TILES; ty++) {
    for (int tx = 0; tx < MAP_TILES; tx++) {
        float x0 = (float)(tx * TILE_PX);
        float y0 = (float)(ty * TILE_PX);
        float x1 = x0 + (float)TILE_PX;
        float y1 = y0 + (float)TILE_PX;
        int parity = (tx + ty) % 2;
        int base = 48;
        if (parity == 1) { base = 60; }
        int band = ((tx + ty) * 3) % 40;
        int r = base + band / 2;
        int g = base + 20 + band / 3;
        int b = base + 10;
        int idx = (ty * MAP_TILES + tx) * 6;
        ground.setVertex(idx + 0, x0, y0, r, g, b, 255, 0.0, 0.0);
        ground.setVertex(idx + 1, x1, y0, r, g, b, 255, 0.0, 0.0);
        ground.setVertex(idx + 2, x1, y1, r, g, b, 255, 0.0, 0.0);
        ground.setVertex(idx + 3, x0, y0, r, g, b, 255, 0.0, 0.0);
        ground.setVertex(idx + 4, x1, y1, r, g, b, 255, 0.0, 0.0);
        ground.setVertex(idx + 5, x0, y1, r, g, b, 255, 0.0, 0.0);
    }
}

// ---- camera ----
float camX    = WORLD_W * 0.5;
float camY    = WORLD_H * 0.5;
float camZoom = 1.0;
View  cam     = Views::create(camX, camY, (float)WIN_W, (float)WIN_H);

// ---- buildings ----
float HALL_X = WORLD_W * 0.5;
float HALL_Y = WORLD_H * 0.5;
float BARRACKS_X = HALL_X + 200.0;
float BARRACKS_Y = HALL_Y - 40.0;

// ---- resource nodes ----
int NODE_COUNT = 9;
Node[] nodes = new Node[NODE_COUNT];
nodes[0] = new Node(HALL_X - 280.0, HALL_Y - 220.0, NODE_AMOUNT_INIT, RES_CRYSTAL);
nodes[1] = new Node(HALL_X + 300.0, HALL_Y - 200.0, NODE_AMOUNT_INIT, RES_CRYSTAL);
nodes[2] = new Node(HALL_X + 260.0, HALL_Y + 240.0, NODE_AMOUNT_INIT, RES_CRYSTAL);
nodes[3] = new Node(HALL_X - 320.0, HALL_Y + 200.0, NODE_AMOUNT_INIT, RES_CRYSTAL);
nodes[4] = new Node(HALL_X,         HALL_Y - 340.0, NODE_AMOUNT_INIT, RES_CRYSTAL);
nodes[5] = new Node(HALL_X - 180.0, HALL_Y - 320.0, NODE_AMOUNT_INIT, RES_WOOD);
nodes[6] = new Node(HALL_X + 180.0, HALL_Y - 320.0, NODE_AMOUNT_INIT, RES_WOOD);
nodes[7] = new Node(HALL_X - 200.0, HALL_Y + 320.0, NODE_AMOUNT_INIT, RES_WOOD);
nodes[8] = new Node(HALL_X + 200.0, HALL_Y + 320.0, NODE_AMOUNT_INIT, RES_WOOD);

int crystalStockpile = 50;
int woodStockpile    = 30;

// ---- enemies ----
int ENEMY_COUNT = 4;
Enemy[] enemies = new Enemy[ENEMY_COUNT];
enemies[0] = new Enemy(HALL_X - 620.0, HALL_Y - 500.0, ENEMY_HP_INIT);
enemies[1] = new Enemy(HALL_X + 640.0, HALL_Y - 480.0, ENEMY_HP_INIT);
enemies[2] = new Enemy(HALL_X + 600.0, HALL_Y + 520.0, ENEMY_HP_INIT);
enemies[3] = new Enemy(HALL_X - 660.0, HALL_Y + 490.0, ENEMY_HP_INIT);

// ---- unit pool ----
Unit[] units = new Unit[MAX_UNITS];
int unitCount = 0;

float seedX = HALL_X - 100.0;
float seedY = HALL_Y + HALL_HALF + 30.0;
for (int sy = 0; sy < 4; sy++) {
    for (int sx = 0; sx < 4; sx++) {
        units[unitCount] = newWorker(seedX + (float)sx * 32.0,
                                      seedY + (float)sy * 32.0);
        unitCount = unitCount + 1;
    }
}
for (int sx = 0; sx < 4; sx++) {
    units[unitCount] = newSoldier(seedX + (float)sx * 36.0,
                                   seedY + 4.0 * 32.0 + 16.0);
    unitCount = unitCount + 1;
}

// ---- build queues (one per building) ----
int[] hallQueue        = new int[QUEUE_MAX];
int   hallQueueCount   = 0;
float hallBuildProg    = 0.0;
int[] barracksQueue    = new int[QUEUE_MAX];
int   barracksQueueCount = 0;
float barracksBuildProg = 0.0;

// ---- fog state ----
int   fogCellCount = FOG_W * FOG_H;
int[] fog     = new int[fogCellCount];   // current frame's per-cell state
int[] fogPrev = new int[fogCellCount];   // previous frame's state — used
                                          // to avoid rebuilding cells whose
                                          // alpha hasn't changed.
// fogPrev starts as 0 (UNEXPLORED) which matches the initial VA color;
// fog also starts UNEXPLORED. The hall + barracks sight pass on frame
// 0 promotes their surroundings to VISIBLE, which then differs from
// fogPrev and triggers the per-cell VA rebuild for those cells only.

// ---- save snapshot buffers ----
int   hasSnapshot      = 0;
int   snapCrystal      = 0;
int   snapWood         = 0;
float snapCamX         = 0.0;
float snapCamY         = 0.0;
float snapCamZoom      = 1.0;
int   snapUnitCount    = 0;
float[] snapUX           = new float[MAX_UNITS];
float[] snapUY           = new float[MAX_UNITS];
float[] snapUTx          = new float[MAX_UNITS];
float[] snapUTy          = new float[MAX_UNITS];
int[]   snapUHasTarget   = new int[MAX_UNITS];
int[]   snapUSelected    = new int[MAX_UNITS];
int[]   snapUType        = new int[MAX_UNITS];
int[]   snapUState       = new int[MAX_UNITS];
float[] snapUSpeed       = new float[MAX_UNITS];
float[] snapURadius      = new float[MAX_UNITS];
int[]   snapUHp          = new int[MAX_UNITS];
int[]   snapUAssignedN   = new int[MAX_UNITS];
int[]   snapUCargo       = new int[MAX_UNITS];
float[] snapUHarvestT    = new float[MAX_UNITS];
int[]   snapUTargetEnemy = new int[MAX_UNITS];
float[] snapUAttackT     = new float[MAX_UNITS];
int[]   snapNodeAmount   = new int[NODE_COUNT];
int[]   snapEnemyHp      = new int[ENEMY_COUNT];
int     snapHallQueueCnt = 0;
int[]   snapHallQueue    = new int[QUEUE_MAX];
float   snapHallBuildProg = 0.0;
int     snapBarracksQueueCnt = 0;
int[]   snapBarracksQueue = new int[QUEUE_MAX];
float   snapBarracksBuildProg = 0.0;
int[]   snapFog          = new int[fogCellCount];   // explored mask only

// ---- spatial grid ----
int   cellCount = SEP_GRID_W * SEP_GRID_H;
int[] cellHead  = new int[cellCount];
int[] cellNext  = new int[MAX_UNITS];

// ---- world-space draw shapes ----
CircleShape workerBody = Circles::create(WORKER_RADIUS);
workerBody.setOrigin(WORKER_RADIUS, WORKER_RADIUS);
workerBody.setFillColor(80, 200, 110, 255);
workerBody.setOutlineColor(20, 50, 25, 255);
workerBody.setOutlineThickness(1.5);

CircleShape workerBodyLoaded = Circles::create(WORKER_RADIUS);
workerBodyLoaded.setOrigin(WORKER_RADIUS, WORKER_RADIUS);
workerBodyLoaded.setFillColor(200, 220, 110, 255);
workerBodyLoaded.setOutlineColor(50, 40, 20, 255);
workerBodyLoaded.setOutlineThickness(1.5);

CircleShape soldierBody = Circles::create(SOLDIER_RADIUS);
soldierBody.setOrigin(SOLDIER_RADIUS, SOLDIER_RADIUS);
soldierBody.setFillColor(220, 80, 80, 255);
soldierBody.setOutlineColor(60, 20, 20, 255);
soldierBody.setOutlineThickness(2.0);

CircleShape unitRing = Circles::create(SOLDIER_RADIUS + 4.0);
unitRing.setOrigin(SOLDIER_RADIUS + 4.0, SOLDIER_RADIUS + 4.0);
unitRing.setFillColor(0, 0, 0, 0);
unitRing.setOutlineColor(255, 230, 80, 255);
unitRing.setOutlineThickness(2.0);

CircleShape nodeShapeCrystal = Circles::create(NODE_RADIUS);
nodeShapeCrystal.setOrigin(NODE_RADIUS, NODE_RADIUS);
nodeShapeCrystal.setFillColor(110, 180, 255, 255);
nodeShapeCrystal.setOutlineColor(40, 60, 120, 255);
nodeShapeCrystal.setOutlineThickness(2.0);

CircleShape nodeShapeWood = Circles::create(NODE_RADIUS);
nodeShapeWood.setOrigin(NODE_RADIUS, NODE_RADIUS);
nodeShapeWood.setFillColor(150, 95, 50, 255);
nodeShapeWood.setOutlineColor(70, 40, 15, 255);
nodeShapeWood.setOutlineThickness(2.0);

CircleShape enemyShape = Circles::create(ENEMY_RADIUS);
enemyShape.setOrigin(ENEMY_RADIUS, ENEMY_RADIUS);
enemyShape.setFillColor(150, 30, 30, 255);
enemyShape.setOutlineColor(40, 10, 10, 255);
enemyShape.setOutlineThickness(2.0);

RectangleShape hpBarBg = Rectangles::create(36.0, 5.0);
hpBarBg.setFillColor(20, 20, 20, 220);
hpBarBg.setOutlineColor(0, 0, 0, 255);
hpBarBg.setOutlineThickness(1.0);

RectangleShape hpBarFg = Rectangles::create(34.0, 3.0);
hpBarFg.setFillColor(220, 70, 70, 255);

RectangleShape hallShape = Rectangles::create(HALL_HALF * 2.0, HALL_HALF * 2.0);
hallShape.setOrigin(HALL_HALF, HALL_HALF);
hallShape.setPosition(HALL_X, HALL_Y);
hallShape.setFillColor(140, 110, 70, 255);
hallShape.setOutlineColor(50, 40, 25, 255);
hallShape.setOutlineThickness(3.0);

RectangleShape hallSelectionRing = Rectangles::create(HALL_HALF * 2.0 + 10.0,
                                                       HALL_HALF * 2.0 + 10.0);
hallSelectionRing.setOrigin(HALL_HALF + 5.0, HALL_HALF + 5.0);
hallSelectionRing.setPosition(HALL_X, HALL_Y);
hallSelectionRing.setFillColor(0, 0, 0, 0);
hallSelectionRing.setOutlineColor(255, 230, 80, 230);
hallSelectionRing.setOutlineThickness(3.0);

RectangleShape barracksShape = Rectangles::create(BARRACKS_HALF * 2.0,
                                                   BARRACKS_HALF * 2.0);
barracksShape.setOrigin(BARRACKS_HALF, BARRACKS_HALF);
barracksShape.setPosition(BARRACKS_X, BARRACKS_Y);
barracksShape.setFillColor(120, 60, 50, 255);
barracksShape.setOutlineColor(40, 15, 10, 255);
barracksShape.setOutlineThickness(3.0);

RectangleShape barracksSelectionRing = Rectangles::create(BARRACKS_HALF * 2.0 + 10.0,
                                                           BARRACKS_HALF * 2.0 + 10.0);
barracksSelectionRing.setOrigin(BARRACKS_HALF + 5.0, BARRACKS_HALF + 5.0);
barracksSelectionRing.setPosition(BARRACKS_X, BARRACKS_Y);
barracksSelectionRing.setFillColor(0, 0, 0, 0);
barracksSelectionRing.setOutlineColor(255, 230, 80, 230);
barracksSelectionRing.setOutlineThickness(3.0);

RectangleShape selBox = Rectangles::create(1.0, 1.0);
selBox.setFillColor(80, 180, 255, 40);
selBox.setOutlineColor(140, 220, 255, 220);
selBox.setOutlineThickness(1.0);

// ---- fog VA (one big triangle list, alpha updated per-cell on change) ----
int fogVertexCount = fogCellCount * 6;
VertexArray fogVA = VertexArrays::create(Primitive::triangles(), fogVertexCount);
for (int gy = 0; gy < FOG_H; gy++) {
    for (int gx = 0; gx < FOG_W; gx++) {
        float x0 = (float)gx * FOG_CELL;
        float y0 = (float)gy * FOG_CELL;
        float x1 = x0 + FOG_CELL;
        float y1 = y0 + FOG_CELL;
        int idx = (gy * FOG_W + gx) * 6;
        int a = 230;   // initial unexplored alpha
        fogVA.setVertex(idx + 0, x0, y0, 0, 0, 0, a, 0.0, 0.0);
        fogVA.setVertex(idx + 1, x1, y0, 0, 0, 0, a, 0.0, 0.0);
        fogVA.setVertex(idx + 2, x1, y1, 0, 0, 0, a, 0.0, 0.0);
        fogVA.setVertex(idx + 3, x0, y0, 0, 0, 0, a, 0.0, 0.0);
        fogVA.setVertex(idx + 4, x1, y1, 0, 0, 0, a, 0.0, 0.0);
        fogVA.setVertex(idx + 5, x0, y1, 0, 0, 0, a, 0.0, 0.0);
    }
}

// ---- HUD ----
Font hudFont = Fonts::load(HUD_FONT_PATH);

RectangleShape hudPanel = Rectangles::create(260.0, 168.0);
hudPanel.setPosition(10.0, 10.0);
hudPanel.setFillColor(0, 0, 0, 150);
hudPanel.setOutlineColor(180, 180, 200, 180);
hudPanel.setOutlineThickness(1.0);

CircleShape hudCrystalIcon = Circles::create(7.0);
hudCrystalIcon.setOrigin(7.0, 7.0);
hudCrystalIcon.setPosition(30.0, 32.0);
hudCrystalIcon.setFillColor(110, 180, 255, 255);
hudCrystalIcon.setOutlineColor(40, 60, 120, 255);
hudCrystalIcon.setOutlineThickness(1.0);

CircleShape hudWoodIcon = Circles::create(7.0);
hudWoodIcon.setOrigin(7.0, 7.0);
hudWoodIcon.setPosition(30.0, 58.0);
hudWoodIcon.setFillColor(150, 95, 50, 255);
hudWoodIcon.setOutlineColor(70, 40, 15, 255);
hudWoodIcon.setOutlineThickness(1.0);

Text txtCrystal = Texts::create(hudFont, "Crystal: 0", 18);
txtCrystal.setFillColor(235, 235, 245, 255);
txtCrystal.setPosition(46.0, 18.0);

Text txtWood = Texts::create(hudFont, "Wood: 0", 18);
txtWood.setFillColor(235, 220, 200, 255);
txtWood.setPosition(46.0, 44.0);

Text txtWorkers = Texts::create(hudFont,
    "Workers: 0   (Sp:" + WORKER_COST_CRYSTAL + "C/" + WORKER_COST_WOOD + "W)", 13);
txtWorkers.setFillColor(180, 220, 190, 255);
txtWorkers.setPosition(22.0, 76.0);

Text txtSoldiers = Texts::create(hudFont,
    "Soldiers: 0   (B:" + SOLDIER_COST_CRYSTAL + "C/" + SOLDIER_COST_WOOD + "W)", 13);
txtSoldiers.setFillColor(230, 180, 180, 255);
txtSoldiers.setPosition(22.0, 98.0);

Text txtSelected = Texts::create(hudFont, "Selected: 0   Queue: 0", 13);
txtSelected.setFillColor(220, 220, 230, 255);
txtSelected.setPosition(22.0, 120.0);

Text txtFps = Texts::create(hudFont, "FPS: --", 12);
txtFps.setFillColor(170, 180, 200, 255);
txtFps.setPosition(22.0, 144.0);

Text txtHint = Texts::create(hudFont,
    "L-click hall/barracks: build   F5: save   F9: load   Esc: quit",
    13);
txtHint.setFillColor(180, 180, 190, 220);
txtHint.setPosition(10.0, (float)WIN_H - 22.0);

Text toastTxt = Texts::create(hudFont, "", 22);
toastTxt.setFillColor(255, 240, 160, 255);
string toastMsg  = "";
float  toastTimer = 0.0;

// ---- build menu (used for both buildings; content swaps) ----
float BM_W   = 400.0;
float BM_H   = 130.0;
float bmX    = (float)WIN_W * 0.5 - BM_W * 0.5;
float bmY    = (float)WIN_H - 32.0 - BM_H;
float BTN_W  = 150.0;
float BTN_H  = 50.0;
float btn0X  = bmX + (BM_W - BTN_W) * 0.5;     // centered button
float btn0Y  = bmY + 12.0;

float QSLOT_W   = 42.0;
float QSLOT_H   = 36.0;
float QSLOT_GAP = 6.0;
float qSlot0X   = bmX + (BM_W - (QSLOT_W * 5.0 + QSLOT_GAP * 4.0)) * 0.5;
float qSlotY    = bmY + 78.0;

RectangleShape bmPanel = Rectangles::create(BM_W, BM_H);
bmPanel.setFillColor(0, 0, 0, 170);
bmPanel.setOutlineColor(180, 180, 200, 200);
bmPanel.setOutlineThickness(1.0);

RectangleShape btnWorkerBg = Rectangles::create(BTN_W, BTN_H);
btnWorkerBg.setFillColor(40, 90, 50, 220);
btnWorkerBg.setOutlineColor(150, 220, 170, 220);
btnWorkerBg.setOutlineThickness(1.0);

RectangleShape btnSoldierBg = Rectangles::create(BTN_W, BTN_H);
btnSoldierBg.setFillColor(110, 40, 40, 220);
btnSoldierBg.setOutlineColor(230, 150, 150, 220);
btnSoldierBg.setOutlineThickness(1.0);

Text btnWorkerLabel = Texts::create(hudFont, "Worker", 17);
btnWorkerLabel.setFillColor(230, 250, 230, 255);
Text btnWorkerCost  = Texts::create(hudFont,
    "" + WORKER_COST_CRYSTAL + "C  " + WORKER_COST_WOOD + "W", 12);
btnWorkerCost.setFillColor(180, 220, 200, 255);

Text btnSoldierLabel = Texts::create(hudFont, "Soldier", 17);
btnSoldierLabel.setFillColor(250, 230, 230, 255);
Text btnSoldierCost  = Texts::create(hudFont,
    "" + SOLDIER_COST_CRYSTAL + "C  " + SOLDIER_COST_WOOD + "W", 12);
btnSoldierCost.setFillColor(230, 190, 190, 255);

Text txtQueueLabel = Texts::create(hudFont, "Queue:", 13);
txtQueueLabel.setFillColor(200, 200, 215, 230);

RectangleShape slotEmpty = Rectangles::create(QSLOT_W, QSLOT_H);
slotEmpty.setFillColor(40, 40, 55, 220);
slotEmpty.setOutlineColor(100, 100, 120, 220);
slotEmpty.setOutlineThickness(1.0);

RectangleShape slotWorker = Rectangles::create(QSLOT_W, QSLOT_H);
slotWorker.setFillColor(40, 90, 50, 240);
slotWorker.setOutlineColor(150, 220, 170, 240);
slotWorker.setOutlineThickness(1.0);

RectangleShape slotSoldier = Rectangles::create(QSLOT_W, QSLOT_H);
slotSoldier.setFillColor(110, 40, 40, 240);
slotSoldier.setOutlineColor(230, 150, 150, 240);
slotSoldier.setOutlineThickness(1.0);

RectangleShape slotProgress = Rectangles::create(QSLOT_W, 4.0);
slotProgress.setFillColor(240, 230, 90, 255);

Text qLetterW = Texts::create(hudFont, "W", 18);
qLetterW.setFillColor(230, 250, 230, 255);
Text qLetterS = Texts::create(hudFont, "S", 18);
qLetterS.setFillColor(250, 230, 230, 255);

// ---- minimap ----
float MM_SIZE  = 220.0;
float MM_PAD   = 12.0;
float mmX      = (float)WIN_W - MM_SIZE - MM_PAD;
float mmY      = MM_PAD;
float mmScale  = MM_SIZE / WORLD_W;

RectangleShape mmBg = Rectangles::create(MM_SIZE, MM_SIZE);
mmBg.setPosition(mmX, mmY);
mmBg.setFillColor(20, 25, 40, 200);
mmBg.setOutlineColor(180, 180, 200, 200);
mmBg.setOutlineThickness(1.0);

RectangleShape mmHallDot = Rectangles::create(7.0, 7.0);
mmHallDot.setOrigin(3.5, 3.5);
mmHallDot.setFillColor(170, 130, 80, 255);

RectangleShape mmBarracksDot = Rectangles::create(6.0, 6.0);
mmBarracksDot.setOrigin(3.0, 3.0);
mmBarracksDot.setFillColor(170, 80, 60, 255);

CircleShape mmNodeCrystal = Circles::create(2.8);
mmNodeCrystal.setOrigin(2.8, 2.8);
mmNodeCrystal.setFillColor(110, 180, 255, 255);

CircleShape mmNodeWood = Circles::create(2.8);
mmNodeWood.setOrigin(2.8, 2.8);
mmNodeWood.setFillColor(170, 110, 60, 255);

CircleShape mmWorkerDot = Circles::create(2.0);
mmWorkerDot.setOrigin(2.0, 2.0);
mmWorkerDot.setFillColor(80, 200, 110, 255);

CircleShape mmSoldierDot = Circles::create(2.5);
mmSoldierDot.setOrigin(2.5, 2.5);
mmSoldierDot.setFillColor(230, 90, 90, 255);

CircleShape mmEnemyDot = Circles::create(3.0);
mmEnemyDot.setOrigin(3.0, 3.0);
mmEnemyDot.setFillColor(170, 40, 40, 255);

RectangleShape mmViewport = Rectangles::create(1.0, 1.0);
mmViewport.setFillColor(0, 0, 0, 0);
mmViewport.setOutlineColor(255, 255, 255, 200);
mmViewport.setOutlineThickness(1.0);

// ---- selection / interaction state ----
int   hallSelected     = 0;
int   barracksSelected = 0;
int   selecting        = 0;
float selStartX        = 0.0;
float selStartY        = 0.0;
float selEndX          = 0.0;
float selEndY          = 0.0;
float CLICK_DRAG_PX    = 6.0;

int evClosed   = Sfml::closedEventId();
int evKeyDown  = Sfml::keyPressedEventId();
int evMouseDn  = Sfml::mouseButtonPressedEventId();
int evMouseUp  = Sfml::mouseButtonReleasedEventId();
int evWheel    = Sfml::mouseWheelScrolledEventId();
int evResized  = Sfml::resizedEventId();
int kEsc = Key::escape();
int kW = Key::w();   int kA = Key::a();
int kS = Key::s();   int kD = Key::d();
int kQ = Key::q();   int kE = Key::e();
int kSpace = Key::space();
int kB  = Key::b();
int kF5 = Key::f5();
int kF9 = Key::f9();
int mbLeft  = MouseButton::left();
int mbRight = MouseButton::right();

int spaceLatch = 0;
int bLatch     = 0;
int f5Latch    = 0;
int f9Latch    = 0;

Clock frame = Clocks::create();

int   fpsFrames = 0;
float fpsAccum  = 0.0;
int   fpsValue  = 0;

int prevCrystal  = -1;
int prevWood     = -1;
int prevWorkers  = -1;
int prevSoldiers = -1;
int prevSelected = -1;
int prevQueue    = -1;
int prevFps      = -1;

float[] sepDX = new float[MAX_UNITS];
float[] sepDY = new float[MAX_UNITS];

function hallSpawnX(int seed): float {
    return HALL_X + (float)((seed * 17) % 60 - 30);
}
function hallSpawnY(int seed): float {
    return HALL_Y + HALL_HALF + 28.0 + (float)((seed * 23) % 30);
}
function barracksSpawnX(int seed): float {
    return BARRACKS_X + (float)((seed * 19) % 60 - 30);
}
function barracksSpawnY(int seed): float {
    return BARRACKS_Y + BARRACKS_HALF + 28.0 + (float)((seed * 29) % 30);
}

while (window.isOpen()) {
    // ============================================================
    // events
    // ============================================================
    int ev = window.pollEvent();
    while (ev != 0) {
        if (ev == evClosed) {
            window.close();
        } else if (ev == evKeyDown) {
            if (Event::key() == kEsc) { window.close(); }
        } else if (ev == evMouseDn) {
            int mb = Event::mouseButton();
            if (mb == mbLeft) {
                float mxe = (float)Event::mouseX();
                float mye = (float)Event::mouseY();
                int consumed = 0;

                if (hallSelected == 1) {
                    if (inRect(mxe, mye, btn0X, btn0Y, BTN_W, BTN_H) == 1) {
                        if (crystalStockpile >= WORKER_COST_CRYSTAL
                                && woodStockpile >= WORKER_COST_WOOD
                                && hallQueueCount < QUEUE_MAX) {
                            hallQueue[hallQueueCount] = UT_WORKER;
                            hallQueueCount = hallQueueCount + 1;
                            crystalStockpile = crystalStockpile - WORKER_COST_CRYSTAL;
                            woodStockpile    = woodStockpile - WORKER_COST_WOOD;
                        }
                        consumed = 1;
                    }
                } else if (barracksSelected == 1) {
                    if (inRect(mxe, mye, btn0X, btn0Y, BTN_W, BTN_H) == 1) {
                        if (crystalStockpile >= SOLDIER_COST_CRYSTAL
                                && woodStockpile >= SOLDIER_COST_WOOD
                                && barracksQueueCount < QUEUE_MAX) {
                            barracksQueue[barracksQueueCount] = UT_SOLDIER;
                            barracksQueueCount = barracksQueueCount + 1;
                            crystalStockpile = crystalStockpile - SOLDIER_COST_CRYSTAL;
                            woodStockpile    = woodStockpile - SOLDIER_COST_WOOD;
                        }
                        consumed = 1;
                    }
                }

                if (consumed == 0) {
                    selecting = 1;
                    selStartX = mxe;
                    selStartY = mye;
                    selEndX = selStartX;
                    selEndY = selStartY;
                }
            } else if (mb == mbRight) {
                float mxe = (float)Event::mouseX();
                float mye = (float)Event::mouseY();
                int rConsumed = 0;

                // Cancel + refund from whichever building is open.
                if (hallSelected == 1) {
                    for (int q = 0; q < hallQueueCount; q++) {
                        float qx = qSlot0X + (float)q * (QSLOT_W + QSLOT_GAP);
                        if (inRect(mxe, mye, qx, qSlotY, QSLOT_W, QSLOT_H) == 1) {
                            int t = hallQueue[q];
                            crystalStockpile = crystalStockpile + costCrystalOf(t);
                            woodStockpile    = woodStockpile + costWoodOf(t);
                            for (int s = q; s < hallQueueCount - 1; s++) {
                                hallQueue[s] = hallQueue[s + 1];
                            }
                            hallQueueCount = hallQueueCount - 1;
                            if (q == 0) { hallBuildProg = 0.0; }
                            rConsumed = 1;
                            break;
                        }
                    }
                } else if (barracksSelected == 1) {
                    for (int q = 0; q < barracksQueueCount; q++) {
                        float qx = qSlot0X + (float)q * (QSLOT_W + QSLOT_GAP);
                        if (inRect(mxe, mye, qx, qSlotY, QSLOT_W, QSLOT_H) == 1) {
                            int t = barracksQueue[q];
                            crystalStockpile = crystalStockpile + costCrystalOf(t);
                            woodStockpile    = woodStockpile + costWoodOf(t);
                            for (int s = q; s < barracksQueueCount - 1; s++) {
                                barracksQueue[s] = barracksQueue[s + 1];
                            }
                            barracksQueueCount = barracksQueueCount - 1;
                            if (q == 0) { barracksBuildProg = 0.0; }
                            rConsumed = 1;
                            break;
                        }
                    }
                }

                if (rConsumed == 0) {
                    float sw = (float)WIN_W * camZoom;
                    float sh = (float)WIN_H * camZoom;
                    float[] w = Coords::screenToWorld((float)Event::mouseX(),
                                                        (float)Event::mouseY(),
                                                        camX, camY, sw, sh,
                                                        (float)WIN_W, (float)WIN_H);
                    float wx = w[0];
                    float wy = w[1];

                    int hitEnemy = -1;
                    float enR2 = (ENEMY_RADIUS + SOLDIER_RADIUS) * (ENEMY_RADIUS + SOLDIER_RADIUS);
                    for (int e = 0; e < ENEMY_COUNT; e++) {
                        if (enemies[e].hp > 0) {
                            // Only target visible enemies — can't
                            // attack what you can't see.
                            int cellX = (int)(enemies[e].x / FOG_CELL);
                            int cellY = (int)(enemies[e].y / FOG_CELL);
                            int vis = 0;
                            if (cellX >= 0 && cellY >= 0
                                    && cellX < FOG_W && cellY < FOG_H) {
                                if (fog[cellY * FOG_W + cellX] == FOG_VISIBLE) {
                                    vis = 1;
                                }
                            }
                            if (vis == 1) {
                                float dx = enemies[e].x - wx;
                                float dy = enemies[e].y - wy;
                                if (dx * dx + dy * dy <= enR2) { hitEnemy = e; }
                            }
                        }
                    }
                    int hitNode = -1;
                    if (hitEnemy < 0) {
                        float ndR2 = (NODE_RADIUS + WORKER_RADIUS) * (NODE_RADIUS + WORKER_RADIUS);
                        for (int k = 0; k < NODE_COUNT; k++) {
                            if (nodes[k].amount > 0) {
                                float dx = nodes[k].x - wx;
                                float dy = nodes[k].y - wy;
                                if (dx * dx + dy * dy <= ndR2) { hitNode = k; }
                            }
                        }
                    }

                    if (hitEnemy >= 0) {
                        for (int i = 0; i < unitCount; i++) {
                            if (units[i].selected == 1 && units[i].type == UT_SOLDIER) {
                                units[i].targetEnemy = hitEnemy;
                                units[i].state = SOL_TO_ENEMY;
                                units[i].tx = enemies[hitEnemy].x;
                                units[i].ty = enemies[hitEnemy].y;
                                units[i].hasTarget = 1;
                            }
                        }
                        int wkCount = 0;
                        for (int i = 0; i < unitCount; i++) {
                            if (units[i].selected == 1 && units[i].type == UT_WORKER) {
                                wkCount = wkCount + 1;
                            }
                        }
                        if (wkCount > 0) {
                            int side = isqrtCeil(wkCount);
                            float ox = wx - (float)(side - 1) * FORM_SPACING * 0.5;
                            float oy = wy - (float)(side - 1) * FORM_SPACING * 0.5;
                            int slot = 0;
                            for (int i = 0; i < unitCount; i++) {
                                if (units[i].selected == 1 && units[i].type == UT_WORKER) {
                                    int sx = slot % side;
                                    int sy = slot / side;
                                    units[i].state = WS_IDLE;
                                    units[i].assignedNode = -1;
                                    units[i].tx = ox + (float)sx * FORM_SPACING;
                                    units[i].ty = oy + (float)sy * FORM_SPACING;
                                    units[i].hasTarget = 1;
                                    slot = slot + 1;
                                }
                            }
                        }
                    } else if (hitNode >= 0) {
                        for (int i = 0; i < unitCount; i++) {
                            if (units[i].selected == 1 && units[i].type == UT_WORKER) {
                                units[i].assignedNode = hitNode;
                                if (units[i].cargo > 0) {
                                    units[i].state = WS_TO_HALL;
                                    units[i].tx = HALL_X;
                                    units[i].ty = HALL_Y;
                                } else {
                                    units[i].state = WS_TO_RES;
                                    units[i].tx = nodes[hitNode].x;
                                    units[i].ty = nodes[hitNode].y;
                                }
                                units[i].hasTarget = 1;
                            }
                        }
                        int solCount = 0;
                        for (int i = 0; i < unitCount; i++) {
                            if (units[i].selected == 1 && units[i].type == UT_SOLDIER) {
                                solCount = solCount + 1;
                            }
                        }
                        if (solCount > 0) {
                            int side = isqrtCeil(solCount);
                            float ox = wx - (float)(side - 1) * FORM_SPACING * 0.5;
                            float oy = wy - (float)(side - 1) * FORM_SPACING * 0.5;
                            int slot = 0;
                            for (int i = 0; i < unitCount; i++) {
                                if (units[i].selected == 1 && units[i].type == UT_SOLDIER) {
                                    int sx = slot % side;
                                    int sy = slot / side;
                                    units[i].state = WS_IDLE;
                                    units[i].targetEnemy = -1;
                                    units[i].tx = ox + (float)sx * FORM_SPACING;
                                    units[i].ty = oy + (float)sy * FORM_SPACING;
                                    units[i].hasTarget = 1;
                                    slot = slot + 1;
                                }
                            }
                        }
                    } else {
                        int selCount = 0;
                        for (int i = 0; i < unitCount; i++) {
                            if (units[i].selected == 1) { selCount = selCount + 1; }
                        }
                        if (selCount > 0) {
                            int side = isqrtCeil(selCount);
                            float ox = wx - (float)(side - 1) * FORM_SPACING * 0.5;
                            float oy = wy - (float)(side - 1) * FORM_SPACING * 0.5;
                            int slot = 0;
                            for (int i = 0; i < unitCount; i++) {
                                if (units[i].selected == 1) {
                                    int sx = slot % side;
                                    int sy = slot / side;
                                    units[i].state = WS_IDLE;
                                    units[i].assignedNode = -1;
                                    units[i].targetEnemy = -1;
                                    units[i].tx = ox + (float)sx * FORM_SPACING;
                                    units[i].ty = oy + (float)sy * FORM_SPACING;
                                    units[i].hasTarget = 1;
                                    slot = slot + 1;
                                }
                            }
                        }
                    }
                }
            }
        } else if (ev == evMouseUp) {
            if (Event::mouseButton() == mbLeft) {
                float dragX = selEndX - selStartX;
                float dragY = selEndY - selStartY;
                if (dragX < 0.0) { dragX = -dragX; }
                if (dragY < 0.0) { dragY = -dragY; }
                int isClick = 0;
                if (dragX < CLICK_DRAG_PX && dragY < CLICK_DRAG_PX) { isClick = 1; }

                float sw = (float)WIN_W * camZoom;
                float sh = (float)WIN_H * camZoom;
                float[] a = Coords::screenToWorld(selStartX, selStartY,
                                                    camX, camY, sw, sh,
                                                    (float)WIN_W, (float)WIN_H);
                float[] b = Coords::screenToWorld(selEndX, selEndY,
                                                    camX, camY, sw, sh,
                                                    (float)WIN_W, (float)WIN_H);
                float minX = a[0]; float maxX = b[0];
                float minY = a[1]; float maxY = b[1];
                if (maxX < minX) { float t = minX; minX = maxX; maxX = t; }
                if (maxY < minY) { float t = minY; minY = maxY; maxY = t; }

                if (isClick == 1) {
                    float cx = a[0]; float cy = a[1];
                    int hitHall = 0;
                    int hitBarracks = 0;
                    if (cx >= HALL_X - HALL_HALF && cx <= HALL_X + HALL_HALF
                            && cy >= HALL_Y - HALL_HALF && cy <= HALL_Y + HALL_HALF) {
                        hitHall = 1;
                    } else if (cx >= BARRACKS_X - BARRACKS_HALF
                            && cx <= BARRACKS_X + BARRACKS_HALF
                            && cy >= BARRACKS_Y - BARRACKS_HALF
                            && cy <= BARRACKS_Y + BARRACKS_HALF) {
                        hitBarracks = 1;
                    }

                    if (hitHall == 1) {
                        hallSelected = 1;
                        barracksSelected = 0;
                        for (int i = 0; i < unitCount; i++) { units[i].selected = 0; }
                    } else if (hitBarracks == 1) {
                        hallSelected = 0;
                        barracksSelected = 1;
                        for (int i = 0; i < unitCount; i++) { units[i].selected = 0; }
                    } else {
                        int hit = -1;
                        for (int i = 0; i < unitCount; i++) {
                            float dx = units[i].x - cx;
                            float dy = units[i].y - cy;
                            float ur = units[i].radius;
                            if (dx * dx + dy * dy <= ur * ur) { hit = i; }
                        }
                        hallSelected = 0;
                        barracksSelected = 0;
                        for (int i = 0; i < unitCount; i++) { units[i].selected = 0; }
                        if (hit >= 0) { units[hit].selected = 1; }
                    }
                } else {
                    hallSelected = 0;
                    barracksSelected = 0;
                    for (int i = 0; i < unitCount; i++) {
                        float ux = units[i].x;
                        float uy = units[i].y;
                        if (ux >= minX && ux <= maxX && uy >= minY && uy <= maxY) {
                            units[i].selected = 1;
                        } else {
                            units[i].selected = 0;
                        }
                    }
                }
                selecting = 0;
            }
        } else if (ev == evWheel) {
            float d = Event::wheelDelta();
            if (d > 0.0) { camZoom = camZoom * ZOOM_STEP; }
            if (d < 0.0) { camZoom = camZoom / ZOOM_STEP; }
        } else if (ev == evResized) {
            WIN_W = Event::resizeWidth();
            WIN_H = Event::resizeHeight();
            txtHint.setPosition(10.0, (float)WIN_H - 22.0);
            mmX = (float)WIN_W - MM_SIZE - MM_PAD;
            mmBg.setPosition(mmX, mmY);
            bmX = (float)WIN_W * 0.5 - BM_W * 0.5;
            bmY = (float)WIN_H - 32.0 - BM_H;
            btn0X = bmX + (BM_W - BTN_W) * 0.5;
            btn0Y = bmY + 12.0;
            qSlot0X = bmX + (BM_W - (QSLOT_W * 5.0 + QSLOT_GAP * 4.0)) * 0.5;
            qSlotY  = bmY + 78.0;
        }
        ev = window.pollEvent();
    }

    float dt = frame.restartSeconds();
    if (dt > 0.1) { dt = 0.1; }

    // ============================================================
    // camera + keyboard shortcuts
    // ============================================================
    float pan = PAN_SPEED * camZoom * dt;
    if (Keyboard::isKeyPressed(kW)) { camY = camY - pan; }
    if (Keyboard::isKeyPressed(kS)) { camY = camY + pan; }
    if (Keyboard::isKeyPressed(kA)) { camX = camX - pan; }
    if (Keyboard::isKeyPressed(kD)) { camX = camX + pan; }

    int[] mp = window.mousePosition();
    float mx = (float)mp[0];
    float my = (float)mp[1];
    if (window.hasFocus()) {
        if (mx >= 0.0 && my >= 0.0 && mx < (float)WIN_W && my < (float)WIN_H) {
            if (mx < EDGE_PAD)                  { camX = camX - pan; }
            if (mx > (float)WIN_W - EDGE_PAD)   { camX = camX + pan; }
            if (my < EDGE_PAD)                  { camY = camY - pan; }
            if (my > (float)WIN_H - EDGE_PAD)   { camY = camY + pan; }
        }
        if (selecting == 1) {
            selEndX = mx;
            selEndY = my;
        }
    }

    if (Keyboard::isKeyPressed(kQ)) { camZoom = camZoom * (1.0 + ZOOM_RATE * dt); }
    if (Keyboard::isKeyPressed(kE)) { camZoom = camZoom / (1.0 + ZOOM_RATE * dt); }
    if (camZoom < ZOOM_MIN) { camZoom = ZOOM_MIN; }
    if (camZoom > ZOOM_MAX) { camZoom = ZOOM_MAX; }

    // Space queues a Worker at the Hall.
    int spaceNow = 0;
    if (Keyboard::isKeyPressed(kSpace)) { spaceNow = 1; }
    if (spaceNow == 1 && spaceLatch == 0
            && hallQueueCount < QUEUE_MAX
            && crystalStockpile >= WORKER_COST_CRYSTAL
            && woodStockpile >= WORKER_COST_WOOD) {
        hallQueue[hallQueueCount] = UT_WORKER;
        hallQueueCount = hallQueueCount + 1;
        crystalStockpile = crystalStockpile - WORKER_COST_CRYSTAL;
        woodStockpile    = woodStockpile - WORKER_COST_WOOD;
    }
    spaceLatch = spaceNow;

    // B queues a Soldier at the Barracks.
    int bNow = 0;
    if (Keyboard::isKeyPressed(kB)) { bNow = 1; }
    if (bNow == 1 && bLatch == 0
            && barracksQueueCount < QUEUE_MAX
            && crystalStockpile >= SOLDIER_COST_CRYSTAL
            && woodStockpile >= SOLDIER_COST_WOOD) {
        barracksQueue[barracksQueueCount] = UT_SOLDIER;
        barracksQueueCount = barracksQueueCount + 1;
        crystalStockpile = crystalStockpile - SOLDIER_COST_CRYSTAL;
        woodStockpile    = woodStockpile - SOLDIER_COST_WOOD;
    }
    bLatch = bNow;

    // F5 / F9.
    int f5Now = 0;
    if (Keyboard::isKeyPressed(kF5)) { f5Now = 1; }
    if (f5Now == 1 && f5Latch == 0) {
        snapCrystal   = crystalStockpile;
        snapWood      = woodStockpile;
        snapCamX      = camX;
        snapCamY      = camY;
        snapCamZoom   = camZoom;
        snapUnitCount = unitCount;
        for (int i = 0; i < unitCount; i++) {
            snapUX[i]           = units[i].x;
            snapUY[i]           = units[i].y;
            snapUTx[i]          = units[i].tx;
            snapUTy[i]          = units[i].ty;
            snapUHasTarget[i]   = units[i].hasTarget;
            snapUSelected[i]    = units[i].selected;
            snapUType[i]        = units[i].type;
            snapUState[i]       = units[i].state;
            snapUSpeed[i]       = units[i].speed;
            snapURadius[i]      = units[i].radius;
            snapUHp[i]          = units[i].hp;
            snapUAssignedN[i]   = units[i].assignedNode;
            snapUCargo[i]       = units[i].cargo;
            snapUHarvestT[i]    = units[i].harvestTimer;
            snapUTargetEnemy[i] = units[i].targetEnemy;
            snapUAttackT[i]     = units[i].attackTimer;
        }
        for (int k = 0; k < NODE_COUNT; k++) {
            snapNodeAmount[k] = nodes[k].amount;
        }
        for (int e = 0; e < ENEMY_COUNT; e++) {
            snapEnemyHp[e] = enemies[e].hp;
        }
        snapHallQueueCnt = hallQueueCount;
        for (int q = 0; q < hallQueueCount; q++) {
            snapHallQueue[q] = hallQueue[q];
        }
        snapHallBuildProg = hallBuildProg;
        snapBarracksQueueCnt = barracksQueueCount;
        for (int q = 0; q < barracksQueueCount; q++) {
            snapBarracksQueue[q] = barracksQueue[q];
        }
        snapBarracksBuildProg = barracksBuildProg;
        // Save the explored mask only — VISIBLE cells re-derive each
        // frame from unit/building positions.
        for (int c = 0; c < fogCellCount; c++) {
            int st = fog[c];
            if (st == FOG_VISIBLE) { snapFog[c] = FOG_EXPLORED; }
            else                    { snapFog[c] = st; }
        }
        hasSnapshot = 1;
        toastMsg = "Saved";
        toastTimer = 2.0;
    }
    f5Latch = f5Now;

    int f9Now = 0;
    if (Keyboard::isKeyPressed(kF9)) { f9Now = 1; }
    if (f9Now == 1 && f9Latch == 0 && hasSnapshot == 1) {
        crystalStockpile = snapCrystal;
        woodStockpile    = snapWood;
        camX             = snapCamX;
        camY             = snapCamY;
        camZoom          = snapCamZoom;
        unitCount        = snapUnitCount;
        for (int i = 0; i < unitCount; i++) {
            if (units[i] == null) {
                units[i] = new Unit(snapUX[i], snapUY[i], snapUType[i],
                                     snapUSpeed[i], snapURadius[i], snapUHp[i]);
            }
            units[i].x            = snapUX[i];
            units[i].y            = snapUY[i];
            units[i].tx           = snapUTx[i];
            units[i].ty           = snapUTy[i];
            units[i].hasTarget    = snapUHasTarget[i];
            units[i].selected     = snapUSelected[i];
            units[i].type         = snapUType[i];
            units[i].state        = snapUState[i];
            units[i].speed        = snapUSpeed[i];
            units[i].radius       = snapURadius[i];
            units[i].hp           = snapUHp[i];
            units[i].assignedNode = snapUAssignedN[i];
            units[i].cargo        = snapUCargo[i];
            units[i].harvestTimer = snapUHarvestT[i];
            units[i].targetEnemy  = snapUTargetEnemy[i];
            units[i].attackTimer  = snapUAttackT[i];
        }
        for (int k = 0; k < NODE_COUNT; k++) {
            nodes[k].amount = snapNodeAmount[k];
        }
        for (int e = 0; e < ENEMY_COUNT; e++) {
            enemies[e].hp = snapEnemyHp[e];
        }
        hallQueueCount = snapHallQueueCnt;
        for (int q = 0; q < hallQueueCount; q++) {
            hallQueue[q] = snapHallQueue[q];
        }
        hallBuildProg = snapHallBuildProg;
        barracksQueueCount = snapBarracksQueueCnt;
        for (int q = 0; q < barracksQueueCount; q++) {
            barracksQueue[q] = snapBarracksQueue[q];
        }
        barracksBuildProg = snapBarracksBuildProg;
        for (int c = 0; c < fogCellCount; c++) {
            fog[c] = snapFog[c];
        }
        hallSelected = 0;
        barracksSelected = 0;
        selecting = 0;
        toastMsg = "Loaded";
        toastTimer = 2.0;
    }
    f9Latch = f9Now;

    float halfW = (float)WIN_W * 0.5 * camZoom;
    float halfH = (float)WIN_H * 0.5 * camZoom;
    if (camX < halfW)            { camX = halfW; }
    if (camY < halfH)            { camY = halfH; }
    if (camX > WORLD_W - halfW)  { camX = WORLD_W - halfW; }
    if (camY > WORLD_H - halfH)  { camY = WORLD_H - halfH; }
    cam.setCenter(camX, camY);
    cam.setSize((float)WIN_W * camZoom, (float)WIN_H * camZoom);

    // ============================================================
    // build queue ticks — each building advances independently
    // ============================================================
    if (hallQueueCount > 0) {
        int headType = hallQueue[0];
        float bt = buildTimeOf(headType);
        hallBuildProg = hallBuildProg + dt;
        if (hallBuildProg >= bt) {
            if (unitCount < MAX_UNITS) {
                if (headType == UT_WORKER) {
                    units[unitCount] = newWorker(hallSpawnX(unitCount),
                                                  hallSpawnY(unitCount));
                } else {
                    units[unitCount] = newSoldier(hallSpawnX(unitCount),
                                                   hallSpawnY(unitCount));
                }
                unitCount = unitCount + 1;
            }
            for (int s = 0; s < hallQueueCount - 1; s++) {
                hallQueue[s] = hallQueue[s + 1];
            }
            hallQueueCount = hallQueueCount - 1;
            hallBuildProg = 0.0;
        }
    }
    if (barracksQueueCount > 0) {
        int headType = barracksQueue[0];
        float bt = buildTimeOf(headType);
        barracksBuildProg = barracksBuildProg + dt;
        if (barracksBuildProg >= bt) {
            if (unitCount < MAX_UNITS) {
                if (headType == UT_WORKER) {
                    units[unitCount] = newWorker(barracksSpawnX(unitCount),
                                                  barracksSpawnY(unitCount));
                } else {
                    units[unitCount] = newSoldier(barracksSpawnX(unitCount),
                                                   barracksSpawnY(unitCount));
                }
                unitCount = unitCount + 1;
            }
            for (int s = 0; s < barracksQueueCount - 1; s++) {
                barracksQueue[s] = barracksQueue[s + 1];
            }
            barracksQueueCount = barracksQueueCount - 1;
            barracksBuildProg = 0.0;
        }
    }

    // ============================================================
    // simulation
    // ============================================================
    float harvestR2 = HARVEST_RANGE * HARVEST_RANGE;
    float depositR2 = DEPOSIT_RANGE * DEPOSIT_RANGE;
    float atkR2     = SOLDIER_ATTACK_RANGE * SOLDIER_ATTACK_RANGE;

    for (int i = 0; i < unitCount; i++) {
        int s = units[i].state;
        if (s == WS_TO_RES) {
            int k = units[i].assignedNode;
            if (k < 0 || nodes[k].amount <= 0) {
                units[i].state = WS_IDLE;
                units[i].assignedNode = -1;
                units[i].hasTarget = 0;
            } else {
                float dx = nodes[k].x - units[i].x;
                float dy = nodes[k].y - units[i].y;
                if (dx * dx + dy * dy <= harvestR2) {
                    units[i].state = WS_HARVEST;
                    units[i].hasTarget = 0;
                    units[i].harvestTimer = HARVEST_TIME;
                }
            }
        } else if (s == WS_HARVEST) {
            units[i].harvestTimer = units[i].harvestTimer - dt;
            if (units[i].harvestTimer <= 0.0) {
                int k = units[i].assignedNode;
                int take = HARVEST_AMOUNT;
                if (k >= 0) {
                    if (nodes[k].amount < take) { take = nodes[k].amount; }
                    nodes[k].amount = nodes[k].amount - take;
                    units[i].cargo = units[i].cargo + take;
                }
                if (units[i].cargo > 0) {
                    units[i].state = WS_TO_HALL;
                    units[i].tx = HALL_X;
                    units[i].ty = HALL_Y;
                    units[i].hasTarget = 1;
                } else {
                    units[i].state = WS_IDLE;
                    units[i].assignedNode = -1;
                }
            }
        } else if (s == WS_TO_HALL) {
            float dx = HALL_X - units[i].x;
            float dy = HALL_Y - units[i].y;
            if (dx * dx + dy * dy <= depositR2) {
                int k = units[i].assignedNode;
                int depositRes = RES_CRYSTAL;
                if (k >= 0) { depositRes = nodes[k].resType; }
                if (depositRes == RES_CRYSTAL) {
                    crystalStockpile = crystalStockpile + units[i].cargo;
                } else {
                    woodStockpile = woodStockpile + units[i].cargo;
                }
                units[i].cargo = 0;
                if (k >= 0 && nodes[k].amount > 0) {
                    units[i].state = WS_TO_RES;
                    units[i].tx = nodes[k].x;
                    units[i].ty = nodes[k].y;
                    units[i].hasTarget = 1;
                } else {
                    units[i].state = WS_IDLE;
                    units[i].assignedNode = -1;
                    units[i].hasTarget = 0;
                }
            }
        } else if (s == SOL_TO_ENEMY) {
            int eIdx = units[i].targetEnemy;
            if (eIdx < 0 || enemies[eIdx].hp <= 0) {
                units[i].state = WS_IDLE;
                units[i].targetEnemy = -1;
                units[i].hasTarget = 0;
            } else {
                float dx = enemies[eIdx].x - units[i].x;
                float dy = enemies[eIdx].y - units[i].y;
                if (dx * dx + dy * dy <= atkR2) {
                    units[i].state = SOL_ATTACKING;
                    units[i].hasTarget = 0;
                    units[i].attackTimer = SOLDIER_ATTACK_COOLDOWN;
                } else {
                    units[i].tx = enemies[eIdx].x;
                    units[i].ty = enemies[eIdx].y;
                    units[i].hasTarget = 1;
                }
            }
        } else if (s == SOL_ATTACKING) {
            int eIdx = units[i].targetEnemy;
            if (eIdx < 0 || enemies[eIdx].hp <= 0) {
                units[i].state = WS_IDLE;
                units[i].targetEnemy = -1;
            } else {
                float dx = enemies[eIdx].x - units[i].x;
                float dy = enemies[eIdx].y - units[i].y;
                float d2 = dx * dx + dy * dy;
                if (d2 > atkR2) {
                    units[i].state = SOL_TO_ENEMY;
                    units[i].tx = enemies[eIdx].x;
                    units[i].ty = enemies[eIdx].y;
                    units[i].hasTarget = 1;
                } else {
                    units[i].attackTimer = units[i].attackTimer - dt;
                    if (units[i].attackTimer <= 0.0) {
                        int newHp = enemies[eIdx].hp - SOLDIER_DAMAGE;
                        if (newHp < 0) { newHp = 0; }
                        enemies[eIdx].hp = newHp;
                        units[i].attackTimer = SOLDIER_ATTACK_COOLDOWN;
                    }
                }
            }
        }
    }

    for (int i = 0; i < unitCount; i++) {
        if (units[i].hasTarget == 1) {
            float ux = units[i].x;
            float uy = units[i].y;
            float dx = units[i].tx - ux;
            float dy = units[i].ty - uy;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist <= ARRIVE_EPS) {
                units[i].x = units[i].tx;
                units[i].y = units[i].ty;
                units[i].hasTarget = 0;
            } else {
                float move = units[i].speed * dt;
                if (move >= dist) {
                    units[i].x = units[i].tx;
                    units[i].y = units[i].ty;
                    units[i].hasTarget = 0;
                } else {
                    float inv = 1.0 / dist;
                    units[i].x = ux + dx * inv * move;
                    units[i].y = uy + dy * inv * move;
                }
            }
        }
    }

    for (int i = 0; i < unitCount; i++) {
        sepDX[i] = 0.0;
        sepDY[i] = 0.0;
    }
    for (int c = 0; c < cellCount; c++) { cellHead[c] = -1; }
    float invCell = 1.0 / SEP_GRID_CELL;
    for (int i = 0; i < unitCount; i++) {
        int cx = (int)(units[i].x * invCell);
        int cy = (int)(units[i].y * invCell);
        if (cx < 0) { cx = 0; }
        if (cy < 0) { cy = 0; }
        if (cx >= SEP_GRID_W) { cx = SEP_GRID_W - 1; }
        if (cy >= SEP_GRID_H) { cy = SEP_GRID_H - 1; }
        int c = cy * SEP_GRID_W + cx;
        cellNext[i] = cellHead[c];
        cellHead[c] = i;
    }
    float sepR2 = SEP_RADIUS * SEP_RADIUS;
    for (int i = 0; i < unitCount; i++) {
        int si = units[i].state;
        if (si == WS_HARVEST || si == SOL_ATTACKING) { continue; }
        float ix = units[i].x;
        float iy = units[i].y;
        int cx = (int)(ix * invCell);
        int cy = (int)(iy * invCell);
        if (cx < 0) { cx = 0; }
        if (cy < 0) { cy = 0; }
        if (cx >= SEP_GRID_W) { cx = SEP_GRID_W - 1; }
        if (cy >= SEP_GRID_H) { cy = SEP_GRID_H - 1; }
        int xMin = cx - 1; if (xMin < 0) { xMin = 0; }
        int xMax = cx + 1; if (xMax >= SEP_GRID_W) { xMax = SEP_GRID_W - 1; }
        int yMin = cy - 1; if (yMin < 0) { yMin = 0; }
        int yMax = cy + 1; if (yMax >= SEP_GRID_H) { yMax = SEP_GRID_H - 1; }

        for (int ncy = yMin; ncy <= yMax; ncy++) {
            int rowBase = ncy * SEP_GRID_W;
            for (int ncx = xMin; ncx <= xMax; ncx++) {
                int j = cellHead[rowBase + ncx];
                while (j != -1) {
                    if (j > i) {
                        int sj = units[j].state;
                        if (sj != WS_HARVEST && sj != SOL_ATTACKING) {
                            float dx = ix - units[j].x;
                            float dy = iy - units[j].y;
                            float d2 = dx * dx + dy * dy;
                            if (d2 > 0.0 && d2 < sepR2) {
                                float dist = sqrt(d2);
                                float overlap = SEP_RADIUS - dist;
                                float inv = 1.0 / dist;
                                float fx = dx * inv * overlap;
                                float fy = dy * inv * overlap;
                                sepDX[i] = sepDX[i] + fx;
                                sepDY[i] = sepDY[i] + fy;
                                sepDX[j] = sepDX[j] - fx;
                                sepDY[j] = sepDY[j] - fy;
                            }
                        }
                    }
                    j = cellNext[j];
                }
            }
        }
    }
    float sepStep = SEP_STRENGTH * dt;
    for (int i = 0; i < unitCount; i++) {
        units[i].x = units[i].x + sepDX[i] * sepStep / SEP_RADIUS;
        units[i].y = units[i].y + sepDY[i] * sepStep / SEP_RADIUS;
    }

    // ============================================================
    // fog of war — downgrade VISIBLE -> EXPLORED, then re-reveal
    // around friendly entities, then rebuild only the cells whose
    // displayed alpha changed since last frame.
    // ============================================================
    for (int c = 0; c < fogCellCount; c++) {
        if (fog[c] == FOG_VISIBLE) { fog[c] = FOG_EXPLORED; }
    }
    for (int i = 0; i < unitCount; i++) {
        float sr = WORKER_SIGHT;
        if (units[i].type == UT_SOLDIER) { sr = SOLDIER_SIGHT; }
        revealAround(fog, units[i].x, units[i].y, sr);
    }
    revealAround(fog, HALL_X, HALL_Y, HALL_SIGHT);
    revealAround(fog, BARRACKS_X, BARRACKS_Y, BARRACKS_SIGHT);

    // Rebuild only the changed fog cells' VA color (positions are
    // static, alpha is the only thing that changes).
    for (int c = 0; c < fogCellCount; c++) {
        if (fog[c] != fogPrev[c]) {
            int st = fog[c];
            int a = 230;
            if      (st == FOG_VISIBLE)  { a = 0; }
            else if (st == FOG_EXPLORED) { a = 130; }
            int gy = c / FOG_W;
            int gx = c - gy * FOG_W;
            float x0 = (float)gx * FOG_CELL;
            float y0 = (float)gy * FOG_CELL;
            float x1 = x0 + FOG_CELL;
            float y1 = y0 + FOG_CELL;
            int idx = c * 6;
            fogVA.setVertex(idx + 0, x0, y0, 0, 0, 0, a, 0.0, 0.0);
            fogVA.setVertex(idx + 1, x1, y0, 0, 0, 0, a, 0.0, 0.0);
            fogVA.setVertex(idx + 2, x1, y1, 0, 0, 0, a, 0.0, 0.0);
            fogVA.setVertex(idx + 3, x0, y0, 0, 0, 0, a, 0.0, 0.0);
            fogVA.setVertex(idx + 4, x1, y1, 0, 0, 0, a, 0.0, 0.0);
            fogVA.setVertex(idx + 5, x0, y1, 0, 0, 0, a, 0.0, 0.0);
            fogPrev[c] = fog[c];
        }
    }

    // ============================================================
    // HUD bookkeeping
    // ============================================================
    int workerCount  = 0;
    int soldierCount = 0;
    int selCount     = 0;
    for (int i = 0; i < unitCount; i++) {
        if (units[i].type == UT_WORKER) {
            workerCount = workerCount + 1;
        } else {
            soldierCount = soldierCount + 1;
        }
        if (units[i].selected == 1) { selCount = selCount + 1; }
    }
    int totalQueue = hallQueueCount + barracksQueueCount;

    fpsFrames = fpsFrames + 1;
    fpsAccum  = fpsAccum + dt;
    if (fpsAccum >= 0.5) {
        fpsValue = (int)((float)fpsFrames / fpsAccum);
        fpsFrames = 0;
        fpsAccum  = 0.0;
    }

    if (crystalStockpile != prevCrystal) {
        txtCrystal.setString("Crystal: " + crystalStockpile);
        prevCrystal = crystalStockpile;
    }
    if (woodStockpile != prevWood) {
        txtWood.setString("Wood: " + woodStockpile);
        prevWood = woodStockpile;
    }
    if (workerCount != prevWorkers) {
        txtWorkers.setString("Workers: " + workerCount
            + "   (Sp:" + WORKER_COST_CRYSTAL + "C/" + WORKER_COST_WOOD + "W)");
        prevWorkers = workerCount;
    }
    if (soldierCount != prevSoldiers) {
        txtSoldiers.setString("Soldiers: " + soldierCount
            + "   (B:" + SOLDIER_COST_CRYSTAL + "C/" + SOLDIER_COST_WOOD + "W)");
        prevSoldiers = soldierCount;
    }
    if (selCount != prevSelected || totalQueue != prevQueue) {
        txtSelected.setString("Selected: " + selCount + "   Queue: " + totalQueue);
        prevSelected = selCount;
        prevQueue = totalQueue;
    }
    if (fpsValue != prevFps) {
        txtFps.setString("FPS: " + fpsValue);
        prevFps = fpsValue;
    }

    if (toastTimer > 0.0) {
        toastTimer = toastTimer - dt;
        if (toastTimer < 0.0) { toastTimer = 0.0; }
    }

    // ============================================================
    // render — world pass
    // ============================================================
    window.clear(12, 14, 20, 255);
    Camera::setView(window, cam);
    Draw::vertexArray(window, ground);

    // Resource nodes — render in explored or visible cells.
    for (int k = 0; k < NODE_COUNT; k++) {
        if (nodes[k].amount > 0) {
            int cx = (int)(nodes[k].x / FOG_CELL);
            int cy = (int)(nodes[k].y / FOG_CELL);
            int seen = 0;
            if (cx >= 0 && cy >= 0 && cx < FOG_W && cy < FOG_H) {
                int st = fog[cy * FOG_W + cx];
                if (st >= FOG_EXPLORED) { seen = 1; }
            }
            if (seen == 1) {
                float ratio = (float)nodes[k].amount / (float)NODE_AMOUNT_INIT;
                float scale = 0.45 + 0.55 * ratio;
                if (nodes[k].resType == RES_CRYSTAL) {
                    nodeShapeCrystal.setScale(scale, scale);
                    nodeShapeCrystal.setPosition(nodes[k].x, nodes[k].y);
                    Draw::circle(window, nodeShapeCrystal);
                } else {
                    nodeShapeWood.setScale(scale, scale);
                    nodeShapeWood.setPosition(nodes[k].x, nodes[k].y);
                    Draw::circle(window, nodeShapeWood);
                }
            }
        }
    }

    // Buildings — always render (yours, always known).
    Draw::rect(window, hallShape);
    if (hallSelected == 1) { Draw::rect(window, hallSelectionRing); }
    Draw::rect(window, barracksShape);
    if (barracksSelected == 1) { Draw::rect(window, barracksSelectionRing); }

    // Enemies — only render if their cell is currently VISIBLE.
    for (int e = 0; e < ENEMY_COUNT; e++) {
        if (enemies[e].hp > 0) {
            int cx = (int)(enemies[e].x / FOG_CELL);
            int cy = (int)(enemies[e].y / FOG_CELL);
            int vis = 0;
            if (cx >= 0 && cy >= 0 && cx < FOG_W && cy < FOG_H) {
                if (fog[cy * FOG_W + cx] == FOG_VISIBLE) { vis = 1; }
            }
            if (vis == 1) {
                enemyShape.setPosition(enemies[e].x, enemies[e].y);
                Draw::circle(window, enemyShape);
                float bx = enemies[e].x - 18.0;
                float by = enemies[e].y - ENEMY_RADIUS - 12.0;
                hpBarBg.setPosition(bx, by);
                Draw::rect(window, hpBarBg);
                float ratio = (float)enemies[e].hp / (float)enemies[e].maxHp;
                hpBarFg.setSize(34.0 * ratio, 3.0);
                hpBarFg.setPosition(bx + 1.0, by + 1.0);
                Draw::rect(window, hpBarFg);
            }
        }
    }

    // Friendly units — always render.
    for (int i = 0; i < unitCount; i++) {
        if (units[i].selected == 1) {
            unitRing.setPosition(units[i].x, units[i].y);
            Draw::circle(window, unitRing);
        }
    }
    for (int i = 0; i < unitCount; i++) {
        if (units[i].type == UT_WORKER && units[i].cargo == 0) {
            workerBody.setPosition(units[i].x, units[i].y);
            Draw::circle(window, workerBody);
        }
    }
    for (int i = 0; i < unitCount; i++) {
        if (units[i].type == UT_WORKER && units[i].cargo > 0) {
            workerBodyLoaded.setPosition(units[i].x, units[i].y);
            Draw::circle(window, workerBodyLoaded);
        }
    }
    for (int i = 0; i < unitCount; i++) {
        if (units[i].type == UT_SOLDIER) {
            soldierBody.setPosition(units[i].x, units[i].y);
            Draw::circle(window, soldierBody);
        }
    }

    // Fog overlay — drawn on top of everything in the world pass, so
    // hidden terrain darkens and unexplored areas blackout.
    Draw::vertexArray(window, fogVA);

    // ============================================================
    // render — HUD pass
    // ============================================================
    Camera::resetView(window);

    if (selecting == 1) {
        float bx = selStartX;
        float by = selStartY;
        float bw = selEndX - selStartX;
        float bh = selEndY - selStartY;
        if (bw < 0.0) { bx = selEndX; bw = -bw; }
        if (bh < 0.0) { by = selEndY; bh = -bh; }
        selBox.setPosition(bx, by);
        selBox.setSize(bw, bh);
        Draw::rect(window, selBox);
    }

    Draw::rect(window, hudPanel);
    Draw::circle(window, hudCrystalIcon);
    Draw::circle(window, hudWoodIcon);
    Draw::text(window, txtCrystal);
    Draw::text(window, txtWood);
    Draw::text(window, txtWorkers);
    Draw::text(window, txtSoldiers);
    Draw::text(window, txtSelected);
    Draw::text(window, txtFps);
    Draw::text(window, txtHint);

    // Build menu — content swaps based on which building is selected.
    if (hallSelected == 1 || barracksSelected == 1) {
        bmPanel.setPosition(bmX, bmY);
        Draw::rect(window, bmPanel);

        // Pick the active queue + label/cost.
        int[]  activeQ;
        int    activeQCount;
        float  activeProg;
        if (hallSelected == 1) {
            activeQ      = hallQueue;
            activeQCount = hallQueueCount;
            activeProg   = hallBuildProg;

            btnWorkerBg.setPosition(btn0X, btn0Y);
            Draw::rect(window, btnWorkerBg);
            btnWorkerLabel.setPosition(btn0X + 36.0, btn0Y + 6.0);
            Draw::text(window, btnWorkerLabel);
            btnWorkerCost.setPosition(btn0X + 36.0, btn0Y + 30.0);
            Draw::text(window, btnWorkerCost);
        } else {
            activeQ      = barracksQueue;
            activeQCount = barracksQueueCount;
            activeProg   = barracksBuildProg;

            btnSoldierBg.setPosition(btn0X, btn0Y);
            Draw::rect(window, btnSoldierBg);
            btnSoldierLabel.setPosition(btn0X + 38.0, btn0Y + 6.0);
            Draw::text(window, btnSoldierLabel);
            btnSoldierCost.setPosition(btn0X + 36.0, btn0Y + 30.0);
            Draw::text(window, btnSoldierCost);
        }

        txtQueueLabel.setPosition(qSlot0X - 50.0, qSlotY + 10.0);
        Draw::text(window, txtQueueLabel);
        for (int q = 0; q < QUEUE_MAX; q++) {
            float qx = qSlot0X + (float)q * (QSLOT_W + QSLOT_GAP);
            if (q < activeQCount) {
                int qtype = activeQ[q];
                if (qtype == UT_WORKER) {
                    slotWorker.setPosition(qx, qSlotY);
                    Draw::rect(window, slotWorker);
                    qLetterW.setPosition(qx + QSLOT_W * 0.5 - 5.0, qSlotY + 6.0);
                    Draw::text(window, qLetterW);
                } else {
                    slotSoldier.setPosition(qx, qSlotY);
                    Draw::rect(window, slotSoldier);
                    qLetterS.setPosition(qx + QSLOT_W * 0.5 - 5.0, qSlotY + 6.0);
                    Draw::text(window, qLetterS);
                }
                if (q == 0) {
                    float total = buildTimeOf(qtype);
                    float ratio = activeProg / total;
                    if (ratio > 1.0) { ratio = 1.0; }
                    slotProgress.setSize(QSLOT_W * ratio, 4.0);
                    slotProgress.setPosition(qx, qSlotY + QSLOT_H - 4.0);
                    Draw::rect(window, slotProgress);
                }
            } else {
                slotEmpty.setPosition(qx, qSlotY);
                Draw::rect(window, slotEmpty);
            }
        }
    }

    // Minimap.
    Draw::rect(window, mmBg);
    mmHallDot.setPosition(mmX + HALL_X * mmScale, mmY + HALL_Y * mmScale);
    Draw::rect(window, mmHallDot);
    mmBarracksDot.setPosition(mmX + BARRACKS_X * mmScale, mmY + BARRACKS_Y * mmScale);
    Draw::rect(window, mmBarracksDot);

    for (int k = 0; k < NODE_COUNT; k++) {
        if (nodes[k].amount > 0) {
            int cx = (int)(nodes[k].x / FOG_CELL);
            int cy = (int)(nodes[k].y / FOG_CELL);
            int seen = 0;
            if (cx >= 0 && cy >= 0 && cx < FOG_W && cy < FOG_H) {
                if (fog[cy * FOG_W + cx] >= FOG_EXPLORED) { seen = 1; }
            }
            if (seen == 1) {
                if (nodes[k].resType == RES_CRYSTAL) {
                    mmNodeCrystal.setPosition(mmX + nodes[k].x * mmScale,
                                               mmY + nodes[k].y * mmScale);
                    Draw::circle(window, mmNodeCrystal);
                } else {
                    mmNodeWood.setPosition(mmX + nodes[k].x * mmScale,
                                            mmY + nodes[k].y * mmScale);
                    Draw::circle(window, mmNodeWood);
                }
            }
        }
    }
    for (int e = 0; e < ENEMY_COUNT; e++) {
        if (enemies[e].hp > 0) {
            int cx = (int)(enemies[e].x / FOG_CELL);
            int cy = (int)(enemies[e].y / FOG_CELL);
            int vis = 0;
            if (cx >= 0 && cy >= 0 && cx < FOG_W && cy < FOG_H) {
                if (fog[cy * FOG_W + cx] == FOG_VISIBLE) { vis = 1; }
            }
            if (vis == 1) {
                mmEnemyDot.setPosition(mmX + enemies[e].x * mmScale,
                                        mmY + enemies[e].y * mmScale);
                Draw::circle(window, mmEnemyDot);
            }
        }
    }
    for (int i = 0; i < unitCount; i++) {
        if (units[i].type == UT_WORKER) {
            mmWorkerDot.setPosition(mmX + units[i].x * mmScale,
                                     mmY + units[i].y * mmScale);
            Draw::circle(window, mmWorkerDot);
        }
    }
    for (int i = 0; i < unitCount; i++) {
        if (units[i].type == UT_SOLDIER) {
            mmSoldierDot.setPosition(mmX + units[i].x * mmScale,
                                      mmY + units[i].y * mmScale);
            Draw::circle(window, mmSoldierDot);
        }
    }
    float vw = (float)WIN_W * camZoom * mmScale;
    float vh = (float)WIN_H * camZoom * mmScale;
    float vx = mmX + (camX - (float)WIN_W * 0.5 * camZoom) * mmScale;
    float vy = mmY + (camY - (float)WIN_H * 0.5 * camZoom) * mmScale;
    mmViewport.setSize(vw, vh);
    mmViewport.setPosition(vx, vy);
    Draw::rect(window, mmViewport);

    if (toastTimer > 0.0) {
        toastTxt.setString(toastMsg);
        toastTxt.setPosition((float)WIN_W * 0.5 - 35.0, 30.0);
        Draw::text(window, toastTxt);
    }

    window.display();
}

// ---- cleanup ----
toastTxt.destroy();

qLetterS.destroy();
qLetterW.destroy();
slotProgress.destroy();
slotSoldier.destroy();
slotWorker.destroy();
slotEmpty.destroy();
txtQueueLabel.destroy();

btnSoldierCost.destroy();
btnSoldierLabel.destroy();
btnWorkerCost.destroy();
btnWorkerLabel.destroy();
btnSoldierBg.destroy();
btnWorkerBg.destroy();
bmPanel.destroy();

mmViewport.destroy();
mmEnemyDot.destroy();
mmSoldierDot.destroy();
mmWorkerDot.destroy();
mmNodeWood.destroy();
mmNodeCrystal.destroy();
mmBarracksDot.destroy();
mmHallDot.destroy();
mmBg.destroy();

txtHint.destroy();
txtFps.destroy();
txtSelected.destroy();
txtSoldiers.destroy();
txtWorkers.destroy();
txtWood.destroy();
txtCrystal.destroy();
hudWoodIcon.destroy();
hudCrystalIcon.destroy();
hudPanel.destroy();
hudFont.destroy();

fogVA.destroy();

selBox.destroy();
barracksSelectionRing.destroy();
barracksShape.destroy();
hallSelectionRing.destroy();
hallShape.destroy();
hpBarFg.destroy();
hpBarBg.destroy();
enemyShape.destroy();
nodeShapeWood.destroy();
nodeShapeCrystal.destroy();
unitRing.destroy();
soldierBody.destroy();
workerBodyLoaded.destroy();
workerBody.destroy();
cam.destroy();
ground.destroy();
frame.destroy();
window.destroy();
__plugin_unload("mt/mtype_sfml.dll");
