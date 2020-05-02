/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include <string>
#include <atomic>
#include "LpcUart.h"

static volatile std::atomic_int counter;

// randomizer is used to randomize the simulated data a bit
static volatile std::atomic_uint8_t randomizer;

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief Handle interrupt from SysTick timer
* @return Nothing
*/
void SysTick_Handler(void)
{
	if (counter > 0) --counter;

	++randomizer;
}
#ifdef __cplusplus
}
#endif

void Sleep(int ms)
{
	counter = ms;
	while(counter > 0) {
		__WFI();
	}
}

#define TICKRATE_HZ1 (1000)	/* 1000 ticks per second */

enum class CMDS {UNK, FRIDGE_TEMP, WINE_CELLAR_TEMP, GREENHOUSE_TEMP, GREENHOUSE_VPD, GREENHOUSE_PAR};

CMDS getCommand(char *cmd);

std::string simulateUNK();
std::string simulateFridgeTemp();
std::string simulateWineCellarTemp();
std::string simulateGreenhouseTemp();
std::string simulateGreenhouseVPD();
std::string simulateGreenhousePAR();

typedef std::string (*simFn)(void);



int main(void) {

#if defined (__USE_LPCOPEN)
    // Read clock settings and update SystemCoreClock variable
    SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
    // Set up and initialize all required blocks and
    // functions related to the board hardware
    Board_Init();

#endif
#endif
    randomizer = 0;
    uint32_t sysTickRate;

    Chip_Clock_SetSysTickClockDiv(1);
    sysTickRate = Chip_Clock_GetSysTickClockRate();
    SysTick_Config(sysTickRate / TICKRATE_HZ1);

	#define MAX_BUF_LEN 8
    char msg[MAX_BUF_LEN];
    int msg_len = 0;
    int c;

    static simFn simulate[6] = {simulateUNK, simulateFridgeTemp, simulateWineCellarTemp,
    						   simulateGreenhouseTemp, simulateGreenhouseVPD, simulateGreenhousePAR};

    while (1) { // echo back what we receive
        c = Board_UARTGetChar();
        if (c != EOF) {

        	if (c != '\n' && c != '\r' && msg_len < MAX_BUF_LEN-1)
        	{
        		msg[msg_len++] = c;
        	}
        	else
        	{
        		msg[msg_len] = '\0';
        	    int cmd = (int)getCommand(msg);
        		for (char c : simulate[cmd]())
				{
        			Board_UARTPutChar(c);
        		}

        		msg_len = 0;
        	}
        }
    }

    return 0;
}

CMDS getCommand(char *cmd) {
	enum parseStates { START, FRIDGE, WINE_CELLAR, GREENHOUSE, FAIL, END};
	parseStates state = START;
	CMDS parseVal = CMDS::UNK;

	while (true) {

		if (*cmd == '\0' && state != END)
			state = FAIL;

		switch (state) {
			case START:
				switch (*cmd) {
					case 'f':
						state = FRIDGE;
						break;
					case 'w':
						state = WINE_CELLAR;
						break;
					case 'g':
						state = GREENHOUSE;
						break;
					default:
						state = FAIL;
				}
				break;

			case FRIDGE:
				if (*cmd != 't') {
					state = FAIL;
				}
				else {
					parseVal = CMDS::FRIDGE_TEMP;
					state = END;
				}
				break;

			case WINE_CELLAR:
				if (*cmd != 't')
					state = FAIL;
				else {
					parseVal = CMDS::WINE_CELLAR_TEMP;
					state = END;
				}
				break;

			case GREENHOUSE:
				switch (*cmd) {
					case 't':
						parseVal = CMDS::GREENHOUSE_TEMP;
						state = END;
						break;
					case 'v':
						parseVal = CMDS::GREENHOUSE_VPD;
						state = END;
						break;
					case 'p':
						parseVal = CMDS::GREENHOUSE_PAR;
						state = END;
						break;
					default:
						state = FAIL;
				}
				break;

				case FAIL:
					return CMDS::UNK;

				case END:
					if (*cmd != '\0')
						state = FAIL;
					else
						return parseVal;
					break;
		}
		++cmd;
	}
}

std::string simulateUNK() {
	return std::string("Unknown Command\n");
}

std::string simulateFridgeTemp(){
	return std::to_string(1.2 + randomizer*0.01) + "\n";
}

std::string simulateWineCellarTemp() {
	return std::to_string(7.9 + randomizer/25.0) + "\n";
}

std::string simulateGreenhouseTemp() {
	return std::to_string(25.90 + randomizer*0.01) + "\n";
}

std::string simulateGreenhouseVPD() {
	// To get between 0.45kPa and 1.25kPa value, the following formula is used:
	static float randomizer_scale_factor = 0.003137;
	return std::to_string(0.45 + (randomizer * randomizer_scale_factor)) + "\n";
}

std::string simulateGreenhousePAR() {
	return std::to_string(375 + randomizer) + "\n";
}
