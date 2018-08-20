//------------------------------------------------------------------------------
/// \file
/// \ingroup extractor
/// \copyright (C) Copyright Aquaveo 2018.
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------

// 1. Precompiled header

// 2. My own header
#include <xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.h>

// 3. Standard library headers
#include <sstream>

// 4. External library headers

// 5. Shared code headers
#include <xmscore/misc/XmError.h>
#include <xmscore/misc/XmLog.h>
#include <xmscore/misc/xmstype.h>
#include <xmsgridtrace/gridtrace/XmUGridTriangles2d.h>
#include <xmsgrid/ugrid/XmUGrid.h>
#include <xmsinterp/geometry/geoms.h>
#include <xmsinterp/geometry/GmTriSearch.h>
#include <xmsinterp/interpolate/InterpUtil.h>

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

////////////////////////////////////////////////////////////////////////////////
/// Implementation for XmUGrid2dDataExtractor
class XmUGrid2dDataExtractorImpl : public XmUGrid2dDataExtractor
{
public:
  XmUGrid2dDataExtractorImpl(BSHP<XmUGrid> a_ugrid);
  XmUGrid2dDataExtractorImpl(BSHP<XmUGrid2dDataExtractorImpl> a_extractor);

  virtual void SetGridPointScalars(const VecFlt& a_pointScalars,
                                   const DynBitset& a_activity,
                                   DataLocationEnum a_activityLocation) override;
  virtual void SetGridCellScalars(const VecFlt& a_cellScalars,
                                  const DynBitset& a_activity,
                                  DataLocationEnum a_activityLocation) override;

  virtual void SetExtractLocations(const VecPt3d& a_locations) override;
  virtual const VecPt3d& GetExtractLocations() const override;
  virtual void ExtractData(VecFlt& a_outData) override;
  virtual float ExtractAtLocation(const Pt3d& a_location) override;

  virtual void SetUseIdwForPointData(bool a_) override;
  virtual void SetNoDataValue(float a_value) override;

  virtual void BuildTriangles(DataLocationEnum a_location) override;
  virtual const BSHP<XmUGridTriangles2d> GetUGridTriangles() const override;

private:
  void ApplyActivity(const DynBitset& a_activity,
                     DataLocationEnum a_location,
                     DynBitset& a_cellActivity);
  void SetGridPointActivity(const DynBitset& a_pointActivity, DynBitset& a_cellActivity);
  void SetGridCellActivity(const DynBitset& a_cellActivity);
  void PushPointDataToCentroids(const DynBitset& a_cellActivity);
  void PushCellDataToTrianglePoints(const VecFlt& a_cellScalars, const DynBitset& a_cellActivity);
  float CalculatePointByAverage(const VecInt& a_cellIdxs,
                                const VecFlt& a_cellScalars,
                                const DynBitset& a_cellActivity);
  float CalculatePointByIdw(int a_pointIdx,
                            const VecInt& a_cellIdxs,
                            const VecFlt& a_cellScalars,
                            const DynBitset& a_cellActivity);

  BSHP<XmUGrid> m_ugrid;                ///< UGrid for dataset
  DataLocationEnum m_triangleType;      ///< if triangles been generated for points or cells
  BSHP<XmUGridTriangles2d> m_triangles; ///< triangles generated from UGrid to use for data extraction
  VecPt3d m_extractLocations;           ///< output locations for interpolated values
  VecFlt m_pointScalars;                ///< scalars to interpolate from
  bool m_useIdwForPointData;            ///< use IDW to calculate point data from cell data
  float m_noDataValue;                  ///< value to use for inactive result
};

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGrid2dDataExtractorImpl
/// \brief Implementation for XmUGrid2dDataExtractor which provides ability
///        to extract dataset values at points for an unstructured grid.
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Construct from a UGrid.
/// \param[in] a_ugrid The UGrid to construct an extractor for.
//------------------------------------------------------------------------------
XmUGrid2dDataExtractorImpl::XmUGrid2dDataExtractorImpl(BSHP<XmUGrid> a_ugrid)
: m_ugrid(a_ugrid)
, m_triangleType(LOC_UNKNOWN)
, m_triangles(XmUGridTriangles2d::New())
, m_extractLocations()
, m_pointScalars()
, m_useIdwForPointData(false)
, m_noDataValue(XM_NODATA)
{
} // XmUGrid2dDataExtractorImpl::XmUGrid2dDataExtractorImpl
//------------------------------------------------------------------------------
/// \brief Create a new XmUGrid2dDataExtractorImpl using shallow copy from
///        existing extractor.
/// \param[in] a_extractor The extractor to shallow copy
/// \return the new XmUGrid2dDataExtractorImpl.
//------------------------------------------------------------------------------
XmUGrid2dDataExtractorImpl::XmUGrid2dDataExtractorImpl(BSHP<XmUGrid2dDataExtractorImpl> a_extractor)
: m_ugrid(a_extractor->m_ugrid)
, m_triangleType(a_extractor->m_triangleType)
, m_triangles(a_extractor->m_triangles)
, m_extractLocations()
, m_pointScalars()
, m_useIdwForPointData(a_extractor->m_useIdwForPointData)
, m_noDataValue(a_extractor->m_noDataValue)
{
} // XmUGrid2dDataExtractorImpl::XmUGrid2dDataExtractorImpl
//------------------------------------------------------------------------------
/// \brief Setup point scalars to be used to extract interpolated data.
/// \param[in] a_pointScalars The point scalars.
/// \param[in] a_activity The activity of the cells.
/// \param[in] a_activityLocation The location at which the data is currently
///            stored.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetGridPointScalars(const VecFlt& a_pointScalars,
                                                     const DynBitset& a_activity,
                                                     DataLocationEnum a_activityLocation)
{
  if (a_pointScalars.size() != m_ugrid->GetNumberOfPoints())
  {
    XM_LOG(xmlog::debug, "Invalid point scalar size in 2D data extractor.");
  }

  BuildTriangles(LOC_POINTS);

  DynBitset cellActivity;
  ApplyActivity(a_activity, a_activityLocation, cellActivity);

  m_pointScalars = a_pointScalars;
  PushPointDataToCentroids(cellActivity);
} // XmUGrid2dDataExtractorImpl::SetGridPointScalars
//------------------------------------------------------------------------------
/// \brief Setup cell scalars to be used to extract interpolated data.
/// \param[in] a_cellScalars The point scalars.
/// \param[in] a_activity The activity of the cells.
/// \param[in] a_activityLocation The location at which the data is currently
///            stored.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetGridCellScalars(const VecFlt& a_cellScalars,
                                                    const DynBitset& a_activity,
                                                    DataLocationEnum a_activityLocation)
{
  if ((int)a_cellScalars.size() != m_ugrid->GetNumberOfCells())
  {
    XM_LOG(xmlog::debug, "Invalid cell scalar size in 2D data extractor.");
  }

  BuildTriangles(LOC_CELLS);

  DynBitset cellActivity;
  ApplyActivity(a_activity, a_activityLocation, cellActivity);

  PushCellDataToTrianglePoints(a_cellScalars, cellActivity);
} // XmUGrid2dDataExtractorImpl::SetGridCellScalars
//------------------------------------------------------------------------------
/// \brief Sets locations of points to extract interpolated scalar data from.
/// \param[in] a_locations The locations.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetExtractLocations(const VecPt3d& a_locations)
{
  m_extractLocations = a_locations;
} // XmUGrid2dDataExtractorImpl::SetExtractLocations
//------------------------------------------------------------------------------
/// \brief Gets locations of points to extract interpolated scalar data from.
/// \return The locations.
//------------------------------------------------------------------------------
const VecPt3d& XmUGrid2dDataExtractorImpl::GetExtractLocations() const
{
  return m_extractLocations;
} // XmUGrid2dDataExtractorImpl::GetExtractLocations
//------------------------------------------------------------------------------
/// \brief Extract interpolated data for the previously set locations.
/// \param[out] a_outData The interpolated scalars.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::ExtractData(VecFlt& a_outData)
{
  a_outData.clear();

  a_outData.reserve(m_extractLocations.size());
  VecInt interpIdxs;
  VecDbl interpWeights;
  for (const auto& pt : m_extractLocations)
  {
    int cellIdx = m_triangles->GetIntersectedCell(pt, interpIdxs, interpWeights);
    if (cellIdx >= 0)
    {
      double interpValue = 0.0;
      for (size_t i = 0; i < interpIdxs.size(); ++i)
      {
        int ptIdx = interpIdxs[i];
        std::cout << i << " Size=" << interpWeights.size()<<"\n";
        double weight = interpWeights[i];
        float scalar = m_pointScalars[ptIdx];
        interpValue += scalar * weight;
      }
      a_outData.push_back(static_cast<float>(interpValue));
    }
    else
    {
      a_outData.push_back(m_noDataValue);
    }
  }
} // XmUGrid2dDataExtractorImpl::ExtractData
//------------------------------------------------------------------------------
/// \brief Extract interpolated data for the previously set locations.
/// \param[in] a_location The location to get the interpolated scalar.
/// \return The interpolated value.
//------------------------------------------------------------------------------
float XmUGrid2dDataExtractorImpl::ExtractAtLocation(const Pt3d& a_location)
{
  VecPt3d locations(1, a_location);
  SetExtractLocations(locations);
  VecFlt values;
  ExtractData(values);
  return values[0];
} // XmUGrid2dDataExtractorImpl::ExtractAtLocation
//------------------------------------------------------------------------------
/// \brief Set to use IDW to calculate point scalar values from cell scalars.
/// \param a_useIdw Whether to turn IDW on or off.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetUseIdwForPointData(bool a_useIdw)
{
  m_useIdwForPointData = a_useIdw;
} // XmUGrid2dDataExtractorImpl::SetUseIdwForPointData
//------------------------------------------------------------------------------
/// \brief Set value to use when extracted value is in inactive cell or doesn't
///        intersect with the grid.
/// \param[in] a_value The no data value
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetNoDataValue(float a_value)
{
  m_noDataValue = a_value;
} // XmUGrid2dDataExtractorImpl::SetNoDataValue
//------------------------------------------------------------------------------
/// \brief Apply point or cell activity to triangles.
/// \param[in] a_activity The activity of the scalar values.
/// \param[in] a_location The location of the activity (cells or points).
/// \param[out] a_cellActivity The cell activity of the scalar values.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::ApplyActivity(const DynBitset& a_activity,
                                               DataLocationEnum a_location,
                                               DynBitset& a_cellActivity)
{
  if (a_activity.empty())
  {
    // when empty, everything gets enabled on the cells
    a_cellActivity = a_activity;
    SetGridCellActivity(a_cellActivity);
  }
  else
  {
    if (a_location == LOC_POINTS)
    {
      SetGridPointActivity(a_activity, a_cellActivity);
    }
    else if (a_location == LOC_CELLS)
    {
      SetGridCellActivity(a_activity);
      a_cellActivity = a_activity;
    }
  }
} // XmUGrid2dDataExtractorImpl::ApplyActivity
//------------------------------------------------------------------------------
/// \brief Set point activity. Turns off each cell attached to an inactive
///        point.
/// \param[in] a_pointActivity The activity for the UGrid points.
/// \param[out] a_cellActivity The resulting activity transfered to the cells.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetGridPointActivity(const DynBitset& a_pointActivity,
                                                      DynBitset& a_cellActivity)
{
  if (a_pointActivity.size() != m_ugrid->GetNumberOfPoints() && !a_pointActivity.empty())
  {
    XM_LOG(xmlog::debug, "Invalid point activity size in 2D data extractor.");
  }

  if (a_pointActivity.empty())
  {
    a_cellActivity = a_pointActivity;
    m_triangles->SetCellActivity(a_cellActivity);
    return;
  }

  a_cellActivity.reset();
  a_cellActivity.resize(m_ugrid->GetNumberOfCells(), true);
  VecInt attachedCells;
  int numPoints = m_ugrid->GetNumberOfPoints();
  for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
  {
    if (pointIdx < a_pointActivity.size() && !a_pointActivity[pointIdx])
    {
      m_ugrid->GetPointCells(pointIdx, attachedCells);
      for (auto cellIdx : attachedCells)
      {
        a_cellActivity[cellIdx] = false;
      }
    }
  }
  m_triangles->SetCellActivity(a_cellActivity);
} // XmUGrid2dDataExtractorImpl::SetGridPointActivity
//------------------------------------------------------------------------------
/// \brief Set activity on cells
/// \param[in] a_cellActivity The cell activity of the scalar values.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::SetGridCellActivity(const DynBitset& a_cellActivity)
{
  if (a_cellActivity.size() != m_ugrid->GetNumberOfCells() && !a_cellActivity.empty())
  {
    XM_LOG(xmlog::debug, "Invalid cell activity size in 2D data extractor.");
  }
  m_triangles->SetCellActivity(a_cellActivity);
} // XmUGrid2dDataExtractorImpl::SetGridCellActivity
//------------------------------------------------------------------------------
/// \brief Push point scalar data to cell centroids using average.
/// \param[in] a_cellActivity The cell activity of the scalar values.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::PushPointDataToCentroids(const DynBitset& a_cellActivity)
{
  // default any missing scalar values to zero
  m_pointScalars.resize(m_triangles->GetPoints().size(), 0.0);

  VecInt cellPoints;
  int numCells = m_ugrid->GetNumberOfCells();
  for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    if (a_cellActivity.empty() || a_cellActivity[cellIdx])
    {
      int centroidIdx = m_triangles->GetCellCentroid(cellIdx);
      if (centroidIdx >= 0)
      {
        m_ugrid->GetPointsOfCell(cellIdx, cellPoints);
        double sum = 0.0;
        for (auto ptIdx : cellPoints)
          sum += m_pointScalars[ptIdx];
        double average = sum / cellPoints.size();
        m_pointScalars[centroidIdx] = static_cast<float>(average);
      }
    }
  }
} // XmUGrid2dDataExtractorImpl::PushPointDataToCentroids
//------------------------------------------------------------------------------
/// \brief Push cell scalar data to triangle points using cells connected to
///        a point with average or IDW.
/// \param[in] a_cellScalars the cell scalar values.
/// \param[in] a_cellActivity the cell activity vector.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::PushCellDataToTrianglePoints(const VecFlt& a_cellScalars,
                                                              const DynBitset& a_cellActivity)
{
  m_pointScalars.resize(m_triangles->GetPoints().size());
  VecInt cellIdxs;
  int numPoints = m_ugrid->GetNumberOfPoints();
  for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
  {
    m_ugrid->GetPointCells(pointIdx, cellIdxs);
    if (m_useIdwForPointData)
    {
      m_pointScalars[pointIdx] =
        CalculatePointByIdw(pointIdx, cellIdxs, a_cellScalars, a_cellActivity);
    }
    else
    {
      m_pointScalars[pointIdx] = CalculatePointByAverage(cellIdxs, a_cellScalars, a_cellActivity);
    }
  }

  int numCells = m_ugrid->GetNumberOfCells();
  for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    int pointIdx = m_triangles->GetCellCentroid(cellIdx);
    if (pointIdx >= 0)
      m_pointScalars[pointIdx] = cellIdx >= a_cellScalars.size() ? 0.0f : a_cellScalars[cellIdx];
  }
} // XmUGrid2dDataExtractorImpl::PushCellDataToTrianglePoints
//------------------------------------------------------------------------------
/// \brief Calculate the average of the cell values connected to a point.
/// \param[in] a_cellIdxs the cells surrounding the point.
/// \param[in] a_cellScalars the cell scalar values.
/// \param[in] a_cellActivity the cell activity vector.
/// \return Average of active surrounding cell scalars.
//------------------------------------------------------------------------------
float XmUGrid2dDataExtractorImpl::CalculatePointByAverage(const VecInt& a_cellIdxs,
                                                          const VecFlt& a_cellScalars,
                                                          const DynBitset& a_cellActivity)
{
  double sum = 0.0;
  int sumCount = 0;
  for (auto cellIdx : a_cellIdxs)
  {
    if (cellIdx >= a_cellActivity.size() || a_cellActivity[cellIdx])
    {
      sum += cellIdx >= a_cellScalars.size() ? 0.0 : a_cellScalars[cellIdx];
      ++sumCount;
    }
  }
  double average;
  if (sumCount)
    average = sum / sumCount;
  else
    average = m_noDataValue;
  return static_cast<float>(average);
} // XmUGrid2dDataExtractorImpl::CalculatePointByAverage
//------------------------------------------------------------------------------
/// \brief Calculate the point value by IDW method from surrounding cells.
/// \param[in] a_pointIdx the point to calculate the value for.
/// \param[in] a_cellIdxs the cells surrounding the point.
/// \param[in] a_cellScalars the cell scalar values.
/// \param[in] a_cellActivity the cell activity vector.
/// \return IDW interpolated value of active surrounding cell scalars.
//------------------------------------------------------------------------------
float XmUGrid2dDataExtractorImpl::CalculatePointByIdw(int a_pointIdx,
                                                      const VecInt& a_cellIdxs,
                                                      const VecFlt& a_cellScalars,
                                                      const DynBitset& a_cellActivity)
{
  Pt3d pt = m_ugrid->GetPoint(a_pointIdx);
  VecInt cellCentroids;
  for (auto cellIdx : a_cellIdxs)
  {
    int centroidIdx = m_triangles->GetCellCentroid(cellIdx);
    if (0 <= centroidIdx && a_cellActivity[cellIdx])
    {
      cellCentroids.push_back(centroidIdx);
    }
  }
  if (!cellCentroids.empty())
  {
    VecDbl d2;
    VecDbl weights;
    inDistanceSquared(pt, cellCentroids, m_triangles->GetPoints(), true, d2);
    inIdwWeights(d2, 2, false, weights);

    double interpValue = 0.0;
    for (size_t cellIdx = 0; cellIdx < cellCentroids.size(); ++cellIdx)
    {
      int ptIdx = cellCentroids[cellIdx];
      double weight = weights[cellIdx];
      interpValue += a_cellScalars[cellIdx] * weight;
    }
    return static_cast<float>(interpValue);
  }
  else
  {
    return CalculatePointByAverage(a_cellIdxs, a_cellScalars, a_cellActivity);
  }
} // XmUGrid2dDataExtractorImpl::CalculatePointByIdw
//------------------------------------------------------------------------------
/// \brief Build triangles for UGrid for either point or cell scalars.
/// \param[in] a_location Location to build on (points or cells).
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorImpl::BuildTriangles(DataLocationEnum a_location)
{
  if (m_triangleType != a_location)
  {
    bool triangleCentroids = a_location == LOC_CELLS;
    m_triangles->BuildTriangles(*m_ugrid, triangleCentroids);
    m_triangleType = a_location;
  }
} // XmUGrid2dDataExtractorImpl::BuildTriangles
//------------------------------------------------------------------------------
/// \brief Get the UGrid triangles.
/// \return The UGrid triangles.
//------------------------------------------------------------------------------
const BSHP<XmUGridTriangles2d> XmUGrid2dDataExtractorImpl::GetUGridTriangles() const
{
  return m_triangles;
} // XmUGrid2dDataExtractorImpl::GetUGridTriangles
////////////////////////////////////////////////////////////////////////////////
/// \class XmUGrid2dDataExtractor
/// \brief Provides ability to interpolate and extract the scalar values  points and along arcs
///        for an unstructured grid.
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Create a new XmUGrid2dDataExtractor.
/// \param[in] a_ugrid The UGrid geometry to use to extract values from
/// \return the new XmUGrid2dDataExtractor
//------------------------------------------------------------------------------
BSHP<XmUGrid2dDataExtractor> XmUGrid2dDataExtractor::New(BSHP<XmUGrid> a_ugrid)
{
  BSHP<XmUGrid2dDataExtractor> extractor(new XmUGrid2dDataExtractorImpl(a_ugrid));
  return extractor;
} // XmUGrid2dDataExtractor::New
//------------------------------------------------------------------------------
/// \brief Create a new XmUGrid2dDataExtractor using shallow copy from existing
///        extractor.
/// \param[in] a_extractor The extractor to shallow copy
/// \return the new XmUGrid2dDataExtractor.
//------------------------------------------------------------------------------
BSHP<XmUGrid2dDataExtractor> XmUGrid2dDataExtractor::New(BSHP<XmUGrid2dDataExtractor> a_extractor)
{
  BSHP<XmUGrid2dDataExtractorImpl> copied = BDPC<XmUGrid2dDataExtractorImpl>(a_extractor);
  if (copied)
  {
    BSHP<XmUGrid2dDataExtractor> extractor(new XmUGrid2dDataExtractorImpl(copied));
    return extractor;
  }

  XM_ASSERT(0);
  return nullptr;
} // XmUGrid2dDataExtractor::New
//------------------------------------------------------------------------------
/// \brief Constructor
//------------------------------------------------------------------------------
XmUGrid2dDataExtractor::XmUGrid2dDataExtractor()
{
} // XmUGrid2dDataExtractor::XmUGrid2dDataExtractor
//------------------------------------------------------------------------------
/// \brief Destructor
//------------------------------------------------------------------------------
XmUGrid2dDataExtractor::~XmUGrid2dDataExtractor()
{
} // XmUGrid2dDataExtractor::~XmUGrid2dDataExtractor

} // namespace xms

#ifdef CXX_TESTx
//------------------------------------------------------------------------------
// Unit Tests
//------------------------------------------------------------------------------
using namespace xms;
#include <xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.t.h>
#include <xmsgrid/ugrid/XmUGrid.t.h>

#include <xmscore/testing/TestTools.h>

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGrid2dDataExtractorUnitTests
/// \brief Class to to test XmUGrid2dDataExtractor
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Test extractor with point scalars only.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testPointScalarsOnly()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);
  extractor->SetNoDataValue(-999.0);

  VecFlt pointScalars = {1, 2, 3, 2};
  extractor->SetGridPointScalars(pointScalars, DynBitset(), LOC_POINTS);
  VecPt3d extractLocations = {
    {0.0, 0.0, 0.0}, {0.25, 0.75, 100.0}, {0.5, 0.5, 0.0}, {0.75, 0.25, -100.0}, {-1.0, -1.0, 0.0}};
  extractor->SetExtractLocations(extractLocations);

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {1.0, 2.0, 2.0, 2.0, -999.0};
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testScalarsOnly
//------------------------------------------------------------------------------
/// \brief Test extractor when using point scalars and cell activity.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testPointScalarCellActivity()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  VecFlt pointScalars = {1, 2, 3, 2};
  DynBitset cellActivity;
  cellActivity.push_back(true);
  cellActivity.push_back(false);
  extractor->SetGridPointScalars(pointScalars, cellActivity, LOC_CELLS);
  extractor->SetExtractLocations({{0.25, 0.75, 100.0}, {0.75, 0.25, -100.0}, {-1.0, -1.0, 0.0}});

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {XM_NODATA, 2.0, XM_NODATA};
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testPointScalarCellActivity
//------------------------------------------------------------------------------
/// \brief Test extractor when using point scalars and point activity.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testPointScalarPointActivity()
{
  // clang-format off
  ///  6----7---------8 point row 3
  ///  |   / \       /|
  ///  |  /   \     / |
  ///  | /     \   /  |
  ///  |/       \ /   |
  ///  3---------4----5 point row 2
  ///  |\       / \   |
  ///  | \     /   \  |
  ///  |  \   /     \ |
  ///  |   \ /       \|
  ///  0----1---------2 point row 1
  ///
  VecPt3d points = {
    {0, 0, 0}, {1, 0, 0}, {3, 0, 0},  // row 1 of points
    {0, 1, 0}, {2, 1, 0}, {3, 1, 0},  // row 2 of points
    {0, 2, 0}, {1, 2, 0}, {3, 2, 0}}; // row 3 of points
  VecInt cells = {
    XMU_TRIANGLE, 3, 0, 1, 3, // row 1 of triangles
    XMU_TRIANGLE, 3, 1, 4, 3,
    XMU_TRIANGLE, 3, 1, 2, 4,
    XMU_TRIANGLE, 3, 2, 5, 4,

    XMU_TRIANGLE, 3, 3, 7, 6, // row 2 of triangles
    XMU_TRIANGLE, 3, 3, 4, 7,
    XMU_TRIANGLE, 3, 4, 8, 7,
    XMU_TRIANGLE, 3, 4, 5, 8};

  VecFlt pointScalars = {
    0, 0, 0,  // row 1
    1, 1, 1,  // row 2
    2, 2, 2}; // row 3

  // extract value for each cell
  VecPt3d extractLocations = {
    {0.25, 0.25, 0}, // cell 0
    {1.00, 0.25, 0}, // cell 1
    {2.00, 0.50, 0}, // cell 2
    {2.75, 0.75, 0}, // cell 3
    {0.25, 1.75, 0}, // cell 4
    {1.00, 1.25, 0}, // cell 5
    {1.50, 1.75, 0}, // cell 6
    {2.75, 1.25, 0}  // cell 7
  };

  // expected results with point 4 inactive
  VecFlt expectedPerCell = {
    0.25, XM_NODATA, XM_NODATA, XM_NODATA,
    1.75, XM_NODATA, XM_NODATA, XM_NODATA };
  // clang-format on

  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  // set point 4 inactive
  // should cause all cells connected to point 4 to return XM_NODATA
  DynBitset pointActivity;
  pointActivity.resize(9, true);
  pointActivity[4] = false;
  extractor->SetGridPointScalars(pointScalars, pointActivity, LOC_POINTS);

  // extract interpolated scalar for each cell
  extractor->SetExtractLocations(extractLocations);

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  TS_ASSERT_EQUALS(expectedPerCell, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testPointScalarPointActivity
//------------------------------------------------------------------------------
/// \brief Test when scalar and activity arrays are sized incorrectly.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testInvalidPointScalarsAndActivitySize()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  VecFlt pointScalars = {1, 2, 3};
  DynBitset activity;
  activity.push_back(true);
  activity.push_back(false);
  extractor->SetGridPointScalars(pointScalars, activity, LOC_POINTS);

  VecPt3d extractLocations = {{0.25, 0.75, 100.0}, {0.75, 0.25, 0.0}};
  extractor->SetExtractLocations(extractLocations);

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {1.0, XM_NODATA};
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testInvalidPointScalarsAndActivitySize
//------------------------------------------------------------------------------
/// \brief Test extractor with cell scalars only.
/// \verbatim
///  3----2
///  | 1 /|
///  |  / |
///  | /  |
///  |/ 0 |
///  0----1
/// \endverbatim
//------------------------------------------------------------------------------
//! [snip_test_Example_SimpleLocationExtractor]
void XmUGrid2dDataExtractorUnitTests::testCellScalarsOnly()
{
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);

  // Step 1. Create an extractor for an existing XmUGrid (call xms::XmUGrid2dDataExtractor).
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  // Step 2. Set scalar and activity values (call xms::XmUGrid2dDataExtractor::SetGridCellScalars or XmUGrid2dDataExtractor::SetPointCellScalars).
  VecFlt cellScalars = {1, 2};
  extractor->SetGridCellScalars(cellScalars, DynBitset(), LOC_CELLS);

  // Step 3. Set extract locations (call XmUGrid2dDataExtractor::SetExtractLocations).
  extractor->SetExtractLocations({{0.0, 0.0, 0.0},
                                  {0.25, 0.75, 100.0},
                                  {0.5, 0.5, 0.0},
                                  {0.75, 0.25, -100.0},
                                  {-0.1, -0.1, 0.0}});

  // Step 4. Extract the data (call xms::XmUGrid2dDataExtractor::ExtractData).
  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {1.5, 2.0, 1.5, 1.0, XM_NODATA};
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testCellScalarsOnly
//! [snip_test_Example_SimpleLocationExtractor]
//------------------------------------------------------------------------------
/// \brief Test extractor when using cell scalars and cell activity.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testCellScalarCellActivity()
{
  // clang-format off
  //  6----7---------8 point row 3
  //  | 4 / \       /|
  //  |  /   \  6  / |
  //  | /  5  \   /  |
  //  |/       \ / 7 |
  //  3---------4----5 point row 2
  //  |\       / \ 3 |
  //  | \  1  /   \  |
  //  |  \   /     \ |
  //  | 0 \ /   2   \|
  //  0----1---------2 point row 1
  //
  VecPt3d points = {
    {0, 0, 0}, {1, 0, 0}, {3, 0, 0},  // row 1 of points
    {0, 1, 0}, {2, 1, 0}, {3, 1, 0},  // row 2 of points
    {0, 2, 0}, {1, 2, 0}, {3, 2, 0}}; // row 3 of points
  VecInt cells = {
    XMU_TRIANGLE, 3, 0, 1, 3, // row 1 of triangles
    XMU_TRIANGLE, 3, 1, 4, 3,
    XMU_TRIANGLE, 3, 1, 2, 4,
    XMU_TRIANGLE, 3, 2, 5, 4,

    XMU_TRIANGLE, 3, 3, 7, 6, // row 2 of triangles
    XMU_TRIANGLE, 3, 3, 4, 7,
    XMU_TRIANGLE, 3, 4, 8, 7,
    XMU_TRIANGLE, 3, 4, 5, 8};

  VecFlt cellScalars = {
    2, 4, 6, 8,  // row 1
    4, 6, 8, 10  // row 2
  };

  // extract value for each cell
  VecPt3d extractLocations = {
    {0.25, 0.25, 0}, // cell 0
    {1.00, 0.25, 0}, // cell 1
    {2.00, 0.50, 0}, // cell 2
    {2.75, 0.75, 0}, // cell 3
    {0.25, 1.75, 0}, // cell 4
    {1.00, 1.25, 0}, // cell 5
    {1.50, 1.75, 0}, // cell 6
    {2.75, 1.25, 0}  // cell 7
  };

  // expected results with point 4 inactive
  VecFlt expectedPerCell = {
    XM_NODATA, 4.0000, XM_NODATA, 8.2500, // row 1 cells
    XM_NODATA, 6.0000, XM_NODATA, 9.750   // row 2 cells
  };
  // clang-format on

  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  // set point 4 inactive
  // should cause all cells connected to point 4 to return XM_NODATA
  DynBitset cellActivity;
  cellActivity.resize(8, true);
  cellActivity[0] = false;
  cellActivity[2] = false;
  cellActivity[4] = false;
  cellActivity[6] = false;
  extractor->SetGridCellScalars(cellScalars, cellActivity, LOC_CELLS);

  // extract interpolated scalar for each cell
  extractor->SetExtractLocations(extractLocations);

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  TS_ASSERT_EQUALS(expectedPerCell, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testCellScalarCellActivity
//------------------------------------------------------------------------------
/// \brief Test extractor when using cell scalars and cell activity.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testCellScalarCellActivityIdw()
{
  // clang-format off
  //  6----7---------8 point row 3
  //  | 4 / \       /|
  //  |  /   \  6  / |
  //  | /  5  \   /  |
  //  |/       \ / 7 |
  //  3---------4----5 point row 2
  //  |\       / \ 3 |
  //  | \  1  /   \  |
  //  |  \   /     \ |
  //  | 0 \ /   2   \|
  //  0----1---------2 point row 1
  //
  VecPt3d points = {
    {0, 0, 0}, {1, 0, 0}, {3, 0, 0},  // row 1 of points
    {0, 1, 0}, {2, 1, 0}, {3, 1, 0},  // row 2 of points
    {0, 2, 0}, {1, 2, 0}, {3, 2, 0}}; // row 3 of points
  VecInt cells = {
    XMU_TRIANGLE, 3, 0, 1, 3, // row 1 of triangles
    XMU_TRIANGLE, 3, 1, 4, 3,
    XMU_TRIANGLE, 3, 1, 2, 4,
    XMU_TRIANGLE, 3, 2, 5, 4,

    XMU_TRIANGLE, 3, 3, 7, 6, // row 2 of triangles
    XMU_TRIANGLE, 3, 3, 4, 7,
    XMU_TRIANGLE, 3, 4, 8, 7,
    XMU_TRIANGLE, 3, 4, 5, 8};

  VecFlt cellScalars = {
    2, 4, 6, 8,  // row 1
    4, 6, 8, 10  // row 2
  };

  // extract value for each cell
  VecPt3d extractLocations = {
    {0.25, 0.25, 0}, // cell 0
    {1.00, 0.25, 0}, // cell 1
    {2.00, 0.50, 0}, // cell 2
    {2.75, 0.75, 0}, // cell 3
    {0.25, 1.75, 0}, // cell 4
    {1.00, 1.25, 0}, // cell 5
    {1.50, 1.75, 0}, // cell 6
    {2.75, 1.25, 0}  // cell 7
  };

  // expected results with point 4 inactive
  VecFlt expectedPerCell = {
    2.0f, 3.4444f, XM_NODATA, 6.75f, // row 1 cells
    3.5f, 5.7303f, 5.4652f, 8.25f    // row 2 cells
  };
  // clang-format on

  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);
  extractor->SetUseIdwForPointData(true);

  // set point 4 inactive
  // should cause all cells connected to point 4 to return XM_NODATA
  DynBitset cellActivity;
  cellActivity.resize(8, true);
  cellActivity[2] = false;
  extractor->SetGridCellScalars(cellScalars, cellActivity, LOC_CELLS);

  // extract interpolated scalar for each cell
  extractor->SetExtractLocations(extractLocations);

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  TS_ASSERT_DELTA_VEC(expectedPerCell, interpValues, 0.001);
} // XmUGrid2dDataExtractorUnitTests::testCellScalarCellActivityIdw
//------------------------------------------------------------------------------
/// \brief Test extractor when using point scalars and point activity.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testCellScalarPointActivity()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  DynBitset pointActivity;
  pointActivity.resize(4, true);
  pointActivity[1] = false;
  VecFlt cellScalars = {1, 2};
  extractor->SetGridCellScalars(cellScalars, pointActivity, LOC_POINTS);
  extractor->SetExtractLocations({{0.0, 0.0, 0.0},
                                  {0.25, 0.75, 100.0},
                                  {0.5, 0.5, 0.0},
                                  {0.75, 0.25, -100.0},
                                  {-1.0, -1.0, 0.0}});

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {2.0, 2.0, 2.0, XM_NODATA, XM_NODATA};
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testCellScalarPointActivity
//------------------------------------------------------------------------------
/// \brief Test extractor with cell scalars only.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testInvalidCellScalarsAndActivitySize()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  VecFlt cellScalars = {1};
  DynBitset activity;
  activity.push_back(false);
  extractor->SetGridCellScalars(cellScalars, activity, LOC_CELLS);
  VecPt3d extractLocations = {{0.25, 0.75, 100.0}, {0.75, 0.25, 0.0}};
  extractor->SetExtractLocations(extractLocations);

  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {0.0, XM_NODATA};
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testInvalidCellScalarsAndActivitySize
//------------------------------------------------------------------------------
/// \brief Test extractor going through time steps with cell and point scalars.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testChangingScalarsAndActivity()
{
  // build a grid with 3 cells in a row
  VecPt3d points = {{0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {3, 1, 0},
                    {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}};
  VecInt cells = {
    XMU_QUAD, 4, 0, 4, 5, 1, // cell 0
    XMU_QUAD, 4, 1, 5, 6, 2, // cell 1
    XMU_QUAD, 4, 2, 6, 7, 3  // cell 2
  };

  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  VecFlt scalars;
  DynBitset activity;
  VecPt3d extractLocations = {
    {0.75, 0.25, 0.0}, // cell 0
    {1.5, 0.5, 0.0},   // cell 1
    {2.25, 0.75, 0.0}  // cell 3
  };

  // timestep 1
  scalars = {1, 2, 3};
  // empty activity means all are enabled
  extractor->SetGridCellScalars(scalars, activity, LOC_CELLS);
  extractor->SetExtractLocations(extractLocations);

  VecFlt extractedValues;
  extractor->ExtractData(extractedValues);
  VecFlt expectedValues = {1.25, 2.0, 2.75};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);

  // timestep 2
  scalars = {2, 3, 4};
  activity.resize(3, true);
  activity[1] = false;
  extractor->SetGridCellScalars(scalars, activity, LOC_CELLS);
  extractor->SetExtractLocations(extractLocations);

  extractor->ExtractData(extractedValues);
  expectedValues = {2, XM_NODATA, 4};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);

  // timestep 3
  scalars = {3, 4, 5};
  activity.clear();
  extractor->SetGridCellScalars(scalars, activity, LOC_CELLS);
  extractor->SetExtractLocations(extractLocations);

  extractor->ExtractData(extractedValues);
  expectedValues = {3.25, 4.0, 4.75};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);

  // change to point data
  // timestep 1
  scalars = {1, 2, 3, 4, 2, 3, 4, 5};
  // empty activity means all are enabled
  extractor->SetGridPointScalars(scalars, activity, LOC_POINTS);
  extractor->SetExtractLocations(extractLocations);

  extractor->ExtractData(extractedValues);
  expectedValues = {2.5, 3.0, 3.5};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);

  // timestep 2
  scalars = {2, 3, 4, 5, 3, 4, 5, 6};
  activity.resize(8, true);
  activity[0] = false;
  extractor->SetGridPointScalars(scalars, activity, LOC_POINTS);
  extractor->SetExtractLocations(extractLocations);

  extractor->ExtractData(extractedValues);
  expectedValues = {XM_NODATA, 4.0, 4.5};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);

  // timestep 3
  scalars = {3, 4, 5, 6, 4, 5, 6, 7};
  activity.resize(8, true);
  activity[1] = false;
  extractor->SetGridPointScalars(scalars, activity, LOC_POINTS);
  extractor->SetExtractLocations(extractLocations);

  extractor->ExtractData(extractedValues);
  expectedValues = {XM_NODATA, XM_NODATA, 5.5};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);

  // timestep 4
  activity.clear();
  extractor->SetGridPointScalars(scalars, activity, LOC_POINTS);
  extractor->SetExtractLocations(extractLocations);

  extractor->ExtractData(extractedValues);
  expectedValues = {4.5, 5, 5.5};
  TS_ASSERT_EQUALS(expectedValues, extractedValues);
} // XmUGrid2dDataExtractorUnitTests::testChangingScalarsAndActivity
//------------------------------------------------------------------------------
/// \brief Test extractor built by copying triangles.
//------------------------------------------------------------------------------
void XmUGrid2dDataExtractorUnitTests::testCopiedExtractor()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);
  TS_ASSERT(extractor);

  VecFlt pointScalars = {1, 2, 3, 4};
  extractor->SetGridPointScalars(pointScalars, DynBitset(), LOC_POINTS);
  extractor->SetExtractLocations({{0.5, 0.5, 0.0}});
  VecFlt interpValues;
  extractor->ExtractData(interpValues);
  VecFlt expected = {2.0};
  TS_ASSERT_EQUALS(expected, interpValues);

  BSHP<XmUGrid2dDataExtractor> extractor2 = XmUGrid2dDataExtractor::New(extractor);
  extractor2->SetGridPointScalars(pointScalars, DynBitset(), LOC_POINTS);
  extractor2->SetExtractLocations({{0.5, 0.5, 0.0}});
  extractor2->ExtractData(interpValues);
  TS_ASSERT_EQUALS(expected, interpValues);
} // XmUGrid2dDataExtractorUnitTests::testCopiedExtractor
//------------------------------------------------------------------------------
/// \brief Test XmUGrid2dDataExtractor for tutorial.
//------------------------------------------------------------------------------
//! [snip_test_Example_TransientLocationExtractor]
void XmUGrid2dDataExtractorUnitTests::testTutorial()
{
  // build 2x3 grid
  VecPt3d points = {
    {288050, 3907770, 0}, {294050, 3907770, 0}, {300050, 3907770, 0},
    {306050, 3907770, 0}, {288050, 3901770, 0}, {294050, 3901770, 0},
    {300050, 3901770, 0}, {306050, 3901770, 0}, {288050, 3895770, 0},
    {294050, 3895770, 0}, {300050, 3895770, 0}, {306050, 3895770, 0}
  };
  VecInt cells = {
    XMU_QUAD, 4, 0, 4, 5, 1,
    XMU_QUAD, 4, 1, 5, 6, 2,
    XMU_QUAD, 4, 2, 6, 7, 3,
    XMU_QUAD, 4, 4, 8, 9, 5,
    XMU_QUAD, 4, 5, 9, 10, 6,
    XMU_QUAD, 4, 6, 10, 11, 7
  };
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  // Step 1. Create an extractor for an XmUGrid (call XmUGrid2dDataExtractor::New).
  BSHP<XmUGrid2dDataExtractor> extractor = XmUGrid2dDataExtractor::New(ugrid);

  // Step 2. Set extract locations (call XmUGrid2dDataExtractor::SetExtractLocations).
  VecPt3d extractLocations = {
    {289780, 3906220, 0},
    {293780, 3899460, 0},
    {298900, 3900780, 0},
    {301170, 3904960, 0},
    {296330, 3906180, 0},
    {307395, 3901463, 0}
  };
  extractor->SetExtractLocations(extractLocations);
  VecFlt extractedData;
  VecPt3d retrievedLocations = extractor->GetExtractLocations();
  TS_ASSERT_EQUALS(extractLocations, retrievedLocations);

  // Step 3. Optionally set the "no data" value for output interpolated values
  //         (XmUGrid2dDataExtractor::SetNoDataValue).
  extractor->SetNoDataValue(-999.0);

  // time step 1
  // Step 4. Set the point scalars for the first time step (XmUGrid2dDataExtractor::SetGridPointScalars).
  VecFlt pointScalars = {730.787f, 1214.54f, 1057.145f, 629.2069f, 351.1153f, 631.6649f, 1244.366f,
    449.9133f, 64.04247f, 240.9716f, 680.0491f, 294.9547f};
  extractor->SetGridPointScalars(pointScalars, DynBitset(), LOC_CELLS);
  // Step 5. Extract the data (call xms::XmUGrid2dDataExtractor::ExtractData).
  extractor->ExtractData(extractedData);

  VecFlt expectedData = {719.6f, 468.6f, 1033.8f, 996.5f, 1204.3f, -999.0f};
  TS_ASSERT_DELTA_VEC(expectedData, extractedData, 0.2);

  // time step 2
  // Step 6. Continue using steps 4 and 5 for remaining time steps.
  pointScalars = {-999.0f, 1220.5f, 1057.1f, 613.2f, 380.1f, 625.6f, 722.2f, 449.9f, 51.0f, 240.9f, 609.0f, 294.9f};
  DynBitset cellActivity;
  cellActivity.resize(ugrid->GetNumberOfCells(), true);
  cellActivity[0] = false;
  extractor->SetGridPointScalars(pointScalars, cellActivity, LOC_CELLS);
  // Step 7. Extract the data (call xms::XmUGrid2dDataExtractor::ExtractData).
  extractor->ExtractData(extractedData);

  expectedData = {-999.0f, 466.4f, 685.0f, 849.4f, 1069.6f, -999.0f};
  TS_ASSERT_DELTA_VEC(expectedData, extractedData, 0.2);
} // XmUGrid2dDataExtractorUnitTests::testTutorial
//! [snip_test_Example_TransientLocationExtractor]

#endif
