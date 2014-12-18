#define BOOST_TEST_MODULE Stable_intersection_test
#include <boost/test/included/unit_test.hpp>
#include <magnet/intersection/stable_poly.hpp>
#include <magnet/math/polynomial.hpp>
#include <cmath>
#include <complex>
#include <random>

using namespace magnet::math;
const Polynomial<1, double, 't'> t{0, 1};
std::mt19937 RNG;

const double rootvals[] = {-1e7, -1e3, -3.14159265, -1, 0, 1, 3.14159265, +100, 1e3, 1e7 };
const size_t tests = 1000;

template<class F, class R>
void test_solution(const F& f, double solution, double tol, R actual_roots) {
  Variable<'t'> t;
  const auto roots = solve_roots(f);
  const auto df = derivative(f, t);
  const auto droots = solve_roots(df);

  double nextroot = HUGE_VAL;
  for (double root : roots)
    if (root > 0)
      nextroot = std::min(nextroot, root);
  
  double nextdroot = HUGE_VAL;
  for (double root : droots)
    if (root > 0)
      nextdroot = std::min(nextdroot, root);

  try {
    if (solution == HUGE_VAL) {
      //No root! so check there really are no roots
      for (double root : roots)
	if (root > 0) {
	  if ((eval(df, t == root) < 0) || ((eval(df, t== root) == 0) && (eval(derivative(df, t), t == root) < 0)))
	    throw std::runtime_error("Did not detect a root!");
	}
    } else if (solution == 0) {
      //Immediate collision! Check that the particles are currently
      //approaching and overlapping
      if (eval(f, t==0.0) > tol)
	throw std::runtime_error("Not sufficiently overlapped during an immediate collision");
      if (eval(df, t==0.0) > tol)
	throw std::runtime_error("Not sufficiently approaching during an immediate collision");
    } else {
      if (eval(f, t==0) >= 0) {
	//Particles started out not overlapping, therefore the solution
	//must be an actual root
	if (nextroot == HUGE_VAL) {
	  //Check if this is a phantom root
	  BOOST_CHECK(nextroot == HUGE_VAL);
	} else {
	  if (!::boost::test_tools::check_is_close(solution, nextroot, ::boost::test_tools::percent_tolerance(tol)))
	    throw std::runtime_error("Solution and root are not close!");
	}
      } else /*if (f(0) <= 0)*/ {
	//The particles started out overlapping, 
	if (::boost::test_tools::check_is_close(solution, nextdroot, ::boost::test_tools::percent_tolerance(tol))) {
	  //Its at the next turning point, the root must be overlapping
	  if (eval(f, t==solution) > 4 * precision(f, solution))
	    throw std::runtime_error("Turning point event is not within the overlapped zone");
	  if (eval(df, t==solution) > 4 * precision(df, solution))
	    throw std::runtime_error("Particles are receeding at turning point root!");
	} else {
	  //The detected root must be after the nextroot (which is the
	  //exit root)
	  BOOST_CHECK(solution >= nextroot);
	  if (std::abs(eval(f, t==solution)) > 4 * precision(f, solution)) {
	    throw std::runtime_error("This is not a root!");
	  }
	  double err = HUGE_VAL;
	  for (double root : roots)
	    err = std::min(err, std::abs(root - solution));
	  BOOST_CHECK_SMALL(err, tol);
	}
      }
    }
  } catch (std::exception& e) {
    BOOST_ERROR(e.what());
    std::cout.precision(50);
    std::cout << "next_event = " << magnet::intersection::nextEvent(f) << std::endl;
    std::cout << "f(x)=" << f << std::endl;
    std::cout << "f'(x)=" << df << std::endl;
    std::cout << "f''(x)=" << derivative(df, t) << std::endl;
    std::cout << "f(0)=" << eval(f, t==0) << std::endl;
    std::cout << "f'(0)=" << eval(df, t==0) << std::endl;
    std::cout << "f("<< solution <<")=" << eval(f, t==solution) << std::endl;
    std::cout << "f'("<< solution <<")=" << eval(df, t==solution) << std::endl;
    std::cout << "f("<< nextroot <<")=" << eval(f, t==nextroot) << std::endl;
    std::cout << "f'("<< nextroot <<")=" << eval(df, t==nextroot) << std::endl;
    std::cout << "actual_roots = " << actual_roots << std::endl;
    std::cout << "roots = " << roots << std::endl;
    std::cout << "f' roots = " << droots << std::endl;
    std::cout << "d|f|("<<nextroot<<") = " << precision(f, nextroot) << std::endl;
    std::cout << "d|f'|("<<nextroot<<") = " << precision(df, nextroot) << std::endl;
    std::cout << "d|f|("<<solution<<") = " << precision(f, solution) << std::endl;
    std::cout << "d|f'|("<<solution<<") = " << precision(df, solution) << std::endl;
  }
}

BOOST_AUTO_TEST_CASE( Linear_function )
{
  RNG.seed(1);

  for (double sign : {-1.0, +1.0})
    for (double root : rootvals) {
      auto poly = (t - root) * sign;
      
      std::uniform_real_distribution<double> shift_dist(-10, 10);
      for (size_t i(0); i < tests; ++i) {
	auto s_poly = shift_function(poly, shift_dist(RNG));
	auto roots = solve_roots(s_poly);
	test_solution(s_poly, magnet::intersection::nextEvent(s_poly), 1e-10, magnet::containers::StackVector<double,1>{root});
      }
    }
}

BOOST_AUTO_TEST_CASE( Quadratic_function )
{
  RNG.seed(1);
  for (double sign : {-1.0, +1.0})
    for (double root1 : rootvals)
      for (double root2 : rootvals) {
	auto poly = (t - root1) * (t - root2) * sign;
	std::uniform_real_distribution<double> shift_dist(-10, 10);
	for (size_t i(0); i < tests; ++i) {
	  double shift = shift_dist(RNG);
	  auto s_poly = shift_function(poly, shift);
	  auto roots = solve_roots(s_poly);
	  test_solution(s_poly, magnet::intersection::nextEvent(s_poly), 1e-8, magnet::containers::StackVector<double,2>{root1, root2});
	}
      }
}

BOOST_AUTO_TEST_CASE( Cubic_function )
{
  RNG.seed(1);
  
const double rootvals[] = {-1e7, -1e3, -3.14159265, -1, 0, 1, 3.14159265, +100, 1e3, 1e7 };

//  double sign = -1;
//  double root1 = -1e7;
//  double root2 = 3.14159265;
//  double root3 = 3.14159265;
//  double shift = -4.2074071211187300534106725535821169614791870117188;
//  auto poly = (t - root1) * (t - root2) * (t - root3) * sign;
//  auto s_poly = shift_function(poly, shift);
//  auto result = magnet::intersection::nextEvent(s_poly);
//  test_solution(s_poly, result, 1e-4);
  
  for (double sign : {-1.0, +1.0})
    for (double root1 : rootvals)
      for (double root2 : rootvals) 
	for (double root3 : rootvals) 
	  {
	    auto poly = (t - root1) * (t - root2) * (t - root3) * sign;
	    std::uniform_real_distribution<double> shift_dist(-10, 10);
	    for (size_t i(0); i < tests; ++i) {
	      double shift = shift_dist(RNG);
	      auto s_poly = shift_function(poly, shift);
	      test_solution(s_poly, magnet::intersection::nextEvent(s_poly), 1e-4, magnet::containers::StackVector<double,3>{root1, root2, root3});
	    }
	  }
}

//BOOST_AUTO_TEST_CASE( Quartic_function )
//{
//  RNG.seed(1);
//  for (double sign : {-1.0, +1.0})
//    for (double root1 : rootvals)
//      for (double root2 : rootvals) 
//	for (double root3 : rootvals) 
//	  for (double root4 : rootvals) 
//	    {
//	      auto poly = (x - root1) * (x - root2) * (x - root3) * (x - root4) * sign;
//	      std::uniform_real_distribution<double> shift_dist(-10, 10);
//	      for (size_t i(0); i < tests; ++i) {
//		double shift = shift_dist(RNG);
//		auto s_poly = shift_polynomial(poly, shift);
//		test_solution(s_poly, magnet::intersection::nextEvent(s_poly, 1.0), 1e-4);
//	      }
//	    }
//}