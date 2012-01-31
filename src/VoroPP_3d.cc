//---------------------------------Spheral++----------------------------------//
// VoroPP_3d
//----------------------------------------------------------------------------//
#include <iostream>
#include <iterator>
#include <algorithm>
#include <map>

#include "polytope.hh" // Pulls in ASSERT and VoroPP_3d.hh.
#include "container.hh"

namespace polytope {

using namespace std;
using namespace voro;
using std::min;
using std::max;
using std::abs;

namespace { // We hide internal functions in an anonymous namespace.

//------------------------------------------------------------------------------
// A simple 3D point.
//------------------------------------------------------------------------------
template<typename Real>
struct Point3 {
  Real x, y, z;
  Point3(): x(0.0), y(0.0) {}
  Point3(const Real& xi, const Real& yi): x(xi), y(yi) {}
  Point3& operator=(const Point3& rhs) { x = rhs.x; y = rhs.y; return *this; }
  bool operator<(const Point3<Real>& rhs) const {
    return ((x < rhs.x) or (x == rhs.x and y < rhs.y));
  }
};

// It's nice being able to print these things.
template<typename Real>
std::ostream&
operator<<(std::ostream& os, const Point3<Real>& p) {
  os << "(" << p.x << " " << p.y << ")";
  return os;
}

//------------------------------------------------------------------------------
// Z coordinate of cross product : (p2 - p1)x(p3 - p1).
//------------------------------------------------------------------------------
template<typename Real>
double zcross(const Point3<Real>& p1, const Point3<Real>& p2, const Point3<Real>& p3) {
  return (p2.x - p1.x)*(p3.y - p1.y) - (p2.y - p1.y)*(p3.x - p1.x);
}

//------------------------------------------------------------------------------
// Sort a set of Point2 points in counter-clockwise order using Andrews 
// monotone chain algorithm.
// Based on an example at http://www.algorithmist.com/index.php/Monotone_Chain_Convex_Hull.cpp
//------------------------------------------------------------------------------
template<typename Real>
vector<Point2<Real> >
sortCounterClockwise(vector<Point2<Real> >& points) {
  const unsigned n = points.size();
  int i, k, t;
  
  // Sort the input points by x coordinate.
  sort(points.begin(), points.end());

  // Prepare the result.
  vector<Point2<Real> > result(2*n);

  // Build the lower hull.
  for (i = 0, k = 0; i < n; i++) {
    while (k >= 2 and zcross(result[k - 2], result[k - 1], points[i]) <= 0.0) k--;
    result[k++] = points[i];
  }

  // Build the upper hull.
  for (i = n - 2, t = k + 1; i >= 0; i--) {
    while (k >= t and zcross(result[k - 2], result[k - 1], points[i]) <= 0.0) k--;
    result[k++] = points[i];
  }

  // Size the result and we're done.
  result.resize(n);
  return result;
}

//------------------------------------------------------------------------------
// Helper method to update our face info.
//------------------------------------------------------------------------------
template<typename Real>
inline
void
insertFaceInfo(const set<unsigned>& fhashi,
               const unsigned icell,
               const vector<unsigned>& faceNodeIDs,
               map<set<unsigned>, unsigned>& faceHash2ID,
               Tessellation<2, Real>& mesh) {
  typedef set<unsigned> FaceHash;

  // Is this a new face?
  map<FaceHash, unsigned>::const_iterator faceItr = faceHash2ID.find(fhashi);
  if (faceItr == faceHash2ID.end()) {

    // Yep, it's a new face.
    const unsigned iface = mesh.faces.size();
    faceHash2ID[fhashi] = iface;
    mesh.cells[icell].push_back(iface);
    mesh.faces.push_back(faceNodeIDs);
    mesh.faceCells.push_back(vector<unsigned>());
    mesh.faceCells.back().push_back(icell);
    ASSERT(count(mesh.cells[icell].begin(), mesh.cells[icell].end(), iface) == 1);
    ASSERT(mesh.faces.size() == iface + 1);
    ASSERT(mesh.faceCells.size() == iface + 1);
    ASSERT(mesh.faceCells[iface].size() == 1 and mesh.faceCells[iface][0] == icell);

  } else {

    // Nope, this is an existing face, so we record it in the 
    // cell list as the 1's complement.
    const unsigned iface = faceItr->second;
    ASSERT(iface < mesh.faces.size());
    mesh.cells[icell].push_back(~iface);
    mesh.faceCells[iface].push_back(icell);
    ASSERT(count(mesh.cells[icell].begin(), mesh.cells[icell].end(), ~iface) == 1);
    ASSERT(mesh.faceCells[iface].size() == 2 and mesh.faceCells[iface][1] == icell);
  }
}

//------------------------------------------------------------------------------
// Compute the distance^2 between points.
//------------------------------------------------------------------------------
template<typename Real>
inline
Real
distance2(const Real& x1, const Real& y1,
          const Real& x2, const Real& y2) {
  return (x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 -y1);
}

//------------------------------------------------------------------------------
// A unique hash for a face as an ordered collection of node indices.
//------------------------------------------------------------------------------
pair<unsigned, unsigned> 
hashFace(const unsigned i, const unsigned j)
{
  ASSERT(i != j);
  return (i < j ? make_pair(i, j) : make_pair(j, i));
}

} // end anonymous namespace

//------------------------------------------------------------------------------
// Constructor.
//------------------------------------------------------------------------------
template<typename Real>
VoroPP_3d<Real>::
VoroPP_3d(const unsigned nx,
          const unsigned ny,
          const Real degeneracy):
  mNx(nx),
  mNy(ny),
  mDegeneracy2(degeneracy*degeneracy) {
  ASSERT(mDegeneracy2 > 0.0);
}

//------------------------------------------------------------------------------
// Destructor.
//------------------------------------------------------------------------------
template<typename Real>
VoroPP_3d<Real>::
~VoroPP_3d() {
}

//------------------------------------------------------------------------------
// Compute the tessellation in the box.
//------------------------------------------------------------------------------
template<typename Real>
void
VoroPP_3d<Real>::
tessellate(vector<Real>& points,
           Real* low,
           Real* high,
           Tessellation<3, Real>& mesh) const {

  typedef set<unsigned> FaceHash;

  const unsigned ncells = points.size()/3;
  const Real xmin = low[0], ymin = low[1], zmin = low[2];
  const Real xmax = high[0], ymax = high[1], zmax = high[3];
  const Real scale = max(xmax - xmin, max(ymax - ymin, zmax - zmin));

  // Pre-conditions.
  ASSERT(xmin < xmax);
  ASSERT(ymin < ymax);
  ASSERT(zmin < zmax);
  ASSERT(scale > 0.0);
  ASSERT(points.size() % 3 == 0);
  for (int i = 0; i != ncells; ++i) {
    ASSERT(points[3*i]     >= xmin and points[3*i]     <= xmax);
    ASSERT(points[3*i + 1] >= ymin and points[3*i + 1] <= ymax);
    ASSERT(points[3*i + 2] >= zmin and points[3*i + 2] <= zmax);
  }
  ASSERT(mesh.nodes.size() == 0);
  ASSERT(mesh.cells.size() == 0);
  ASSERT(mesh.faces.size() == 0);
  ASSERT(mesh.faceCells.size() == 0);

  bool newNode;
  unsigned i, j, k, n, icell, jcell;
  double xc, yc, zc;
  Real xv, yv, xv_last, yv_last;

  // Size the output arrays.
  mesh.cells.resize(ncells);

  // Scale the input coordinates to a unit box, which seems to be more robust for
  // Voro++.
  vector<Real> generators;
  generators.reserve(3*ncells);
  for (i = 0; i != ncells; ++i) {
    generators.push_back((points[3*i]     - xmin)/scale);
    generators.push_back((points[3*i + 1] - ymin)/scale);
    generators.push_back((points[3*i + 2] - zmin)/scale);
    ASSERT(generators[3*i]     >= 0.0 and generators[3*i]     <= 1.0);
    ASSERT(generators[3*i + 1] >= 0.0 and generators[3*i + 1] <= 1.0);
    ASSERT(generators[3*i + 2] >= 0.0 and generators[3*i + 2] <= 1.0);
  }
  ASSERT(generators.size() == 3*ncells);

  // Build the Voro++ container, and add the generators.
  container con(0.0, 1.0,
                0.0, 1.0,
                0.0, 1.0,
                mNx, mNy, mNz,
                false, false, false, 8);
  for (i = 0; i != ncells; ++i) con.put(i, generators[3*i], generators[3*i + 1], generators[3*i + 2]);
  ASSERT(con.total_particles() == ncells);

  // Build the tessellation cell by cell.
  voronoicell_neighbor cell;                       // Use cells with neighbor tracking.
  vector<vector<unsigned> > cellNeighbors(ncells); // Keep track of neighbor cells.
  vector<set<unsigned> > cellNodes(ncells);        // Keep track of the cell nodes.
  map<FaceHash, unsigned> faceHash2ID;             // map from face hash to ID.
  c_loop_all loop(con); // Loop over all cells.
  if (loop.start()) {
    do {
      if (con.compute_cell(cell, loop)) {
        icell = loop.pid();                   // The cell index.

        // Get the cell centroid.
        double *pp = con.p[loop.ijk] + con.ps*loop.q;   // Man, this is obvious!
        cell.centroid(xc, yc, zc);
        xc += pp[0];
        yc += pp[1];
        zc += pp[2];

        // Read the neighbor cell IDs.  Any negative IDs indicate a boundary
        // surface, so just throw them away.
        vector<int> tmpNeighbors;
        cell.neighbors(tmpNeighbors);
        remove_copy_if(tmpNeighbors.begin(), tmpNeighbors.end(), 
                       back_inserter(cellNeighbors[icell]),
                       bind2nd(less<int>(), 0));

        // Read out the vertices into a temporary array.
        vector<Point3<Real> > vertices;
        for (k = 0; k != cell.p; ++k) vertices.push_back(Point3<Real>(xc + 0.5*cell.pts[3*k],
                                                                      yc + 0.5*cell.pts[3*k + 1],
                                                                      zc + 0.5*cell.pts[3*k + 2]));
        ASSERT(vertices.size() >= 4);

        // Read the face vertex indices to a temporary array as well.
        vector<int> voroFaceVertexIndices;
        cell.face_vertices(voroFaceVertexIndices);
        vector<unsigned> faceNodeIDs;

        // Walk the faces.
        nf = cell.number_of_faces();
        ASSERT(nf >= 4);
        k = 0;
        for (if = 0; if != nf; ++if) {
          ASSERT(k < voroFaceVertexIndices.size());
          fhashi = FaceHash();

          // Read the vertices for this face.  We assume they are listed in the
          // proper counter-clockwise order (viewed from outside) here!
          nvf = voroFaceVertexIndices[k++];
          xv_last = 10.0;
          yv_last = 10.0;
          zv_last = 10.0;
          for (iv = 0; iv != nvf; ++iv) {
            ASSERT(k < voroFaceVertexIndices.size());
            j = voroFaceVertexIndices[k++];

            // Is this node distinct from the last one we visited?
            if (distance2(vertices[j].x, vertices[j].y, vertices[j].z,
                          xv_last, yv_last, zv_last) > mDegneracy2) {

              // Has this vertex already been created by one of our neighbors?
              newNode = true;
              vector<unsigned>::const_iterator neighborItr = cellNeighbors[icell].begin();
              while (newNode and neighborItr != cellNeighbors[icell].end()) {
                jcell = *neighborItr++;
                ASSERT(jcell < ncells);
                set<unsigned>::const_iterator nodeItr = cellNodes[jcell].begin();
                while (newNode and nodeItr != cellNodes[jcell].end()) {
                  i = *nodeItr++;
                  ASSERT(i < mesh.nodes.size());
                  if (distance2(vertices[j].x, vertices[j].y, vertices[j].z,
                                mesh.nodes[3*i], mesh.nodes[3*i + 1], mesh.nodes[3*i + 2]) < mDegeneracy2) {
                    // Found it!
                    newNode = false;
                    cellNodes[icell].insert(i);
                    faceNodeIDs.push_back(i);
                    fhashi.insert(i);
                  }
                }
              }

              // This is a new vertex position, so create a new node.
              if (newNode) {
                i = mesh.nodes.size()/3;
                cellNodes[icell].insert(i);
                faceNodeIDs.push_back(i);
                fhashi.insert(i);
                mesh.nodes.push_back(vertices[j].x);
                mesh.nodes.push_back(vertices[j].y);
                mesh.nodes.push_back(vertices[j].z);
              }
            }
            ASSERT(faceNodeIDs.size() >= 3);

            // Add this face to the cell.
            insertFaceInfo(fhashi, icell, faceNodeIDs, faceHash2ID, mesh);
          }
        }
        ASSERT(cellNodes[icell].size() >= 3);
      }
    } while (loop.inc());
  }
        
  // De-normalize the vertex coordinates back to the input frame.
  for (int i = 0; i != mesh.nodes.size(); ++i) mesh.nodes[i] = xmin + scale*mesh.nodes[i];
}

//------------------------------------------------------------------------------
// Explicit instantiation.
//------------------------------------------------------------------------------
template class VoroPP_3d<double>;

}
