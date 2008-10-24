/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2008  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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
#include "reverseEvents.hpp"
#include "../../dynamics/dynamics.hpp"
#include "../../dynamics/units/units.hpp"
#include "../../dynamics/globals/globEvent.hpp"
#include "../../dynamics/interactions/intEvent.hpp"
#include "../../dynamics/locals/localEvent.hpp"

COPReverseEventsCheck::COPReverseEventsCheck(const DYNAMO::SimData* t1, const XMLNode&):
  COutputPlugin(t1,"ReverseEventsChecker"),
  lReverseEvents(0),
  localeps(0)
{
}

void
COPReverseEventsCheck::initialise()
{
  localeps = -eps * Sim->Dynamics.units().unitTime();
}

void 
COPReverseEventsCheck::eventUpdate(const CIntEvent& eevent, 
				   const C2ParticleData&)
{
  if (eevent.getdt() < localeps) ++lReverseEvents;
}

void 
COPReverseEventsCheck::eventUpdate(const CGlobEvent& eevent, 
				   const CNParticleData&)
{
  if (eevent.getdt() < localeps) ++lReverseEvents;
}

void 
COPReverseEventsCheck::eventUpdate(const CLocalEvent& eevent, 
				   const CNParticleData&)
{
  if (eevent.getdt() < localeps) ++lReverseEvents;
}

void 
COPReverseEventsCheck::eventUpdate(const CSystem&, const CNParticleData&, 
				   const Iflt& dt)
{
  if (dt < localeps) ++lReverseEvents;  
}

void 
COPReverseEventsCheck::output(xmlw::XmlStream& XML)
{
  I_cout() << "Reverse Event Count " << lReverseEvents;

  XML << xmlw::tag("ReverseEvents")
      << xmlw::attr("Count") << lReverseEvents
      << xmlw::endtag("ReverseEvents");

}
