#pragma once

#include "KeyboardManager.hpp"
#include "MouseManager.hpp"
#include "ControllerManager.hpp"
#include "Controller.hpp"
#include "DllControllerUtils.hpp"
//#include "Enum.hpp"

#include <vector>
#include <array>
/*
TODO: Delete?
ENUM ( TrialMenu, None,
       TrialSelect,
       Demo,
       RecordDemo,
       InputGuide,
       Interface,
       GuideOptions,
       Exit );
*/

class DllControllerManager
    : private KeyboardManager::Owner
    , private ControllerManager::Owner
    , private Controller::Owner
    , protected DllControllerUtils
{
public:

    // Local vs remote player numbers
    uint8_t localPlayer = 1, remotePlayer = 2;

    // Single player setting
    bool isSinglePlayer = false;

    // In trial mode
    bool isTrial = false;

    // Initialize all controllers with the given mappings
    void initControllers ( const ControllerMappings& mappings );

    // True only if both controllers are not mapping
    bool isNotMapping() const;

    // Update local controls and overlay UI inputs
    void updateControls ( uint16_t *localInputs );

    // KeyboardManager callback
    void keyboardEvent ( uint32_t vkCode, uint32_t scanCode, bool isExtended, bool isDown ) override;

    // ControllerManager callbacks
    void joystickAttached ( Controller *controller ) override;
    void joystickToBeDetached ( Controller *controller ) override;

    // Controller callback
    void controllerKeyMapped ( Controller *controller, uint32_t key ) override;

    // To be implemented
    virtual void saveMappings ( const Controller *controller ) const = 0;

    bool framestepEnabled;

private:

    std::vector<Controller *> _allControllers;

    #define USEDCONTROLLERCOUNT 4

    std::array<Controller *, USEDCONTROLLERCOUNT> _playerControllers = {{ 0, 0, 0, 0 }};

    std::array<size_t, USEDCONTROLLERCOUNT> _overlayPositions = {{ 0, 0, 0, 0}};
    std::array<size_t, 3> _trialOverlayPositions = {{ 0, 0, 0 }};
    uint8_t _trialMenuIndex = 0;
    uint8_t _trialMenuSelection = 0;
    uint8_t _trialSubMenuSelection = 0;
    uint8_t _trialScrollSelect = 0;

    std::array<bool, USEDCONTROLLERCOUNT> _finishedMapping = {{ false, false, false, false }};

    bool _controllerAttached = false;

    void handleInputEditor();
    void handleTrialMenuOverlay();
    void deleteTrialRow( int row );
    void insertTrialRow( int row );
    void handleMappingOverlay();
    void disableTrialMenuOverlay();
    void saveEdits();

};
