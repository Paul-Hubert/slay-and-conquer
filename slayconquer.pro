SHADERS = $$files(./src/renderer/shaders/*.glsl, true)

defineReplace(shadersOutput) {
    return(build/shaders/$$basename(1).spv)
}

spirv.output_function = shadersOutput
spirv.commands = glslangValidator -V ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
spirv.depends = $$SHADERS
spirv.input = SHADERS
spirv.variable_out = COMPILED_SHADERS
spirv.CONFIG = target_predeps


SOURCES = $$files(src/*.cpp, true)
SOURCES += $$files(3rdparty/imgui/*.cpp, true)

HEADERS = $$files(src/*.h, true)
HEADERS += $$files(3rdparty/imgui/*.h, true)

# install
target.path = build
target.depends = spirv

DESTDIR = bin #Target file directory
OBJECTS_DIR = build #Intermediate object files directory
MOC_DIR = build #Intermediate moc files directory


CONFIG += c++1z # entt a besoin de c++17
QMAKE_EXTRA_COMPILERS += spirv

CONFIG += vulkan
CONFIG += exceptions
CONFIG += qt debug
WARNINGS += -Wno-class-memaccess -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-variable
QMAKE_CXXFLAGS_WARN_ON += $(and $(filter-out moc_% qrc_%, $@),$${WARNINGS})

INCLUDEPATH += include
INCLUDEPATH += $$(VULKAN_SDK)/include
INCLUDEPATH += $$(VULKAN_SDK)/Include
INCLUDEPATH += 3rdparty
INCLUDEPATH += src

LIBS += "3rdparty/volk/libvolk.a"

linux {
    LIBS += -ldl
}

QT += widgets
QT += network
QT += multimedia
QT += testlib

RESOURCES = slayandconquer.qrc
