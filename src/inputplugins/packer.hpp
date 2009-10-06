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

#pragma once
#include "../base/is_base.hpp"
#include "../datatypes/vector.hpp"
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

using namespace std;
using namespace boost;
namespace po = boost::program_options;

class CUCell;

class CIPPacker: public DYNAMO::SimBase
{
 public:
  CIPPacker(po::variables_map&, DYNAMO::SimData* tmp);

  void initialise();

  static po::options_description getOptions();

  void processOptions();

 protected:
  CVector<long> getCells();
  Vector  getNormalisedCellDimensions();
  Vector  getRandVelVec();
  CUCell* standardPackingHelper(CUCell* );

  po::variables_map& vm;
};
