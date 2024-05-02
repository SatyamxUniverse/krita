include(KritaAddBrokenUnitTest)

kis_add_test(
    KisQoiTest.cpp
    TEST_NAME KisQoiTest
    LINK_LIBRARIES kritaui kritatestsdk
    NAME_PREFIX "plugins-impex-"
    )
