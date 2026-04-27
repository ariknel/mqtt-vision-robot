package com.ariknel.mqttvisionrobot

import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue

object RobotState {

    var isConnected by mutableStateOf(false)
    var isTiltMode by mutableStateOf(false)
    var speed by mutableIntStateOf(180)

    var irLeft by mutableStateOf(false)
    var irCenter by mutableStateOf(false)
    var irRight by mutableStateOf(false)

    var ultraLeft by mutableIntStateOf(0)
    var ultraCenter by mutableIntStateOf(0)
    var ultraRight by mutableIntStateOf(0)

    var batteryVoltage by mutableFloatStateOf(0f)
    var motorA by mutableIntStateOf(0)
    var motorB by mutableIntStateOf(0)

    // Forward triggers at tiltSensitivity - 0.6 (earlier than left/right)
    // Left/right trigger at tiltSensitivity
    // Back triggers at tiltSensitivity + reverseGuard
    // reverseGuard max 10f = practically disabled
    var tiltSensitivity by mutableFloatStateOf(2.0f)
    var reverseGuard by mutableFloatStateOf(2.5f)

    val forwardOffset = 0.6f   // forward triggers this much earlier than sides
    val fullSpeedRange = 4.0f  // tilt range from trigger to full speed

    var sensitivityPanelExpanded by mutableStateOf(false)
}