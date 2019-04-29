################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables
C_SRCS += \

OBJS += \

C_DEPS += \


# Each subdirectory must supply rules for building sources it contributes
amazon-freertos/FreeRTOS/portable/%.o: ../amazon-freertos/FreeRTOS/portable/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	@gcc -fPIC -m64 -std=gnu99 -I../drivers -I../ros/portable  -I../amazon-freertos/portable -I../amazon-freertos/include -I../source -O0 -g -Wall -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

# -I../../redlib/include -I../../redlib/include/sys -D__REDLIB__
# -nostdinc
