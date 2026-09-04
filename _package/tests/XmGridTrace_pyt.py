"""Test GridTrace."""

# 1. Standard Python modules
import unittest

# 2. Third party modules
import numpy as np

# 3. Aquaveo modules
from xms.grid.ugrid import UGrid

# 4. Local modules
from xms.gridtrace import _xmsgridtrace as gridtrace_module
from xms.gridtrace import exit_reason_enum, GridTrace


class TestGridTrace(unittest.TestCase):
    """GridTrace tests.

    Traced times are compared approximately, not exactly. They are doubles derived from
    float32 grid scalars, and the compiler may contract ``a * b + c * d`` into an FMA -- which
    of the two products lands inside the FMA is computed exactly while the other is rounded,
    so the last bit depends on the order the terms are written in. These assertions used
    ``assert_array_equal``, which pinned that decision rather than the tracer's behaviour, and
    it broke on a correct interpolation fix that only swapped which weight multiplies which
    time step. Positions were already compared approximately; times now match.
    """

    def create_default_single_cell(self):
        """Create a default single cell.

        Returns:
            GridTrace: A tracer for a two triangle grid
        """
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [UGrid.cell_type_enum.TRIANGLE, 3, 0, 1, 2,
                 UGrid.cell_type_enum.TRIANGLE, 3, 2, 3, 0]
        ugrid = UGrid(points, cells)
        tracer = GridTrace(ugrid)
        self.assertIsInstance(tracer, GridTrace)
        tracer.vector_multiplier = 1
        tracer.max_tracing_time = 100
        tracer.max_tracing_distance = 100
        tracer.min_delta_time = .1
        tracer.max_change_distance = 100
        tracer.max_change_velocity = 100
        tracer.max_change_direction_in_radians = 1.5 * np.pi
        scalars = [(1, 1, 0), (1, 1, 0), (1, 1, 0), (1, 1, 0)]
        point_activity = [True] * 4
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 10)
        return tracer

    def create_default_two_cell(self):
        """Create a default two cell.

        Returns:
            GridTrace: A tracer for a two quad grid
        """
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0, 0), (2, 1, 0)]
        cells = [UGrid.cell_type_enum.QUAD, 4, 0, 1, 2, 3, UGrid.cell_type_enum.QUAD, 4, 1, 4, 5, 2]
        ugrid = UGrid(points, cells)
        tracer = GridTrace(ugrid)
        self.assertIsInstance(tracer, GridTrace)
        tracer.vector_multiplier = 1
        tracer.max_tracing_time = 100
        tracer.max_tracing_distance = 100
        tracer.min_delta_time = .1
        tracer.max_change_distance = 100
        tracer.max_change_velocity = 100
        tracer.max_change_direction_in_radians = 1.5 * np.pi
        scalars = [(.1, 0, 0), (.2, 0, 0)]
        point_activity = [True] * 2
        tracer.add_grid_scalars_at_time(scalars, "cells", point_activity, "cells", 0)
        tracer.add_grid_scalars_at_time(scalars, "cells", point_activity, "cells", 10)
        return tracer

    def test_basic_trace_point(self):
        """Test basic tracing functionality."""
        tracer = self.create_default_single_cell()
        start_time = .5

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(.5, .5, 0), (1, 1, 0)]
        expected_out_times = [.5, 1]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_max_change_distance(self):
        """Test max change distance functionality."""
        tracer = self.create_default_single_cell()
        start_time = .5
        tracer.max_change_distance = .25

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(.5, .5, 0),
                              (0.67677668424809445, 0.67677668424809445, 0.00000000000000000),
                              (0.85355336849618890, 0.85355336849618890, 0.00000000000000000),
                              (1, 1, 0)]
        expected_out_times = [.5, 0.67677668424809445, 0.85355336849618890, 1]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_small_scalars_trace_point(self):
        """Test functionality with small scalars."""
        tracer = self.create_default_single_cell()
        start_time = .5
        tracer.max_change_distance = .25
        scalars = [(.1, .1, 0), (.1, .1, 0), (.1, .1, 0), (.1, .1, 0)]
        point_activity = [True] * 4
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 10)

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(.5, .5, 0),
                              (0.60000000149011612, 0.60000000149011612, 0),
                              (0.72000000327825542, 0.72000000327825542, 0),
                              (0.86400000542402267, 0.86400000542402267, 0),
                              (1, 1, 0)]
        expected_out_times = [.5, 1.5, 2.7, 4.14, 5.5]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_strong_direction_change(self):
        """Test functionality with strong changes in direction."""
        tracer = self.create_default_single_cell()
        tracer.max_change_direction_in_radians = np.pi * .2
        tracer.min_delta_time = -1
        start_time = .5

        scalars = [(0, 1, 0), (-1, 0, 0), (0, -1, 0), (1, 0, 0)]
        point_activity = [True] * 4
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 10)

        result_tuple = tracer.trace_point((0, 0, 0), start_time)

        expected_out_trace = [(0, 0, 0),
                              (0.00000000000000000, 0.25000000000000000, 0.00000000000000000),
                              (0.074999999999999997, 0.47499999999999998, 0.00000000000000000),
                              (0.21900000214576720, 0.63699999570846555, 0.00000000000000000),
                              (0.30928799843788146, 0.66810399758815764, 0.00000000000000000),
                              (0.40229310507774352, 0.67396399235725402, 0.00000000000000000),
                              (0.48679361495018003, 0.65024498560905453, 0.00000000000000000),
                              (0.54780151323509219, 0.59909560095787040, 0.00000000000000000),
                              (0.55928876277122497, 0.56619817004051198, 0.00000000000000000),
                              (0.56114558691518779, 0.53247499044700608, 0.00000000000000000),
                              (0.55189971330840681, 0.50228363992173752, 0.00000000000000000),
                              (0.53269911067322617, 0.48131557500677169, 0.00000000000000000),
                              (0.52076836142536975, 0.47806150355091476, 0.00000000000000000),
                              (0.50886902895577013, 0.47838753608466128, 0.00000000000000000),
                              (0.49867742691962913, 0.48264835153512164, 0.00000000000000000),
                              (0.49224616907898289, 0.49014090685121131, 0.00000000000000000),
                              (0.49173935940609609, 0.49438094923206660, 0.00000000000000000),
                              (0.49250246625151450, 0.49839053740482164, 0.00000000000000000),
                              (0.49454361321306389, 0.50154755045413602, 0.00000000000000000),
                              (0.49745717820065949, 0.50317358562752867, 0.00000000000000000),
                              (0.49888395770889871, 0.50301615091938545, 0.00000000000000000),
                              (0.50012160117661586, 0.50244704462921241, 0.00000000000000000),
                              (0.50095740046883197, 0.50152383477622209, 0.00000000000000000),
                              (0.50107955145675120, 0.50098875888952354, 0.00000000000000000),
                              (0.50105605626599747, 0.50045352403892940, 0.00000000000000000),
                              (0.50086894918345870, 0.49998474718699493, 0.00000000000000000),
                              (0.50053945884260675, 0.49966662451478433, 0.00000000000000000),
                              (0.50034430627617277, 0.49962054739305783, 0.00000000000000000),
                              (0.50015012042108842, 0.49962997721910873, 0.00000000000000000),
                              (0.49998265395837810, 0.49970077747304897, 0.00000000000000000),
                              (0.49987374966305814, 0.49982308521808211, 0.00000000000000000),
                              (0.49986302487024292, 0.49988726006383088, 0.00000000000000000),
                              (0.49986815504448728, 0.49994012045071656, 0.00000000000000000)]
        expected_out_times = [.5,
                              0.75000000000000000,
                              1.0500000000000000,
                              1.4100000000000001,
                              1.6260000000000001,
                              1.8852000000000002,
                              2.1962400000000004,
                              2.5694880000000002,
                              2.7934368000000003,
                              3.0621753600000003,
                              3.3846616320000003,
                              3.7716451584000001,
                              4.0038352742400001,
                              4.2824634132480002,
                              4.6168171800576001,
                              5.0180417002291202,
                              5.2587764123320317,
                              5.5476580668555258,
                              5.8943160522837186,
                              6.3103056347975501,
                              6.5598993843058491,
                              6.8594118837158078,
                              7.2188268830077584,
                              7.4344758825829285,
                              7.6932546820731327,
                              8.0037892414613783,
                              8.3764307127272737,
                              8.6000155954868092,
                              8.8683174547982535,
                              9.1902796859719871,
                              9.5766343633804656,
                              9.7883171816902319,
                              10.000000000000000]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_max_tracing_time(self):
        """Test functionality of max tracing time."""
        tracer = self.create_default_single_cell()
        tracer.max_change_direction_in_radians = np.pi * .2
        tracer.min_delta_time = -1
        tracer.max_tracing_time = 5

        start_time = .5
        scalars = [(0, 1, 0), (-1, 0, 0), (0, -1, 0), (1, 0, 0)]
        point_activity = [True] * 4
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 10)

        result_tuple = tracer.trace_point((0, 0, 0), start_time)

        expected_out_trace = [(0, 0, 0),
                              (0.00000000000000000, 0.25000000000000000, 0.00000000000000000),
                              (0.074999999999999997, 0.47499999999999998, 0.00000000000000000),
                              (0.21900000214576720, 0.63699999570846555, 0.00000000000000000),
                              (0.30928799843788146, 0.66810399758815764, 0.00000000000000000),
                              (0.40229310507774352, 0.67396399235725402, 0.00000000000000000),
                              (0.48679361495018003, 0.65024498560905453, 0.00000000000000000),
                              (0.54780151323509219, 0.59909560095787040, 0.00000000000000000),
                              (0.55928876277122497, 0.56619817004051198, 0.00000000000000000),
                              (0.56114558691518779, 0.53247499044700608, 0.00000000000000000),
                              (0.55189971330840681, 0.50228363992173752, 0.00000000000000000),
                              (0.53269911067322617, 0.48131557500677169, 0.00000000000000000),
                              (0.52076836142536975, 0.47806150355091476, 0.00000000000000000),
                              (0.50886902895577013, 0.47838753608466128, 0.00000000000000000),
                              (0.49867742691962913, 0.48264835153512164, 0.00000000000000000),
                              (0.49224616907898289, 0.49014090685121131, 0.00000000000000000),
                              (0.49173935940609609, 0.49438094923206660, 0.00000000000000000),
                              (0.49237657318600692, 0.49772905815126539, 0.00000000000000000)]
        expected_out_times = [.5,
                              0.75000000000000000,
                              1.0500000000000000,
                              1.4100000000000001,
                              1.6260000000000001,
                              1.8852000000000002,
                              2.1962400000000004,
                              2.5694880000000002,
                              2.7934368000000003,
                              3.0621753600000003,
                              3.3846616320000003,
                              3.7716451584000001,
                              4.0038352742400001,
                              4.2824634132480002,
                              4.6168171800576001,
                              5.0180417002291202,
                              5.2587764123320317,
                              5.5]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def create_rotating_field_tracer(self):
        """Create a tracer over one cell spanning the domain, with the field rotating +x -> +y.

        One cell means the field is spatially uniform, so any change in a path comes from time.

        Returns:
            GridTrace: A tracer with two time steps loaded
        """
        points = [(0, 0, 0), (40, 0, 0), (40, 40, 0), (0, 40, 0)]
        cells = [UGrid.cell_type_enum.QUAD, 4, 0, 1, 2, 3]
        tracer = GridTrace(UGrid(points, cells))
        tracer.vector_multiplier = 1
        tracer.max_tracing_time = 18
        tracer.max_tracing_distance = 1000
        tracer.min_delta_time = .01
        tracer.max_change_distance = .5
        tracer.max_change_velocity = -1
        tracer.max_change_direction_in_radians = np.pi  # never subdivide on direction
        tracer.add_grid_scalars_at_time([(1, 0, 0)], 'cells', [True], 'cells', 0)
        tracer.add_grid_scalars_at_time([(0, 1, 0)], 'cells', [True], 'cells', 10)
        return tracer

    def test_traces_continue_across_time_steps(self):
        """A trace continues past the second time step once a later one is supplied."""
        seeds = [(20, 10, 0)]
        seed_times = [0]

        # Never given the third time step: it must stop at the second and say so.
        stopped = self.create_rotating_field_tracer()
        stopped.start_traces(seeds, seed_times)
        self.assertEqual(1, stopped.continue_traces())
        stopped_traces, stopped_times, stopped_reasons = stopped.get_trace_results()
        self.assertEqual(exit_reason_enum.WAITING_FOR_TIME_STEP, stopped_reasons[0])
        self.assertAlmostEqual(10.0, stopped_times[0][-1])

        # Given the third: it must resume and run out its tracing time instead.
        tracer = self.create_rotating_field_tracer()
        tracer.start_traces(seeds, seed_times)
        self.assertEqual(1, tracer.continue_traces())
        tracer.add_grid_scalars_at_time([(-1, 0, 0)], 'cells', [True], 'cells', 20)
        self.assertEqual(0, tracer.continue_traces())
        traces, times, reasons = tracer.get_trace_results()
        self.assertEqual(exit_reason_enum.MAX_TRACING_TIME, reasons[0])
        self.assertAlmostEqual(18.0, times[0][-1])

        # Resuming extends the path; it does not restart it.
        self.assertGreater(len(traces[0]), len(stopped_traces[0]))
        np.testing.assert_array_almost_equal(stopped_traces[0], traces[0][:len(stopped_traces[0])])
        np.testing.assert_array_almost_equal(stopped_times[0], times[0][:len(stopped_times[0])])

        # The third time step reverses the eastward drift, so the path turns back on itself --
        # something no single pair of these time steps can produce.
        max_x = max(pt[0] for pt in traces[0])
        self.assertGreater(max_x, seeds[0][0])
        self.assertLess(traces[0][-1][0], max_x)

    def test_start_traces_rejects_mismatched_times(self):
        """A caller supplying the wrong number of start times gets an error, not a silent no-op."""
        tracer = self.create_rotating_field_tracer()
        tracer.start_traces([(20, 10, 0), (21, 10, 0)], [0, 0])
        with self.assertRaises(ValueError):
            tracer.start_traces([(20, 10, 0), (21, 10, 0)], [0])
        # Refused, and the batch it refused is gone rather than readable: a caller that catches
        # this must not get the previous batch's results back as though they were these seeds'.
        traces, _times, _reasons = tracer.get_trace_results()
        self.assertEqual(0, len(traces))

    def test_batch_matches_trace_point(self):
        """The batch returns what serial trace_point calls return."""
        seeds = [(.5, .5, 0), (.25, .75, 0), (-.1, 0, 0)]
        seed_times = [.5, .5, .5]

        serial = self.create_default_single_cell()
        expected = [serial.trace_point(pt, t) for pt, t in zip(seeds, seed_times)]

        batch = self.create_default_single_cell()
        batch.start_traces(seeds, seed_times)
        batch.continue_traces()
        traces, times, reasons = batch.get_trace_results()

        self.assertEqual(len(seeds), len(traces))
        for i, (expected_trace, expected_times) in enumerate(expected):
            np.testing.assert_array_almost_equal(expected_trace, traces[i])
            np.testing.assert_array_almost_equal(expected_times, times[i])
        # The seed outside the grid yields no polyline -- callers cannot assume one per seed.
        self.assertEqual(0, len(traces[2]))
        self.assertEqual(exit_reason_enum.SEED_NOT_TRACEABLE, reasons[2])

    def test_seed_magnitudes_report_the_field(self):
        """The seed speeds describe the field, not the tracing."""
        # The default fixture's field is a uniform (1, 1), so every seed on the grid sees
        # exactly sqrt(2). The third seed is off the grid and is never evaluated.
        seeds = [(.5, .5, 0), (.25, .75, 0), (-.1, 0, 0)]
        seed_times = [.5, .5, .5]
        expected = np.sqrt(2)

        tracer = self.create_default_single_cell()
        tracer.start_traces(seeds, seed_times)
        tracer.continue_traces()
        _traces, _times, reasons = tracer.get_trace_results()
        magnitudes = list(tracer.get_seed_magnitudes())

        self.assertEqual(len(seeds), len(magnitudes))
        self.assertAlmostEqual(expected, magnitudes[0])
        self.assertAlmostEqual(expected, magnitudes[1])
        # Negative, not zero: zero is a legal speed, and this seed never got a field value at
        # all. The wrapper hands back the tracer's XM_NODATA sentinel; asserting only that it is
        # not a speed keeps the test off the sentinel's exact value. Which unevaluated case it
        # was is in the exit reason.
        self.assertLess(magnitudes[2], 0.0)
        self.assertEqual(exit_reason_enum.SEED_NOT_TRACEABLE, reasons[2])

        # The multiplier scales how far the tracer steps, not what the field measures. If it
        # leaked in here, a caller would be sizing its glyphs by a tracing parameter.
        scaled = self.create_default_single_cell()
        scaled.vector_multiplier = 5
        scaled.start_traces(seeds, seed_times)
        scaled.continue_traces()
        scaled_magnitudes = list(scaled.get_seed_magnitudes())
        self.assertAlmostEqual(expected, scaled_magnitudes[0])
        self.assertAlmostEqual(expected, scaled_magnitudes[1])

        # A refused batch reports no magnitudes either, rather than a stale set from the batch
        # before it, so the speeds can never be zipped against a different batch's seeds.
        with self.assertRaises(ValueError):
            tracer.start_traces(seeds, [.5])
        self.assertEqual(0, len(list(tracer.get_seed_magnitudes())))

    def test_sample_vectors_reports_the_field_without_tracing(self):
        """A point with no field is absent from the result rather than marked in it."""
        # The default fixture's field is a uniform (1, 1). The miss sits between two hits: an
        # implementation that stopped at the first point it could not place would drop the
        # third too, and one that let the two outputs slip would pair the third point with
        # the second's vector.
        pts = [(.5, .5, 0), (-1, -1, 0), (.25, .75, 0)]

        tracer = self.create_default_single_cell()
        kept, vectors = tracer.sample_vectors(pts, 0)

        self.assertEqual(2, len(kept))
        self.assertEqual(2, len(vectors))
        # The points that survived are the caller's own, echoed in order -- which is what
        # lets a glyph be drawn from the result without consulting the input at all.
        self.assertAlmostEqual(pts[0][0], kept[0][0])
        self.assertAlmostEqual(pts[0][1], kept[0][1])
        self.assertAlmostEqual(pts[2][0], kept[1][0])
        self.assertAlmostEqual(pts[2][1], kept[1][1])
        for vector in vectors:
            self.assertAlmostEqual(1.0, vector[0])
            self.assertAlmostEqual(1.0, vector[1])
            # z is written, not left as the tracer found it. It never reads a z velocity.
            self.assertEqual(0.0, vector[2])

        # Sampling touches no tracing state, so a batch started afterward is unaffected and a
        # tracer that has never traced can still be sampled.
        tracer.start_traces([(.5, .5, 0)], [.5])
        tracer.continue_traces()
        traces, _times, _reasons = tracer.get_trace_results()
        self.assertEqual(1, len(traces))

        # The multiplier scales how far the tracer steps, not what the field measures.
        scaled = self.create_default_single_cell()
        scaled.vector_multiplier = 5
        _scaled_pts, scaled_vectors = scaled.sample_vectors([(.5, .5, 0)], 0)
        self.assertAlmostEqual(1.0, scaled_vectors[0][0])

    def test_sample_vectors_keeps_only_traceable_seeds(self):
        """The points kept are the seeds start_traces will accept, inactive cells included."""
        # Two quads side by side, the second inactive in both loaded steps. A point in it is
        # inside the grid, so a hit test that only asked whether a point lands in the mesh
        # would keep it and then hand the caller a seed that traces nothing.
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0, 0), (2, 1, 0)]
        cells = [UGrid.cell_type_enum.QUAD, 4, 0, 1, 2, 3,
                 UGrid.cell_type_enum.QUAD, 4, 1, 4, 5, 2]
        tracer = GridTrace(UGrid(points, cells))
        tracer.vector_multiplier = 1
        tracer.max_tracing_time = 100
        tracer.max_tracing_distance = 100
        tracer.min_delta_time = .1
        tracer.max_change_distance = 100
        tracer.max_change_velocity = 100
        tracer.max_change_direction_in_radians = 1.5 * np.pi
        scalars = [(1, 0, 0), (1, 0, 0)]
        activity = [True, False]
        tracer.add_grid_scalars_at_time(scalars, "cells", activity, "cells", 20)
        tracer.add_grid_scalars_at_time(scalars, "cells", activity, "cells", 30)

        # In the active cell, in the inactive one, and off the grid entirely.
        pts = [(.5, .5, 0), (1.5, .5, 0), (-1, -1, 0)]
        kept, _vectors = tracer.sample_vectors(pts, 20)

        self.assertEqual(1, len(kept))
        self.assertAlmostEqual(pts[0][0], kept[0][0])

        # The same three as seeds: only the one sample_vectors kept produces a path.
        tracer.start_traces(pts, [20, 20, 20])
        tracer.continue_traces()
        traces, _times, reasons = tracer.get_trace_results()
        self.assertEqual(3, len(traces))
        self.assertTrue(len(traces[0]) > 0)
        self.assertEqual(0, len(traces[1]))
        self.assertEqual(0, len(traces[2]))
        self.assertEqual(exit_reason_enum.SEED_NOT_TRACEABLE, reasons[1])
        self.assertEqual(exit_reason_enum.SEED_NOT_TRACEABLE, reasons[2])

    def test_extractor_can_be_imported_alongside(self):
        """xms.extractor and xms.gridtrace must both load in one process.

        Both wheels bind xmsextractor's DataLocationEnum, and pybind11's type registry is
        process-global, so registering it here too made whichever module imported second raise
        "generic_type: type data_location_enum is already registered", which made the two
        mutually exclusive whichever way round a program imported them. This module no longer
        registers it; its own location arguments are strings.

        This test exercises one of those two orders -- gridtrace is imported at module scope,
        so extractor is always the one loading second here. The reverse needs its own process.
        """
        try:
            import xms.extractor  # noqa: F401
        except ModuleNotFoundError as exc:  # pragma: no cover - depends on optional wheel
            # Deliberately not ImportError: pybind11 reports a duplicate registration through
            # PyExc_ImportError, so catching the base class would turn the very regression this
            # test exists to catch into a green skip.
            self.skipTest(f'xms.extractor not installed: {exc}')

        # On the gridtrace submodule, which is where the enums are registered -- the top-level
        # extension never had this attribute, so asserting there would pass either way.
        self.assertFalse(hasattr(gridtrace_module.gridtrace, 'data_location_enum'))

    def test_max_tracing_distance(self):
        """Test functionality of max tracing distance."""
        tracer = self.create_default_single_cell()
        tracer.max_change_direction_in_radians = np.pi * .2
        tracer.min_delta_time = -1
        tracer.max_tracing_distance = 1.0
        start_time = .5

        scalars = [(0, 1, 0), (-1, 0, 0), (0, -1, 0), (1, 0, 0)]
        point_activity = [True] * 4
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 10)

        result_tuple = tracer.trace_point((0, 0, 0), start_time)

        expected_out_trace = [(0, 0, 0),
                              (0.00000000000000000, 0.25000000000000000, 0.00000000000000000),
                              (0.074999999999999997, 0.47499999999999998, 0.00000000000000000),
                              (0.21900000214576720, 0.63699999570846555, 0.00000000000000000),
                              (0.30928799843788146, 0.66810399758815764, 0.00000000000000000),
                              (0.40229310507774352, 0.67396399235725402, 0.00000000000000000),
                              (0.48679361495018003, 0.65024498560905453, 0.00000000000000000),
                              (0.50183556502673621, 0.63763372523131490, 0.00000000000000000)]
        expected_out_times = [.5,
                              0.75000000000000000,
                              1.0500000000000000,
                              1.4100000000000001,
                              1.6260000000000001,
                              1.8852000000000002,
                              2.1962400000000004,
                              2.4774609356360582]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0], 6)
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_start_out_of_cell(self):
        """Test functionality of starting outside of cell."""
        tracer = self.create_default_single_cell()
        start_time = .5

        result_tuple = tracer.trace_point((-1, 0, 0), start_time)

        expected_out_times = []
        np.testing.assert_equal(0, len(result_tuple[0]))
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])
        self.assertEqual(exit_reason_enum.SEED_NOT_TRACEABLE, tracer.get_exit_reason())

    def test_beyond_timestep(self):
        """Test that a start time past the loaded window waits rather than failing."""
        tracer = self.create_default_single_cell()
        start_time = 10.1

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_times = []
        np.testing.assert_equal(0, len(result_tuple[0]))
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])
        # This and test_start_out_of_cell both produce an empty trace, so emptiness alone
        # cannot tell them apart -- which is how this case went unnoticed as an extraction
        # failure. The field is not known this far ahead yet; the trace is waiting for data.
        self.assertEqual(exit_reason_enum.WAITING_FOR_TIME_STEP, tracer.get_exit_reason())

    def test_before_timestep(self):
        """Test functionality of starting before the time step."""
        tracer = self.create_default_single_cell()
        start_time = -0.1

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(.5, .5, 0), (1, 1, 0)]
        expected_out_times = [-.1, .4]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_vector_multiplier(self):
        """Test functionality of vector multiplier."""
        tracer = self.create_default_single_cell()
        tracer.max_change_direction_in_radians = np.pi * .2
        tracer.min_delta_time = -1
        tracer.vector_multiplier = 0.5
        start_time = .5

        scalars = [(0, 1, 0), (-1, 0, 0), (0, -1, 0), (1, 0, 0)]
        point_activity = [True] * 4
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 10)

        result_tuple = tracer.trace_point((0, 0, 0), start_time)

        expected_out_trace = [(0, 0, 0),
                              (0.00000000000000000, 0.25000000000000000, 0.00000000000000000),
                              (0.074999999999999997, 0.47499999999999998, 0.00000000000000000),
                              (0.21900000214576720, 0.63699999570846555, 0.00000000000000000),
                              (0.30928799843788146, 0.66810399758815764, 0.00000000000000000),
                              (0.40229310507774352, 0.67396399235725402, 0.00000000000000000),
                              (0.48679361495018003, 0.65024498560905453, 0.00000000000000000),
                              (0.54780151323509219, 0.59909560095787040, 0.00000000000000000),
                              (0.55928876277122497, 0.56619817004051198, 0.00000000000000000),
                              (0.56114558691518779, 0.53247499044700608, 0.00000000000000000),
                              (0.55189971330840681, 0.50228363992173752, 0.00000000000000000),
                              (0.53269911067322617, 0.48131557500677169, 0.00000000000000000),
                              (0.52076836142536975, 0.47806150355091476, 0.00000000000000000),
                              (0.50886902895577013, 0.47838753608466128, 0.00000000000000000),
                              (0.49867742691962913, 0.48264835153512164, 0.00000000000000000),
                              (0.49224616907898289, 0.49014090685121131, 0.00000000000000000),
                              (0.49175783605462037, 0.49422637094165467, 0.00000000000000000)]
        expected_out_times = [.5,
                              1.0000000000000000,
                              1.6000000000000001,
                              2.3200000000000003,
                              2.7520000000000002,
                              3.2704000000000004,
                              3.8924800000000004,
                              4.6389760000000004,
                              5.0868736000000006,
                              5.6243507200000007,
                              6.2693232640000005,
                              7.0432903168000003,
                              7.5076705484800001,
                              8.0649268264960003,
                              8.7336343601152002,
                              9.5360834004582404,
                              10.000000000000000]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_multi_cell(self):
        """Test default functionality of multiple cells."""
        tracer = self.create_default_two_cell()
        start_time = 0

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(.5, .5, 0),
                              (0.60000000149011612, 0.50000000000000000, 0.00000000000000000),
                              (0.73200000077486038, 0.50000000000000000, 0.00000000000000000),
                              (0.90940801054239273, 0.50000000000000000, 0.00000000000000000),
                              (1.1529537134766579, 0.50000000000000000, 0.00000000000000000),
                              (1.4957102079987525, 0.50000000000000000, 0.00000000000000000),
                              (1.9923067892670629, 0.50000000000000000, 0.00000000000000000),
                              (2, .5, 0)]
        expected_out_times = [0,
                              1.0000000000000000,
                              2.2000000000000002,
                              3.6400000000000001,
                              5.3680000000000003,
                              7.4416000000000002,
                              9.9299199999999992,
                              9.9683860530914945]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_max_change_velocity(self):
        """Test functionality of max change in velocity."""
        tracer = self.create_default_two_cell()
        tracer.max_change_velocity = .01
        tracer.min_delta_time = .001
        start_time = 0

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(.5, .5, 0),
                              (0.60000000149011612, 0.50000000000000000, 0.00000000000000000),
                              (0.66600000113248825, 0.50000000000000000, 0.00000000000000000),
                              (0.74995200067758561, 0.50000000000000000, 0.00000000000000000),
                              (0.80394992786645891, 0.50000000000000000, 0.00000000000000000),
                              (0.87154669338464741, 0.50000000000000000, 0.00000000000000000),
                              (0.95686786960840231, 0.50000000000000000, 0.00000000000000000),
                              (1.0112451727318765, 0.50000000000000000, 0.00000000000000000),
                              (1.0789334834771158, 0.50000000000000000, 0.00000000000000000),
                              (1.1637975516948893, 0.50000000000000000, 0.00000000000000000),
                              (1.2174527417415202, 0.50000000000000000, 0.00000000000000000),
                              (1.2839153379250163, 0.50000000000000000, 0.00000000000000000),
                              (1.3667568384715398, 0.50000000000000000, 0.00000000000000000),
                              (1.4187699365302351, 0.50000000000000000, 0.00000000000000000),
                              (1.4829247317645506, 0.50000000000000000, 0.00000000000000000),
                              (1.5624845364724593, 0.50000000000000000, 0.00000000000000000),
                              (1.6587784227485147, 0.50000000000000000, 0.00000000000000000),
                              (1.7743310862797812, 0.50000000000000000, 0.00000000000000000),
                              (1.9129942825173010, 0.50000000000000000, 0.00000000000000000),
                              (2, .5, 0)]
        expected_out_times = [0,
                              1.0000000000000000,
                              1.6000000000000001,
                              2.3200000000000003,
                              2.7520000000000002,
                              3.2704000000000004,
                              3.8924800000000004,
                              4.2657280000000002,
                              4.7136256000000003,
                              5.2511027200000004,
                              5.5735889920000004,
                              5.9605725184000002,
                              6.4249527500800001,
                              6.7035808890880002,
                              7.0379346558976001,
                              7.4391591760691202,
                              7.9206286002749442,
                              8.4983919093219331,
                              9.1917078801783187,
                              9.6267364611093829]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_unique_time_steps(self):
        """Test functionality of unique time steps."""
        tracer = self.create_default_two_cell()
        start_time = 10

        scalars = [(.2, 0, 0), (.3, 0, 0)]
        point_activity = [True] * 2
        tracer.add_grid_scalars_at_time(scalars, "cells", point_activity, "cells", 20)

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(0.5, 0.5, 0),
                              (0.60000000149011612, 0.5, 0),
                              (0.74400000184774395, 0.5, 0),
                              (0.95481600679159162, 0.5, 0),
                              (1.2691074101881981, 0.5, 0),
                              (1.747260385068264, 0.5, 0),
                              (2, 0.5, 0)]
        expected_out_times = [10,
                              11,
                              12.199999999999999,
                              13.640000000000001,
                              15.368,
                              17.441600000000001,
                              18.362609001148471]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_inactive_cell(self):
        """Test functionality of inactive cells."""
        tracer = self.create_default_two_cell()
        start_time = 10

        scalars = [(.2, 0, 0), (99999, 0, 0)]
        point_activity = [True] * 2
        point_activity[1] = False
        tracer.add_grid_scalars_at_time(scalars, "cells", point_activity, "cells", 20)

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_trace = [(0.5, 0.5, 0),
                              (0.60000000149011612, 0.5, 0),
                              (0.74280000120401379, 0.5, 0),
                              (0.94575130454301826, 0.5, 0),
                              (1, 0.5, 0)]
        expected_out_times = [10,
                              11,
                              12.199999999999999,
                              13.640000000000001,
                              13.969279307058475]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_start_inactive_cell(self):
        """Test functionality of starting in an inactive cell."""
        tracer = self.create_default_two_cell()
        start_time = 10

        scalars = [(.2, 0, 0), (99999, 0, 0)]
        point_activity = [True] * 2
        point_activity[0] = False
        tracer.add_grid_scalars_at_time(scalars, "cells", point_activity, "cells", 20)

        result_tuple = tracer.trace_point((.5, .5, 0), start_time)

        expected_out_times = []
        np.testing.assert_equal(0, len(result_tuple[0]))
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])

    def test_tutorial(self):
        """A test to serve as a tutorial."""
        #  ->   ->
        #  6----7----8|
        #  |    |    |v
        #  |    |    |
        #  |    |   |
        # ^|    |    |
        # |3----4----5|
        #  |    |    |v
        #  |    |    |
        #  |    |    |
        # ^|    |    |
        # |0----1----2
        #      <-   <--
        # Step 1: Create the grid
        points = [(0, 0, 0), (1, 0, 0), (2, 0, 0),
                  (0, 1, 0), (1, 1, 0), (2, 1, 0),
                  (0, 2, 0), (1, 2, 0), (2, 2, 0)]
        cells = [UGrid.cell_type_enum.QUAD, 4, 0, 1, 4, 3,
                 UGrid.cell_type_enum.QUAD, 4, 1, 2, 5, 4,
                 UGrid.cell_type_enum.QUAD, 4, 3, 4, 7, 6,
                 UGrid.cell_type_enum.QUAD, 4, 4, 5, 8, 7]
        ugrid = UGrid(points, cells)
        # Step 2: Create the tracer from the grid
        tracer = GridTrace(ugrid)

        # Step 3: Set up the constraints on the tracer
        self.assertIsInstance(tracer, GridTrace)
        tracer.vector_multiplier = 2
        tracer.max_tracing_time = -1
        tracer.max_tracing_distance = -1
        tracer.min_delta_time = .01
        tracer.max_change_distance = -1
        tracer.max_change_velocity = -1
        tracer.max_change_direction_in_radians = .25 * np.pi
        # Step 4: Set up the velocity vectors for both time steps. Insert timesteps sequentially
        # For this case Scalars are set such that they circle around the edge of the graph in a clockwise direction
        # Z component is not used in scalars
        scalars = [(0, 1, 0), (-.1, 0, 0), (-1, 0, 0),
                   (0, .1, 0), (0, 0, 0), (0, -.1, 0),
                   (1, 0, 0), (.1, 0, 0), (0, -1, 0)]
        point_activity = [True] * 9
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 0)
        # For the second timestep scalars are doubled to indicate an increase in magnitude
        scalars = [(0, 2, 0), (-.2, 0, 0), (-2, 0, 0),
                   (0, .2, 0), (0, 0, 0), (0, -.2, 0),
                   (2, 0, 0), (.2, 0, 0), (0, -2, 0)]
        tracer.add_grid_scalars_at_time(scalars, "points", point_activity, "points", 20)

        start_time = 0
        start_point = (.5, .5, 0)
        result_tuple = tracer.trace_point(start_point, start_time)
        # show the cause for termination by calling get_exit_message
        print(tracer.get_exit_message())

        # Expected values for this simulation
        expected_out_trace = [(0.5, 0.5, 0),
                              (0.5, 1.5, 0),
                              (0.62600000187754634, 1.6260000018775462, 0),
                              (0.82611968728899965, 1.7455603212296962, 0),
                              (0.97840008102011689, 1.7810753047635555, 0),
                              (1.0280095840364933, 1.7824472100312621, 0),
                              (1.0861189816907613, 1.7608732599310344, 0),
                              (1.1492686295114336, 1.6802752810470523, 0),
                              (1.2097920698566107, 1.5101408581884392, 0),
                              (1.2515951471975522, 1.2181485463468757, 0),
                              (1.2515951471975522, 0.84053651390559747, 0),
                              (1.2181758214493843, 0.78780883088769804, 0),
                              (1.1632869448015855, 0.73137186792498654, 0),
                              (1.0771209832183524, 0.67899546053648097, 0),
                              (1.0129487663521615, 0.66357815692798783, 0),
                              (0.97169356095126669, 0.66199025753694563, 0),
                              (0.92552080990281416, 0.70419149113367874, 0),
                              (0.88530832700558759, 0.83950990950827409, 0),
                              (0.87513974259796246, 1.0941588844381676, 0),
                              (0.90077009637050098, 1.128146252166127, 0),
                              (0.943692705404238, 1.1613833261644337, 0),
                              (0.97709108330292604, 1.1730361561747586, 0),
                              (0.99894959169213471, 1.1759300874982919, 0),
                              (1.0124203987349505, 1.1760105163064269, 0),
                              (1.0275428271398932, 1.1645289800266216, 0),
                              (1.042848666622334, 1.1337546211004945, 0),
                              (1.055142468614698, 1.0758075939238765, 0),
                              (1.0585305184379035, 0.98540145004498747, 0),
                              (1.0556233679912082, 0.97374570199926891, 0),
                              (1.0492587242876892, 0.9602613226646981, 0),
                              (1.0375007181419984, 0.94568649411103145, 0),
                              (1.017827020259642, 0.93210280494582176, 0),
                              (1.0175992759724071, 0.93204300863222744, 0)]
        expected_out_times = [0,
                              1,
                              2.2000000000000002,
                              3.6400000000000001,
                              4.5040000000000004,
                              4.7632000000000003,
                              5.0742400000000005,
                              5.4474880000000008,
                              5.8953856000000009,
                              6.432862720000001,
                              7.0778352640000008,
                              7.8518023168000006,
                              8.7805627801600004,
                              9.8950753361920007,
                              10.563782869811201,
                              10.96500738998272,
                              11.446476814188543,
                              12.024240123235531,
                              12.717556094091917,
                              13.54953525911958,
                              14.547910257152775,
                              15.146935255972693,
                              15.506350255264643,
                              15.721999254839814,
                              15.980778054330019,
                              16.291312613718265,
                              16.663954084984159,
                              17.111123850503233,
                              17.647727569126122,
                              18.291652031473589,
                              19.064361386290546,
                              19.991612612070895,
                              20]
        np.testing.assert_array_almost_equal(expected_out_trace, result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times, result_tuple[1])
