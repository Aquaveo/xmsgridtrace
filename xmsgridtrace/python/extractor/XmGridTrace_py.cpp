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

void initXmGridTrace(py::module &m) {
    py::class_<xms::XmGridTrace, boost::shared_ptr<xms::XmGridTrace>> gridtrace(m, "XmGridTrace");
    gridtrace
        .def(py::init([](boost::shared_ptr<xms::XmUGrid> a_ugrid) {
            return boost::shared_ptr<xms::XmGridTrace>(xms::XmGridTrace::New(a_ugrid));
        }))
        .def(py::init([](boost::shared_ptr<xms::XmGridTrace> a_gridTrace) {
            return boost::shared_ptr<xms::XmGridTrace>(xms::XmGridTrace::New(a_gridTrace));
        }))
          //  virtual double GetVectorMultiplier() const = 0;
          .def("get_vector_multiplier", &xms::XmGridTrace::GetVectorMultiplier, "Gets the vector multiplier")
          // virtual void SetVectorMultiplier(const double a_vectorMultiplier) = 0;
          .def("set_vector_multiplier", &xms::XmGridTrace::SetVectorMultiplier, "Sets the vector multiplier")
          // virtual double GetMaxTracingTimeSeconds() const = 0;
          .def("get_max_tracing_time_seconds", &xms::XmGridTrace::GetMaxTracingTimeSeconds, "Gets the max tracing time in seconds")
          // virtual void SetMaxTracingTimeSeconds(const double a_maxTracingTime) = 0;
          .def("set_max_tracing_time_seconds", &xms::XmGridTrace::SetMaxTracingTimeSeconds, "Sets the max tracing time in seconds")
          // virtual double GetMaxTracingDistanceMeters() const = 0;
          .def("get_max_tracing_distance_meters", &xms::XmGridTrace::GetMaxTracingDistanceMeters, "Gets the max tracing distance in meters")
          // virtual void SetMaxTracingDistanceMeters(const double a_maxTracingDistance) = 0;
          .def("set_max_tracing_distance_meters", &xms::XmGridTrace::SetMaxTracingDistanceMeters, "Sets the max tracing distance in meters")
          // virtual double GetMinDeltaTimeSeconds() const = 0;
          .def("get_min_delta_time_seconds", &xms::XmGridTrace::GetMinDeltaTimeSeconds, "Gets the minimum delta time")
          // virtual void SetMinDeltaTimeSeconds(const double a_minDeltaTime) = 0;
          .def("set_min_delta_time_seconds", &xms::XmGridTrace::SetMinDeltaTimeSeconds, "Sets the minimum delta time")
          // virtual double GetMaxChangeDistanceMeters() const = 0;
          .def("get_max_change_distance_meters", &xms::XmGridTrace::GetMaxChangeDistanceMeters, "Gets the maximum change in distance in meters")
          // virtual void SetMaxChangeDistanceMeters(const double a_maxChangeDistance) = 0;
          .def("set_max_change_distance_meters", &xms::XmGridTrace::SetMaxChangeDistanceMeters, "Sets the maximum change in distance in meters")
          // virtual double GetMaxChangeVelocityMetersPerSecond() const = 0;
          .def("get_max_change_velocity_meters_per_second", &xms::XmGridTrace::GetMaxChangeVelocityMetersPerSecond, "Gets the maximum changee in velocity in meters per second")
          // virtual void SetMaxChangeVelocityMetersPerSecond(const double a_maxChangeVelocity) = 0;
          .def("set_max_change_velocity_meters_per_second", &xms::XmGridTrace::SetMaxChangeVelocityMetersPerSecond, "Sets the maximum changee in velocity in meters per second")
          // virtual double GetMaxChangeDirectionInRadians() const = 0;
          .def("get_max_change_direction_in_radians", &xms::XmGridTrace::GetMaxChangeDirectionInRadians, "Gets the max change in direction in radians")
          // virtual void SetMaxChangeDirectionInRadians(const double a_maxChangeDirection) = 0;
          .def("set_max_change_direction_in_radians", &xms::XmGridTrace::SetMaxChangeDirectionInRadians, "Sets the max change in direction in radians")
          
#if 0
          // virtual void AddGridScalarsAtTime(const VecPt3d& a_scalars, DataLocationEnum a_scalarLoc, xms::DynBitset& a_activity, DataLocationEnum a_activityLoc, double a_time) = 0;
          .def("add_grid_scalars_at_time", [](xms::XmGridTrace &self, py::iterable a_scalars,
            xms::DataLocationEnum a_scalarLoc,
            py::iterable a_activity
            xms::DataLocationEnum a_activityLoc,
            double a_time) {
              self.AddGridScalarsAtTime();
              return;
          }, "adds a grid scalar with a time to the tracer")
          // virtual void TracePoint(const Pt3d& a_pt, const double& a_ptTime, VecPt3d& a_outTrace, VecDbl& a_outTimes) = 0;
          .def("get_vector_multiplier", &xms::XmGridTrace::GetVectorMultiplier, "Gets the vector multiplier")
#endif
            ;

    // DataLocationEnum
    py::enum_<xms::DataLocationEnum>(m, "data_location_enum",
                                    "data_location_enum location mapping for dataset values")
        .value("LOC_POINTS", xms::LOC_POINTS)
        .value("LOC_CELLS", xms::LOC_CELLS)
        .value("LOC_UNKNOWN", xms::LOC_UNKNOWN);
}
