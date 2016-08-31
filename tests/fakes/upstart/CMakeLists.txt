set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS}
                    "${CMAKE_BINARY_DIR}/tests/fakes/upstart")

add_definitions(-DQT_NO_KEYWORDS=1)

set(upstart_interface_xml "com.ubuntu.Upstart0_6.xml")
set(upstart_job_interface_xml "com.ubuntu.Upstart0_6.Job.xml")
set(upstart_instance_interface_xml "com.ubuntu.Upstart0_6.Instance.xml")

set_source_files_properties(${upstart_interface_xml}
                            PROPERTIES
                              NO_NAMESPACE YES
                              CLASSNAME UpstartMockInterface)

set_source_files_properties(${upstart_job_interface_xml}
                            PROPERTIES
                              NO_NAMESPACE YES
                              CLASSNAME UpstartJobMockInterface)

set_source_files_properties(${upstart_instance_interface_xml}
                            PROPERTIES
                              NO_NAMESPACE YES
                              CLASSNAME UpstartInstanceMockInterface)

qt5_add_dbus_interface(interface_files ${upstart_interface_xml} UpstartMockInterface)
qt5_add_dbus_interface(interface_files ${upstart_job_interface_xml} UpstartJobMockInterface)
qt5_add_dbus_interface(interface_files ${upstart_instance_interface_xml} UpstartInstanceMockInterface)

qt5_add_dbus_adaptor(adaptor_files
    ${upstart_interface_xml}
    upstart-mock.h
    testing::UpstartMock
    UpstartMockAdaptor
    UpstartMockAdaptor)

qt5_add_dbus_adaptor(adaptor_files
    ${upstart_job_interface_xml}
    upstart-job-mock.h
    testing::UpstartJobMock
    UpstartJobMockAdaptor
    UpstartJobMockAdaptor)

qt5_add_dbus_adaptor(adaptor_files
    ${upstart_instance_interface_xml}
    upstart-instance-mock.h
    testing::UpstartInstanceMock
    UpstartInstanceMockAdaptor
    UpstartInstanceMockAdaptor)

add_executable(
  upstart-mock
  ${adaptor_files}
  ${interface_files}
  upstart-instance-mock.cpp
  upstart-job-mock.cpp
  upstart-mock.cpp
  main.cpp
)

qt5_use_modules(
    upstart-mock
    Core
    DBus
)
