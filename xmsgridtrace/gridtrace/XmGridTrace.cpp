//------------------------------------------------------------------------------
/// \file
/// \ingroup extractor
/// \copyright (C) Copyright Aquaveo 2018. Distributed under FreeBSD License
/// (See accompanying file LICENSE or https://aqaveo.com/bsd/license.txt)
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------

// 1. Precompiled header

// 2. My own header
#include <xmsgridtrace/gridtrace/XmGridTrace.h>

// 3. Standard library headers
#include <sstream>

// 4. External library headers

// 5. Shared code headers
#include <xmscore/math/math.h>
#include <xmscore/misc/XmError.h>
#include <xmscore/misc/XmLog.h>
#include <xmscore/misc/xmstype.h> // XM_ZERO_TOL
#include <xmsextractor/extractor/XmUGrid2dDataExtractor.h>
#include <xmsextractor/extractor/XmUGrid2dPolylineDataExtractor.h>
#include <xmsgrid/geometry/geoms.h>

// 6. Non-shared code headers

//----- Forward declarations ---------------------------------------------------

//----- External globals -------------------------------------------------------

//----- Namespace declaration --------------------------------------------------

//----- Constants / Enumerations -----------------------------------------------

//----- Classes / Structs ------------------------------------------------------

//----- Internal functions -----------------------------------------------------
namespace xms
{
namespace
{
/// XMS Namespace

#ifdef CXX_TEST
/// \brief Count of XmUGrid2dDataExtractor::ExtractData calls since it was last zeroed.
/// Test-build-only instrumentation for testTraceBenchmark. A trace's cost is dominated by
/// the point-location search each ExtractData performs, so the benchmark needs the search
/// count and not only wall time -- otherwise an algorithmic win cannot be told apart from
/// a faster machine. Not thread safe; the benchmark is single threaded.
size_t g_extractDataCalls = 0;
/// \brief Adds a_n to the ExtractData call count. Compiles away outside test builds.
#define XMGT_COUNT_EXTRACT_DATA(a_n) (g_extractDataCalls += (a_n))
#else
/// \brief No-op outside test builds, so production traces pay nothing for instrumentation.
#define XMGT_COUNT_EXTRACT_DATA(a_n) ((void)0)
#endif

//----- Class / Function definitions -------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// Implementation for XmGridTrace
class XmGridTraceImpl : public XmGridTrace
{
public:
  XmGridTraceImpl(std::shared_ptr<XmUGrid> a_ugrid);
  ~XmGridTraceImpl(){};

  double GetVectorMultiplier() const final;
  void SetVectorMultiplier(const double a_vectorMultiplier) final;

  double GetMaxTracingTime() const final;
  void SetMaxTracingTime(const double a_maxTracingTime) final;

  double GetMaxTracingDistance() const final;
  void SetMaxTracingDistance(const double a_maxTracingDistance) final;

  double GetMinDeltaTime() const final;
  void SetMinDeltaTime(const double a_minDeltaTime) final;

  double GetMaxChangeDistance() const final;
  void SetMaxChangeDistance(const double a_maxChangeDistance) final;

  double GetMaxChangeVelocity() const final;
  void SetMaxChangeVelocity(const double a_maxChangeVelocity) final;

  double GetMaxChangeDirectionInRadians() const final;
  void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) final;

  void AddGridScalarsAtTime(const VecPt3d& a_scalars,
                            DataLocationEnum a_scalarLoc,
                            xms::DynBitset& a_activity,
                            DataLocationEnum a_activityLoc,
                            double a_time) final;

  void TracePoint(const Pt3d& a_pt,
                  const double& a_ptTime,
                  VecPt3d& a_outTrace,
                  VecDbl& a_outTimes) final;

  std::string GetExitMessage() final;

private:
  bool GetVectorAtLocationAndTime(const xms::Pt3d& a_pt,
                                  double a_currentTime,
                                  xms::Pt3d& a_data) const;

  std::shared_ptr<XmUGrid> m_ugrid;                ///< UGrid for the TracePoint operation
  double m_vectorMultiplier=1;          ///< multiplier for all vectors in grid
  double m_maxTracingTime=-1;           ///< maximum time for trace
  double m_maxTracingDistance=-1;       ///< maximum distance for trace
  double m_minDeltaTime=1;              ///< minimum time per trace step
  double m_maxChangeDistance=-1;        ///< maximum distance per trace step
  double m_maxChangeVelocity=-1;        ///< maximum change in velocity per trace step
  double m_maxChangeDirectionInRadians=XM_PI/4; ///< maxmium change in direction per trace step

  /// data extractor for the x component for the first time step
  BSHP<XmUGrid2dDataExtractor> m_extractor1x;
  /// data extractor for the y component for the first time step
  BSHP<XmUGrid2dDataExtractor> m_extractor1y;
  double m_time1=-1;  ///< time of the first time step
  /// data extractor for the x component for the second time step
  BSHP<XmUGrid2dDataExtractor> m_extractor2x;
  /// data extractor for the y component for the second time step
  BSHP<XmUGrid2dDataExtractor> m_extractor2y;
  double m_time2=-1;        ///< time of the second time step
  double m_distTraveled=0; ///< distance traveled in the last TracePoint operation

  std::string m_exitMessage; ///< exit message for the last TracePoint operation
protected:
};
double iGetDirAsCosTheta(double a_vx0, double a_vy0, double a_vx1, double a_vy1)
{
  // Should be the cosine of the angle
  double mag0 = sqrt(a_vx0 * a_vx0 + a_vy0 * a_vy0);
  double mag1 = sqrt(a_vx1 * a_vx1 + a_vy1 * a_vy1);
  return (a_vx0 * a_vx1 + a_vy0 * a_vy1) / (mag0 * mag1);
}
//------------------------------------------------------------------------------
/// \brief Construct a new XmGridTrace using a UGrid.
/// \param[in] a_ugrid The UGrid to construct a grid trace for
//------------------------------------------------------------------------------
XmGridTraceImpl::XmGridTraceImpl(std::shared_ptr<XmUGrid> a_ugrid)
: m_ugrid(a_ugrid)
{
}

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
/// \brief Returns the Max Tracing Time
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxTracingTime() const
{
  return m_maxTracingTime;
} // XmGridTraceImpl::GetMaxTracingTime
//------------------------------------------------------------------------------
/// \brief Sets the max tracing time
/// \param[in] a_maxTracingTime the new max tracing time
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxTracingTime(const double a_maxTracingTime)
{
  m_maxTracingTime = a_maxTracingTime;
} // XmGridTraceImpl::SetMaxTracingTime
//------------------------------------------------------------------------------
/// \brief Returns the Max Tracing Distance
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxTracingDistance() const
{
  return m_maxTracingDistance;
} // XmGridTraceImpl::GetMaxTracingDistance
//------------------------------------------------------------------------------
/// \brief Sets the max tracing distance
/// \param[in] a_maxTracingDistance the new max tracing distance
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxTracingDistance(const double a_maxTracingDistance)
{
  m_maxTracingDistance = a_maxTracingDistance;
} // XmGridTraceImpl::SetMaxTracingDistance
//------------------------------------------------------------------------------
/// \brief Returns the min delta time
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMinDeltaTime() const
{
  return m_minDeltaTime;
} // XmGridTraceImpl::GetMinDeltaTime
//------------------------------------------------------------------------------
/// \brief Sets the min delta time
/// \param[in] a_minDeltaTime the new min delta time
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMinDeltaTime(const double a_minDeltaTime)
{
  if (a_minDeltaTime <= 0) // Must have an exit condition to avoid infinite loops
  {
    m_minDeltaTime = XM_ZERO_TOL;
  }
  else
  {
    m_minDeltaTime = a_minDeltaTime;
  }
} // XmGridTraceImpl::SetMinDeltaTime
//------------------------------------------------------------------------------
/// \brief Returns the max change distance
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxChangeDistance() const
{
  return m_maxChangeDistance;
} // XmGridTraceImpl::GetMaxChangeDistance
//------------------------------------------------------------------------------
/// \brief Sets the max change distance
/// \param[in] a_maxChangeDistance the new max change distance
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxChangeDistance(const double a_maxChangeDistance)
{
  m_maxChangeDistance = a_maxChangeDistance;
} // XmGridTraceImpl::SetMaxChangeDistance
//------------------------------------------------------------------------------
/// \brief Returns the max change in velcoity
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxChangeVelocity() const
{
  return m_maxChangeVelocity;
} // XmGridTraceImpl::GetMaxChangeVelocity
//------------------------------------------------------------------------------
/// \brief Sets the max change in velocity
/// \param[in] a_maxChangeVelocity the new max change in velocity
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxChangeVelocity(const double a_maxChangeVelocity)
{
  m_maxChangeVelocity = a_maxChangeVelocity;
} // XmGridTraceImpl::SetMaxChangeVelocity
//------------------------------------------------------------------------------
/// \brief Returns the max change in direction in radians
//------------------------------------------------------------------------------
double XmGridTraceImpl::GetMaxChangeDirectionInRadians() const
{
  return m_maxChangeDirectionInRadians;
} // XmGridTraceImpl::GetMaxChangeDirectionInRadians
//------------------------------------------------------------------------------
/// \brief Sets the max change in direction in radians
/// \param[in] a_maxChangeDirection the new max change in direction in radians
//------------------------------------------------------------------------------
void XmGridTraceImpl::SetMaxChangeDirectionInRadians(const double a_maxChangeDirection)
{
  if (a_maxChangeDirection > XM_PI || a_maxChangeDirection < 0)
  {
  }
  m_maxChangeDirectionInRadians = a_maxChangeDirection;
} // XmGridTraceImpl::SetMaxChangeDirectionInRadians
//------------------------------------------------------------------------------
/// \brief returns a message describing what caused trace to exit
//------------------------------------------------------------------------------
std::string XmGridTraceImpl::GetExitMessage()
{
  return m_exitMessage;
} // XmGridTraceImpl::GetExitMessage
//------------------------------------------------------------------------------
/// \brief Assigns velocity vectors to each point or cell for a time step,
///        keeping the previous step, and dropping the one before that
///        for a maximum of two time steps.
/// \param[in] a_scalars The velocity vectors
/// \param[in] a_scalarLoc Whether the vectors are assigned to cells or points
/// \param[in] a_activity Whether each cell or point is active
/// \param[in] a_activityLoc Whether the activities are assigned to cells or points
/// \param[in] a_time The time of the scalars
//------------------------------------------------------------------------------
void XmGridTraceImpl::AddGridScalarsAtTime(const VecPt3d& a_scalars,
                                           DataLocationEnum a_scalarLoc,
                                           xms::DynBitset& a_activity,
                                           DataLocationEnum a_activityLoc,
                                           double a_time)
{
  if (m_extractor2x && m_extractor2y)
  {
    m_extractor1x = m_extractor2x;
    m_extractor1y = m_extractor2y;
    m_time1 = m_time2;
  }
  m_extractor2x = XmUGrid2dDataExtractor::New(m_ugrid);
  m_extractor2y = XmUGrid2dDataExtractor::New(m_ugrid);

  m_time2 = a_time;
  std::vector<float> xx, yy;
  for (auto& pt : a_scalars)
  {
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
/// \param[in] a_pt The starting point of the trace
/// \param[in] a_ptTime The starting time of the trace
/// \param[out] a_outTrace the resultant positions at each step
/// \param[out] a_outTimes the resultant times at each step
//------------------------------------------------------------------------------
void XmGridTraceImpl::TracePoint(const Pt3d& a_pt,
                                 const double& a_ptTime,
                                 VecPt3d& a_outTrace,
                                 VecDbl& a_outTimes)
{
  m_exitMessage.clear();
  double deltaT = 1.00;
  double mag0 = 0, mag1 = 0;
  Pt3d pt0 = a_pt, pt1;
  double vx0 = 0, vx1 = 0, vy0 = 0, vy1 = 0, elapsedTime = 0;
  bool bContinue = true;
  Pt3d vtkVec; // Rename this variable
  Pt3d vtkPt;
  Pt3d vector;

  m_distTraveled = 0;
  a_outTrace.clear();
  a_outTimes.clear();
  if (a_ptTime > m_time2 || // Test if the time specified is after the time range
      !GetVectorAtLocationAndTime(a_pt, a_ptTime, vector)) // Ensure nothing fails during extraction
  {
    m_exitMessage = "Error occurred while extracting point0.";
    return;
  }
  if (EQ_TOL(vector.x, XM_NODATA, 1) || EQ_TOL(vector.y, XM_NODATA, 1))
  {
    m_exitMessage = "Point does not start inside an active cell.";
    return;
  }

  a_outTrace.push_back(a_pt);
  a_outTimes.push_back(a_ptTime);

  vx0 = vector.x * m_vectorMultiplier;
  vy0 = vector.y * m_vectorMultiplier;
  mag0 = sqrt(vector.x * vector.x + vector.y * vector.y);

  double maxAngleChange = cos(m_maxChangeDirectionInRadians);

  while (bContinue)
  {
    if (m_maxChangeDistance > 0)
    {
      // make sure deltaT is small enough to not go past the max dist
      double d2 = m_maxChangeDistance * m_maxChangeDistance;
      double denom = (vx0 * vx0) + (vy0 * vy0) + (m_maxChangeDistance * XM_ZERO_TOL);
      double dt = sqrt(d2 / denom);
      if (deltaT > dt)
      {
        deltaT = dt;
        m_exitMessage = "Change distance was greater than the max change distance.";
      }
    }
    // If the change in DeltaT would push us beyond the time step, set it to hit the timestep
    if (elapsedTime + deltaT + a_ptTime > m_time2)
    {
      deltaT = m_time2 - elapsedTime - a_ptTime;
      bContinue = false; // This will be the last point traced
      m_exitMessage = "The point has traveled beyond, or reached the second time step.";
    }
    // If the change in delta time would push beyond the max tracing time, set it to hit max tracing
    // time
    if (m_maxTracingTime > 0 && (elapsedTime + deltaT) > m_maxTracingTime)
    {
      deltaT = m_maxTracingTime - elapsedTime;
      bContinue = false; // This will be the last point traced
      m_exitMessage = "Exceeded or reached max tracing time.";
    }

    // compute candidate point
    pt1.x = pt0.x + deltaT * vx0;
    pt1.y = pt0.y + deltaT * vy0;

    if (!GetVectorAtLocationAndTime(pt1, a_ptTime + elapsedTime + deltaT, vtkVec))
    {
      a_outTrace.clear();
      a_outTimes.clear();
      m_exitMessage = "Error occurred while extracting point1";
      return;
    }
    // if pt1 outside of domain, compute new deltaT to get to boundary
    if (EQ_TOL(vtkVec.x, XM_NODATA, 1) || EQ_TOL(vtkVec.y, XM_NODATA, 1))
    {
      m_exitMessage = "Point has traveled out of domain.";
      VecPt3d points = {pt0, pt1};
      // DataLocationEnum is irrelevant here.
      BSHP<XmUGrid2dPolylineDataExtractor> polylineExtractor =
        XmUGrid2dPolylineDataExtractor::New(m_ugrid, DataLocationEnum::LOC_POINTS);
      polylineExtractor->SetPolyline(points);
      points = polylineExtractor->GetExtractLocations();
      if (points.size() < 3)
      {
        XM_LOG(xmlog::error, "Gridtracer failed to find an intersection when exiting grid.");
        return;
      }
      double segDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
      pt1 = points[points.size() - 2];
      double newSegDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
      deltaT *= (newSegDist / segDist);
      bContinue = false;
      if (!GetVectorAtLocationAndTime(pt1, a_ptTime + elapsedTime + deltaT, vtkVec) ||
          vtkVec.x == XM_NODATA || vtkVec.y == XM_NODATA)
      {
        m_exitMessage = "Error occurred while extracting point1";
        return;
      }
    }
    vx1 = vtkVec.x;
    vy1 = vtkVec.y;
    vx1 *= m_vectorMultiplier;
    vy1 *= m_vectorMultiplier;

    if (EQ_TOL(vx1, 0.0, .0001) && EQ_TOL(vy1, 0.0, .0001)) // No velocity
    {
      a_outTrace.push_back(pt1);
      a_outTimes.push_back(a_ptTime + elapsedTime + deltaT);
      m_exitMessage = "Velocity has gone to zero.";
      return;
    }
    bool bSplit = false;

    mag1 = sqrt(vx1 * vx1 + vy1 * vy1);

    // do we subdivide?

    if (!bSplit && m_maxChangeVelocity > 0)
    {
      double changeVel = fabs(mag1 - mag0);
      if (changeVel > m_maxChangeVelocity)
      {
        bSplit = true;
        m_exitMessage = "Point has exceeded max change velocity.";
      }
    }
    if (!bSplit && m_maxChangeDirectionInRadians > 0)
    {
      double dir = iGetDirAsCosTheta(vx0, vy0, vx1, vy1);
      if (dir < maxAngleChange)
      {
        bSplit = true;
        m_exitMessage = "Point has exceeded max change direction.";
      }
    }
    if (bSplit)
    {
      bContinue = true;
      deltaT /= 2;
      if (m_minDeltaTime > 0 && deltaT < m_minDeltaTime)
      {
        // done, exit
        bContinue = false;
        m_exitMessage += " Delta time was less than min delta time.";
      }
    }
    else
    {
      double segDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
      m_distTraveled += segDist;
      if (m_maxTracingDistance > 0 && m_distTraveled > m_maxTracingDistance)
      {
        // because our last point exceeded the exitDistance
        // find this point by linear calculations
        double distancePast = m_distTraveled - m_maxTracingDistance;
        double perc = distancePast / segDist;
        Pt3d newPt;
        newPt.x = (pt0.x * perc) + (pt1.x * (1 - perc));
        newPt.y = (pt0.y * perc) + (pt1.y * (1 - perc));

        m_distTraveled = m_maxTracingDistance;
        a_outTrace.push_back(newPt);
        a_outTimes.push_back(a_ptTime + elapsedTime + deltaT * perc);
        m_exitMessage = "Point has reached or exceeded the max tracing distance.";
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
      mag0 = mag1;
      a_outTimes.push_back(a_ptTime + elapsedTime);
    }
  } // while ()
} // XmGridTraceImpl::TracePoint
//------------------------------------------------------------------------------
/// \brief Returns the velocity scalar for a given point and time
/// \param[in] a_pt The point
/// \param[in] a_currentTime The time at extraction
/// \param[out] a_data the resultant velocity scalar
//------------------------------------------------------------------------------
bool XmGridTraceImpl::GetVectorAtLocationAndTime(const xms::Pt3d& a_pt,
                                                 double a_currentTime,
                                                 xms::Pt3d& a_data) const
{
  xms::VecPt3d loc;
  loc.push_back(a_pt);
  m_extractor1x->SetExtractLocations(loc);
  m_extractor1y->SetExtractLocations(loc);
  xms::VecFlt dataOutx1;
  xms::VecFlt dataOuty1;
  m_extractor1x->ExtractData(dataOutx1);
  m_extractor1y->ExtractData(dataOuty1);
  XMGT_COUNT_EXTRACT_DATA(2);
  if (dataOutx1.size() != 1 || dataOuty1.size() != 1)
  {
    XM_LOG(xmlog::error, "Gridtracer: An error occured when extracting data");
    return false;
  }

  m_extractor2x->SetExtractLocations(loc);
  m_extractor2y->SetExtractLocations(loc);
  xms::VecFlt dataOutx2;
  xms::VecFlt dataOuty2;
  m_extractor2x->ExtractData(dataOutx2);
  m_extractor2y->ExtractData(dataOuty2);
  XMGT_COUNT_EXTRACT_DATA(2);
  if (dataOutx2.size() != 1 || dataOuty2.size() != 1)
  {
    XM_LOG(xmlog::error, "Gridtracer: An error occured when extracting data");
    return false;
  }

  if (a_currentTime < m_time1 - XM_ZERO_TOL)
  {
    XM_LOG(xmlog::warning, "Gridtracer: The given time is before the first time step.");
    a_currentTime = m_time1;
  }
  double totalTime = fabs(m_time1 - m_time2);
  double perc1 = fabs(a_currentTime - m_time1) / totalTime;
  double perc2 = fabs(a_currentTime - m_time2) / totalTime;
  a_data.x = dataOutx1[0] * perc1 + dataOutx2[0] * perc2;
  a_data.y = dataOuty1[0] * perc1 + dataOuty2[0] * perc2;
  return true;
} // XmGridTraceImpl::GetVectorAtLocationAndTime
} // namespace {}
////////////////////////////////////////////////////////////////////////////////
/// \class XmGridTrace
/// \brief Traces points in an XmUGrid following a vector dataset
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief Empty constructer for abstract class
//------------------------------------------------------------------------------
XmGridTrace::XmGridTrace()
{
} // XmGridTrace::XmGridTrace
//------------------------------------------------------------------------------
/// \brief Empty destructer for abstract class
//------------------------------------------------------------------------------
XmGridTrace::~XmGridTrace()
{
} // XmGridTrace::~XmGridTrace
//------------------------------------------------------------------------------
/// \brief Construct from a new XmGridTrace using a UGrid.
/// \param[in] a_ugrid The UGrid to construct a grid trace for
/// \return a boost shared pointer to an XmGridTrace
//------------------------------------------------------------------------------
BSHP<XmGridTrace> XmGridTrace::New(std::shared_ptr<XmUGrid> a_ugrid)
{
  return BSHP<XmGridTraceImpl>(new XmGridTraceImpl(a_ugrid));
} // XmGridTrace::New

} // namespace xms
#ifdef CXX_TEST
#include <xmsgridtrace/gridtrace/XmGridTrace.t.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>

#include <xmscore/testing/TestTools.h>
#include <xmsextractor/ugrid/XmUGridTriangles2d.h>
#include <xmsgrid/ugrid/XmUGrid.h>

using namespace xms;
namespace
{
//------------------------------------------------------------------------------
/// \brief Returns a tracer for a default cell
/// \param[out] a_tracer The tracer for a default cell
//------------------------------------------------------------------------------
void iCreateDefaultSingleCell(BSHP<XmGridTrace>& a_tracer)
{
  //  3----2
  //  | 1 /|
  //  |  / |
  //  | /  |
  //  |/ 0 |
  //  0----1
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  VecInt cells = {XMU_TRIANGLE, 3, 0, 1, 2, XMU_TRIANGLE, 3, 2, 3, 0};
  std::shared_ptr<XmUGrid> ugrid = XmUGrid::New(points, cells);
  a_tracer = XmGridTrace::New(ugrid);
  const double vm = 1;
  a_tracer->SetVectorMultiplier(vm);
  TS_ASSERT_EQUALS(a_tracer->GetVectorMultiplier(), vm);

  const double tt = 100;
  a_tracer->SetMaxTracingTime(tt);
  TS_ASSERT_EQUALS(a_tracer->GetMaxTracingTime(), tt);

  const double td = 100;
  a_tracer->SetMaxTracingDistance(td);
  TS_ASSERT_EQUALS(a_tracer->GetMaxTracingDistance(), td);

  const double dt = .1;
  a_tracer->SetMinDeltaTime(dt);
  TS_ASSERT_EQUALS(a_tracer->GetMinDeltaTime(), dt);

  const double cd = 100;
  a_tracer->SetMaxChangeDistance(cd);
  TS_ASSERT_EQUALS(a_tracer->GetMaxChangeDistance(), cd);

  const double cv = 100;
  a_tracer->SetMaxChangeVelocity(cv);
  TS_ASSERT_EQUALS(a_tracer->GetMaxChangeVelocity(), cv);

  const double cdir = 1.5 * XM_PI;
  a_tracer->SetMaxChangeDirectionInRadians(cdir);
  TS_ASSERT_EQUALS(a_tracer->GetMaxChangeDirectionInRadians(), cdir);

  double time = 0;
  VecPt3d scalars = {{1, 1, 0}, {1, 1, 0}, {1, 1, 0}, {1, 1, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                                 DataLocationEnum::LOC_POINTS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                                 DataLocationEnum::LOC_POINTS, time);
} // iCreateDefaultSingleCell
//------------------------------------------------------------------------------
/// \brief Returns a tracer for two default cells
/// \param[out] a_tracer The tracer for two default cells
//------------------------------------------------------------------------------
void iCreateDefaultTwoCell(BSHP<XmGridTrace>& a_tracer)
{
  // clang-format off
    //      3========2========5
    //      |        |        |
    //      |        |        |
    //      |        |        |
    //      0--------1--------4
  // clang-format on

  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {2, 0, 0}, {2, 1, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3, XMU_QUAD, 4, 1, 4, 5, 2};
  std::shared_ptr<XmUGrid> ugrid = XmUGrid::New(points, cells);
  a_tracer = XmGridTrace::New(ugrid);
  const double vm = 1;
  a_tracer->SetVectorMultiplier(vm);

  const double tt = 100;
  a_tracer->SetMaxTracingTime(tt);

  const double td = 100;
  a_tracer->SetMaxTracingDistance(td);

  const double dt = .1;
  a_tracer->SetMinDeltaTime(dt);

  const double cd = 100;
  a_tracer->SetMaxChangeDistance(cd);

  const double cv = 100;
  a_tracer->SetMaxChangeVelocity(cv);

  const double cdir = 1.5 * XM_PI;
  a_tracer->SetMaxChangeDirectionInRadians(cdir);

  double time = 0;
  VecPt3d scalars = {{.1, 0, 0}, {.2, 0, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 2; ++i)
  {
    pointActivity.push_back(true);
  }
  a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity,
                                 DataLocationEnum::LOC_CELLS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  a_tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity,
                                 DataLocationEnum::LOC_CELLS, time);
} // iCreateDefaultTwoCell

//------------------------------------------------------------------------------
/// \brief A structured quad grid plus its point locations, for the tracing benchmark.
/// The locations are kept alongside the ugrid so the velocity field can be evaluated
/// without depending on how the ugrid exposes its points.
//------------------------------------------------------------------------------
struct BenchmarkGrid
{
  std::shared_ptr<XmUGrid> m_ugrid; ///< the grid itself
  VecPt3d m_points;                 ///< grid point locations, in grid point order
};

//------------------------------------------------------------------------------
/// \brief Measurements from one benchmark batch.
//------------------------------------------------------------------------------
struct BenchmarkStats
{
  int m_seeds = 0;                          ///< seed points handed to TracePoint
  int m_traced = 0;                         ///< seeds that produced a usable (2+ point) polyline
  size_t m_tracePoints = 0;                 ///< total polyline points produced
  size_t m_extractCalls = 0;                ///< XmUGrid2dDataExtractor::ExtractData calls consumed
  double m_seconds = 0;                     ///< wall time of the traced batch, excluding setup
  std::map<std::string, int> m_exitReasons; ///< exit message -> count, over a sample
};

//------------------------------------------------------------------------------
/// \brief Builds a structured quad grid standing in for a real hydrodynamic mesh.
/// \param[in] a_cellsPerSide Number of cells along each axis
/// \param[in] a_length Length of the square domain along each axis
/// \return the grid and its point locations
//------------------------------------------------------------------------------
BenchmarkGrid iBuildBenchmarkGrid(int a_cellsPerSide, double a_length)
{
  const int ptsPerSide = a_cellsPerSide + 1;
  const double dx = a_length / a_cellsPerSide;
  BenchmarkGrid grid;
  grid.m_points.reserve((size_t)ptsPerSide * ptsPerSide);
  for (int j = 0; j < ptsPerSide; ++j)
  {
    for (int i = 0; i < ptsPerSide; ++i)
      grid.m_points.push_back({i * dx, j * dx, 0.0});
  }

  VecInt cells;
  cells.reserve((size_t)a_cellsPerSide * a_cellsPerSide * 6);
  for (int j = 0; j < a_cellsPerSide; ++j)
  {
    for (int i = 0; i < a_cellsPerSide; ++i)
    {
      const int p0 = j * ptsPerSide + i;
      cells.push_back(XMU_QUAD);
      cells.push_back(4);
      cells.push_back(p0);
      cells.push_back(p0 + 1);
      cells.push_back(p0 + ptsPerSide + 1);
      cells.push_back(p0 + ptsPerSide);
    }
  }
  grid.m_ugrid = XmUGrid::New(grid.m_points, cells);
  return grid;
} // iBuildBenchmarkGrid
//------------------------------------------------------------------------------
/// \brief Builds a rotating-plus-drifting velocity field over the grid points.
/// A vortex is used rather than a uniform field for two reasons: the curvature makes the
/// adaptive stepping subdivide the way it does on real flow, and the drift carries part
/// of the seed population off the grid so the out-of-domain exit path -- which builds a
/// fresh polyline extractor per event -- is measured rather than assumed away.
/// \param[in] a_points Grid point locations
/// \param[in] a_omega Angular rate of the vortex; negative reverses the rotation
/// \param[in] a_drift Uniform velocity added in +x
/// \param[in] a_length Length of the square domain along each axis
/// \return velocity vectors, one per grid point
//------------------------------------------------------------------------------
VecPt3d iBenchmarkVectors(const VecPt3d& a_points, double a_omega, double a_drift, double a_length)
{
  const double cx = a_length / 2, cy = a_length / 2;
  VecPt3d vectors;
  vectors.reserve(a_points.size());
  for (const auto& pt : a_points)
    vectors.push_back({-a_omega * (pt.y - cy) + a_drift, a_omega * (pt.x - cx), 0.0});
  return vectors;
} // iBenchmarkVectors
//------------------------------------------------------------------------------
/// \brief Builds seed points scattered inside a rectangular band of the domain.
/// The scatter is driven by a fixed linear congruential generator rather than std::rand
/// so that reruns and different machines trace the identical seed set; a benchmark whose
/// input changes between runs cannot measure a delta.
/// \param[in] a_count Number of seeds
/// \param[in] a_lo Low corner of the band, on both axes
/// \param[in] a_hi High corner of the band, on both axes
/// \param[in] a_holeLo Low corner of a rectangular hole to reject seeds from
/// \param[in] a_holeHi High corner of the hole; pass a_holeHi <= a_holeLo for no hole
/// \return the seed points
//------------------------------------------------------------------------------
VecPt3d iBenchmarkSeeds(int a_count, double a_lo, double a_hi, double a_holeLo, double a_holeHi)
{
  unsigned int state = 12345u;
  auto nextUnit = [&state]() {
    state = state * 1664525u + 1013904223u;
    return (state >> 8) / 16777216.0;
  };

  VecPt3d seeds;
  seeds.reserve(a_count);
  while ((int)seeds.size() < a_count)
  {
    const double x = a_lo + nextUnit() * (a_hi - a_lo);
    const double y = a_lo + nextUnit() * (a_hi - a_lo);
    const bool inHole =
      a_holeHi > a_holeLo && x > a_holeLo && x < a_holeHi && y > a_holeLo && y < a_holeHi;
    if (!inHole)
      seeds.push_back({x, y, 0.0});
  }
  return seeds;
} // iBenchmarkSeeds
//------------------------------------------------------------------------------
/// \brief Traces every seed and measures the batch.
/// Timing covers only the TracePoint calls. The exit-reason histogram is gathered in a
/// separate untimed pass over a sample, because GetExitMessage returns a std::string by
/// value and a per-seed map insert would show up in a measurement this small.
/// \param[in] a_tracer The tracer, already loaded with two time steps
/// \param[in] a_seeds The seed points
/// \param[out] a_stats The measurements
//------------------------------------------------------------------------------
void iRunTraceBenchmark(BSHP<XmGridTrace>& a_tracer,
                        const VecPt3d& a_seeds,
                        BenchmarkStats& a_stats)
{
  a_stats = BenchmarkStats();
  a_stats.m_seeds = (int)a_seeds.size();

  VecPt3d trace;
  VecDbl times;
  g_extractDataCalls = 0;
  const auto start = std::chrono::steady_clock::now();
  for (const auto& seed : a_seeds)
  {
    a_tracer->TracePoint(seed, 0.0, trace, times);
    if (trace.size() > 1)
    {
      ++a_stats.m_traced;
      a_stats.m_tracePoints += trace.size();
    }
  }
  const auto end = std::chrono::steady_clock::now();
  a_stats.m_seconds = std::chrono::duration<double>(end - start).count();
  a_stats.m_extractCalls = g_extractDataCalls;

  const int sampleSize = std::min((int)a_seeds.size(), 1000);
  for (int i = 0; i < sampleSize; ++i)
  {
    a_tracer->TracePoint(a_seeds[i], 0.0, trace, times);
    a_stats.m_exitReasons[a_tracer->GetExitMessage()]++;
  }
} // iRunTraceBenchmark
//------------------------------------------------------------------------------
/// \brief Prints one benchmark batch in a form that can be pasted into a results table.
/// \param[in] a_label Which seed population this batch was
/// \param[in] a_stats The measurements
//------------------------------------------------------------------------------
void iReportTraceBenchmark(const char* a_label, const BenchmarkStats& a_stats)
{
  const double seeds = a_stats.m_seeds ? (double)a_stats.m_seeds : 1.0;
  const double usPerSeed = a_stats.m_seconds * 1e6 / seeds;
  const double extractsPerSeed = a_stats.m_extractCalls / seeds;
  const double usPerExtract =
    a_stats.m_extractCalls ? a_stats.m_seconds * 1e6 / a_stats.m_extractCalls : 0.0;
  const double ptsPerTrace =
    a_stats.m_traced ? (double)a_stats.m_tracePoints / a_stats.m_traced : 0.0;

  std::cout << std::fixed << std::setprecision(3) << "\n  [" << a_label
            << "] seeds=" << a_stats.m_seeds << " traced=" << a_stats.m_traced << "\n"
            << "    wall            " << a_stats.m_seconds * 1e3 << " ms\n"
            << "    per seed        " << usPerSeed << " us\n"
            << "    ExtractData     " << a_stats.m_extractCalls << " calls ("
            << std::setprecision(1) << extractsPerSeed << "/seed, " << std::setprecision(3)
            << usPerExtract << " us/call)\n"
            << "    trace points    " << a_stats.m_tracePoints << " (" << std::setprecision(1)
            << ptsPerTrace << "/trace)\n"
            << "    exit reasons (sampled):\n";
  for (const auto& reason : a_stats.m_exitReasons)
    std::cout << "      " << std::setw(5) << reason.second << "  " << reason.first << "\n";
  std::cout << std::flush;
} // iReportTraceBenchmark
//------------------------------------------------------------------------------
/// \brief Reads a positive integer from the environment, or returns a fallback.
/// \param[in] a_name Environment variable name
/// \param[in] a_fallback Value to use when unset, unparseable, or not positive
/// \return the resolved value
//------------------------------------------------------------------------------
int iEnvInt(const char* a_name, int a_fallback)
{
  const char* raw = std::getenv(a_name);
  if (!raw)
    return a_fallback;
  const int value = std::atoi(raw);
  return value > 0 ? value : a_fallback;
} // iEnvInt
}
////////////////////////////////////////////////////////////////////////////////
/// \class XmGridTraceUnitTests
/// \brief Tests functionality of XmGridTrace
////////////////////////////////////////////////////////////////////////////////
//------------------------------------------------------------------------------
/// \brief test the basic functionality of trace point
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBasicTracePoint()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = .5;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0}, {1, 1, 0}};
  VecDbl expectedOutTimes = {.5, 1};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testTracePoint
//------------------------------------------------------------------------------
/// \brief Speed is limited to .25. It doesnt reach the edge because it goes below min delta time
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxChangeDistance()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);
  tracer->SetMaxChangeDistance(.25);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = .5;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0},
                              {0.67677668424809445, 0.67677668424809445, 0.00000000000000000},
                              {0.85355336849618890, 0.85355336849618890, 0.00000000000000000},
                              {1, 1, 0}};
  VecDbl expectedOutTimes = {.5, 0.67677668424809445, 0.85355336849618890, 1};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMaxChangeDistance
//------------------------------------------------------------------------------
/// \brief test with small scalars to create more points
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testSmallScalarsTracePoint()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  // Push on different scalars
  double time = 0;
  VecPt3d scalars = {{.1, .1, 0}, {.1, .1, 0}, {.1, .1, 0}, {.1, .1, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0},
                              {0.60000000149011612, 0.60000000149011612, 0},
                              {0.72000000327825542, 0.72000000327825542, 0},
                              {0.86400000542402267, 0.86400000542402267, 0},
                              {1, 1, 0}};
  VecDbl expectedOutTimes = {.5, 1.5, 2.7, 4.14, 5.5};

  if (expectedOutTrace.size() == outTrace.size())
  {
    for (int i = 0; i < expectedOutTrace.size(); ++i)
    {
      TS_ASSERT_DELTA(expectedOutTrace[i].x, outTrace[i].x, .001);
      TS_ASSERT_DELTA(expectedOutTrace[i].y, outTrace[i].y, .001);
      TS_ASSERT_DELTA(expectedOutTrace[i].z, outTrace[i].z, .001);
    }
  }
  else
    TS_FAIL("Expected trace size != actual trace size");
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, 0.001);
} // XmGridTraceUnitTests::testSmallScalarsTracePoint
//------------------------------------------------------------------------------
/// \brief test behavior when having large changes in direction
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testStrongDirectionChange()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI * .2);
  tracer->SetMinDeltaTime(-1);
  // Push on different scalars
  double time = 0;
  VecPt3d scalars = {{0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {1, 0, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {0, 0, 0};
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{0, 0, 0},
                              {0.00000000000000000, 0.25000000000000000, 0.00000000000000000},
                              {0.074999999999999997, 0.47499999999999998, 0.00000000000000000},
                              {0.21900000214576720, 0.63699999570846555, 0.00000000000000000},
                              {0.30928799843788146, 0.66810399758815764, 0.00000000000000000},
                              {0.40229310507774352, 0.67396399235725402, 0.00000000000000000},
                              {0.48679361495018003, 0.65024498560905453, 0.00000000000000000},
                              {0.54780151323509219, 0.59909560095787040, 0.00000000000000000},
                              {0.55928876277122497, 0.56619817004051198, 0.00000000000000000},
                              {0.56114558691518779, 0.53247499044700608, 0.00000000000000000},
                              {0.55189971330840681, 0.50228363992173752, 0.00000000000000000},
                              {0.53269911067322617, 0.48131557500677169, 0.00000000000000000},
                              {0.52076836142536975, 0.47806150355091476, 0.00000000000000000},
                              {0.50886902895577013, 0.47838753608466128, 0.00000000000000000},
                              {0.49867742691962913, 0.48264835153512164, 0.00000000000000000},
                              {0.49224616907898289, 0.49014090685121131, 0.00000000000000000},
                              {0.49173935940609609, 0.49438094923206660, 0.00000000000000000},
                              {0.49250246625151450, 0.49839053740482164, 0.00000000000000000},
                              {0.49454361321306389, 0.50154755045413602, 0.00000000000000000},
                              {0.49745717820065949, 0.50317358562752867, 0.00000000000000000},
                              {0.49888395770889871, 0.50301615091938545, 0.00000000000000000},
                              {0.50012160117661586, 0.50244704462921241, 0.00000000000000000},
                              {0.50095740046883197, 0.50152383477622209, 0.00000000000000000},
                              {0.50107955145675120, 0.50098875888952354, 0.00000000000000000},
                              {0.50105605626599747, 0.50045352403892940, 0.00000000000000000},
                              {0.50086894918345870, 0.49998474718699493, 0.00000000000000000},
                              {0.50053945884260675, 0.49966662451478433, 0.00000000000000000},
                              {0.50034430627617277, 0.49962054739305783, 0.00000000000000000},
                              {0.50015012042108842, 0.49962997721910873, 0.00000000000000000},
                              {0.49998265395837810, 0.49970077747304897, 0.00000000000000000},
                              {0.49987374966305814, 0.49982308521808211, 0.00000000000000000},
                              {0.49986302487024292, 0.49988726006383088, 0.00000000000000000},
                              {0.49986815504448728, 0.49994012045071656, 0.00000000000000000}};
  VecDbl expectedOutTimes = {.5,
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
                             9.5766343633804656,
                             9.7883171816902319,
                             10.000000000000000};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testStrongDirectionChange
//------------------------------------------------------------------------------
/// \brief test setting max tracing time
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxTracingTime()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI * .2);
  tracer->SetMinDeltaTime(-1);
  tracer->SetMaxTracingTime(5);
  // Push on different scalars
  double time = 0;
  VecPt3d scalars = {{0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {1, 0, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {0, 0, 0};
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{0, 0, 0},
                              {0.00000000000000000, 0.25000000000000000, 0.00000000000000000},
                              {0.074999999999999997, 0.47499999999999998, 0.00000000000000000},
                              {0.21900000214576720, 0.63699999570846555, 0.00000000000000000},
                              {0.30928799843788146, 0.66810399758815764, 0.00000000000000000},
                              {0.40229310507774352, 0.67396399235725402, 0.00000000000000000},
                              {0.48679361495018003, 0.65024498560905453, 0.00000000000000000},
                              {0.54780151323509219, 0.59909560095787040, 0.00000000000000000},
                              {0.55928876277122497, 0.56619817004051198, 0.00000000000000000},
                              {0.56114558691518779, 0.53247499044700608, 0.00000000000000000},
                              {0.55189971330840681, 0.50228363992173752, 0.00000000000000000},
                              {0.53269911067322617, 0.48131557500677169, 0.00000000000000000},
                              {0.52076836142536975, 0.47806150355091476, 0.00000000000000000},
                              {0.50886902895577013, 0.47838753608466128, 0.00000000000000000},
                              {0.49867742691962913, 0.48264835153512164, 0.00000000000000000},
                              {0.49224616907898289, 0.49014090685121131, 0.00000000000000000},
                              {0.49173935940609609, 0.49438094923206660, 0.00000000000000000},
                              {0.49237657318600692, 0.49772905815126539, 0.00000000000000000}};
  VecDbl expectedOutTimes = {.5,
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
                             5.5};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMaxTracingTime
//------------------------------------------------------------------------------
/// \brief test setting the max tracing distance
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxTracingDistance()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI * .2);
  tracer->SetMinDeltaTime(-1);
  tracer->SetMaxTracingDistance(1.0);
  // Push on different scalars
  double time = 0;
  VecPt3d scalars = {{0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {1, 0, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {0, 0, 0};
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{0, 0, 0},
                              {0.00000000000000000, 0.25000000000000000, 0.00000000000000000},
                              {0.074999999999999997, 0.47499999999999998, 0.00000000000000000},
                              {0.21900000214576720, 0.63699999570846555, 0.00000000000000000},
                              {0.30928799843788146, 0.66810399758815764, 0.00000000000000000},
                              {0.40229310507774352, 0.67396399235725402, 0.00000000000000000},
                              {0.48679361495018003, 0.65024498560905453, 0.00000000000000000},
                              {0.50183556502673621, 0.63763372523131490, 0.00000000000000000}};
  VecDbl expectedOutTimes = {.5,
                             0.75000000000000000,
                             1.0500000000000000,
                             1.4100000000000001,
                             1.6260000000000001,
                             1.8852000000000002,
                             2.1962400000000004,
                             2.4774609356360582};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMaxTracingDistance
//------------------------------------------------------------------------------
/// \brief test behavior when starting point is out of the cell
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testStartOutOfCell()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {-.1, 0, 0};
  double startTime = .5;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {};
  VecDbl expectedOutTimes = {};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testStartOutOfCell
//------------------------------------------------------------------------------
/// \brief test the angle function
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testDotProduct()
{
  TS_ASSERT_DELTA(iGetDirAsCosTheta(0, 1, 1, 0), 0, .1);     // 90 degree angle
  TS_ASSERT_DELTA(iGetDirAsCosTheta(0, 1, 1, 1), .707, .1);  // 45 degree angle
  TS_ASSERT_DELTA(iGetDirAsCosTheta(0, 1, 0, -1), -1, .1);   // 180 degree angle
  TS_ASSERT_DELTA(iGetDirAsCosTheta(1, 1, -1, -1), -1, .1);  // 180 degree angle
  TS_ASSERT_DELTA(iGetDirAsCosTheta(2, 5, 3, 6), .9965, .1); // almost 0 degree angle
  TS_ASSERT_DELTA(iGetDirAsCosTheta(0, 1, 0, 1), 1, .1);     // 0 degree angle
} // XmGridTraceUnitTests::testDotProduct
//------------------------------------------------------------------------------
/// \brief test behavior when starting beyond the second time step
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBeyondTimestep()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 10.1; // Just beyond the 2nd time

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {};
  VecDbl expectedOutTimes = {};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testBeyondTimestep
//------------------------------------------------------------------------------
/// \brief test the behavior when starting before the first timestep
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBeforeTimestep()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = -0.1; // Just beyond the 2nd time

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0}, {1, 1, 0}};
  VecDbl expectedOutTimes = {-.1, .4};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testBeforeTimestep
//------------------------------------------------------------------------------
/// \brief test behavior of the vector multiplier
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testVectorMultiplier()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  tracer->SetMaxChangeDirectionInRadians(XM_PI * .2);
  tracer->SetMinDeltaTime(-1);
  tracer->SetVectorMultiplier(0.5);
  // Push on different scalars
  double time = 0;
  VecPt3d scalars = {{0, 1, 0}, {-1, 0, 0}, {0, -1, 0}, {1, 0, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 4; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  time = 10;
  // Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {0, 0, 0};
  double startTime = .5;
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{0, 0, 0},
                              {0.00000000000000000, 0.25000000000000000, 0.00000000000000000},
                              {0.074999999999999997, 0.47499999999999998, 0.00000000000000000},
                              {0.21900000214576720, 0.63699999570846555, 0.00000000000000000},
                              {0.30928799843788146, 0.66810399758815764, 0.00000000000000000},
                              {0.40229310507774352, 0.67396399235725402, 0.00000000000000000},
                              {0.48679361495018003, 0.65024498560905453, 0.00000000000000000},
                              {0.54780151323509219, 0.59909560095787040, 0.00000000000000000},
                              {0.55928876277122497, 0.56619817004051198, 0.00000000000000000},
                              {0.56114558691518779, 0.53247499044700608, 0.00000000000000000},
                              {0.55189971330840681, 0.50228363992173752, 0.00000000000000000},
                              {0.53269911067322617, 0.48131557500677169, 0.00000000000000000},
                              {0.52076836142536975, 0.47806150355091476, 0.00000000000000000},
                              {0.50886902895577013, 0.47838753608466128, 0.00000000000000000},
                              {0.49867742691962913, 0.48264835153512164, 0.00000000000000000},
                              {0.49224616907898289, 0.49014090685121131, 0.00000000000000000},
                              {0.49175783605462037, 0.49422637094165467, 0.00000000000000000}};
  VecDbl expectedOutTimes = {.5,
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
                             10.000000000000000};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testVectorMultiplier
//------------------------------------------------------------------------------
/// \brief test behavior of multiple cells
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMultiCell()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultTwoCell(tracer);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 0;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0},
                              {0.60000000149011612, 0.50000000000000000, 0.00000000000000000},
                              {0.73200000077486038, 0.50000000000000000, 0.00000000000000000},
                              {0.90940801054239273, 0.50000000000000000, 0.00000000000000000},
                              {1.1529537134766579, 0.50000000000000000, 0.00000000000000000},
                              {1.4957102079987525, 0.50000000000000000, 0.00000000000000000},
                              {1.9923067892670629, 0.50000000000000000, 0.00000000000000000},
                              {2, .5, 0}};
  VecDbl expectedOutTimes = {0,
                             1.0000000000000000,
                             2.2000000000000002,
                             3.6400000000000001,
                             5.3680000000000003,
                             7.4416000000000002,
                             9.9299199999999992,
                             9.9683860530914945};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMultiCell
//------------------------------------------------------------------------------
/// \brief Testing what happens when the maximum change in velocity is low
////         It reaches a point of high acceleration, and then the delta time
////         decreases until it goes below the minimum delta time.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testMaxChangeVelocity()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultTwoCell(tracer);
  tracer->SetMaxChangeVelocity(.01);
  tracer->SetMinDeltaTime(0.001);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 0;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0},
                              {0.60000000149011612, 0.50000000000000000, 0.00000000000000000},
                              {0.66600000113248825, 0.50000000000000000, 0.00000000000000000},
                              {0.74995200067758561, 0.50000000000000000, 0.00000000000000000},
                              {0.80394992786645891, 0.50000000000000000, 0.00000000000000000},
                              {0.87154669338464741, 0.50000000000000000, 0.00000000000000000},
                              {0.95686786960840231, 0.50000000000000000, 0.00000000000000000},
                              {1.0112451727318765, 0.50000000000000000, 0.00000000000000000},
                              {1.0789334834771158, 0.50000000000000000, 0.00000000000000000},
                              {1.1637975516948893, 0.50000000000000000, 0.00000000000000000},
                              {1.2174527417415202, 0.50000000000000000, 0.00000000000000000},
                              {1.2839153379250163, 0.50000000000000000, 0.00000000000000000},
                              {1.3667568384715398, 0.50000000000000000, 0.00000000000000000},
                              {1.4187699365302351, 0.50000000000000000, 0.00000000000000000},
                              {1.4829247317645506, 0.50000000000000000, 0.00000000000000000},
                              {1.5624845364724593, 0.50000000000000000, 0.00000000000000000},
                              {1.6587784227485147, 0.50000000000000000, 0.00000000000000000},
                              {1.7743310862797812, 0.50000000000000000, 0.00000000000000000},
                              {1.9129942825173010, 0.50000000000000000, 0.00000000000000000},
                              {2, .5, 0}};
  VecDbl expectedOutTimes = {0,
                             1.0000000000000000,
                             1.6000000000000001,
                             2.3200000000000003,
                             2.7520000000000002,
                             3.2704000000000004,
                             3.8924800000000004,
                             4.2657280000000002,
                             4.7136256000000003,
                             5.2511027200000004,
                             5.5735889920000004,
                             5.9605725184000002,
                             6.4249527500800001,
                             6.7035808890880002,
                             7.0379346558976001,
                             7.4391591760691202,
                             7.9206286002749442,
                             8.4983919093219331,
                             9.1917078801783187,
                             9.6267364611093829};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testMaxChangeVelocity
//------------------------------------------------------------------------------
/// \brief Test behavior for unique timesteps
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testUniqueTimeSteps()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultTwoCell(tracer);

  double time = 20;
  VecPt3d scalars = {{.2, 0, 0}, {.3, 0, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 2; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity,
                               DataLocationEnum::LOC_CELLS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 10;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0},
                              {0.70000000298023224, 0.50000000000000000, 0.00000000000000000},
                              {0.95200000226497650, 0.50000000000000000, 0.00000000000000000},
                              {1.2734079944372176, 0.50000000000000000, 0.00000000000000000},
                              {1.6897536998434066, 0.50000000000000000, 0.00000000000000000},
                              {2, .5, 0}};
  VecDbl expectedOutTimes = {10,
                             11.000000000000000,
                             12.199999999999999,
                             13.640000000000001,
                             15.368000000000000,
                             16.627525378316030};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testUniqueTimeSteps
//------------------------------------------------------------------------------
/// \brief 2nd cell is inactive in the 2nd time step.
/////        Thus it does not pull as hard. Also once the point reaches the 2nd cell
/////        It stops entirely.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testInactiveCell()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultTwoCell(tracer);

  double time = 20;
  VecPt3d scalars = {{.2, 0, 0}, {99999, 0, 0}};
  DynBitset pointActivity;
  pointActivity.push_back(true);
  pointActivity.push_back(false);
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity,
                               DataLocationEnum::LOC_CELLS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 10;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {{.5, .5, 0},
                              {0.70000000298023224, 0.50000000000000000, 0.00000000000000000},
                              {0.93040000677108770, 0.50000000000000000, 0.00000000000000000},
                              {0.99788877571821222, 0.50000000000000000, 0.00000000000000000}};
  VecDbl expectedOutTimes = {10, 11.000000000000000, 12.199999999999999, 12.560000000000000};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testInactiveCell
//------------------------------------------------------------------------------
/// \brief The point starts in an inactive cell, and doesnt move
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testStartInactiveCell()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultTwoCell(tracer);

  double time = 20;
  VecPt3d scalars = {{1, 0, 0}, {99999, 0, 0}};
  DynBitset pointActivity;
  pointActivity.push_back(false);
  pointActivity.push_back(true);
  tracer->AddGridScalarsAtTime(scalars, DataLocationEnum::LOC_CELLS, pointActivity,
                               DataLocationEnum::LOC_CELLS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 10;

  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);

  VecPt3d expectedOutTrace = {};
  VecDbl expectedOutTimes = {};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testStartInactiveCell
//------------------------------------------------------------------------------
/// \brief 2nd cell is inactive in the 2nd time step.
/////        Thus it does not pull as hard. Also once the point reaches the 2nd cell
/////        It stops entirely.
//------------------------------------------------------------------------------
//! [snip_test_Example_XmGridTrace]
void XmGridTraceUnitTests::testTutorial()
{
  // Graph with scalar vectors indicated
  //  ->   ->
  //  6----7----8|
  //  |    |    |v
  //  |    |    |
  //  |    |    |
  // ^|    |    |
  // |3----4----5|
  //  |    |    |v
  //  |    |    |
  //  |    |    |
  // ^|    |    |
  // |0----1----2
  //      <-   <--
  // Step 1: Create the grid
  VecPt3d points = {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {0, 1, 0}, {1, 1, 0},
                    {2, 1, 0}, {0, 2, 0}, {1, 2, 0}, {2, 2, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 4, 3, XMU_QUAD, 4, 1, 2, 5, 4,
                  XMU_QUAD, 4, 3, 4, 7, 6, XMU_QUAD, 4, 4, 5, 8, 7};
  std::shared_ptr<XmUGrid> ugrid = XmUGrid::New(points, cells);

  // Step 2: Create the tracer from the grid
  BSHP<XmGridTrace> tracer = XmGridTrace::New(ugrid);

  // Step 3: Set up the constraints on the tracer
  tracer->SetVectorMultiplier(2);
  tracer->SetMaxTracingTime(-1);
  tracer->SetMaxTracingDistance(-1);
  tracer->SetMinDeltaTime(.01);
  tracer->SetMaxChangeDistance(-1);
  tracer->SetMaxChangeVelocity(-1);
  tracer->SetMaxChangeDirectionInRadians(XM_PI / 4);

  // Step 4: Set up the velocity vectors for both time steps. Insert timesteps sequentially
  double time = 0;
  // Scalars are set such that they circle around the edge of the graph in a clockwise direction
  // Z component is not used in scalars
  VecPt3d scalars1 = {{0, 1, 0},   {-.1, 0, 0}, {-1, 0, 0}, {0, .1, 0}, {0, 0, 0},
                      {0, -.1, 0}, {1, 0, 0},   {.1, 0, 0}, {0, -1, 0}};
  DynBitset pointActivity;
  for (int i = 0; i < 9; ++i)
  {
    pointActivity.push_back(true);
  }
  tracer->AddGridScalarsAtTime(scalars1, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  // For the second timestep scalars are doubled to indicate an increase in magnitude
  VecPt3d scalars2 = {{0, 2, 0},   {-.2, 0, 0}, {-2, 0, 0}, {0, .2, 0}, {0, 0, 0},
                      {0, -.2, 0}, {2, 0, 0},   {.2, 0, 0}, {0, -2, 0}};
  time = 20;
  // Uses exact same scalars/pointActivity
  tracer->AddGridScalarsAtTime(scalars2, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, time);

  VecPt3d outTrace;
  VecDbl outTimes;
  Pt3d startPoint = {.5, .5, 0};
  double startTime = 0;

  // Step 5: Trace the point
  tracer->TracePoint(startPoint, startTime, outTrace, outTimes);
  // show the cause for termination by calling GetExitMessage
  // std::cout << tracer->GetExitMessage();

  // Expected values for this simulation
  VecPt3d expectedOutTrace = {{0.50000000000000000, 0.50000000000000000, 0.00000000000000000},
                              {0.50000000000000000, 1.2500000000000000, 0.00000000000000000},
                              {0.54457812566426578, 1.3391562513285316, 0.00000000000000000},
                              {0.61632493250262921, 1.4354984729093498, 0.00000000000000000},
                              {0.72535406450374607, 1.5315533661126233, 0.00000000000000000},
                              {0.88236797164001590, 1.6126801842666139, 0.00000000000000000},
                              {0.98873181403598276, 1.6331015959080102, 0.00000000000000000},
                              {1.0538503898747653, 1.6342606013582104, 0.00000000000000000},
                              {1.1249433009705341, 1.5683006835455087, 0.00000000000000000},
                              {1.1895097427498795, 1.3863448896225066, 0.00000000000000000},
                              {1.2235242118635632, 1.0588590059131318, 0.00000000000000000},
                              {1.2235242118635632, 0.90477286425654002, 0.00000000000000000},
                              {1.2005336220528682, 0.85080764250970042, 0.00000000000000000},
                              {1.1581790674742278, 0.79387770198395835, 0.00000000000000000},
                              {1.0896874578697060, 0.74131697161132859, 0.00000000000000000},
                              {0.98966250551038770, 0.70663752692174131, 0.00000000000000000},
                              {0.95806149614159530, 0.71817980325332686, 0.00000000000000000},
                              {0.92629620502521459, 0.77371504022050730, 0.00000000000000000},
                              {0.90239412753251202, 0.88917318465162865, 0.00000000000000000},
                              {0.89995172701803572, 1.0694875660697027, 0.00000000000000000},
                              {0.91503139037776327, 1.0911992829869794, 0.00000000000000000},
                              {0.93816744602651825, 1.1127546977629765, 0.00000000000000000},
                              {0.97140028507849163, 1.1309789606067331, 0.00000000000000000},
                              {0.99364912627842006, 1.1358370729524059, 0.00000000000000000},
                              {1.0071524474802995, 1.1364684019706512, 0.00000000000000000},
                              {1.0223447138862345, 1.1280655805979485, 0.00000000000000000},
                              {1.0369737821057583, 1.0971462034407997, 0.00000000000000000},
                              {1.0467397711865176, 1.0371377237101163, 0.00000000000000000},
                              {1.0467397711865176, 0.96499504248441559, 0.00000000000000000},
                              {1.0390576209755447, 0.95473758230148376, 0.00000000000000000},
                              {1.0276444556154691, 0.94488898976070590, 0.00000000000000000},
                              {1.0208791233912420, 0.94149540451099356, 0.00000000000000000}};
  VecDbl expectedOutTimes = {
    0.00000000000000000, 0.37500000000000000, 0.82499999999999996, 1.3649999999999998,
    2.0129999999999999,  2.7905999999999995,  3.2571599999999994,  3.5370959999999991,
    3.8730191999999990,  4.2761270399999987,  4.7598564479999981,  5.3403317375999979,
    6.0369020851199977,  6.8727865021439971,  7.8758478025727969,  9.0795213630873555,
    9.4406234312417237,  9.8739459130269651,  10.393932891169255,  11.017917264940003,
    11.766698513464901,  12.665236011694777,  13.743481009570628,  14.390428008296139,
    14.778596207531445,  15.244398046613812,  15.803360253512654,  16.474114901791264,
    17.279020479725595,  18.244907173246794,  19.403971205472232,  20.000000000000000};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testTutorial
  //! [snip_test_Example_XmGridTrace]
//------------------------------------------------------------------------------
/// \brief Measures the cost of tracing many seed points over a realistic grid.
///
/// This is the baseline for routing the "follow flow path" vector display option through
/// XmGridTrace: the display traces every visible glyph, so the number that matters is the
/// per-seed cost at glyph counts, not the cost of one trace. Three seed populations are
/// measured separately because they exercise different code:
///
///   interior  seeds far enough from the edge that no trace can reach it -- the pure
///             stepping cost, four ExtractData searches per integration step
///   boundary  seeds in a band along the edge, so traces run out of the domain and pay
///             for a freshly constructed XmUGrid2dPolylineDataExtractor and
///             GmMultiPolyIntersector per exit event, inside the stepping loop
///   mixed     seeds spread over the whole domain -- what the display actually does
///
/// Reported alongside wall time is the ExtractData call count, so a later optimization
/// can be shown to have removed searches rather than merely found a faster machine.
///
/// Seed count and grid size come from XMGT_BENCH_SEEDS and XMGT_BENCH_CELLS so a sweep
/// needs no recompile; the defaults are small enough to leave in the regular suite. The
/// assertions are deliberately loose -- this guards against order-of-magnitude
/// regressions, and a tight bound would only make the suite flaky on shared runners.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testTraceBenchmark()
{
  const int seedCount = iEnvInt("XMGT_BENCH_SEEDS", 250);
  const int cellsPerSide = iEnvInt("XMGT_BENCH_CELLS", 200);
  const double length = 200.0;
  const double omega = 0.05; // vortex rate; reversed at the second time step
  const double drift = 1.0;  // uniform +x velocity, carries seeds off the +x edge
  const double timeStepInterval = 10.0;
  const double maxTracingDistance = 15.0;

  const auto setupStart = std::chrono::steady_clock::now();
  BenchmarkGrid grid = iBuildBenchmarkGrid(cellsPerSide, length);
  const auto gridBuilt = std::chrono::steady_clock::now();

  BSHP<XmGridTrace> tracer = XmGridTrace::New(grid.m_ugrid);
  tracer->SetVectorMultiplier(1);
  tracer->SetMaxTracingTime(timeStepInterval);
  tracer->SetMaxTracingDistance(maxTracingDistance);
  tracer->SetMinDeltaTime(.01);
  tracer->SetMaxChangeDistance(2.0);
  tracer->SetMaxChangeVelocity(-1);
  tracer->SetMaxChangeDirectionInRadians(0.2);

  DynBitset pointActivity;
  pointActivity.resize(grid.m_points.size(), true);
  // The rotation reverses between the two steps, so a trace that spans them is genuinely
  // time dependent -- a single-timestep tracer cannot reproduce its path.
  VecPt3d vectors1 = iBenchmarkVectors(grid.m_points, omega, drift, length);
  VecPt3d vectors2 = iBenchmarkVectors(grid.m_points, -omega, drift, length);
  tracer->AddGridScalarsAtTime(vectors1, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, 0.0);
  tracer->AddGridScalarsAtTime(vectors2, DataLocationEnum::LOC_POINTS, pointActivity,
                               DataLocationEnum::LOC_POINTS, timeStepInterval);
  const auto setupEnd = std::chrono::steady_clock::now();

  const double gridSeconds = std::chrono::duration<double>(gridBuilt - setupStart).count();
  const double scalarSeconds = std::chrono::duration<double>(setupEnd - gridBuilt).count();

  // Break the per-timestep setup cost into its parts. This decides whether two timesteps
  // with *different* cell activity can share one triangulation: activity is not baked into
  // the triangles, it is latched onto the search object (XmUGridTriangles2d.cpp:146-164),
  // so the question is whether flipping it per query is cheaper than triangulating twice.
  DynBitset benchActivity;
  benchActivity.resize(grid.m_ugrid->GetCellCount(), true);

  BSHP<XmUGridTriangles2d> tris = XmUGridTriangles2d::New();
  const auto triStart = std::chrono::steady_clock::now();
  tris->BuildTriangles(*grid.m_ugrid, XmUGridTriangles2d::PO_CENTROIDS_ONLY);
  const auto triBuilt = std::chrono::steady_clock::now();
  tris->SetCellActivity(benchActivity); // first call also builds the GmTriSearch R-tree
  const auto searchBuilt = std::chrono::steady_clock::now();
  tris->SetCellActivity(benchActivity); // second call is the activity mask alone
  const auto activityFlipped = std::chrono::steady_clock::now();

  const double triSeconds = std::chrono::duration<double>(triBuilt - triStart).count();
  const double searchSeconds = std::chrono::duration<double>(searchBuilt - triBuilt).count();
  const double flipSeconds = std::chrono::duration<double>(activityFlipped - searchBuilt).count();

  std::cout << std::fixed << std::setprecision(3) << "\n=== XmGridTrace trace benchmark ===\n"
            << "  grid            " << cellsPerSide << "x" << cellsPerSide << " quads, "
            << grid.m_points.size() << " points\n"
            << "  seeds per set   " << seedCount << "\n"
            << "  grid build      " << gridSeconds * 1e3 << " ms\n"
            << "  add 2 timesteps " << scalarSeconds * 1e3 << " ms\n"
            << "  setup breakdown, one XmUGridTriangles2d:\n"
            << "    BuildTriangles      " << triSeconds * 1e3 << " ms\n"
            << "    + R-tree & activity " << searchSeconds * 1e3 << " ms\n"
            << "    activity flip only  " << flipSeconds * 1e3 << " ms\n"
            << std::flush;

  // No trace can travel maxTracingDistance from this band, so nothing exits the grid.
  const double interiorMargin = maxTracingDistance + 5.0;
  VecPt3d interiorSeeds =
    iBenchmarkSeeds(seedCount, interiorMargin, length - interiorMargin, 0.0, 0.0);
  // Seeds within a band of the edge; the hole rejects anything that is not in the band.
  const double boundaryBand = 5.0;
  VecPt3d boundarySeeds =
    iBenchmarkSeeds(seedCount, 0.5, length - 0.5, boundaryBand, length - boundaryBand);
  VecPt3d mixedSeeds = iBenchmarkSeeds(seedCount, 0.5, length - 0.5, 0.0, 0.0);

  BenchmarkStats interior, boundary, mixed;
  iRunTraceBenchmark(tracer, interiorSeeds, interior);
  iReportTraceBenchmark("interior", interior);
  iRunTraceBenchmark(tracer, boundarySeeds, boundary);
  iReportTraceBenchmark("boundary", boundary);
  iRunTraceBenchmark(tracer, mixedSeeds, mixed);
  iReportTraceBenchmark("mixed", mixed);

  // Interior seeds cannot reach a boundary, so every one of them must trace.
  TS_ASSERT_EQUALS(interior.m_traced, seedCount);
  // Seeds that can leave the grid are not guaranteed a usable polyline: a seed that exits
  // on its first step can hit the "failed to find an intersection when exiting grid" early
  // return (:404-408) and come back holding only the seed point. Measured at roughly 1 in
  // 100,000, so allow a small tail rather than asserting a false invariant -- but keep the
  // bound tight enough that a real breakage in tracing still fails here.
  TS_ASSERT(mixed.m_traced >= seedCount - 1 - seedCount / 1000);
  // The instrumentation itself has to be working, or the search counts mean nothing.
  TS_ASSERT(interior.m_extractCalls > (size_t)seedCount);
  // The boundary set must actually leave the grid, otherwise this benchmark silently
  // stops measuring the per-exit extractor construction it exists to measure.
  const std::string outOfDomain = "Point has traveled out of domain.";
  TS_ASSERT(boundary.m_exitReasons.count(outOfDomain) > 0);
  TS_ASSERT_EQUALS(interior.m_exitReasons.count(outOfDomain), 0);
  // Re-latching activity onto an existing search must stay cheaper than rebuilding the
  // triangulation, or "share one triangulation and flip activity" is not even a candidate.
  TS_ASSERT(flipSeconds < triSeconds);
  // Order-of-magnitude guard only. Measured at ~0.1 ms/seed; 10 ms leaves room for a
  // debug build on a loaded machine while still catching a real algorithmic regression.
  TS_ASSERT(mixed.m_seconds * 1e3 / seedCount < 10.0);
} // XmGridTraceUnitTests::testTraceBenchmark

#endif