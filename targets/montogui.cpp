#include "DllOverlayUi.hpp"
#include "DllOverlayPrimitives.hpp"
#include "DllHacks.hpp"
#include "DllTrialManager.hpp"
#include "ProcessManager.hpp"
#include "Enum.hpp"

#include "DllDirectX.hpp"
#include "PlayerState.hpp"

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

#include <vector>
#include <sstream>

struct HealthBarFlash {
    float prevHealth = 11400.0f;
    float flashStartTime = 0.0f;
    float hitstunEndTime = 0.0f;
    bool isFlashing = false;
    bool hasStoredHitstunEnd = false;
    float flashX = 0.0f;
    float flashStartWidth = 0.0f;
    float comboStartHealth = 0.0f;
    float displayedHealth = 11400.0f;
};

static HealthBarFlash healthFlashes[4];
static float g_healthAnimationSpeed = 18000.0f;

void drawHealthBar(ImDrawList* draw_list, ImVec2 pos, int playerIndex, bool isRightSide, float width, float height, float padding)
{
    const float maxHealth = 11400.0f;
    HealthBarFlash& flash = healthFlashes[playerIndex];
    float currentHealth = players[playerIndex].health;
    
    float targetHealth = currentHealth;
    float currentTime = GetTickCount() / 1000.0f;
    
    if (flash.displayedHealth > targetHealth) {
        float delta = (currentTime - flash.flashStartTime) * g_healthAnimationSpeed;
        flash.displayedHealth = std::max(targetHealth, flash.displayedHealth - delta);
    } else {
        flash.displayedHealth = targetHealth;
    }

    float xPos = isRightSide ? 
        pos.x + ImGui::GetWindowWidth() - width - padding : 
        pos.x + padding;
    float yPos = pos.y + padding;
    
    // White outer border (3px) - expanded by 1px
    draw_list->AddRect(
        ImVec2(xPos - 1, yPos - 1),
        ImVec2(xPos + width + 1, yPos + height + 1),
        IM_COL32(255, 255, 255, 255),
        1.0f,  // rounding
        0,     // flags
        3.0f   // thickness
    );

 

    // Background (50% opacity black)
    draw_list->AddRectFilled(
        ImVec2(xPos, yPos),
        ImVec2(xPos + width, yPos + height),
        IM_COL32(0, 0, 0, 128)
    );

    // Red health (gradient from dark red to red)
    float redHealthWidth = (players[playerIndex].redHealth / maxHealth) * width;
    ImVec2 redStart, redEnd;
    if (isRightSide) {  // P2/P4 side - drain left to right
        redStart = ImVec2(xPos, yPos);
        redEnd = ImVec2(xPos + redHealthWidth, yPos + height);
        draw_list->AddRectFilledMultiColor(
            redStart, redEnd,
            IM_COL32(101, 0, 0, 255),    // Dark red
            IM_COL32(218, 0, 0, 255),    // Red
            IM_COL32(218, 0, 0, 255),    // Red
            IM_COL32(101, 0, 0, 255)     // Dark red
        );
    } else {  // P1/P3 side - drain right to left
        redStart = ImVec2(xPos + width - redHealthWidth, yPos);
        redEnd = ImVec2(xPos + width, yPos + height);
        draw_list->AddRectFilledMultiColor(
            redStart, redEnd,
            IM_COL32(218, 0, 0, 255),    // Red
            IM_COL32(101, 0, 0, 255),    // Dark red
            IM_COL32(101, 0, 0, 255),    // Dark red
            IM_COL32(218, 0, 0, 255)     // Red
        );
    }

    // Current health (gradient from orange to yellow)
    float healthWidth = (flash.displayedHealth / maxHealth) * width;
    ImVec2 healthStart, healthEnd;
    if (isRightSide) {  // P2/P4 side - drain left to right
        healthStart = ImVec2(xPos, yPos);
        healthEnd = ImVec2(xPos + healthWidth, yPos + height);
        draw_list->AddRectFilledMultiColor(
            healthStart, healthEnd,
            IM_COL32(255, 136, 5, 255),   // Orange
            IM_COL32(255, 251, 125, 255), // Yellow
            IM_COL32(255, 251, 125, 255), // Yellow
            IM_COL32(255, 136, 5, 255)    // Orange
        );
    } else {  // P1/P3 side - drain right to left
        healthStart = ImVec2(xPos + width - healthWidth, yPos);
        healthEnd = ImVec2(xPos + width, yPos + height);
        draw_list->AddRectFilledMultiColor(
            healthStart, healthEnd,
            IM_COL32(255, 251, 125, 255), // Yellow
            IM_COL32(255, 136, 5, 255),   // Orange
            IM_COL32(255, 136, 5, 255),   // Orange
            IM_COL32(255, 251, 125, 255)  // Yellow
        );
    }

    // Handle flashing effect
    if (currentHealth < flash.prevHealth) {
        flash.flashStartTime = GetTickCount() / 1000.0f;
        
        if (!flash.isFlashing || players[playerIndex].hitstun == 0) {
            float initialHealthWidth = (flash.prevHealth / 11400.0f) * width;
            float flashStartWidth = initialHealthWidth - (currentHealth / 11400.0f * width);
            
            // Adjust flash position based on side
            float flashX;
            if (isRightSide) {
                // P2/P4 side - mirror of P1/P3
                flashX = xPos + initialHealthWidth;
            } else {
                // P1/P3 side - unchanged
                flashX = xPos + width - initialHealthWidth;
            }
            
            flash.isFlashing = true;
            flash.flashX = flashX;
            flash.flashStartWidth = flashStartWidth;
            flash.comboStartHealth = flash.prevHealth;
        } else {
            // Extend flash width during combo
            float currentWidth = (currentHealth / 11400.0f) * width;
            float comboStartWidth = (flash.comboStartHealth / 11400.0f) * width;
            flash.flashStartWidth = comboStartWidth - currentWidth;
        }
    }

    if (flash.isFlashing) {
        if (players[playerIndex].hitstun > 0) {
            flash.hasStoredHitstunEnd = false;
            float timeInHitstun = (GetTickCount() / 1000.0f) - flash.flashStartTime;
            
            ImU32 flashColor;
            if (timeInHitstun < 0.3f) {
                float transitionProgress = timeInHitstun / 0.3f;
                BYTE alpha = (BYTE)(0xBB - (transitionProgress * (0xBB - 0x40)));
                flashColor = IM_COL32(255, 255, 255, alpha);
            } else {
                flashColor = IM_COL32(255, 255, 255, 0x40);
            }
            
            if (isRightSide) {
                // P2/P4 side - mirror of P1/P3
                draw_list->AddRectFilled(
                    ImVec2(flash.flashX - flash.flashStartWidth, yPos),
                    ImVec2(flash.flashX, yPos + height),
                    flashColor
                );
            } else {
                // P1/P3 side - unchanged
                draw_list->AddRectFilled(
                    ImVec2(flash.flashX, yPos),
                    ImVec2(flash.flashX + flash.flashStartWidth, yPos + height),
                    flashColor
                );
            }
        } else {
            if (!flash.hasStoredHitstunEnd) {
                flash.hitstunEndTime = GetTickCount() / 1000.0f;
                flash.hasStoredHitstunEnd = true;
            }
            
            float timeSinceHitstunEnd = (GetTickCount() / 1000.0f) - flash.hitstunEndTime;
            if (timeSinceHitstunEnd < 0.25f) {
                float animationProgress = timeSinceHitstunEnd * 4.0f;
                float currentWidth = flash.flashStartWidth * (1.0f - animationProgress);
                
                if (isRightSide) {
                    // P2/P4 side - mirror of P1/P3
                    float targetX = xPos + (players[playerIndex].health / 11400.0f * width);
                    float startX = flash.flashX;
                    float currentX = startX - (startX - targetX) * animationProgress;
                    
                    draw_list->AddRectFilled(
                        ImVec2(currentX - currentWidth, yPos),
                        ImVec2(currentX, yPos + height),
                        IM_COL32(255, 255, 255, 0x40)
                    );
                } else {
                    // P1/P3 side - unchanged
                    float targetX = xPos + width - (players[playerIndex].health / 11400.0f * width);
                    float startX = flash.flashX;
                    float currentX = startX + (targetX - startX) * animationProgress;
                    
                    draw_list->AddRectFilled(
                        ImVec2(currentX, yPos),
                        ImVec2(currentX + currentWidth, yPos + height),
                        IM_COL32(255, 255, 255, 0x40)
                    );
                }
            } else {
                flash.isFlashing = false;
            }
        }
    }
    
    flash.prevHealth = currentHealth;

       // Black inner border (1px)
    draw_list->AddRect(
        ImVec2(xPos, yPos),
        ImVec2(xPos + width, yPos + height),
        IM_COL32(0, 0, 0, 255)
    );

}

void uiHP()
{
    static bool open = true;
    ImGui::GetIO().MouseDrawCursor = true;
    
    // Set window background to fully transparent
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    
    if (ImGui::Begin("Meters", &open, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::SetWindowSize(ImVec2(640, 200));
        ImGui::SetWindowPos(ImVec2(0, 280));
        
        const float healthWidth = 213.0f;
        const float healthHeight = 12.0f;
        const float padding = 60.0f;
        const float verticalSpacing = 6.0f;  // Space between P1->P3 and P2->P4
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();

        // P1 (top left)
        drawHealthBar(draw_list, pos, 0, false, healthWidth, healthHeight, padding);
        
        // P2 (top right)
        drawHealthBar(draw_list, pos, 1, true, healthWidth, healthHeight, padding);
        
        // P3 (bottom left)
        ImVec2 bottomPos = ImVec2(pos.x, pos.y + healthHeight + verticalSpacing);
        drawHealthBar(draw_list, bottomPos, 2, false, healthWidth, healthHeight, padding);
        
        // P4 (bottom right)
        drawHealthBar(draw_list, bottomPos, 3, true, healthWidth, healthHeight, padding);
    }
    ImGui::End();
    
    ImGui::PopStyleColor();

}

void uiMeter()
{
    static bool open = true;
    static float scale = 3.0f;
    static float startX = 50.0f;
    static float startY = 50.0f;
    static float fillAmount = 0.0f;
    static ImVec4 bgColor = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
    static ImVec4 fillColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    static ImVec4 borderColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    static float borderThickness = 2.0f;
    
    // Point editing array (normalized to scale=1.0)
    static float points[][2] = {
        {0.0f, 0.0f},              // p0
        {23.12f, 0.0f},            // p1
        {25.83f, -2.71f},          // p2

    };
    
    // Debug window
    ImGui::Begin("Meter Debug");
    ImGui::SliderFloat("Scale", &scale, 1.0f, 10.0f);
    ImGui::SliderFloat("X Position", &startX, 0.0f, 200.0f);
    ImGui::SliderFloat("Y Position", &startY, 0.0f, 200.0f);
    ImGui::SliderFloat("Fill Amount", &fillAmount, 0.0f, 1.0f);
    ImGui::ColorEdit4("Background Color", (float*)&bgColor);
    ImGui::ColorEdit4("Fill Color", (float*)&fillColor);
    ImGui::ColorEdit4("Border Color", (float*)&borderColor);
    ImGui::SliderFloat("Border Thickness", &borderThickness, 0.0f, 5.0f);
    
    // Point editing
    if (ImGui::TreeNode("Point Editor")) {
        for (int i = 0; i < IM_ARRAYSIZE(points); i++) {
            ImGui::PushID(i);
            ImGui::Text("Point %d", i);
            ImGui::SameLine();
            ImGui::DragFloat2("##point", points[i], 0.1f);
            ImGui::PopID();
        }
        
        // Export button
        if (ImGui::Button("Export Points")) {
            std::stringstream output;
            output << "static float points[][2] = {\n";
            for (int i = 0; i < IM_ARRAYSIZE(points); i++) {
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "    {%.2ff, %.2ff}", points[i][0], points[i][1]);
                output << buffer;
                if (i < IM_ARRAYSIZE(points) - 1) output << ",";
                output << " // p" << i << "\n";
            }
            output << "};";
            ImGui::SetClipboardText(output.str().c_str());
        }
        ImGui::TreePop();
    }
    
    static bool autoAnimate = true;
    static float animationSpeed = 1.0f;
    ImGui::Checkbox("Auto Animate", &autoAnimate);
    ImGui::SliderFloat("Animation Speed", &animationSpeed, 0.1f, 3.0f);
    ImGui::End();

    if (autoAnimate) {
        fillAmount = (sinf(ImGui::GetTime() * animationSpeed) + 1.0f) * 0.5f;
    }
    
    if (ImGui::Begin("Meters", &open, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus))
    {
        ImGui::SetWindowSize(ImVec2(640, 200));
        ImGui::SetWindowPos(ImVec2(0, 280));
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();

        float x = pos.x + startX;
        float y = pos.y + startY;
        float totalWidth = points[5][0] * scale;

        // Convert normalized points to screen space
        ImVec2 screenPoints[IM_ARRAYSIZE(points)];
        float minY = y + points[0][1] * scale;
        float maxY = minY;
        
        for (int i = 0; i < IM_ARRAYSIZE(points); i++) {
            screenPoints[i] = ImVec2(
                x + points[i][0] * scale,
                y + points[i][1] * scale
            );
            minY = std::min(minY, screenPoints[i].y);
            maxY = std::max(maxY, screenPoints[i].y);
        }

        // Use stencil buffer approach
        draw_list->PushClipRect(ImVec2(x - 1, minY - 1), ImVec2(x + totalWidth + 1, maxY + 1), true);
        
        // Create stencil mask using the shape
        draw_list->AddConvexPolyFilled(screenPoints, IM_ARRAYSIZE(screenPoints), IM_COL32(255, 255, 255, 255));
        
        // Draw the fill rectangle, it will be masked by the shape
        float fillWidth = totalWidth * fillAmount;
        draw_list->AddRectFilled(
            ImVec2(x, minY - 1),
            ImVec2(x + fillWidth, maxY + 1),
            ImGui::ColorConvertFloat4ToU32(fillColor)
        );
        
        draw_list->PopClipRect();

        // Draw background (after clearing stencil)
        draw_list->AddConvexPolyFilled(screenPoints, IM_ARRAYSIZE(screenPoints), ImGui::ColorConvertFloat4ToU32(bgColor));

        // Draw outline
        for (int i = 0; i < IM_ARRAYSIZE(screenPoints); i++) {
            draw_list->PathLineTo(screenPoints[i]);
        }
        draw_list->PathLineTo(screenPoints[0]);
        draw_list->PathStroke(ImGui::ColorConvertFloat4ToU32(borderColor), ImDrawFlags_Closed, borderThickness);
    }
    ImGui::End();
}