################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.c 

OBJS += \
./tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.o 

C_DEPS += \
./tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.d 


# Each subdirectory must supply rules for building sources it contributes
tinyusb/src/portable/ti/msp430x5xx/%.o tinyusb/src/portable/ti/msp430x5xx/%.su tinyusb/src/portable/ti/msp430x5xx/%.cyclo: ../tinyusb/src/portable/ti/msp430x5xx/%.c tinyusb/src/portable/ti/msp430x5xx/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I"/Users/hung1fps/Embedded-System-Project/firmware/tinyusb/src" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-tinyusb-2f-src-2f-portable-2f-ti-2f-msp430x5xx

clean-tinyusb-2f-src-2f-portable-2f-ti-2f-msp430x5xx:
	-$(RM) ./tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.cyclo ./tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.d ./tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.o ./tinyusb/src/portable/ti/msp430x5xx/dcd_msp430x5xx.su

.PHONY: clean-tinyusb-2f-src-2f-portable-2f-ti-2f-msp430x5xx

