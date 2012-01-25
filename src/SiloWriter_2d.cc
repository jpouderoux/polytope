#ifdef HAVE_SILO
#include "polytope.hh"
#include <fstream>
#include <set>
#include "silo.h"

#ifdef HAVE_MPI
#include "mpi.h"
#include "pmpio.h"
#endif

namespace polytope
{

using namespace std;

namespace 
{

//-------------------------------------------------------------------
// Traverse the nodes of cell i within the given tessellation in 
// order, writing their indices to nodes.
//-------------------------------------------------------------------
template <typename Real>
void 
traverseNodes(const Tessellation<Real>& mesh,
              int i,
              vector<int>& nodes)
{
  const vector<int>& cellFaces = mesh.cells[i];

  // Start with the first face, entering both nodes.
  {
    const vector<unsigned>& faceNodes = mesh.faces[cellFaces[0]];
    nodes.push_back(faceNodes[0]);
    nodes.push_back(faceNodes[1]);
  }

  // Proceed through the other faces till we're done.
  set<int> facesUsed;
  while (facesUsed.size() < cellFaces.size()-1)
  {
    int numFacesUsed = facesUsed.size();
    for (int f = 1; f < cellFaces.size(); ++f)
    {
      if (facesUsed.find(f) == facesUsed.end())
      {
        const vector<unsigned>& faceNodes = mesh.faces[cellFaces[f]];

        // Does it go on the back?
        if (faceNodes[0] == nodes.back())
        {
          nodes.push_back(faceNodes[1]);
          facesUsed.insert(f);
        }
        else if (faceNodes[1] == nodes.back())
        {
          nodes.push_back(faceNodes[0]);
          facesUsed.insert(f);
        }

        // How about the front?
        else if (faceNodes[0] == nodes.front())
        {
          nodes.insert(nodes.begin(), faceNodes[1]);
          facesUsed.insert(f);
        }
        else if (faceNodes[1] == nodes.front())
        {
          nodes.insert(nodes.begin(), faceNodes[0]);
          facesUsed.insert(f);
        }
      }
    }

    // If we're here and haven't processed any more 
    // faces, we have a face remaining that doesn't 
    // connect to the end of the chain. What about 
    // the beginning?
    if (numFacesUsed == facesUsed.size())
    {
      for (int f = 1; f < cellFaces.size(); ++f)
      {
        if (facesUsed.find(f) == facesUsed.end())
        {
          const vector<unsigned>& faceNodes = mesh.faces[cellFaces[f]];
          if (faceNodes[0] == nodes.front())
          {
            nodes.push_back(faceNodes[0]);
            facesUsed.insert(f);
          }
          else if (faceNodes[1] == nodes.front())
          {
            nodes.push_back(faceNodes[1]);
            facesUsed.insert(f);
          }
        }
      }
    }
  }
}
//-------------------------------------------------------------------

#ifdef HAVE_MPI

//-------------------------------------------------------------------
PMPIO_createFile(const char* filename,
                 const char* dirname,
                 void* userData)
{
  int driver = DB_HDF5;
  DBfile* file = DBCreate(filename, 0, DB_LOCAL, prefix.c_str(), 
                          driver);
  DBMkDir(file, dirname);
  DBSetDir(file, dirname);
  return (void*)file;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
void* 
PMPIO_openFile(const char* filename, 
               const char* dirname,
               PMPIO_iomode_t iomode, 
               void* userData)
{
  int driver = DB_HDF5;
  DBfile* file;
  if (iomode == PMPIO_WRITE)
  { 
    file = DBCreate(filename, 0, DB_LOCAL, prefix.c_str(), 
                    driver);
    DBMkDir(file, dirname);
    DBSetDir(file, dirname);
  }
  else
  {
    file = DBOpen(filename, driver, DB_READ);
    DBSetDir(file, dirname);
  }
  return (void*)file;
}
//-------------------------------------------------------------------

//-------------------------------------------------------------------
void*
PMPIO_closeFile(void* file,
                void* userData)
{
  DBClose((DBfile*)file);
}
//-------------------------------------------------------------------

#endif
}

//-------------------------------------------------------------------
template <typename Real>
void 
SiloWriter<2, Real>::
write(const Tessellation<Real>& mesh, 
      const map<string, Real*>& fields,
      const string& filePrefix,
      int cycle,
      Real time,
      MPI_Comm comm,
      int numFiles,
      int mpiTag)
{
  int nproc, rank;
  MPI_Comm_size(comm, &nproc);
  MPI_Comm_rank(comm, &rank);
  ASSERT(numFiles <= nproc);

  // Strip .silo off of the prefix if it's there.
  string prefix = filePrefix;
  size_t index = prefix.find(".silo");
  if (index >= 0)
    prefix.erase(index);

  // Open a file in Silo/HDF5 format for writing.
  char filename[1024];
#ifdef HAVE_MPI
  PMPIO_baton_t* baton = PMPIO_Init(numFiles, PMPIO_WRITE, comm, mpiTag, 
                                    &PMPIO_createFile, 
                                    &PMPIO_openFile, 
                                    &PMPIO_closeFile,
                                    0);
  int groupRank = PMPIO_GroupRank(baton, rank);
  int rankInGroup = PMPIO_RankInGroup(baton, rank);
  if (cycle >= 0)
  {
    snprintf(filename, 1024, "%s-chunk-%d-%d.silo", prefix.c_str(), 
             groupRank, cycle);
  }
  else
  {
    snprintf(filename, 1024, "%s-chunk-%d.silo", prefix.c_str(),
             groupRank);
  }

  char dirname[1024];
  snprintf(dirname, 1024, "domain_%d", rankInGroup);
  DBfile* file = (DBfile*)PMPIO_WaitForBaton(baton, filename, dirname);
#else
  if (cycle >= 0)
    snprintf(filename, 1024, "%s-%d.silo", prefix.c_str(), cycle);
  else
    snprintf(filename, 1024, "%s.silo", prefix.c_str());

  int driver = DB_HDF5;
  DBfile* file = DBCreate(filename, 0, DB_LOCAL, prefix.c_str(), 
                          driver);
  DBSetDir(file, "/");
#endif

  // Add cycle/time metadata if needed.
  DBoptlist* optlist = DBMakeOptlist(10);
  double dtime = static_cast<double>(time);
  if (cycle >= 0)
    DBAddOption(optlist, DBOPT_CYCLE, &cycle);
  if (dtime != -FLT_MAX)
    DBAddOption(optlist, DBOPT_DTIME, &dtime);

  // This is optional for now, but we'll give it anyway.
  char *coordnames[2];
  coordnames[0] = (char*)"xcoords";
  coordnames[1] = (char*)"ycoords";

  // Node coordinates.
  int numNodes = mesh.nodes.size() / 2;
  vector<double> x(numNodes), y(numNodes);
  for (int i = 0; i < numNodes; ++i)
  {
    x[i] = mesh.nodes[2*i];
    y[i] = mesh.nodes[2*i+1];
  }
  double* coords[2];
  coords[0] = &(x[0]);
  coords[1] = &(y[0]);

  // All zones are polygonal.
  int numCells = mesh.cells.size();
  vector<int> shapesize(numCells, 0),
              shapetype(numCells, DB_ZONETYPE_POLYGON),
              shapecount(numCells, 1),
              nodeList;
  for (int i = 0; i < numCells; ++i)
  {
    // Gather the nodes from this cell in traversal order.
    vector<int> cellNodes;
    traverseNodes(mesh, i, cellNodes);

    // Insert the cell's node connectivity into the node list.
    nodeList.push_back(cellNodes.size());
    nodeList.insert(nodeList.end(), cellNodes.begin(), cellNodes.end());
  }

  // Write out the 2D polygonal mesh.
  DBPutUcdmesh(file, (char*)"mesh", 2, coordnames, coords,
               numNodes, numCells, 
               (char *)"mesh_zonelist", 0, DB_DOUBLE, optlist); 
  DBPutZonelist2(file, (char*)"mesh_zonelist", numCells,
                 2, &nodeList[0], nodeList.size(), 0, 0, 0,
                 &shapetype[0], &shapesize[0], &shapecount[0],
                 numCells, optlist);

  // Write out the cell-centered mesh data.

  // Scalar fields.
  for (typename map<string, Real*>::const_iterator iter = fields.begin();
       iter != fields.end(); ++iter)
  {
    DBPutUcdvar1(file, (char*)iter->first.c_str(), (char*)"mesh",
                 (void*)iter->second, numCells, 0, 0,
                 DB_DOUBLE, DB_ZONECENT, optlist);
  }

#if 0
  // Vector fields.
  {
    vector<Real> xdata(mesh.numCells()), ydata(mesh.numCells());
    char* compNames[2];
    compNames[0] = new char[1024];
    compNames[1] = new char[1024];
    for (typename map<string, Vector<2>*>::const_iterator iter = vectorFields.begin();
         iter != vectorFields.end(); ++iter)
    {
      snprintf(compNames[0], 1024, "%s_x" , iter->first.c_str());
      snprintf(compNames[1], 1024, "%s_y" , iter->first.c_str());
      Vector<2>* data = iter->second;
      for (int i = 0; i < mesh.numCells(); ++i)
      {
        xdata[i] = data[i][0];
        ydata[i] = data[i][1];
      }
      void* vardata[2];
      vardata[0] = (void*)&xdata[0];
      vardata[1] = (void*)&ydata[0];
      DBPutUcdvar(file, (char*)iter->first.c_str(), (char*)"mesh",
                  2, compNames, vardata, mesh.numCells(), 0, 0,
                  DB_DOUBLE, DB_ZONECENT, optlist);
    }
    delete [] compNames[0];
    delete [] compNames[1];
  }
#endif

  // Clean up.
  DBFreeOptlist(optlist);

#ifdef HAVE_MPI
  PMPIO_HandOffBaton(baton, (void*)file);
  PMPIO_finish(baton);

  // Write the multi-block objects to the file.
  if (rankInGroup == 0)
  {
    vector<char*> meshNames(numFiles);
    vector<int> meshTypes(numFiles);
    for (int i = 0; i < numFiles; ++i)
    {
      char meshName[1024];
      if (cycle >= 0)
        snprintf(meshName, 1024, "%s-chunk-%d-%d.silo", prefix.c_str(), i, cycle);
      else
        snprintf(meshName, 1024, "%s-chunk-%d.silo", prefix.c_str(), i);
      meshNames[i] = strdup(meshName);
      meshTypes[i] = DB_UCDMESH;
    }

    DBoptlist* optlist = DBMakeOptlist(10);
    double dtime = static_cast<double>(time);
    if (cycle >= 0)
      DBAddOption(optlist, DBOPT_CYCLE, &cycle);
    if (dtime != -FLT_MAX)
      DBAddOption(optlist, DBOPT_DTIME, &dtime);

    DBPutMultimesh(file, "mesh", numChunks, &meshNames[0], 
                   &meshTypes[0], optlist);
    DBFreeOptlist(optlist);
  }
#else
  // Write the file.
  DBClose(file);
#endif
}
//-------------------------------------------------------------------

// Explicit instantiation.
template class SiloWriter<2, double>;


} // end namespace

#endif
