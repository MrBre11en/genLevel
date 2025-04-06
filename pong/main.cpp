//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "math.h"
#include "debugapi.h"
#include <vector>

void ShowAll();

// секция данных игры  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
} sprite;

// фундаментальные настройки игры
namespace ginfo {
    int cornerCount = 1000;

    const int gridSize = 256;
    float cornerOffset = 0.125f;
    int cellSize = 4;
    int outerWallSize = 8;

    float hallwaySize = 0.1f;
}

// библиотека типов клеток
enum class cellType {
    empty,
    floor,
    wall,
    door,
    lift,

    reserved,
};

struct node_vector3 {
    vector3 *value;
    node_vector3* next;
    node_vector3* prev;

public:
    node_vector3(vector3 data) {
        value = &data;
        prev = next = NULL;
    }
};

struct list_vector3 {
    node_vector3* first;
    node_vector3* last;

    node_vector3* push_front(vector3 data) {
        node_vector3* node = new node_vector3(data);

        node->next = first;
        if (first != NULL)
            first->prev = node;
        if (last == NULL)
            last = node;
        first = node;

        return node;
    }

    node_vector3* push_back(double data) {
        node_vector3* node = new node_vector3(data);

        node->prev = last;
        if (last != NULL)
            last->next = node;
        if (first == NULL)
            first = node;
        last = node;

        return node;
    }

    void pop_front() {
        if (first == NULL) return;

        node_vector3* node = first->next;
        if (node != NULL)
            node->prev = NULL;
        else
            last = NULL;

        delete first;
        first = node;
    }


    void pop_back() {
        if (last == NULL) return;

        node_vector3* node = last->prev;
        if (node != NULL)
            node->next = NULL;
        else
            first = NULL;

        delete last;
        last = node;
    }
};

struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

class vector3 {
public:
    float x = 0;
    float y = 0;
    float z = 0;

    float magnitude()
    {
        return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    }

    vector3 unit()
    {
        float mag = magnitude();
        return vector3(x / mag, y / mag, z / mag);
    }

    //

    vector3 operator + (const vector3& vector) const
    {
        return vector3{ x + vector.x, y + vector.y, z + vector.z };
    }

    vector3 operator - (const vector3& vector) const
    {
        return vector3{ x - vector.x, y - vector.y, z - vector.z };
    }

    //

    vector3(float X = 0, float Y = 0, float Z = 0)
    {
        x = X;
        y = Y;
        z = Z;
    }

};

struct room {
    std::vector<vector3> corners;
};

HBITMAP hBack;// хэндл для фонового изображения
HBITMAP hFloor;

const float PI = acos(-1.0);
///////////////////////////////////////////////////////////////////////////////////////////////////////////// cекция кода

// Core функции
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetRandom(int min = 1, int max = 0)
{
    if (max > min)
    {
        return rand() % (max - min) + min;
    }
    else
    {
        return rand() % min;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShowBitmap(HDC hDC, int x, int y, int x1, int y1, HBITMAP hBitmapBall, bool alpha = false)
{
    HBITMAP hbm, hOldbm;
    HDC hMemDC;
    BITMAP bm;

    hMemDC = CreateCompatibleDC(hDC); // Создаем контекст памяти, совместимый с контекстом отображения
    hOldbm = (HBITMAP)SelectObject(hMemDC, hBitmapBall);// Выбираем изображение bitmap в контекст памяти

    if (hOldbm) // Если не было ошибок, продолжаем работу
    {
        GetObject(hBitmapBall, sizeof(BITMAP), (LPSTR)&bm); // Определяем размеры изображения

        if (alpha)
        {
            TransparentBlt(window.context, x, y, x1, y1, hMemDC, 0, 0, x1, y1, RGB(0, 0, 0));//все пиксели черного цвета будут интепретированы как прозрачные
        }
        else
        {
            StretchBlt(hDC, x, y, x1, y1, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY); // Рисуем изображение bitmap
        }

        SelectObject(hMemDC, hOldbm);// Восстанавливаем контекст памяти
    }

    DeleteDC(hMemDC); // Удаляем контекст памяти
}

//Выводит квадратные спрайты на экран
HBRUSH hbrush;
void ShowRect(HDC hDC, float left, float up, float sizeX, float sizeY, COLORREF color = RGB(255, 255, 255))
{
    hbrush = CreateSolidBrush(color);
    RECT rect;

    SelectObject(window.device_context, (HGDIOBJ)hbrush);
    SetRect(&rect, left, up, left + sizeX, up + sizeY);
    FillRect(hDC, &rect, hbrush);

    DeleteObject(hbrush);
}

//Рисует линию между двумя точками
void Drawline(vector3 p1, vector3 p2)
{
    HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
    SelectObject(window.context, pen);

    MoveToEx(window.context, p1.x, p1.y, NULL);
    LineTo(window.context, p2.x, p2.y);

    DeleteObject(pen);
}

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон


// Функции для чего-то там в генерации
///////////////////////////////////////////////////////////////////////////////////////////////////

void ThickenCell(int x, int y, int thickness, cellType targetType = cellType::floor, cellType newType = cellType::wall)
{
    if (map[x][y] != newType) {
        return;
    }
    int halfThickness = thickness / 2;

    for (int _x = x - halfThickness; _x <= x + halfThickness; _x++)
    {
        for (int _y = y - halfThickness; _y <= y + halfThickness; _y++)
        {
            if (_x >= 0 && _x < ginfo::gridSize && _y >= 0 && _y < ginfo::gridSize && map[_x][_y] == targetType) {
                map[_x][_y] = newType;
            }
            /*else
            {
                int new_x = x - _x;
                int new_y = y - _y;
                if (new_x >= 0 && new_x < ginfo::gridSize && new_y >= 0 && new_y < ginfo::gridSize && map[new_x][new_y] == targetType) {
                    map[new_x][new_y] = newType;
                }
            }*/
        }
    }
}

void DivideArea(int count, int area[ginfo::gridSize][ginfo::gridSize], vector2 upLeftCorner, vector2 downRightCorner)
{
    float middle = 0.5 + GetRandom(-25, 25) / 100;
    count -= 1;
    if (count % 2 == 0)
    {
        int line = downRightCorner.x - upLeftCorner.x;
        int dividation = line * middle;
    }

    if (count > 0)
    {
        DivideArea(count, area, upLeftCorner, downRightCorner);
    }
}

// Паттерны генерации этажей
///////////////////////////////////////////////////////////////////////////////////////////////////

void BuildLevel()
{
    for (int i = 0; i < ginfo::cornerCount; i++)
    {
        int next;
        if (i < ginfo::cornerCount - 1)
        {
            if ((map[x][y] == cellType::floor || map[x][y] == cellType::reserved) && HasNeightbour(x, y, cellType::wall)) {
                map[x][y] = cellType::reserved;
                ThickenCell(x, y, hallwayRealSize, cellType::floor, cellType::reserved);
            }
        }
    }

    int halfHallwayRealSize = hallwayRealSize / 2;

    for (int x = halfGridSize - halfHallwayRealSize; x <= halfGridSize + halfHallwayRealSize; x++)
    {
        for (int y = 0; y < ginfo::gridSize; y++)
        {
            if (map[x][y] == cellType::floor) {
                map[x][y] = cellType::reserved;
            }
        }
    }

    for (int y = halfGridSize - halfHallwayRealSize; y <= halfGridSize + halfHallwayRealSize; y++)
    {
        for (int x = 0; x < ginfo::gridSize; x++)
        {
            if (map[x][y] == cellType::floor) {
                map[x][y] = cellType::reserved;
            }
        }
    }

    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            int quarter[halfGridSize][halfGridSize];

            for (int x = i * halfGridSize; x < i * halfGridSize + halfGridSize; x++)
            {
                for (int y = j * halfGridSize; y < j * halfGridSize + halfGridSize; y++)
                {
                    if (map[x][y] == cellType::floor) {
                        int _x = x - i * halfGridSize;
                        int _y = y - j * halfGridSize;
                        quarter[x - i * halfGridSize][y - j * halfGridSize] = 1;
                    }
                    else
                    {
                        quarter[x - i * halfGridSize][y - j * halfGridSize] = 0;
                    }
                }
            }

            std::vector<room> rooms;
            for (int x = 0; x < halfGridSize; x++)
            {
                for (int y = 0; y < halfGridSize; y++)
                {
                    int possibility = quarter[x][y];
                    if (possibility > 0 && GetRandom(80000) <= possibility)
                    {
                        int realx = x + i * halfGridSize;
                        int realy = y + j * halfGridSize;
                        room _room = room();
                        vector2 vector = vector2(realx, realy);
                        for (int n = 0; n < 4; n++)
                        {
                            _room.corners.push_back(vector);
                        }

                        rooms.push_back(_room);
                        map[realx][realy] = cellType::reserved;
                    }
                }
            }

            bool freeCells = true;
            while (freeCells)
            {
                freeCells = false;
                for (int i = 0; i < rooms.size(); i++)
                {
                    room* _room = &rooms[i];
                    int size = _room->corners.size();

                    int theBiggestWall = 0;
                    vector2* selected_point1 = &_room->corners[0];
                    vector2* selected_point2 = &_room->corners[0];
                    int point2_index = 0;
                    for (int n = 0; n < size; n++)
                    {
                        vector2* point1 = &_room->corners[n];
                        vector2* point2;
                        if (n < size - 1)
                        {
                            point2 = &_room->corners[n + 1];
                        }
                        else
                        {
                            point2 = &_room->corners[0];
                        }

                        int wallSize = abs((point2->x - point1->x) + (point2->y - point1->y));
                        if (wallSize >= theBiggestWall)
                        {
                            theBiggestWall = wallSize;
                            selected_point1 = point1;
                            selected_point2 = point2;
                            point2_index = n;
                        }
                    }

                    vector2 wallVector = vector2(selected_point2->x - selected_point1->x, selected_point2->y - selected_point1->y);
                    vector2 normal = vector2(-wallVector.y, wallVector.x).unit();

                    if (wallVector.x > 0)
                    {
                        int y = selected_point1->y;
                        for (int x = selected_point1->x; x <= selected_point2->x; x++)
                        {
                            if (map[x][y] == cellType::floor) {
                                //vector2 newvector = vector2(_x, _y);
                                //_room->cells.push_back(newvector);

                                map[x][y] = cellType::reserved;
                                freeCells = true;
                            }
                            else
                            {
                                selected_point2->x = x - 1;

                                cellType cell = map[x][y];
                                
                                bool reservedCell = true;
                                for (int _x = x; _x <= selected_point2->x; _x++)
                                {
                                    if (cell == cellType::floor || not reservedCell)
                                    {
                                        reservedCell = not reservedCell;
                                        vector2 new_corner = vector2(_x, y);
                                        _room->corners.insert(_room->corners.begin() + point2_index + 1, new_corner);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void (*levelPatterns[])() = { CrossLayout };

///////////////////////////////////////////////////////////////////////////////////////////////////

void BuildLevel()
{
    float halfGridSize = ginfo::gridSize * ginfo::cellSize / 2;
    vector3 startOffset = vector3(window.width / 2, window.height / 2);

    int pointNum = ginfo::cornerCount;
    float k = PI * 2 / pointNum;
    for (int i = 0; i < pointNum; i++)
    {
        float j = k * i;
        vector3 point = startOffset + vector3(cos(j) * halfGridSize, sin(j) * halfGridSize);
        outerPoints[i] = point;
    }
}

void InitWindow()
{
    SetProcessDPIAware();
    window.hWnd = CreateWindow("edit", 0, WS_POPUP | WS_VISIBLE | WS_MAXIMIZE, 0, 0, 0, 0, 0, 0, 0, 0);

    RECT r;
    GetClientRect(window.hWnd, &r);
    window.device_context = GetDC(window.hWnd);//из хэндла окна достаем хэндл контекста устройства для рисования
    window.width = r.right - r.left;//определяем размеры и сохраняем
    window.height = r.bottom - r.top;
    window.context = CreateCompatibleDC(window.device_context);//второй буфер
    SelectObject(window.context, CreateCompatibleBitmap(window.device_context, window.width, window.height));//привязываем окно к контексту
    GetClientRect(window.hWnd, &r);
}

void InitGame()
{
    //в этой секции загружаем спрайты с помощью функций gdi
    //пути относительные - файлы должны лежать рядом с .exe 
    //результат работы LoadImageA сохраняет в хэндлах битмапов, рисование спрайтов будет произовдиться с помощью этих хэндлов
    //racket.hBitmap = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    hBack = (HBITMAP)LoadImageA(NULL, "back.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    //------------------------------------------------------

    //racket.x = window.width / 2.;//ракетка посередине окна
    //racket.y = window.height - racket.height;//чуть выше низа экрана - на высоту ракетки
    GenerateLevel();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ShowAll()
{
    BuildLevel();
    BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
    Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    srand(timeGetTime());
    //srand(0);

    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры


    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        ShowRacketAndBall();//рисуем фон, ракетку и шарик
        ShowAll();
    }
}