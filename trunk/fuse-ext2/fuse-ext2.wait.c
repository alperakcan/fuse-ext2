/*-
 * fuse_wait - Light wrapper around a FUSE mount program that waits
 *             for the "mounted" notification before exiting.
 *
 * Copyright (C) 2007 Erik Larsson
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
 * 02110-1301, USA
 */

/*
 * fuse_wait
 *   Written as a replacement to shadowofged's little utility.
 *   This tool executes a specified mount program with all of its
 *   arguments. The path to the mount program and all its arguments
 *   start at the third argument. The first argument is the mount
 *   point to check for mount notification, and the second argument
 *   is the amount of seconds to wait until timeout.
 *   Primarily used for mounting ntfs-3g filesystems on Mac OS X.
 *
 * Updated 2007-10-07:
 *   I didn't read the man page for waitpid correctly. fuse_wait
 *   thus returned incorrect exit values, but it's fixed now.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for realpath
#include <CoreFoundation/CoreFoundation.h>

#define FUSE_LISTEN_OBJECT "com.google.filesystems.fusefs.unotifications"
#define FUSE_MOUNT_NOTIFICATION_NAME "com.google.filesystems.fusefs.unotifications.mounted"
#define FUSE_MOUNT_PATH_KEY "kFUSEMountPath"
#define DEBUGMODE FALSE
#if DEBUGMODE
#define DEBUG(...) do { fprintf(debugFile, __VA_ARGS__); fflush(debugFile); } while(0)
static FILE *debugFile = stderr;
#else
#define DEBUG(...)
#endif

#define MIN(X, Y) X < Y ? X : Y

static CFStringRef mountPath = NULL;

/* There seems to be a bug with MacFUSE such that the CFString that
 * it returns for kFUSEMountPath in the dictionary accompanying the
 * com.google.filesystems.fusefs.unotifications.mounted notification
 * is represented as UTF-8 within the internal representation.
 * Probably UTF-8 encoded text represented as UTF-16 values. This
 * function transforms it to a true null terminated UTF-8 string.
 * This function assumes that the target buffer out is at least
 * CFStringGetLength(in) bytes long. */
static void GetCorruptedMacFUSEStringAsUTF8(CFStringRef in, char* out, int outsize) {
  int inLength = CFStringGetLength(in);
  int bytesToWrite = MIN(inLength, outsize-1);
  int i;
  for(i = 0; i < bytesToWrite; ++i)
    out[i] = (char)CFStringGetCharacterAtIndex(in, i);
  out[i] = '\0';
}

/* Debug code for printing entries in the CFDictionary 'userInfo'. */
static void PrintDictEntry(const void *key, const void *value, void *context) {
  char buffer[512];
  CFStringGetCString(key, buffer, 512, kCFStringEncodingUTF8);
  DEBUG("    Key: \"%s\"", buffer);
  GetCorruptedMacFUSEStringAsUTF8(value, buffer, 512);
  DEBUG(", Value: \"%s\"\n", buffer);
/*   FILE *dbgoutput = fopen("valuedump-utf32be.txt", "w"); */
/*   memset(buffer, 0, 512); */
/*   CFStringGetCString(value, buffer, 512, kCFStringEncodingUTF32BE); */
/*   fwrite(buffer, 512, 1, dbgoutput); */
/*   fclose(dbgoutput); */
}

/* Callback function which will recieve the 'mounted' notification
 * from FUSE. It is full with debug code... but I'll let it stay
 * that way. */
static void NotificationCallback(CFNotificationCenterRef center,
				 void *observer,
				 CFStringRef name,
				 const void *object,
				 CFDictionaryRef userInfo) {
  if(DEBUGMODE) {
    char buffer[512];
    DEBUG("Received notification:\n");
    if(CFStringGetCString(name, buffer, 512, kCFStringEncodingUTF8) == true)
      DEBUG("  Name: %s\n", buffer);
    else
      DEBUG("  <Cound not get name>\n");
  }
  if(userInfo != NULL) { // It's only null when testing
    DEBUG("  userInfo:\n");
    if(DEBUGMODE) CFDictionaryApplyFunction(userInfo, PrintDictEntry, NULL);
    
    const void *value = NULL;
    DEBUG("CFDictionaryGetValueIfPresent(%X, \"%s\", %X\n",
	    (int)userInfo, FUSE_MOUNT_PATH_KEY, (int)&value);

    if(CFDictionaryGetValueIfPresent(userInfo, CFSTR(FUSE_MOUNT_PATH_KEY), &value) == true) {
      DEBUG("CFGetTypeID(%X) == %X ?\n", (int)value, (int)CFStringGetTypeID());
      if(CFGetTypeID((CFStringRef)value) == CFStringGetTypeID()) {
	DEBUG("mountPath=%X\n", (int)mountPath);
	if(mountPath != NULL)
	  CFRelease(mountPath); // No memory leaks please.
	DEBUG("assigning mountpath the value %X\n", (int)value);

	mountPath = (CFStringRef)value;
	CFRetain(mountPath);

	DEBUG("done with assigning.\n");
      }
    }
  }
}

/* Prints a help text to a stream. */
static void PrintUsage(FILE *stream) {
  /* 80 chars    <-------------------------------------------------------------------------------->*/
  fprintf(stream, "usage: fuse_wait <mountpoint> <timeout> <mount_command> [<args...>]\n");  
  fprintf(stream, "  mountpoint    - where you wish to mount the MacFUSE file system\n");
  fprintf(stream, "  timeout       - time (in seconds) to wait for the mount operation to\n");
  fprintf(stream, "                  complete (may be a floating point value)\n");
  fprintf(stream, "  mount_command - a path to the executable file containing the fuse program\n");
  fprintf(stream, "                  used to mount the file system\n");
  fprintf(stream, "  args          - the arguments that you would normally pass to <mount_command>\n");
  fprintf(stream, "                  (note that this includes the mount point)\n");
}

int main(int argc, char** argv) {
  if(argc < 4) {
    PrintUsage(stdout);
    return 0;
  }
  
  /* <argv parsing> */
  
  /* Parse argument: mountpoint (CFString) */
  /*CFStringRef mountpoint = CFStringCreateWithCString(kCFAllocatorDefault, 
    argv[1], kCFStringEncodingUTF8); */
  /* Parse argument: mountpointRaw (char*) */
  char *mountpointRaw = argv[1];
  
  /* Parse argument: timeout (double) */
  double timeout;
  {
    CFStringRef timeoutString = CFStringCreateWithCString(kCFAllocatorDefault, 
							 argv[2], kCFStringEncodingUTF8);
    timeout = CFStringGetDoubleValue(timeoutString);
    CFRelease(timeoutString);
    if(timeout == 0.0) {
      fprintf(stdout, "Invalid argument: timeout (\"%s\"", argv[2]);
      return -1;
    }
  }
  
  /* Parse argument: mount_command */
  const char *mount_command = argv[3];
  
  /* </argv parsing> */
 
  CFNotificationCenterRef centerRef;
  CFStringRef notificationObjectName = CFSTR(FUSE_LISTEN_OBJECT);
  CFStringRef notificationName = CFSTR(FUSE_MOUNT_NOTIFICATION_NAME);
  CFRunLoopRef crl;
  
  if(DEBUGMODE) {
    DEBUG("Testing NotificationCallback...\n");
    NotificationCallback(NULL, NULL, notificationObjectName, NULL, NULL);
    DEBUG("Test completed. Adding observer...\n");
  }
  
  centerRef = CFNotificationCenterGetDistributedCenter();
  crl = CFRunLoopGetCurrent();
  
  /* Think. Will the child process also be an observer? I don't think so... */
  CFNotificationCenterAddObserver(centerRef, NULL, NotificationCallback, notificationName,
				  notificationObjectName, CFNotificationSuspensionBehaviorDrop);
    

  int forkRetval = fork();
  if(forkRetval == -1) {
    fprintf(stderr, "Could not fork!\n");
    return -1;
  }
  else if(forkRetval != 0) {
    // Parent process
    int childProcessPID = forkRetval;

    int waitpid_status = 0;
    DEBUG("Waiting for PID %d...\n", childProcessPID);
    int waitpidres = waitpid(childProcessPID, &waitpid_status, 0);
    if(waitpidres == childProcessPID) {
      if(!WIFEXITED(waitpid_status)) {
	DEBUG("Child process did not exit cleanly! Returning -1.");
	return -1;
      }
      
      int retval = WEXITSTATUS(waitpid_status);
      DEBUG("PID %d returned with exit code: %d Exiting fuse_wait with this exit code...\n", childProcessPID, retval);
      if(retval != 0) {
	DEBUG("Exit value indicates an error while executing mount command. Returning without\n");
	DEBUG("waiting for notification.\n");
	DEBUG("Returning retval: %d\n", retval);
	return retval;
      }
    }
    else {
      DEBUG("Abnormal termination of process %d. :( waitpid returned: %d\n", childProcessPID, waitpidres);
      return -1;
    }
    
    DEBUG("Running run loop a long time...\n");
    CFStringRef mountPathSnapshot = NULL;
    while(mountPathSnapshot == NULL) {
      int crlrimRetval = CFRunLoopRunInMode(kCFRunLoopDefaultMode, timeout, true);
      DEBUG("Exited from run loop. Let's find out why... crlrimRetval: %d (handled: %d)\n", crlrimRetval, kCFRunLoopRunHandledSource);
      mountPathSnapshot = mountPath; // Might have been modified during run loop.
      if(crlrimRetval != kCFRunLoopRunHandledSource) {
	fprintf(stderr, "Did not receive a signal within %f seconds. Exiting...\n", timeout);
	break;
      }
      else if(mountPathSnapshot != NULL) {
	DEBUG("mountPathSnapshot: %X\n", (int)mountPathSnapshot);
	
	int mountPathUTF8Length = CFStringGetLength(mountPath) + 1;  // null terminator
	char *mountPathUTF8 = malloc(mountPathUTF8Length);
	memset(mountPathUTF8, 0, mountPathUTF8Length);
	GetCorruptedMacFUSEStringAsUTF8(mountPath, mountPathUTF8, mountPathUTF8Length);
	
	char *canonicalMountPath = malloc(PATH_MAX);
	char *canonicalMountpoint = malloc(PATH_MAX);
	memset(canonicalMountPath, 0, PATH_MAX);
	memset(canonicalMountpoint, 0, PATH_MAX);
	
	realpath(mountPathUTF8, canonicalMountPath);
	realpath(mountpointRaw, canonicalMountpoint);
	
	int cmpres = strncmp(canonicalMountPath, canonicalMountpoint, PATH_MAX);
	
	if(cmpres != 0) {
	  if(DEBUGMODE) {
	    DEBUG("Strings NOT equal. cmpres=%d\n", cmpres);
	    DEBUG("mountPath (UTF-8): \"%s\"\n", canonicalMountPath);
	    DEBUG("mountpoint (UTF-8): \"%s\"\n", canonicalMountpoint);
	  }
	  mountPathSnapshot = NULL;
	}
	else
	  DEBUG("Mounter has signaled! Great success!\n");
	
	free(mountPathUTF8);
	free(canonicalMountPath);
	free(canonicalMountpoint);
      }
      //CFRunLoopRun();
    }
    DEBUG("Run loop done.\n");
    if(mountPath != NULL)
      CFRelease(mountPath);
    
    return 0; // We have previously checked that the return value from the child process is 0. We can't get here if it isn't.
  }
  else { // forkRetval == 0
    // Child process
    const int childargc = argc-3;
    char *childargv[childargc+1];
    childargv[childargc] = NULL; // Null terminated
    int i;
    for(i = 0; i < childargc; ++i)
      childargv[i] = argv[i+(argc-childargc)];
    
    if(DEBUGMODE) {
      DEBUG("Contents of argv:\n");
      for(i = 0; i < argc; ++i)
	DEBUG("  argv[%i]: \"%s\"\n", i, argv[i]);
      
      DEBUG("Contents of childargv:\n");
      for(i = 0; i < childargc; ++i)
	DEBUG("  childargv[%i]: \"%s\"\n", i, childargv[i]);
    }
    
    execvp(mount_command, childargv);
    fprintf(stderr, "Could not execute %s!\n", argv[3]);
    return -1;
  }
}
