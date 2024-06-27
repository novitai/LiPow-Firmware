ADC channels redefined, increased from 7 to 8
uint32_t adc_buffer[8];

**extern** osThreadId;
**extern** osThreadId adcTaskHandle;
**extern** osThreadId regulatorTaskHandle;
**extern** osThreadId CLITaskHandle;


main.c
ADC and pin configurations
  hadc1.Init.NbrOfConversion = 8;
  sConfig.Channel = ADC_CHANNEL_6;