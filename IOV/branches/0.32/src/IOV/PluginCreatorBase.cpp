// Copyright (C) Infiscape Corporation 2005-2006

#include <sstream>

#include <IOV/PluginCreatorBase.h>


namespace inf
{

PluginCreatorBase::PluginCreatorBase(const std::string& pluginName,
                                     const vpr::Uint32 pluginMajorVer,
                                     const vpr::Uint32 pluginMinorVer,
                                     const vpr::Uint32 pluginPatchVer)
   : mPluginName(pluginName)
   , mPluginMajorVer(pluginMajorVer)
   , mPluginMinorVer(pluginMinorVer)
   , mPluginPatchVer(pluginPatchVer)
{
   std::stringstream str_stream;
   str_stream << mPluginMajorVer << "." << mPluginMinorVer << "."
              << mPluginPatchVer;
   mPluginVersionStr = str_stream.str();
}

PluginCreatorBase::~PluginCreatorBase()
{
}

}
