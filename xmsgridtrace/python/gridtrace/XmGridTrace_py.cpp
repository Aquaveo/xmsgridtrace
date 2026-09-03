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
#include <limits>
#include <xmscore/math/math.h>      // EQ_TOL
#include <xmscore/misc/xmstype.h>   // XM_NODATA
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
  gridtrace.def(py::init([](std::shared_ptr<xms::XmUGrid> ugrid,
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
  // ---------------------------------------------------------------------------
  // function: get_exit_reason
  // ---------------------------------------------------------------------------
  const char* get_exit_reason_doc = R"pydoc(
      Returns why the last trace operation ended.

      Prefer this over get_exit_message when deciding what to do with a trace; the message
      is for display. WAITING_FOR_TIME_STEP means the path stops early because the field is
      not known past the second loaded time step, not that the particle came to rest.

      Returns:
          exit_reason_enum: The exit reason of the last trace operation.
  )pydoc";
  gridtrace.def("get_exit_reason", &xms::XmGridTrace::GetExitReason,
    get_exit_reason_doc);
  // ---------------------------------------------------------------------------
  // function: start_traces
  // ---------------------------------------------------------------------------
  const char* start_traces_doc = R"pydoc(
      Begins tracing a batch of seeds against the currently loaded time steps.

      A trace runs only as far as the second loaded time step, because that is as far as
      the field is known. Supply the next time step with add_grid_scalars_at_time and call
      continue_traces to carry every unfinished trace onward::

          tracer.start_traces(seeds, seed_times)
          while tracer.continue_traces() > 0:
              step = series.next()
              if step is None:
                  break
              tracer.add_grid_scalars_at_time(*step)
          traces, times, reasons = tracer.get_trace_results()

      Stopping early is fine: traces still waiting end where they got to. One batch is in
      flight per tracer; starting a batch discards any previous one.

      Args:
          pts (iterable): The starting point of each trace.

          pt_times (iterable): The starting time of each trace, one per point.
  )pydoc";
  gridtrace.def("start_traces", [](xms::XmGridTrace &self, py::iterable pts,
    py::iterable pt_times) {
          boost::shared_ptr<xms::VecPt3d> points = xms::VecPt3dFromPyIter(pts);
          boost::shared_ptr<xms::VecDbl> times = xms::VecDblFromPyIter(pt_times);
          if (points->size() != times->size())
          {
            // Refuse it through StartTraces first. That clears the batch before validating,
            // and "starting a batch discards any previous one" has to hold on the error path
            // too: without this, a caller that catches the error reads the previous batch
            // back through get_trace_results as though it belonged to these seeds.
            self.StartTraces(*points, *times);
            // Raised rather than logged: the C++ side refuses the batch and returns empty,
            // which from Python would look like a tracer that silently did nothing.
            std::string msg = "start_traces needs one start time per point, got " +
                              std::to_string(points->size()) + " points and " +
                              std::to_string(times->size()) + " times";
            throw py::value_error(msg);
          }
          self.StartTraces(*points, *times);
        }, start_traces_doc, py::arg("pts"), py::arg("pt_times"));
  // ---------------------------------------------------------------------------
  // function: continue_traces
  // ---------------------------------------------------------------------------
  const char* continue_traces_doc = R"pydoc(
      Advances every unfinished trace as far as the loaded time steps allow.

      Releases the GIL while tracing, so a caller on a worker thread does not stall the
      interpreter. Tracing tens of thousands of seeds takes long enough for that to matter.

      Returns:
          int: How many traces are waiting on a later time step. Zero means every trace has
          ended for a reason more data cannot change.
  )pydoc";
  gridtrace.def("continue_traces", &xms::XmGridTrace::ContinueTraces,
    continue_traces_doc, py::call_guard<py::gil_scoped_release>());
  // ---------------------------------------------------------------------------
  // function: get_trace_results
  // ---------------------------------------------------------------------------
  const char* get_trace_results_doc = R"pydoc(
      Returns the batch traced so far.

      Valid at any point, complete once continue_traces has returned zero. An entry can hold
      fewer than two points: a seed that leaves the grid on its first step yields only the
      seed itself, so callers must not assume one usable polyline per seed.

      Returns:
          tuple: The positions of each trace, the times of each trace, and why each trace
          stopped as an exit_reason_enum. All three are parallel to the seeds passed to
          start_traces, and each entry's times are parallel to its positions.
  )pydoc";
  gridtrace.def("get_trace_results", [](const xms::XmGridTrace &self) -> py::iterable {
          std::vector<xms::VecPt3d> outTraces;
          std::vector<xms::VecDbl> outTimes;
          std::vector<xms::XmGridTraceExitEnum> outReasons;
          self.GetTraceResults(outTraces, outTimes, outReasons);
          py::list traces, times, reasons;
          for (size_t i = 0; i < outTraces.size(); ++i)
          {
            traces.append(xms::PyIterFromVecPt3d(outTraces[i]));
            times.append(xms::PyIterFromVecDbl(outTimes[i]));
            reasons.append(outReasons[i]);
          }
          return py::make_tuple(traces, times, reasons);
        }, get_trace_results_doc);
  // ---------------------------------------------------------------------------
  // function: get_seed_magnitudes
  // ---------------------------------------------------------------------------
  const char* get_seed_magnitudes_doc = R"pydoc(
      Returns the speed of the field at each seed of the batch, when it was released.

      Reports the batch, exactly as get_trace_results does: empty before start_traces and
      after a refused one, and untouched by trace_point, which traces through its own state.

      Recorded when the seed is first evaluated, before the vector multiplier is applied, so
      it describes the field rather than the tracing. Two components -- the tracer is
      two-dimensional and never reads a z velocity -- so this is sqrt(vx*vx + vy*vy).

      A seed the tracer never evaluated reports the XM_NODATA sentinel, a large negative
      value, rather than 0.0. Zero is a legal speed -- a seed in still water measures it and
      exits ZERO_VELOCITY -- so the two must not share a value. The sentinel covers every
      unevaluated case alike: not started, waiting for a later time step, not traceable, and
      extraction failed. Which one it was is in get_trace_results' exit reasons.

      Not safe to call while continue_traces runs on another thread: that call releases the
      GIL, and this one reads the batch it is writing.

      Returns:
          Sequence[float]: One speed per seed, parallel to the seeds passed to start_traces
          and to everything get_trace_results returns.
  )pydoc";
  gridtrace.def("get_seed_magnitudes", [](const xms::XmGridTrace &self) -> py::iterable {
          xms::VecDbl outMagnitudes;
          self.GetSeedMagnitudes(outMagnitudes);
          return xms::PyIterFromVecDbl(outMagnitudes);
        }, get_seed_magnitudes_doc);
  // ---------------------------------------------------------------------------
  // function: sample_vectors
  // ---------------------------------------------------------------------------
  const char* sample_vectors_doc = R"pydoc(
      Returns the field at each of a set of points, without tracing any of them.

      The interpolation a trace step already performs -- one point-location search per
      point against the cached triangulation, then the two loaded time steps blended at
      the given time -- for callers that want the field itself rather than a path through
      it.

      ON_GRID vector placement is the case this exists for. Its glyphs sit on a lattice
      anchored to the view and stepped by the camera's parallel scale, so every pan and
      every zoom moves all of them and the field has to be resampled at the new positions
      -- whether or not those glyphs are drawn following the flow. Rotation alone does
      not, since it moves neither the anchor nor the step.

      Two time steps must have been added, as tracing requires them. With fewer, every
      row is NaN rather than a partial answer.

      Reports the field, not the tracing: the vector multiplier is not applied, matching
      get_seed_magnitudes.

      Releases the GIL while sampling, so a caller on a worker thread does not stall the
      interpreter.

      Args:
          pts (iterable): The points to sample.

          time (float): The time to sample at, blended between the two loaded steps.

      Returns:
          numpy.ndarray: The field at each point, shape (N, 3), one row per point in the
          order given. A point outside the grid, in an inactive cell, or missing from
          either bracketing time step reports NaN in x and y. That is what xms.extractor's
          own binding reports for a miss, so numpy.isnan is the single test for one; note
          it differs from get_seed_magnitudes, which passes the C++ XM_NODATA sentinel
          through unchanged. The z column is always zero -- the tracer is two-dimensional
          and never reads or writes a z velocity.
  )pydoc";
  gridtrace.def("sample_vectors", [](const xms::XmGridTrace &self, py::iterable pts,
    double time) -> py::array {
          boost::shared_ptr<xms::VecPt3d> points = xms::VecPt3dFromPyIter(pts);
          xms::VecPt3d out;
          {
            // Pure computation over C++ state, as continue_traces is: nothing here
            // touches a Python object while the GIL is released.
            py::gil_scoped_release release;
            self.SampleVectors(*points, time, out);
          }
          py::array_t<double> result({static_cast<py::ssize_t>(out.size()),
                                      static_cast<py::ssize_t>(3)});
          auto rows = result.mutable_unchecked<2>();
          const double nan = std::numeric_limits<double>::quiet_NaN();
          for (py::ssize_t i = 0; i < rows.shape(0); ++i)
          {
            const xms::Pt3d& v = out[static_cast<size_t>(i)];
            // Translated at the boundary rather than in the caller: every consumer would
            // otherwise have to know a magic negative, and one that forgot would take
            // -9999999 for a velocity and size a glyph by it.
            const bool noData =
              xms::EQ_TOL(v.x, XM_NODATA, 1) || xms::EQ_TOL(v.y, XM_NODATA, 1);
            rows(i, 0) = noData ? nan : v.x;
            rows(i, 1) = noData ? nan : v.y;
            rows(i, 2) = 0.0;
          }
          return result;
        }, sample_vectors_doc, py::arg("pts"), py::arg("time"));

    // XmGridTraceExitEnum
    py::enum_<xms::XmGridTraceExitEnum>(m, "exit_reason_enum",
                    "exit_reason_enum why a trace stopped")
        .value("NOT_STARTED", xms::GTEXIT_NOT_STARTED)
        .value("WAITING_FOR_TIME_STEP", xms::GTEXIT_WAITING_FOR_TIME_STEP)
        .value("MAX_TRACING_TIME", xms::GTEXIT_MAX_TRACING_TIME)
        .value("MAX_TRACING_DISTANCE", xms::GTEXIT_MAX_TRACING_DISTANCE)
        .value("LEFT_GRID", xms::GTEXIT_LEFT_GRID)
        .value("ZERO_VELOCITY", xms::GTEXIT_ZERO_VELOCITY)
        .value("MIN_DELTA_TIME", xms::GTEXIT_MIN_DELTA_TIME)
        .value("SEED_NOT_TRACEABLE", xms::GTEXIT_SEED_NOT_TRACEABLE)
        .value("EXTRACTION_FAILED", xms::GTEXIT_EXTRACTION_FAILED);

    // DataLocationEnum is deliberately NOT registered here. It is xmsextractor's type, and
    // xms.extractor registers it under this same name; pybind11's type registry is
    // process-global, so a second registration of the same C++ type makes whichever module
    // imports second raise "generic_type: type data_location_enum is already registered" --
    // in either order, which made the two wheels mutually exclusive in one process. Nothing
    // needed it: this module's location arguments are strings, converted to the enum as a
    // local in add_grid_scalars_at_time. A caller who does want the type itself finds it at
    // xms.extractor._xmsextractor.extractor.data_location_enum -- the extractor package does
    // not re-export it under its public name either, so there is no tidier spelling to give.
}
