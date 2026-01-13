/**
 * @file chat_window.cpp
 * @brief Implémentation de la fenêtre de chat
 * 
 * Ce fichier contient le code de rendu de l'interface utilisateur
 * utilisant Dear ImGui pour créer une interface moderne et réactive.
 * 
 * Auteur: Étudiant en Master Cybersécurité
 * Date: Janvier 2026
 */

#include <windows.h>

// Désactivation des macros Windows conflictuelles
#ifdef SendMessage
#undef SendMessage
#endif

#include "chat_window.h"
#include "imgui.h"
#include <algorithm>

/**
 * @brief Constructeur - Initialise les buffers et l'état
 */
ChatWindow::ChatWindow(MatrixClient* client)
    : m_client(client)
    , m_showPassword(false)
    , m_loginError(false)
    , m_isRegistering(false)
    , m_scrollToBottom(true)
    , m_showCreateRoom(false)
    , m_showJoinRoom(false)
{
    // Initialisation des buffers à zéro
    memset(m_username, 0, sizeof(m_username));
    memset(m_password, 0, sizeof(m_password));
    memset(m_messageInput, 0, sizeof(m_messageInput));
    memset(m_newRoomName, 0, sizeof(m_newRoomName));
    memset(m_joinRoomId, 0, sizeof(m_joinRoomId));
}

/**
 * @brief Point d'entrée principal du rendu
 * 
 * Affiche soit l'écran de connexion, soit l'interface de chat
 * selon l'état de connexion
 */
void ChatWindow::Render()
{
    // Configuration de la fenêtre principale pour occuper tout l'écran
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags windowFlags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("Kitty Chat", nullptr, windowFlags);

    if (m_client->IsLoggedIn())
    {
        RenderChatInterface();
    }
    else
    {
        RenderLoginScreen();
    }

    ImGui::End();
}

/**
 * @brief Affiche l'écran de connexion centré
 */
void ChatWindow::RenderLoginScreen()
{
    // Centrage vertical et horizontal
    ImVec2 windowSize = ImGui::GetWindowSize();
    float formWidth = 400.0f;
    float formHeight = 320.0f;
    
    ImGui::SetCursorPos(ImVec2(
        (windowSize.x - formWidth) * 0.5f,
        (windowSize.y - formHeight) * 0.5f
    ));

    // Conteneur du formulaire
    ImGui::BeginChild("LoginForm", ImVec2(formWidth, formHeight), true);

    // Titre avec émoji chat et art ASCII
    ImGui::PushFont(nullptr);
    
    // Art ASCII d'un chat mignon
    const char* catArt = 
        "  /\\_/\\  \n"
        " ( o.o ) \n"
        "  > ^ <  ";
    float catWidth = ImGui::CalcTextSize("  /\\_/\\  ").x;
    ImGui::SetCursorPosX((formWidth - catWidth) * 0.5f - 10);
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "%s", catArt);
    
    ImGui::Spacing();
    
    float titleWidth = ImGui::CalcTextSize("~ Kitty Chat ~").x;
    ImGui::SetCursorPosX((formWidth - titleWidth) * 0.5f - 10);
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "~ Kitty Chat ~");
    ImGui::PopFont();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // Sous-titre
    const char* subtitle = "Connexion au serveur Matrix";
    float subtitleWidth = ImGui::CalcTextSize(subtitle).x;
    ImGui::SetCursorPosX((formWidth - subtitleWidth) * 0.5f - 10);
    ImGui::TextDisabled("%s", subtitle);

    ImGui::Spacing();
    ImGui::Spacing();

    // Champ nom d'utilisateur
    ImGui::Text("Nom d'utilisateur");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##username", m_username, sizeof(m_username));
    
    ImGui::Spacing();

    // Champ mot de passe
    ImGui::Text("Mot de passe");
    ImGui::SetNextItemWidth(-1);
    
    ImGuiInputTextFlags passwordFlags = ImGuiInputTextFlags_None;
    if (!m_showPassword)
    {
        passwordFlags |= ImGuiInputTextFlags_Password;
    }
    ImGui::InputText("##password", m_password, sizeof(m_password), passwordFlags);

    // Case à cocher pour afficher le mot de passe
    ImGui::Checkbox("Afficher le mot de passe", &m_showPassword);

    ImGui::Spacing();
    ImGui::Spacing();

    // Affichage de l'erreur si présente
    if (m_loginError)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("%s", m_errorMessage.c_str());
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    // Boutons de connexion/inscription côte à côte
    float totalButtonWidth = 360.0f;
    float singleButtonWidth = 175.0f;
    ImGui::SetCursorPosX((formWidth - totalButtonWidth) * 0.5f - 10);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.45f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.5f, 0.25f, 1.0f));
    
    // Bouton Connexion
    if (ImGui::Button("Miaou! (Connexion)", ImVec2(singleButtonWidth, 35)))
    {
        m_loginError = false;
        m_successMessage.clear();
        if (m_client->Login(m_username, m_password))
        {
            memset(m_password, 0, sizeof(m_password));
        }
        else
        {
            m_loginError = true;
            m_errorMessage = m_client->GetLastError();
        }
    }
    
    ImGui::SameLine();
    
    // Bouton Inscription
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.5f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.6f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.45f, 0.3f, 1.0f));
    
    if (ImGui::Button("Nouveau chaton?", ImVec2(singleButtonWidth, 35)))
    {
        m_loginError = false;
        m_successMessage.clear();
        if (m_client->Register(m_username, m_password))
        {
            memset(m_password, 0, sizeof(m_password));
            m_successMessage = "Bienvenue petit chaton!";
        }
        else
        {
            m_loginError = true;
            m_errorMessage = m_client->GetLastError();
        }
    }
    
    ImGui::PopStyleColor(6);

    ImGui::Spacing();
    ImGui::Spacing();

    // Message de succès
    if (!m_successMessage.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        float successWidth = ImGui::CalcTextSize(m_successMessage.c_str()).x;
        ImGui::SetCursorPosX((formWidth - successWidth) * 0.5f - 10);
        ImGui::Text("%s", m_successMessage.c_str());
        ImGui::PopStyleColor();
    }

    // Information sur le serveur
    ImGui::TextDisabled("Serveur: vault.buffertavern.com");

    ImGui::EndChild();
}

/**
 * @brief Affiche l'interface principale de chat
 */
void ChatWindow::RenderChatInterface()
{
    // Barre de titre
    RenderTitleBar();

    // Zone principale avec sidebar et messages
    float sidebarWidth = 250.0f;
    float availableHeight = ImGui::GetContentRegionAvail().y;

    // Barre latérale (liste des salons)
    ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, availableHeight), true);
    RenderSidebar();
    ImGui::EndChild();

    ImGui::SameLine();

    // Zone de chat principale
    ImGui::BeginChild("ChatArea", ImVec2(0, availableHeight), true);
    
    // Zone des messages (prend tout l'espace sauf la zone de saisie)
    float inputAreaHeight = 60.0f;
    ImGui::BeginChild("Messages", ImVec2(0, -inputAreaHeight), true);
    RenderMessageArea();
    ImGui::EndChild();

    // Zone de saisie
    RenderInputArea();

    ImGui::EndChild();
}

/**
 * @brief Affiche la barre de titre avec les informations de connexion
 */
void ChatWindow::RenderTitleBar()
{
    ImGui::BeginChild("TitleBar", ImVec2(0, 40), true);

    // Logo/Titre avec pattes de chat
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "=^.^=");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Kitty Chat");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "=^.^=");
    
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Statut de connexion
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", m_client->GetConnectionStatus().c_str());

    // Utilisateur connecté (aligné à droite)
    std::string userInfo = m_client->GetUserId();
    float textWidth = ImGui::CalcTextSize(userInfo.c_str()).x;
    float buttonWidth = 100.0f;
    
    ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - buttonWidth - 40);
    ImGui::Text("%s", userInfo.c_str());
    
    ImGui::SameLine();
    
    // Bouton de déconnexion
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("Faire dodo", ImVec2(buttonWidth, 0)))
    {
        m_client->Logout();
    }
    ImGui::PopStyleColor();

    ImGui::EndChild();

    ImGui::Spacing();
}

/**
 * @brief Affiche la liste des salons dans la barre latérale
 */
void ChatWindow::RenderSidebar()
{
    // En-tête avec patte de chat
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), " ~(=^..^)");
    ImGui::SameLine();
    ImGui::Text("Salons");
    ImGui::Separator();
    ImGui::Spacing();

    // Boutons pour créer/rejoindre un salon
    float buttonWidth = (ImGui::GetContentRegionAvail().x - 5) / 2;
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.5f, 0.35f, 1.0f));
    if (ImGui::Button("+ Creer", ImVec2(buttonWidth, 25)))
    {
        m_showCreateRoom = true;
        memset(m_newRoomName, 0, sizeof(m_newRoomName));
    }
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.35f, 0.5f, 1.0f));
    if (ImGui::Button("Rejoindre", ImVec2(buttonWidth, 25)))
    {
        m_showJoinRoom = true;
        memset(m_joinRoomId, 0, sizeof(m_joinRoomId));
    }
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const auto& rooms = m_client->GetRooms();
    const Room* selectedRoom = m_client->GetSelectedRoom();

    if (rooms.empty())
    {
        ImGui::TextDisabled("Aucun salon...");
        ImGui::Spacing();
        ImGui::TextWrapped("Creez un salon ou rejoignez-en un avec les boutons ci-dessus!");
    }
    else
    {
        for (const auto& room : rooms)
        {
            // Détermine si ce salon est sélectionné
            bool isSelected = (selectedRoom && selectedRoom->id == room.id);

            // Style différent pour le salon sélectionné
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.35f, 0.2f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.2f, 0.15f, 1.0f));
            }

            // Affichage du nom du salon avec badge si messages non lus
            std::string displayName = room.name;
            if (room.unreadCount > 0)
            {
                displayName += " (" + std::to_string(room.unreadCount) + ")";
            }

            if (ImGui::Button(displayName.c_str(), ImVec2(-1, 30)))
            {
                m_client->SelectRoom(room.id);
                m_scrollToBottom = true;
            }

            ImGui::PopStyleColor();
        }
    }

    // Espace flexible pour pousser le footer vers le bas
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 50);
    ImGui::Separator();
    ImGui::Spacing();
    
    // Nombre de salons
    ImGui::TextDisabled("%d salon(s)", static_cast<int>(rooms.size()));
    
    // Popup pour créer un salon
    if (m_showCreateRoom)
    {
        ImGui::OpenPopup("Creer un salon");
    }
    
    if (ImGui::BeginPopupModal("Creer un salon", &m_showCreateRoom, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Nom du nouveau salon:");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##newroom", m_newRoomName, sizeof(m_newRoomName));
        
        ImGui::Spacing();
        
        if (ImGui::Button("Creer", ImVec2(95, 0)))
        {
            if (strlen(m_newRoomName) > 0)
            {
                if (m_client->CreateRoom(m_newRoomName))
                {
                    m_showCreateRoom = false;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Annuler", ImVec2(95, 0)))
        {
            m_showCreateRoom = false;
        }
        
        if (!m_client->GetLastError().empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_client->GetLastError().c_str());
        }
        
        ImGui::EndPopup();
    }
    
    // Popup pour rejoindre un salon
    if (m_showJoinRoom)
    {
        ImGui::OpenPopup("Rejoindre un salon");
    }
    
    if (ImGui::BeginPopupModal("Rejoindre un salon", &m_showJoinRoom, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("ID ou alias du salon:");
        ImGui::TextDisabled("Ex: #general:vault.buffertavern.com");
        ImGui::SetNextItemWidth(250);
        ImGui::InputText("##joinroom", m_joinRoomId, sizeof(m_joinRoomId));
        
        ImGui::Spacing();
        
        if (ImGui::Button("Rejoindre", ImVec2(95, 0)))
        {
            if (strlen(m_joinRoomId) > 0)
            {
                if (m_client->JoinRoom(m_joinRoomId))
                {
                    m_showJoinRoom = false;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Annuler", ImVec2(95, 0)))
        {
            m_showJoinRoom = false;
        }
        
        if (!m_client->GetLastError().empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", m_client->GetLastError().c_str());
        }
        
        ImGui::EndPopup();
    }
}

/**
 * @brief Affiche les messages du salon actif
 */
void ChatWindow::RenderMessageArea()
{
    const Room* room = m_client->GetSelectedRoom();

    if (!room)
    {
        // Aucun salon sélectionné - afficher un message d'accueil avec chat
        ImVec2 windowSize = ImGui::GetWindowSize();
        float textHeight = 180.0f;
        ImGui::SetCursorPosY((windowSize.y - textHeight) * 0.5f);

        // Grand chat ASCII au centre
        const char* bigCat = 
            "      /\\_____/\\      \n"
            "     /  o   o  \\     \n"
            "    ( ==  ^  == )    \n"
            "     )         (     \n"
            "    (           )    \n"
            "   ( (  )   (  ) )   \n"
            "  (__(__)___(__)__)  ";
        
        float catWidth = ImGui::CalcTextSize("      /\\_____/\\      ").x;
        ImGui::SetCursorPosX((windowSize.x - catWidth) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "%s", bigCat);
        
        ImGui::Spacing();
        ImGui::Spacing();

        const char* welcomeText = "Miaou ! Bienvenue sur Kitty Chat !";
        float textWidth = ImGui::CalcTextSize(welcomeText).x;
        ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", welcomeText);

        const char* helpText = "Selectionnez un salon pour commencer a ronronner...";
        textWidth = ImGui::CalcTextSize(helpText).x;
        ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
        ImGui::TextDisabled("%s", helpText);

        return;
    }

    // Affichage des messages
    for (const auto& message : room->messages)
    {
        RenderMessage(message);
    }

    // Défilement automatique vers le bas si nécessaire
    if (m_scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }
}

/**
 * @brief Affiche un message individuel
 */
void ChatWindow::RenderMessage(const Message& message)
{
    ImGui::PushID(message.id.c_str());

    // Espacement entre les messages
    ImGui::Spacing();

    // Couleur différente pour nos propres messages
    if (message.isOwn)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.85f, 1.0f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.9f, 0.85f, 1.0f));
    }

    // Ligne d'en-tête : nom de l'expéditeur et timestamp
    if (message.isOwn)
    {
        ImGui::TextColored(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), "%s", message.senderName.c_str());
    }
    else
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "%s", message.senderName.c_str());
    }
    
    ImGui::SameLine();
    ImGui::TextDisabled("[%s]", message.timestamp.c_str());

    // Contenu du message
    ImGui::TextWrapped("%s", message.content.c_str());

    ImGui::PopStyleColor();
    ImGui::PopID();
}

/**
 * @brief Affiche la zone de saisie de message
 */
void ChatWindow::RenderInputArea()
{
    ImGui::Separator();
    ImGui::Spacing();

    const Room* room = m_client->GetSelectedRoom();
    bool canSend = (room != nullptr);

    // Désactiver la zone de saisie si aucun salon n'est sélectionné
    if (!canSend)
    {
        ImGui::BeginDisabled();
    }

    // Zone de saisie de texte
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 110);
    
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
    
    bool sendMessage = ImGui::InputText(
        "##MessageInput",
        m_messageInput,
        sizeof(m_messageInput),
        inputFlags
    );

    ImGui::SameLine();

    // Bouton d'envoi
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.35f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.45f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.5f, 0.25f, 1.0f));
    
    if (ImGui::Button("Miaou!", ImVec2(100, 0)) || sendMessage)
    {
        if (strlen(m_messageInput) > 0)
        {
            m_client->SendMessage(m_messageInput);
            memset(m_messageInput, 0, sizeof(m_messageInput));
            m_scrollToBottom = true;
            
            // Remettre le focus sur la zone de saisie
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    ImGui::PopStyleColor(3);

    if (!canSend)
    {
        ImGui::EndDisabled();
    }
}

