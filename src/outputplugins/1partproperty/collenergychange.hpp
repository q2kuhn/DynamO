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

#ifndef COPCollEnergyChange_H
#define COPCollEnergyChange_H

#include "1partproperty.hpp"
#include <vector>
#include "../../datatypes/histogram.hpp"

class COPCollEnergyChange: public COP1PP
{
 public:
  COPCollEnergyChange(const DYNAMO::SimData*, const XMLNode&);

  void A1ParticleChange(const C1ParticleData&);

  void A2ParticleChange(const C2ParticleData&);

  void stream(const Iflt&) {}

  void output(xmlw::XmlStream &); 

  void periodicOutput() {}

  virtual void initialise();

  virtual COutputPlugin *Clone() const 
  { return new COPCollEnergyChange(*this); }

  void operator<<(const XMLNode&);

 protected:  
  Iflt binWidth; 

  std::vector<C1DHistogram> data;
};

#endif
