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

#include "include.hpp"
#include "../../extcode/xmlwriter.hpp"
#include "../../extcode/xmlParser.h"

CSCEntry::CSCEntry(DYNAMO::SimData* const tmp, const char *aName):
  SimBase(tmp, aName, IC_cyan)
{

}

CSCEntry* 
CSCEntry::getClass(const XMLNode& XML, DYNAMO::SimData* const Sim)
{
  if (!strcmp(XML.getAttribute("Type"),"NeighbourList"))
    return new CSCENBList(XML, Sim);      
  else 
    D_throw() << "Unknown type of ComplexSchedulerEntry `" 
	      << XML.getAttribute("Type") << "`encountered";
}

xmlw::XmlStream& operator<<(xmlw::XmlStream& XML, 
			    const CSCEntry& g)
{
  g.outputXML(XML);
  return XML;
}

bool 
CSCEntry::isApplicable(const CParticle& part) const
{
  return range->isInRange(part);
}
