/**********************************************************************************************************************
File: tilt_controller.c                                                                

Description:
Tilt controller for a modified hospital bed

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void TiltControllerInitialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void TiltControllerRunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_TiltController"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32TiltControllerFlags;                       /* Global state flags */

volatile bool G_LimitSwitchesActive[TOTAL_LIMIT_SWITCHES]; 

/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "TiltController_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type TiltController_StateMachine;            /* The state machine function pointer */
//static u32 TiltController_u32Timeout;                      /* Timeout counter used across states */

static u8 TiltController_au8IdleMessage[] = "SYSTEM IDLE";

static u32 TiltController_servoDty =  PWM_CDTY2_INIT;

/**********************************************************************************************************************
Function Definitions
**********************************************************************************************************************/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Public functions                                                                                                   */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------*/
/* Protected functions                                                                                                */
/*--------------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------------------------------------------
Function: TiltControllerInitialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void TiltControllerInitialize(void)
{
  /* If good initialization, set state to Idle */
  if( 1 )
  {
    LCDClearChars(LINE1_START_ADDR, 40);
    LCDMessage(LINE1_START_ADDR, TiltController_au8IdleMessage);
    TiltController_StateMachine = TiltControllerSM_Idle;
    
    //Set up PWM Channel 2 for SERVO, which is operated using GPIO -> PA_06_SERVO 
    AT91C_BASE_PWMC_CH2->PWMC_CMR = PWM_CMR2_INIT;
    AT91C_BASE_PWMC_CH2->PWMC_CPRDR    = PWM_CPRD2_INIT; /* Period is set to 20 ms, or 60,000 cycles of the scaled 3Mhz clock */
    AT91C_BASE_PWMC_CH2->PWMC_CPRDUPDR = PWM_CPRD2_INIT;
    AT91C_BASE_PWMC_CH2->PWMC_CDTYR    = PWM_CDTY2_INIT; /* Duty is inialized to 2ms, which is 10% duty cycle, this should put the arm around nuetral position*/
    AT91C_BASE_PWMC_CH2->PWMC_CDTYUPDR = PWM_CDTY2_INIT; 
    
    AT91C_BASE_PWMC->PWMC_ENA = AT91C_PWMC_CHID2;
    
    /*Initialize interrupts for the limit switches */
    u32 u32PortBInterruptMask = (PB_08_LIMIT1 | PB_07_LIMIT2);
    AT91C_BASE_PIOB->PIO_IER |= u32PortBInterruptMask;
    // Read ISR register to clear any existing flags
    u32PortBInterruptMask = AT91C_BASE_PIOB->PIO_ISR;
    // Configure the NVIC to ensure PIOB interrupts are active
    NVIC_ClearPendingIRQ(IRQn_PIOB);
    NVIC_EnableIRQ(IRQn_PIOB);
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
    TiltController_StateMachine = TiltControllerSM_Error;
  }

} /* end TiltControllerInitialize() */

  
/*----------------------------------------------------------------------------------------------------------------------
Function TiltControllerRunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void TiltControllerRunActiveState(void)
{
  TiltController_StateMachine();

} /* end TiltControllerRunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for ??? */
static void TiltControllerSM_Idle(void)
{
  static bool button0Active = FALSE;
  static bool button1Active = FALSE;
  static u8 (*au8CurrentMessage)[] = &TiltController_au8IdleMessage;
  static u8 (*au8LastMessage)[] = &TiltController_au8IdleMessage;
  static u8 au8UpMessage[] = "TILTING UP";
  static u8 au8DownMessage[] = "TILTING DOWN";
  
  static u32 currentServoDty = PWM_CDTY2_INIT;
  static u32 currentServoPeriod = PWM_CPRD2_INIT;
  
  
  
  // button 0 controls tilting UP. Should not actuate if button 1 is also being pressed
  /* start button 0  */
  if(IsButtonPressed(BUTTON0) && !G_LimitSwitchesActive[0]) {
    if(!button0Active && !button1Active) {
      button0Active = TRUE;
      
      /*
      For hospital bed model this is commented out, as a servo is being used instead of an actuator
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (SODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_SODR) + 0x80;
      //Turn Actuator ON
      *pu32SetAddress = PB_05_ACTUATOR_UP;
      */
      
      au8CurrentMessage = &au8UpMessage;  
    }
    //For DEMO only -> increase servo angle as long as it is within its upper bound
    
    if(currentServoDty > (PWM_CDTY2_MIN - 1)) {
      currentServoDty= currentServoDty - 1;
      
      AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID2;
      AT91C_BASE_PWMC_CH2->PWMC_CDTYR = currentServoDty;
      AT91C_BASE_PWMC_CH2->PWMC_CDTYUPDR = currentServoDty; 
      AT91C_BASE_PWMC->PWMC_ENA = AT91C_PWMC_CHID2;
    }
    else {
      u8 au8MaxUP[] = "MAX UP";
      au8CurrentMessage = &au8MaxUP;
    }
  }
  else {
      button0Active = FALSE;
      
      /*
      For hospital bed model this is commented out, as a servo is being used instead of an actuator
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (CODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_CODR) + 0x80;
      //Turn Actuator OFF
      *pu32SetAddress = PB_05_ACTUATOR_UP;
      */
  }
  /* end button 0 */
  
  // button 1 controls tilting DOWN. Should not actuate if button 0 is also being pressed
  /* start button 1 */
  if(IsButtonPressed(BUTTON1) && !G_LimitSwitchesActive[1]) {
    if(!button1Active && !button0Active) {
      button1Active = TRUE;
      u32 *pu32SetAddress;
      /*
      For hospital bed model this is commented out, as a servo is being used instead of an actuator
      //build address for Actuator Set output data register (SODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_SODR) + 0x80;
      //Turn Actuator ON
      *pu32SetAddress = PB_08_ACTUATOR_DOWN;
      */
      
      au8CurrentMessage = &au8DownMessage;
      
      
    }
    if(currentServoDty < (PWM_CDTY2_MAX + 1)) {
      currentServoDty = currentServoDty + 1;
      
      AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID2;
      AT91C_BASE_PWMC_CH2->PWMC_CDTYR = currentServoDty;
      AT91C_BASE_PWMC_CH2->PWMC_CDTYUPDR = currentServoDty; 
      AT91C_BASE_PWMC->PWMC_ENA = AT91C_PWMC_CHID2; 
    }
    else {
      u8 au8MaxDOWN[] = "MAX DOWN";
      au8CurrentMessage = &au8MaxDOWN;
    }
  }
  else {
      button1Active = FALSE;
      
      /*
      For hospital bed model this is commented out, as a servo is being used instead of an actuator
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (CODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_CODR) + 0x80;
      //Turn Actuator OFF
      *pu32SetAddress = PB_08_ACTUATOR_DOWN;
      */
  }
  /* end button 1 */

  /* if system is idle, set idle message */
  if (!button0Active && !button1Active) {
      au8CurrentMessage = &TiltController_au8IdleMessage;
  }
  
  /* if message has changed, update the screen */
  if(au8CurrentMessage != au8LastMessage) {
      LCDClearChars(LINE1_START_ADDR, 40);
      LCDMessage(LINE1_START_ADDR, *au8CurrentMessage);
      au8LastMessage = au8CurrentMessage;
  }
  
  //Currently Button 2 and 3 are unused
  if(WasButtonPressed(BUTTON2)) {
    ButtonAcknowledge(BUTTON2);
  }
  if(WasButtonPressed(BUTTON3)) {
    ButtonAcknowledge(BUTTON3);
  }
  
} /* end TiltControllerSM_Idle() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void TiltControllerSM_Error(void)          
{
  
} /* end TiltControllerSM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
