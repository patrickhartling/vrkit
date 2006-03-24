// Copyright (C) Infiscape Corporation 2005-2006

#include <IOV/StaticSceneObject.h>
#include <IOV/Util/Exceptions.h>

#include <sstream>

namespace inf
{

StaticSceneObject::~StaticSceneObject()
{
   /* Do nothing. */ ;
}

StaticSceneObject::StaticSceneObject()
{
   mEmptyVolume.setEmpty();
}

void StaticSceneObject::moveTo(const OSG::Matrix& matrix)
{
   OSG::beginEditCP(mTransformNode.core(), OSG::Transform::MatrixFieldMask);
      mTransformNode->setMatrix(matrix);
   OSG::endEditCP(mTransformNode.core(), OSG::Transform::MatrixFieldMask);
}

OSG::Matrix StaticSceneObject::getPos()
{
   return mTransformNode->getMatrix();
}

OSG::NodeRefPtr StaticSceneObject::getRoot()
{
   return OSG::NodeRefPtr(mTransformNode.node());
}

OSG::DynamicVolume& StaticSceneObject::getVolume(const bool update)
{
   if (OSG::NullFC == mTransformNode.node())
   {
      return mEmptyVolume;
   }
   return mTransformNode.node()->getVolume(update);
}


bool StaticSceneObject::hasParent() const
{
   return mParent.lock().get() != NULL;
}

SceneObjectPtr StaticSceneObject::getParent()
{
   return mParent.lock();
}

void StaticSceneObject::setParent(SceneObjectPtr parent)
{
   mParent = parent;
}

bool StaticSceneObject::hasChildren() const
{
   return ! mChildren.empty();
}

unsigned int StaticSceneObject::getChildCount() const
{
   return mChildren.size();
}

void StaticSceneObject::addChild(SceneObjectPtr child)
{
   // We only add a child if it points to valid data.
   if ( child.get() != NULL )
   {
      // If child already has a parent, we have to break that relationship.
      if ( child->hasParent() )
      {
         child->getParent()->removeChild(child);
      }

      child->setParent(shared_from_this());
      mChildren.push_back(child);

      //fireChildAdded(child);
   }
}

void StaticSceneObject::removeChild(SceneObjectPtr child)
{
   std::vector<SceneObjectPtr>::iterator c = std::find(mChildren.begin(),
                                                       mChildren.end(),
                                                       child);

   if ( c != mChildren.end() )
   {
      // child no longer has a parent.
      child->setParent(SceneObjectPtr());
      mChildren.erase(c);

      //fireChildRemoved(child);
   }
}

void StaticSceneObject::removeChild(const unsigned int childIndex)
{
   if ( childIndex < mChildren.size() )
   {
      removeChild(mChildren[childIndex]);
   }
   else
   {
      std::ostringstream msg;
      msg << "removeChild() failed: Child index " << childIndex
          << " is not in the range [0," << mChildren.size() << ")";
      throw inf::Exception(msg.str(), IOV_LOCATION);
   }
}

SceneObjectPtr StaticSceneObject::getChild(const unsigned int childIndex)
{
   if ( childIndex < mChildren.size() )
   {
      return mChildren[childIndex];
   }
   else
   {
      std::ostringstream msg;
      msg << "getChild() failed: Child index " << childIndex
          << " is not in the range [0," << mChildren.size() << ")";
      throw inf::Exception(msg.str(), IOV_LOCATION);
   }
}

std::vector<SceneObjectPtr> StaticSceneObject::getChildren()
{
   return mChildren;
}

}