################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tinyusb/src/portable/dialog/da146xx/dcd_da146xx.c 

OBJS += \
./tinyusb/src/portable/dialog/da146xx/dcd_da146xx.o 

C_DEPS += \
./tinyusb/src/portable/dialog/da146xx/dcd_da146xx.d 


# Each subdirectory must supply rules for building sources it contributes
tinyusb/src/portable/dialog/da146xx/%.o tinyusb/src/portable/dialog/da146xx/%.su tinyusb/src/portable/dialog/da146xx/%.cyclo: ../tinyusb/src/portable/dialog/da146xx/%.c tinyusb/src/portable/dialog/da146xx/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Le Hung/EmbeddedProject/firmware/tinyusb/src" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-tinyusb-2f-src-2f-portable-2f-dialog-2f-da146xx

clean-tinyusb-2f-src-2f-portable-2f-dialog-2f-da146xx:
	-$(RM) ./tinyusb/src/portable/dialog/da146xx/dcd_da146xx.cyclo ./tinyusb/src/portable/dialog/da146xx/dcd_da146xx.d ./tinyusb/src/portable/dialog/da146xx/dcd_da146xx.o ./tinyusb/src/portable/dialog/da146xx/dcd_da146xx.su

.PHONY: clean-tinyusb-2f-src-2f-portable-2f-dialog-2f-da146xx

