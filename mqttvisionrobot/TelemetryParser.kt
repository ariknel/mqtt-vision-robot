package com.ariknel.mqttvisionrobot

import android.util.Log
import org.json.JSONObject

object TelemetryParser {

    private const val TAG = "TelemetryParser"

    fun parse(topic: String, message: String) {
        try {
            when (topic) {
                "robot/telemetry/ir" -> parseIr(message)
                "robot/telemetry/ultrasonic" -> parseUltrasonic(message)
                "robot/telemetry/battery" -> parseBattery(message)
                "robot/telemetry/speed" -> parseSpeed(message)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Parse error on $topic: ${e.message}")
        }
    }

    private fun parseIr(message: String) {
        val json = JSONObject(message)
        RobotState.irLeft   = json.getInt("left")   == 1
        RobotState.irCenter = json.getInt("center") == 1
        RobotState.irRight  = json.getInt("right")  == 1
    }

    private fun parseUltrasonic(message: String) {
        val json = JSONObject(message)
        RobotState.ultraLeft   = json.getInt("left")
        RobotState.ultraCenter = json.getInt("center")
        RobotState.ultraRight  = json.getInt("right")
    }

    private fun parseBattery(message: String) {
        RobotState.batteryVoltage = message.toFloat()
    }

    private fun parseSpeed(message: String) {
        val json = JSONObject(message)
        RobotState.motorA = json.getInt("a")
        RobotState.motorB = json.getInt("b")
    }
}