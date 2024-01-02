import matplotlib.pyplot as plt
import numpy as np


class Axis:

    def __init__(self, name, offset, step_size) -> None:
        self.name = name
        self.offset = offset
        self.step_size = step_size

    def from_index(self, index):
        return self.offset + index * self.step_size


class Coordinate:

    def __init__(self, x, y) -> None:
        self.x = x
        self.y = y

    def __str__(self) -> str:
        return f"({round(self.x, 2)}, {round(self.y, 2)})"


class Grid:

    def __init__(self, xori, yori, xinc, yinc, rotation) -> None:
        self.xori = xori
        self.yori = yori
        self.xinc = xinc
        self.yinc = yinc
        self.radian_rotation = rotation / 360 * 2 * np.pi

    def index_to_utm(self, x, y):

        # Using Euler's formula
        complex_rotation = np.power(
            np.e, 1j*self.radian_rotation) * (x*self.xinc + y*1j*self.yinc)
        return Coordinate(self.xori + complex_rotation.real, self.yori + complex_rotation.imag)


class Vertex:
    def __init__(self, in_line, x_line, samples, utm_grid, i_v, j_v, data_values):
        self.in_line = in_line
        self.x_line = x_line
        self.samples = samples
        self.grid = utm_grid
        self.i = i_v
        self.j = j_v
        self.data = data_values

    @property
    def depths(self):
        return range(self.samples.offset,
                     (len(self.data) + 1) * self.samples.step_size,
                     self.samples.step_size)

    @property
    def index_coordinates(self):
        return Coordinate(self.i, self.j)

    @property
    def annotated_coordinates(self):
        return Coordinate(self.in_line.from_index(self.i), self.x_line.from_index(self.j))

    @property
    def utm_coordinates(self):
        return self.grid.index_to_utm(self.i, self.j)

    def sample_at_depth(self, depth_value):
        index = int((depth_value-self.samples.offset)/self.samples.step_size)
        return self.data[index]

    def coordinate_str(self, coordinate_type):
        if coordinate_type == "Index":
            coord_str = self.index_coordinates.__str__()
        elif coordinate_type == "Annotated":
            coord_str = self.annotated_coordinates.__str__()
        elif coordinate_type == "UTM":
            coord_str = self.utm_coordinates.__str__()
        else:
            coord_str = ""
        return coord_str


class Edge:

    def __init__(self, e_u: Vertex, e_v: Vertex) -> None:
        self.u = e_u
        self.v = e_v


data = np.array([
    # Traces linear in range [0, len(trace)]
    #  4       8     12     16     20     24     28     32     36     40
    [-4.5,  -3.5,  -2.5,  -1.5,  -0.5,   0.5,   1.5,   2.5,   3.5,   4.5],
    [4.5,   3.5,   2.5,   1.5,   0.5,  -0.5,  -1.5,  -2.5,  -3.5,  -4.5],
    # Traces linear in range [1:-1]
    [25.5, -14.5, -12.5, -10.5,  -8.5,  -6.5,  -4.5,  -2.5,  -0.5,  25.5],
    [25.5,   0.5,   2.5,   4.5,   6.5,   8.5,  10.5,  12.5,  14.5,  25.5],
    # Traces linear in range [1:6] and [6:len(trace)]
    [25.5,   4.5,   8.5,  12.5,  16.5,  20.5,  24.5,  20.5,  16.5,   8.5],
    [25.5,  -4.5,  -8.5, -12.5, -16.5, -20.5, -24.5, -20.5, -16.5,  -8.5],])
data = data.reshape([2, 3, 10])

axes = [Axis("Inline", 1, 2),
        Axis("Xline", 10, 1),
        Axis("Samples", 4, 4)]

x_inc = np.sqrt((8 - 2) * (8 - 2) + 4 * 4)  # ~ 7.2111
y_inc = np.sqrt((-2) * (-2) + 3 * 3)       # ~ 3.6056
angle = np.arcsin(4 / np.sqrt((8 - 2) * (8 - 2) + 4 * 4)) / \
    (2 * np.pi) * 360  # ~ 33.69

grid = Grid(xori=2.0, yori=0.0, xinc=x_inc, yinc=y_inc, rotation=angle)

vertices = []
for i in range(3):
    for j in range(2):
        vertices.append(
            Vertex(axes[0], axes[1], axes[2], grid, i, j, data[j, i]))

edges = [
    Edge(vertices[0], vertices[2]),
    Edge(vertices[2], vertices[4]),
    Edge(vertices[1], vertices[3]),
    Edge(vertices[3], vertices[5]),
    Edge(vertices[0], vertices[1]),
    Edge(vertices[2], vertices[3]),
    Edge(vertices[4], vertices[5])
]

if __name__ == "__main__":

    title_text = f"Visualization of: 10_samples_default"

    plot_vertices = True
    vertex_color = "blue"
    vertex_size = 15

    plot_edges = True
    edge_color = "green"

    plot_data_values = True
    text_offset_z = -1

    coordinate_types = ["Index", "Annotated", "UTM", None]
    plot_coordinates = coordinate_types[3]

    fig = plt.figure(figsize=(18, 12))
    fig.canvas.manager.set_window_title('10_samples_default')
    ax = fig.add_subplot(projection='3d')

    for v in vertices:
        for depth in v.depths:

            if plot_vertices:
                ax.scatter(v.utm_coordinates.x, v.utm_coordinates.y, depth, s=vertex_size **
                           2, marker=".", color=vertex_color)

            if plot_edges:
                for e in edges:
                    ax.plot([e.u.utm_coordinates.x, e.v.utm_coordinates.x],
                            [e.u.utm_coordinates.y, e.v.utm_coordinates.y],
                            [depth, depth],
                            color=edge_color)
                    if depth != v.samples.offset:
                        ax.plot([v.utm_coordinates.x, v.utm_coordinates.x],
                                [v.utm_coordinates.y, v.utm_coordinates.y],
                                [depth, depth-v.samples.step_size],
                                color=edge_color)

            if plot_coordinates and plot_data_values:

                coord_str = v.coordinate_str(plot_coordinates)
                data_value_str = v.sample_at_depth(depth).__str__()
                ax.text(v.utm_coordinates.x, v.utm_coordinates.y, depth + text_offset_z,
                        coord_str + "\n" + data_value_str,
                        size="small", multialignment='center', verticalalignment='center')
            elif plot_coordinates:
                coord_str = v.coordinate_str(plot_coordinates)
                ax.text(v.utm_coordinates.x, v.utm_coordinates.y, depth + text_offset_z,
                        coord_str,
                        size="small", multialignment='center', verticalalignment='center')
            elif plot_data_values:
                value = v.sample_at_depth(depth)
                if value < 0:
                    ax.text(v.utm_coordinates.x, v.utm_coordinates.y,
                            depth + text_offset_z, value, color="red")
                else:
                    ax.text(v.utm_coordinates.x, v.utm_coordinates.y,
                            depth + text_offset_z, value, color="blue")


ax.set_xlabel('X coordinate')
ax.set_ylabel('Y coordinate')
ax.set_zlabel('Depth')
ax.invert_zaxis()
ax.view_init(20, -45, 0)

plt.title(title_text)

plt.show()
