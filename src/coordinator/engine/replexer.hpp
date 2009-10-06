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
/*! \file replexer.hpp
 * Holds the definition of the CEReplexer class.
 */

#ifndef CEReplexer_H
#define CEReplexer_H

#include "engine.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

/*! \brief The Replica Exchange/Parallel Tempering Engine.
 *
 * This Engine is quite complex, it runs several simulations at
 * different state points simultaneously. These are halted
 * periodically and then the configurations of the particles positions
 * are swapped along with a rescaling of the particles velocities.
 *
 * This class uses the CThreadPool to parallelise the running of the
 * simulations.
 */
class CEReplexer: public CEngine
{
public:
  /*! \brief The only constructor.
   *
   * \param vm The parsed command line options held by the CCoordinator.
   * \param tp The CThreadPool for this instance of dynarun.
   */
  CEReplexer(const boost::program_options::variables_map& vm, 
	     CThreadPool& tp);
  
  /*! \brief A trivial virtual destructor. 
   */
  virtual ~CEReplexer() {}
  
  /*! \brief Print the replica exchange status of the simulations.
   *
   * This outputs information on replica exchanges, acceptance ratio
   * and system diffusion rates to the screen so the system can be
   * inspected to see if its been run long enough.
   */
  virtual void printStatus();
  
  /*! \brief Run the CSimulation's and periodically attempt a replica echange.
   */
  virtual void runSimulation();

  /*! \brief Perform multiple initialisations of CSimulation's and
   * initialise the replica exchange data.
   */
  virtual void initialisation();
  
  /*! \brief Flag every CSimulation to end now and set the main loop
   * to terminate.
   *
   * This is called by an interrupt in a threaded environment. It may
   * be completely unsafe.
   */
  virtual void forceShutdown();

  /*! \brief Set the system to output its current data at the next
   * available instance.
   * 
   * This is called by an interrupt in a threaded environment. It may
   * be completely unsafe.
   */
  virtual void peekData();

  /*! \brief No finalisation is required in this engine.
   */
  virtual void finaliseRun() {}
  
  /*! \brief Return the options for the CEReplexer Engine.
   */
  static void getOptions(boost::program_options::options_description&);

  /*! \brief Output the data of the CSimulation's and also output
   * statistics on the replica exchange.
   */
  virtual void outputData();
  
  /*! \brief Output the CSimulation's configurations.
   *
   * The simulations are outputted with some sequential numbering.
   */
  virtual void outputConfigs();

protected:

  //! \brief Type of replica exchange moves to attempt
  typedef enum {
    NoSwapping = 0, /*!< Disable replica exchange moves for testing.*/
    AlternatingSequence = 1, /*!< Attempt to swap neighbouring pairs only.*/
    SinglePair = 2, /*!< Pick a random sim to attempt to swap with its
                       neighbour*/
    RandomPairs = 3, /*!< For 5*No. of CSimulations, pick two random
                        CSimulations and attempt to swap them*/
    RandomSelection = 4 /*!< Pick randomly between RandomPairs and
                           AlternatingSequence.*/
  } Replex_Mode_Type;

  /*! \brief A structure to hold replica exchange data on a single
   * temperature point.
   * 
   * This holds the detail about a temperature and the current
   * simulation id number occupying this temperature.
   */
  struct simData
  {
    /*! \brief A simple RAII constructor.
     */
    explicit simData(int ID, Iflt rT):
      simID(ID), swaps(0), attempts(0), upSims(0), downSims(0),
      realTemperature(rT)
    {}

    /*! \brief compares simData by their contained simulation ID's
     *
     * This is only used to compare two simulation points at the same
     * temperature when sorting the boxes by temperature.
     */
    bool operator<(const simData& sdat) const
    {return simID < sdat.simID; }
    
    /*! \brief The current CSimulation's id number*/
    int simID;
    /*! \brief The number of swaps carried out on this box.*/
    size_t swaps;
    /*! \brief The number of attempted swaps carried out on this box.*/
    size_t attempts;
    /*! \brief The number of times a CSimulation instance that last visited
        the coldest temperature was found in this box. */
    size_t upSims;
    /*! \brief The number of times a CSimulation instance that last visited
        the hottest temperature was found in this box. */
    size_t downSims;
    /*! \brief The temperature of this simulation point */
    Iflt realTemperature;
  };

  typedef std::pair<Iflt, simData> replexPair;

  /*! \brief The array of CSimulations being run.
   */
  boost::scoped_array<CSimulation> Simulations;
  
  /*! \brief The system time to end the CSimulations at
   */
  Iflt replicaEndTime;  

  /*! \brief What type of replica exchange moves to attempt.
   */
  Replex_Mode_Type ReplexMode;
  
  /*! \brief A sorted list in temperatures with each corresponding simData.
   */
  std::vector<replexPair> temperatureList;
  
  /*! \brief Holds the current direction/which temperature extreme the
   * simulation last visited.
   */
  std::vector<int> SimDirection;
  
  /*! \brief Just a marker set once a CSimulation is making a round
   * trip in temperatures from high to low and vice versa.
   */
  std::vector<char> roundtrip;
  
  /*! \brief Total number of replica exchange phases attempted. 
   *
   * The number of replica exchanges attempted varies with some types
   * of replica exhange moves.
   */
  size_t replexSwapCalls;
  
  /*! \brief The number of systems that have made it from the highest
   * temperature to the coldest to the highest. And cold-hot-cold trips too.
   */
  size_t round_trips;

  /*! \brief The start time of the simulations.
   */
  boost::posix_time::ptime start_Time;

  /*! \brief The end time of the simulations.
   */
  boost::posix_time::ptime end_Time;

  /*! \brief A variable used by the AlternatingSequence
   * Replex_Mode_Type to indicate which set of pairs to swap.
  */
  bool SeqSelect;

  /*! \brief Total number of CSimulation instances being run.
   */
  unsigned int nSims;
  
  /*! \brief If its true the CEReplexer loop will restart after outputting data.
   */
  bool peekMode;

  /*! \brief Initialises this class ready for the replica exchange.
   */
  virtual void preSimInit();

  /*! \brief Sets up each simulation.
   *
   * Ensures the systems are in the right CEnsemble, have a thermostat,
   * etc.
   */
  virtual void setupSim(CSimulation&, const std::string);

  //Replica Exchange attempt code

  /*! \brief Carry out a certain type of replica exchange phase.
   *
   * \param localMode The type of replica exchange phase to attempt.
   */
  void ReplexSwap(Replex_Mode_Type localMode);

  /*! \brief Output sequential configuration files, sorted in
   * temperature, for each CSimulation.
   */
  void ReplexConfigOutput(std::vector<std::string>&);

  /*! \brief Output sequential data files, sorted in
   * temperature, for each CSimulation.
   *
   * The output for the Replica exchange moves is also printed here
   */
  void ReplexDataOutput(std::vector<std::string>&);
  
  /*! \brief After every replica exchange phase this function is
   * called to update the replica exchange data collected.
   */
  void ReplexSwapTicker();

  /*! \brief Attempt a replica exchange move between two configurations.
   *
   * \param id1 First Simulation to attempt to exchange.
   * \param id2 Second Simulation to attempt to exchange.
   */
  void AttemptSwap(const unsigned int id1, const unsigned int id2);
};

#endif
