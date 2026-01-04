# Kitty Chat C++

Client de messagerie Matrix en C++ avec interface graphique Dear ImGui.

## Description

Kitty Chat est un client de chat qui se connecte au protocole Matrix, permettant de communiquer sur le serveur `buffertavern.com`. L'application est développée en C++ avec une interface graphique moderne utilisant Dear ImGui et DirectX11.

## Fonctionnalités

- **Connexion Matrix** : Authentification par nom d'utilisateur et mot de passe
- **Liste des salons** : Affichage des salons rejoints avec compteur de messages non lus
- **Messagerie temps réel** : Envoi et réception de messages synchronisés
- **Interface moderne** : Thème chaleureux inspiré des chats/minous
- **Synchronisation** : Mise à jour automatique des messages en arrière-plan

## Prérequis

### Windows
- **Windows 10/11** (64 bits recommandé)
- **Visual Studio 2019 ou 2022** avec les composants C++ desktop
- **CMake 3.16+** ([télécharger ici](https://cmake.org/download/))
- **Git** (optionnel, pour le clonage)

### Composants Visual Studio requis
- MSVC v142 ou v143 (compilateur C++)
- Windows 10/11 SDK
- C++ CMake tools for Windows

## Compilation

### Option 1 : Utiliser le script de lancement

Double-cliquez sur `launch-kitty-chat.bat` dans le dossier parent. Le script va :
1. Vérifier la présence de CMake
2. Configurer le projet
3. Compiler l'application
4. Lancer le programme

### Option 2 : Compilation manuelle

```batch
cd kitty-chat-cpp

rem Création du dossier de build
mkdir build
cd build

rem Configuration avec CMake (utilise Visual Studio par défaut)
cmake ..

rem Compilation en mode Release
cmake --build . --config Release

rem Lancement
Release\KittyChat.exe
```

## Structure du Projet

```
kitty-chat-cpp/
├── CMakeLists.txt          # Configuration de build CMake
├── README.md               # Ce fichier
├── src/
│   ├── main.cpp            # Point d'entrée, initialisation ImGui/DirectX
│   ├── matrix_client.h     # Déclaration du client Matrix
│   ├── matrix_client.cpp   # Implémentation du client Matrix
│   ├── chat_window.h       # Déclaration de l'interface utilisateur
│   └── chat_window.cpp     # Implémentation de l'interface
└── assets/                 # Ressources (icônes, etc.)
```

## Architecture

### Composants principaux

1. **main.cpp** - Initialisation de la fenêtre Windows, DirectX11 et la boucle principale ImGui
2. **MatrixClient** - Gère la communication avec le serveur Matrix (HTTP/HTTPS via WinHTTP)
3. **ChatWindow** - Rendu de l'interface utilisateur avec ImGui

### Dépendances (téléchargées automatiquement via CMake)

- **Dear ImGui** v1.90.1 - Bibliothèque d'interface graphique
- **nlohmann/json** v3.11.3 - Parsing JSON
- **cpp-httplib** v0.14.3 - Requêtes HTTP (non utilisé directement, WinHTTP préféré)

### APIs Windows utilisées

- **DirectX11** - Rendu graphique
- **WinHTTP** - Requêtes HTTPS sécurisées vers le serveur Matrix

## Utilisation

1. Lancez l'application
2. Entrez votre nom d'utilisateur (ex: `monpseudo` ou `@monpseudo:buffertavern.com`)
3. Entrez votre mot de passe
4. Cliquez sur "Se connecter"
5. Sélectionnez un salon dans la liste de gauche
6. Tapez votre message et appuyez sur Entrée ou cliquez "Envoyer"

## API Matrix

L'application utilise l'API Matrix Client-Server :

- `POST /_matrix/client/v3/login` - Authentification
- `GET /_matrix/client/v3/sync` - Synchronisation des événements
- `PUT /_matrix/client/v3/rooms/{roomId}/send/m.room.message/{txnId}` - Envoi de messages
- `POST /_matrix/client/v3/logout` - Déconnexion

## Sécurité

- Les connexions utilisent HTTPS exclusivement
- Les mots de passe ne sont jamais stockés sur le disque
- Les tokens d'accès sont gardés en mémoire uniquement
- La bibliothèque WinHTTP gère la validation des certificats SSL

## Dépannage

### "CMake n'est pas reconnu"
Installez CMake depuis https://cmake.org/download/ et ajoutez-le au PATH

### "Visual Studio non trouvé"
Installez Visual Studio avec les composants C++ desktop depuis https://visualstudio.microsoft.com/

### "Erreur de connexion au serveur"
- Vérifiez votre connexion Internet
- Vérifiez que le serveur buffertavern.com est accessible
- Vérifiez vos identifiants

### "Build échoue"
- Assurez-vous d'avoir les droits d'écriture dans le dossier
- Supprimez le dossier `build` et recommencez

## Licence

Ce projet est distribué sous licence MIT.

## Auteur

Projet réalisé dans le cadre du Master Cybersécurité - Janvier 2026
