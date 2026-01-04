/**
 * @file matrix_client.cpp
 * @brief Implémentation du client Matrix
 * 
 * Ce fichier contient l'implémentation des méthodes de la classe MatrixClient.
 * Il utilise WinHTTP pour les requêtes HTTPS vers le serveur Matrix.
 * 
 * Auteur: Étudiant en Master Cybersécurité
 * Date: Janvier 2026
 */

#include <windows.h>
#include <winhttp.h>

// Désactivation des macros Windows conflictuelles
// SendMessage est une macro définie dans windows.h qui interfère avec notre méthode
#ifdef SendMessage
#undef SendMessage
#endif

#include "matrix_client.h"
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>

using json = nlohmann::json;

// URL par défaut du serveur Matrix
// Utilisation du tunnel SSH : localhost:8008 -> serveur Matrix
static const std::string DEFAULT_HOMESERVER = "http://localhost:8008";

/**
 * @brief Constructeur - Initialise le client avec les valeurs par défaut
 */
MatrixClient::MatrixClient()
    : m_homeserver(DEFAULT_HOMESERVER)
    , m_isLoggedIn(false)
    , m_isSyncing(false)
    , m_stopSync(false)
{
}

/**
 * @brief Destructeur - Arrête la synchronisation et libère les ressources
 */
MatrixClient::~MatrixClient()
{
    StopSync();
}

/**
 * @brief Tente de se connecter avec les identifiants fournis
 * 
 * Cette méthode utilise l'API /_matrix/client/v3/login pour s'authentifier.
 * Le format du nom d'utilisateur peut être :
 * - Complet : @utilisateur:serveur
 * - Simple : utilisateur (le serveur par défaut sera utilisé)
 */
bool MatrixClient::Login(const std::string& username, const std::string& password)
{
    // Préparation du nom d'utilisateur
    std::string user = username;
    if (user.empty() || password.empty())
    {
        m_lastError = "Nom d'utilisateur ou mot de passe vide";
        return false;
    }

    // Si le nom d'utilisateur ne commence pas par @, on le complète
    if (user[0] != '@')
    {
        // Le domaine Matrix est buffertavern.com (même si on utilise localhost pour le tunnel)
        std::string domain = "buffertavern.com";
        user = "@" + user + ":" + domain;
    }

    // Construction de la requête de login
    json loginRequest = {
        {"type", "m.login.password"},
        {"identifier", {
            {"type", "m.id.user"},
            {"user", user}
        }},
        {"password", password},
        {"initial_device_display_name", "Kitty Chat C++"}
    };

    std::string response;
    bool success = HttpRequest("POST", "/_matrix/client/v3/login", 
                               loginRequest.dump(), response);

    if (!success)
    {
        m_lastError = "Erreur de connexion au serveur";
        return false;
    }

    try
    {
        // Analyse de la réponse JSON
        json loginResponse = json::parse(response);

        if (loginResponse.contains("errcode"))
        {
            // Erreur retournée par le serveur
            m_lastError = loginResponse.value("error", "Erreur inconnue");
            return false;
        }

        // Extraction des informations de connexion
        m_accessToken = loginResponse.value("access_token", "");
        m_userId = loginResponse.value("user_id", user);
        m_deviceId = loginResponse.value("device_id", "");

        if (m_accessToken.empty())
        {
            m_lastError = "Token d'accès non reçu";
            return false;
        }

        m_isLoggedIn = true;
        m_lastError.clear();

        // Démarrage de la synchronisation
        StartSync();

        return true;
    }
    catch (const json::exception& e)
    {
        m_lastError = std::string("Erreur de parsing JSON: ") + e.what();
        return false;
    }
}

/**
 * @brief Déconnecte l'utilisateur et nettoie les données
 */
void MatrixClient::Logout()
{
    StopSync();

    if (m_isLoggedIn && !m_accessToken.empty())
    {
        // Appel à l'API de logout (optionnel, on ignore les erreurs)
        std::string response;
        HttpRequest("POST", "/_matrix/client/v3/logout", "{}", response);
    }

    // Nettoyage des données
    m_accessToken.clear();
    m_userId.clear();
    m_deviceId.clear();
    m_syncToken.clear();
    m_isLoggedIn = false;

    std::lock_guard<std::mutex> lock(m_roomsMutex);
    m_rooms.clear();
    m_selectedRoomId.clear();
}

/**
 * @brief Sélectionne un salon comme actif
 */
void MatrixClient::SelectRoom(const std::string& roomId)
{
    std::lock_guard<std::mutex> lock(m_roomsMutex);
    m_selectedRoomId = roomId;

    // Remise à zéro du compteur de messages non lus
    for (auto& room : m_rooms)
    {
        if (room.id == roomId)
        {
            room.unreadCount = 0;
            break;
        }
    }
}

/**
 * @brief Retourne le salon actuellement sélectionné
 */
const Room* MatrixClient::GetSelectedRoom() const
{
    std::lock_guard<std::mutex> lock(m_roomsMutex);
    for (const auto& room : m_rooms)
    {
        if (room.id == m_selectedRoomId)
        {
            return &room;
        }
    }
    return nullptr;
}

/**
 * @brief Envoie un message texte dans le salon actif
 */
bool MatrixClient::SendMessage(const std::string& message)
{
    if (!m_isLoggedIn || m_selectedRoomId.empty() || message.empty())
    {
        return false;
    }

    // Construction du contenu du message
    json msgContent = {
        {"msgtype", "m.text"},
        {"body", message}
    };

    // Génération d'un identifiant de transaction unique
    std::string txnId = GenerateTransactionId();

    // Construction de l'endpoint avec l'ID de transaction
    std::string endpoint = "/_matrix/client/v3/rooms/" + m_selectedRoomId + 
                          "/send/m.room.message/" + txnId;

    std::string response;
    bool success = HttpRequest("PUT", endpoint, msgContent.dump(), response);

    if (!success)
    {
        m_lastError = "Erreur lors de l'envoi du message";
        return false;
    }

    try
    {
        json sendResponse = json::parse(response);
        if (sendResponse.contains("errcode"))
        {
            m_lastError = sendResponse.value("error", "Erreur d'envoi");
            return false;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

/**
 * @brief Démarre le thread de synchronisation
 */
void MatrixClient::StartSync()
{
    if (m_isSyncing)
        return;

    m_stopSync = false;
    m_isSyncing = true;
    m_syncThread = std::thread(&MatrixClient::SyncLoop, this);
}

/**
 * @brief Arrête le thread de synchronisation
 */
void MatrixClient::StopSync()
{
    if (!m_isSyncing)
        return;

    m_stopSync = true;
    if (m_syncThread.joinable())
    {
        m_syncThread.join();
    }
    m_isSyncing = false;
}

/**
 * @brief Boucle de synchronisation exécutée dans un thread séparé
 * 
 * Cette boucle appelle régulièrement l'API /sync pour récupérer
 * les nouveaux événements (messages, changements de salon, etc.)
 */
void MatrixClient::SyncLoop()
{
    while (!m_stopSync && m_isLoggedIn)
    {
        // Construction de l'URL de sync
        std::string endpoint = "/_matrix/client/v3/sync?timeout=30000";
        
        if (!m_syncToken.empty())
        {
            endpoint += "&since=" + m_syncToken;
        }
        else
        {
            // Première synchronisation : limiter l'historique
            endpoint += "&filter={\"room\":{\"timeline\":{\"limit\":50}}}";
        }

        std::string response;
        bool success = HttpRequest("GET", endpoint, "", response);

        if (success && !response.empty())
        {
            ProcessSyncResponse(response);
        }

        // Pause courte entre les syncs (le timeout de 30s sur le serveur
        // fait office de long polling)
        if (!m_stopSync)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

/**
 * @brief Traite la réponse de synchronisation
 * 
 * Parse le JSON de réponse et met à jour les salons et messages
 */
void MatrixClient::ProcessSyncResponse(const std::string& syncResponse)
{
    try
    {
        json sync = json::parse(syncResponse);

        // Mise à jour du token de sync pour la prochaine requête
        if (sync.contains("next_batch"))
        {
            m_syncToken = sync["next_batch"].get<std::string>();
        }

        // Traitement des salons
        if (sync.contains("rooms") && sync["rooms"].contains("join"))
        {
            std::lock_guard<std::mutex> lock(m_roomsMutex);

            for (auto& [roomId, roomData] : sync["rooms"]["join"].items())
            {
                // Recherche du salon existant ou création
                Room* room = nullptr;
                for (auto& r : m_rooms)
                {
                    if (r.id == roomId)
                    {
                        room = &r;
                        break;
                    }
                }

                if (!room)
                {
                    // Nouveau salon
                    m_rooms.push_back({});
                    room = &m_rooms.back();
                    room->id = roomId;
                    room->name = roomId; // Nom par défaut
                    room->unreadCount = 0;
                }

                // Mise à jour du nom depuis l'état du salon
                if (roomData.contains("state") && roomData["state"].contains("events"))
                {
                    for (const auto& event : roomData["state"]["events"])
                    {
                        if (event.value("type", "") == "m.room.name")
                        {
                            room->name = event["content"].value("name", room->name);
                        }
                        else if (event.value("type", "") == "m.room.topic")
                        {
                            room->topic = event["content"].value("topic", "");
                        }
                    }
                }

                // Traitement des événements de timeline (messages)
                if (roomData.contains("timeline") && roomData["timeline"].contains("events"))
                {
                    for (const auto& event : roomData["timeline"]["events"])
                    {
                        std::string eventType = event.value("type", "");

                        // Mise à jour du nom si présent dans la timeline
                        if (eventType == "m.room.name")
                        {
                            room->name = event["content"].value("name", room->name);
                        }
                        // Traitement des messages
                        else if (eventType == "m.room.message")
                        {
                            Message msg;
                            msg.id = event.value("event_id", "");
                            msg.sender = event.value("sender", "");
                            msg.content = event["content"].value("body", "");
                            msg.isOwn = (msg.sender == m_userId);

                            // Extraction du nom d'affichage depuis le sender
                            size_t colonPos = msg.sender.find(':');
                            if (colonPos != std::string::npos && msg.sender[0] == '@')
                            {
                                msg.senderName = msg.sender.substr(1, colonPos - 1);
                            }
                            else
                            {
                                msg.senderName = msg.sender;
                            }

                            // Formatage du timestamp
                            if (event.contains("origin_server_ts"))
                            {
                                long long ts = event["origin_server_ts"].get<long long>();
                                time_t time = static_cast<time_t>(ts / 1000);
                                struct tm* tm = localtime(&time);
                                char buffer[32];
                                strftime(buffer, sizeof(buffer), "%H:%M", tm);
                                msg.timestamp = buffer;
                            }

                            room->messages.push_back(msg);

                            // Incrémenter le compteur si ce n'est pas le salon actif
                            if (room->id != m_selectedRoomId && !msg.isOwn)
                            {
                                room->unreadCount++;
                            }
                        }
                    }
                }
            }
        }

        // Notification de mise à jour
        if (m_updateCallback)
        {
            m_updateCallback();
        }
    }
    catch (const json::exception& e)
    {
        m_lastError = std::string("Erreur de parsing sync: ") + e.what();
    }
}

/**
 * @brief Effectue une requête HTTP vers l'API Matrix
 * 
 * Utilise WinHTTP pour les connexions HTTP/HTTPS
 * Supporte les URLs http:// et https://
 */
bool MatrixClient::HttpRequest(const std::string& method, const std::string& endpoint,
                               const std::string& body, std::string& response)
{
    // Analyse de l'URL du serveur pour extraire host, port et protocole
    std::string url = m_homeserver;
    bool useHttps = true;
    std::string host = "localhost";
    int port = 443;
    
    // Détection du protocole
    if (url.find("http://") == 0)
    {
        useHttps = false;
        url = url.substr(7);  // Enlever "http://"
        port = 80;
    }
    else if (url.find("https://") == 0)
    {
        useHttps = true;
        url = url.substr(8);  // Enlever "https://"
        port = 443;
    }
    
    // Extraction du host et du port
    size_t colonPos = url.find(':');
    size_t slashPos = url.find('/');
    
    if (colonPos != std::string::npos && (slashPos == std::string::npos || colonPos < slashPos))
    {
        host = url.substr(0, colonPos);
        size_t portEnd = (slashPos != std::string::npos) ? slashPos : url.length();
        port = std::stoi(url.substr(colonPos + 1, portEnd - colonPos - 1));
    }
    else if (slashPos != std::string::npos)
    {
        host = url.substr(0, slashPos);
    }
    else
    {
        host = url;
    }
    
    // Conversion en wide string pour WinHTTP
    std::wstring wHost(host.begin(), host.end());
    
    // Ouverture d'une session WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"KittyChat/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
    {
        m_lastError = "Impossible d'ouvrir la session HTTP";
        return false;
    }

    // Connexion au serveur avec le port approprié
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        wHost.c_str(),
        static_cast<INTERNET_PORT>(port),
        0
    );

    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        m_lastError = "Impossible de se connecter au serveur";
        return false;
    }

    // Conversion de l'endpoint en wide string
    std::wstring wEndpoint(endpoint.begin(), endpoint.end());
    std::wstring wMethod(method.begin(), method.end());

    // Création de la requête (avec ou sans HTTPS)
    DWORD flags = useHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        wMethod.c_str(),
        wEndpoint.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!hRequest)
    {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        m_lastError = "Impossible de créer la requête";
        return false;
    }

    // Ajout des headers
    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!m_accessToken.empty())
    {
        std::wstring authToken(m_accessToken.begin(), m_accessToken.end());
        headers += L"Authorization: Bearer " + authToken + L"\r\n";
    }

    WinHttpAddRequestHeaders(hRequest, headers.c_str(), -1, WINHTTP_ADDREQ_FLAG_ADD);

    // Envoi de la requête
    BOOL bResults = WinHttpSendRequest(
        hRequest,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        (LPVOID)body.c_str(),
        static_cast<DWORD>(body.length()),
        static_cast<DWORD>(body.length()),
        0
    );

    if (!bResults)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        m_lastError = "Erreur lors de l'envoi de la requête";
        return false;
    }

    // Réception de la réponse
    bResults = WinHttpReceiveResponse(hRequest, nullptr);

    if (!bResults)
    {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        m_lastError = "Erreur lors de la réception de la réponse";
        return false;
    }

    // Lecture du corps de la réponse
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    response.clear();

    do
    {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
            break;

        if (dwSize == 0)
            break;

        std::vector<char> buffer(dwSize + 1);
        if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded))
            break;

        buffer[dwDownloaded] = '\0';
        response += buffer.data();

    } while (dwSize > 0);

    // Nettoyage
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return true;
}

/**
 * @brief Génère un identifiant de transaction unique
 * 
 * Utilisé pour l'envoi de messages afin d'éviter les doublons
 */
std::string MatrixClient::GenerateTransactionId()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;
    ss << std::hex;

    // Ajout du timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    ss << timestamp << "-";

    // Ajout de caractères aléatoires
    for (int i = 0; i < 8; i++)
    {
        ss << dis(gen);
    }

    return ss.str();
}

/**
 * @brief Retourne l'état de connexion sous forme de texte
 */
std::string MatrixClient::GetConnectionStatus() const
{
    if (!m_isLoggedIn)
        return "Endormi zzZ";
    if (m_isSyncing)
        return "Eveille et curieux ~(=^.^)";
    return "Ronronne doucement";
}

