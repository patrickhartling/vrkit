// Copyright (C) Infiscape Corporation 2005-2006

#include <algorithm>

#include <gmtl/Generate.h>
#include <gmtl/External/OpenSGConvert.h>

#include <vpr/vpr.h>
#include <jccl/Config/ConfigElement.h>

#include <IOV/PluginCreator.h>
#include <IOV/StatusPanel.h>
#include <IOV/StatusPanelPlugin.h>
#include <IOV/Status.h>
#include <IOV/User.h>
#include <IOV/Viewer.h>

#include "Grid.h"
#include "GridPlugin.h"


static inf::PluginCreator<inf::Plugin> sPluginCreator(&inf::GridPlugin::create,
                                                      "Grid Plug-in");

extern "C"
{

/** @name Plug-in Entry Points */
//@{
IOV_PLUGIN_API(void) getPluginInterfaceVersion(vpr::Uint32& majorVer,
                                               vpr::Uint32& minorVer)
{
   majorVer = INF_PLUGIN_API_MAJOR;
   minorVer = INF_PLUGIN_API_MINOR;
}

IOV_PLUGIN_API(inf::PluginCreatorBase*) getCreator()
{
   return &sPluginCreator;
}
//@}
}

namespace inf
{

GridPlugin::GridPlugin()
   : mSelectedGridIndex(-1)
   , mGridsVisible(false)
   , mAnalogNum(-1)
   , mForwardVal(1.0f)
   , mActivateText("Activate/Deactivate Grids")
   , mCycleText("Cycle Grid Selection")
   , mHideText("Show/Hide Selected Grid")
   , mResetText("Reset Selected Grid Position")
{
   /* Do nothing. */ ;
}

PluginPtr GridPlugin::init(inf::ViewerPtr viewer)
{
   mWandInterface = viewer->getUser()->getInterfaceTrader().getWandInterface();

   jccl::ConfigElementPtr cfg_elt =
      viewer->getConfiguration().getConfigElement(getElementType());

   if ( cfg_elt )
   {
      configure(cfg_elt);
   }

   std::vector<inf::GridPtr>::iterator g;
   for ( g = mGrids.begin(); g != mGrids.end(); ++g )
   {
      OSG::GroupNodePtr decorator_root =
         viewer->getSceneObj()->getDecoratorRoot();
      OSG::beginEditCP(decorator_root.node(), OSG::Node::ChildrenFieldMask);
         decorator_root.node()->addChild((*g)->getRoot());
      OSG::endEditCP(decorator_root.node(), OSG::Node::ChildrenFieldMask);
      (*g)->setVisible(mGridsVisible);
   }

   return shared_from_this();
}

void GridPlugin::update(inf::ViewerPtr viewer)
{
   if ( isFocused() )
   {
      // Change the visiblity of all the grids of the activate/deactivate
      // button is toggled on.
      if ( mActivateBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         mGridsVisible = ! mGridsVisible;
         std::vector<inf::GridPtr>::iterator g;
         for ( g = mGrids.begin(); g != mGrids.end(); ++g )
         {
            (*g)->setVisible(mGridsVisible);
         }
      }

      // Change the grid selection state if the cycle button has toggled on.
      if ( mCycleBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
      {
         if ( mSelectedGridIndex != -1 )
         {
            mGrids[mSelectedGridIndex]->setSelected(false);
         }

         // If there are grids defined, move mSelectedGridIndex to the next one
         // (using a circular increment) and tell that grid that it is now
         // selected.
         if ( ! mGrids.empty() )
         {
            mSelectedGridIndex = (mSelectedGridIndex + 1) % mGrids.size();
            mGrids[mSelectedGridIndex]->setSelected(true);

            IOV_STATUS << "Grid #" << mSelectedGridIndex
                       << " (" << mGrids[mSelectedGridIndex]->getName()
                       << ") selected" << std::endl;

            // XXX: Should the visible state change when the grid is selected?
            // If it doesn't, then there is no visual cue to the user that the
            // grid has been selected. On the other hand, if the user is just
            // trying to cycle to some other grid (visible or not), it may not
            // be desirable to make invisible grids along the way visible.
//            mGrids[mSelectedGridIndex]->setVisible(true);
         }
      }

      // If the user has selected a grid, check to see if any manipulations are
      // being performed on it.
      if ( mSelectedGridIndex != -1 )
      {
         float in_out_val(0.0f);

         // If an analog device index is configured, check to see if analog
         // data is available that would result in the selected grid being
         // slid along its Z-axis.
         if ( -1 != mAnalogNum )
         {
            gadget::AnalogInterface& analog_dev =
               mWandInterface->getAnalog(mAnalogNum);

            // Only check for analog data if the indicated analog device is not
            // stupefied.
            if ( ! analog_dev->isStupefied() )
            {
               const float to_meters(viewer->getDrawScaleFactor());
               const float analog_value(analog_dev->getData());

               // Rescale [0,1] to [-1,1].
               in_out_val = analog_value * 2.0 - 1.0f;

               const float eps_limit(0.1f * to_meters);

               if ( gmtl::Math::abs(in_out_val) < eps_limit )
               {
                  in_out_val = 0.0f;
               }

               // The above code treats the forward value as 1.0. If the
               // forward value is 1.0, then we need to invert the sliding
               // direction.
               if ( mForwardVal == 0.0f )
               {
                  in_out_val = -in_out_val;
               }
            }
         }

         // If in_out_val is not zero, then slide the selected grid along its
         // Z-axis based on in_out_val.
         if ( in_out_val != 0.0f )
         {
            const float in_out_scale(0.20f);
            const float trans_val(-in_out_val * in_out_scale);

            const gmtl::Matrix44f delta_trans_mat =
               gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0.0f, 0.0f,
                                                            trans_val));

            inf::GridPtr grid = mGrids[mSelectedGridIndex];
            gmtl::Matrix44f grid_xform;
            gmtl::set(grid_xform, grid->getCurrentXform());
            grid->move(grid_xform * delta_trans_mat);
         }
         // If no sliding is being performed and the reset button is on, then
         // reset the position of the selected grid.
         else if ( mResetBtn.test(mWandInterface, gadget::Digital::ON) )
         {
            mGrids[mSelectedGridIndex]->reset();
         }

         // If the show/hide button has been toggled on, toggle the visible
         // state of the selected grid.
         if ( mHideBtn.test(mWandInterface, gadget::Digital::TOGGLE_ON) )
         {
            mGrids[mSelectedGridIndex]->setVisible(
               ! mGrids[mSelectedGridIndex]->isVisible()
            );

            // Determine if at least one grid is visible now. If there is one
            // such grid, then mGridsVisible should remain true. Otherwise,
            // it should be set to false so that all the grids can be toggled
            // on at once.
            bool grid_visible(false);
            std::vector<inf::GridPtr>::iterator g;
            for ( g = mGrids.begin(); g != mGrids.end(); ++g )
            {
               if ( (*g)->isVisible() )
               {
                  grid_visible = true;
                  break;
               }
            }

            mGridsVisible = grid_visible;
         }
      }
   }
}

void GridPlugin::focusChanged(inf::ViewerPtr viewer)
{
   inf::ScenePtr scene = viewer->getSceneObj();
   StatusPanelPluginDataPtr status_panel_data =
      scene->getSceneData<StatusPanelPluginData>();

   if ( ! isFocused() )
   {
      // If a grid is selected, deselect it. We allow grids to remain visible
      // when this plug-in is not focused, but there seems to be little value
      // to having a grid remain selected when this plug-in is not able to
      // receive user input to manipulate the selected grid.
      if ( mSelectedGridIndex != -1 )
      {
         mGrids[mSelectedGridIndex]->setSelected(false);
         mSelectedGridIndex = -1;
      }

      if ( status_panel_data->mStatusPanelPlugin )
      {
         inf::StatusPanel& panel =
            status_panel_data->mStatusPanelPlugin->getPanel();

         if ( mActivateBtn.isConfigured() )
         {
            // The button numbers in mActivateBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(
               transformButtonVec(mActivateBtn.getButtons())
            );
            panel.removeControlText(btns, mActivateText);
         }

         if ( mCycleBtn.isConfigured() )
         {
            // The button numbers in mCycleBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(transformButtonVec(mCycleBtn.getButtons()));
            panel.removeControlText(btns, mCycleText);
         }

         if ( mHideBtn.isConfigured() )
         {
            // The button numbers in mHideBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(transformButtonVec(mHideBtn.getButtons()));
            panel.removeControlText(btns, mHideText);
         }

         if ( mResetBtn.isConfigured() )
         {
            // The button numbers in mResetBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(transformButtonVec(mResetBtn.getButtons()));
            panel.removeControlText(btns, mResetText);
         }
      }
   }
   else
   {
      if ( status_panel_data->mStatusPanelPlugin )
      {
         inf::StatusPanel& panel =
            status_panel_data->mStatusPanelPlugin->getPanel();

         if ( mActivateBtn.isConfigured() )
         {
            // The button numbers in mActivateBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(
               transformButtonVec(mActivateBtn.getButtons())
            );

            if ( ! panel.hasControlText(btns, mActivateText) )
            {
               panel.addControlText(btns, mActivateText);
            }
         }

         if ( mCycleBtn.isConfigured() )
         {
            // The button numbers in mCycleBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(transformButtonVec(mCycleBtn.getButtons()));

            if ( ! panel.hasControlText(btns, mCycleText) )
            {
               panel.addControlText(btns, mCycleText);
            }
         }

         if ( mHideBtn.isConfigured() )
         {
            // The button numbers in mHideBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(transformButtonVec(mHideBtn.getButtons()));

            if ( ! panel.hasControlText(btns, mHideText) )
            {
               panel.addControlText(btns, mHideText);
            }
         }

         if ( mResetBtn.isConfigured() )
         {
            // The button numbers in mResetBtn are zero-based, but we would
            // like them to be one-based in the status panel display.
            std::vector<int> btns(transformButtonVec(mResetBtn.getButtons()));

            if ( ! panel.hasControlText(btns, mResetText) )
            {
               panel.addControlText(btns, mResetText);
            }
         }
      }
   }
}

void GridPlugin::configure(jccl::ConfigElementPtr elt)
{
   vprASSERT(elt->getID() == getElementType());

   const unsigned int req_cfg_version(1);

   // Check for correct version of plugin configuration.
   if ( elt->getVersion() < req_cfg_version )
   {
      std::ostringstream msg;
      msg << "Configuration of GridPlugin failed.  Required config "
          << "element version is " << req_cfg_version << ", but element '"
          << elt->getName() << "' is version " << elt->getVersion();
      throw PluginException(msg.str(), IOV_LOCATION);
   }

   const std::string activate_btn_prop("activate_button_nums");
   const std::string cycle_btn_prop("cycle_button_nums");
   const std::string hide_btn_prop("hide_button_nums");
   const std::string reset_btn_prop("reset_button_nums");
   const std::string grids_prop("grids");

   mActivateBtn.configButtons(elt->getProperty<std::string>(activate_btn_prop));
   mCycleBtn.configButtons(elt->getProperty<std::string>(cycle_btn_prop));
   mHideBtn.configButtons(elt->getProperty<std::string>(hide_btn_prop));
   mResetBtn.configButtons(elt->getProperty<std::string>(reset_btn_prop));

   const unsigned int num_grids(elt->getNum(grids_prop));

   for ( unsigned int i = 0; i < num_grids; ++i )
   {
      try
      {
         mGrids.push_back(
            inf::Grid::create()->init(
               elt->getProperty<jccl::ConfigElementPtr>(grids_prop, i)
            )
         );
      }
      catch (inf::Exception& ex)
      {
         std::cerr << "Failed to configure grid #" << i << ":\n" << ex.what()
                   << std::endl;
      }
   }
}

struct IncValue
{
   int operator()(int v)
   {
      return v + 1;
   }
};

std::vector<int> GridPlugin::transformButtonVec(const std::vector<int>& btns)
{
   std::vector<int> result(btns.size());
   IncValue inc;
   std::transform(btns.begin(), btns.end(), result.begin(), inc);
   return result;
}

}
