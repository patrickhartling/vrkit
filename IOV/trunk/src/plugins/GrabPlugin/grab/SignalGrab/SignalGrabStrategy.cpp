// Copyright (C) Infiscape Corporation 2005-2007

#include <IOV/Config.h>

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>

#include <OpenSG/OSGConfig.h>

#include <IOV/Plugin/PluginConfig.h>
#include <IOV/PluginCreator.h>
#include <IOV/GrabSignalData.h>
#include <IOV/InterfaceTrader.h>
#include <IOV/WandInterface.h>
#include <IOV/User.h>
#include <IOV/Scene.h>
#include <IOV/SceneObject.h>
#include <IOV/Viewer.h>
#include <IOV/StatusPanelData.h>
#include <IOV/Util/OpenSGHelpers.h>
#include <IOV/Util/Exceptions.h>
#include <IOV/Config.h>
#include <IOV/Version.h>
#include <IOV/Plugin/Info.h>

#include "SignalGrabStrategy.h"


using namespace boost::assign;

static const inf::plugin::Info sInfo(
   "com.infiscape.grab", "SignalGrabStrategy",
   list_of(IOV_VERSION_MAJOR)(IOV_VERSION_MINOR)(IOV_VERSION_PATCH)
);
static inf::PluginCreator<inf::GrabStrategy> sPluginCreator(
   boost::bind(&inf::SignalGrabStrategy::create, sInfo)
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

SignalGrabStrategy::SignalGrabStrategy(const inf::plugin::Info& info)
   : GrabStrategy(info)
   , mChooseText("Choose object(s) to grab")
   , mGrabText("Grab object(s)")
   , mReleaseText("Release object(s)")
   , mGrabReleaseToggle(false)
   , mGrabbing(false)
{
   /* Do nothing. */ ;
}

SignalGrabStrategy::~SignalGrabStrategy()
{
   std::for_each(mConnections.begin(), mConnections.end(),
                 boost::bind(&boost::signals::connection::disconnect, _1));
}

GrabStrategyPtr SignalGrabStrategy::init(ViewerPtr viewer,
                                         grab_callback_t grabCallback,
                                         release_callback_t releaseCallback)
{
   mGrabCallback    = grabCallback;
   mReleaseCallback = releaseCallback;

   mGrabSignalData = viewer->getSceneObj()->getSceneData<GrabSignalData>();

   mConnections.push_back(
      mGrabSignalData->asyncRelease.connect(
         boost::bind(&SignalGrabStrategy::objectsReleased, this, _1)
      )
   );

   InterfaceTrader& if_trader = viewer->getUser()->getInterfaceTrader();
   mWandInterface = if_trader.getWandInterface();

   // Configure
   jccl::ConfigElementPtr cfg_elt =
      viewer->getConfiguration().getConfigElement(getElementType());

   if ( cfg_elt )
   {
      // Configure ourself.
      configure(cfg_elt);
   }

   return shared_from_this();
}

void SignalGrabStrategy::setFocus(ViewerPtr viewer, const bool focused)
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

void SignalGrabStrategy::update(ViewerPtr viewer)
{
   // The user wants to choose an object for later grabbing.
   if ( mChooseBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
   {
      mGrabSignalData->choose();
   }
   // If the grab and release operations are activated using the same button
   // sequence, then we have to toggle back and forth between grabbing and
   // releasing objects.
   else if ( mGrabReleaseToggle )
   {
      // Just test the state of the grab command. Testing the state of the
      // release command would give the same result in this case.
      if ( mGrabBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         // If the grab/release toggle has been activated, then we have to
         // look at mGrabbing to figure out which operation to perform.

         // If we are not currently grabbing anything, then this is a grab
         // operation.
         if ( ! mGrabbing )
         {
            grab();
         }
         // If we are currently grabbing something, then this is a release
         // operation.
         else
         {
            release();
         }
      }
   }
   else
   {
      // The user has requested to grab the currently selected objects.
      if ( mGrabBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         grab();
      }
      // The user has requested to release grabbed objects.
      else if ( mReleaseBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         release();
      }
   }
}

std::vector<SceneObjectPtr> SignalGrabStrategy::getGrabbedObjects()
{
   return mGrabbedObjects;
}

void SignalGrabStrategy::configure(jccl::ConfigElementPtr elt)
{
   vprASSERT(elt->getID() == getElementType());

   const unsigned int req_cfg_version(1);

   // Check for correct version of plugin configuration.
   if ( elt->getVersion() < req_cfg_version )
   {
      std::stringstream msg;
      msg << "Configuration of SignalGrabStrategy failed.  Required config "
          << "element version is " << req_cfg_version << ", but element '"
          << elt->getName() << "' is version " << elt->getVersion();
      throw PluginException(msg.str(), IOV_LOCATION);
   }

   const std::string choose_btn_prop("choose_button_nums");
   const std::string grab_btn_prop("grab_button_nums");
   const std::string release_btn_prop("release_button_nums");

   mChooseBtn.configButtons(elt->getProperty<std::string>(choose_btn_prop));
   mGrabBtn.configButtons(elt->getProperty<std::string>(grab_btn_prop));
   mReleaseBtn.configButtons(elt->getProperty<std::string>(release_btn_prop));

   // Determine if grab and release are activated using the same button
   // sequence. This indicates that the grab/release operation is a toggle
   // and must be handled differently than if the two operations are
   // separate.
   std::vector<int> grab_btns(mGrabBtn.getButtons());
   std::vector<int> release_btns(mReleaseBtn.getButtons());
   std::sort(grab_btns.begin(), grab_btns.end());
   std::sort(release_btns.begin(), release_btns.end());

   if ( grab_btns.size() == release_btns.size() &&
        std::equal(grab_btns.begin(), grab_btns.end(), release_btns.begin()) )
   {
      mGrabReleaseToggle = true;
   }
   else
   {
      mGrabReleaseToggle = false;
   }
}

void SignalGrabStrategy::grab()
{
   // Emit the grab signal and get the collection of grabbed objects and
   // the intersection point.
   std::vector<SceneObjectPtr> grabbed_objs;
   gmtl::Point3f isect_pnt;
   mGrabSignalData->grab(grabbed_objs, isect_pnt);

   if ( ! grabbed_objs.empty() )
   {
      mGrabbing = true;
      mGrabbedObjects.insert(mGrabbedObjects.end(), grabbed_objs.begin(),
                             grabbed_objs.end());
      mGrabCallback(grabbed_objs, isect_pnt);
   }
}

void SignalGrabStrategy::release()
{
   // Emit the grab signal and get the collection of released objects.
   std::vector<SceneObjectPtr> released_objs;
   mGrabSignalData->release(released_objs);

   if ( ! released_objs.empty() )
   {
      std::vector<SceneObjectPtr>::iterator o;
      for ( o = released_objs.begin(); o != released_objs.end(); ++o )
      {
         mGrabbedObjects.erase(std::remove(mGrabbedObjects.begin(),
                                        mGrabbedObjects.end(), *o),
                               mGrabbedObjects.end());
      }

      mGrabbing = ! mGrabbedObjects.empty();
      mReleaseCallback(released_objs);
   }
}

void SignalGrabStrategy::
objectsReleased(const std::vector<SceneObjectPtr>& objs)
{
   std::vector<SceneObjectPtr>::const_iterator o;
   for ( o = objs.begin(); o != objs.end(); ++o )
   {
      mGrabbedObjects.erase(std::remove(mGrabbedObjects.begin(),
                                        mGrabbedObjects.end(), *o),
                            mGrabbedObjects.end());
   }

   mGrabbing = ! mGrabbedObjects.empty();
   mReleaseCallback(objs);
}

struct IncValue
{
   int operator()(int v)
   {
      return v + 1;
   }
};

std::vector<int> SignalGrabStrategy::
transformButtonVec(const std::vector<int>& btns)
{
   std::vector<int> result(btns.size());
   IncValue inc;
   std::transform(btns.begin(), btns.end(), result.begin(), inc);
   return result;
}

}
