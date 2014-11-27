#include "Message.h"

Message::Message( void )
{
	msg = read = last = NULL;
	nowmessage = beginmessage = 0;
	x = y = w = h = 0;

	maxmessage = 100;

	fontsize = 0;

	viewtime = 1;
	viewtimer = 0;

	autoread = 1;
}

Message::~Message( void )
{
	Release();
}

int Message::Init( void )
{
	filenum = 5;
	MikanFile->Open( filenum, "log.txt", "w" );


	MikanSystem->CreateLock( 5 );
	fontsize = 25;
	MikanDraw->LoadFontFile( NULL, "SYSFONT" );
	MikanDraw->CreateFont( 10, "MigMix 1M", fontsize, 0xFFFFFFFF );

	autoread = 1;
	return 0;
}

int Message::Term( void )
{
	MikanFile->Close( filenum );
	return 0;
}

int Message::Release( void )
{
	struct Msg *del;
	while(msg)
	{
		del = msg;
		msg = msg->next;
		free( del->line );
		free( del );
	}
	msg = read = last = NULL;
	nowmessage = 0;
	return 0;
}

int Message::SetMessageFormat( char *format, ... )
{
	char str[ MAX_BUFFER_SIZE ];
	//va_listを使う方法もあるけど、別にこれで問題ないよね。
	vsnprintf_s( str, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE, format, (char*)( &format + 1 ) );
	return SetMessage( str );
}

int Message::SetMessage( char *message )
{
	struct Msg *del;
	MikanSystem->Lock( 5 );

	// ログファイルへ出力。
	MikanFile->Write( filenum, message );
	MikanFile->Write( filenum, "\n" );

	if(msg == NULL)
	{
		msg = read = last = AddMessage( message );
		beginmessage = 0;
	} else
	{
		last->next = AddMessage( message );
		last->next->prev = last;
		while(last->next)
		{
			last = last->next;
		}
	}

	while(maxmessage < nowmessage + mh)
	{
		del = msg;
		msg->next->prev = NULL;
		msg = msg->next;
		if(read == del)
		{
			read = msg;
		}
		free( del->line );
		free( del );
		--nowmessage;
		if(0 < beginmessage)
		{
			--beginmessage;
		}
	}

	MikanSystem->Unlock( 5 );
	return 0;
}

struct Msg * Message::AddMessage( char *message )
{
	int len;
	struct Msg *add;
	unsigned char c0, c1;

	++nowmessage;
	add = ( struct Msg * )calloc( 1, sizeof( struct Msg ) );
	if(mw <= (int)strlen( message ))
	{
		add->line = (char *)calloc( mw + 1, sizeof( char ) );
		add->next = add; // NULLじゃない何かを入れて次があることを示す。
	} else
	{
		add->line = (char *)calloc( strlen( message ) + 1, sizeof( char ) );
	}

	// 文字列のコピー。ただしShiftJISを考慮したもの。
	len = 0;
	while(len < mw)
	{
		c0 = message[ len ];
		c1 = message[ len + 1 ];
		if(( 0x81 <= c0 && c0 <= 0x9f ) || ( 0xe0 <= c0 && c0 <= 0xfc ) && ( 0x40 <= c1 && c1 <= 0xfc ))
		{
			// 2バイト文字がはみ出るのでやめる。
			if(mw <= len + 1)
			{
				break;
			}
			add->line[ len ] = message[ len ];
			++len;
		}
		add->line[ len ] = message[ len ];
		++len;
		if(message[ len ] == '\0')
		{
			break;
		}
	}

	if(add->next)
	{
		// NULLじゃないので次の表示が必要。
		add->next = AddMessage( message + len );//NULLじゃなくて再帰。
		add->next->prev = add;
	}

	return add;
}


int Message::SetArea( int dx, int dy, int width, int height )
{
	x = dx;
	y = dy;
	w = width;
	h = height;

	mw = w / ( fontsize * 5 / 13 );// 4 / 11
	mh = h / ( fontsize );

	SetMessage( "[ OK ] System boot" );
	SetMessage( "Welcome to Ten-LAN Game Loader" );

	return 0;
}

int Message::Draw( void )
{
	int i;
	struct Msg *write;

	write = read;
	MikanSystem->Lock( 5 );

	for(i = 0 ; i < mh && write ; ++i)
	{
		MikanDraw->Printf( 10, x, y + fontsize * i, "%s", write->line );
		if(strncmp( write->line, "[FAIL]", 6 ) == 0)
		{
			MikanDraw->Printf( 10, x, y + fontsize * i, 0xFFFF0000, " %.4s", write->line + 1 );
		} else if(strncmp( write->line, "[ OK ]", 6 ) == 0)
		{
			MikanDraw->Printf( 10, x, y + fontsize * i, 0xFF00FF00, " %.4s", write->line + 1 );
		}
		write = write->next;
	}

	if(autoread)
	{
		if(write && viewtime < viewtimer)
		{
			read = read->next;
			++beginmessage;
			viewtimer = 0;
		} else
		{
			++viewtimer;
		}
	}
	if(y < MikanInput->GetMousePosY())
	{
		if(MikanInput->GetMouseWheel() > 0)
		{
			if(read->prev && read->prev->prev)
			{
				read = read->prev;
				--beginmessage;
				//--nowmessage;
			}
			autoread = 0;
		} else if(MikanInput->GetMouseWheel() < 0)
		{
			if(write)
			{
				read = read->next;
				++beginmessage;
				//++nowmessage;
			}
			if(write == NULL || write->next == NULL)
			{
				autoread = 1;
			}
		}
	}
	//if ( nowmessage < beginmessage + mh ){ beginmessage = nowmessage - mh; }
	DrawScroll( x + w - 10, y, h, nowmessage, mh, beginmessage );
	MikanSystem->Unlock( 5 );

	return 0;
}
