import png
from cairosvg import svg2png
from kopf_lichinski import Vectorizer
from os import path
from svgwrite import Drawing
from typing import List, Tuple


def color(rgb_value_tuple: Tuple[int, int, int]):
    return "rgb(%s,%s,%s)" % rgb_value_tuple


class SVGWriter(object):
    PIXEL_SCALE = 10
    CURVE_COLOR = (255, 127, 0)

    def __init__(self, data: Vectorizer, filename, scale=None, color=None):
        self.name = filename
        self.pixel_data = data
        if scale:
            self.PIXEL_SCALE = scale
        if color:
            self.CURVE_COLOR = color

    def scale_pt(self, pt, offset=(0, 0)):
        return tuple(int((n + o) * self.PIXEL_SCALE) for n, o in zip(pt, offset))

    def output_image(self, raster_factors = [], original_width: int = 1, original_height: int = 1):
        filename = self.name
        filename_svg = path.join("outputs", "%s.%s" % (filename, "svg"))
        drawing = Drawing(filename_svg)
        self.draw_shapes(drawing)
        drawing.saveas(filename_svg, pretty=True)

        if raster_factors:
            for factor in raster_factors:
                filename_png = path.join("outputs", f"{filename}-{factor}X.png")
                svg2png(url=filename_svg, write_to=filename_png,
                        parent_width=(original_width * self.PIXEL_SCALE), parent_height=(original_height * self.PIXEL_SCALE),
                        output_width=(original_width * factor), output_height=(original_height * factor),
                        scale=factor)
            
    def draw_shapes(self, drawing):
        for shape in self.pixel_data.shapes:
            paths = getattr(shape, "smooth_splines")
            self.draw_spline(drawing, paths, shape.value)

    def draw_spline(self, drawing, splines, fill):
        if fill == (255, 255, 255):
            return
        path = []
        for spline in splines:
            curves = list(spline.quadratic_bezier_segments())
            path.append("M")
            path.append(self.scale_pt(curves[0][0]))
            for curve in curves:
                path.append("Q")
                path.append(self.scale_pt(curve[1]))
                path.append(self.scale_pt(curve[2]))
            path.append("Z")
        drawing.add(drawing.path(path, stroke=color(fill), fill=color(fill)))


def read_png(filename):
    """
    Read PNG data to a 2D list

    Args:
        filename: Path to a PNG file
    
    Returns:
        Height of image, width of image, 2D list where basic elements are 3-tuples of RGB values
    """
    width: int
    height: int
    width, height, image_rows, _ = png.Reader(filename=filename).asRGB8()

    image_data: List[List[Tuple[int, int, int]]] = []
    for row in image_rows:
        row_data = []
        while row:
            pixel = (row.pop(0), row.pop(0), row.pop(0))
            row_data.append(pixel)
        image_data.append(row_data)
    return height, width, image_data
