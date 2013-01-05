#include <iostream>
#include <vector>
#include <set>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "polytope.hh"
#include "Boundary2D.hh"
#include "Generators.hh"

#define POLY_CHECK(x) if (!(x)) { cout << "FAIL: " << #x << endl; return; } //exit(-1); }
#define POLY_CHECK2(x) if (!(x)) { cout << "FAIL: "; return false; }

using namespace std;
using namespace polytope;

// -------------------------------------------------------------------- //
template <class T>
inline std::string to_string(const T& t){
   std::stringstream ss;
   ss << t;
   return ss.str();
}
// -------------------------------------------------------------------- //


// -----------------------------------------------------------------------
// minLength
// -----------------------------------------------------------------------
double minLength(Tessellation<2,double>& mesh)
{
   double faceLength = FLT_MAX;
   for (unsigned iface = 0; iface != mesh.faces.size(); ++iface)
   // for (std::vector<std::vector<unsigned> >::const_iterator faceItr =
   //         mesh.faces.begin(); faceItr != mesh.faces.end(); ++faceItr )
   {
      POLY_ASSERT( mesh.faces[iface].size() == 2 );
      //POLY_ASSERT( faceItr->second.size() == 2 );
      const unsigned inode0 = mesh.faces[iface][0];
      const unsigned inode1 = mesh.faces[iface][1];
      double x0 = mesh.nodes[2*inode0], y0 = mesh.nodes[2*inode0+1];
      double x1 = mesh.nodes[2*inode1], y1 = mesh.nodes[2*inode1+1];
      double len = (x1-x0)*(x1-x0) + (y1-y0)*(y1-y0);
      faceLength = min( faceLength, sqrt(len) );
   }
}

// -----------------------------------------------------------------------
// checkIfCartesian
// -----------------------------------------------------------------------
bool checkIfCartesian(Tessellation<2,double>& mesh, unsigned nx, unsigned ny)
{
   POLY_CHECK2(mesh.nodes.size()/2 == (nx + 1)*(ny + 1) );
   POLY_CHECK2(mesh.cells.size()   == nx*ny );
   POLY_CHECK2(mesh.faces.size()   == nx*(ny + 1) + ny*(nx + 1) );
   for (unsigned i = 0; i != nx*ny; ++i) POLY_CHECK2(mesh.cells[i].size() == 4);
   
   std::vector<std::set<unsigned> > nodeCells = mesh.computeNodeCells();
   for (unsigned i = 0; i != (nx+1)*(ny+1); ++i)
   {
      POLY_CHECK2( (nodeCells[i].size() == 4) ||
                   (nodeCells[i].size() == 2) ||
                   (nodeCells[i].size() == 1) );
   }
   return true;
}

// -----------------------------------------------------------------------
// tessellate
// -----------------------------------------------------------------------
void tessellate(Boundary2D<double>& boundary,
                Generators<2,double>& generators,
                Tessellator<2,double>& tessellator,
                Tessellation<2,double>& mesh)
{
   if( tessellator.handlesPLCs() ){
      tessellator.tessellate(generators.mGenerators,
                             boundary.mGens, boundary.mPLC, mesh);
   }else{
      tessellator.tessellate(generators.mGenerators,
                             boundary.mLow, boundary.mHigh, mesh);
   }
}

// -----------------------------------------------------------------------
// generateMesh
// -----------------------------------------------------------------------
void generateMesh(Tessellator<2,double>& tessellator)
{
   // Set the boundary
   Boundary2D<double> boundary;
   boundary.unitSquare();
   Generators<2,double> generators( boundary );

   unsigned nx = 50;
   std::vector<unsigned> nxny(2,nx);
   
   // Create generators
   cout << "Generator locations randomly perturbed by" << endl;
   double epsilon = 2.0e-16;
   for ( int i = 0; i != 10; ++i, epsilon *= 10)
   {
      cout << "+/- " << epsilon/2 << "...";
      generators.cartesianPoints( nxny );         // reset locations
      generators.perturb( epsilon );              // perturb
      Tessellation<2,double> mesh;
      tessellate(boundary,generators,tessellator,mesh);
      bool isCartesian = checkIfCartesian(mesh,nx,nx);
      if( isCartesian ){ 
         cout << "PASS" << endl; 
      }else{
         cout << "Minimum face length = " << minLength(mesh) << endl;
      }

#if HAVE_SILO
      std::string name = "test_Degenerate_" + to_string(epsilon);
      vector<double> index( mesh.cells.size());
      for (int i = 0; i < mesh.cells.size(); ++i) index[i] = double(i);
      map<string,double*> nodeFields, edgeFields, faceFields, cellFields;
      cellFields["cell_index"] = &index[0];
      polytope::SiloWriter<2, double>::write(mesh, nodeFields, edgeFields, 
                                             faceFields, cellFields, name);
#endif
   }
}


// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
int main(int argc, char** argv)
{
#if HAVE_MPI
   MPI_Init(&argc, &argv);
#endif


#if HAVE_TRIANGLE
   cout << "\nTriangle Tessellator:\n" << endl;
   TriangleTessellator<double> triangle;
   generateMesh(triangle);
#endif   

   cout << "\nVoro 2D Tessellator:\n" << endl;
   VoroPP_2d<double> voro;
   generateMesh(voro);

#if HAVE_MPI
   MPI_Finalize();
#endif
   return 0;
}
