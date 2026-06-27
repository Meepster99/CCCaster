#include "DllChatManager.hpp"
#include "DllOverlayUi.hpp"
#include "NetLogger.hpp"
#include "KeyboardState.hpp"


void KeyboardManagerHooker::keyboardEvent ( uint32_t vkCode, uint32_t scanCode, bool isExtended, bool isDown ) {

	// how the hell does this thing even access controllermanager?
	Lock lock ( ControllerManager::get().mutex );
	

	// i should probs have a mutex for accessing the typingmessage struing
	log("event got %02X", vkCode);

	if((vkCode >= '0' && vkCode <= 'Z') || vkCode == VK_SPACE) {
		DllChatManager::typingMessage += (char)vkCode;
	} else if(vkCode == VK_BACK) {
		if(DllChatManager::typingMessage.size() >= 1) {
			DllChatManager::typingMessage.pop_back();
		}
	} else if(vkCode == VK_ENBABLE_CHAT) {
		DllChatManager::shouldStop = true;
	}
	


}


namespace DllChatManager {

uint64_t typeStartTime = 0;
bool isTyping = false;

std::string typingMessage = "";
uint64_t lastCheckTime = 0;

std::deque<std::string> messages;

KeyboardManagerHooker keyboardManager;

bool shouldStop = false; // this is ass code. god. please maddy actually fix this 

void startTyping() {

	if(isTyping) {
		return;
	}

	typingMessage = "";
	typeStartTime = lastCheckTime = TimerManager::get().getNow ( true );
	isTyping = true;
	shouldStop = false;


	//KeyboardManager::get().keyboardWindow = DllHacks::windowHandle;
	KeyboardManager::get().matchedKeys.clear();
	KeyboardManager::get().ignoredKeys.clear();
	KeyboardManager::get().hook ( &keyboardManager, true );
}

void addMessage(const ChatMessage& m) {
    log("got a message: %s", m.msg.c_str());
    DllOverlayUi::showChatMessage(m);
}

void typeMessage() {




	// caster already has some keyboard manager thing. i should use that. 
	// issue. multiple keypresses in the space of a frame should all record, would suck if they didnt
	// keyboardstate.cpp seems to be what i want
	// god, in the pursuit of perfection, i end up with a bunch of comments and nothing else

	//KeyboardState::update();
	typingMessage += KeyboardState::getRecentPresses(lastCheckTime);

	//log("current msg: %s", typingMessage.c_str());

	lastCheckTime = TimerManager::get().getNow ( true );

	if(shouldStop) {
		if(lastCheckTime - typeStartTime > 300) {
			isTyping = false;
			ChatMessage newMsg;
			newMsg.msg = typingMessage;
			addMessage(newMsg);
			//log("got told to stop and send the message");
			KeyboardManager::get().unhook();
		} else {
			shouldStop = false;
		}
	}

	


}




};
