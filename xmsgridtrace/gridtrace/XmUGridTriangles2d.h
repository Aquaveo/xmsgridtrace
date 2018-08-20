#pragma once
//------------------------------------------------------------------------------
/// \file
/// \brief Contains the XmUGrid Class and supporting data types.
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
class GmTriSearch;
class XmUGrid;

//----- Constants / Enumerations -----------------------------------------------

//----- Structs / Classes ------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
class XmUGridTriangles2d
{
public:
  static BSHP<XmUGridTriangles2d> New();
  virtual ~XmUGridTriangles2d();

  /// \brief Generate triangles for the UGrid.
  /// \param[in] a_ugrid The UGrid for which triangles are generated.
  /// \param[in] a_addTriangleCenters Whether or not triangle cells get a centroid added.
  virtual void BuildTriangles(const XmUGrid& a_ugrid, bool a_addTriangleCenters) = 0;
  /// \brief Generate triangles for the UGrid using earcut algorithm.
  /// \param[in] a_ugrid The UGrid for which triangles are generated.
  virtual void BuildEarcutTriangles(const XmUGrid& a_ugrid) = 0;
  /// \brief Set triangle activity based on each triangles cell.
  /// \param[in] a_cellActivity The cell activity to set on the triangles.
  virtual void SetCellActivity(const DynBitset& a_cellActivity) = 0;

  /// \brief Get the generated triangle points.
  /// \return The triangle points
  virtual const VecPt3d& GetPoints() const = 0;
  /// \brief Get the generated triangles.
  /// \return a vector of indices for the triangles.
  virtual const VecInt& GetTriangles() const = 0;
  /// \brief Get the generated triangle points as a shared pointer.
  /// \return The triangle points
  virtual BSHP<VecPt3d> GetPointsPtr() = 0;
  /// \brief Get the generated triangles as a shared pointer.
  /// \return a vector of indices for the triangles.
  virtual BSHP<VecInt> GetTrianglesPtr() = 0;

  /// \brief Get the centroid of a cell.
  /// \param[in] a_cellIdx The cell index.
  /// \return The index of the cell point.
  virtual int GetCellCentroid(int a_cellIdx) const = 0;

  /// \brief Get the cell index and interpolation values intersected by a point.
  /// \param[in] a_point The point to intersect with the UGrid.
  /// \param[out] a_idxs The interpolation points.
  /// \param[out] a_weights The interpolation weights.
  /// \return The cell intersected by the point or -1 if outside of the UGrid.
  virtual int GetIntersectedCell(const Pt3d& a_point, VecInt& a_idxs, VecDbl& a_weights) = 0;

protected:
  XmUGridTriangles2d();

private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmUGridTriangles2d)
};

//----- Function prototypes ----------------------------------------------------

} // namespace xms
