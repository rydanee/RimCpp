#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

#define WIDTH 800
#define HEIGHT 800

static constexpr int MAP_SIZE = 50;

static int targetTickRate = 60;
static int tickRate = 60;
static int tickCounter = 0;
static float tpsTimer = 0.0f;
static float tickTime = 1.0f / targetTickRate;
static float timeAccumulated = 0.0f;

enum class ItemType { WOOD, STEEL, LAVISHMEATDISH };

struct Item {
  ItemType type;
  int amount;  
  float x, y;

  bool isPerishable;
  int currentLifeTime;
  int maxLifeTime;  
};

Vector2 findBestTile(int startX, int startY, ItemType type, const std::vector<Item>& itemsList) {
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
    int getTile(int x, int y) {
      int ind = y * MAP_SIZE + x;

      if (tiles[ind] != -1) {
        return tiles[ind];
      }
      return 0;
    }
    
    void setTile(int x, int y, int type) {
      int ind = y * MAP_SIZE + x;

      tiles[ind] = type;
    }

    std::vector<Item> getItems() { return items; }

 void addItem(int x, int y, int count, ItemType type) {
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
    
    if (type == ItemType::LAVISHMEATDISH) {
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
    int tiles[MAP_SIZE*MAP_SIZE];
    std::vector<Item> items;
};

static int tileSize = 16;

static Color tileColors[] = {
    {0, 0, 0, 255}, {0, 175, 0, 255}, {175, 0, 0, 255}};

static Color itemColors[] = {
    BROWN, // WOOD
    GRAY,   // STEEL
    MAROON // LAVISHMEATDISH
};

static map base;

void drawGrid(Color gridColor) {
  int mapVisualSize = MAP_SIZE * tileSize;

  for (int i = 0; i <= MAP_SIZE; i++) {
    int pos = i * tileSize;

    DrawLine(pos, 0, pos, mapVisualSize, gridColor);

    DrawLine(0, pos, mapVisualSize, pos, gridColor);
  }
}

void render() {
  for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
    int x = i % MAP_SIZE;
    int y = i / MAP_SIZE;

    DrawRectangle(x * tileSize, y * tileSize, tileSize, tileSize,
                  tileColors[base.getTile(x, y)]);
  }

  for (const auto &item : base.getItems()) {
    int renderX = item.x * tileSize + (tileSize / 4);
    int renderY = item.y * tileSize + (tileSize / 4);

    int itemVisualSize = tileSize / 2;

    Color color = itemColors[static_cast<int>(item.type)];

    DrawRectangle(renderX, renderY, itemVisualSize, itemVisualSize, color);
    if (item.amount != 1) {
        DrawText(TextFormat("%d", item.amount), renderX, renderY, 4, RAYWHITE);
    }
  }
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
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D))
        camera.target.x += currentSpeed * (4.1 - targetZoom);
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))
        camera.target.x -= currentSpeed * (4.1 - targetZoom);
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S))
        camera.target.y += currentSpeed * (4.1 - targetZoom);
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W))
        camera.target.y -= currentSpeed * (4.1 - targetZoom);
    if (IsKeyPressed(KEY_ONE))
        changeTPS(60);
    if (IsKeyPressed(KEY_TWO))
      changeTPS(180);
    if (IsKeyPressed(KEY_THREE))
      changeTPS(360);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 targetTile = getTileUnderCursor(camera);

      if (targetTile.x >= 0 && targetTile.x < MAP_SIZE && targetTile.y >= 0 &&
          targetTile.y < MAP_SIZE) {

          base.addItem(targetTile.x, targetTile.y, 500, ItemType::LAVISHMEATDISH);
      }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        
        targetZoom += wheel * 0.15f;
        if (targetZoom < 0.1f) targetZoom = 0.1f;
        if (targetZoom > 4.0f) targetZoom = 4.0f;

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

  Camera2D camera = initCamera();
  
  for (int i = 0; i < MAP_SIZE * MAP_SIZE; i++) {
    int x = i % MAP_SIZE;
    int y = i / MAP_SIZE;

    if (x != MAP_SIZE-1) base.setTile(x, y, 1);
    else base.setTile(x, y, 0);
    }

  while (!WindowShouldClose()) {
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
    
    BeginMode2D(camera);
    render();
    drawGrid((Color){255, 255, 255, 255});
    EndMode2D();

    DrawFPS(10, 10);
    DrawText(TextFormat("Current TPS: %d", tickRate), 10, 30, 18, RAYWHITE);

    EndDrawing();
  }

  CloseWindow();

  return 0;
}
