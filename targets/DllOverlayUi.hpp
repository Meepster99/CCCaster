#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <d3dx9.h>


#define DEFAULT_MESSAGE_TIMEOUT ( 3000 )


namespace DllOverlayUi
{

void init();


void enable();

void disable();

void toggle();

bool isEnabled();

bool isDisabled();

std::array<std::string, 5> getText();

int getHeight();

int getNewHeight();

std::array<RECT, 4> getSelector();

std::array<bool, 4> getShouldDrawSelector();

std::array<std::string, 4> getSelectorLine();

bool isTrial();

bool isMapping();

void setTrial();

void setMapping();

bool isToggling();

void updateText();

void updateText ( const std::array<std::string, 5>& text );

void updateSelector ( uint8_t index, int position = 0, const std::string& line = "" );


void showMessage ( const std::string& text, int timeout = DEFAULT_MESSAGE_TIMEOUT );

void updateMessage();

bool isShowingMessage();

#ifndef RELEASE

extern std::string debugText;

extern int debugTextAlign;

#endif

}
