################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tinyusb/src/class/cdc/cdc_device.c \
../tinyusb/src/class/cdc/cdc_host.c \
../tinyusb/src/class/cdc/cdc_rndis_host.c 

OBJS += \
./tinyusb/src/class/cdc/cdc_device.o \
./tinyusb/src/class/cdc/cdc_host.o \
./tinyusb/src/class/cdc/cdc_rndis_host.o 

C_DEPS += \
./tinyusb/src/class/cdc/cdc_device.d \
./tinyusb/src/class/cdc/cdc_host.d \
./tinyusb/src/class/cdc/cdc_rndis_host.d 


# Each subdirectory must supply rules for building sources it contributes
tinyusb/src/class/cdc/%.o tinyusb/src/class/cdc/%.su tinyusb/src/class/cdc/%.cyclo: ../tinyusb/src/class/cdc/%.c tinyusb/src/class/cdc/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Le Hung/EmbeddedProject/firmware/tinyusb/src" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-tinyusb-2f-src-2f-class-2f-cdc

clean-tinyusb-2f-src-2f-class-2f-cdc:
	-$(RM) ./tinyusb/src/class/cdc/cdc_device.cyclo ./tinyusb/src/class/cdc/cdc_device.d ./tinyusb/src/class/cdc/cdc_device.o ./tinyusb/src/class/cdc/cdc_device.su ./tinyusb/src/class/cdc/cdc_host.cyclo ./tinyusb/src/class/cdc/cdc_host.d ./tinyusb/src/class/cdc/cdc_host.o ./tinyusb/src/class/cdc/cdc_host.su ./tinyusb/src/class/cdc/cdc_rndis_host.cyclo ./tinyusb/src/class/cdc/cdc_rndis_host.d ./tinyusb/src/class/cdc/cdc_rndis_host.o ./tinyusb/src/class/cdc/cdc_rndis_host.su

.PHONY: clean-tinyusb-2f-src-2f-class-2f-cdc

