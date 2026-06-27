#include "DllChatManager.hpp"
#include "DllOverlayUi.hpp"
#include "NetLogger.hpp"
#include "KeyboardState.hpp"
#include "DllNetplayManager.hpp"

void DllChatManager::keyboardEvent ( uint32_t vkCode, uint32_t scanCode, bool isExtended, bool isDown ) {

	// how the hell does this thing even access controllermanager?
	Lock lock ( ControllerManager::get().mutex );
	

	// i should probs have a mutex for accessing the typingmessage struing
	//log("event got %02X", vkCode);

	if((vkCode >= '0' && vkCode <= 'Z') || vkCode == VK_SPACE) {
		typingMessage += (char)vkCode;
	} else if(vkCode == VK_BACK) {
		if(typingMessage.size() >= 1) {
			typingMessage.pop_back();
		}
	} else if(vkCode == VK_ENBABLE_CHAT) {
		shouldStop = true;
	}
}

void DllChatManager::frameStep(SocketPtr& dataSocket) {

	while(!recvMsg.empty()) { // display recieved messages.
		ChatMessage m = recvMsg.front();
		recvMsg.pop_front();
		DllOverlayUi::showChatMessage(m);
	}

	while(!sendMsg.empty()) { 
		ChatMessage m = sendMsg.front();
		sendMsg.pop_front();
		// log("sending message: %s", m.msg.c_str());
		m.player = netManPtr->_localPlayer;
		if(dataSocket && dataSocket->isConnected()) {
			dataSocket->send(m);
		} 
		DllOverlayUi::showChatMessage(m);
	}

}

void DllChatManager::startTyping() {

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
	KeyboardManager::get().hook ( this, true );
}

void DllChatManager::displayMessage(const ChatMessage& m) {
    log("got a message: %s", m.msg.c_str());
    DllOverlayUi::showChatMessage(m);
}

void DllChatManager::recvMessage(const ChatMessage& m) {
	recvMsg.push_back(m);
}

void DllChatManager::sendMessage(const ChatMessage& m) {
	sendMsg.push_back(m);
}

void DllChatManager::typeMessage() {

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
			sendMessage(newMsg);
			//log("got told to stop and send the message");
			KeyboardManager::get().unhook();
		} else {
			shouldStop = false;
		}
	}


}

