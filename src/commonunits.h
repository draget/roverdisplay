#ifndef COMMONUNITS_H
#define COMMONUNITS_H

#include <stdint.h>
#include <stdbool.h>

typedef enum SpeedUnits
{
  MPH,
  KPH
} SpeedUnits;

typedef enum TemperatureUnits
{
  Fahrenheit,
  Celsius
} TemperatureUnits;

typedef enum SampleType
{
  SampleType_EngineTemperature,
  SampleType_RoadSpeed,
  SampleType_EngineRPM,
  SampleType_FuelTemperature,
  SampleType_MAF,
  SampleType_Throttle,
  SampleType_IdleBypassPosition,
  SampleType_TargetIdleRPM,
  SampleType_GearSelection,
  SampleType_MainVoltage,
  SampleType_LambdaTrimShort,
  SampleType_LambdaTrimLong,
  SampleType_COTrimVoltage,
  SampleType_FuelPumpRelay,
  SampleType_FuelMapRowCol,
  SampleType_FuelMapData,
  SampleType_FuelMapIndex,
  SampleType_InjectorPulseWidth,
  SampleType_MIL,
  SampleType_NumSampleTypes
} SampleType;

#endif // COMMONUNITS_H
