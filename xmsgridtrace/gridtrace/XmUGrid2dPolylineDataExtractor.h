#pragma once
//------------------------------------------------------------------------------
/// \file
/// \brief Contains the XmUGrid2dPolylineDataExtractor Class and supporting data
///         types.
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
#include <xmsgridtrace/gridtrace/XmUGrid2dDataExtractor.h>

//----- Forward declarations ---------------------------------------------------

//----- Namespace declaration --------------------------------------------------

/// XMS Namespace
namespace xms
{
//----- Forward declarations ---------------------------------------------------
class XmUGrid;

//----- Constants / Enumerations -----------------------------------------------

//----- Structs / Classes ------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
class XmUGrid2dPolylineDataExtractor
{
public:
  static BSHP<XmUGrid2dPolylineDataExtractor> New(BSHP<XmUGrid> a_ugrid,
                                                  DataLocationEnum a_scalarLocation);
  virtual ~XmUGrid2dPolylineDataExtractor();

  /// \brief Setup point scalars to be used to extract interpolated data.
  /// \param[in] a_scalars The cell or point scalars.
  /// \param[in] a_activity The activity of the cells.
  /// \param[in] a_activityLocation The location at which the data is currently stored.
  virtual void SetGridScalars(const VecFlt& a_scalars,
                              const DynBitset& a_activity,
                              DataLocationEnum a_activityLocation) = 0;

  /// \brief Set the polyline along which to extract the scalar data. Locations
  ///        crossing cell boundaries are computed along the polyline.
  /// \param[in] a_polyline The polyline.
  virtual void SetPolyline(const VecPt3d& a_polyline) = 0;
  /// \brief Gets computed locations along polyline to extract interpolated scalar data from.
  /// \return The locations.
  virtual const VecPt3d& GetExtractLocations() const = 0;
  /// \brief Extract data at previously computed locations returned by GetExtractLocations.
  /// \param[out] a_extractedData The interpolated scalar data at the previously computed locations.
  virtual void ExtractData(VecFlt& a_extractedData) = 0;

  /// \brief Extract data for given polyline.
  /// \param[in] a_polyline The polyline.
  /// \param[out] a_extractedData The interpolated scalar data at the extracted locations.
  /// \param[out] a_extractedLocations The locations.
  virtual void ComputeLocationsAndExtractData(const VecPt3d& a_polyline,
                                              VecFlt& a_extractedData,
                                              VecPt3d& a_extractedLocations) = 0;

  /// \brief Set to use IDW to calculate point scalar values from cell scalars.
  /// \param a_useIdw Whether to turn IDW on or off.
  virtual void SetUseIdwForPointData(bool a_useIdw) = 0;
  /// \brief Set value to use when extracted value is in inactive cell or doesn't
  ///        intersect with the grid.
  /// \param[in] a_noDataValue The no data value
  virtual void SetNoDataValue(float a_noDataValue) = 0;

private:
  XM_DISALLOW_COPY_AND_ASSIGN(XmUGrid2dPolylineDataExtractor)

protected:
  XmUGrid2dPolylineDataExtractor();
};

//----- Function prototypes ----------------------------------------------------

} // namespace xms
