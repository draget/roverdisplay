#include <string.h>
#include <sys/time.h>
#include "cuxinterface.h"

static struct timeval now;

static int lastReadTime[19];
static int readIntervals[19];

bool is_sample_appropriate_for_mode(SampleType type);
read_result merge_result(read_result total, bool single);
uint64_t ms_since_epoch();

bool connect_to_ecu(ecu_data* dat, const char* dev) {

	dat->m_roadSpeedMPH = 0;
	dat->m_engineSpeedRPM = 0;
	dat->m_targetIdleSpeed = 0;
	dat->m_coolantTempF = 0;
	dat->m_fuelTempF = 0;
	dat->m_throttlePos = 0.0;
	dat->m_gear = 0;
	dat->m_mainVoltage = 0.0;
	dat->m_fuelMapIndexRead = false;
	dat->m_currentFuelMapIndex = 0;
	dat->m_currentFuelMapRowIndex = 0;
	dat->m_fuelMapRowWeighting = 0;
	dat->m_currentFuelMapColumnIndex = 0;
	dat->m_fuelMapColWeighting = 0;
	dat->m_mafReading = 0.0;
	dat->m_idleBypassPos = 0.0;
	dat->m_fuelPumpRelayOn = false;
	dat->m_lambdaTrimOdd = 0;
	dat->m_lambdaTrimEven = 0;
	dat->m_coTrimVoltage = 0.0;
	dat->m_milOn = false;
	dat->m_rpmLimit = 0;
	dat->m_idleMode = false;
	dat->m_injectorPulseWidthUs = 0;
	dat->m_injectorPulseWidthMs = 0.0;
	dat->m_tune = 0;
	dat->m_checksumFixer = 0;
	dat->m_ident = 0;
	dat->m_rowScaler[FUEL_MAP_COUNT] = 0;
	dat->m_mafScaler = 0;

    memset(&dat->m_faultCodes, 0, sizeof(dat->m_faultCodes));


	readIntervals[SampleType_EngineTemperature]  = 1499;
	readIntervals[SampleType_RoadSpeed]          = 997;
	readIntervals[SampleType_EngineRPM]          = 0;
	readIntervals[SampleType_FuelTemperature]    = 1801;
	readIntervals[SampleType_MAF]                = 0;
	readIntervals[SampleType_Throttle]           = 0;
	readIntervals[SampleType_IdleBypassPosition] = 0;
	readIntervals[SampleType_TargetIdleRPM]      = 487;
	readIntervals[SampleType_GearSelection]      = 563;
	readIntervals[SampleType_MainVoltage]        = 283;
	readIntervals[SampleType_LambdaTrimShort]    = 0;
	readIntervals[SampleType_LambdaTrimLong]     = 331;
	readIntervals[SampleType_COTrimVoltage]      = 317;
	readIntervals[SampleType_FuelPumpRelay]      = 313;
	readIntervals[SampleType_FuelMapRowCol]      = 0;
	readIntervals[SampleType_FuelMapData]        = 3511;
	readIntervals[SampleType_FuelMapIndex]       = 1201;
	readIntervals[SampleType_InjectorPulseWidth] = 0;
	readIntervals[SampleType_MIL]                = 347;

	int type;
	for(type = 0; type < SampleType_NumSampleTypes; type++) {
		lastReadTime[type] = 0;
	}

	c14cux_init(&cuxinfo);

	return c14cux_connect(&cuxinfo, dev, C14CUX_BAUD);

}

void disconnect_from_ecu() {

	bool connected = c14cux_isConnected(&cuxinfo);

	if(connected) {
		c14cux_disconnect(&cuxinfo);
	}

}

read_result read_fault_codes(ecu_data* dat) {

	if(c14cux_isConnected(&cuxinfo)) {

		memset(&dat->m_faultCodes, 0, sizeof(dat->m_faultCodes));

		if(c14cux_getFaultCodes(&cuxinfo, &dat->m_faultCodes)) {
			return readresult_success;
		}
		else {
			return readresult_failure;
		}

	}

	return readresult_nostatement;

}


read_result merge_result(read_result total, bool single) {
	read_result result = total;

	if(total == readresult_nostatement) {
		result = single ? readresult_success : readresult_failure;
	}
	else if((total == readresult_failure) && single) {
		result = readresult_success;
	}

	return result;
}

uint64_t ms_since_epoch() {

	gettimeofday(&now, NULL);

	uint64_t millisecondsSinceEpoch = (uint64_t)(now.tv_sec) * 1000 + (uint64_t)(now.tv_usec) / 1000;

	return millisecondsSinceEpoch;

}

bool is_due_for_measurement(SampleType type) {
	bool status = false;

	if(is_sample_appropriate_for_mode(type)) {
		uint64_t now = ms_since_epoch();

		if(now - lastReadTime[type] >= readIntervals[type]) {
			status = true;
			lastReadTime[type] = now;
		}
	}

	return status;
}

bool is_sample_appropriate_for_mode(SampleType type) {
	bool status = true;

	if (type == SampleType_LambdaTrimLong)
	{
		status = (m_feedbackMode == C14CUX_FeedbackMode_ClosedLoop) &&
						 (m_lambdaTrimType == C14CUX_LambdaTrimType_LongTerm);
	}
	else if (type == SampleType_LambdaTrimShort)
	{
		status = (m_feedbackMode == C14CUX_FeedbackMode_ClosedLoop) &&
						 (m_lambdaTrimType == C14CUX_LambdaTrimType_ShortTerm);
	}
	else if (type == SampleType_COTrimVoltage)
	{
		status = (m_feedbackMode == C14CUX_FeedbackMode_OpenLoop);
	}
	else if (type == SampleType_FuelMapData)
	{
		// only refresh the fuel map data itself if a special option is set
		status = FUEL_MAP_REFRESH;
	}

	return status;
}

read_result read_data(ecu_data* dat) {
	read_result result = readresult_nostatement;

	if(! dat->m_readTuneId) {
		if(c14cux_getTuneRevision(&cuxinfo, &(dat->m_tune), &(dat->m_checksumFixer), &(dat->m_ident))) dat->m_readTuneId = true;
	}

	if (is_due_for_measurement(SampleType_MAF))
	{
		result = merge_result(result, c14cux_getMAFReading(&cuxinfo, m_airflowType, &(dat->m_mafReading)));
	}

	if (is_due_for_measurement(SampleType_Throttle))
	{
		result = merge_result(result, c14cux_getThrottlePosition(&cuxinfo, m_throttlePosType, &(dat->m_throttlePos)));
	}

	if (is_due_for_measurement(SampleType_LambdaTrimShort))
	{
		result = merge_result(result, c14cux_getLambdaTrimShort(&cuxinfo, C14CUX_Bank_Odd, &(dat->m_lambdaTrimOdd)));
		result = merge_result(result, c14cux_getLambdaTrimShort(&cuxinfo, C14CUX_Bank_Even, &(dat->m_lambdaTrimEven)));
	}

	if (is_due_for_measurement(SampleType_EngineRPM))
	{
		result = merge_result(result, c14cux_getEngineRPM(&cuxinfo, &(dat->m_engineSpeedRPM)));

		// If we haven't yet reported the RPM limit, see if we can read it now.
		// This is a special case because the limit is only read into its RAM
		// location in the ECU once the main spark interrupt has run; we therefore
		// wait until the engine speed > 0 before attempting this.
		if (!dat->m_rpmLimitRead &&
				(result == readresult_success) &&
				(dat->m_engineSpeedRPM > 0) &&
				c14cux_getRPMLimit(&cuxinfo, &(dat->m_rpmLimit)))
		{
			dat->m_rpmLimitRead = true;
		}
	}

	if (is_due_for_measurement(SampleType_FuelMapRowCol))
	{
		result = merge_result(result, c14cux_getFuelMapRowIndex(&cuxinfo, &(dat->m_currentFuelMapRowIndex), &(dat->m_fuelMapRowWeighting)));
		result = merge_result(result, c14cux_getFuelMapColumnIndex(&cuxinfo, &(dat->m_currentFuelMapColumnIndex), &(dat->m_fuelMapColWeighting)));
	}

	if (is_due_for_measurement(SampleType_InjectorPulseWidth))
	{
		result = merge_result(result, c14cux_getInjectorPulseWidth(&cuxinfo, &(dat->m_injectorPulseWidthUs)));
		dat->m_injectorPulseWidthMs = (float)dat->m_injectorPulseWidthUs / 1000.0;
	}

	if (is_due_for_measurement(SampleType_IdleBypassPosition))
	{
		result = merge_result(result, c14cux_getIdleBypassMotorPosition(&cuxinfo, &(dat->m_idleBypassPos)));
	}

	if (is_due_for_measurement(SampleType_LambdaTrimLong))
	{
		result = merge_result(result, c14cux_getLambdaTrimLong(&cuxinfo, C14CUX_Bank_Odd, &(dat->m_lambdaTrimOdd)));
		result = merge_result(result, c14cux_getLambdaTrimLong(&cuxinfo, C14CUX_Bank_Even, &(dat->m_lambdaTrimEven)));
	}

	if (is_due_for_measurement(SampleType_MainVoltage))
	{
		result = merge_result(result, c14cux_getMainVoltage(&cuxinfo, &(dat->m_mainVoltage)));
	}

	if (is_due_for_measurement(SampleType_TargetIdleRPM))
	{
		result = merge_result(result, c14cux_getTargetIdle(&cuxinfo, &(dat->m_targetIdleSpeed)));
		result = merge_result(result, c14cux_getIdleMode(&cuxinfo, &(dat->m_idleMode)));
	}

	if (is_due_for_measurement(SampleType_FuelPumpRelay))
	{
		result = merge_result(result, c14cux_getFuelPumpRelayState(&cuxinfo, &(dat->m_fuelPumpRelayOn)));
	}

	if (is_due_for_measurement(SampleType_GearSelection))
	{
		result = merge_result(result, c14cux_getGearSelection(&cuxinfo, &(dat->m_gear)));
	}

	if (is_due_for_measurement(SampleType_RoadSpeed))
	{
		result = merge_result(result, c14cux_getRoadSpeed(&cuxinfo, &(dat->m_roadSpeedMPH)));
	}

	if (is_due_for_measurement(SampleType_EngineTemperature))
	{
		result = merge_result(result, c14cux_getCoolantTemp(&cuxinfo, &(dat->m_coolantTempF)));
	}

	if (is_due_for_measurement(SampleType_FuelTemperature))
	{
		result = merge_result(result, c14cux_getFuelTemp(&cuxinfo, &(dat->m_fuelTempF)));
	}
/*
	if (is_due_for_measurement(SampleType_FuelMapData))
	{
		if (readFuelMap(m_currentFuelMapIndex))
		{
			emit fuelMapReady(m_currentFuelMapIndex);
		}
	}
*/
	// attempt to read the MIL status; if it can't be read, default it to off on the display
	if (is_due_for_measurement(SampleType_MIL))
	{
		if (c14cux_isMILOn(&cuxinfo, &(dat->m_milOn)))
		{
			result = merge_result(result, true);
		}
		else
		{
			result = merge_result(result, false);
			dat->m_milOn = false;
		}
	}
/*
	if (is_due_for_measurement(SampleType_FuelMapIndex))
	{
		uint8_t newFuelMapIndex = 0;
		bool fuelMapIndexReadResult = c14cux_getCurrentFuelMap(&cuxinfo, &newFuelMapIndex);
		result = merge_result(result, fuelMapIndexReadResult);

		// do some processing that is only relevant if we successfully read the current map ID
		if (fuelMapIndexReadResult)
		{
			// if the fuel map index has changed, or if this is the first time we've read it
			if ((newFuelMapIndex != m_currentFuelMapIndex) || !m_fuelMapIndexRead)
			{
				m_currentFuelMapIndex = newFuelMapIndex;
				emit fuelMapIndexHasChanged(m_currentFuelMapIndex);
			}

			// regardless of whether the map has changed, we know now
			// that is has been read at least once
			m_fuelMapIndexRead = true;

			// set the current fueling mode (open-loop or closed-loop)
			c14cux_feedback_mode newFeedbackMode = C14CUX_FeedbackMode_ClosedLoop;

			if ((m_currentFuelMapIndex >= s_firstOpenLoopMap) &&
					(m_currentFuelMapIndex <= s_lastOpenLoopMap))
			{
				newFeedbackMode = C14CUX_FeedbackMode_OpenLoop;
			}

			// if the feedback mode has changed, emit a signal
			if (newFeedbackMode != m_feedbackMode)
			{
				m_feedbackMode = newFeedbackMode;
				emit feedbackModeHasChanged(m_feedbackMode);
			}
		}
	}
*/	
	if (is_due_for_measurement(SampleType_COTrimVoltage))
	{
		result = merge_result(result, c14cux_getCOTrimVoltage(&cuxinfo, &(dat->m_coTrimVoltage)));
	}

	return result;
}



unsigned int convertSpeed(unsigned int speedMph, int speedUnits) {
	float speed = (float)speedMph;

	if (speedUnits == KPH) {
		speed *= 1.609344;
	}

	return (unsigned int)speed;
}

int convertTemperature(int tempF, int tempUnits) {
	double temp = tempF;

	switch(tempUnits) {
  		case Celsius:
    			temp = (temp - 32) * (0.5555556);
    			break;

		case Fahrenheit:
		default:
    			break;
  	}

	return (int)temp;
}
