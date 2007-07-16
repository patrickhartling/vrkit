// Copyright (C) Infiscape Corporation 2005-2007

#include <IOV/Config.h>

#include <functional>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>

#include <gmtl/External/OpenSGConvert.h>

#include <vpr/Util/Assert.h>
#include <jccl/Config/ConfigElement.h>

#include <IOV/Plugin/PluginConfig.h>
#include <IOV/PluginCreator.h>
#include <IOV/EventData.h>
#include <IOV/Scene.h>
#include <IOV/User.h>
#include <IOV/Viewer.h>
#include <IOV/InterfaceTrader.h>
#include <IOV/WandInterface.h>
#include <IOV/SceneObject.h>
#include <IOV/StatusPanelData.h>
#include <IOV/Util/Exceptions.h>
#include <IOV/Config.h>
#include <IOV/Version.h>
#include <IOV/Plugin/Info.h>

#include "MultiObjectGrabStrategy.h"


using namespace boost::assign;

static const inf::plugin::Info sInfo(
   "com.infiscape.grab", "MultiObjectGrabStrategy",
   list_of(IOV_VERSION_MAJOR)(IOV_VERSION_MINOR)(IOV_VERSION_PATCH)
);
static inf::PluginCreator<inf::GrabStrategy> sPluginCreator(
   boost::bind(&inf::MultiObjectGrabStrategy::create, sInfo)
);

extern "C"
{

/** @name Plug-in Entry Points */
//@{
IOV_PLUGIN_API(const inf::plugin::Info*) getPluginInfo()
{
   return &sInfo;
}

IOV_PLUGIN_API(void) getPluginInterfaceVersion(vpr::Uint32& majorVer,
                                               vpr::Uint32& minorVer)
{
   majorVer = INF_GRAB_STRATEGY_PLUGIN_API_MAJOR;
   minorVer = INF_GRAB_STRATEGY_PLUGIN_API_MINOR;
}

IOV_PLUGIN_API(inf::PluginCreatorBase*) getGrabStrategyCreator()
{
   return &sPluginCreator;
}
//@}

}

namespace inf
{

MultiObjectGrabStrategy::MultiObjectGrabStrategy(const inf::plugin::Info& info)
   : GrabStrategy(info)
   , mChooseText("Choose object to grab")
   , mGrabText("Grab object(s)")
   , mReleaseText("Release object(s)")
   , mGrabbing(false)
{
   /* Do nothing. */ ;
}

MultiObjectGrabStrategy::~MultiObjectGrabStrategy()
{
   std::for_each(
      mObjConnections.begin(), mObjConnections.end(),
      boost::bind(&boost::signals::connection::disconnect,
                  boost::bind(&obj_conn_map_t::value_type::second, _1))
   );
   std::for_each(mIsectConnections.begin(), mIsectConnections.end(),
                 boost::bind(&boost::signals::connection::disconnect, _1));
}

GrabStrategyPtr MultiObjectGrabStrategy::
init(ViewerPtr viewer, grab_callback_t grabCallback,
     release_callback_t releaseCallback)
{
   mEventData = viewer->getSceneObj()->getSceneData<EventData>();

   mGrabCallback    = grabCallback;
   mReleaseCallback = releaseCallback;

   // Connect the intersection signal to our slot.
   mIsectConnections.push_back(
      mEventData->mObjectIntersectedSignal.connect(
         0, boost::bind(&MultiObjectGrabStrategy::objectIntersected, this,
                        _1, _2)
      )
   );

   // Connect the de-intersection signal to our slot.
   mIsectConnections.push_back(
      mEventData->mObjectDeintersectedSignal.connect(
         0, boost::bind(&MultiObjectGrabStrategy::objectDeintersected,
                        this, _1)
      )
   );

   InterfaceTrader& if_trader = viewer->getUser()->getInterfaceTrader();
   mWandInterface = if_trader.getWandInterface();

   // Configure
   std::string elt_type_name = getElementType();
   jccl::ConfigElementPtr cfg_elt =
      viewer->getConfiguration().getConfigElement(elt_type_name);

   if ( cfg_elt )
   {
      // Configure ourself.
      configure(cfg_elt);
   }

   return shared_from_this();
}

void MultiObjectGrabStrategy::setFocus(ViewerPtr viewer, const bool focused)
{
   // If we have focus, we will try to update the staus panel to include our
   // commands.
   if ( focused )
   {
      inf::ScenePtr scene = viewer->getSceneObj();
      StatusPanelDataPtr status_panel_data =
         scene->getSceneData<StatusPanelData>();

      // If choose button(s) is/are configured, we will update the status
      // panel to include that information.
      if ( mChooseBtn.isConfigured() )
      {
         bool has = false;
         status_panel_data->mHasControlTexts(mChooseBtn.getButtons(), mChooseText, has);
         if ( ! has )
         {
            // The button numbers in mChooseBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            status_panel_data->mAddControlTexts(
               transformButtonVec(mChooseBtn.getButtons()), mChooseText, 1
            );
         }
      }

      // If grab button(s) is/are configured, we will update the status
      // panel to include that information.
      if ( mGrabBtn.isConfigured() )
      {
         bool has = false;
         status_panel_data->mHasControlTexts(mGrabBtn.getButtons(), mGrabText, has);
         if ( ! has )
         {
            // The button numbers in mGrabBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            status_panel_data->mAddControlTexts(transformButtonVec(mGrabBtn.getButtons()),
                                 mGrabText, 1);
         }
      }

      // If release button(s) is/are configured, we will update the status
      // panel to include that information.
      if ( mReleaseBtn.isConfigured() )
      {
         bool has = false;
         status_panel_data->mHasControlTexts(mReleaseBtn.getButtons(), mReleaseText, has);
         if ( ! has )
         {
            // The button numbers in mReleaseBtn are zero-based, but we
            // would like them to be one-based in the status panel display.
            status_panel_data->mAddControlTexts(
               transformButtonVec(mReleaseBtn.getButtons()), mReleaseText, 1
            );
         }
      }
   }
   else
   {
      inf::ScenePtr scene = viewer->getSceneObj();
      StatusPanelDataPtr status_panel_data =
         scene->getSceneData<StatusPanelData>();

      // The button numbers in mChooseBtn are zero-based, but we would like
      // them to be one-based in the status panel display.
      status_panel_data->mRemoveControlTexts(transformButtonVec(mChooseBtn.getButtons()),
                              mChooseText);

      // The button numbers in mGrabBtn are zero-based, but we would like
      // them to be one-based in the status panel display.
      status_panel_data->mRemoveControlTexts(transformButtonVec(mGrabBtn.getButtons()),
                              mGrabText);

      // The button numbers in mReleaseBtn are zero-based, but we would like
      // them to be one-based in the status panel display.
      status_panel_data->mRemoveControlTexts(transformButtonVec(mReleaseBtn.getButtons()),
                              mReleaseText);
   
   }
}

void MultiObjectGrabStrategy::update(ViewerPtr viewer)
{
   if ( ! mGrabbing )
   {
      // The user has requested to add mCurIsectObject to the collection of
      // objects selected for later grabbing.
      if ( mCurIsectObject && mCurIsectObject->isGrabbable() &&
           mChooseBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         std::vector<SceneObjectPtr>::iterator o =
            std::find(mChosenObjects.begin(), mChosenObjects.end(),
                      mCurIsectObject);

         // Only choose mCurIsectObject for grabbing if it is not already in
         // mChosenObjcts.
         if ( o == mChosenObjects.end() )
         {
            mChosenObjects.push_back(mCurIsectObject);

            // Connect the grabbable object state change signal to our slot.
            mObjConnections[mCurIsectObject] =
               mCurIsectObject->grabbableStateChanged().connect(
                  boost::bind(
                     &MultiObjectGrabStrategy::grabbableObjStateChanged,
                     this, _1
                  )
               );

            std::vector<SceneObjectPtr> objs(1, mCurIsectObject);
            mEventData->mSelectionListExpandedSignal(objs);

            // Use the intersection point of the most recently chosen object.
            mIntersectPoint = mCurIntersectPoint;
         }
      }
      // The user has requested to grab all the selected objects (those in
      // mChosenObjects).
      else if ( mGrabBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         // If mChosenObjects is not empty, those objects are the ones that
         // we will grab.
         if ( ! mChosenObjects.empty() )
         {
            mGrabbedObjects = mChosenObjects;
            mChosenObjects.clear();
            mGrabbing = true;
         }
         // If mChosenObjects is empty but we are intersecting an object,
         // then that will be the one that we grab.
         else if ( mCurIsectObject )
         {
            mGrabbedObjects.resize(1);
            mGrabbedObjects[0] = mCurIsectObject;
            mGrabbing = true;
         }

         // If mGrabbing is false at this point, then there is nothing to
         // grab. Otherwise, invoke mGrabCallback to indicate that one or more
         // objects are now grabbed.
         if ( mGrabbing )
         {
            mGrabCallback(mGrabbedObjects, mIntersectPoint);
         }
      }
   }
   // If we are grabbing an object and the release button has just been
   // pressed, then release the grabbed object.
   else if ( mGrabbing &&
             mReleaseBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
   {
      // We no longer care about grabbable state changes because
      // mGrabbedObjects is about to be emptied.
      std::for_each(
         mObjConnections.begin(), mObjConnections.end(),
         boost::bind(&boost::signals::connection::disconnect,
                     boost::bind(&obj_conn_map_t::value_type::second, _1))
      );

      // Update our state to reflect that we are no longer grabbing an object.
      mGrabbing = false;
      mReleaseCallback(mGrabbedObjects);
      mGrabbedObjects.clear();
   }
}

std::vector<SceneObjectPtr> MultiObjectGrabStrategy::getGrabbedObjects()
{
   return mGrabbedObjects;
}

void MultiObjectGrabStrategy::configure(jccl::ConfigElementPtr elt)
{
   vprASSERT(elt->getID() == getElementType());

   const unsigned int req_cfg_version(1);

   // Check for correct version of plugin configuration.
   if ( elt->getVersion() < req_cfg_version )
   {
      std::stringstream msg;
      msg << "Configuration of MultiObjectGrabStrategy failed.  Required "
          << "config element version is " << req_cfg_version
          << ", but element '" << elt->getName() << "' is version "
          << elt->getVersion();
      throw PluginException(msg.str(), IOV_LOCATION);
   }

   const std::string choose_btn_prop("choose_button_nums");
   const std::string grab_btn_prop("grab_button_nums");
   const std::string release_btn_prop("release_button_nums");

   mChooseBtn.configButtons(elt->getProperty<std::string>(choose_btn_prop));
   mGrabBtn.configButtons(elt->getProperty<std::string>(grab_btn_prop));
   mReleaseBtn.configButtons(elt->getProperty<std::string>(release_btn_prop));
}

struct IncValue
{
   int operator()(int v)
   {
      return v + 1;
   }
};

std::vector<int> MultiObjectGrabStrategy::
transformButtonVec(const std::vector<int>& btns)
{
   std::vector<int> result(btns.size());
   IncValue inc;
   std::transform(btns.begin(), btns.end(), result.begin(), inc);
   return result;
}

inf::Event::ResultType MultiObjectGrabStrategy::
objectIntersected(SceneObjectPtr obj,
                  const gmtl::Point3f& pnt)
{
   if ( ! mGrabbing )
   {
      // If we intersected a grabbable object.
      if ( obj->isGrabbable() )
      {
         mCurIsectObject = obj;
         mCurIntersectPoint = pnt;
      }
   }
   else
   {
      return inf::Event::DONE;
   }
   
   return inf::Event::CONTINUE;
}

inf::Event::ResultType MultiObjectGrabStrategy::
objectDeintersected(SceneObjectPtr obj)
{
   if ( mGrabbing )
   {
      return inf::Event::DONE;
   }

   mCurIsectObject = SceneObjectPtr();

   return inf::Event::CONTINUE;
}

void MultiObjectGrabStrategy::grabbableObjStateChanged(inf::SceneObjectPtr obj)
{
   // If mChosenObjects is not empty, then we have a collection of objects to
   // grab that have not yet been grabbed. obj may be in mChosenObjects, and
   // if it is not grabbable, then it must be removed.
   if ( ! mChosenObjects.empty() )
   {
      std::vector<SceneObjectPtr>::iterator o =
         std::find(mChosenObjects.begin(), mChosenObjects.end(), obj);

      if ( o != mChosenObjects.end() && ! (*o)->isGrabbable() )
      {
         mObjConnections[*o].disconnect();
         mObjConnections.erase(*o);

         std::vector<SceneObjectPtr> objs(1, *o);
         mEventData->mSelectionListReducedSignal(objs);

         mChosenObjects.erase(o);
      }
   }
   // If mGrabbedObjects is not empty, then we have grabbed objects, and obj
   // may be in that collection. If obj is no longer grabbable, it must be
   // removed from mGrabbedObjects.
   else if ( ! mGrabbedObjects.empty() )
   {
      std::vector<SceneObjectPtr>::iterator o =
         std::find(mGrabbedObjects.begin(), mGrabbedObjects.end(), obj);

      if ( o != mGrabbedObjects.end() && ! (*o)->isGrabbable() )
      {
         mObjConnections[*o].disconnect();
         mObjConnections.erase(*o);

         // Inform the code that is using us that we have released a grabbed
         // object.
         mReleaseCallback(std::vector<SceneObjectPtr>(1, *o));

         mGrabbedObjects.erase(o);

         // Keep the grabbing state up to date now that mGrabbedObjects has
         // changed.
         mGrabbing = ! mGrabbedObjects.empty();
      }
   }
}

}
