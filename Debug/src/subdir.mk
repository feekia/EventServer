################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/CommandLine.cpp \
../src/Controller.cpp \
../src/Helloword.cpp \
../src/Server.cpp \
../src/ServerSocket.cpp 

OBJS += \
./src/CommandLine.o \
./src/Controller.o \
./src/Helloword.o \
./src/Server.o \
./src/ServerSocket.o 

CPP_DEPS += \
./src/CommandLine.d \
./src/Controller.d \
./src/Helloword.d \
./src/Server.d \
./src/ServerSocket.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


