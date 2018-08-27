"""Test XmUGrid2dDataExtractor_py.cpp"""
import unittest
import xmsgridtrace_py
from xmsgrid_py.ugrid import XmUGrid
from xmsgridtrace_py.gridtrace import XmGridTrace
from xmsgridtrace_py.gridtrace import data_location_enum
import numpy as np

class TestXmGridTrace(unittest.TestCase):
    """XmGridTrace tests"""
    def create_default_single_cell(self):
        """Create a default single cell"""
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2,
                 XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)
        tracer = XmGridTrace(ugrid)
        self.assertIsInstance(tracer,XmGridTrace)
        tracer.set_vector_multiplier(1)
        tracer.set_max_tracing_time(100)
        tracer.set_max_tracing_distance(100)
        tracer.set_min_delta_time(.1)
        tracer.set_max_change_distance(100)
        tracer.set_max_change_velocity(100)
        tracer.set_max_change_direction_in_radians(1.5*np.pi)
        scalars = [(1,1,0),(1,1,0),(1,1,0),(1,1,0)]
        point_activity = [True]*4
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,10)
        return tracer
    def create_default_two_cell(self):
        """Create a default two cell"""
        points = [ ( 0, 0, 0 ),( 1, 0, 0 ),( 1, 1, 0 ),( 0, 1, 0 ),( 2, 0, 0 ),( 2, 1, 0 ) ]
        cells = [ XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3, XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 4, 5, 2 ]
        ugrid = XmUGrid(points,cells)
        tracer = XmGridTrace(ugrid)
        self.assertIsInstance(tracer,XmGridTrace)
        tracer.set_vector_multiplier(1)
        tracer.set_max_tracing_time(100)
        tracer.set_max_tracing_distance(100)
        tracer.set_min_delta_time(.1)
        tracer.set_max_change_distance(100)
        tracer.set_max_change_velocity(100)
        tracer.set_max_change_direction_in_radians(1.5*np.pi)
        scalars = [ ( .1,0,0 ),( .2,0,0 ) ]
        point_activity = [True]*2
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_CELLS,point_activity,data_location_enum.LOC_CELLS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_CELLS,point_activity,data_location_enum.LOC_CELLS,10)
        return tracer
    def test_basic_trace_point(self):
        """test basic tracing functionality"""
        tracer=self.create_default_single_cell()
        start_time=.5

        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = [(.5,.5,0),(1,1,0)]
        expected_out_times = [.5,1]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_max_change_distance(self):
        """test max change distance functionality"""
        tracer=self.create_default_single_cell()
        start_time=.5
        tracer.set_max_change_distance(.25)

        result_tuple = tracer.trace_point((.5,.5,0),start_time)

        expected_out_trace =     [(.5,.5,0 ),
                             ( 0.67677668424809445, 0.67677668424809445, 0.00000000000000000 ),
                             ( 0.85355336849618890, 0.85355336849618890, 0.00000000000000000 ),
                             (1,1,0)]
        expected_out_times = [.5, 0.67677668424809445, 0.85355336849618890, 1]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_small_scalars_trace_point(self):
        """test functionality with small scalars"""
        tracer=self.create_default_single_cell()
        start_time=.5
        tracer.set_max_change_distance(.25)
        scalars = [(.1,.1,0),(.1,.1,0),(.1,.1,0),(.1,.1,0)]
        point_activity = [True]*4
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,10)

        result_tuple = tracer.trace_point((.5,.5,0),start_time)

        expected_out_trace =     [(.5,.5,0 ),
                             ( 0.60000000149011612, 0.60000000149011612, 0  ),
                             ( 0.72000000327825542, 0.72000000327825542, 0 ),
                             ( 0.86400000542402267, 0.86400000542402267, 0  ),
                             (1,1,0)]
        expected_out_times = [.5, 1.5, 2.7, 4.14, 5.5]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_almost_equal(expected_out_times,result_tuple[1])
    def test_strong_direction_change(self):
        """test functionality with strong changes in direction"""
        tracer=self.create_default_single_cell()
        tracer.set_max_change_direction_in_radians(np.pi*.2)
        tracer.set_min_delta_time(-1)
        start_time=.5

        scalars = [(0,1,0),(-1,0,0),(0,-1,0),(1,0,0)]
        point_activity = [True]*4
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,10)

        result_tuple = tracer.trace_point((0,0,0),start_time)

        expected_out_trace =     [  ( 0,0,0 ),
                                    (0.00000000000000000, 0.25000000000000000, 0.00000000000000000 ),
                                    (0.074999999999999997, 0.47499999999999998, 0.00000000000000000 ),
                                    (0.21900000214576720, 0.63699999570846555, 0.00000000000000000 ),
                                    (0.30928799843788146, 0.66810399758815764, 0.00000000000000000 ),
                                    (0.40229310507774352, 0.67396399235725402, 0.00000000000000000 ),
                                    (0.48679361495018003, 0.65024498560905453, 0.00000000000000000 ),
                                    (0.54780151323509219, 0.59909560095787040, 0.00000000000000000 ),
                                    (0.55928876277122497, 0.56619817004051198, 0.00000000000000000 ),
                                    (0.56114558691518779, 0.53247499044700608, 0.00000000000000000 ),
                                    (0.55189971330840681, 0.50228363992173752, 0.00000000000000000 ),
                                    (0.53269911067322617, 0.48131557500677169, 0.00000000000000000 ),
                                    (0.52076836142536975, 0.47806150355091476, 0.00000000000000000 ),
                                    (0.50886902895577013, 0.47838753608466128, 0.00000000000000000 ),
                                    (0.49867742691962913, 0.48264835153512164, 0.00000000000000000 ),
                                    (0.49224616907898289, 0.49014090685121131, 0.00000000000000000 ),
                                    (0.49173935940609609, 0.49438094923206660, 0.00000000000000000 ),
                                    (0.49250246625151450, 0.49839053740482164, 0.00000000000000000 ),
                                    (0.49454361321306389, 0.50154755045413602, 0.00000000000000000 ),
                                    (0.49745717820065949, 0.50317358562752867, 0.00000000000000000 ),
                                    (0.49888395770889871, 0.50301615091938545, 0.00000000000000000 ),
                                    (0.50012160117661586, 0.50244704462921241, 0.00000000000000000 ),
                                    (0.50095740046883197, 0.50152383477622209, 0.00000000000000000 ),
                                    (0.50107955145675120, 0.50098875888952354, 0.00000000000000000 ),
                                    (0.50105605626599747, 0.50045352403892940, 0.00000000000000000 ),
                                    (0.50086894918345870, 0.49998474718699493, 0.00000000000000000 ),
                                    (0.50053945884260675, 0.49966662451478433, 0.00000000000000000 ),
                                    (0.50034430627617277, 0.49962054739305783, 0.00000000000000000 ),
                                    (0.50015012042108842, 0.49962997721910873, 0.00000000000000000 ),
                                    (0.49998265395837810, 0.49970077747304897, 0.00000000000000000 ),
                                    (0.49987374966305814, 0.49982308521808211, 0.00000000000000000 ),
                                    (0.49986302487024292, 0.49988726006383088, 0.00000000000000000 ),
                                    (0.49986815504448728, 0.49994012045071656, 0.00000000000000000 )]
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
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_max_tracing_time(self):
        """test functionality of max tracing time"""
        tracer=self.create_default_single_cell()
        tracer.set_max_change_direction_in_radians(np.pi*.2)
        tracer.set_min_delta_time(-1)
        tracer.set_max_tracing_time(5)

        start_time=.5
        scalars = [(0,1,0),(-1,0,0),(0,-1,0),(1,0,0)]
        point_activity = [True]*4
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,10)

        result_tuple = tracer.trace_point((0,0,0),start_time)

        expected_out_trace =[( 0,0,0 ),
                            (0.00000000000000000, 0.25000000000000000, 0.00000000000000000 ),
                            (0.074999999999999997, 0.47499999999999998, 0.00000000000000000 ),
                            (0.21900000214576720, 0.63699999570846555, 0.00000000000000000 ),
                            (0.30928799843788146, 0.66810399758815764, 0.00000000000000000 ),
                            (0.40229310507774352, 0.67396399235725402, 0.00000000000000000 ),
                            (0.48679361495018003, 0.65024498560905453, 0.00000000000000000 ),
                            (0.54780151323509219, 0.59909560095787040, 0.00000000000000000 ),
                            (0.55928876277122497, 0.56619817004051198, 0.00000000000000000 ),
                            (0.56114558691518779, 0.53247499044700608, 0.00000000000000000 ),
                            (0.55189971330840681, 0.50228363992173752, 0.00000000000000000 ),
                            (0.53269911067322617, 0.48131557500677169, 0.00000000000000000 ),
                            (0.52076836142536975, 0.47806150355091476, 0.00000000000000000 ),
                            (0.50886902895577013, 0.47838753608466128, 0.00000000000000000 ),
                            (0.49867742691962913, 0.48264835153512164, 0.00000000000000000 ),
                            (0.49224616907898289, 0.49014090685121131, 0.00000000000000000 ),
                            (0.49173935940609609, 0.49438094923206660, 0.00000000000000000 ),
                            (0.49237657318600692, 0.49772905815126539, 0.00000000000000000 )]
        expected_out_times =    [.5,
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
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_max_tracing_distance(self):
        """test functionality of max tracing distance"""
        tracer=self.create_default_single_cell()
        tracer.set_max_change_direction_in_radians(np.pi*.2)
        tracer.set_min_delta_time(-1)
        tracer.set_max_tracing_distance(1.0)
        start_time=.5

        scalars = [(0,1,0),(-1,0,0),(0,-1,0),(1,0,0)]
        point_activity = [True]*4
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,10)

        result_tuple = tracer.trace_point((0,0,0),start_time)


        expected_out_trace =     [  ( 0,0,0 ),
                                    ( 0.00000000000000000, 0.25000000000000000, 0.00000000000000000 ),
                                    ( 0.074999999999999997, 0.47499999999999998, 0.00000000000000000 ),
                                    ( 0.21900000214576720, 0.63699999570846555, 0.00000000000000000 ),
                                    ( 0.30928799843788146, 0.66810399758815764, 0.00000000000000000 ),
                                    ( 0.40229310507774352, 0.67396399235725402, 0.00000000000000000 ),
                                    ( 0.48679361495018003, 0.65024498560905453, 0.00000000000000000 ),
                                    ( 0.50183556502673621, 0.63763372523131490, 0.00000000000000000 )]
        expected_out_times = [  .5,
                                0.75000000000000000,
                                1.0500000000000000,
                                1.4100000000000001,
                                1.6260000000000001,
                                1.8852000000000002,
                                2.1962400000000004,
                                2.4774609356360582]
        print(len(result_tuple[0]))
        print(" Length of Expected:")
        print(len(expected_out_trace))
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0],6)
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])    
    def test_start_out_of_cell(self):
        """test functionality of starting outside of cell"""
        tracer=self.create_default_single_cell()
        start_time=.5

        result_tuple = tracer.trace_point((-1,0,0),start_time)
        
        expected_out_trace = []
        expected_out_times = []
        np.testing.assert_equal(0,len(result_tuple[0]))
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_beyond_timestep(self):
        """test functionality of starting beyond the time step"""
        tracer=self.create_default_single_cell()
        start_time=10.1

        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = []
        expected_out_times = []
        np.testing.assert_equal(0,len(result_tuple[0]))
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_before_timestep(self):
        """test functionality of starting before the time step"""
        tracer=self.create_default_single_cell()
        start_time=-0.1

        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = [ ( .5,.5,0 ), (1,1,0)]
        expected_out_times = [-.1,.4]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_vector_multiplier(self):
        """test functionality of vector multiplier"""
        tracer=self.create_default_single_cell()
        tracer.set_max_change_direction_in_radians(np.pi*.2)
        tracer.set_min_delta_time(-1)
        tracer.set_vector_multiplier(0.5)
        start_time=.5

        scalars = [(0,1,0),(-1,0,0),(0,-1,0),(1,0,0)]
        point_activity = [True]*4
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,10)

        result_tuple = tracer.trace_point((0,0,0),start_time)

        expected_out_trace =     [( 0,0,0 ),
                                (0.00000000000000000, 0.25000000000000000, 0.00000000000000000 ),
                                (0.074999999999999997, 0.47499999999999998, 0.00000000000000000 ),
                                (0.21900000214576720, 0.63699999570846555, 0.00000000000000000 ),
                                (0.30928799843788146, 0.66810399758815764, 0.00000000000000000 ),
                                (0.40229310507774352, 0.67396399235725402, 0.00000000000000000 ),
                                (0.48679361495018003, 0.65024498560905453, 0.00000000000000000 ),
                                (0.54780151323509219, 0.59909560095787040, 0.00000000000000000 ),
                                (0.55928876277122497, 0.56619817004051198, 0.00000000000000000 ),
                                (0.56114558691518779, 0.53247499044700608, 0.00000000000000000 ),
                                (0.55189971330840681, 0.50228363992173752, 0.00000000000000000 ),
                                (0.53269911067322617, 0.48131557500677169, 0.00000000000000000 ),
                                (0.52076836142536975, 0.47806150355091476, 0.00000000000000000 ),
                                (0.50886902895577013, 0.47838753608466128, 0.00000000000000000 ),
                                (0.49867742691962913, 0.48264835153512164, 0.00000000000000000 ),
                                (0.49224616907898289, 0.49014090685121131, 0.00000000000000000 ),
                                (0.49175783605462037, 0.49422637094165467, 0.00000000000000000 )]
        expected_out_times = [  .5,
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
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])    
    def test_multi_cell(self):
        """test default functionality of multiple cells"""
        tracer=self.create_default_two_cell()
        start_time=0

        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = [    ( .5,.5,0 ),
                                (0.60000000149011612, 0.50000000000000000, 0.00000000000000000 ), 
                                (0.73200000077486038, 0.50000000000000000, 0.00000000000000000 ), 
                                (0.90940801054239273, 0.50000000000000000, 0.00000000000000000 ), 
                                (1.1529537134766579, 0.50000000000000000, 0.00000000000000000 ), 
                                (1.4957102079987525, 0.50000000000000000, 0.00000000000000000 ), 
                                (1.9923067892670629, 0.50000000000000000, 0.00000000000000000 ),
                                (2,.5,0)]
        expected_out_times = [  0,
                                1.0000000000000000, 
                                2.2000000000000002, 
                                3.6400000000000001, 
                                5.3680000000000003, 
                                7.4416000000000002, 
                                9.9299199999999992,
                                9.9683860530914945]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_max_change_velocity(self):
        """test functionality of max change in velocity"""
        tracer=self.create_default_two_cell()
        tracer.set_max_change_velocity(.01)
        tracer.set_min_delta_time(.001)
        start_time=0

        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = [  (.5,.5,0 ),
                                (0.60000000149011612, 0.50000000000000000, 0.00000000000000000 ),
                                (0.66600000113248825, 0.50000000000000000, 0.00000000000000000 ),
                                (0.74995200067758561, 0.50000000000000000, 0.00000000000000000 ),
                                (0.80394992786645891, 0.50000000000000000, 0.00000000000000000 ),
                                (0.87154669338464741, 0.50000000000000000, 0.00000000000000000 ),
                                (0.95686786960840231, 0.50000000000000000, 0.00000000000000000 ),
                                (1.0112451727318765, 0.50000000000000000, 0.00000000000000000 ),
                                (1.0789334834771158, 0.50000000000000000, 0.00000000000000000 ),
                                (1.1637975516948893, 0.50000000000000000, 0.00000000000000000 ),
                                (1.2174527417415202, 0.50000000000000000, 0.00000000000000000 ),
                                (1.2839153379250163, 0.50000000000000000, 0.00000000000000000 ),
                                (1.3667568384715398, 0.50000000000000000, 0.00000000000000000 ),
                                (1.4187699365302351, 0.50000000000000000, 0.00000000000000000 ),
                                (1.4829247317645506, 0.50000000000000000, 0.00000000000000000 ),
                                (1.5624845364724593, 0.50000000000000000, 0.00000000000000000 ),
                                (1.6587784227485147, 0.50000000000000000, 0.00000000000000000 ),
                                (1.7743310862797812, 0.50000000000000000, 0.00000000000000000 ),
                                (1.9129942825173010, 0.50000000000000000, 0.00000000000000000 ),
                                (2,.5,0)]
        expected_out_times = [  0,
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
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_unique_time_steps(self):
        """test functionality of unique time steps"""
        tracer=self.create_default_two_cell()
        start_time=10

        
        scalars = [(.2,0,0),(.3,0,0)]
        point_activity = [True]*2
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_CELLS,point_activity,data_location_enum.LOC_CELLS,20)


        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = [  ( .5,.5,0 ),
                                (0.70000000298023224, 0.50000000000000000, 0.00000000000000000 ),
                                (0.95200000226497650, 0.50000000000000000, 0.00000000000000000 ),
                                (1.2734079944372176, 0.50000000000000000, 0.00000000000000000 ),
                                (1.6897536998434066, 0.50000000000000000, 0.00000000000000000 ),
                                (2,.5,0)]
        expected_out_times = [  10,
                                11.000000000000000,
                                12.199999999999999,
                                13.640000000000001,
                                15.368000000000000,
                                16.627525378316030]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_inactive_cell(self):
        """test functionality of inactive cells"""
        tracer=self.create_default_two_cell()
        start_time=10

        
        scalars = [(.2,0,0),(99999,0,0)]
        point_activity = [True]*2
        point_activity[1]=False
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_CELLS,point_activity,data_location_enum.LOC_CELLS,20)


        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = [  ( .5,.5,0 ),
                                (0.70000000298023224, 0.50000000000000000, 0.00000000000000000 ),
                                (0.93040000677108770, 0.50000000000000000, 0.00000000000000000 ),
                                (0.99788877571821222, 0.50000000000000000, 0.00000000000000000 )]
        expected_out_times = [  10,
                                11.000000000000000,
                                12.199999999999999,
                                12.560000000000000]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_start_inactive_cell(self):
        """test functionality of starting in an inactive cell"""
        tracer=self.create_default_two_cell()
        start_time=10

        
        scalars = [(.2,0,0),(99999,0,0)]
        point_activity = [True]*2
        point_activity[0]=False
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_CELLS,point_activity,data_location_enum.LOC_CELLS,20)


        result_tuple = tracer.trace_point((.5,.5,0),start_time)
        
        expected_out_trace = []
        expected_out_times = []
        np.testing.assert_equal(0,len(result_tuple[0]))
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
    def test_tutorial(self):
        """A test to serve as a tutorial"""
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
        points = [ ( 0, 0, 0 ),( 1, 0, 0 ),( 2, 0, 0 ),
      ( 0, 1, 0 ),( 1, 1, 0 ),( 2, 1, 0 ),
      ( 0, 2, 0 ),( 1, 2, 0 ),( 2, 2, 0 )]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 4, 3,
                XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 2, 5, 4,
                XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 3, 4, 7, 6,
                XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 4, 5, 8, 7]
        ugrid = XmUGrid(points, cells)
        # Step 2: Create the tracer from the grid
        tracer = XmGridTrace(ugrid)

        # Step 3: Set up the constraints on the tracer
        self.assertIsInstance(tracer,XmGridTrace)
        tracer.set_vector_multiplier(2)
        tracer.set_max_tracing_time(-1)
        tracer.set_max_tracing_distance(-1)
        tracer.set_min_delta_time(.01)
        tracer.set_max_change_distance(-1)
        tracer.set_max_change_velocity(-1)
        tracer.set_max_change_direction_in_radians(.25*np.pi)
        # Step 4: Set up the velocity vectors for both time steps. Insert timesteps sequentially
        # For this case Scalars are set such that they circle around the edge of the graph in a clockwise direction
        # Z component is not used in scalars
        scalars = [(  0,1,0 ),( -.1,0,0 ),( -1,0,0 ),
                    ( 0,.1,0 ),( 0,0,0 ),( 0,-.1,0 ),
                    ( 1,0,0 ),( .1,0,0 ),( 0,-1,0 )]
        point_activity = [True]*9
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,0)
        # For the second timestep scalars are doubled to indicate an increase in magnitude
        scalars = [(  0,2,0 ),( -.2,0,0 ),( -2,0,0 ),
                    ( 0,.2,0 ),( 0,0,0 ),( 0,-.2,0 ),
                    ( 2,0,0 ),( .2,0,0 ),( 0,-2,0 )]
        tracer.add_grid_scalars_at_time(scalars,data_location_enum.LOC_POINTS,point_activity,data_location_enum.LOC_POINTS,20)
    
        start_time=0
        start_point = (.5,.5,0)
        result_tuple=tracer.trace_point(start_point,start_time)
        # show the cause for termination by calling get_exit_message
        print(tracer.get_exit_message())

        # Expected values for this simulation
        expected_out_trace = [   (0.50000000000000000, 0.50000000000000000, 0.00000000000000000 ),
                                (0.50000000000000000, 1.2500000000000000, 0.00000000000000000 ),
                                (0.54457812566426578, 1.3391562513285316, 0.00000000000000000 ),
                                (0.61632493250262921, 1.4354984729093498, 0.00000000000000000 ),
                                (0.72535406450374607, 1.5315533661126233, 0.00000000000000000 ),
                                (0.88236797164001590, 1.6126801842666139, 0.00000000000000000 ),
                                (0.98873181403598276, 1.6331015959080102, 0.00000000000000000 ),
                                (1.0538503898747653, 1.6342606013582104, 0.00000000000000000 ),
                                (1.1249433009705341, 1.5683006835455087, 0.00000000000000000 ),
                                (1.1895097427498795, 1.3863448896225066, 0.00000000000000000 ),
                                (1.2235242118635632, 1.0588590059131318, 0.00000000000000000 ),
                                (1.2235242118635632, 0.90477286425654002, 0.00000000000000000 ),
                                (1.2005336220528682, 0.85080764250970042, 0.00000000000000000 ),
                                (1.1581790674742278, 0.79387770198395835, 0.00000000000000000 ),
                                (1.0896874578697060, 0.74131697161132859, 0.00000000000000000 ),
                                (0.98966250551038770, 0.70663752692174131, 0.00000000000000000 ),
                                (0.95806149614159530, 0.71817980325332686, 0.00000000000000000 ),
                                (0.92629620502521459, 0.77371504022050730, 0.00000000000000000 ),
                                (0.90239412753251202, 0.88917318465162865, 0.00000000000000000 ),
                                (0.89995172701803572, 1.0694875660697027, 0.00000000000000000 ),
                                (0.91503139037776327, 1.0911992829869794, 0.00000000000000000 ),
                                (0.93816744602651825, 1.1127546977629765, 0.00000000000000000 ),
                                (0.97140028507849163, 1.1309789606067331, 0.00000000000000000 ),
                                (0.99364912627842006, 1.1358370729524059, 0.00000000000000000 ),
                                (1.0071524474802995, 1.1364684019706512, 0.00000000000000000 ),
                                (1.0223447138862345, 1.1280655805979485, 0.00000000000000000 ),
                                (1.0369737821057583, 1.0971462034407997, 0.00000000000000000 ),
                                (1.0467397711865176, 1.0371377237101163, 0.00000000000000000 ),
                                (1.0467397711865176, 0.96499504248441559, 0.00000000000000000 ),
                                (1.0390576209755447, 0.95473758230148376, 0.00000000000000000 ),
                                (1.0276444556154691, 0.94488898976070590, 0.00000000000000000 ),
                                (1.0208791233912420, 0.94149540451099356, 0.00000000000000000 )]
        expected_out_times = [    0.00000000000000000,
                                0.37500000000000000,
                                0.82499999999999996,
                                1.3649999999999998,
                                2.0129999999999999,
                                2.7905999999999995,
                                3.2571599999999994,
                                3.5370959999999991,
                                3.8730191999999990,
                                4.2761270399999987,
                                4.7598564479999981,
                                5.3403317375999979,
                                6.0369020851199977,
                                6.8727865021439971,
                                7.8758478025727969,
                                9.0795213630873555,
                                9.4406234312417237,
                                9.8739459130269651,
                                10.393932891169255,
                                11.017917264940003,
                                11.766698513464901,
                                12.665236011694777,
                                13.743481009570628,
                                14.390428008296139,
                                14.778596207531445,
                                15.244398046613812,
                                15.803360253512654,
                                16.474114901791264,
                                17.279020479725595,
                                18.244907173246794,
                                19.403971205472232,
                                20.000000000000000]
        np.testing.assert_array_almost_equal(expected_out_trace,result_tuple[0])
        np.testing.assert_array_equal(expected_out_times,result_tuple[1])
