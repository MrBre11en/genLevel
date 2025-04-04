//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "math.h"
#include "debugapi.h"
#include <vector>

// секция данных игры  
typedef struct {
    float x, y, width, height, rad, dx, dy, speed;
    HBITMAP hBitmap;//хэндл к спрайту шарика 
} sprite;

int seed = 0;

// фундаментальные настройки игры
namespace ginfo {
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

struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

struct vector2 {
    float x, y;
};

cellType map[ginfo::gridSize][ginfo::gridSize];// массив сетки уровня

HBITMAP hBack;// хэндл для фонового изображения
HBITMAP hFloor;
///////////////////////////////////////////////////////////////////////////////////////////////////////////// cекция кода

// Core функции
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

int GetRandom(int min = 1, int max = 0)
{
    if (max > min)
    {
        return rand() % min + (max - min);
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

void ShowRacketAndBall()
{
    ShowBitmap(window.context, 0, 0, window.width, window.height, hBack);//задний фон



    //ShowBitmap(window.context, racket.x - racket.width / 2., racket.y, racket.width, racket.height, racket.hBitmap);// ракетка игрока
}

// Функции-условия
///////////////////////////////////////////////////////////////////////////////////////////////////

bool HasNeightbour(int x, int y, cellType neightbour = cellType::empty) {
    if (map[x][y] == neightbour) {
        return false;
    }
    if (neightbour == cellType::empty && (x == 0 || x == ginfo::gridSize - 1 || y == 0 || y == ginfo::gridSize - 1)) {
        return true;
    }

    for (int _x = x - 1; _x <= x + 1; _x++)
    {
        for (int _y = y - 1; _y <= y + 1; _y++)
        {
            if (_x != x && _y != y)
            {
                if (map[_x][_y] == neightbour) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool isInnerCorner(int x, int y) {
    if (map[x][y] == cellType::empty) {
        return false;
    }
    if ((map[x - 1][y] != cellType::empty && map[x][y - 1] != cellType::empty && map[x - 1][y - 1] == cellType::empty) ||
        (map[x + 1][y] != cellType::empty && map[x][y + 1] != cellType::empty && map[x + 1][y + 1] == cellType::empty) ||
        (map[x + 1][y] != cellType::empty && map[x][y - 1] != cellType::empty && map[x + 1][y - 1] == cellType::empty) ||
        (map[x - 1][y] != cellType::empty && map[x][y + 1] != cellType::empty && map[x - 1][y + 1] == cellType::empty)) {
        return true;
    }
    return false;
}

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

// Паттерны генерации этажей
///////////////////////////////////////////////////////////////////////////////////////////////////

void CrossLayout()
{
    int hallwayRealSize = ceil(ginfo::gridSize * ginfo::hallwaySize);
    const int halfGridSize = ginfo::gridSize / 2;

    for (int x = 0; x < ginfo::gridSize; x++)
    {
        for (int y = 0; y < ginfo::gridSize; y++)
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
                        quarter[x - i * halfGridSize][y - j * halfGridSize] = 100;
                    }
                    else
                    {
                        quarter[x - i * halfGridSize][y - j * halfGridSize] = 0;
                    }
                }
            }

            for (int x = 0; x < halfGridSize; x++)
            {
                for (int y = 0; y < halfGridSize; y++)
                {
                    if (x == 8 && y == 13) {
                        int f = 1;
                    }
                    int possibility = quarter[x][y];
                    int rnd = GetRandom(100);
                    //if (possibility > 0)
                    //{
                        if (rnd <= 100)
                        {
                            map[x + i * halfGridSize][y + j * halfGridSize] == cellType::wall;
                        }
                    //}
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
    vector2 startOffset;
    float halfGridSize = ginfo::gridSize * ginfo::cellSize / 2;
    startOffset.x = window.width / 2 - halfGridSize;
    startOffset.y = window.height / 2 - halfGridSize;

    //cellType filledCells[ginfo::gridSize][ginfo::gridSize];
    hFloor = (HBITMAP)LoadImageA(NULL, "racket.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

    for (int x = 0; x < ginfo::gridSize; x++)
    {
        for (int y = 0; y < ginfo::gridSize; y++)
        {
            cellType cell = map[x][y];
            COLORREF color = RGB(0, 0, 0);


            ///switch()

            if (cell == cellType::floor)
            {
                color = RGB(100, 100, 100);
            }
            else if ((cell == cellType::wall))
            {
                color = RGB(255, 255, 255);
            }

            if (cell != cellType::empty) {
                ShowRect(window.context, x * ginfo::cellSize + startOffset.x, y * ginfo::cellSize + startOffset.y, ginfo::cellSize, ginfo::cellSize, color);
            }
        }
    }
}

float floorS(float a, float d)
{
    return d*((int)(a/d));
}

int pCount = 0;

int sign(float a)
{
    if (fabs(a) > 0.5)
    {
        if (a > 0) return 1; else return -1;
    }
    return 0;
}

struct point {
    int x;
    int y;
};

int rec_depth=4;
int rec;
int rec2;

float getR(int q)
{
    return (rand() % q - q / 2) / rec2;
}

float sign_s(float a)
{
    if (a > 0) return 1;
    if (a < 0) return -1;
    return 0;
}

void div(std::vector<point> &p)
{
    int q = ginfo::gridSize / 4.;
    rec--;
    if (rec < 1) return;
    int s = 100;
    
    for (int i = 0; i < p.size(); i+=2)
    {
        auto sz = p.size();
        auto tx = (p[i%sz].x + p[(i + 1)%sz].x) / 2.;
        auto ty = (p[i%sz].y + p[(i + 1)%sz].y) / 2.;

        auto rX = getR(q);
        auto rY = getR(q);

        rX = sign_s(tx - ginfo::gridSize/2) * abs(rX);
        rY = sign_s(ty - ginfo::gridSize / 2) * abs(rY);

        tx += rX;
        ty += rY;



        point p2 = { tx,ty };
        if (i >= p.size()-1)
        {
            p.push_back(p2);
        }
        else
        {
            p.insert(p.begin() + i + 1, p2);
        }
    }
    rec2 *= 2;

    div(p);
}


void GenerateLevel()
{
    int cornerSize = floor(ginfo::gridSize * ginfo::cornerOffset);
    int oppositeSize = ginfo::gridSize - cornerSize;
    for (int x = 0; x < ginfo::gridSize; x++)
    {
        for (int y = 0; y < ginfo::gridSize; y++)
        {
            map[x][y] = cellType::empty;

            if (x == 0 || x == ginfo::gridSize - 1 || y == 0|| y == ginfo::gridSize - 1)
            {
                map[x][y] = cellType::wall;
            }
        }
    }

    //while (!GetAsyncKeyState(VK_RETURN))
    {
     //  Sleep(16);
    }
    
    std::vector<point> p;
    int q = ginfo::gridSize/4.;
    p.push_back({ q, q });
    p.push_back({ q*3, q });
    p.push_back({ q*3, q*3 });
    p.push_back({ q, q*3 });
    //srand(timeGetTime()*.001);
    srand(seed++);
    //rec++;
    div(p);


    for (int i = 0; i < p.size(); i++)
    {
        int j = i % p.size();
        int k = (i+1) % p.size();
        int x = p[j].x;
        int y = p[j].y;
        int x1 = p[k].x;
        int y1 = p[k].y;

        for (int tx = min(x,x1); tx <= max(x,x1); tx++)
        {
            map[tx][y] = cellType::wall;
        }
        for (int ty = min(y,y1); ty < max(y,y1); ty++)
        {
            map[x1][ty] = cellType::wall;
        }

        //map[x][y] = cellType::wall;

       /* for (int tx = x; tx < x1; tx++)
        {
            map[tx][y1] = cellType::wall;
        }
        for (int ty = y; ty < y1; ty++)
        {
            map[x][ty] = cellType::wall;
        }*/

    }

    /*pCount++;
    int pM = 1000;
    float dirX = 1;
    float dirY = 0;
    bool dir = false;
    srand(0);
    {
        int len = 5;
        int x = ginfo::gridSize /5;
        int y = ginfo::gridSize - ginfo::gridSize/5;

            for (int i = 0; i < pCount; i++)
            {
                int j = i;
                if (i > len)
                {
                    len = i + rand() % 7+3;
                    float jt = (rand() % 4-2)/2.;
                    dirX = sin(j / (float)pCount * 3.14 * 2.+jt);
                    dirY = cos(j / (float)pCount * 3.14 * 2.+jt);

                    if (fabs(dirX) > fabs(dirY))
                    {
                        dirY = 0;
                        if (x < ginfo::gridSize / 8) dirX = 1;
                        if (x > ginfo::gridSize - ginfo::gridSize / 8) dirX = -1;
                    }
                    else
                    {
                        dirX = 0;
                        if (y < ginfo::gridSize / 8) dirY = 1;
                        if (y > ginfo::gridSize - ginfo::gridSize / 8) dirY = -1;


                    }
                    dirX = sign(dirX);
                    dirY = sign(dirY);



                }

                map[(int)x][(int)y] = cellType::wall;

                x += dirX;
                y += dirY;
            }
    }

    */


    float f = ginfo::gridSize/16;
    for (int y = 0; y < ginfo::gridSize; y++)
    {
        for (int x = 0; x < ginfo::gridSize; x++)
        {
            float a = sin(f * x / (float)ginfo::gridSize);
            float b = sin(f * y / (float)ginfo::gridSize);
            float factor = 1-3*max(a , b);
          //  if (factor>0) map[x][y] = cellType::wall;
        }
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


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    //srand(timeGetTime());
    srand(0);

    InitWindow();//здесь инициализируем все что нужно для рисования в окне
    InitGame();//здесь инициализируем переменные игры

    
    while (!GetAsyncKeyState(VK_ESCAPE))
    {
        rec2 = 1;
        rec = rec_depth;
        GenerateLevel();
        ShowRacketAndBall();//рисуем фон, ракетку и шарик
        BuildLevel();
        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)
        while (!GetAsyncKeyState(VK_RETURN))
        {
            Sleep(60);
        }
    }
}