/**
 * @file matrix_client.h
 * @brief Déclaration du client Matrix pour la communication avec le serveur
 * 
 * Ce fichier définit la classe MatrixClient qui gère toutes les interactions
 * avec le protocole Matrix : authentification, synchronisation, envoi de messages.
 */

#ifndef MATRIX_CLIENT_H
#define MATRIX_CLIENT_H

// Désactivation des macros Windows qui causent des conflits
#ifdef SendMessage
#undef SendMessage
#endif

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>

/**
 * @struct Message
 * @brief Représente un message dans un salon Matrix
 */
struct Message
{
    std::string id;         // Identifiant unique du message
    std::string sender;     // Identifiant de l'expéditeur (@user:server)
    std::string senderName; // Nom d'affichage de l'expéditeur
    std::string content;    // Contenu du message
    std::string timestamp;  // Horodatage du message
    bool isOwn;             // true si c'est notre propre message
};

/**
 * @struct Room
 * @brief Représente un salon Matrix
 */
struct Room
{
    std::string id;         // Identifiant du salon (!xxx:server)
    std::string name;       // Nom du salon
    std::string topic;      // Sujet/description du salon
    int unreadCount;        // Nombre de messages non lus
    std::vector<Message> messages;  // Messages du salon
};

/**
 * @class MatrixClient
 * @brief Client pour le protocole Matrix
 * 
 * Cette classe encapsule toute la logique de communication avec un serveur Matrix.
 * Elle utilise l'API HTTP de Matrix pour l'authentification et la synchronisation.
 */
class MatrixClient
{
public:
    /**
     * @brief Constructeur par défaut
     * 
     * Initialise le client avec le serveur par défaut (buffertavern.com)
     */
    MatrixClient();
    
    /**
     * @brief Destructeur
     * 
     * Arrête le thread de synchronisation et libère les ressources
     */
    ~MatrixClient();

    // === Méthodes d'authentification ===
    
    /**
     * @brief Tente de se connecter au serveur Matrix
     * @param username Nom d'utilisateur (avec ou sans @user:server)
     * @param password Mot de passe
     * @return true si la connexion a réussi
     */
    bool Login(const std::string& username, const std::string& password);
    
    /**
     * @brief Crée un nouveau compte sur le serveur Matrix
     * @param username Nom d'utilisateur souhaité
     * @param password Mot de passe
     * @return true si l'inscription a réussi
     */
    bool Register(const std::string& username, const std::string& password);
    
    /**
     * @brief Déconnecte l'utilisateur actuel
     */
    void Logout();
    
    /**
     * @brief Vérifie si l'utilisateur est connecté
     * @return true si connecté
     */
    bool IsLoggedIn() const { return m_isLoggedIn; }
    
    /**
     * @brief Retourne le nom d'utilisateur actuel
     * @return Identifiant Matrix complet (@user:server)
     */
    const std::string& GetUserId() const { return m_userId; }

    // === Méthodes de gestion des salons ===
    
    /**
     * @brief Retourne la liste des salons rejoints
     * @return Référence vers le vecteur de salons
     */
    const std::vector<Room>& GetRooms() const { return m_rooms; }
    
    /**
     * @brief Sélectionne un salon comme actif
     * @param roomId Identifiant du salon
     */
    void SelectRoom(const std::string& roomId);
    
    /**
     * @brief Retourne le salon actuellement sélectionné
     * @return Pointeur vers le salon ou nullptr
     */
    const Room* GetSelectedRoom() const;

    // === Méthodes de messagerie ===
    
    /**
     * @brief Envoie un message dans le salon actif
     * @param message Contenu du message à envoyer
     * @return true si l'envoi a réussi
     */
    bool SendMessage(const std::string& message);
    
    /**
     * @brief Crée un nouveau salon
     * @param name Nom du salon
     * @return true si la création a réussi
     */
    bool CreateRoom(const std::string& name);
    
    /**
     * @brief Rejoint un salon existant par son ID ou alias
     * @param roomIdOrAlias ID ou alias du salon (ex: #general:server ou !xxx:server)
     * @return true si le join a réussi
     */
    bool JoinRoom(const std::string& roomIdOrAlias);

    // === Méthodes de synchronisation ===
    
    /**
     * @brief Démarre la synchronisation en arrière-plan
     */
    void StartSync();
    
    /**
     * @brief Arrête la synchronisation
     */
    void StopSync();

    // === Callbacks pour les mises à jour ===
    
    /**
     * @brief Définit le callback appelé lors d'une mise à jour
     * @param callback Fonction à appeler
     */
    void SetUpdateCallback(std::function<void()> callback) { m_updateCallback = callback; }

    // === Getters pour l'état ===
    
    /**
     * @brief Retourne le dernier message d'erreur
     */
    const std::string& GetLastError() const { return m_lastError; }
    
    /**
     * @brief Retourne l'état de connexion sous forme de texte
     */
    std::string GetConnectionStatus() const;

private:
    // Configuration du serveur
    std::string m_homeserver;       // URL du serveur Matrix
    std::string m_accessToken;      // Token d'accès pour les requêtes
    std::string m_userId;           // Identifiant de l'utilisateur
    std::string m_deviceId;         // Identifiant de l'appareil
    
    // État de connexion
    std::atomic<bool> m_isLoggedIn;
    std::atomic<bool> m_isSyncing;
    std::string m_lastError;
    std::string m_syncToken;        // Token pour la synchronisation incrémentale
    
    // Données des salons
    std::vector<Room> m_rooms;
    std::string m_selectedRoomId;
    mutable std::mutex m_roomsMutex;
    
    // Thread de synchronisation
    std::thread m_syncThread;
    std::atomic<bool> m_stopSync;
    
    // Callback pour notifier les mises à jour
    std::function<void()> m_updateCallback;
    
    // === Méthodes internes ===
    
    /**
     * @brief Effectue une requête HTTP vers l'API Matrix
     * @param method Méthode HTTP (GET, POST, PUT)
     * @param endpoint Point de terminaison de l'API
     * @param body Corps de la requête (JSON)
     * @param response Réponse reçue
     * @return true si la requête a réussi
     */
    bool HttpRequest(const std::string& method, const std::string& endpoint,
                     const std::string& body, std::string& response);
    
    /**
     * @brief Boucle de synchronisation exécutée dans un thread séparé
     */
    void SyncLoop();
    
    /**
     * @brief Traite la réponse de synchronisation
     * @param syncResponse Réponse JSON du serveur
     */
    void ProcessSyncResponse(const std::string& syncResponse);
    
    /**
     * @brief Génère un identifiant de transaction unique
     * @return Identifiant unique
     */
    std::string GenerateTransactionId();
};

#endif // MATRIX_CLIENT_H

