/*
    Platypus - create MacOS X application bundles that execute scripts
        This is the executable that goes into Platypus apps
    Copyright (C) 2003 Sveinbjorn Thordarson <sveinbt@hi.is>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    main.c - main program file

	***
	This has been slightly modified to work for Gamera

*/

///////////////////////////////////////
// Includes
///////////////////////////////////////    
#pragma mark Includes

// Apple stuff
#include <Carbon/Carbon.h>
#include <CoreFoundation/CoreFoundation.h>

// Unix stuff
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

///////////////////////////////////////
// Definitions
///////////////////////////////////////    
#pragma mark Definitions

// name length limits
#define	kMaxPathLength 1024

// names of files bundled with app
#define	kScriptFileName "script"
#define kOpenDocFileName "openDoc"

// custom carbon events
#define kEventClassRedFatalAlert 911
#define kEventKindX11Failed 911

//maximum arguments the script accepts 
#define	kMaxArgumentsToScript 252

///////////////////////////////////////
// Prototypes
///////////////////////////////////////    
#pragma mark Prototypes

static void *Execute(void *arg);
static void *OpenDoc(void *arg);
static OSErr ExecuteScript(char *script, pid_t *pid);

static void  GetParameters(void);
static char* GetScript(void);
static char* GetOpenDoc(void);

OSErr LoadMenuBar(char *appName);

static OSStatus FSMakePath(FSSpec file, char *path, long maxPathSize);
static void RedFatalAlert(Str255 errorString, Str255 expStr);
static short DoesFileExist(char *path);

static OSErr AppQuitAEHandler(const AppleEvent *theAppleEvent,
                              AppleEvent *reply, long refCon);
static OSErr AppOpenDocAEHandler(const AppleEvent *theAppleEvent,
                                 AppleEvent *reply, long refCon);
static OSErr AppOpenAppAEHandler(const AppleEvent *theAppleEvent,
                                 AppleEvent *reply, long refCon);
static OSStatus X11FailedHandler(EventHandlerCallRef theHandlerCall,
                                 EventRef theEvent, void *userData);

///////////////////////////////////////
// Globals
///////////////////////////////////////    
#pragma mark Globals

// process id of forked process
pid_t pid = 0;

// thread id of threads that start scripts
pthread_t odtid = 0, tid = 0;

// indicator of whether the script has completed executing
short taskDone = true;

// execution parameters
char scriptPath[kMaxPathLength];
char openDocPath[kMaxPathLength];

//arguments to the script
char *arguments[kMaxArgumentsToScript+3];
char *fileArgs[kMaxArgumentsToScript];
short numArgs = 0;

extern char **environ;

#pragma mark -

///////////////////////////////////////
// Program entrance point
///////////////////////////////////////
int main(int argc, char* argv[])
{
    OSErr err = noErr;
    EventTypeSpec events = { kEventClassRedFatalAlert, kEventKindX11Failed };

    InitCursor();

    //install Apple Event handlers
    err += AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
                                 NewAEEventHandlerUPP(AppQuitAEHandler),
                                 0, false);
    err += AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
                                 NewAEEventHandlerUPP(AppOpenDocAEHandler),
                                 0, false);
    err += AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
                                 NewAEEventHandlerUPP(AppOpenAppAEHandler),
                                 0, false);
    err += InstallEventHandler(GetApplicationEventTarget(),
                               NewEventHandlerUPP(X11FailedHandler), 1,
                               &events, NULL, NULL);

    if (err) RedFatalAlert("\pInitialization Error",
                           "\pError initing Apple Event handlers.");

    //create the menu bar
    if (err = LoadMenuBar(NULL)) RedFatalAlert("\pInitialization Error",
                                               "\pError loading MenuBar.nib.");
    
    GetParameters(); //load data from files containing exec settings

    RunApplicationEventLoop(); //Run the event loop
    return 0;
}
                                 
#pragma mark -

///////////////////////////////////
// Execution thread starts here
///////////////////////////////////
static void *Execute (void *arg)
{
    EventRef event;
    
    taskDone = false;
    if (ExecuteScript(scriptPath, &pid) == (OSErr)11) {
        CreateEvent(NULL, kEventClassRedFatalAlert, kEventKindX11Failed, 0,
                    kEventAttributeNone, &event);
        PostEventToQueue(GetMainEventQueue(), event, kEventPriorityStandard);
    }
    else ExitToShell();
    return 0;
}

///////////////////////////////////
// Open additional documents thread starts here
///////////////////////////////////
static void *OpenDoc (void *arg)
{
    ExecuteScript(openDocPath, NULL);
    return 0;
}

///////////////////////////////////////
// Run a script via the system command
///////////////////////////////////////
static OSErr ExecuteScript (char *script, pid_t *pid)
{
    pid_t wpid = 0, p = 0;
    int status, i;
    
    if (! pid) pid = &p;
    
    // Generate the array of argument strings before we do any executing
    arguments[0] = script;
    for (i = 0; i < numArgs; i++) arguments[i + 1] = fileArgs[i];
    arguments[i + 1] = NULL;

    *pid = fork(); //open fork
    
    if (*pid == (pid_t)-1) exit(13); //error
    else if (*pid == 0) { //child process started
        execve(arguments[0], arguments, environ);
        exit(13); //if we reach this point, there's an error
    }

    wpid = waitpid(*pid, &status, 0); //wait while child process finishes
    
    if (wpid == (pid_t)-1) return wpid;
    return (OSErr)WEXITSTATUS(status);
}

#pragma mark -

///////////////////////////////////////
// This function loads all the neccesary settings
// from config files in the Resources folder
///////////////////////////////////////
static void GetParameters (void)
{
    char *str;
    if (! (str = (char *)GetScript())) //get path to script to be executed
        RedFatalAlert("\pInitialization Error",
                      "\pError getting script from application bundle.");
    strcpy((char *)&scriptPath, str);
    
    if (! (str = (char *)GetOpenDoc())) //get path to openDoc
        RedFatalAlert("\pInitialization Error",
                      "\pError getting openDoc from application bundle.");
    strcpy((char *)&openDocPath, str);
}

///////////////////////////////////////
// Get path to the script in Resources folder
///////////////////////////////////////
static char* GetScript (void)
{
    CFStringRef fileName;
    CFBundleRef appBundle;
    CFURLRef scriptFileURL;
    FSRef fileRef;
    FSSpec fileSpec;
    char *path;

    //get CF URL for script
    if (! (appBundle = CFBundleGetMainBundle())) return NULL;
    if (! (fileName = CFStringCreateWithCString(NULL, kScriptFileName,
                                                kCFStringEncodingASCII)))
        return NULL;
    if (! (scriptFileURL = CFBundleCopyResourceURL(appBundle, fileName, NULL,
                                                   NULL))) return NULL;
    
    //Get file reference from Core Foundation URL
    if (! CFURLGetFSRef(scriptFileURL, &fileRef)) return NULL;
    
    //dispose of the CF variables
    CFRelease(scriptFileURL);
    CFRelease(fileName);
    
    //convert FSRef to FSSpec
    if (FSGetCatalogInfo(&fileRef, kFSCatInfoNone, NULL, NULL, &fileSpec,
                         NULL)) return NULL;
        
    //create path string
    if (! (path = malloc(kMaxPathLength))) return NULL;
    if (FSMakePath(fileSpec, path, kMaxPathLength)) return NULL;
    if (! DoesFileExist(path)) return NULL;
    
    return path;
}

///////////////////////////////////////
// Gets the path to openDoc in Resources folder
///////////////////////////////////////
static char* GetOpenDoc (void)
{
    CFStringRef fileName;
    CFBundleRef appBundle;
    CFURLRef openDocFileURL;
    FSRef fileRef;
    FSSpec fileSpec;
    char *path;
    
    //get CF URL for openDoc
    if (! (appBundle = CFBundleGetMainBundle())) return NULL;
    if (! (fileName = CFStringCreateWithCString(NULL, kOpenDocFileName,
                                                kCFStringEncodingASCII)))
        return NULL;
    if (! (openDocFileURL = CFBundleCopyResourceURL(appBundle, fileName, NULL,
                                                    NULL))) return NULL;
    
    //Get file reference from Core Foundation URL
    if (! CFURLGetFSRef( openDocFileURL, &fileRef )) return NULL;
    
    //dispose of the CF variables
    CFRelease(openDocFileURL);
    CFRelease(fileName);
        
    //convert FSRef to FSSpec
    if (FSGetCatalogInfo(&fileRef, kFSCatInfoNone, NULL, NULL, &fileSpec,
                         NULL)) return NULL;

    //create path string
    if (! (path = malloc(kMaxPathLength))) return NULL;
    if (FSMakePath(fileSpec, path, kMaxPathLength)) return NULL;
    if (! DoesFileExist(path)) return NULL;
    
    return path;
}

#pragma mark -

/////////////////////////////////////
// Load menu bar from nib
/////////////////////////////////////
OSErr LoadMenuBar (char *appName)
{
    OSErr err;
    IBNibRef nibRef;
    
    if (err = CreateNibReference(CFSTR("MenuBar"), &nibRef)) return err;
    if (err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"))) return err;
    DisposeNibReference(nibRef);

    return noErr;
}

#pragma mark -

///////////////////////////////////////
// Generate path string from FSSpec record
///////////////////////////////////////
static OSStatus FSMakePath(FSSpec file, char *path, long maxPathSize)
{
    OSErr err = noErr;
    FSRef fileRef;

    //create file reference from file spec
    if (err = FSpMakeFSRef(&file, &fileRef)) return err;

    // and then convert the FSRef to a path
    return FSRefMakePath(&fileRef, path, maxPathSize);
}

////////////////////////////////////////
// Standard red error alert, then exit application
////////////////////////////////////////
static void RedFatalAlert (Str255 errorString, Str255 expStr)
{
    StandardAlert(kAlertStopAlert, errorString,  expStr, NULL, NULL);
    ExitToShell();
}

///////////////////////////////////////
// Determines whether file exists at path or not
///////////////////////////////////////
static short DoesFileExist (char *path)
{
    if (access(path, F_OK) == -1) return false;
    return true;	
}

#pragma mark -

///////////////////////////////////////
// Apple Event handler for Quit i.e. from
// the dock or Application menu item
///////////////////////////////////////
static OSErr AppQuitAEHandler(const AppleEvent *theAppleEvent,
                              AppleEvent *reply, long refCon)
{
    #pragma unused (reply, refCon, theAppleEvent)

    while (numArgs > 0) free(fileArgs[numArgs--]);
    
    if (! taskDone && pid) { //kill the script process brutally
        kill(pid, 9);
        printf("Platypus App: PID %d killed brutally\n", pid);
    }
    
    pthread_cancel(tid);
    if (odtid) pthread_cancel(odtid);
    
    ExitToShell();
    
    return noErr;
}

/////////////////////////////////////
// Handler for docs dragged on app icon
/////////////////////////////////////
static OSErr AppOpenDocAEHandler(const AppleEvent *theAppleEvent,
                                 AppleEvent *reply, long refCon)
{
    #pragma unused (reply, refCon)
	
    OSErr err = noErr;
    AEDescList fileSpecList;
    AEKeyword keyword;
    DescType type;
        
    short i;
    long count, actualSize;
        
    FSSpec fileSpec;
    char path[kMaxPathLength];
    
    while (numArgs > 0) free(fileArgs[numArgs--]);
        
    //Read the AppleEvent
    err = AEGetParamDesc(theAppleEvent, keyDirectObject, typeAEList,
                         &fileSpecList);
		
    err = AECountItems(&fileSpecList, &count); //Count number of files
                
    for (i = 1; i <= count; i++) { //iteratively process each file
        //get fsspec from apple event
        if (! (err = AEGetNthPtr(&fileSpecList, i, typeFSS, &keyword, &type,
                                 (Ptr)&fileSpec, sizeof(FSSpec), &actualSize)))
        {
            //get path from file spec
            if ((err = FSMakePath(fileSpec, (unsigned char *)&path,
                                  kMaxPathLength))) return err;
                            
            if (numArgs == kMaxArgumentsToScript) break;

            if (! (fileArgs[numArgs] = malloc(kMaxPathLength))) return true;

            strcpy(fileArgs[numArgs++], (char *)&path);
        }
        else return err;
    }
        
    if (! taskDone) pthread_create(&odtid, NULL, OpenDoc, NULL);
    else pthread_create(&tid, NULL, Execute, NULL);
        
    return err;
}

///////////////////////////////
// Handler for clicking on app icon
///////////////////////////////
static OSErr AppOpenAppAEHandler(const AppleEvent *theAppleEvent,
                                 AppleEvent *reply, long refCon)
{
    #pragma unused (reply, refCon, theAppleEvent)
	
    // the app has been opened without any items dragged on to it
    pthread_create(&tid, NULL, Execute, NULL);
	return noErr;
}

//////////////////////////////////
// Handler for when X11 fails to start
//////////////////////////////////
static OSStatus X11FailedHandler(EventHandlerCallRef theHandlerCall, 
                                 EventRef theEvent, void *userData)
{
    #pragma unused(theHanderCall, theEvent, userData)

    pthread_join(tid, NULL);
    if (odtid) pthread_join(odtid, NULL);

    RedFatalAlert("\pFailed to start X11",
                  "\pGimp.app requires Apple's X11.");

    return noErr;
}
