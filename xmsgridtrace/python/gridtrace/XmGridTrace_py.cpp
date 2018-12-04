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
  const char* XmGridTrace_doc = R"pydoc(
        Class in which the flow trace of a point between 2 velocity vector time 
        steps on an XmGrid is calculated and the velocity vectors can be 
        assigned to either the points or to the cells.
    )pydoc";
    py::class_<xms::XmGridTrace, boost::shared_ptr<xms::XmGridTrace>> gridtrace(
      m, "GridTrace", XmGridTrace_doc);

  // ---------------------------------------------------------------------------
  // function: __init__
  // ---------------------------------------------------------------------------
  gridtrace.def(py::init([](boost::shared_ptr<xms::XmUGrid> ugrid,
                py::object vector_multiplier, py::object max_tracing_time,
                py::object max_tracing_distance, py::object min_delta_time,
                py::object max_change_distance, py::object max_change_velocity,
                py::object max_change_direction_in_radians)
      {
        boost::shared_ptr<xms::XmGridTrace> rval(xms::XmGridTrace::New(ugrid));
        if (!vector_multiplier.is_none())
        {
          rval->SetVectorMultiplier(vector_multiplier.cast<double>());
        }
        if (!max_tracing_time.is_none())
        {
          rval->SetMaxTracingTime(max_tracing_time.cast<double>());
        }
        if (!max_tracing_distance.is_none())
        {
          rval->SetMaxTracingDistance(max_tracing_distance.cast<double>());
        }
        if (!min_delta_time.is_none())
        {
          rval->SetMinDeltaTime(min_delta_time.cast<double>());
        }
        if (!max_change_distance.is_none())
        {
          rval->SetMaxChangeDistance(max_change_distance.cast<double>());
        }
        if (!max_change_velocity.is_none())
        {
          rval->SetMaxChangeVelocity(max_change_velocity.cast<double>());
        }
        if (!max_change_direction_in_radians.is_none())
        {
          rval->SetMaxChangeDirectionInRadians(max_change_direction_in_radians.cast<double>());
        }
        return rval;
      }), py::arg("ugrid"), py::arg("vector_multiplier") = py::none(), py::arg("max_tracing_time") = py::none(),
          py::arg("max_tracing_distance") = py::none(), py::arg("min_delta_time") = py::none(),
          py::arg("max_change_distance") = py::none(), py::arg("max_change_velocity") = py::none(),
          py::arg("max_change_direction_in_radians") = py::none());

  // -------------------------------------------------------------------------
  // function: __repr__
  // -------------------------------------------------------------------------
  gridtrace.def("__repr__", [](xms::XmGridTrace &self) {
    std::stringstream ss;
    ss << "vector_multiplier: " << self.GetVectorMultiplier() << "\n";
    ss << "max_tracing_time: " << self.GetMaxTracingTime() << "\n";
    ss << "max_tracing_distance: " << self.GetMaxTracingDistance() << "\n";
    ss << "min_delta_time: " << self.GetMinDeltaTime() << "\n";
    ss << "max_change_distance: " << self.GetMaxChangeDistance() << "\n";
    ss << "max_change_velocity: " << self.GetMaxChangeVelocity() << "\n";
    ss << "max_change_direction_in_radians: " << self.GetMaxChangeDirectionInRadians();
    return ss.str();
  });


  // ---------------------------------------------------------------------------
  // property: get_vector_multiplier
  // ---------------------------------------------------------------------------
  const char* vector_multiplier_doc = R"pydoc(
      The vector multiplier.
  )pydoc";
  gridtrace.def_property("vector_multiplier",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetVectorMultiplier();
      },
      [](xms::XmGridTrace &self, double vector_multiplier)
      {
        self.SetVectorMultiplier(vector_multiplier);
      },
      vector_multiplier_doc);

  // ---------------------------------------------------------------------------
  // property: max_tracing_time
  // ---------------------------------------------------------------------------
  const char* max_tracing_time_doc = R"pydoc(
      The max tracing time.
  )pydoc";
  gridtrace.def_property("max_tracing_time",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetMaxTracingTime();
      },
      [](xms::XmGridTrace &self, double max_tracing_time)
      {
        self.SetMaxTracingTime(max_tracing_time);
      },
      max_tracing_time_doc);

  // ---------------------------------------------------------------------------
  // property: max_tracing_distance
  // ---------------------------------------------------------------------------
  const char* max_tracing_distance_doc = R"pydoc(
      The max tracing distance.
  )pydoc";
  gridtrace.def_property("max_tracing_distance",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetMaxTracingDistance();
      },
      [](xms::XmGridTrace &self, double max_tracing_distance)
      {
        self.SetMaxTracingDistance(max_tracing_distance);
      },
      max_tracing_distance_doc);

  // ---------------------------------------------------------------------------
  // property: min_delta_time
  // ---------------------------------------------------------------------------
  const char* min_delta_time_doc = R"pydoc(
      The min delta time.
  )pydoc";
  gridtrace.def_property("min_delta_time",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetMinDeltaTime();
      },
      [](xms::XmGridTrace &self, double min_delta_time)
      {
        self.SetMinDeltaTime(min_delta_time);
      },
      min_delta_time_doc);

  // ---------------------------------------------------------------------------
  // property: max_change_distance
  // ---------------------------------------------------------------------------
  const char* max_change_distance_doc = R"pydoc(
      The max change distance.
  )pydoc";
  gridtrace.def_property("max_change_distance",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetMaxChangeDistance();
      },
      [](xms::XmGridTrace &self, double max_change_distance)
      {
        self.SetMaxChangeDistance(max_change_distance);
      },
      max_change_distance_doc);

  // ---------------------------------------------------------------------------
  // property: max_change_velocity
  // ---------------------------------------------------------------------------
  const char* max_change_velocity_doc = R"pydoc(
      The max change velocity.
  )pydoc";
  gridtrace.def_property("max_change_velocity",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetMaxChangeVelocity();
      },
      [](xms::XmGridTrace &self, double max_change_velocity)
      {
        self.SetMaxChangeVelocity(max_change_velocity);
      },
      max_change_velocity_doc);

  // ---------------------------------------------------------------------------
  // property: max_change_direction_in_radians
  // ---------------------------------------------------------------------------
  const char* max_change_direction_in_radians_doc = R"pydoc(
      The max change direction in radians.
  )pydoc";
  gridtrace.def_property("max_change_direction_in_radians",
      [](xms::XmGridTrace &self) -> double
      {
        return self.GetMaxChangeDirectionInRadians();
      },
      [](xms::XmGridTrace &self, double max_change_direction_in_radians)
      {
        self.SetMaxChangeDirectionInRadians(max_change_direction_in_radians);
      },
      max_change_direction_in_radians_doc);







  // ---------------------------------------------------------------------------
  // function: add_grid_scalars_at_time
  // ---------------------------------------------------------------------------
  const char* add_grid_scalars_at_time_doc = R"pydoc(
      Assigns velocity vectors to each point or cell for a time step,
      keeping the previous step, and dropping the one before that
      for a maximum of two time steps.

      Args:
          scalars (iterable): The velocity vectors.

          scalar_loc (string): Whether the vectors are assigned to cells or points.

          cell_activity (iterable): Whether each cell or point is active.

          activity_loc (string): Whether the activities are assigned to cells or points.

          time (float): The time of the scalars.
  )pydoc";
  gridtrace.def("add_grid_scalars_at_time", [](xms::XmGridTrace &self, 
          py::iterable vel_scalars,
          std::string scalar_loc,
          py::iterable cell_activity,
          std::string activity_loc,
          double time) {

            xms::DataLocationEnum scalar_loc_e;
            if (scalar_loc == "points")
              scalar_loc_e = xms::DataLocationEnum::LOC_POINTS;
            else if (scalar_loc == "cells")
              scalar_loc_e = xms::DataLocationEnum::LOC_CELLS;
            else if (scalar_loc == "unknown")
              scalar_loc_e = xms::DataLocationEnum::LOC_UNKNOWN;
            else
            {
              std::string msg = "nodal_func_type string must be one of 'points', 'cells', "
                                "'unknown' not " + scalar_loc;
              throw py::value_error(msg);
            }

            xms::DataLocationEnum activity_loc_e;
            if (activity_loc == "points")
              activity_loc_e = xms::DataLocationEnum::LOC_POINTS;
            else if (activity_loc == "cells")
              activity_loc_e = xms::DataLocationEnum::LOC_CELLS;
            else if (activity_loc == "unknown")
              activity_loc_e = xms::DataLocationEnum::LOC_UNKNOWN;
            else
            {
              std::string msg = "nodal_func_type string must be one of 'points', 'cells', "
                                "'unknown' not " + activity_loc;
              throw py::value_error(msg);
            }

            boost::shared_ptr<xms::VecPt3d> scalars = 
              xms::VecPt3dFromPyIter(vel_scalars);
            xms::DynBitset activity = xms::DynamicBitsetFromPyIter(cell_activity);
            self.AddGridScalarsAtTime(*scalars, scalar_loc_e, activity,
              activity_loc_e, time);
            return;
          }, add_grid_scalars_at_time_doc, py::arg("scalars"), 
          py::arg("scalar_loc"), py::arg("cell_activity"), py::arg("activity_loc"),
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
          iterable: Contains the resultant positions at each step and the resultant times at each step.
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
