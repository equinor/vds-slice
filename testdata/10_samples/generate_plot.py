import matplotlib.pyplot as plt
import numpy as np

origin = np.array([19, -32])
il_step = np.array([3, 2])
xl_step = np.array([-2, 3])


def draw_axis_and_tics(axis):

    # Plot axis
    axis.plot([-3, 22], [0, 0], color='k', linewidth=1.5)
    axis.plot([0, 0], [-33, 20], color='k', linewidth=1.5)

    # Plot tics
    x_tics = np.arange(-3, 25, 1)
    y_tics = np.arange(-33, 12, 1)
    axis.set_xticks(x_tics)
    axis.set_yticks(y_tics)
    axis.set_xlim([-0.5, 14.5])
    axis.set_ylim([-0.5, 11.8])
    axis.grid(True)


def draw_graph(axis):

    vertices = [
        [2, 0, "(1,10)"],
        [0, 3, "(1,11)"],
        [8, 4, "(3,10)"],
        [6, 7, "(3,11)"],
        [14, 8, "(5,10)"],
        [12, 11, "(5,11)"]
    ]

    edges = [
        [vertices[0], vertices[2]],
        [vertices[2], vertices[4]],
        [vertices[1], vertices[3]],
        [vertices[3], vertices[5]],
        [vertices[0], vertices[1]],
        [vertices[2], vertices[3]],
        [vertices[4], vertices[5]],
    ]

    # Plot edges
    for e in edges:
        e_s = e[0]
        e_e = e[1]
        axis.plot([e_s[0], e_e[0]], [e_s[1], e_e[1]],
                  color='grey', linewidth=3.0)

    # Plot vertices
    for v in vertices:
        axis.scatter(v[0], v[1], c="g",
                     label=v[2],
                     alpha=1.0, edgecolors='none')
        # axis.text(v[0] + 0.0, v[1] + 0.25, v[2], size="large", rotation=33.69)
        axis.text(v[0] - 0.25, v[1] + 0.25, v[2], size="large", rotation=33.69)

    axis.text(1, 10, "Annotated coordinates:\n (InLine, CrossLine)")

    # Draw an arc representing the rotation
    angles = np.linspace(0 * np.pi, 2 * np.pi / 360 * 33.69, 100)
    xs = np.cos(angles) * 4 + 2
    ys = np.sin(angles) * 4
    axis.plot(xs, ys, color='green', linewidth=1.5)
    axis.text(6.5, 0.5, r'$\theta$=33.69°')


def draw_step_arrows(axis, x_pos, y_pos):

    axis.arrow(x_pos, y_pos, il_step[0], il_step[1], color='black',
               head_width=0.3, length_includes_head=True)
    axis.arrow(x_pos, y_pos, xl_step[0], xl_step[1], color='black',
               head_width=0.3, length_includes_head=True)
    axis.text(x_pos - 1.8, y_pos, "CrossLine step",
              size="large", rotation=33.69-90)
    axis.text(x_pos + 0.4, y_pos-0.1, "InLine step",
              size="large", rotation=33.69)


def draw_origin_path(axis):
    axis.text(-1, -1.3, r'Inline', rotation=33.69)
    axis.text(7.76, -16.3, r'Crossline', rotation=33.69 - 90)
    axis.text(19, -32.8, r'Origin', size="large")
    axis.text(19.2, -32, r'(0, 0)', size="large")

    x_pos = 14
    y_pos = -13
    axis.arrow(x_pos, y_pos, il_step[0], il_step[1], color='black',
               head_width=0.3, length_includes_head=True)
    axis.arrow(x_pos, y_pos, xl_step[0], xl_step[1], color='black',
               head_width=0.3, length_includes_head=True)
    axis.text(x_pos - 3.0, y_pos-1.0, "CrossLine step", rotation=33.69-90)
    axis.text(x_pos + 0.4, y_pos-0.5, "InLine step", rotation=33.69)

    for i in range(11):
        pos = origin + xl_step*i
        axis.scatter(pos[0], pos[1], color='grey', s=15)

    for i in range(10):
        pos = origin + xl_step*i
        axis.arrow(pos[0], pos[1], xl_step[0], xl_step[1], color='grey',
                   head_width=0.3, length_includes_head=True)

    axis.arrow(-1, -2, il_step[0], il_step[1], color='grey',
               head_width=0.3, length_includes_head=True)


if __name__ == "__main__":

    fig_s, ax_s = plt.subplots(1, 1, figsize=(10, 8))
    draw_axis_and_tics(ax_s)
    draw_graph(ax_s)
    draw_step_arrows(ax_s, 11, 1.0)
    ax_s.set_title("10_samples_default")
    ax_s.set_xlabel("CDP X")
    ax_s.set_ylabel("CDP Y")

    plt.savefig("10_samples_default.png")

    fig_o, ax_o = plt.subplots(1, 1, figsize=(5*1.5, 9*1.5))
    draw_axis_and_tics(ax_o)
    draw_graph(ax_o)
    draw_origin_path(ax_o)

    ax_o.set_xlim([-3, 22])
    ax_o.set_ylim([-33, 12])
    ax_o.set_title("10_samples_default")
    ax_o.set_xlabel("CDP X")
    ax_o.set_ylabel("CDP Y")
    plt.savefig("10_samples_default_origin.png")
