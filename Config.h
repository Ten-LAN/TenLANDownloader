#ifndef CLASSDEF_CONFIG
#define CLASSDEF_CONFIG

#include "Common.h"
#define SUBURLSIZE 1024
class Config
{
private:
	char *baseurl;
	char *gamedir;
	char subaddr[ SUBURLSIZE ];
	int categorymax;
	int sound;
	int pad[ 7 ];
	// 表示。
	int newgame, category, dvd;
	int hidetaskbar, fullscreen;
	int esccannon[ 6 ];
	// URLカーソル。
	int ucur;
	// パッドキーコンフィグ。
	int kcesccannon;
public:
	Config( void );
	virtual ~Config( void );

	static char * CopyString( const char *str );
	static char * CopyString( const char *str, unsigned int size );
	static int AtoI( const char *str );
	static int AtoI( const char *str, int figure );

	virtual int Load( const char *file );

	virtual const char * GetGameDirectory( void );
	virtual int GetCategoryMax( void );
	virtual const char * GetBaseURL( void );

	virtual int Draw( void );
};

#endif
