/**
 * @file texture_manager.cpp
 * @brief Implémentation du gestionnaire de textures
 * 
 * Gère le téléchargement de GIFs depuis internet et leur animation.
 */

#include <windows.h>
#include <winhttp.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "texture_manager.h"
#include <thread>
#include <mutex>

static std::mutex g_textureMutex;

/**
 * @brief Constructeur
 */
TextureManager::TextureManager(ID3D11Device* device)
    : m_device(device)
{
}

/**
 * @brief Destructeur - libère toutes les textures
 */
TextureManager::~TextureManager()
{
    for (auto& pair : m_gifs)
    {
        for (auto& frame : pair.second.frames)
        {
            if (frame.texture)
            {
                frame.texture->Release();
            }
        }
    }
    
    for (auto& pair : m_staticImages)
    {
        if (pair.second)
        {
            pair.second->Release();
        }
    }
}

/**
 * @brief Crée une texture DirectX11 depuis des données RGBA
 */
ID3D11ShaderResourceView* TextureManager::CreateTexture(const unsigned char* data, int width, int height)
{
    if (!m_device || !data || width <= 0 || height <= 0)
        return nullptr;

    // Description de la texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    // Données initiales
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(initData));
    initData.pSysMem = data;
    initData.SysMemPitch = width * 4;

    // Création de la texture
    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = m_device->CreateTexture2D(&desc, &initData, &texture);
    if (FAILED(hr))
        return nullptr;

    // Création de la shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = m_device->CreateShaderResourceView(texture, &srvDesc, &srv);
    texture->Release();

    if (FAILED(hr))
        return nullptr;

    return srv;
}

/**
 * @brief Télécharge un fichier depuis internet via WinHTTP
 */
std::vector<unsigned char> TextureManager::DownloadFile(const std::string& url)
{
    std::vector<unsigned char> result;

    // Parse l'URL
    std::string host;
    std::string path = "/";
    bool useHttps = true;
    
    size_t start = 0;
    if (url.find("https://") == 0)
    {
        start = 8;
        useHttps = true;
    }
    else if (url.find("http://") == 0)
    {
        start = 7;
        useHttps = false;
    }

    size_t pathStart = url.find('/', start);
    if (pathStart != std::string::npos)
    {
        host = url.substr(start, pathStart - start);
        path = url.substr(pathStart);
    }
    else
    {
        host = url.substr(start);
    }

    // Conversion en wide string
    std::wstring wHost(host.begin(), host.end());
    std::wstring wPath(path.begin(), path.end());

    // Session WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"KittyChat/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
        return result;

    // Connexion
    INTERNET_PORT port = useHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    HINTERNET hConnect = WinHttpConnect(hSession, wHost.c_str(), port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        return result;
    }

    // Requête
    DWORD flags = useHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        wPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return result;
    }

    // Envoyer la requête
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    );

    if (bResults)
    {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    // Lire les données
    if (bResults)
    {
        DWORD dwSize = 0;
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                break;

            if (dwSize > 0)
            {
                std::vector<unsigned char> buffer(dwSize);
                DWORD dwDownloaded = 0;
                
                if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
                {
                    result.insert(result.end(), buffer.begin(), buffer.begin() + dwDownloaded);
                }
            }
        } while (dwSize > 0);
    }

    // Nettoyage
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return result;
}

/**
 * @brief Décode un GIF et crée les textures pour chaque frame
 */
bool TextureManager::DecodeGif(const std::vector<unsigned char>& data, AnimatedGif& gif)
{
    if (data.empty())
        return false;

    int* delays = nullptr;
    int width, height, frames, comp;

    unsigned char* pixels = stbi_load_gif_from_memory(
        data.data(),
        static_cast<int>(data.size()),
        &delays,
        &width,
        &height,
        &frames,
        &comp,
        4
    );

    if (!pixels)
        return false;

    gif.width = width;
    gif.height = height;
    gif.frames.reserve(frames);

    int stride = width * height * 4;

    for (int i = 0; i < frames; ++i)
    {
        GifFrame frame;
        frame.texture = CreateTexture(pixels + (i * stride), width, height);
        frame.delay = delays ? delays[i] : 100;
        
        if (frame.delay < 20)
            frame.delay = 100; // Minimum 20ms
            
        gif.frames.push_back(frame);
    }

    if (delays)
        free(delays);
    stbi_image_free(pixels);

    gif.loaded = true;
    gif.loading = false;
    gif.lastFrameTime = std::chrono::steady_clock::now();

    return true;
}

/**
 * @brief Lance le téléchargement d'un GIF en arrière-plan
 */
bool TextureManager::LoadGifFromUrl(const std::string& url, const std::string& name)
{
    std::lock_guard<std::mutex> lock(g_textureMutex);
    
    // Vérifie si déjà en cours de chargement
    auto it = m_gifs.find(name);
    if (it != m_gifs.end())
    {
        if (it->second.loaded || it->second.loading)
            return true;
    }

    // Marquer comme en cours de chargement
    m_gifs[name].loading = true;

    // Télécharger et décoder dans un thread séparé
    std::thread([this, url, name]()
    {
        auto data = DownloadFile(url);
        
        if (!data.empty())
        {
            AnimatedGif gif;
            if (DecodeGif(data, gif))
            {
                std::lock_guard<std::mutex> lock(g_textureMutex);
                m_gifs[name] = std::move(gif);
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(g_textureMutex);
            m_gifs[name].loading = false;
        }
    }).detach();

    return true;
}

/**
 * @brief Met à jour les animations
 */
void TextureManager::Update()
{
    std::lock_guard<std::mutex> lock(g_textureMutex);
    
    auto now = std::chrono::steady_clock::now();

    for (auto& pair : m_gifs)
    {
        AnimatedGif& gif = pair.second;
        if (!gif.loaded || gif.frames.empty())
            continue;

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - gif.lastFrameTime
        ).count();

        if (elapsed >= gif.frames[gif.currentFrame].delay)
        {
            gif.currentFrame = (gif.currentFrame + 1) % gif.frames.size();
            gif.lastFrameTime = now;
        }
    }
}

/**
 * @brief Récupère la texture de la frame actuelle
 */
ID3D11ShaderResourceView* TextureManager::GetCurrentFrame(const std::string& name)
{
    std::lock_guard<std::mutex> lock(g_textureMutex);
    
    auto it = m_gifs.find(name);
    if (it == m_gifs.end() || !it->second.loaded || it->second.frames.empty())
        return nullptr;

    return it->second.frames[it->second.currentFrame].texture;
}

/**
 * @brief Récupère les dimensions d'un GIF
 */
bool TextureManager::GetGifSize(const std::string& name, int& width, int& height)
{
    std::lock_guard<std::mutex> lock(g_textureMutex);
    
    auto it = m_gifs.find(name);
    if (it == m_gifs.end() || !it->second.loaded)
        return false;

    width = it->second.width;
    height = it->second.height;
    return true;
}

/**
 * @brief Vérifie si un GIF est chargé
 */
bool TextureManager::IsLoaded(const std::string& name)
{
    std::lock_guard<std::mutex> lock(g_textureMutex);
    
    auto it = m_gifs.find(name);
    return it != m_gifs.end() && it->second.loaded;
}

/**
 * @brief Charge une image statique depuis la mémoire
 */
bool TextureManager::LoadImageFromMemory(const unsigned char* data, int size, const std::string& name)
{
    // Utilise stbi pour décoder l'image
    // Pour l'instant, on ne supporte que les GIFs animés
    return false;
}
