// Copyright (C) Infiscape Corporation 2005-2007

#ifndef _INF_MODE_HARNESS_PLUGIN_H_
#define _INF_MODE_HARNESS_PLUGIN_H_

#include <IOV/Plugin/PluginConfig.h>

#include <string>
#include <vector>
#include <boost/enable_shared_from_this.hpp>
#include <boost/signal.hpp>

#include <vpr/vpr.h>
#include <vpr/Sync/Mutex.h>

#include <jccl/Config/ConfigElementPtr.h>

#include <IOV/Plugin.h>
#include <IOV/ModeComponentPtr.h>


namespace inf
{

/**
 * The Mode Harness Plug-in provides a way to switch between mutually
 * exclusive "mode components" programatically. Switching is done by emitting
 * a signal via inf::SignalRepository. The component activated in response to
 * a signal is associated with the signal through the configuration of this
 * plug-in.
 *
 * The main difference between this plug-in and the Mode Switch Plug-in is
 * that the act of switching is initiated by a signal that is emitted
 * programatically. The Mode Switch Plug-in relies upon user input to switch.
 * Moreover, the Mode Switch Plug-in uses a cycle to go from one plug-in to
 * the next whereas this plug-in can switch from any component to any other
 * component with no restriction on ordering.
 *
 * Another difference between this plug-in and the Mode Switch Plug-in is
 * that only one mode component is currently allowed to be active at a time.
 * In other words, only one mode component can be configured to be activated
 * in response to some signal being emitted. One mode component can be
 * activated by different signals, however. This limitation of mutually
 * exclusive mode components is largely artificial, and it could be relaxed
 * in the future if necessary.
 *
 * @since 0.23
 */
class ModeHarnessPlugin
   : public inf::Plugin
   , public boost::enable_shared_from_this<ModeHarnessPlugin>
{
protected:
   ModeHarnessPlugin(const inf::plugin::Info& info);

public:
   static inf::PluginPtr create(const inf::plugin::Info& info)
   {
      return inf::PluginPtr(new ModeHarnessPlugin(info));
   }

   virtual ~ModeHarnessPlugin();

   virtual std::string getDescription()
   {
      return std::string("Mode Harness");
   }

   /**
    * Initializes this plug-in. Configuration is performed, and each mode
    * component is loaded, instantiated, and hooked up with a slot in order
    * for the comonent to be activated if and when the signal is emitted.
    *
    * @post All configured mode components are loaded from disk and
    *       instantiated. The slots needed for activating each of the
    *       components are connected to the appropriate signal in the
    *       Signal Repository. \c mNextComponent points to the default mode
    *       component if one was configured.
    *
    * @param viewer The viewer application object.
    *
    * @return A pointer to this plug-in is returned.
    *
    * @see configure()
    * @see inf::SignalRepository
    */
   virtual inf::PluginPtr init(inf::ViewerPtr viewer);

   /**
    * If a component switch is scheduled to occur, the currently active
    * component (if there is one) is deactivated. Then, the component switch
    * occurs. If there is a new component to replace the old, the new
    * component is activated. Finally, inf::ModeComponent::update() is
    * invoked on the currently active component.
    *
    * @post \c mNextComponent is a NULL pointer. \c mCurComponent is the
    *       active component based on the most recently received activation
    *       signal.
    *
    * @param viewer The viewer application object.
    */
   virtual void update(inf::ViewerPtr viewer);

private:
   static std::string getElementType()
   {
      return std::string("iov_mode_harness_plugin");
   }

   /**
    * Configures this plug-in.
    *
    * @pre The type of \p elt matches the value returned by getElementType().
    *
    * @param elt The config element that will be used to configure this
    *            plug-in.
    */
   void configure(jccl::ConfigElementPtr elt);

   void registerModule(vpr::LibraryPtr module);

   void pluginInstantiated(inf::AbstractPluginPtr plugin);

   /**
    * Instantiates and initializes the named component. The ready-to-use mode
    * component is returned to the caller.
    *
    * @post A new instance of the named component is created and initialized
    *       using the given instance of inf::Viewer.
    *
    * @param pluginType The type ID for the plug-in object to instantiate.
    * @param viewer     The VR Juggler application object.
    *
    * @return A fully initialized component that implements the interface
    *         inf::ModeComponent is returned.
    *
    * @throw std::runtime_error is thrown if the named component cannot be
    *        loaded for some reason.
    */
   inf::ModeComponentPtr makeComponent(const std::string& pluginType,
                                       inf::ViewerPtr viewer);

   /**
    * Prepares to switch the current mode component to \p newComponent.
    * This method is a slot for signals emitted by inf::ModeSignalData. The
    * mode switching is performed in a thread-safe manner since this method
    * may be invoked from a thread other than the main application loop.
    *
    * @post \c mNextComponent is assigned \p newComponent.
    *
    * @param newComponent The next component that will be used.
    */
   void prepComponentSwitch(inf::ModeComponentPtr newComponent);

   struct ComponentInfo
   {
      ComponentInfo(const std::string& name_, const std::string& plugin_)
         : name(name_)
         , plugin(plugin_)
      {
         /* Do nothing. */ ;
      }

      std::string name;
      std::string plugin;
   };

   struct SignalDef
   {
      SignalDef(const std::string& name_, const std::string& componentName_)
         : name(name_)
         , componentName(componentName_)
      {
         /* Do nothing. */ ;
      }

      std::string name;
      std::string componentName;
   };

   inf::ViewerPtr mViewer;

   /** @name Component Management */
   //@{
   std::string mDefaultComponentName;

   /**
    * Collection of component information as loaded from the configuration.
    */
   std::vector<ComponentInfo> mComponentInfo;

   /**
    * Collection of signal definitions as loaded from the configuration.
    */
   std::vector<SignalDef> mSignalDefs;

   /** All the components that were instantiated based on the configuration. */
   std::map<std::string, inf::ModeComponentPtr> mComponents;

   /** The currently active component. This may be a NULL pointer. */
   inf::ModeComponentPtr mCurComponent;

   /** The next component to be activated. */
   inf::ModeComponentPtr mNextComponent;
   vpr::Mutex            mNextComponentMutex;   /**< Lock for mNextComponent */
   //@}

   /** Slot connection to the Signal Repository. */
   std::vector<boost::signals::connection> mConnections;
};

}


#endif /* _INF_MODE_HARNESS_PLUGIN_H_ */
