//------------------------------------------------------------------------
// TriangleTessellator
// 
// An implemenation of the Tessellator interface that uses the Triangle
// library by Jonathan Shewchuk.
//------------------------------------------------------------------------
#ifndef __Polytope_TriangleTessellator__
#define __Polytope_TriangleTessellator__

#ifdef HAVE_TRIANGLE

#include <vector>
#include <cmath>

#include "Tessellator.hh"
#include "DimensionTraits.hh"
#include "QuantizedCoordinates.hh"
#include "polytope_tessellator_utilities.hh"

#include "Clipper2d.hh"
#include "BoostOrphanage.hh"

struct triangulateio;
  
namespace polytope {

template<typename RealType>
class TriangleTessellator: public Tessellator<2, RealType> {
public:

  // Typedefs for edges, coordinates, and points
  typedef std::pair<int, int> EdgeHash;
  typedef typename DimensionTraits<2, RealType>::CoordHash CoordHash;
  typedef typename DimensionTraits<2, RealType>::IntPoint IntPoint;
  typedef typename DimensionTraits<2, RealType>::RealPoint RealPoint;

  
  // Constructor, destructor.
  TriangleTessellator();
  ~TriangleTessellator();

  // Tessellate the given generators. A bounding box is constructed about
  // the generators, and the corners of the bounding box are added as 
  // additional generators if they are not present in the list.
  void tessellate(const std::vector<RealType>& points,
                  Tessellation<2, RealType>& mesh) const;

  // Tessellate with a bounding box representing the boundaries.
  void tessellate(const std::vector<RealType>& points,
                  RealType* low,
                  RealType* high,
                  Tessellation<2, RealType>& mesh) const;

  // Tessellate obeying the given boundaries.
  void tessellate(const std::vector<RealType>& points,
                  const std::vector<RealType>& PLCpoints,
                  const PLC<2, RealType>& geometry,
                  Tessellation<2, RealType>& mesh) const;

  // Tessellate obeying the given boundaries.
  void tessellate(const std::vector<RealType>& points,
                  const ReducedPLC<2, RealType>& geometry,
                  Tessellation<2, RealType>& mesh) const;


  // This Tessellator handles PLCs!
  bool handlesPLCs() const { return true; }

  // Return the tessellator name
  std::string name() const { return "TriangleTessellator"; }

  //! Returns the accuracy to which this tessellator can distinguish coordinates.
  //! Should be returned appropriately for normalized coordinates, i.e., if all
  //! coordinates are in the range xi \in [0,1], what is the minimum allowed 
  //! delta in x.
  virtual RealType degeneracy() const { return mDegeneracy; }
  void degeneracy(RealType degeneracy) const { mDegeneracy = degeneracy; }

private:
  //-------------------- Private interface ---------------------- //
   
  // Compute node IDs around each generator and their quantized locations
  void computeCellNodesCollinear(const std::vector<RealType>& points,
                                 std::vector<std::vector<unsigned> >& cellNodes,
                                 std::map<int, IntPoint>& id2node,
                                 std::vector<unsigned>& infNodes,
                                 const bool expandCoordinates) const;

  // Compute node IDs around each generator and their quantized locations
  void computeCellNodes(const std::vector<RealType>& points,
                        std::vector<std::vector<unsigned> >& cellNodes,
                        std::map<int, IntPoint>& id2node,
                        std::vector<unsigned>& infNodes,
                        const bool expandCoordinates) const;

  // Computes the triangularization using Triangle
  void computeDelaunay(const std::vector<RealType>& points,
                       triangulateio& delaunay,
                       const bool addStabilizingPoints) const;

  // Pulls out the relevant data from Triangle's output struct
  void computeDelaunayConnectivity(const std::vector<RealType>& points,
                                   std::vector<RealPoint>& circumcenters,
                                   std::vector<unsigned>& triMask,
                                   std::map<EdgeHash, std::vector<unsigned> >& edge2tris,
                                   std::map<int, std::set<unsigned> >& gen2tri,
                                   std::vector<int>& triangleList,
                                   RealType* low,
                                   RealType* high) const;

  // Fill in the mesh struct using the final quantized cell data
  void constructBoundedTopology(const std::vector<RealType>& points,
                                const ReducedPLC<2, RealType>& geometry,
                                const std::vector<ReducedPLC<2, CoordHash> >& intCells,
                                Tessellation<2, RealType>& mesh) const;

  // ----------------------------------------------------- //
  // Private tessellate calls used by internal algorithms  //
  // ----------------------------------------------------- //


  // Bounded tessellation with prescribed bounding box
  void tessellate(const std::vector<RealType>& points,
                  const ReducedPLC<2, CoordHash>& intGeometry,
                  const QuantizedCoordinates<2, RealType>& coords,
                  std::vector<ReducedPLC<2, CoordHash> >& intCells) const;

  // -------------------------- //
  // Private member variables   //
  // -------------------------- //

  // The quantized coordinates for this tessellator (inner and outer)
  static CoordHash coordMax;
  static RealType mDegeneracy; 
  mutable QuantizedCoordinates<2,RealType> mCoords;

  friend class BoostOrphanage<RealType>;
};


//------------------------------------------------------------------------------
// Static initializations.
//------------------------------------------------------------------------------
template<typename RealType> 
typename TriangleTessellator<RealType>::CoordHash 
TriangleTessellator<RealType>::coordMax = (1LL << 26);

template<typename RealType> 
RealType  
TriangleTessellator<RealType>::mDegeneracy = 1.0/TriangleTessellator<RealType>::coordMax;

} //end polytope namespace

#endif
#endif
