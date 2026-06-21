#include "DllChatManager.hpp"
#include "DllOverlayUi.hpp"
#include "NetLogger.hpp"

void DllChatManager::addMessage(const ChatMessage& m) {
    log("got a message omfg");
    DllOverlayUi::showMessage("bruh");

}

bool DllChatManager::isTyping() {
    return status;
}
