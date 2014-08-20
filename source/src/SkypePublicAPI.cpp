#ifdef SKYPE_PAPI
#include "SkypePublicAPI.h"

#ifdef WIN32

// ConsoleApplication1.cpp : Defines the entry point for the console application.
//
// Taken from http://nah6.com/~itsme/cvs-xdadevtools/skype/skypeapilog.cpp


#include <windows.h>
#include <iostream>

using std::cerr;
using std::cout;
class BaseError {
protected:
	std::string msg_;
	va_list ap_;
public:
	BaseError(char *fmt, va_list ap)
	{
		int desired_length= _vsnprintf(NULL, 0, fmt, ap);
		msg_.resize(desired_length);
		int printedlength= _vsnprintf(&msg_[0], msg_.size(), fmt, ap);

		va_end(ap);

		if (printedlength!=-1)
			msg_.resize(printedlength);
	}
	friend std::ostream& operator << (std::ostream& os, const BaseError& e)
	{
		return os << e.msg_ << std::endl;
	}
};
class SystemError : public BaseError {
private:
	DWORD dwErrorCode;
	std::string err_;
public:
	SystemError(char *fmt, ...)
		: BaseError(fmt, (va_start(ap_, fmt), ap_))
		, dwErrorCode(GetLastError())
	{
		char* msgbuf;
		int rc= FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dwErrorCode, 0, (char*) &msgbuf, 0, NULL);
		if (rc)
		{
			err_= msgbuf;
			LocalFree(msgbuf);
		}
	}
	friend std::ostream& operator << (std::ostream& os, const SystemError& e)
	{
		os << e.msg_ << ": ";
		if (e.err_.empty())
			os << std::hex << e.dwErrorCode << std::endl;
		else
			os << e.err_;
		return os;
	}
};
class SkypeError : public BaseError {
public:
	SkypeError(char *fmt, ...)
		: BaseError(fmt, (va_start(ap_, fmt), ap_))
	{
	}
};

std::ostream& operator << (std::ostream& os, HWND hWnd)
{
	return os << (UINT)hWnd;
}

HWND g_hWndSkype;       // window handle received in SkypeControlAPIAttach message
HWND g_hWndClient;      // our window handle
int g_bNotAvailable;   // set by not-available msg from skype

// windows messages registered by skype
UINT SkypeControlAPIDiscover;
UINT SkypeControlAPIAttach;

// obtain the skype registered windows messages
void SkypeRegisterMessages()
{
	SkypeControlAPIDiscover =RegisterWindowMessageA("SkypeControlAPIDiscover");
	if (SkypeControlAPIDiscover==0)
		throw SystemError((char*)"RegisterWindowMessage");

	//cout << boost::format("SkypeControlAPIDiscover=%04x\n") % SkypeControlAPIDiscover;

	SkypeControlAPIAttach   =RegisterWindowMessageA("SkypeControlAPIAttach");
	if (SkypeControlAPIAttach==0)
		throw SystemError((char*)"RegisterWindowMessage");

	//cout << boost::format("SkypeControlAPIAttach=%04x\n") % SkypeControlAPIAttach;
}


// handle skype api message, currently outputs the message to stdout.
void HandleSkypeMessage(HWND hWndSkype, COPYDATASTRUCT* cds)
{
	if (hWndSkype!=g_hWndSkype)
		cout << "msg:" << hWndSkype << " global: " <<g_hWndSkype << "\n";

	//cout << ">>" << (char*)cds->lpData << std::endl;
	callback.mcb(&callback,(char*)cds->lpData);
}


// initiate connnection with skype.
void SkypeDiscover(HWND hWnd)
{
	if (g_bNotAvailable)
		return;
	g_hWndSkype= NULL;

	LRESULT res= SendMessage(HWND_BROADCAST, SkypeControlAPIDiscover, (WPARAM)hWnd, 0);

	//cout << boost::format("discover result=%08lx\n") % res;
}

enum {
	SKYPECONTROLAPI_ATTACH_SUCCESS=0,
	SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION,
	SKYPECONTROLAPI_ATTACH_REFUSED,
	SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE,
	SKYPECONTROLAPI_ATTACH_API_AVAILABLE= 0x8001,
};

// process SkypeControlAPIAttach message
void HandleSkypeAttach(LPARAM lParam, WPARAM wParam)
{
	switch(lParam) {
	case SKYPECONTROLAPI_ATTACH_SUCCESS:
		g_hWndSkype= (HWND)wParam;
//printf("g_hWndSkype: %s\n", szBuff);
		printf("%s","authorization success\n");
		callback.mcb(&callback,"authorization success");
		break;
	case SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION:
		printf("%s","pending authorization\n");
		break;
	case SKYPECONTROLAPI_ATTACH_REFUSED:
		printf("%s","attach refused\n");
		g_hWndSkype= NULL;
		break;
	case SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE:
		printf("skype api not available\n");
		g_bNotAvailable= 1;
		break;
	case SKYPECONTROLAPI_ATTACH_API_AVAILABLE:
		printf("skype api available\n");
		g_bNotAvailable= 0;
		SkypeDiscover(g_hWndClient);
		break;
		//default:
		//cout << boost::format("UNKNOWN SKYPEMSG %08lx: %08lx\n") % lParam % wParam;
	}
}

// our windowsproc
LRESULT CALLBACK SkypeWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg==WM_CREATE) {
		// lParam -> CREATESTRUCT -> lpCreateParams
		try {
			SkypeRegisterMessages();
			SkypeDiscover(hWnd);
			return 0;
		}
		catch (BaseError e) {
			cerr << e;
		}
		catch (...) {
			cerr << "unknown exception\n";
		}
		return -1;
	}
	else if (uMsg==WM_DESTROY) {
		return 0;
	}
	else if (uMsg==SkypeControlAPIAttach) {
		try {
			HandleSkypeAttach(lParam, wParam);
		}
		catch (BaseError e) {
			cerr << e;
		}
		catch (...) {
			cerr << "unknown exception in HandleSkypeAttach\n";
		}
		return 0;
	}
	else if (uMsg==SkypeControlAPIDiscover) {
		HWND hWndOther= (HWND)wParam;
		if (hWndOther!=hWnd)
			cout <<"detected other skype api client:" << hWndOther << "\n";
		return 0;
	}
	else if (uMsg==WM_COPYDATA) {
		try {
			HandleSkypeMessage((HWND)wParam, (COPYDATASTRUCT*)lParam);
			return TRUE;
		}
		catch (BaseError e) {
			cerr << e;
		}
		catch (...) {
			cerr << "unknown exception in HandleSkypeMessage\n";
		}
		return FALSE;
	}
	else {
		//cout << boost::format("wnd %08lx msg %08lx %08lx %08lx\n") % hWnd % uMsg % wParam % lParam;
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

// creates our window, needed to communicate with skype.
HWND MakeWindow()
{
	//cout << "MakeWindow\n";

	WNDCLASS wndcls;
	memset(&wndcls, 0, sizeof(WNDCLASS));   // start with NULL
	wndcls.lpfnWndProc = SkypeWindowProc;
	wndcls.lpszClassName = "itsme skype window";
	ATOM a= RegisterClass(&wndcls);
	if (a==0)
		throw SystemError((char*)"RegisterClass");

	//cout << boost::format("windowclass: %04x\n") % a;

	HWND hWnd= CreateWindowEx(0, wndcls.lpszClassName, "itsme skype window", 0, -1, -1, 0, 0, (HWND)NULL, (HMENU)NULL, (HINSTANCE)NULL, NULL);
	if (hWnd==NULL)
		throw SystemError((char*)"CreateWindowEx");

	return hWnd;
}

// destroy our window.
void UnmakeWindow(HWND hWnd)
{
	if (!DestroyWindow(hWnd))
		fprintf(stderr,"unknown exception in DestroyWindow\n" );
	if (!UnregisterClass("itsme skype window", NULL))
		fprintf(stderr,"unknown exception in UnregisterClass\n");
}

// window thread, creates window, and runs messageloop.
DWORD WINAPI SkypeWindowThread(LPVOID lpParameter)
{
	//cout << "SkypeWindowThread\n";

	g_hWndClient= MakeWindow();

//printf("g_hWndClient:%p\n", g_hWndClient);

	//cout << boost::format("starting messageloop clientwnd=%08lx\n") % g_hWndClient;
	MSG msg;
	BOOL bRet;
	while ((bRet= GetMessage(&msg, NULL, 0, 0))!=0)
	{
		if (bRet==-1 || msg.message == WM_QUIT)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//cout << "end thread\n";
	return 0;
}

// creates window thread
void MakeThread()
{
	//cout << "MakeThread\n";
	HANDLE hThread= CreateThread(NULL, 0, SkypeWindowThread, NULL, 0, NULL);
	if (hThread==NULL || hThread==INVALID_HANDLE_VALUE) {
		throw SystemError("CreateThread");
	}
	//cout << boost::format("hThread=%08lx\n") % hThread;
}

// sends skype api message to skype
int sendToSkype( const char *msg )
{
	COPYDATASTRUCT cds;
	cds.dwData= 0;
	cds.lpData= (char*)msg;
	cds.cbData= strlen(msg)+1;
//printf("g_hWndSkype: %p, g_hWndClient: %p, data: %s\n", g_hWndSkype, g_hWndClient, cds.lpData);
	if (!SendMessage(g_hWndSkype, WM_COPYDATA, (WPARAM)g_hWndClient, (LPARAM)&cds)) {
		fprintf(stderr,"skypesendmessage failed\n");
		SkypeDiscover(g_hWndClient);
	}
}

void run_skype_loop()
{
	//cout << "run_skype_loop\n";

	MakeThread();
	printf("ready\n");
}


int initialize(msg_callback_func cb)
{
	callback.mcb = cb;
	///try {
		run_skype_loop();
		///}
		///catch (...) {
		///fprintf(stderr, "unknown exception\n");
		///}
}


void startLoop()
{
}

void endLoop()
{
	UnmakeWindow(g_hWndClient);
	g_hWndClient= NULL;
}


#else

#ifdef __cplusplus
extern "C" {
#endif

// Borrowed from http://konrad.familie-kieling.de/code/skype_dbus_client.c
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib.h>
DBusConnection *connection;
GMainLoop *loop;



/* iterate through DBus messages and output the string components */
static void output_iter( DBusMessageIter *iter )
{
	do
	{
		int type = dbus_message_iter_get_arg_type( iter );
		const char *str;

		if ( type == DBUS_TYPE_INVALID ) break;

		switch ( type )
		{
		case DBUS_TYPE_STRING:					// it is a string message
			dbus_message_iter_get_basic( iter, &str );
			//g_print( "%s\n", str );
			callback.mcb(&callback,str);
			break;

		case DBUS_TYPE_VARIANT:					// another set of items
		{
			DBusMessageIter subiter;

			dbus_message_iter_recurse( iter, &subiter );
			output_iter( &subiter );
			break;
		}

		default:
			break;
		}
	}
	while ( dbus_message_iter_next( iter ) );
}


/* the handler gets called if the DBus connection receives any data */
static DBusHandlerResult notify_handler( DBusConnection *bus, DBusMessage *msg, void *user_data )
{
	DBusMessageIter iter;
	dbus_message_iter_init( msg, &iter );
	output_iter( &iter );

	return DBUS_HANDLER_RESULT_HANDLED;
}

/* send a string to skype */
int sendToSkype(const char *msg )
{
	DBusMessageIter iter;
	const int reply_timeout = -1;					// do not timeout -- block
	DBusMessage *reply;
	DBusMessage *message;
	DBusError error;

	dbus_error_init( &error );
	message = dbus_message_new_method_call(
			"com.Skype.API",
			"/com/Skype",
			"com.Skype.API",
			"Invoke"
	);

	if( !dbus_message_append_args( message, DBUS_TYPE_STRING, &msg, DBUS_TYPE_INVALID ) )
	{
		fprintf( stderr, "Error: reply is not except format\n" );
		exit( 1 );
	}

	reply = dbus_connection_send_with_reply_and_block( connection, message, reply_timeout, &error );

	if ( dbus_error_is_set( &error ) )
	{
		fprintf ( stderr, "Error: %s\n", error.message );
		dbus_error_free( &error );
		return -1;
	}

	dbus_message_iter_init( reply, &iter );
	output_iter( &iter );

	if( dbus_error_is_set( &error ) )
	{
		fprintf( stderr, "Error: %s\n", error.message );
		dbus_error_free( &error );
		return -1;
	}
	return 0;
}


int initialize(msg_callback_func cb)
{

	callback.mcb = cb;
	//callback.str = msg;

	DBusObjectPathVTable vtable;

	GIOChannel *in;
	DBusError error;

	dbus_error_init( &error );
	connection = dbus_bus_get( DBUS_BUS_SESSION, &error );
	if ( connection == NULL )
	{
		fprintf( stderr, "Failed to open connection to bus: %s\n", error.message );
		dbus_error_free( &error );
		exit( 1 );
	}


	dbus_connection_setup_with_g_main( connection, NULL );	// set up te DBus connection

	vtable.message_function = notify_handler;			// register handler for incoming data on DBus
	dbus_connection_register_object_path( connection, "/com/Skype/Client", &vtable, 0 );

	return 0;
}

void startLoop()
{
	loop = g_main_loop_new( NULL, FALSE );
	g_main_loop_run( loop );
}
void endLoop()
{
	g_main_loop_quit(loop);
}

#ifdef __cplusplus
}
#endif

#endif

#endif
