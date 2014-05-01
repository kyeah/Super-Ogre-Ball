#include "Interpolator.h"
#include "MenuActivity.h"
#include "HostPlayerActivity.h"
#include "OBAnimationManager.h"
#include "Networking.h"
#include "Sounds.h"
#include "common.h"


HostPlayerActivity::HostPlayerActivity(OgreBallApplication *app, const char* levelName) : Activity(app) {
  MAX_TILT = .10;  //Increasing this increases the maximum degree to which the level can rotate
  tiltDelay = 300;  // Increasing this increases the time it takes for the level to rotate

  currentLevelName = std::string(levelName, std::strlen(levelName));
  menuActive = false;
  ceguiActive = false;

  Networking::serverConnect();
  inGame = false;
  waitingForClientsToLoad = false;
  chatFocus = false;
  myId = 0;

  for (int i = 0; i < MAX_PLAYERS; i++)
    players[i] = 0;
}

HostPlayerActivity::~HostPlayerActivity(void) {
  close();
}

void HostPlayerActivity::close(void) {
  ServerPacket msg;
  msg.type = SERVER_CLOSED;
  for(int i = 1; i < MAX_PLAYERS; i++) {
    if(players[i]) {
      Networking::Send(players[i]->csd, (char*)&msg, sizeof(msg));
    }
  }

  Networking::Close();
  //  delete player;
  //  delete mCameraObj;
}

Player* HostPlayerActivity::addPlayer(int userID, const char* name) {
  Player* mPlayer = new Player(userID);

  strcpy(mPlayer->name, name);
  mPlayer->character = 0;
  mPlayer->currTiltDelay = tiltDelay;
  mPlayer->lastTilt = btQuaternion(0,0,0);
  mPlayer->currTilt = btQuaternion(0,0,0);
  mPlayer->tiltDest = btQuaternion(0,0,0);

  lobbyPlayerWindows[userID]->setText(mPlayer->name);
  players[userID] = mPlayer;
  return mPlayer;
}

/*
  void HostPlayerActivity::removePlayer(int userID) {
  players[userID] = NULL;
  // send message to all other players
  }
*/

void HostPlayerActivity::start(void) {
  //  Sounds::playBackground("media/OgreBall/sounds/StandardLevel.mp3", Sounds::volume);

  guiSheet = app->Wmgr->getWindow("SinglePlayerHUD");
  scoreDisplay = app->Wmgr->getWindow("SinglePlayerHUD/Score");
  livesDisplay = app->Wmgr->getWindow("SinglePlayerHUD/Lives");
  collectDisplay = app->Wmgr->getWindow("SinglePlayerHUD/Collectibles");
  timeDisplay = app->Wmgr->getWindow("SinglePlayerHUD/Timer");
  levelDisplay = app->Wmgr->getWindow("SinglePlayerHUD/Level");

  readyWindow = app->Wmgr->getWindow("SPHUD/Ready");
  goWindow = app->Wmgr->getWindow("SPHUD/Go");

  pauseMenuSheet = app->Wmgr->getWindow("PauseMenu");
  pauseQuit = app->Wmgr->getWindow("PauseMenu/Quit");
  pauseReturn = app->Wmgr->getWindow("PauseMenu/Return");

  pauseQuit->removeEvent(CEGUI::PushButton::EventClicked);
  pauseQuit
    ->subscribeEvent(CEGUI::PushButton::EventClicked,
                     CEGUI::Event::Subscriber(&HostPlayerActivity::ExitToMenu, this));

  pauseReturn->removeEvent(CEGUI::PushButton::EventClicked);
  pauseReturn
    ->subscribeEvent(CEGUI::PushButton::EventClicked,
                     CEGUI::Event::Subscriber(&HostPlayerActivity::togglePauseMenu, this));

  lobbySheet = app->Wmgr->getWindow("GameLobby");
  lobbySelectLevel = app->Wmgr->getWindow("GameLobby/SelectLevel");
  lobbySelectCharacter = app->Wmgr->getWindow("GameLobby/SelectCharacter");
  lobbyLeave = app->Wmgr->getWindow("GameLobby/Leave");
  lobbyStart = app->Wmgr->getWindow("GameLobby/Ready");
  lobbyStart->setText("Start");

  lobbyStart->removeEvent(CEGUI::PushButton::EventClicked);
  lobbyStart->subscribeEvent(CEGUI::PushButton::EventClicked,
                             CEGUI::Event::Subscriber(&HostPlayerActivity::startGame, this));

  lobbyLeave->subscribeEvent(CEGUI::PushButton::EventClicked,
                             CEGUI::Event::Subscriber(&HostPlayerActivity::ExitToMenu, this));

  for (int i = 0; i < 4; i++) {
    std::stringstream ss;
    ss << "GameLobby/" << i+1;
    lobbyPlayerWindows[i] = app->Wmgr->getWindow(ss.str());
  }

  // TODO: Add Chatbox Window to layout
  CEGUI::MouseCursor::getSingleton().show();
  CEGUI::System::getSingleton().setGUISheet(lobbySheet);
  addPlayer(0, "host-username");
  //  loadLevel(currentLevelName);
}

void HostPlayerActivity::handleLobbyState(void) {

  // Handle New Connections
  TCPsocket csd_t = SDLNet_TCP_Accept(Networking::server_socket);
  if(csd_t){

    remoteIP = SDLNet_TCP_GetPeerAddress(csd_t);
    if(remoteIP){
      printf("Successfully connected to %x %d\n", SDLNet_Read32(&remoteIP->host), SDLNet_Read16(&remoteIP->port));
    }

    PingMessage ping;

    if(SDLNet_TCP_Recv(csd_t, &ping, sizeof(ping)) > 0) {

      if (!ping.isJoining) {
        PingResponseMessage response;
        strcpy(response.lobbyName, "TEMPORARY SWAGGY P");

        response.numPlayers = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) {
          if (players[i]) response.numPlayers++;
        }

        response.maxPlayers = MAX_PLAYERS;
        Networking::Send(csd_t, (char*)&response, sizeof(response));
        SDLNet_TCP_Close(csd_t);

      } else {

        // Add a new player to the lobby
        if (SDLNet_TCP_AddSocket(Networking::server_socketset, csd_t) == -1) {
          printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());

        } else {
          for (int i = 0; i < MAX_PLAYERS; i++) {
            if (!players[i]) {
              addPlayer(i, ping.name);
              players[i]->csd = csd_t;

              // Send ack with its new user ID
              ConnectAck ack;
              ack.id = i;
              strcpy(ack.level, currentLevelName.c_str());
              strcpy(ack.lobbyName, "TEMPORARY SWAGGY P");

              for (int j = 0; j < MAX_PLAYERS; j++) {
                if (players[j]) {
                  ack.ids[j] = 1;
                  strcpy(ack.playerInfo[j].name, players[j]->name);
                  // ack.playerInfo[j].characterChoice = WHATEVER CHOICE;
                } else {
                  ack.ids[j] = 0;
                }
              }

              Networking::Send(csd_t, (char*)&ack, sizeof(ack));

              // Send notifications to rest of players
              ServerPacket packet;
              packet.type = SERVER_CLIENT_CONNECT;
              //strcpy(packet.level, currentLevelName.c_str());
              packet.clientID = i;
              for (int j = 1; j < MAX_PLAYERS; j++) {
                if (i != j && players[j])
                  Networking::Send(players[j]->csd, (char*)&packet, sizeof(packet));
              }
              break;
            }
          }
        }
      }
    }
  }
}

void HostPlayerActivity::handleWaiting() {
  // Wait for clients to send READY messages
}

bool HostPlayerActivity::startGame( const CEGUI::EventArgs& e ) {
  ServerPacket packet;
  packet.type = SERVER_GAME_START;

  for (int j = 1; j < MAX_PLAYERS; j++) {
    if (players[j]) {
      Networking::Send(players[j]->csd, (char*)&packet, sizeof(packet));
    }
  }

  inGame = true;
  waitingForClientsToLoad = true;
  loadLevel(currentLevelName.c_str());
  CEGUI::System::getSingleton().setGUISheet(guiSheet);
  CEGUI::MouseCursor::getSingleton().hide();
}

void HostPlayerActivity::loadLevel(const char* name) {
  currentLevelName = std::string(name, std::strlen(name));
  app->destroyAllEntitiesAndNodes();
  app->levelLoader->currObjID = 0;

  app->levelLoader->loadLevel(name);

  levelDisplay->setText(currentLevelName.c_str());

  timeLeft = 60000;  // TODO: Should get timeLeft from level script
  collectibles = 0;
  score = 0;

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i]) {
      std::stringstream ss;
      ss << "Player" << i;
      std::string playerEnt = ss.str();
      ss << "node";

      players[i]->setBall(new OgreBall(app->mSceneMgr, ss.str(), ss.str(), "penguin.mesh",  0,
                                       app->mPhysics,
                                       app->levelLoader->playerStartPositions[0], btVector3(1,1,1),
                                       btVector3(0,0,0), 16000.0f, 0.5f, btVector3(0,0,0),
                                       &app->levelLoader->playerStartRotations[0]));

      if (i == 0) {
        players[i]->mCameraLookAtNode = app->mCameraLookAtNode;
        players[i]->mCameraNode = app->mCameraNode;
      } else {
        ss << "lookAt";
        players[i]->mCameraLookAtNode = app->mSceneMgr->getRootSceneNode()->createChildSceneNode(ss.str());

        ss << "cam";
        players[i]->mCameraNode = players[i]->mCameraLookAtNode->createChildSceneNode(ss.str());
        players[i]->mCameraNode->setFixedYawAxis(true);
      }

      players[i]->mCameraObj = new CameraObject(players[i]->mCameraLookAtNode, players[i]->mCameraNode,
                                                (Ogre::Vector3)players[i]->getBall()->getPosition(),
                                                app->levelLoader->cameraStartPos);
    }
  }

  app->mCamera->setPosition(Ogre::Vector3(0,0,0));

  gameEnded = false;
  countdown = 2000;
  readyWindow->setAlpha(0.0);
  goWindow->setAlpha(0.0);

  menuActive = false;
  ceguiActive = false;
}

bool HostPlayerActivity::frameRenderingQueued( const Ogre::FrameEvent& evt ) {
  CEGUI::System::getSingleton().injectTimePulse(evt.timeSinceLastFrame);
  return true;
}

bool HostPlayerActivity::frameStarted( Ogre::Real elapsedTime ) {
  if (!inGame) {
    handleLobbyState();
    return true;
  } else if (waitingForClientsToLoad) {
    handleWaiting();
    //    return true;  // Comment out for now
  }

  if (countdown != -1 && !menuActive && !ceguiActive) {
    int lastcountdown = countdown;
    countdown = std::max((int)(countdown - elapsedTime), -1);
    if (lastcountdown > 1750 && countdown <= 1750) {
      OBAnimationManager::startAnimation("SpinPopin", readyWindow, 0.8);
    } else if (lastcountdown > 0 && countdown <= 0) {
      OBAnimationManager::startAnimation("Popin", goWindow);
    }
  }

  timeLeft = std::max(timeLeft - elapsedTime, 0.0f);

  /*
    if (!gameEnded  && countdown == -1) {
    timeLeft = std::max(timeLeft - elapsedTime, 0.0f);

    if (timeLeft == 0.0f) {
    handleGameOver();
    } else if (app->levelLoader->fallCutoff > player->getPosition()[1]) {
    lives--;
    if (lives < 0) {
    handleGameOver();
    } else {
    OBAnimationManager::startAnimation("SpinPopup", livesDisplay);
    loadLevel(currentLevelName.c_str());
    return true;
    }
    }
    }
  */

  // Update HUD
  std::stringstream sst;
  sst << "SCORE: " << score;
  scoreDisplay->setText(sst.str());

  /*
    std:: stringstream livesSS;
    livesSS << lives << (lives != 1 ? " Lives" : " Life");
    livesDisplay->setText(livesSS.str());
  */

  std::stringstream css;
  css << collectibles << "/" << app->levelLoader->numCollectibles;
  collectDisplay->setText(css.str());

  std::stringstream timess;
  int seconds = std::round(timeLeft/1000);
  int millis = std::min((float)99.0, (float)std::round(fmod(timeLeft,1000)/10));

  timess << seconds << ":";
  if (millis < 10) timess << "0";
  timess << millis;

  timeDisplay->setText(timess.str());

  // Update tilts
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i]) {
      Player *player = players[i];

      player->currTilt = Interpolator::interpQuat(player->currTiltDelay, elapsedTime,
                                                  tiltDelay,
                                                  player->lastTilt,
                                                  player->tiltDest);
    }
  }

  //TODO: Keep Local ghost camera for every player, or have them update and send their state to us?

  for (int i = 0; i < MAX_PLAYERS; i++) {
    Player *player = players[i];
    if (player) {

      // Set player's gravity based on the direction they are facing
      Ogre::Vector3 ocam = player->mCameraNode->_getDerivedPosition();
      btVector3 facingDirection = player->getBall()->getPosition() - btVector3(ocam[0], ocam[1], ocam[2]);
      facingDirection[1] = 0;
      facingDirection.normalize();

      btScalar yaw = btVector3(0,0,-1).angle(facingDirection);
      btQuaternion q((facingDirection[0] > 0 ? -yaw : yaw),0,0);
      q.normalize();

      //      if (gameEnded) {
      if (player->crossedFinishLine) {
        player->getBall()->getBody()->setGravity(btVector3(0, 1000, 0));
      } else if (countdown == -1) {
        btVector3 tweakedGrav = 2*app->mPhysics->getDynamicsWorld()->getGravity()
          .rotate(player->currTilt.getAxis(), -3*player->currTilt.getAngle())
          .rotate(q.getAxis(), q.getAngle());

        tweakedGrav[1] /= 2;
        player->getBall()->getBody()->setGravity(tweakedGrav);

      }

      // Update Camera Position
      player->mCameraObj->update((Ogre::Vector3)player->getBall()->getPosition(), elapsedTime);

      // This only works in this method, not from CameraObject. DONT ASK JUST ACCEPT
      player->mCameraNode->lookAt((Ogre::Vector3)player->getBall()->getPosition() + Ogre::Vector3(0,250,0), 
                                  Ogre::SceneNode::TS_WORLD);
      //      player->mCamera->lookAt((Ogre::Vector3)player->getPosition() + Ogre::Vector3(0,250,0));
      // Do ^ in client

      // Tilt Camera to simulate level tilt
      if (countdown == -1) {
        Ogre::Quaternion oq = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z());
        Ogre::Quaternion noq = Ogre::Quaternion(-q.w(), q.x(), q.y(), q.z());

        Ogre::Real xTilt = player->currTilt.x();
        if (xTilt < 0) xTilt /= 3;
        Ogre::Quaternion notilt = Ogre::Quaternion(-player->currTilt.w(),
                                                   xTilt,
                                                   player->currTilt.y(),
                                                   player->currTilt.z());

        if (!oq.isNaN() && !notilt.isNaN() && !noq.isNaN()) {
          player->mCameraLookAtNode->rotate(oq);
          player->mCameraLookAtNode->rotate(notilt*notilt);
          player->mCameraLookAtNode->rotate(noq);
        }
      }
    }
  }

  app->mCamera->lookAt((Ogre::Vector3)players[0]->getBall()->getPosition() + Ogre::Vector3(0,250,0));

  handleClientEvents();
  updateClients();

  return true;
}

void HostPlayerActivity::handleClientEvents(void) {
  while (SDLNet_CheckSockets(Networking::server_socketset, 1) > 0) {
    for (int i = 1; i < MAX_PLAYERS; i++) {
      if (players[i]) {
        TCPsocket csd = players[i]->csd;
        if (SDLNet_SocketReady(csd)) {
          ClientPacket cmsg;
          ServerPacket closemsg;

          if(SDLNet_TCP_Recv(csd, &cmsg, sizeof(cmsg)) > 0) {
            switch (cmsg.type) {
            case KEY_PRESSED:
              handleKeyPressed(cmsg.keyArg, cmsg.userID);
              break;
            case KEY_RELEASED:
              handleKeyReleased(cmsg.keyArg, cmsg.userID);
              break;
            case CLIENT_CHAT:
              /*              addChatMessage(cmsg.msg);

                              ServerPacket packet;
                              packet.type = SERVER_CLIENT_MESSAGE;
                              packet.clientId = i;
                              memcpy(packet.msg, cmsg.msg, 512);

                              for (int j = 1; j < MAX_PLAYERS; j++) {
                              if (i != j && players[j]) {
                              Networking::Send(players[j]->csd, (char*)&closemsg, sizeof(closemsg));
                              }
                              }*/
              break;
            case CLIENT_CLOSE:
              app->mPhysics->removeObject(players[cmsg.userID]->getBall());
              app->mSceneMgr->destroyEntity(players[cmsg.userID]->getBall()->getEntity());
              // Need to destroy more entities?

              SDLNet_TCP_Close(players[cmsg.userID]->csd);
              players[cmsg.userID] = NULL;

              closemsg.type = SERVER_CLIENT_CLOSED;
              closemsg.clientID = cmsg.userID;
              for (int j = 1; j < MAX_PLAYERS; j++) {
                if (players[j]) {
                  Networking::Send(players[j]->csd, (char*)&closemsg, sizeof(closemsg));
                }
              }
            }
          }
        }
      }
    }
  }
}

void HostPlayerActivity::updateClients(void) {
  ServerPacket msg;

  msg.type = SERVER_UPDATE;
  msg.timeLeft = timeLeft;

  std::deque<GameObject*> objects = app->mPhysics->getObjects();
  for (int i = 0; i < objects.size() && i < 200; i++) {
    msg.objectInfo[i].position = objects[i]->getPosition();
    msg.objectInfo[i].orientation = objects[i]->getOrientation();
  }

  for (int i = 1; i < MAX_PLAYERS; i++) {
    if (players[i]) {
      msg.camInfo.position = players[i]->mCameraNode->_getDerivedPosition();
      msg.camInfo.orientation = players[i]->mCameraNode->_getDerivedOrientation();

      Networking::Send(players[i]->csd, (char*)&msg, sizeof(msg));
    }
  }
}

//-------------------------------------------------------------------------------------

bool HostPlayerActivity::togglePauseMenu( const CEGUI::EventArgs& e ) {
  togglePauseMenu();
  return true;
}

void HostPlayerActivity::togglePauseMenu( ) {
  menuActive = !menuActive;
  if (menuActive) {
    CEGUI::MouseCursor::getSingleton().show();
    CEGUI::System::getSingleton().setGUISheet(app->Wmgr->getWindow("PauseMenu"));
    app->Wmgr->getWindow("PauseMenu/Quit")
      ->subscribeEvent(CEGUI::PushButton::EventClicked,
                       CEGUI::Event::Subscriber(&HostPlayerActivity::ExitToMenu, this));
    app->Wmgr->getWindow("PauseMenu/Return")
      ->subscribeEvent(CEGUI::PushButton::EventClicked,
                       CEGUI::Event::Subscriber(&HostPlayerActivity::togglePauseMenu, this));
  } else {
    CEGUI::MouseCursor::getSingleton().hide();
    CEGUI::System::getSingleton().setGUISheet(app->Wmgr->getWindow("SinglePlayerHUD"));
  }
}

bool HostPlayerActivity::ExitToMenu( const CEGUI::EventArgs& e ) {
  app->switchActivity(new MenuActivity(app));
  return true;
}

void HostPlayerActivity::handleGameEnd() {
  ceguiActive = true;
  gameEnded = true;

  CEGUI::MouseCursor::getSingleton().show();
  CEGUI::System::getSingleton().setGUISheet(app->Wmgr->getWindow("GameWon"));

  app->Wmgr->getWindow("GameWon/BackToMenu")
    ->subscribeEvent(CEGUI::PushButton::EventClicked,
                     CEGUI::Event::Subscriber(&HostPlayerActivity::ExitToMenu, this));

  /*  app->Wmgr->getWindow("GameWon/NextLevel")
      ->subscribeEvent(CEGUI::PushButton::EventClicked,
      CEGUI::Event::Subscriber(&HostPlayerActivity::nextLevel, this));*/
}

//-------------------------------------------------------------------------------------

bool HostPlayerActivity::keyPressed( const OIS::KeyEvent &arg )
{
  if (arg.key == OIS::KC_ESCAPE) {
    if (chatFocus) {
      // toggleChat();
      // chatEditBox->setText("");
      // return true;
    } else if (inGame) {
      togglePauseMenu();
      return true;
    }
  }

  if (!inGame || chatFocus || ceguiActive || menuActive) {
    CEGUI::System::getSingleton().injectKeyDown(arg.key);
    CEGUI::System::getSingleton().injectChar(arg.text);
    return true;
  }

  return handleKeyPressed(arg.key, myId);
}

bool HostPlayerActivity::handleKeyPressed( OIS::KeyCode arg, int userId ) {
  Player *player = players[userId];
  if (!player) return false;

  switch(arg){
  case OIS::KC_D:
    player->tiltDest *= btQuaternion(0,0,-MAX_TILT);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  case OIS::KC_A:
    player->tiltDest *= btQuaternion(0,0,MAX_TILT);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  case OIS::KC_W:
    player->tiltDest *= btQuaternion(0,-MAX_TILT,0);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  case OIS::KC_S:
    player->tiltDest *= btQuaternion(0,MAX_TILT,0);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  default:
    return false;
  }

  return true;
}
//-------------------------------------------------------------------------------------

bool HostPlayerActivity::keyReleased( const OIS::KeyEvent &arg )
{
  if (!inGame || chatFocus || menuActive || ceguiActive) {
    CEGUI::System::getSingleton().injectKeyUp(arg.key);
  }

  return handleKeyReleased(arg.key, myId);
}

bool HostPlayerActivity::handleKeyReleased( OIS::KeyCode arg, int userId ) {
  Player *player = players[userId];
  if (!player) return false;

  switch(arg){
  case OIS::KC_D:
    player->tiltDest *= btQuaternion(0,0,MAX_TILT);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  case OIS::KC_A:
    player->tiltDest *= btQuaternion(0,0,-MAX_TILT);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  case OIS::KC_W:
    player->tiltDest *= btQuaternion(0,MAX_TILT,0);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  case OIS::KC_S:
    player->tiltDest *= btQuaternion(0,-MAX_TILT,0);
    player->lastTilt = player->currTilt;
    player->currTiltDelay = 0;
    break;
  default:
    return false;
  }

  return true;
}

//-------------------------------------------------------------------------------------

bool HostPlayerActivity::mouseMoved( const OIS::MouseEvent &arg )
{
  if (!inGame || chatFocus || menuActive || ceguiActive) {
    CEGUI::System &sys = CEGUI::System::getSingleton();
    sys.injectMouseMove(arg.state.X.rel, arg.state.Y.rel);
    // Scroll wheel.
    if (arg.state.Z.rel)
      sys.injectMouseWheelChange(arg.state.Z.rel / 120.0f);
    return true;
  }

  return true;
}

//-------------------------------------------------------------------------------------

bool HostPlayerActivity::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
  if (!inGame || chatFocus || menuActive || ceguiActive) {
    CEGUI::System::getSingleton().injectMouseButtonDown(OgreBallApplication::convertButton(id));
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------------------

bool HostPlayerActivity::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
  if (!inGame || chatFocus || menuActive || ceguiActive) {
    CEGUI::System::getSingleton().injectMouseButtonUp(OgreBallApplication::convertButton(id));
    return true;
  }
  return false;
}
