// vrkit is (C) Copyright 2005-2007
//    by Allen Bierbaum, Aron Bierbuam, Patrick Hartling, and Daniel Shipton
//
// This file is part of vrkit.
//
// vrkit is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// vrkit is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
// more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <vpr/System.h>
#include <IOV/Util/Exceptions.h>


namespace inf
{

Exception::Exception(const std::string& desc, const std::string& location)
   throw ()
   : std::runtime_error(desc)
   , mDescription(desc)
   , mLocation(location)
{
   mStackTrace = vpr::System::getCallStack();
}

Exception::~Exception() throw ()
{
   ;
}

const char* Exception::what() const throw()
{
   m_full_desc = getFullDescription();
   return m_full_desc.c_str();
}

std::string Exception::getExceptionName() const
{
   return std::string("inf::Exception");
}

std::string Exception::getDescription() const
{
   return mDescription;
}

void Exception::setDescription(const std::string& desc)
{
   mDescription = desc;
}

std::string Exception::getExtendedDescription() const
{
   return this->getExceptionName() + std::string(": ") + getDescription();
}

std::string Exception::getFullDescription() const
{
   return getExtendedDescription() + std::string("  ") + mLocation +
             std::string("\n") + mStackTrace;
}

PluginException::PluginException(const std::string& msg,
                                 const std::string& location) throw ()
   : Exception(msg, location)
{
   /* Do nothing. */ ;
}

PluginException::~PluginException() throw ()
{
   /* Do nothign. */ ;
}

PluginLoadException::PluginLoadException(const std::string& msg,
                                         const std::string& location) throw ()
   : PluginException(msg, location)
{
   /* Do nothing. */ ;
}

PluginLoadException::~PluginLoadException() throw ()
{
   /* Do nothing. */ ;
}

NoSuchPluginException::NoSuchPluginException(const std::string& msg,
                                             const std::string& location)
   throw ()
   : PluginException(msg, location)
{
   /* Do nothing. */ ;
}

NoSuchPluginException::~NoSuchPluginException() throw ()
{
   /* Do nothing. */ ;
}

PluginInterfaceException::PluginInterfaceException(const std::string& msg,
                                                   const std::string& location)
   throw ()
   : PluginException(msg, location)
{
   /* Do nothing. */ ;
}

PluginInterfaceException::~PluginInterfaceException() throw ()
{
   /* Do nothing. */ ;
}

}
