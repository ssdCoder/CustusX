project(org_custusx_registration_method_landmark)

set(PLUGIN_export_directive "${PROJECT_NAME}_EXPORT")

set(PLUGIN_SRCS
    cxRegistrationMethodLandmarkPluginActivator.cpp
    cxRegistrationMethodLandmarkService.h
    cxRegistrationMethodLandmarkService.cpp
    cxImageLandmarksWidget.cpp
    cxLandmarkRegistrationWidget.cpp
    cxLandmarkPatientRegistrationWidget.cpp
    cxLandmarkImage2ImageRegistrationWidget.cpp
    cxLandmarkListener.cpp
    cxPatientLandMarksWidget.cpp
    fast/cxFastImageRegistrationWidget.cpp
    fast/cxFastPatientRegistrationWidget.cpp
    fast/cxFastOrientationRegistrationWidget.cpp
	fast/cxFastLandmarkPatientRegistrationWidget.cpp
	fast/cxFastLandmarkPatientRegistrationWidget.h
)

# Files which should be processed by Qts moc
set(PLUGIN_MOC_SRCS
    cxRegistrationMethodLandmarkPluginActivator.h
    cxLandmarkRegistrationWidget.h
    cxImageLandmarksWidget.h
    cxLandmarkPatientRegistrationWidget.h
    cxLandmarkImage2ImageRegistrationWidget.h
    fast/cxFastPatientRegistrationWidget.h
    fast/cxFastImageRegistrationWidget.h
    fast/cxFastOrientationRegistrationWidget.h
    cxLandmarkListener.h
    cxPatientLandMarksWidget.h
)


cx_create_export_header(${PROJECT_NAME})

# Qt Designer files which should be processed by Qts uic
set(PLUGIN_UI_FORMS
)

# QRC Files which should be compiled into the plugin
set(PLUGIN_resources
)

#Compute the plugin dependencies
ctkFunctionGetTargetLibraries(PLUGIN_target_libraries)
set(PLUGIN_target_libraries 
    ${PLUGIN_target_libraries}   
    cxResourceWidgets
	org_custusx_registration
    )

set(PLUGIN_OUTPUT_DIR "")
if(CX_WINDOWS)
    #on windows we want dlls to be placed with the executables
    set(PLUGIN_OUTPUT_DIR "../")
endif(CX_WINDOWS)

ctkMacroBuildPlugin(
  NAME ${PROJECT_NAME}
  EXPORT_DIRECTIVE ${PLUGIN_export_directive}
  SRCS ${PLUGIN_SRCS}
  MOC_SRCS ${PLUGIN_MOC_SRCS}
  UI_FORMS ${PLUGIN_UI_FORMS}
  RESOURCES ${PLUGIN_resources}
  TARGET_LIBRARIES ${PLUGIN_target_libraries}
  OUTPUT_DIR ${PLUGIN_OUTPUT_DIR}
  ${CX_CTK_PLUGIN_NO_INSTALL}
)

target_include_directories(org_custusx_registration_method_landmark
    PUBLIC
    .
    ./fast
    ${CMAKE_CURRENT_BINARY_DIR}
)

cx_doc_define_plugin_user_docs("${PROJECT_NAME}" "${CMAKE_CURRENT_SOURCE_DIR}/doc")
cx_add_non_source_file("doc/org_custusx_registration_method_landmark.md")
cx_add_non_source_file("doc/org_custusx_registration_method_landmark.h")
cx_add_non_source_file("doc/org_custusx_registration_method_landmark_image_to_image.md")
cx_add_non_source_file("doc/org_custusx_registration_method_landmark_image_to_patient.md")
cx_add_non_source_file("doc/org_custusx_registration_method_fast_landmark_image_to_patient.md")

add_subdirectory(testing)
