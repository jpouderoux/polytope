// Try tessellating a simple lattice of generators in a box.

#include <iostream>
#include <vector>
#include <cassert>

#include "polytope.hh"

#define POLY_CHECK(x) if (!(x)) { cout << "FAIL: " << #x << endl; exit(-1); }

using namespace std;
using namespace polytope;

int main() {

  // Create the generators.
  vector<double> generators;
  const int nx = 2;
  const double x1 = 0.0, y1 = 0.0, z1 = 0.0;
  const double x2 = 100.0, y2 = 100.0, z2 = 100.0;
  const double dx = (x2 - x1)/nx, dy = (y2 - y1)/nx, dz = (z2-z1)/nx;
  unsigned ix, iy, iz;
  double xi, yi, zi;
  for (iz = 0; iz != nx; ++iz) 
  {
    zi = z1 + (iz + 0.5)*dz;
    for (iy = 0; iy != nx; ++iy) 
    {
      yi = y1 + (iy + 0.5)*dy;
      for (ix = 0; ix != nx; ++ix) 
      {
        xi = x1 + (ix + 0.5)*dx;
        generators.push_back(xi);
        generators.push_back(yi);
        generators.push_back(zi);
      }
    }
  }

  // Create the piecewise linear complex representing the box. Note that 
  // the box consists of facets that are defined by their connections to 
  // generating points.
  // Should look like the following:
  //
  //        6--------7            y
  //       /        /|            |
  //      /        / |            |
  //     2--------3  |             ------x
  //     |  .     |  |           /
  //     |  4.....|..5          z
  //     | .      | / 
  //     |.       |/
  //     0--------1             
  //
  // Create the vertices for our bounding surface.
  vector<double> PLCpoints(3*8);
  PLCpoints[3*0+0] = x1; PLCpoints[3*0+1] = y1; PLCpoints[3*0+2] = z2;
  PLCpoints[3*1+0] = x2; PLCpoints[3*1+1] = y1; PLCpoints[3*1+2] = z2;
  PLCpoints[3*2+0] = x1; PLCpoints[3*2+1] = y2; PLCpoints[3*2+2] = z2;
  PLCpoints[3*3+0] = x2; PLCpoints[3*3+1] = y2; PLCpoints[3*3+2] = z2;
  PLCpoints[3*4+0] = x1; PLCpoints[3*4+1] = y1; PLCpoints[3*4+2] = z1;
  PLCpoints[3*5+0] = x2; PLCpoints[3*5+1] = y1; PLCpoints[3*5+2] = z1;
  PLCpoints[3*6+0] = x1; PLCpoints[3*6+1] = y2; PLCpoints[3*6+2] = z1;
  PLCpoints[3*7+0] = x2; PLCpoints[3*7+1] = y2; PLCpoints[3*7+2] = z1;

  // 6 facets
  PLC<3, double> box;
  box.facets.resize(6);

  // facet 0 -- bottom face.
  box.facets[0].resize(4);
  box.facets[0][0] = 0;
  box.facets[0][1] = 4;
  box.facets[0][2] = 5;
  box.facets[0][3] = 1;

  // facet 1 -- top face.
  box.facets[1].resize(4);
  box.facets[1][0] = 2;
  box.facets[1][1] = 3;
  box.facets[1][2] = 7;
  box.facets[1][3] = 6;

  // facet 2 -- left face.
  box.facets[2].resize(4);
  box.facets[2][0] = 0;
  box.facets[2][1] = 2;
  box.facets[2][2] = 6;
  box.facets[2][3] = 4;

  // facet 3 -- right face.
  box.facets[2].resize(4);
  box.facets[2][0] = 1;
  box.facets[2][1] = 5;
  box.facets[2][2] = 7;
  box.facets[2][3] = 3;

  // facet 4 -- front face.
  box.facets[4].resize(4);
  box.facets[4][0] = 0;
  box.facets[4][1] = 1;
  box.facets[4][2] = 3;
  box.facets[4][3] = 2;

  // facet 5 -- back face.
  box.facets[5].resize(4);
  box.facets[5][0] = 5;
  box.facets[5][1] = 4;
  box.facets[5][2] = 6;
  box.facets[5][3] = 7;

  // Create the tessellation.
  Tessellation<3, double> mesh;
  TetgenTessellator tetgen(true);
  tetgen.tessellate(generators, PLCpoints, box, mesh);
  POLY_CHECK(mesh.nodes.size()/2 == (nx + 1)*(nx + 1)*(nx + 1));
  POLY_CHECK(mesh.cells.size() == nx*nx*nx);
  for (unsigned i = 0; i != nx*nx; ++i) 
  {
    POLY_CHECK(mesh.cells[i].size() == 6);
  }
  POLY_CHECK(mesh.faces.size() == 2*nx*nx*(nx + 1));

//   // Write out the file if we can.
// #if HAVE_SILO
//   vector<double> r2(nx*nx*nx, 1.0);
//   map<string, double*> nodeFields, edgeFields, faceFields, cellFields;
//   cellFields["data"] = &r2[0];
//   SiloWriter<3, double>::write(mesh, nodeFields, edgeFields, faceFields, cellFields, "test_TetgenTessellator");
// #endif

  cout << "PASS" << endl;
  return 0;
}
