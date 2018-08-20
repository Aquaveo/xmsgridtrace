//------------------------------------------------------------------------------
/// \file
/// \ingroup extractor
/// \copyright (C) Copyright Aquaveo 2018.
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------

// 1. Precompiled header

// 2. My own header
#include <xmsgridtrace/gridtrace/XmGridTrace.h>

// 3. Standard library headers
#include <sstream>

// 4. External library headers

// 5. Shared code headers
#include <xmscore/misc/XmError.h>
#include <xmscore/misc/XmLog.h>
#include <xmscore/math/math.h>
#include <xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.h>
#include <xmscore/misc/xmstype.h> // XM_ZERO_TOL
//#include <xmsinterp/geometry/geoms.h>
//#include <xmsinterp/geometry/GmTriSearch.h>
//#include <xmsinterp/interpolate/InterpUtil.h>

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
bool extract(XmUGrid2dDataExtractor& a_extractorX, XmUGrid2dDataExtractor& a_extractorY, const Pt3d& a_pt, Pt3d& a_data)
{
  VecPt3d loc;
  loc.push_back(a_pt);
  a_extractorX.SetExtractLocations(loc);
  a_extractorY.SetExtractLocations(loc);
  VecFlt dataOutx;
  VecFlt dataOuty;
  a_extractorX.ExtractData(dataOutx);
  a_extractorY.ExtractData(dataOuty);
  if (dataOutx.size() != 1 || dataOuty.size() != 1)
  {
    XM_LOG(xmlog::error, "Extracted data had an error");
    return false;
  }
  a_data.x = dataOutx[0];
  a_data.y = dataOuty[0];
  return true;
}
//----- Class / Function definitions -------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// Implementation for XmUGrid2dDataExtractor
class XmGridTraceImpl : public XmGridTrace
{
public:
  XmGridTraceImpl(BSHP<XmUGrid> a_ugrid);
  ~XmGridTraceImpl() {};

  double GetVectorMultiplier() const final;
  void SetVectorMultiplier(const double a_vectorMultiplier) final;

  double GetMaxTracingTimeSeconds() const final;
  void SetMaxTracingTimeSeconds(const double a_maxTracingTime) final;

  double GetMaxTracingDistanceMeters() const final;
  void SetMaxTracingDistanceMeters(const double a_maxTracingDistance) final;

  double GetMinDeltaTimeSeconds() const final;
  void SetMinDeltaTimeSeconds(const double a_minDeltaTime) final;

  double GetMaxChangeDistanceMeters() const final;
  void SetMaxChangeDistanceMeters(const double a_maxChangeDistance) final;

  double GetMaxChangeVelocityMetersPerSecond() const final;
  void SetMaxChangeVelocityMetersPerSecond(const double a_maxChangeVelocity) final;

  double GetMaxChangeDirectionInRadians() const final;
  void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) final;

  void AddGridScalarsAtTime(const VecPt3d& a_scalars, DataLocationEnum a_scalarLoc, xms::DynBitset& a_activity, DataLocationEnum a_activityLoc, double a_time) final;

  void TracePoint(const Pt3d& a_pt, const double& a_ptTime, VecPt3d& a_outTrace, VecDbl& a_outTimes) final;

private:

  BSHP<XmUGrid> m_ugrid;
  double m_vectorMultiplier; //a_vectorMultiplier
  double m_maxTracingTimeSeconds; // a_exitSpecifiedTime?
  double m_maxTracingDistanceMeters; // a_exitDistance?
  double m_minDeltaTimeSeconds; // a_minDeltaT
  double m_maxChangeDistanceMeters;// a_maxChangeDist
  double m_maxChangeVelocityMetersPerSecond; // a_maxChangeVel
  double m_maxChangeDirectionInRadians; // a_maxChangeDir

  BSHP<XmUGrid2dDataExtractor> m_extractor1x;
  BSHP<XmUGrid2dDataExtractor> m_extractor1y;
  double m_time1;
  BSHP<XmUGrid2dDataExtractor> m_extractor2x;
  BSHP<XmUGrid2dDataExtractor> m_extractor2y;
  double m_time2;
  double m_distTraveled;

protected:
  XmGridTraceImpl();
};

//------------------------------------------------------------------------------
/// \brief Construct a new XmGridTrace using a UGrid.
/// \param[in] a_ugrid The UGrid to construct a grid trace for
//------------------------------------------------------------------------------
XmGridTraceImpl::XmGridTraceImpl(BSHP<XmUGrid> a_ugrid)
  : m_ugrid(a_ugrid)
{}

//------------------------------------------------------------------------------
/// \brief Construct from a new XmGridTrace using a UGrid.
/// \param[in] a_ugrid The UGrid to construct a grid trace for
//------------------------------------------------------------------------------
BSHP<XmGridTrace> XmGridTrace::New(BSHP<XmUGrid> a_ugrid)
{
  return BSHP<XmGridTraceImpl>(new XmGridTraceImpl(a_ugrid));
} // XmGridTrace::New

////////////////////////////////////////////////////////////////////////////////
/// \class XmGridTraceImpl
/// \brief Implementation for XmGridTrace
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Returns the vector multiplier
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetVectorMultiplier() const
{
  return m_vectorMultiplier;
} // XmGridTraceImpl::GetVectorMultiplier
//------------------------------------------------------------------------------
/// \brief Sets the vector multiplier
/// \param[in] a_vectorMultiplier the new vector multiplier
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetVectorMultiplier(const double a_vectorMultiplier)
{
  m_vectorMultiplier = a_vectorMultiplier;
} // XmGridTraceImpl::GetVectorMultiplier
//------------------------------------------------------------------------------
/// \brief Returns the Max Tracing Time in seconds
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxTracingTimeSeconds() const
{
  return m_maxTracingTimeSeconds;
} // XmGridTraceImpl::GetMaxTracingTimeSeconds
//------------------------------------------------------------------------------
/// \brief Sets the max tracing time in seconds
/// \param[in] a_maxTracingTime the new max tracing time in seconds
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxTracingTimeSeconds(const double a_maxTracingTime)
{
  m_maxTracingTimeSeconds = a_maxTracingTime;
} // XmGridTraceImpl::SetMaxTracingTimeSeconds
//------------------------------------------------------------------------------
/// \brief Returns the Max Tracing Distance in meters
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxTracingDistanceMeters() const
{
  return m_maxTracingDistanceMeters;
} // XmGridTraceImpl::GetMaxTracingDistanceMeters
//------------------------------------------------------------------------------
/// \brief Sets the max tracing distance in meters
/// \param[in] a_maxTracingDistance the new max tracing distance in meters
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxTracingDistanceMeters(const double a_maxTracingDistance)
{
  m_maxTracingDistanceMeters = a_maxTracingDistance;
} // XmGridTraceImpl::SetMaxTracingDistanceMeters
//------------------------------------------------------------------------------
/// \brief Returns the min delta time in seconds
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMinDeltaTimeSeconds() const
{
  return m_minDeltaTimeSeconds;
} // XmGridTraceImpl::GetMinDeltaTimeSeconds
//------------------------------------------------------------------------------
/// \brief Sets the min delta time in seconds
/// \param[in] a_minDeltaTime the new min delta time in seconds
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMinDeltaTimeSeconds(const double a_minDeltaTime)
{
  m_minDeltaTimeSeconds = a_minDeltaTime;
} // XmGridTraceImpl::SetMinDeltaTimeSeconds
//------------------------------------------------------------------------------
/// \brief Returns the max change distance in meters
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxChangeDistanceMeters() const
{
  return m_maxChangeDistanceMeters;
} // XmGridTraceImpl::GetMaxChangeDistanceMeters
//------------------------------------------------------------------------------
/// \brief Sets the max change distance in meters
/// \param[in] a_maxChangeDistance the new max change distance in meters
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxChangeDistanceMeters(const double a_maxChangeDistance)
{
  m_maxChangeDistanceMeters = a_maxChangeDistance;
} // XmGridTraceImpl::SetMaxChangeDistanceMeters
//------------------------------------------------------------------------------
/// \brief Returns the max change in velcoity in meters per second
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxChangeVelocityMetersPerSecond() const
{
  return m_maxChangeVelocityMetersPerSecond;
} // XmGridTraceImpl::GetMaxChangeVelocityMetersPerSecond
//------------------------------------------------------------------------------
/// \brief Sets the max change in velocity in meters per seconds
/// \param[in] a_maxChangeVelocity the new max change in velocity in meters per second
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxChangeVelocityMetersPerSecond(const double a_maxChangeVelocity)
{
  m_maxChangeVelocityMetersPerSecond = a_maxChangeVelocity;
} // XmGridTraceImpl::SetMaxChangeVelocityMetersPerSecond
//------------------------------------------------------------------------------
/// \brief Returns the max change in direction in radians
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxChangeDirectionInRadians() const
{
  return m_maxChangeDirectionInRadians;
} // XmGridTraceImpl::GetMaxChangeDirectionInRadians
//------------------------------------------------------------------------------
/// \brief Sets the max change in direction in radians
/// \param[in] a_maxChangeVelocity the new max change in direction in radians
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxChangeDirectionInRadians(const double a_maxChangeDirection)
{
  m_maxChangeDirectionInRadians = a_maxChangeDirection;
} // XmGridTraceImpl::SetMaxChangeDirectionInRadians
//------------------------------------------------------------------------------
/// \brief 
/// \param[in] a_scalars
/// \param[in] a_scalarLoc
/// \param[in] a_activity
/// \param[in] a_activityLoc
/// \param[in] a_time
//------------------------------------------------------------------------------
void XmGridTraceImpl::AddGridScalarsAtTime(const VecPt3d& a_scalars, DataLocationEnum a_scalarLoc, xms::DynBitset& a_activity, DataLocationEnum a_activityLoc, double a_time)
{
  if (m_extractor1x&&m_extractor1y) {
    m_extractor2x = XmUGrid2dDataExtractor::New(m_extractor1x);
    m_extractor2y = XmUGrid2dDataExtractor::New(m_extractor1y);
    m_time2 = m_time1;
  }
  m_extractor1x = XmUGrid2dDataExtractor::New(m_ugrid);
  m_extractor1y = XmUGrid2dDataExtractor::New(m_extractor1x);
  
  m_time1 = a_time;
  std::vector<float> xx, yy;
  for (auto &pt : a_scalars) {
    xx.push_back((float)pt.x);
    yy.push_back((float)pt.y);
  }
  if (a_scalarLoc == DataLocationEnum::LOC_POINTS)
  {
    m_extractor1x->SetGridPointScalars(xx, a_activity, a_activityLoc);
    m_extractor1y->SetGridPointScalars(yy, a_activity, a_activityLoc);
  }
  else
  {
    m_extractor1x->SetGridCellScalars(xx, a_activity, a_activityLoc);
    m_extractor1y->SetGridCellScalars(yy, a_activity, a_activityLoc);
  }
}

//------------------------------------------------------------------------------
/// \brief Runs the Grid Trace for a point
/// \param[in] a_pt The point to process
/// \param[out] a_outTrace the resultant trace
//------------------------------------------------------------------------------
void XmGridTraceImpl::TracePoint(const Pt3d& a_pt, const double& a_ptTime, VecPt3d& a_outTrace, VecDbl& a_outTimes)
{
  double deltaT = 1.00; // seconds
  double mag0 = 0, mag1 = 0;
  Pt3d pt0 = a_pt, pt1;
  double vx0 = 0, vx1 = 0, vy0 = 0, vy1 = 0, elapsedTime = 0;
  bool bContinue = true;
  Pt3d vtkVec; // Rename this variable
  Pt3d vtkPt;
  Pt3d vector;

  m_distTraveled = 0;
  
  if (!extract(*m_extractor1x, *m_extractor1y, a_pt, vector))
  {
    a_outTrace.clear();
    return;
  }

  vx0 = vector.x * m_vectorMultiplier;
  vy0 = vector.y * m_vectorMultiplier;
  mag0 = sqrt(vector.x * vector.x + vector.y * vector.y);
  double maxAngleChange = sin(m_maxChangeDirectionInRadians);

  while (bContinue)
  {
    if (m_maxChangeDistanceMeters > 0)
    {
      // make sure deltaT is small enough to not go past the max dist
      double d2 = m_maxChangeDistanceMeters * m_maxChangeDistanceMeters;
      double denom = (vx0 * vx0) + (vy0 * vy0) + (m_maxChangeDistanceMeters * XM_ZERO_TOL);
      double dt = sqrt(d2 / denom);
      if (deltaT > dt)
      {
        deltaT = dt;
      }
    }
     // If the change in DeltaT would push us beyond the time step, set it to hit the timestep
    if (elapsedTime + deltaT + a_ptTime > m_time2)
    {
      deltaT = m_time2 - elapsedTime - a_ptTime;
      bContinue = false; // This will be the last point traced
    }
    // If the change in delta time would push beyond the max tracing time, set it to hit max tracing time
    if (m_maxTracingTimeSeconds > 0 && (elapsedTime + deltaT) > m_maxTracingTimeSeconds)
    {
      deltaT = m_maxTracingTimeSeconds - elapsedTime;
      bContinue = false; // This will be the last point traced
    }

    // compute candidate point
    pt1.x = pt0.x + deltaT * vx0;
    pt1.y = pt0.y + deltaT * vy0;
    bool bInDomain = true;

    // if pt1 outside of domain, compute new deltaT to get to boundary
    if (!extract(*m_extractor2x, *m_extractor2y, vtkVec, vector))
    {
      a_outTrace.clear();
      return;
    }
    vx1 = vtkVec.x;
    vy1 = vtkVec.y;
    if (vtkVec.x == XM_NODATA || vtkVec.y == XM_NODATA)
    {
      deltaT /= 2;
      if (deltaT < m_minDeltaTimeSeconds)
      {
        // done ?
        bContinue = false;
      }
      bInDomain = false;
    }
    if (bInDomain)//Could be replaced with an else, removing bInDomain entirely
    {
      vx1 *= m_vectorMultiplier;
      vy1 *= m_vectorMultiplier;

      if (EQ_TOL(vx1, 0.0, .0001) && EQ_TOL(vy1, 0.0, .0001)) //No velocity
      {
        a_outTrace.push_back(pt1);
        return;
      }
      bool bSplit = false;

      mag1 = sqrt(vx1 * vx1 + vy1 * vy1);

      // do we subdivide?

      if (!bSplit && m_maxChangeVelocityMetersPerSecond < 0)
      {
        double vel = abs(mag1 - mag0);
        if (vel > m_maxChangeVelocityMetersPerSecond)
        {
          bSplit = true;
        }
      }
      if (!bSplit && m_maxChangeDirectionInRadians < 0)
      {
        // cross product of unit vectors gives the sin of the angle between
        // the vectors
        double dir = abs(((vx0 * vy1) - (vx1 * vy0)) / (mag0 * mag1));
        if (dir > maxAngleChange)
        {
          bSplit = true;
        }
      }
      if (bSplit)
      {
        deltaT /= 2;
        if (deltaT < m_minDeltaTimeSeconds) {
        // done, exit
        bContinue = false;
        }
      }
      else
      {
        double segDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
        m_distTraveled += segDist;
        if (m_maxTracingDistanceMeters != -1 && m_distTraveled > m_maxTracingDistanceMeters)
        {
          // because our last point exceeded the exitDistance
          // find this point by linear calculations
          double distancePast = m_distTraveled - m_maxTracingDistanceMeters;
          double perc = distancePast / segDist;
          Pt3d newPt;
          newPt.x = (pt0.x * perc) + (pt1.x * (1 - perc));
          newPt.y = (pt0.y * perc) + (pt1.y * (1 - perc));

          m_distTraveled = m_maxTracingDistanceMeters;
          a_outTrace.push_back(newPt);
          return;
        }


        // add new pt if not identical to last
        int size = (int)a_outTrace.size();
        if (size > 0)
        {
          if (!EQ_TOL(pt1.x, a_outTrace.at(size - 1).x, XM_ZERO_TOL) ||
            !EQ_TOL(pt1.y, a_outTrace.at(size - 1).y, XM_ZERO_TOL))
          {
            a_outTrace.push_back(pt1);
          }
        }
        else
        {
          a_outTrace.push_back(pt1);
        }

        pt0 = pt1;
        elapsedTime += deltaT;
        vx0 = vx1;
        vy0 = vy1;
        deltaT *= 1.2;
      }
    }
  } // while ()
}
} // namespace xms

#ifdef CXX_TEST
#include <xmsgridtrace/gridtrace/XmGridTrace.t.h>


#include <xmscore/testing/TestTools.h>
#include <xmsgrid/ugrid/XmUGrid.h>

using namespace xms;
//
//#include <shared1/feature/fline.h>
//#include <shared1/feature/flineutl.h>
//#include <Shared1/general/gen_ask.h>
//#include <Shared1/general/gen_messages.h>
////#include <Shared1/testing/TestTools.h>
//#include <xmsinterp/geometry/GmPolyLinePtRedistributer.h>
//
//#include <mesh2d/Mesh2dTree.h>
//#include <ugrid_module/UGridModuleTree.h>

//namespace xms
//{
//  extern coveragelisttype g_coveragelist;
//}
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testTracePoint()
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = { { 0, 0, 0 },{ 1, 0, 0 },{ 1, 1, 0 },{ 0, 1, 0 } };
  VecInt cells = { XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0 };
  BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
  BSHP<XmGridTrace> tracer=XmGridTrace::New(ugrid);
  const double vm = 2;
  tracer->SetVectorMultiplier(vm) ;
  TS_ASSERT_EQUALS(tracer->GetVectorMultiplier(), vm);

  const double tt = 2;
  tracer->SetMaxTracingTimeSeconds(tt);
  TS_ASSERT_EQUALS(tracer->GetMaxTracingTimeSeconds(), tt);

  const double td = 2;
  tracer->SetMaxTracingDistanceMeters(td);
  TS_ASSERT_EQUALS(tracer->GetMaxTracingDistanceMeters(), td);

  /*GetMinDeltaTimeSeconds()  ;
  SetMinDeltaTimeSeconds(  a_minDeltaTime) ;

  GetMaxChangeDistanceMeters()  ;
  SetMaxChangeDistanceMeters(  a_maxChangeDistance) ;

  GetMaxChangeVelocityMetersPerSecond()  ;
  SetMaxChangeVelocityMetersPerSecond(  a_maxChangeVelocity) ;

  GetMaxChangeDirectionInRadians()  ;
  SetMaxChangeDirectionInRadians(  a_maxChangeDirection) = 0;*/

} // XmGridTraceUnitTests::testTracePoint

////------------------------------------------------------------------------------
///// \brief Does a flow trace and creates a new coverage given the
///// WorldPointDrifter
///// \param a_ WorldPointDrifter class that is set up with a mesh 2d or a UGrid
////------------------------------------------------------------------------------
//static void iDoFlowTrace(BSHP<WorldPointDrifter> a_)
//{
//  coveragerec* cov = feGetActiveCoverage();
//  if (!cov)
//  {
//    XM_LOG(xmlog::error, "No active coverage defined");
//    return;
//  }
//
//  double dir(1);
//  gnGetFloat(&dir, 2, -1, 1, "Direction (1 - forward, -1 - backward):", "Flow Direction");
//  double outputSpacing(1);
//  gnGetFloat(&outputSpacing, 2, 1e-7, 100000, "Spacing:", "Arc Vertex Spacing");
//  double maxDist(10000);
//  gnGetFloat(&maxDist, 2, 1e-7, 100000, "Max dist:", "Max Trace Distance");
//
//  std::stringstream covName;
//  covName << "Flow Trace";
//  coveragerec* aCov = feNewFeatureCoverage(&g_coveragelist, true, NULL, false, covName.str());
//  BSHP<GmPolyLinePtRedistributer> redist = GmPolyLinePtRedistributer::New();
//  VecPt3d trace;
//  fpointrec* fpt = cov->pointlist.list;
//  for (; fpt; fpt = fpt->next)
//  {
//    // AKZ testing tolerance
//    double maxChangeDist = 1;          // AML had 0.01
//    double maxChangeDirInRadians = .1; // AML had 0.01
//                                       //
//    trace = a_->Run(fpt->location, dir, 0.1, maxChangeDist, -1, maxChangeDirInRadians, -1, maxDist);
//    trace = redist->Redistribute(trace, outputSpacing);
//    feCreateArcFromMpoints(aCov, trace);
//  }
//
//  // double tol = gmXyTol(false, 0);
//  // feCleanFeatureObjects(true, tol, true, true, tol, false, false, aCov, tol);
//} // iDoFlowTrace
  ////////////////////////////////////////////////////////////////////////////////
  /// \class WorldPtDrifterExperimentalTests
  /// \brief Exposes methods for doing a flow trace on a UGrid or a mesh. This
  /// is not exposed in the interface.
  ////////////////////////////////////////////////////////////////////////////////
  //------------------------------------------------------------------------------
  /// \brief    Defines the test group.
  //------------------------------------------------------------------------------
//const CxxTest::TestGroup& WorldPtDrifterExperimentalTests::group()
//{
//  return *CxxTest::TestGroup::GetGroup(CxxTest::TG_EXPERIMENTAL);
//} // WorldPtDrifterExperimentalTests::group
//  //------------------------------------------------------------------------------
//  /// \brief Uses the points in the active coverage to create a new coverage of
//  /// arcs that are a flow trace from those points
//  //------------------------------------------------------------------------------
//void WorldPtDrifterExperimentalTests::testMesh2FlowTraceCoverage()
//{
//  meshrec* mesh = GetActiveMesh2D();
//  if (!mesh)
//  {
//    XM_LOG(xmlog::error, "No active 2d mesh defined");
//    return;
//  }
//  BSHP<WorldPointDrifter> pd(new WorldPointDrifter(mesh, true));
//  iDoFlowTrace(pd);
//} // WorldPtDrifterExperimentalTests::testMesh2FlowTraceCoverage

#endif