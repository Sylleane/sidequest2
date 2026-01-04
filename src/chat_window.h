/**
 * @file chat_window.h
 * @brief Déclaration de la fenêtre de chat principale
 * 
 * Ce fichier définit la classe ChatWindow qui gère l'interface utilisateur
 * du client de chat : écran de connexion, liste des salons, zone de messages.
 * 
 * Auteur: Étudiant en Master Cybersécurité
 * Date: Janvier 2026
 */

#ifndef CHAT_WINDOW_H
#define CHAT_WINDOW_H

#include "matrix_client.h"
#include <string>

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
     */
    explicit ChatWindow(MatrixClient* client);
    
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
    MatrixClient* m_client;     // Référence vers le client Matrix
    
    // État de l'écran de connexion
    char m_username[256];       // Buffer pour le nom d'utilisateur
    char m_password[256];       // Buffer pour le mot de passe
    bool m_showPassword;        // Afficher/masquer le mot de passe
    bool m_loginError;          // Indicateur d'erreur de connexion
    std::string m_errorMessage; // Message d'erreur à afficher
    
    // État de la zone de chat
    char m_messageInput[4096];  // Buffer pour le message à envoyer
    bool m_scrollToBottom;      // Défiler vers le bas automatiquement
    
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
};

#endif // CHAT_WINDOW_H

