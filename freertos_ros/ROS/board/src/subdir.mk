################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../board/src/board.c \
../board/src/clock_config.c

OBJS += \
./board/src/board.o \
./board/src/clock_config.o

C_DEPS += \
./board/src/board.d \
./board/src/clock_config.d


# Each subdirectory must supply rules for building sources it contributes
board/src/%.o: ../board/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	@gcc -fPIC -m64 -std=gnu99 -I../drivers -I../ros/portable -I../amazon-freertos/portable -I../amazon-freertos/include -I../source -O0 -g -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
