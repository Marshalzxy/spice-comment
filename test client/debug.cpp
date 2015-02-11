#include <windows.h>
CRITICAL_SECTION gDebugCs;
void  initDebugSystem(void){
	InitializeCriticalSectionAndSpinCount(&gDebugCs,10);
}
void spinLockDebugSystem(){
	EnterCriticalSection(&gDebugCs);
}
void spinUnLockDebugSystem(){

	LeaveCriticalSection(&gDebugCs);
}