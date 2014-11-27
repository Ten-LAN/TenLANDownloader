#ifndef CLASSDEF_MESSAGE
#define CLASSDEF_MESSAGE

#include "Common.h"

struct Msg
{
	char *line;
	struct Msg *next, *prev;
};

class Message
{
private:
	int fontsize;
	int x, y, w, h;
	int maxmessage, nowmessage, beginmessage;
	struct Msg *msg, *read, *last;
	// ���ɓ��镶�����A�c�ɓ��镶�����B
	int mw, mh;

	virtual struct Msg * AddMessage( char *message );

	int viewtime, viewtimer;

	int filenum;

	int autoread;
public:
	Message( void );
	virtual ~Message( void );
	virtual int Init( void );
	virtual int Term( void );

	virtual int Release( void );

	virtual int SetMessageFormat( char *format, ... );
	virtual int SetMessage( char *message ); // ���b�N���K�v�B

	virtual int SetArea( int dx, int dy, int width, int height );
	virtual int Draw( void ); // ���b�N���K�v�B
};

#endif
