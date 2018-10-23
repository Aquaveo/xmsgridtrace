//------------------------------------------------------------------------------
/// \file
/// \brief
/// \copyright (C) Copyright Aquaveo 2018. Distributed under FreeBSD License
/// (See accompanying file LICENSE or https://aqaveo.com/bsd/license.txt)
//------------------------------------------------------------------------------

//----- Included files ---------------------------------------------------------
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <boost/shared_ptr.hpp>
#include <xmscore/python/misc/PyUtils.h>
#include <xmscore/misc/DynBitset.h>
#include <xmscore/stl/vector.h>
#include <xmsgrid/ugrid/XmUGrid.h>
#include <xmsgridtrace/gridtrace/XmGridTrace.h>
#include <xmsextractor/extractor/XmUGrid2dDataExtractor.h>

//----- Namespace declaration --------------------------------------------------
namespace py = pybind11;

//----- Python Interface -------------------------------------------------------
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);

void initXmGridTrace(py::module &m) {
  const char* XmGridtrace_doc = R"pydoc(
        Class in which the flow trace of a point between 2 velocity vector time 
        steps on an XmGrid is calculated and the velocity vectors can be 
        assigned to either the points or to the cells.
    )pydoc";
    py::class_<xms::XmGridTrace, boost::shared_ptr<xms::XmGridTrace>> gridtrace(
      m, "XmGridTrace", XmGridTrace_doc);
    gridtrace.def(py::init([](boost::shared_ptr<xms::XmUGrid> ugrid) {
            return boost::shared_ptr<xms::XmGridTrace>(xms::XmGridTrace::New(
              ugrid));
        }));

  // ---------------------------------------------------------------------------
  // function: get_vector_multiplier
  // ---------------------------------------------------------------------------
  const char* get_vector_multiplier_doc = R"pydoc(
      Returns the vector multiplier.

      Returns:
          float: The vector multiplier.
  )pydoc";
  gridtrace.def("get_vector_multiplier", &xms::XmGridTrace::GetVectorMultiplier,
    get_vector_multiplier_doc);
  // ---------------------------------------------------------------------------
  // function: set_vector_multiplier
  // ---------------------------------------------------------------------------
  const char* set_vector_multiplier_doc = R"pydoc(
      Sets the vector multiplier.

      Args:
          vector_multiplier (float): The new vector multiplier.
  )pydoc";
  gridtrace.def("set_vector_multiplier", &xms::XmGridTrace::SetVectorMultiplier, 
    set_vector_multiplier_doc, py::arg("vector_multiplier"));
  // ---------------------------------------------------------------------------
  // function: get_max_tracing_time
  // ---------------------------------------------------------------------------
  const char* get_max_tracing_time_doc = R"pydoc(
      Returns the max tracing time.

      Returns:
          float: The max tracing time.
  )pydoc";
  gridtrace.def("get_max_tracing_time", &xms::XmGridTrace::GetMaxTracingTime,
    get_max_tracing_time_doc);
  // ---------------------------------------------------------------------------
  // function: set_max_tracing_time
  // ---------------------------------------------------------------------------
  const char* set_max_tracing_time_doc = R"pydoc(
      Sets the max tracing time.

      Args:
          max_tracing_time (float): The new max tracing time.
  )pydoc";
  gridtrace.def("set_max_tracing_time", &xms::XmGridTrace::SetMaxTracingTime,
    set_max_tracing_time_doc, py::arg("max_tracing_time"));
  // ---------------------------------------------------------------------------
  // function: get_max_tracing_distance
  // ---------------------------------------------------------------------------
  const char* get_max_tracing_distance_doc = R"pydoc(
      Returns the max tracing distance.

      Returns:
          float: The max tracing distance.
  )pydoc";
  gridtrace.def("get_max_tracing_distance", 
    &xms::XmGridTrace::GetMaxTracingDistance, get_max_tracing_distance_doc);
  // ---------------------------------------------------------------------------
  // function: set_max_tracing_distance
  // ---------------------------------------------------------------------------
  const char* set_max_tracing_distance_doc = R"pydoc(
      Sets the max tracing distance.

      Args:
          max_tracing_distance (float): The new max tracing distance.
  )pydoc";
  gridtrace.def("set_max_tracing_distance", 
    &xms::XmGridTrace::SetMaxTracingDistance, set_max_tracing_distance_doc,
    py::arg("max_tracing_distance"));
  // ---------------------------------------------------------------------------
  // function: get_min_delta_time
  // ---------------------------------------------------------------------------
  const char* get_min_delta_time_doc = R"pydoc(
      Returns the minimum delta time.

      Returns:
          float: The minimum time between trace steps.
  )pydoc";
  gridtrace.def("get_min_delta_time", &xms::XmGridTrace::GetMinDeltaTime, 
    get_min_delta_time_doc);
  // ---------------------------------------------------------------------------
  // function: set_min_delta_time
  // ---------------------------------------------------------------------------
  const char* set_min_delta_time_doc = R"pydoc(
      Sets the minimum delta time.

      Args:
          min_delta_time (float): The new minimum delta time.
  )pydoc";
  gridtrace.def("set_min_delta_time", &xms::XmGridTrace::SetMinDeltaTime, 
    set_min_delta_time_doc, py::arg("min_delta_time"));
  // ---------------------------------------------------------------------------
  // function: get_max_change_distance
  // ---------------------------------------------------------------------------
  const char* get_max_change_distance_doc = R"pydoc(
      Returns the maximum change in distance.

      Returns:
          float: The maximum change in distance between trace steps.
  )pydoc";
  gridtrace.def("get_max_change_distance", 
    &xms::XmGridTrace::GetMaxChangeDistance, get_max_change_distance_doc);
  // ---------------------------------------------------------------------------
  // function: set_max_change_distance
  // ---------------------------------------------------------------------------
  const char* set_max_change_distance_doc = R"pydoc(
      Sets the maximum change in distance.

      Args:
          maximum_change_distance (float): The new maximum change in distance.
  )pydoc";
  gridtrace.def("set_max_change_distance", 
    &xms::XmGridTrace::SetMaxChangeDistance, set_max_change_distance_doc,
    py::arg("maximum_change_distance"));
  // ---------------------------------------------------------------------------
  // function: get_max_change_velocity
  // ---------------------------------------------------------------------------
  const char* get_max_change_velocity_doc = R"pydoc(
      Returns the maximum change in velocity.

      Returns:
          float: The maximum change in velocity between trace steps.
  )pydoc";
  gridtrace.def("get_max_change_velocity", 
    &xms::XmGridTrace::GetMaxChangeVelocity, get_max_change_velocity_doc);
  // ---------------------------------------------------------------------------
  // function: set_max_change_velocity
  // ---------------------------------------------------------------------------
  const char* set_max_change_velocity_doc = R"pydoc(
      Sets the maximum change in velocity.

      Args:
          maximum_change_velocity (float): The new maximum change in velocity.
  )pydoc";
  gridtrace.def("set_max_change_velocity",
    &xms::XmGridTrace::SetMaxChangeVelocity, set_max_change_velocity_doc, 
    py::arg("maximum_change_velocity"));
  // ---------------------------------------------------------------------------
  // function: get_max_change_direction_in_radians
  // ---------------------------------------------------------------------------
  const char* get_max_change_direction_in_radians_doc = R"pydoc(
      Returns the maximum change in direction in radians.

      Returns:
          float: The maximum change in direction between trace steps.
  )pydoc";
  gridtrace.def("get_max_change_direction_in_radians", 
    &xms::XmGridTrace::GetMaxChangeDirectionInRadians,
    get_max_change_direction_in_radians_doc);
  // ---------------------------------------------------------------------------
  // function: set_max_change_direction_in_radians
  // ---------------------------------------------------------------------------
  const char* set_max_change_direction_in_radians_doc = R"pydoc(
      Sets the maximum change in direction in radians.

      Args:
          maximum_change_direction (float): The new maximum change in direction
            in radians.
  )pydoc";
  gridtrace.def("set_max_change_direction_in_radians", 
    &xms::XmGridTrace::SetMaxChangeDirectionInRadians, 
    set_max_change_direction_in_radians_doc, 
    py::arg("maximum_change_direction"));
  // ---------------------------------------------------------------------------
  // function: add_grid_scalars_at_time
  // ---------------------------------------------------------------------------
  const char* add_grid_scalars_at_time_doc = R"pydoc(
      Assigns velocity vectors to each point or cell for a time step,
      keeping the previous step, and dropping the one before that
      for a maximum of two time steps.

      Args:
          scalars (iterable): The velocity vectors.
          scalar_loc (data_location_enum): Whether the vectors are assigned to 
            cells or points.
          activity (iterable): Whether each cell or point is active.
          activity_loc (data_location_enum): Whether the activities are assigned 
            to cells or points.
          time (float): The time of the scalars.
  )pydoc";
  gridtrace.def("add_grid_scalars_at_time", [](xms::XmGridTrace &self, 
          py::iterable vel_scalars,
          xms::DataLocationEnum scalar_loc,
          py::iterable activity,
          xms::DataLocationEnum activity_loc,
          double time) {
            boost::shared_ptr<xms::VecPt3d> scalars = 
              xms::VecPt3dFromPyIter(vel_scalars);
            xms::DynBitset activity = xms::DynamicBitsetFromPyIter(activity);
            self.AddGridScalarsAtTime(*scalars, scalar_loc, activity, 
              activity_loc, time);
            return;
          }, add_grid_scalars_at_time_doc, py::arg("scalars"), 
          py::arg("scalar_loc"), py::arg("activity"), py::arg("activity_loc"),
           py::arg("time"));
  // ---------------------------------------------------------------------------
  // function: trace_point
  // ---------------------------------------------------------------------------
  const char* trace_point_doc = R"pydoc(
      Runs the Grid Trace for a point.

      Args:
          pt (iterable): The starting point of the trace.
          pt_time (float): The starting time of the trace.

      Returns:
          iterable: Contains the resultant positions at each step and the 
            resultant times at each step.
  )pydoc";
  gridtrace.def("trace_point", [](xms::XmGridTrace &self, py::iterable pt, 
    double pt_time) -> py::iterable {
          xms::Pt3d point = xms::Pt3dFromPyIter(pt);
          xms::VecPt3d outTrace;
          xms::VecDbl outTimes;
          self.TracePoint(point, pt_time, outTrace, outTimes);
          py::iterable resultTrace = xms::PyIterFromVecPt3d(outTrace);
          py::iterable resultTimes = xms::PyIterFromVecDbl(outTimes);
          return py::make_tuple(resultTrace, resultTimes);
        }, trace_point_doc, py::arg("pt"), py::arg("pt_time"));
  // ---------------------------------------------------------------------------
  // function: get_exit_message
  // ---------------------------------------------------------------------------
  const char* get_exit_message_doc = R"pydoc(
      Returns a message describing what caused trace to exit.

      Returns:
          str: The exit message of the last TracePoint operation.
  )pydoc";
  gridtrace.def("get_exit_message", &xms::XmGridTrace::GetExitMessage,
    get_exit_message_doc);

    // DataLocationEnum
    py::enum_<xms::DataLocationEnum>(m, "data_location_enum",
                    "data_location_enum location mapping for dataset values")
        .value("LOC_POINTS", xms::LOC_POINTS)
        .value("LOC_CELLS", xms::LOC_CELLS)
        .value("LOC_UNKNOWN", xms::LOC_UNKNOWN);
}
