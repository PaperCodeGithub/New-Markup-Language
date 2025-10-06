#include <windows.h>
#include <iostream>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

HWND g_mainWindow = NULL;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void createControlsFromAST(const json& node, HWND parent, int depth = 0);

// Create main window
void createWindowFromAST(const json& ast) {
    // Position
    int x = ast["attrs"].value("x", "500") == "matchParent" ? CW_USEDEFAULT : std::stoi(ast["attrs"].value("x", "500"));
    int y = ast["attrs"].value("y", "500") == "matchParent" ? CW_USEDEFAULT : std::stoi(ast["attrs"].value("y", "500"));

    // Default size
    int width = 800, height = 600;

    // Title
    std::string title = "Untitled Window";
    for (auto& child : ast["children"]) {
        if (child.contains("tag") && child["tag"] == "title") {
            if (!child["children"].empty() && child["children"][0].is_string()) {
                title = child["children"][0].get<std::string>();
            }
        }
    }

    // Register
    const char g_szClassName[] = "WinMLWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = g_szClassName;

    RegisterClass(&wc);

    // Create main window
    g_mainWindow = CreateWindowEx(
        0,
        g_szClassName,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        x, y, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!g_mainWindow) {
        MessageBox(NULL, "Failed to create window", "Error", MB_ICONERROR);
        return;
    }

    // Look for <body>
    for (auto& child : ast["children"]) {
        if (child.contains("tag") && child["tag"] == "body") {
            createControlsFromAST(child, g_mainWindow, 1);
        }
    }

    ShowWindow(g_mainWindow, SW_SHOW);
    UpdateWindow(g_mainWindow);
}

// Recursive control builder
void createControlsFromAST(const json& node, HWND parent, int depth) {
    if (!node.contains("children")) return;

    for (auto& child : node["children"]) {
        if (!child.contains("tag")) continue;
        std::string tag = child["tag"];

        if (tag == "label") {
            std::string type = child["attrs"].value("type", "");

            if (type == "toolbar") {
                // Just a container (STATIC) for now
                HWND toolbar = CreateWindowEx(
                    0, "STATIC", "",
                    WS_CHILD | WS_VISIBLE | SS_LEFT,
                    0, 0, 800, 30, // simple layout
                    parent, NULL, GetModuleHandle(NULL), NULL
                );

                // Add children buttons
                createControlsFromAST(child, toolbar, depth + 1);

            } else if (type == "editText") {
                // Multiline editable textbox
                HWND edit = CreateWindowEx(
                    WS_EX_CLIENTEDGE, "EDIT", "",
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
                    0, 40, 780, 500, // fill body area
                    parent, NULL, GetModuleHandle(NULL), NULL
                );

                // Insert default text if exists
                if (!child["children"].empty()) {
                    for (auto& sub : child["children"]) {
                        if (sub.is_string()) {
                            SetWindowText(edit, sub.get<std::string>().c_str());
                        }
                    }
                }
            }else if (type == "textView") {
                // Extract text (could be direct string or inside <centerInParent>)
                std::string text = "";
                for (auto& sub : child["children"]) {
                    if (sub.is_string()) {
                        text = sub.get<std::string>();
                    } else if (sub.contains("tag") && sub["tag"] == "centerInParent") {
                        if (!sub["children"].empty() && sub["children"][0].is_string()) {
                            text = sub["children"][0].get<std::string>();
                        }
                    }
                }

                // Create a static text control centered in parent
                RECT rc;
                GetClientRect(parent, &rc);
                int width = 300;
                int height = 30;
                int x = (rc.right - width) / 2;
                int y = (rc.bottom - height) / 2;

                CreateWindowEx(
                    0, "STATIC", text.c_str(),
                    WS_CHILD | WS_VISIBLE | SS_CENTER,
                    x, y, width, height,
                    parent, NULL, GetModuleHandle(NULL), NULL
                );
            }

        }
        else if (tag == "button") {
            // Get text
            std::string text = child["children"].empty() ? "Button" : child["children"][0].get<std::string>();

            static int btnX = 10;
            HWND btn = CreateWindowEx(
                0, "BUTTON", text.c_str(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                btnX, 5, 80, 25,
                parent, NULL, GetModuleHandle(NULL), NULL
            );
            btnX += 90;
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    std::ifstream f("ast.json");
    if (!f.is_open()) {
        MessageBox(NULL, "Could not open ast.json", "Error", MB_ICONERROR);
        return 1;
    }

    json data;
    f >> data;

    if (!data.contains("tag") || data["tag"] != "window") {
        MessageBox(NULL, "Root element must be <window>", "Error", MB_ICONERROR);
        return 1;
    }

    createWindowFromAST(data);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
