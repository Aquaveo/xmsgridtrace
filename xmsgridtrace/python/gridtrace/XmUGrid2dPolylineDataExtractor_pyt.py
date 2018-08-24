"""Test XmUGrid2dPolylineDataExtractor_py.cpp"""
import unittest
import xmsextractor_py
from xmsgrid_py.ugrid import XmUGrid
from xmsextractor_py.extractor import XmUGrid2dPolylineDataExtractor
from xmsextractor_py.extractor import data_location_enum
import numpy as np

class TestXmUGrid2dPolylineDataExtractor(unittest.TestCase):
    """XmUGrid2dPolylineDataExtractor tests"""

    def test_one_cell_one_segment(self):
        """Test extractor with point scalars only."""
        # clang-format off
        #     3-------2
        #     |       |
        # 0===============1
        #     |       |
        #     0-------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-1, 0.5, 0), (2, 0.5, 0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 0.5, 1.5, 2.5, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(-1, 0.5, 0), (0.0, 0.5, 0.0), (0.5, 0.5, 0.0), (1.0, 0.5, 0.0), (2, 0.5, 0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentAllInCell(self):
        """Test extractor with single segment all inside of cell."""
        # clang-format off
        # (1)  3--------2  (3)
        #      |        |
        #      | 0====1 |
        #      |        |
        # (0)  0--------1  (2)
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(0.25, 0.50, 0.0), (0.75, 0.50, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [1.0, 1.5, 2.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(0.25, 0.5, 0.0), (0.5, 0.5, 0.0), (0.75, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentAlongEdge(self):
        """Test extractor with single segment along an edge."""
        # clang-format off
        #  0===3========2===1
        #      |        |
        #      |        |
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-0.5, 1.0, 0.0), (1.55, 1.0, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 1.0, 3.0, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(-0.5, 1.0, 0.0), (0.0, 1.0, 0.0), (1.0, 1.0, 0.0), (1.55, 1.0, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentAllOutside(self):
        """Test extractor with single segment all outside of cell."""
        # clang-format off
        #      3========2
        #      |        |
        # 0==1 |        |
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-0.5, 0.5, 0.0), (-0.25, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(-0.5, 0.5, 0.0), (-0.25, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentOutToTouch(self):
        """Test extractor with single segment with endpoint touching cell."""
        # clang-format off
        #      3========2
        #      |        |
        # 0====1        |
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-0.5, 0.5, 0.0), (0.0, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 0.5]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(-0.5, 0.5, 0.0), (0.0, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentTouchToOut(self):
        """Test extractor with single segment with first point touching edge."""
        # clang-format off
        #      3========2
        #      |        |
        #      |        0===1
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(1.0, 0.5, 0.0), (1.5, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [2.5, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(1.0, 0.5, 0.0), (1.5, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentCrossCellPoint(self):
        """Test extractor with single segment touching cell point."""
        # clang-format off
        #        1
        #       /
        #      3========2
        #    / |        |
        #   0  |        |
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-0.5, 0.5, 0.0), (0.0, 1.0, 0.0), (0.5, 1.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 1.0, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(-0.5, 0.5, 0.0), (0.0, 1.0, 0.0), (0.5, 1.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentAcrossCellIntoSecond(self):
        """Test extractor with single segment crossing first cell into second."""
        # clang-format off
        #      3========2========5
        #      |        |        |
        #   0===============1    |
        #      |        |        |
        #      0--------1--------4
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0, 0), (2, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3, XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 4, 5, 2]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1, 4, 5]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-0.5, 0.5, 0.0), (1.5, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 0.5, 1.5, 2.5, 3.5]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(-0.5, 0.5, 0.0), (0.0, 0.5, 0.0), (0.5, 0.5, 0.0),
            (1.0, 0.5, 0.0), (1.5, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testSegmentAcrossSplitCells(self):
        """Test extractor with single segment going across unconnected cells."""
        # clang-format off
        #       3-------2       7-------6
        #       |       |       |       |
        #  0========================1   |
        #       |       |       |       |
        #       0-------1       4-------5
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0),
                            (2, 0, 0), (3, 0, 0), (3, 1, 0), (2, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3, XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 4, 5, 6, 7]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1, 4, 6, 7, 5]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-1, 0.5, 0), (2.5, 0.5, 0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 0.5, 1.5, 2.5, -9999999.0, 4.5, 5.5]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(-1.0, 0.5, 0.0), (0.0, 0.5, 0.0), (0.5, 0.5, 0.0),
                                       (1.0, 0.5, 0.0), (1.5, 0.5, 0.0), (2.0, 0.5, 0.0),
                                       (2.5, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testTwoSegmentsAcrossOneCell(self):
        """Test extractor with two segments going across a single cell."""
        # clang-format off
        #      3--------2
        #      |        |
        # 0=========1=========2
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(-1, 0.5, 0), (0.5, 0.5, 0.0), (2, 0.5, 0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 0.5, 1.5, 2.5, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [
            (-1.0, 0.5, 0.0), (0.0, 0.5, 0.0), (0.5, 0.5, 0.0), (1.0, 0.5, 0.0), (2.0, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testTwoSegmentsAllOutside(self):
        """Test extractor with two segments all outside of cell."""
        # clang-format off
        #      3--------2
        #      |        |
        #      |        | 0====1====2
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(2.0, 0.5, 0), (3.0, 0.5, 0.0), (4.0, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, -9999999.0, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(2.0, 0.5, 0), (3.0, 0.5, 0.0), (4.0, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testTwoSegmentsFirstExiting(self):
        """Test extractor with two segments: first in to outside, second outside."""
        # clang-format off
        #      3--------2
        #      |        |
        #      |   0========1=======2
        #      |        |
        #      0--------1
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(0.5, 0.5, 0), (3.0, 0.5, 0.0), (4.0, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [1.5, 2.5, -9999999.0, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(0.5, 0.5, 0), (1.0, 0.5, 0), (3.0, 0.5, 0.0), (4.0, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testTwoSegmentsJoinInCell(self):
        """Test extractor with  two segments joining in a cell."""
        # clang-format off
        #      3========2========5
        #      |        |        |
        #      |   0========1=========2
        #      |        |        |
        #      0--------1--------4
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0, 0), (2, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3, XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 4, 5, 2]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1, 4, 5]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(0.5, 0.5, 0.0), (1.5, 0.5, 0.0), (2.5, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [1.5, 2.5, 3.5, 4.5, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(0.5, 0.5, 0.0), (1.0, 0.5, 0.0), (1.5, 0.5, 0.0),
            (2.0, 0.5, 0.0), (2.5, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testTwoSegmentsJoinOnBoundary(self):
        """Test extractor with two segments joining on two cell boundary."""
        # clang-format off
        #      3========2========5
        #      |        |        |
        #      |   0====1=============2
        #      |        |        |
        #      0--------1--------4
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0, 0), (2, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3, XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 4, 5, 2]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1, 4, 5]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(0.5, 0.5, 0.0), (1.0, 0.5, 0.0), (2.5, 0.5, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [1.5, 2.5, 3.5, 4.5, -9999999.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(0.5, 0.5, 0.0), (1.0, 0.5, 0.0), (1.5, 0.5, 0.0),
            (2.0, 0.5, 0.0), (2.5, 0.5, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testThreeSegmentsCrossOnBoundary(self):
        """Test extractor with three segments two crossing at boundary."""
        # clang-format off
        #      3----3---2
        #      |      \ |
        #      |   0========1
        #      |        | \ |
        #      0--------1   2
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        point_scalars = [0, 2, 3, 1]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_POINTS)

        polyline = [(0.5, 0.5, 0), (1.5, 0.5, 0.0), (1.5, 0.0, 0.0), (0.5, 1.0, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [1.5, 2.5, -9999999.0, -9999999.0, 2.5, 2.25, 2.0]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations = [(0.5, 0.5, 0), (1.0, 0.5, 0), (1.5, 0.5, 0.0),
            (1.5, 0.0, 0.0), (1.0, 0.5, 0), (0.75, 0.75, 0.0), (0.5, 1.0, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testCellScalars(self):
        """Test extractor with cell scalars."""
        # clang-format off
        #      3========2========5
        #      |        |        |
        #   0===============1    |
        #      |        |        |
        #      0--------1--------4
        # clang-format on

        points = [(0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (2, 0, 0), (2, 1, 0)]
        cells = [XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 0, 1, 2, 3, XmUGrid.xmugrid_celltype_enum.XMU_QUAD, 4, 1, 4, 5, 2]
        ugrid = XmUGrid(points, cells)
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_CELLS)

        cellScalars = [1, 2]
        extractor.set_grid_scalars(cellScalars, [], data_location_enum.LOC_CELLS)

        polyline = [(-0.5, 0.75, 0.0), (1.5, 0.75, 0.0)]
        extractor.set_polyline(polyline)
        extracted_locations = extractor.get_extract_locations()
        extracted_data = extractor.extract_data()

        expected_data = [-9999999.0, 1.0, 1.0, 1.25, 1.5, 1.75, 1.875]
        np.testing.assert_array_equal(expected_data, extracted_data)
        expected_locations =  [(-0.5, 0.75, 0.0), (0.0, 0.75, 0.0), (0.25, 0.75, 0.0),
            (0.75, 0.75, 0.0), (1., 0.75, 0.0), (1.25, 0.75, 0.0), (1.5, 0.75, 0.0)]
        np.testing.assert_array_equal(expected_locations, extracted_locations)

    def testTransientTutorial(self):
        """Test XmUGrid2dPolylineDataExtractor for tutorial with transient data."""
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
        # Step 1. Create an extractor for an XmUGrid giving the mapped location of the scalar values
        extractor = XmUGrid2dPolylineDataExtractor(ugrid, data_location_enum.LOC_POINTS)

        # Step 2. Optionally set the "no data" value.
        extractor.set_no_data_value(-999.0)

        # Step 3. Set the polyline to be extracted along.
        polyline = [
            (290764, 3895106, 0), (291122, 3909108, 0),
            (302772, 3909130, 0), (302794, 3895775, 0)
        ]
        extractor.set_polyline(polyline)

        # Step 4. Optionally get the locations used for extraction along the polyline.
        extracted_locations = extractor.get_extract_locations()
        expected_locations = [
            (290764.0, 3895106.0, 0.0),
            (290780.9, 3895770.0, 0.0),
            (290862.4, 3898957.5, 0.0),
            (290934.3, 3901770.0, 0.0),
            (291012.0, 3904807.9, 0.0),
            (291087.7, 3907770.0, 0.0),
            (291122.0, 3909108.0, 0.0),
            (302772.0, 3909130.0, 0.0),
            (302774.2, 3907770.0, 0.0),
            (302778.7, 3905041.2, 0.0),
            (302784.1, 3901770.0, 0.0),
            (302788.6, 3899031.3, 0.0),
            (302794.0, 3895775.0, 0.0),
        ]
        np.testing.assert_allclose(expected_locations, extracted_locations, atol=0.15)

        # time step 1
        # Step 5. Set the point scalars for the first time step.
        point_scalars = [
            730.787, 1214.54, 1057.145, 629.2069, 351.1153, 631.6649,
            1244.366, 449.9133, 64.04247, 240.9716, 680.0491, 294.9547
        ]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_CELLS)
        # Step 6. Extract the data.
        extracted_data = extractor.extract_data()
        expected_data = [-999.0, 144.5, 299.4, 485.9, 681.8,
                                 975.7, -999.0, -999.0, 862.8, 780.9,
                                 882.3, 811.0, 504.4]
        np.testing.assert_allclose(expected_data, extracted_data, atol=0.2)

        # time step 2
        # Step 7. Continue using steps 5 and 6 for remaining time steps.
        point_scalars = [-999.0, 1220.5, 1057.1, 613.2, 380.1, 625.6, 722.2, 449.9, 51.0, 240.9,
                          609.0, 294.9]
        extractor.set_grid_scalars(point_scalars, [], data_location_enum.LOC_CELLS)
        extracted_data = extractor.extract_data()
        expected_data = [-999.0, 137.4, 314.8, 498.1, -196.9, 124.7, -999.0, -999.0, 855.5,
            780.9, 598.1, 527.1, 465.4]
        np.testing.assert_allclose(expected_data, extracted_data, atol=0.2)
