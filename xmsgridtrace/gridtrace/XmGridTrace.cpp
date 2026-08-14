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
#include <xmsextractor/ugrid/XmUGridTriangles2d.h>
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
/// \brief Count of point-location searches since it was last zeroed.
/// Test-build-only instrumentation for testTraceBenchmark. A trace's cost is dominated by
/// these searches, so the benchmark needs the count and not only wall time -- otherwise an
/// algorithmic win cannot be told apart from a faster machine. Not thread safe; the
/// benchmark is single threaded.
size_t g_searchCalls = 0;
/// \brief Adds a_n to the search count. Compiles away outside test builds.
#define XMGT_COUNT_SEARCH(a_n) (g_searchCalls += (a_n))
/// \brief Count of XmUGrid2dPolylineDataExtractor constructions since it was last zeroed.
/// Test-build-only instrumentation for testBoundaryExtractorIsCached. Caching that extractor
/// is a pure performance change with no effect on trace output, so a construction count is
/// the only thing that can tell a cached run from an uncached one.
size_t g_boundaryExtractorBuilds = 0;
/// \brief Records one boundary-extractor construction. Compiles away outside test builds.
#define XMGT_COUNT_BOUNDARY_EXTRACTOR_BUILD() (++g_boundaryExtractorBuilds)
#else
/// \brief No-op outside test builds, so production traces pay nothing for instrumentation.
#define XMGT_COUNT_SEARCH(a_n) ((void)0)
/// \brief No-op outside test builds.
#define XMGT_COUNT_BOUNDARY_EXTRACTOR_BUILD() ((void)0)
#endif

//----- Class / Function definitions -------------------------------------------

//------------------------------------------------------------------------------
/// \brief Whether a reason means the trace can never advance again.
/// \param[in] a_reason The exit reason
/// \return true if no amount of further time step data can move the trace
//------------------------------------------------------------------------------
bool iIsTerminal(XmGridTraceExitEnum a_reason)
{
  return a_reason != GTEXIT_NOT_STARTED && a_reason != GTEXIT_WAITING_FOR_TIME_STEP;
} // iIsTerminal

//------------------------------------------------------------------------------
/// \brief Applies one set of interpolation weights to a time step's x and y scalars.
///
/// Reproduces XmUGrid2dDataExtractor::ExtractData exactly -- accumulating in double, then
/// narrowing to float -- so that replacing four ExtractData calls with one search plus this
/// gives bit-identical answers rather than merely close ones.
/// \param[in] a_x The extractor holding the x component
/// \param[in] a_y The extractor holding the y component, sharing a_x's triangulation
/// \param[in] a_idxs Triangulation point indices from the search
/// \param[in] a_weights Interpolation weights parallel to a_idxs
/// \param[out] a_outX The interpolated x component
/// \param[out] a_outY The interpolated y component
//------------------------------------------------------------------------------
void iApplyWeights(const XmUGrid2dDataExtractor& a_x,
                   const XmUGrid2dDataExtractor& a_y,
                   const VecInt& a_idxs,
                   const VecDbl& a_weights,
                   float& a_outX,
                   float& a_outY)
{
  const VecFlt& xScalars = a_x.GetScalars();
  const VecFlt& yScalars = a_y.GetScalars();
  double interpX = 0.0, interpY = 0.0;
  for (size_t i = 0; i < a_idxs.size(); ++i)
  {
    const int ptIdx = a_idxs[i];
    const double weight = a_weights[i];
    interpX += xScalars[ptIdx] * weight;
    interpY += yScalars[ptIdx] * weight;
  }
  a_outX = static_cast<float>(interpX);
  a_outY = static_cast<float>(interpY);
} // iApplyWeights

////////////////////////////////////////////////////////////////////////////////
/// One trace in progress, and everything about it that has to survive a time step change.
///
/// A trace stops when it reaches the second of the two loaded time steps and continues once
/// a later one is supplied. Position and time are the obvious carry-overs; the rest are the
/// ones whose absence would be a silent defect. The distance and elapsed-time budgets are
/// whole-trace, not per-window. The step size and previous velocity feed the subdivision
/// tests, which compare each step against the one before it -- restarting those at a window
/// boundary would kink the path exactly where the time step changes, which is the one place
/// this has to be smooth.
struct TraceState
{
  Pt3d m_pt;                 ///< current position
  double m_ptTime = 0;       ///< time the trace was released; never advanced
  double m_elapsedTime = 0;  ///< time advanced since release, against m_maxTracingTime
  double m_distTraveled = 0; ///< distance covered, against m_maxTracingDistance
  double m_deltaT = 1.0;     ///< adaptive step size carried into the next step
  double m_vx = 0;           ///< velocity x at m_pt, for the subdivision tests
  double m_vy = 0;           ///< velocity y at m_pt, for the subdivision tests
  double m_mag = 0;          ///< speed at m_pt, for the change-in-velocity test
  bool m_started = false;    ///< the seed has been evaluated and recorded
  /// Why it stopped, or that it is waiting. Doubles as the resume flag -- see iIsTerminal --
  /// so there is one source of truth rather than a reason and a separate finished bool that
  /// could disagree.
  XmGridTraceExitEnum m_exitReason = GTEXIT_NOT_STARTED;
  VecPt3d m_trace; ///< positions so far
  VecDbl m_times;  ///< times so far, parallel to m_trace
};

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
                            const xms::DynBitset& a_activity,
                            DataLocationEnum a_activityLoc,
                            double a_time) final;

  void TracePoint(const Pt3d& a_pt,
                  const double& a_ptTime,
                  VecPt3d& a_outTrace,
                  VecDbl& a_outTimes) final;

  void StartTraces(const VecPt3d& a_pts, const VecDbl& a_ptTimes) final;
  int ContinueTraces() final;
  void GetTraceResults(std::vector<VecPt3d>& a_outTraces,
                       std::vector<VecDbl>& a_outTimes,
                       std::vector<XmGridTraceExitEnum>& a_outExitReasons) const final;

  XmGridTraceExitEnum GetExitReason() const final;
  const std::string& GetExitMessage() const final;

private:
  void StepTrace(TraceState& a_state);

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
  xms::DynBitset m_activity2; ///< activity of the second time step, to compare with the next
  /// Whether both time steps share one triangulation, which they can when their activity
  /// matches. When they do, one search serves all four extractors instead of one per step.
  bool m_sharedAcrossTime = false;
  /// Scratch for the point-location search. Members rather than locals because
  /// GetVectorAtLocationAndTime runs a few dozen times per traced seed and these would
  /// otherwise reallocate on every call. They make the tracer unsafe to share across
  /// threads, which it already was -- GmTriSearch caches barycentric state per query.
  mutable VecInt m_searchIdxs;
  mutable VecDbl m_searchWeights;
  /// Extractor used to find where a trace leaves the grid, built lazily on the first
  /// out-of-domain step and reused for every one after it. Its construction triangulates the
  /// whole grid and its first SetPolyline indexes every triangle into a GmMultiPolyIntersector;
  /// neither depends on the polyline, and both were previously rebuilt per exit event at a
  /// measured ~40 ms each. Null until a trace actually exits, so a tracer whose traces all
  /// stay inside the grid never pays the memory.
  BSHP<XmUGrid2dPolylineDataExtractor> m_boundaryExtractor;
  /// Traces started by StartTracePoints and advanced by ContinueTracePoints. Empty unless
  /// a batch is in flight; one batch per tracer, because the time step window it runs
  /// against is itself instance state.
  std::vector<TraceState> m_batch;

  /// Why the last trace operation ended. Kept beside the message so the single-point
  /// TracePoint can answer the same question GetTraceResults answers per seed.
  XmGridTraceExitEnum m_exitReason = GTEXIT_NOT_STARTED;
  std::string m_exitMessage; ///< exit message for the last trace operation
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
/// \brief returns why the last trace operation ended
/// \return the exit reason of the last trace operation
//------------------------------------------------------------------------------
XmGridTraceExitEnum XmGridTraceImpl::GetExitReason() const
{
  return m_exitReason;
} // XmGridTraceImpl::GetExitReason
//------------------------------------------------------------------------------
/// \brief returns a message describing what caused trace to exit
/// \return the exit message of the last trace operation
//------------------------------------------------------------------------------
const std::string& XmGridTraceImpl::GetExitMessage() const
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
                                           const xms::DynBitset& a_activity,
                                           DataLocationEnum a_activityLoc,
                                           double a_time)
{
  const bool hadPrevious = m_extractor2x && m_extractor2y;
  if (hadPrevious)
  {
    m_extractor1x = m_extractor2x;
    m_extractor1y = m_extractor2y;
    m_time1 = m_time2;
  }

  m_time2 = a_time;
  std::vector<float> xx, yy;
  xx.reserve(a_scalars.size());
  yy.reserve(a_scalars.size());
  for (auto& pt : a_scalars)
  {
    xx.push_back((float)pt.x);
    yy.push_back((float)pt.y);
  }

  // Share the triangulation with the previous time step when the two agree on activity. The
  // triangulation and its GmTriSearch R-tree depend only on the grid and the activity mask,
  // so an identical mask makes them interchangeable -- which both skips a rebuild and lets a
  // single point-location query serve all four extractors instead of one per time step.
  // Differing activity is the case that cannot share, and it is why the mask is compared
  // rather than assumed.
  m_sharedAcrossTime = hadPrevious && a_activity == m_activity2;
  m_extractor2x = m_sharedAcrossTime ? XmUGrid2dDataExtractor::New(m_extractor1x)
                                     : XmUGrid2dDataExtractor::New(m_ugrid);
  if (a_scalarLoc == DataLocationEnum::LOC_POINTS)
    m_extractor2x->SetGridPointScalars(xx, a_activity, a_activityLoc);
  else
    m_extractor2x->SetGridCellScalars(xx, a_activity, a_activityLoc);

  // y is built from x, and only after x's scalars are set. The sharing constructor copies
  // the triangulation *and* the flag saying what it was built for; copying x before it has
  // built one would leave y thinking it must build, and y would then rebuild the very
  // triangulation it is sharing. Only the scalar arrays differ between the two.
  m_extractor2y = XmUGrid2dDataExtractor::New(m_extractor2x);
  if (a_scalarLoc == DataLocationEnum::LOC_POINTS)
    m_extractor2y->SetGridPointScalars(yy, a_activity, a_activityLoc);
  else
    m_extractor2y->SetGridCellScalars(yy, a_activity, a_activityLoc);

  m_activity2 = a_activity;
}

//------------------------------------------------------------------------------
/// \brief Advances one trace as far as the currently loaded pair of time steps allows.
///
/// Starting a trace and resuming one differ only in the prologue: a fresh state has to
/// evaluate and record its seed, while a resumed one already carries a position, its
/// budgets, its step size and its previous velocity.
/// \param[in,out] a_state The trace to advance
//------------------------------------------------------------------------------
void XmGridTraceImpl::StepTrace(TraceState& a_state)
{
  if (iIsTerminal(a_state.m_exitReason))
    return;

  const double ptTime = a_state.m_ptTime;
  Pt3d pt0 = a_state.m_pt, pt1;
  double deltaT = a_state.m_deltaT;
  double elapsedTime = a_state.m_elapsedTime;
  double distTraveled = a_state.m_distTraveled;
  double vx0 = a_state.m_vx, vy0 = a_state.m_vy, mag0 = a_state.m_mag;
  double vx1 = 0, vy1 = 0, mag1 = 0;
  bool bContinue = true;
  Pt3d vtkVec;
  Pt3d vector;
  VecPt3d& outTrace = a_state.m_trace;
  VecDbl& outTimes = a_state.m_times;

  // Writes back everything the next call resumes from. Every exit from this function goes
  // through it, so there is no path that advances the trace without recording where it got to.
  auto stopWith = [&](XmGridTraceExitEnum a_reason) {
    a_state.m_pt = pt0;
    a_state.m_deltaT = deltaT;
    a_state.m_elapsedTime = elapsedTime;
    a_state.m_distTraveled = distTraveled;
    a_state.m_vx = vx0;
    a_state.m_vy = vy0;
    a_state.m_mag = mag0;
    a_state.m_exitReason = a_reason;
    m_exitReason = a_reason;
    m_exitMessage = XmGridTraceExitReasonToString(a_reason);
  };

  if (!a_state.m_started)
  {
    outTrace.clear();
    outTimes.clear();
    if (ptTime > m_time2 || // Test if the time specified is after the time range
        !GetVectorAtLocationAndTime(pt0, ptTime, vector)) // Ensure extraction did not fail
    {
      stopWith(GTEXIT_EXTRACTION_FAILED);
      return;
    }
    if (EQ_TOL(vector.x, XM_NODATA, 1) || EQ_TOL(vector.y, XM_NODATA, 1))
    {
      stopWith(GTEXIT_SEED_NOT_TRACEABLE);
      return;
    }

    outTrace.push_back(pt0);
    outTimes.push_back(ptTime);

    vx0 = vector.x * m_vectorMultiplier;
    vy0 = vector.y * m_vectorMultiplier;
    mag0 = sqrt(vector.x * vector.x + vector.y * vector.y);
    a_state.m_started = true;
  }

  double maxAngleChange = cos(m_maxChangeDirectionInRadians);
  // Which reason the loop will stop with. Tracked explicitly rather than inferred afterwards:
  // several conditions in one iteration overwrite each other, and a later split can put the
  // trace back into motion after the time step clamp has already fired.
  XmGridTraceExitEnum stopReason = GTEXIT_WAITING_FOR_TIME_STEP;

  while (bContinue)
  {
    if (m_maxChangeDistance > 0)
    {
      // make sure deltaT is small enough to not go past the max dist
      double d2 = m_maxChangeDistance * m_maxChangeDistance;
      double denom = (vx0 * vx0) + (vy0 * vy0) + (m_maxChangeDistance * XM_ZERO_TOL);
      double dt = sqrt(d2 / denom);
      if (deltaT > dt)
        deltaT = dt;
    }
    // If the change in DeltaT would push us beyond the time step, set it to hit the timestep
    if (elapsedTime + deltaT + ptTime > m_time2)
    {
      deltaT = m_time2 - elapsedTime - ptTime;
      bContinue = false; // This will be the last point traced in this window
      stopReason = GTEXIT_WAITING_FOR_TIME_STEP;
    }
    // If the change in delta time would push beyond the max tracing time, set it to hit max
    // tracing time
    if (m_maxTracingTime > 0 && (elapsedTime + deltaT) > m_maxTracingTime)
    {
      deltaT = m_maxTracingTime - elapsedTime;
      bContinue = false; // This will be the last point traced
      stopReason = GTEXIT_MAX_TRACING_TIME;
    }

    // compute candidate point
    pt1.x = pt0.x + deltaT * vx0;
    pt1.y = pt0.y + deltaT * vy0;

    if (!GetVectorAtLocationAndTime(pt1, ptTime + elapsedTime + deltaT, vtkVec))
    {
      outTrace.clear();
      outTimes.clear();
      stopWith(GTEXIT_EXTRACTION_FAILED);
      return;
    }
    // if pt1 outside of domain, compute new deltaT to get to boundary
    if (EQ_TOL(vtkVec.x, XM_NODATA, 1) || EQ_TOL(vtkVec.y, XM_NODATA, 1))
    {
      VecPt3d points = {pt0, pt1};
      if (!m_boundaryExtractor)
      {
        // DataLocationEnum is irrelevant here: only the extract locations are consumed below,
        // never the extracted values, so the dummy zero scalars the constructor installs do
        // not matter and the instance stays valid for this tracer's lifetime.
        m_boundaryExtractor =
          XmUGrid2dPolylineDataExtractor::New(m_ugrid, DataLocationEnum::LOC_POINTS);
        XMGT_COUNT_BOUNDARY_EXTRACTOR_BUILD();
      }
      m_boundaryExtractor->SetPolyline(points);
      points = m_boundaryExtractor->GetExtractLocations();
      if (points.size() < 3)
      {
        XM_LOG(xmlog::error, "Gridtracer failed to find an intersection when exiting grid.");
        stopWith(GTEXIT_LEFT_GRID);
        return;
      }
      double segDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
      pt1 = points[points.size() - 2];
      double newSegDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
      deltaT *= (newSegDist / segDist);
      bContinue = false;
      stopReason = GTEXIT_LEFT_GRID;
      if (!GetVectorAtLocationAndTime(pt1, ptTime + elapsedTime + deltaT, vtkVec) ||
          vtkVec.x == XM_NODATA || vtkVec.y == XM_NODATA)
      {
        stopWith(GTEXIT_EXTRACTION_FAILED);
        return;
      }
    }
    vx1 = vtkVec.x;
    vy1 = vtkVec.y;
    vx1 *= m_vectorMultiplier;
    vy1 *= m_vectorMultiplier;

    if (EQ_TOL(vx1, 0.0, .0001) && EQ_TOL(vy1, 0.0, .0001)) // No velocity
    {
      outTrace.push_back(pt1);
      outTimes.push_back(ptTime + elapsedTime + deltaT);
      pt0 = pt1;
      elapsedTime += deltaT;
      stopWith(GTEXIT_ZERO_VELOCITY);
      return;
    }
    bool bSplit = false;

    mag1 = sqrt(vx1 * vx1 + vy1 * vy1);

    // do we subdivide?

    if (!bSplit && m_maxChangeVelocity > 0)
    {
      double changeVel = fabs(mag1 - mag0);
      if (changeVel > m_maxChangeVelocity)
        bSplit = true;
    }
    if (!bSplit && m_maxChangeDirectionInRadians > 0)
    {
      double dir = iGetDirAsCosTheta(vx0, vy0, vx1, vy1);
      if (dir < maxAngleChange)
        bSplit = true;
    }
    if (bSplit)
    {
      // A split puts the trace back in motion, so any stop decided earlier in this iteration
      // is void -- including the time step clamp, which is why resumability cannot be read
      // off the loop's final state without this.
      bContinue = true;
      stopReason = GTEXIT_WAITING_FOR_TIME_STEP;
      deltaT /= 2;
      if (m_minDeltaTime > 0 && deltaT < m_minDeltaTime)
      {
        // done, exit
        bContinue = false;
        stopReason = GTEXIT_MIN_DELTA_TIME;
      }
    }
    else
    {
      double segDist = Mdist(pt0.x, pt0.y, pt1.x, pt1.y);
      distTraveled += segDist;
      if (m_maxTracingDistance > 0 && distTraveled > m_maxTracingDistance)
      {
        // because our last point exceeded the exitDistance
        // find this point by linear calculations
        double distancePast = distTraveled - m_maxTracingDistance;
        double perc = distancePast / segDist;
        Pt3d newPt;
        newPt.x = (pt0.x * perc) + (pt1.x * (1 - perc));
        newPt.y = (pt0.y * perc) + (pt1.y * (1 - perc));

        distTraveled = m_maxTracingDistance;
        outTrace.push_back(newPt);
        outTimes.push_back(ptTime + elapsedTime + deltaT * perc);
        pt0 = newPt;
        elapsedTime += deltaT * perc;
        stopWith(GTEXIT_MAX_TRACING_DISTANCE);
        return;
      }

      // add new pt if not identical to last -- and push its time only when the point is
      // pushed. The time push used to be unconditional, so a step shorter than XM_ZERO_TOL
      // left the times array one longer and silently misaligned every later pair, which a
      // caller reading them as parallel arrays cannot detect.
      const bool moved = outTrace.empty() || !EQ_TOL(pt1.x, outTrace.back().x, XM_ZERO_TOL) ||
                         !EQ_TOL(pt1.y, outTrace.back().y, XM_ZERO_TOL);
      pt0 = pt1;
      elapsedTime += deltaT;
      vx0 = vx1;
      vy0 = vy1;
      deltaT *= 1.2;
      mag0 = mag1;
      if (moved)
      {
        outTrace.push_back(pt1);
        outTimes.push_back(ptTime + elapsedTime);
      }
    }
  } // while ()
  stopWith(stopReason);
} // XmGridTraceImpl::StepTrace
//------------------------------------------------------------------------------
/// \brief Runs the Grid Trace for a point against the currently loaded time steps
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
  TraceState state;
  state.m_pt = a_pt;
  state.m_ptTime = a_ptTime;
  StepTrace(state);
  a_outTrace.swap(state.m_trace);
  a_outTimes.swap(state.m_times);
} // XmGridTraceImpl::TracePoint
//------------------------------------------------------------------------------
/// \brief Begins tracing a batch of seeds against the currently loaded time steps
/// \param[in] a_pts The starting point of each trace
/// \param[in] a_ptTimes The starting time of each trace; must be one per point
//------------------------------------------------------------------------------
void XmGridTraceImpl::StartTraces(const VecPt3d& a_pts, const VecDbl& a_ptTimes)
{
  m_batch.clear();
  if (a_pts.size() != a_ptTimes.size())
  {
    // Refusing the whole batch rather than seeding the common prefix: a caller that
    // mismatched these has a bug, and a partial batch would let it go unnoticed.
    XM_LOG(xmlog::error, "Gridtracer: StartTraces needs one start time per point.");
    return;
  }
  m_batch.resize(a_pts.size());
  for (size_t i = 0; i < a_pts.size(); ++i)
  {
    m_batch[i].m_pt = a_pts[i];
    m_batch[i].m_ptTime = a_ptTimes[i];
  }
} // XmGridTraceImpl::StartTraces
//------------------------------------------------------------------------------
/// \brief Advances every unfinished trace as far as the loaded time steps allow
/// \return How many traces are waiting on a later time step
//------------------------------------------------------------------------------
int XmGridTraceImpl::ContinueTraces()
{
  int waiting = 0;
  for (auto& state : m_batch)
  {
    StepTrace(state); // returns immediately for traces that are already finished
    if (state.m_exitReason == GTEXIT_WAITING_FOR_TIME_STEP)
      ++waiting;
  }
  return waiting;
} // XmGridTraceImpl::ContinueTraces
//------------------------------------------------------------------------------
/// \brief Copies out the batch traced so far
/// \param[out] a_outTraces The positions of each trace, one entry per seed
/// \param[out] a_outTimes The times of each trace, parallel to a_outTraces
/// \param[out] a_outExitReasons Why each trace stopped, one entry per seed
//------------------------------------------------------------------------------
void XmGridTraceImpl::GetTraceResults(std::vector<VecPt3d>& a_outTraces,
                                      std::vector<VecDbl>& a_outTimes,
                                      std::vector<XmGridTraceExitEnum>& a_outExitReasons) const
{
  a_outTraces.clear();
  a_outTimes.clear();
  a_outExitReasons.clear();
  a_outTraces.reserve(m_batch.size());
  a_outTimes.reserve(m_batch.size());
  a_outExitReasons.reserve(m_batch.size());
  for (const auto& state : m_batch)
  {
    a_outTraces.push_back(state.m_trace);
    a_outTimes.push_back(state.m_times);
    a_outExitReasons.push_back(state.m_exitReason);
  }
} // XmGridTraceImpl::GetTraceResults
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
  if (!m_extractor1x || !m_extractor1y || !m_extractor2x || !m_extractor2y)
  {
    // Two time steps are required. This used to dereference a null first extractor when only
    // one had been supplied.
    XM_LOG(xmlog::error, "Gridtracer: two time steps must be added before tracing.");
    return false;
  }

  // One point-location query per distinct triangulation, rather than one per scalar array.
  // The weights returned index the triangulation's points, and every extractor sharing that
  // triangulation indexes its own scalars the same way, so a single query serves the x and y
  // of a time step -- and both time steps too when they share a triangulation.
  float x1 = m_extractor1x->GetNoDataValue();
  float y1 = m_extractor1y->GetNoDataValue();
  const int cell1 =
    m_extractor1x->GetUGridTriangles()->GetIntersectedCell(a_pt, m_searchIdxs, m_searchWeights);
  XMGT_COUNT_SEARCH(1);
  if (cell1 >= 0)
    iApplyWeights(*m_extractor1x, *m_extractor1y, m_searchIdxs, m_searchWeights, x1, y1);

  float x2 = m_extractor2x->GetNoDataValue();
  float y2 = m_extractor2y->GetNoDataValue();
  if (m_sharedAcrossTime)
  {
    if (cell1 >= 0)
      iApplyWeights(*m_extractor2x, *m_extractor2y, m_searchIdxs, m_searchWeights, x2, y2);
  }
  else
  {
    const int cell2 =
      m_extractor2x->GetUGridTriangles()->GetIntersectedCell(a_pt, m_searchIdxs, m_searchWeights);
    XMGT_COUNT_SEARCH(1);
    if (cell2 >= 0)
      iApplyWeights(*m_extractor2x, *m_extractor2y, m_searchIdxs, m_searchWeights, x2, y2);
  }

  if (a_currentTime < m_time1 - XM_ZERO_TOL)
  {
    XM_LOG(xmlog::warning, "Gridtracer: The given time is before the first time step.");
    a_currentTime = m_time1;
  }
  // A location outside the grid or in an inactive cell in *either* bracketing timestep has no
  // usable velocity, and the sentinel must be propagated rather than weighted: blending
  // XM_NODATA (-9999999) against a real value produces something like -999999.9, which is
  // neither no-data nor meaningful, and every caller tests for XM_NODATA exactly. Returning
  // true is correct -- extraction succeeded, and no-data is the answer.
  if (EQ_TOL(x1, XM_NODATA, 1) || EQ_TOL(y1, XM_NODATA, 1) || EQ_TOL(x2, XM_NODATA, 1) ||
      EQ_TOL(y2, XM_NODATA, 1))
  {
    a_data.x = XM_NODATA;
    a_data.y = XM_NODATA;
    return true;
  }

  double totalTime = fabs(m_time1 - m_time2);
  // Each timestep is weighted by its *closeness* to the current time, so the distance from
  // one timestep is the weight of the other: at a_currentTime == m_time1 the field is
  // entirely timestep 1's. Weighting each timestep by its own distance instead -- which is
  // what this did until the weights were swapped -- inverts the interpolation, advecting a
  // particle released at m_time1 entirely by the field at m_time2.
  double weight1 = fabs(a_currentTime - m_time2) / totalTime;
  double weight2 = fabs(a_currentTime - m_time1) / totalTime;
  a_data.x = x1 * weight1 + x2 * weight2;
  a_data.y = y1 * weight1 + y2 * weight2;
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
//------------------------------------------------------------------------------
/// \brief Returns a human-readable description of an exit reason.
/// \param[in] a_reason The exit reason
/// \return a description suitable for a log or a tooltip
//------------------------------------------------------------------------------
const char* XmGridTraceExitReasonToString(XmGridTraceExitEnum a_reason)
{
  switch (a_reason)
  {
  case GTEXIT_NOT_STARTED:
    return "Trace has not started.";
  case GTEXIT_WAITING_FOR_TIME_STEP:
    return "Trace reached the second time step and is waiting for a later one.";
  case GTEXIT_MAX_TRACING_TIME:
    return "Exceeded or reached max tracing time.";
  case GTEXIT_MAX_TRACING_DISTANCE:
    return "Point has reached or exceeded the max tracing distance.";
  case GTEXIT_LEFT_GRID:
    return "Point has traveled out of domain.";
  case GTEXIT_ZERO_VELOCITY:
    return "Velocity has gone to zero.";
  case GTEXIT_MIN_DELTA_TIME:
    return "Delta time was less than min delta time.";
  case GTEXIT_SEED_NOT_TRACEABLE:
    return "Point does not start inside an active cell.";
  case GTEXIT_EXTRACTION_FAILED:
    return "Error occurred while extracting a vector.";
  }
  return "Unknown exit reason.";
} // XmGridTraceExitReasonToString

} // namespace xms
#ifdef CXX_TEST
#include <xmsgridtrace/gridtrace/XmGridTrace.t.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <algorithm>
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
  size_t m_searchCalls = 0;                 ///< point-location searches consumed
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
  g_searchCalls = 0;
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
  a_stats.m_searchCalls = g_searchCalls;

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
  const double searchesPerSeed = a_stats.m_searchCalls / seeds;
  const double usPerExtract =
    a_stats.m_searchCalls ? a_stats.m_seconds * 1e6 / a_stats.m_searchCalls : 0.0;
  const double ptsPerTrace =
    a_stats.m_traced ? (double)a_stats.m_tracePoints / a_stats.m_traced : 0.0;

  std::cout << std::fixed << std::setprecision(3) << "\n  [" << a_label
            << "] seeds=" << a_stats.m_seeds << " traced=" << a_stats.m_traced << "\n"
            << "    wall            " << a_stats.m_seconds * 1e3 << " ms\n"
            << "    per seed        " << usPerSeed << " us\n"
            << "    searches        " << a_stats.m_searchCalls << " (" << std::setprecision(1)
            << searchesPerSeed << "/seed, " << std::setprecision(3) << usPerExtract << " us/call)\n"
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

  VecPt3d expectedOutTrace = {{0.5, 0.5, 0},
                              {0.60000000149011612, 0.5, 0},
                              {0.74400000184774395, 0.5, 0},
                              {0.95481600679159162, 0.5, 0},
                              {1.2691074101881981, 0.5, 0},
                              {1.747260385068264, 0.5, 0},
                              {2, 0.5, 0}};
  VecDbl expectedOutTimes = {10,
                             11,
                             12.199999999999999,
                             13.640000000000001,
                             15.368,
                             17.441600000000001,
                             18.362609001148471};
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

  VecPt3d expectedOutTrace = {{0.5, 0.5, 0},
                              {0.60000000149011612, 0.5, 0},
                              {0.74280000120401379, 0.5, 0},
                              {0.94575130454301826, 0.5, 0},
                              {1, 0.5, 0}};
  VecDbl expectedOutTimes = {10,
                             11,
                             12.199999999999999,
                             13.640000000000001,
                             13.969279307058475};
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
  VecPt3d expectedOutTrace = {{0.5, 0.5, 0},
                              {0.5, 1.5, 0},
                              {0.62600000187754634, 1.6260000018775462, 0},
                              {0.82611968728899965, 1.7455603212296962, 0},
                              {0.97840008102011689, 1.7810753047635555, 0},
                              {1.0280095840364933, 1.7824472100312621, 0},
                              {1.0861189816907613, 1.7608732599310344, 0},
                              {1.1492686295114336, 1.6802752810470523, 0},
                              {1.2097920698566107, 1.5101408581884392, 0},
                              {1.2515951471975522, 1.2181485463468757, 0},
                              {1.2515951471975522, 0.84053651390559747, 0},
                              {1.2181758214493843, 0.78780883088769804, 0},
                              {1.1632869448015855, 0.73137186792498654, 0},
                              {1.0771209832183524, 0.67899546053648097, 0},
                              {1.0129487663521615, 0.66357815692798783, 0},
                              {0.97169356095126669, 0.66199025753694563, 0},
                              {0.92552080990281416, 0.70419149113367874, 0},
                              {0.88530832700558759, 0.83950990950827409, 0},
                              {0.87513974259796246, 1.0941588844381676, 0},
                              {0.90077009637050098, 1.128146252166127, 0},
                              {0.943692705404238, 1.1613833261644337, 0},
                              {0.97709108330292604, 1.1730361561747586, 0},
                              {0.99894959169213471, 1.1759300874982919, 0},
                              {1.0124203987349505, 1.1760105163064269, 0},
                              {1.0275428271398932, 1.1645289800266216, 0},
                              {1.042848666622334, 1.1337546211004945, 0},
                              {1.055142468614698, 1.0758075939238765, 0},
                              {1.0585305184379035, 0.98540145004498747, 0},
                              {1.0556233679912082, 0.97374570199926891, 0},
                              {1.0492587242876892, 0.9602613226646981, 0},
                              {1.0375007181419984, 0.94568649411103145, 0},
                              {1.017827020259642, 0.93210280494582176, 0},
                              {1.0175992759724071, 0.93204300863222744, 0}};
  VecDbl expectedOutTimes = {0,
                             1,
                             2.2000000000000002,
                             3.6400000000000001,
                             4.5040000000000004,
                             4.7632000000000003,
                             5.0742400000000005,
                             5.4474880000000008,
                             5.8953856000000009,
                             6.432862720000001,
                             7.0778352640000008,
                             7.8518023168000006,
                             8.7805627801600004,
                             9.8950753361920007,
                             10.563782869811201,
                             10.96500738998272,
                             11.446476814188543,
                             12.024240123235531,
                             12.717556094091917,
                             13.54953525911958,
                             14.547910257152775,
                             15.146935255972693,
                             15.506350255264643,
                             15.721999254839814,
                             15.980778054330019,
                             16.291312613718265,
                             16.663954084984159,
                             17.111123850503233,
                             17.647727569126122,
                             18.291652031473589,
                             19.064361386290546,
                             19.991612612070895,
                             20};
  TS_ASSERT_DELTA_VECPT3D(expectedOutTrace, outTrace, .0001);
  TS_ASSERT_DELTA_VEC(expectedOutTimes, outTimes, .0001);
} // XmGridTraceUnitTests::testTutorial
  //! [snip_test_Example_XmGridTrace]
//------------------------------------------------------------------------------
/// \brief A trace through a field that changes between timesteps follows neither timestep.
///
/// This is the regression guard for the time interpolation, which is the whole reason this
/// tracer is worth routing a display option through: a tracer that samples one frozen
/// timestep would be no better than the render-time drifter it replaces.
///
/// The field rotates from +x at the first timestep to +y at the second rather than
/// reversing, so the interpolated velocity never passes through zero and cannot trip the
/// "velocity has gone to zero" exit partway along.
///
/// The first assertion is the one that catches an inverted interpolation: a particle
/// released exactly at the first timestep must be advected by that timestep's field alone,
/// so its first step is due east with y untouched. Weighting each timestep by its own
/// distance from the current time instead sends that first step due north.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testTimeVaryingFieldChangesPath()
{
  // One cell spanning the whole domain, so cell-located scalars give a spatially uniform
  // field and any curvature in the path can only have come from time.
  VecPt3d points = {{0, 0, 0}, {10, 0, 0}, {10, 10, 0}, {0, 10, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  std::shared_ptr<XmUGrid> ugrid = XmUGrid::New(points, cells);

  DynBitset activity;
  activity.push_back(true);

  auto traceWithField = [&](const Pt3d& a_first, const Pt3d& a_second, VecPt3d& a_outTrace) {
    BSHP<XmGridTrace> tracer = XmGridTrace::New(ugrid);
    tracer->SetVectorMultiplier(1);
    tracer->SetMaxTracingTime(5);
    tracer->SetMaxTracingDistance(100);
    tracer->SetMinDeltaTime(.01);
    tracer->SetMaxChangeDistance(.5);
    tracer->SetMaxChangeVelocity(-1);
    tracer->SetMaxChangeDirectionInRadians(XM_PI); // never subdivide on direction
    VecPt3d first = {a_first};
    VecPt3d second = {a_second};
    tracer->AddGridScalarsAtTime(first, DataLocationEnum::LOC_CELLS, activity,
                                 DataLocationEnum::LOC_CELLS, 0.0);
    tracer->AddGridScalarsAtTime(second, DataLocationEnum::LOC_CELLS, activity,
                                 DataLocationEnum::LOC_CELLS, 10.0);
    VecDbl outTimes;
    tracer->TracePoint({1, 1, 0}, 0.0, a_outTrace, outTimes);
    TS_ASSERT_EQUALS(a_outTrace.size(), outTimes.size());
  };

  const Pt3d startPoint = {1, 1, 0};

  VecPt3d rotating;
  traceWithField({1, 0, 0}, {0, 1, 0}, rotating);

  // The same field at both timesteps -- what a single-timestep tracer would produce.
  VecPt3d frozen;
  traceWithField({1, 0, 0}, {1, 0, 0}, frozen);

  TS_ASSERT(rotating.size() >= 3);
  TS_ASSERT(frozen.size() >= 3);

  // Released at the first timestep, so the first step is that timestep's field alone.
  TS_ASSERT_DELTA(startPoint.y, rotating[1].y, 1e-9);
  TS_ASSERT(rotating[1].x > startPoint.x);

  // A frozen field never turns.
  for (size_t i = 0; i < frozen.size(); ++i)
  {
    TS_ASSERT_DELTA(startPoint.y, frozen[i].y, 1e-9);
  }

  // A changing one does, and that difference is the feature.
  TS_ASSERT(rotating.back().y > startPoint.y + 0.1);
  TS_ASSERT(rotating.back().x < frozen.back().x);
} // XmGridTraceUnitTests::testTimeVaryingFieldChangesPath
//------------------------------------------------------------------------------
/// \brief A single-window batch returns exactly what serial TracePoint calls return.
///
/// The batch exists to cross a language boundary once instead of once per seed, so its value
/// depends on being a faithful stand-in. Comparing against serial TracePoint on an identical
/// fixture is a stronger oracle than a recorded baseline, which would drift with the tracer
/// rather than pin the equivalence.
///
/// The seeds cover the shapes a caller has to handle: traces that leave the grid, and a seed
/// outside the grid entirely, which yields no polyline at all.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBatchMatchesSerialTracePoint()
{
  const VecPt3d seeds = {{.5, .5, 0}, {.25, .75, 0}, {-.1, 0, 0}};
  const VecDbl seedTimes = {.5, .5, .5};

  BSHP<XmGridTrace> serialTracer;
  iCreateDefaultSingleCell(serialTracer);
  std::vector<VecPt3d> serialTraces(seeds.size());
  std::vector<VecDbl> serialTimes(seeds.size());
  std::vector<XmGridTraceExitEnum> serialReasons(seeds.size());
  for (size_t i = 0; i < seeds.size(); ++i)
  {
    serialTracer->TracePoint(seeds[i], seedTimes[i], serialTraces[i], serialTimes[i]);
    serialReasons[i] = serialTracer->GetExitReason();
  }

  BSHP<XmGridTrace> batchTracer;
  iCreateDefaultSingleCell(batchTracer);
  batchTracer->StartTraces(seeds, seedTimes);
  batchTracer->ContinueTraces();
  std::vector<VecPt3d> batchTraces;
  std::vector<VecDbl> batchTimes;
  std::vector<XmGridTraceExitEnum> reasons;
  batchTracer->GetTraceResults(batchTraces, batchTimes, reasons);

  TS_ASSERT_EQUALS(seeds.size(), batchTraces.size());
  TS_ASSERT_EQUALS(seeds.size(), batchTimes.size());
  TS_ASSERT_EQUALS(seeds.size(), reasons.size());
  for (size_t i = 0; i < seeds.size(); ++i)
  {
    TS_ASSERT_DELTA_VECPT3D(serialTraces[i], batchTraces[i], 1e-12);
    TS_ASSERT_DELTA_VEC(serialTimes[i], batchTimes[i], 1e-12);
    // GetExitReason is the single-point path's answer to what GetTraceResults reports per
    // seed; if they can disagree, a caller cannot use TracePoint and the batch interchangeably.
    TS_ASSERT_EQUALS((int)reasons[i], (int)serialReasons[i]);
    // Positions and times are documented as parallel arrays, so a caller may zip them.
    TS_ASSERT_EQUALS(batchTraces[i].size(), batchTimes[i].size());
  }

  // The seed outside the grid produces no polyline -- callers cannot assume one per seed.
  TS_ASSERT(batchTraces[2].empty());
  TS_ASSERT_EQUALS((int)GTEXIT_SEED_NOT_TRACEABLE, (int)reasons[2]);
  TS_ASSERT(batchTraces[0].size() >= 2);
  TS_ASSERT(batchTraces[1].size() >= 2);

  // A caller supplying the wrong number of start times has a bug; seeding the common prefix
  // would hide it, so the whole batch is refused.
  batchTracer->StartTraces(seeds, {.5});
  batchTracer->GetTraceResults(batchTraces, batchTimes, reasons);
  TS_ASSERT(batchTraces.empty());
  TS_ASSERT(batchTimes.empty());
  TS_ASSERT(reasons.empty());
} // XmGridTraceUnitTests::testBatchMatchesSerialTracePoint
//------------------------------------------------------------------------------
/// \brief A trace continues past the second time step once a later one is supplied.
///
/// This is the point of the whole batch design: the field is only known between the two
/// loaded time steps, so a trace that wants to run further has to stop, ask for more, and
/// resume where it was -- carrying its budgets, its adaptive step size and its previous
/// velocity with it.
///
/// The strongest assertion here is not that the continued trace is longer. It is that the
/// trace which never received the third time step is a byte-for-byte *prefix* of the one
/// that did. That is what shows resuming extends the path rather than recomputing it, and it
/// is what would fail if any carried-over state were dropped at the window boundary.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testTracesContinueAcrossTimeSteps()
{
  // One cell spanning the domain, so the field is spatially uniform and every change in the
  // path comes from time. It rotates +x -> +y -> -x across three time steps.
  VecPt3d points = {{0, 0, 0}, {40, 0, 0}, {40, 40, 0}, {0, 40, 0}};
  VecInt cells = {XMU_QUAD, 4, 0, 1, 2, 3};
  std::shared_ptr<XmUGrid> ugrid = XmUGrid::New(points, cells);

  DynBitset activity;
  activity.push_back(true);
  const VecPt3d seeds = {{20, 10, 0}};
  const VecDbl seedTimes = {0};

  auto buildTracer = [&]() {
    BSHP<XmGridTrace> tracer = XmGridTrace::New(ugrid);
    tracer->SetVectorMultiplier(1);
    tracer->SetMaxTracingTime(18);
    tracer->SetMaxTracingDistance(1000);
    tracer->SetMinDeltaTime(.01);
    tracer->SetMaxChangeDistance(.5);
    tracer->SetMaxChangeVelocity(-1);
    tracer->SetMaxChangeDirectionInRadians(XM_PI); // never subdivide on direction
    VecPt3d east = {{1, 0, 0}}, north = {{0, 1, 0}};
    tracer->AddGridScalarsAtTime(east, DataLocationEnum::LOC_CELLS, activity,
                                 DataLocationEnum::LOC_CELLS, 0.0);
    tracer->AddGridScalarsAtTime(north, DataLocationEnum::LOC_CELLS, activity,
                                 DataLocationEnum::LOC_CELLS, 10.0);
    return tracer;
  };

  // Never given the third time step: it must stop at the second and say so.
  BSHP<XmGridTrace> stopped = buildTracer();
  stopped->StartTraces(seeds, seedTimes);
  TS_ASSERT_EQUALS(1, stopped->ContinueTraces());
  std::vector<VecPt3d> stoppedTraces;
  std::vector<VecDbl> stoppedTimes;
  std::vector<XmGridTraceExitEnum> stoppedReasons;
  stopped->GetTraceResults(stoppedTraces, stoppedTimes, stoppedReasons);
  TS_ASSERT_EQUALS((int)GTEXIT_WAITING_FOR_TIME_STEP, (int)stoppedReasons[0]);
  TS_ASSERT_DELTA(10.0, stoppedTimes[0].back(), 1e-9);

  // Given the third: it must resume and run out its tracing time instead.
  BSHP<XmGridTrace> continued = buildTracer();
  continued->StartTraces(seeds, seedTimes);
  TS_ASSERT_EQUALS(1, continued->ContinueTraces());
  VecPt3d west = {{-1, 0, 0}};
  continued->AddGridScalarsAtTime(west, DataLocationEnum::LOC_CELLS, activity,
                                  DataLocationEnum::LOC_CELLS, 20.0);
  TS_ASSERT_EQUALS(0, continued->ContinueTraces());
  std::vector<VecPt3d> traces;
  std::vector<VecDbl> times;
  std::vector<XmGridTraceExitEnum> reasons;
  continued->GetTraceResults(traces, times, reasons);
  TS_ASSERT_EQUALS((int)GTEXIT_MAX_TRACING_TIME, (int)reasons[0]);
  TS_ASSERT_DELTA(18.0, times[0].back(), 1e-9);

  // Resuming extends; it does not restart.
  TS_ASSERT(traces[0].size() > stoppedTraces[0].size());
  for (size_t i = 0; i < stoppedTraces[0].size(); ++i)
  {
    TS_ASSERT_DELTA(stoppedTraces[0][i].x, traces[0][i].x, 1e-12);
    TS_ASSERT_DELTA(stoppedTraces[0][i].y, traces[0][i].y, 1e-12);
    TS_ASSERT_DELTA(stoppedTimes[0][i], times[0][i], 1e-12);
  }

  // The third time step reverses the eastward drift, so the path must turn back on itself --
  // something no single pair of these time steps can produce.
  double maxX = traces[0][0].x;
  for (const auto& pt : traces[0])
    maxX = std::max(maxX, pt.x);
  TS_ASSERT(maxX > seeds[0].x);
  TS_ASSERT(traces[0].back().x < maxX);
} // XmGridTraceUnitTests::testTracesContinueAcrossTimeSteps
//------------------------------------------------------------------------------
/// \brief Verifies the boundary-exit extractor is built once per tracer, not once per exit.
///
/// The extractor's constructor triangulates the whole grid and its first SetPolyline indexes
/// every triangle into a GmMultiPolyIntersector -- both grid-only work, and both measured at
/// ~40 ms per exit event when rebuilt inside the stepping loop. Caching it changes no output,
/// so the construction count is what has to be asserted; the trace comparison is here to
/// catch the reuse silently changing an answer.
//------------------------------------------------------------------------------
void XmGridTraceUnitTests::testBoundaryExtractorIsCached()
{
  BSHP<XmGridTrace> tracer;
  iCreateDefaultSingleCell(tracer);

  // The default single cell has a uniform (1, 1) field, so a trace from the middle leaves the
  // grid on its first step.
  const Pt3d startPoint = {.5, .5, 0};
  const double startTime = .5;
  const std::string outOfDomain = "Point has traveled out of domain.";

  g_boundaryExtractorBuilds = 0;

  VecPt3d firstTrace;
  VecDbl firstTimes;
  tracer->TracePoint(startPoint, startTime, firstTrace, firstTimes);
  TS_ASSERT_EQUALS(outOfDomain, tracer->GetExitMessage());
  TS_ASSERT_EQUALS(size_t(1), g_boundaryExtractorBuilds);
  TS_ASSERT(firstTrace.size() >= 2);

  VecPt3d secondTrace;
  VecDbl secondTimes;
  tracer->TracePoint(startPoint, startTime, secondTrace, secondTimes);
  TS_ASSERT_EQUALS(outOfDomain, tracer->GetExitMessage());
  TS_ASSERT_EQUALS(size_t(1), g_boundaryExtractorBuilds);

  TS_ASSERT_DELTA_VECPT3D(firstTrace, secondTrace, 1e-12);
  TS_ASSERT_DELTA_VEC(firstTimes, secondTimes, 1e-12);
} // XmGridTraceUnitTests::testBoundaryExtractorIsCached
//------------------------------------------------------------------------------
/// \brief Measures the cost of tracing many seed points over a realistic grid.
///
/// This is the baseline for routing the "follow flow path" vector display option through
/// XmGridTrace: the display traces every visible glyph, so the number that matters is the
/// per-seed cost at glyph counts, not the cost of one trace. Three seed populations are
/// measured separately because they exercise different code:
///
///   interior  seeds far enough from the edge that no trace can reach it -- the pure
///             stepping cost, one point-location search per triangulation per step
///   boundary  seeds in a band along the edge, so traces run out of the domain and pay for
///             the XmUGrid2dPolylineDataExtractor path -- a whole-grid triangulation plus a
///             GmMultiPolyIntersector, once per tracer since that extractor is cached
///             (it was once per exit event, inside the stepping loop)
///   mixed     seeds spread over the whole domain -- what the display actually does
///
/// Reported alongside wall time is the point-location search count, so a later optimization
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
  TS_ASSERT(interior.m_searchCalls > (size_t)seedCount);
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