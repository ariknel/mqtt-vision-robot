package com.ariknel.mqttvisionrobot

import android.content.Context
import android.util.Log
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence

object MqttClientManager {

    private const val TAG = "MqttClientManager"
    private const val BROKER_URL = "tcp://localhost:1883"
    private const val CLIENT_ID = "AndroidController"

    private var client: MqttClient? = null
    private var onMessageCallback: ((topic: String, message: String) -> Unit)? = null

    fun setOnMessageCallback(callback: (topic: String, message: String) -> Unit) {
        onMessageCallback = callback
    }

    fun connect(context: Context) {
        try {
            client = MqttClient(BROKER_URL, CLIENT_ID, MemoryPersistence())

            val options = MqttConnectOptions().apply {
                isAutomaticReconnect = true
                isCleanSession = true
                connectionTimeout = 10
            }

            client?.setCallback(object : MqttCallback {
                override fun connectionLost(cause: Throwable?) {
                    Log.w(TAG, "Connection lost: ${cause?.message}")
                }

                override fun messageArrived(topic: String, message: MqttMessage) {
                    val payload = message.toString()
                    Log.d(TAG, "Message on $topic: $payload")
                    onMessageCallback?.invoke(topic, payload)
                }

                override fun deliveryComplete(token: IMqttDeliveryToken?) {}
            })

            client?.connect(options)
            Log.d(TAG, "Connected to broker")

            // Subscribe to all telemetry
            client?.subscribe("robot/telemetry/#", 0)
            Log.d(TAG, "Subscribed to robot/telemetry/#")

        } catch (e: MqttException) {
            Log.e(TAG, "Connection failed: ${e.message}")
        }
    }

    fun publish(topic: String, payload: String) {
        try {
            if (client?.isConnected == true) {
                val message = MqttMessage(payload.toByteArray()).apply {
                    qos = 0
                }
                client?.publish(topic, message)
            } else {
                Log.w(TAG, "Cannot publish — not connected")
            }
        } catch (e: MqttException) {
            Log.e(TAG, "Publish failed: ${e.message}")
        }
    }

    fun isConnected(): Boolean = client?.isConnected == true

    fun disconnect() {
        try {
            client?.disconnect()
            client?.close()
            Log.d(TAG, "Disconnected")
        } catch (e: MqttException) {
            Log.e(TAG, "Disconnect failed: ${e.message}")
        }
    }

    // Control helpers
    fun sendMove(direction: String) = publish("robot/control/move", direction)
    fun sendSpeed(value: Int) = publish("robot/control/speed", value.toString())
    fun sendMode(mode: String) = publish("robot/control/mode", mode)
}