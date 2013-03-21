// test_Circumcenters
//
// Tests for Triangle Tessellator involving degenerate Delaunay
// 1. small-area triangle has a circumcenter approaching infinity

#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "polytope.hh"

#if HAVE_MPI
#include "mpi.h"
#endif

using namespace std;
using namespace polytope;

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------
int main(int argc, char** argv)
{
#if HAVE_MPI
   MPI_Init(&argc, &argv);
#endif

  vector<double> PLCpoints;
  PLCpoints.push_back(0.0);  PLCpoints.push_back(0.0);
  PLCpoints.push_back(3.0);  PLCpoints.push_back(0.0);
  PLCpoints.push_back(3.0);  PLCpoints.push_back(2.0);
  PLCpoints.push_back(0.0);  PLCpoints.push_back(2.0);

  PLC<2,double> boundary;
  boundary.facets.resize(4, vector<int>(2));
  for (unsigned i=0; i != 4; ++i) {
     boundary.facets[i][0] = i;
     boundary.facets[i][1] = (i+1) % 4;
  }
  
  vector<double> points;
  points.push_back(1.5);  points.push_back(1.5);
  points.push_back(0.5);  points.push_back(0.5);
  points.push_back(2.5);  points.push_back(0.5);
  points.push_back(0.5);  points.push_back(1.5);
  points.push_back(1.5);  points.push_back(1.5);
  points.push_back(2.5);  points.push_back(1.5);
  
  TriangleTessellator<double> triangle;
  
  int N = 34;
  double vert1[2] = {0.5, 0.5}, vert2[2] = {2.5, 0.5}, circumcenter[2];
  double displacement = 1.0;
  for (int i=0; i != N; ++i) {
     displacement *= 0.5;
     double vert3[2] = {1.5,0.5+displacement};
     geometry::computeCircumcenter2d(vert1, vert2, vert3, circumcenter);
     cout << "Step " << i << ":" << endl;
     cout << "  New generator position  = (1.5," << 0.5+displacement << ")" << endl;
     cout << "  Degenerate tri height   = " << displacement << endl;
     cout << "  Degenerate circumcenter = (" << circumcenter[0] << ","
          << circumcenter[1] << ")" << endl;
     points[1] = 0.5 + displacement;
     Tessellation<2,double> mesh;
     triangle.tessellate(points,PLCpoints,boundary,mesh);
     //triangle.tessellate(points,mesh);

     cout << "          Number of nodes = " << mesh.nodes.size()/2 << endl;
     cout << "          Number of faces = " << mesh.faces.size() << endl << endl;
  }

  cout << "PASS" << endl;

#if HAVE_MPI
   MPI_Finalize();
#endif
   return 0;
}