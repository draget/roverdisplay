#ifndef CUXINTERFACE_H
#define CUXINTERFACE_H

#include "comm14cux.h"
#include "commonunits.h"

#define FUEL_MAP_COUNT 6
#define FUEL_MAP_REFRESH false

c14cux_info cuxinfo;

typedef enum read_result {
	readresult_success,
	readresult_failure,
	readresult_nostatement
	} read_result;

enum c14cux_lambda_trim_type m_lambdaTrimType;
enum c14cux_feedback_mode m_feedbackMode;
enum c14cux_airflow_type m_airflowType;
enum c14cux_throttle_pos_type m_throttlePosType;

typedef struct ecu_data {
	bool m_rpmLimitRead;
	uint8_t m_roadSpeedMPH;
	uint16_t m_engineSpeedRPM;
	uint16_t m_targetIdleSpeed;
	int16_t m_coolantTempF;
	int16_t m_fuelTempF;
	float m_throttlePos;
	enum c14cux_gear m_gear;
	float m_mainVoltage;
	bool m_fuelMapIndexRead;
	uint8_t m_currentFuelMapIndex;
	uint8_t m_currentFuelMapRowIndex;
	uint8_t m_fuelMapRowWeighting;
	uint8_t m_currentFuelMapColumnIndex;
	uint8_t m_fuelMapColWeighting;
	float m_mafReading;
	float m_idleBypassPos;
	bool m_fuelPumpRelayOn;
	int16_t m_lambdaTrimOdd;
	int16_t m_lambdaTrimEven;
	float m_coTrimVoltage;
	bool m_milOn;
	uint16_t m_rpmLimit;
	bool m_idleMode;
	uint16_t m_injectorPulseWidthUs;
	float m_injectorPulseWidthMs;
	uint16_t m_tune;
	uint8_t m_checksumFixer;
	uint16_t m_ident;
	uint8_t m_rowScaler[FUEL_MAP_COUNT];
	uint16_t m_mafScaler;
	c14cux_faultcodes m_faultCodes;
	} ecu_data;

extern bool connect_to_ecu(ecu_data* dat, const char* dev);
extern void disconnect_from_ecu();
extern read_result read_data(ecu_data* dat);
extern read_result read_fault_codes(ecu_data* dat);
extern unsigned int convertSpeed(unsigned int speedMph, int speedUnits);
extern int convertTemperature(int tempF, int tempUnits);

#endif
