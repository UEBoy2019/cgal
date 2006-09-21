#include <CGAL/basic.h>
#include <CGAL/Cartesian.h>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <list>

#include <CGAL/Ridges.h> 
#include <CGAL/Umbilic.h>

// #include <CGAL/Monge_via_jet_fitting.h> //does not work since not in the release yet
// #include <CGAL/Lapack/Linear_algebra_lapack.h>
#include "../../../Jet_fitting_3/include/CGAL/Monge_via_jet_fitting.h" 
#include "../../../Jet_fitting_3/include/CGAL/Lapack/Linear_algebra_lapack.h" 
 
//this is an enriched Polyhedron with facets' normal
#include "PolyhedralSurf.h"
#include "PolyhedralSurf_rings.h"

typedef PolyhedralSurf::Traits          Kernel;
typedef Kernel::FT                      FT;
typedef Kernel::Point_3                 Point_3;
typedef Kernel::Vector_3                Vector_3;

typedef PolyhedralSurf::Vertex          Vertex;
typedef PolyhedralSurf::Vertex_handle   Vertex_handle;
typedef PolyhedralSurf::Vertex_iterator Vertex_iterator;

typedef T_PolyhedralSurf_rings<PolyhedralSurf> Poly_rings;
typedef CGAL::Monge_via_jet_fitting<Kernel>    Monge_via_jet_fitting;
typedef Monge_via_jet_fitting::Monge_form      Monge_form;
typedef Monge_via_jet_fitting::Monge_form_condition_numbers Monge_form_condition_numbers;
      
typedef CGAL::Vertex2Data_Property_Map_with_std_map<PolyhedralSurf> Vertex2Data_Property_Map_with_std_map;
typedef Vertex2Data_Property_Map_with_std_map::Vertex2FT_map Vertex2FT_map;
typedef Vertex2Data_Property_Map_with_std_map::Vertex2Vector_map Vertex2Vector_map;
typedef Vertex2Data_Property_Map_with_std_map::Vertex2FT_property_map Vertex2FT_property_map;
typedef Vertex2Data_Property_Map_with_std_map::Vertex2Vector_property_map Vertex2Vector_property_map;

//RIDGES
typedef CGAL::Ridge_line<PolyhedralSurf> Ridge_line;
typedef CGAL::Ridge_approximation < PolyhedralSurf,
				    back_insert_iterator< std::vector<Ridge_line*> >,
				    Vertex2FT_property_map,
				    Vertex2Vector_property_map > Ridge_approximation;
  

//UMBILICS
typedef CGAL::Umbilic<PolyhedralSurf> Umbilic;
typedef CGAL::Umbilic_approximation < PolyhedralSurf,
				      back_insert_iterator< std::vector<Umbilic*> >, 
				      Vertex2FT_property_map, 
				      Vertex2Vector_property_map > Umbilic_approximation;

//create property maps, to be moved in main?
Vertex2FT_map vertex2k1_map, vertex2k2_map, 
  vertex2b0_map, vertex2b3_map, 
  vertex2P1_map, vertex2P2_map;
Vertex2Vector_map vertex2d1_map, vertex2d2_map;

Vertex2FT_property_map vertex2k1_pm(vertex2k1_map), vertex2k2_pm(vertex2k2_map), 
  vertex2b0_pm(vertex2b0_map), vertex2b3_pm(vertex2b3_map), 
  vertex2P1_pm(vertex2P1_map), vertex2P2_pm(vertex2P2_map);
Vertex2Vector_property_map vertex2d1_pm(vertex2d1_map), vertex2d2_pm(vertex2d2_map);

  // default fct parameter values and global variables
  unsigned int d_fitting = 4;
  unsigned int d_monge = 4;
  unsigned int nb_rings = 0;//seek min # of rings to get the required #pts
  unsigned int nb_points_to_use = 0;//
  Ridge_approximation::Tag_order tag_order = Ridge_approximation::Tag_3;
  double umb_size = 1;
  bool verbose = false;
  unsigned int min_nb_points = (d_fitting + 1) * (d_fitting + 2) / 2;

/* gather points around the vertex v using rings on the
   polyhedralsurf. the collection of points resorts to 3 alternatives:
   1. the exact number of points to be used
   2. the exact number of rings to be used
   3. nothing is specified
*/
void gather_fitting_points(Vertex* v, 
			   std::vector<Point_3> &in_points,
			   Poly_rings& poly_rings)
{
  //container to collect vertices of v on the PolyhedralSurf
  std::vector<Vertex*> gathered; 
  //initialize
  in_points.clear();  
  
  //OPTION -p nb_points_to_use, with nb_points_to_use != 0. Collect
  //enough rings and discard some points of the last collected ring to
  //get the exact "nb_points_to_use" 
  if ( nb_points_to_use != 0 ) {
    poly_rings.collect_enough_rings(v, nb_points_to_use, gathered);//, vpm);
    if ( gathered.size() > nb_points_to_use ) gathered.resize(nb_points_to_use);
  }
  else { // nb_points_to_use=0, this is the default and the option -p is not considered;
    // then option -a nb_rings is checked. If nb_rings=0, collect
    // enough rings to get the min_nb_points required for the fitting
    // else collect the nb_rings required
    if ( nb_rings == 0 ) 
      poly_rings.collect_enough_rings(v, min_nb_points, gathered);//, vpm);
    else poly_rings.collect_i_rings(v, nb_rings, gathered);//, vpm);
  }
     
  //store the gathered points
  std::vector<Vertex*>::iterator 
    itb = gathered.begin(), ite = gathered.end();
  CGAL_For_all(itb,ite) in_points.push_back((*itb)->point());
}

/* Use the jet_fitting package and the class Poly_rings to compute
   diff quantities.
*/
void compute_differential_quantities(PolyhedralSurf& P, Poly_rings& poly_rings)
{
  //container for approximation points
  std::vector<Point_3> in_points;
 
  //MAIN LOOP
  Vertex_iterator vitb = P.vertices_begin(), vite = P.vertices_end();
  for (; vitb != vite; vitb++) {
    //initialize
    Vertex* v = &(*vitb);
    in_points.clear();  
    Monge_form monge_form;
    Monge_form_condition_numbers monge_form_condition_numbers;
      
    //gather points around the vertex using rings
    gather_fitting_points(v, in_points, poly_rings);

    //exit if the nb of points is too small 
    if ( in_points.size() < min_nb_points )
      {std::cerr << "Too few points to perform the fitting" << std::endl; exit(1);}

    //For Ridges we need at least 3rd order info
    assert( d_monge >= 3);
    // run the main fct : perform the fitting
    Monge_via_jet_fitting do_it(in_points.begin(), in_points.end(),
				d_fitting, d_monge, 
				monge_form, monge_form_condition_numbers);
    
    //switch min-max ppal curv/dir wrt the mesh orientation
    const Vector_3 normal_mesh = P.computeFacetsAverageUnitNormal(v);
    monge_form.comply_wrt_given_normal(normal_mesh);
       
    //Store monge data needed for ridge computations in property maps
    vertex2d1_pm[v] = monge_form.d1();
    vertex2d2_pm[v] = monge_form.d2();
    vertex2k1_pm[v] = monge_form.coefficients()[0];
    vertex2k2_pm[v] = monge_form.coefficients()[1];
    vertex2b0_pm[v] = monge_form.coefficients()[2];
    vertex2b3_pm[v] = monge_form.coefficients()[5];
    if ( d_monge >= 4) {
      //= 3*b1^2+(k1-k2)(c0-3k1^3)
      vertex2P1_pm[v] =
	3*monge_form.coefficients()[3]*monge_form.coefficients()[3]
	+(monge_form.coefficients()[0]-monge_form.coefficients()[1])
	*(monge_form.coefficients()[6]
	  -3*monge_form.coefficients()[0]*monge_form.coefficients()[0]
	  *monge_form.coefficients()[0]); 
      //= 3*b2^2+(k2-k1)(c4-3k2^3)
      vertex2P2_pm[v] = 
	3*monge_form.coefficients()[4]*monge_form.coefficients()[4]
	+(-monge_form.coefficients()[0]+monge_form.coefficients()[1])
	*(monge_form.coefficients()[10]
	  -3*monge_form.coefficients()[1]*monge_form.coefficients()[1]
	  *monge_form.coefficients()[1]); 
    }
  } //END FOR LOOP
}


int main()
{  
  //load the model from <mesh.off>
  PolyhedralSurf P;
  std::ifstream stream("data/ellipsoid.off");
  stream >> P;
  fprintf(stderr, "loadMesh %d Ves %d Facets\n",
	  P.size_of_vertices(), P.size_of_facets());
  
  //exit if not enough points in the model
  if (min_nb_points > P.size_of_vertices())  
    {std::cerr << "not enough points in the model" << std::endl;   return 1;}

  //initialize Polyhedral data : normal of facets
  P.compute_facets_normals();
  
  //create a Poly_rings object
  Poly_rings poly_rings(P);

  std::cout << "Compute differential quantities via jet fitting..." << std::endl;
  //initialize the diff quantities property maps
  compute_differential_quantities(P, poly_rings);
  
  std::cout << "Compute ridges... tag_3" << std::endl;
  //---------------------------------------------------------------------------
  //Ridges
  //--------------------------------------------------------------------------
  Ridge_approximation ridge_approximation(P, 
					  vertex2k1_pm, vertex2k2_pm,
					  vertex2b0_pm, vertex2b3_pm,
					  vertex2P1_pm, vertex2P2_pm,
					  vertex2d1_pm, vertex2d2_pm);
  std::vector<Ridge_line*> ridge_lines;
  back_insert_iterator<std::vector<Ridge_line*> > ii(ridge_lines);
  
  //Find BLUE_RIDGE, RED_RIDGE, CREST or all ridges
  ridge_approximation.compute_ridges(CGAL::BLUE_RIDGE, ii, tag_order);  
  ridge_approximation.compute_ridges(CGAL::RED_RIDGE, ii, tag_order);  
  ridge_approximation.compute_ridges(CGAL::CREST_RIDGE, ii, tag_order);  
  ridge_approximation.compute_all_ridges(ii, tag_order);  
 
  std::cout << "Compute ridges... tag_4" << std::endl;
  tag_order = Ridge_approximation::Tag_4;
   //Find BLUE_RIDGE, RED_RIDGE, CREST or all ridges
  ridge_approximation.compute_ridges(CGAL::BLUE_RIDGE, ii, tag_order);  
  ridge_approximation.compute_ridges(CGAL::RED_RIDGE, ii, tag_order);  
  ridge_approximation.compute_ridges(CGAL::CREST_RIDGE, ii, tag_order);  
  ridge_approximation.compute_all_ridges(ii, tag_order); 
 
  //---------------------------------------------------------------------------
  // UMBILICS
  //--------------------------------------------------------------------------
  Umbilic_approximation umbilic_approximation(P, 
					      vertex2k1_pm, vertex2k2_pm,
					      vertex2d1_pm, vertex2d2_pm);
  std::vector<Umbilic*> umbilics;
  back_insert_iterator<std::vector<Umbilic*> > umb_it(umbilics);
  std::cout << "compute umbilics u=1" << std::endl;
  umbilic_approximation.compute(umb_it, umb_size);
  umb_size=2;
  std::cout << "compute umbilics u=2" << std::endl;
  umb_size=5;
  std::cout << "compute umbilics u=2" << std::endl;
 

 std::vector<Umbilic*>::iterator iter_umb = umbilics.begin(), 
   iter_umb_end = umbilics.end();
  // output
  std::cout << "nb of umbilics " << umbilics.size() << std::endl;
  for (;iter_umb!=iter_umb_end;iter_umb++) std::cout << **iter_umb;

  return 1;
}
 
