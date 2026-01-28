/**
 * @file chat_window.cpp
 * @brief Implémentation de la fenêtre de chat avec GIFs animés
 * 
 * Interface utilisateur magnifique avec animations, GIFs de chats,
 * et effets visuels modernes.
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
    , m_catEyeTargetX(0.0f)
    , m_catEyeTargetY(0.0f)
    , m_catEyeCurrentX(0.0f)
    , m_catEyeCurrentY(0.0f)
    , m_tailCoverAmount(0.0f)
    , m_layDownAmount(0.0f)
    , m_peekAmount(0.0f)
    , m_passwordFieldFocused(false)
    , m_catBodyRotation(0.0f)
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
 * @brief Charge les GIFs de chats depuis cataas.com (Cat As A Service)
 */
void ChatWindow::LoadCatGifs()
{
    if (m_gifsLoaded || !m_texManager) return;
    
    // Utiliser cataas.com - service fiable pour les GIFs de chats
    // Format: https://cataas.com/cat/gif?type=sq (carré)
    
    // Chat normal - GIF animé
    m_texManager->LoadGifFromUrl(
        "https://cataas.com/cat/gif",
        "cat_looking"
    );
    
    // On utilise le même GIF pour tous les états pour l'instant
    // car cataas ne permet pas de choisir le type de chat
    m_texManager->LoadGifFromUrl(
        "https://cataas.com/cat/gif",
        "cat_sleeping"
    );
    
    m_texManager->LoadGifFromUrl(
        "https://cataas.com/cat/gif",
        "cat_peeking"
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
 * @brief Affiche l'écran de connexion avec chat interactif
 */
void ChatWindow::RenderLoginScreen()
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    float formWidth = 450.0f;
    float formHeight = 550.0f;  // Un peu plus haut pour le chat
    
    // Position du formulaire
    float formX = (windowSize.x - formWidth) * 0.5f;
    float formY = (windowSize.y - formHeight) * 0.5f;
    
    // Centrage
    ImGui::SetCursorPos(ImVec2(formX, formY));

    // Style du formulaire avec transparence
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.08f, 0.15f, 0.85f));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(30, 20));
    
    ImGui::BeginChild("LoginForm", ImVec2(formWidth, formHeight), true);
    
    // Récupérer la position absolue du formulaire pour le chat
    ImVec2 formPos = ImGui::GetWindowPos();

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
    
    // Chat interactif qui suit le curseur !
    // Calculer le centre du chat dans le formulaire
    float catCenterX = formWidth * 0.5f - 15;
    float catCenterY = ImGui::GetCursorPosY() + 70;  // Position Y actuelle + offset
    float catSize = 120.0f;
    
    // ASCII Art de chat selon l'état - FONCTIONNE TOUJOURS
    const char* catArt;
    const char* subtitle;
    ImVec4 catColor;
    
    if (m_passwordFieldFocused && m_showPassword)
    {
        // Peeking - le chat jette un œil
        catArt = 
            "    /\\_____/\\    \n"
            "   /  o   -  \\   \n"
            "  ( ==  w  == )  \n"
            "   )  ~ ^ ~  (   \n"
            "  (  peeking! )  \n"
            "   (  _   _  )   \n"
            "    ~~     ~~    ";
        subtitle = "Bon, je jette un petit coup d'oeil... 👀";
        catColor = ImVec4(1.0f, 0.85f, 0.5f, 1.0f);  // Jaune/Or
    }
    else if (m_passwordFieldFocused)
    {
        // Sleeping - le chat dort avec Zzz
        catArt = 
            "    /\\_____/\\   z\n"
            "   /  -   -  \\ z \n"
            "  ( ==  w  == )  \n"
            "   )   ___   (   \n"
            "  (   /   \\   )  \n"
            "   \\_/     \\_/   \n"
            "      ~~~~       ";
        subtitle = "Zzz... Je ne regarde pas, promis ! 😴";
        catColor = ImVec4(0.7f, 0.7f, 1.0f, 1.0f);  // Bleu clair (endormi)
    }
    else
    {
        // Normal - chat curieux avec yeux ouverts
        catArt = 
            "    /\\_____/\\    \n"
            "   /  o   o  \\   \n"
            "  ( ==  ^  == )  \n"
            "   )         (   \n"
            "  (           )  \n"
            "   (  )   (  )   \n"
            "    ~~     ~~    ";
        subtitle = "Connectez-vous au serveur Matrix";
        catColor = ImVec4(1.0f, 0.75f, 0.4f, 1.0f);  // Orange (éveillé)
    }
    
    // Centrer et afficher le chat ASCII
    float catWidth = ImGui::CalcTextSize("    /\\_____/\\    ").x;
    ImGui::SetCursorPosX((formWidth - catWidth) * 0.5f - 10);
    ImGui::PushStyleColor(ImGuiCol_Text, catColor);
    ImGui::Text("%s", catArt);
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
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

    // Champ mot de passe avec bouton œil pour révéler
    ImGui::Text("🔒 Mot de passe");
    
    float eyeButtonWidth = 40.0f;
    float inputWidth = ImGui::GetContentRegionAvail().x - eyeButtonWidth - 8.0f;
    
    ImGui::SetNextItemWidth(inputWidth);
    ImGuiInputTextFlags passwordFlags = m_showPassword ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
    ImGui::InputText("##password", m_password, sizeof(m_password), passwordFlags);
    
    // Vérifier si le champ mot de passe a le focus
    m_passwordFieldFocused = ImGui::IsItemActive();
    
    // Bouton œil pour révéler/cacher le mot de passe
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.22f, 0.35f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.30f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.45f, 0.40f, 0.55f, 1.0f));
    
    // L'icône change selon l'état
    const char* eyeIcon = m_showPassword ? "🙈" : "👁";
    if (ImGui::Button(eyeIcon, ImVec2(eyeButtonWidth, 0)))
    {
        m_showPassword = !m_showPassword;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip(m_showPassword ? "Cacher le mot de passe" : "Afficher le mot de passe");
    }
    
    ImGui::PopStyleColor(3);

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);

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
        
        // Mini chat ASCII d'attente
        const char* miniCat = 
            "  /\\_/\\  \n"
            " ( o.o ) \n"
            "  > ^ <  ";
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.4f, 1.0f), "%s", miniCat);
        
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
        // Écran d'accueil avec grand chat ASCII
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGui::SetCursorPosY((windowSize.y - 250) * 0.5f);

        // Grand chat ASCII qui fait coucou
        const char* bigCat = 
            "      /\\_____/\\      \n"
            "     /  o   o  \\     \n"
            "    ( ==  ^  == )    \n"
            "     )  ~~~~~  (     \n"
            "    (   \\   /   )    \n"
            "   ( (  ) ^ (  ) )   \n"
            "  (__(__)   (__)__)  ";
        
        float catWidth = ImGui::CalcTextSize("      /\\_____/\\      ").x;
        ImGui::SetCursorPosX((windowSize.x - catWidth) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.4f, 1.0f), "%s", bigCat);
        
        ImGui::Spacing();
        ImGui::Spacing();

        const char* welcomeText = "Miaou ! Bienvenue sur Kitty Chat !";
        float textWidth = ImGui::CalcTextSize(welcomeText).x;
        ImGui::SetCursorPosX((windowSize.x - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), "%s", welcomeText);

        const char* helpText = "Selectionnez un salon pour commencer a ronronner...";
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

/**
 * @brief Met à jour l'animation du chat (yeux, pose, queue)
 */
void ChatWindow::UpdateCatAnimation()
{
    // Interpolation douce des yeux vers la cible
    float smoothing = 0.15f;
    m_catEyeCurrentX += (m_catEyeTargetX - m_catEyeCurrentX) * smoothing;
    m_catEyeCurrentY += (m_catEyeTargetY - m_catEyeCurrentY) * smoothing;
    
    // Animation "se coucher" quand on tape le mot de passe
    float targetLayDown = m_passwordFieldFocused ? 1.0f : 0.0f;
    m_layDownAmount += (targetLayDown - m_layDownAmount) * 0.08f;
    
    // Animation "peek" quand on révèle le mot de passe
    float targetPeek = (m_passwordFieldFocused && m_showPassword) ? 1.0f : 0.0f;
    m_peekAmount += (targetPeek - m_peekAmount) * 0.12f;
    
    // Animation de la queue pour cacher les yeux
    // La queue couvre moins quand le chat peek
    float targetTailCover = m_passwordFieldFocused ? (1.0f - m_peekAmount * 0.7f) : 0.0f;
    m_tailCoverAmount += (targetTailCover - m_tailCoverAmount) * 0.1f;
}

/**
 * @brief Dessine le chat interactif avec 3 poses : assis, couché, peek
 */
void ChatWindow::RenderInteractiveCat(float centerX, float centerY, float size)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    
    // Position absolue du chat
    float catX = windowPos.x + centerX;
    float catY = windowPos.y + centerY;
    
    // Récupérer la position de la souris
    ImVec2 mousePos = ImGui::GetMousePos();
    
    // Calculer la direction vers la souris (seulement si pas couché)
    float dx = mousePos.x - catX;
    float dy = mousePos.y - catY;
    float distance = sqrtf(dx * dx + dy * dy);
    
    // Les yeux suivent le curseur seulement si le chat n'est pas couché
    float eyeTrackingStrength = 1.0f - m_layDownAmount * 0.8f + m_peekAmount * 0.5f;
    float maxEyeMove = size * 0.08f;
    if (distance > 0)
    {
        m_catEyeTargetX = (dx / distance) * std::min(distance * 0.02f, maxEyeMove) * eyeTrackingStrength;
        m_catEyeTargetY = (dy / distance) * std::min(distance * 0.02f, maxEyeMove) * eyeTrackingStrength;
    }
    
    // Rotation du corps vers la souris (moins quand couché)
    float targetRotation = atan2f(dy, dx) * 0.1f * (1.0f - m_layDownAmount * 0.7f);
    m_catBodyRotation += (targetRotation - m_catBodyRotation) * 0.05f;
    
    // Mise à jour de l'animation
    UpdateCatAnimation();
    
    // Raccourcis pour les états
    float lay = m_layDownAmount;   // 0 = assis, 1 = couché
    float peek = m_peekAmount;     // 0 = yeux cachés, 1 = peek
    
    // === COULEURS ===
    ImU32 furColor = IM_COL32(255, 180, 120, 255);       // Orange clair
    ImU32 furDark = IM_COL32(220, 140, 80, 255);         // Orange foncé (rayures)
    ImU32 furLight = IM_COL32(255, 230, 200, 255);       // Crème (ventre)
    ImU32 eyeWhite = IM_COL32(255, 255, 255, 255);
    ImU32 eyeColor = IM_COL32(80, 200, 120, 255);        // Vert émeraude
    ImU32 pupilColor = IM_COL32(20, 20, 20, 255);
    ImU32 noseColor = IM_COL32(255, 140, 140, 255);
    ImU32 innerEarColor = IM_COL32(255, 190, 190, 255);
    ImU32 whiskerColor = IM_COL32(80, 80, 80, 200);
    ImU32 closedEyeColor = IM_COL32(60, 60, 60, 255);
    
    float s = size;
    
    // === CORPS - Change de forme selon la pose ===
    float bodyOffsetX = m_catBodyRotation * s * 0.2f * (1.0f - lay);
    
    // Assis : corps plus vertical, Couché : corps plus horizontal et bas
    float bodyCenterY = catY + s * (0.25f + lay * 0.2f);
    float bodyRadiusX = s * (0.45f + lay * 0.25f);  // Plus large couché
    float bodyRadiusY = s * (0.35f - lay * 0.1f);   // Plus plat couché
    
    // Corps principal
    drawList->AddEllipseFilled(
        ImVec2(catX + bodyOffsetX, bodyCenterY),
        bodyRadiusX, bodyRadiusY,
        furColor, 0.0f, 32
    );
    
    // Ventre (visible quand assis)
    float bellyAlpha = 1.0f - lay * 0.5f;
    ImU32 bellyColor = IM_COL32(255, 230, 200, (int)(255 * bellyAlpha));
    drawList->AddEllipseFilled(
        ImVec2(catX + bodyOffsetX, bodyCenterY + bodyRadiusY * 0.15f),
        bodyRadiusX * 0.6f, bodyRadiusY * 0.7f,
        bellyColor, 0.0f, 24
    );
    
    // Rayures sur le corps
    for (int i = 0; i < 3; i++)
    {
        float stripeX = catX + bodyOffsetX + (i - 1) * bodyRadiusX * 0.4f;
        float stripeY = bodyCenterY - bodyRadiusY * 0.3f;
        drawList->AddEllipseFilled(
            ImVec2(stripeX, stripeY),
            bodyRadiusX * 0.12f, bodyRadiusY * 0.5f,
            furDark, 0.0f, 12
        );
    }
    
    // === PATTES ===
    float pawY = bodyCenterY + bodyRadiusY * 0.7f;
    float pawSpread = s * (0.25f + lay * 0.15f);  // Plus écartées quand couché
    
    // Pattes avant
    for (int side = -1; side <= 1; side += 2)
    {
        float pawX = catX + side * pawSpread;
        float pawW = s * 0.1f;
        float pawH = s * (0.12f - lay * 0.04f);
        
        drawList->AddEllipseFilled(ImVec2(pawX, pawY), pawW, pawH, furColor, 0.0f, 12);
        // Coussinets
        drawList->AddCircleFilled(ImVec2(pawX, pawY + pawH * 0.3f), pawW * 0.6f, furLight, 10);
    }
    
    // Pattes arrière (visibles quand couché)
    if (lay > 0.3f)
    {
        float backPawX = catX - bodyRadiusX * 0.8f;
        float backPawY = bodyCenterY + bodyRadiusY * 0.3f;
        drawList->AddEllipseFilled(ImVec2(backPawX, backPawY), s * 0.12f, s * 0.08f, furColor, 0.0f, 12);
    }
    
    // === QUEUE - Enroulée autour quand couché ===
    float tailWave = sinf(m_animTime * 2.5f) * 0.08f * (1.0f - lay * 0.5f);
    
    // Base de la queue
    float tailBaseX = catX + bodyRadiusX * (0.7f - lay * 0.3f);
    float tailBaseY = bodyCenterY - bodyRadiusY * (0.2f - lay * 0.3f);
    
    // La queue s'enroule autour et peut couvrir les yeux
    float tailCurl = lay * 0.8f + m_tailCoverAmount * 0.5f;
    
    // Points de contrôle de la queue
    ImVec2 tailP1(tailBaseX, tailBaseY);
    
    // Quand couché : queue enroulée autour du corps puis monte vers le visage
    float ctrl1X = tailBaseX + s * (0.15f + tailWave) * (1.0f - tailCurl * 0.5f);
    float ctrl1Y = tailBaseY - s * 0.2f - tailCurl * s * 0.4f;
    
    // La queue monte vers le visage quand elle couvre
    float headY = catY - s * (0.2f - lay * 0.1f);
    float headRadius = s * (0.38f - lay * 0.05f);
    float eyeY = headY - headRadius * (0.05f + lay * 0.1f);
    
    float ctrl2X = catX + s * (0.1f - tailCurl * 0.2f);
    float ctrl2Y = eyeY + s * (0.3f - tailCurl * 0.4f);
    
    float endX = catX - tailCurl * s * 0.05f;
    float endY = eyeY + (1.0f - tailCurl) * s * 0.2f;
    
    ImVec2 tailCtrl1(ctrl1X, ctrl1Y);
    ImVec2 tailCtrl2(ctrl2X, ctrl2Y);
    ImVec2 tailP2(endX, endY);
    
    // Dessiner la queue
    int tailSegments = 22;
    for (int i = 0; i < tailSegments; i++)
    {
        float t = (float)i / (float)(tailSegments - 1);
        float u = 1.0f - t;
        
        float px = u*u*u*tailP1.x + 3*u*u*t*tailCtrl1.x + 3*u*t*t*tailCtrl2.x + t*t*t*tailP2.x;
        float py = u*u*u*tailP1.y + 3*u*u*t*tailCtrl1.y + 3*u*t*t*tailCtrl2.y + t*t*t*tailP2.y;
        
        float thickness = s * (0.07f - t * 0.03f) * (1.0f + tailCurl * 0.2f);
        ImU32 segColor = (i % 5 < 3) ? furColor : furDark;
        
        drawList->AddCircleFilled(ImVec2(px, py), thickness, segColor, 10);
    }
    
    // Bout duveteux
    drawList->AddCircleFilled(tailP2, s * 0.05f, furLight, 12);
    
    // === TÊTE ===
    float headOffsetY = lay * s * 0.08f;  // Tête plus basse quand couché
    headY = catY - s * (0.2f - lay * 0.12f) + headOffsetY;
    headRadius = s * (0.38f - lay * 0.03f);
    
    // Tête principale
    drawList->AddCircleFilled(ImVec2(catX, headY), headRadius, furColor, 48);
    
    // Joues
    float cheekY = headY + headRadius * 0.25f;
    drawList->AddCircleFilled(ImVec2(catX - headRadius * 0.45f, cheekY), headRadius * 0.32f, furLight, 20);
    drawList->AddCircleFilled(ImVec2(catX + headRadius * 0.45f, cheekY), headRadius * 0.32f, furLight, 20);
    
    // Rayures sur le front
    for (int i = 0; i < 3; i++)
    {
        float stripeX = catX + (i - 1) * headRadius * 0.25f;
        float stripeY = headY - headRadius * 0.5f;
        drawList->AddEllipseFilled(
            ImVec2(stripeX, stripeY),
            headRadius * 0.08f, headRadius * 0.2f,
            furDark, 0.0f, 8
        );
    }
    
    // === OREILLES ===
    float earY = headY - headRadius * 0.75f;
    float earTilt = lay * 0.15f;  // Oreilles plus plates quand couché
    
    for (int side = -1; side <= 1; side += 2)
    {
        float earX = catX + side * headRadius * 0.55f;
        
        ImVec2 e1(earX, earY + s * 0.12f);
        ImVec2 e2(earX + side * s * 0.12f, earY - s * (0.18f - earTilt));
        ImVec2 e3(earX - side * s * 0.08f, earY - s * 0.05f);
        
        drawList->AddTriangleFilled(e1, e2, e3, furColor);
        
        // Intérieur rose
        ImVec2 ei1(earX, earY + s * 0.08f);
        ImVec2 ei2(earX + side * s * 0.08f, earY - s * (0.1f - earTilt));
        ImVec2 ei3(earX - side * s * 0.04f, earY - s * 0.02f);
        drawList->AddTriangleFilled(ei1, ei2, ei3, innerEarColor);
    }
    
    // === YEUX ===
    float eyeSpacing = headRadius * 0.4f;
    eyeY = headY - headRadius * 0.05f;
    float eyeRadius = headRadius * 0.2f;
    
    // Quand couché sans peek : yeux fermés (petites lignes)
    // Quand peek : yeux mi-ouverts qui regardent
    float eyeOpenAmount = (1.0f - lay) + peek * 0.7f;
    eyeOpenAmount = std::min(1.0f, eyeOpenAmount);
    
    // Position des yeux avec suivi
    float eyeOffsetX = m_catEyeCurrentX * eyeOpenAmount;
    float eyeOffsetY = m_catEyeCurrentY * eyeOpenAmount;
    
    for (int side = -1; side <= 1; side += 2)
    {
        float eyeX = catX + side * eyeSpacing;
        
        if (eyeOpenAmount > 0.3f && m_tailCoverAmount < 0.7f)
        {
            // Yeux ouverts ou mi-ouverts
            float openScale = std::min(1.0f, eyeOpenAmount);
            float actualRadius = eyeRadius * openScale;
            
            // Blanc de l'œil (forme d'amande quand mi-ouvert)
            if (openScale > 0.5f)
            {
                drawList->AddCircleFilled(ImVec2(eyeX, eyeY), actualRadius, eyeWhite, 20);
            }
            else
            {
                // Œil mi-fermé - forme d'amande
                drawList->AddEllipseFilled(ImVec2(eyeX, eyeY), actualRadius, actualRadius * 0.5f, eyeWhite, 0.0f, 16);
            }
            
            // Iris
            float irisRadius = actualRadius * 0.65f;
            drawList->AddCircleFilled(
                ImVec2(eyeX + eyeOffsetX, eyeY + eyeOffsetY),
                irisRadius, eyeColor, 16
            );
            
            // Pupille
            float pupilW = irisRadius * 0.35f;
            float pupilH = irisRadius * 0.75f * openScale;
            drawList->AddEllipseFilled(
                ImVec2(eyeX + eyeOffsetX, eyeY + eyeOffsetY),
                pupilW, pupilH,
                pupilColor, 0.0f, 12
            );
            
            // Reflet
            drawList->AddCircleFilled(
                ImVec2(eyeX + eyeOffsetX - pupilW * 0.4f, eyeY + eyeOffsetY - pupilH * 0.3f),
                eyeRadius * 0.12f, eyeWhite, 6
            );
        }
        else
        {
            // Yeux fermés - petites courbes
            float lineY = eyeY;
            ImVec2 start(eyeX - eyeRadius * 0.8f, lineY);
            ImVec2 ctrl(eyeX, lineY + eyeRadius * 0.3f);
            ImVec2 end(eyeX + eyeRadius * 0.8f, lineY);
            drawList->AddBezierQuadratic(start, ctrl, end, closedEyeColor, 2.5f, 12);
        }
    }
    
    // === COUVERTURE PAR LA QUEUE (quand peek, on voit les yeux par-dessus) ===
    if (m_tailCoverAmount > 0.3f && peek < 0.5f)
    {
        float coverY = eyeY + eyeRadius * 0.3f;
        float coverWidth = headRadius * 0.8f * m_tailCoverAmount;
        float coverHeight = eyeRadius * 1.2f * m_tailCoverAmount;
        
        for (int i = 0; i < 5; i++)
        {
            float ox = (i - 2) * coverWidth * 0.22f;
            float sz = coverHeight * (1.0f - fabsf(i - 2) * 0.12f);
            ImU32 col = (i % 2 == 0) ? furColor : furDark;
            drawList->AddCircleFilled(ImVec2(catX + ox, coverY), sz, col, 14);
        }
    }
    
    // === NEZ ===
    float noseY = headY + headRadius * 0.25f;
    float noseSize = s * 0.045f;
    ImVec2 n1(catX, noseY - noseSize);
    ImVec2 n2(catX - noseSize, noseY + noseSize * 0.5f);
    ImVec2 n3(catX + noseSize, noseY + noseSize * 0.5f);
    drawList->AddTriangleFilled(n1, n2, n3, noseColor);
    
    // === BOUCHE ===
    float mouthY = noseY + s * 0.04f;
    
    // Ligne centrale
    drawList->AddLine(
        ImVec2(catX, noseY + noseSize * 0.3f),
        ImVec2(catX, mouthY + s * 0.02f),
        closedEyeColor, 2.0f
    );
    
    // Sourire (plus petit quand couché/endormi)
    float smileSize = s * (0.1f - lay * 0.03f + peek * 0.02f);
    drawList->AddBezierQuadratic(
        ImVec2(catX, mouthY),
        ImVec2(catX - smileSize, mouthY + smileSize * 0.5f),
        ImVec2(catX - smileSize * 1.2f, mouthY - smileSize * 0.1f),
        closedEyeColor, 1.8f, 10
    );
    drawList->AddBezierQuadratic(
        ImVec2(catX, mouthY),
        ImVec2(catX + smileSize, mouthY + smileSize * 0.5f),
        ImVec2(catX + smileSize * 1.2f, mouthY - smileSize * 0.1f),
        closedEyeColor, 1.8f, 10
    );
    
    // === MOUSTACHES ===
    float whiskerBaseY = noseY + s * 0.01f;
    float whiskerLen = s * (0.25f + peek * 0.05f);
    
    for (int side = -1; side <= 1; side += 2)
    {
        float baseX = catX + side * s * 0.08f;
        for (int i = -1; i <= 1; i++)
        {
            float angle = i * 0.15f + side * 0.1f;
            float wy = whiskerBaseY + i * s * 0.02f;
            float endX = baseX + side * whiskerLen;
            float endY = wy + i * s * 0.04f;
            drawList->AddLine(ImVec2(baseX, wy), ImVec2(endX, endY), whiskerColor, 1.3f);
        }
    }
    
    // === "Zzz" quand le chat dort (couché sans peek) ===
    if (lay > 0.5f && peek < 0.3f)
    {
        float zzzAlpha = (lay - 0.5f) * 2.0f * (1.0f - peek);
        float zzzOffset = sinf(m_animTime * 1.5f) * s * 0.03f;
        
        ImU32 zzzColor = IM_COL32(200, 200, 255, (int)(180 * zzzAlpha));
        
        float zX = catX + headRadius * 0.8f;
        float zY = headY - headRadius * 0.8f + zzzOffset;
        
        // Petits "z" qui montent
        for (int i = 0; i < 3; i++)
        {
            float scale = 0.6f + i * 0.2f;
            float alpha = zzzAlpha * (1.0f - i * 0.25f);
            ImU32 col = IM_COL32(200, 200, 255, (int)(150 * alpha));
            
            float zSize = s * 0.04f * scale;
            float zy = zY - i * s * 0.08f - fmodf(m_animTime * 0.3f, 0.5f) * s * 0.1f;
            float zx = zX + i * s * 0.03f;
            
            // Dessiner un "z"
            drawList->AddLine(ImVec2(zx - zSize, zy - zSize), ImVec2(zx + zSize, zy - zSize), col, 1.5f);
            drawList->AddLine(ImVec2(zx + zSize, zy - zSize), ImVec2(zx - zSize, zy + zSize), col, 1.5f);
            drawList->AddLine(ImVec2(zx - zSize, zy + zSize), ImVec2(zx + zSize, zy + zSize), col, 1.5f);
        }
    }
}
