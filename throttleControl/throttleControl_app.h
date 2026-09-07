#ifndef THROTTLECONTROL_APP_H_
#define THROTTLECONTROL_APP_H_

// Called periodically from the real time control loop
void throttleControl_appCyclicEntryPoint(void);
// Called once when the application starts
void throttleControl_appInitAll(void);

#endif // THROTTLECONTROL_APP_H_