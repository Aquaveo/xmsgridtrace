#pragma once
//------------------------------------------------------------------------------
/// \file
/// \brief Contains the XmGridTrace Class and supporting data types.
/// \ingroup ugrid
/// \copyright (C) Copyright Aquaveo 2018. Distributed under the xmsng
///  Software License, Version 1.0. (See accompanying file
///  LICENSE_1_0.txt or copy at http://www.aquaveo.com/xmsng/LICENSE_1_0.txt)
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
  static BSHP<XmGridTrace> New(BSHP<XmUGrid> a_ugrid);

  /// \brief Deconstruct XmGridTrace.
  virtual ~XmGridTrace() {};

  /// \brief Returns the vector multiplier
  virtual double GetVectorMultiplier() const = 0;
  /// \brief Sets the vector multiplier
  /// \param[in] a_vectorMultiplier the new vector multiplier
  virtual void SetVectorMultiplier(const double a_vectorMultiplier) = 0;

  /// \brief Returns the Max Tracing Time 
  virtual double GetMaxTracingTime() const = 0;
  /// \brief Sets the max tracing time 
  /// \param[in] a_maxTracingTime the new max tracing time 
  virtual void SetMaxTracingTime(const double a_maxTracingTime) = 0;

  /// \brief Returns the Max Tracing Distance 
  virtual double GetMaxTracingDistance() const = 0;
  /// \brief Sets the max tracing distance 
  /// \param[in] a_maxTracingDistance the new max tracing distance 
  virtual void SetMaxTracingDistance(const double a_maxTracingDistance) = 0;

  /// \brief Returns the min delta time 
  virtual double GetMinDeltaTime() const = 0;
  /// \brief Sets the min delta time 
  /// \param[in] a_minDeltaTime the new min delta time 
  virtual void SetMinDeltaTime(const double a_minDeltaTime) = 0;

  /// \brief Returns the max change distance 
  virtual double GetMaxChangeDistance() const = 0;
  /// \brief Sets the max change distance 
  /// \param[in] a_maxChangeDistance the new max change distance 
  virtual void SetMaxChangeDistance(const double a_maxChangeDistance) = 0;

  /// \brief Returns the max change in velcoity
  virtual double GetMaxChangeVelocity() const = 0;
  /// \brief Sets the max change in velocity
  /// \param[in] a_maxChangeVelocity the new max change in velocity
  virtual void SetMaxChangeVelocity(const double a_maxChangeVelocity) = 0;

  /// \brief Returns the max change in direction in radians
  virtual double GetMaxChangeDirectionInRadians() const = 0;
  /// \brief Sets the max change in direction in radians
  /// \param[in] a_maxChangeVelocity the new max change in direction in radians
  virtual void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) = 0;

  /// \brief Assigns velocity vectors to each point or cell for a time step,
  ///        keeping the previous step, and dropping the one before that
  ///        for a maximum of two time steps.
  /// \param[in] a_scalars The velocity vectors
  /// \param[in] a_scalarLoc Whether the vectors are assigned to cells or points
  /// \param[in] a_activity Whether each cell or point is active
  /// \param[in] a_activityLoc Whether the activities are assigned to cells or points
  /// \param[in] a_time The time of the scalars
  virtual void AddGridScalarsAtTime(const VecPt3d& a_scalars, DataLocationEnum a_scalarLoc, xms::DynBitset& a_activity, DataLocationEnum a_activityLoc, double a_time) = 0;

  /// \brief Runs the Grid Trace for a point
  /// \param[in] a_pt The starting point of the trace
  /// \param[in] a_ptTime The starting time of the trace
  /// \param[out] a_outTrace the resultant positions at each step
  /// \param[out] a_outTimes the resultant times at each step
  virtual void TracePoint(const Pt3d& a_pt, const double& a_ptTime, VecPt3d& a_outTrace, VecDbl& a_outTimes) = 0;
  
  /// \brief returns a message describing what caused trace to exit
  virtual std::string GetExitMessage() = 0;
private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmGridTrace)

protected:
  XmGridTrace() {};
};

//----- Function prototypes ----------------------------------------------------

} // namespace xms
