################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \
../ros/portable/heap_1.c \
../ros/portable/port.c

OBJS += \
./ros/portable/heap_1.o \
./ros/portable/port.o

C_DEPS += \
./ros/portable/heap_1.d \
./ros/portable/port.d


# Each subdirectory must supply rules for building sources it contributes
ros/portable/%.o: ../ros/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	@gcc -fPIC -m64 -std=gnu99 -I../drivers -I../amazon-freertos/portable -I../amazon-freertos/include -I../ros/portable  -I../source -O0 -g -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

# -I../../redlib/include -I../../redlib/include/sys -D__REDLIB__
# -nostdinc
