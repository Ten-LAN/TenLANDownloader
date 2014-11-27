#include "Common.h"
#include "Config.h"
#include "GameData.h"

class Config conf;
class Message msg;
class GameData game;

extern "C" __declspec( dllexport ) int ZipNums( const char *ZipFilename );
extern "C" __declspec( dllexport ) int Unzip( const char *ZipFilename, const char *TargetPath, int *count );

int( *FileNumsInZip )( const char * );
int( *UnzipFile )( const char *, const char *, int * );

const char ZIPFILE[] = "tmp.zip";

HINSTANCE hdll;

#define PATHSIZE 260

enum
{
	MODE_DEFAULT,
	MODE_CONFIG,
};

struct WINDOWHEADER
{
	int timer;
	int wx, wy, drg;

	// アイコンの表示の大きさ保持のための値。
	int close;
	int config;
	int game;
	int jsondownload;
	int download;

	// プログレスバーの制御。
	int progress, progressmax;
	// ゲーム用のプログレスバーの制御。
	struct
	{
		unsigned int gamenum;
		int now, max;
		char *basead;
	} gameprogress;

	// 画面サイズ。
	int width, height;

	// 現在何かの処理中かどうか。
	int working;

	// ウィンドウモード。
	int mode;

	// カレントディレクトリ。
	char curdir[ PATHSIZE ];
} window = {};

int MouseOver( int x, int y, int w, int h )
{
	int mx, my;
	mx = MikanInput->GetMousePosX();
	my = MikanInput->GetMousePosY();
	return ( x < mx && mx < x + w && y < my && my < y + h );
}

int DrawScroll( int x, int y, int high, int all, int view, int begin )
{
	//int rbegin = begin;
	int length = 0;
	// スクロールバー。

	// 上。
	MikanDraw->DrawTexture( 0, x, y, 680, 40, 10, 10 );
	MikanDraw->DrawTexture( 0, x, y + 10, 680, 40, 10, 10, DRAW_UD );

	// 下。
	MikanDraw->DrawTexture( 0, x, y + high - 20, 680, 40, 10, 10, DRAW_LR );
	MikanDraw->DrawTexture( 0, x, y + high - 10, 680, 40, 10, 10, DRAW_LR | DRAW_UD );

	// 長さ。
	if(all <= view)
	{
		// 全件が表示件数以下の場合。
		length = high - 40;
	} else
	{
		length = ( high - 40 ) * view / all;
	}

	begin = ( high - 40 ) * begin / all;

	// バー本体。
	MikanDraw->DrawTexture( 0, x, y + 10 + begin, 680, 50, 10, 10 ); // 上。
	MikanDraw->DrawTextureScaling( 0, x, y + 20 + begin, 680, 60, 10, 10, 10, length, 0 ); // 中段。
	MikanDraw->DrawTexture( 0, x, y + 20 + begin + length, 680, 70, 10, 10 ); // 下。

	if(MikanInput->GetMouseNum( 0 ) == 1)
	{
		if(MouseOver( x, y, 10, 20 ))
		{
			return -1;
		} else if(MouseOver( x, y + high - 20, 10, 20 ))
		{
			return 1;
		}
	}

	//return rbegin;
	return 0;
}

// JSONのアップデート。
int JsonUpdate_( int *progress, int *progressmax )
{
	char *url;
	unsigned int size;

	if(conf.GetBaseURL() == NULL)
	{
		msg.SetMessage( "[FAIL] Failure json update." );
		return -1;
	}

	msg.SetMessage( "Begin download Gamelist." );

	size = strlen( conf.GetBaseURL() ) + 14;
	url = (char *)calloc( size, sizeof( char ) );
	sprintf_s( url, size, "%s/gamelist.cgi", conf.GetBaseURL() );
	MikanNet->HttpGet( url, "gamelist.json", progressmax, progress );
	free( url );

	msg.SetMessage( "End download." );

	msg.SetMessage( "Begin Analysis gamelist.json." );
	game.LoadFromJson( "gamelist.json" );
	msg.SetMessage( "End Analysis." );
	return 0;
}

int JsonUpdate( void )
{
	MikanSystem->Lock( 0 );
	++window.working;
	JsonUpdate_( &( window.progress ), &( window.progressmax ) );
	--window.working;
	MikanSystem->Unlock( 0 );
	return 0;
}

// ゲームのダウンロードとインストール。
int GameDownload_( void )
{
	char *adzip = NULL;
	unsigned int size;
	unsigned int gamenum = window.gameprogress.gamenum;

	msg.SetMessage( "Begin Download..." );
	// サイズ取得とかしてポインタ突っ込む。
	if(game.GetIdName( gamenum ))
	{
		// ベースアドレス/Game/数値_ID/
		size = strlen( conf.GetBaseURL() ) + 6 + 7 + 1 + strlen( game.GetIdName( gamenum ) ) + 1 + 1;
	} else
	{
		// ベースアドレス/Game/数値/
		size = strlen( conf.GetBaseURL() ) + 6 + 7 + 1 + 1;
	}
	window.gameprogress.basead = (char *)calloc( size, sizeof( char ) );
	if(game.GetIdName( gamenum ))
	{
		sprintf_s( window.gameprogress.basead, size, "%s/Game/%d_%s/", conf.GetBaseURL(), game.GetGameNumber( gamenum ), game.GetIdName( gamenum ) );
	} else
	{
		sprintf_s( window.gameprogress.basead, size, "%s/Game/%d/", conf.GetBaseURL(), game.GetGameNumber( gamenum ) );
	}

	// ZIPアドレスの作成。
	// ベースアドレスバージョン.zip
	size = strlen( window.gameprogress.basead ) + 7 + 4 + 1;
	adzip = (char *)calloc( size, sizeof( char ) );
	sprintf_s( adzip, size, "%s%d.zip", window.gameprogress.basead, game.GetGameVersion( gamenum ) );

	msg.SetMessageFormat( "Address: %s", adzip );
	// ダウンロード。
	MikanNet->HttpGet( adzip, ZIPFILE, game.GetProgressMax(), game.GetProgressNow() );//&(window.gameprogress.max), &(window.gameprogress.now) );

	free( adzip );

	return 0;
}

int GameInstall_( void )
{
	char *ad_base = NULL, *adconf = NULL, *adss = NULL, *dir = NULL;
	unsigned int size;
	unsigned int gamenum = window.gameprogress.gamenum;

	// サイズ取得とかしてポインタ突っ込む。
	// インストール(ZIP解凍するだけ)。
	game.SetProgressMax( FileNumsInZip( ZIPFILE ) );
	UnzipFile( ZIPFILE, ".", game.GetProgressMax() );//ZipNums( ZIPFILE );

	// カレントディレクトリ移動
	if(game.GetIdName( gamenum ))
	{
		// Game/数値_idname/
		size = strlen( "./Game/" ) + 7 + 1 + strlen( game.GetIdName( gamenum ) ) + 1 + 1;
	} else
	{
		// Game/数値_idname/
		size = strlen( "./Game/" ) + 7 + 1 + 1;
	}
	dir = (char *)calloc( size, sizeof( char ) );
	if(game.GetIdName( gamenum ))
	{
		sprintf_s( dir, size, "./Game/%d_%s/", game.GetGameNumber( gamenum ), game.GetIdName( gamenum ) );
	} else
	{
		sprintf_s( dir, size, "./Game/%d/", game.GetGameNumber( gamenum ) );
	}
	SetCurrentDirectory( dir );

	game.SetProgressMode( 0 );

	// 設定アドレスの作成。
	// ベースアドレスdata.ini
	size = strlen( window.gameprogress.basead ) + 8 + 1;
	adconf = (char *)calloc( size, sizeof( char ) );
	sprintf_s( adconf, size, "%sdata.ini", window.gameprogress.basead );
	MikanNet->HttpGet( adconf, "data.ini", game.GetProgressMax(), game.GetProgressNow() );

	// スクリーンショットアドレスの作成。
	// ベースアドレスss.png
	size = strlen( window.gameprogress.basead ) + 6 + 1;
	adss = (char *)calloc( size, sizeof( char ) );
	sprintf_s( adss, size, "%sss.png", window.gameprogress.basead );
	MikanNet->HttpGet( adss, "ss.png", game.GetProgressMax(), game.GetProgressNow() );

	SetCurrentDirectory( window.curdir );

	free( dir );
	free( window.gameprogress.basead );
	free( adconf );
	free( adss );

	return 0;
}

int GameUpdate( void )
{
	int gamemax;
	unsigned int i;

	if(conf.GetBaseURL() == NULL)
	{
		msg.SetMessage( "[FAIL] Failure game update." );
		return -1;
	}

	MikanSystem->Lock( 0 );
	++window.working;

	// ゲーム最大値取得。
	gamemax = 0;
	for(i = 0 ; i < game.GetGameMax() ; ++i)
	{
		if(game.IsChecked( i ))
		{
			++gamemax;
		}
	}

	for(i = 0 ; i < game.GetGameMax() ; ++i)
	{
		if(game.IsChecked( i ))
		{
			// 下準備。
			window.gameprogress.gamenum = i;
			window.gameprogress.now = 0;
			window.gameprogress.max = 0;
			window.gameprogress.basead = NULL;
			game.SetTarget( i );
			// ゲームダウンロード。
			game.SetProgressMode( 0 );
			GameDownload_();
			// ゲームインストール。
			game.SetProgressMode( 1 );
			GameInstall_();
			// チェックを外す。
			game.Unckeck( i );
		}
	}
	game.SetTarget();

	// 再度情報を更新。
	game.LoadFromDirectory( conf.GetGameDirectory() );

	--window.working;
	MikanSystem->Unlock( 0 );

	return 0;
}

// スレッドで実行。
int FirstLoad( void )
{
	MikanSystem->Lock( 0 );
	++window.working;

	window.progressmax = 100;
	window.progress = 0;
	// 設定ファイルを読む。
	conf.Load( "./config.ini" );
	window.progress = 10;

	// 最大カテゴリ数の登録。
	game.Init( conf.GetCategoryMax() );

	// ディレクトリ内のゲーム情報を取得。
	game.LoadFromDirectory( conf.GetGameDirectory() );
	window.progress = 40;

	// JSONダウンロード。
	JsonUpdate_( NULL, NULL );

	window.progress = 70;

	// JSON解析。
	msg.SetMessage( "Begin analysis gamelist.json" );

	window.progress = 100;

	msg.SetMessage( "FirstLoad() end." );

	--window.working;
	MikanSystem->Unlock( 0 );
	return 0;
}

// ウィンドウの移動。
int MoveWindow( void )
{
	int my;
	my = MikanInput->GetMousePosY();
	if(MikanInput->GetMouseNum( 0 ) == 1 && my < 40)
	{
		window.wx = MikanInput->GetMousePosX();
		window.wy = MikanInput->GetMousePosY();
		window.drg = 1;
	} else if(window.drg && MikanInput->GetMouseNum( 0 ) == -1)
	{
		window.drg = 0;
	} else if(window.drg)
	{
		window.wx = MikanWindow->GetPositionX() - ( window.wx - MikanInput->GetMousePosX() );
		window.wy = MikanWindow->GetPositionY() - ( window.wy - MikanInput->GetMousePosY() );
		MikanWindow->SetPositionXY( window.wx, window.wy );
		window.wx = MikanInput->GetMousePosX();
		window.wy = MikanInput->GetMousePosY();
	}
	return 0;
}

// ウィンドウの上。
int DrawHeader( void )
{
	//int mx, my;
	int csize;
	int x_close, x_config, x_json, x_game, x_download;
	const char *msg = NULL;

	//mx = MikanInput->GetMousePosX();
	//my = MikanInput->GetMousePosY();

	x_close = 640 - ( 20 / 2 + 10 );
	x_json = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 );
	x_download = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 ) * 2;

	x_config = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 ) * 4;
	x_game = 640 - ( 20 / 2 + 10 ) - ( 20 / 2 + 40 ) * 5;

	if(MikanInput->GetMousePosY() < 40)
	{
		// 終了ボタン。
		if(MouseOver( x_close - 15, 5, 30, 30 )) //x_close - 15 < mx && mx < x_close + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.close < 10)
			{
				++window.close;
			}
			msg = "Exit";
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				return 1;
			}
		} else if(window.close)
		{
			--window.close;
		}

		// 設定ボタン。
		if(MouseOver( x_config - 15, 5, 30, 30 )) //x_config - 15 < mx && mx < x_config + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.config < 10)
			{
				++window.config;
			}
			msg = "Config";
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				window.mode = MODE_CONFIG;
			}
		} else if(window.config)
		{
			--window.config;
		}

		// JSONアップデートボタン。
		if(MouseOver( x_json - 15, 5, 30, 30 )) //x_json - 15 < mx && mx < x_json + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.jsondownload < 10)
			{
				++window.jsondownload;
			}
			msg = "Database update";
			if(MikanInput->GetMouseNum( 0 ) == -1 && MikanSystem->GetThreadHandle( 0 ) == NULL)
			{
				MikanSystem->RunThread( 0, JsonUpdate );
			}
		} else if(window.jsondownload)
		{
			--window.jsondownload;
		}

		// JSONアップデートボタン。
		if(MouseOver( x_download - 15, 5, 30, 30 )) //x_json - 15 < mx && mx < x_json + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.download < 10)
			{
				++window.download;
			}
			msg = "Download & Install";
			if(MikanInput->GetMouseNum( 0 ) == -1 && MikanSystem->GetThreadHandle( 0 ) == NULL)
			{
				MikanSystem->RunThread( 0, GameUpdate );
			}
		} else if(window.download)
		{
			--window.download;
		}

		// ゲーム一覧ボタン。
		if(MouseOver( x_game - 15, 5, 30, 30 )) //x_json - 15 < mx && mx < x_json + 15 && 5 < my && my < 30 + 5 )
		{
			if(window.game < 10)
			{
				++window.game;
			}
			msg = "Game List";
			if(MikanInput->GetMouseNum( 0 ) == -1)
			{
				window.mode = MODE_DEFAULT;
			}
		} else if(window.game)
		{
			--window.game;
		}
	} else
	{
		if(0 < window.close)
		{
			--window.close;
		}
		if(0 < window.config)
		{
			--window.config;
		}
		if(0 < window.jsondownload)
		{
			--window.jsondownload;
		}//window.mode != MODE_CONFIG
		if(0 < window.download)
		{
			--window.download;
		}
		if(0 < window.game)
		{
			--window.game;
		}
	}

	// プログレスバー。
	if(window.progress > window.progressmax)
	{
		window.progress = window.progressmax;
	}
	if(window.progressmax > 0)
	{
		MikanDraw->DrawTexture( 0, 0, 30, 640 + 50, 40, 105 * window.progress / window.progressmax, 10 );
		MikanDraw->SetAlpha( 0, 255 * abs( window.timer % 121 - 60 ) / 60 );
		MikanDraw->DrawTexture( 0, 0, 30, 640 + 50, 50, 105 * window.progress / window.progressmax, 10 );
		MikanDraw->SetColor( 0, 0xFFFFFFFF );
	}
	if(window.progress >= window.progressmax)
	{
		window.progress = window.progressmax = 0;
	}

	// 閉じる。
	csize = 30 + (int)( 10.0 * window.close / 10.0 );
	MikanDraw->DrawTextureScalingC( 0, x_close, 15 + 5, 640, 0, 40, 40, csize, csize, 0 );

	// コンフィグ。
	if(window.mode == MODE_CONFIG)
	{
		MikanDraw->DrawTexture( 0, x_config - 20, 0, 840, 40, 40, 40 );
		MikanDraw->DrawTextureC( 0, x_config, 15 + 5, 640 + 80, 0, 40, 40 );
		window.config = 10;
	} else
	{
		csize = 30 + (int)( 10.0 * window.config / 10.0 );
		MikanDraw->DrawTextureScalingC( 0, x_config, 15 + 5, 640 + 80, 0, 40, 40, csize, csize, 0 );
	}

	// `ゲーム一覧。。
	if(window.mode == MODE_DEFAULT)
	{
		MikanDraw->DrawTexture( 0, x_game - 20, 0, 840, 40, 40, 40 );
		MikanDraw->DrawTextureC( 0, x_game, 15 + 5, 640 + 160, 0, 40, 40 );
		window.game = 10;
	} else
	{
		csize = 30 + (int)( 10.0 * window.game / 10.0 );
		MikanDraw->DrawTextureScalingC( 0, x_game, 15 + 5, 640 + 160, 0, 40, 40, csize, csize, 0 );
	}

	// JSONダウンロード。
	csize = 30 + (int)( 10.0 * window.jsondownload / 10.0 );
	MikanDraw->DrawTextureScalingC( 0, x_json, 15 + 5, 640 + 40, 0, 40, 40, csize, csize, 0 );

	// ゲームダウンロード。
	csize = 30 + (int)( 10.0 * window.download / 10.0 );
	MikanDraw->DrawTextureScalingC( 0, x_download, 15 + 5, 640 + 120, 0, 40, 40, csize, csize, 0 );

	// メニュー選択時の説明。
	if(msg)
	{
		MikanDraw->Printf( 1, 110, 30, msg );
	}
	return 0;
}

HINSTANCE LoadUnzipDLL( const char *dll )
{
	HINSTANCE hdll;
	if(( hdll = LoadLibrary( dll ) ) == NULL)
	{
		return NULL;
	}
	FileNumsInZip = ( int( *)( const char * ) )GetProcAddress( hdll, "ZipNums" );
	UnzipFile = ( int( *)( const char *, const char *, int * ) )GetProcAddress( hdll, "Unzip" );
	return hdll;
}

void SystemInit( void )
{
	hdll = LoadUnzipDLL( "Unzip.dll" );

	_MikanWindow->SetWindowSize( 640, 480 );
	_MikanWindow->SetWindow( WT_NOFRAME );

	MikanWindow->SetWindowName( "TenLAN Game Downloader" );
	MikanWindow->SetWindowIcon( "GAME_ICON" );

	GetCurrentDirectory( PATHSIZE, window.curdir );

	unsigned long vol;
	// test sound
	unsigned int devmax = mixerGetNumDevs();
	unsigned int i;
	MIXERCAPS mixercaps;
	HMIXER hmixer;
	MIXERLINE mixerline = {};
	MIXERLINECONTROLS controls = {};
	MIXERCONTROL control = {};
	MIXERCONTROLDETAILS details = {};
	MIXERCONTROLDETAILS_UNSIGNED *detailsus;
	MMRESULT mres;
	for(i = 0 ; i < devmax ; ++i)
	{
		mixerGetDevCaps( i, &mixercaps, sizeof( MIXERCAPS ) );
		mixercaps.szPname;// デバイスの名前。
		mixercaps.cDestinations;// ラインの数。
	}
	if(0 < devmax && ( mres = mixerOpen( &hmixer, 0, 0, 0, MIXER_OBJECTF_MIXER ) ) == MMSYSERR_NOERROR)
	{
		mixerline.cbStruct = sizeof( MIXERLINE );
		//mixerline.dwDestination = 0;
		mixerline.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
		if(( mres = mixerGetLineInfo( (HMIXEROBJ)( hmixer ), &mixerline, MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			MIXERR_INVALLINE;
			MMSYSERR_BADDEVICEID;
			MMSYSERR_INVALFLAG;
			if(mres == MMSYSERR_INVALHANDLE)
			{
				int b = 0;
			}
			MMSYSERR_INVALPARAM;
			MMSYSERR_NODRIVER;
			int a = 0;
		}

		controls.cbStruct = sizeof( MIXERLINECONTROLS );
		controls.dwLineID = mixerline.dwLineID;
		controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
		controls.cControls = 1;
		controls.cbmxctrl = sizeof( MIXERCONTROL );
		controls.pamxctrl = &control;
		if(( mres = mixerGetLineControls( (HMIXEROBJ)( hmixer ), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			mres = 0;
		}


		details.cbStruct = sizeof( MIXERCONTROLDETAILS );
		details.cbDetails = sizeof( MIXERCONTROLDETAILS_UNSIGNED );
		//if ( control.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE )
		//{
		details.cMultipleItems = 0;//control.cMultipleItems;
		//}
		//if ( control.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM )
		//{
		details.cChannels = 1;
		//} else
		//{
		//  details.cChannels = mixerline.cChannels;
		//}
		details.dwControlID = control.dwControlID;
		details.paDetails = (MIXERCONTROLDETAILS_UNSIGNED *)calloc( details.cChannels, sizeof( MIXERCONTROLDETAILS_UNSIGNED ) );

		if(( mres = mixerGetControlDetails( (HMIXEROBJ)( hmixer ), &details, MIXER_GETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			mres = 0;
		}

		control.Bounds.dwMinimum;
		control.Bounds.dwMaximum;

		unsigned int a = ( (MIXERCONTROLDETAILS_UNSIGNED *)details.paDetails )->dwValue;
		( (MIXERCONTROLDETAILS_UNSIGNED *)details.paDetails )->dwValue = control.Bounds.dwMaximum / 3;
		if(( mres = mixerSetControlDetails( (HMIXEROBJ)( hmixer ), &details, MIXER_SETCONTROLDETAILSF_VALUE | MIXER_OBJECTF_HMIXER ) ) != MMSYSERR_NOERROR)
		{
			mres = 0;
		}
		free( details.paDetails );
		mixerClose( hmixer );
	}
	//vol = auxGetNumDevs();
	//auxGetVolume( 0, &vol );
	//auxSetVolume( 0, 0xFFFF );
}

void UserInit( void )
{
	window.width = 640;//MikanDraw->GetScreenWidth();
	window.height = 480;//MikanDraw->GetScreenHeight();

	// システムフォントの設定。
	MikanDraw->LoadFontFile( NULL, "SYSFONT" );
	MikanDraw->CreateFont( 0, "MigMix 1M", 40, 0xFFFFFFFF );
	MikanDraw->CreateFont( 2, "MigMix 1M", 20, 0xFFFFFFFF );

	// ドットフォントの設定。
	MikanDraw->LoadFontFile( NULL, "DOTFONT" );
	MikanDraw->CreateFont( 1, "美咲ゴシック", 8, 0xFFFFFFFF );

	// システムテクスチャの準備。
	MikanDraw->CreateTexture( 0, NULL, "SYSIMG", TRC_NONE );

	// テスト用システムサウンドの準備。
	MikanSound->Load( 0, NULL, "SYSSOUND" );

	//MikanSound->Play( 0, true );

	// メッセージ処理クラスの初期化。
	msg.Init();
	// 表示エリアの設定。
	msg.SetArea( 0, 380, 640, 100 );

	game.SetArea( 0, 40, 340, 340 );

	MikanSystem->CreateLock( 0 );
	// 別スレッドで読み込み。
	// メッセージの操作と描画は同期を取ってるので大丈夫。
	MikanSystem->RunThread( 0, FirstLoad );

	//MikanSystem->SetInactiveWindow( 0 );
}

int MainLoop( void )
{
	int ret;

	//画面の初期化
	MikanDraw->ClearScreen();

	// 背景の描画。
	MikanDraw->DrawTexture( 0, 0, 0, 0, 0, 640, 480 );

	switch(window.mode)
	{
	case MODE_CONFIG: // コンフィグ。
		conf.Draw();
		break;
	default: // ゲームリスト描画。
		game.Draw( ( window.working == 0 ) );
		break;
	}

	// メッセージ描画。
	msg.Draw();

	// ウィンドウ移動。
	MoveWindow();

	// ウィンドウの上。
	ret = DrawHeader();

	// 待機中。
	if(window.working)
	{
		MikanDraw->DrawTextureRotationC( 0, window.width / 2, window.height / 2, 640, 80, 100, 100, MIKAN_PI * ( window.timer % 100 ) / 100.0 );
	}
	++window.timer;
	return ( MikanInput->GetKeyNum( K_ESC ) == 1 ) || ret;
}

void CleanUp( void )
{
	FreeLibrary( hdll );
	msg.Term();
}
