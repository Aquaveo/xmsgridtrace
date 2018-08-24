\tableofcontents
# Gridtrace Tutorial {#Gridtrace_Tutorial}

## Introduction {#Intro_Gridtrace}
The purpose of this tutorial is to provide explanation on how to use the class defined in xmsgridtrace to trace a given point through a velocity vector field grid. The example provided in this tutorial refers to a test case that is in the xmsgridtrace/gridtrace/XmGridTrace.cpp source file.

## Example - Simple Gridtrace {#Example_Simple_Gridtrace}
This is the "hello world" example for using the gridtrace library.

This example shows how to trace a point through a simple grid. The testing code for this example is in XmGridTraceUnitTests::testTutorial. A picture of the example is shown below. Notice that the UGrid is 4 simple squares from (0,0) to (2,2) with each square having a side length of 1. The velocity vectors are mapped to the points.

![Simple XmUGrid with 4 Cells and 9 Velocity Vectors](images/Simple_Gridtrace_Tutorial.png)

The basic steps to trace a point through a velocity vector field are as follows:
1. Create the grid
2. Create the tracer from the grid
3. Set up the constraints on the tracer
4. Set up the velocity vectors for both time steps. Insert timesteps sequentially
5. Trace the point

\snippet xmsgridtrace/gridtrace/XmGridTrace.cpp snip_test_Example_XmGridTrace
