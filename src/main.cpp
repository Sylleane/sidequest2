/**
 * @file main.cpp
 * @brief Point d'entrée principal de Kitty Chat
 * 
 * Ce fichier contient la boucle principale de l'application et initialise
 * l'interface graphique Dear ImGui avec le backend Win32/DirectX11.
 * 
 * Auteur: Étudiant en Master Cybersécurité
 * Date: Janvier 2026
 */

#include <windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <memory>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#include "chat_window.h"
#include "matrix_client.h"
#include "texture_manager.h"

// Déclaration du gestionnaire de messages Windows pour ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Variables globales pour DirectX11
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

// Prototypes des fonctions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

/**
 * @brief Point d'entrée Windows
 * 
 * Initialise la fenêtre, DirectX11, ImGui et lance la boucle principale.
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Ignorer les paramètres non utilisés
    (void)hPrevInstance;
    (void)lpCmdLine;

    // Création de la classe de fenêtre Windows
    WNDCLASSEXW wc = {
        sizeof(wc),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        hInstance,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"KittyChatClass",
        nullptr
    };
    RegisterClassExW(&wc);

    // Création de la fenêtre principale
    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"🐱 Kitty Chat - Client Matrix",
        WS_OVERLAPPEDWINDOW,
        100, 100,           // Position
        1280, 720,          // Taille
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    // Initialisation de DirectX11
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Affichage de la fenêtre
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Initialisation du contexte ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Configuration d'ImGui
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Navigation clavier
    io.IniFilename = nullptr;                                // Pas de fichier de configuration

    // Configuration du style visuel - thème moderne violet/rose
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 15.0f;
    style.FrameRounding = 10.0f;
    style.GrabRounding = 8.0f;
    style.ScrollbarRounding = 10.0f;
    style.TabRounding = 8.0f;
    style.ChildRounding = 12.0f;
    style.PopupRounding = 12.0f;
    style.WindowPadding = ImVec2(15, 15);
    style.FramePadding = ImVec2(12, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;

    // Couleurs modernes - palette violet/rose/doré
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.08f, 0.06f, 0.12f, 1.00f);   // Violet très foncé
    colors[ImGuiCol_ChildBg]            = ImVec4(0.10f, 0.08f, 0.15f, 0.90f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.12f, 0.10f, 0.18f, 0.98f);
    colors[ImGuiCol_Border]             = ImVec4(0.50f, 0.35f, 0.55f, 0.40f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.22f, 0.18f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.28f, 0.22f, 0.38f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.12f, 0.10f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.18f, 0.14f, 0.25f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.12f, 0.10f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.08f, 0.06f, 0.12f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.40f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.60f, 0.50f, 0.70f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(1.00f, 0.70f, 0.40f, 1.00f);   // Doré
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.90f, 0.60f, 0.35f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(1.00f, 0.70f, 0.45f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.45f, 0.30f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.55f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.65f, 0.50f, 0.70f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.35f, 0.25f, 0.45f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.45f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.50f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_Tab]                = ImVec4(0.25f, 0.18f, 0.32f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.45f, 0.35f, 0.55f, 1.00f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.38f, 0.28f, 0.48f, 1.00f);
    colors[ImGuiCol_Text]               = ImVec4(0.95f, 0.92f, 0.98f, 1.00f);   // Blanc légèrement rose
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.55f, 0.50f, 0.60f, 1.00f);
    colors[ImGuiCol_PlotLines]          = ImVec4(1.00f, 0.70f, 0.40f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]   = ImVec4(1.00f, 0.80f, 0.50f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]     = ImVec4(0.50f, 0.35f, 0.60f, 0.50f);
    colors[ImGuiCol_ModalWindowDimBg]   = ImVec4(0.05f, 0.03f, 0.08f, 0.70f);

    // Initialisation des backends ImGui
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Création des composants de l'application
    auto matrixClient = std::make_unique<MatrixClient>();
    auto textureManager = std::make_unique<TextureManager>(g_pd3dDevice);
    auto chatWindow = std::make_unique<ChatWindow>(matrixClient.get(), textureManager.get());

    // Couleur de fond de la fenêtre - violet profond
    ImVec4 clearColor = ImVec4(0.05f, 0.03f, 0.08f, 1.00f);

    // Boucle principale de l'application
    bool running = true;
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (running)
    {
        // Traitement des messages Windows
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                running = false;
            }
        }

        if (!running)
            break;

        // Début d'une nouvelle frame ImGui
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Rendu de l'interface de chat
        chatWindow->Render();

        // Fin de la frame et rendu
        ImGui::Render();
        const float clearColorArray[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColorArray);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Présentation du buffer (VSync activé)
        g_pSwapChain->Present(1, 0);
    }

    // Nettoyage des ressources
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

/**
 * @brief Crée le device DirectX11 et la swap chain
 * @param hWnd Handle de la fenêtre
 * @return true si succès, false sinon
 */
bool CreateDeviceD3D(HWND hWnd)
{
    // Configuration de la swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Création du device avec le feature level approprié
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );

    if (res == DXGI_ERROR_UNSUPPORTED)
    {
        // Fallback sur le driver WARP si le matériel n'est pas supporté
        res = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            createDeviceFlags,
            featureLevelArray,
            2,
            D3D11_SDK_VERSION,
            &sd,
            &g_pSwapChain,
            &g_pd3dDevice,
            &featureLevel,
            &g_pd3dDeviceContext
        );
    }

    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

/**
 * @brief Nettoie les ressources DirectX11
 */
void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

/**
 * @brief Crée la render target view
 */
void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

/**
 * @brief Nettoie la render target view
 */
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

/**
 * @brief Gestionnaire de messages Windows
 * 
 * Traite les événements de fenêtre (redimensionnement, fermeture, etc.)
 */
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Laisser ImGui traiter les événements d'abord
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        // Redimensionnement de la fenêtre
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_SYSCOMMAND:
        // Désactiver le menu système ALT
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        // Fermeture de l'application
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
