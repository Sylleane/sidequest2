/**
 * @file chat_window.h
 * @brief Déclaration de la fenêtre de chat principale
 * 
 * Ce fichier définit la classe ChatWindow qui gère l'interface utilisateur
 * du client de chat : écran de connexion, liste des salons, zone de messages.
 */

#ifndef CHAT_WINDOW_H
#define CHAT_WINDOW_H

#include "matrix_client.h"
#include "texture_manager.h"
#include <string>
#include <chrono>

/**
 * @class ChatWindow
 * @brief Fenêtre principale de l'interface de chat
 * 
 * Cette classe gère tout l'affichage ImGui de l'application :
 * - Écran de connexion avec formulaire
 * - Barre latérale avec liste des salons
 * - Zone principale avec les messages
 * - Zone de saisie pour envoyer des messages
 */
class ChatWindow
{
public:
    /**
     * @brief Constructeur
     * @param client Pointeur vers le client Matrix
     * @param texManager Pointeur vers le gestionnaire de textures
     */
    ChatWindow(MatrixClient* client, TextureManager* texManager);
    
    /**
     * @brief Destructeur
     */
    ~ChatWindow() = default;
    
    /**
     * @brief Rendu de l'interface complète
     * 
     * Cette méthode est appelée à chaque frame pour dessiner l'interface
     */
    void Render();

private:
    MatrixClient* m_client;           // Référence vers le client Matrix
    TextureManager* m_texManager;     // Gestionnaire de textures pour les GIFs
    
    // État de l'écran de connexion
    char m_username[256];             // Buffer pour le nom d'utilisateur
    char m_password[256];             // Buffer pour le mot de passe
    bool m_showPassword;              // Afficher/masquer le mot de passe
    bool m_loginError;                // Indicateur d'erreur de connexion
    bool m_isRegistering;             // Mode inscription (true) ou connexion (false)
    std::string m_errorMessage;       // Message d'erreur à afficher
    std::string m_successMessage;     // Message de succès
    
    // État de la zone de chat
    char m_messageInput[4096];        // Buffer pour le message à envoyer
    bool m_scrollToBottom;            // Défiler vers le bas automatiquement
    
    // État pour créer/rejoindre des salons
    char m_newRoomName[256];          // Nom du nouveau salon
    char m_joinRoomId[256];           // ID du salon à rejoindre
    bool m_showCreateRoom;            // Afficher le popup de création
    bool m_showJoinRoom;              // Afficher le popup de join
    
    // Animation et effets visuels
    std::chrono::steady_clock::time_point m_startTime;
    float m_animTime;                 // Temps d'animation
    bool m_gifsLoaded;                // GIFs chargés
    
    // Chat interactif qui suit le curseur
    float m_catEyeTargetX;            // Position cible des yeux X
    float m_catEyeTargetY;            // Position cible des yeux Y
    float m_catEyeCurrentX;           // Position actuelle des yeux X
    float m_catEyeCurrentY;           // Position actuelle des yeux Y
    float m_tailCoverAmount;          // Quantité de couverture par la queue (0-1)
    float m_layDownAmount;            // 0 = assis, 1 = allongé/lové
    float m_peekAmount;               // 0 = caché, 1 = il peek par-dessus
    bool m_passwordFieldFocused;      // Le champ password a le focus
    float m_catBodyRotation;          // Rotation du corps vers le curseur
    
    // Méthodes de rendu privées
    
    /**
     * @brief Affiche l'écran de connexion
     */
    void RenderLoginScreen();
    
    /**
     * @brief Affiche l'interface principale de chat
     */
    void RenderChatInterface();
    
    /**
     * @brief Affiche la barre latérale avec la liste des salons
     */
    void RenderSidebar();
    
    /**
     * @brief Affiche la zone de messages du salon actif
     */
    void RenderMessageArea();
    
    /**
     * @brief Affiche un message individuel
     * @param message Message à afficher
     */
    void RenderMessage(const Message& message);
    
    /**
     * @brief Affiche la zone de saisie de message
     */
    void RenderInputArea();
    
    /**
     * @brief Affiche la barre de titre avec les infos du salon
     */
    void RenderTitleBar();
    
    /**
     * @brief Affiche un GIF animé
     * @param name Nom du GIF
     * @param maxWidth Largeur max
     * @param maxHeight Hauteur max
     */
    void RenderGif(const std::string& name, float maxWidth, float maxHeight);
    
    /**
     * @brief Dessine un fond animé avec des étoiles/particules
     */
    void RenderAnimatedBackground();
    
    /**
     * @brief Charge les GIFs depuis internet
     */
    void LoadCatGifs();
    
    /**
     * @brief Dessine le chat interactif qui suit le curseur
     * @param centerX Position X du centre du chat
     * @param centerY Position Y du centre du chat
     * @param size Taille du chat
     */
    void RenderInteractiveCat(float centerX, float centerY, float size);
    
    /**
     * @brief Met à jour l'animation du chat
     */
    void UpdateCatAnimation();
};

#endif // CHAT_WINDOW_H


