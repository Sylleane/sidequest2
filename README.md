# Kitty Chat C++

Client de messagerie Matrix en C++ avec interface graphique Dear ImGui.

## Description

Kitty Chat est un client de chat qui se connecte au protocole Matrix, permettant de communiquer sur le serveur matrix.buffertavern.com. L'application est developpee en C++ avec une interface graphique moderne utilisant Dear ImGui et DirectX11, avec un theme chat mignon.

## Fonctionnalites

- Connexion Matrix : Authentification par nom d'utilisateur et mot de passe
- Creation de compte : Inscription directe depuis l'application
- Liste des salons : Affichage des salons rejoints avec compteur de messages non lus
- Messagerie temps reel : Envoi et reception de messages synchronises
- Interface thematique : Design chat/minou avec ASCII art et couleurs chaleureuses
- Synchronisation : Mise a jour automatique des messages en arriere-plan

## Prerequis

### Windows
- Windows 10/11 (64 bits recommande)
- Visual Studio 2019 ou 2022 avec les composants C++ desktop
- CMake 3.16+ (telecharger: https://cmake.org/download/)

### Composants Visual Studio requis
- MSVC v142 ou v143 (compilateur C++)
- Windows 10/11 SDK
- C++ CMake tools for Windows

## Lancement Rapide

### Option 1 : Script automatique (recommande)

Double-cliquez sur launch-kitty-chat.bat dans le dossier parent. Le script va :
1. Detecter CMake (dans le PATH ou via Visual Studio)
2. Configurer le projet CMake
3. Compiler l'application
4. Lancer Kitty Chat

### Option 2 : Compilation manuelle

```
cd kitty-chat-cpp
mkdir build
cd build
cmake ..
cmake --build . --config Release
Release\KittyChat.exe
```

## Guide d'Utilisation

### 1. Premiere connexion / Creation de compte

1. Lancez l'application via le script ou manuellement
2. Sur l'ecran de connexion avec le chat ASCII, vous avez deux options :
   
   Pour creer un nouveau compte :
   - Entrez un nom d'utilisateur (ex: monpseudo)
   - Entrez un mot de passe securise
   - Cliquez sur "Nouveau chaton?" (bouton vert)
   - Si le nom est disponible, vous serez connecte automatiquement
   
   Pour vous connecter :
   - Entrez votre nom d'utilisateur existant
   - Entrez votre mot de passe
   - Cliquez sur "Miaou! (Connexion)" (bouton orange)

3. Apres connexion :
   - La liste des salons apparait a gauche
   - Cliquez sur un salon pour voir les messages
   - Tapez votre message et appuyez sur Entree ou cliquez "Miaou!"

### 2. Fonctions de l'interface

- =^.^= : Indicateur de theme chat
- Liste des salons : Cliquez pour selectionner, badge (X) = messages non lus
- Zone de messages : Affiche les messages avec nom et heure
- "Miaou!" : Envoie votre message
- "Faire dodo" : Deconnexion

## Structure du Projet

```
kitty-chat-cpp/
  CMakeLists.txt          - Configuration de build CMake
  README.md               - Ce fichier
  src/
    main.cpp              - Point d'entree, initialisation ImGui/DirectX
    matrix_client.h       - Declaration du client Matrix
    matrix_client.cpp     - Implementation du client Matrix (login, register, sync)
    chat_window.h         - Declaration de l'interface utilisateur
    chat_window.cpp       - Implementation de l'interface (theme chat)
  assets/                 - Ressources (reserve pour futures ameliorations)
  build/                  - Dossier de compilation (genere)
```

## Architecture Technique

### Composants principaux

1. main.cpp - Initialisation de la fenetre Windows, DirectX11 et boucle principale ImGui
2. MatrixClient - Gere la communication avec le serveur Matrix (HTTP/HTTPS via WinHTTP)
3. ChatWindow - Rendu de l'interface utilisateur avec ImGui et theme chat

### Dependances (telechargees automatiquement via CMake)

- Dear ImGui v1.90.1 - Interface graphique
- nlohmann/json v3.11.3 - Parsing JSON
- cpp-httplib v0.14.3 - Support HTTP (backup)

### APIs Windows utilisees

- DirectX11 - Rendu graphique hardware-accelere
- WinHTTP - Requetes HTTPS securisees vers le serveur Matrix

## API Matrix Utilisee

L'application utilise l'API Matrix Client-Server :

- POST /_matrix/client/v3/login - Authentification
- POST /_matrix/client/v3/register - Creation de compte
- GET /_matrix/client/v3/sync - Synchronisation des evenements
- PUT /_matrix/client/v3/rooms/{roomId}/send/... - Envoi de messages
- POST /_matrix/client/v3/logout - Deconnexion

## Securite

- Les connexions utilisent HTTPS exclusivement (via Cloudflare Tunnel)
- Les mots de passe ne sont jamais stockes sur le disque
- Les tokens d'acces sont gardes en memoire uniquement
- La bibliotheque WinHTTP gere la validation des certificats SSL

## Depannage

### CMake n'est pas reconnu
Le script tente de trouver CMake automatiquement via Visual Studio. Sinon, installez CMake depuis https://cmake.org/download/ et ajoutez-le au PATH.

### Visual Studio non trouve
Installez Visual Studio avec les composants C++ desktop depuis https://visualstudio.microsoft.com/

### Erreur de connexion au serveur
- Verifiez votre connexion Internet
- Verifiez que le serveur matrix.buffertavern.com est accessible
- Verifiez vos identifiants

### Ce nom d'utilisateur est deja pris
Choisissez un autre nom d'utilisateur pour l'inscription.

### L'inscription est desactivee
L'administrateur du serveur a desactive les inscriptions publiques. Contactez-le pour obtenir un compte.

### Build echoue
- Assurez-vous d'avoir les droits d'ecriture dans le dossier
- Fermez KittyChat.exe s'il est en cours d'execution
- Supprimez le dossier build et recommencez

## Theme Chat

L'application utilise un theme visuel inspire des chats :

### Elements Visuels
- ASCII Art : Chat mignon sur l'ecran de connexion
```
  /\_/\  
 ( o.o ) 
  > ^ <
```
- Chat ASCII dans la zone de messages (quand aucun salon selectionne)
- Emojis textuels : =^.^= et ~(=^..^) dans la barre de titre et les en-tetes

### Couleurs
- Tons oranges et bruns rappelant la fourrure de chat
- Couleur d'accent chaleureuse pour les boutons

### Textes Thematiques
- "Miaou!" pour envoyer un message
- "Faire dodo" pour se deconnecter
- "Nouveau chaton?" pour s'inscrire
- "Le chat cherche des salons..." pendant le chargement
- "Eveille et curieux" comme statut de connexion

Note: L'application utilise de l'ASCII art plutot que des GIFs pour garder le code simple et eviter les dependances d'image complexes. Cela permet aussi une meilleure compatibilite et des temps de chargement plus rapides.

## Licence

Ce projet est distribue sous licence MIT.

## Tests Effectues

Les fonctionnalites suivantes ont ete testees et validees :

### Test de Creation de Compte
1. Lancement de l'application via launch-kitty-chat.bat
2. Saisie d'un nom d'utilisateur (ex: kittytest)
3. Saisie d'un mot de passe securise
4. Clic sur "Nouveau chaton?" (bouton vert)
5. Resultat: Compte cree avec succes, connexion automatique

### Test de Connexion
1. Saisie du nom d'utilisateur existant
2. Saisie du mot de passe
3. Clic sur "Miaou! (Connexion)" (bouton orange)
4. Resultat: Connexion reussie, affichage des salons

### Test de l'Interface
1. Theme chat avec ASCII art visible sur l'ecran de connexion
2. Couleurs chaudes (tons orange/brun) appliquees
3. Boutons thematiques fonctionnels ("Miaou!", "Faire dodo", "Nouveau chaton?")
4. Liste des salons affichee correctement
5. Zone de messages avec scroll automatique

### Test de Messagerie
1. Selection d'un salon dans la liste
2. Saisie d'un message dans la zone de texte
3. Envoi via bouton "Miaou!" ou touche Entree
4. Resultat: Message envoye et affiche

### Infrastructure Serveur
- Serveur Matrix Synapse sur vault.buffertavern.com
- Accessible via Cloudflare Tunnel (HTTPS)
- API v3 Matrix Client-Server

## Auteur

Projet realise dans le cadre du Master Cybersecurite - Janvier 2026
