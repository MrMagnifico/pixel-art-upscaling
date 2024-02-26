from typing import Tuple


Y_THRESHOLD = 48
U_THRESHOLD = 7
V_THRESHOLD = 6


def rgb_to_yuv(pixel: Tuple[int, int, int]) -> Tuple[int, int, int]:
    '''
    Convert an RGB pixel to YUV components in the same manner as hqx

    Args:
        pixel: 3-tuple of RGB values
    
    Returns:
        3-tuple of YUV values
    '''
    return (
        int(0.299 * pixel[0]      + 0.587 * pixel[1]    + 0.114 * pixel[2]),
        int((-0.169 * pixel[0]    + -0.331 * pixel[1]   + 0.5 * pixel[2]) + 128),
        int((0.5 * pixel[0]       + -0.419 * pixel[1]   + -0.081 * pixel[2]) + 128))


def pixels_similar(p0: Tuple[int, int, int], p1: Tuple[int, int, int]) -> bool:
    """
    Identify if two pixels are similar in the same manner as hqx using YUV components

    Args:
        p0: First pixel as RGB 3-tuple
        p1: Second pixel as RGB 3-tuple

    Returns:
        True if the pixels are similar (i.e. YUV differences are at or below thresholds), False otherwise
    """
    p0_yuv = rgb_to_yuv(p0)
    p1_yuv = rgb_to_yuv(p1)
    return  (abs(p0_yuv[0] - p1_yuv[0]) <= Y_THRESHOLD and
             abs(p0_yuv[1] - p1_yuv[1]) <= U_THRESHOLD and
             abs(p0_yuv[2] - p1_yuv[2]) <= V_THRESHOLD)


def pixel_coords(width: int, height: int):
    """
    Generate range of image dimensions as (x, y) pairscovering all pixels row-by-row, column-by-column

    Args:
        width: Width of the image
        height: Height of the image
    """
    for y in range(height):
        for x in range(width):
            yield (x, y)


def clamp(num, min_value, max_value):
   return max(min(num, max_value), min_value)


def within_bounds(coordinate: Tuple[int, int], size: Tuple[int, int], offset: Tuple[int, int] = (0, 0)) -> bool:
    x, y = coordinate[0] + offset[0], coordinate[1] + offset[1]
    width, height = size
    return (0 <= x < width) and (0 <= y < height)


def sorted_vertices(edge) -> Tuple:
    """
    Return vertices making up the edge in a sorted order

    Args:
        edge: Edge whose vertices are to be sorted

    Returns:
        Tuple containing the vertices in sorted order
    """
    return tuple(sorted(edge[:2]))


def safe_remove(collection, element):
    """
    Remove an element from a collection only if the collection actually contains the element

    Args:
        collection: Collection to remove element from
        element: Element to be removed
    """
    if element in collection:
        collection.remove(element)


def slope(p0, p1) -> float:
    """
    Compute the slope from p0 to p1
    
    Args:
        p0: Source point
        p1: Destination point

    Returns:
        The gradient of the straight line from p0 to p1
    """
    dx = p1[0] - p0[0]
    dy = p1[1] - p0[1]

    # If dx is zero (i.e. the gradient is infinite) then return a very large number instead
    if dx == 0:
        return dy * 99999999999999
    return dy / dx
