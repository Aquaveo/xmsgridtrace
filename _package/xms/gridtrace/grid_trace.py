"""Trace the movement of a point through a velocity vector grid."""

# 1. Standard Python modules

# 2. Third party modules

# 3. Aquaveo modules

# 4. Local modules
from ._xmsgridtrace import gridtrace


class GridTrace(object):
    """Computes the flow trace of a point between two velocity vector time steps on a UGrid."""

    def __init__(self, ugrid=None, vector_multiplier=None, max_tracing_time=None, max_tracing_distance=None,
                 min_delta_time=None, max_change_distance=None, max_change_velocity=None,
                 max_change_direction_in_radians=None, **kwargs):
        """Constructor.

        Args:
            ugrid (UGrid): The ugrid the point is traced through
            vector_multiplier (float): Scale applied to the velocity vectors
            max_tracing_time (float): Maximum time a trace is allowed to run
            max_tracing_distance (float): Maximum distance a trace is allowed to cover
            min_delta_time (float): Minimum time between trace steps
            max_change_distance (float): Maximum distance between trace steps
            max_change_velocity (float): Maximum change in velocity between trace steps
            max_change_direction_in_radians (float): Maximum change in direction between trace steps
            **kwargs (dict): Generic keyword arguments
        """
        if 'instance' in kwargs:
            self._instance = kwargs['instance']
            return

        if ugrid is None:
            raise ValueError("ugrid is a required argument")

        self._instance = gridtrace.GridTrace(
            ugrid._instance,
            vector_multiplier=vector_multiplier,
            max_tracing_time=max_tracing_time,
            max_tracing_distance=max_tracing_distance,
            min_delta_time=min_delta_time,
            max_change_distance=max_change_distance,
            max_change_velocity=max_change_velocity,
            max_change_direction_in_radians=max_change_direction_in_radians,
        )

    def __repr__(self):
        """Returns a string representation of the tracer.

        Returns:
            str: The tracer's constraint values
        """
        return repr(self._instance)

    @property
    def vector_multiplier(self):
        """Scale applied to the velocity vectors."""
        return self._instance.vector_multiplier

    @vector_multiplier.setter
    def vector_multiplier(self, value):
        """Set the scale applied to the velocity vectors."""
        self._instance.vector_multiplier = value

    @property
    def max_tracing_time(self):
        """Maximum time a trace is allowed to run."""
        return self._instance.max_tracing_time

    @max_tracing_time.setter
    def max_tracing_time(self, value):
        """Set the maximum time a trace is allowed to run."""
        self._instance.max_tracing_time = value

    @property
    def max_tracing_distance(self):
        """Maximum distance a trace is allowed to cover."""
        return self._instance.max_tracing_distance

    @max_tracing_distance.setter
    def max_tracing_distance(self, value):
        """Set the maximum distance a trace is allowed to cover."""
        self._instance.max_tracing_distance = value

    @property
    def min_delta_time(self):
        """Minimum time between trace steps."""
        return self._instance.min_delta_time

    @min_delta_time.setter
    def min_delta_time(self, value):
        """Set the minimum time between trace steps."""
        self._instance.min_delta_time = value

    @property
    def max_change_distance(self):
        """Maximum distance between trace steps."""
        return self._instance.max_change_distance

    @max_change_distance.setter
    def max_change_distance(self, value):
        """Set the maximum distance between trace steps."""
        self._instance.max_change_distance = value

    @property
    def max_change_velocity(self):
        """Maximum change in velocity between trace steps."""
        return self._instance.max_change_velocity

    @max_change_velocity.setter
    def max_change_velocity(self, value):
        """Set the maximum change in velocity between trace steps."""
        self._instance.max_change_velocity = value

    @property
    def max_change_direction_in_radians(self):
        """Maximum change in direction between trace steps, in radians."""
        return self._instance.max_change_direction_in_radians

    @max_change_direction_in_radians.setter
    def max_change_direction_in_radians(self, value):
        """Set the maximum change in direction between trace steps, in radians."""
        self._instance.max_change_direction_in_radians = value

    def add_grid_scalars_at_time(self, scalars, scalar_loc, cell_activity, activity_loc, time):
        """Assign velocity vectors to each point or cell for a time step.

        Keeps the previous step and drops the one before that, for a maximum of two time steps.

        Args:
            scalars (iterable): The velocity vectors
            scalar_loc (str): Where the vectors are assigned. One of 'points', 'cells', or 'unknown'
            cell_activity (iterable): Whether each cell or point is active
            activity_loc (str): Where the activities are assigned. One of 'points', 'cells', or 'unknown'
            time (float): The time of the scalars
        """
        self._instance.add_grid_scalars_at_time(scalars, scalar_loc, cell_activity, activity_loc, time)

    def trace_point(self, pt, pt_time):
        """Run the grid trace for a point.

        Args:
            pt (iterable): The starting point of the trace
            pt_time (float): The starting time of the trace

        Returns:
            tuple: The resultant positions at each step and the resultant times at each step
        """
        return self._instance.trace_point(pt, pt_time)

    def get_exit_message(self):
        """Returns a message describing what caused the trace to exit.

        Returns:
            str: The exit message of the last trace_point operation
        """
        return self._instance.get_exit_message()

    def get_exit_reason(self):
        """Returns why the last trace operation ended.

        Prefer this over get_exit_message when deciding what to do with a trace; the message is for
        display. WAITING_FOR_TIME_STEP means the path stops early because the field is not known past
        the second loaded time step, not that the particle came to rest.

        Returns:
            exit_reason_enum: The exit reason of the last trace operation
        """
        return self._instance.get_exit_reason()

    def start_traces(self, pts, pt_times):
        """Begin tracing a batch of seeds against the currently loaded time steps.

        A trace runs only as far as the second loaded time step, because that is as far as the field is
        known. Supply the next time step with add_grid_scalars_at_time and call continue_traces to carry
        every unfinished trace onward::

            tracer.start_traces(seeds, seed_times)
            while tracer.continue_traces() > 0:
                step = series.next()
                if step is None:
                    break
                tracer.add_grid_scalars_at_time(*step)
            traces, times, reasons = tracer.get_trace_results()

        Stopping early is fine: traces still waiting end where they got to. One batch is in flight per
        tracer; starting a batch discards any previous one.

        Args:
            pts (iterable): The starting point of each trace
            pt_times (iterable): The starting time of each trace, one per point

        Raises:
            ValueError: If pt_times does not have one entry per point
        """
        self._instance.start_traces(pts, pt_times)

    def continue_traces(self):
        """Advance every unfinished trace as far as the loaded time steps allow.

        Releases the GIL while tracing, so calling this from a worker thread does not stall the
        interpreter.

        Returns:
            int: How many traces are waiting on a later time step. Zero means every trace has ended for
            a reason more data cannot change
        """
        return self._instance.continue_traces()

    def get_trace_results(self):
        """Return the batch traced so far.

        Valid at any point, complete once continue_traces has returned zero. An entry can hold fewer
        than two points: a seed that leaves the grid on its first step yields only the seed itself, so
        callers must not assume one usable polyline per seed.

        Returns:
            tuple: The positions of each trace, the times of each trace, and why each trace stopped as
            an exit_reason_enum. All three are parallel to the seeds passed to start_traces, and each
            entry's times are parallel to its positions
        """
        return self._instance.get_trace_results()

    def get_seed_magnitudes(self):
        """Return the speed of the field at each seed of the batch, when it was released.

        Reports the batch, exactly as get_trace_results does: empty before start_traces and after a
        refused one, and untouched by trace_point, which traces through its own state.

        Recorded when the seed is first evaluated, before the vector multiplier is applied, so it
        describes the field rather than the tracing. Two components -- the tracer is two-dimensional
        and never reads a z velocity -- so this is sqrt(vx*vx + vy*vy).

        A seed the tracer never evaluated reports the XM_NODATA sentinel, a large negative value,
        rather than 0.0. Zero is a legal speed -- a seed in still water measures it and exits
        ZERO_VELOCITY -- so the two must not share a value. The sentinel covers every unevaluated
        case alike: not started, waiting for a later time step, not traceable, and extraction failed.
        Which one it was is in get_trace_results' exit reasons.

        Not safe to call while continue_traces runs on another thread: that call releases the GIL,
        and this one reads the batch it is writing.

        Returns:
            Sequence[float]: One speed per seed, parallel to the seeds passed to start_traces and to
            everything get_trace_results returns
        """
        return self._instance.get_seed_magnitudes()

    def sample_vectors(self, pts, time):
        """Return the field's vector at each of the given points that has one, without tracing.

        The interpolation a trace performs at its first step, exposed on its own. The case this
        exists for is a display that draws a glyph per position on a moving set of positions:
        the field has to be resampled wherever they land, whether or not those glyphs go on to
        follow the flow.

        Only the points that resolved come back, each beside its own vector, so the caller gets
        position-paired output and never handles a no-data marker. A set of positions covering a
        view is mostly outside the grid or over inactive cells. Both arrays follow the order
        given, so a caller that wants to know which of its points missed can walk its own list
        and the returned points in step.

        The points kept are exactly the seeds a trace will accept: both this and start_traces
        test the same interpolation for no data, so a point dropped here is one that would have
        exited SEED_NOT_TRACEABLE with an empty path. Following the flow from these points adds
        no further filtering.

        Reads the tracer's loaded time steps and touches none of its tracing state: safe to call
        before start_traces, between batches, or never having traced at all, and it neither
        starts nor disturbs a batch. Two time steps must be loaded, as for any trace; with fewer
        both arrays come back empty.

        The vector multiplier is not applied, matching get_seed_magnitudes: this describes the
        field, not how far the tracer would step through it.

        One tracer serves one thread at a time. The point-location search writes scratch held on
        the tracer, so two threads sampling the same tracer race on it -- and so does a sample
        running alongside continue_traces, which releases the GIL for the same reason this does.
        Give each thread its own tracer, or serialize the calls.

        Args:
            pts (iterable): The points to sample, in grid coordinates
            time (float): The time to sample at, interpolated between the loaded time steps. A
                time outside them is clamped to the nearer one, which is where a trace given the
                same time comes to rest

        Returns:
            tuple(numpy.ndarray, numpy.ndarray): The points that had a value, shape (N, 3),
            echoed from the input unchanged and in order; and the field at each, shape (N, 3),
            parallel to them. Every vector's z is zero -- the tracer is two-dimensional and
            never reads a z velocity
        """
        return self._instance.sample_vectors(pts, time)
