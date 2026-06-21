#pragma once

#include "Messages.hpp"
#include <string>
#include <deque>


class DllChatManager {
public:

    void addMessage(const ChatMessage& m);

    bool isTyping(); // disable inputs when this is true,,or at least disable keyboard inputs?

private:

    std::deque<std::string> messages;

    bool status = false;

};


