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


//----- Constants / Enumerations -----------------------------------------------

//----- Classes / Structs ------------------------------------------------------

//----- Internal functions -----------------------------------------------------
namespace {
  bool extract(xms::XmUGrid2dDataExtractor& a_extractorX, xms::XmUGrid2dDataExtractor& a_extractorY, const xms::Pt3d& a_pt, xms::Pt3d& a_data)
  {
    xms::VecPt3d loc;
    loc.push_back(a_pt);
    a_extractorX.SetExtractLocations(loc);
    a_extractorY.SetExtractLocations(loc);
    xms::VecFlt dataOutx;
    xms::VecFlt dataOuty;
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
  double getDirAsCosTheta(double a_vx0, double a_vy0, double a_vx1, double a_vy1)
  {
    // Should be the cosine of the angle
    double mag0 = sqrt(a_vx0 * a_vx0 + a_vy0 * a_vy0);
    double mag1 = sqrt(a_vx1 * a_vx1 + a_vy1 * a_vy1);
    return (a_vx0*a_vx1 + a_vy0*a_vy1) / (mag0 * mag1);
  }
}
namespace xms
{
/// XMS Namespace

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
  if (a_minDeltaTime <= 0) // Must have an exit condition to avoid infinite loops
  {
    m_minDeltaTimeSeconds = XM_ZERO_TOL;
  }
  else
  {
    m_minDeltaTimeSeconds = a_minDeltaTime;
  }
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
  if (a_maxChangeDirection > XM_PI||a_maxChangeDirection<0)
  {

  }
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
  if (m_extractor2x&&m_extractor2y) {
    m_extractor1x = m_extractor2x;
    m_extractor1y = m_extractor2y;
    m_time1 = m_time2;
  }
  m_extractor2x = XmUGrid2dDataExtractor::New(m_ugrid);
  m_extractor2y = XmUGrid2dDataExtractor::New(m_ugrid);
  
  m_time2 = a_time;
  std::vector<float> xx, yy;
  for (auto &pt : a_scalars) {
    xx.push_back((float)pt.x);
    yy.push_back((float)pt.y);
  }
  if (a_scalarLoc == DataLocationEnum::LOC_POINTS)
  {
    m_extractor2x->SetGridPointScalars(xx, a_activity, a_activityLoc);
    m_extractor2y->SetGridPointScalars(yy, a_activity, a_activityLoc);
  }
  else
  {
    m_extractor2x->SetGridCellScalars(xx, a_activity, a_activityLoc);
    m_extractor2y->SetGridCellScalars(yy, a_activity, a_activityLoc);
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

  if (a_ptTime > m_time2 || a_ptTime < m_time1|| //Test if it is within the time boundaries
    !extract(*m_extractor1x, *m_extractor1y, a_pt, vector)) // Ensure nothing fails during extraction
  {
    a_outTrace.clear();
    a_outTimes.clear();
    return;
  }

  vx0 = vector.x * m_vectorMultiplier;
  vy0 = vector.y * m_vectorMultiplier;
  mag0 = sqrt(vector.x * vector.x + vector.y * vector.y);
  double maxAngleChange = cos(m_maxChangeDirectionInRadians);

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
    if (!extract(*m_extractor2x, *m_extractor2y, pt1, vtkVec))
    {
      a_outTrace.clear();
      a_outTimes.clear();
      return;
    }
    vx1 = vtkVec.x;
    vy1 = vtkVec.y;
    if (vtkVec.x == XM_NODATA || vtkVec.y == XM_NODATA)
    {
      deltaT /= 2;
      if (m_minDeltaTimeSeconds>0 && deltaT < m_minDeltaTimeSeconds)
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
        a_outTimes.push_back(a_ptTime + elapsedTime + deltaT);
        return;
      }
      bool bSplit = false;

      mag1 = sqrt(vx1 * vx1 + vy1 * vy1);

      // do we subdivide?

      if (!bSplit && m_maxChangeVelocityMetersPerSecond > 0)
      {
        double changeVel = abs(mag1 - mag0);
        if (changeVel > m_maxChangeVelocityMetersPerSecond)
        {
          bSplit = true;
        }
      }
      if (!bSplit && m_maxChangeDirectionInRadians > 0)
      {
        double dir = getDirAsCosTheta(vx0, vy0, vx1, vy1);
        if (dir < maxAngleChange)
        {
          bSplit = true;
        }
      }
      if (bSplit)
      {
        deltaT /= 2;
        if (m_minDeltaTimeSeconds>0 && deltaT < m_minDeltaTimeSeconds) {
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
          a_outTimes.push_back(a_ptTime + elapsedTime + deltaT*perc);
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
        a_outTimes.push_back(a_ptTime + elapsedTime);
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
namespace
{
  void createDefaultSingleCell(BSHP<XmGridTrace>& a_tracer)
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
    a_tracer = XmGridTrace::New(ugrid);
    const double vm = 1;
    a_tracer->SetVectorMultiplier(vm);
    TS_ASSERT_EQUALS(a_tracer->GetVectorMultiplier(), vm);

    const double tt = 100;
    a_tracer->SetMaxTracingTimeSeconds(tt);
    TS_ASSERT_EQUALS(a_tracer->GetMaxTracingTimeSeconds(), tt);

    const double td = 100;
    a_tracer->SetMaxTracingDistanceMeters(td);
    TS_ASSERT_EQUALS(a_tracer->GetMaxTracingDistanceMeters(), td);

    const double dt = .1;
    a_tracer->SetMinDeltaTimeSeconds(dt);
    TS_ASSERT_EQUALS(a_tracer->GetMinDeltaTimeSeconds(), dt);

    const double cd = 100;
    a_tracer->SetMaxChangeDistanceMeters(cd);
    TS_ASSERT_EQUALS(a_tracer->GetMaxChangeDistanceMeters(), cd);

    const double cv = 100;
    a_tracer->SetMaxChangeVelocityMetersPerSecond(cv);
    TS_ASSERT_EQUALS(a_tracer->GetMaxChangeVelocityMetersPerSecond(), cv);

    const double cdir = 1.5*XM_PI;
    a_tracer->SetMaxChangeDirectionInRadians(cdir);
    TS_ASSERT_EQUALS(a_tracer->GetMaxChangeDirectionInRadians(), cdir);

    double time = 0;
    VecPt3d scalars = { { 1,1,0 },{ 1,1,0 },{ 1,1,0 },{ 1,1,0 } };
    DynBitset pointActivity;
    for (int i = 0; i < 4; ++i)
    {
      pointActivity.push_back(true);
    }
    a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

    time = 10;
    //Uses exact same scalars/pointActivity
    a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);
  }
  void createDefaultTwoCell(BSHP<XmGridTrace>& a_tracer)
  {
    // clang-format off
    //      3========2========5
    //      |        |        |
    //      |        |        |
    //      |        |        |
    //      0--------1--------4
    // clang-format on

    VecPt3d points = { { 0, 0, 0 },{ 1, 0, 0 },{ 1, 1, 0 },{ 0, 1, 0 },{ 2, 0, 0 },{ 2, 1, 0 } };
    VecInt cells = { XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 1, 4, 5, 2 };
    BSHP<XmUGrid> ugrid = XmUGrid::New(points, cells);
    a_tracer = XmGridTrace::New(ugrid);
    const double vm = 1;
    a_tracer->SetVectorMultiplier(vm);

    const double tt = 100;
    a_tracer->SetMaxTracingTimeSeconds(tt);

    const double td = 100;
    a_tracer->SetMaxTracingDistanceMeters(td);

    const double dt = .1;
    a_tracer->SetMinDeltaTimeSeconds(dt);

    const double cd = 100;
    a_tracer->SetMaxChangeDistanceMeters(cd);

    const double cv = 100;
    a_tracer->SetMaxChangeVelocityMetersPerSecond(cv);

    const double cdir = 1.5*XM_PI;
    a_tracer->SetMaxChangeDirectionInRadians(cdir);

    double time = 0;
    VecPt3d scalars = { { .1,0,0 },{ .2,0,0 } };
    DynBitset pointActivity;
    for (int i = 0; i < 2; ++i)
    {
      pointActivity.push_back(true);
    }
    a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity, DataLocationEnum::LOC_CELLS, time);

    time = 10;
    //Uses exact same scalars/pointActivity
    a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity, DataLocationEnum::LOC_CELLS, time);
  } // createDefaultTwoCell
}
namespace xms
{  // COPIED FROM GEOMS.CPP because dependencies were difficult

  bool gmEqualPointsXYZ(double x1,
    double y1,
    double z1,
    double x2,
    double y2,
    double z2,
    double tolerance);
  bool gmEqualPointsXYZ(const Pt3d& pt1, const Pt3d& pt2, double tol);
}
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBasicTracePoint()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);
  
  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime=.5;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = { {1,1,0} };
  VecDbl expectedOutTimes= { 1 };
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testTracePoint
////------------------------------------------------------------------------------
///// \brief Speed is limited to .25. It doesnt reach the edge because it goes below min delta time
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxChangeDistance()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);
  tracer->SetMaxChangeDistanceMeters(.25);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime = .5;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = 
  {
    { 0.67677668424809445, 0.67677668424809445, 0.00000000000000000 },
    { 0.85355336849618890, 0.85355336849618890, 0.00000000000000000 }
  };
  VecDbl expectedOutTimes = 
  { 
    0.67677668424809445,
    0.85355336849618890
  };
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testMaxChangeDistance
  ////------------------------------------------------------------------------------
  ///// \brief 
  ////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testSmallScalarsTracePoint()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  //Push on different scalars
  double time = 0;
  VecPt3d scalars = { { .1,.1,0 },{ .1,.1,0 },{ .1,.1,0 },{ .1,.1,0 } };
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  time = 10;
  //Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);


  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = 
  {
    {0.60000000149011612, 0.60000000149011612, 0 }	,
    {0.72000000327825542, 0.72000000327825542, 0 }	,
    {0.86400000542402267, 0.86400000542402267, 0 }	,
    {0.95040000671148295, 0.95040000671148295, 0 }	,
    {0.97632000709772104, 0.97632000709772104, 0 }	,
    {0.99187200732946390, 0.99187200732946390, 0 }
  };
  VecDbl expectedOutTimes =
  {
    1.5,
    2.7,
    4.14,
    5.004,
    5.2632,
    5.41872
  };

  if (expectedOutTrace.size() == outTrace.size()) {
    for (int i = 0; i < expectedOutTrace.size(); ++i) 
    {
      TS_ASSERT_DELTA(expectedOutTrace[i].x, outTrace[i].x,.001);
      TS_ASSERT_DELTA(expectedOutTrace[i].y, outTrace[i].y, .001);
      TS_ASSERT_DELTA(expectedOutTrace[i].z, outTrace[i].z, .001);
    }
  }
  else TS_FAIL("Expected trace size != actual trace size");
  //TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  //TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, 0.001);
} // XmGridTraceUnitTests::testSmallScalarsTracePoint
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testStrongDirectionChange()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI*.2);
  tracer->SetMinDeltaTimeSeconds(-1);
  //Push on different scalars
  double time = 0;
  VecPt3d scalars = { { 0,1,0 },{ -1,0,0 },{ 0,-1,0 },{ 1,0,0 } };
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  time = 10;
  //Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { 0,0,0 };
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    {0.00000000000000000, 0.25000000000000000, 0.00000000000000000 }	,
    {0.074999999999999997, 0.47499999999999998, 0.00000000000000000 }	,
    {0.21900000214576720, 0.63699999570846555, 0.00000000000000000 }	,
    {0.30928799843788146, 0.66810399758815764, 0.00000000000000000 }	,
    {0.40229310507774352, 0.67396399235725402, 0.00000000000000000 }	,
    {0.48679361495018003, 0.65024498560905453, 0.00000000000000000 }	,
    {0.54780151323509219, 0.59909560095787040, 0.00000000000000000 }	,
    {0.55928876277122497, 0.56619817004051198, 0.00000000000000000 }	,
    {0.56114558691518779, 0.53247499044700608, 0.00000000000000000 }	,
    {0.55189971330840681, 0.50228363992173752, 0.00000000000000000 }	,
    {0.53269911067322617, 0.48131557500677169, 0.00000000000000000 }	,
    {0.52076836142536975, 0.47806150355091476, 0.00000000000000000 }	,
    {0.50886902895577013, 0.47838753608466128, 0.00000000000000000 }	,
    {0.49867742691962913, 0.48264835153512164, 0.00000000000000000 }	,
    {0.49224616907898289, 0.49014090685121131, 0.00000000000000000 }	,
    {0.49173935940609609, 0.49438094923206660, 0.00000000000000000 }	,
    {0.49250246625151450, 0.49839053740482164, 0.00000000000000000 }	,
    {0.49454361321306389, 0.50154755045413602, 0.00000000000000000 }	,
    {0.49745717820065949, 0.50317358562752867, 0.00000000000000000 }	,
    {0.49888395770889871, 0.50301615091938545, 0.00000000000000000 }	,
    {0.50012160117661586, 0.50244704462921241, 0.00000000000000000 }	,
    {0.50095740046883197, 0.50152383477622209, 0.00000000000000000 }	,
    {0.50107955145675120, 0.50098875888952354, 0.00000000000000000 }	,
    {0.50105605626599747, 0.50045352403892940, 0.00000000000000000 }	,
    {0.50086894918345870, 0.49998474718699493, 0.00000000000000000 }	,
    {0.50053945884260675, 0.49966662451478433, 0.00000000000000000 }	,
    {0.50034430627617277, 0.49962054739305783, 0.00000000000000000 }	,
    {0.50015012042108842, 0.49962997721910873, 0.00000000000000000 }	,
    {0.49998265395837810, 0.49970077747304897, 0.00000000000000000 }	,
    {0.49987374966305814, 0.49982308521808211, 0.00000000000000000 }
  };
  VecDbl expectedOutTimes =
  {
    	0.75000000000000000, 
    	1.0500000000000000, 
    	1.4100000000000001, 
    	1.6260000000000001, 
    	1.8852000000000002, 
    	2.1962400000000004, 
    	2.5694880000000002, 
    	2.7934368000000003, 
    	3.0621753600000003, 
    	3.3846616320000003, 
    	3.7716451584000001, 
    	4.0038352742400001, 
    	4.2824634132480002, 
    	4.6168171800576001, 
    	5.0180417002291202, 
    	5.2587764123320317, 
    	5.5476580668555258, 
    	5.8943160522837186, 
    	6.3103056347975501, 
    	6.5598993843058491, 
    	6.8594118837158078, 
    	7.2188268830077584, 
    	7.4344758825829285, 
    	7.6932546820731327, 
    	8.0037892414613783, 
    	8.3764307127272737, 
    	8.6000155954868092, 
    	8.8683174547982535, 
    	9.1902796859719871, 
    	9.5766343633804656
  };
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace,.0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testStrongDirectionChange
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxTracingTime()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI*.2);
  tracer->SetMinDeltaTimeSeconds(-1);
  tracer->SetMaxTracingTimeSeconds(5);
  //Push on different scalars
  double time = 0;
  VecPt3d scalars = { { 0,1,0 },{ -1,0,0 },{ 0,-1,0 },{ 1,0,0 } };
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  time = 10;
  //Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { 0,0,0 };
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    { 0.00000000000000000, 0.25000000000000000, 0.00000000000000000 }	,
    { 0.074999999999999997, 0.47499999999999998, 0.00000000000000000 }	,
    { 0.21900000214576720, 0.63699999570846555, 0.00000000000000000 }	,
    { 0.30928799843788146, 0.66810399758815764, 0.00000000000000000 }	,
    { 0.40229310507774352, 0.67396399235725402, 0.00000000000000000 }	,
    { 0.48679361495018003, 0.65024498560905453, 0.00000000000000000 }	,
    { 0.54780151323509219, 0.59909560095787040, 0.00000000000000000 }	,
    { 0.55928876277122497, 0.56619817004051198, 0.00000000000000000 }	,
    { 0.56114558691518779, 0.53247499044700608, 0.00000000000000000 }	,
    { 0.55189971330840681, 0.50228363992173752, 0.00000000000000000 }	,
    { 0.53269911067322617, 0.48131557500677169, 0.00000000000000000 }	,
    { 0.52076836142536975, 0.47806150355091476, 0.00000000000000000 }	,
    { 0.50886902895577013, 0.47838753608466128, 0.00000000000000000 }	,
    { 0.49867742691962913, 0.48264835153512164, 0.00000000000000000 }	,
    { 0.49224616907898289, 0.49014090685121131, 0.00000000000000000 }	,
    { 0.49173935940609609, 0.49438094923206660, 0.00000000000000000 }	,
    { 0.49237657318600692, 0.49772905815126539, 0.00000000000000000 }
  };
  VecDbl expectedOutTimes =
  {
    0.75000000000000000,
    1.0500000000000000,
    1.4100000000000001,
    1.6260000000000001,
    1.8852000000000002,
    2.1962400000000004,
    2.5694880000000002,
    2.7934368000000003,
    3.0621753600000003,
    3.3846616320000003,
    3.7716451584000001,
    4.0038352742400001,
    4.2824634132480002,
    4.6168171800576001,
    5.0180417002291202,
    5.2587764123320317,
    5.5
  };
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMaxTracingTime
  ////------------------------------------------------------------------------------
  ///// \brief 
  ////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxTracingDistance()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI*.2);
  tracer->SetMinDeltaTimeSeconds(-1);
  tracer->SetMaxTracingDistanceMeters(1.0);
  //Push on different scalars
  double time = 0;
  VecPt3d scalars = { { 0,1,0 },{ -1,0,0 },{ 0,-1,0 },{ 1,0,0 } };
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  time = 10;
  //Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { 0,0,0 };
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    { 0.00000000000000000, 0.25000000000000000, 0.00000000000000000 }	,
    { 0.074999999999999997, 0.47499999999999998, 0.00000000000000000 }	,
    { 0.21900000214576720, 0.63699999570846555, 0.00000000000000000 }	,
    { 0.30928799843788146, 0.66810399758815764, 0.00000000000000000 }	,
    { 0.40229310507774352, 0.67396399235725402, 0.00000000000000000 }	,
    { 0.48679361495018003, 0.65024498560905453, 0.00000000000000000 }	,
    { 0.50183556502673621, 0.63763372523131490, 0.00000000000000000 }
  };
  VecDbl expectedOutTimes =
  {
    0.75000000000000000,
    1.0500000000000000,
    1.4100000000000001,
    1.6260000000000001,
    1.8852000000000002,
    2.1962400000000004,
    2.4774609356360582
  };
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMaxTracingDistance
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testStartOutOfCell()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { -.1,0,0 };
  double startTime = .5;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {};
  VecDbl expectedOutTimes = {};
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testStartOutOfCell
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testDotProduct()
{
  TS_ASSERT_DELTA(getDirAsCosTheta(0, 1, 1, 0), 0, .1);//90 degree angle
  TS_ASSERT_DELTA(getDirAsCosTheta(0, 1, 1, 1), .707, .1);//45 degree angle
  TS_ASSERT_DELTA(getDirAsCosTheta(0, 1, 0, -1), -1, .1);//180 degree angle
  TS_ASSERT_DELTA(getDirAsCosTheta(1, 1, -1, -1), -1, .1);//180 degree angle
  TS_ASSERT_DELTA(getDirAsCosTheta(2, 5, 3, 6), .9965, .1);//almost 0 degree angle
  TS_ASSERT_DELTA(getDirAsCosTheta(0, 1, 0, 1), 1, .1);//0 degree angle
} // XmGridTraceUnitTests::testDotProduct
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBeyondTimestep()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5,.5,0 };
  double startTime = 10.1;//Just beyond the 2nd time

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {};
  VecDbl expectedOutTimes = {};
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testBeyondTimestep
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testVectorMultiplier()
{
  BSHP<XmGridTrace> tracer;
  createDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI*.2);
  tracer->SetMinDeltaTimeSeconds(-1);
  tracer->SetVectorMultiplier(0.5);
  //Push on different scalars
  double time = 0;
  VecPt3d scalars = { { 0,1,0 },{ -1,0,0 },{ 0,-1,0 },{ 1,0,0 } };
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  time = 10;
  //Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity, DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { 0,0,0 };
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    {0.00000000000000000, 0.25000000000000000, 0.00000000000000000 },
    {0.074999999999999997, 0.47499999999999998, 0.00000000000000000 },
    {0.21900000214576720, 0.63699999570846555, 0.00000000000000000 },
    {0.30928799843788146, 0.66810399758815764, 0.00000000000000000 },
    {0.40229310507774352, 0.67396399235725402, 0.00000000000000000 },
    {0.48679361495018003, 0.65024498560905453, 0.00000000000000000 },
    {0.54780151323509219, 0.59909560095787040, 0.00000000000000000 },
    {0.55928876277122497, 0.56619817004051198, 0.00000000000000000 },
    {0.56114558691518779, 0.53247499044700608, 0.00000000000000000 },
    {0.55189971330840681, 0.50228363992173752, 0.00000000000000000 },
    {0.53269911067322617, 0.48131557500677169, 0.00000000000000000 },
    {0.52076836142536975, 0.47806150355091476, 0.00000000000000000 },
    {0.50886902895577013, 0.47838753608466128, 0.00000000000000000 },
    {0.49867742691962913, 0.48264835153512164, 0.00000000000000000 },
    {0.49224616907898289, 0.49014090685121131, 0.00000000000000000 },
    {0.49175783605462037, 0.49422637094165467, 0.00000000000000000 },
  };
  VecDbl expectedOutTimes =
  {
    	1.0000000000000000,
    	1.6000000000000001,
    	2.3200000000000003,
    	2.7520000000000002,
    	3.2704000000000004,
    	3.8924800000000004,
    	4.6389760000000004,
    	5.0868736000000006,
    	5.6243507200000007,
    	6.2693232640000005,
    	7.0432903168000003,
    	7.5076705484800001,
    	8.0649268264960003,
    	8.7336343601152002,
    	9.5360834004582404,
    	10.000000000000000,
  };
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testVectorMultiplier
////------------------------------------------------------------------------------
///// \brief 
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMultiCell()
{
  BSHP<XmGridTrace> tracer;
  createDefaultTwoCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime = 0;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = 
  { 
    {0.60000000149011612, 0.50000000000000000, 0.00000000000000000 }, 
    {0.73200000077486038, 0.50000000000000000, 0.00000000000000000 }, 
    {0.90940801054239273, 0.50000000000000000, 0.00000000000000000 }, 
    {1.1529537134766579, 0.50000000000000000, 0.00000000000000000 }, 
    {1.4957102079987525, 0.50000000000000000, 0.00000000000000000 }, 
    {1.9923067892670629, 0.50000000000000000, 0.00000000000000000 }
  };
  VecDbl expectedOutTimes = 
  {
    	1.0000000000000000, 
    	2.2000000000000002, 
    	3.6400000000000001, 
    	5.3680000000000003, 
    	7.4416000000000002, 
    	9.9299199999999992
  };
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testMultiCell
  ////------------------------------------------------------------------------------
  ///// \brief Testing what happens when the maximum change in velocity is low
  ////         It reaches a point of high acceleration, and then the delta time
  ////         decreases until it goes below the minimum delta time.
  ////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxChangeVelocity()
{
  BSHP<XmGridTrace> tracer;
  createDefaultTwoCell(tracer);
  tracer->SetMaxChangeVelocityMetersPerSecond(.1);
  tracer->SetMinDeltaTimeSeconds(0.001);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime = 0;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    {0.60000000149011612, 0.50000000000000000, 0.00000000000000000 },
    {0.73200000077486038, 0.50000000000000000, 0.00000000000000000 },
    {0.90940801054239273, 0.50000000000000000, 0.00000000000000000 },
    {1.1529537134766579, 0.50000000000000000, 0.00000000000000000 },
    {1.4957102079987525, 0.50000000000000000, 0.00000000000000000 },
    {1.4995898687899112, 0.50000000000000000, 0.00000000000000000 },
    {1.4998814090045989, 0.50000000000000000, 0.00000000000000000 },

  };
  VecDbl expectedOutTimes =
  {
    	1.0000000000000000,
    	2.2000000000000002,
    	3.6400000000000001,
    	5.3680000000000003,
    	7.4416000000000002,
    	7.4610400000000006,
    	7.4624980000000010,
  };
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testMaxChangeVelocity
  ////------------------------------------------------------------------------------
  ///// \brief 
  ////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testUniqueTimeSteps()
{
  BSHP<XmGridTrace> tracer;
  createDefaultTwoCell(tracer);

  double time = 20;
  VecPt3d scalars = { { .2,0,0 },{ .3,0,0 } };
  DynBitset pointActivity;
  for (int i = 0; i < 2; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity, DataLocationEnum::LOC_CELLS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime = 10;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    {0.60000000149011612, 0.50000000000000000, 0.00000000000000000 },
    {0.85200001150369642, 0.50000000000000000, 0.00000000000000000 },
    {1.1906880155205726, 0.50000000000000000, 0.00000000000000000 },
    {1.6556389146447181, 0.50000000000000000, 0.00000000000000000 },
    {1.9666789270043372, 0.50000000000000000, 0.00000000000000000 }
  };
  VecDbl expectedOutTimes =
  {
    	11.000000000000000,
    	12.199999999999999,
    	13.640000000000001,
    	15.368000000000000,
    	16.404800000000002
  };
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testUniqueTimeSteps
////------------------------------------------------------------------------------
///// \brief 2nd cell is inactive in the 2nd time step.
/////        Thus it does not pull as hard. Also once the point reaches the 2nd cell
/////        It stops entirely.
////------------------------------------------------------------------------------
void XmGridTraceUnitTests::testInactiveCell()
{
  BSHP<XmGridTrace> tracer;
  createDefaultTwoCell(tracer);

  double time = 20;
  VecPt3d scalars = { { .2,0,0 },{ .3,0,0 } };
  DynBitset pointActivity;
  pointActivity.push_back(true);
  pointActivity.push_back(false);
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity, DataLocationEnum::LOC_CELLS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = { .5,.5,0 };
  double startTime = 10;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace =
  {
    {0.60000000149011612, 0.50000000000000000, 0.00000000000000000 },
    {0.84000000506639483, 0.50000000000000000, 0.00000000000000000 },
    {0.98400000721216208, 0.50000000000000000, 0.00000000000000000 },
  };
  VecDbl expectedOutTimes =
  {
    	11.000000000000000,
    	12.199999999999999,
    	12.920000000000000,
  };
  TS_ASSERT_EQUALS(expectedOutTrace, outTrace);
  TS_ASSERT_EQUALS(expectedOutTimes, outTimes);
} // XmGridTraceUnitTests::testInactiveCell

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