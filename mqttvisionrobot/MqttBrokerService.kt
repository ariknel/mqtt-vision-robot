package com.ariknel.mqttvisionrobot

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.IBinder
import android.util.Log
import io.moquette.broker.Server
import io.moquette.broker.config.MemoryConfig
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import java.util.Properties

class MqttBrokerService : Service() {

    private val broker = Server()
    private val scope = CoroutineScope(Dispatchers.IO)
    private val tag = "MqttBrokerService"
    private val channelId = "mqtt_broker_channel"
    private val notificationId = 1

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(notificationId, buildNotification())
        startBroker()
    }

    private fun startBroker() {
        scope.launch {
            try {
                val config = MemoryConfig(Properties().apply {
                    setProperty("port", "1883")
                    setProperty("host", "0.0.0.0")
                    setProperty("allow_anonymous", "true")
                })
                broker.startServer(config)
                Log.d(tag, "MQTT Broker started on port 1883")
            } catch (e: Exception) {
                Log.e(tag, "Failed to start broker: ${e.message}")
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        broker.stopServer()
        Log.d(tag, "MQTT Broker stopped")
    }

    override fun onBind(intent: Intent?): IBinder? = null

    private fun createNotificationChannel() {
        val channel = NotificationChannel(
            channelId,
            "MQTT Broker",
            NotificationManager.IMPORTANCE_LOW
        ).apply {
            description = "MQTT broker running for robot control"
        }
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(channel)
    }

    private fun buildNotification(): Notification {
        return Notification.Builder(this, channelId)
            .setContentTitle("MQTT Robot Broker")
            .setContentText("Broker running on port 1883")
            .setSmallIcon(android.R.drawable.ic_dialog_info)
            .build()
    }
}