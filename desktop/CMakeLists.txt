cmake_minimum_required(VERSION 3.21)

project(XiaomiLightBar VERSION 1.0.0 LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "This project targets Windows only.")
endif()
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Configure with -A x64 or -A ARM64.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64")
  set(WASDK_ARCH "arm64")
  set(WASDK_WIN_ARCH "win-arm64")
else()
  set(WASDK_ARCH "x64")
  set(WASDK_WIN_ARCH "win-x64")
endif()

set(PACKAGES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/packages")

function(find_pkg out_var pattern)
  file(GLOB _dirs "${PACKAGES_DIR}/${pattern}")
  list(LENGTH _dirs _n)
  if(_n EQUAL 0)
    message(FATAL_ERROR "NuGet package '${pattern}' not found in ${PACKAGES_DIR}. Run tools/restore-packages.ps1 first.")
  endif()
  list(GET _dirs 0 _dir)
  set(${out_var} "${_dir}" PARENT_SCOPE)
endfunction()

find_pkg(WASDK_WINUI "Microsoft.WindowsAppSDK.WinUI.*")
find_pkg(WASDK_FOUNDATION "Microsoft.WindowsAppSDK.Foundation.*")
find_pkg(WASDK_RUNTIME "Microsoft.WindowsAppSDK.Runtime.*")
find_pkg(WASDK_INTERACTIVE "Microsoft.WindowsAppSDK.InteractiveExperiences.*")
find_pkg(WASDK_DWRITE "Microsoft.WindowsAppSDK.DWrite.*")
find_pkg(WEBVIEW2 "Microsoft.Web.WebView2.*")

file(GLOB _sdk_bin_dirs "C:/Program Files (x86)/Windows Kits/10/bin/*/x64")
if(_sdk_bin_dirs)
  list(SORT _sdk_bin_dirs)
  list(GET _sdk_bin_dirs -1 _sdk_bin_dir)
endif()
find_program(CPPWINRT_EXE cppwinrt HINTS ${_sdk_bin_dir} REQUIRED)

set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GEN_STAMP "${GEN_DIR}/.cppwinrt.stamp")

file(GLOB WASDK_WINMDS
  "${WASDK_WINUI}/metadata/*.winmd"
  "${WASDK_FOUNDATION}/metadata/*.winmd"
  "${WASDK_INTERACTIVE}/metadata/10.0.18362.0/*.winmd")

add_custom_command(
  OUTPUT "${GEN_STAMP}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${GEN_DIR}"
  COMMAND "${CPPWINRT_EXE}"
          -input sdk
          -input "${WASDK_WINUI}/metadata"
          -input "${WASDK_INTERACTIVE}/metadata/10.0.18362.0"
          -input "${WASDK_FOUNDATION}/metadata"
          -input "${WEBVIEW2}/lib/Microsoft.Web.WebView2.Core.winmd"
          -output "${GEN_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E touch "${GEN_STAMP}"
  DEPENDS ${WASDK_WINMDS}
  COMMENT "Generating C++/WinRT projections (cppwinrt)"
  VERBATIM)

add_custom_target(cppwinrt_projection DEPENDS "${GEN_STAMP}")

enable_language(RC)
set(APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico")
file(TO_CMAKE_PATH "${APP_ICON_PATH}" APP_ICON_PATH)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/app.rc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/app.rc" @ONLY)
set(APP_RC "${CMAKE_CURRENT_BINARY_DIR}/app.rc")

add_executable(XiaomiLightBar WIN32
  src/pch.cpp
  src/main.cpp
  src/App.cpp
  src/MainWindow.cpp
  src/HomeView.cpp
  src/LightBarView.cpp
  src/SettingsView.cpp
  src/SerialLink.cpp
  src/Settings.cpp
  src/ShellIntegration.cpp
  ${APP_RC}
  "${WASDK_FOUNDATION}/include/WindowsAppRuntimeAutoInitializer.cpp"
  "${WASDK_FOUNDATION}/include/MddBootstrapAutoInitializer.cpp"
  "${WASDK_FOUNDATION}/include/UndockedRegFreeWinRT-AutoInitializer.cpp"
)

add_dependencies(XiaomiLightBar cppwinrt_projection)

target_include_directories(XiaomiLightBar PRIVATE
  "${GEN_DIR}"
  "${WASDK_WINUI}/include"
  "${WASDK_FOUNDATION}/include"
  "${WASDK_RUNTIME}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(XiaomiLightBar PRIVATE
  UNICODE
  _UNICODE
  WIN32_LEAN_AND_MEAN
  NOMINMAX
  MICROSOFT_WINDOWSAPPSDK_AUTOINITIALIZE_BOOTSTRAP
  MICROSOFT_WINDOWSAPPSDK_AUTOINITIALIZE_UNDOCKEDREGFREEWINRT
  MICROSOFT_WINDOWSAPPSDK_UNDOCKEDREGFREEWINRT_AUTO_INITIALIZE_LOADLIBRARY
)

if(MSVC)
  target_compile_options(XiaomiLightBar PRIVATE /EHsc /bigobj /utf-8)
endif()

target_link_libraries(XiaomiLightBar PRIVATE
  "${WASDK_FOUNDATION}/lib/native/${WASDK_ARCH}/Microsoft.WindowsAppRuntime.Bootstrap.lib"
  "${WASDK_FOUNDATION}/lib/native/${WASDK_ARCH}/Microsoft.WindowsAppRuntime.lib"
  delayimp.lib
  WindowsApp.lib
  runtimeobject.lib
  shell32.lib
  user32.lib
)

target_link_options(XiaomiLightBar PRIVATE
  "/DELAYLOAD:Microsoft.WindowsAppRuntime.dll"
  "/MANIFEST:EMBED"
  "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/app.manifest")

add_custom_command(TARGET XiaomiLightBar POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${WASDK_FOUNDATION}/runtimes/${WASDK_WIN_ARCH}/native/Microsoft.WindowsAppRuntime.Bootstrap.dll"
          "$<TARGET_FILE_DIR:XiaomiLightBar>"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${WASDK_WINUI}/runtimes-framework/${WASDK_WIN_ARCH}/native/Microsoft.UI.Xaml.Controls.pri"
          "$<TARGET_FILE_DIR:XiaomiLightBar>/resources.pri"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.ico"
          "$<TARGET_FILE_DIR:XiaomiLightBar>/app.ico"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/assets/app.png"
          "$<TARGET_FILE_DIR:XiaomiLightBar>/app.png"
  VERBATIM)
