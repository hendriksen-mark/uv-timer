'****************************************************************
'*  Name    : UV PCB Exposure System Timer                      *
'*  Author  : Stijn Coenen [Stynus]                             *
'*  Notice  : Licensed under Creative Commons                   *
'*          : Attribution-Noncommercial 2.0 Belgium Licence     *
'*          : http://creativecommons.org/licenses/by-nc/2.0/be/ *
'*  Date    : 28/08/2011                                        *
'*  Version : 2.11                                              *
'*  Url     : English: en.elektronicastynus.be/projects/81/     *
'*          : Dutch:  www.elektronicastynus.be/Projecten/81/    *
'****************************************************************
Device  16F628A
Config  INTRC_OSC_NOCLKOUT, WDT_off, PWRTE_ON, LVP_off, MCLRE_OFF
All_Digital     TRUE 
Xtal  = 4     
'Set the default data in the eeprom at programming
    EData 0,0,10, 107   'Singe Sided, Exposure Time minutes, Exposure Time Seconds, Interrupt preload                      
'**************************************************************** 
'In and outputs 
    'Tris registers
            '76543210
    TRISA = %01100000
    TRISB = %00001111
    Symbol Relay1 = PORTA.2
    Symbol Relay2 = PORTA.3
    Symbol Buzzer = PORTA.7
    High Buzzer                 'Buzzer is connected to the 5V, so pin must be high to disable
    Symbol Btn_1  = PORTB.2     '3 input buttons
    Symbol Btn_2  = PORTB.1
    Symbol Btn_3  = PORTB.0
    'Lcd screen
    Declare LCD_ENPin  PORTA.1
    Declare LCD_RSPin  PORTA.0
    Declare LCD_DTPin  PORTB.4
    Declare LCD_Interface 4 
    Declare LCD_Lines 2
    Symbol LcdTime  = 200 'Time for each step at the startup message and hidden menu
    'On-chip pull-up resistors active
    PortB_Pullups On               
    
'Settings:
    Dim  SingleDouble   As Bit
    Dim  Time_Min       As Byte
    Dim  Time_Sec       As Byte
    Dim  Count_Min      As Byte
    Dim  Count_Sec      As Byte

'Interupt   
    On_Hardware_Interrupt GoTo Int_Routine 
    Symbol  T0IE        = INTCON.5        'TMR0 Overflow Interrupt Enable 
    Symbol  T0IF        = INTCON.2        'TMR0 Overflow Interrupt Flag 
    Symbol  GIE         = INTCON.7        'Global Interrupt Enable 
    Symbol  PS0         = OPTION_REG.0    'Prescaler bit-0 
    Symbol  PS1         = OPTION_REG.1    'Prescaler bit-1 
    Symbol  PS2         = OPTION_REG.2    'Prescaler bit-2  
    Symbol  PSA         = OPTION_REG.3    'Prescaler Assignment  
    Symbol  T0CS        = OPTION_REG.5    'Timer0 Clock Source Select
    Dim  Timer_Go       As Bit
    Dim  Int_Counter    As Byte
    Dim  Int_Preload    As Byte
    PSA                 = 0 
    PS0                 = 1 
    PS1                 = 1
    PS2                 = 1 
    T0CS                = 0                 
    TMR0                = 0 
    T0IF                = 0
    T0IE                = 1
    GIE                 = 1

'Eeprom adresses
    Symbol Ee_SS_DS     = 0 'Singe Sided / Double Sided
    Symbol Ee_Ti_Mi     = 1 'Exposure Time minutes
    Symbol Ee_Ti_Se     = 2 'Exposure Time Seconds
    Symbol Ee_Inter     = 3 'Interrupt preload value (for fine tuning the timing)
    Symbol Ee_Skip      = 4 'Skip startup message?    
    Symbol Ee_TopBot    = 5 'Is single sided top or bottom?  
    Symbol Ee_En_DS     = 6 'Is double sided enabled?  

'Div.:
    'Custom LCD characters
    Print $FE,$40,$04,$04,$04,$07,$00,$00,$00,$00
    Dim  Temp           As Byte     'Byte for temporary storage
    
'****************************************************************
StartUp:  
    Clear           'Clear ram
    Cls             'Clear lcd
    DelayMS 300
    
    'Enter hidden menu?
    If Btn_1 = 0 Then
        GoSub Hidden
    EndIf                
    
    'Read Eeprom data     
    Temp            = ERead Ee_SS_DS
    SingleDouble    = Temp.0
    Time_Min        = ERead Ee_Ti_Mi    
    Time_Sec        = ERead Ee_Ti_Se
    Int_Preload     = ERead Ee_Inter
             
    'Skip startup message?
    Temp            = ERead Ee_Skip
    If Temp <> 1 Then
        'Startup message           
                      '1234567890123456
        Print At 1,1, "UV Exposure Syst"
        Print At 2,1, "V2.1 Stynus 2011"
        DelayMS  LcdTime 
        DelayMS  LcdTime              
        Print At 1,1, "V Exposure Syste"
        DelayMS  LcdTime
        Print At 1,1, " Exposure System"
        DelayMS  LcdTime
        Print At 1,1, "Exposure System "
        DelayMS  LcdTime
        Print At 1,1, "xposure System T"
        DelayMS  LcdTime               
        Print At 1,1, "posure System Ti"
        DelayMS  LcdTime             
        Print At 1,1, "osure System Tim"
        DelayMS  LcdTime
        Print At 1,1, "sure System Time"
        DelayMS  LcdTime
        Print At 1,1, "ure System Timer"
        DelayMS  700
        Cls
    EndIf
    
'**************************************************************** 
StartScreen:             
While 1 = 1        '1234567890123456
    Print At 1, 1,  "Time:"
    Print At 1, 7, Dec2 Time_Min, ":", Dec2 Time_Sec 'Display the exposure time
    Print At 2, 1,  "Start"
    'If double sided is not enabled we don't need this setting
    Temp = ERead Ee_En_DS
    If Temp > 0 Then
        If SingleDouble = 0 Then
            Print At 2, 6, "|S.S.|"
        Else              
            Print At 2, 6, "|D.S.|"        
        EndIf
    EndIf
    
    Print At 2,12, "SetT"
    While 1 = 1     'Button read loop
        If Btn_1 = 0 Then
            'Start the timer
            GoSub Timer
            Break
        EndIf
        If Btn_2 = 0 Then   
            Temp = ERead Ee_En_DS
            'If double sided is possible then toggle between double and single sided
            If Temp > 0 Then
                GoSub Debounce_BTN_2
                SingleDouble =  ~ SingleDouble    'Toggle the value of SingleDouble (bit)
                EWrite  Ee_SS_DS, [SingleDouble]  'Write the setting to the eeprom
                Break
            EndIf
        EndIf  
        If Btn_3 = 0 Then
            'Enter the menu to change the exposure time
            GoSub Debounce_BTN_3
            GoSub TimeMenu
            Break
        EndIf
    Wend
Wend

'**************************************************************** 
Timer:
    'This sub contains the exposure timer
    Cls     'Clear the display
    'Load timer
    Count_Min  = Time_Min
    Count_Sec  = Time_Sec
    Timer_Go   = 1
    Restart:
    'Put the relays on  
    If SingleDouble = 0 Then
        'For single sided check if top or bottom is used.
        Temp = ERead Ee_TopBot
        If Temp = 0 Then
            High Relay1
        Else
            High Relay2  
        EndIf
        Temp = ERead Ee_En_DS
        If Temp > 0 Then
            Print At 1, 11, "(S.S.)"
        EndIf
    Else
        High Relay1
        High Relay2
        Print At 1, 11, "(D.S.)" 
    EndIf
    Print At 1,1,  "Status:On"
    Print At 2,12, "|Stop"
    'Timer check loop
    While 1 = 1
        'Print timer value    
         Print At 2, 1, Dec2 Count_Min, ":", Dec2 Count_Sec
         DelayMS 300  
        'Check if time is done 
        'The timer counts down in the interrupt routine)       
        If Timer_Go = 0 Then
            Break
        EndIf
        'If the right button is pressed the timer stops
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3
            Timer_Go = 0
            Break
        EndIf
    Wend
    Print At 1,1, "Status:Off      "
    'Turn relays back off
    Low Relay1
    Low Relay2
    'Is the time over, or is the stop button pressed?
    If Count_Min > 0 Or Count_Sec > 0 Then
        'Stop button   1234567890123456
        Print At 1,1, "Stopped at      "
        Print At 2,6,      " Start|Exit"
        '(Time is still on the second line from above)
        'Check the buttons
        While 1 = 1
            'If the middle button is pressed then the timer is restarted to do the remaining time
            If Btn_2 = 0 Then  
               GoSub Debounce_BTN_2
               Timer_Go   = 1
               GoTo Restart
            EndIf
            'If the right button is pressed then the timer is cancelled and the program returs from this sub
            If Btn_3 = 0 Then  
               GoSub Debounce_BTN_3
               Cls
               Return
            EndIf
        Wend
    Else
        'Exposure time over
        Cls           '1234567890123456
        Print At 1,1, "      DONE      "
        'Beep
        Buzzer = 0
        DelayMS 200
        Buzzer = 1
        DelayMS 250
        Buzzer = 0
        DelayMS 200
        Buzzer = 1   
        Print At 2,1, " Press Any Key. " 
        While 1 = 1 
            'Stay on this screen until a button is pressed
            If Btn_1 = 0 Then 
                GoSub Debounce_BTN_1
                Break
            EndIf   
            If Btn_2 = 0 Then 
                GoSub Debounce_BTN_2
                Break
            EndIf  
            If Btn_3 = 0 Then 
                GoSub Debounce_BTN_3
                Break
            EndIf    
        Wend
    EndIf
    'Wait for button press
    
Cls
Return   

'**************************************************************** 
TimeMenu:
    'This subroutine contains the menu to change the exposure time
    Cls     'Clear the display
    Temp = 1
    Print At 1, 1,  "Set Time:"
    Print At 1, 11, Dec2 Time_Min, ":", Dec2 Time_Sec 
    Print At 2, 1,  "  + |  -  | ", 0, "-> "
    Cursor 1, 11
    Print $FE, $0E     '   Underline cursor on  
    Print $FE, $0F      ' Blinking cursor on 
    'Minutes first digit 
    While 1 = 1
        If Btn_1 = 0 Then 
            If Time_Min < 89 Then
                Time_Min = Time_Min + 10
                Print At 1, 11, Dec2 Time_Min
                Cursor 1, 11
            EndIf
            GoSub Debounce_BTN_1
        EndIf   
        If Btn_2 = 0 Then   
            If Time_Min > 9 Then
                Time_Min = Time_Min - 10
                Print At 1, 11, Dec2 Time_Min
                Cursor 1, 11 
            EndIf
            GoSub Debounce_BTN_2
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3
            Break
        EndIf
    Wend
    'Minutes second digit 
    Cursor 1, 12
    While 1 = 1
        If Btn_1 = 0 Then   
            If Time_Min < 98 Then
                Time_Min = Time_Min + 1
                Print At 1, 11, Dec2 Time_Min
                Cursor 1, 12
            EndIf
            GoSub Debounce_BTN_1
        EndIf   
        If Btn_2 = 0 Then   
            If Time_Min > 0 Then
                Time_Min = Time_Min - 1
                Print At 1, 11, Dec2 Time_Min
                Cursor 1, 12 
            EndIf
            GoSub Debounce_BTN_2
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3
            Break
        EndIf
    Wend
    EWrite  Ee_Ti_Mi, [Time_Min] 
    'Seconds first digit 
    Cursor 1, 14
    While 1 = 1
        If Btn_1 = 0 Then   
            If Time_Sec < 49 Then
                Time_Sec = Time_Sec + 10
                Print At 1, 14, Dec2 Time_Sec
                Cursor 1, 14
            EndIf
            GoSub Debounce_BTN_1
        EndIf   
        If Btn_2 = 0 Then   
            If Time_Sec > 9 Then
                Time_Sec = Time_Sec - 10
                Print At 1, 14, Dec2 Time_Sec
                Cursor 1, 14 
            EndIf
            GoSub Debounce_BTN_2
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3
            Break
        EndIf
    Wend
    'Seconds second digit 
    Cursor 1, 15
    While 1 = 1
        If Btn_1 = 0 Then   
            If Time_Sec < 58 Then
                Time_Sec = Time_Sec + 1
                Print At 1, 14, Dec2 Time_Sec
                Cursor 1, 15
            EndIf
            GoSub Debounce_BTN_1
        EndIf   
        If Btn_2 = 0 Then   
            If Time_Sec > 0 Then
                Time_Sec = Time_Sec - 1
                Print At 1, 14, Dec2 Time_Sec
                Cursor 1, 15 
            EndIf
            GoSub Debounce_BTN_2
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3
            Break
        EndIf
    Wend
    EWrite  Ee_Ti_Se, [Time_Sec]
    Cls             'Clear display
    Print $FE, $0C  'Cursor off
Return
'****************************************************************
Debounce_BTN_1: 
    DelayMS 300
    While Btn_1 = 0
        DelayMS 300
    Wend
Return 
'****************************************************************
Debounce_BTN_2: 
    DelayMS 300
    While Btn_2 = 0
        DelayMS 300
    Wend
Return
'****************************************************************
Debounce_BTN_3: 
    DelayMS 300
    While Btn_3 = 0
        DelayMS 300
    Wend
Return
'****************************************************************
Hidden: 'Menu    
    'Adjust the timing
                  '1234567890123456
    Print At 1,1, " - Hidden Menu -"
    DelayMS  1000 '1234567890123456
    Print At 1,1, "Check manual for"
    Print At 2,1, "    instructions" 
    DelayMS  1000 '1234567890123456
    Print At 1,1, "Timing adjustmen"
    Print At 2,1, "Value:          "          
    Int_Preload     = ERead Ee_Inter
    Print At 2, 7, Dec3 Int_Preload
    While 1 = 1
        If Btn_1 = 0 Then   
            If Int_Preload < 255 Then
                Int_Preload = Int_Preload + 1
                Print At 2, 7, Dec3 Int_Preload
            EndIf
            GoSub Debounce_BTN_1
        EndIf   
        If Btn_2 = 0 Then   
            If Int_Preload > 0 Then
                Int_Preload = Int_Preload - 1
                Print At 2, 7, Dec3 Int_Preload
            EndIf
            GoSub Debounce_BTN_2
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3
            Break
        EndIf
    Wend
    EWrite Ee_Inter, [Int_Preload]
    Cls
    'Enable double sided exposure?
                  '1234567890123456
    Print At 1,1, "Enable double "
    Print At 2,1, "sided?  Yes|No"
    While 1 = 1
        If Btn_2 = 0 Then    
            GoSub Debounce_BTN_2
            EWrite Ee_En_DS, [1]
            Break
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3 
            EWrite Ee_En_DS, [0]
            Break
        EndIf
    Wend
    'Skip start up message?
                  '1234567890123456
    Print At 1,1, "Skip start up   "
    Print At 2,1, "message?  Yes|No"
    While 1 = 1
        If Btn_2 = 0 Then    
            GoSub Debounce_BTN_2
            EWrite Ee_Skip, [1]
            Break
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3 
            EWrite Ee_Skip, [0]
            Break
        EndIf
    Wend
    'Single side top or bottom?
                  '1234567890123456
    Print At 1,1, "Single side top/"
    Print At 2,1, "bottom?  Top|bot"
    While 1 = 1
        If Btn_2 = 0 Then    
            GoSub Debounce_BTN_2
            EWrite Ee_TopBot, [1]
            Break
        EndIf
        If Btn_3 = 0 Then   
            GoSub Debounce_BTN_3 
            EWrite Ee_TopBot, [0]
            Break
        EndIf
    Wend
    Cls
Return
'****************************************************************
Int_Routine:   'Interrupt routine to time the exposure time
Context Save
    If T0IF = 1 Then        'Check if the interrupt does come from timer 0
        T0IF = 0            'Reset timer overflow flag
        TMR0 = Int_Preload  'Preload the timer with the Int_Preload value
                            'This value can be set in the hidden menu
        Inc Int_Counter     ''Increment the timer
        If Int_Counter = 26 Then            '26 times the interrupt routine = 1sec
            Int_Counter = 0            
            If Timer_Go = 1 Then            'If the timer is enabled then decrease          
                If Count_Sec > 0 Then       'enough seconds left to dec?
                    Dec Count_Sec
                Else
                    If Count_Min > 0 Then   'enough minutes left to dec?
                        Dec Count_Min
                        Count_Sec = 59      
                    Else
                        Timer_Go = 0        'Time finished, disable timer
                    EndIf
                EndIf
            EndIf                             
        EndIf
    EndIf
Context Restore
'****************************************************************
End
