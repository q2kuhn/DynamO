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

#ifndef COPEventEffects_H
#define COPEventEffects_H

#include "../outputplugin.hpp"
#include "../../dynamics/eventtypes.hpp"
#include <map>
#include <vector>
#include "../eventtypetracking.hpp"

using namespace EventTypeTracking;

class CParticle;

class COPEventEffects: public COutputPlugin
{
private:
  
public:
  COPEventEffects(const DYNAMO::SimData*, const XMLNode&);
  ~COPEventEffects();

  virtual void initialise();

  virtual void eventUpdate(const CIntEvent&, const C2ParticleData&);

  virtual void eventUpdate(const CGlobEvent&, const CNParticleData&);

  virtual void eventUpdate(const CLocalEvent&, const CNParticleData&);

  virtual void eventUpdate(const CSystem&, const CNParticleData&, const Iflt&);

  void output(xmlw::XmlStream &);

  virtual COutputPlugin *Clone() const { return new COPEventEffects(*this); };

  //This is fine to replica exchange as the interaction, global and system lookups are done using id's
  virtual void changeSystem(COutputPlugin* plug) { std::swap(Sim, static_cast<COPEventEffects*>(plug)->Sim); }
  
 protected:
  typedef std::pair<classKey, EEventType> eventKey;

  void newEvent(const EEventType&, const classKey&, const Iflt&, const Vector &);
  
  struct counterData
  {
    counterData():count(0), energyLoss(0), momentumChange(0,0,0) {}
    unsigned long count;
    Iflt energyLoss;
    Vector  momentumChange;
  };
  
  std::map<eventKey, counterData> counters;
};

#endif
