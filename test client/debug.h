#ifndef DEBUG_H
#define DEBUG_H	1

#if		DEBUGLEVEL>0
#define err(fmt,...)	do { spinLockDebugSystem();printf("ERROR:%s %d ", __FUNCTION__, __LINE__);printf(fmt,__VA_ARGS__); spinUnLockDebugSystem();} while (0)
#if 		DEBUGLEVEL>1
#define notice(fmt,...)	do {spinLockDebugSystem();printf("NOTICE: %s %d " ,__FUNCTION__, __LINE__);printf(fmt,__VA_ARGS__); spinUnLockDebugSystem(); } while (0)
#if		DEBUGLEVEL>2

#define monitor(fmt,...)	do {spinLockDebugSystem(); printf("MON:%s %d " ,__FUNCTION__, __LINE__);printf(fmt,__VA_ARGS__);spinUnLockDebugSystem();} while (0)

#else
#define monitor(fmt,...)	
#endif
#else
#define monitor(fmt, ...)	 do { } while(0)
#define notice(fmt, ...)	do { } while(0)
#endif
#else
#define monitor(fmt,...)	 do { } while(0)
#define notice(fmt,...)	do { } while(0)
#define err(fmt,...)	do { } while(0)
#endif

#endif