/**********************************************************************************************************************
File: user_app1.c                                                                

----------------------------------------------------------------------------------------------------------------------
To start a new task using this user_app1 as a template:
 1. Copy both user_app1.c and user_app1.h to the Application directory
 2. Rename the files yournewtaskname.c and yournewtaskname.h
 3. Add yournewtaskname.c and yournewtaskname.h to the Application Include and Source groups in the IAR project
 4. Use ctrl-h (make sure "Match Case" is checked) to find and replace all instances of "user_app1" with "yournewtaskname"
 5. Use ctrl-h to find and replace all instances of "UserApp1" with "YourNewTaskName"
 6. Use ctrl-h to find and replace all instances of "USER_APP1" with "YOUR_NEW_TASK_NAME"
 7. Add a call to YourNewTaskNameInitialize() in the init section of main
 8. Add a call to YourNewTaskNameRunActiveState() in the Super Loop section of main
 9. Update yournewtaskname.h per the instructions at the top of yournewtaskname.h
10. Delete this text (between the dashed lines) and update the Description below to describe your task
----------------------------------------------------------------------------------------------------------------------

Description:
This is a user_app1.c file template 

------------------------------------------------------------------------------------------------------------------------
API:

Public functions:


Protected System functions:
void UserApp1Initialize(void)
Runs required initialzation for the task.  Should only be called once in main init section.

void UserApp1RunActiveState(void)
Runs current task state.  Should only be called once in main loop.


**********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_UserApp1"
***********************************************************************************************************************/
/* New variables */
volatile u32 G_u32UserApp1Flags;                       /* Global state flags */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemFlags;                  /* From main.c */
extern volatile u32 G_u32ApplicationFlags;             /* From main.c */

extern volatile u32 G_u32SystemTime1ms;                /* From board-specific source file */
extern volatile u32 G_u32SystemTime1s;                 /* From board-specific source file */


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "UserApp1_" and be declared as static.
***********************************************************************************************************************/
static fnCode_type UserApp1_StateMachine;            /* The state machine function pointer */
//static u32 UserApp1_u32Timeout;                      /* Timeout counter used across states */

static u8 UserApp1_au8IdleMessage[] = "SYSTEM IDLE";

static u32 UserApp1_servoDty =  PWM_CDTY2_INIT;

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
Function: UserApp1Initialize

Description:
Initializes the State Machine and its variables.

Requires:
  -

Promises:
  - 
*/
void UserApp1Initialize(void)
{
  /* If good initialization, set state to Idle */
  if( 1 )
  {
    LCDClearChars(LINE1_START_ADDR, 40);
    LCDMessage(LINE1_START_ADDR, UserApp1_au8IdleMessage);
    UserApp1_StateMachine = UserApp1SM_Idle;
    
    //Set up PWM Channel 2 for SERVO, which is operated using GPIO -> PA_06 
    AT91C_BASE_PWMC_CH2->PWMC_CMR = PWM_CMR2_INIT;
    AT91C_BASE_PWMC_CH2->PWMC_CPRDR    = PWM_CPRD2_INIT;
    AT91C_BASE_PWMC_CH2->PWMC_CPRDUPDR = PWM_CPRD2_INIT;
    AT91C_BASE_PWMC_CH2->PWMC_CDTYR    = PWM_CDTY2_INIT; /* initialize with 5% duty -> should correspond to 0 degrees */
    AT91C_BASE_PWMC_CH2->PWMC_CDTYUPDR = PWM_CDTY2_INIT; /* Latch CDTY values */
    
    AT91C_BASE_PWMC->PWMC_ENA = AT91C_PWMC_CHID2;
  }
  else
  {
    /* The task isn't properly initialized, so shut it down and don't run */
    UserApp1_StateMachine = UserApp1SM_Error;
  }

} /* end UserApp1Initialize() */

  
/*----------------------------------------------------------------------------------------------------------------------
Function UserApp1RunActiveState()

Description:
Selects and runs one iteration of the current state in the state machine.
All state machines have a TOTAL of 1ms to execute, so on average n state machines
may take 1ms / n to execute.

Requires:
  - State machine function pointer points at current state

Promises:
  - Calls the function to pointed by the state machine function pointer
*/
void UserApp1RunActiveState(void)
{
  UserApp1_StateMachine();

} /* end UserApp1RunActiveState */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Private functions                                                                                                  */
/*--------------------------------------------------------------------------------------------------------------------*/


/**********************************************************************************************************************
State Machine Function Definitions
**********************************************************************************************************************/

/*-------------------------------------------------------------------------------------------------------------------*/
/* Wait for ??? */
static void UserApp1SM_Idle(void)
{
  static bool button0Active = FALSE;
  static bool button1Active = FALSE;
  static u8 (*au8CurrentMessage)[] = &UserApp1_au8IdleMessage;
  static u8 (*au8LastMessage)[] = &UserApp1_au8IdleMessage;
  static u8 au8UpMessage[] = "TILTING UP";
  static u8 au8DownMessage[] = "TILTING DOWN";
  
  static u32 currentServoDty = PWM_CDTY2_INIT;
  static u32 currentServoPeriod = PWM_CPRD2_INIT;
  
  
  
  // button 0 controls tilting UP. Should not actuate if button 1 is also being pressed
  /* start button 0  */
  if(IsButtonPressed(BUTTON0)) {
    if(!button0Active && !button1Active) {
      button0Active = TRUE;
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (SODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_SODR) + 0x80;
      //Turn Actuator ON
      *pu32SetAddress = PB_05_ACTUATOR_UP;
      
      au8CurrentMessage = &au8UpMessage;  
    }
        //For DEMO only -> increase servo angle
    if(currentServoDty < (PWM_CDTY2_MAX + 1)) {
      currentServoDty= currentServoDty + 1;
      
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
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (CODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_CODR) + 0x80;
      //Turn Actuator OFF
      *pu32SetAddress = PB_05_ACTUATOR_UP;
  }
  /* end button 0 */
  
  // button 1 controls tilting DOWN. Should not actuate if button 0 is also being pressed
  /* start button 1 */
  if(IsButtonPressed(BUTTON1)) {
    if(!button1Active && !button0Active) {
      button1Active = TRUE;
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (SODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_SODR) + 0x80;
      //Turn Actuator ON
      *pu32SetAddress = PB_08_ACTUATOR_DOWN;
      
      au8CurrentMessage = &au8DownMessage;
      
      
    }
    if(currentServoDty > (PWM_CDTY2_MIN - 1)) {
      currentServoDty = currentServoDty - 1;
      
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
      u32 *pu32SetAddress;
      //build address for Actuator Set output data register (CODR)
      pu32SetAddress = (u32*)&(AT91C_BASE_PIOA->PIO_CODR) + 0x80;
      //Turn Actuator OFF
      *pu32SetAddress = PB_08_ACTUATOR_DOWN;
  }
  /* end button 1 */
  
  /* if system is idle, set idle message */
  if (!button0Active && !button1Active) {
      au8CurrentMessage = &UserApp1_au8IdleMessage;
  }
  
  /* if message has changed, update the screen */
  if(au8CurrentMessage != au8LastMessage) {
      LCDClearChars(LINE1_START_ADDR, 40);
      LCDMessage(LINE1_START_ADDR, *au8CurrentMessage);
      au8LastMessage = au8CurrentMessage;
  }
  
  
  if(WasButtonPressed(BUTTON2)) {
    ButtonAcknowledge(BUTTON2);
    if(currentServoPeriod < PWM_CPRD2_MAX ) {
      currentServoPeriod = currentServoPeriod + 500;
      
      AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID2;
      
      AT91C_BASE_PWMC_CH2->PWMC_CPRDR = currentServoPeriod;
      AT91C_BASE_PWMC_CH2->PWMC_CPRDUPDR = currentServoPeriod; 
      AT91C_BASE_PWMC->PWMC_ENA = AT91C_PWMC_CHID2; 
    }
  }
  if(WasButtonPressed(BUTTON3)) {
    ButtonAcknowledge(BUTTON3);
    if(currentServoPeriod > PWM_CPRD2_MIN ) {
      currentServoPeriod = currentServoPeriod - 500;
      
      AT91C_BASE_PWMC->PWMC_DIS = AT91C_PWMC_CHID2;
      AT91C_BASE_PWMC_CH2->PWMC_CPRDR = currentServoPeriod;
      AT91C_BASE_PWMC_CH2->PWMC_CPRDUPDR = currentServoPeriod; 
      AT91C_BASE_PWMC->PWMC_ENA = AT91C_PWMC_CHID2; 
    } 
  }
  
} /* end UserApp1SM_Idle() */

/*-------------------------------------------------------------------------------------------------------------------*/
/* Handle an error */
static void UserApp1SM_Error(void)          
{
  
} /* end UserApp1SM_Error() */



/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File                                                                                                        */
/*--------------------------------------------------------------------------------------------------------------------*/
