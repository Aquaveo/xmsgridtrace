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
#include <xmsgrid/ugrid/XmUGrid.h>

//----- Namespace declaration --------------------------------------------------
namespace py = pybind11;

//----- Python Interface -------------------------------------------------------
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);

void initXmUGrid2dDataExtractor(py::module &m) {
    py::class_<xms::XmUGrid2dDataExtractor, boost::shared_ptr<xms::XmUGrid2dDataExtractor>> extractor(m, "XmUGrid2dDataExtractor");
    extractor
        .def(py::init([](boost::shared_ptr<xms::XmUGrid> a_ugrid) {
            return boost::shared_ptr<xms::XmUGrid2dDataExtractor>(xms::XmUGrid2dDataExtractor::New(a_ugrid));
        }))
        .def(py::init([](boost::shared_ptr<xms::XmUGrid2dDataExtractor> a_extractor) {
            return boost::shared_ptr<xms::XmUGrid2dDataExtractor>(xms::XmUGrid2dDataExtractor::New(a_extractor));
        }))
        //virtual void SetGridPointScalars(const VecFlt& a_pointScalars,
        //                                 const DynBitset& a_activity,
        //                                 DataLocationEnum a_activityType) = 0;
        .def("set_grid_point_scalars", [](xms::XmUGrid2dDataExtractor &self, py::iterable a_pointScalars,
                                          py::iterable a_activity,
                                          xms::DataLocationEnum a_activityType) {
            boost::shared_ptr<xms::VecFlt> pointScalars = xms::VecFltFromPyIter(a_pointScalars);
            xms::DynBitset activity = xms::DynamicBitsetFromPyIter(a_activity);
            self.SetGridPointScalars(*pointScalars, activity, a_activityType);
        },"Set the point scalar values",py::arg("point_scalars"),py::arg("activity"),py::arg("activity_type"))
        //virtual void SetGridCellScalars(const VecFlt& a_cellScalars,
        //                                const DynBitset& a_activity,
        //                                DataLocationEnum a_activityType) = 0;
        .def("set_grid_cell_scalars", [](xms::XmUGrid2dDataExtractor &self, py::iterable a_cellScalars,
                                          py::iterable a_activity,
                                          xms::DataLocationEnum a_activityType) {
            boost::shared_ptr<xms::VecFlt> cellScalars = xms::VecFltFromPyIter(a_cellScalars);
            xms::DynBitset activity = xms::DynamicBitsetFromPyIter(a_activity);
            self.SetGridCellScalars(*cellScalars, activity, a_activityType);
        },"Set the point scalar values",py::arg("point_scalars"),py::arg("activity"),py::arg("activity_type"))
        //virtual void SetExtractLocations(const VecPt3d& a_locations) = 0;
        .def("set_extract_locations", [](xms::XmUGrid2dDataExtractor &self, py::iterable a_locations) {
            boost::shared_ptr<xms::VecPt3d> locations = xms::VecPt3dFromPyIter(a_locations);
            self.SetExtractLocations(*locations);
        })
        //virtual const VecPt3d& GetExtractLocations() const = 0;
        .def("get_extract_locations", [](xms::XmUGrid2dDataExtractor &self) -> py::iterable {
            xms::VecPt3d locations = self.GetExtractLocations();
            return xms::PyIterFromVecPt3d(locations);
        })
        //virtual void ExtractData(VecFlt& a_outData) = 0;
        .def("extract_data", [](xms::XmUGrid2dDataExtractor &self) -> py::iterable {
            xms::VecFlt outData;
            self.ExtractData(outData);
            return xms::PyIterFromVecFlt(outData);
        })
        //virtual float ExtractAtLocation(const Pt3d& a_location) = 0;
        .def("extract_at_location", [](xms::XmUGrid2dDataExtractor &self, py::iterable a_location) -> float {
            xms::Pt3d location = xms::Pt3dFromPyIter(a_location);
            float value = self.ExtractAtLocation(location);
            return value;
        })
        //virtual void SetUseIdwForPointData(bool a_useIdw) = 0;
        .def("set_use_idw_for_point_data", &xms::XmUGrid2dDataExtractor::SetUseIdwForPointData)
        //virtual void SetNoDataValue(float a_noDataValue) = 0;
        .def("set_no_data_value", &xms::XmUGrid2dDataExtractor::SetNoDataValue,"Set the no data value",py::arg("no_data_value"))
    ;

    // DataLocationEnum
    py::enum_<xms::DataLocationEnum>(m, "data_location_enum",
                                    "data_location_enum location mapping for dataset values")
        .value("LOC_POINTS", xms::LOC_POINTS)
        .value("LOC_CELLS", xms::LOC_CELLS)
        .value("LOC_UNKNOWN", xms::LOC_UNKNOWN);
}
