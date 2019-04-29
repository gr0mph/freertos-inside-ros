################################################################################
# Github from gr0mph
#	https://github.com/gr0mph
# Makefile to generate hello_freertos
################################################################################

# All of the sources participating in the build are defined here
-include sources.mk
-include source/subdir.mk
-include ros/portable/subdir.mk
-include amazon-freertos/portable/subdir.mk
-include amazon-freertos/FreeRTOS/subdir.mk
-include subdir.mk
-include objects.mk

all: libfreertos.so

# Tool invocations
libfreertos.so: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	gcc -shared $(OBJS) $(USER_OBJS) $(LIBS) -o "libfreertos.so"
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) libfreertos.so
	-@echo ' '
