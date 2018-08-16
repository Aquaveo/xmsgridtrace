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

//----- Forward declarations ---------------------------------------------------

//----- Namespace declaration --------------------------------------------------

/// XMS Namespace
namespace xms
{
//----- Forward declarations ---------------------------------------------------
class XmUGrid;
class dyn_bitset;

//----- Constants / Enumerations -----------------------------------------------

/// The location at which the data will be stored.
enum DataLocationEnum { LOC_POINTS, LOC_CELLS, LOC_UNKNOWN };

//----- Structs / Classes ------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
class XmGridTrace
{
public:

  /// \brief Construct XmGridTrace for a UGrid.
  /// \param[in] a_ugrid a ugrid
  static BSHP<XmGridTrace> New(BSHP<XmUGrid> a_ugrid);
  static BSHP<XmGridTrace> New(BSHP<XmGridTrace> a_gridTrace);
  virtual ~XmGridTrace();

  virtual double GetVectorMultiplier() const = 0;
  virtual void SetVectorMultiplier(const double a_vectorMultiplier) = 0;

  virtual double GetMaxTracingTimeSeconds() const = 0;
  virtual void SetMaxTracingTimeSeconds(const double a_maxTracingTime) = 0;

  virtual double GetMaxTracingDistanceMeters() const = 0;
  virtual void SetMaxTracingDistanceMeters(const double a_maxTracingDistance) = 0;

  virtual double GetMinDeltaTimeSeconds() const = 0;
  virtual void SetMinDeltaTimeSeconds(const double a_minDeltaTime) = 0;

  virtual double GetMaxChangeDistanceMeters() const = 0;
  virtual void SetMaxChangeDistanceMeters(const double a_maxChangeDistance) = 0;

  virtual double GetMaxChangeVelocityMetersPerSecond() const = 0;
  virtual void SetMaxChangeVelocityMetersPerSecond(const double a_maxChangeVelocity) = 0;

  virtual double GetMaxChangeDirectionInRadians() const = 0;
  virtual void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) = 0;

  virtual void AddGridPointScalarsAtTime(const VecPt3d& a_scalars, xms::DynBitset& a_activity, double a_time) = 0;
  virtual void AddGridCellScalarsAtTime(const VecPt3d& a_scalars, xms::DynBitset& a_activity, double a_time) = 0;
  virtual void AddGridScalarsAtTime(const VecPt3d& a_scalars, DataLocationEnum a_scalarLoc, xms::DynBitset& a_activity, DataLocationEnum a_activityLoc, double a_time) = 0;

  virtual void TracePoint(const Pt3d& a_pt, const double& a_ptTime, VecPt3d& a_outTrace, VecDbl& a_outTimes) = 0;
  
  virtual double GetDistanceTraveled() const = 0;

private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmGridTrace)

protected:
  XmGridTrace();
};

//----- Function prototypes ----------------------------------------------------

} // namespace xms
