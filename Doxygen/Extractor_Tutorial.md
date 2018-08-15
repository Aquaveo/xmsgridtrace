\tableofcontents
# Extractor Tutorial {#Extractor_Tutorial}

## Introduction {#Intro_Extractor}
The purpose of this tutorial is to provide explanation on how to use the classes defined in xmsextractor to extract data at given locations on an XmUGrid with scalar values. The examples provided in this tutorial refer to test cases that are in the xmsextractor/extractor/XmUGrid2dDataExtractor.cpp and xmsextractor/extractor/XmUGrid2dPolylineDataExtractor.cpp source files.

## Example - Simple Location Extractor {#Example_Simple_Extractor}
This is the "hello world" example for using the extractor library.

This first example shows how to extract data from a simple XmUGrid. The testing code for this example is in XmUGrid2dDataExtractorUnitTests::testCellScalarsOnly. A picture of the example is shown below. Notice that the UGrid is a simple square from (0,0) to (1,1) split into two triangle cells. The scalar data is mapped to the cells.

![Simple XmUGrid with 2 Triangle Cells](images/Simple_Location_Tutorial.png)

The basic steps to extract interpolated locations from an XmUGrid and scalar values is as follows:
1. Create an extractor for an existing XmUGrid (call xms::XmUGrid2dDataExtractor::New).
2. Set scalar and activity values (call xms::XmUGrid2dDataExtractor::SetGridCellScalars or XmUGrid2dDataExtractor::SetPointCellScalars).
3. Set extract locations (call xms::XmUGrid2dDataExtractor::SetExtractLocations).
4. Extract the data (call xms::XmUGrid2dDataExtractor::ExtractData). Values that are outside of the UGrid are returned as XM_NODATA by default. The value for no data can be set using xms::XmUGrid2dDataExtractor::SetNoDataValue.

\snippet xmsextractor/extractor/XmUGrid2dDataExtractor.cpp snip_test_Example_SimpleLocationExtractor

## Example - Transient Location Extractor {#Example_Transient_Extractor}
This example shows how to use a location extractor on transient data.

This example shows how to extract data from a XmUGrid that has transient scalar data. The testing code for this example is in XmUGrid2dDataExtractorUnitTests::testTutorial. A picture of the example is shown below. Notice that the UGrid is a 2x3 structured grid with quadrillateral cells. There are two time steps with scalar data mapped to the points.

![XmUGrid With Extract Locations - Time Step 1](images/Transient_Location_Tutorial1.png)
![XmUGrid With Extract Locations - Time Step 2](images/Transient_Location_Tutorial2.png)

The steps to extract interpolated locations for transient scalar values include:
1. Create an extractor for an XmUGrid (call xms::XmUGrid2dDataExtractor::New).
2. Set extract locations (call xms::XmUGrid2dDataExtractor::SetExtractLocations).
3. Optionally set the "no data" value for output interpolated values (xms::XmUGrid2dDataExtractor::SetNoDataValue).
4. Set the point scalars for the first time step (xms::XmUGrid2dDataExtractor::SetGridPointScalars).
5. Extract the data (call xms::XmUGrid2dDataExtractor::ExtractData).
6. Continue using steps 4 and 5 for remaining time steps.

\snippet xmsextractor/extractor/XmUGrid2dDataExtractor.cpp snip_test_Example_TransientLocationExtractor

## Example - Transient Polyline Extractor {#Example_Polyline_Extractor}
This example shows how to use a polyline extractor on transient data.

This example shows how to extract data along a polyline from a XmUGrid that has transient scalar data. The testing code for this example is in XmUGrid2dPolylineDataExtractorUnitTests::testTransientTutorial. A picture of the example is shown below. Notice that the UGrid is a 2x3 structured grid with quadrillateral cells. There are two time steps with scalar data mapped to the points.

![XmUGrid With Extract Polyline - Time Step 1](images/Transient_Polyline_Tutorial1.png)

The steps to extract interpolated values along a polyline for transient scalar values include:
1. Create an extractor for an XmUGrid giving the mapped location of the scalar values (call xms::XmUGrid2dPolylineDataExtractor::New).
2. Optionally set the "no data" value. (call xms::XmUGrid2dPolylineDataExtractor::SetNoDataValue).
3. Set the polyline to be extracted along (call xms::XmUGrid2dPolylineDataExtractor::SetPolyline).
4. Optionally get the locations used for extraction along the polyline (call xms::XmUGrid2dPolylineDataExtractor::GetExtractLocations).
5. Set the point scalars for the first time step (call xms::XmUGrid2dPolylineDataExtractor::SetGridScalars).
6. Extract the data (call xms::XmUGrid2dPolylineDataExtractor::ExtractData)
7. Continue using steps 5 and 6 for remaining time steps.

\snippet xmsextractor/extractor/XmUGrid2dPolylineDataExtractor.cpp snip_test_Example_TransientPolylineExtractor

![XmUGrid With Extract Locations Along Polyline - Time Step 2](images/Transient_Polyline_Tutorial3.png)
