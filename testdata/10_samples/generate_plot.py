import matplotlib.pyplot as plt
import numpy as np

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

fig, ax = plt.subplots(1, 1, figsize=(10, 8))

# Plot axis
ax.plot([-1, 15], [0, 0], color='k', linewidth=1.5)
ax.plot([0, 0], [-1, 12], color='k', linewidth=1.5)


# Plot edges
for e in edges:
    e_s = e[0]
    e_e = e[1]
    ax.plot([e_s[0], e_e[0]], [e_s[1], e_e[1]], color='grey', linewidth=3.0)

# Plot vertices
for v in vertices:
    ax.scatter(v[0], v[1], c="g",
               label=v[2],
               alpha=1.0, edgecolors='none')
    ax.text(v[0] + 0.0, v[1] + 0.25, v[2], size="large")

x_tics = np.arange(-1, 15, 1)
y_tics = np.arange(-1, 12, 1)
plt.xticks(x_tics)
plt.yticks(y_tics)
plt.xlim([-0.5, 14.5])
plt.ylim([-0.5, 11.8])
ax.grid(True)

# draw a circle
angles = np.linspace(0 * np.pi, 2 * np.pi / 360 * 33.69, 100)
xs = np.cos(angles) * 2 + 2
ys = np.sin(angles) * 2
plt.plot(xs, ys, color='green', linewidth=1.5)
plt.text(4, 0.5, r'$\theta$=33.69°')
plt.title("10_samples_default")
plt.xlabel("X")
plt.ylabel("Y")
plt.savefig("10_samples_default.png")
plt.show()
