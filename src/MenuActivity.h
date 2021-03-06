#pragma once

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/CEGUIOgreRenderer.h>
#include "Activity.h"

class MenuActivity : public Activity {
 public:
  MenuActivity(OgreBallApplication *app);
  virtual ~MenuActivity(void);
  virtual void close(void);

  virtual void start(void);

  virtual bool frameStarted( Ogre::Real elapsedTime );
  virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);

  virtual bool keyPressed( const OIS::KeyEvent &arg );
  virtual bool keyReleased( const OIS::KeyEvent &arg );

  virtual bool mouseMoved( const OIS::MouseEvent &arg );
  virtual bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
  virtual bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
  virtual void handleGameEnd();

  bool SwitchToMainMenu(const CEGUI::EventArgs& e);
  bool SwitchToPlayerSelectMenu(const CEGUI::EventArgs& e);
  bool SwitchToServerListMenu(const CEGUI::EventArgs& e);
  bool SwitchToMultiMenu(const CEGUI::EventArgs& e);
  bool SwitchToOptions( const CEGUI::EventArgs& e );

  bool JoinServer(const CEGUI::EventArgs& e);
  bool PromptForJoinServer(const CEGUI::EventArgs& e);
  bool PromptForHost(const CEGUI::EventArgs& e);

  bool CancelPrompt(const CEGUI::EventArgs& e);
  bool CancelHprompt(const CEGUI::EventArgs& e);

  bool SwitchToHostSelectMenu( const CEGUI::EventArgs& e);
  bool SinglePlayerLevelSelectWrapper(const CEGUI::EventArgs& e);
  bool MultiPlayerLevelSelectWrapper(const CEGUI::EventArgs& e);

  bool StartSinglePlayer( const CEGUI::EventArgs& e );
  bool StartMultiPlayerHost( const CEGUI::EventArgs& e);
  bool StartMultiPlayerClient( const CEGUI::EventArgs& e);
  bool onSliderValueChanged( const CEGUI::EventArgs& e );
  bool onSliderSEValueChanged( const CEGUI::EventArgs& e );

  bool quit(const CEGUI::EventArgs& e);
  
  CEGUI::Window *mainMenuSheet, *singlePlayerButton,
    *multiPlayerButton, *quitButton, *optionsButton;

  CEGUI::Window *multiMenuSheet, *hostButton, *clientButton, *returnButton, *optionsMenuSheet, *returnOptionButton;

  CEGUI::Slider *mSoundSlider, *mSoundEffectSlider;

  CEGUI::Window *serverListWindow, *serverListBack;
  CEGUI::Listbox *serverListbox;

  CEGUI::Window *promptWindow, *promptHeader, *promptSubmit, *promptCancel;
  CEGUI::Editbox *promptInputbox;

  CEGUI::Window *hPromptWindow, *hPromptHeader, *hPromptSubmit, *hPromptCancel;
  CEGUI::Editbox *hPromptInputbox, *hPromptLobbyNameInputbox;
};
