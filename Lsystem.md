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
| float | radiusscale | 0.9 | scale the radius. |

## Symbols
- `=`: rule delimiter. E.g. `F=F+XF`.
- `(`: push current state to stack.
- `)`: pop state from stack.
- `>`: Move forward one step and draw a tube. 
- `~`: Move forward one step without drawing a tube. 
- `'`: Scale the radius of tube. 

## Polygon
- `{`: Indicate start of polygon.
- `}`: Indicate end of polygon.
- `@`: add current position as vertex in polygon

### Rotations
The turtle initially points upward ($z$-axis) with its right hand side on 
$x$-axis and heading $y$-axis. Thus we define the following three vertices to 
track the orientation of the turtle: 
- `U`: heading, initially $(0, 1, 0)$.
- `F`: front, initially $(0, 0, 1)$
- `R`: right, initially $(1, 0, 0)$

Thus we have the following 7 rotations:
- `+`: roll clockwise around $$-axis. $R_R(\delta)$. 
- `-`: roll counterclockwise around $x$-axis. $R_R(-\delta)$. 
- `&`: roll clockwise around $y$-axis. $R_U(\delta)$. 
- `^`: roll counterclockwise around $y$-axis. $R_U(-\delta)$. 
- `` ` ``: roll clockwise around $z$-axis. $R_F(\delta)$. 
- `/`: roll counterclockwise around $z$-axis. $R_F(-\delta)$. 
- `|`: turn around, i.e. roll 180 degress around $x$-axis. $R_R(180)$