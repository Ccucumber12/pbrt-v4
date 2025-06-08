# L-System In PBRT-V4


## Parameters

| Type | Name | Default Value | Description |
|------|------|---------------|-------------|
| string[n] | rules | (None) | Rules of the L-system.  |
| string | axiom | (None) | Axiom of the L-system.  |
| float | radius | 0.1 | Base radius of the cylinder tube. |
| float | stepsize | 1.0 | Base Length of one step. |
| float | angle | 28.0 | Rotate angle $\delta$. |
| int | n | 3 | Number of generations. |

## Symbols
- `=`: rule delimiter. E.g. `F=F+XF`.
- `[`: push current state to stack.
- `]`: pop state from stack.

### Movement
- `F`: Move forward one step and draw a tube. 

### Rotations
- `+`: roll clockwise around $x$-axis. $R_x(\delta)$. 
- `-`: roll counterclockwise around $x$-axis. $R_x(-\delta)$. 
- `&`: roll clockwise around $y$-axis. $R_y(\delta)$. 
- `^`: roll counterclockwise around $y$-axis. $R_y(-\delta)$. 
- `\`: roll clockwise around $z$-axis. $R_z(\delta)$. 
- `/`: roll counterclockwise around $z$-axis. $R_z(-\delta)$. 