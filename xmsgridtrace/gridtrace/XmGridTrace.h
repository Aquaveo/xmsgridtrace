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
namespace xms {
  enum DataLocationEnum;}
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
  virtual ~XmGridTrace() {};

  virtual double GetVectorMultiplier() const = 0;
  virtual void SetVectorMultiplier(const double a_vectorMultiplier) = 0;

  virtual double GetMaxTracingTime() const = 0;
  virtual void SetMaxTracingTime(const double a_maxTracingTime) = 0;

  virtual double GetMaxTracingDistance() const = 0;
  virtual void SetMaxTracingDistance(const double a_maxTracingDistance) = 0;

  virtual double GetMinDeltaTime() const = 0;
  virtual void SetMinDeltaTime(const double a_minDeltaTime) = 0;

  virtual double GetMaxChangeDistance() const = 0;
  virtual void SetMaxChangeDistance(const double a_maxChangeDistance) = 0;

  virtual double GetMaxChangeVelocity() const = 0;
  virtual void SetMaxChangeVelocity(const double a_maxChangeVelocity) = 0;

  virtual double GetMaxChangeDirectionInRadians() const = 0;
  virtual void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) = 0;

  virtual void AddGridScalarsAtTime(const VecPt3d& a_scalars, DataLocationEnum a_scalarLoc, xms::DynBitset& a_activity, DataLocationEnum a_activityLoc, double a_time) = 0;

  virtual void TracePoint(const Pt3d& a_pt, const double& a_ptTime, VecPt3d& a_outTrace, VecDbl& a_outTimes) = 0;
  
  virtual std::string GetExitMessage() = 0;
private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmGridTrace)

protected:
  XmGridTrace() {};
};

//----- Function prototypes ----------------------------------------------------

} // namespace xms
