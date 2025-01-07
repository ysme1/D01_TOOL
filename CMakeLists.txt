cmake_minimum_required(VERSION 3.5)

project(D01_TOOL VERSION 0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/.cmake/)
include(GetGitRevisionDescription)

#配置通用编译
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

if (APPLE)
    set(APPLICATION_DIR_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/MacOS)
else ()
    set(APPLICATION_DIR_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif ()

#add_definitions(-DQT_ANGLE_PLATFORM)

set(QT_VERSION_MAJOR 6.4)

#导入Qt相关依赖包
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Quick Svg Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Quick Svg Network)

#添加国际化脚本
find_program(QT_LUPDATE NAMES lupdate lupdate-qt6)
find_program(QT_LRELEASE NAMES lrelease lrelease-qt6)
file(GLOB TS_FILE_PATHS ${CMAKE_CURRENT_LIST_DIR}/ *.ts)
add_custom_target(Script-UpdateTranslations
        COMMAND ${QT_LUPDATE} ${CMAKE_CURRENT_LIST_DIR} -ts ${PROJECT_NAME}_en_US.ts WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND ${QT_LUPDATE} ${CMAKE_CURRENT_LIST_DIR} -ts ${PROJECT_NAME}_zh_CN.ts WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND ${QT_LRELEASE} ${PROJECT_NAME}_en_US.ts WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND ${QT_LRELEASE} ${PROJECT_NAME}_zh_CN.ts WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${APPLICATION_DIR_PATH}/i18n
        COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}_en_US.qm ${PROJECT_NAME}_zh_CN.qm ${APPLICATION_DIR_PATH}/i18n
        SOURCES ${TS_FILE_PATHS}
)

##生成版本信息头文件
set(HEADER_FILE_VERSION_PATH ${CMAKE_SOURCE_DIR}/Version.h)
configure_file(
        ${CMAKE_SOURCE_DIR}/.cmake/Version.h.in
        ${HEADER_FILE_VERSION_PATH}
)

#遍历所有Cpp文件
file(GLOB_RECURSE CPP_FILES *.cpp *.h *.ui *.hpp)
foreach (filepath ${CPP_FILES})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" filename ${filepath})
    list(APPEND sources_files ${filename})
endforeach (filepath)

if (WIN32)
    list(APPEND sources_files "src/app_dmp.h")
endif ()

#如果是Windows平台，则生成rc文件，还有inno setup脚本文件
set(EXAMPLE_VERSION_RC_PATH "")
if (WIN32)
    set(EXAMPLE_VERSION_RC_PATH ${CMAKE_CURRENT_BINARY_DIR}/version_${PROJECT_NAME}.rc)
    configure_file(
            ${CMAKE_SOURCE_DIR}/.cmake/version_exe.rc.in
            ${EXAMPLE_VERSION_RC_PATH}
    )
    configure_file(
            ${CMAKE_SOURCE_DIR}/.cmake/InstallerScript.iss.in
            ${CMAKE_SOURCE_DIR}/package/InstallerScript.iss
    )
endif ()

#加快qrc编译
qt_add_big_resources(QRC_RESOURCES ${PROJECT_NAME}.qrc)
list(APPEND QRC_RESOURCES ${PROJECT_NAME}.qrc)
set_property(SOURCE ${PROJECT_NAME}.qrc PROPERTY SKIP_AUTORCC ON)
list(APPEND sources_files ${QRC_RESOURCES})

#添加可执行文件
if (WIN32)
    list(APPEND sources_files ${EXAMPLE_VERSION_RC_PATH})
endif ()
if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
            MANUAL_FINALIZATION
            ${sources_files}
    )
else ()
    add_executable(${PROJECT_NAME}
            ${sources_files}
    )
endif ()
add_dependencies(${PROJECT_NAME} Script-UpdateTranslations)

#导入头文件
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libusb
        ${CMAKE_CURRENT_SOURCE_DIR}/src/component
)

#设置属性
set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER my.${PROJECT_NAME}.com
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

#Release也支持日志打印代码位置
target_compile_definitions(${PROJECT_NAME}
        PRIVATE
        QT_MESSAGELOGCONTEXT
)

#目标文件链接库
file(GLOB_RECURSE LIB_FILES ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/*.lib)
foreach (filepath ${LIB_FILES})
    list(APPEND lib_files ${filepath})
endforeach (filepath)

list(APPEND lib_files
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${lib_files}
)

#复制程序运行所需要的动态库
if (WIN32)
    if (MSVC)
        if (CMAKE_SIZEOF_VOID_P EQUAL 4)
            file(GLOB_RECURSE 3RDPARTY_DLL_DIR ${CMAKE_SOURCE_DIR}/3rdparty/msvc/x86/*.dll)
        elseif (CMAKE_SIZEOF_VOID_P EQUAL 8)
            file(GLOB_RECURSE 3RDPARTY_DLL_DIR ${CMAKE_SOURCE_DIR}/3rdparty/msvc/x64/*.dll)
        endif ()
    elseif (MINGW)
        file(GLOB_RECURSE 3RDPARTY_DLL_DIR ${CMAKE_SOURCE_DIR}/3rdparty/mingw/*.dll)
    endif ()
    file(COPY ${3RDPARTY_DLL_DIR} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif ()

#添加部署脚本
if (CMAKE_BUILD_TYPE MATCHES "Release")
    if (APPLE)
        find_program(QT_DEPLOY_QT NAMES macdeployqt)
        add_custom_target(Script-DeployRelease
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${QT_DEPLOY_QT} ${CMAKE_SOURCE_DIR}/dist/${PROJECT_NAME}.app -qmldir=${CMAKE_CURRENT_LIST_DIR}
                COMMENT "MacOs Deploying Qt Dependencies After Build........."
                SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif ()
    if (WIN32)
        find_program(QT_DEPLOY_QT NAMES windeployqt)
        add_custom_target(Script-DeployRelease
                COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} ${CMAKE_SOURCE_DIR}/dist
                COMMAND ${QT_DEPLOY_QT} --qmldir=${CMAKE_CURRENT_LIST_DIR} --plugindir ${CMAKE_SOURCE_DIR}/dist/plugins --no-translations --compiler-runtime ${CMAKE_SOURCE_DIR}/dist/${PROJECT_NAME}.exe
                COMMENT "Windows Deploying Qt Dependencies After Build........."
                SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif ()
endif ()
