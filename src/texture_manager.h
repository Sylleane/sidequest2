/**
 * @file texture_manager.h
 * @brief Gestionnaire de textures et GIFs animés pour Kitty Chat
 * 
 * Ce fichier gère le téléchargement, le décodage et l'animation
 * de GIFs de chats depuis internet.
 * 
 * Auteurs: Enzo Dupuy, Eric Deswarte, Mathis abbadie 
 */

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <d3d11.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

/**
 * @struct GifFrame
 * @brief Une frame d'un GIF animé
 */
struct GifFrame
{
    ID3D11ShaderResourceView* texture = nullptr;
    int delay = 100; // Délai en ms
};

/**
 * @struct AnimatedGif
 * @brief Un GIF animé complet avec toutes ses frames
 */
struct AnimatedGif
{
    std::vector<GifFrame> frames;
    int currentFrame = 0;
    std::chrono::steady_clock::time_point lastFrameTime;
    int width = 0;
    int height = 0;
    bool loaded = false;
    bool loading = false;
};

/**
 * @class TextureManager
 * @brief Gestionnaire de textures et de GIFs animés
 */
class TextureManager
{
public:
    /**
     * @brief Constructeur
     * @param device Device DirectX11
     */
    explicit TextureManager(ID3D11Device* device);
    
    /**
     * @brief Destructeur - libère toutes les textures
     */
    ~TextureManager();
    
    /**
     * @brief Télécharge un GIF depuis internet
     * @param url URL du GIF
     * @param name Nom pour identifier le GIF
     * @return true si le téléchargement a démarré
     */
    bool LoadGifFromUrl(const std::string& url, const std::string& name);
    
    /**
     * @brief Met à jour les animations (appeler chaque frame)
     */
    void Update();
    
    /**
     * @brief Récupère la texture actuelle d'un GIF
     * @param name Nom du GIF
     * @return Texture ou nullptr si pas trouvé
     */
    ID3D11ShaderResourceView* GetCurrentFrame(const std::string& name);
    
    /**
     * @brief Récupère les dimensions d'un GIF
     * @param name Nom du GIF
     * @param width Largeur (sortie)
     * @param height Hauteur (sortie)
     * @return true si trouvé
     */
    bool GetGifSize(const std::string& name, int& width, int& height);
    
    /**
     * @brief Vérifie si un GIF est chargé
     * @param name Nom du GIF
     * @return true si chargé
     */
    bool IsLoaded(const std::string& name);
    
    /**
     * @brief Charge une image PNG/JPG depuis des données en mémoire
     * @param data Données de l'image
     * @param size Taille des données
     * @param name Nom pour identifier l'image
     * @return true si succès
     */
    bool LoadImageFromMemory(const unsigned char* data, int size, const std::string& name);

private:
    ID3D11Device* m_device;
    std::map<std::string, AnimatedGif> m_gifs;
    std::map<std::string, ID3D11ShaderResourceView*> m_staticImages;
    
    /**
     * @brief Crée une texture DirectX11 depuis des données RGBA
     */
    ID3D11ShaderResourceView* CreateTexture(const unsigned char* data, int width, int height);
    
    /**
     * @brief Télécharge un fichier depuis internet
     */
    std::vector<unsigned char> DownloadFile(const std::string& url);
    
    /**
     * @brief Décode un GIF et crée les textures
     */
    bool DecodeGif(const std::vector<unsigned char>& data, AnimatedGif& gif);
};

#endif // TEXTURE_MANAGER_H
