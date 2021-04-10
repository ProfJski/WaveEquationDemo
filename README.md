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

The system below randomizes position based on a continuous Gaussian distribution around the red particle and randomizes velocity using a uniform distribution quantized into 8 distinct velocities, giving 8 cohorts of bell-curve distributed particles, only one of which corresponds to the red particle.

![Position-Time view with position histogram](/images/WP1.gif)

**Press H** to see a multi-column chart of the position histogram as it has evolved with time.  This more compact view uses brightness of color (from dark red to bright red, then yellow and white) to represent the number of particles in one place at one time.  Every frame of the demo adds a line to this page.  If all columns are filled, additional pages are plotted and you can **press "S" and "W"** to navigate between them.  Each page indicates the timestamp of the starting and ending frame on the left, so you can later zero in on interesting moments in the system.

**Press N** to see a fancy 3D scrolling histogram which basically represents the same data as the previous one, but we make use of the extra dimension to represent the count of particles by height once more.  The display loops through all histogram data.

![Histogram views for all times](/images/WP2.gif)

**Press J** to return to the first view.  

You can also control the frame rate.  **Press +** increase it by 10 FPS.  **Press -** to decrease by 10.  **Press 0** for no frame rate throttling -- it runs as fast as your machine can go.  This mode evolves the system quickly and is useful to amass histogram data as below.

![Frame rate control](/images/WP3.gif)

The frame rate for the fancy 3D histogram is unfortunately slow, so don't expect much more than 30 FPS for that view.

## OpenCL kernels
Two are provided.  The sinusoidal one is useful because simple harmonic motion is intuitive and a common textbook example.  The "basic" one is a starting point to craft one's own mechanics equations.  To keep the particle within our domain, its default force = -kx^3.

OpenCL helps if you want to render a large number of particles.  Since they all move independently of each other, they can be updated in parallel.  If one doesn't want OpenCL, the kernel functions can simply be replaced by C++ functions, albeit slower.

## 
