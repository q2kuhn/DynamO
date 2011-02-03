/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "sleep.hpp"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/random/uniform_int.hpp>
#include "../../extcode/xmlwriter.hpp"
#include "../../extcode/xmlParser.h"
#include "../dynamics.hpp"
#include "../units/units.hpp"
#include "../BC/BC.hpp"
#include "../../simulation/particle.hpp"
#include "../species/species.hpp"
#include "../NparticleEventData.hpp"
#include "../ranges/include.hpp"
#include "../liouvillean/NewtonianGravityL.hpp"
#include "../../schedulers/scheduler.hpp"

SSleep::SSleep(const XMLNode& XML, DYNAMO::SimData* tmp): 
  System(tmp),
  _range(NULL)
{
  dt = HUGE_VAL;
  operator<<(XML);
  type = SLEEP;
}

SSleep::SSleep(DYNAMO::SimData* nSim, std::string nName, CRange* r1, double sleepV):
  System(nSim),
  _range(r1),
  _sleepVelocity(sleepV)
{
  sysName = nName;
  type = SLEEP;
}

void
SSleep::initialise(size_t nID)
{
  ID = nID;

  Sim->registerParticleUpdateFunc
    (magnet::function::MakeDelegate(this, &SSleep::particlesUpdated));

  recalculateTime();
}

void
SSleep::operator<<(const XMLNode& XML)
{
  if (strcmp(XML.getAttribute("Type"),"Sleep"))
    M_throw() << "Attempting to load Sleep from a " 
	      << XML.getAttribute("Type") <<  " entry"; 
  
  try {
    sysName = XML.getAttribute("Name");
    
    _sleepVelocity = Sim->dynamics.units().unitVelocity() * 
      boost::lexical_cast<double>(XML.getAttribute("SleepV"));

    _range.set_ptr(CRange::loadClass(XML, Sim));
  }
  catch (boost::bad_lexical_cast &)
    { M_throw() << "Failed a lexical cast in SSleep"; }
}

void 
SSleep::outputXML(xml::XmlStream& XML) const
{
  XML << xml::tag("System")
      << xml::attr("Type") << "Sleep"
      << xml::attr("Name") << sysName
      << xml::attr("SleepV") << _sleepVelocity / Sim->dynamics.units().unitVelocity()
      << _range
      << xml::endtag("System");
}


void 
SSleep::recalculateTime()
{
  dt = (stateChange.empty()) ? HUGE_VAL : -std::numeric_limits<float>::max();
  type = (stateChange.empty()) ? NONE : SLEEP;
}

bool 
SSleep::sleepCondition(const Vector& vel, const Vector& g)
{
  return ((vel.nrm() < _sleepVelocity));
}


void 
SSleep::particlesUpdated(const NEventData& PDat)
{
  BOOST_FOREACH(const PairEventData& pdat, PDat.L2partChanges)
    {
      const Particle& p1 = pdat.particle1_.getParticle();
      const Particle& p2 = pdat.particle2_.getParticle();
      
      //FC = Fixed collider, DP = Dynamic particle, SP = Static
      //particle, ODP = other dynamic particle, OSP = other static
      //particle

      //Check if either particle is static and should not move

       //[O?P-O?P]
      if (!_range->isInRange(p1) && !_range->isInRange(p2)) break;

      //DP-[DP/ODP]
      if (p1.testState(Particle::DYNAMIC) && p2.testState(Particle::DYNAMIC)) break; 
 
      //SP-[FC/SP/OSP]
#ifdef DYNAMO_DEBUG
      if (!p1.testState(Particle::DYNAMIC) && !p2.testState(Particle::DYNAMIC))
          M_throw() << "Static particles colliding!";
#endif

      //We're guaranteed by the previous tests that
      //p1.testState(Particle::DYNAMIC) != p2.testState(Particle::DYNAMIC)
      //and that at least one particle is in the range

      //Sort the particles
      const Particle& dp = p1.testState(Particle::DYNAMIC) ? p1 : p2;
      const Particle& sp = p1.testState(Particle::DYNAMIC) ? p2 : p1;

      //Other variables
      Vector g(static_cast<const LNewtonianGravity&>(Sim->dynamics.getLiouvillean()).getGravityVector());

      if (!_range->isInRange(sp))  //DP-FC
	{
	  //If the dynamic particle is going to fall asleep, mark its impulse as 0
	  if (sleepCondition(dp.getVelocity(), g))
	    stateChange[dp.getID()] = Vector(0,0,0);
	  break;
	}

      if (!_range->isInRange(dp)) break;

      //Final case
      //DP-SP
      //sp is in the range (a wakeable particle)

      //If the static particle sleeps
      if ((sleepCondition(0.1 * sp.getVelocity(), g)))
	{
	  stateChange[sp.getID()] = Vector(0,0,0);
	  stateChange[dp.getID()] = -sp.getVelocity() * Sim->dynamics.getSpecies(sp).getMass();

	  if ((sleepCondition(dp.getVelocity() + stateChange[dp.getID()] 
	  		      / Sim->dynamics.getSpecies(dp).getMass(), g)))
	    stateChange[dp.getID()] = Vector(0,0,0);

	  break;
	}
	else
	  {
	      stateChange[sp.getID()] = Vector(1,1,1);
	  }
    }

  if (!stateChange.empty())
    {
      recalculateTime();
      Sim->ptrScheduler->rebuildSystemEvents();
    }
}

void 
SSleep::runEvent() const
{
  double locdt = 0;

  dt = HUGE_VAL;
    
#ifdef DYNAMO_DEBUG 
  if (boost::math::isnan(locdt))
    M_throw() << "A NAN system event time has been found";
#endif
  
  Sim->dSysTime += locdt;
  Sim->ptrScheduler->stream(locdt);
  
  //dynamics must be updated first
  Sim->dynamics.stream(locdt);

  //++Sim->eventCount;

  NEventData SDat;

  typedef std::map<size_t, Vector>::value_type locPair;
  BOOST_FOREACH(const locPair& p, stateChange)
    {
      const Particle& part = Sim->particleList[p.first];
      Sim->dynamics.getLiouvillean().updateParticle(part);
      
#ifdef DYNAMO_DEBUG 
      if (stateChange.find(part.getID()) == stateChange.end())
	M_throw() << "Running an event for a particle with no state change!";
#endif

      EEventType type = WAKEUP;
      if ((stateChange[part.getID()][0] == 0) 
	  && (stateChange[part.getID()][1] == 0) 
	  && (stateChange[part.getID()][2] == 0))
	{
	  if (part.testState(Particle::DYNAMIC)) 
	    type = SLEEP;
	  else
	    type = RESLEEP;
	}
      else
	{
	  if (part.testState(Particle::DYNAMIC)) 
	    type = CORRECT;
	  else
	    type = WAKEUP;
	}

      ParticleEventData EDat(part, Sim->dynamics.getSpecies(part), type);

      switch (type)
	{
	case SLEEP:
	  const_cast<Particle&>(part).clearState(Particle::DYNAMIC);
	case RESLEEP:
	  const_cast<Particle&>(part).getVelocity() = Vector(0,0,0);
  	  break;
	case CORRECT:
	  const_cast<Particle&>(part).getVelocity() += stateChange[part.getID()] / EDat.getSpecies().getMass();
	case WAKEUP:
	  const_cast<Particle&>(part).setState(Particle::DYNAMIC);
	  break;
	default:
	  M_throw() << "Bad event type!";
	}
	  
      EDat.setDeltaKE(0.5 * EDat.getSpecies().getMass()
		      * (part.getVelocity().nrm2() 
			 - EDat.getOldVel().nrm2()));

      SDat.L1partChanges.push_back(EDat);
    }

  Sim->signalParticleUpdate(SDat);

  BOOST_FOREACH(const ParticleEventData& PDat, SDat.L1partChanges)
    Sim->ptrScheduler->fullUpdate(PDat.getParticle());
  
  stateChange.clear();
  
  locdt += Sim->freestreamAcc;

  Sim->freestreamAcc = 0;
  
  BOOST_FOREACH(magnet::ClonePtr<OutputPlugin>& Ptr, Sim->outputPlugins)
    Ptr->eventUpdate(*this, SDat, locdt); 
}
