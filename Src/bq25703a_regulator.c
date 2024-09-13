/**
 ******************************************************************************
 * @file           : bq25703a_regulator.c
 * @brief          : Handles battery state information
 ******************************************************************************
 */

#include "adc_interface.h"
#include "bq25703a_regulator.h"
#include "battery.h"
#include "error.h"
#include "main.h"
#include "string.h"
#include "printf.h"
#include "usbpd.h"

extern I2C_HandleTypeDef hi2c1;		// I2C handle

uint16_t testword;  // [PR] test byte readable by debugger

// Serial debug output:
// ChargeState,Timer,Errors,Vusb,Vbat,Vsys,Icharge,Iinput,CStatus,Connections,CellCount,charge_current_limit,current_setting,C4,C3,C2,C1,Balance

/* Private typedef -----------------------------------------------------------*/
struct Regulator {
	uint8_t connected;
	uint8_t charging_status;
	//uint16_t max_charge_voltage;			// Seems unused [PR]
	//uint8_t input_current_limit;			// Seems unused [PR]
	uint8_t current_setting;				// Added [PR]
	//uint16_t min_input_voltage_limit;		// Seems unused [PR]
	uint32_t vbus_voltage;
	uint32_t vbat_voltage;
	uint32_t vsys_voltage;
	uint32_t charge_current;
	uint32_t input_current;
	uint32_t max_charge_current_ma;
};

/* Private variables ---------------------------------------------------------*/
struct Regulator regulator;

unsigned char connection_state;			// State of power and balance connections
#define CONNECTION_OK			11
#define CONNECTION_NO_XT60		12
#define CONNECTION_NO_PD		13
#define CONNECTION_FAIL			14

uint8_t charge_state;					// Charging state machine
#define C_CHARGE				21
#define C_RECOVER				22
#define C_MEASURE				23

/* The maximum time to wait for the mutex that guards the I2C bus to become
 available. */
#define cmdMAX_MUTEX_WAIT	pdMS_TO_TICKS( 300 )

/* Private function prototypes -----------------------------------------------*/
void I2C_Transfer(uint8_t *pData, uint16_t size);
void I2C_Receive(uint8_t *pData, uint16_t size);
void I2C_Write_Register(uint8_t addr_to_write, uint8_t *pData);
void I2C_Write_Two_Byte_Register(uint8_t addr_to_write, uint8_t lsb_data, uint8_t msb_data);
void I2C_Read_Register(uint8_t addr_to_read, uint8_t *pData, uint16_t size);
uint8_t Query_Regulator_Connection(void);
uint8_t Read_Charge_Okay(void);
void Read_Charge_Status(void);
void Regulator_Set_ADC_Option(void);
void Regulator_Read_ADC(void);
void Regulator_HI_Z(uint8_t hi_z_en);
void Regulator_OTG_EN(uint8_t otg_en);
void Regulator_Set_Charge_Option_0(void);
void Set_Charge_Voltage(uint8_t number_of_cells);

/**
 * @brief Returns whether the regulator is connected over I2C
 * @retval uint8_t CONNECTED or NOT_CONNECTED
 */
uint8_t Get_Regulator_Connection_State() {
	return regulator.connected;
}

/**
 * @brief Returns whether the regulator is charging
 * @retval uint8_t 1 if charging, 0 if not charging
 */
uint8_t Get_Regulator_Charging_State() {
	return regulator.charging_status;
}

/**
 * @brief Gets VBAT voltage that was read in from the ADC on the regulator
 * @retval VBAT voltage in volts * REG_ADC_MULTIPLIER
 */
uint32_t Get_VBAT_ADC_Reading() {
	return regulator.vbat_voltage;
}

/**
 * @brief Gets VBUS voltage that was read in from the ADC on the regulator
 * @retval VBUS voltage in volts * REG_ADC_MULTIPLIER
 */
uint32_t Get_VBUS_ADC_Reading() {
	return regulator.vbus_voltage;
}

/**
 * @brief Gets Input Current that was read in from the ADC on the regulator
 * @retval Input Current in amps * REG_ADC_MULTIPLIER
 */
uint32_t Get_Input_Current_ADC_Reading() {
	return regulator.input_current;
}

/**
 * @brief Gets Charge Current that was read in from the ADC on the regulator
 * @retval Charge Current in amps * REG_ADC_MULTIPLIER
 */
uint32_t Get_Charge_Current_ADC_Reading() {
	return regulator.charge_current;
}

/**
 * @brief Gets the max output current for charging
 * @retval Max Charge Current in miliamps
 */
uint32_t Get_Max_Charge_Current() {
	return regulator.max_charge_current_ma;
}

/**
 * @brief Performs an I2C transfer
 * @param pData Pointer to location of data to transfer
 * @param size Size of data to be transferred
 */
void I2C_Transfer(uint8_t *pData, uint16_t size) {

	if ( xSemaphoreTake( xTxMutex_Regulator, cmdMAX_MUTEX_WAIT ) == pdPASS) {
		do
		{
			TickType_t xtimeout_start = xTaskGetTickCount();
			while (HAL_I2C_Master_Transmit_DMA(&hi2c1, (uint16_t)BQ26703A_I2C_ADDRESS, pData, size) != HAL_OK) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
			}
		    while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
		    }
		}
		while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
		xSemaphoreGive(xTxMutex_Regulator);
	}
}

/**
 * @brief Performs an I2C transfer
 * @param pData Pointer to location to store received data
 * @param size Size of data to be received
 */
void I2C_Receive(uint8_t *pData, uint16_t size) {
	if ( xSemaphoreTake( xTxMutex_Regulator, cmdMAX_MUTEX_WAIT ) == pdPASS) {
		do
		{
			TickType_t xtimeout_start = xTaskGetTickCount();
			while (HAL_I2C_Master_Receive_DMA(&hi2c1, (uint16_t)BQ26703A_I2C_ADDRESS, pData, size) != HAL_OK) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
			}
			while (HAL_I2C_GetState(&hi2c1) != HAL_I2C_STATE_READY) {
				if (((xTaskGetTickCount()-xtimeout_start)/portTICK_PERIOD_MS) > I2C_TIMEOUT) {
					Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
					break;
				}
			}
		}
		while(HAL_I2C_GetError(&hi2c1) == HAL_I2C_ERROR_AF);
		xSemaphoreGive(xTxMutex_Regulator);
	}
}

/**
 * @brief Automatically performs two I2C writes to write a register on the regulator
 * @param pData Pointer to data to be transferred
 */
void I2C_Write_Register(uint8_t addr_to_write, uint8_t *pData) {
	uint8_t data[2];
	data[0] = addr_to_write;
	data[1] = *pData;
	I2C_Transfer(data, 2);
}

/**
 * @brief Automatically performs three I2C writes to write a two byte register on the regulator
 * @param lsb_data Pointer to least significant byte of data to be transferred
 * @param msb_data Pointer to most significant byte of data to be transferred
 */
void I2C_Write_Two_Byte_Register(uint8_t addr_to_write, uint8_t lsb_data, uint8_t msb_data) {

	uint8_t data[3];
	data[0] = addr_to_write;
	data[1] = lsb_data;
	data[2] = msb_data;

	I2C_Transfer(data, 3);
}

/**
 * @brief Automatically performs one I2C write and an I2C read to get the value of a register
 * @param pData Pointer to where to store data
 */
void I2C_Read_Register(uint8_t addr_to_read, uint8_t *pData, uint16_t size) {
		I2C_Transfer((uint8_t *)&addr_to_read, 1);
		I2C_Receive(pData, size);
}

/**
 * @brief Checks if the regulator is connected over I2C
 * @retval uint8_t CONNECTED or NOT_CONNECTED
 */
uint8_t Query_Regulator_Connection() {
	/* Get the manufacturer id */
	uint8_t manufacturer_id;
	I2C_Read_Register(MANUFACTURER_ID_ADDR, (uint8_t *) &manufacturer_id, 1);

	/* Get the device id */
	uint8_t device_id;
	I2C_Read_Register(DEVICE_ID_ADDR, (uint8_t *) &device_id, 1);

	if ( (device_id == BQ26703A_DEVICE_ID) && (manufacturer_id == BQ26703A_MANUFACTURER_ID) ) {
		Clear_Error_State(REGULATOR_COMMUNICATION_ERROR);
		return CONNECTED;
	}
	else {
		Set_Error_State(REGULATOR_COMMUNICATION_ERROR);
		return NOT_CONNECTED;
	}
}

/**
 * @brief Checks the state of the Charge okay pin and returns the value
 * @retval 0 if VBUS falls below 3.2 V or rises above 26 V, 1 if VBUS is between 3.5V and 24.5V
 */
uint8_t Read_Charge_Okay() {
	return HAL_GPIO_ReadPin(CHRG_OK_GPIO_Port, CHRG_OK_Pin);
}

/**
 * @brief Reads ChargeStatus register and sets status
 */
void Read_Charge_Status() {
	uint8_t data[2];
	I2C_Read_Register(CHARGE_STATUS_ADDR, data, 2);

	testword = (data[1] << 8) | data[0];			// Copy status word into variable for debugging

	if (data[1] & CHARGING_ENABLED_MASK) {
		regulator.charging_status = 1;
	}
	else {
		regulator.charging_status = 0;
	}
	
	// Output CHARGE_STATUS_ADDR register (21/20h)
	//printf("s%08b %08b,", data[1], data[0]);

	// Output ChargeOption2 register (33/32h)
	//I2C_Read_Register(CHARGE_OPTION_2_ADDR, data, 2);
	//printf("s%08b %08b,", data[1], data[0]);
}

/**
 * @brief Sets the Regulators ADC settings
 */
void Regulator_Set_ADC_Option() {

	uint8_t ADC_lsb_3A = ADC_ENABLED_BITMASK;

	I2C_Write_Register(ADC_OPTION_ADDR, (uint8_t *) &ADC_lsb_3A);
}

/**
 * @brief Initiates and reads a single ADC conversion on the regulator
 */
void Regulator_Read_ADC() {
	TickType_t xDelay = 80 / portTICK_PERIOD_MS;

	uint8_t ADC_msb_3B = ADC_START_CONVERSION_MASK;

	I2C_Write_Register((ADC_OPTION_ADDR+1), (uint8_t *) &ADC_msb_3B);			// Start ADC conversion on BQ

	/* Wait for the conversion to finish */
	while (ADC_msb_3B & (1<<6)) {
		vTaskDelay(xDelay);
		I2C_Read_Register((ADC_OPTION_ADDR+1), (uint8_t *) &ADC_msb_3B, 1);		// Wait for conversion to complete
	}

	// Read ADC conversion results
	uint8_t temp = 0;

	I2C_Read_Register(VBAT_ADC_ADDR, (uint8_t *) &temp, 1);
	regulator.vbat_voltage = (temp * VBAT_ADC_SCALE) + VBAT_ADC_OFFSET;

	I2C_Read_Register(VSYS_ADC_ADDR, (uint8_t *) &temp, 1);
	regulator.vsys_voltage = (temp * VSYS_ADC_SCALE) + VSYS_ADC_OFFSET;

	I2C_Read_Register(ICHG_ADC_ADDR, (uint8_t *) &temp, 1);
	regulator.charge_current = temp * ICHG_ADC_SCALE;

	I2C_Read_Register(IIN_ADC_ADDR, (uint8_t *) &temp, 1);
	regulator.input_current = temp * IIN_ADC_SCALE;
	//testword = temp;

	I2C_Read_Register(VBUS_ADC_ADDR, (uint8_t *) &temp, 1);
	regulator.vbus_voltage = (temp * VBUS_ADC_SCALE) + VBUS_ADC_OFFSET;
	//testword = temp;

}

/**
 * @brief Enables or disables high impedance mode on the output of the regulator
 * @param hi_z_en 1 puts the output of the regulator in hiz mode. 0 takes the regulator out of hi_z and allows charging
 */
void Regulator_HI_Z(uint8_t hi_z_en) {
	if (hi_z_en == 1) {
		HAL_GPIO_WritePin(ILIM_HIZ_GPIO_Port, ILIM_HIZ_Pin, GPIO_PIN_RESET);
	}
	else {
		HAL_GPIO_WritePin(ILIM_HIZ_GPIO_Port, ILIM_HIZ_Pin, GPIO_PIN_SET);
	}
}

/**
 * @brief Enables or disables On the Go Mode
 * @param otg_en 0 disables On the GO Mode. 1 Enables.
 */
void Regulator_OTG_EN(uint8_t otg_en) {
	if (otg_en == 0) {
		HAL_GPIO_WritePin(EN_OTG_GPIO_Port, EN_OTG_Pin, GPIO_PIN_RESET);
	}
	else {
		HAL_GPIO_WritePin(EN_OTG_GPIO_Port, EN_OTG_Pin, GPIO_PIN_SET);
	}
}

/**
 * @brief Sets Charge Option 0 Based on #defines in header
 */
void Regulator_Set_Charge_Option_0() {

	uint8_t charge_option_0_register_1_value = 0b00100110;
	uint8_t charge_option_0_register_2_value = 0b00001110;

	I2C_Write_Two_Byte_Register(CHARGE_OPTION_0_ADDR, charge_option_0_register_2_value, charge_option_0_register_1_value);

	return;
}

/**
 * @brief Sets the charging current limit. From 64mA to 8.128A in 64mA steps. Maps from 0 - 128. 7 bit value.
 * @param charge_current_limit Charge current limit in mA
 */
void Set_Charge_Current(uint32_t charge_current_limit) {

	if (charge_current_limit > MAX_CHARGE_CURRENT_MA) {
		charge_current_limit = MAX_CHARGE_CURRENT_MA;
	}

	regulator.max_charge_current_ma = charge_current_limit;

	regulator.current_setting = 0;
	if (charge_current_limit != 0){
		regulator.current_setting = charge_current_limit/64;
	}

	if (regulator.current_setting > 128) {
		regulator.current_setting = 128;
	}

	// 0-128 which remaps to 64mA-8.128A
	uint8_t charge_current_register_1_value = 0;
	uint8_t charge_current_register_2_value = 0;

	if ((regulator.current_setting >= 0) && (regulator.current_setting <= 128)) {
		charge_current_register_1_value = (regulator.current_setting >> 2);
		charge_current_register_2_value = (regulator.current_setting << 6);
	}

	// Display current setting registers
	//printf("r:%u,%u,", charge_current_register_1_value, charge_current_register_2_value);

	I2C_Write_Two_Byte_Register(CHARGE_CURRENT_ADDR, charge_current_register_2_value, charge_current_register_1_value);

	return;
}

/**
 * @brief Sets the charging voltage based on the number of cells. 1 - 4.192V, 2 - 8.400V, 3 - 12.592V, 4 - 16.800V
 * @param number_of_cells number of cells connected
 */
void Set_Charge_Voltage(uint8_t number_of_cells) {

	uint8_t max_charge_register_1_value = 0;
	uint8_t max_charge_register_2_value = 0;

	uint8_t	minimum_system_voltage_value = MIN_VOLT_ADD_1024_MV;

	switch (number_of_cells) {
		case 1:
			// Set max voltage to 4.2V
			max_charge_register_1_value = MAX_VOLT_ADD_4096_MV;
			max_charge_register_2_value = MAX_VOLT_ADD_64_MV | MAX_VOLT_ADD_32_MV;
			// Set min voltage to 2.8V
			minimum_system_voltage_value = MIN_VOLT_ADD_2048_MV | MIN_VOLT_ADD_512_MV | MIN_VOLT_ADD_256_MV;
			break;
		case 2:
			// Set max voltage to 8.4V
			max_charge_register_1_value = MAX_VOLT_ADD_8192_MV;
			max_charge_register_2_value = MAX_VOLT_ADD_128_MV | MAX_VOLT_ADD_64_MV | MAX_VOLT_ADD_16_MV;
			// Set min voltage to 5.6V
			minimum_system_voltage_value = MIN_VOLT_ADD_4096_MV | MIN_VOLT_ADD_1024_MV | MIN_VOLT_ADD_512_MV;
			break;
		case 3:
			// Set max voltage to 12.6V
			max_charge_register_1_value = MAX_VOLT_ADD_8192_MV | MAX_VOLT_ADD_4096_MV | MAX_VOLT_ADD_256_MV;
			max_charge_register_2_value = MAX_VOLT_ADD_32_MV | MAX_VOLT_ADD_16_MV;
			// Set min voltage to 8.4V
			minimum_system_voltage_value = MIN_VOLT_ADD_8192_MV |  MIN_VOLT_ADD_256_MV;
			break;
		case 4:
			// Set max voltage to 16.8V
			max_charge_register_1_value = MAX_VOLT_ADD_16384_MV | MAX_VOLT_ADD_256_MV;
			max_charge_register_2_value = MAX_VOLT_ADD_128_MV | MAX_VOLT_ADD_32_MV;
			// Set min voltage to 11.2V
			minimum_system_voltage_value = MIN_VOLT_ADD_8192_MV | MIN_VOLT_ADD_2048_MV | MIN_VOLT_ADD_1024_MV;
			break;
		default:
			max_charge_register_1_value = 0;
			max_charge_register_2_value = 0;
			minimum_system_voltage_value = MIN_VOLT_ADD_1024_MV;
			break;
		}

	I2C_Write_Register(MINIMUM_SYSTEM_VOLTAGE_ADDR, (uint8_t *) &minimum_system_voltage_value);

	I2C_Write_Two_Byte_Register(MAX_CHARGE_VOLTAGE_ADDR, max_charge_register_2_value, max_charge_register_1_value);

	return;
}

/**
 * @brief Calculates the max charge power based on temperature of MCU
 * @retval Max charging power in mW
 */
uint32_t Calculate_Max_Charge_Power() {

	//Account for system losses with ASSUME_EFFICIENCY fudge factor to not overload source
	uint32_t charging_power_mw = (((float)(regulator.vbus_voltage/REG_ADC_MULTIPLIER) * Get_Max_Input_Current()) * ASSUME_EFFICIENCY);

	if (charging_power_mw > MAX_CHARGING_POWER) {
		charging_power_mw = MAX_CHARGING_POWER;
	}

	if (charging_power_mw > Get_Max_Input_Power()){
		charging_power_mw = Get_Max_Input_Power() * ASSUME_EFFICIENCY;
	}

	//Throttle charging power if temperature is too high
	if (Get_MCU_Temperature() > TEMP_THROTTLE_THRESH_C){
		float temperature = (float)Get_MCU_Temperature();

		float power_scalar = 1.0f - ((float)(0.0333 * temperature) - 1.33f);

		if (power_scalar > 1.0f) {
			power_scalar = 1.0f;
		}
		if (power_scalar < 0.00f) {
			power_scalar = 0.00f;
		}

		charging_power_mw = charging_power_mw * power_scalar;
	}

	return charging_power_mw;
}

/**
 * @brief Determines if charger output should be on and sets voltage and current parameters as needed
 */
void Control_Charger_Output() {

	TickType_t xDelay = 1000 / portTICK_PERIOD_MS;

	// Charging for USB PD enabled supplies
	if ((Get_XT60_Connection_State() == CONNECTED) && (Get_Balance_Connection_State() == CONNECTED) && (Get_Error_State() == 0) && (Get_Input_Power_Ready() == READY) && (Get_Cell_Over_Voltage_State() == 0)) {

		// XT60 connected, balance lead connected, no error, input PD power ready, no cell over voltage

		Set_Charge_Voltage(Get_Number_Of_Cells());

		uint32_t charging_power_mw = Calculate_Max_Charge_Power();
		uint32_t charging_current_ma = (charging_power_mw / (float)(Get_Battery_Voltage() / BATTERY_ADC_MULTIPLIER));  // [PR] warning - precision lost here, move (float) inside brackets

		Set_Charge_Current(charging_current_ma);

		Regulator_HI_Z(0);
		connection_state = CONNECTION_OK;

		// Check if XT60 was disconnected
		if (regulator.vbat_voltage > (BATTERY_DISCONNECT_THRESH * Get_Number_Of_Cells())) {
			Regulator_HI_Z(1);
			vTaskDelay(xDelay*2);
			Regulator_HI_Z(0);
			connection_state = CONNECTION_NO_XT60;
		}
	}
	// Case to handle non USB PD supplies. Limited to 5V 500mA.
	else if ((Get_XT60_Connection_State() == CONNECTED) && (Get_Balance_Connection_State() == CONNECTED) && (Get_Error_State() == 0) && (Get_Input_Power_Ready() == NO_USB_PD_SUPPLY) && (Get_Cell_Over_Voltage_State() == 0)) {

		// XT60 connected, balance lead connected, no error, power not PD, no cell over voltage

		Set_Charge_Voltage(Get_Number_Of_Cells());

		uint32_t charging_current_ma = ((NON_USB_PD_CHARGE_POWER * ASSUME_EFFICIENCY) / (Get_Battery_Voltage() / BATTERY_ADC_MULTIPLIER));

		Set_Charge_Current(charging_current_ma);

		Regulator_HI_Z(0);

		connection_state = CONNECTION_NO_PD;
	}
	else {

		// Charge conditions not met, turn charge output off

		Regulator_HI_Z(1);
		Set_Charge_Voltage(0);
		Set_Charge_Current(0);
		
		connection_state = CONNECTION_FAIL;
	}
}

/**
 * @brief Main regulator task
 */
void vRegulator(void const *pvParameters) {

	TickType_t xLastWakeTime;
	const TickType_t xPeriod = 500 / portTICK_PERIOD_MS;	// 500ms tick period
	uint8_t charge_state_new;								// Temporary variable for state change

	/* Disable the output of the regulator for safety */
	Regulator_HI_Z(1);

	/* Disable OTG mode */
	Regulator_OTG_EN(0);

	/* Check if the regulator is connected */
	regulator.connected = Query_Regulator_Connection();

	/* Set Charge Option 0 */
	Regulator_Set_Charge_Option_0();

	/* Setup the ADC on the Regulator */
	Regulator_Set_ADC_Option();

	uint8_t timer_count = 0;
	xLastWakeTime = xTaskGetTickCount();

	charge_state = C_RECOVER;		// Set initial state

	for (;;) {

		// Check if power into regulator is okay by reading CHRG_OK pin (fails if USB power is not connected)
		if (Read_Charge_Okay() != 1) {
			Set_Error_State(VOLTAGE_INPUT_ERROR);
		}
		else if ((Get_Error_State() & VOLTAGE_INPUT_ERROR) == VOLTAGE_INPUT_ERROR) {
			// Error no longer detected, clear error state
			Clear_Error_State(VOLTAGE_INPUT_ERROR);
		}

		// Check if STM32G0 can communicate with regulator
		if ((Get_Error_State() & REGULATOR_COMMUNICATION_ERROR) == REGULATOR_COMMUNICATION_ERROR) {
			regulator.connected = 0;
			printf("Reg error!");
		}

		Read_Charge_Status();  // Currently returns 0 whether USB powered or not
		Regulator_Read_ADC();

		// Charge control state machine
		switch (charge_state) {
			case C_CHARGE:
			// Charge cycle state: Charge
			Control_Charger_Output();

			if (timer_count > 20) {charge_state_new = C_RECOVER;}
			break;

			case C_RECOVER:
			// Charge cycle state: Recover
			Balance_Off();
			Regulator_HI_Z(1);

			if (timer_count > 4) {charge_state_new = C_MEASURE;}
			break;

			case C_MEASURE:
			// Charge cycle state: Measure & balance
			Balance_Battery();

			charge_state_new = C_CHARGE;
			break;
		}

		// Debug output (end of rest period)
		if (charge_state_new == C_MEASURE || 1) {
			printf("%u,", charge_state);											// Charging state
			printf("%u,", timer_count);												// State timer
			printf("%u (%06b),", Get_Error_State(), Get_Error_State());				// Error bits
			// Vusb,Vbat,Vsys,Icharge,Iinput
			printf("%u,%.2f,%.2f,%u,%u,", Get_Input_Voltage()/1000, (float)regulator.vbat_voltage/100000, (float)regulator.vsys_voltage/100000, regulator.charge_current/100, regulator.input_current/100);
			printf("%u,", regulator.charging_status);								// Charging status
			printf("%u,", connection_state);										// Connection state
			printf("%uS,", Get_Number_Of_Cells());									// CellCount
			printf("%u,", regulator.max_charge_current_ma);							// charge_current_limit (capped to max charge current)
			printf("%u,", regulator.current_setting * 64);							// Regulator charge current setting
			printf("%.3f,", (float)Get_Cell_Voltage(3) / BATTERY_ADC_MULTIPLIER);	// Cell 4
			printf("%.3f,", (float)Get_Cell_Voltage(2) / BATTERY_ADC_MULTIPLIER);	// Cell 3
			printf("%.3f,", (float)Get_Cell_Voltage(1) / BATTERY_ADC_MULTIPLIER);	// Cell 2
			printf("%.3f,", (float)Get_Cell_Voltage(0) / BATTERY_ADC_MULTIPLIER);	// Cell 1
			printf("B%04b,", Get_Balancing_State());								// Cell balancing state
			printf("\r\n");
		}

		// Update state and timer
		if (charge_state_new != 0) {
			charge_state = charge_state_new;
			charge_state_new = 0;
			timer_count = 0;
		}
		else {
			timer_count++;
		}

		vTaskDelayUntil(&xLastWakeTime, xPeriod);
	}
}
