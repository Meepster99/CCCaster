#pragma once

#include "Messages.hpp"
#include <string>
#include <deque>

#include "ControllerManager.hpp"

#define VK_ENBABLE_CHAT ( VK_F6 ) // could have this be something different? rebindable maybe? idrek, and idrec. this is a lie. i care deeply.



class KeyboardManagerHooker 
	: public KeyboardManager::Owner
	//, private Controller::Owner
    //, private ControllerManager::Owner
	//: private KeyboardManager::Owner
    //, private ControllerManager::Owner
    //, private Controller::Owner
    //, protected DllControllerUtils
	{
	// i really dont like this
	// i could just pass this if i had the chat manager be a class instead of a namespace? maybe this implies i should have it that way? i know nothing
	// i worry im going through the wrong layer of abstraction for this
	// i should probs just have dllchatmanager be a class, but idrek, and idrec
	// i take it back, dllchatmanager should DEF be a class. god. i dont like that tho?
public:

	//KeyboardManagerHooker() {}

	void keyboardEvent ( uint32_t vkCode, uint32_t scanCode, bool isExtended, bool isDown ) override;

};

namespace DllChatManager {

    void addMessage(const ChatMessage& m);
	
	void startTyping();

    //bool isTyping(); // disable inputs when this is true,,or at least disable keyboard inputs?

	void typeMessage();

	extern uint64_t typeStartTime; 
	extern bool isTyping;
	extern bool shouldStop;
	extern std::string typingMessage;
	extern uint64_t lastCheckTime;

    extern std::deque<std::string> messages;

	extern KeyboardManagerHooker keyboardManager;

    

};


