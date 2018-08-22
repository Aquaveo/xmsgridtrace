//------------------------------------------------------------------------------
/// \file
/// \ingroup extractor
/// \copyright (C) Copyright Aquaveo 2018.
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------

// 1. Precompiled header

// 2. My own header
#include <xmsgridtrace/gridtrace/XmUGrid2dPolylineDataExtractor.h>

// 3. Standard library headers
#include <sstream>

// 4. External library headers

// 5. Shared code headers
#include <xmscore/misc/XmLog.h>
#include <xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.h>
#include <xmsgridtrace/gridtrace/XmUGridTriangles2d.h>
#include <xmsgrid/ugrid/XmUGrid.h>
#include <xmsinterp/geometry/GmMultiPolyIntersector.h>
#include <xmsinterp/geometry/GmMultiPolyIntersectionSorterTerse.h>

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

////////////////////////////////////////////////////////////////////////////////
/// Implementation for XmUGrid2dPolylineDataExtractor
class XmUGrid2dPolylineDataExtractorImpl : public XmUGrid2dPolylineDataExtractor
{
public:
  XmUGrid2dPolylineDataExtractorImpl(BSHP<XmUGrid> a_ugrid,
                                     DataLocationEnum a_scalarLocation);

  virtual void SetGridScalars(const VecFlt& a_pointScalars,
                              const DynBitset& a_activity,
                              DataLocationEnum a_activityLocation) override;

  virtual void SetPolyline(const VecPt3d& a_polyline) override;
  virtual const VecPt3d& GetExtractLocations() const override;
  virtual void ExtractData(VecFlt& a_extractedData) override;

  virtual void ComputeLocationsAndExtractData(const VecPt3d& a_polyline,
                                              VecFlt& a_extractedData,
                                              VecPt3d& a_extractedLocations) override;

  virtual void SetUseIdwForPointData(bool a_useIdw) override;
  virtual void SetNoDataValue(float a_noDataValue) override;

private:
  void ComputeExtractLocations(const VecPt3d& a_polyline, VecPt3d& a_locations);
  DataLocationEnum m_scalarLocation;        ///< The location of the scalars (points or cells).
  BSHP<XmUGrid> m_ugrid;                    ///< The ugrid which holds the points and the grid.
  
  BSHP<XmUGrid2dDataExtractor> m_extractor; ///< The data extractor.
  mutable BSHP<GmMultiPolyIntersector> m_multiPolyIntersector; ///< The intersection tool used from
                                                       ///   xmsinterp to find the intersections
                                                       ///   with the polyline.
};

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGrid2dPolylineDataExtractorImpl
/// \brief Implementation for XmUGrid2dDataExtractor which provides ability
///        to extract dataset values along arcs for an unstructured grid.
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Construct from a UGrid.
/// \param[in] a_ugrid The UGrid to construct an extractor for.
/// \param[in] a_scalarLocation The location of the scalars (points or cells).
//------------------------------------------------------------------------------
XmUGrid2dPolylineDataExtractorImpl::XmUGrid2dPolylineDataExtractorImpl(BSHP<XmUGrid> a_ugrid,
                                                                       DataLocationEnum a_scalarLocation)
: m_ugrid(a_ugrid)
, m_scalarLocation(a_scalarLocation)
, m_extractor(XmUGrid2dDataExtractor::New(a_ugrid))
{
  if (a_scalarLocation == LOC_UNKNOWN)
  {
    XM_LOG(xmlog::error, "Scalar locations are unknown in polyline extractor.");
    m_scalarLocation = LOC_POINTS;
  }
} // XmUGrid2dPolylineDataExtractorImpl::XmUGrid2dPolylineDataExtractorImpl
//------------------------------------------------------------------------------
/// \brief Setup point scalars to be used to extract interpolated data.
/// \param[in] a_scalars The cell or point scalars.
/// \param[in] a_activity The activity of the points or cells.
/// \param[in] a_activityLocation The location of the activity (points or cells).
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::SetGridScalars(const VecFlt& a_scalars,
                                                        const DynBitset& a_activity,
                                                        DataLocationEnum a_activityLocation)
{
  if (m_scalarLocation == LOC_POINTS)
    m_extractor->SetGridPointScalars(a_scalars, a_activity, a_activityLocation);
  else if (m_scalarLocation == LOC_CELLS)
    m_extractor->SetGridCellScalars(a_scalars, a_activity, a_activityLocation);
} // XmUGrid2dPolylineDataExtractorImpl::SetGridScalars
//------------------------------------------------------------------------------
/// \brief Set the polyline along which to extract the scalar data. Locations
///        crossing cell boundaries are computed along the polyline.
/// \param[in] a_polyline The polyline.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::SetPolyline(const VecPt3d& a_polyline)
{
  m_extractor->BuildTriangles(m_scalarLocation);
  VecPt3d locations;
  ComputeExtractLocations(a_polyline, locations);
  m_extractor->SetExtractLocations(locations);
} // XmUGrid2dPolylineDataExtractorImpl::SetPolyline
//------------------------------------------------------------------------------
/// \brief Gets computed locations along polyline to extract interpolated scalar
///        data from.
/// \return The locations.
//------------------------------------------------------------------------------
const VecPt3d& XmUGrid2dPolylineDataExtractorImpl::GetExtractLocations() const
{
  return m_extractor->GetExtractLocations();
} // XmUGrid2dPolylineDataExtractorImpl::GetExtractLocations
//------------------------------------------------------------------------------
/// \brief \brief Extract data at previously computed locations returned by
///               GetExtractLocations.
/// \param[out] a_extractedData The extracted values interpolated from the
///             scalar values.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::ExtractData(VecFlt& a_extractedData)
{
  m_extractor->ExtractData(a_extractedData);
} // XmUGrid2dPolylineDataExtractorImpl::ExtractData
//------------------------------------------------------------------------------
/// \brief Extracts a scalar profile for the UGrid across the given polyline.
/// \param[in] a_polyline The polyline to extract the data across.
/// \param[out] a_extractedData The extracted values interpolated from the
///             scalar values.
/// \param[out] a_extractedLocations The locations of the extracted data values.
///             The polyline will get split up into locations across the UGrid.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::ComputeLocationsAndExtractData(const VecPt3d& a_polyline,
                                                                     VecFlt& a_extractedData,
                                                                     VecPt3d& a_extractedLocations)
{
  SetPolyline(a_polyline);
  ExtractData(a_extractedData);
  a_extractedLocations = GetExtractLocations();
} // XmUGrid2dPolylineDataExtractorImpl::ComputeLocationsAndExtractData
//------------------------------------------------------------------------------
/// \brief Set to use IDW to calculate point scalar values from cell scalars.
/// \param a_useIdw Whether to turn IDW on or off.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::SetUseIdwForPointData(bool a_useIdw)
{
  m_extractor->SetUseIdwForPointData(a_useIdw);
} // XmUGrid2dPolylineDataExtractorImpl::SetUseIdwForPointData
//------------------------------------------------------------------------------
/// \brief Set value to use when extracted value is in inactive cell or doesn't
///        intersect with the grid.
/// \param[in] a_value The no data value
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::SetNoDataValue(float a_value)
{
  m_extractor->SetNoDataValue(a_value);
} // XmUGrid2dPolylineDataExtractorImpl::SetNoDataValue
//------------------------------------------------------------------------------
/// \brief Compute locations to extract scalar values from across a polyline.
/// \param[in] a_polyline The line used to calculate the extraction points.
/// \param[out] a_locations The points at which will the data will be extracted.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorImpl::ComputeExtractLocations(const VecPt3d& a_polyline,
                                                                 VecPt3d& a_locations)
{
  a_locations.clear();
  if (a_polyline.empty())
  {
    XM_LOG(xmlog::error, "Attempting to extract polyline profile with empty polyline.");
    return;
  }

  if (!m_extractor->GetUGridTriangles())
  {
    XM_LOG(xmlog::error, "Attempting to extract polyline profile without setting scalars.");
    return;
  }

  if (!m_multiPolyIntersector)
  {
    const VecPt3d& points = m_extractor->GetUGridTriangles()->GetPoints();
    const VecInt& triangles = m_extractor->GetUGridTriangles()->GetTriangles();
    VecInt2d polygons;
    VecInt triangle;
    for (size_t triangleIdx = 0; triangleIdx <triangles.size(); triangleIdx += 3)
    {
      triangle = { triangles[triangleIdx], triangles[triangleIdx+1], triangles[triangleIdx+2] };
      polygons.push_back(triangle);
    }

    BSHP<GmMultiPolyIntersectionSorter> sorter(new GmMultiPolyIntersectionSorterTerse());
    m_multiPolyIntersector = GmMultiPolyIntersector::New(points, polygons, sorter, 0);
  }

  // get points crossing cell edges
  Pt3d lastPoint = a_polyline[0];
  a_locations.push_back(lastPoint);

  for (size_t polyIdx = 1; polyIdx < a_polyline.size(); ++polyIdx)
  {
    Pt3d pt1 = a_polyline[polyIdx - 1];
    Pt3d pt2 = a_polyline[polyIdx];
    VecInt intersectIdxs;
    VecPt3d intersectPts;

    m_multiPolyIntersector->TraverseLineSegment(pt1.x, pt1.y, pt2.x, pt2.y, intersectIdxs,
                                                intersectPts);

    if (intersectIdxs.size() != intersectPts.size())
    {
      XM_LOG(xmlog::error, "Internal error when extracting polyline profile.");
      return;
    }

    for (size_t i = 0; i < intersectIdxs.size(); ++i)
    {
      const Pt3d& currPoint = intersectPts[i];
      if (lastPoint != currPoint)
      {
        if (i != 0 && intersectIdxs[i - 1] == -1)
        {
          a_locations.push_back((currPoint + lastPoint)/2.0);
        }
        a_locations.push_back(currPoint);
        lastPoint = currPoint;
      }
    }

    if (pt2 != a_locations.back())
    {
      a_locations.push_back(pt2);
      lastPoint = pt2;
    }
  }
} // XmUGrid2dPolylineDataExtractorImpl::ComputeExtractLocations

} // namespace

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGrid2dPolylineDataExtractor
/// \brief Provides ability to interpolate and extract the scalar values along a
///        polyline for an unstructured grid.
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Create a new XmUGrid2dPolylineDataExtractor.
/// \param[in] a_ugrid The UGrid geometry to use to extract values from
/// \param[in] a_scalarLocation The location of the scalars (points or cells).
/// \return the new XmUGrid2dPolylineDataExtractor
//------------------------------------------------------------------------------
BSHP<XmUGrid2dPolylineDataExtractor> XmUGrid2dPolylineDataExtractor::New(BSHP<XmUGrid> a_ugrid,
                                                                 DataLocationEnum a_scalarLocation)
{
  BSHP<XmUGrid2dPolylineDataExtractor> extractor(new XmUGrid2dPolylineDataExtractorImpl(a_ugrid,
    a_scalarLocation));
  return extractor;
} // XmUGrid2dPolylineDataExtractor::New
//------------------------------------------------------------------------------
/// \brief Constructor
//------------------------------------------------------------------------------
XmUGrid2dPolylineDataExtractor::XmUGrid2dPolylineDataExtractor()
{
} // XmUGrid2dPolylineDataExtractor::XmUGrid2dPolylineDataExtractor
//------------------------------------------------------------------------------
/// \brief Destructor
//------------------------------------------------------------------------------
XmUGrid2dPolylineDataExtractor::~XmUGrid2dPolylineDataExtractor()
{
} // XmUGrid2dPolylineDataExtractor::~XmUGrid2dPolylineDataExtractor

} // namespace xms

#ifdef CXX_TESTx
//------------------------------------------------------------------------------
// Unit Tests
//------------------------------------------------------------------------------
using namespace xms;
#include <xmsgridtrace/gridtrace/XmUGrid2dPolylineDataExtractor.t.h>
#include <xmsgrid/ugrid/XmUGrid.t.h>

#include <xmscore/misc/xmstype.h>
#include <xmscore/testing/TestTools.h>
#include <xmsinterp/geometry/geoms.h>

////////////////////////////////////////////////////////////////////////////////
/// \class XmUGrid2dPolylineDataExtractorUnitTests
/// \brief Class to to test XmUGrid2dPolylineDataExtractor
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Test extractor with point scalars only.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testOneCellOneSegment()
{
  // clang-format off
  //     3-------2
  //     |       |
  // 0===============1
  //     |       |
  //     0-------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-1, 0.5, 0}, {2, 0.5, 0}};
  extractor->SetPolyline(polyline);
  extractedLocations = extractor->GetExtractLocations();
  extractor->ExtractData(extractedData);

  VecFlt expectedData = {XM_NODATA, 0.5, 1.5, 2.5, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{-1, 0.5, 0}, {0.0, 0.5, 0.0}, {0.5, 0.5, 0.0}, {1.0, 0.5, 0.0}, {2, 0.5, 0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testOneCellOneSegment
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment all inside of cell.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAllInCell()
{
  // clang-format off
  // (1)  3--------2  (3)
  //      |        |
  //      | 0====1 |
  //      |        |
  // (0)  0--------1  (2)
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{0.25, 0.50, 0.0}, {0.75, 0.50, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {1.0, 1.5, 2.0};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{0.25, 0.5, 0.0}, {0.5, 0.5, 0.0}, {0.75, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAllInCell
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment along an edge.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAlongEdge()
{
  // clang-format off
  //  0===3========2===1
  //      |        |
  //      |        |
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-0.5, 1.0, 0.0}, {1.55, 1.0, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 1.0, 3.0, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{-0.5, 1.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 1.0, 0.0}, {1.55, 1.0, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAlongEdge
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment all outside of cell.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAllOutside()
{
  // clang-format off
  //      3========2
  //      |        |
  // 0==1 |        |
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-0.5, 0.5, 0.0}, {-0.25, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{-0.5, 0.5, 0.0}, {-0.25, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAllOutside
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment with endpoint touching cell.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentOutToTouch()
{
  // clang-format off
  //      3========2
  //      |        |
  // 0====1        |
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-0.5, 0.5, 0.0}, {0.0, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 0.5};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{-0.5, 0.5, 0.0}, {0.0, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentOutToTouch
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment with first point touching edge.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentTouchToOut()
{
  // clang-format off
  //      3========2
  //      |        |
  //      |        0===1
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{1.0, 0.5, 0.0}, {1.5, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {2.5, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{1.0, 0.5, 0.0}, {1.5, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentTouchToOut
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment touching cell point.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentCrossCellPoint()
{
  // clang-format off
  //        1
  //       /
  //      3========2
  //    / |        |
  //   0  |        |
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-0.5, 0.5, 0.0}, {0.0, 1.0, 0.0}, {0.5, 1.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 1.0, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{-0.5, 0.5, 0.0}, {0.0, 1.0, 0.0}, {0.5, 1.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentCrossCellPoint
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment crossing first cell into second.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAcrossCellIntoSecond()
{
  // clang-format off
  //      3========2========5
  //      |        |        |
  //   0===============1    |
  //      |        |        |
  //      0--------1--------4
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {2, 0, 0}, {2, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 1, 4, 5, 2};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1, 4, 5};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-0.5, 0.5, 0.0}, {1.5, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 0.5, 1.5, 2.5, 3.5};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{-0.5, 0.5, 0.0}, {0.0, 0.5, 0.0}, {0.5, 0.5, 0.0},
    {1.0, 0.5, 0.0}, {1.5, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAcrossCellIntoSecond
//------------------------------------------------------------------------------
/// \brief Test extractor with single segment going across unconnected cells.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAcrossSplitCells()
{
  // clang-format off
  //       3-------2       7-------6
  //       |       |       |       |
  //  0========================1   |
  //       |       |       |       |
  //       0-------1       4-------5
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                    {2, 0, 0}, {3, 0, 0}, {3, 1, 0}, {2, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 4, 5, 6, 7};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1, 4, 6, 7, 5};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-1, 0.5, 0}, {2.5, 0.5, 0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 0.5, 1.5, 2.5, XM_NODATA, 4.5, 5.5};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{-1.0, 0.5, 0.0}, {0.0, 0.5, 0.0}, {0.5, 0.5, 0.0},
                               {1.0, 0.5, 0.0}, {1.5, 0.5, 0.0}, {2.0, 0.5, 0.0},
                               {2.5, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testSegmentAcrossSplitCells
//------------------------------------------------------------------------------
/// \brief Test extractor with two segments going across a single cell.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsAcrossOneCell()
{
  // clang-format off
  //      3--------2
  //      |        |
  // 0=========1=========2
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-1, 0.5, 0}, {0.5, 0.5, 0.0}, {2, 0.5, 0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 0.5, 1.5, 2.5, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {
    {-1.0, 0.5, 0.0}, {0.0, 0.5, 0.0}, {0.5, 0.5, 0.0}, {1.0, 0.5, 0.0}, {2.0, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsAcrossOneCell
//------------------------------------------------------------------------------
/// \brief Test extractor with two segments all outside of cell.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsAllOutside()
{
  // clang-format off
  //      3--------2
  //      |        |
  //      |        | 0====1====2
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{2.0, 0.5, 0}, {3.0, 0.5, 0.0}, {4.0, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, XM_NODATA, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{2.0, 0.5, 0}, {3.0, 0.5, 0.0}, {4.0, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsAllOutside
//------------------------------------------------------------------------------
/// \brief Test extractor with two segments: first in to outside, second outside.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsFirstExiting()
{
  // clang-format off
  //      3--------2
  //      |        |
  //      |   0========1=======2
  //      |        |
  //      0--------1
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{0.5, 0.5, 0}, {3.0, 0.5, 0.0}, {4.0, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {1.5, 2.5, XM_NODATA, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{0.5, 0.5, 0}, {1.0, 0.5, 0}, {3.0, 0.5, 0.0}, {4.0, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsFirstExiting
//------------------------------------------------------------------------------
/// \brief Test extractor with  two segments joining in a cell.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsJoinInCell()
{
  // clang-format off
  //      3========2========5
  //      |        |        |
  //      |   0========1=========2
  //      |        |        |
  //      0--------1--------4
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {2, 0, 0}, {2, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 1, 4, 5, 2};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1, 4, 5};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{0.5, 0.5, 0.0}, {1.5, 0.5, 0.0}, {2.5, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {1.5, 2.5, 3.5, 4.5, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{0.5, 0.5, 0.0}, {1.0, 0.5, 0.0}, {1.5, 0.5, 0.0},
    {2.0, 0.5, 0.0}, {2.5, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsJoinInCell
//------------------------------------------------------------------------------
/// \brief Test extractor with two segments joining on two cell boundary.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsJoinOnBoundary()
{
  // clang-format off
  //      3========2========5
  //      |        |        |
  //      |   0====1=============2
  //      |        |        |
  //      0--------1--------4
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {2, 0, 0}, {2, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 1, 4, 5, 2};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1, 4, 5};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{0.5, 0.5, 0.0}, {1.0, 0.5, 0.0}, {2.5, 0.5, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {1.5, 2.5, 3.5, 4.5, XM_NODATA};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{0.5, 0.5, 0.0}, {1.0, 0.5, 0.0}, {1.5, 0.5, 0.0},
    {2.0, 0.5, 0.0}, {2.5, 0.5, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsJoinOnBoundary
//------------------------------------------------------------------------------
/// \brief Test extractor with three segments two crossing at boundary.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testThreeSegmentsCrossOnBoundary()
{
  // clang-format off
  //      3----3---2
  //      |      \ |
  //      |   0========1
  //      |        | \ |
  //      0--------1   2
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  VecFlt pointScalars = {0, 2, 3, 1};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_POINTS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{0.5, 0.5, 0}, {1.5, 0.5, 0.0}, {1.5, 0.0, 0.0}, {0.5, 1.0, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {1.5, 2.5, XM_NODATA, XM_NODATA, 2.5, 2.25, 2.0};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations = {{0.5, 0.5, 0}, {1.0, 0.5, 0}, {1.5, 0.5, 0.0},
    {1.5, 0.0, 0.0}, {1.0, 0.5, 0}, {0.75, 0.75, 0.0}, {0.5, 1.0, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTwoSegmentsJoinOnBoundary
//------------------------------------------------------------------------------
/// \brief Test extractor with cell scalars.
//------------------------------------------------------------------------------
void XmUGrid2dPolylineDataExtractorUnitTests::testCellScalars()
{
  // clang-format off
  //      3========2========5
  //      |        |        |
  //   0===============1    |
  //      |        |        |
  //      0--------1--------4
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {2, 0, 0}, {2, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 1, 4, 5, 2};
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_CELLS);

  VecFlt cellScalars = {1, 2};
  extractor->SetGridScalars(cellScalars, DynBitset(), LOC_CELLS);

  VecFlt extractedData;
  VecPt3d extractedLocations;
  VecPt3d polyline = {{-0.5, 0.75, 0.0}, {1.5, 0.75, 0.0}};
  extractor->ComputeLocationsAndExtractData(polyline, extractedData, extractedLocations);

  VecFlt expectedData = {XM_NODATA, 1.0, 1.0, 1.25, 1.5, 1.75, 1.875};
  TS_ASSERT_EQUALS(expectedData, extractedData);
  VecPt3d expectedLocations =  {{-0.5, 0.75, 0.0}, {0.0, 0.75, 0.0}, {0.25, 0.75, 0.0},
    {0.75, 0.75, 0.0}, {1., 0.75, 0.0}, {1.25, 0.75, 0.0}, {1.5, 0.75, 0.0}};
  TS_ASSERT_EQUALS(expectedLocations, extractedLocations);
} // XmUGrid2dPolylineDataExtractorUnitTests::testCellScalars
//------------------------------------------------------------------------------
/// \brief Test XmUGrid2dPolylineDataExtractor for tutorial with transient data.
//------------------------------------------------------------------------------
//! [snip_test_Example_TransientPolylineExtractor]
void XmUGrid2dPolylineDataExtractorUnitTests::testTransientTutorial()
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
  // Step 1. Create an extractor for an XmUGrid giving the mapped location of the scalar values
  BSHP<XmUGrid2dPolylineDataExtractor> extractor = XmUGrid2dPolylineDataExtractor::New(ugrid, LOC_POINTS);

  // Step 2. Optionally set the "no data" value.
  extractor->SetNoDataValue(-999.0);

  // Step 3. Set the polyline to be extracted along.
  VecPt3d extractedLocations;
  VecPt3d polyline = {
    {290764, 3895106, 0}, {291122, 3909108, 0},
    {302772, 3909130, 0}, {302794, 3895775, 0}
  };
  extractor->SetPolyline(polyline);

  // Step 4. Optionally get the locations used for extraction along the polyline.
  extractedLocations = extractor->GetExtractLocations();
  VecPt3d expectedLocations = {
    {290764.0, 3895106.0, 0.0},
    {290780.9, 3895770.0, 0.0},
    {290862.4, 3898957.5, 0.0},
    {290934.3, 3901770.0, 0.0},
    {291012.0, 3904807.9, 0.0},
    {291087.7, 3907770.0, 0.0},
    {291122.0, 3909108.0, 0.0},
    {302772.0, 3909130.0, 0.0},
    {302774.2, 3907770.0, 0.0},
    {302778.7, 3905041.2, 0.0},
    {302784.1, 3901770.0, 0.0},
    {302788.6, 3899031.3, 0.0},
    {302794.0, 3895775.0, 0.0},
  };
  TS_ASSERT_DELTA_VECPT3D(expectedLocations, extractedLocations, 0.15);

  VecFlt extractedData;

  // time step 1
  // Step 5. Set the point scalars for the first time step.
  VecFlt pointScalars = {
    730.787f, 1214.54f, 1057.145f, 629.2069f, 351.1153f, 631.6649f,
    1244.366f, 449.9133f, 64.04247f, 240.9716f, 680.0491f, 294.9547f
  };
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_CELLS);
  // Step 6. Extract the data.
  extractor->ExtractData(extractedData);
  VecFlt expectedData = {-999.0f, 144.5f, 299.4f, 485.9f, 681.8f,
                         975.7f, -999.0f, -999.0f, 862.8f, 780.9f,
                         882.3f, 811.0f, 504.4f};
  TS_ASSERT_DELTA_VEC(expectedData, extractedData, 0.2);

  // time step 2
  // Step 7. Continue using steps 5 and 6 for remaining time steps.
  pointScalars = {-999.0f, 1220.5f, 1057.1f, 613.2f, 380.1f, 625.6f, 722.2f, 449.9f, 51.0f, 240.9f,
                  609.0f, 294.9f};
  extractor->SetGridScalars(pointScalars, DynBitset(), LOC_CELLS);
  extractor->ExtractData(extractedData);
  expectedData = {-999.0f, 137.4f, 314.8f, 498.1f, -196.9f, 124.7f, -999.0f, -999.0f, 855.5f,
    780.9f, 598.1f, 527.1f, 465.4f};
  TS_ASSERT_DELTA_VEC(expectedData, extractedData, 0.2);
} // XmUGrid2dPolylineDataExtractorUnitTests::testTransientTutorial
//! [snip_test_Example_TransientPolylineExtractor]

#endif
