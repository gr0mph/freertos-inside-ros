################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../source/freertos_hello.c

OBJS += \
./source/freertos_hello.o

C_DEPS += \
./source/freertos_hello.d

# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	@gcc -fPIC -m64 -std=gnu99 -I../drivers -I../ros/portable -I../amazon-freertos/include -I../source -O0 -g -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '
