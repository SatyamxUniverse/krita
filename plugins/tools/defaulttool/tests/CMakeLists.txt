include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..
                    ${CMAKE_SOURCE_DIR}/sdk/tests
                    ${CMAKE_BINARY_DIR}/plugins/tools/defaulttool)



macro_add_unittest_definitions()

########### next target ###############

set(KisBooleanOperationsTestSrc KisBooleanOperationsTest.cpp )
ecm_add_tests(${KisBooleanOperationsTestSrc}
NAME_PREFIX "plugins-tools-defaulttool-"
LINK_LIBRARIES defaulttool Qt5::Test)

########### next target ###############

set(KisIntersectionFinderTestSrc KisIntersectionFinderTest.cpp )
ecm_add_tests(${KisIntersectionFinderTestSrc}
NAME_PREFIX "plugins-tools-defaulttool-"
LINK_LIBRARIES defaulttool Qt5::Test)

########### next target ###############

set(KisNumericalEngineTestSrc KisNumericalEngineTest.cpp )
ecm_add_tests(${KisNumericalEngineTestSrc}
NAME_PREFIX "plugins-tools-defaulttool-"
LINK_LIBRARIES defaulttool Qt5::Test)

