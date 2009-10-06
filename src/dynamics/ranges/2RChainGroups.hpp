/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2009  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef C2RChainGroups_H
#define C2RChainGroups_H

#include "1range.hpp"
#include "2range.hpp"
#include "../../datatypes/pluginpointer.hpp"

class C2RChainGroups:public C2Range
{
public:
  C2RChainGroups(const XMLNode&, const DYNAMO::SimData*);

  C2RChainGroups(size_t, size_t, size_t);
  
  virtual C2Range* Clone() const 
  { return new C2RChainGroups(*this); };

  virtual bool isInRange(const CParticle&, const CParticle&) const;
  
  virtual void operator<<(const XMLNode&);
  
protected:
  virtual void outputXML(xmlw::XmlStream&) const;

  size_t range1;
  size_t range2;
  size_t length;
};

#endif
