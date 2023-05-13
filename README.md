# Power Window Control System
 Implementation of front passenger door window with both passenger and driver control panels using Tiva C running FreeRTOS operating system
## System Features
<ol>
 <li> <b>Manual open/close function</b> </li>
When the power window switch is pushed or pulled
continuously, the window opens or closes until the switch
is released.<br/>

<li> <b>One touch auto open/close function</b> </li>
When the power window switch is pushed or pulled
shortly, the window fully opens or closes.<br/>
 
<li> <b>Window lock function</b> </li>
When the window lock switch is turned on, the opening and closing of
all windows except the driver’s window is disabled.<br/>
 
<li> <b>Jam protection function</b> </li>
This function automatically stops the power window and moves it
downward about 0.5 second if foreign matter gets caught in the
window during one touch auto close operation.
<br/>
</ol>

## Implemented Buttons
![image](https://github.com/OmarElbanna/Power-Window-Control-System/assets/96841295/099be9d0-c9b2-47c6-8be0-0642babc920a)

## Used Components
<ol>
 <li> DC Motor</li>
 <li> Push Buttons</li>
 <li> ON/OFF Switch</li>
 <li> 9V Power Source</li>
 <li> Regulator</li>
 <li> Dual H Bridge DC Motor Driver L298N</li>

 </ol>

