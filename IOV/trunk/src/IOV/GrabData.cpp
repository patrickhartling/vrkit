// Copyright (C) Infiscape Corporation 2005-2006

#include <IOV/GrabData.h>
#include <algorithm>

namespace inf
{

const vpr::GUID GrabData::type_guid("C2FD2FE0-B7FD-11D9-8DA1-000D933B5E6A");

GrabData::~GrabData()
{
   /* Do nothing. */ ;
}

GrabData::GrabData()
{
   /* Do nothing. */ ;
}

boost::signals::connection GrabData::connectToAdd(signal_t::slot_type slot)
{
   return mAddSignal.connect(slot);
}

boost::signals::connection GrabData::connectToRemove(signal_t::slot_type slot)
{
   return mRemoveSignal.connect(slot);
}

void GrabData::addObject(SceneObjectPtr obj)
{
   mObjects.push_back(obj);
   mAddSignal(obj);
}

void GrabData::removeObject(SceneObjectPtr obj)
{
   object_list_t::iterator found
      = std::find(mObjects.begin(), mObjects.end(), obj);

   if (mObjects.end() != found)
   {
      mObjects.erase(found);
      mRemoveSignal(obj);
   }
}

}
