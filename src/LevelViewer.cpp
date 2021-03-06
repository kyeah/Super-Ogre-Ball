#include <OgreRenderTexture.h>
#include <OgreViewport.h>

#include "Interpolator.h"
#include "LevelLoader.h"
#include "LevelViewer.h"

using namespace std;

LevelViewer::LevelViewer(CEGUI::OgreRenderer *mRenderer, const char* levelName) {

  static int id = 0;

  stringstream ss;
  ss << "RTT" << id++;
  string ogreTextureName = ss.str();
  ss << "Image";
  myCeguiImageName = ss.str();
  ss << "set";
  string ceguiImagesetName = ss.str();

  // Create Scene
  mSceneMgr = Ogre::Root::getSingletonPtr()->createSceneManager(Ogre::ST_GENERIC);
  mCamera = mSceneMgr->createCamera("levelCam");
  mCamera->setNearClipDistance(5);
  levelRoot = mSceneMgr->getRootSceneNode()->createChildSceneNode("root");
  mPhysics = new Physics(btVector3(0, -19600, 0));

  // Create Ogre Render-to-Texture
  Ogre::TexturePtr tex = Ogre::Root::getSingletonPtr()->getTextureManager()
    ->createManual(
                   ogreTextureName,
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                   Ogre::TEX_TYPE_2D,
                   512,
                   512,
                   0,
                   Ogre::PF_R8G8B8,
                   Ogre::TU_RENDERTARGET);

  Ogre::RenderTexture *rtex = tex->getBuffer()->getRenderTarget();
  Ogre::Viewport *v = rtex->addViewport(mCamera);
  v->setOverlaysEnabled(false);
  v->setClearEveryFrame(true);
  v->setBackgroundColour(Ogre::ColourValue::Black);

  // Create CEGUI Texture from Ogre Texture
  CEGUI::Texture &guiTex = mRenderer->createTexture(tex);
  myImageSet = &CEGUI::ImagesetManager::getSingleton().create(ceguiImagesetName, guiTex);

  myImageSet->defineImage(myCeguiImageName,
                          CEGUI::Point(0.0f, 0.0f),
                          CEGUI::Size(guiTex.getSize().d_width,
                                      guiTex.getSize().d_height),
                          CEGUI::Point(0.0f, 0.0f));

  loadLevel(levelName);
}

LevelViewer::~LevelViewer(void) {
  mPhysics->removeAllObjects();
  mSceneMgr->destroyAllEntities();
  mSceneMgr->getRootSceneNode()->removeAndDestroyAllChildren();
  mSceneMgr->destroyAllLights();
  mSceneMgr->destroyAllParticleSystems();
  mSceneMgr->destroyAllRibbonTrails();
  mSceneMgr->destroyAllCameras();

  mSceneMgr->clearScene();

  CEGUI::WindowManager::getSingleton().destroyWindow(window);
}

void LevelViewer::loadLevel(const char* levelName) {
  mPhysics->removeAllObjects();
  mSceneMgr->destroyAllEntities();
  mSceneMgr->getRootSceneNode()->removeAndDestroyAllChildren();
  mSceneMgr->destroyAllLights();
  mSceneMgr->destroyAllParticleSystems();
  mSceneMgr->destroyAllRibbonTrails();
  
  levelRoot = mSceneMgr->getRootSceneNode()->createChildSceneNode("root");

  LevelLoader::getSingleton()->loadLevel(this, levelName);

  // Setup the window associated with the level
  CEGUI::WindowManager *wmgr = CEGUI::WindowManager::getSingletonPtr();
  if (wmgr->isWindowPresent(levelName))
    window = wmgr->getWindow(levelName);
  else
    window = wmgr->createWindow("OgreBall/SelectorImage", levelName);

  window->setText(levelName);
  window->setSize(CEGUI::UVector2(CEGUI::UDim(0.21f, 0),
                                  CEGUI::UDim(0.15f, 0)));
  window->setPosition(CEGUI::UVector2(CEGUI::UDim(0.5f, 0),
                                      CEGUI::UDim(0.1f, 0)));

  window->setProperty("Image", CEGUI::PropertyHelper::imageToString(&myImageSet->getImage(myCeguiImageName)));
}

void LevelViewer::setPositionPercent(float x, float y) {
  window->setPosition(CEGUI::UVector2(CEGUI::UDim(x, 0),
                                      CEGUI::UDim(y, 0)));
}

bool LevelViewer::frameStarted(Ogre::Real elapsedTime) {
  if (mPhysics) mPhysics->stepSimulation(elapsedTime);

  // Rad camera panning; not yet parsed in scripts or read into LevelViewer variables.
  if (camPosKnobs.size() > 0) {
    btVector3 pos = Interpolator::interpV3(currentInterpCamPosTime,
                                           elapsedTime,
                                           totalCamPosInterpTime,
                                           camPosKnobs,
                                           camPosInterpTimes);

    mCamera->setPosition(pos[0], pos[1], pos[2]);
  }

  if (camLookAtKnobs.size() > 0) {
    btVector3 lk = Interpolator::interpV3(currentInterpCamLookAtTime,
                                          elapsedTime,
                                          totalCamLookAtInterpTime,
                                          camLookAtKnobs,
                                          camLookAtInterpTimes);

    mCamera->lookAt(lk[0], lk[1], lk[2]);
  }
  return true;
}
