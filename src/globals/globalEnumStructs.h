#ifndef GLOBALENUMSTRUCTS_H
#define GLOBALENUMSTRUCTS_H

// Include the appropriate NIDAQmx.h based on whether it's a cross-compiled environment or not
#include "../config.h"
#ifdef CrossCompiled
  #include <NIDAQmx.h>
#else
  #include "../../DAQMX_INCLUDE/NIDAQmx.h"
#endif

// Enum for module I/O types: INPUT or OUTPUT
enum class ModuleIo {
    INPUT = 0,
    OUTPUT = 1
};

// Enum for types of modules, including various analog, digital, counter, and coder types
enum ModuleType
{
    errorOrMissingModule   =-1,          
    isAnalogicInputCurrent = 0,
    isAnalogicInputVoltage = 1,
    isDigitalInput         = 2,
    isDigitalOutput        = 3,
    isCounter              = 4,
    isCoder                = 5 
};

// Enum for shunt locations in a module, including internal and external locations
enum moduleShuntLocation
{
    noShunt          =  0,                    
    defaultLocation  = -1,  //default
    internalLocation =  DAQmx_Val_Internal,  //10200  Internal
    externalLocation =  DAQmx_Val_External   // 10167  External
};

// Enum for terminal configurations in a module, including single-ended and differential types
enum moduleTerminalConfig
{
    noTerminalConfig          =  0,
    defaultCfg                = -1,
    referencedSingleEnded     = DAQmx_Val_RSE,  //10083  RSE
    nonReferencedSingleEnded  = DAQmx_Val_NRSE, //10078  NRSE
    differencial              = DAQmx_Val_Diff, //10106  Differential
    pseudoDifferencial        = DAQmx_Val_PseudoDiff //12529  Pseudodifferential
};

// Enum for counter edge configurations in a module, including rising and falling edges
enum moduleCounterEdgeConfig
{
    NoEdge      =  0, 
    Val_Rising  = DAQmx_Val_Rising,  //10280 the counter will count uprising fronts
    Val_Falling = DAQmx_Val_Falling  //10171 the counter will count downfalling fronts
};

// Enum for counter modes in a module, including count up, count down, and externally controlled
enum moduleCounterMode
{
    NoCountMode         =   0,
    Val_CountUp         =   DAQmx_Val_CountUp,   //10128  Count Up (Counter ++)
    Val_CountDown       =   DAQmx_Val_CountDown, //10124  Count Down (Counter --)
    Val_ExtControlled   =   DAQmx_Val_ExtControlled //10326  Externally Controlled
};


// Enum for units of measurement in a module, covering a wide range of physical units
enum moduleUnit
{
     // Various measurement units used in modules
    // Each unit is mapped to a specific DAQmx_Val_* constant


    NoUnit                    = 0, // Represents no specific unit.
    Val_Volts                 = DAQmx_Val_Volts, // Represents voltage in volts.
    Val_Amps                  = DAQmx_Val_Amps, // Represents current in amperes.
    Val_DegF                  = DAQmx_Val_DegF, // Represents temperature in degrees Fahrenheit.
    Val_DegC                  = DAQmx_Val_DegC, // Represents temperature in degrees Celsius.
    Val_DegR                  = DAQmx_Val_DegR, // Represents temperature in degrees Rankine.
    Val_Kelvins               = DAQmx_Val_Kelvins, // Represents temperature in kelvins.
    Val_Strain                = DAQmx_Val_Strain, // Represents strain (dimensionless).
    Val_Ohms                  = DAQmx_Val_Ohms, // Represents electrical resistance in ohms.
    Val_Hz                    = DAQmx_Val_Hz, // Represents frequency in hertz.
    Val_Seconds               = DAQmx_Val_Seconds, // Represents time in seconds.
    Val_Meters                = DAQmx_Val_Meters, // Represents length in meters.
    Val_Inches                = DAQmx_Val_Inches, // Represents length in inches.
    Val_Degrees               = DAQmx_Val_Degrees, // Represents angular measurement in degrees.
    Val_Radians               = DAQmx_Val_Radians, // Represents angular measurement in radians.
    Val_Ticks                 = DAQmx_Val_Ticks, // Represents time in ticks.
    Val_RPM                   = DAQmx_Val_RPM, // Represents rotational speed in revolutions per minute.
    Val_RadiansPerSecond      = DAQmx_Val_RadiansPerSecond, // Represents angular velocity in radians per second.
    Val_DegreesPerSecond      = DAQmx_Val_DegreesPerSecond, // Represents angular velocity in degrees per second.
    Val_g                     = DAQmx_Val_g, // Represents acceleration in g-forces.
    Val_MetersPerSecondSquared= DAQmx_Val_MetersPerSecondSquared, // Represents acceleration in meters per second squared.
    Val_InchesPerSecondSquared= DAQmx_Val_InchesPerSecondSquared, // Represents acceleration in inches per second squared.
    Val_MetersPerSecond       = DAQmx_Val_MetersPerSecond, // Represents velocity in meters per second.
    Val_InchesPerSecond       = DAQmx_Val_InchesPerSecond, // Represents velocity in inches per second.
    Val_Pascals               = DAQmx_Val_Pascals, // Represents pressure in pascals.
    Val_Newtons               = DAQmx_Val_Newtons, // Represents force in newtons.
    Val_Pounds                = DAQmx_Val_Pounds, // Represents weight in pounds.
    Val_KilogramForce         = DAQmx_Val_KilogramForce, // Represents force in kilogram-force.
    Val_PoundsPerSquareInch   = DAQmx_Val_PoundsPerSquareInch, // Represents pressure in pounds per square inch.
    Val_Bar                   = DAQmx_Val_Bar, // Represents pressure in bar.
    Val_NewtonMeters          = DAQmx_Val_NewtonMeters, // Represents torque in newton-meters.
    Val_InchOunces            = DAQmx_Val_InchOunces,   // Represents torque in inch-ounces.
    Val_InchPounds            = DAQmx_Val_InchPounds,   // Represents torque in inch-pounds.
    Val_FootPounds            = DAQmx_Val_FootPounds,   // Represents torque in foot-pounds.
    Val_VoltsPerVolt          = DAQmx_Val_VoltsPerVolt, // Represents dimensionless ratio for voltage.
    Val_mVoltsPerVolt         = DAQmx_Val_mVoltsPerVolt, // Represents dimensionless ratio for millivoltage.
    Val_Coulombs              = DAQmx_Val_Coulombs,     // Represents electric charge in coulombs.
    Val_PicoCoulombs          = DAQmx_Val_PicoCoulombs, // Represents electric charge in picocoulombs.
    Val_FromTEDS              = DAQmx_Val_FromTEDS,     // Represents units derived from TEDS (Transducer Electronic Data Sheet).
    // Additional units can be added here as per the requirements of the DAQ system.
};

// Struct for defining a configuration for mapping data between different systems or formats
struct MappingConfig {
    int index;                // Index or identifier for this particular mapping configuration
    ModuleType moduleType;    // Type of module involved in this mapping (e.g., analog input, digital output)
    std::string module;       // Identifier for the module, could be a name or unique identifier
    std::string channel;      // Channel identifier within the module
    float minSource;          // Minimum value expected from the source (e.g., sensor reading)
    float maxSource;          // Maximum value expected from the source
    uint16_t minDest;         // Minimum value for the destination after mapping (e.g., in a control system or display)
    uint16_t maxDest;         // Maximum value for the destination after mapping
    int modbusChannel;        // Modbus channel number, if applicable (used in industrial communication protocols)
 //These are only for counters tracking, so they are not initialized by the csv files
    std::chrono::time_point<std::chrono::steady_clock> currentTime;  //when asking for counters this variable will be filled to help for frequency calculation
    std::chrono::time_point<std::chrono::steady_clock> previousTime; //when asking for counters this variable will keep track of the previous time , then we can calculate delta time
    unsigned int currentCounterValue; //when asking for counters this variable will be filled with the current counter value
    unsigned int previousCounterValue; //when asking for counters this variable will keep track of the previous counter , then we can calculate delta count between 2 acquisitions
    // Constructor to initialize a MappingConfig object with default values.
    // Sets default module type to analog input current, and initializes all numerical fields to zero.
    MappingConfig() : index         (0                                 ), //Initialize the index  
                      moduleType    (ModuleType::isAnalogicInputCurrent), //Initialize the type of module (analogic, digital, relays and so on)
                      minSource     (0.0f                              ), //Initialize the 4 parameters for linear in terpolation
                      maxSource     (0.0f                              ),
                      minDest       (0                                 ),
                      maxDest       (0                                 ),
                      modbusChannel (0                                 ), //Initialize the destination channel in modbus
                      currentTime(std::chrono::steady_clock::now()), // Initialize to current time
                      previousTime(std::chrono::steady_clock::now()), // Initialize to current time, will be updated on first use
                      currentCounterValue(0), // Initialize to zero, will be updated on first read
                      previousCounterValue(0) // Initialize to zero, will provide delta on update
                      {}
    };

    struct AlarmsMappingConfig
    {
        int index;
        const ModuleType moduleType = ModuleType::isDigitalOutput;
        std::string module;
        std::string alarmRole;
        std::string channel;
        int modbusCoilsChannel;
        AlarmsMappingConfig() : index              (0               ), //Initialize the index 
                                module             ("Mod6"          ), //alarms are on Mod 6 by defaults
                                alarmRole          ("Buzzer"        ), //first alarm in sru by default
                                channel            ("/port0/line0"  ), //destination side of the mapping
                                modbusCoilsChannel (0               )  //modubus side of the mapping 
                                {}  
    };

    struct GlobalFileNamesContainer
    {
        std::string newModbusServerLogFile  ;
        std::string niDeviceModuleLogFile   ;
        std::string iniObjectLogFile        ;
        std::string startupManagerLogFile   ;
        std::string niToModbusBridgeLogFile ;
        std::string CrioSSLServerLogFile    ;
        std::string digitalReaderLogFile    ;
        std::string QNiDaqWrapperLogFile    ;
        std::string DigitalWriterLogFile    ;
        std::string modbusIniFile           ;
        std::string modbusMappingFile       ;
        std::string modbusAlarmsMappingFile ;  
        GlobalFileNamesContainer() : newModbusServerLogFile  ("./newModbusServerLogFile.txt"  ) ,
                                     niDeviceModuleLogFile   ("./niDeviceModuleLogFile.txt"   ) ,
                                     iniObjectLogFile        ("./iniObjectLogFile.txt"        ) ,
                                     startupManagerLogFile   ("./startupManagerLogFile.txt"   ) ,
                                     niToModbusBridgeLogFile ("./niToModbusBridgeLogFile.txt" ) ,
                                     CrioSSLServerLogFile    ("./CrioSSLServerLogFile.txt"    ) ,
                                     digitalReaderLogFile    ("./digitalReaderLogFile.txt"    ) ,
                                     QNiDaqWrapperLogFile    ("./QNiDaqWrapperLogFile.txt"    ) ,
                                     DigitalWriterLogFile    ("DigitalWriterLogFile.txt"      ) ,
                                     modbusIniFile           ("./modbus.ini"                  ) ,
                                     modbusMappingFile       ("./mapping.csv"                 ) ,
                                     modbusAlarmsMappingFile ("./alarmsMapping.csv"           ){}
    };


#endif // GLOBALENUMSTRUCTS_H