# 🐱 Kitty Chat C++ v2.0

Client de messagerie Matrix en C++ avec interface graphique moderne et GIFs animés.

## ✨ Nouveautés v2.0

- **GIFs Animés** : Chargement de GIFs de chats depuis internet (Tenor)
- **Interface Moderne** : Thème violet/rose avec dégradés et particules
- **Animations** : Fond étoilé animé, effets de brillance
- **Bulles de Messages** : Style moderne avec couleurs différenciées
- **Emojis** : Utilisation extensive d'emojis dans l'interface

## Description

Kitty Chat est un client de chat qui se connecte au protocole Matrix, permettant de communiquer sur le serveur vault.buffertavern.com. L'application est développée en C++ avec une interface graphique moderne utilisant Dear ImGui et DirectX11.

## 🎨 Fonctionnalités

- 🔐 **Connexion Matrix** : Authentification par nom d'utilisateur et mot de passe
- ✨ **Création de compte** : Inscription directe depuis l'application
- 💬 **Liste des salons** : Affichage des salons avec badges de messages non lus
- ⚡ **Messagerie temps réel** : Envoi et réception de messages synchronisés
- 🐱 **GIFs de Chats** : GIFs animés téléchargés depuis Tenor
- 🌟 **Interface Animée** : Fond avec particules et étoiles scintillantes
- 🎨 **Thème Moderne** : Dégradés violet/rose/doré

## Prérequis

### Windows
- Windows 10/11 (64 bits recommandé)
- Visual Studio 2019 ou 2022 avec les composants C++ desktop
- CMake 3.16+ (télécharger: https://cmake.org/download/)
- Connexion Internet (pour télécharger les GIFs)

### Composants Visual Studio requis
- MSVC v142 ou v143 (compilateur C++)
- Windows 10/11 SDK
- C++ CMake tools for Windows

## 🚀 Lancement Rapide

### Option 1 : Script automatique (recommandé)

Double-cliquez sur `launch-kitty-chat.bat` dans le dossier parent. Le script va :
1. Détecter CMake (dans le PATH ou via Visual Studio)
2. Configurer le projet CMake
3. Compiler l'application
4. Lancer Kitty Chat

### Option 2 : Compilation manuelle

```bash
cd kitty-chat-cpp
mkdir build
cd build
cmake ..
cmake --build . --config Release
Release\KittyChat.exe
```

## 📖 Guide d'Utilisation

### 1. Première connexion / Création de compte

1. Lancez l'application via le script ou manuellement
2. Admirez le GIF de chat animé sur l'écran de connexion ! 🐱
3. Sur l'écran de connexion, vous avez deux options :
   
   **Pour créer un nouveau compte :**
   - Entrez un nom d'utilisateur (ex: monpseudo)
   - Entrez un mot de passe sécurisé
   - Cliquez sur "✨ S'inscrire" (bouton vert)
   - Si le nom est disponible, vous serez connecté automatiquement
   
   **Pour vous connecter :**
   - Entrez votre nom d'utilisateur existant
   - Entrez votre mot de passe
   - Cliquez sur "🐾 Connexion" (bouton orange)

### 2. Fonctions de l'interface

- **🐱 Logo animé** : Rebondit doucement dans la barre de titre
- **🏠 Liste des salons** : Cliquez pour sélectionner, badge 🔴 = messages non lus
- **💬 Zone de messages** : Bulles colorées avec nom et heure
- **🐾 Miaou!** : Envoie votre message
- **😴 Dodo** : Déconnexion
- **➕ Créer** : Créer un nouveau salon
- **🚪 Rejoindre** : Rejoindre un salon existant

### 3. Création et gestion des salons

1. Cliquez sur "➕ Créer" dans la barre latérale
2. Entrez le nom du salon (ex: "Mon Salon")
3. Cliquez sur "✅ Créer"
4. Le salon apparaît dans la liste et vous pouvez y envoyer des messages

Pour rejoindre un salon existant :
1. Cliquez sur "🚪 Rejoindre"
2. Entrez l'ID ou l'alias (ex: #general:vault.buffertavern.com)
3. Cliquez sur "✅ Rejoindre"

## 📁 Structure du Projet

```
kitty-chat-cpp/
  CMakeLists.txt          - Configuration de build CMake
  README.md               - Ce fichier
  launch.bat              - Script de lancement local
  src/
    main.cpp              - Point d'entrée, initialisation ImGui/DirectX
    matrix_client.h       - Déclaration du client Matrix
    matrix_client.cpp     - Implémentation du client Matrix
    chat_window.h         - Déclaration de l'interface utilisateur
    chat_window.cpp       - Interface avec animations et GIFs
    texture_manager.h     - Gestion des textures DirectX11
    texture_manager.cpp   - Chargement et animation des GIFs
    stb_image.h           - Décodeur GIF intégré
  assets/                 - Ressources (réservé)
  build/                  - Dossier de compilation (généré)
```

## 🔧 Architecture Technique

### Composants principaux

1. **main.cpp** - Initialisation Windows, DirectX11 et boucle principale ImGui
2. **MatrixClient** - Communication avec le serveur Matrix (HTTPS via WinHTTP)
3. **ChatWindow** - Interface utilisateur avec animations
4. **TextureManager** - Téléchargement et animation des GIFs

### Système de GIFs

Le `TextureManager` gère :
- Téléchargement asynchrone des GIFs depuis internet
- Décodage des frames avec stb_image
- Création de textures DirectX11 pour chaque frame
- Animation fluide avec timing précis

### Dépendances (téléchargées automatiquement via CMake)

- Dear ImGui v1.90.1 - Interface graphique
- nlohmann/json v3.11.3 - Parsing JSON
- cpp-httplib v0.14.3 - Support HTTP (backup)

### APIs Windows utilisées

- DirectX11 - Rendu graphique hardware-accéléré
- WinHTTP - Requêtes HTTPS sécurisées
- Win32 API - Fenêtrage et messages

## 🌐 API Matrix Utilisée

L'application utilise l'API Matrix Client-Server v3 :

- `POST /_matrix/client/v3/login` - Authentification
- `POST /_matrix/client/v3/register` - Création de compte
- `GET /_matrix/client/v3/sync` - Synchronisation des événements
- `PUT /_matrix/client/v3/rooms/{roomId}/send/...` - Envoi de messages
- `POST /_matrix/client/v3/createRoom` - Création de salon
- `POST /_matrix/client/v3/join/{roomIdOrAlias}` - Rejoindre un salon
- `POST /_matrix/client/v3/logout` - Déconnexion

## 🔒 Sécurité

- Les connexions utilisent HTTPS exclusivement (via Cloudflare Tunnel)
- Les mots de passe ne sont jamais stockés sur le disque
- Les tokens d'accès sont gardés en mémoire uniquement
- La bibliothèque WinHTTP gère la validation des certificats SSL

## ❓ Dépannage

### CMake n'est pas reconnu
Le script tente de trouver CMake automatiquement via Visual Studio. Sinon, installez CMake depuis https://cmake.org/download/ et ajoutez-le au PATH.

### Visual Studio non trouvé
Installez Visual Studio avec les composants C++ desktop depuis https://visualstudio.microsoft.com/

### GIFs qui ne chargent pas
- Vérifiez votre connexion Internet
- Les GIFs sont téléchargés depuis Tenor, assurez-vous que le site est accessible
- Un placeholder animé s'affiche pendant le chargement

### Erreur de connexion au serveur
- Vérifiez votre connexion Internet
- Vérifiez que vault.buffertavern.com est accessible
- Vérifiez vos identifiants

### Build échoue
- Assurez-vous d'avoir les droits d'écriture dans le dossier
- Fermez KittyChat.exe s'il est en cours d'exécution
- Supprimez le dossier build et recommencez

## 🎨 Design et Thème

### Palette de Couleurs
- **Fond** : Dégradé violet foncé vers rose/mauve
- **Accent** : Doré/orange pour les éléments interactifs
- **Texte** : Blanc légèrement rosé
- **Bulles** : Bleu pour vos messages, violet pour les autres

### Animations
- Particules/étoiles scintillantes en arrière-plan
- Logo qui rebondit doucement
- Titre avec effet arc-en-ciel
- Placeholder animé pendant le chargement des GIFs
- Pattes de chat 🐾 flottantes en arrière-plan

### GIFs de Chats
L'application charge plusieurs GIFs de chats depuis Tenor :
- Chat mignon sur l'écran de connexion
- Chat qui fait coucou dans la zone d'accueil
- Chaton dans la sidebar quand aucun salon

## 📜 Licence

Ce projet est distribué sous licence MIT.

## ✅ Tests Effectués

### Test de l'Interface v2.0
1. ✅ Fond animé avec particules visibles
2. ✅ Dégradé violet/rose appliqué
3. ✅ GIFs chargés et animés
4. ✅ Bulles de messages modernes
5. ✅ Emojis affichés correctement

### Test de Création de Salon
1. ✅ Bouton "➕ Créer" fonctionnel
2. ✅ Popup de création de salon
3. ✅ Salon créé et visible dans la liste
4. ✅ Messages envoyables dans le nouveau salon

### Test de Rejoindre un Salon
1. ✅ Bouton "🚪 Rejoindre" fonctionnel
2. ✅ Saisie d'alias de salon
3. ✅ Salon rejoint avec succès

### Infrastructure Serveur
- Serveur Matrix Synapse sur vault.buffertavern.com
- Accessible via Cloudflare Tunnel (HTTPS)
- API v3 Matrix Client-Server

## 👤 Auteur

Projet réalisé dans le cadre du Master Cybersécurité - Janvier 2026

## 📝 Historique des Versions

### v2.0 (Janvier 2026)
- ✨ Ajout des GIFs animés depuis internet
- 🎨 Nouveau thème violet/rose moderne
- 🌟 Fond animé avec particules
- 💬 Bulles de messages stylisées
- 🚀 Création et jonction de salons

### v1.0 (Janvier 2026)
- 🐱 Version initiale avec thème chat ASCII
- 🔐 Connexion et inscription Matrix
- 💬 Messagerie de base
