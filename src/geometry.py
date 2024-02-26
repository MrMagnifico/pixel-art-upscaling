import networkx
import random
from math import sqrt, sin, cos, pi
from typing import List, Tuple
from utils import slope


class Point(object):
    """
    Represents a point in 2D Cartesian space.
    Utilises complex numbers for underlying coordinates as it makes B-spline calculation easier
    """
    def __init__(self, value):
        if isinstance(value, complex):
            self.value = value
        elif isinstance(value, (tuple, list)):
            self.value = value[0] + value[1] * 1j
        elif isinstance(value, Point):
            self.value = value.value
        else:
            raise ValueError(f"Invalid value for Point: {value}")

    def __str__(self):
        return f"<Point ({self.x}, {self.y})>"

    def __repr__(self):
        return str(self)

    @property
    def x(self):
        return self.value.real

    @property
    def y(self):
        return self.value.imag

    @property
    def tuple(self):
        return (self.x, self.y)

    def _op(self, op, other):
        if isinstance(other, Point):
            other = other.value
        return Point(getattr(self.value, op)(other))

    def __eq__(self, other):
        try:
            other = Point(other).value
        except ValueError:
            pass
        return self.value.__eq__(other)

    def __add__(self, other):
        return self._op('__add__', other)

    def __radd__(self, other):
        return self._op('__radd__', other)

    def __sub__(self, other):
        return self._op('__sub__', other)

    def __rsub__(self, other):
        return self._op('__rsub__', other)

    def __mul__(self, other):
        return self._op('__mul__', other)

    def __rmul__(self, other):
        return self._op('__rmul__', other)

    def __div__(self, other):
        return self._op('__div__', other)

    def __rdiv__(self, other):
        return self._op('__rdiv__', other)

    def __abs__(self):
        return abs(self.value)

    def round(self, places=5):
        return Point((round(self.x, places), round(self.y, places)))


class Path(object):
    """The path class encodes all the splines that are fit to a shape"""
    def __init__(self, shape_graph):
        self.path = self.make_path(shape_graph)
        self.shapes = set()

    def key(self):
        """Return the underlying path as a tuple to be used as a dict key"""
        return tuple(self.path)

    def make_path(self, shape_graph: networkx.Graph):
        """
        Create a path encoding the points and edges in a graph with a single connected component

        Args:
            shape_graph: Graph with only a single connected component
        """
        # Create set of nodes to be traversed and added to the path (and traverse the first one)
        nodes = set(shape_graph.nodes())
        path = [min(nodes)]
        neighbors = sorted(shape_graph.neighbors(path[0]), key=lambda p: slope(path[0], p))
        path.append(neighbors[0])
        nodes.difference_update(path)

        # Traverse rest of nodes
        p = path[-1]
        i = 0
        while nodes:
            for neighbor in shape_graph.neighbors(path[-1]):
                if neighbor in nodes:
                    nodes.remove(neighbor)
                    path.append(neighbor)
                    break
            
            # Infinite loop detection
            if p != path[-1]:
                p = path[-1]
                i = 0
            else:
                i += 1
                if i == 3:
                    break
        return path


    def make_spline(self):
        """Fit a spline to the path"""
        self.spline = curve_to_closed_bspline(self.path)

    def smooth_spline(self):
        """Optimise the spline that is already fit to the path"""
        self.smooth = smooth_spline(self.spline)


class Shape(object):
    """Glorified container class for Paths with additional image data"""
    def __init__(self, pixels, value, corners):
        self.pixels                     = pixels
        self.value                      = value
        self.corners                    = corners
        self._outside_path: Path        = None  # type: ignore (Screw you Pylance, I do whatever I want)
        self._inside_paths: List[Path]  = []

    def _paths_attr(self, attr):
        paths = [list(reversed(getattr(self._outside_path, attr)))]
        paths.extend(getattr(path, attr) for path in self._inside_paths)

    @property
    def paths(self):
        paths = [list(reversed(self._outside_path.path))]
        paths.extend(path.path for path in self._inside_paths)
        return paths

    @property
    def splines(self):
        paths = [self._outside_path.spline.reversed()]
        paths.extend(path.spline for path in self._inside_paths)
        return paths

    @property
    def smooth_splines(self):
        paths = [self._outside_path.smooth.reversed()]
        paths.extend(path.smooth for path in self._inside_paths)
        return paths

    def add_outline(self, path: Path, outside=False):
        if outside:
            self._outside_path = path
        else:
            self._inside_paths.append(path)
        path.shapes.add(self)


class BSpline(object):
    """
    This is made out of mathematics. You have been warned.
    A B-spline has:
      * n + 1 control points
      * m + 1 knots
      * degree p
      * m = n + p + 1
    """
    def __init__(self, knots, points, degree=None):
        """
        Construct a BSpline curve object

        Args:
            knots: Knot points where the individual component curves intersect
            points: Control points dictating the shape of the curve
            degree: Degree of the polynomials making up the individual component curves
        """
        self.knots = tuple(knots)
        self._points = [Point(p) for p in points]
        expected_degree = len(self.knots) - len(self._points) - 1
        if degree is None:
            degree = expected_degree
        if degree != expected_degree:
            raise ValueError(f"Expected degree {expected_degree}, got f{degree}")
        self.degree = degree
        self._reset_cache()

    def _reset_cache(self):
        self._cache = {}

    def move_point(self, i: int, value: Point):
        """
        Move the point at the given index to the given position

        Args:
            i: Index of the point to be moved
            value: New position of the point
        """
        self._points[i] = value
        self._reset_cache()

    def __str__(self):
        return f"<{type(self).__name__} degree={self.degree}, points={len(self.points)}, knots={len(self.knots)}>"

    def copy(self):
        return type(self)(self.knots, self.points, self.degree)

    @property
    def domain(self):
        return (self.knots[self.degree],
                self.knots[len(self.knots) - self.degree - 1])

    @property
    def points(self):
        return tuple(self._points)

    @property
    def useful_points(self):
        return self.points

    def __call__(self, u):
        """
        De Boor's Algorithm. Made out of more maths.
        """
        s = len([uk for uk in self.knots if uk == u])
        for k, uk in enumerate(self.knots):
            if uk >= u:
                break
        if s == 0:
            k -= 1
        if self.degree == 0:
            if k == len(self.points):
                k -= 1
            return self.points[k]
        ps = [dict(zip(range(k - self.degree, k - s + 1),
                       self.points[k - self.degree:k - s + 1]))]

        for r in range(1, self.degree - s + 1):
            ps.append({})
            for i in range(k - self.degree + r, k - s + 1):
                a = (u - self.knots[i]) / (self.knots[i + self.degree - r + 1]
                                           - self.knots[i])
                ps[r][i] = (1 - a) * ps[r - 1][i - 1] + a * ps[r - 1][i]
        return ps[-1][k - s]

    def quadratic_bezier_segments(self):
        """
        Extract a sequence of quadratic Bezier curves making up this spline.
        NOTE: This assumes our spline is quadratic.
        """
        assert self.degree == 2
        control_points = self.points[1:-1]
        on_curve_points = [self(u) for u in self.knots[2:-2]]
        ocp0 = on_curve_points[0]
        for cp, ocp1 in zip(control_points, on_curve_points[1:]):
            yield (ocp0.tuple, cp.tuple, ocp1.tuple)
            ocp0 = ocp1

    def derivative(self):
        """
        Take the derivative.
        """
        cached = self._cache.get('derivative')
        if cached:
            return cached

        new_points = []
        p = self.degree
        for i in range(0, len(self.points) - 1):
            coeff = p / (self.knots[i + 1 + p] - self.knots[i + 1])
            new_points.append(coeff * (self.points[i + 1] - self.points[i]))

        cached = BSpline(self.knots[1:-1], new_points, p - 1)
        self._cache['derivative'] = cached
        return cached

    def _clamp_domain(self, value: float):
        """Clamp the given value to the domain of the spline curve"""
        return max(self.domain[0], min(self.domain[1], value))

    def _get_span(self, index: int):
        """
        Get the span of the curve whose start point is the knot at the given index

        Args:
            index: Index of the knot point dictating the start of the curve whose span is to be computed

        Returns:
            Span defined by the respective curve
        """
        return (self._clamp_domain(self.knots[index]),
                self._clamp_domain(self.knots[index + 1]))

    def _get_point_spans(self, index: int):
        """
        Acquire the spans defined by the curves whose knot points are in the range [index, index + degree_of_curve]

        Args:
            index: Index of the knot point defining the start of the first polynomial sub-curve in the sequence
        """
        return [self._get_span(index + i) for i in range(self.degree)]

    def integrate_over_span(self, func, span: Tuple[float, float], intervals: int):
        """
        Numerically evaluate the integral of a function

        Args:
            func: The function whose integral is to be numerically evaluated
            span: The limits over which to compute the definite integral
            intervals: The number of intervals that the span is divided into for numerical evaluation

        Returns:
            The value of the numerically computed integral
        """
        if span[0] == span[1]:
            return 0

        interval = (span[1] - span[0]) / intervals
        result = (func(span[0]) + func(span[1])) / 2
        for i in range(1, intervals):
            result += func(span[0] + i * interval)
        result *= interval

        return result

    def integrate_for(self, index, func, intervals):
        """
        Integrate a given function in the span of the curves whose knot points are in the range [index, index + degree_of_curve]

        Args:
            index: Index of the knot point defining the start of the first polynomial sub-curve in the sequence
            func: Function whose integral is to be computed
            intervals: The number of intervals that each span is divided into for numerical evaluation
        """
        spans_ = self._get_point_spans(index)
        spans = [span for span in spans_ if span[0] != span[1]]
        return sum(self.integrate_over_span(func, span, intervals)
                   for span in spans)

    def curvature(self, u):
        """
        Compute the curvature at the given point according to
        (https://en.wikipedia.org/wiki/Curvature#In_terms_of_a_general_parametrization)

        Args:
            u: The point at which to compute the curvature

        Returns:
            The value of the curvature as a positive number
        """
        d1 = self.derivative()(u)
        d2 = self.derivative().derivative()(u)
        num = d1.x * d2.y - d1.y * d2.x
        den = sqrt(d1.x ** 2 + d1.y ** 2) ** 3
        if den == 0:
            return 0
        return abs(num / den)

    def curvature_energy(self, index, intervals_per_span):
        """
        Compute the curvature energy term as defined in section 3.4 of the paper for a particular
        sequence of sub-curves

        Args:
            index: Index of the knot point defining the start of the first polynomial sub-curve in the sequence
            intervals_per_span: The number of intervals that each span is divided into for numerical evaluation

        Returns:
            The value of the curvature energy term
        """
        return self.integrate_for(index, self.curvature, intervals_per_span)

    def reversed(self):
        return type(self)(
            (1 - k for k in reversed(self.knots)), reversed(self._points),
            self.degree)


class ClosedBSpline(BSpline):
    def __init__(self, knots, points, degree=None):
        """
        Construct a ClosedBSpline curve object

        Args:
            knots: Knot points where the individual component curves intersect
            points: Control points dictating the shape of the curve
            degree: Degree of the polynomials making up the individual component curves
        """
        super(ClosedBSpline, self).__init__(knots, points, degree)
        self._unwrapped_len = len(self._points) - self.degree
        self._check_wrapped()

    def _check_wrapped(self):
        if self._points[:self.degree] != self._points[-self.degree:]:
            raise ValueError(f"Points not wrapped at degree {self.degree}")

    def move_point(self, index, value):
        """
        Move the point at the given index to the given position

        Args:
            index: Index of the point to be moved
            value: New position of the point
        """
        if not 0 <= index < len(self._points):
            raise IndexError(index)
        index = index % self._unwrapped_len
        super(ClosedBSpline, self).move_point(index, value)
        if index < self.degree:
            super(ClosedBSpline, self).move_point(
                index + self._unwrapped_len, value)

    @property
    def useful_points(self):
        return self.points[:-self.degree]

    def _get_span(self, index):
        """
        Get the span of the curve whose start point is the knot at the given index

        Args:
            index: Index of the knot point dictating the start of the curve whose span is to be computed

        Returns:
            Span defined by the respective curve
        """
        span = lambda i: (self.knots[i], self.knots[i + 1])
        d0, d1 = span(index)
        if d0 < self.domain[0]:
            d0, d1 = span(index + len(self.points) - self.degree)
        elif d1 > self.domain[1]:
            d0, d1 = span(index + self.degree - len(self.points))
        return self._clamp_domain(d0), self._clamp_domain(d1)


def polyline_to_closed_bspline(path, degree=2):
    """
    Make a closed B-spline from a path through some nodes.
    """
    points = path + path[:degree]
    m = len(points) + degree
    knots = [float(i) / m for i in range(m + 1)]

    return ClosedBSpline(knots, points, degree)


class SplineSmoother(object):
    INTERVALS_PER_SPAN = 20
    POINT_GUESSES = 20
    GUESS_OFFSET = 0.05
    ITERATIONS = 20
    POSITIONAL_ENERGY_MULTIPLIER = 1


    def __init__(self, spline):
        self.orig = spline
        self.spline = spline.copy()

    def _e_curvature(self, index):
        """
        Compute the curvature energy term as defined in section 3.4 of the paper for a particular
        sequence of sub-curves

        Args:
            index: Index of the knot point defining the start of the first polynomial sub-curve in the sequence

        Returns:
            The value of the curvature energy term
        """
        return self.spline.curvature_energy(index, self.INTERVALS_PER_SPAN)

    def _e_positional(self, index):
        """
        Compute the positional energy term as defined in section 3.4 of the paper for a particular point

        Args:
            index: Index of the point to compute the positional energy for

        Returns:
            The value of the positional energy term
        """
        orig = self.orig.points[index]
        point = self.spline.points[index]
        e_positional = abs(point - orig) ** 4
        return e_positional * self.POSITIONAL_ENERGY_MULTIPLIER

    def point_energy(self, index):
        """
        Compute the sum of the curvature and positional energy terms for a particular point

        Args:
            index: Index of the point for which to compute the sum

        Returns:
            Sum of the curvature and positional energy terms
        """
        e_curvature = self._e_curvature(index)
        e_positional = self._e_positional(index)
        return e_positional + e_curvature

    def _rand(self):
        """Compute a random point"""
        offset = random.random() * self.GUESS_OFFSET
        angle = random.random() * 2 * pi
        return offset * Point((cos(angle), sin(angle)))

    def smooth_point(self, index, start):
        """
        Iteratively optimize the position of a particular point

        Args:
            index: Index of the point whose position is to be optimized
            start: The starting position of the point which is then moved by the optimization procedure
        """
        energies = [(self.point_energy(index), start)]
        for _ in range(self.POINT_GUESSES):
            point = start + self._rand()
            self.spline.move_point(index, point)
            energies.append((self.point_energy(index), point))
        self.spline.move_point(index, min(energies)[1])

    def smooth(self):
        """Iteratively smoothen the underlying curve of the smoother"""
        for _it in range(self.ITERATIONS):
            for i, point in enumerate(self.spline.useful_points):
                self.smooth_point(i, point)


def smooth_spline(spline):
    """
    Smooth a given spline according to the procedure in section 3.4

    Args:
        spline: The spline curve to be smoothed

    Returns:
        A smoothed spline curve
    """
    smoother = SplineSmoother(spline)
    smoother.smooth()
    return smoother.spline


def curve_to_closed_bspline(path, degree=2):
    """
    Compute a ClosedBSpline curve fitted to the points defined by the given path
    
    Args:
        path: The path containing the 
        degree: The degree of the polynomials of the sub-curves of the resultant ClosedBSpline
    """
    points = path + path[:degree]
    m = len(points) + degree
    KnotVector = [float(i) / m for i in range(m + 1)]

    return ClosedBSpline(KnotVector, points, degree)
