#ifndef SERVO_H
#define SERVO_H

// Initialize PWM for MG996 servo
int servo_init(void);

// Set servo angle 0–180 degrees
void servo_set_angle(int angle);

// Disable PWM output
void servo_disable(void);

#endif
