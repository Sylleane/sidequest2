# 📚 Rapport Technique - Partie 3 : Protocole, Sécurité et Conclusion

---

# 4. Protocole Matrix - Détails Techniques

## 4.1 Vue d'Ensemble du Protocole

Matrix est un protocole de communication ouvert qui définit des APIs REST pour la messagerie en temps réel.

### 4.1.1 Concepts Fondamentaux

| Concept | Description | Exemple |
|---------|-------------|---------|
| **Homeserver** | Serveur hébergeant les comptes et données | vault.buffertavern.com |
| **User ID** | Identifiant unique d'un utilisateur | @kitty:vault.buffertavern.com |
| **Room ID** | Identifiant unique d'un salon | !abc123:vault.buffertavern.com |
| **Event** | Unité de donnée (message, état, etc.) | m.room.message |
| **Device ID** | Identifiant d'un appareil connecté | ABCDEFGH |
| **Access Token** | Jeton d'authentification | syt_abc123... |

### 4.1.2 Format des Identifiants

```
User ID:   @<localpart>:<server_name>
           @kitty:vault.buffertavern.com

Room ID:   !<opaque_id>:<server_name>
           !HGjFKDWaGpJsGxXt:vault.buffertavern.com

Event ID:  $<opaque_id>
           $15323758_123:vault.buffertavern.com

Alias:     #<alias>:<server_name>
           #general:vault.buffertavern.com
```

## 4.2 API Client-Server Détaillée

### 4.2.1 Authentification - POST /_matrix/client/v3/login

**Requête :**
```http
POST /_matrix/client/v3/login HTTP/1.1
Host: vault.buffertavern.com
Content-Type: application/json

{
    "type": "m.login.password",
    "identifier": {
        "type": "m.id.user",
        "user": "kitty"
    },
    "password": "meow123",
    "initial_device_display_name": "Kitty Chat C++"
}
```

**Réponse (succès) :**
```http
HTTP/1.1 200 OK
Content-Type: application/json

{
    "user_id": "@kitty:vault.buffertavern.com",
    "access_token": "syt_a2l0dHk_XyQzY3JldF9UT0tFTg_1a2B3c",
    "device_id": "ABCDEFGHIJ",
    "home_server": "vault.buffertavern.com",
    "well_known": {
        "m.homeserver": {
            "base_url": "https://vault.buffertavern.com"
        }
    }
}
```

**Réponse (échec) :**
```http
HTTP/1.1 403 Forbidden
Content-Type: application/json

{
    "errcode": "M_FORBIDDEN",
    "error": "Invalid username or password"
}
```

### 4.2.2 Inscription - POST /_matrix/client/v3/register

**Requête :**
```http
POST /_matrix/client/v3/register HTTP/1.1
Host: vault.buffertavern.com
Content-Type: application/json

{
    "username": "newuser",
    "password": "securepassword123",
    "auth": {
        "type": "m.login.dummy"
    },
    "initial_device_display_name": "Kitty Chat C++"
}
```

**Types d'authentification possibles :**

| Type | Description |
|------|-------------|
| m.login.dummy | Aucune vérification (dev/test) |
| m.login.recaptcha | Vérification reCAPTCHA |
| m.login.email.identity | Vérification par email |
| m.login.msisdn | Vérification par SMS |
| m.login.token | Token pré-partagé |

**Réponse (succès) :**
```json
{
    "user_id": "@newuser:vault.buffertavern.com",
    "access_token": "syt_newtoken...",
    "device_id": "NEWDEVICE"
}
```

**Réponse (flow incomplet) :**
```json
{
    "session": "abc123",
    "flows": [
        {
            "stages": ["m.login.recaptcha", "m.login.email.identity"]
        },
        {
            "stages": ["m.login.dummy"]
        }
    ],
    "params": {
        "m.login.recaptcha": {
            "public_key": "6LcXXXXXXXXXXXXXXXXXXXXXXXXXX"
        }
    }
}
```

### 4.2.3 Synchronisation - GET /_matrix/client/v3/sync

C'est l'endpoint le plus important : il récupère tous les événements depuis le dernier point de synchronisation.

**Requête :**
```http
GET /_matrix/client/v3/sync?timeout=30000&since=s123_456_789 HTTP/1.1
Host: vault.buffertavern.com
Authorization: Bearer syt_access_token...
```

**Paramètres :**

| Paramètre | Type | Description |
|-----------|------|-------------|
| timeout | int | Durée max d'attente (ms) pour long-polling |
| since | string | Token de sync précédent (vide = sync initial) |
| filter | string | Filtre JSON pour limiter les données |
| full_state | bool | Récupérer l'état complet des rooms |
| set_presence | string | online, offline, unavailable |

**Réponse (structure complète) :**
```json
{
    "next_batch": "s123_456_790",
    
    "rooms": {
        "join": {
            "!roomid:server": {
                "state": {
                    "events": [
                        {
                            "type": "m.room.name",
                            "state_key": "",
                            "sender": "@admin:server",
                            "content": {
                                "name": "Salon Principal"
                            },
                            "origin_server_ts": 1704067200000
                        },
                        {
                            "type": "m.room.topic",
                            "state_key": "",
                            "content": {
                                "topic": "Discussion générale"
                            }
                        }
                    ]
                },
                "timeline": {
                    "events": [
                        {
                            "type": "m.room.message",
                            "event_id": "$event123:server",
                            "sender": "@user:server",
                            "origin_server_ts": 1704067200000,
                            "content": {
                                "msgtype": "m.text",
                                "body": "Bonjour tout le monde !"
                            }
                        }
                    ],
                    "limited": false,
                    "prev_batch": "s123_456_788"
                },
                "ephemeral": {
                    "events": [
                        {
                            "type": "m.typing",
                            "content": {
                                "user_ids": ["@someone:server"]
                            }
                        }
                    ]
                },
                "account_data": {
                    "events": []
                },
                "unread_notifications": {
                    "notification_count": 2,
                    "highlight_count": 0
                }
            }
        },
        "invite": {},
        "leave": {}
    },
    
    "presence": {
        "events": [
            {
                "type": "m.presence",
                "sender": "@friend:server",
                "content": {
                    "presence": "online",
                    "last_active_ago": 5000
                }
            }
        ]
    },
    
    "account_data": {
        "events": []
    },
    
    "to_device": {
        "events": []
    }
}
```

### 4.2.4 Envoi de Message - PUT /_matrix/client/v3/rooms/{roomId}/send/{eventType}/{txnId}

**Requête :**
```http
PUT /_matrix/client/v3/rooms/!roomid:server/send/m.room.message/txn123 HTTP/1.1
Host: vault.buffertavern.com
Authorization: Bearer syt_access_token...
Content-Type: application/json

{
    "msgtype": "m.text",
    "body": "Mon message"
}
```

**Types de messages (msgtype) :**

| Type | Description | Contenu supplémentaire |
|------|-------------|------------------------|
| m.text | Message texte simple | body |
| m.emote | Action (/me) | body |
| m.notice | Notification/bot | body |
| m.image | Image | url, info (mimetype, size, w, h) |
| m.file | Fichier | url, filename, info |
| m.audio | Audio | url, info (duration) |
| m.video | Vidéo | url, info (duration, w, h) |
| m.location | Position GPS | geo_uri, info |

**Exemple message formaté (Markdown) :**
```json
{
    "msgtype": "m.text",
    "body": "Message en **gras** et *italique*",
    "format": "org.matrix.custom.html",
    "formatted_body": "Message en <strong>gras</strong> et <em>italique</em>"
}
```

**Réponse :**
```json
{
    "event_id": "$1234567890:vault.buffertavern.com"
}
```

### 4.2.5 Création de Room - POST /_matrix/client/v3/createRoom

**Requête :**
```http
POST /_matrix/client/v3/createRoom HTTP/1.1
Host: vault.buffertavern.com
Authorization: Bearer syt_access_token...
Content-Type: application/json

{
    "name": "Mon Nouveau Salon",
    "topic": "Description du salon",
    "preset": "public_chat",
    "room_alias_name": "monsalon",
    "visibility": "private",
    "invite": ["@user2:server", "@user3:server"],
    "initial_state": [
        {
            "type": "m.room.guest_access",
            "state_key": "",
            "content": {
                "guest_access": "can_join"
            }
        }
    ],
    "creation_content": {
        "m.federate": true
    }
}
```

**Presets disponibles :**

| Preset | Description |
|--------|-------------|
| private_chat | Invitation requise, chiffrement E2E |
| trusted_private_chat | Privé, tous les membres = admins |
| public_chat | Ouvert, pas de chiffrement |

**Réponse :**
```json
{
    "room_id": "!abcdefgh:vault.buffertavern.com",
    "room_alias": "#monsalon:vault.buffertavern.com"
}
```

### 4.2.6 Rejoindre une Room - POST /_matrix/client/v3/join/{roomIdOrAlias}

**Requête :**
```http
POST /_matrix/client/v3/join/%23general%3Avault.buffertavern.com HTTP/1.1
Host: vault.buffertavern.com
Authorization: Bearer syt_access_token...
Content-Type: application/json

{}
```

**Note :** Les caractères spéciaux doivent être URL-encodés :
- `#` → `%23`
- `:` → `%3A`
- `!` → `%21`

**Réponse :**
```json
{
    "room_id": "!roomid:vault.buffertavern.com"
}
```

### 4.2.7 Déconnexion - POST /_matrix/client/v3/logout

**Requête :**
```http
POST /_matrix/client/v3/logout HTTP/1.1
Host: vault.buffertavern.com
Authorization: Bearer syt_access_token...
Content-Type: application/json

{}
```

**Réponse :**
```json
{}
```

L'access token devient invalide après cette requête.

## 4.3 Codes d'Erreur Matrix

| Code | Description |
|------|-------------|
| M_FORBIDDEN | Accès refusé |
| M_UNKNOWN_TOKEN | Token invalide/expiré |
| M_MISSING_TOKEN | Token manquant |
| M_BAD_JSON | JSON malformé |
| M_NOT_JSON | Content-Type incorrect |
| M_NOT_FOUND | Ressource introuvable |
| M_LIMIT_EXCEEDED | Rate limiting |
| M_USER_IN_USE | Username déjà pris |
| M_INVALID_USERNAME | Username invalide |
| M_ROOM_IN_USE | Room alias déjà utilisé |
| M_INVALID_ROOM_STATE | État de room invalide |
| M_THREEPID_IN_USE | Email/téléphone déjà utilisé |
| M_UNKNOWN | Erreur inconnue |

---

# 5. Sécurité et Bonnes Pratiques

## 5.1 Sécurité Côté Serveur

### 5.1.1 Architecture Zero-Trust avec Cloudflare Tunnel

```
┌─────────────────────────────────────────────────────────────────┐
│                    MODÈLE DE SÉCURITÉ                           │
└─────────────────────────────────────────────────────────────────┘

Internet ──►  Cloudflare  ──►  Tunnel (chiffré)  ──►  Serveur
              │                                       │
              ├─ WAF (Web Application Firewall)       ├─ Nginx (localhost only)
              ├─ DDoS Protection                      ├─ Synapse (localhost only)
              ├─ Bot Management                       └─ Firewall: DROP all INPUT
              ├─ SSL/TLS Termination                     sauf localhost
              └─ Rate Limiting
```

**Avantages :**
- **Aucun port ouvert** : Le serveur n'accepte aucune connexion entrante directe
- **Protection DDoS** : Cloudflare absorbe les attaques
- **WAF gratuit** : Filtrage des requêtes malveillantes
- **SSL automatique** : Certificats gérés par Cloudflare

### 5.1.2 Configuration Firewall

```bash
# UFW (Uncomplicated Firewall)
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow ssh  # Important : ne pas se bloquer !
sudo ufw enable

# Vérification
sudo ufw status
# Status: active
# To                         Action      From
# --                         ------      ----
# 22/tcp                     ALLOW       Anywhere
```

### 5.1.3 Hardening Synapse

Dans `/etc/matrix-synapse/homeserver.yaml` :

```yaml
# Désactiver la fédération si non nécessaire
# (réduit la surface d'attaque)
federation_domain_whitelist: []

# Limiter les inscriptions
enable_registration: false  # Ou avec token
registration_shared_secret: "un_secret_tres_long_et_aleatoire"

# Rate limiting
rc_message:
  per_second: 0.2
  burst_count: 10

rc_login:
  address:
    per_second: 0.17
    burst_count: 3
  account:
    per_second: 0.17
    burst_count: 3

# Taille max des uploads
max_upload_size: 10M

# Durée des sessions
session_lifetime: 24h
refresh_token_lifetime: 1w

# Logs
log_config: "/etc/matrix-synapse/log.yaml"
```

### 5.1.4 Mises à Jour Automatiques

```bash
# Activer les mises à jour de sécurité automatiques
sudo apt install unattended-upgrades
sudo dpkg-reconfigure -plow unattended-upgrades
```

## 5.2 Sécurité Côté Client

### 5.2.1 Gestion des Tokens

```cpp
class MatrixClient {
private:
    // Token en mémoire uniquement, jamais sur le disque
    std::string m_accessToken;
    
    // Effacement sécurisé à la déconnexion
    void SecureClear(std::string& str) {
        volatile char* p = &str[0];
        size_t len = str.size();
        while (len--) {
            *p++ = 0;
        }
        str.clear();
    }
    
public:
    void Logout() {
        // Appel API
        HttpRequest("POST", "/_matrix/client/v3/logout", "{}", response);
        
        // Effacement sécurisé des données sensibles
        SecureClear(m_accessToken);
        SecureClear(m_userId);
        m_isLoggedIn = false;
    }
};
```

### 5.2.2 Validation des Entrées

```cpp
bool ValidateUsername(const std::string& username) {
    // Longueur raisonnable
    if (username.length() < 3 || username.length() > 32) {
        return false;
    }
    
    // Caractères autorisés uniquement
    for (char c : username) {
        if (!isalnum(c) && c != '_' && c != '-' && c != '.') {
            return false;
        }
    }
    
    return true;
}

bool ValidateMessage(const std::string& message) {
    // Pas vide
    if (message.empty()) return false;
    
    // Taille raisonnable
    if (message.length() > 10000) return false;
    
    return true;
}
```

### 5.2.3 Protection contre les Injections

```cpp
// Échappement pour l'affichage (évite XSS si jamais rendu en HTML)
std::string EscapeForDisplay(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    
    for (char c : input) {
        switch (c) {
            case '<': output += "&lt;"; break;
            case '>': output += "&gt;"; break;
            case '&': output += "&amp;"; break;
            case '"': output += "&quot;"; break;
            case '\'': output += "&#39;"; break;
            default: output += c;
        }
    }
    
    return output;
}
```

### 5.2.4 Vérification SSL/TLS

WinHTTP vérifie automatiquement les certificats SSL via le magasin Windows :

```cpp
// Forcer la vérification (déjà activée par défaut)
DWORD flags = SECURITY_FLAG_IGNORE_UNKNOWN_CA;
WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));

// En production, ne JAMAIS ignorer les erreurs SSL !
// Le code ci-dessus est uniquement pour le debug
```

## 5.3 Chiffrement de Bout en Bout (E2EE)

Matrix supporte le chiffrement E2EE via les bibliothèques Olm et Megolm. Ce n'est pas implémenté dans ce projet, mais voici le principe :

### 5.3.1 Protocole

```
┌─────────────┐                              ┌─────────────┐
│   Alice     │                              │    Bob      │
│  (Device A) │                              │  (Device B) │
└──────┬──────┘                              └──────┬──────┘
       │                                            │
       │  1. Échange de clés Olm (1-to-1)          │
       │ ◄─────────────────────────────────────────►│
       │                                            │
       │  2. Établissement session Megolm (group)  │
       │ ◄─────────────────────────────────────────►│
       │                                            │
       │  3. Messages chiffrés avec clé de session │
       │ ─────────────────────────────────────────► │
       │                                            │
```

### 5.3.2 Implémentation (Référence)

```cpp
// Pseudo-code - nécessite libolm
#include <olm/olm.h>

class E2EEManager {
    OlmAccount* m_account;
    std::map<std::string, OlmSession*> m_sessions;
    
public:
    void Initialize() {
        // Création du compte Olm
        size_t accountSize = olm_account_size();
        m_account = (OlmAccount*)malloc(accountSize);
        olm_account(m_account);
        
        // Génération des clés
        size_t randomLength = olm_create_account_random_length(m_account);
        std::vector<uint8_t> random(randomLength);
        // Remplir random avec des données aléatoires cryptographiques
        
        olm_create_account(m_account, random.data(), randomLength);
    }
    
    std::string Encrypt(const std::string& roomId, const std::string& message) {
        // Récupérer la session Megolm pour cette room
        // Chiffrer le message
        // Retourner le ciphertext
    }
    
    std::string Decrypt(const std::string& roomId, const std::string& ciphertext) {
        // Récupérer la session Megolm
        // Déchiffrer
        // Retourner le plaintext
    }
};
```

---

# 6. Difficultés Rencontrées et Solutions

## 6.1 Problèmes Backend

### 6.1.1 Tunnel Cloudflare Non Actif

**Symptôme :** Erreur 1033 côté client

**Diagnostic :**
```bash
# Vérifier si cloudflared tourne
ps aux | grep cloudflared

# Vérifier les logs
journalctl -u cloudflared -f
```

**Solution :**
```bash
# Relancer le tunnel
cloudflared tunnel run matrix

# Ou via systemd
sudo systemctl restart cloudflared
```

### 6.1.2 Conflits Nginx

**Symptôme :** `conflicting server name` dans les logs

**Diagnostic :**
```bash
# Lister tous les fichiers de config
ls -la /etc/nginx/sites-enabled/

# Chercher les doublons
grep -r "server_name" /etc/nginx/sites-enabled/
```

**Solution :**
```bash
# Supprimer les configs en double
sudo rm /etc/nginx/sites-enabled/default
sudo rm /etc/nginx/sites-enabled/old-config

# Recharger
sudo nginx -t && sudo systemctl reload nginx
```

### 6.1.3 Synapse Refuse les Inscriptions

**Symptôme :** `M_FORBIDDEN` lors du register

**Solution :**
```yaml
# Dans homeserver.yaml
enable_registration: true
enable_registration_without_verification: true
```

```bash
sudo systemctl restart matrix-synapse
```

## 6.2 Problèmes Frontend

### 6.2.1 CMake Non Trouvé

**Symptôme :** `cmake is not recognized`

**Solution dans launch.bat :**
```batch
@echo off
setlocal enabledelayedexpansion

:: Chercher CMake dans différents emplacements
set "CMAKE_PATH="

:: 1. Dans le PATH
where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "CMAKE_PATH=cmake"
    goto :found
)

:: 2. Visual Studio 2022
set "VS_CMAKE=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if exist "!VS_CMAKE!" (
    set "CMAKE_PATH=!VS_CMAKE!"
    goto :found
)

:: 3. Visual Studio 2019
set "VS_CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if exist "!VS_CMAKE!" (
    set "CMAKE_PATH=!VS_CMAKE!"
    goto :found
)

echo CMake non trouve. Installez Visual Studio avec les outils C++.
exit /b 1

:found
echo CMake trouve: !CMAKE_PATH!
```

### 6.2.2 Conflit Macro Windows SendMessage

**Symptôme :** `'SendMessageA': is not a member of 'MatrixClient'`

**Cause :** Windows définit une macro `SendMessage` → `SendMessageA/W`

**Solution :**
```cpp
// Au début de matrix_client.h
#ifdef SendMessage
#undef SendMessage
#endif

// Ou renommer la méthode
bool PostMessage(const std::string& roomId, const std::string& message);
```

### 6.2.3 Erreur Parsing JSON

**Symptôme :** `parse error at line 1 column 1`

**Cause :** Tentative de parser une réponse non-JSON (erreur HTTP, HTML)

**Solution :**
```cpp
bool MatrixClient::HttpRequest(...) {
    // Vérifier le code HTTP AVANT de parser
    if (statusCode < 200 || statusCode >= 300) {
        m_lastError = "HTTP " + std::to_string(statusCode) + 
                      ": " + response.substr(0, 100);
        return false;  // Ne pas parser
    }
    
    return true;
}

bool MatrixClient::Login(...) {
    std::string response;
    if (!HttpRequest("POST", endpoint, body, response)) {
        // m_lastError déjà défini
        return false;
    }
    
    // Maintenant on peut parser en sécurité
    try {
        json j = json::parse(response);
        // ...
    } catch (const json::exception& e) {
        m_lastError = "JSON invalide: " + std::string(e.what());
        return false;
    }
}
```

### 6.2.4 GIFs Non Chargés

**Symptôme :** Images cassées ou placeholder permanent

**Cause :** URLs Tenor invalides ou blocage réseau

**Solution :** Retour à l'ASCII art fiable
```cpp
// Pas de dépendance réseau pour l'affichage
const char* catArt = R"(
      /\_____/\
     /  o   o  \
    ( ==  ^  == )
     )         (
    (           )
   ( (  )   (  ) )
  (__(__)___(__)__)
)";
```

---

# 7. Conclusion

## 7.1 Résumé des Réalisations

Ce projet a permis de mettre en place une infrastructure complète de messagerie instantanée :

### Backend
- ✅ Serveur Matrix Synapse auto-hébergé
- ✅ Reverse proxy Nginx correctement configuré
- ✅ Exposition sécurisée via Cloudflare Tunnel
- ✅ HTTPS avec certificats valides
- ✅ Inscription et authentification fonctionnelles

### Frontend
- ✅ Application native C++ performante
- ✅ Interface graphique moderne avec Dear ImGui
- ✅ Communication HTTPS via WinHTTP
- ✅ Synchronisation temps réel des messages
- ✅ Création et gestion des salons
- ✅ Thème visuel personnalisé

## 7.2 Compétences Acquises

| Domaine | Compétences |
|---------|-------------|
| **Réseau** | Protocole HTTP/HTTPS, REST API, Long Polling, DNS |
| **Sécurité** | TLS/SSL, Zero-Trust, Firewall, Authentification |
| **Linux** | Administration système, Nginx, Systemd |
| **Windows** | Win32 API, WinHTTP, DirectX 11 |
| **C++** | Programmation moderne C++17, Multithreading |
| **Protocole** | Matrix Client-Server API |

## 7.3 Améliorations Futures

### Court Terme
- Notifications Windows (toast notifications)
- Indicateur de frappe (typing indicators)
- Réactions aux messages (emojis)
- Support des images inline

### Moyen Terme
- Chiffrement de bout en bout (libolm)
- Support multi-comptes
- Historique persistant (cache local SQLite)
- Thèmes personnalisables

### Long Terme
- Appels audio/vidéo (WebRTC)
- Version Linux (GTK ou Qt)
- Version mobile (Android/iOS)
- Fédération avec d'autres serveurs Matrix

## 7.4 Retour d'Expérience

### Points Positifs
- Matrix est un protocole bien documenté et moderne
- Cloudflare Tunnel simplifie énormément l'hébergement
- Dear ImGui est très adapté pour du prototypage rapide
- WinHTTP est robuste et bien intégré à Windows

### Difficultés
- La gestion des macros Windows (`SendMessage`) est piégeuse
- Les GIFs distants sont peu fiables (URLs qui changent)
- Le debugging réseau nécessite de bons outils (Wireshark, curl)
- La documentation Synapse peut être confuse sur certains points

### Conseils pour un Projet Similaire
1. **Commencer simple** : ASCII art avant les GIFs
2. **Tester le backend d'abord** : curl avant le client C++
3. **Logger tout** : Les messages d'erreur détaillés sauvent du temps
4. **Versionner** : Git + tags pour les releases stables

---

## Annexes

### A. Commandes Utiles

```bash
# Test rapide du serveur Matrix
curl https://vault.buffertavern.com/_matrix/client/versions

# Login via curl
curl -X POST https://vault.buffertavern.com/_matrix/client/v3/login \
    -H "Content-Type: application/json" \
    -d '{"type":"m.login.password","identifier":{"type":"m.id.user","user":"kitty"},"password":"meow123"}'

# Statut des services
sudo systemctl status nginx matrix-synapse cloudflared

# Logs en temps réel
sudo journalctl -u matrix-synapse -f
```

### B. Fichiers de Configuration

- Synapse : `/etc/matrix-synapse/homeserver.yaml`
- Nginx : `/etc/nginx/sites-available/matrix`
- Cloudflared : `~/.cloudflared/config.yml`
- Logs Synapse : `/var/log/matrix-synapse/`

### C. Ressources

- Documentation Matrix : https://spec.matrix.org/
- Dear ImGui : https://github.com/ocornut/imgui
- nlohmann/json : https://github.com/nlohmann/json
- Cloudflare Tunnel : https://developers.cloudflare.com/cloudflare-one/connections/connect-apps/

---

**Projet réalisé dans le cadre du Master Cybersécurité - Janvier 2026**
