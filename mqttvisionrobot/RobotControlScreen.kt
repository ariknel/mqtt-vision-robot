package com.ariknel.mqttvisionrobot

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import java.util.Locale
import kotlin.math.abs
import kotlin.math.min

@Composable
fun RobotControlScreen() {
    val context = LocalContext.current
    var tiltDirection by remember { mutableStateOf("stop") }
    var lastSentSpeed by remember { mutableIntStateOf(-1) }

    DisposableEffect(RobotState.isTiltMode) {
        if (!RobotState.isTiltMode) {
            return@DisposableEffect onDispose {}
        }

        val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
        val sensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)

        val listener = object : SensorEventListener {
            override fun onSensorChanged(event: SensorEvent) {
                val x = event.values[0]
                val y = event.values[1]

                val forwardTrigger = RobotState.tiltSensitivity - RobotState.forwardOffset
                val backTrigger    = RobotState.tiltSensitivity + RobotState.reverseGuard
                val sideTrigger    = RobotState.tiltSensitivity

                val direction = when {
                    y < -forwardTrigger -> "forward"
                    y >  backTrigger    -> "back"
                    x < -sideTrigger    -> "right"
                    x >  sideTrigger    -> "left"
                    else                -> "stop"
                }

                val tiltMagnitude = when (direction) {
                    "forward", "back" -> abs(y)
                    "left", "right"   -> abs(x)
                    else              -> 0f
                }

                val trigger = when (direction) {
                    "back"          -> backTrigger
                    else            -> forwardTrigger
                }

                val rawSpeed = if (direction == "stop") {
                    0
                } else {
                    val progress = min(1f, (tiltMagnitude - trigger) / RobotState.fullSpeedRange)
                    (80 + (progress * 175)).toInt().coerceIn(80, 255)
                }

                if (direction != tiltDirection) {
                    tiltDirection = direction
                    MqttClientManager.sendMove(direction)
                }

                if (abs(rawSpeed - lastSentSpeed) > 8) {
                    lastSentSpeed = rawSpeed
                    RobotState.speed = rawSpeed
                    MqttClientManager.sendSpeed(rawSpeed)
                }
            }
            override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
        }

        sensorManager.registerListener(listener, sensor, SensorManager.SENSOR_DELAY_GAME)
        onDispose {
            sensorManager.unregisterListener(listener)
            tiltDirection = "stop"
            lastSentSpeed = -1
            MqttClientManager.sendMove("stop")
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(Color(0xFF0A0A0F))
            .systemBarsPadding()
            .padding(horizontal = 20.dp, vertical = 12.dp)
            .verticalScroll(rememberScrollState()),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        StatusBar()
        ControlPad(tiltDirection = tiltDirection)
        SpeedSlider()
        TiltSensitivityPanel()
        TelemetryPanel()
        Spacer(modifier = Modifier.height(8.dp))
    }
}

@Composable
fun StatusBar() {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(Color(0xFF161622))
            .padding(horizontal = 16.dp, vertical = 10.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Box(
                modifier = Modifier
                    .size(10.dp)
                    .clip(CircleShape)
                    .background(
                        if (RobotState.isConnected) Color(0xFF00FF88)
                        else Color(0xFFFF3333)
                    )
            )
            Spacer(modifier = Modifier.width(8.dp))
            Text(
                text = if (RobotState.isConnected) "CONNECTED" else "DISCONNECTED",
                color = if (RobotState.isConnected) Color(0xFF00FF88) else Color(0xFFFF3333),
                fontSize = 13.sp,
                fontWeight = FontWeight.Bold,
                letterSpacing = 1.sp
            )
        }

        Button(
            onClick = { RobotState.isTiltMode = !RobotState.isTiltMode },
            colors = ButtonDefaults.buttonColors(
                containerColor = if (RobotState.isTiltMode) Color(0xFF0055DD) else Color(0xFF222233)
            ),
            shape = RoundedCornerShape(8.dp),
            contentPadding = PaddingValues(horizontal = 16.dp, vertical = 6.dp)
        ) {
            Text(
                text = if (RobotState.isTiltMode) "🎮 TILT" else "🕹 WASD",
                fontSize = 13.sp,
                fontWeight = FontWeight.Bold
            )
        }
    }
}

@Composable
fun ControlPad(tiltDirection: String) {
    val accentColor = Color(0xFF0066FF)
    val activeColor = Color(0xFF00CCFF)
    val isTilt = RobotState.isTiltMode

    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        DpadButton(
            label = "▲",
            isActive = isTilt && tiltDirection == "forward",
            activeColor = activeColor,
            defaultActiveColor = accentColor
        ) { MqttClientManager.sendMove("forward") }

        Row(
            horizontalArrangement = Arrangement.spacedBy(10.dp),
            verticalAlignment = Alignment.CenterVertically
        ) {
            DpadButton(
                label = "◀",
                isActive = isTilt && tiltDirection == "left",
                activeColor = activeColor,
                defaultActiveColor = accentColor
            ) { MqttClientManager.sendMove("left") }

            Button(
                onClick = { MqttClientManager.sendMove("stop") },
                modifier = Modifier.size(76.dp),
                shape = CircleShape,
                colors = ButtonDefaults.buttonColors(containerColor = Color(0xFFCC2222)),
                elevation = ButtonDefaults.buttonElevation(8.dp)
            ) {
                Text("■", fontSize = 26.sp, color = Color.White)
            }

            DpadButton(
                label = "▶",
                isActive = isTilt && tiltDirection == "right",
                activeColor = activeColor,
                defaultActiveColor = accentColor
            ) { MqttClientManager.sendMove("right") }
        }

        DpadButton(
            label = "▼",
            isActive = isTilt && tiltDirection == "back",
            activeColor = Color(0xFFFF6600),
            defaultActiveColor = accentColor
        ) { MqttClientManager.sendMove("back") }

        if (isTilt) {
            Text(
                text = "TILT: ${tiltDirection.uppercase(Locale.US)}",
                color = if (tiltDirection == "stop") Color(0xFF555566) else Color(0xFF00CCFF),
                fontSize = 12.sp,
                fontWeight = FontWeight.Bold,
                letterSpacing = 2.sp
            )
        }
    }
}

@Composable
fun DpadButton(
    label: String,
    isActive: Boolean,
    activeColor: Color,
    defaultActiveColor: Color,
    onClick: () -> Unit
) {
    val interactionSource = remember { MutableInteractionSource() }
    val isPressed by interactionSource.collectIsPressedAsState()
    val isTilt = RobotState.isTiltMode

    LaunchedEffect(isPressed) {
        if (!isTilt) {
            if (isPressed) onClick()
            else MqttClientManager.sendMove("stop")
        }
    }

    val bgColor = when {
        isActive             -> activeColor
        isPressed && !isTilt -> defaultActiveColor
        else                 -> Color(0xFF161622)
    }

    Button(
        onClick = { if (!isTilt) onClick() },
        interactionSource = interactionSource,
        modifier = Modifier
            .size(82.dp)
            .border(
                width = if (isActive || (isPressed && !isTilt)) 2.dp else 0.dp,
                color = Color(0xFF00CCFF),
                shape = RoundedCornerShape(18.dp)
            ),
        shape = RoundedCornerShape(18.dp),
        colors = ButtonDefaults.buttonColors(containerColor = bgColor),
        elevation = ButtonDefaults.buttonElevation(
            defaultElevation = 4.dp,
            pressedElevation = 1.dp
        )
    ) {
        Text(label, fontSize = 30.sp, color = Color.White)
    }
}

@Composable
fun SpeedSlider() {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(Color(0xFF161622))
            .padding(horizontal = 16.dp, vertical = 10.dp)
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                "SPEED", color = Color(0xFF6666AA), fontSize = 11.sp,
                fontWeight = FontWeight.Bold, letterSpacing = 2.sp
            )
            Text(
                "${RobotState.speed}",
                color = Color(0xFF0066FF),
                fontSize = 13.sp,
                fontWeight = FontWeight.Bold
            )
        }
        Slider(
            value = RobotState.speed.toFloat(),
            onValueChange = {
                RobotState.speed = it.toInt()
                MqttClientManager.sendSpeed(RobotState.speed)
            },
            valueRange = 0f..255f,
            colors = SliderDefaults.colors(
                thumbColor = Color(0xFF0066FF),
                activeTrackColor = Color(0xFF0066FF),
                inactiveTrackColor = Color(0xFF2A2A3A)
            )
        )
    }
}

@Composable
fun TiltSensitivityPanel() {
    if (!RobotState.isTiltMode) return

    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(12.dp))
            .background(Color(0xFF161622))
            .border(1.dp, Color(0xFF2A2A4A), RoundedCornerShape(12.dp))
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(horizontal = 16.dp, vertical = 12.dp),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                "TILT SENSITIVITY",
                color = Color(0xFF6666AA),
                fontSize = 11.sp,
                fontWeight = FontWeight.Bold,
                letterSpacing = 2.sp
            )
            TextButton(
                onClick = { RobotState.sensitivityPanelExpanded = !RobotState.sensitivityPanelExpanded },
                contentPadding = PaddingValues(0.dp)
            ) {
                Text(
                    text = if (RobotState.sensitivityPanelExpanded) "▲ HIDE" else "▼ SHOW",
                    color = Color(0xFF0066FF),
                    fontSize = 11.sp,
                    fontWeight = FontWeight.Bold
                )
            }
        }

        if (RobotState.sensitivityPanelExpanded) {
            Column(
                modifier = Modifier
                    .padding(horizontal = 16.dp)
                    .padding(bottom = 12.dp),
                verticalArrangement = Arrangement.spacedBy(4.dp)
            ) {
                SensitivitySlider(
                    label = "SENSITIVITY",
                    value = RobotState.tiltSensitivity,
                    range = 0.5f..5f,
                    description = "Lower = easier to trigger"
                ) { RobotState.tiltSensitivity = it }

                SensitivitySlider(
                    label = "REVERSE GUARD",
                    value = RobotState.reverseGuard,
                    range = 0.5f..10f,
                    description = "Higher = harder to reverse"
                ) { RobotState.reverseGuard = it }
            }
        }
    }
}

@Composable
fun SensitivitySlider(
    label: String,
    value: Float,
    range: ClosedFloatingPointRange<Float>,
    description: String = "",
    onValueChange: (Float) -> Unit
) {
    Column(modifier = Modifier.fillMaxWidth()) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Column {
                Text(
                    label,
                    color = Color(0xFF6666AA),
                    fontSize = 10.sp,
                    fontWeight = FontWeight.Bold
                )
                if (description.isNotEmpty()) {
                    Text(
                        description,
                        color = Color(0xFF444455),
                        fontSize = 9.sp
                    )
                }
            }
            Text(
                String.format(Locale.US, "%.1f", value),
                color = Color(0xFF0066FF),
                fontSize = 12.sp,
                fontWeight = FontWeight.Bold
            )
        }
        Slider(
            value = value,
            onValueChange = onValueChange,
            valueRange = range,
            colors = SliderDefaults.colors(
                thumbColor = Color(0xFF0066FF),
                activeTrackColor = Color(0xFF0066FF),
                inactiveTrackColor = Color(0xFF2A2A3A)
            )
        )
    }
}

@Composable
fun TelemetryPanel() {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(16.dp))
            .background(Color(0xFF161622))
            .border(1.dp, Color(0xFF2A2A4A), RoundedCornerShape(16.dp))
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(14.dp)
    ) {
        Text(
            "TELEMETRY",
            color = Color(0xFF6666AA),
            fontSize = 11.sp,
            fontWeight = FontWeight.Bold,
            letterSpacing = 2.sp
        )

        TelemetryRow(label = "IR SENSORS") {
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                IrIndicator("L", RobotState.irLeft)
                IrIndicator("C", RobotState.irCenter)
                IrIndicator("R", RobotState.irRight)
            }
        }

        HorizontalDivider(color = Color(0xFF2A2A3A))

        TelemetryRow(label = "ULTRASONIC") {
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                SensorValue("L", "${RobotState.ultraLeft}cm")
                SensorValue("C", "${RobotState.ultraCenter}cm")
                SensorValue("R", "${RobotState.ultraRight}cm")
            }
        }

        HorizontalDivider(color = Color(0xFF2A2A3A))

        TelemetryRow(label = "BATTERY") {
            val voltage = RobotState.batteryVoltage
            val battColor = when {
                voltage > 7.8f -> Color(0xFF00FF88)
                voltage > 7.2f -> Color(0xFFFFAA00)
                else           -> Color(0xFFFF3333)
            }
            Text(
                "${String.format(Locale.US, "%.2f", voltage)}V",
                color = battColor,
                fontSize = 14.sp,
                fontWeight = FontWeight.Bold
            )
        }

        HorizontalDivider(color = Color(0xFF2A2A3A))

        TelemetryRow(label = "MOTORS") {
            Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
                SensorValue("A", "${RobotState.motorA}")
                SensorValue("B", "${RobotState.motorB}")
            }
        }
    }
}

@Composable
fun TelemetryRow(label: String, content: @Composable () -> Unit) {
    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            label, color = Color(0xFF6666AA), fontSize = 11.sp,
            fontWeight = FontWeight.Bold, letterSpacing = 1.sp
        )
        content()
    }
}

@Composable
fun SensorValue(label: String, value: String) {
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text("$label:", color = Color(0xFF6666AA), fontSize = 12.sp)
        Spacer(modifier = Modifier.width(4.dp))
        Text(value, color = Color.White, fontSize = 13.sp, fontWeight = FontWeight.Bold)
    }
}

@Composable
fun IrIndicator(label: String, active: Boolean) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Box(
            modifier = Modifier
                .size(22.dp)
                .clip(CircleShape)
                .background(if (active) Color(0xFF00FF88) else Color(0xFF2A2A3A))
                .border(
                    1.dp,
                    if (active) Color(0xFF00FF88) else Color(0xFF3A3A5A),
                    CircleShape
                )
        )
        Spacer(modifier = Modifier.height(2.dp))
        Text(label, color = Color(0xFF6666AA), fontSize = 10.sp, fontWeight = FontWeight.Bold)
    }
}