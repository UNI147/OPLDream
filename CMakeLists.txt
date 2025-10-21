cmake_minimum_required(VERSION 3.10)
project(OPLDream)

# Настройка компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск PortAudio
find_package(portaudio CONFIG REQUIRED)

# Включение директорий
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Добавление исходных файлов
set(SOURCES
    src/main.cpp
    src/OPL3Emulator.cpp
    src/OPL3Driver.cpp
    src/MidiFileParser.cpp
)

# Создание исполняемого файла
add_executable(OPLDream ${SOURCES})

# Подключение библиотек
target_link_libraries(OPLDream PRIVATE portaudio)

# Настройки компиляции для Windows
if(WIN32)
    target_link_libraries(OPLDream PRIVATE winmm)
    
    # Настройки компиляции для MSVC
    if(MSVC)
        target_compile_options(OPLDream PRIVATE /W4)
    else()
        # Для других компиляторов (GCC, Clang)
        target_compile_options(OPLDream PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    # Для Linux/MacOS
    target_compile_options(OPLDream PRIVATE -Wall -Wextra -Wpedantic)
endif()