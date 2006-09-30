// Copyright (C) Infiscape Corporation 2005-2006

#ifndef _INF_GRAB_PLUGIN_H_
#define _INF_GRAB_PLUGIN_H_

#include <IOV/Plugin/PluginConfig.h>

#include <string>
#include <map>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

#include <gmtl/Matrix.h>
#include <gmtl/Point.h>

#include <IOV/EventData.h>
#include <IOV/Plugin.h>
#include <IOV/WandInterfacePtr.h>
#include <IOV/PluginFactory.h>
#include <IOV/Grab/GrabStrategyPtr.h>
#include <IOV/Grab/MoveStrategyPtr.h>
#include <IOV/SceneObjectPtr.h>


namespace inf
{

class GrabPlugin
   : public inf::Plugin
   , public boost::enable_shared_from_this<GrabPlugin>
{
protected:
   GrabPlugin();

public:
   static inf::PluginPtr create()
   {
      return inf::PluginPtr(new GrabPlugin());
   }

   virtual ~GrabPlugin();

   virtual std::string getDescription()
   {
      return std::string("Grabbing");
   }

   virtual PluginPtr init(inf::ViewerPtr viewer);

   virtual void update(inf::ViewerPtr viewer);

   bool config(jccl::ConfigElementPtr elt);

   /**
    * Invokes the global scope delete operator.  This is required for proper
    * releasing of memory in DLLs on Win32.
    */
   void operator delete(void* p)
   {
      ::operator delete(p);
   }

protected:
   /**
    * Deletes this object.  This is an implementation of the pure virtual
    * inf::Plugin::destroy() method.
    */
   virtual void destroy()
   {
      delete this;
   }

   void focusChanged(inf::ViewerPtr viewer);

   inf::Event::ResultType
      defaultObjectsMovedSlot(const EventData::moved_obj_list_t& objs);

private:
   static std::string getElementType()
   {
      return std::string("iov_grab_plugin");
   }

   void objectsGrabbed(inf::ViewerPtr viewer,
                       const std::vector<SceneObjectPtr>& objs,
                       const gmtl::Point3f& isectPoint);

   void objectsReleased(inf::ViewerPtr viewer,
                        const std::vector<SceneObjectPtr>& objs);

   /**
    * Responds to the signal emitted when the grabbable state of a scene
    * object changes. If \p obj is currently grabbed, then it is released.
    *
    * @param obj    The scene object that was removed from inf::GrabData.
    * @param viewer The VR Juggler application object within which this
    *               plug-in is active.
    *
    * @see objectsReleased()
    */
   void grabbableObjStateChanged(inf::SceneObjectPtr obj,
                                 inf::ViewerPtr viewer);

   WandInterfacePtr mWandInterface;

   /** @name Plug-in Management */
   //@{
   std::vector<std::string> mStrategyPluginPath;
   inf::PluginFactoryPtr mPluginFactory;
   //@}

   /** @name Grab Strategy */
   //@{
   std::string     mGrabStrategyName;
   GrabStrategyPtr mGrabStrategy;
   //@}

   /** @name Move Strategies */
   //@{
   std::map<SceneObjectPtr, gmtl::Matrix44f> mGrabbed_pobj_M_obj_map;
   std::vector<MoveStrategyPtr> mMoveStrategies;
   std::vector<std::string> mMoveStrategyNames;
   //@}

   EventDataPtr mEventData;
};

}


#endif
