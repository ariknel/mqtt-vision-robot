package com.ariknel.mqttvisionrobot

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import com.ariknel.mqttvisionrobot.ui.theme.MqttVisionRobotTheme
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        // Start broker service
        startForegroundService(Intent(this, MqttBrokerService::class.java))

        // Connect MQTT client after short delay to let broker start
        CoroutineScope(Dispatchers.IO).launch {
            delay(1500)
            MqttClientManager.connect(this@MainActivity)
            MqttClientManager.setOnMessageCallback { topic, message ->
                TelemetryParser.parse(topic, message)
            }
            RobotState.isConnected = MqttClientManager.isConnected()
        }

        setContent {
            MqttVisionRobotTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = Color(0xFF0D0D0D)
                ) {
                    RobotControlScreen()
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        MqttClientManager.disconnect()
        stopService(Intent(this, MqttBrokerService::class.java))
    }
}