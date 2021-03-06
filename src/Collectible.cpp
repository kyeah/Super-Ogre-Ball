#include "OgreBallApplication.h"
#include "GameObject.h"
#include "GameObjectDescription.h"
#include "../libs/MeshStrider.h"
#include "SinglePlayerActivity.h"
#include "HostPlayerActivity.h"
#include "Networking.h"
#include "common.h"
#include "Sounds.h"

Collectible::Collectible(Ogre::SceneManager *mgr, Ogre::String _entName, Ogre::String _meshName, Ogre::String _nodeName, Ogre::SceneNode* parentNode,
                         Physics* _physics,
                         btVector3 origin, btVector3 scale,
                         btVector3 velocity, btScalar _mass, btScalar _rest,
                         btVector3 _localInertia, btQuaternion *rotation, Ogre::String hitSound)
  : GameObject(mgr, _entName, _nodeName, parentNode, _physics, origin, scale, velocity, _mass, _rest, _localInertia, rotation)
{

  entity = mgr->createEntity(_entName, _meshName);
  entity->setCastShadows(true);


  node->attachObject(entity);

  Ogre::MeshPtr meshptr = Ogre::Singleton<Ogre::MeshManager>::getSingletonPtr()->load(_meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  MeshStrider *strider = new MeshStrider(meshptr.get());
  collisionShape = new btBvhTriangleMeshShape(strider,true,true);

  addToSimulator(Collisions::CollisionTypes::COL_COLLECTIBLE,
                 Collisions::collectibleColliders);

  setAmbient(0.5,0.0,0.0);
  setSpecular(0.1,0,0,0.4);
  if (rotation) rotate(*rotation);

  isHit = false;
  mHitSound = hitSound;
}

void Collectible::update(float elapsedTime) {
  GameObject::update(elapsedTime);
  //check collisions
  if(physics->checkCollisions(this)){
    for(int i = 0; i < contexts.size(); i++){
      if(contexts[i]->object){
        OgreBall *ob = dynamic_cast<OgreBall*>(contexts[i]->object);
        if(ob && !isHit){
          isHit = true;
          std::cout << "Hit" << std::endl;

          Activity *a = OgreBallApplication::getSingleton()->activity;
          HostPlayerActivity *h = dynamic_cast<HostPlayerActivity*>(a);

          if (h) {
            ServerPacket packet;
            packet.type = SERVER_OBJECT_REMOVAL;
            packet.clientID = physics->indexOfObject(this);

            for (int j = 1; j < MAX_PLAYERS; j++) {
              if (players[j]) {
                Networking::Send(players[j]->csd, (char*)&packet, sizeof(packet));
              }
            }
          }
          removeFromSimulator();

          if (a) {
            a->score += 8000;
            a->collectibles += 1;
          }

          Sounds::playSoundEffect(mHitSound.c_str());
        }
      }
    }
  }
}
