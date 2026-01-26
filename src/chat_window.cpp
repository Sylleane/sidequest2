/**
 * @file chat_window.cpp
 * @brief Implémentation de la fenêtre de chat avec GIFs animés
 * 
 * Interface utilisateur magnifique avec animations, GIFs de chats,
 * et effets visuels modernes.
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
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>

// URLs des GIFs de chats mignons (hébergés sur des CDN populaires)
static const char* CAT_GIFS[] = {
    "https://media.tenor.com/eFPFHSN4rJ8AAAAM/example.gif",  // Placeholder, sera remplacé
};

// Nombre de particules pour le fond animé
static const int NUM_PARTICLES = 50;

// Structure pour une particule d'étoile
struct Particle {
    float x, y;
    float speed;
    float size;
    float alpha;
};

static Particle g_particles[NUM_PARTICLES];
static bool g_particlesInitialized = false;

/**
 * @brief Initialise les particules
 */
static void InitParticles()
{
    if (g_particlesInitialized) return;
    
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        g_particles[i].x = (float)(rand() % 1280);
        g_particles[i].y = (float)(rand() % 720);
        g_particles[i].speed = 0.2f + (float)(rand() % 100) / 200.0f;
        g_particles[i].size = 1.0f + (float)(rand() % 30) / 10.0f;
        g_particles[i].alpha = 0.3f + (float)(rand() % 70) / 100.0f;
    }
    g_particlesInitialized = true;
}

/**
 * @brief Constructeur - Initialise les buffers et charge les GIFs
 */
ChatWindow::ChatWindow(MatrixClient* client, TextureManager* texManager)
    : m_client(client)
    , m_texManager(texManager)
    , m_showPassword(false)
    , m_loginError(false)
    , m_isRegistering(false)
    , m_scrollToBottom(true)
    , m_showCreateRoom(false)
    , m_showJoinRoom(false)
    , m_animTime(0.0f)
    , m_gifsLoaded(false)
{
    // Initialisation des buffers à zéro
    memset(m_username, 0, sizeof(m_username));
    memset(m_password, 0, sizeof(m_password));
    memset(m_messageInput, 0, sizeof(m_messageInput));
    memset(m_newRoomName, 0, sizeof(m_newRoomName));
    memset(m_joinRoomId, 0, sizeof(m_joinRoomId));
    
    m_startTime = std::chrono::steady_clock::now();
    
    // Initialisation des particules
    InitParticles();
    
    // Chargement des GIFs
    LoadCatGifs();
}

/**
 * @brief Charge les GIFs de chats depuis internet
 */
void ChatWindow::LoadCatGifs()
{
    if (m_gifsLoaded || !m_texManager) return;
    
    // GIFs de chats mignons depuis Tenor/Giphy
    m_texManager->LoadGifFromUrl(
        "https://media.tenor.com/eFPFHSN4rJ8AAAAM/happy-cat.gif",
        "happy_cat"
    );
    m_texManager->LoadGifFromUrl(
        "https://media.tenor.com/yk0zpZpPZzsAAAAM/typing-cat.gif",
        "typing_cat"
    );
    m_texManager->LoadGifFromUrl(
        "https://media.tenor.com/vJWpL9b8n8QAAAAM/cat-cute.gif",
        "cute_cat"
    );
    m_texManager->LoadGifFromUrl(
        "https://media.tenor.com/wkbs_8uGKUAAAAAM/kitten-cat.gif",
        "kitten"
    );
    m_texManager->LoadGifFromUrl(
        "https://media.tenor.com/2zJhG8yyf0YAAAAM/cats-cat.gif",
        "wave_cat"
    );
    
    m_gifsLoaded = true;
}

/**
 * @brief Dessine le fond animé avec des particules et des dégradés
 */
void ChatWindow::RenderAnimatedBackground()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    
    // Mise à jour du temps d'animation
    auto now = std::chrono::steady_clock::now();
    m_animTime = std::chrono::duration<float>(now - m_startTime).count();
    
    // Dégradé de fond - du brun foncé vers le violet/rose
    ImU32 topColor = IM_COL32(25, 20, 35, 255);      // Violet foncé
    ImU32 midColor = IM_COL32(45, 30, 50, 255);      // Mauve
    ImU32 bottomColor = IM_COL32(60, 35, 45, 255);   // Rose foncé
    
    // Premier dégradé (haut vers milieu)
    drawList->AddRectFilledMultiColor(
        windowPos,
        ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y * 0.5f),
        topColor, topColor, midColor, midColor
    );
    
    // Deuxième dégradé (milieu vers bas)
    drawList->AddRectFilledMultiColor(
        ImVec2(windowPos.x, windowPos.y + windowSize.y * 0.5f),
        ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
        midColor, midColor, bottomColor, bottomColor
    );
    
    // Dessiner les particules/étoiles
    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        Particle& p = g_particles[i];
        
        // Animation douce
        float offsetY = sinf(m_animTime * p.speed + i) * 20.0f;
        float offsetX = cosf(m_animTime * p.speed * 0.5f + i) * 10.0f;
        
        float px = windowPos.x + p.x + offsetX;
        float py = windowPos.y + p.y + offsetY;
        
        // Wrap around
        if (px < windowPos.x) px += windowSize.x;
        if (px > windowPos.x + windowSize.x) px -= windowSize.x;
        if (py < windowPos.y) py += windowSize.y;
        if (py > windowPos.y + windowSize.y) py -= windowSize.y;
        
        // Scintillement
        float twinkle = 0.5f + 0.5f * sinf(m_animTime * 3.0f + i * 0.7f);
        int alpha = (int)(p.alpha * twinkle * 255);
        
        // Couleur dorée/orange pour les étoiles
        ImU32 starColor = IM_COL32(255, 180 + (int)(twinkle * 75), 100, alpha);
        
        drawList->AddCircleFilled(
            ImVec2(px, py),
            p.size,
            starColor
        );
        
        // Halo autour des grosses étoiles
        if (p.size > 2.5f)
        {
            drawList->AddCircleFilled(
                ImVec2(px, py),
                p.size * 2.0f,
                IM_COL32(255, 200, 150, alpha / 4)
            );
        }
    }
    
    // Emojis de pattes de chat flottants
    const char* pawEmojis[] = { "🐾", "✨", "💫", "⭐" };
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 0.15f));
    for (int i = 0; i < 8; i++)
    {
        float x = fmodf(100.0f + i * 150.0f + m_animTime * 5.0f, windowSize.x);
        float y = 100.0f + sinf(m_animTime * 0.5f + i) * 30.0f + i * 80.0f;
        y = fmodf(y, windowSize.y);
        
        ImGui::SetCursorPos(ImVec2(x, y));
        ImGui::Text("%s", pawEmojis[i % 4]);
    }
    ImGui::PopStyleColor();
}

/**
 * @brief Affiche un GIF animé centré
 */
void ChatWindow::RenderGif(const std::string& name, float maxWidth, float maxHeight)
{
    if (!m_texManager) return;
    
    m_texManager->Update();
    
    ID3D11ShaderResourceView* tex = m_texManager->GetCurrentFrame(name);
    if (tex)
    {
        int gifW, gifH;
        if (m_texManager->GetGifSize(name, gifW, gifH))
        {
            // Calculer la taille avec aspect ratio
            float scale = std::min(maxWidth / gifW, maxHeight / gifH);
            float displayW = gifW * scale;
            float displayH = gifH * scale;
            
            // Centrer horizontalement
            float availWidth = ImGui::GetContentRegionAvail().x;
            float offsetX = (availWidth - displayW) * 0.5f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
            
            // Afficher l'image avec bordure arrondie
            ImGui::Image((ImTextureID)tex, ImVec2(displayW, displayH));
        }
    }
    else if (!m_texManager->IsLoaded(name))
    {
        // Afficher un placeholder pendant le chargement
        float pulse = 0.5f + 0.5f * sinf(m_animTime * 3.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.4f, pulse));
        
        float textWidth = ImGui::CalcTextSize("Chargement du chat...").x;
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - textWidth) * 0.5f);
        ImGui::Text("Chargement du chat...");
        
        // Spinner avec patte de chat
        const char* frames[] = { "🐾", "  🐾", "    🐾", "  🐾" };
        int frameIdx = (int)(m_animTime * 4.0f) % 4;
        float spinnerWidth = ImGui::CalcTextSize(frames[frameIdx]).x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - spinnerWidth) * 0.5f);
        ImGui::Text("%s", frames[frameIdx]);
        
        ImGui::PopStyleColor();
    }
}

/**
 * @brief Point d'entrée principal du rendu
 */
void ChatWindow::Render()
{
    // Configuration de la fenêtre principale
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags windowFlags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Kitty Chat", nullptr, windowFlags);
    ImGui::PopStyleVar();

    // Fond animé avec dégradé et particules
    RenderAnimatedBackground();
    
    // Mise à jour des animations de textures
    if (m_texManager)
    {
        m_texManager->Update();
    }

    // Conteneur principal avec padding
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImVec2 contentSize = ImVec2(
        viewport->Size.x - 40,
        viewport->Size.y - 40
    );
    
    ImGui::BeginChild("MainContent", contentSize, false);
    
    if (m_client->IsLoggedIn())
    {
        RenderChatInterface();
    }
    else
    {
        RenderLoginScreen();
    }
    
    ImGui::EndChild();
    ImGui::End();
}

/**
 * @brief Affiche l'écran de connexion avec GIF animé
 */
void ChatWindow::RenderLoginScreen()
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    float formWidth = 450.0f;
    float formHeight = 500.0f;
    
    // Centrage
    ImGui::SetCursorPos(ImVec2(
        (windowSize.x - formWidth) * 0.5f,
        (windowSize.y - formHeight) * 0.5f
    ));

    // Style du formulaire avec transparence
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.08f, 0.15f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 20));
    
    ImGui::BeginChild("LoginForm", ImVec2(formWidth, formHeight), true);

    // Titre animé avec effet arc-en-ciel
    float hue = fmodf(m_animTime * 0.1f, 1.0f);
    ImVec4 titleColor = ImVec4(
        0.5f + 0.5f * sinf(hue * 6.28f),
        0.5f + 0.5f * sinf(hue * 6.28f + 2.09f),
        0.5f + 0.5f * sinf(hue * 6.28f + 4.18f),
        1.0f
    );
    
    // Grand titre centré
    const char* title = "🐱 Kitty Chat 🐱";
    float titleWidth = ImGui::CalcTextSize(title).x;
    ImGui::SetCursorPosX((formWidth - titleWidth) * 0.5f - 15);
    ImGui::PushStyleColor(ImGuiCol_Text, titleColor);
    ImGui::Text("%s", title);
    ImGui::PopStyleColor();

    ImGui::Spacing();
    
    // GIF de chat mignon
    RenderGif("cute_cat", 150, 150);
    
    ImGui::Spacing();
    ImGui::Spacing();

    // Sous-titre
    const char* subtitle = "Connectez-vous au serveur Matrix";
    float subtitleWidth = ImGui::CalcTextSize(subtitle).x;
    ImGui::SetCursorPosX((formWidth - subtitleWidth) * 0.5f - 15);
    ImGui::TextColored(ImVec4(0.7f, 0.65f, 0.8f, 1.0f), "%s", subtitle);

    ImGui::Spacing();
    ImGui::Spacing();

    // Champs de saisie avec style moderne
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 10));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.12f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.17f, 0.25f, 1.0f));
    
    ImGui::Text("👤 Nom d'utilisateur");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##username", m_username, sizeof(m_username));
    
    ImGui::Spacing();

    ImGui::Text("🔒 Mot de passe");
    ImGui::SetNextItemWidth(-1);
    ImGuiInputTextFlags passwordFlags = m_showPassword ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
    ImGui::InputText("##password", m_password, sizeof(m_password), passwordFlags);

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    // Case à cocher
    ImGui::Checkbox("👁 Afficher le mot de passe", &m_showPassword);

    ImGui::Spacing();

    // Messages d'erreur/succès
    if (m_loginError)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::TextWrapped("❌ %s", m_errorMessage.c_str());
        ImGui::PopStyleColor();
    }
    
    if (!m_successMessage.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.6f, 1.0f));
        ImGui::Text("✅ %s", m_successMessage.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Boutons avec dégradé
    float buttonWidth = (formWidth - 80) / 2;
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(20, 12));
    
    // Bouton Connexion (orange/doré)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.6f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.7f, 0.4f, 1.0f));
    
    if (ImGui::Button("🐾 Connexion", ImVec2(buttonWidth, 0)))
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
    
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    
    // Bouton Inscription (vert/émeraude)
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.8f, 0.7f, 1.0f));
    
    if (ImGui::Button("✨ S'inscrire", ImVec2(buttonWidth, 0)))
    {
        m_loginError = false;
        m_successMessage.clear();
        if (m_client->Register(m_username, m_password))
        {
            memset(m_password, 0, sizeof(m_password));
            m_successMessage = "Bienvenue petit chaton ! 🎉";
        }
        else
        {
            m_loginError = true;
            m_errorMessage = m_client->GetLastError();
        }
    }
    
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    ImGui::Spacing();
    
    // Information serveur
    const char* serverInfo = "🌐 vault.buffertavern.com";
    float serverWidth = ImGui::CalcTextSize(serverInfo).x;
    ImGui::SetCursorPosX((formWidth - serverWidth) * 0.5f - 15);
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "%s", serverInfo);

    ImGui::EndChild();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

/**
 * @brief Affiche l'interface principale de chat
 */
void ChatWindow::RenderChatInterface()
{
    // Barre de titre
    RenderTitleBar();

    float sidebarWidth = 280.0f;
    float availableHeight = ImGui::GetContentRegionAvail().y;

    // Sidebar avec style moderne
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.1f, 0.18f, 0.9f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    
    ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, availableHeight), true);
    RenderSidebar();
    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // Zone de chat principale
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.07f, 0.12f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    
    ImGui::BeginChild("ChatArea", ImVec2(0, availableHeight), true);
    
    float inputAreaHeight = 70.0f;
    
    ImGui::BeginChild("Messages", ImVec2(0, -inputAreaHeight), false);
    RenderMessageArea();
    ImGui::EndChild();

    RenderInputArea();

    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

/**
 * @brief Barre de titre avec statut et boutons
 */
void ChatWindow::RenderTitleBar()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.12f, 0.2f, 0.95f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 15.0f);
    
    ImGui::BeginChild("TitleBar", ImVec2(0, 50), true);

    // Logo avec animation
    float bounce = sinf(m_animTime * 2.0f) * 3.0f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + bounce);
    
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "🐱");
    ImGui::SameLine();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - bounce);
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "Kitty Chat");
    
    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    // Statut avec indicateur
    bool connected = !m_client->GetConnectionStatus().empty();
    ImVec4 statusColor = connected ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) : ImVec4(0.9f, 0.4f, 0.3f, 1.0f);
    ImGui::TextColored(statusColor, "● %s", m_client->GetConnectionStatus().c_str());

    // Utilisateur (aligné à droite)
    std::string userInfo = "👤 " + m_client->GetUserId();
    float textWidth = ImGui::CalcTextSize(userInfo.c_str()).x;
    float buttonWidth = 130.0f;
    
    ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - buttonWidth - 50);
    ImGui::TextColored(ImVec4(0.8f, 0.75f, 0.9f, 1.0f), "%s", userInfo.c_str());
    
    ImGui::SameLine();
    
    // Bouton déconnexion
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.35f, 0.35f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    
    if (ImGui::Button("😴 Dodo", ImVec2(buttonWidth, 0)))
    {
        m_client->Logout();
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
}

/**
 * @brief Liste des salons
 */
void ChatWindow::RenderSidebar()
{
    // En-tête
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "🏠 Vos Salons");
    ImGui::Separator();
    ImGui::Spacing();

    // Boutons créer/rejoindre
    float buttonWidth = (ImGui::GetContentRegionAvail().x - 10) / 2;
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.55f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.65f, 0.5f, 1.0f));
    if (ImGui::Button("➕ Créer", ImVec2(buttonWidth, 30)))
    {
        m_showCreateRoom = true;
        memset(m_newRoomName, 0, sizeof(m_newRoomName));
    }
    ImGui::PopStyleColor(2);
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.4f, 0.6f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.5f, 0.7f, 1.0f));
    if (ImGui::Button("🚪 Rejoindre", ImVec2(buttonWidth, 30)))
    {
        m_showJoinRoom = true;
        memset(m_joinRoomId, 0, sizeof(m_joinRoomId));
    }
    ImGui::PopStyleColor(2);
    
    ImGui::PopStyleVar();
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    const auto& rooms = m_client->GetRooms();
    const Room* selectedRoom = m_client->GetSelectedRoom();

    if (rooms.empty())
    {
        ImGui::TextColored(ImVec4(0.6f, 0.55f, 0.7f, 1.0f), "Aucun salon...");
        ImGui::Spacing();
        
        // Mini GIF d'attente
        RenderGif("kitten", 100, 100);
        
        ImGui::Spacing();
        ImGui::TextWrapped("Créez ou rejoignez un salon pour commencer !");
    }
    else
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        
        for (const auto& room : rooms)
        {
            bool isSelected = (selectedRoom && selectedRoom->id == room.id);

            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.35f, 0.55f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.4f, 0.6f, 1.0f));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.18f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.25f, 0.35f, 1.0f));
            }

            std::string displayName = "💬 " + room.name;
            if (room.unreadCount > 0)
            {
                displayName += " 🔴 " + std::to_string(room.unreadCount);
            }

            if (ImGui::Button(displayName.c_str(), ImVec2(-1, 35)))
            {
                m_client->SelectRoom(room.id);
                m_scrollToBottom = true;
            }

            ImGui::PopStyleColor(2);
        }
        
        ImGui::PopStyleVar();
    }

    // Footer
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 60);
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "📊 %d salon(s)", static_cast<int>(rooms.size()));
    
    // Popups
    if (m_showCreateRoom)
        ImGui::OpenPopup("Créer un salon");
    
    if (ImGui::BeginPopupModal("Créer un salon", &m_showCreateRoom, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("🏷️ Nom du nouveau salon:");
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::SetNextItemWidth(250);
        ImGui::InputText("##newroom", m_newRoomName, sizeof(m_newRoomName));
        ImGui::PopStyleVar();
        
        ImGui::Spacing();
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        if (ImGui::Button("✅ Créer", ImVec2(120, 0)))
        {
            if (strlen(m_newRoomName) > 0 && m_client->CreateRoom(m_newRoomName))
            {
                m_showCreateRoom = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("❌ Annuler", ImVec2(120, 0)))
        {
            m_showCreateRoom = false;
        }
        ImGui::PopStyleVar();
        
        ImGui::EndPopup();
    }
    
    if (m_showJoinRoom)
        ImGui::OpenPopup("Rejoindre un salon");
    
    if (ImGui::BeginPopupModal("Rejoindre un salon", &m_showJoinRoom, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("🔗 ID ou alias du salon:");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "Ex: #general:vault.buffertavern.com");
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##joinroom", m_joinRoomId, sizeof(m_joinRoomId));
        ImGui::PopStyleVar();
        
        ImGui::Spacing();
        
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        if (ImGui::Button("✅ Rejoindre", ImVec2(140, 0)))
        {
            if (strlen(m_joinRoomId) > 0 && m_client->JoinRoom(m_joinRoomId))
            {
                m_showJoinRoom = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("❌ Annuler", ImVec2(140, 0)))
        {
            m_showJoinRoom = false;
        }
        ImGui::PopStyleVar();
        
        ImGui::EndPopup();
    }
}

/**
 * @brief Zone des messages
 */
void ChatWindow::RenderMessageArea()
{
    const Room* room = m_client->GetSelectedRoom();

    if (!room)
    {
        // Écran d'accueil avec GIF
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPosY((windowSize.y - 300) * 0.5f);

        // GIF de chat qui fait coucou
        RenderGif("wave_cat", 200, 200);
        
        ImGui::Spacing();
        ImGui::Spacing();

        const char* welcomeText = "🐱 Miaou ! Bienvenue sur Kitty Chat ! 🐱";
        float textWidth = ImGui::CalcTextSize(welcomeText).x;
        ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "%s", welcomeText);

        const char* helpText = "Sélectionnez un salon pour commencer à ronronner...";
        textWidth = ImGui::CalcTextSize(helpText).x;
        ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.6f, 0.55f, 0.7f, 1.0f), "%s", helpText);

        return;
    }

    // Affichage des messages
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 12));
    
    for (const auto& message : room->messages)
    {
        RenderMessage(message);
    }
    
    ImGui::PopStyleVar();

    if (m_scrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }
}

/**
 * @brief Affiche un message avec style moderne
 */
void ChatWindow::RenderMessage(const Message& message)
{
    ImGui::PushID(message.id.c_str());

    // Bulle de message
    bool isOwn = message.isOwn;
    
    ImVec4 bubbleColor = isOwn 
        ? ImVec4(0.35f, 0.45f, 0.6f, 0.8f)   // Bleu pour nos messages
        : ImVec4(0.25f, 0.22f, 0.35f, 0.8f); // Violet foncé pour les autres
    
    ImVec4 nameColor = isOwn
        ? ImVec4(0.6f, 0.8f, 1.0f, 1.0f)
        : ImVec4(1.0f, 0.75f, 0.5f, 1.0f);

    // Indentation pour nos messages
    if (isOwn)
    {
        float indent = ImGui::GetContentRegionAvail().x * 0.2f;
        ImGui::Indent(indent);
    }

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bubbleColor);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
    
    // Calculer la taille du message
    float maxWidth = ImGui::GetContentRegionAvail().x * 0.75f;
    ImVec2 textSize = ImGui::CalcTextSize(message.content.c_str(), nullptr, false, maxWidth - 24);
    float bubbleHeight = textSize.y + 35;
    
    ImGui::BeginChild(("msg_" + message.id).c_str(), ImVec2(maxWidth, bubbleHeight), true);
    
    // Nom et timestamp
    ImGui::TextColored(nameColor, "%s", message.senderName.c_str());
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.6f, 1.0f), "[%s]", message.timestamp.c_str());
    
    // Contenu
    ImGui::TextWrapped("%s", message.content.c_str());
    
    ImGui::EndChild();
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();

    if (isOwn)
    {
        ImGui::Unindent(ImGui::GetContentRegionAvail().x * 0.2f);
    }

    ImGui::PopID();
}

/**
 * @brief Zone de saisie avec style moderne
 */
void ChatWindow::RenderInputArea()
{
    ImGui::Separator();
    ImGui::Spacing();

    const Room* room = m_client->GetSelectedRoom();
    bool canSend = (room != nullptr);

    if (!canSend)
    {
        ImGui::BeginDisabled();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 10));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.13f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.18f, 0.25f, 1.0f));
    
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 130);
    
    bool sendMessage = ImGui::InputText(
        "##MessageInput",
        m_messageInput,
        sizeof(m_messageInput),
        ImGuiInputTextFlags_EnterReturnsTrue
    );

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

    ImGui::SameLine();

    // Bouton d'envoi stylisé
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.5f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.6f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.7f, 0.5f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 15.0f);
    
    if (ImGui::Button("🐾 Miaou!", ImVec2(120, 40)) || sendMessage)
    {
        if (strlen(m_messageInput) > 0)
        {
            m_client->SendMessage(m_messageInput);
            memset(m_messageInput, 0, sizeof(m_messageInput));
            m_scrollToBottom = true;
            ImGui::SetKeyboardFocusHere(-1);
        }
    }
    
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    if (!canSend)
    {
        ImGui::EndDisabled();
    }
}
