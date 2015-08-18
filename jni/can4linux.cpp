#include<string>
#include<algorithm>
#include<utility>

#include<cstring>
#include<cstddef>
#include<cerrno>

extern "C" {
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <sys/ioctl.h>
#include <unistd.h>
#include <can4linux.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
}

#if defined(ANDROID) || defined(__ANDROID__)
#include "jni.h"
#else
#include "can4linux_CAN4LinuxAdapter.h"
#endif

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)


static const int ERRNO_BUFFER_LEN = 1024;

static void throwException(JNIEnv *env, const std::string& exception_name,
			   const std::string& msg)
{
	const jclass exception = env->FindClass(exception_name.c_str());
	if (exception == NULL) {
		return;
	}
	env->ThrowNew(exception, msg.c_str());
}

static void throwIOExceptionMsg(JNIEnv *env, const std::string& msg)
{
	throwException(env, "java/io/IOException", msg);
}

static void throwIOExceptionErrno(JNIEnv *env, const int exc_errno)
{
	char message[ERRNO_BUFFER_LEN];
	const char *const msg = (char *) strerror_r(exc_errno, message, ERRNO_BUFFER_LEN);
	if (((long)msg) == 0) {
		// POSIX strerror_r, success
		throwIOExceptionMsg(env, std::string(message));
	} else if (((long)msg) == -1) {
		// POSIX strerror_r, failure
		// (Strictly, POSIX only guarantees a value other than 0. The safest
		// way to implement this function is to use C++ and overload on the
		// type of strerror_r to accurately distinguish GNU from POSIX. But
		// realistic implementations will always return -1.)
		snprintf(message, ERRNO_BUFFER_LEN, "errno %d", exc_errno);
		throwIOExceptionMsg(env, std::string(message));
	} else {
		// glibc strerror_r returning a string
		throwIOExceptionMsg(env, std::string(msg));
	}
}

static void throwIllegalArgumentException(JNIEnv *env, const std::string& message)
{
	throwException(env, "java/lang/IllegalArgumentException", message);
}

static void throwOutOfMemoryError(JNIEnv *env, const std::string& message)
{
    	throwException(env, "java/lang/OutOfMemoryError", message);
}


/*
 * Class:     can4linux_CAN4LinuxAdapter
 * Method:    canOpen
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_can4linux_CAN4LinuxAdapter_canOpen(JNIEnv *env, jclass obj, jint port) {
  char dev[100];
  sprintf(dev,"/dev/can%d", port);
  const int fd = open(dev, O_RDWR | O_NONBLOCK);
  if (fd != -1) {
    return fd;
  }
  throwIOExceptionErrno(env, errno);
  return -1;
}

/*
 * Class:     can4linux_CAN4LinuxAdapter
 * Method:    canClose
 * Signature: (I)I
 */
JNIEXPORT void JNICALL Java_can4linux_CAN4LinuxAdapter_canClose(JNIEnv *env, jclass obj, jint fd) {
  if (close(fd) == -1) {
    throwIOExceptionErrno(env, errno);
  }
}

/*
 * Class:     can4linux_CAN4LinuxAdapter
 * Method:    canSend
 * Signature: (IZIZ[B)I
 */
JNIEXPORT void JNICALL Java_can4linux_CAN4LinuxAdapter_canSend(JNIEnv *env, jclass obj, jint fd, jboolean ext, jint id, jboolean rtr, jbyteArray data) {
  canmsg_t tx;
  int sent=0;

  memset(&tx, 0, sizeof(tx));

  if (rtr) tx.flags |= MSG_RTR;
  if (ext) tx.flags |= MSG_EXT;

  tx.id = id;
    
  const jsize len = env->GetArrayLength(data);
  if (env->ExceptionCheck() == JNI_TRUE) {
    return;
  }
  if(len>CAN_MSG_LENGTH) {
    #define EXCEPTIONMSG "data has more than " STR(CAN_MSG_LENGTH) " bytes"
    throwIllegalArgumentException(env, EXCEPTIONMSG);
    return;
  }
  tx.length = len;
  
  env->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte *>(&tx.data));
  if (env->ExceptionCheck() == JNI_TRUE) {
    return;
  }
  
  sent= write(fd, &tx, 1);
  if (sent != 1) {
    if (sent==-1)
      throwIOExceptionErrno(env, errno);
    else
      throwIOExceptionMsg(env, "number of sent frames not 1");
  }
}

/*
 * Class:     can4linux_CAN4LinuxAdapter
 * Method:    canRead
 * Signature: (II)Lcan4linux/CANMessage;
 */
JNIEXPORT jobject JNICALL Java_can4linux_CAN4LinuxAdapter_canRead(JNIEnv *env, jclass obj, jint fd, jint timeout, jboolean filterSelf) {
  fd_set   rfds;
  struct   timeval tv;
  int      rv;
  canmsg_t rx;

  FD_ZERO(&rfds);
  FD_SET(fd,&rfds);

  /* wait some time  before process terminates, if timeout > 0 */
  tv.tv_sec  = 0;
  tv.tv_usec = timeout;
  
  rv= select(fd+1, &rfds, NULL, NULL, ( timeout >= 0 ? &tv : NULL ));
  if (rv==-1) {
    throwIOExceptionErrno(env, errno);
    return NULL;
  }
  
  if (rv==0 || !FD_ISSET(fd,&rfds))
    return NULL;
  
  rv= read(fd, &rx , 1);
  if (rv!=1) {
    if(rv==-1)
      throwIOExceptionErrno(env, errno);
    else
      throwIOExceptionMsg(env, "invalid length of received frame");
    
    return NULL;
  }

  if(filterSelf && (rx.flags & MSG_SELF))
    return NULL;
  
  const jsize fsize = static_cast<jsize>(rx.length);
  
  const jclass can_frame_clazz = env->FindClass("can4linux/CANMessage");
  if (can_frame_clazz == NULL) {
    throwIOExceptionMsg(env, "could not get class can4linux/CANMessage");
    return NULL;
  }

  const jmethodID can_message_cstr = env->GetMethodID(can_frame_clazz, "<init>", "(IZZZ[BI)V");
  if (can_message_cstr == NULL) {
    throwIOExceptionMsg(env, "could not get constructor for CANMessage");
    return NULL;
  }

  const jbyteArray data = env->NewByteArray(fsize);
  if (data == NULL) {
    if (env->ExceptionCheck() != JNI_TRUE) {
      throwOutOfMemoryError(env, "could not allocate ByteArray");
    }
    return NULL;
  }
  
  env->SetByteArrayRegion(data, 0, fsize, reinterpret_cast<jbyte *>(&rx.data));
  if (env->ExceptionCheck() == JNI_TRUE) {
    return NULL;
  }
  
  const jobject ret = env->NewObject(can_frame_clazz, can_message_cstr, rx.id, (rx.flags & MSG_RTR)!=0, (rx.flags & MSG_EXT)!=0, (rx.flags & MSG_ERR_MASK)!=0, data, rx.flags);

  return ret;
}


#if 0 // from pyCan.c
int can_filter(int fd,char *fstring) {
  char *token;
  int i;
  if(( token = strtok(fstring,",")) != NULL ) {
    if( token[0] == '*' ) {
      can_Config(fd, CONF_FILTER, 0 ); 
printf("\nfilter disabled");
    } else {
      can_Config(fd, CONF_FILTER, 1 );
      can_Config(fd, CONF_FENABLE, strtol(token,NULL,0));
printf("\naccept %d",strtol(token,NULL,0));
    }
    while((token=strtok(NULL,",")) != NULL ) {
      can_Config(fd, CONF_FENABLE, strtol(token,NULL,0));
printf("\naccept %d",strtol(token,NULL,0));
    }
    return 1;
  }
  return -1;
}  
#endif