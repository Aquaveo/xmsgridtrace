#pragma once
//------------------------------------------------------------------------------
/// \file
/// \ingroup extractor
/// \copyright (C) Copyright Aquaveo 2018.
//------------------------------------------------------------------------------

#ifdef CXX_TEST

// 3. Standard Library Headers

// 4. External Library Headers
#include <cxxtest/TestSuite.h>

// 5. Shared Headers
#include <xmscore/misc/base_macros.h>
#include <xmscore/misc/boost_defines.h>
#include <xmscore/points/pt.h>

// 6. Non-shared Headers

////////////////////////////////////////////////////////////////////////////////
class XmUGrid2dDataExtractorUnitTests : public CxxTest::TestSuite
{
public:
  void testPointScalarsOnly();
  void testPointScalarCellActivity();
  void testPointScalarPointActivity();
  void testInvalidPointScalarsAndActivitySize();

  void testCellScalarsOnly();
  void testCellScalarCellActivity();
  void testCellScalarCellActivityIdw();
  void testCellScalarPointActivity();
  void testInvalidCellScalarsAndActivitySize();

  void testChangingScalarsAndActivity();

  void testCopiedExtractor();

  void testTutorial();
}; // XmUGrid2dDataExtractorUnitTests

#endif
