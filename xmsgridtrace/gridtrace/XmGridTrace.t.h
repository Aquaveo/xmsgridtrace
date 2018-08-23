#pragma once
//------------------------------------------------------------------------------
/// \file
/// \ingroup GridTrace
/// \copyright (C) Copyright Aquaveo 2018.
//------------------------------------------------------------------------------

#ifdef CXX_TEST

// 3. Standard Library Headers

// 4. External Library Headers
#include <cxxtest/TestSuite.h>

// 5. Shared Headers

// 6. Non-shared Headers

////////////////////////////////////////////////////////////////////////////////
class XmGridTraceUnitTests : public CxxTest::TestSuite
{
public:
  void testBasicTracePoint();
  void testMaxChangeDistance();
  void testSmallScalarsTracePoint();
  void testStrongDirectionChange();
  void testMaxTracingTime();
  void testMaxTracingDistance();
  void testStartOutOfCell();
  void testDotProduct();
  void testBeyondTimestep();
  void testBeforeTimestep();
  void testVectorMultiplier();
  void testMultiCell();
  void testMaxChangeVelocity();
  void testUniqueTimeSteps();
  void testInactiveCell();
  void testStartInactiveCell();
  void testTutorial();

}; // XmGridTraceUnitTests

#endif
