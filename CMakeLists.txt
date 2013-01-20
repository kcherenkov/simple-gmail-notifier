cmake_minimum_required (VERSION 2.6)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set (PROJECT simple-gmail-notifier)
set (SRC src)

project (${PROJECT})

find_package (CURL REQUIRED)
find_package (GTK2 REQUIRED)

find_package(PkgConfig)
pkg_check_modules(MXML mxml REQUIRED)
pkg_check_modules(LIBNOTIFY libnotify REQUIRED)
pkg_check_modules(LIBINDICATE indicate-0.7 REQUIRED)
pkg_check_modules(LIBINDICATE_GTK indicate-gtk-0.7 REQUIRED)
pkg_check_modules(GNOMEKEYRING gnome-keyring-1 REQUIRED)

include_directories(include ${MXML_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} ${GTK2_INCLUDE_DIR} ${LIBNOTIFY_INCLUDE_DIRS} ${LIBINDICATE_INCLUDE_DIRS} ${LIBINDICATE_GTK_INCLUDE_DIRS} ${GNOMEKEYRING_INCLUDE_DIRS})

add_library (simple-gmail-checker SHARED ${SRC}/simple-gmail-checker.c)
add_executable (${PROJECT} ${SRC}/simple-gmail-notifier.c)
add_executable (simple-gmail-notifier-config ${SRC}/simple-gmail-notifier-config.c)

target_link_libraries(simple-gmail-checker ${MXML_LIBRARIES} ${CURL_LIBRARIES})
target_link_libraries(${PROJECT} simple-gmail-checker ${LIBNOTIFY_LIBRARIES} ${LIBINDICATE_LIBRARIES} ${LIBINDICATE_GTK_LIBRARIES} ${GNOMEKEYRING_LIBRARIES})
target_link_libraries(${PROJECT}-config simple-gmail-checker ${GTK2_LIBRARIES} ${GNOMEKEYRING_LIBRARIES})

INSTALL(TARGETS ${PROJECT} ${PROJECT}-config simple-gmail-checker
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib)
INSTALL(FILES data/simple-gmail-notifier.desktop
  DESTINATION share/applications)
INSTALL(FILES data/config.glade
  DESTINATION share/simple-gmail-notifier)
INSTALL(FILES data/simple-gmail-notifier.indicator
  DESTINATION share/indicators/messages/applications)

#SET(CPACK_PACKAGE_NAME "Simple Gmail Notifier")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Simple Gmail Notifier checks your gmail and displays notifications")
SET(CPACK_PACKAGE_VENDOR "Konstantin Cherenkov")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/debian/copyright")
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "1")
SET(CPACK_PACKAGE_VERSION_PATCH "9")
#SET(CPACK_PACKAGE_INSTALL_DIRECTORY "usr")
SET(CPACK_PACKAGE_EXECUTABLES "simple-gmail-notifier" "Simple Gmail Notifier")

SET(CPACK_GENERATOR "DEB")
IF( CMAKE_SIZEOF_VOID_P EQUAL 8 )
	SET(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
ENDIF( CMAKE_SIZEOF_VOID_P EQUAL 8 )
SET(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}_${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
SET (CPACK_OUTPUT_FILE_PREFIX "../..")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Konstantin Cherenkov <k.cherenkov@gmail.com>")
SET(CPACK_DEBIAN_PACKAGE_SECTION "Mail")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "curl, libxml2, libindicate-gtk3, gnome-keyring")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/prerm;")

INCLUDE(CPack)
