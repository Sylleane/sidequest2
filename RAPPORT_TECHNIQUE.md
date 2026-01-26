# 📚 Rapport Technique - Kitty Chat

## Application de Messagerie Instantanée Matrix

**Projet Master Cybersécurité - Janvier 2026**

---

# Table des Matières

1. [Introduction](#1-introduction)
2. [Partie Backend - Infrastructure Serveur](#2-partie-backend---infrastructure-serveur)
3. [Partie Frontend - Application Cliente](#3-partie-frontend---application-cliente)
4. [Protocole Matrix - Détails Techniques](#4-protocole-matrix---détails-techniques)
5. [Sécurité et Bonnes Pratiques](#5-sécurité-et-bonnes-pratiques)
6. [Difficultés et Solutions](#6-difficultés-et-solutions)
7. [Conclusion](#7-conclusion)

---

# 1. Introduction

## 1.1 Contexte du Projet

Ce projet consiste en la réalisation d'une application de messagerie instantanée complète, comprenant :
- Un serveur de communication auto-hébergé
- Un client natif Windows avec interface graphique
- Une infrastructure réseau sécurisée

## 1.2 Objectifs Pédagogiques

- Comprendre le fonctionnement d'un protocole de messagerie moderne
- Maîtriser le déploiement d'infrastructure serveur Linux
- Développer une application native C++ avec interface graphique
- Implémenter des communications HTTPS sécurisées

## 1.3 Choix Techniques Justifiés

### Pourquoi Matrix ?

Matrix est un protocole de communication décentralisé et open-source. Contrairement aux solutions propriétaires (WhatsApp, Discord), Matrix permet :

| Avantage | Description |
|----------|-------------|
| **Décentralisation** | Chaque organisation peut héberger son serveur |
| **Fédération** | Communication inter-serveurs possible |
| **Open Source** | Code auditable, pas de backdoor |
| **API REST** | Standard HTTP/JSON facile à implémenter |
| **E2EE** | Chiffrement de bout en bout (Olm/Megolm) |

### Pourquoi C++ et Dear ImGui ?

| Technologie | Justification |
|-------------|---------------|
| **C++17** | Performance native, contrôle mémoire, pas de runtime |
| **Dear ImGui** | Rendu immédiat, léger (~300KB), personnalisable |
| **DirectX 11** | API graphique Windows native, accélération GPU |
| **WinHTTP** | Bibliothèque Windows native, SSL/TLS intégré |

### Pourquoi Cloudflare Tunnel ?

| Avantage | Description |
|----------|-------------|
| **Sécurité** | Aucun port ouvert sur le firewall/routeur |
| **HTTPS gratuit** | Certificats SSL gérés automatiquement |
| **Protection DDoS** | Incluse dans le service Cloudflare |
| **Simplicité** | Configuration minimale côté serveur |

---

# 2. Partie Backend - Infrastructure Serveur

## 2.1 Architecture Serveur

```
┌─────────────────────────────────────────────────────────────────────┐
│                        SERVEUR LINUX                                │
│                     (Debian 12 / Ubuntu 22.04)                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────────────┐   │
│  │ cloudflared │────►│   nginx     │────►│  matrix-synapse     │   │
│  │             │     │  (port 80)  │     │    (port 8008)      │   │
│  │  (tunnel)   │     │             │     │                     │   │
│  └─────────────┘     └─────────────┘     └──────────┬──────────┘   │
│        ▲                                            │              │
│        │                                            ▼              │
│        │                                 ┌─────────────────────┐   │
│        │                                 │  SQLite Database    │   │
│        │                                 │  /var/lib/matrix-   │   │
│        │                                 │  synapse/homeserver │   │
│        │                                 │  .db                │   │
│        │                                 └─────────────────────┘   │
└────────┼───────────────────────────────────────────────────────────┘
         │
         │ Connexion sortante chiffrée
         │ (pas de port entrant ouvert)
         ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    CLOUDFLARE EDGE NETWORK                          │
│                                                                     │
│  - Terminaison SSL/TLS                                              │
│  - Protection DDoS                                                  │
│  - Cache CDN                                                        │
│  - DNS vault.buffertavern.com                                       │
└─────────────────────────────────────────────────────────────────────┘
```

## 2.2 Installation de Matrix Synapse

### 2.2.1 Qu'est-ce que Synapse ?

Synapse est l'implémentation de référence du serveur Matrix, développée par la fondation Matrix.org. Écrit en Python, il gère :

- L'authentification des utilisateurs
- Le stockage des messages
- La synchronisation en temps réel
- La fédération avec d'autres serveurs

### 2.2.2 Prérequis Système

```bash
# Mise à jour du système
sudo apt update && sudo apt upgrade -y

# Installation des dépendances Python
sudo apt install -y \
    build-essential \
    python3-dev \
    python3-pip \
    python3-venv \
    python3-setuptools \
    libffi-dev \
    libssl-dev \
    libjpeg-dev \
    libxslt1-dev \
    sqlite3

# Vérification des versions
python3 --version  # Python 3.10+ recommandé
pip3 --version
```

### 2.2.3 Installation via le Dépôt Officiel

```bash
# Ajout de la clé GPG du dépôt Matrix
sudo wget -O /usr/share/keyrings/matrix-org-archive-keyring.gpg \
    https://packages.matrix.org/debian/matrix-org-archive-keyring.gpg

# Ajout du dépôt
echo "deb [signed-by=/usr/share/keyrings/matrix-org-archive-keyring.gpg] \
    https://packages.matrix.org/debian/ $(lsb_release -cs) main" | \
    sudo tee /etc/apt/sources.list.d/matrix-org.list

# Installation de Synapse
sudo apt update
sudo apt install -y matrix-synapse-py3
```

Lors de l'installation, le système demande le nom du serveur. Ce nom est **permanent** et doit correspondre au domaine final (ex: `vault.buffertavern.com`).

### 2.2.4 Configuration Détaillée de Synapse

Le fichier de configuration principal est `/etc/matrix-synapse/homeserver.yaml`. Voici les sections importantes :

#### Identité du Serveur

```yaml
# Nom du serveur - PERMANENT, ne peut pas être changé après
# C'est la partie après le @ dans les identifiants (@user:server)
server_name: "vault.buffertavern.com"

# Identifiant unique de ce serveur (généré automatiquement)
server_key: "/etc/matrix-synapse/homeserver.signing.key"

# PID file pour le processus
pid_file: "/var/run/matrix-synapse.pid"
```

#### Configuration des Listeners (Ports d'écoute)

```yaml
listeners:
  # Port principal pour l'API client et la fédération
  - port: 8008
    type: http
    tls: false           # TLS géré par nginx/cloudflare
    x_forwarded: true    # Fait confiance au header X-Forwarded-For
    
    # Bind sur localhost uniquement (sécurité)
    bind_addresses: ['127.0.0.1']
    
    resources:
      - names: [client]      # API Client-Server
        compress: true       # Compression gzip activée
      - names: [federation]  # API Server-Server (fédération)
        compress: false
```

**Explication des ressources :**
- `client` : Endpoints utilisés par les applications (login, sync, send message)
- `federation` : Endpoints pour la communication inter-serveurs

#### Base de Données

```yaml
# SQLite (simple, pour <100 utilisateurs)
database:
  name: sqlite3
  args:
    database: /var/lib/matrix-synapse/homeserver.db

# Alternative PostgreSQL (production, performances)
# database:
#   name: psycopg2
#   args:
#     user: synapse_user
#     password: <password>
#     database: synapse
#     host: localhost
#     cp_min: 5
#     cp_max: 10
```

#### Inscription des Utilisateurs

```yaml
# Permettre l'inscription publique
enable_registration: true

# Ne pas exiger de vérification email
enable_registration_without_verification: true

# Alternative : inscription uniquement via token partagé
# registration_shared_secret: "votre_secret_ici"

# Bloquer certains noms d'utilisateur
# registration_requires_token: true
```

#### Stockage des Médias

```yaml
# Dossier de stockage des fichiers uploadés
media_store_path: "/var/lib/matrix-synapse/media"

# Taille maximale d'upload (en bytes)
max_upload_size: 52428800  # 50 MB

# Durée de rétention des médias distants (fédération)
# remote_media_lifetime: 14d

# Génération de thumbnails
dynamic_thumbnails: true
thumbnail_sizes:
  - width: 32
    height: 32
    method: crop
  - width: 96
    height: 96
    method: crop
  - width: 320
    height: 240
    method: scale
```

#### Journalisation (Logs)

```yaml
log_config: "/etc/matrix-synapse/log.yaml"
```

Contenu de `/etc/matrix-synapse/log.yaml` :

```yaml
version: 1

formatters:
  precise:
    format: '%(asctime)s - %(name)s - %(lineno)d - %(levelname)s - %(message)s'

handlers:
  file:
    class: logging.handlers.TimedRotatingFileHandler
    formatter: precise
    filename: /var/log/matrix-synapse/homeserver.log
    when: midnight
    backupCount: 7
    encoding: utf8

  console:
    class: logging.StreamHandler
    formatter: precise
    stream: 'ext://sys.stdout'

loggers:
  synapse.storage.SQL:
    level: WARNING  # Réduire le bruit SQL

root:
  level: INFO
  handlers: [file, console]
```

### 2.2.5 Création d'Utilisateurs

#### Via ligne de commande (admin) :

```bash
# Création d'un utilisateur admin
register_new_matrix_user \
    -c /etc/matrix-synapse/homeserver.yaml \
    http://localhost:8008 \
    -u admin \
    -p motdepasse123 \
    -a  # Flag admin

# Création d'un utilisateur normal
register_new_matrix_user \
    -c /etc/matrix-synapse/homeserver.yaml \
    http://localhost:8008 \
    -u kitty \
    -p meow123
```

#### Via l'API (si inscription activée) :

```bash
curl -X POST "http://localhost:8008/_matrix/client/v3/register" \
    -H "Content-Type: application/json" \
    -d '{
        "username": "nouveauuser",
        "password": "motdepasse",
        "auth": {
            "type": "m.login.dummy"
        }
    }'
```

### 2.2.6 Gestion du Service

```bash
# Démarrer le service
sudo systemctl start matrix-synapse

# Arrêter
sudo systemctl stop matrix-synapse

# Redémarrer (après modification config)
sudo systemctl restart matrix-synapse

# Voir le statut
sudo systemctl status matrix-synapse

# Voir les logs en temps réel
sudo journalctl -u matrix-synapse -f

# Activer le démarrage automatique
sudo systemctl enable matrix-synapse
```

## 2.3 Configuration de Nginx

### 2.3.1 Rôle de Nginx

Nginx agit comme **reverse proxy** entre Cloudflare et Synapse :

```
Client → Cloudflare (HTTPS) → Tunnel → Nginx (HTTP) → Synapse
```

Fonctions :
- Redirection des requêtes vers Synapse
- Ajout des headers (X-Forwarded-For, Host)
- Gestion des WebSockets (pour /sync)
- Éventuel cache des ressources statiques

### 2.3.2 Installation

```bash
sudo apt install -y nginx
```

### 2.3.3 Configuration Complète

Fichier `/etc/nginx/sites-available/matrix` :

```nginx
# Upstream vers Synapse
upstream synapse {
    server 127.0.0.1:8008;
    
    # Keepalive pour de meilleures performances
    keepalive 32;
}

server {
    # Écoute sur le port 80 (HTTP)
    # Le HTTPS est terminé par Cloudflare
    listen 80;
    listen [::]:80;
    
    # Nom du serveur
    server_name vault.buffertavern.com;
    
    # Taille maximale du body (pour les uploads)
    client_max_body_size 50M;
    
    # === API Matrix Client-Server ===
    location /_matrix {
        proxy_pass http://synapse;
        
        # Headers importants
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        
        # Support WebSocket (nécessaire pour /sync)
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        
        # Timeouts longs pour le long-polling /sync
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 600s;  # 10 minutes pour sync
        
        # Buffers
        proxy_buffering off;
        proxy_buffer_size 16k;
        proxy_busy_buffers_size 24k;
        proxy_buffers 64 4k;
    }
    
    # === Well-Known (découverte automatique) ===
    # Permet aux clients de découvrir le serveur Matrix
    location /.well-known/matrix/client {
        default_type application/json;
        add_header Access-Control-Allow-Origin *;
        
        return 200 '{
            "m.homeserver": {
                "base_url": "https://vault.buffertavern.com"
            }
        }';
    }
    
    location /.well-known/matrix/server {
        default_type application/json;
        
        return 200 '{
            "m.server": "vault.buffertavern.com:443"
        }';
    }
    
    # === Synapse Admin API (optionnel) ===
    location /_synapse {
        proxy_pass http://synapse;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
    
    # === Page d'accueil simple ===
    location / {
        return 200 'Matrix server vault.buffertavern.com is running!';
        add_header Content-Type text/plain;
    }
}
```

### 2.3.4 Activation et Test

```bash
# Créer le lien symbolique
sudo ln -sf /etc/nginx/sites-available/matrix /etc/nginx/sites-enabled/

# Supprimer la config par défaut (optionnel)
sudo rm -f /etc/nginx/sites-enabled/default

# Tester la configuration
sudo nginx -t
# Output attendu:
# nginx: the configuration file /etc/nginx/nginx.conf syntax is ok
# nginx: configuration file /etc/nginx/nginx.conf test is successful

# Recharger nginx
sudo systemctl reload nginx

# Vérifier le statut
sudo systemctl status nginx
```

### 2.3.5 Test Local

```bash
# Test direct vers Synapse
curl http://localhost:8008/_matrix/client/versions
# Réponse attendue: {"versions":["r0.0.1","r0.1.0",...,"v1.1","v1.2",...]}

# Test via Nginx
curl http://localhost/_matrix/client/versions
# Même réponse attendue
```

## 2.4 Cloudflare Tunnel

### 2.4.1 Principe de Fonctionnement

Contrairement à un VPN ou un port forward classique, Cloudflare Tunnel fonctionne en **connexion sortante** :

```
┌──────────────┐                    ┌─────────────────┐
│   SERVEUR    │ ──── sortant ───► │   CLOUDFLARE    │
│  cloudflared │                    │   EDGE NETWORK  │
└──────────────┘                    └────────┬────────┘
                                             │
                                    ◄──── entrant ────
                                             │
                                    ┌────────┴────────┐
                                    │    CLIENTS      │
                                    │  (navigateurs)  │
                                    └─────────────────┘
```

**Avantages :**
- Le serveur n'expose aucun port entrant
- Pas de configuration NAT/firewall nécessaire
- Protection DDoS gratuite
- Certificats SSL automatiques

### 2.4.2 Installation de cloudflared

```bash
# Téléchargement du paquet Debian
wget https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64.deb

# Installation
sudo dpkg -i cloudflared-linux-amd64.deb

# Vérification
cloudflared --version
# cloudflared version 2024.x.x
```

### 2.4.3 Authentification

```bash
# Lance le navigateur pour l'authentification Cloudflare
cloudflared tunnel login
```

Cette commande :
1. Ouvre une URL dans le navigateur
2. Demande de se connecter au compte Cloudflare
3. Demande de sélectionner le domaine à utiliser
4. Télécharge un certificat dans `~/.cloudflared/cert.pem`

### 2.4.4 Création du Tunnel

```bash
# Créer un nouveau tunnel nommé "matrix"
cloudflared tunnel create matrix

# Output:
# Tunnel credentials written to /home/user/.cloudflared/<UUID>.json
# Created tunnel matrix with id <UUID>
```

Le fichier JSON contient les credentials du tunnel et ne doit **jamais** être partagé.

### 2.4.5 Configuration du Tunnel

Créer le fichier `~/.cloudflared/config.yml` :

```yaml
# ID du tunnel (récupéré lors de la création)
tunnel: a1b2c3d4-e5f6-7890-abcd-ef1234567890

# Fichier de credentials
credentials-file: /home/nintae/.cloudflared/a1b2c3d4-e5f6-7890-abcd-ef1234567890.json

# Règles d'ingress (routage)
ingress:
  # Requêtes vers vault.buffertavern.com → localhost:80 (nginx)
  - hostname: vault.buffertavern.com
    service: http://localhost:80
    originRequest:
      # Désactiver la vérification TLS (connexion locale)
      noTLSVerify: true
  
  # Règle par défaut (obligatoire)
  - service: http_status:404
```

### 2.4.6 Configuration DNS

Dans le dashboard Cloudflare (dash.cloudflare.com) :

1. Aller dans **DNS** → **Records**
2. Ajouter un enregistrement **CNAME** :
   - **Name:** `vault` (ou `@` pour le domaine racine)
   - **Target:** `<tunnel-id>.cfargotunnel.com`
   - **Proxy status:** Proxied (orange cloud)

Ou via la CLI :

```bash
cloudflared tunnel route dns matrix vault.buffertavern.com
```

### 2.4.7 Lancement du Tunnel

#### Mode manuel (test) :

```bash
cloudflared tunnel run matrix
```

#### Mode service (production) :

```bash
# Installation en tant que service systemd
sudo cloudflared service install

# Démarrage
sudo systemctl start cloudflared

# Activation au boot
sudo systemctl enable cloudflared

# Vérification
sudo systemctl status cloudflared
```

### 2.4.8 Vérification

```bash
# Depuis n'importe où sur Internet
curl https://vault.buffertavern.com/_matrix/client/versions

# Réponse attendue:
# {"versions":["r0.0.1","r0.1.0",...,"v1.6"],"unstable_features":{...}}
```

---

*Suite dans la Partie 3 : Application Cliente*
