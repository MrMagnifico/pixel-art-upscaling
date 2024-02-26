from enum import Enum
from typing import List, Tuple, Set
from heuristics import *
from utils import *
from geometry import *
import networkx


class OutOfBoundsStrategy(Enum):
    NEAREST = 1
    ZERO    = 2
    PANIC   = 3 


class Vectorizer():

    def __init__(self, width: int, height: int, pixel_data: List[List[Tuple[int, int, int]]]):
        self._pixel_data    = pixel_data
        self._width         = width
        self._height        = height
        self._size          = (width, height)

        self._BLOCK_SIZE    = 2


    def vectorize(self):
        """Produce a vector representation of the stored image using the Kopf-Lischinski approach"""
        print("Creating similarity graph...")
        self._create_similarity_graph()
        print("Removing diagonals from similarity graph...")
        self._remove_diagonals()
        print("Creating pixel cell graph...")
        self._create_pixel_graph()
        print("Deforming pixel cell graph...")
        self._deform_pixel_grid()
        print("Creating spline curves...")
        self._create_shapes()
        print("Isolating exterior shapes...")
        self._isolate_outlines()
        print("Adding exterior shapes...")
        self._add_shape_outlines()
        print("Smoothing spline curves...")
        self._smooth_splines()


    def _create_similarity_graph(self):
        """
        Create similarity graph between pixels where each node represents a pixel and edges represent
        connections between pixels that are 'similar' according to hqx YUV channel difference thresholds
        """
        self._similarity_graph = networkx.Graph()

        for x, y in pixel_coords(self._width, self._height):
            corners = set([(x, y), (x + 1, y), (x, y + 1), (x + 1, y + 1)]) # Corner vertices that the current pixel corresponds to in the pixel graph
            self._similarity_graph.add_node((x, y), value=self._pixel_data_safe_access(x, y), corners=corners)

            # Connect edge to all rightward neighbours and bottom-most neighbour
            self._add_edge_if_valid((x, y), (x + 1, y))
            self._add_edge_if_valid((x, y), (x + 1, y - 1))
            self._add_edge_if_valid((x, y), (x + 1, y + 1))
            self._add_edge_if_valid((x, y), (x, y + 1))


    def _remove_diagonals(self):
        """
        Remove diagonals from graph according to the Gestalt laws based heuristics
        outlined in the paper
        """
        ambiguous_diagonal_pairs = []

        for block_nodes in self._enumerate_blocks(self._BLOCK_SIZE):
            # Acquire all edges covered by the nodes in the block
            edges = [edge
                     for edge in self._similarity_graph.edges(block_nodes, data=True)
                     if (edge[0] in block_nodes) and (edge[1] in block_nodes)]

            # Examine diagonals and remove if fully connected (case 1) or resolve ambiguous pairs based on heuristics (case 2)
            diagonals = [edge for edge in edges if edge[2]["diagonal"]]  # type: ignore (return type is a 3-tuple PyLance, thank you)
            if len(diagonals) == 2:
                # Fully connected block (case 1 - remove all diagonals)
                if len(edges) == 6:
                    for diagonal in diagonals:
                        self._similarity_graph.remove_edge(diagonal[0], diagonal[1])
                # Ambiguous pair (case 2 - must be resolved with heuristics)
                elif len(edges) == 2:
                    ambiguous_diagonal_pairs.append(edges)
                else:
                    raise RuntimeError("Unexpected diagonal format (there is likely an error in the _create_similarity_graph() method)")

        self._HEURISTICS = Heuristics(self._similarity_graph)
        self._HEURISTICS.apply_heuristics(ambiguous_diagonal_pairs)


    def _create_pixel_graph(self):
        """Create a lattice graph where each closed cell represents a pixel"""
        self._pixel_graph: networkx.Graph = networkx.grid_2d_graph(self._width + 1, self._height + 1)


    def _deform_pixel_grid(self):
        """
        Reshape the pixel cell graph according to the edges of the similarity graph
        """
        # Compute the graph as a Voronoi diagram on a per vertex basis with respect to the similarity graph
        for node in self._similarity_graph.nodes():
            self._deform_cell(node)

        # Acquire all valence-n nodes in the graph, where n <= 2
        removals = []
        for node in self._pixel_graph.nodes():
            # Do not collapse corner pixels
            if node in [(0, 0), (self._width, 0), (0, self._height), (self._width, self._height)]:
                continue

            neighbors = list(self._pixel_graph.neighbors(node))
            if len(neighbors) == 2: # Directly connect the neighbours of the valence-2 nodes by an edge
                self._pixel_graph.add_edge(neighbors[0], neighbors[1])
            if len(neighbors) <= 2:
                removals.append(node)

        # Remove all valence-(n <= 2) nodes in the graph
        for node in removals:
            self._pixel_graph.remove_node(node)

        # Update corners in similarity graph to reflect pixel graph vertices which were removed
        for _, attributes in self._similarity_graph.nodes(data=True):
            corners = attributes["corners"] # type: ignore (return type is a 2-tuple PyLance, thank you)
            for corner in corners.copy():
                if corner not in self._pixel_graph:
                    corners.remove(corner)

    
    def _create_shapes(self):
        """Create a Shape object for each connected component in the similarity graph"""
        self.shapes: Set[Shape] = set()

        # Identify shapes in the graph by identifying the connected component subgraphs
        for pcg in (self._similarity_graph.subgraph(c).copy() for c in networkx.connected_components(self._similarity_graph)):
            pixels = set()
            value = None
            corners = set()
            for pixel, attrs in pcg.nodes(data=True):
                pixels.add(pixel)
                corners.update(attrs["corners"])
                value = attrs["value"]
            
            # Create a separate shape for each connected component subgraph and store all the pixels of the subgraph in it
            self.shapes.add(Shape(pixels, value, corners))
            

    def _isolate_outlines(self):
        """Compute a copy of the pixel cell graph with all interior edges removed"""
        self._outlines_graph = networkx.Graph(self._pixel_graph)

        for pixel, attrs in self._similarity_graph.nodes(data=True):
            corners = attrs["corners"] # type: ignore (return type is a 2-tuple PyLance, thank you)
            for neighbor in self._similarity_graph.neighbors(pixel):
                edge = corners & self._similarity_graph.nodes[neighbor]["corners"]
                if len(edge) == 2 and self._outlines_graph.has_edge(*edge):
                    self._outlines_graph.remove_edge(*edge)

        # Remove all unconnected nodes (i.e. interior nodes, as we have removed all interior edges)
        for node in list(networkx.isolates(self._outlines_graph)):
            self._outlines_graph.remove_node(node)


    def _add_shape_outlines(self):
        self._paths: dict[Tuple, Path] = {}

        for shape in self.shapes:
            subgraph = self._outlines_graph.subgraph(shape.corners)
            for graph in (subgraph.subgraph(c).copy() for c in networkx.connected_components(subgraph)):
                path = self._make_path(graph)
                if min(graph.nodes()) == min(subgraph.nodes()):
                    shape.add_outline(path, True)
                else:
                    shape.add_outline(path)

    
    def _smooth_splines(self):
        """Smoothen the BSpline curves making up the paths of the pixel cell graph"""
        for i, path in enumerate(self._paths.values()):
            print(" * %s/%s (%s, %s)..." % (i + 1, len(self._paths), len(path.shapes), len(path.path)))
            if len(path.shapes) == 1:
                path.smooth = path.spline.copy()
                continue
            path.smooth_spline()


    def _pixel_data_safe_access(self, x: int, y: int, out_of_bounds_strategy = OutOfBoundsStrategy.PANIC) -> Tuple[int, int, int]:
        """
        Access the data of the pixel at the given coordinates in a safe manner

        Args:
            x: X-coordinate of the pixel [0..(width - 1)]
            y: Y-coordinate of the pixel [0..(height - 1)]
            out_of_bounds_strategy: Strategy to adopt if the given coordinates are out of bounds

        Returns:
            RGB data as a 3-tuple
        """
        # Enforce out of bounds strategy if either coordinate is out of bounds
        if x < 0 or x >= self._width:
            if out_of_bounds_strategy is OutOfBoundsStrategy.NEAREST:
                x = clamp(x, 0, self._width - 1)
            elif out_of_bounds_strategy is OutOfBoundsStrategy.ZERO:
                return (int(0), int(0), int(0))
            elif out_of_bounds_strategy is OutOfBoundsStrategy.PANIC:
                raise RuntimeError(f"Out of bounds image access - Coords ({x}, {y}) - Image Dims ({self._width}, {self._height})")
        if y < 0 or y >= self._height:
            if out_of_bounds_strategy is OutOfBoundsStrategy.NEAREST:
                x = clamp(y, 0, self._height - 1)
            elif out_of_bounds_strategy is OutOfBoundsStrategy.ZERO:
                return (int(0), int(0), int(0))
            elif out_of_bounds_strategy is OutOfBoundsStrategy.PANIC:
                raise RuntimeError(f"Out of bounds image access - Coords ({x}, {y}) - Image Dims ({self._width}, {self._height})")

        return self._pixel_data[y][x]


    def _add_edge_if_valid(self, n0: Tuple[int, int], n1: Tuple[int, int]):
        """
        Create edge e = (n0, n1) in the similarity graph if n1 is within image
        bounds and if the pixels that n0 and n1 represent are 'similar'
        according to hqx YUV channel difference thresholds

        Args:
            n0: Starting point of edge
            n1: Ending point of edge
        """
        p0 = self._pixel_data_safe_access(n0[0], n0[1], OutOfBoundsStrategy.ZERO)
        p1 = self._pixel_data_safe_access(n1[0], n1[1], OutOfBoundsStrategy.ZERO)

        if within_bounds(n1, self._size) and pixels_similar(p0, p1):
            attrs = {"diagonal": (n0[0] != n1[0]) and (n0[1] != n1[1])}
            self._similarity_graph.add_edge(n0, n1, **attrs)

    
    def _enumerate_blocks(self, size: int):
        """
        Generate indices corresponding to all valid blocks of given size that
        exist in the image

        Args:
            size: Length of the blocks such that their final dimensions are (size x size)
        """
        for x, y in pixel_coords(self._width - size + 1, self._height - size + 1):
            yield [(x + dx, y + dy) for dx in range(size) for dy in range(size)]

    
    def _pixel_corners(self, pixel: Tuple[int, int]):
        """
        Return the vertices representing the corners of the given pixel in the similarity graph

        Args:
            pixel: (x, y) coordinate pair corresponding to the pixel
        """
        return self._similarity_graph.nodes[pixel]["corners"]


    def _deform_cell(self, node):
        """
        Deform cell based on Voronoi graph computation rules

        Args:
            node: Vertex to deform in the pixel cell graph
        """
        for neighbor in self._similarity_graph.neighbors(node):
            # Only consider diagonals
            if node[0] == neighbor[0] or node[1] == neighbor[1]:
                continue

            px_x = max(neighbor[0], node[0])
            px_y = max(neighbor[1], node[1])
            pixnode = (px_x, px_y)
            offset_x = neighbor[0] - node[0]
            offset_y = neighbor[1] - node[1]
            adj_node = (neighbor[0], node[1])

            if not self._pixel_values_equal(node, adj_node):
                pn = (px_x, px_y - offset_y)
                mpn = (px_x, px_y - 0.5 * offset_y)
                npn = (px_x + 0.25 * offset_x, px_y - 0.25 * offset_y)
                safe_remove(self._pixel_corners(adj_node), pixnode)
                self._pixel_corners(adj_node).add(npn)
                self._pixel_corners(node).add(npn)
                self._deform(pixnode, pn, mpn, npn)

            adj_node = (node[0], neighbor[1])
            if not self._pixel_values_equal(node, adj_node):
                pn = (px_x - offset_x, px_y)
                mpn = (px_x - 0.5 * offset_x, px_y)
                npn = (px_x - 0.25 * offset_x, px_y + 0.25 * offset_y)
                safe_remove(self._pixel_corners(adj_node), pixnode)
                self._pixel_corners(adj_node).add(npn)
                self._pixel_corners(node).add(npn)
                self._deform(pixnode, pn, mpn, npn)

    
    def _pixel_values_equal(self, p0_coords: Tuple[int, int], p1_coords: Tuple[int, int]) -> bool:
        return self._pixel_data_safe_access(*p0_coords) == self._pixel_data_safe_access(*p1_coords)


    def _deform(self, pixnode, pn, mpn, npn):
        if mpn in self._pixel_graph:
            self._pixel_graph.remove_edge(mpn, pixnode)
        else:
            self._pixel_graph.remove_edge(pn, pixnode)
            self._pixel_graph.add_edge(pn, mpn)
        self._pixel_graph.add_edge(mpn, npn)
        self._pixel_graph.add_edge(npn, pixnode)


    def _make_path(self, graph: networkx.Graph):
        """
        Create a path encoding the points and edges in a graph with a single connected component

        Args:
            shape_graph: Graph with only a single connected component
        """
        path = Path(graph)
        key = path.key()
        if key not in self._paths:
            self._paths[key] = path
            path.make_spline()  # and also fit a spline to the path
        return self._paths[key]
