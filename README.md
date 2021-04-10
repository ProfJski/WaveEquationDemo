# WaveEquationDemo
An intuitive approach to the Schrodinger wave equation using a classical particle, uncertainty and quantization of velocity and/or position, and a histogram.

## Background
I sometimes have occasion to teach physics concepts to liberal arts students.  The perspective can be rewarding.  Often in physics textbooks, the approach is motivated substantially by learning the considerable mathematics -- understandably so.  Yet one sometimes does not see the forest for the trees.  Learning the mathematical methods can detract from considering the principles which inform the mathematics.

Students can struggle with the notion of the Schrodinger Wave Equation as a replacement for classical (Newtonian / Lagrangian) descriptions of the state of a system.  The inspiration for this demo came from a history of science essay (sadly I've lost the citation) which remarked that Schrodinger developed his wave equation from the schoastic mechanics of thermodynamics.  Bing!  I was surprised I hadn't heard that before.  So insighful.

## What the Demo does
We represent a classical particle as a small red circle.  It has a definite velocity and position throughout the demo.  Above it, we generate a host of other classical particles in white based on our red one.  The position and/or velocity of these particles varies by some random amount from the red one.  Thus we can represent uncertainty about the red particle's precise position and velocity by a "cloud" of white particles plotted above it.  Both the red and white particles move according to the same classical dynamics.

At the botton, a histogram adds up how many particles are in a given position at a certain time.  The wave-like nature of the superposition of the classical particles can readily be seen.  Hopefully this provides an intuitive approach to the wave equation's inspiration.

Simple modifications of the code allow one to vary how the velocity and position of the white particles are arranged around the red one.  The [C++ Random Library](https://www.cplusplus.com/reference/random/) provides a variety of random distributions: uniform linear, Gaussian, etc.  One can vary whether positions and/or velocities are quantized.

## Three views of the data
The initial view shows what was described above: the red "classical particle" (hard to see in the reduced-size image), the white particle cloud, and the blue histogram of particle positions, all of which change with time.

![Position-Time view with position histogram](/images/WP1.gif)



