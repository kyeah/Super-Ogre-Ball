#pragma once

#include <CEGUI/CEGUI.h>
#include <CEGUI/RendererModules/Ogre/CEGUIOgreRenderer.h>
#include "Activity.h"

class MenuActivity : public Activity {
 private:
  static std::vector<LevelViewer*> viewerPool;

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
  bool SwitchToLevelSelectMenu(const CEGUI::EventArgs& e);
  bool SwitchToPlayerSelectMenu(const CEGUI::EventArgs& e);
  bool SwitchToMultiMenu(const CEGUI::EventArgs& e);

  bool SelectPenguin( const CEGUI::EventArgs& e);
  bool SelectOgre( const CEGUI::EventArgs& e);

  bool SwitchToHostSelectMenu( const CEGUI::EventArgs& e);
  bool SinglePlayerLevelSelectWrapper(const CEGUI::EventArgs& e);
  bool MultiPlayerLevelSelectWrapper(const CEGUI::EventArgs& e);

  bool StartSinglePlayer( const CEGUI::EventArgs& e );
  bool StartMultiPlayerHost( const CEGUI::EventArgs& e);
  bool StartMultiPlayerClient( const CEGUI::EventArgs& e);

  bool ShowLeaderboard( const CEGUI::EventArgs& e );
  bool handleLSPrev( const CEGUI::EventArgs& e );
  bool handleLSNext( const CEGUI::EventArgs& e );

  bool quit(const CEGUI::EventArgs& e);

  CEGUI::Window *levelSelectorWindow, *lsBack, *lsPrev, *lsNext;
  CEGUI::Window *lsButtons[8];

  int selectorStart, selectorRows, selectorColumns;
  int chosenCharacter;
  std::vector<LevelViewer*> levelViewers;
};
