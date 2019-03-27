#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <WiFi.h>
#include <string.h>
#include <math.h>
#include "Source.h"
#include "Arduino.h"
#include "EEPROM.h"
#include "constants.h"
#include "Servo_Control.hpp"
#include "Motor_Control_rev1.hpp"

void initServer(AsyncWebServer* server, ParamsStruct* params) {
    //Create Access Point
    WiFi.softAP("Drive", "drive1234");
    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
    
    AsyncEventSource events("/events");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

    /* XHR Example.
        - Param "name" is sent from mission control.
        - "name" is copied to the params object
        - params object is then passed to vSayHelloTask - see main.cpp
        - vSayHello task then accesses name directly.
        Note: for ANY parameters you want to use, you must add them to
        the paramsStruct struct located in Source.h first. 
    */


    server->on("/handle_update", HTTP_POST, [=](AsyncWebServerRequest *request){
        const char *vars[9] = {
            "mode", "AXIS_X", "AXIS_Y", "THROTTLE", 
            "button_0", "wheel_A", "wheel_B", "wheel_C", "mast_position"
        };
        for (int i=0; i<9; i++) {
            if (request->hasArg(vars[i])) {
                if (strcmp(vars[i], "mode")) {
                    params->mode = request->arg("mode").toInt();  
                }
                if (strcmp(vars[i], "AXIS_X")) {
                    params->AXIS_X = 0 - (request->arg("AXIS_X").toFloat());
                    if (params->AXIS_X == -1)
                    {
                        params->AXIS_X = -0.99;
                    }   
                }
                if (strcmp(vars[i], "AXIS_Y")) {
                    params->AXIS_Y = request->arg("AXIS_Y").toFloat();    
                }
                if (strcmp(vars[i], "THROTTLE")) {
                    params->THROTTLE = (0 - (request->arg("THROTTLE").toFloat()) + 1)/2;
                    params->THROTTLE = params->THROTTLE * 0.3;    
                }
                if (strcmp(vars[i], "button_0")) {
                    params->button_0 = request->arg("button_0").toInt();    
                }
                if (strcmp(vars[i], "wheel_A")) {
                    params->wheel_A = request->arg("wheel_A").toInt();  
                }
                if (strcmp(vars[i], "wheel_B")) {
                    params->wheel_B = request->arg("wheel_B").toInt();  
                }
                if (strcmp(vars[i], "wheel_C")) {
                    params->wheel_C = request->arg("wheel_C").toInt();  
                }
                if (strcmp(vars[i], "mast_position")) {
                    params->mast_position = request->arg("mast_position").toFloat();    
                }
            }
            else 
            {
                printf("ERROR. %s doesn't exist\n", vars[i]);
            }
        }
        /*
        printf("handle_update endpoint running\n");
        printf("    mode: %i \n", params->mode);
        printf("    AXIS_X: %f \n", params->AXIS_X);
        printf("    AXIS_Y: %f \n", params->AXIS_Y);
        printf("    THROTTLE: %f \n", params->THROTTLE);
        printf("    button_0: %d \n", params->button_0);
        printf("    wheel_A: %d \n", params->wheel_A);
        printf("    wheel_B: %d \n", params->wheel_B);
        printf("    wheel_C: %d \n", params->wheel_C);
        printf("    mast_position: %f \n", params->mast_position);
        printf("\n");
        */
        request->send(200, "text/plain", "Success");
    });
    /* SSE Example.
        - SSEs will be used to continuously send data that was
        not necessarily requested by mission control
        (e.g. temperature, something we should send periodically)
        - Once mission control declares the ESPs IP address at a certain 
        endpoint to be an EventSource, the ESP can trigger events on the web
        interface, which the web interface can attach event listeners to
        (similar to how we are attaching event listeners for when we recieve
        XHRs to /update_name above, allowing us to do things when we recieve an 
        XHR).
        - Below's example is an example of sending SSEs when mission control
        declares our ip address and endpoint (e.g. 192.168.4.1/events) to be
        an event source.
        - More info on this concept here: 
            https://developer.mozilla.org/en-US/docs/Web/API/EventSource
    */
    
    events.onConnect([](AsyncEventSourceClient *client) 
    {
        if(client->lastId())
        {
         Serial.printf("Client reconnected! Last message ID that it gat is: %u\n", client->lastId());
        }
        // send event with message "hello!", id current millis
        // and set reconnect delay to 1 second
        client->send("hello!", NULL, millis(), 1000);
        delay(1000);
        client->send("hello!", NULL, millis(), 1000);
        delay(1000);
        client->send("hello!", NULL, millis(), 1000);
        delay(1000);
        client->send("hello!", NULL, millis(), 1000);
        delay(1000);
    });

    //Attach event source to the server.
    server->addHandler(&events);

    //Start server.
    server->begin();
    //printf("initServer done\n");
}

void initComponents()
{
    ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1);

    servo_A.InitServo(SERVO_A_PIN, SERVO_A_CHANNEL, SERVO_TIMER, 
                      SERVO_FREQUENCY, SERVO_MAX, SERVO_MIN);

    servo_B.InitServo(SERVO_B_PIN, SERVO_B_CHANNEL, SERVO_TIMER,
                      SERVO_FREQUENCY, SERVO_MAX, SERVO_MIN);


    servo_C.InitServo(SERVO_C_PIN, SERVO_C_CHANNEL, SERVO_TIMER,
                      SERVO_FREQUENCY, SERVO_MAX, SERVO_MIN);

    motor_A.InitMotor(MOTOR_A_PIN, MOTOR_A_BRAKE, MOTOR_A_DIR, MOTOR_A_CHANNEL,
                      MOTOR_TIMER, MOTOR_FREQUENCY, MOTOR_MIN, MOTOR_MAX);

    motor_B.InitMotor(MOTOR_B_PIN, MOTOR_B_BRAKE, MOTOR_B_DIR, MOTOR_B_CHANNEL,
                      MOTOR_TIMER, MOTOR_FREQUENCY, MOTOR_MIN, MOTOR_MAX);
    
    motor_C.InitMotor(MOTOR_C_PIN, MOTOR_B_BRAKE, MOTOR_C_DIR, MOTOR_C_CHANNEL,
                      MOTOR_TIMER, MOTOR_FREQUENCY, MOTOR_MIN, MOTOR_MAX);

}

void initDriveMode(uint32_t heading)
{
    switch(heading)
    {
        case 0:
            servo_A.SetPositionPercent(DRIVE_POSITION_0[0]);
            servo_B.SetPositionPercent(DRIVE_POSITION_0[1]);
            servo_C.SetPositionPercent(DRIVE_POSITION_0[2]);
            break;
        case 1:            
            servo_A.SetPositionPercent(DRIVE_POSITION_1[0]);
            servo_B.SetPositionPercent(DRIVE_POSITION_1[1]);
            servo_C.SetPositionPercent(DRIVE_POSITION_1[2]);
            break;
        case 2:
            servo_A.SetPositionPercent(DRIVE_POSITION_2[0]);
            servo_B.SetPositionPercent(DRIVE_POSITION_2[1]);
            servo_C.SetPositionPercent(DRIVE_POSITION_2[2]);
            break;
        default:
            initDriveMode(0);
            break;
    }
}

void initSpinMode(bool direction)
{
    if (!direction)
    {
        servo_A.SetPositionPercent(ROTATE_POSITION_0);
        servo_B.SetPositionPercent(ROTATE_POSITION_0);
        servo_C.SetPositionPercent(ROTATE_POSITION_0);
    }
    else
    {
        servo_A.SetPositionPercent(ROTATE_POSITION_1);
        servo_B.SetPositionPercent(ROTATE_POSITION_1);
        servo_C.SetPositionPercent(ROTATE_POSITION_1);
    }
    printf("Spin Mode set to position %d.\n", direction);
}

void initCrabMode()
{
    servo_A.SetPositionPercent(50);
    servo_B.SetPositionPercent(50);
    servo_C.SetPositionPercent(50);
}

void setDirectionAllWheels(bool direction)
{
    motor_A.SetDirection(direction);
    motor_B.SetDirection(direction);
    motor_C.SetDirection(direction);
}

void setDirection(uint32_t wheel, bool direction)
{
    switch(wheel)
    {
        case 0: 
            motor_A.SetDirection(direction);
            break;
        case 1:
            motor_B.SetDirection(direction);
            break;
        case 2:
            motor_C.SetDirection(direction);
            break;
        default: break;
    }
}

void setSpeedAllWheels(double speed)
{
    motor_A.SetSpeed(speed);
    motor_B.SetSpeed(speed);
    motor_C.SetSpeed(speed);
}

void setSpeed(uint32_t wheel, uint32_t speed)
{
    switch(wheel)
    {
        case 0: 
            motor_A.SetSpeed(speed);
            break;
        case 1:
            motor_B.SetSpeed(speed);
            break;
        case 2:
            motor_C.SetSpeed(speed);
            break;
        default: break;
    }
}

void setHeading(uint32_t wheel, double percentage)
{
    switch(wheel)
    {
        case 0: 
            servo_A.SetPositionPercent(percentage);
            break;
        case 1:
            servo_B.SetPositionPercent(percentage);
            break;
        case 2:
            servo_C.SetPositionPercent(percentage);
            break;
        default: break;
    }
}

void applyBrakes(bool signal)
{
    //motor_A.Brake(percentage);
    motor_B.Brake(!signal);
    //motor_C.Brake(percentage);
    //printf("brakes applied\n");
}

double fmap(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

double driveModeMapping(double x, double y)
{
    // convert raw data to target angle
    double target_angle = atan2(fabs(x), fabs(y));
    double param1 = (target_angle * 180/3.1416) / 90;
    double param2 = 0;
    if(target_angle * 180/3.1416 == 45)
    {
        param2 = sqrt(pow(x, 2) + pow(y, 2)) / sqrt(2);
    }
    else if (target_angle * 180/3.1416 > 45)
    {
        param2 = sqrt(pow(x, 2) + pow(y, 2)) / sqrt(1 + pow(y, 2));
    }
    else
    {
        param2 = sqrt(pow(x, 2) + pow(y, 2)) / sqrt(pow(x, 2) + 1);
    }
    target_angle = param1 * param2 * 45;
    return target_angle;
}
/*
char *getHeading(double gps_data);
{

}
*/
/*bool getCurrentDirection(uint32_t wheel)
{
    bool direction;
    switch(wheel)
    {
        case 0:
            direction = motor_A->direction;
            break;
        case 1:
            direction = motor_B->direction;
            break;
        case 2:
            direction = motor_C->direction;
            break;
        default:
            direction = false;
            break;
    }
    return direction;
}*/