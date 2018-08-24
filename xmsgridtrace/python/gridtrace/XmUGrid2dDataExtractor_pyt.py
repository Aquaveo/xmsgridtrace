"""Test XmUGrid2dDataExtractor_py.cpp"""
import unittest
import xmsextractor_py
from xmsgrid_py.ugrid import XmUGrid
from xmsextractor_py.extractor import XmUGrid2dDataExtractor
from xmsextractor_py.extractor import data_location_enum
import numpy as np

class TestXmUGrid2dDataExtractor(unittest.TestCase):
    """XmUGrid2dDataExtractor tests"""

    def test_point_scalars_only(self):
        """Test extractor with point scalars only."""
        #  3----2
        #  | 1 /|
        #  |  / |
        #  | /  |
        #  |/ 0 |
        #  0----1

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2,
                 XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)
        extractor.set_no_data_value(-999.0)

        point_scalars = np.array((1, 2, 3, 2))
        extractor.set_grid_point_scalars(point_scalars, [], data_location_enum.LOC_POINTS)
        extract_locations = [(0, 0, 0), (0.25, 0.75, 100), (0.5, 0.5, 0), (0.75, 0.25, -100), (-1, -1, 0)]
        extractor.set_extract_locations(extract_locations)

        interp_values = extractor.extract_data()
        expected = [1, 2, 2, 2, -999]
        np.testing.assert_array_equal(expected, interp_values)

    def test_point_scalar_cell_activity(self):
        """Test extractor when using point scalars and cell activity."""
        #  3----2
        #  | 1 /|
        #  |  / |
        #  | /  |
        #  |/ 0 |
        #  0----1
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2, XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        point_scalars = [1, 2, 3, 2]
        cell_activity = [True, False]
        extractor.set_grid_point_scalars(point_scalars, cell_activity, data_location_enum.LOC_CELLS)
        extractor.set_extract_locations([(0.25, 0.75, 100.0), (0.75, 0.25, -100.0), (-1.0, -1.0, 0.0)])

        interp_values = extractor.extract_data()
        expected = [-9999999.0, 2.0, -9999999.0]
        np.testing.assert_array_equal(expected, interp_values)

    def test_point_scalar_point_activity(self):
        """Test extractor when using point scalars and point activity."""
        #  6----7---------8 point row 3
        #  |   / \       /|
        #  |  /   \     / |
        #  | /     \   /  |
        #  |/       \ /   |
        #  3---------4----5 point row 2
        #  |\       / \   |
        #  | \     /   \  |
        #  |  \   /     \ |
        #  |   \ /       \|
        #  0----1---------2 point row 1
        #
        points = [
            (0, 0, 0), (1, 0, 0), (3, 0, 0),  # row 1 of points
            (0, 1, 0), (2, 1, 0), (3, 1, 0),  # row 2 of points
            (0, 2, 0), (1, 2, 0), (3, 2, 0)]  # row 3 of points
        cells = [
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 3, # row 1 of triangles
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 1, 4, 3,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 1, 2, 4,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 5, 4,

            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 3, 7, 6, # row 2 of triangles
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 3, 4, 7,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 4, 8, 7,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 4, 5, 8]

        point_scalars = [
        0, 0, 0,  # row 1
        1, 1, 1,  # row 2
        2, 2, 2] # row 3

        # extract value for each cell
        extract_locations = [
            (0.25, 0.25, 0), # cell 0
            (1.00, 0.25, 0), # cell 1
            (2.00, 0.50, 0), # cell 2
            (2.75, 0.75, 0), # cell 3
            (0.25, 1.75, 0), # cell 4
            (1.00, 1.25, 0), # cell 5
            (1.50, 1.75, 0), # cell 6
            (2.75, 1.25, 0)] # cell 7

        # expected results with point 4 inactive
        expected_interp_values = [
        0.25, -9999999.0, -9999999.0, -9999999.0,
        1.75, -9999999.0, -9999999.0, -9999999.0 ]

        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        # set point 4 inactive
        # should cause all cells connected to point 4 to return -9999999.0
        pointActivity = [True]*9
        pointActivity[4] = False
        extractor.set_grid_point_scalars(point_scalars, pointActivity, data_location_enum.LOC_POINTS)

        # extract interpolated scalar for each cell
        extractor.set_extract_locations(extract_locations)

        interp_values = extractor.extract_data()
        np.testing.assert_array_equal(expected_interp_values, interp_values)

    def test_invalid_point_scalars_and_activity_size(self):
        """Test when scalar and activity arrays are sized incorrectly."""
        #  3----2
        #  | 1 /|
        #  |  / |
        #  | /  |
        #  |/ 0 |
        #  0----1
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2, XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        point_scalars = [1, 2, 3]
        activity = [True, False]
        extractor.set_grid_point_scalars(point_scalars, activity, data_location_enum.LOC_POINTS)

        extract_locations = [(0.25, 0.75, 100.0), (0.75, 0.25, 0.0)]
        extractor.set_extract_locations(extract_locations)

        interp_values = extractor.extract_data()
        expected = [1.0, -9999999.0]
        np.testing.assert_array_equal(expected, interp_values)

    def test_cell_scalars_only(self):
        """Test extractor with cell scalars only."""
        # \verbatim
        #  3----2
        #  | 1 /|
        #  |  / |
        #  | /  |
        #  |/ 0 |
        #  0----1
        # \endverbatim
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2, XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)

        # Step 1. Create an extractor for an existing XmUGrid (call xms::XmUGrid2dDataExtractor).
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        # Step 2. Set scalar and activity values (call xms::XmUGrid2dDataExtractor::set_grid_cell_scalars or XmUGrid2dDataExtractor::set_point_cell_scalars).
        cellScalars = [1, 2]
        extractor.set_grid_cell_scalars(cellScalars, [], data_location_enum.LOC_CELLS)

        # Step 3. Set extract locations (call XmUGrid2dDataExtractor::set_extract_locations).
        extractor.set_extract_locations([(0.0, 0.0, 0.0),
                                        (0.25, 0.75, 100.0),
                                        (0.5, 0.5, 0.0),
                                        (0.75, 0.25, -100.0),
                                        (-0.1, -0.1, 0.0)])

        # Step 4. Extract the data (call xms::XmUGrid2dDataExtractor::extract_data).
        interp_values = extractor.extract_data()
        expected = [1.5, 2.0, 1.5, 1.0, -9999999.0]
        np.testing.assert_array_equal(expected, interp_values)

    def test_cell_scalar_cell_activity(self):
        """Test extractor when using cell scalars and cell activity."""
        #  6----7---------8 point row 3
        #  | 4 / \       /|
        #  |  /   \  6  / |
        #  | /  5  \   /  |
        #  |/       \ / 7 |
        #  3---------4----5 point row 2
        #  |\       / \ 3 |
        #  | \  1  /   \  |
        #  |  \   /     \ |
        #  | 0 \ /   2   \|
        #  0----1---------2 point row 1
        #
        points = [
            (0, 0, 0), (1, 0, 0), (3, 0, 0),  # row 1 of points
            (0, 1, 0), (2, 1, 0), (3, 1, 0),  # row 2 of points
            (0, 2, 0), (1, 2, 0), (3, 2, 0)]  # row 3 of points
        cells = [
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 3, # row 1 of triangles
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 1, 4, 3,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 1, 2, 4,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 5, 4,

            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 3, 7, 6, # row 2 of triangles
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 3, 4, 7,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 4, 8, 7,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 4, 5, 8]

        cellScalars = [
            2, 4, 6, 8,  # row 1
            4, 6, 8, 10  # row 2
        ]

        # extract value for each cell
        extract_locations = [
            (0.25, 0.25, 0), # cell 0
            (1.00, 0.25, 0), # cell 1
            (2.00, 0.50, 0), # cell 2
            (2.75, 0.75, 0), # cell 3
            (0.25, 1.75, 0), # cell 4
            (1.00, 1.25, 0), # cell 5
            (1.50, 1.75, 0), # cell 6
            (2.75, 1.25, 0)  # cell 7
        ]

        # expected results with point 4 inactive
        expected_interp_values = [
            -9999999.0, 4.0000, -9999999.0, 8.2500, # row 1 cells
            -9999999.0, 6.0000, -9999999.0, 9.750   # row 2 cells
        ]

        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        # set point 4 inactive
        # should cause all cells connected to point 4 to return -9999999.0
        cell_activity = [True]*8
        cell_activity[0] = False
        cell_activity[2] = False
        cell_activity[4] = False
        cell_activity[6] = False
        extractor.set_grid_cell_scalars(cellScalars, cell_activity, data_location_enum.LOC_CELLS)

        # extract interpolated scalar for each cell
        extractor.set_extract_locations(extract_locations)

        interp_values = extractor.extract_data()
        np.testing.assert_array_equal(expected_interp_values, interp_values)

    def test_cell_scalar_cell_activity_idw(self):
        """Test extractor when using cell scalars and cell activity."""
        #  6----7---------8 point row 3
        #  | 4 / \       /|
        #  |  /   \  6  / |
        #  | /  5  \   /  |
        #  |/       \ / 7 |
        #  3---------4----5 point row 2
        #  |\       / \ 3 |
        #  | \  1  /   \  |
        #  |  \   /     \ |
        #  | 0 \ /   2   \|
        #  0----1---------2 point row 1
        #
        points = [
            (0, 0, 0), (1, 0, 0), (3, 0, 0),  # row 1 of points
            (0, 1, 0), (2, 1, 0), (3, 1, 0),  # row 2 of points
            (0, 2, 0), (1, 2, 0), (3, 2, 0)]  # row 3 of points
        cells = [
        XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 3, # row 1 of triangles
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 1, 4, 3,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 1, 2, 4,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 5, 4,

            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 3, 7, 6, # row 2 of triangles
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 3, 4, 7,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 4, 8, 7,
            XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 4, 5, 8]

        cellScalars = [
            2, 4, 6, 8,  # row 1
            4, 6, 8, 10  # row 2
        ]

        # extract value for each cell
        extract_locations = [
            (0.25, 0.25, 0), # cell 0
            (1.00, 0.25, 0), # cell 1
            (2.00, 0.50, 0), # cell 2
            (2.75, 0.75, 0), # cell 3
            (0.25, 1.75, 0), # cell 4
            (1.00, 1.25, 0), # cell 5
            (1.50, 1.75, 0), # cell 6
            (2.75, 1.25, 0)  # cell 7
        ]

        # expected results with point 4 inactive
        expected_interp_values = [
            2.0, 3.4444, -9999999.0, 6.75, # row 1 cells
            3.5, 5.7303, 5.4652, 8.25     # row 2 cells
        ]

        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)
        extractor.set_use_idw_for_point_data(True)

        # set point 4 inactive
        # should cause all cells connected to point 4 to return -9999999.0
        cell_activity = [True]*8
        cell_activity[2] = False
        extractor.set_grid_cell_scalars(cellScalars, cell_activity, data_location_enum.LOC_CELLS)

        # extract interpolated scalar for each cell
        extractor.set_extract_locations(extract_locations)

        interp_values = extractor.extract_data()
        np.testing.assert_allclose(expected_interp_values, interp_values, atol=0.001)

    def test_cell_scalar_point_activity(self):
        """Test extractor when using point scalars and point activity."""
        #  3----2
        #  | 1 /|
        #  |  / |
        #  | /  |
        #  |/ 0 |
        #  0----1
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2, XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        pointActivity = [True]*4
        pointActivity[1] = False
        cellScalars = [1, 2]
        extractor.set_grid_cell_scalars(cellScalars, pointActivity, data_location_enum.LOC_POINTS)
        extractor.set_extract_locations([(0.0, 0.0, 0.0),
                                        (0.25, 0.75, 100.0),
                                        (0.5, 0.5, 0.0),
                                        (0.75, 0.25, -100.0),
                                        (-1.0, -1.0, 0.0)])

        interp_values = extractor.extract_data()
        expected = [2.0, 2.0, 2.0, -9999999.0, -9999999.0]
        np.testing.assert_array_equal(expected, interp_values)

    def test_invalid_cell_scalars_and_activity_size(self):
        """Test extractor with cell scalars only."""
        #  3----2
        #  | 1 /|
        #  |  / |
        #  | /  |
        #  |/ 0 |
        #  0----1
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 0, 1, 2, XmUGrid.xmugrid_celltype_enum.XMU_TRIANGLE, 3, 2, 3, 0]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        cellScalars = [1]
        activity = [False]
        extractor.set_grid_cell_scalars(cellScalars, activity, data_location_enum.LOC_CELLS)
        extract_locations = [(0.25, 0.75, 100.0), (0.75, 0.25, 0.0)]
        extractor.set_extract_locations(extract_locations)

        interp_values = extractor.extract_data()
        expected = [0.0, -9999999.0]
        np.testing.assert_array_equal(expected, interp_values)

    def test_changing_scalars_and_activity(self):
        """Test extractor going through time steps with cell and point scalars."""
        # build a grid with 3 cells in a row
        points = [(0, 1, 0), (1, 1, 0), (2, 1, 0), (3, 1, 0), (0, 0, 0), (1, 0, 0), (2, 0, 0), (3, 0, 0)]
        cells = [
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 4, 5, 1, # cell 0
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 5, 6, 2, # cell 1
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 2, 6, 7, 3  # cell 2
        ]

        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        extract_locations = [
            (0.75, 0.25, 0.0), # cell 0
            (1.5, 0.5, 0.0),   # cell 1
            (2.25, 0.75, 0.0)  # cell 3
        ]

        # timestep 1
        scalars = [1, 2, 3]
        # empty activity means all are enabled
        activity = []
        extractor.set_grid_cell_scalars(scalars, activity, data_location_enum.LOC_CELLS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [1.25, 2.0, 2.75]
        np.testing.assert_array_equal(expected_values, extracted_values)

        # timestep 2
        scalars = [2, 3, 4]
        activity = [True]*3
        activity[1] = False
        extractor.set_grid_cell_scalars(scalars, activity, data_location_enum.LOC_CELLS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [2, -9999999.0, 4]
        np.testing.assert_array_equal(expected_values, extracted_values)

        # timestep 3
        scalars = [3, 4, 5]
        activity.clear()
        extractor.set_grid_cell_scalars(scalars, activity, data_location_enum.LOC_CELLS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [3.25, 4.0, 4.75]
        np.testing.assert_array_equal(expected_values, extracted_values)

        # change to point data
        # timestep 1
        scalars = [1, 2, 3, 4, 2, 3, 4, 5]
        # empty activity means all are enabled
        extractor.set_grid_point_scalars(scalars, activity, data_location_enum.LOC_POINTS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [2.5, 3.0, 3.5]
        np.testing.assert_array_equal(expected_values, extracted_values)

        # timestep 2
        scalars = [2, 3, 4, 5, 3, 4, 5, 6]
        activity = [True]*8
        activity[0] = False
        extractor.set_grid_point_scalars(scalars, activity, data_location_enum.LOC_POINTS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [-9999999.0, 4.0, 4.5]
        np.testing.assert_array_equal(expected_values, extracted_values)

        # timestep 3
        scalars = [3, 4, 5, 6, 4, 5, 6, 7]
        activity = [True]*8
        activity[1] = False
        extractor.set_grid_point_scalars(scalars, activity, data_location_enum.LOC_POINTS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [-9999999.0, -9999999.0, 5.5]
        np.testing.assert_array_equal(expected_values, extracted_values)

        # timestep 4
        activity.clear()
        extractor.set_grid_point_scalars(scalars, activity, data_location_enum.LOC_POINTS)
        extractor.set_extract_locations(extract_locations)

        extracted_values = extractor.extract_data()
        expected_values = [4.5, 5, 5.5]
        np.testing.assert_array_equal(expected_values, extracted_values)

    def test_copied_extractor(self):
        """Test extractor built by copying triangles."""
        #  3----2
        #  |    |
        #  |    |
        #  |    |
        #  |    |
        #  0----1
        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dDataExtractor(ugrid)
        self.assertIsInstance(extractor, XmUGrid2dDataExtractor)

        point_scalars = [1, 2, 3, 4]
        extractor.set_grid_point_scalars(point_scalars, [], data_location_enum.LOC_POINTS)
        extract_locations = [(0.5, 0.5, 0.0)]
        extractor.set_extract_locations(extract_locations)
        interp_values = extractor.extract_data()
        expected = [2.0]
        np.testing.assert_array_equal(expected, interp_values)

        extractor2 = XmUGrid2dDataExtractor(extractor)
        extractor2.set_grid_point_scalars(point_scalars, [], data_location_enum.LOC_POINTS)
        extractor2.set_extract_locations(extract_locations)
        interp_values = extractor2.extract_data()
        np.testing.assert_array_equal(expected, interp_values)

    def test_tutorial(self):
        """Test XmUGrid2dDataExtractor for tutorial."""
        # build 2x3 grid
        points = [
            (288050, 3907770, 0), (294050, 3907770, 0), (300050, 3907770, 0),
            (306050, 3907770, 0), (288050, 3901770, 0), (294050, 3901770, 0),
            (300050, 3901770, 0), (306050, 3901770, 0), (288050, 3895770, 0),
            (294050, 3895770, 0), (300050, 3895770, 0), (306050, 3895770, 0)
        ]
        cells = [
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 4, 5, 1,
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 5, 6, 2,
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 2, 6, 7, 3,
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 4, 8, 9, 5,
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 5, 9, 10, 6,
            XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 6, 10, 11, 7
        ]
        ugrid = XmUGrid(points, cells)
        # Step 1. Create an extractor for an XmUGrid (call XmUGrid2dDataExtractor).
        extractor = XmUGrid2dDataExtractor(ugrid)

        # Step 2. Set extract locations (call XmUGrid2dDataExtractor::set_extract_locations).
        extract_locations = [
            (289780, 3906220, 0),
            (293780, 3899460, 0),
            (298900, 3900780, 0),
            (301170, 3904960, 0),
            (296330, 3906180, 0),
            (307395, 3901463, 0)
        ]
        extractor.set_extract_locations(extract_locations)
        retrieved_locations = extractor.get_extract_locations()
        np.testing.assert_array_equal(extract_locations, retrieved_locations)

        # Step 3. Optionally set the "no data" value for output interpolated values
        #         (XmUGrid2dDataExtractor::set_no_data_value).
        extractor.set_no_data_value(-999.0)

        # time step 1
        # Step 4. Set the point scalars for the first time step (XmUGrid2dDataExtractor::set_grid_point_scalars).
        point_scalars = [730.787, 1214.54, 1057.145, 629.2069, 351.1153, 631.6649, 1244.366,
        449.9133, 64.04247, 240.9716, 680.0491, 294.9547]
        extractor.set_grid_point_scalars(point_scalars, [], data_location_enum.LOC_CELLS)
        # Step 5. Extract the data (call xms::XmUGrid2dDataExtractor::extract_data).
        extracted_data = extractor.extract_data()

        expectedData = [719.6, 468.6, 1033.8, 996.5, 1204.3, -999.0]
        np.testing.assert_allclose(expectedData, extracted_data, atol=0.2)

        # time step 2
        # Step 6. Continue using steps 4 and 5 for remaining time steps.
        point_scalars = [-999.0, 1220.5, 1057.1, 613.2, 380.1, 625.6, 722.2, 449.9, 51.0, 240.9, 609.0, 294.9]
        cell_activity = [True]*ugrid.get_number_of_cells()
        cell_activity[0] = False
        extractor.set_grid_point_scalars(point_scalars, cell_activity, data_location_enum.LOC_CELLS)
        # Step 7. Extract the data (call xms::XmUGrid2dDataExtractor::extract_data).
        extracted_data = extractor.extract_data()

        expectedData = [-999.0, 466.4, 685.0, 849.4, 1069.6, -999.0]
        np.testing.assert_allclose(expectedData, extracted_data, atol=0.2)


class TestDataLocationEnum(unittest.TestCase):
    """CellType enum tests"""

    def test_data_location_enum(self):
        from xmsextractor_py.extractor import data_location_enum
        self.assertEqual("data_location_enum.LOC_POINTS",
                         str(data_location_enum.LOC_POINTS))
        self.assertEqual("data_location_enum.LOC_CELLS",
                         str(data_location_enum.LOC_CELLS))
        self.assertEqual("data_location_enum.LOC_UNKNOWN",
                         str(data_location_enum.LOC_UNKNOWN))
        self.assertEqual(3, len(data_location_enum.__members__))
