// Copyright (C) Infiscape Corporation 2005-2006

#include <IOV/Config.h>

#include <gmtl/MatrixOps.h>
#include <gmtl/External/OpenSGConvert.h>

#include <IOV/Plugin/PluginConfig.h>
#include <IOV/PluginCreator.h>
#include <IOV/User.h>
#include <IOV/Viewer.h>
#include <IOV/WandInterface.h>

#include "SlideMoveStrategy.h"


static inf::PluginCreator<inf::MoveStrategy> sPluginCreator(
   &inf::SlideMoveStrategy::create, "Slide Move Strategy Plug-in"
);

extern "C"
{

/** @name Plug-in Entry Points */
//@{
IOV_PLUGIN_API(void) getPluginInterfaceVersion(vpr::Uint32& majorVer,
                                               vpr::Uint32& minorVer)
{
   majorVer = INF_MOVE_STRATEGY_PLUGIN_API_MAJOR;
   minorVer = INF_MOVE_STRATEGY_PLUGIN_API_MINOR;
}

IOV_PLUGIN_API(inf::PluginCreatorBase*) getMoveStrategyCreator()
{
   return &sPluginCreator;
}
//@}

}

namespace inf
{

void SlideMoveStrategy::init(inf::ViewerPtr)
{
}

void SlideMoveStrategy::objectGrabbed(inf::ViewerPtr,
                                      OSG::TransformNodePtr,
                                      const gmtl::Matrix44f&)
{
}

void SlideMoveStrategy::objectReleased(inf::ViewerPtr viewer,
                                       OSG::TransformNodePtr obj)
{
   mTransValue = 0.0f;
}

gmtl::Matrix44f
SlideMoveStrategy::computeMove(inf::ViewerPtr viewer,
                               OSG::TransformNodePtr obj,
                               const gmtl::Matrix44f& vp_M_wand)
{
   int mInOutAnalogNum = 0;

   WandInterfacePtr wand_if = viewer->getUser()->getInterfaceTrader().getWandInterface();

   float analog_value = wand_if->getAnalog(mInOutAnalogNum)->getData();
   //std::cout << "Analog Value: " << analog_value << std::endl;

   // Rescale [0,1] to [-1,1]
   float in_out_val = (analog_value*2.0)-1.0f;

   //std::cout << "Analog values: " << analog0_val << ", " << analog1_val << std::endl;
   const float eps_limit(0.1);

   if(fabs(in_out_val) < eps_limit)
   { in_out_val = 0.0f; }

   // If we don't have an analog configured, then set in-out to 0
   if(-1 == mInOutAnalogNum)
   { in_out_val = 0.0f; }

   //std::cout << "In out: " << in_out_val << std::endl;

   gmtl::Matrix44f new_obj_pos;
   const float in_out_scale = 0.20f;
   float trans_val = -in_out_val*in_out_scale;
   mTransValue += trans_val;

   //std::cout << "trans val: " << trans_val << std::endl;
   //std::cout << "mTransValue: " << mTransValue << std::endl;


   // pobj_M_vp is the inverse of the object in view platform space.
   OSG::Matrix world_xform;
   obj.node()->getParent()->getToWorld(world_xform);
   gmtl::Matrix44f pobj_M_vp;
   gmtl::set(pobj_M_vp, world_xform);
   gmtl::invert(pobj_M_vp);

   // XXX: Move strategy
   const gmtl::Matrix44f pobj_M_wand = pobj_M_vp * vp_M_wand;
   gmtl::Matrix44f wand_M_pobj;
   gmtl::invert(wand_M_pobj, pobj_M_wand);

   // Accumulate translation matrix.
   gmtl::Matrix44f delta_trans_mat =
      gmtl::makeTrans<gmtl::Matrix44f>(gmtl::Vec3f(0,0,mTransValue));
   gmtl::Matrix44f pobj_trans_mat = pobj_M_wand * delta_trans_mat;

   return pobj_M_wand * delta_trans_mat * wand_M_pobj;
}

}
