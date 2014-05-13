Super Ogre Ball
===========================================================
    
    Kevin Yeh, Alyssa Sallean, Patrick Grayson, Matt Navarifar

A loving recreation of Super Monkey Ball, built in five weeks for UT Austin's Game Technology course.

##Running
Consult README.md for instructions on building, then run the ./OgreBall executable. If you are building under Ogre 1.8, checkout the Ogre-1.8 branch instead of Master.

##Controls
Keyboard:

    WASD - Level Tilt
    ESC - Menu
    
Gamecube USB:

    Left Stick - Level Tilt

Single Player:

    R - Restart (Requires at least one life)

Multi Player:
  
    C - Toggle Chatlog Visibility
    Enter - Toggle Message
    ESC - Menu / Exit Message Context

##Final Implementation Overview
Swanky New Game Engine Features:

* Ogre Procedural Integration (Procedurally Generated Meshes)
* Level and Mesh-building Scripting Language and Parser (stored in media/OgreBall/scripts/)
* Single Player Leaderboards (persisted in data/*.obhs)
* 4-Player Network Support with User Tags and Chatlog
* Swanky Menus and Gamestate Screens + CEGUI Animations
* Interpolated Objects for Dynamic Levels
* USB GAMECUBE CONTROLLER SUPPORT FOR SINGLE PLAYER WHAT UP
* Smart camera
* More cool stuff we probably forgot about

##FILES

    common.*
    
    BaseApplication.*, OgreBallApplication.* (Main Application)
    
    Activity.*, MenuActivity.*, SinglePlayerActivity.*, BaseMultiActivity.*, 
    HostPlayerActivity.*, ClientPlayerActivity.* (Sub-Application States)

    CameraObject.* (Smart Camera Positioning)

    LevelLoader.* (Level and Mesh-building Script Parser)

    LevelViewer.* (Ogre-Camera Viewers in CEGUI Windows)

    Interpolator.*, OBAnimationManager.* (Dynamic Helpers)

    SelectorHelper.* (Menu Select Helpers)

    GameObject.*, GameObjectDescriptions.h, Collectible.cpp, GoalObject.cpp, 

    OgreBall.cpp, DecorativeObject.cpp, MeshObject.cpp (Game Object Definitions)

    Physics.*, OgreMotionState.* Sounds.*, Networking.* (Library Integration Helpers)
