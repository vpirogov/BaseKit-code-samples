set(DEVICE_SOURCE_FILE qrd.cpp)
set(DEVICE_HEADER_FILE qrd.hpp)
set(HOST_SOURCE_FILE qrd_demo.cpp)
set(TARGET_NAME qrd)

set(EMULATOR_TARGET ${TARGET_NAME}.fpga_emu)
set(CPU_HOST_TARGET ${TARGET_NAME}.cpu_host)
set(FPGA_TARGET ${TARGET_NAME}.fpga)
set(REPORTS_TARGET ${TARGET_NAME}_report) 

# Intel supported FPGA Boards and their names
set(A10_PAC_BOARD_NAME "intel_a10gx_pac:pac_a10")
set(S10_PAC_BOARD_NAME "intel_s10sx_pac:pac_s10")

# Design specific constant values
set(FIXED_ITERATIONS_A10 64)
set(FIXED_ITERATIONS_S10 85)
set(SEED_A10 2)
set(SEED_S10 4)

# set FPGA_BOARD value for the example design
IF (NOT DEFINED FPGA_BOARD)
    MESSAGE(STATUS "\tFPGA_BOARD was not specified, defaulting to using the ${A10_PAC_BOARD_NAME} (A10 PAC) board. Please refer to the README for more information on how to run the design on the S10 PAC board.")
    MESSAGE(STATUS "\tSetting the value of the constant FIXED_ITERATIONS to ${FIXED_ITERATIONS_A10}.")

    SET(FPGA_BOARD ${A10_PAC_BOARD_NAME})
    SET(FIXED_ITERATIONS ${FIXED_ITERATIONS_A10})
    SET(SEED ${SEED_A10})

ELSEIF(FPGA_BOARD STREQUAL ${A10_PAC_BOARD_NAME})
    MESSAGE(STATUS "\tConfiguring the design to run on the ${A10_PAC_BOARD_NAME} (A10 PAC) board.")
    MESSAGE(STATUS "\tSetting the value of the constant FIXED_ITERATIONS to ${FIXED_ITERATIONS_A10}.")

    SET(FPGA_BOARD ${A10_PAC_BOARD_NAME})
    SET(FIXED_ITERATIONS ${FIXED_ITERATIONS_A10})
    SET(SEED ${SEED_A10})

ELSEIF(FPGA_BOARD STREQUAL ${S10_PAC_BOARD_NAME})
    MESSAGE(STATUS "\tConfiguring the design to run on the ${S10_PAC_BOARD_NAME} (S10 PAC) board.")
    MESSAGE(STATUS "\tSetting the value of the constant FIXED_ITERATIONS to ${FIXED_ITERATIONS_S10}.")

    SET(FPGA_BOARD ${S10_PAC_BOARD_NAME})
    SET(FIXED_ITERATIONS ${FIXED_ITERATIONS_S10})
    SET(SEED ${SEED_S10})

ELSE()
    MESSAGE(STATUS "\tAn invalid board name was passed in using the FPGA_BOARD flag. The default ${A10_PAC_BOARD_NAME} (A10 PAC) board will be used. Please refer to the README for the list of valid board names.")
    MESSAGE(STATUS "\tSetting the value of the constant FIXED_ITERATIONS to ${FIXED_ITERATIONS_A10}.")

    SET(FPGA_BOARD ${A10_PAC_BOARD_NAME})
    SET(FIXED_ITERATIONS ${FIXED_ITERATIONS_A10})
    SET(SEED ${SEED_A10})
ENDIF()

set(HARDWARE_COMPILE_FLAGS -fintelfpga -c -DFIXED_ITERATIONS=${FIXED_ITERATIONS})

# use cmake -D USER_HARDWARE_FLAGS=<flags> to set extra flags for FPGA backend compilation
separate_arguments(USER_HARDWARE_FLAGS)
set(HARDWARE_LINK_FLAGS -fintelfpga -Xshardware -Xsno-accessor-aliasing -Xsclock=300MHz -Xsfp-relaxed -Xsparallel=2 -Xsseed=${SEED} -Xsboard=${FPGA_BOARD} ${USER_HARDWARE_FLAGS} -DFIXED_ITERATIONS=${FIXED_ITERATIONS})
set(FINAL_LINK_FLAGS -fintelfpga -DFIXED_ITERATIONS=${FIXED_ITERATIONS})

set(EMULATOR_COMPILE_FLAGS "-fintelfpga -DFPGA_EMULATOR -DFIXED_ITERATIONS=${FIXED_ITERATIONS}")
set(EMULATOR_LINK_FLAGS -fintelfpga )

set(CPU_HOST_COMPILE_FLAGS "-DCPU_HOST -DFIXED_ITERATIONS=${FIXED_ITERATIONS}")

# fpga emulator
if(WIN32)
    set(WIN_EMULATOR_TARGET ${EMULATOR_TARGET}.exe)
    add_custom_target(fpga_emu DEPENDS ${WIN_EMULATOR_TARGET})
    separate_arguments(WIN_EMULATOR_COMPILE_FLAGS WINDOWS_COMMAND "${EMULATOR_COMPILE_FLAGS}")
    add_custom_command(OUTPUT ${WIN_EMULATOR_TARGET} 
                   COMMAND ${CMAKE_CXX_COMPILER} ${WIN_EMULATOR_COMPILE_FLAGS} /GX ${CMAKE_CURRENT_SOURCE_DIR}/${DEVICE_SOURCE_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${HOST_SOURCE_FILE} -o ${CMAKE_BINARY_DIR}/${WIN_EMULATOR_TARGET}
                   DEPENDS ${DEVICE_SOURCE_FILE} ${HOST_SOURCE_FILE})
else()
    add_executable(${EMULATOR_TARGET} ${DEVICE_SOURCE_FILE} ${HOST_SOURCE_FILE})
    add_custom_target(fpga_emu DEPENDS ${EMULATOR_TARGET})
    set_target_properties(${EMULATOR_TARGET} PROPERTIES COMPILE_FLAGS ${EMULATOR_COMPILE_FLAGS})
    set_target_properties(${EMULATOR_TARGET} PROPERTIES LINK_FLAGS ${EMULATOR_LINK_FLAGS})
endif()

# cpu host
if(WIN32)
    set(WIN_HOST_TARGET ${CPU_HOST_TARGET}.exe)
    add_custom_target(cpu_host DEPENDS ${WIN_HOST_TARGET})
    separate_arguments(WIN_HOST_COMPILE_FLAGS WINDOWS_COMMAND "${CPU_HOST_COMPILE_FLAGS}")
    add_custom_command(OUTPUT ${WIN_HOST_TARGET} 
                   COMMAND ${CMAKE_CXX_COMPILER} ${WIN_HOST_COMPILE_FLAGS} /GX ${CMAKE_CURRENT_SOURCE_DIR}/${DEVICE_SOURCE_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/${HOST_SOURCE_FILE} -o ${CMAKE_BINARY_DIR}/${WIN_HOST_TARGET}
                   DEPENDS ${SOURCE_FILE})  

else()    
    add_executable(${CPU_HOST_TARGET} EXCLUDE_FROM_ALL ${DEVICE_SOURCE_FILE} ${HOST_SOURCE_FILE})
    add_custom_target(cpu_host DEPENDS ${CPU_HOST_TARGET})
    set_target_properties(${CPU_HOST_TARGET} PROPERTIES COMPILE_FLAGS ${CPU_HOST_COMPILE_FLAGS})
endif()

# fpga
if(WIN32)
    add_custom_target(fpga
                  COMMAND echo "FPGA hardware flow is not supported in Windows")
else()
    add_custom_target(fpga DEPENDS ${FPGA_TARGET})
    set(DEVICE_FPGA_OBJ "qrd_fpga.o")
    set(DEVICE_IMAGE_FPGA_OBJ "qrd_fpga.a")
    set(HOST_FPGA_OBJ "qrd_host.o")

    add_custom_command(OUTPUT ${DEVICE_FPGA_OBJ}
                       COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${HARDWARE_COMPILE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/${DEVICE_SOURCE_FILE} -o ${DEVICE_FPGA_OBJ}
                       DEPENDS ${DEVICE_SOURCE_FILE} ${DEVICE_HEADER_FILE})

    add_custom_command(OUTPUT ${HOST_FPGA_OBJ}
                       COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${HARDWARE_COMPILE_FLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/${HOST_SOURCE_FILE} -o ${HOST_FPGA_OBJ}
                       DEPENDS ${HOST_SOURCE_FILE})

    add_custom_command(OUTPUT ${DEVICE_IMAGE_FPGA_OBJ}
                       COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${HARDWARE_LINK_FLAGS} -fsycl-link=image ${DEVICE_FPGA_OBJ} -o ${DEVICE_IMAGE_FPGA_OBJ}
                       DEPENDS ${DEVICE_FPGA_OBJ})

    add_custom_command(OUTPUT ${FPGA_TARGET}
                       COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${FINAL_LINK_FLAGS} ${HOST_FPGA_OBJ} ${DEVICE_IMAGE_FPGA_OBJ} -o  ${CMAKE_BINARY_DIR}/${FPGA_TARGET}
                       DEPENDS ${DEVICE_IMAGE_FPGA_OBJ} ${HOST_FPGA_OBJ})
endif()

# fpga report
if(WIN32)
    add_custom_target(report DEPENDS ${REPORTS_TARGET} )

    separate_arguments(WIN_FLAGS WINDOWS_COMMAND)
    add_custom_command(OUTPUT ${REPORTS_TARGET}
        COMMAND ${CMAKE_CXX_COMPILER} /EHsc ${CMAKE_CXX_FLAGS} ${WIN_FLAGS} ${HARDWARE_LINK_FLAGS} -fsycl-link ${CMAKE_CURRENT_SOURCE_DIR}/${DEVICE_SOURCE_FILE} -o ${CMAKE_BINARY_DIR}/${REPORTS_TARGET}
                      DEPENDS ${DEVICE_SOURCE_FILE} ${DEVICE_HEADER_FILE})

else()
    add_custom_target(report DEPENDS ${REPORTS_TARGET} )

    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${DEVICE_SOURCE_FILE} ${DEVICE_SOURCE_FILE} COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${DEVICE_HEADER_FILE} ${DEVICE_HEADER_FILE} COPYONLY)

    add_custom_command(OUTPUT ${REPORTS_TARGET}
                      COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${HARDWARE_LINK_FLAGS} -fsycl-link ${DEVICE_SOURCE_FILE} -o ${CMAKE_BINARY_DIR}/${REPORTS_TARGET}
                      DEPENDS ${DEVICE_SOURCE_FILE} ${DEVICE_HEADER_FILE})
endif()

# run
add_custom_target(run
                  COMMAND ../${TARGET_NAME}.fpga_emu
                  DEPENDS ${TARGET_NAME}.fpga_emu)
                  
