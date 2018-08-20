//------------------------------------------------------------------------------
/// \file
/// \ingroup extractor
/// \copyright (C) Copyright Aquaveo 2018.
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------

// 1. Precompiled header

// 2. My own header
#include <xmsgridtrace/gridtrace/XmUGridTriangles2d.h>

// 3. Standard library headers
#include <sstream>

// 4. External library headers

// 5. Shared code headers
#include <xmscore/misc/XmError.h>
#include <xmscore/misc/XmLog.h>
#include <xmsinterp/geometry/geoms.h>
#include <xmsinterp/geometry/GmTriSearch.h>
#include <xmsgrid/ugrid/XmUGrid.h>

// 6. Non-shared code headers

//----- Forward declarations ---------------------------------------------------

//----- External globals -------------------------------------------------------

//----- Namespace declaration --------------------------------------------------

/// XMS Namespace
namespace xms
{
//----- Constants / Enumerations -----------------------------------------------

//----- Classes / Structs ------------------------------------------------------

//----- Internal functions -----------------------------------------------------

//----- Class / Function definitions -------------------------------------------

namespace
{
class XmUGridTriangles2dImpl : public XmUGridTriangles2d
{
public:
  XmUGridTriangles2dImpl();

  virtual void BuildTriangles(const XmUGrid& a_ugrid, bool a_addTriangleCenters) override;
  virtual void BuildEarcutTriangles(const XmUGrid& a_ugrid) override;
  virtual void SetCellActivity(const DynBitset& a_cellActivity) override;

  virtual const VecPt3d& GetPoints() const override;
  virtual const VecInt& GetTriangles() const override;
  virtual BSHP<VecPt3d> GetPointsPtr() override;
  virtual BSHP<VecInt> GetTrianglesPtr() override;

  int AddCellCentroid(int a_cellIdx, const Pt3d& a_point);
  void AddCellTriangle(int a_cellIdx, int a_idx1, int a_idx2, int a_idx3);

  virtual int GetCellCentroid(int a_cellIdx) const override;

  virtual int GetIntersectedCell(const Pt3d& a_point, VecInt& a_idxs, VecDbl& a_weights) override;

private:
  void Initialize(const XmUGrid& a_ugrid);
  BSHP<GmTriSearch> GetTriSearch();

  BSHP<VecPt3d> m_points;                ///< Triangle points for the UGrid
  BSHP<VecInt> m_triangles;              ///< Triangles for the UGrid
  VecInt m_centroidIdxs;                 ///< Index of each cell centroid or -1 if none
  VecInt m_triangleToCellIdx;            ///< The cell index for each triangle
  mutable BSHP<GmTriSearch> m_triSearch; ///< Triangle searcher for triangles
};

//------------------------------------------------------------------------------
/// \brief Calculate the magnitude of a vector.
/// \param[in] a_vec: the vector
//------------------------------------------------------------------------------
double iMagnitude(const Pt3d& a_vec)
{
  double magnitude = sqrt(a_vec.x * a_vec.x + a_vec.y * a_vec.y + a_vec.z * a_vec.z);
  return magnitude;
} // iMagnitude
//------------------------------------------------------------------------------
/// \brief Calculate a quality ratio to use to determine which triangles to cut
///        using earcut triangulation. Better triangles have a lower ratio.
/// \param[in] a_points: the array the points come from
/// \param[in] a_idx1: first point of first edge
/// \param[in] a_idx2: second point of first edge, first point of second edge
/// \param[in] a_idx2: second point of second edge
/// \return The ratio, -1.0 for an inverted triangle, or -2.0 for a zero area
///         triangle
//------------------------------------------------------------------------------
double iGetEarcutTriangleRatio(const VecPt3d& a_points, int a_idx1, int a_idx2, int a_idx3)
{
  Pt3d pt1 = a_points[a_idx1];
  Pt3d pt2 = a_points[a_idx2];
  Pt3d pt3 = a_points[a_idx3];
  Pt3d v1 = pt1 - pt2;
  Pt3d v2 = pt3 - pt2;
  Pt3d v3 = pt3 - pt1;

  // get 2*area
  Pt3d cross;
  gmCross3D(v2, v1, &cross);
  double ratio;
  if (cross.z <= 0.0)
  {
    // inverted
    ratio = -1.0;
  }
  else
  {
    double area2 = iMagnitude(cross);
    if (area2 == 0.0)
    {
      // degenerate triangle
      ratio = -2.0;
    }
    else
    {
      double perimeter = iMagnitude(v1) + iMagnitude(v2) + iMagnitude(v3);
      ratio = perimeter * perimeter / area2;
    }
  }

  return ratio;
} // iGetEarcutTriangleRatio
//------------------------------------------------------------------------------
/// \brief
//------------------------------------------------------------------------------
bool iValidTriangle(const VecPt3d& a_points,
                    const VecInt a_polygon,
                    int a_idx1,
                    int a_idx2,
                    int a_idx3)
{
  const Pt3d& pt1 = a_points[a_idx1];
  const Pt3d& pt2 = a_points[a_idx2];
  const Pt3d& pt3 = a_points[a_idx3];
  for (size_t pointIdx = 0; pointIdx < a_polygon.size(); ++pointIdx)
  {
    int idx = a_polygon[pointIdx];
    if (idx != a_idx1 && idx != a_idx2 && idx != a_idx3)
    {
      const Pt3d& pt = a_points[idx];
      if (gmTurn(pt1, pt2, pt, 0.0) == TURN_LEFT && gmTurn(pt2, pt3, pt) == TURN_LEFT &&
          gmTurn(pt3, pt1, pt, 0.0) == TURN_LEFT)
      {
        return false;
      }
    }
  }

  return true;
} // iValidTriangle
//------------------------------------------------------------------------------
/// \brief Attempt to generate triangles for a cell by adding a point at the
///        centroid.
/// \param[in] a_ugridTris The triangles to add to.
/// \param[in] a_cellIdx The cell index.
/// \param[in] a_polygonIdxs The indices for the cell polygon.
/// \return true on success (can fail for concave cell)
//------------------------------------------------------------------------------
bool iGenerateCentroidTriangles(XmUGridTriangles2dImpl& a_ugridTris,
                                int a_cellIdx,
                                const VecInt& a_polygonIdxs)
{
  const VecPt3d& points = a_ugridTris.GetPoints();
  size_t numPoints = a_polygonIdxs.size();

  VecPt3d polygon;
  for (size_t pointIdx = 0; pointIdx < numPoints; ++pointIdx)
    polygon.push_back(points[a_polygonIdxs[pointIdx]]);

  Pt3d centroid = gmComputeCentroid(polygon);

  // make sure none of the triangles connected to the centroid are inverted
  for (size_t pointIdx = 0; pointIdx < polygon.size(); ++pointIdx)
  {
    const Pt3d& pt1 = polygon[pointIdx];
    const Pt3d& pt2 = polygon[(pointIdx + 1) % numPoints];

    // centroid should be to the left of each edge
    if (gmTurn(pt1, pt2, centroid, 0.0) != TURN_LEFT)
      return false;
  }

  // add centroid to list of points
  int centroidIdx = a_ugridTris.AddCellCentroid(a_cellIdx, centroid);

  // add triangles
  for (size_t pointIdx = 0; pointIdx < polygon.size(); ++pointIdx)
  {
    int idx1 = a_polygonIdxs[pointIdx];
    int idx2 = a_polygonIdxs[(pointIdx + 1) % numPoints];
    a_ugridTris.AddCellTriangle(a_cellIdx, idx1, idx2, centroidIdx);
  }

  return true;
} // iGenerateCentroidTriangles
//------------------------------------------------------------------------------
/// \brief Generate triangles using ear cut algorithm for plan view 2D cells.
/// \param[in] a_ugridTris The triangles to add to.
/// \param[in] a_cellIdx The cell index.
/// \param[in] a_polygonIdxs The indices for the cell polygon.
//------------------------------------------------------------------------------
void iBuildEarcutTriangles(XmUGridTriangles2dImpl& a_ugridTris,
                           int a_cellIdx,
                           const VecInt& a_polygonIdxs)
{
  VecInt polygonIdxs = a_polygonIdxs;
  const VecPt3d& points = a_ugridTris.GetPoints();

  // continually find best triangle on adjacent edges and cut it off polygon
  while (polygonIdxs.size() >= 4)
  {
    int bestIdx = -1;
    int secondBestIdx = -1;

    int numPoints = (int)polygonIdxs.size();
    double bestRatio = std::numeric_limits<double>::max();
    double secondBestRatio = std::numeric_limits<double>::max();
    for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
    {
      int idx1 = polygonIdxs[(pointIdx + numPoints - 1) % numPoints];
      int idx2 = polygonIdxs[pointIdx];
      int idx3 = polygonIdxs[(pointIdx + 1) % numPoints];

      // make sure triangle is valid (not inverted and doesn't have other points in it)
      double ratio = iGetEarcutTriangleRatio(points, idx1, idx2, idx3);
      if (ratio > 0.0)
      {
        if (iValidTriangle(points, polygonIdxs, idx1, idx2, idx3))
        {
          if (ratio < bestRatio)
          {
            secondBestRatio = bestRatio;
            bestRatio = ratio;
            secondBestIdx = bestIdx;
            bestIdx = pointIdx;
          }
          else if (ratio < secondBestRatio)
          {
            secondBestRatio = ratio;
            secondBestIdx = pointIdx;
          }
        }
      }
    }

    if (bestIdx >= 0)
    {
      if (numPoints == 4 && secondBestIdx >= 0)
        bestIdx = secondBestIdx;

      // cut off the ear triangle
      int polygonIdx1 = (bestIdx + numPoints - 1) % numPoints;
      int polygonIdx2 = bestIdx;
      int polygonIdx3 = (bestIdx + 1) % numPoints;
      int idx1 = polygonIdxs[polygonIdx1];
      int idx2 = polygonIdxs[polygonIdx2];
      int idx3 = polygonIdxs[polygonIdx3];
      a_ugridTris.AddCellTriangle(a_cellIdx, idx1, idx2, idx3);
      polygonIdxs.erase(polygonIdxs.begin() + polygonIdx2);
    }
    else
    {
      std::ostringstream ss;
      ss << "Unable to split cell number " << a_cellIdx + 1 << " into triangles.";
      XM_LOG(xmlog::error, ss.str());
      return;
    }
  }

  // push on remaining triangle
  a_ugridTris.AddCellTriangle(a_cellIdx, polygonIdxs[0], polygonIdxs[1], polygonIdxs[2]);
} // iBuildEarcutTriangles

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGridTriangles2dImpl
/// \brief Class to store XmUGrid triangles. Tracks where midpoints and
///        triangles came from.
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Constructor
//------------------------------------------------------------------------------
XmUGridTriangles2dImpl::XmUGridTriangles2dImpl()
: m_points(new VecPt3d)
, m_triangles(new VecInt)
, m_centroidIdxs()
, m_triangleToCellIdx()
, m_triSearch()
{
} // XmUGridTriangles2dImpl::XmUGridTriangles2dImpl
//------------------------------------------------------------------------------
/// \brief Generate triangles for the UGrid.
/// \param[in] a_ugrid The UGrid for which triangles are generated.
/// \param[in] a_addTriangleCenters Whether or not triangle cells get a centroid added.
//------------------------------------------------------------------------------
void XmUGridTriangles2dImpl::BuildTriangles(const XmUGrid& a_ugrid, bool a_addTriangleCenters)
{
  Initialize(a_ugrid);

  int numCells = a_ugrid.GetNumberOfCells();
  VecInt cellPoints;
  for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    a_ugrid.GetPointsOfCell(cellIdx, cellPoints);
    bool builtTriangles = false;
    if (a_addTriangleCenters)
      builtTriangles = iGenerateCentroidTriangles(*this, cellIdx, cellPoints);
    if (!builtTriangles)
      iBuildEarcutTriangles(*this, cellIdx, cellPoints);
  }
} // XmUGridTriangles2dImpl::BuildTriangles
//------------------------------------------------------------------------------
/// \brief Generate triangles for the UGrid using earcut algorithm.
/// \param[in] a_ugrid The UGrid for which triangles are generated.
//------------------------------------------------------------------------------
void XmUGridTriangles2dImpl::BuildEarcutTriangles(const XmUGrid& a_ugrid)
{
  Initialize(a_ugrid);

  int numCells = a_ugrid.GetNumberOfCells();
  VecInt cellPoints;
  for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    a_ugrid.GetPointsOfCell(cellIdx, cellPoints);
    iBuildEarcutTriangles(*this, cellIdx, cellPoints);
  }
} // XmUGridTriangles2dImpl::BuildEarcutTriangles
//------------------------------------------------------------------------------
/// \brief Set triangle activity based on each triangles cell.
//------------------------------------------------------------------------------
void XmUGridTriangles2dImpl::SetCellActivity(const DynBitset& a_cellActivity)
{
  if (a_cellActivity.empty())
  {
    DynBitset emptyActivity;
    GetTriSearch()->SetTriActivity(emptyActivity);
    return;
  }

  DynBitset triangleActivity;
  int numTriangles = (int)m_triangleToCellIdx.size();
  triangleActivity.resize(numTriangles);
  for (size_t triangleIdx = 0; triangleIdx < numTriangles; ++triangleIdx)
  {
    int cellIdx = m_triangleToCellIdx[triangleIdx];
    triangleActivity[triangleIdx] = cellIdx >= a_cellActivity.size() || a_cellActivity[cellIdx];
  }
  GetTriSearch()->SetTriActivity(triangleActivity);
} // XmUGridTriangles2dImpl::SetCellActivity
//------------------------------------------------------------------------------
/// \brief Get the generated triangle points.
/// \return The triangle points
//------------------------------------------------------------------------------
const VecPt3d& XmUGridTriangles2dImpl::GetPoints() const
{
  return *m_points;
} // XmUGridTriangles2dImpl::GetPoints
//------------------------------------------------------------------------------
/// \brief Get the generated triangles.
/// \return a vector of indices for the triangles.
//------------------------------------------------------------------------------
const VecInt& XmUGridTriangles2dImpl::GetTriangles() const
{
  return *m_triangles;
} // XmUGridTriangles2dImpl::GetTriangles
//------------------------------------------------------------------------------
/// \brief Get the generated triangle points as a shared pointer.
/// \return The triangle points
//------------------------------------------------------------------------------
BSHP<VecPt3d> XmUGridTriangles2dImpl::GetPointsPtr()
{
  return m_points;
} // XmUGridTriangles2dImpl::GetPointsPtr
//------------------------------------------------------------------------------
/// \brief Get the generated triangles as a shared pointer.
/// \return a vector of indices for the triangles.
//------------------------------------------------------------------------------
BSHP<VecInt> XmUGridTriangles2dImpl::GetTrianglesPtr()
{
  return m_triangles;
} // XmUGridTriangles2dImpl::GetTrianglesPtr
//------------------------------------------------------------------------------
/// \brief Add a cell centroid point.
/// \param a_cellIdx The cell index for the centroid point
/// \param a_point The centroid point
/// \return The index of the added point
//------------------------------------------------------------------------------
int XmUGridTriangles2dImpl::AddCellCentroid(int a_cellIdx, const Pt3d& a_point)
{
  int centroidIdx = (int)m_points->size();
  m_points->push_back(a_point);
  m_centroidIdxs[a_cellIdx] = centroidIdx;
  return centroidIdx;
} // XmUGridTriangles2dImpl::AddCellCentroid
//------------------------------------------------------------------------------
/// \brief Add a triangle cell.
/// \param[in] a_cellIdx The cell index the triangle is from
/// \param[in] a_idx1 The first triangle point index (counter clockwise)
/// \param[in] a_idx2 The second triangle point index (counter clockwise)
/// \param[in] a_idx3 The third triangle point index (counter clockwise)
//------------------------------------------------------------------------------
void XmUGridTriangles2dImpl::AddCellTriangle(int a_cellIdx, int a_idx1, int a_idx2, int a_idx3)
{
  m_triangles->push_back(a_idx1);
  m_triangles->push_back(a_idx2);
  m_triangles->push_back(a_idx3);
  m_triangleToCellIdx.push_back(a_cellIdx);
} // XmUGridTriangles2dImpl::AddCellTriangle
//------------------------------------------------------------------------------
/// \brief Get the centroid of a cell.
/// \param[in] a_cellIdx The cell index.
/// \return The index of the cell point.
//------------------------------------------------------------------------------
int XmUGridTriangles2dImpl::GetCellCentroid(int a_cellIdx) const
{
  int pointIdx = -1;
  if (a_cellIdx >= 0 && a_cellIdx < (int)m_centroidIdxs.size())
    pointIdx = m_centroidIdxs[a_cellIdx];
  return pointIdx;
} // XmUGridTriangles2dImpl::GetCellCentroid
//------------------------------------------------------------------------------
/// \brief Get the cell index and interpolation values intersected by a point.
/// \param[in] a_point The point to intersect with the UGrid.
/// \param[out] a_idxs The interpolation points.
/// \param[out] a_weights The interpolation weights.
/// \return The cell intersected by the point or -1 if outside of the UGrid.
//------------------------------------------------------------------------------
int XmUGridTriangles2dImpl::GetIntersectedCell(const Pt3d& a_point, VecInt& a_idxs, VecDbl& a_weights)
{
  if (!m_triSearch)
    GetTriSearch();
  int cellIdx = -1;
  int triangleLocation;
  if (m_triSearch->InterpWeightsTriangleIdx(a_point, triangleLocation, a_idxs, a_weights))
  {
    int triangleIdx = triangleLocation / 3;
    cellIdx = m_triangleToCellIdx[triangleIdx];
  }
  return cellIdx;
} // XmUGridTriangles2dImpl::GetIntersectedCell
//------------------------------------------------------------------------------
/// \brief Initialize triangulation for a UGrid.
/// \param[in] a_ugrid The UGrid for which triangles are generated.
//------------------------------------------------------------------------------
void XmUGridTriangles2dImpl::Initialize(const XmUGrid& a_ugrid)
{
  *m_points = a_ugrid.GetPoints();
  m_triangles->clear();
  m_centroidIdxs.assign(a_ugrid.GetNumberOfCells(), -1);
  m_triangleToCellIdx.clear();
  m_triSearch.reset();
} // XmUGridTriangles2dImpl::Initialize
//------------------------------------------------------------------------------
/// \brief Get triangle search object.
//------------------------------------------------------------------------------
BSHP<GmTriSearch> XmUGridTriangles2dImpl::GetTriSearch()
{
  if (!m_triSearch)
  {
    m_triSearch = GmTriSearch::New();
    m_triSearch->TrisToSearch(m_points, m_triangles);
  }
  return m_triSearch;
} // XmUGridTriangles2dImpl::GetTriSearch

} // namespace

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGridTriangles2d
/// \brief Class to store XmUGrid triangles. Tracks where midpoints and
///        triangles came from.
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Build an instance of XmUGridTriangles2d
/// \return The new instance.
//------------------------------------------------------------------------------
BSHP<XmUGridTriangles2d> XmUGridTriangles2d::New()
{
  BSHP<XmUGridTriangles2d> triangles(new XmUGridTriangles2dImpl);
  return triangles;
} // XmUGridTriangles2d::New
//------------------------------------------------------------------------------
/// \brief Default contructor.
//------------------------------------------------------------------------------
XmUGridTriangles2d::XmUGridTriangles2d()
{
} // XmUGridTriangles2d::XmUGridTriangles2d
//------------------------------------------------------------------------------
/// \brief Destructor.
//------------------------------------------------------------------------------
XmUGridTriangles2d::~XmUGridTriangles2d()
{
} // XmUGridTriangles2d::XmUGridTriangles2d

} // namespace xms