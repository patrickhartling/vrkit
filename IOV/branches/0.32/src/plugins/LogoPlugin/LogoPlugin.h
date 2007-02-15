// Copyright (C) Infiscape Corporation 2005-2007

#ifndef _INF_LOGO_PLUGIN_H_
#define _INF_LOGO_PLUGIN_H_

#include <IOV/Plugin/PluginConfig.h>

#include <string>
#include <boost/enable_shared_from_this.hpp>
#include <gmtl/Matrix.h>
#include <vector>

#include <IOV/Plugin.h>
#include <IOV/WandInterfacePtr.h>

namespace inf
{

/** Plugin for loading logos into the scene.
 *
 */
class LogoPlugin
   : public inf::Plugin, public boost::enable_shared_from_this<LogoPlugin>
{
protected:
   LogoPlugin()
   {;}

public:
   static PluginPtr create();

   virtual ~LogoPlugin()
   {;}

   virtual std::string getDescription()
   {
      return std::string("Logo Plugin");
   }

   virtual PluginPtr init(inf::ViewerPtr viewer);

   virtual void update(inf::ViewerPtr viewer);

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

protected:
   struct Logo
   {
      std::string           name;
      OSG::TransformNodePtr xformNode;
   };

private:
   std::vector<Logo>    mLogos;  /**< List of loaded logos. */


};


} // namespace inf

#endif
