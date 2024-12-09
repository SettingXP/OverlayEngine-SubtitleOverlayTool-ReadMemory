#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <d3d9.h>
#include <d3dx9.h>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <TlHelp32.h>
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

LPDIRECT3DDEVICE9 d3ddev = nullptr;
LPD3DXFONT font = nullptr;

uintptr_t ReadMemoryChain(HANDLE hProcess, uintptr_t baseAddress, const std::vector<uintptr_t>& offsets, std::string& output) {
    uintptr_t address = baseAddress;
    for (auto offset : offsets) {
        if (!ReadProcessMemory(hProcess, (LPCVOID)address, &address, sizeof(address), nullptr)) {
            std::cerr << "Failed to read memory at address: " << std::hex << address << std::endl;
            return 0;
        }
        address += offset;
    }
    char buffer[256];
    if (ReadProcessMemory(hProcess, (LPCVOID)address, buffer, sizeof(buffer), nullptr)) {
        output = buffer;
    }
    else {
        output.clear();
        std::cerr << "Failed to read memory at address: " << std::hex << address << std::endl;
    }
    return address;
}

DWORD GetProcessID(const wchar_t* processName) {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (!_wcsicmp(pe32.szExeFile, processName)) {
                CloseHandle(hProcessSnap);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }

    CloseHandle(hProcessSnap);
    return 0;
}

HRESULT initD3D(HWND hwnd) {
    LPDIRECT3D9 d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (d3d == nullptr) return E_FAIL;

    D3DPRESENT_PARAMETERS d3dpp = {};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    d3ddev = nullptr;
    if (FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev))) {
        d3d->Release();
        return E_FAIL;
    }

    // ใช้ฟอนต์ขนาดเล็กลง (20px -> 16px)
    if (FAILED(D3DXCreateFont(d3ddev, 40, 0, FW_NORMAL, 1, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Arial", &font))) {
        return E_FAIL;
    }

    return S_OK;
}

void cleanD3D() {
    if (font != nullptr) {
        font->Release();
        font = nullptr;
    }
    if (d3ddev != nullptr) {
        d3ddev->Release();
        d3ddev = nullptr;
    }
}

void renderText(const std::string& text) {
    if (font == nullptr) return;


    int windowWidth = GetSystemMetrics(SM_CXSCREEN);
    int windowHeight = GetSystemMetrics(SM_CYSCREEN);

    int offsetFromBottom = 100;


    RECT rect = {
        0,
        windowHeight - offsetFromBottom - 40,  
        windowWidth,
        windowHeight
    };

    // Clear the screen to black to ensure the previous frames do not overlap
    d3ddev->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    // Begin rendering
    d3ddev->BeginScene();

    // Draw text to the overlay (ตำแหน่งจะถูกจัดกลาง)
    font->DrawTextA(nullptr, text.c_str(), -1, &rect, DT_CENTER | DT_NOCLIP, D3DCOLOR_XRGB(255, 255, 255));

    // End rendering
    d3ddev->EndScene();

    // Swap buffers to display the rendered frame
    d3ddev->Present(nullptr, nullptr, nullptr, nullptr);
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        cleanD3D();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

bool containsInvalidCharacters(const std::string& text) {

    return text.find('�') != std::string::npos || text.find('\x7F') != std::string::npos;
}


bool isValidText(const std::string& text) {
  
    return !containsInvalidCharacters(text);
}

std::string toLowerCase(const std::string& word) {
    std::string lowerWord = word;
    std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
    return lowerWord;
}


int main() {

    const wchar_t* processName = L"Game.exe";
    DWORD processID = GetProcessID(processName);

    if (processID == 0) {
        std::cerr << "Process process not found!" << std::endl;
        return 1;
    }

    std::cout << "Found Process process with PID: " << processID << std::endl;

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, processID);
    if (!hProcess) {
        std::cerr << "Failed to open process!" << std::endl;
        return 1;
    }
    //Address Offset
    uintptr_t baseAddress = 0x7FF668D4C118;
    std::vector<uintptr_t> offsets = { 0x48, 0x98, 0x130, 0x0, 0x28, 0x18, 0x960 };


    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"OverlayWindow";
    RegisterClass(&wc);

 
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    std::cout << "Screen Width: " << screenWidth << ", Screen Height: " << screenHeight << std::endl;  // แสดงขนาดหน้าจอ

    int windowWidth = GetSystemMetrics(SM_CXSCREEN);
    int windowHeight = GetSystemMetrics(SM_CYSCREEN);

  
    int posX = (screenWidth - windowWidth) / 2;
    int posY = (screenHeight - windowHeight) / 2;

    HWND hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName, L"Overlay", WS_POPUP,
        posX, posY, windowWidth, windowHeight, NULL, NULL, wc.hInstance, NULL);

    SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    // Initialize Direct3D
    if (FAILED(initD3D(hWnd))) {
        std::cerr << "Direct3D initialization failed!" << std::endl;
        return 1;
    }

    std::string output;
    MSG msg = { 0 };

    while (true) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

 
        if (ReadMemoryChain(hProcess, baseAddress, offsets, output)) {
            if (isValidText(output)) {

           
                renderText(output); 
            }
            else {
                //std::cerr << "Invalid characters found in text!" << std::endl;
            }
        }
        else {
            std::cerr << "Failed to read memory!" << std::endl;
        }

        Sleep(1000);
    }

    cleanD3D();
    CloseHandle(hProcess);

    return 0;
}
