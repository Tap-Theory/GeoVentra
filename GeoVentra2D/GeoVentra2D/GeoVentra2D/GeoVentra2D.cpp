// ========================================================
// GeoVentra2D.cpp
// Pure Win32 + GDI - White Canvas + Ghost Preview + Menu Bar
// ========================================================

#include "framework.h"
#include "GeoVentra2D.h"
#include <vector>
#include <windowsx.h>
#include <cmath>

#define MAX_LOADSTRING 100

// ====================== GLOBAL VARIABLES ======================

HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];

enum class ShapeType { Rectangle, Ellipse, Triangle, Hexagon };

struct PlacedShape {
    ShapeType type;
    RECT rect;
    POINT points[6];
    int pointCount = 0;
    COLORREF color;
    bool isDragging = false;
    POINT dragOffset = { 0, 0 };
};

std::vector<PlacedShape> placedShapes;
ShapeType selectedPaletteType = ShapeType::Rectangle;
bool isDraggingFromPalette = false;
POINT dragPreviewPos = { 0, 0 };

// ====================== FUNCTION DECLARATIONS ======================
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GEOVENTRA2D, szWindowClass, MAX_LOADSTRING);

    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GEOVENTRA2D));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

// ====================== REGISTER WINDOW CLASS (with menu) ======================
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GEOVENTRA2D));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GEOVENTRA2D);   // ← This restores the menu bar
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 1200, 800, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

// ====================== MOVE SHAPE ======================
void MoveShape(PlacedShape& shape, int dx, int dy)
{
    OffsetRect(&shape.rect, dx, dy);
    for (int i = 0; i < shape.pointCount; ++i)
    {
        shape.points[i].x += dx;
        shape.points[i].y += dy;
    }
}

// ====================== DRAW SHAPE (with ghost support) ======================
void DrawShape(HDC hdc, const PlacedShape& shape, bool isGhost = false)
{
    HPEN hPen = CreatePen(PS_SOLID, isGhost ? 2 : 3, RGB(30, 30, 30));
    HBRUSH hBrush = CreateSolidBrush(shape.color);

    auto oldPen = SelectObject(hdc, hPen);
    auto oldBrush = SelectObject(hdc, hBrush);

    switch (shape.type)
    {
    case ShapeType::Rectangle:
        Rectangle(hdc, shape.rect.left, shape.rect.top, shape.rect.right, shape.rect.bottom);
        break;
    case ShapeType::Ellipse:
        Ellipse(hdc, shape.rect.left, shape.rect.top, shape.rect.right, shape.rect.bottom);
        break;
    case ShapeType::Triangle:
    case ShapeType::Hexagon:
        Polygon(hdc, shape.points, shape.pointCount);
        break;
    }

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

// ====================== PAINT FUNCTION ======================
void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);
    RECT client;
    GetClientRect(hWnd, &client);

    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, client.right, client.bottom);
    auto oldBmp = SelectObject(memDC, memBmp);

    FillRect(memDC, &client, (HBRUSH)(COLOR_WINDOW + 1));

    // Light Palette
    RECT sidebar{ 0, 0, 220, client.bottom };
    FillRect(memDC, &sidebar, CreateSolidBrush(RGB(240, 240, 245)));

    SetTextColor(memDC, RGB(0, 0, 0));
    SetBkMode(memDC, TRANSPARENT);
    TextOutW(memDC, 35, 25, L"SHAPE PALETTE", 13);

    int py = 90;

    RECT r{ 40, py, 160, py + 65 };
    Rectangle(memDC, r.left, r.top, r.right, r.bottom);
    TextOutW(memDC, 45, py + 75, L"Rectangle", 9);
    py += 130;

    Ellipse(memDC, 50, py, 150, py + 65);
    TextOutW(memDC, 55, py + 75, L"Ellipse", 7);
    py += 130;

    POINT tri[3] = { {95, py}, {155, py + 75}, {35, py + 75} };
    Polygon(memDC, tri, 3);
    TextOutW(memDC, 50, py + 90, L"Triangle", 8);
    py += 140;

    POINT hex[6];
    for (int i = 0; i < 6; ++i) {
        float a = i * 3.1415926535f * 2 / 6;
        hex[i].x = 95 + (int)(38 * cos(a));
        hex[i].y = py + 38 + (int)(38 * sin(a));
    }
    Polygon(memDC, hex, 6);
    TextOutW(memDC, 55, py + 85, L"Hexagon", 7);

    // White Canvas
    RECT canvasRect{ 220, 0, client.right, client.bottom };
    FillRect(memDC, &canvasRect, CreateSolidBrush(RGB(255, 255, 255)));

    // Grid
    HPEN gridPen = CreatePen(PS_DOT, 1, RGB(220, 220, 220));
    auto oldPen = SelectObject(memDC, gridPen);
    for (int x = 220; x < client.right; x += 40) {
        MoveToEx(memDC, x, 0, NULL); LineTo(memDC, x, client.bottom);
    }
    for (int y = 0; y < client.bottom; y += 40) {
        MoveToEx(memDC, 220, y, NULL); LineTo(memDC, client.right, y);
    }
    SelectObject(memDC, oldPen);
    DeleteObject(gridPen);

    // Draw placed shapes
    for (const auto& s : placedShapes)
        DrawShape(memDC, s);

    // ==================== GHOST PREVIEW ====================
    if (isDraggingFromPalette && dragPreviewPos.x > 220)
    {
        PlacedShape ghost{};
        ghost.type = selectedPaletteType;
        ghost.color = RGB(180, 180, 200);   // Light gray-blue ghost

        int w = 100, h = 80;
        ghost.rect = { dragPreviewPos.x - w / 2, dragPreviewPos.y - h / 2,
                      dragPreviewPos.x + w / 2, dragPreviewPos.y + h / 2 };

        if (ghost.type == ShapeType::Triangle)
        {
            ghost.points[0] = { dragPreviewPos.x,     dragPreviewPos.y - 40 };
            ghost.points[1] = { dragPreviewPos.x + 50, dragPreviewPos.y + 35 };
            ghost.points[2] = { dragPreviewPos.x - 50, dragPreviewPos.y + 35 };
            ghost.pointCount = 3;
        }
        else if (ghost.type == ShapeType::Hexagon)
        {
            for (int i = 0; i < 6; ++i) {
                float a = i * 3.1415926535f * 2 / 6;
                ghost.points[i].x = dragPreviewPos.x + (int)(42 * cos(a));
                ghost.points[i].y = dragPreviewPos.y + (int)(42 * sin(a));
            }
            ghost.pointCount = 6;
        }

        DrawShape(memDC, ghost, true);
    }

    BitBlt(hdc, 0, 0, client.right, client.bottom, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);

    EndPaint(hWnd, &ps);
}

// ====================== WINDOW PROCEDURE ======================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;

    case WM_PAINT:
        OnPaint(hWnd);
        break;

    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (x < 220)
        {
            if (y > 80 && y < 200)      selectedPaletteType = ShapeType::Rectangle;
            else if (y > 210 && y < 330) selectedPaletteType = ShapeType::Ellipse;
            else if (y > 340 && y < 470) selectedPaletteType = ShapeType::Triangle;
            else if (y > 480)            selectedPaletteType = ShapeType::Hexagon;

            isDraggingFromPalette = true;
        }
        else
        {
            for (auto& shape : placedShapes)
            {
                if (PtInRect(&shape.rect, { x, y }))
                {
                    shape.isDragging = true;
                    shape.dragOffset.x = x - shape.rect.left;
                    shape.dragOffset.y = y - shape.rect.top;
                    break;
                }
            }
        }
        InvalidateRect(hWnd, NULL, FALSE);
    }
    break;

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        dragPreviewPos.x = x;
        dragPreviewPos.y = y;

        bool needsRedraw = false;

        for (auto& shape : placedShapes)
        {
            if (shape.isDragging)
            {
                int dx = x - (shape.rect.left + shape.dragOffset.x);
                int dy = y - (shape.rect.top + shape.dragOffset.y);
                MoveShape(shape, dx, dy);
                needsRedraw = true;
            }
        }

        if (isDraggingFromPalette || needsRedraw)
            InvalidateRect(hWnd, NULL, FALSE);
    }
    break;

    case WM_LBUTTONUP:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        if (isDraggingFromPalette && x > 220)
        {
            PlacedShape ns{};
            ns.type = selectedPaletteType;

            int randomColor = rand() % 4;
            if (randomColor == 0)      ns.color = RGB(255, 60, 60);
            else if (randomColor == 1) ns.color = RGB(60, 255, 60);
            else if (randomColor == 2) ns.color = RGB(60, 100, 255);
            else                       ns.color = RGB(255, 230, 50);

            int w = 100, h = 80;
            ns.rect = { x - w / 2, y - h / 2, x + w / 2, y + h / 2 };

            if (ns.type == ShapeType::Triangle)
            {
                ns.points[0] = { x, y - 40 };
                ns.points[1] = { x + 50, y + 35 };
                ns.points[2] = { x - 50, y + 35 };
                ns.pointCount = 3;
            }
            else if (ns.type == ShapeType::Hexagon)
            {
                for (int i = 0; i < 6; ++i) {
                    float a = i * 3.1415926535f * 2 / 6;
                    ns.points[i].x = x + (int)(42 * cos(a));
                    ns.points[i].y = y + (int)(42 * sin(a));
                }
                ns.pointCount = 6;
            }

            placedShapes.push_back(ns);
        }

        for (auto& shape : placedShapes)
            shape.isDragging = false;

        isDraggingFromPalette = false;
        InvalidateRect(hWnd, NULL, FALSE);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}