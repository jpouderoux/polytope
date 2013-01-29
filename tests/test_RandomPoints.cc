// test_RandomPoints
//
// Stress test for meshing complicated PLC boundaries with/without holes.
// Iterate over each of the default boundaries defined in Boundary2D.hh
// and tessellate using N randomly-distributed generators for N=10,100,1000.
// Can test both Triangle and Voro++ 2D tessellators. Voro++ has been
// commented out since it currently lacks PLC capabilities.

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "polytope.hh"
#include "Boundary2D.hh"
#include "Generators.hh"
#include "polytope_test_utilities.hh"

#if HAVE_MPI
// extern "C" {
#include "mpi.h"
// }
#endif

using namespace std;
using namespace polytope;

// -----------------------------------------------------------------------
// outputResult
// -----------------------------------------------------------------------
void outputResult(Tessellator<2,double>& tessellator,
		  int bType,
		  unsigned nPoints)
{
   Boundary2D<double> boundary;

   boundary.computeDefaultBoundary(bType);
   Generators<2,double> generators( boundary );

   generators.randomPoints( nPoints );

   Tessellation<2,double> mesh;
   tessellate2D(generators.mPoints,boundary,tessellator,mesh);
   POLY_ASSERT( mesh.cells.size() == nPoints );
   double area = computeTessellationArea(mesh);
   cout << "Tessellation Area = " << area << endl;
   cout << "Relative error    = " << (boundary.mArea-area)/boundary.mArea << endl;
   
#if HAVE_SILO
   vector<double> index(mesh.cells.size());
   vector<double> genx (mesh.cells.size());
   vector<double> geny (mesh.cells.size());
   for (int i = 0; i < mesh.cells.size(); ++i){
      index[i] = double(i);
      genx[i]  = generators.mPoints[2*i];
      geny[i]  = generators.mPoints[2*i+1];
   }
   map<string,double*> nodeFields, edgeFields, faceFields, cellFields;
   cellFields["cell_index"   ] = &index[0];
   cellFields["cell_center_x"] = &genx[0];
   cellFields["cell_center_y"] = &geny[0];
   ostringstream os;
   os << "test_RandomPoints_boundary_" << bType << "_" << nPoints << "points";
   polytope::SiloWriter<2, double>::write(mesh, nodeFields, edgeFields, 
                                          faceFields, cellFields, os.str());
#endif
}


// -----------------------------------------------------------------------
// testBoundary
// -----------------------------------------------------------------------
void testBoundary(Boundary2D<double>& boundary,
                  Tessellator<2,double>& tessellator)
{
   Generators<2,double> generators( boundary );
   unsigned nPoints = 1;
   Tessellation<2,double> mesh;
   for( unsigned n = 0; n < 3; ++n ){
      POLY_ASSERT( mesh.empty() );
      nPoints = nPoints * 10;
      cout << nPoints << " points...";

      generators.randomPoints( nPoints );      
      tessellate2D(generators.mPoints,boundary,tessellator,mesh);

      POLY_ASSERT( mesh.cells.size() == nPoints );
      cout << "PASS" << endl;
#if HAVE_SILO
      vector<double> index( mesh.cells.size());
      for (int i = 0; i < mesh.cells.size(); ++i) index[i] = double(i);
      map<string,double*> nodeFields, edgeFields, faceFields, cellFields;
      cellFields["cell_index"] = &index[0];
      ostringstream os;
      os << "test_RandomPoints_boundary_" << boundary.mType << "_" << nPoints << "_points";
      polytope::SiloWriter<2, double>::write(mesh, nodeFields, edgeFields, 
                                             faceFields, cellFields, os.str());
#endif
      mesh.clear();   
   }
}

// -----------------------------------------------------------------------
// testAllBoundaries
// -----------------------------------------------------------------------
void testAllBoundaries(Tessellator<2,double>& tessellator)
{
   for (int bid = 0; bid < 7; ++bid){
      cout << "Testing boundary type " << bid << endl;
      Boundary2D<double> boundary;
      boundary.computeDefaultBoundary(bid);
      testBoundary( boundary, tessellator );
   }
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main(int argc, char** argv)
{
#if HAVE_MPI
   MPI_Init(&argc, &argv);
#endif

   cout << "\nTriangle Tessellator:\n" << endl;
   TriangleTessellator<double> triangle;
   //testAllBoundaries(triangle);

   //srand(10489593);  //cell parents multiple orphans            - PASSES
   //srand(10489594);  //orphan neighbors are orphan parents      - PASSES
   //srand(10489609);  //overlapping orphans (order issue)        - FAILS
   //srand(10489611);  //empty orphan neighbor set                - FAILS
   //srand(10489612);  //Boost.Geometry invalid overlay exception - FAILS
   int rnum = 10489616;

   // for (int i=0; i<30; ++i){
   //    srand(rnum+i);
   //    outputResult(triangle,5,20);
   //    cout << "*****This completes test " << rnum+i << "******" << endl;
   // }
   
   srand(rnum);
   outputResult(triangle,5,20);
   cout << "*****This completes test " << rnum << "******" << endl;




   // NOTE: Voro++ currently lacks PLC boundary capabilities
   //
   // cout << "\nVoro 2D Tessellator:\n" << endl;
   // VoroPP_2d<double> voro;
   // testAllBoundaries(voro);

#if HAVE_MPI
   MPI_Finalize();
#endif
   return 0;
}
