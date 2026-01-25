# 📚 Rapport Technique - Partie 2 : Application Cliente

---

# 3. Partie Frontend - Application Cliente C++

## 3.1 Architecture de l'Application

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           KITTY CHAT - ARCHITECTURE                         │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│                              main.cpp                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                    Initialisation Windows                            │    │
│  │  - Création de la fenêtre Win32 (CreateWindowEx)                    │    │
│  │  - Initialisation DirectX 11 (Device, SwapChain, RenderTarget)      │    │
│  │  - Initialisation Dear ImGui                                         │    │
│  │  - Boucle principale (Message Loop + Render Loop)                   │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│                                    ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         ChatWindow                                   │    │
│  │  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐  │    │
│  │  │  RenderLogin     │  │  RenderSidebar   │  │  RenderMessages  │  │    │
│  │  │  Screen()        │  │  ()              │  │  ()              │  │    │
│  │  │                  │  │                  │  │                  │  │    │
│  │  │  - Username      │  │  - Liste rooms   │  │  - Historique    │  │    │
│  │  │  - Password      │  │  - Boutons       │  │  - Saisie        │  │    │
│  │  │  - Chat ASCII    │  │  - Déconnexion   │  │  - Envoi         │  │    │
│  │  └──────────────────┘  └──────────────────┘  └──────────────────┘  │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                    │                                         │
│                                    ▼                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                        MatrixClient                                  │    │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │    │
│  │  │   Login()    │  │   Sync()     │  │ SendMessage()│               │    │
│  │  │   Register() │  │   (thread)   │  │ CreateRoom() │               │    │
│  │  └──────────────┘  └──────────────┘  └──────────────┘               │    │
│  │                           │                                          │    │
│  │                           ▼                                          │    │
│  │  ┌─────────────────────────────────────────────────────────────┐    │    │
│  │  │                    HttpRequest()                             │    │    │
│  │  │  - WinHTTP API (Windows native)                              │    │    │
│  │  │  - HTTPS avec validation SSL                                 │    │    │
│  │  │  - JSON body via nlohmann/json                               │    │    │
│  │  └─────────────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 3.2 Fichier main.cpp - Point d'Entrée

### 3.2.1 Création de la Fenêtre Windows

```cpp
#include <windows.h>
#include <d3d11.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "chat_window.h"
#include "matrix_client.h"

// Variables globales DirectX
ID3D11Device*           g_pd3dDevice = nullptr;
ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
IDXGISwapChain*         g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Procédure de fenêtre Windows (gestion des messages)
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // ImGui capture les inputs en premier
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        // Redimensionnement de la fenêtre
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, 
                (UINT)LOWORD(lParam),  // Nouvelle largeur
                (UINT)HIWORD(lParam),  // Nouvelle hauteur
                DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // Enregistrement de la classe de fenêtre
    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),           // Taille de la structure
        CS_CLASSDC,                    // Style de classe
        WndProc,                       // Procédure de fenêtre
        0, 0,                          // Extra bytes
        hInstance,                     // Instance
        nullptr,                       // Icône
        nullptr,                       // Curseur
        nullptr,                       // Brush de fond
        nullptr,                       // Menu
        L"KittyChatClass",            // Nom de la classe
        nullptr                        // Petite icône
    };
    RegisterClassEx(&wc);

    // Création de la fenêtre
    HWND hwnd = CreateWindowEx(
        0,                             // Style étendu
        wc.lpszClassName,              // Classe
        L"🐱 Kitty Chat",             // Titre (avec emoji!)
        WS_OVERLAPPEDWINDOW,           // Style (redimensionnable)
        100, 100,                      // Position X, Y
        1280, 720,                     // Taille
        nullptr,                       // Parent
        nullptr,                       // Menu
        hInstance,                     // Instance
        nullptr                        // Paramètre
    );

    // Initialisation DirectX 11
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // ... suite dans la section suivante
}
```

### 3.2.2 Initialisation de DirectX 11

```cpp
bool CreateDeviceD3D(HWND hWnd)
{
    // Description de la swap chain
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;                              // Double buffering
    sd.BufferDesc.Width = 0;                         // Auto-détection
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 32 bits RGBA
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;                         // Pas d'antialiasing
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    // Feature levels à essayer (du plus récent au plus ancien)
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    // Création du device et de la swap chain
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    // Adaptateur par défaut
        D3D_DRIVER_TYPE_HARDWARE,   // Accélération matérielle
        nullptr,                    // Pas de software rasterizer
        0,                          // Flags (debug si nécessaire)
        featureLevelArray,          // Feature levels
        2,                          // Nombre de feature levels
        D3D11_SDK_VERSION,          // Version SDK
        &sd,                        // Description swap chain
        &g_pSwapChain,              // [out] Swap chain
        &g_pd3dDevice,              // [out] Device
        &featureLevel,              // [out] Feature level obtenu
        &g_pd3dDeviceContext        // [out] Device context
    );

    if (FAILED(hr))
        return false;

    CreateRenderTarget();
    return true;
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    
    // Récupération du back buffer
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    
    // Création du render target view
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, 
                                         &g_mainRenderTargetView);
    pBackBuffer->Release();
}
```

### 3.2.3 Initialisation de Dear ImGui

```cpp
// Contexte ImGui
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();

// Configuration
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Navigation clavier

// Style personnalisé
ImGuiStyle& style = ImGui::GetStyle();
style.WindowRounding = 8.0f;       // Coins arrondis
style.FrameRounding = 4.0f;
style.GrabRounding = 4.0f;
style.WindowBorderSize = 0.0f;     // Pas de bordure
style.FrameBorderSize = 0.0f;

// Couleurs personnalisées (thème violet/rose)
ImVec4* colors = style.Colors;
colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.08f, 0.15f, 0.95f);
colors[ImGuiCol_Button] = ImVec4(0.6f, 0.3f, 0.5f, 1.0f);
colors[ImGuiCol_ButtonHovered] = ImVec4(0.7f, 0.4f, 0.6f, 1.0f);
colors[ImGuiCol_ButtonActive] = ImVec4(0.8f, 0.5f, 0.7f, 1.0f);
colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.1f, 0.2f, 1.0f);
colors[ImGuiCol_TitleBg] = ImVec4(0.2f, 0.1f, 0.25f, 1.0f);
colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.15f, 0.35f, 1.0f);

// Police personnalisée (optionnel)
io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);

// Initialisation des backends
ImGui_ImplWin32_Init(hwnd);
ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
```

### 3.2.4 Boucle Principale

```cpp
// Instanciation des objets principaux
MatrixClient client;
ChatWindow chatWindow(&client);

// Boucle de messages Windows + rendu
MSG msg;
ZeroMemory(&msg, sizeof(msg));

while (msg.message != WM_QUIT)
{
    // Traitement des messages Windows (non-bloquant)
    if (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        continue;
    }

    // Début de frame ImGui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Rendu de l'interface Kitty Chat
    chatWindow.Render();

    // Fin de frame et présentation
    ImGui::Render();
    
    // Clear du backbuffer (couleur de fond)
    const float clear_color[4] = { 0.1f, 0.08f, 0.15f, 1.0f };
    g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
    g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
    
    // Rendu ImGui via DirectX
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    
    // Swap des buffers (VSync activé)
    g_pSwapChain->Present(1, 0);  // 1 = VSync on, 0 = VSync off
}

// Nettoyage
ImGui_ImplDX11_Shutdown();
ImGui_ImplWin32_Shutdown();
ImGui::DestroyContext();
CleanupDeviceD3D();
DestroyWindow(hwnd);
UnregisterClass(wc.lpszClassName, hInstance);

return 0;
```

## 3.3 Fichier matrix_client.cpp - Communication avec le Serveur

### 3.3.1 Structure de la Classe

```cpp
// matrix_client.h
#pragma once

#include <string>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

// Structure d'un message
struct Message {
    std::string sender;      // @user:server
    std::string body;        // Contenu du message
    uint64_t timestamp;      // Timestamp Unix (ms)
    std::string eventId;     // ID unique de l'événement
};

// Structure d'une room
struct Room {
    std::string id;          // !roomid:server
    std::string name;        // Nom affiché
    std::vector<Message> messages;
    int unreadCount = 0;
};

class MatrixClient {
public:
    MatrixClient();
    ~MatrixClient();

    // Authentification
    bool Login(const std::string& username, const std::string& password);
    bool Register(const std::string& username, const std::string& password);
    void Logout();

    // Rooms
    bool CreateRoom(const std::string& name);
    bool JoinRoom(const std::string& roomIdOrAlias);
    const std::map<std::string, Room>& GetRooms() const { return m_rooms; }

    // Messages
    bool SendMessage(const std::string& roomId, const std::string& message);

    // État
    bool IsLoggedIn() const { return m_isLoggedIn; }
    std::string GetUserId() const { return m_userId; }
    std::string GetLastError() const { return m_lastError; }

private:
    // Synchronisation
    void StartSync();
    void StopSync();
    void SyncLoop();
    void ProcessSyncResponse(const nlohmann::json& response);

    // HTTP
    bool HttpRequest(const std::string& method,
                     const std::string& endpoint,
                     const std::string& body,
                     std::string& response);

    // Données
    std::string m_homeserver = "vault.buffertavern.com";
    std::string m_accessToken;
    std::string m_userId;
    std::string m_deviceId;
    std::string m_syncToken;
    std::string m_lastError;
    
    std::atomic<bool> m_isLoggedIn{false};
    std::atomic<bool> m_stopSync{false};
    
    std::map<std::string, Room> m_rooms;
    std::mutex m_roomsMutex;
    
    std::thread m_syncThread;
};
```

### 3.3.2 Implémentation des Requêtes HTTP avec WinHTTP

```cpp
#include <windows.h>
#include <winhttp.h>
#include <string>

#pragma comment(lib, "winhttp.lib")

bool MatrixClient::HttpRequest(
    const std::string& method,
    const std::string& endpoint,
    const std::string& body,
    std::string& response)
{
    // Conversion en wide strings (WinHTTP utilise WCHAR)
    std::wstring wMethod(method.begin(), method.end());
    std::wstring wHost(m_homeserver.begin(), m_homeserver.end());
    std::wstring wEndpoint(endpoint.begin(), endpoint.end());

    // 1. Ouverture de la session WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"KittyChat/2.0",                    // User-Agent
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,   // Utiliser le proxy système
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0                                     // Flags (0 = synchrone)
    );
    
    if (!hSession) {
        m_lastError = "Erreur WinHttpOpen: " + std::to_string(GetLastError());
        return false;
    }

    // 2. Connexion au serveur
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        wHost.c_str(),                       // Serveur
        INTERNET_DEFAULT_HTTPS_PORT,         // Port 443 (HTTPS)
        0
    );
    
    if (!hConnect) {
        m_lastError = "Erreur WinHttpConnect: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hSession);
        return false;
    }

    // 3. Création de la requête
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        wMethod.c_str(),                     // GET, POST, PUT, etc.
        wEndpoint.c_str(),                   // Endpoint (ex: /_matrix/client/v3/login)
        nullptr,                             // HTTP/1.1 par défaut
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE                  // HTTPS obligatoire
    );
    
    if (!hRequest) {
        m_lastError = "Erreur WinHttpOpenRequest: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // 4. Ajout des headers
    std::wstring headers = L"Content-Type: application/json\r\n";
    
    // Ajouter le token d'authentification si connecté
    if (!m_accessToken.empty()) {
        std::wstring wToken(m_accessToken.begin(), m_accessToken.end());
        headers += L"Authorization: Bearer " + wToken + L"\r\n";
    }
    
    WinHttpAddRequestHeaders(
        hRequest,
        headers.c_str(),
        -1,                                  // Longueur auto
        WINHTTP_ADDREQ_FLAG_ADD
    );

    // 5. Envoi de la requête
    BOOL success = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS, 0,    // Headers supplémentaires
        (LPVOID)body.c_str(),                // Body
        (DWORD)body.length(),                // Taille du body
        (DWORD)body.length(),                // Taille totale
        0                                     // Context
    );
    
    if (!success) {
        m_lastError = "Erreur WinHttpSendRequest: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // 6. Réception de la réponse
    success = WinHttpReceiveResponse(hRequest, nullptr);
    
    if (!success) {
        m_lastError = "Erreur WinHttpReceiveResponse: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // 7. Vérification du code de statut HTTP
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusCodeSize,
        WINHTTP_NO_HEADER_INDEX
    );

    // 8. Lecture du body de la réponse
    response.clear();
    DWORD bytesAvailable = 0;
    
    do {
        bytesAvailable = 0;
        WinHttpQueryDataAvailable(hRequest, &bytesAvailable);
        
        if (bytesAvailable > 0) {
            std::vector<char> buffer(bytesAvailable + 1);
            DWORD bytesRead = 0;
            
            WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead);
            buffer[bytesRead] = '\0';
            response += buffer.data();
        }
    } while (bytesAvailable > 0);

    // 9. Nettoyage
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    // 10. Vérification du code de statut
    if (statusCode < 200 || statusCode >= 300) {
        m_lastError = "Erreur HTTP " + std::to_string(statusCode) + ": " + 
                      response.substr(0, 200);
        return false;
    }

    return true;
}
```

### 3.3.3 Authentification (Login)

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool MatrixClient::Login(const std::string& username, const std::string& password)
{
    // Construction du body JSON
    json loginRequest = {
        {"type", "m.login.password"},
        {"identifier", {
            {"type", "m.id.user"},
            {"user", username}
        }},
        {"password", password},
        {"initial_device_display_name", "Kitty Chat C++"}
    };

    std::string response;
    bool success = HttpRequest("POST", "/_matrix/client/v3/login", 
                               loginRequest.dump(), response);

    if (!success) {
        // m_lastError déjà défini par HttpRequest
        return false;
    }

    try {
        json loginResponse = json::parse(response);
        
        // Récupération des informations de session
        m_accessToken = loginResponse["access_token"].get<std::string>();
        m_userId = loginResponse["user_id"].get<std::string>();
        m_deviceId = loginResponse["device_id"].get<std::string>();
        
        m_isLoggedIn = true;
        
        // Démarrage de la synchronisation en arrière-plan
        StartSync();
        
        return true;
    }
    catch (const json::exception& e) {
        m_lastError = "Erreur parsing JSON: " + std::string(e.what());
        return false;
    }
}
```

### 3.3.4 Inscription (Register)

```cpp
bool MatrixClient::Register(const std::string& username, const std::string& password)
{
    // Étape 1: Requête initiale pour obtenir les flows d'authentification
    json registerRequest = {
        {"username", username},
        {"password", password},
        {"auth", {
            {"type", "m.login.dummy"}  // Type d'auth le plus simple
        }},
        {"initial_device_display_name", "Kitty Chat C++"}
    };

    std::string response;
    bool success = HttpRequest("POST", "/_matrix/client/v3/register",
                               registerRequest.dump(), response);

    if (!success) {
        return false;
    }

    try {
        json registerResponse = json::parse(response);
        
        // Vérification si l'inscription a réussi
        if (registerResponse.contains("access_token")) {
            m_accessToken = registerResponse["access_token"].get<std::string>();
            m_userId = registerResponse["user_id"].get<std::string>();
            m_deviceId = registerResponse["device_id"].get<std::string>();
            
            m_isLoggedIn = true;
            StartSync();
            return true;
        }
        
        // Si on reçoit un flow d'authentification à compléter
        if (registerResponse.contains("flows")) {
            m_lastError = "Inscription nécessite une vérification supplémentaire";
            return false;
        }
        
        // Erreur Matrix
        if (registerResponse.contains("error")) {
            m_lastError = registerResponse["error"].get<std::string>();
            return false;
        }
    }
    catch (const json::exception& e) {
        m_lastError = "Erreur parsing JSON: " + std::string(e.what());
        return false;
    }

    return false;
}
```

### 3.3.5 Synchronisation (Sync Loop)

La synchronisation utilise le **long polling** : le client fait une requête qui reste ouverte jusqu'à ce qu'il y ait de nouvelles données ou timeout.

```cpp
void MatrixClient::StartSync()
{
    m_stopSync = false;
    m_syncThread = std::thread(&MatrixClient::SyncLoop, this);
}

void MatrixClient::StopSync()
{
    m_stopSync = true;
    if (m_syncThread.joinable()) {
        m_syncThread.join();
    }
}

void MatrixClient::SyncLoop()
{
    while (!m_stopSync && m_isLoggedIn)
    {
        // Construction de l'endpoint avec paramètres
        std::string endpoint = "/_matrix/client/v3/sync?timeout=30000";
        
        // Si on a un token de sync, on ne récupère que les nouveaux événements
        if (!m_syncToken.empty()) {
            endpoint += "&since=" + m_syncToken;
        }
        
        // Filtre pour ne récupérer que les événements utiles
        endpoint += "&filter={\"room\":{\"timeline\":{\"limit\":50}}}";

        std::string response;
        bool success = HttpRequest("GET", endpoint, "", response);

        if (success) {
            try {
                json syncResponse = json::parse(response);
                
                // Sauvegarde du token pour la prochaine sync
                if (syncResponse.contains("next_batch")) {
                    m_syncToken = syncResponse["next_batch"].get<std::string>();
                }
                
                // Traitement des événements
                ProcessSyncResponse(syncResponse);
            }
            catch (const json::exception& e) {
                // Log l'erreur mais continue la boucle
            }
        }
        else {
            // En cas d'erreur, attendre avant de réessayer
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

void MatrixClient::ProcessSyncResponse(const json& response)
{
    std::lock_guard<std::mutex> lock(m_roomsMutex);

    // Traitement des rooms où l'utilisateur est membre
    if (response.contains("rooms") && response["rooms"].contains("join")) {
        for (auto& [roomId, roomData] : response["rooms"]["join"].items()) {
            
            // Créer la room si elle n'existe pas
            if (m_rooms.find(roomId) == m_rooms.end()) {
                m_rooms[roomId] = Room{roomId, roomId, {}, 0};
            }
            
            Room& room = m_rooms[roomId];
            
            // Récupérer le nom de la room depuis l'état
            if (roomData.contains("state") && roomData["state"].contains("events")) {
                for (const auto& event : roomData["state"]["events"]) {
                    if (event["type"] == "m.room.name") {
                        room.name = event["content"]["name"].get<std::string>();
                    }
                }
            }
            
            // Récupérer les nouveaux messages
            if (roomData.contains("timeline") && roomData["timeline"].contains("events")) {
                for (const auto& event : roomData["timeline"]["events"]) {
                    if (event["type"] == "m.room.message") {
                        Message msg;
                        msg.sender = event["sender"].get<std::string>();
                        msg.body = event["content"]["body"].get<std::string>();
                        msg.timestamp = event["origin_server_ts"].get<uint64_t>();
                        msg.eventId = event["event_id"].get<std::string>();
                        
                        // Éviter les doublons
                        bool exists = false;
                        for (const auto& existing : room.messages) {
                            if (existing.eventId == msg.eventId) {
                                exists = true;
                                break;
                            }
                        }
                        
                        if (!exists) {
                            room.messages.push_back(msg);
                            
                            // Incrémenter le compteur de non-lus
                            // (sauf si c'est notre propre message)
                            if (msg.sender != m_userId) {
                                room.unreadCount++;
                            }
                        }
                    }
                }
            }
            
            // Compteur de non-lus depuis le serveur
            if (roomData.contains("unread_notifications")) {
                if (roomData["unread_notifications"].contains("notification_count")) {
                    room.unreadCount = roomData["unread_notifications"]
                                       ["notification_count"].get<int>();
                }
            }
        }
    }
}
```

### 3.3.6 Envoi de Messages

```cpp
bool MatrixClient::SendMessage(const std::string& roomId, const std::string& message)
{
    // Génération d'un ID de transaction unique
    // (évite les doublons si la requête est réessayée)
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    std::string txnId = std::to_string(timestamp);

    // Endpoint avec l'ID de transaction
    std::string endpoint = "/_matrix/client/v3/rooms/" + roomId + 
                          "/send/m.room.message/" + txnId;

    // Contenu du message
    json msgContent = {
        {"msgtype", "m.text"},
        {"body", message}
    };

    std::string response;
    bool success = HttpRequest("PUT", endpoint, msgContent.dump(), response);

    if (success) {
        // Le message sera reçu via la sync loop
        return true;
    }

    return false;
}
```

### 3.3.7 Création de Room

```cpp
bool MatrixClient::CreateRoom(const std::string& name)
{
    json createRequest = {
        {"name", name},
        {"preset", "public_chat"},  // public_chat, private_chat, trusted_private_chat
        {"visibility", "private"}, // public = visible dans le répertoire
        {"initial_state", json::array()}
    };

    std::string response;
    bool success = HttpRequest("POST", "/_matrix/client/v3/createRoom",
                               createRequest.dump(), response);

    if (success) {
        try {
            json createResponse = json::parse(response);
            std::string roomId = createResponse["room_id"].get<std::string>();
            
            // La room sera ajoutée via la sync loop
            return true;
        }
        catch (...) {
            m_lastError = "Erreur parsing réponse createRoom";
        }
    }

    return false;
}
```

## 3.4 Fichier chat_window.cpp - Interface Graphique

### 3.4.1 Structure de la Classe

```cpp
// chat_window.h
#pragma once

#include "matrix_client.h"
#include <string>

class ChatWindow {
public:
    ChatWindow(MatrixClient* client);
    void Render();

private:
    // Écrans
    void RenderLoginScreen();
    void RenderMainScreen();
    
    // Composants de l'écran principal
    void RenderSidebar();
    void RenderMessageArea();
    void RenderInputArea();
    
    // Effets visuels
    void RenderBackground();
    void RenderCatAscii();

    // Données
    MatrixClient* m_client;
    
    // État de l'interface
    char m_username[64] = "";
    char m_password[64] = "";
    char m_messageInput[1024] = "";
    char m_newRoomName[64] = "";
    char m_joinRoomId[128] = "";
    
    std::string m_selectedRoom;
    std::string m_errorMessage;
    std::string m_successMessage;
    
    bool m_showPassword = false;
    bool m_passwordFocused = false;
    bool m_isRegistering = false;
    bool m_showCreateRoom = false;
    bool m_showJoinRoom = false;
    
    // Animation
    float m_animTime = 0.0f;
    float m_lastFrameTime = 0.0f;
};
```

### 3.4.2 Fonction Render Principale

```cpp
void ChatWindow::Render()
{
    // Calcul du delta time pour les animations
    float currentTime = (float)ImGui::GetTime();
    float deltaTime = currentTime - m_lastFrameTime;
    m_lastFrameTime = currentTime;
    m_animTime += deltaTime;

    // Rendu du fond animé
    RenderBackground();

    // Choix de l'écran selon l'état de connexion
    if (!m_client->IsLoggedIn()) {
        RenderLoginScreen();
    } else {
        RenderMainScreen();
    }
}
```

### 3.4.3 Écran de Connexion

```cpp
void ChatWindow::RenderLoginScreen()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;

    // Fenêtre centrée
    ImVec2 windowSize(400, 500);
    ImVec2 windowPos(
        (displaySize.x - windowSize.x) / 2,
        (displaySize.y - windowSize.y) / 2
    );
    
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Login", nullptr, flags);

    // Titre avec effet arc-en-ciel
    ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("Kitty Chat").x * 2) / 2);
    
    float hue = fmodf(m_animTime * 0.1f, 1.0f);
    ImVec4 titleColor = ImColor::HSV(hue, 0.6f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
    ImGui::Text("Kitty Chat");
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Chat ASCII animé
    RenderCatAscii();

    ImGui::Spacing();
    ImGui::Spacing();

    // Champ utilisateur
    ImGui::Text("Utilisateur");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##username", m_username, sizeof(m_username));

    ImGui::Spacing();

    // Champ mot de passe
    ImGui::Text("Mot de passe");
    ImGui::SetNextItemWidth(-1);
    
    // Détecter si le champ password a le focus
    ImGuiInputTextFlags passFlags = m_showPassword ? 0 : ImGuiInputTextFlags_Password;
    ImGui::InputText("##password", m_password, sizeof(m_password), passFlags);
    m_passwordFocused = ImGui::IsItemActive();

    // Checkbox montrer le mot de passe
    ImGui::Checkbox("Afficher", &m_showPassword);

    ImGui::Spacing();
    ImGui::Spacing();

    // Boutons
    float buttonWidth = 180;
    
    // Bouton Connexion (orange)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.5f, 0.2f, 1.0f));
    
    if (ImGui::Button("Connexion", ImVec2(buttonWidth, 40))) {
        m_errorMessage.clear();
        m_successMessage.clear();
        
        if (!m_client->Login(m_username, m_password)) {
            m_errorMessage = m_client->GetLastError();
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // Bouton Inscription (vert)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.4f, 1.0f));
    
    if (ImGui::Button("S'inscrire", ImVec2(buttonWidth, 40))) {
        m_errorMessage.clear();
        
        if (m_client->Register(m_username, m_password)) {
            m_successMessage = "Compte cree avec succes !";
        } else {
            m_errorMessage = m_client->GetLastError();
        }
    }
    ImGui::PopStyleColor(2);

    // Messages d'erreur/succès
    if (!m_errorMessage.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("%s", m_errorMessage.c_str());
        ImGui::PopStyleColor();
    }
    
    if (!m_successMessage.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
        ImGui::TextWrapped("%s", m_successMessage.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::End();
}
```

### 3.4.4 Chat ASCII Interactif

```cpp
void ChatWindow::RenderCatAscii()
{
    // Calcul de l'animation
    float bounce = sinf(m_animTime * 2.0f) * 3.0f;
    
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + bounce);

    // Choix de l'art ASCII selon l'état
    const char* catArt;
    
    if (m_passwordFocused && m_showPassword) {
        // Le chat "peek" - un œil ouvert (curieux)
        catArt = 
            "      /\\_____/\\     \n"
            "     /  o   -  \\    \n"
            "    ( ==  ^  == )   \n"
            "     )         (    \n"
            "    (           )   \n"
            "   ( (  )   (  ) )  \n"
            "  (__(__)___(__)__) \n";
    }
    else if (m_passwordFocused) {
        // Le chat dort - yeux fermés (cache le mot de passe)
        catArt = 
            "      /\\_____/\\   z \n"
            "     /  -   -  \\ z  \n"
            "    ( ==  w  == )   \n"
            "     )         (    \n"
            "    (   _____   )   \n"
            "   (   /     \\   )  \n"
            "  (___(_________)_) \n";
    }
    else {
        // Le chat regarde - yeux ouverts
        catArt = 
            "      /\\_____/\\     \n"
            "     /  o   o  \\    \n"
            "    ( ==  ^  == )   \n"
            "     )         (    \n"
            "    (           )   \n"
            "   ( (  )   (  ) )  \n"
            "  (__(__)___(__)__) \n";
    }
    
    // Affichage centré avec couleur
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
    
    // Centrer le texte
    float textWidth = ImGui::CalcTextSize(catArt).x;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) / 2);
    
    ImGui::Text("%s", catArt);
    ImGui::PopStyleColor();
}
```

### 3.4.5 Écran Principal (Chat)

```cpp
void ChatWindow::RenderMainScreen()
{
    ImGuiIO& io = ImGui::GetIO();
    
    // Sidebar à gauche (250px)
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(250, io.DisplaySize.y));
    
    ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove;
    
    ImGui::Begin("Sidebar", nullptr, sidebarFlags);
    RenderSidebar();
    ImGui::End();

    // Zone principale (reste de l'écran)
    ImGui::SetNextWindowPos(ImVec2(250, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 250, io.DisplaySize.y));
    
    ImGui::Begin("Messages", nullptr, sidebarFlags);
    RenderMessageArea();
    RenderInputArea();
    ImGui::End();
}

void ChatWindow::RenderSidebar()
{
    // Titre avec ID utilisateur
    ImGui::Text("Connecte: %s", m_client->GetUserId().c_str());
    ImGui::Separator();

    // Boutons d'action
    if (ImGui::Button("+ Creer", ImVec2(-1, 30))) {
        m_showCreateRoom = true;
    }
    
    if (ImGui::Button("Rejoindre", ImVec2(-1, 30))) {
        m_showJoinRoom = true;
    }
    
    ImGui::Separator();
    ImGui::Text("Salons:");
    ImGui::Separator();

    // Liste des rooms
    const auto& rooms = m_client->GetRooms();
    
    for (const auto& [roomId, room] : rooms) {
        // Affichage avec badge de non-lus
        std::string label = room.name.empty() ? roomId : room.name;
        
        if (room.unreadCount > 0) {
            label += " (" + std::to_string(room.unreadCount) + ")";
        }
        
        // Sélection
        bool isSelected = (m_selectedRoom == roomId);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            m_selectedRoom = roomId;
        }
    }

    // Bouton déconnexion en bas
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 40);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Deconnexion", ImVec2(-1, 30))) {
        m_client->Logout();
        m_selectedRoom.clear();
    }
    ImGui::PopStyleColor();

    // Popups de création/jonction de room
    RenderCreateRoomPopup();
    RenderJoinRoomPopup();
}
```

### 3.4.6 Zone de Messages

```cpp
void ChatWindow::RenderMessageArea()
{
    if (m_selectedRoom.empty()) {
        // Message d'accueil
        ImGui::SetCursorPos(ImVec2(
            ImGui::GetWindowWidth() / 2 - 100,
            ImGui::GetWindowHeight() / 2 - 50
        ));
        ImGui::Text("Selectionnez un salon");
        return;
    }

    // Récupération des messages de la room sélectionnée
    const auto& rooms = m_client->GetRooms();
    auto it = rooms.find(m_selectedRoom);
    
    if (it == rooms.end()) {
        ImGui::Text("Room introuvable");
        return;
    }

    const Room& room = it->second;

    // Titre de la room
    ImGui::Text("# %s", room.name.c_str());
    ImGui::Separator();

    // Zone scrollable pour les messages
    ImGui::BeginChild("MessagesScroll", ImVec2(0, -60), true);

    for (const Message& msg : room.messages) {
        // Formatage du timestamp
        time_t timestamp = msg.timestamp / 1000;
        struct tm* timeinfo = localtime(&timestamp);
        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%H:%M", timeinfo);

        // Couleur différente pour nos messages vs les autres
        bool isOwnMessage = (msg.sender == m_client->GetUserId());
        
        if (isOwnMessage) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.8f, 1.0f));
        }

        // Affichage: [HH:MM] @user: message
        ImGui::Text("[%s] %s:", timeStr, msg.sender.c_str());
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        ImGui::TextWrapped("%s", msg.body.c_str());
    }

    // Auto-scroll vers le bas
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}
```

### 3.4.7 Zone de Saisie

```cpp
void ChatWindow::RenderInputArea()
{
    ImGui::Separator();

    // Champ de saisie
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 120);
    
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
    bool enterPressed = ImGui::InputText("##message", m_messageInput, 
                                          sizeof(m_messageInput), inputFlags);

    ImGui::SameLine();

    // Bouton d'envoi
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
    bool sendClicked = ImGui::Button("Envoyer", ImVec2(100, 0));
    ImGui::PopStyleColor();

    // Envoi du message
    if ((enterPressed || sendClicked) && strlen(m_messageInput) > 0) {
        if (m_client->SendMessage(m_selectedRoom, m_messageInput)) {
            m_messageInput[0] = '\0';  // Vider le champ
        }
        
        // Remettre le focus sur le champ de saisie
        ImGui::SetKeyboardFocusHere(-1);
    }
}
```

### 3.4.8 Fond Animé avec Particules

```cpp
void ChatWindow::RenderBackground()
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    
    float width = io.DisplaySize.x;
    float height = io.DisplaySize.y;

    // Dégradé de fond
    ImU32 topLeft = IM_COL32(30, 20, 45, 255);
    ImU32 topRight = IM_COL32(40, 25, 55, 255);
    ImU32 bottomLeft = IM_COL32(25, 18, 35, 255);
    ImU32 bottomRight = IM_COL32(35, 22, 50, 255);
    
    drawList->AddRectFilledMultiColor(
        ImVec2(0, 0),
        ImVec2(width, height),
        topLeft, topRight, bottomRight, bottomLeft
    );

    // Particules/étoiles scintillantes
    static std::vector<ImVec4> stars;  // x, y, size, twinklePhase
    
    // Initialisation des étoiles (une seule fois)
    if (stars.empty()) {
        for (int i = 0; i < 100; i++) {
            stars.push_back(ImVec4(
                (float)(rand() % (int)width),
                (float)(rand() % (int)height),
                (float)(rand() % 3 + 1),
                (float)(rand() % 628) / 100.0f  // Phase 0-2π
            ));
        }
    }

    // Rendu des étoiles avec scintillement
    for (auto& star : stars) {
        float alpha = 0.3f + 0.5f * sinf(m_animTime * 2.0f + star.w);
        ImU32 color = IM_COL32(255, 255, 255, (int)(alpha * 255));
        
        drawList->AddCircleFilled(
            ImVec2(star.x, star.y),
            star.z,
            color
        );
    }

    // Quelques pattes de chat décoratives
    const char* paw = "🐾";
    float pawAlpha = 0.1f + 0.05f * sinf(m_animTime);
    ImU32 pawColor = IM_COL32(255, 200, 150, (int)(pawAlpha * 255));
    
    // Afficher quelques pattes à des positions fixes
    float pawPositions[][2] = {
        {50, 100}, {width - 80, 200}, {100, height - 150},
        {width - 120, height - 100}, {width / 2, 50}
    };
    
    for (auto& pos : pawPositions) {
        drawList->AddText(ImVec2(pos[0], pos[1]), pawColor, paw);
    }
}
```

---

*Suite dans la Partie 3 : Protocole Matrix et Sécurité*
