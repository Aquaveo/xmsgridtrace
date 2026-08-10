"""Trace the movement of a point through a velocity vector grid."""
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
