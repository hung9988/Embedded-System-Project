################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tinyusb/src/portable/microchip/samg/dcd_samg.c 

OBJS += \
./tinyusb/src/portable/microchip/samg/dcd_samg.o 

C_DEPS += \
./tinyusb/src/portable/microchip/samg/dcd_samg.d 


# Each subdirectory must supply rules for building sources it contributes
tinyusb/src/portable/microchip/samg/%.o tinyusb/src/portable/microchip/samg/%.su tinyusb/src/portable/microchip/samg/%.cyclo: ../tinyusb/src/portable/microchip/samg/%.c tinyusb/src/portable/microchip/samg/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/Le Hung/EmbeddedProject/firmware/tinyusb/src" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-tinyusb-2f-src-2f-portable-2f-microchip-2f-samg

clean-tinyusb-2f-src-2f-portable-2f-microchip-2f-samg:
	-$(RM) ./tinyusb/src/portable/microchip/samg/dcd_samg.cyclo ./tinyusb/src/portable/microchip/samg/dcd_samg.d ./tinyusb/src/portable/microchip/samg/dcd_samg.o ./tinyusb/src/portable/microchip/samg/dcd_samg.su

.PHONY: clean-tinyusb-2f-src-2f-portable-2f-microchip-2f-samg

