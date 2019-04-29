################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../board/peripherals.c \
../board/pin_mux.c

OBJS += \
./board/peripherals.o \
./board/pin_mux.o

C_DEPS += \
./board/peripherals.d \
./board/pin_mux.d


# Each subdirectory must supply rules for building sources it contributes
board/%.o: ../board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	@gcc -fPIC -m64 -std=gnu99 -I../drivers -I../ros/portable -I../amazon-freertos/portable -I../amazon-freertos/include -I../source -O0 -g -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
