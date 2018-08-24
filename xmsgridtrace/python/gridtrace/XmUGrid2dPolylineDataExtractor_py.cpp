//------------------------------------------------------------------------------
/// \file
/// \brief
/// \copyright (C) Copyright Aquaveo 2018. Distributed under the xmsng
///  Software License, Version 1.0. (See accompanying file
///  LICENSE_1_0.txt or copy at http://www.aquaveo.com/xmsng/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <boost/shared_ptr.hpp>
#include <xmscore/python/misc/PyUtils.h>
#include <xmscore/misc/DynBitset.h>
#include <xmscore/stl/vector.h>
#include <xmsextractor/extractor/XmUGrid2dDataExtractor.h>
#include <xmsextractor/extractor/XmUGrid2dPolylineDataExtractor.h>
#include <xmsgrid/ugrid/XmUGrid.h>

//----- Namespace declaration --------------------------------------------------
namespace py = pybind11;

//----- Python Interface -------------------------------------------------------
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);

void initXmUGrid2dPolylineDataExtractor(py::module &m) {
    py::class_<xms::XmUGrid2dPolylineDataExtractor, boost::shared_ptr<xms::XmUGrid2dPolylineDataExtractor>> extractor(m, "XmUGrid2dPolylineDataExtractor");
    extractor
        //static BSHP<XmUGrid2dPolylineDataExtractor> New(BSHP<XmUGrid> a_ugrid,
        //                                                DataLocationEnum a_scalarLocation);
        .def(py::init([](boost::shared_ptr<xms::XmUGrid> a_ugrid, xms::DataLocationEnum a_scalarLocation) {
            return boost::shared_ptr<xms::XmUGrid2dPolylineDataExtractor>(xms::XmUGrid2dPolylineDataExtractor::New(
                a_ugrid, a_scalarLocation));
        }),"Create a new XmUGrid2dPolylineDataExtractor",py::arg("a_ugrid"),py::arg("a_scalarLocation"))
        // virtual void SetGridScalars(const VecFlt& a_scalars,
        //                         const DynBitset& a_activity,
        //                         DataLocationEnum a_activityLocation) = 0;
        .def("set_grid_scalars", [](xms::XmUGrid2dPolylineDataExtractor &self, py::iterable a_scalars,
                                    py::iterable a_activity, xms::DataLocationEnum a_activityType) {
            boost::shared_ptr<xms::VecFlt> scalars = xms::VecFltFromPyIter(a_scalars);
            xms::DynBitset activity = xms::DynamicBitsetFromPyIter(a_activity);
            self.SetGridScalars(*scalars, activity, a_activityType);
        },"Set the grid scalars", py::arg("a_scalars"),py::arg("a_activity"),py::arg("a_activityType"))
        // virtual void SetPolyline(const VecPt3d& a_polyline) = 0;
        .def("set_polyline", [](xms::XmUGrid2dPolylineDataExtractor &self, py::iterable a_polyline) {
            boost::shared_ptr<xms::VecPt3d> polyline = xms::VecPt3dFromPyIter(a_polyline);
            self.SetPolyline(*polyline);
        },"Set a polyline",py::arg("a_polyline"))
        // virtual const VecPt3d& GetExtractLocations() const = 0;
        .def("get_extract_locations", [](xms::XmUGrid2dPolylineDataExtractor &self) -> py::iterable {
            xms::VecPt3d locations = self.GetExtractLocations();
            return xms::PyIterFromVecPt3d(locations);
        },"Get the extract locations.")
        // virtual void ExtractData(VecFlt& a_extractedData) = 0;
        .def("extract_data", [](xms::XmUGrid2dPolylineDataExtractor &self) -> py::iterable {
            xms::VecFlt outData;
            self.ExtractData(outData);
            return xms::PyIterFromVecFlt(outData);
        },"Extracts the data from the Grid")
        // virtual void SetUseIdwForPointData(bool a_useIdw) = 0;
        .def("set_use_idw_for_point_data", &xms::XmUGrid2dPolylineDataExtractor::SetUseIdwForPointData,
          "Set to use IDW to calculate point scalar values from cell scalars.", py::arg("a_useIdw"))
        // virtual void SetNoDataValue(float a_noDataValue) = 0;
        .def("set_no_data_value", &xms::XmUGrid2dPolylineDataExtractor::SetNoDataValue,"Set the no data value",py::arg("no_data_value"))
    ;
}
