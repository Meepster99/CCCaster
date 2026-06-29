#include "DllChatManager.hpp"
#include "DllOverlayUi.hpp"
#include "NetLogger.hpp"
#include "KeyboardState.hpp"
#include "DllNetplayManager.hpp"
#include "DllOverlayPrimitives.hpp"

/*

issues:
need to see while typing
setting keys just fucks it.
switch to my own text renderer so i dont have to deal with this bs

*/



void DllChatManager::keyboardEvent ( uint32_t vkCode, uint32_t scanCode, bool isExtended, bool isDown ) {

	// how the hell does this thing even access controllermanager?
	Lock lock ( ControllerManager::get().mutex );
	
	if((vkCode >= '0' && vkCode <= 'Z') || vkCode == VK_SPACE) {
		typingMessage += (char)vkCode;
	} else if(vkCode == VK_BACK) {
		if(typingMessage.size() >= 1) {
			typingMessage.pop_back();
		} else {
			shouldStop = true; // if someone backspaces when theres nothing left, just dont send shit.
		}
	} else if(vkCode == VK_ENBABLE_CHAT) {
		shouldStop = true;
	}
}

void DllChatManager::frameStep(SocketPtr& dataSocket) {

	while(!recvMsg.empty()) { // display recieved messages.
		ChatMessage m = recvMsg.front();
		recvMsg.pop_front();
		m.framesToDisplay = framesToDisplayMsg;
		displayQueue.push_front(m);
	}

	while(!sendMsg.empty()) { 
		ChatMessage m = sendMsg.front();
		sendMsg.pop_front();
		m.player = netManPtr->_localPlayer;
		if(dataSocket && dataSocket->isConnected()) {
			dataSocket->send(m);
		} 
		m.framesToDisplay = framesToDisplayMsg;
		displayQueue.push_front(m);	
	}

}

void DllChatManager::displayChat() {

	int drawIndex = 1 + isTyping; // index 0 is below the screen, 1 is left for the typing dialogue. move the messages down if not typing.

	for(auto it = displayQueue.begin(); it != displayQueue.end(); ) {
		if(it->framesToDisplay < 0) {
			it = displayQueue.erase(it);
			continue;
		}
		DllOverlayUi::showChatMessage(drawIndex, *it);
		it->framesToDisplay--;
		++it;
		drawIndex++;
	}

	if(isTyping) {
		DllOverlayUi::showTypingDialogue(typingMessage);
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

	KeyboardManager::get().matchedKeys.clear();
	KeyboardManager::get().ignoredKeys.clear();
	KeyboardManager::get().hook ( this, true );
}

void DllChatManager::recvMessage(const ChatMessage& m) {
	recvMsg.push_back(m);
}

void DllChatManager::sendMessage(const ChatMessage& m) {
	if(m.msg.size() >= 1) {
		sendMsg.push_back(m);
	}
}

void DllChatManager::typeMessage() {

	// caster already has some keyboard manager thing. i should use that. 
	// issue. multiple keypresses in the space of a frame should all record, would suck if they didnt
	// keyboardstate.cpp seems to be what i want
	// god, in the pursuit of perfection, i end up with a bunch of comments and nothing else

	typingMessage += KeyboardState::getRecentPresses(lastCheckTime);
	lastCheckTime = TimerManager::get().getNow ( true );

	if(shouldStop) {
		if(lastCheckTime - typeStartTime > 300) {
			isTyping = false;
			ChatMessage newMsg;
			newMsg.msg = typingMessage;
			sendMessage(newMsg);
			KeyboardManager::get().unhook();
		} else {
			shouldStop = false;
		}
	}
}
