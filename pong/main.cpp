//linker::system::subsystem  - Windows(/ SUBSYSTEM:WINDOWS)
//configuration::advanced::character set - not set
//linker::input::additional dependensies Msimg32.lib; Winmm.lib

#include "windows.h"
#include "math.h"
#include "debugapi.h"
#include <vector>
#include <string>
#include <stdlib.h>
#include <iostream>


struct {
    HWND hWnd;//хэндл окна
    HDC device_context, context;// два контекста устройства (для буферизации)
    int width, height;//сюда сохраним размеры окна которое создаст программа
} window;

struct point3d {
    float x;
    float y;
    float z;
};

int rec_depth=3;
int rec;
float rec2;

float getR(int q)
{
    int f = q;
    return (rand() % f - f / 2) / rec2 ;
}

float getR2(int q)
{
    return (rand() % q - q / 2);
}

float getR11()
{
    return ((rand()%1000)/1000.0)*2.- 1.;
}

float sign_s(float a)
{
    if (a > 0) return 1;
    if (a < 0) return -1;
    return 0;
}

void div(std::vector<point3d> &p)
{
    int q = window.width / 4.;
    rec--;
    if (rec < 1) return;
    int s = 100;
    
    for (int i = 0; i < p.size(); i+=2)
    {
        auto sz = p.size();
        auto tx = (p[i%sz].x + p[(i + 1)%sz].x) / 2.;
        auto ty = (p[i%sz].y + p[(i + 1)%sz].y) / 2.;

        auto rX = getR11()/(rec2+5);
        auto rY = getR11()/(rec2+5);

        tx += rX;
        ty += rY;

        point3d p2 = { tx,ty,0 };

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



void Line(point3d p1, point3d p2)
{
    p1.x = window.width / 2. + p1.x * window.height / 4.;
    p1.y = window.height / 2. + p1.y * window.height / 4.;
    p2.x = window.width / 2. + p2.x * window.height / 4.;
    p2.y = window.height / 2. + p2.y * window.height / 4.;

    MoveToEx(window.context, p1.x, p1.y, NULL);
    LineTo(window.context, p2.x, p2.y);
}
int seed = 0;

void clrScr()
{
    RECT rect;
    GetClientRect(window.hWnd, &rect);
    auto blackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(window.context, &rect, blackBrush);
    DeleteObject(blackBrush);
}

void notmalize2d(point3d& p)
{
    float len = sqrt(p.x * p.x + p.y * p.y);
    p.x /= len;
    p.y /= len;
}
const float PI = 3.1415926535897;

void rotateX(point3d& p, float angle)// поворот по Оси X.
{
    float a = angle * PI / 180.;

    float x1 = p.x;
    float y1 = p.y * cos(a) - p.z * sin(a);
    float z1 = p.y * sin(a) + p.z * cos(a);

    p.x = x1;
    p.y = y1;
    p.z = z1;
}

void rotateY(point3d& p, float angle)// поворот по Оси Y.
{
    float a = angle * PI / 180.;

    float x1 = p.x * cos(a) - p.z * sin(a);
    float y1 = p.y;
    float z1 = p.x * sin(a) + p.z * cos(a);

    p.x = x1;
    p.y = y1;
    p.z = z1;
}

void rotateZ(point3d& p, float angle)// поворот по Оси Z.
{
    float a = angle * PI / 180.;

    float x1 = p.x * cos(a) - p.y * sin(a);
    float y1 = p.x * sin(a) + p.y * cos(a);
    float z1 = p.z;

    p.x = x1;
    p.y = y1;
    p.z = z1;
}

void rotate2d(point3d& p, float angle)// поворот по Оси Z.
{
    float a = angle * PI / 180.;

    float x1 = p.x * cos(a) - p.y * sin(a);
    float y1 = p.x * sin(a) + p.y * cos(a);

    p.x = x1;
    p.y = y1;
}

void rotate(point3d& p1, point3d& p2)
{
    float t = timeGetTime() * .01;
    rotateZ(p1,t);
    rotateX(p1, -60);
    rotateZ(p2, t);
    rotateX(p2, -60);

}

void project(point3d& p)
{
    float camDist = 3;
    float x = p.x * camDist / (p.z + camDist);
    float y = p.y * camDist / (p.z + camDist);
    p.x = x;
    p.y = y;
}

void line3d(point3d p1, point3d p2)
{
    rotate(p1, p2);
    project(p1);
    project(p2);
    Line(p1, p2);
}

float dot(point3d p1, point3d p2)
{
    return p1.x * p2.x + p1.y * p2.y;
}

void GenerateLevel()
{

  
    std::vector<point3d> p;

    srand(seed);
    p.push_back({ -1.,-1,0.});
    p.push_back({  1.,-1,0. });
    p.push_back({  1., 1,0. });
    p.push_back({ -1., 1,0. });
    
    //rec++;
    div(p);

    clrScr();


    HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 255, 255));
    SelectObject(window.context, pen);

    for (int i = 0; i < p.size(); i++)
    {
        int j = i % p.size();
        int k = (i+1) % p.size();
        auto p1 = p[j];
        auto p2 = p[k];
        line3d(p1, p2);

    }

    std::vector<point3d> w;

    for (int i = 0; i < p.size(); i++)
    {
        int j = (i - 1) % p.size();
        int k = (i + 1) % p.size();

        auto p0 = p[i];
        auto p1 = p[j];
        auto p2 = p[k];

        auto dx1 = p1.x - p0.x;
        auto dy1 = p1.y - p0.y;
        auto dx2 = p2.x - p0.x;
        auto dy2 = p2.y - p0.y;

        point3d delta = { p2.x - p1.x, p2.y - p1.y };
        point3d deltaC = delta;
        deltaC.x /= 6;
        deltaC.y /= 6;
        notmalize2d(delta);
        rotate2d(delta, 90);

        float a = dot({ dx1,dy1 }, { dx2,dy2 });
        //std::string s = std::to_string(a);
        //TextOutA(window.context, p0.x*window.height/4+window.width/2, p0.y * window.height / 4 + window.height / 2, s.c_str(), s.length());

        float scale = .1;
        point3d p3 = { p0.x + delta.x*scale,p0.y + delta.y*scale };

        if (a < -.1)
        {

            line3d(p0, p3);
            point3d p4 = { p3.x + deltaC.x,p3.y + deltaC.y,0 };
            line3d(p3, p4);
            point3d p5 = { p3.x - deltaC.x,p3.y - deltaC.y,0 };
            line3d(p3, p5);
        }

    }

/*    for (int i = 0; i < w.size(); i++)
    {
        int j = i % w.size();
        int k = (i + 1) % w.size();

        Line(w[j], w[k]);

    }
    */


    DeleteObject(pen);



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
        BitBlt(window.device_context, 0, 0, window.width, window.height, window.context, 0, 0, SRCCOPY);//копируем буфер в окно
        Sleep(16);//ждем 16 милисекунд (1/количество кадров в секунду)
        if (GetAsyncKeyState(VK_RETURN))
        {
            seed++;
        }
    }
}