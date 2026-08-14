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
                                    xms::DynBitset& a_activity,
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

  /// \brief Runs the Grid Trace for many points against the current two time steps.
  ///
  /// Equivalent to calling TracePoint once per point, but crossing a language or module
  /// boundary once instead of once per point, and reporting why every trace ended rather
  /// than only the last -- GetExitMessage describes a single operation, so it cannot
  /// answer that for a batch.
  ///
  /// The time steps are not advanced: every trace runs against whichever pair
  /// AddGridScalarsAtTime has most recently supplied. Callers wanting traces that span more
  /// of a series feed the next time step and trace again.
  ///
  /// A_outTraces[i] can hold fewer than two points. A seed that leaves the grid on its very
  /// first step yields only the seed itself, so callers must not assume one usable polyline
  /// per point.
  ///
  /// \param[in] a_pts The starting point of each trace
  /// \param[in] a_ptTimes The starting time of each trace; must be one per point
  /// \param[out] a_outTraces The resultant positions at each step, one entry per point
  /// \param[out] a_outTimes The resultant times, parallel to and the same length as
  ///             the matching entry of a_outTraces
  /// \param[out] a_outExitMessages What ended each trace, one entry per point
  virtual void TracePoints(const VecPt3d& a_pts,
                           const VecDbl& a_ptTimes,
                           std::vector<VecPt3d>& a_outTraces,
                           std::vector<VecDbl>& a_outTimes,
                           VecStr& a_outExitMessages) = 0;

  /// \brief returns a message describing what caused trace to exit
  /// \return the exit message of the last TracePoint operation
  virtual std::string GetExitMessage() = 0;

private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmGridTrace)

protected:
  XmGridTrace();
};

//----- Function prototypes ----------------------------------------------------

} // namespace xms
