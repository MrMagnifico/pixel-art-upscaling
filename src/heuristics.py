import networkx
from utils import *

class Heuristics():

    def __init__(self, similarity_graph: networkx.Graph):
        self._similarity_graph          = similarity_graph

        self._SPARSE_WINDOW_DIMS        = (8, 8) # Dimensions must be even
        self._ISLAND_HEURISTIC_WEIGHT   = 5

    
    def apply_heuristics(self, ambiguous_diagonal_pairs):
        """
        Compute heuristics for all given pairs as in Section 3.2 and use them
        to decide which diagonal(s) in each pair to remove

        Args:
            ambiguous_diagonal_pairs: All pairs of diagonals to resolve
        """
        # Compute weights
        for edge_pair in ambiguous_diagonal_pairs:
            self._compute_weights(edge_pair)

        # Remove edge(s) based on weights
        for edge_pair in ambiguous_diagonal_pairs:
            min_weight = min(edge[2]["weight"] for edge in edge_pair)
            for edge in edge_pair:
                # Remove edge(s) with minimum weight
                if edge[2]["weight"] == min_weight:
                    self._similarity_graph.remove_edge(edge[0], edge[1])
                # If we didn't remove the edge, remove the 'weight' attribute to minimise bookkeeping
                else:
                    edge[2].pop("weight")

    
    def _compute_weights(self, edge_pair):
        """
        Compute weights for both edges in the pair

        Args:
            edge_pair: Pair of edges to compute heuristics weights for
        """
        for edge in edge_pair:
            self._compute_single_weight(edge)

    
    def _compute_single_weight(self, edge):
        """
        Compute weight for a single edge and assign it as an attribute
        in the edge's attribute dict

        Args:
            edge: Edge to compute heuristics weights for
        """
        weights = sum([
            self._weight_curve(edge),
            self._weight_sparse(edge),
            self._weight_island(edge)])
        edge[2]['weight'] = weights

    
    def _weight_curve(self, edge) -> int:
        """
        Compute the 'Curves' heuristic value directly, as opposed to being
        a difference between the values computed for the two edges being compared

        Args:
            edge: Edge to compute heuristic weights for

        Returns:
            Value of the 'Curves' heuristic
        """
        edges_in_curve = set(sorted_vertices(edge))
        nodes_in_curve = list(edge[:2])

        # Search through all vertices in the path defining the curve
        while nodes_in_curve:
            node = nodes_in_curve.pop()
            connected_edges = self._similarity_graph.edges(node, data=True)

            # Not a valence-2 node, can't be part of a curve
            if len(connected_edges) != 2:
                continue

            # Explore edge if it has not already been seen
            for con_edge in connected_edges:
                con_edge = sorted_vertices(con_edge)
                if con_edge not in edges_in_curve:
                    edges_in_curve.add(con_edge)
                    nodes_in_curve.extend(other_node for other_node in con_edge if other_node != node)

        # Weight is the number of edges along the curve/path
        return len(edges_in_curve) 

    
    def _weight_sparse(self, edge) -> int:
        """
        Compute the 'Sparse pixels' heuristic value directly as the negative of
        the size of the component connected to the edge, as opposed to being
        a difference between the values computed for the two edges being compared

        Args:
            edge: Edge to compute heuristic weights for

        Returns:
            Value of the 'Sparse pixels' heuristic
        """
        nodes = list(edge[:2])
        nodes_in_sparse_window = set(nodes)
        offset = self._sparse_offset(edge)

        # Search through all vertices connected to the nodes defining the given edge that lie in the sampling window
        while nodes:
            node = nodes.pop()
            for neighbour_node in self._similarity_graph.neighbors(node):
                if neighbour_node in nodes_in_sparse_window:
                    continue
                if within_bounds(neighbour_node, self._SPARSE_WINDOW_DIMS, offset=offset):
                    nodes_in_sparse_window.add(neighbour_node)
                    nodes.append(neighbour_node)

        # Weight is the negative of the connected vertices in the sampling window
        return -len(nodes_in_sparse_window)


    def _weight_island(self, edge) -> int:
        """
        Compute the 'Island' heuristic value

        Args:
            edge: Edge to compute heuristic weights for

        Returns:
            Value of the 'Island' heuristic
        """
        # If either of the nodes forming the edge is valence-1, cutting off the
        # diagonal creates a visually disconnected island, which is penalized
        if len(self._similarity_graph[edge[0]]) == 1 or len(self._similarity_graph[edge[1]]) == 1:
            return self._ISLAND_HEURISTIC_WEIGHT
        return 0


    def _sparse_offset(self, edge) -> Tuple[int, int]:
        """
        Compute the offset required to place the vertices composing the given edge in the center
        of the sparse heuristic window

        Args:
            edge: Edge whose vertices should be offset

        Returns:
            Offset required to position the vertices in the window range
        """
        return (self._SPARSE_WINDOW_DIMS[0] / 2 - 1 - min((edge[0][0], edge[1][0])),
                self._SPARSE_WINDOW_DIMS[1] / 2 - 1 - min((edge[0][1], edge[1][1])))
