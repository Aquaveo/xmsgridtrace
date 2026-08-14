#pragma once
//------------------------------------------------------------------------------
/// \file
/// \brief Contains the XmGridTrace Class and supporting data types.
/// \ingroup ugrid
/// \copyright (C) Copyright Aquaveo 2018. Distributed under FreeBSD License
/// (See accompanying file LICENSE or https://aqaveo.com/bsd/license.txt)
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------

// 3. Standard library headers

// 4. External library headers

// 5. Shared code headers
#include <xmscore/misc/base_macros.h>
#include <xmscore/misc/boost_defines.h>
#include <xmscore/misc/DynBitset.h>
#include <xmscore/stl/vector.h>
#include <xmsextractor/extractor/XmUGrid2dDataExtractor.h>

//----- Forward declarations ---------------------------------------------------

//----- Namespace declaration --------------------------------------------------

/// XMS Namespace
namespace xms
{
//----- Forward declarations ---------------------------------------------------
class XmUGrid;
class dyn_bitset;

//----- Constants / Enumerations -----------------------------------------------

/// \brief Why a trace stopped.
///
/// Reported per trace instead of the message string it replaced. A batch traces every
/// visible glyph -- tens of thousands of them -- and a caller has to be able to tell "left
/// the grid, draw it short" from "spent its distance budget, this is the normal ending"
/// without comparing strings. The old messages could not support that anyway: they were
/// composed by appending, so no fixed string identified a case.
enum XmGridTraceExitEnum {
  GTEXIT_NOT_STARTED,           ///< no stepping has happened yet
  GTEXIT_WAITING_FOR_TIME_STEP, ///< reached the 2nd loaded step; supply a later one to resume
  GTEXIT_MAX_TRACING_TIME,      ///< the trace spent its time budget
  GTEXIT_MAX_TRACING_DISTANCE,  ///< the trace spent its distance budget
  GTEXIT_LEFT_GRID,             ///< stepped out of the grid; the path stops at the boundary
  GTEXIT_ZERO_VELOCITY,         ///< the field went still under the particle
  GTEXIT_MIN_DELTA_TIME,        ///< subdividing reached the smallest allowed step
  GTEXIT_SEED_NOT_TRACEABLE,    ///< the seed was outside the grid or in an inactive cell
  GTEXIT_EXTRACTION_FAILED      ///< a field lookup failed; the trace is discarded
};


//----- Structs / Classes ------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
class XmGridTrace
{
public:
  /// \brief Construct XmGridTrace for a UGrid.
  /// \param[in] a_ugrid a ugrid
  static BSHP<XmGridTrace> New(std::shared_ptr<XmUGrid> a_ugrid);

  /// \brief Deconstruct XmGridTrace.
  virtual ~XmGridTrace();

  /// \brief Returns the vector multiplier
  /// \return the vector multiplier
  virtual double GetVectorMultiplier() const = 0;
  /// \brief Sets the vector multiplier
  /// \param[in] a_vectorMultiplier the new vector multiplier
  virtual void SetVectorMultiplier(const double a_vectorMultiplier) = 0;

  /// \brief Returns the Max Tracing Time
  /// \return the max tracing time
  virtual double GetMaxTracingTime() const = 0;
  /// \brief Sets the max tracing time
  /// \param[in] a_maxTracingTime the new max tracing time
  virtual void SetMaxTracingTime(const double a_maxTracingTime) = 0;

  /// \brief Returns the Max Tracing Distance
  /// \return the max tracing distance
  virtual double GetMaxTracingDistance() const = 0;
  /// \brief Sets the max tracing distance
  /// \param[in] a_maxTracingDistance the new max tracing distance
  virtual void SetMaxTracingDistance(const double a_maxTracingDistance) = 0;

  /// \brief Returns the min delta time
  /// \return the minimum time between trace steps
  virtual double GetMinDeltaTime() const = 0;
  /// \brief Sets the min delta time
  /// \param[in] a_minDeltaTime the new min delta time
  virtual void SetMinDeltaTime(const double a_minDeltaTime) = 0;

  /// \brief Returns the max change distance
  /// \return the maximum change in distance between trace steps
  virtual double GetMaxChangeDistance() const = 0;
  /// \brief Sets the max change distance
  /// \param[in] a_maxChangeDistance the new max change distance
  virtual void SetMaxChangeDistance(const double a_maxChangeDistance) = 0;

  /// \brief Returns the max change in velcoity
  /// \return the maximum change in velocity between trace steps
  virtual double GetMaxChangeVelocity() const = 0;
  /// \brief Sets the max change in velocity
  /// \param[in] a_maxChangeVelocity the new max change in velocity
  virtual void SetMaxChangeVelocity(const double a_maxChangeVelocity) = 0;

  /// \brief Returns the max change in direction in radians
  /// \return the maximum change in direction between trace steps
  virtual double GetMaxChangeDirectionInRadians() const = 0;
  /// \brief Sets the max change in direction in radians
  /// \param[in] a_maxChangeDirection the new max change in direction in radians
  virtual void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) = 0;

  /// \brief Assigns velocity vectors to each point or cell for a time step,
  ///        keeping the previous step, and dropping the one before that
  ///        for a maximum of two time steps.
  /// \param[in] a_scalars The velocity vectors
  /// \param[in] a_scalarLoc Whether the vectors are assigned to cells or points
  /// \param[in] a_activity Whether each cell or point is active
  /// \param[in] a_activityLoc Whether the activities are assigned to cells or points
  /// \param[in] a_time The time of the scalars
  virtual void AddGridScalarsAtTime(const VecPt3d& a_scalars,
                                    DataLocationEnum a_scalarLoc,
                                    const xms::DynBitset& a_activity,
                                    DataLocationEnum a_activityLoc,
                                    double a_time) = 0;

  /// \brief Runs the Grid Trace for a point
  /// \param[in] a_pt The starting point of the trace
  /// \param[in] a_ptTime The starting time of the trace
  /// \param[out] a_outTrace the resultant positions at each step
  /// \param[out] a_outTimes the resultant times at each step
  virtual void TracePoint(const Pt3d& a_pt,
                          const double& a_ptTime,
                          VecPt3d& a_outTrace,
                          VecDbl& a_outTimes) = 0;

  /// \brief Begins tracing a batch of seeds against the currently loaded time steps.
  ///
  /// A trace runs only as far as the second loaded time step, because that is as far as the
  /// field is known. Supply the next time step with AddGridScalarsAtTime and call
  /// ContinueTraces to carry every unfinished trace onward; the two-step window means memory
  /// stays bounded however long the series is, and the caller reads time steps only as the
  /// traces actually need them:
  ///
  /// \code
  /// tracer->StartTraces(seeds, seedTimes);
  /// while (tracer->ContinueTraces() > 0 && series.HasNext())
  ///   tracer->AddGridScalarsAtTime(series.Next(), ...);
  /// tracer->GetTraceResults(traces, times, reasons);
  /// \endcode
  ///
  /// Stopping early is legitimate: traces still waiting simply end where they got to, with
  /// GTEXIT_WAITING_FOR_TIME_STEP. Calling ContinueTraces twice without supplying a time step
  /// in between does no useful work.
  ///
  /// One batch is in flight per tracer, because the time step window it runs against is
  /// itself state on the tracer. Starting a batch discards any previous one.
  ///
  /// \param[in] a_pts The starting point of each trace
  /// \param[in] a_ptTimes The starting time of each trace; must be one per point, or the
  ///            batch is refused entirely
  virtual void StartTraces(const VecPt3d& a_pts, const VecDbl& a_ptTimes) = 0;

  /// \brief Advances every unfinished trace as far as the loaded time steps allow.
  /// \return How many traces are waiting on a later time step. Zero means every trace has
  ///         ended for a reason that more data cannot change.
  virtual int ContinueTraces() = 0;

  /// \brief Copies out the batch traced so far. Valid at any point, complete once
  ///        ContinueTraces has returned zero.
  ///
  /// An entry can hold fewer than two points: a seed that leaves the grid on its very first
  /// step yields only the seed itself, so callers must not assume one usable polyline per
  /// seed.
  ///
  /// \param[out] a_outTraces The positions of each trace, one entry per seed
  /// \param[out] a_outTimes The times of each trace, parallel to and the same length as the
  ///             matching entry of a_outTraces
  /// \param[out] a_outExitReasons Why each trace stopped, one entry per seed
  virtual void GetTraceResults(std::vector<VecPt3d>& a_outTraces,
                               std::vector<VecDbl>& a_outTimes,
                               std::vector<XmGridTraceExitEnum>& a_outExitReasons) const = 0;

  /// \brief Returns a human-readable description of what ended the last trace operation.
  ///        Use GetTraceResults' exit reasons to make decisions; this is for display and logs.
  /// \return the exit message of the last trace operation
  virtual const std::string& GetExitMessage() const = 0;

private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmGridTrace)

protected:
  XmGridTrace();
};

//----- Function prototypes ----------------------------------------------------

/// \brief Returns a human-readable description of an exit reason.
/// \param[in] a_reason The exit reason
/// \return a description suitable for a log or a tooltip
const char* XmGridTraceExitReasonToString(XmGridTraceExitEnum a_reason);

} // namespace xms
