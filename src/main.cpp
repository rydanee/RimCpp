#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>
#include <map>
#include <filesystem>

#define WIDTH 800
#define HEIGHT 800

static constexpr int MAP_SIZE = 250;

static int targetTickRate = 60;
static int tickRate = 60;
static int tickCounter = 0;
static float tpsTimer = 0.0f;
static float tickTime = 1.0f / targetTickRate;
static float timeAccumulated = 0.0f;

static bool redrawCanvas = false;

static std::map<std::string, Texture> textures;

bool loadTexture(const std::string &name, const std::string &filePath) {
  if (textures.find(name) != textures.end()) {
    std::cout << "[WARN] Texture with name '" << name
              << "' is already aded.\n";
    return true;
  }

  Texture2D tex = LoadTexture(filePath.c_str());

  if (tex.id == 0) {
    std::cout << "[ERROR] Failed to load texture from path: " << filePath
              << "\n";
    return false;
  }

  GenTextureMipmaps(&tex);
  //SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
  SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);

  textures[name] = tex;
  std::cout << "[INFO] Successfully loaded texture: " << name << " ("
            << filePath << ")\n";
  return true;
}

void loadAllTextures() {
    namespace fs = std::filesystem;
    std::string directoryPath = "assets/textures";
    
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cout << "[ERROR] Resources directory does not exist: " << directoryPath << "\n";
        return;
    }

    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file()) {
            fs::path filePath = entry.path();
            std::string extension = filePath.extension().string();

            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp") {
                
                std::string textureName = filePath.stem().string();
                std::string fullPath = filePath.string();

                std::cout << textureName << std::endl;
                
                loadTexture(textureName, fullPath);
            }
        }
    }
    std::cout << "[INFO] Directory batch load complete.\n";
}

const Texture2D &getTexture(const std::string &name) {
  auto it = textures.find(name);
  if (it != textures.end()) {
    return it->second;
  }

  std::cout << "[ERROR] Texture '" << name
            << "' not found in TextureManager!\n";

  static Texture2D emptyTex = {0};
  return emptyTex;
}

// MAIN DATA TYPES!!

struct Item {
  std::string type;
  
  int amount;  
  float x, y;

  bool isPerishable;
  int currentLifeTime;
  int maxLifeTime;
};

std::string floorTextures[] = {
    "grass"
};
std::string tileTextures[] = {
    "void", "grass"
};
std::string roofTextures[] = {
    "grass"
};

struct Tile {
  uint8_t floorID = 0;
  uint8_t tileID = 1;
  uint8_t roofID = 0;
  bool isWalkable;

  float movementCost = 1.0f;  
}; // only 8 BYTES WOWWWW

Vector2 findBestTile(int startX, int startY, std::string type, const std::vector<Item>& itemsList) {
    int dx[] = {0,  -1, 1, 0, 0,  -1, -1, 1, 1};
    int dy[] = {0,   0, 0, -1, 1,  -1, 1, -1, 1};

    for (int i = 0; i < 9; i++) {
        int targetX = startX + dx[i];
        int targetY = startY + dy[i];

        if (targetX < 0 || targetX >= MAP_SIZE || targetY < 0 || targetY >= MAP_SIZE) continue;

        for (const auto& item : itemsList) {
            if (item.x == targetX && item.y == targetY && item.type == type && item.amount < 75) {
                return (Vector2){(float)targetX, (float)targetY};
            }
        }
    }

    for (int radius = 0; radius <= 3; radius++) {
        for (int yOffset = -radius; yOffset <= radius; yOffset++) {
            for (int xOffset = -radius; xOffset <= radius; xOffset++) {
                int targetX = startX + xOffset;
                int targetY = startY + yOffset;

                if (targetX < 0 || targetX >= MAP_SIZE || targetY < 0 || targetY >= MAP_SIZE) continue;

                bool tileIsOccupied = false;
                for (const auto& item : itemsList) {
                    if (item.x == targetX && item.y == targetY) {
                        tileIsOccupied = true;
                        break;
                    }
                }

                if (!tileIsOccupied) {
                    return (Vector2){(float)targetX, (float)targetY};
                }
            }
        }
    }

    return (Vector2){-1.0f, -1.0f};
}

class map {
    Vector2 size;

  public:
    Tile& getTile(int x, int y) {
      int ind = y * MAP_SIZE + x;

      if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE) {
        return tiles[ind];
      }

      static Tile emt = (Tile) {0, 0, 0, false, 0.0f};
      return emt;
    }
    
    void setTile(int x, int y, Tile newTile) {
      int ind = y * MAP_SIZE + x;

      tiles[ind] = newTile;
    }

    std::vector<Item> getItems() { return items; }

    void addItem(int x, int y, int count, std::string type) {
    if (count <= 0) return;

    Vector2 target = findBestTile(x, y, type, items);
    int targetX = (int)target.x;
    int targetY = (int)target.y;

    if (targetX == -1 || targetY == -1) {
        return; 
    }

    int foundIndex = -1;
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].x == targetX && items[i].y == targetY && items[i].type == type) {
            foundIndex = (int)i;
            break;
        }
    }

    if (foundIndex != -1) {
        int spaceLeft = 75 - items[foundIndex].amount;

        if (count <= spaceLeft) {
            items[foundIndex].amount += count;
        } else {
            items[foundIndex].amount = 75;
            int remainder = count - spaceLeft;

            addItem(targetX, targetY, remainder, type);
        }
        return;
    }

    Item newItem;
    newItem.type = type;
    newItem.x = targetX;
    newItem.y = targetY;
    newItem.isPerishable = false;
    
    if (type == "lavishmeatdish") {
        newItem.isPerishable = true;
        newItem.maxLifeTime = 24 * 10 * 60;
        newItem.currentLifeTime = 0;
    }

    if (count <= 75) {
        newItem.amount = count;
        items.push_back(newItem);
    } else {
        newItem.amount = 75;
        items.push_back(newItem);
        
        addItem(targetX, targetY, count - 75, type);
    }
}  
    
  private:
    Tile tiles[MAP_SIZE*MAP_SIZE];
    std::vector<Item> items;
};

static int tileSize = 16;

static map base;

const Texture2D& getFloorTexture(int id) {
  const Texture2D& tex = getTexture(floorTextures[id]);
return tex;
}

const Texture2D &getTileTexture(int id) {
  const Texture2D &tex = getTexture(tileTextures[id]);
  return tex;
}

const Texture2D &getRoofTexture(int id) {
  const Texture2D &tex = getTexture(roofTextures[id]);
  return tex;
}

void drawGrid(Color gridColor) {
  int mapVisualSize = MAP_SIZE * tileSize;

  for (int i = 0; i <= MAP_SIZE; i++) {
    int pos = i * tileSize;

    DrawLine(pos, 0, pos, mapVisualSize, gridColor);

    DrawLine(0, pos, mapVisualSize, pos, gridColor);
  }
}

Camera2D camera;

static RenderTexture2D canvas;
static bool isCanvasReady = false;

void initCanvas() {
    canvas = LoadRenderTexture(MAP_SIZE * tileSize, MAP_SIZE * tileSize);
    isCanvasReady = true;
}

void loadToRenderTexture() {
  if (!isCanvasReady)
    return;  
  
  BeginTextureMode(canvas);
  ClearBackground(BLACK);

        for (int y = 0; y <= MAP_SIZE; y++) {
            for (int x = 0; x <= MAP_SIZE; x++) {
                const Tile tile = base.getTile(x, y);
 
                const Texture2D &floortex = getFloorTexture(tile.floorID);
                DrawTexturePro(floortex,
                               (Rectangle){0.0f, 0.0f, (float)floortex.width,
                                           (float)floortex.height},
                               (Rectangle){(float)x * tileSize,
                                           (float)y * tileSize, (float)tileSize,
                                           (float)tileSize},
                               (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

                if (tile.tileID != 0) {
                    const Texture2D &tiletex = getTileTexture(tile.tileID);
                    DrawTexturePro(
                        tiletex,
                        (Rectangle){0.0f, 0.0f, (float)tiletex.width,
                                    (float)tiletex.height},
                        (Rectangle){(float)x * tileSize, (float)y * tileSize,
                                    (float)tileSize, (float)tileSize},
                        (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
                }

                if (tile.roofID != 0) {
                  const Texture2D& rooftex = getRoofTexture(tile.roofID);
                  DrawTexturePro(rooftex,
                                 (Rectangle){0.0f, 0.0f, (float)rooftex.width,
                                             (float)rooftex.height},
                                 (Rectangle){(float)x * tileSize,
                                             (float)y * tileSize,
                                             (float)tileSize, (float)tileSize},
                                 (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
                }
            }            
        }

        for (const auto &item : base.getItems()) {
            int renderX = item.x * tileSize + (tileSize / 4);
            int renderY = item.y * tileSize + (tileSize / 4);

            int itemVisualSize = tileSize / 2;

            Texture2D tex = getTexture(item.type);

            DrawTexturePro(
                tex,
                (Rectangle){0.0f, 0.0f, (float)tex.width, (float)tex.height},
                (Rectangle){(float)renderX, (float)renderY,
                            (float)itemVisualSize, (float)itemVisualSize},
                (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
            if (item.amount != 1) {
              DrawText(TextFormat("%d", item.amount), renderX, renderY, 4,
                       RAYWHITE);
          }
        }

        EndTextureMode();        
}

void render() {
  if (!isCanvasReady)
    return;  
  
  DrawTextureRec(canvas.texture,
                 (Rectangle){0, 0, (float)canvas.texture.width,
                             (float)-canvas.texture.height},
                 (Vector2){0, 0}, WHITE);
}


Camera2D initCamera() {
    Camera2D camera = { 0 };
    camera.target = (Vector2){ 0.0f, 0.0f };      
    camera.offset = (Vector2){ WIDTH / 2.0f, HEIGHT / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    return camera;
}

void changeTPS(int tps) {
    targetTickRate = tps;
    tickTime = 1.0f / targetTickRate;
    std::cout << tps << std::endl;
    }

static float targetZoom = 1.0f;

Vector2 getTileUnderCursor(Camera2D camera) {
  Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);

  int tileX = (int)(mouseWorldPos.x / tileSize);
  int tileY = (int)(mouseWorldPos.y / tileSize);

  if (mouseWorldPos.x < 0)
    tileX--;
  if (mouseWorldPos.y < 0)
    tileY--;

  return (Vector2){(float)tileX, (float)tileY};
}

void updateCamera(Camera2D &camera, float speed) {
    float dt = GetFrameTime(); 

    float currentSpeed = speed * dt;
    
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) camera.target.x += currentSpeed * (4.1 - targetZoom);
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  camera.target.x -= currentSpeed * (4.1 - targetZoom);
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))  camera.target.y += currentSpeed * (4.1 - targetZoom);
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))    camera.target.y -= currentSpeed * (4.1 - targetZoom);

    if (IsKeyPressed(KEY_ONE))   changeTPS(60);
    if (IsKeyPressed(KEY_TWO))   changeTPS(180);
    if (IsKeyPressed(KEY_THREE)) changeTPS(360);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 targetTile = getTileUnderCursor(camera);
        
        if (targetTile.x >= 0 && targetTile.x < MAP_SIZE && targetTile.y >= 0 && targetTile.y < MAP_SIZE) {
            base.addItem(targetTile.x, targetTile.y, 15, "lavishmeatdish");
            redrawCanvas = true;
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        
        targetZoom += wheel * 0.15f;
        if (targetZoom < 0.1f) targetZoom = 0.1f;
        if (targetZoom > 3.5f) targetZoom = 3.5f;

        camera.offset = GetMousePosition();
        camera.target = mouseWorldPos;
    }

    if (camera.zoom != targetZoom) {
        Vector2 mouseWorldPosBefore = GetScreenToWorld2D(GetMousePosition(), camera);
        
        float zoomInterpolationSpeed = 10.0f; 
        camera.zoom += (targetZoom - camera.zoom) * zoomInterpolationSpeed * dt;
        
        Vector2 mouseWorldPosAfter = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.target.x += (mouseWorldPosBefore.x - mouseWorldPosAfter.x);
        camera.target.y += (mouseWorldPosBefore.y - mouseWorldPosAfter.y);
    }

    
}

void updateGameLogic() {
    
}

int main() {
  InitWindow(WIDTH, HEIGHT, "RimCpp");
  InitAudioDevice();
  
  camera = initCamera();

  loadAllTextures();
  
  for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
    int x = i % MAP_SIZE;
    int y = i / MAP_SIZE;

    base.setTile(x, y, (Tile) {0, 1, 0, true, 1.0f});
    }

  initCanvas();
  loadToRenderTexture();
  
  while (!WindowShouldClose()) {
    redrawCanvas = false;    
    
    timeAccumulated += GetFrameTime();
    tpsTimer += GetFrameTime();
    
    updateCamera(camera, 280.0f);    

    BeginDrawing();
    ClearBackground(BLUE);

    while (timeAccumulated >= tickTime) {
      updateGameLogic();
      timeAccumulated -= tickTime;

      tickCounter++;
    }

    if (tpsTimer >= 1.0f) {
      tickRate = tickCounter;
      tickCounter = 0;
      tpsTimer -= 1.0f;
    }

    if (redrawCanvas) {
      loadToRenderTexture();
    }

    BeginMode2D(camera);
    render();
    //    drawGrid((Color){255, 255, 255, 255});
    EndMode2D();

    DrawFPS(10, 10);
    DrawText(TextFormat("Current TPS: %d", tickRate), 10, 30, 18, RAYWHITE);

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
