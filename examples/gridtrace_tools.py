from shapely.geometry import Polygon
import holoviews as hv
import geoviews as gv
import pandas as pd
import xarray as xr
import numpy as np
import math

import hvplot.pandas
import hvplot.xarray

from xmsgrid.ugrid import UGrid
import xmsgrid


def create_hv_objects_from_ugrid(_ug, _projection=None):
    """
    Get the holoviews objects from a ugrid to be able to display
    
    Args:
        _ug (UGrid): a grid with points and cells
        
    Returns:
        holoviews.Polygons: polygons to display
        holoviews.Path: lines to display
        holoviews.Points: points to display
    """
    points = _ug.get_locations()
    cell_stream = _ug.get_cellstream()

    cellstream_index = 0
    
    # Parse Stream
    polygons = []
    lines = []
    while cellstream_index < len(cell_stream):
        cell_type = cell_stream[cellstream_index]
        point_count = cell_stream[cellstream_index + 1]
        cell_point_indexs = [cell_stream[x] for x in range(cellstream_index + 2, cellstream_index + 2 +  point_count)]
        cell_points = [points[x] for x in cell_point_indexs]
        if cell_type not in [xmsgrid.ugrid.UGrid.POLY_LINE, xmsgrid.ugrid.UGrid.LINE]:
            if cell_type == xmsgrid.ugrid.UGrid.PIXEL:
                i3 = cell_points[2]
                cell_points[2] = cell_points[3]
                cell_points[3] = i3
            poly = Polygon(cell_points)
            polygons.append(poly)
        else:
            lines.append(cell_points)
        cellstream_index += (2 + point_count)
         
    if not _projection:
      # Polygons
      r_polygons = hv.Polygons(polygons).options(alpha=.25)
              
      # Lines
      r_lines = hv.Path(lines).options(line_color='red')
      
      # Points
      r_points = hv.Points(points).options(color='black', size=1.5)

    else:
      # Polygons
      r_polygons = gv.Polygons(polygons, crs=_projection)
              
      # Lines
      r_lines = gv.Path(lines, crs=_projection).options(line_color='red')
      
      # Points
      r_points = gv.Points(points, crs=_projection).options(color='black', size=1.5)
      
      
    return r_polygons, r_lines, r_points

def hv_vector_field_from_2_timesteps(s1, s2, pts):
    x0, y0, _ = zip(*s1)
    x1, y1, _ = zip(*s2)
    mag = [math.sqrt(abs((x1[i] - x0[i]) + (y1[i] - y0[i]))) for i in range(len(s1))]
    angle = [math.atan2((y1[i] - y0[i]), (x1[i] - x0[i])) for i in range(len(s1))]
    xs, ys, _ = zip(*pts)
    overlay = hv.VectorField((xs, ys, angle, mag))
    return overlay.opts(
      hv.opts.VectorField(line_width=1.5, color='red', magnitude=hv.dim('Magnitude').norm()*0.2, pivot='tail')
    )




