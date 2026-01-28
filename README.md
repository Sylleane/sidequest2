# Kitty Chat C++ - Application de Messagerie Matrix

Client de messagerie instantanée basé sur le protocole Matrix, développé en C++ avec une interface graphique moderne utilisant Dear ImGui et DirectX 11.

---

## Table des Matières

1. [Structure du dépôt](#structure-du-dépôt)
2. [Présentation du Projet](#présentation-du-projet)
3. [Architecture Globale](#architecture-globale)
4. [Infrastructure Serveur](#infrastructure-serveur)
5. [Application Cliente](#application-cliente)
6. [Protocole Matrix](#protocole-matrix)
7. [Guide d'Installation](#guide-dinstallation)
8. [Guide d'Utilisation](#guide-dutilisation)
9. [Difficultés Rencontrées](#difficultés-rencontrées)
10. [Conclusion](#conclusion)

---

## Structure du dépôt

```
.
├── README.md              # Ce fichier (Documentation d'accueil)
├── CMakeLists.txt         # Configuration du build
├── launch.bat             # Raccourci vers cicd/launch.bat
├── src/                   # CODE SOURCE : C++, ImGui, client Matrix
├── assets/                # Ressources (fonts, icons)
├── cicd/                  # CI/CD : Scripts de build et déploiement
│   ├── launch.bat         # Compilation + lancement de l'application
│   └── check-tunnel.sh    # Vérification du tunnel Cloudflare (serveur)
├── documentation/         # DOCUMENTATION : Rapports techniques, guides
│   ├── RAPPORT_TECHNIQUE_COMPLET.md

├── presentation/          # PRÉSENTATION : Supports pour la soutenance
│   ├── KittyChat_Presentation.pptx

```

---

## Présentation du Projet

### Objectif

L'objectif de ce projet est de développer une application de messagerie instantanée complète, comprenant :
- Un **serveur Matrix** auto-hébergé pour la gestion des communications
- Un **client natif C++** avec interface graphique moderne
- Une **infrastructure sécurisée** avec chiffrement HTTPS

### Pourquoi Matrix ?

Matrix est un protocole de communication décentralisé et open-source qui offre :
- **Fédération** : Possibilité de communiquer entre différents serveurs
- **Chiffrement de bout en bout** : Sécurité des communications (E2EE)
- **API standardisée** : Documentation complète et stable
- **Interopérabilité** : Compatible avec de nombreux clients (Element, FluffyChat, etc.)

### Stack Technique

| Composant | Technologie | Justification |
|-----------|-------------|---------------|
| Serveur Matrix | Synapse (Python) | Implémentation de référence, stable et bien documentée |
| Tunnel HTTPS | Cloudflare Tunnel | Exposition sécurisée sans ouvrir de ports |
| Client | C++ / Win32 | Performance native, contrôle total |
| Interface | Dear ImGui + DirectX 11 | Rendu GPU, personnalisation complète |
| Requêtes HTTP | WinHTTP | API Windows native, support SSL/TLS intégré |
| Parsing JSON | nlohmann/json | Bibliothèque C++ moderne et performante |

---

## Architecture Globale

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ARCHITECTURE GLOBALE                            │
└─────────────────────────────────────────────────────────────────────────────┘

    ┌──────────────────┐         HTTPS          ┌──────────────────────────┐
    │                  │ ◄────────────────────► │                          │
    │   Kitty Chat     │                        │   Cloudflare Edge        │
    │   (Client C++)   │                        │   (CDN + DDoS Protection)│
    │                  │                        │                          │
    │   Windows 10/11  │                        │   vault.buffertavern.com │
    └──────────────────┘                        └────────────┬─────────────┘
                                                             │
                                                    Cloudflare Tunnel
                                                    (Connexion chiffrée)
                                                             │
                                                             ▼
                                                ┌──────────────────────────┐
                                                │   Serveur Linux          │
                                                │   (Debian/Ubuntu)        │
                                                │                          │
                                                │   ┌──────────────────┐   │
                                                │   │   cloudflared    │   │
                                                │   │   (Tunnel Agent) │   │
                                                │   └────────┬─────────┘   │
                                                │            │             │
                                                │            ▼             │
                                                │   ┌──────────────────┐   │
                                                │   │   Nginx          │   │
                                                │   │   (Reverse Proxy)│   │
                                                │   │   Port 80        │   │
                                                │   └────────┬─────────┘   │
                                                │            │             │
                                                │            ▼             │
                                                │   ┌──────────────────┐   │
                                                │   │   Synapse        │   │
                                                │   │   (Matrix Server)│   │
                                                │   │   Port 8008      │   │
                                                │   └────────┬─────────┘   │
                                                │            │             │
                                                │            ▼             │
                                                │   ┌──────────────────┐   │
                                                │   │   SQLite/PostgreSQL│  │
                                                │   │   (Base de données)│  │
                                                │   └──────────────────┘   │
                                                └──────────────────────────┘
```

---

## Infrastructure Serveur

### 1. Installation de Matrix Synapse

Matrix Synapse est l'implémentation de référence du serveur Matrix, écrite en Python.

#### 1.1 Prérequis Système

```bash
# Mise à jour du système
sudo apt update && sudo apt upgrade -y

# Installation des dépendances
sudo apt install -y build-essential python3-dev libffi-dev \
    python3-pip python3-setuptools sqlite3 libssl-dev \
    python3-venv libjpeg-dev libxslt1-dev
```

#### 1.2 Installation de Synapse

```bash
# Ajout du dépôt officiel Matrix
sudo apt install -y lsb-release wget apt-transport-https
sudo wget -O /usr/share/keyrings/matrix-org-archive-keyring.gpg \
    https://packages.matrix.org/debian/matrix-org-archive-keyring.gpg

echo "deb [signed-by=/usr/share/keyrings/matrix-org-archive-keyring.gpg] \
    https://packages.matrix.org/debian/ $(lsb_release -cs) main" | \
    sudo tee /etc/apt/sources.list.d/matrix-org.list

# Installation
sudo apt update
sudo apt install -y matrix-synapse-py3
```

#### 1.3 Configuration de Synapse

Le fichier de configuration principal est `/etc/matrix-synapse/homeserver.yaml` :

```yaml
# Nom du serveur (doit correspondre au domaine)
server_name: "vault.buffertavern.com"

# Port d'écoute local (Nginx fera le proxy)
listeners:
  - port: 8008
    type: http
    tls: false
    x_forwarded: true
    resources:
      - names: [client, federation]
        compress: false

# Base de données (SQLite pour les petites installations)
database:
  name: sqlite3
  args:
    database: /var/lib/matrix-synapse/homeserver.db

# Activation de l'inscription publique
enable_registration: true
enable_registration_without_verification: true

# Clés de signature (générées automatiquement)
signing_key_path: "/etc/matrix-synapse/homeserver.signing.key"

# Journalisation
log_config: "/etc/matrix-synapse/log.yaml"

# Média (avatars, fichiers partagés)
media_store_path: "/var/lib/matrix-synapse/media"
max_upload_size: 50M

# Fédération (communication inter-serveurs)
federation_domain_whitelist: []
```

#### 1.4 Création d'un Utilisateur Administrateur

```bash
# Création d'un utilisateur via la ligne de commande
register_new_matrix_user -c /etc/matrix-synapse/homeserver.yaml \
    http://localhost:8008 -u admin -p password123 -a
```

#### 1.5 Démarrage du Service

```bash
# Activation au démarrage
sudo systemctl enable matrix-synapse

# Démarrage
sudo systemctl start matrix-synapse

# Vérification du statut
sudo systemctl status matrix-synapse
```

---

### 2. Configuration de Nginx (Reverse Proxy)

Nginx agit comme reverse proxy pour :
- Rediriger les requêtes vers Synapse
- Gérer les en-têtes HTTP (X-Forwarded-For, etc.)
- Servir les fichiers statiques

#### 2.1 Configuration Nginx

Fichier `/etc/nginx/sites-available/matrix` :

```nginx
server {
    listen 80;
    server_name vault.buffertavern.com;

    # Proxy vers l'API Matrix Client-Server
    location /_matrix {
        proxy_pass http://127.0.0.1:8008;
        proxy_set_header X-Forwarded-For $remote_addr;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_set_header Host $host;
        
        # WebSocket support (pour la synchronisation temps réel)
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        
        # Timeouts pour les requêtes longues (sync)
        proxy_read_timeout 600s;
    }

    # Well-known pour la découverte automatique
    location /.well-known/matrix {
        proxy_pass http://127.0.0.1:8008;
        proxy_set_header X-Forwarded-For $remote_addr;
        proxy_set_header Host $host;
    }
}
```

#### 2.2 Activation du Site

```bash
sudo ln -s /etc/nginx/sites-available/matrix /etc/nginx/sites-enabled/
sudo nginx -t  # Test de la configuration
sudo systemctl reload nginx
```

---

### 3. Cloudflare Tunnel (Exposition Sécurisée)

Cloudflare Tunnel permet d'exposer le serveur sur Internet sans ouvrir de ports sur le routeur/firewall. C'est une solution idéale pour :
- **Sécurité** : Pas de ports ouverts, protection DDoS incluse
- **HTTPS automatique** : Certificat SSL géré par Cloudflare
- **Simplicité** : Pas besoin de configuration NAT/Port forwarding

#### 3.1 Installation de cloudflared

```bash
# Téléchargement du binaire
wget https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64.deb
sudo dpkg -i cloudflared-linux-amd64.deb

# Authentification (ouvre un navigateur)
cloudflared tunnel login
```

#### 3.2 Création du Tunnel

```bash
# Création d'un nouveau tunnel
cloudflared tunnel create matrix

# Configuration du tunnel
# Fichier ~/.cloudflared/config.yml
cat > ~/.cloudflared/config.yml << EOF
tunnel: <TUNNEL_ID>
credentials-file: /home/user/.cloudflared/<TUNNEL_ID>.json

ingress:
  - hostname: vault.buffertavern.com
    service: http://localhost:80
  - service: http_status:404
EOF
```

#### 3.3 Configuration DNS

Dans le dashboard Cloudflare :
1. Aller dans DNS
2. Ajouter un enregistrement CNAME :
   - Nom : `vault` (ou `@` pour le domaine racine)
   - Cible : `<TUNNEL_ID>.cfargotunnel.com`
   - Proxy : Activé (orange)

#### 3.4 Lancement du Tunnel

```bash
# Lancement manuel
cloudflared tunnel run matrix

# Ou en tant que service systemd
sudo cloudflared service install
sudo systemctl enable cloudflared
sudo systemctl start cloudflared
```

---

## Application Cliente

### 1. Structure du code (src/)

```
src/
├── main.cpp             # Point d'entrée + Init DirectX/ImGui
├── matrix_client.h      # Déclaration du client Matrix
├── matrix_client.cpp    # Implémentation API Matrix
├── chat_window.h        # Déclaration interface utilisateur
├── chat_window.cpp      # Interface graphique + animations
├── texture_manager.h    # Gestion des textures
├── texture_manager.cpp  # Chargement d'images/GIFs
└── stb_image.h          # Décodeur d'images (header-only)
```

Le dossier `build/` (généré) contient `Release/KittyChat.exe` et `_deps/` (Dear ImGui, nlohmann/json).

### 2. Système de Build (CMake)

Le fichier `CMakeLists.txt` gère :
- La récupération automatique des dépendances via `FetchContent`
- La configuration de DirectX 11
- La liaison avec les bibliothèques Windows (WinHTTP)

```cmake
cmake_minimum_required(VERSION 3.16)
project(KittyChat VERSION 2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Téléchargement des dépendances ===
include(FetchContent)

# Dear ImGui - Interface graphique
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.1
)

# nlohmann/json - Parsing JSON
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(imgui json)

# === Configuration DirectX 11 ===
find_package(DirectX REQUIRED)  # Windows SDK

# === Exécutable ===
add_executable(KittyChat WIN32
    src/main.cpp
    src/matrix_client.cpp
    src/chat_window.cpp
    src/texture_manager.cpp
    # ImGui sources
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

# === Bibliothèques ===
target_link_libraries(KittyChat PRIVATE
    d3d11          # DirectX 11
    dxgi           # DXGI (swap chain)
    d3dcompiler    # Compilation shaders
    winhttp        # Requêtes HTTPS
    nlohmann_json::nlohmann_json
)
```

### 3. Client Matrix (matrix_client.cpp)

Le client Matrix implémente l'API Client-Server v3 :

#### 3.1 Authentification

```cpp
bool MatrixClient::Login(const std::string& username, const std::string& password)
{
    // Construction de la requête JSON
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

    if (success) {
        json loginResponse = json::parse(response);
        m_accessToken = loginResponse["access_token"];
        m_userId = loginResponse["user_id"];
        m_deviceId = loginResponse["device_id"];
        m_isLoggedIn = true;
        StartSync();  // Démarre la synchronisation
    }
    return success;
}
```

#### 3.2 Synchronisation (Long Polling)

Matrix utilise le "long polling" pour la synchronisation temps réel :

```cpp
void MatrixClient::SyncLoop()
{
    while (!m_stopSync)
    {
        std::string endpoint = "/_matrix/client/v3/sync?timeout=30000";
        if (!m_syncToken.empty()) {
            endpoint += "&since=" + m_syncToken;
        }

        std::string response;
        bool success = HttpRequest("GET", endpoint, "", response);

        if (success) {
            json syncResponse = json::parse(response);
            m_syncToken = syncResponse["next_batch"];
            ProcessSyncResponse(syncResponse);
        }
    }
}
```

#### 3.3 Envoi de Messages

```cpp
bool MatrixClient::SendMessage(const std::string& roomId, const std::string& message)
{
    // Génération d'un ID de transaction unique
    std::string txnId = std::to_string(std::chrono::system_clock::now()
                                       .time_since_epoch().count());

    std::string endpoint = "/_matrix/client/v3/rooms/" + roomId +
                          "/send/m.room.message/" + txnId;

    json msgContent = {
        {"msgtype", "m.text"},
        {"body", message}
    };

    std::string response;
    return HttpRequest("PUT", endpoint, msgContent.dump(), response);
}
```

### 4. Interface Graphique (chat_window.cpp)

L'interface utilise Dear ImGui pour un rendu moderne et fluide.

#### 4.1 Écran de Connexion

L'écran de connexion affiche un chat ASCII animé qui réagit au focus :

```cpp
void ChatWindow::RenderLoginScreen()
{
    // Chat ASCII qui change selon l'état
    const char* catArt;
    
    if (m_passwordFieldFocused && m_showPassword) {
        // Le chat "peek" - un œil ouvert
        catArt = "   /\\_____/\\    \n"
                 "  /  o   -  \\   \n"
                 " ( ==  w  == )  ";
    }
    else if (m_passwordFieldFocused) {
        // Le chat dort - yeux fermés
        catArt = "   /\\_____/\\   z\n"
                 "  /  -   -  \\ z \n"
                 " ( ==  w  == )  ";
    }
    else {
        // Le chat regarde - yeux ouverts
        catArt = "   /\\_____/\\    \n"
                 "  /  o   o  \\   \n"
                 " ( ==  ^  == )  ";
    }
    
    ImGui::Text("%s", catArt);
    
    // Champs de saisie
    ImGui::InputText("Utilisateur", m_username, sizeof(m_username));
    ImGui::InputText("Mot de passe", m_password, sizeof(m_password),
                     m_showPassword ? 0 : ImGuiInputTextFlags_Password);
    
    if (ImGui::Button("Connexion")) {
        m_client->Login(m_username, m_password);
    }
}
```

#### 4.2 Animations de Fond

Le fond animé utilise des "particules" qui flottent :

```cpp
void ChatWindow::RenderBackground()
{
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    
    // Dégradé de fond violet/rose
    drawList->AddRectFilledMultiColor(
        ImVec2(0, 0), ImVec2(width, height),
        IM_COL32(30, 20, 40, 255),   // Haut gauche
        IM_COL32(40, 25, 50, 255),   // Haut droite
        IM_COL32(50, 30, 60, 255),   // Bas droite
        IM_COL32(35, 22, 45, 255)    // Bas gauche
    );
    
    // Particules scintillantes
    for (auto& star : m_stars) {
        star.y += star.speed * deltaTime;
        if (star.y > height) star.y = 0;
        
        float alpha = 0.3f + 0.7f * sinf(m_animTime * star.twinkleSpeed);
        drawList->AddCircleFilled(
            ImVec2(star.x, star.y),
            star.size,
            IM_COL32(255, 255, 255, (int)(alpha * 255))
        );
    }
}
```

### 5. Gestion des Requêtes HTTP (WinHTTP)

WinHTTP est l'API Windows native pour les requêtes HTTPS :

```cpp
bool MatrixClient::HttpRequest(const std::string& method,
                               const std::string& endpoint,
                               const std::string& body,
                               std::string& response)
{
    // Ouverture de session
    HINTERNET hSession = WinHttpOpen(
        L"KittyChat/2.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0
    );

    // Connexion au serveur
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"vault.buffertavern.com",
        INTERNET_DEFAULT_HTTPS_PORT, 0
    );

    // Création de la requête
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",  // ou GET, PUT selon method
        L"/_matrix/client/v3/login",
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE  // HTTPS
    );

    // Ajout des headers
    WinHttpAddRequestHeaders(hRequest,
        L"Content-Type: application/json",
        -1, WINHTTP_ADDREQ_FLAG_ADD);

    // Envoi
    WinHttpSendRequest(hRequest, ...);
    WinHttpReceiveResponse(hRequest, NULL);

    // Lecture de la réponse
    // ...
    
    return true;
}
```

---

## Protocole Matrix

### Endpoints API Utilisés

| Endpoint | Méthode | Description |
|----------|---------|-------------|
| `/_matrix/client/v3/login` | POST | Authentification |
| `/_matrix/client/v3/register` | POST | Création de compte |
| `/_matrix/client/v3/logout` | POST | Déconnexion |
| `/_matrix/client/v3/sync` | GET | Synchronisation (long polling) |
| `/_matrix/client/v3/rooms/{roomId}/send/m.room.message/{txnId}` | PUT | Envoi de message |
| `/_matrix/client/v3/createRoom` | POST | Création de salon |
| `/_matrix/client/v3/join/{roomIdOrAlias}` | POST | Rejoindre un salon |

### Format des Messages

```json
{
  "msgtype": "m.text",
  "body": "Bonjour tout le monde !"
}
```

### Réponse de Synchronisation

```json
{
  "next_batch": "s123456789",
  "rooms": {
    "join": {
      "!roomid:server": {
        "timeline": {
          "events": [
            {
              "type": "m.room.message",
              "sender": "@user:server",
              "content": {
                "msgtype": "m.text",
                "body": "Hello!"
              },
              "origin_server_ts": 1704067200000
            }
          ]
        }
      }
    }
  }
}
```

---

## 🚀 Guide d'Installation

### Prérequis Windows

1. **Visual Studio 2019/2022** avec :
   - Développement Desktop en C++
   - Windows 10/11 SDK
   - Outils CMake pour Windows

2. **CMake 3.16+** (inclus dans Visual Studio ou téléchargeable)

### Compilation

```powershell
# Cloner ou extraire le projet
cd kitty-chat-cpp

# Créer le dossier de build
mkdir build
cd build

# Configuration CMake
cmake ..

# Compilation
cmake --build . --config Release

# Lancement
.\Release\KittyChat.exe
```

### Script automatique

Lancer **`launch.bat`** (à la racine) ou **`cicd/launch.bat`** :
1. Détection de CMake (PATH ou Visual Studio)
2. Configuration du projet
3. Compilation
4. Lancement de l'application

---

## Guide d'Utilisation

### Connexion

1. Lancer l'application
2. Entrer le nom d'utilisateur : `kitty`
3. Entrer le mot de passe : `meow123`
4. Cliquer sur "Connexion"

### Création de Compte

1. Entrer un nouveau nom d'utilisateur
2. Entrer un mot de passe
3. Cliquer sur "S'inscrire"

### Messagerie

- **Sélectionner un salon** : Cliquer dans la liste à gauche
- **Envoyer un message** : Taper le texte + Entrée ou clic sur "Miaou!"
- **Créer un salon** : Bouton "Créer"
- **Rejoindre un salon** : Bouton "Rejoindre"

---

## Difficultés Rencontrées

### 1. Chargement des GIFs

**Problème** : Les URLs Tenor devinées ne fonctionnaient pas, résultant en images cassées.

**Solution** : Retour à une solution ASCII art fiable qui fonctionne sans dépendance réseau pour l'affichage du chat.

### 2. Tunnel Cloudflare

**Problème** : Erreur 1033 lors des requêtes - le tunnel n'était pas actif.

**Solution** : Vérification systématique du statut du tunnel et redémarrage si nécessaire :
```bash
cloudflared tunnel run matrix
```

### 3. Parsing JSON

**Problème** : Crash lors du parsing de réponses d'erreur non-JSON.

**Solution** : Ajout de vérifications avant le parsing et messages d'erreur détaillés incluant le début de la réponse.

### 4. Compilation Windows

**Problème** : Difficulté à trouver CMake sur différentes configurations.

**Solution** : Script batch intelligent qui cherche CMake dans :
1. Le PATH système
2. L'installation Visual Studio
3. Les emplacements standards

### 5. HTTPS et Certificats

**Problème** : Validation SSL avec WinHTTP sur des tunnels Cloudflare.

**Solution** : WinHTTP gère automatiquement la validation via les certificats Windows, et Cloudflare fournit des certificats valides.

---

## Tests Effectués

| Test | Résultat |
|------|----------|
| Compilation sur Windows 10/11 | OK |
| Connexion au serveur Matrix | OK |
| Création de compte | OK |
| Envoi de messages | OK |
| Réception de messages | OK |
| Création de salon | OK |
| Rejoindre un salon | OK |
| Déconnexion | OK |
| Animations d'interface | OK |

---

## Conclusion

Ce projet démontre la mise en place complète d'un système de messagerie instantanée, de l'infrastructure serveur jusqu'à l'application cliente. Les points clés sont :

1. **Protocole Matrix** : Choix d'un protocole ouvert et standardisé
2. **Sécurité** : HTTPS via Cloudflare Tunnel, pas de ports ouverts
3. **Performance** : Application native C++ avec rendu GPU
4. **Expérience Utilisateur** : Interface moderne avec animations

### Améliorations Futures

- Support du chiffrement de bout en bout (E2EE avec Olm/Megolm)
- Notifications système Windows
- Envoi de fichiers et images
- Appels audio/vidéo (WebRTC)
- Version multi-plateforme (Linux, macOS)

---

## Licence

Ce projet est distribué sous licence MIT.

---

## Documentation Technique Complète

### Rapport Technique Principal

**[documentation/RAPPORT_TECHNIQUE_COMPLET.md](documentation/RAPPORT_TECHNIQUE_COMPLET.md)** - **Documentation détaillée du projet**

Ce rapport couvre en détail :

1. **Infrastructure Serveur**
   - Installation complète de Matrix Synapse
   - Configuration détaillée de Nginx (reverse proxy)
   - Mise en place de Cloudflare Tunnel (exposition sécurisée)
   - Architecture réseau complète avec schémas

2. **Application Cliente**
   - Architecture de l'application C++
   - Initialisation DirectX 11 et Dear ImGui
   - Implémentation complète du client Matrix
   - Interface graphique et animations
   - Gestion des textures et GIFs

3. **Sécurité**
   - Chiffrement HTTPS/TLS
   - Gestion des tokens d'accès
   - Protection du serveur (Cloudflare, Nginx)
   - Bonnes pratiques de sécurité

4. **Protocole Matrix**
   - API Client-Server v3 détaillée
   - Exemples de requêtes/réponses
   - Long polling et synchronisation temps réel
   - Format des identifiants Matrix

5. **Détails d'Implémentation**
   - Gestion des threads
   - Gestion de la mémoire
   - Gestion des erreurs
   - Tests et validation

6. **Difficultés et Solutions**
   - Problèmes rencontrés
   - Solutions apportées
   - Apprentissages

### Rapports Techniques par Partie

Pour une lecture par sections :

1. **[documentation/RAPPORT_TECHNIQUE.md](documentation/RAPPORT_TECHNIQUE.md)** - Backend & Infrastructure
2. **[documentation/RAPPORT_TECHNIQUE_PARTIE2.md](documentation/RAPPORT_TECHNIQUE_PARTIE2.md)** - Frontend & Client C++
3. **[documentation/RAPPORT_TECHNIQUE_PARTIE3.md](documentation/RAPPORT_TECHNIQUE_PARTIE3.md)** - Protocole & Sécurité

---

## Historique des Versions

### v2.0
- Interface moderne avec thème violet/rose
- Animations de fond (particules, étoiles)
- Chat ASCII interactif sur l'écran de connexion
- Création et gestion des salons
- Amélioration de la gestion d'erreurs

### v1.0
- Version initiale
- Connexion et inscription Matrix
- Messagerie de base
